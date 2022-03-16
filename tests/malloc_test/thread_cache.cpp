#include "thread_cache.h"

#include <stddef.h>
#include <algorithm>

#include "stmalloc.h"
#include "central_free_list.h"
#include "free_list.h"
#include "link_list.h"

ThreadCache::ThreadCache()
{
	size_ = 0;
	max_size_ = 0;
}

ThreadCache::~ThreadCache()
{

}

s32 ThreadCache::Init()
{
	s32 ret = 0;
	size_ = 0;
	max_size_ = 0;
	ret = IncreaseCacheLimit();
	if (ret != 0)
	{
		error_tlog("IncreaseCacheLimit failed, ret<%d>.", ret);
		return ret;
	}

	for (SizeClass cl = 0; cl < kNumClasses; ++cl)
	{
		list_[cl].Init();
	}

	return 0;
}

void ThreadCache::CleanUp()
{

}

void* ThreadCache::Allocate(SizeClass cl, size_t size)
{
	if (size > kMaxSize || cl == 0 || cl >= kNumClasses)
	{
		error_tlog("invalid args.");
		return NULL;
	}

	if (g_st_malloc->size_map().ByteSizeForClass(cl) != size)
	{
		error_tlog("size <%llu> not equal class<%u>.", size, cl);
		return NULL;
	}

	void* ptr = NULL;
	FreeList* list = &list_[cl];
	if (list->empty())
	{
		ptr = FetchFromCentralCache(cl, size);
		if (NULL == ptr)
		{
			error_tlog("FetchFromCentralCache failed,size <%llu>, class<%u>.", size, cl);
			return NULL;
		}
	}
	else
	{
		ptr = list->Pop();
		if (NULL == ptr)
		{
			error_tlog("list cl<%u> pop failed.", cl);
			return NULL;
		}
		size_ -= size;
	}

	return ptr;
}

s32 ThreadCache::Deallocate(void* ptr, SizeClass cl)
{
	if (NULL == ptr || cl == 0 || cl >= kNumClasses)
	{
		error_tlog("invalid args.");
		return -1;
	}

	s32 ret = 0;
	FreeList* list = &list_[cl];
	size_ += g_st_malloc->size_map().ByteSizeForClass(cl);
	list->Push(ptr);

	bool is_exceed = list->max_length() < list->length();
	if (is_exceed)
	{
		ret = ReduceTooLongList(list, cl);
		if (ret != 0)
		{
			error_tlog("ReduceTooLongList failed, ret<%d>.", ret);
		}
	}
	if (size_ >= max_size_)
	{
		ret = Scavenge();
		if (ret != 0)
		{
			error_tlog("Scavenge failed, ret<%d>.", ret);
		}
	}

	return 0;
}

void* ThreadCache::FetchFromCentralCache(SizeClass cl, size_t byte_size)
{
	if (cl == 0 || cl >= kNumClasses)
	{
		error_tlog("invalid args");
		return NULL;
	}

	FreeList* list = &list_[cl];
	if (!list->empty())
	{
		error_tlog("must be have internal error.");
		return NULL;
	}

	s32 batch_size = g_st_malloc->size_map().num_objects_to_move(cl);
	s32 num_to_move = std::min<s32>(list->max_length(), batch_size);

	void *start = NULL, *end = NULL;
	s32 fetch_count = g_st_malloc->center_lists()[cl].RemoveRange(
		&start, &end, num_to_move);

	if (fetch_count <= 0 || start == NULL)
	{
		error_tlog("RemoveRange failed, fetch count is zero.");
		return NULL;
	}

	// �ѳ��˵�һ��Ԫ��֮�������Ԫ�ؼ�������
	--fetch_count;
	size_ += byte_size * fetch_count;
	list->PushRange(fetch_count, SLL_Next(start), end);
	// �������������Ĳ��֣���������ͨ�����ó�ʼmax_lengthֱ�������ⲿ���߼�
	if (list->max_length() < batch_size)
	{
		list->set_max_length(list->max_length() + 1);
	}
	else
	{
		s32 new_length = std::min<s32>(list->max_length() + batch_size, kMaxDynamicFreeListLength);
		new_length -= new_length % batch_size;
		list->set_max_length(new_length);
	}

	return start;
}

s32 ThreadCache::ReleaseToCentralCache(FreeList* src, SizeClass cl, s32 num)
{
	if (NULL == src || cl == 0 || cl >= kNumClasses)
	{
		error_tlog("invalid args.");
		return -1;
	}

	s32 ret = 0;
	if (num > src->length())
	{
		num = src->length();
	}

	void *tail = NULL, *head = NULL;
	size_t delta_bytes = num * g_st_malloc->size_map().ByteSizeForClass(cl);
	s32 batch_size = g_st_malloc->size_map().num_objects_to_move(cl);
	while (num > batch_size)
	{
		src->PopRange(batch_size, &head, &tail);
		ret = g_st_malloc->center_lists()[cl].InsertRange(head, tail, batch_size);
		if (ret != 0)
		{
			error_tlog("InsertRange failed, cl<%u>, ret<%d>.", cl, ret);
			return -2;
		}

		num -= batch_size;
	}

	// ǰ�汣֤num ����batch size, ���ʣ��Ŀ϶�����0��
	src->PopRange(num, &head, &tail);
	ret = g_st_malloc->center_lists()[cl].InsertRange(head, tail, num);
	if (ret != 0)
	{
		error_tlog("InsertRange failed, cl<%u>, ret<%d>.", cl, ret);
		return -3;
	}

	size_ -= delta_bytes;

	return 0;
}

s32 ThreadCache::Scavenge()
{
	s32 ret = 0;
	for (size_t cl = 1; cl < kNumClasses; ++cl)
	{
		FreeList* list = &list_[cl];
		s32 lowmark = list->lowwater_mark();
		if (lowmark > 0)
		{
			s32 drop = (lowmark > 1) ? lowmark / 2 : 1;
			ret = ReleaseToCentralCache(list, cl, drop);
			if (ret != 0)
			{
				error_tlog("ReleaseToCentralCache failed, ret<%d>.", ret);
			}
			s32 batch_size = g_st_malloc->size_map().num_objects_to_move(cl);
			if (list->max_length() > batch_size)
			{
				list->set_max_length(std::max<s32>(list->max_length() - batch_size, batch_size));
			}
		}
		list->clear_lowwater_mark();
	}

	return 0;
}

s32 ThreadCache::ReduceTooLongList(FreeList* list, size_t cl)
{
	if (NULL == list || cl == 0 || cl >= kNumClasses)
	{
		error_tlog("invalid args.");
		return -1;
	}

	s32 ret = 0;
	s32 batch_size = g_st_malloc->size_map().num_objects_to_move(cl);
	ret = ReleaseToCentralCache(list, cl, batch_size);
	if (ret != 0)
	{
		error_tlog("ReleaseToCentralCache failed, ret<%d>.", ret);
		return ret;
	}

	if (list->max_length() < batch_size)
	{
		list->set_max_length(list->max_length() + 1);
	}
	else if (list->max_length() > batch_size)
	{
		list->set_length_overages(list->length_overages() + 1);
		if (list->length_overages() > kMaxOverages)
		{
			list->set_max_length(list->max_length() - batch_size);
			list->set_length_overages(0);
		}
	}

	return 0;
}

s32 ThreadCache::IncreaseCacheLimit()
{
	if (max_size_ == 0)
	{
		max_size_ = kDefaultOverallThreadCacheSize;
	}

	return 0;
}
