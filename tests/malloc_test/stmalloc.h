#pragma once

#include "span.h"
#include "size_map.h"
#include "central_free_list.h"
#include "thread_cache.h"
#include "page_heap.h"
#include "meta_allocator.h"
#include "span.h"

#include "st_common.h"

class CentralFreeList;
struct Span;

using SpanAllocator = MetaAllocator<Span>;
using PageMapNodeAllocator = PageMapType::NodeAllocator;
using PageMapLeafAllocator = PageMapType::LeafAllocator;

// 内存分配器统计信息
struct STMallInfo
{
	u64 total_alloc_size;
	u64 meta_alloc_size; // meta系统分配的内存数目
	STMallInfo()
	{
		total_alloc_size = 0;
		meta_alloc_size = 0;
	}
};

// 单线程内存分配器
class STMalloc
{
public:
	STMalloc();
	~STMalloc();

public:
	s32 Init(AllocPagesPtr alloc_ptr, FreePagesPtr free_ptr);
	s32 Resume(AllocPagesPtr alloc_ptr, FreePagesPtr free_ptr);

	void Dump();

public:
	bool is_inited() { return is_inited_; }
	SizeMap& size_map() { return size_map_; }
	ThreadCache& thread_cache() { return thread_cache_; }
	CentralFreeList* center_lists() { return central_lists_; }

	PageHeap& page_heap() { return page_heap_; }
	STMallInfo& mall_info() { return mall_info_; }
	SpanAllocator& span_allocator() { return span_allocator_; }
	// page map分配节点所用的分配器
	PageMapNodeAllocator& page_map_node_allocator() { return page_map_node_allocator_; }
	PageMapLeafAllocator& page_map_leaf_allocator() { return page_map_leaf_allocator_; }

private:
	bool is_inited_; // 是否完成了初始化
	SizeMap size_map_;
	CentralFreeList central_lists_[kNumClasses];
	ThreadCache thread_cache_;
	PageHeap page_heap_;
	STMallInfo mall_info_;

	// metadata 分配器，只分配不释放
	SpanAllocator span_allocator_;
	PageMapNodeAllocator  page_map_node_allocator_;
	PageMapLeafAllocator page_map_leaf_allocator_;
};

extern STMalloc* g_st_malloc;

void* st_malloc(size_t size);
void st_free(void* ptr);


