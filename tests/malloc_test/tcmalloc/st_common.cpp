#include "st_common.h"

#include <stddef.h>




void* MetaDataChunkAlloc(size_t bytes)
{    
    if (bytes < kPageSize)
    {
        error_tlog("alloc bytes<%llu> less than page size.", (unsigned long long)bytes);
        return NULL;
    }

    int order = log2_ceil(bytes);
    void* ptr = STAllocPages(order);
    if (NULL == ptr)
    {
        error_tlog("alloc pages failed, order<%d>.", order);
        return NULL;
    }

    return ptr;
}

int STSetPageOpPtrs(AllocPagesPtr alloc_ptr, FreePagesPtr free_ptr)
{
    return 0;
}

void* STAllocPages(int order)
{
    unsigned long long req_size = 1ULL << order;
#ifdef WIN32
    char* addr = (char*)_aligned_malloc(req_size, kPageSize);
#else
    char* addr = (char*)aligned_alloc(kPageSize, req_size);
#endif // WIN32
    if (addr == NULL)
    {
        return NULL;
    }
    return addr;
}

int STFreePages(void* ptr, size_t size)
{
#ifdef WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif // WIN32
    (void)size;
    return 0;
}
