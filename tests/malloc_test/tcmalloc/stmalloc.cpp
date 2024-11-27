#include <stddef.h>

#include "st_common.h"
#include "stmalloc.h"
#include "span.h"

STMalloc* g_st_malloc = NULL;

STMalloc::STMalloc()
{
    is_inited_ = false;
}

STMalloc::~STMalloc()
{

}

int STMalloc::Init(AllocPagesPtr alloc_ptr, FreePagesPtr free_ptr)
{


    int ret = 0;
    ret = STSetPageOpPtrs(alloc_ptr, free_ptr);
    if (ret != 0)
    {
        error_tlog("STSetPageOpPtrs failed, ret<%d>.", ret);
        return ret;
    }

    g_st_malloc = this;
    ret = size_map_.Init();
    if (ret != 0)
    {
        error_tlog("size map init failed.ret<%d>.", ret);
        return ret;
    }
    //size_map_.Dump();

    for (SizeClass i = 0; i < kNumClasses; ++i)
    {
        ret = central_lists_[i].Init(i);
        if (ret != 0)
        {
            error_tlog("central list<%u> init failed, ret<%d>.", i, ret);
            return ret;
        }
    }

    ret = thread_cache_.Init();
    if (ret != 0)
    {
        error_tlog("thread cache init failed, ret<%d>.", ret);
        return ret;
    }

    ret = span_allocator_.Init();
    if (ret != 0)
    {
        error_tlog("span_allocator failed, ret<%d>.", ret);
        return ret;
    }

    ret = page_map_node_allocator_.Init();
    if (ret != 0)
    {
        error_tlog("page_map_node_allocator failed, ret<%d>.", ret);
        return ret;
    }
    
    ret = page_map_leaf_allocator_.Init();
    if (ret != 0)
    {
        error_tlog("page_map_leaf_allocator failed, ret<%d>.", ret);
        return ret;
    }

    ret = page_heap_.Init(&page_map_node_allocator_,  &page_map_leaf_allocator_);
    if (ret != 0)
    {
        error_tlog("page heap init failed, ret<%d>.", ret);
        return ret;
    }

    is_inited_ = true;
    return 0;
}

int STMalloc::Resume(AllocPagesPtr alloc_ptr, FreePagesPtr free_ptr)
{
    if (NULL == alloc_ptr || NULL == free_ptr)
    {
        error_tlog("invalid args.");
        return -1;
    }

    int ret = 0;
    ret = STSetPageOpPtrs(alloc_ptr, free_ptr);
    if (ret != 0)
    {
        error_tlog("STSetPageOpPtrs failed, ret<%d>.", ret);
        return ret;
    }

    g_st_malloc = this;
    return 0;
}

void STMalloc::Dump()
{

}

// 用来分配小于等于256KB的小对象
static void* st_alloc_do_malloc_small(size_t size)
{
    if (size > kMaxSize || size == 0)
    {
        error_tlog("invalid size<%llu>.", (unsigned long long)size);
        return NULL;
    }


    SizeClass cl = g_st_malloc->size_map().SizeClass((int)size);
    size = g_st_malloc->size_map().ByteSizeForClass(cl);
    void* ptr = g_st_malloc->thread_cache().Allocate(cl, size);
    if (NULL == ptr)
    {
        error_tlog("Allocate failed, size<%llu>, cl<%u>.", (unsigned long long)size, cl);
        return NULL;
    }

    return ptr;
}
// 用来分配大于256KB的对象
static void* st_alloc_do_malloc_pages(size_t size)
{
    if (size <= kMaxSize)
    {
        error_tlog("invalid size<%llu>.", (unsigned long long)size);
        return NULL;
    }


    void* ptr = NULL;
    Length num_pages = GetPagesNum(size);
    size = num_pages << kPageShift;

    Span* span = g_st_malloc->page_heap().New(num_pages);
    if (NULL == span)
    {
        error_tlog("NewSpan failed, page num<%llu>.", num_pages);
        return NULL;
    }

    g_st_malloc->page_heap().CacheSizeClass(span->start, 0);
    ptr = reinterpret_cast<void*>(span->start << kPageShift);
    return ptr;
}


void* st_malloc(size_t size)
{
    if (NULL == g_st_malloc || !g_st_malloc->is_inited())
    {
        error_tlog("stmalloc not init.");
        return NULL;
    }
    if (size == 0)
    {
        error_tlog("invalid args.");
        return NULL;
    }

    void* ptr = NULL;
    if (size <= kMaxSize)
    {
        ptr = st_alloc_do_malloc_small(size);
        if (NULL == ptr)
        {
            error_tlog("st_alloc_do_malloc_small failed, size<%llu>.", (unsigned long long)size);
        }
    }
    else
    {
        ptr = st_alloc_do_malloc_pages(size);
        if (NULL == ptr)
        {
            error_tlog("st_alloc_do_malloc_pages failed, size<%llu>.", (unsigned long long)size);
        }

    }
    return ptr;
}

void st_free(void* ptr)
{
    if (NULL == g_st_malloc || !g_st_malloc->is_inited())
    {
        error_tlog("stmalloc not init.");
        return;
    }
    if (NULL == ptr)
    {
        error_tlog("invaid args.");
        return;
    }

    int ret = 0;
    Span* span = NULL;
    PageID id = reinterpret_cast<unsigned long long>(ptr) >> kPageShift;
    SizeClass cl = (SizeClass)g_st_malloc->page_heap().GetSizeClassIfCached((SizeClass)id);
    if (cl == 0)
    {
        span = g_st_malloc->page_heap().GetDescriptor(id);
        if (NULL == span)
        {
            error_tlog("GetDescriptor failed, page id<%llu>.", id);
            return;
        }
        cl = span->sizeclass;
        g_st_malloc->page_heap().CacheSizeClass(id, cl);
    }
    
    if (cl != 0)
    {
        ret = g_st_malloc->thread_cache().Deallocate(ptr, cl);
        if (ret != 0)
        {
            error_tlog("Deallocate failed, ptr<%p>, cl<%u>.", ptr, cl);
        }
    }
    else
    {
        if ((reinterpret_cast<unsigned long long>(ptr) % kPageSize) != 0)
        {
            error_tlog("ptr <%p> not align to page size.", ptr);
            return;
        }
        if ((span->start<<kPageShift) != (unsigned long long)ptr)
        {
            error_tlog("ptr <%p> not equal start addr<%llu>.", ptr, (unsigned long long)(void*)(span->start << kPageShift));
            return;
        }
        g_st_malloc->page_heap().Delete(span);
    }

}
