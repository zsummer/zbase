#pragma once

#include "page_map.h"
#include "page_map_cache.h"
#include "st_common.h"

using PageMapType = PageMap3<kAddressBits - kPageShift>;
using PageCacheType = PageMapCache<kAddressBits - kPageShift, unsigned long long>;
struct Span;

//
struct PageHeapStats
{
    unsigned long long page_count;
    PageHeapStats()
    {
        page_count = 0;
    }
};

class PageHeap
{
public:
    PageHeap();
    ~PageHeap();

public:
    int Init(PageMapType::NodeAllocator* node_allocator, PageMapType::LeafAllocator* leaf_allocator);
    Span* New(Length num);
    int Delete(Span* span);
    // 给span 指定他所分配小对象对应的size class
    int RegisterSizeClass(Span* span, SizeClass sc);

    Span* GetDescriptor(PageID id)
    {
        return reinterpret_cast<Span*>(page_map_.get(id));
    }

    size_t GetSizeClassIfCached(PageID id)
    {
        return page_cache_.GetOrDefault(id, 0);
    }

    void CacheSizeClass(PageID id, SizeClass cl)
    {
        page_cache_.Put(id, cl);
    }
    // 记录span信息
    int RecordSpan(Span* span);

    PageHeapStats& stats() { return stats_; }

private:
    PageMapType page_map_;
    PageCacheType page_cache_;
    PageHeapStats stats_;
};
