#include <string.h>
#include <algorithm>

#include "st_common.h"
#include "central_free_list.h"
#include "stmalloc.h"
#include "link_list.h"
#include "page_heap.h"

static Span* MapObjectToSpan(void* object)
{
	const PageID p = reinterpret_cast<u64>(object) >> kPageShift;
	Span* span = g_st_malloc->page_heap().GetDescriptor(p);
	return span;
}

CentralFreeList::CentralFreeList()
{
    cl_ = 0;
    memset(&empty_, 0, sizeof(empty_));
    memset(&noempty_, 0, sizeof(noempty_));
	SpanDoubleListInit(&empty_);
	SpanDoubleListInit(&noempty_);
	num_spans_ = 0;
	free_obj_num_ = 0;
	used_slots_ = 0;

	max_cache_size_ = kMaxNumTransferEntries;
	cache_size_ = 16;

    memset(tc_slots_, 0, sizeof(tc_slots_));
}

CentralFreeList::~CentralFreeList()
{

}

s32 CentralFreeList::Init(SizeClass cl)
{
	cl_ = cl;
	SpanDoubleListInit(&empty_);
	SpanDoubleListInit(&noempty_);
	num_spans_ = 0;
	free_obj_num_ = 0;

	max_cache_size_ = kMaxNumTransferEntries;
	cache_size_ = 16;

	if (cl > 0)
	{
		s32 bytes = g_st_malloc->size_map().ByteSizeForClass(cl);
		s32 objs_to_move = g_st_malloc->size_map().num_objects_to_move(cl);

		max_cache_size_ = (std::min)(max_cache_size_,
			(std::max)(1, (1024 * 1024) / (bytes * objs_to_move)));
		cache_size_ = (std::min)(cache_size_, max_cache_size_);
	}

	used_slots_ = 0;
	return 0;
}

s32 CentralFreeList::InsertRange(void* start, void* end, s32 num)
{
	if (NULL == start || NULL == end || 0 >= num)
	{
		error_tlog("invalid args.");
		return -1;
	};

	if (num == g_st_malloc->size_map().num_objects_to_move(cl_))
	{
		// 能够批量返回tranfer cache,就尽量直接返回transfer cache
		bool is_ok = TryMakeCacheSpace();
		if (is_ok)
		{
			s32 slot = used_slots_++;
			TCEntry* entry = &tc_slots_[slot];
			entry->head = start;
			entry->tail = end;
			return 0;
		}
	}

	s32 ret = ReleaseListToSpans(start);
	if (ret != 0)
	{
		error_tlog("ReleaseListToSpans failed, ret<%d>.", ret);
	}
	return ret;
}

s32 CentralFreeList::RemoveRange(void** start, void** end, s32 num)
{
	if (NULL == start || NULL == end || 0 >= num)
	{
		error_tlog("invalid args.");
		return 0;
	};

	// 申请的数量正好和TCEntry缓存的一致，而且tc entry缓存有元素，直接返回
	if (num == g_st_malloc->size_map().num_objects_to_move(cl_) && used_slots_ > 0)
	{
		s32 slot = --used_slots_;
		TCEntry* entry = &tc_slots_[slot];
		*start = entry->head;
		*end = entry->tail;
		return num;
	}

	s32 ret = 0;
	*start = NULL;
	*end = NULL;

	s32 result = FetchFromOneSpanSafe(num, start, end);
	if (result != 0)
	{
		while (result < num)
		{
			s32 fetch_num = 0;
			void* head = NULL;
			void* tail = NULL;
			// 取不到更多的对象就返回
			fetch_num = FetchFromOneSpan(num - result, &head, &tail);
			if (fetch_num == 0)
			{
				break;
			}
			result += fetch_num;
			SLL_PushRange(start, head, tail);
		}
	}

	return result;
}

bool CentralFreeList::TryMakeCacheSpace()
{
	// 还有可用slots
	if (used_slots_ < cache_size_)
	{
		return true;
	}
	// 已经达到最大限制，不能再加了
	if (cache_size_ >= max_cache_size_)
	{
		return false;
	}

	// 尝试去释放其他class 的内存, 这个地方服务器使用，可以直接让cache_size = max_cache_size
	// 服务器不差那点内存，反复释放，分配意义不大
	EvictRandomSizeClass(cl_, false);
	EvictRandomSizeClass(cl_, true);

	if (cache_size_ < max_cache_size_)
	{
		++cache_size_;
		return true;
	}
	return false;
}

s32 CentralFreeList::ShrinkCache(bool is_force)
{
	return 0;
}

u64 CentralFreeList::OverheadBytes() const
{
	if (cl_ == 0)
	{
		debug_tlog("cl 0 is invalid class.");
		return 0;
	}

	size_t pages = g_st_malloc->size_map().class_to_pages(cl_);
	size_t obj_size = g_st_malloc->size_map().ByteSizeForClass(cl_);
	size_t overhead_per_span = (pages * kPageSize) % obj_size;

	return num_spans_ * overhead_per_span;
}

s32 CentralFreeList::TCTotalLength() const
{
	return used_slots_ * g_st_malloc->size_map().num_objects_to_move(cl_);
}

bool CentralFreeList::EvictRandomSizeClass(SizeClass lock_cl, bool is_force)
{
	return true;
}

s32 CentralFreeList::FetchFromOneSpan(s32 num, void** start, void** end)
{
	if (NULL == start || NULL == end || 0 >= num)
	{
		error_tlog("invalid args.");
		return 0;
	};

	if (SpanDoubleListIsEmpty(&noempty_))
	{
		//debug_tlog("noempty list  is empty.");
		return 0;
	}

	Span* span = noempty_.next;
	if (NULL == span || NULL == span->objects)
	{
		error_tlog("invalid span data.");
		return 0;
	}

	s32 result = 0;
	void* prev = NULL, *curr = NULL;
	curr = span->objects;
	do
	{
		prev = curr;
		curr = *(reinterpret_cast<void**>(curr));
	} while (++result < num && curr != NULL);

	if (curr == NULL)
	{
		SpanDoubleListRemove(span);
		SpanDoubleListPrepend(&empty_, span);
	}

	// 直接从链上摘除一部分
	*start = span->objects;
	*end = prev;

	span->objects = curr;
	SLL_SetNext(*end, NULL);
	span->refcount += result;
	free_obj_num_ -= result;

	return result;
}

s32 CentralFreeList::FetchFromOneSpanSafe(s32 num, void** start, void** end)
{
	if (NULL == start || NULL == end || 0 >= num)
	{
		error_tlog("invalid args.");
		return 0;
	};

	s32 result = FetchFromOneSpan(num, start, end);
	if (result == 0)
	{
		s32 ret = Populate();
		if (ret != 0)
		{
			error_tlog("Populate failed, ret<%d>.", ret);
			return 0;
		}
		result = FetchFromOneSpan(num, start, end);
		return result;
	}

	return result;
}

s32 CentralFreeList::ReleaseListToSpans(void* start)
{
	if (NULL == start)
	{
		error_tlog("invalid args.");
		return -1;
	}

	s32 ret = 0;
	while (start)
	{
		void* next = SLL_Next(start);
		ret = ReleaseToSpans(start);
		if (ret != 0)
		{
			error_tlog("ReleaseToSpans failed, ret<%d>.", ret);
		}
		start = next;
	}

	return 0;
}

s32 CentralFreeList::ReleaseToSpans(void* object)
{
	if (NULL == object)
	{
		error_tlog("invalid args.");
		return -1;
	}

	s32 ret = 0;
	Span* span = MapObjectToSpan(object);
	if (NULL == span || span->refcount == 0)
	{
		error_tlog("invalid span.");
		return -2;
	}

	// span里之前没有元素了
	if (span->objects == NULL)
	{
		SpanDoubleListRemove(span);
		SpanDoubleListPrepend(&noempty_, span);
	}

	++free_obj_num_;
	--span->refcount;

	// span没有关联对象了，直接反还给page heap
	if (span->refcount == 0)
	{
		free_obj_num_ -= (span->page_num<<kPageShift) /
				(g_st_malloc->size_map().ByteSizeForClass(span->sizeclass));
		SpanDoubleListRemove(span);
		--num_spans_;

		ret = g_st_malloc->page_heap().Delete(span);
		if (ret != 0)
		{
			error_tlog("page heap Delete span failed, ret<%d>.", ret);
		}
	}
	else
	{
		// 直接挂在单向链表上面
		*(reinterpret_cast<void**>(object)) = span->objects;
		span->objects = object;
	}

	return 0;
}

s32 CentralFreeList::Populate()
{
	size_t pages = g_st_malloc->size_map().class_to_pages(cl_);
	if (0 == pages)
	{
		error_tlog("invalid args.");
		return -1;
	}

	s32 ret = 0;
	Span* span = NULL;
	span = g_st_malloc->page_heap().New(pages);
	if (NULL == span || span->page_num != pages)
	{
		error_tlog("New pages<%llu> span failed.", pages);
		return -2;
	}

	g_st_malloc->page_heap().RegisterSizeClass(span, cl_);
	for (size_t i = 0; i < pages; ++i)
	{
		g_st_malloc->page_heap().CacheSizeClass(span->start + i, cl_);
	}

	void** tail = &span->objects;
	char* ptr = reinterpret_cast<char*>(span->start << kPageShift);
	char* ptr_limit = ptr + (pages << kPageShift);
	const size_t size = g_st_malloc->size_map().ByteSizeForClass(cl_);
	s32 num = 0;
	while ((ptr + size) <= ptr_limit)
	{
		*tail = ptr;
		tail = reinterpret_cast<void**>(ptr);
		ptr += size;
		++num;
	}

	if (ptr > ptr_limit)
	{
		error_tlog("must be some error in assign pointer link list.");
		return -3;
	}

	*tail = NULL;
	span->refcount = 0;

	SpanDoubleListPrepend(&noempty_, span);
	++num_spans_;
	free_obj_num_ += num;

	return 0;
}
