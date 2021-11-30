#include <stddef.h>

#include "st_common.h"
#include "page_heap.h"
#include "span.h"

PageHeap::PageHeap():page_cache_(0)
{

}

PageHeap::~PageHeap()
{

}

s32 PageHeap::Init(PageMapType::NodeAllocator* node_allocator, PageMapType::LeafAllocator* leaf_allocator)
{
	if (NULL == node_allocator || NULL == leaf_allocator)
	{
		error_tlog("invalid args.");
		return -1;
	}

	s32 ret = 0;
	ret = page_map_.Init(node_allocator, leaf_allocator);
	if (ret != 0)
	{
		error_tlog("page map init failed,ret<%d>.", ret);
		return ret;
	}

	return ret;
}

Span* PageHeap::New(Length num)
{
	if (num <= 0)
	{
		error_tlog("invalid args.");
		return NULL;
	}

	s32 ret = 0;
	s32 order = log2_ceil(num) + kPageShift;
	void* ptr = STAllocPages(order);
	if (NULL == ptr)
	{
		error_tlog("STAllocPages failed.");
		return NULL;
	}

	Span* span = NewSpan(reinterpret_cast<u64>(ptr) >> kPageShift,  num);
	if (NULL == span)
	{
		error_tlog("NewSpan failed.");
		ret = STFreePages(ptr,  1ull<<order);
		if (ret != 0)
		{
			error_tlog("STFreePages failed, ret<%d>.", ret);
		}
		return NULL;
	}
	span->sizeclass = 0;
	page_map_.Reserve((u64)ptr >> kPageShift, num);

	return span;
}

s32 PageHeap::Delete(Span* span)
{
	if (NULL == span)
	{
		error_tlog("invalid args.");
		return -1;
	}

	STFreePages((void*)(span->start << kPageShift),  span->page_num<<kPageShift);
	DeleteSpan(span);

	return 0;
}

s32 PageHeap::RegisterSizeClass(Span* span, SizeClass sc)
{
	if (NULL == span || sc == 0)
	{
		error_tlog("invalid args.");
		return -1;
	}

	span->sizeclass = sc;
	for (Length i = 0; i < span->page_num; ++i)
	{
		page_map_.set(span->start + i, span);
	}
	return 0;
}

s32 PageHeap::RecordSpan(Span* span)
{
	if (NULL == span)
	{
		error_tlog("invalid args.");
		return -1;
	}

	page_map_.set(span->start, span);
	if (span->page_num > 1)
	{
		page_map_.set(span->start + span->page_num - 1, span);
	}
	return 0;
}

