#include "st_common.h"

#include <stddef.h>


static AllocPagesPtr s_alloc_pages_ptr = NULL;
static FreePagesPtr s_free_pages_ptr = NULL;

void* MetaDataChunkAlloc(size_t bytes)
{	
	if (bytes < kPageSize)
	{
		error_tlog("alloc bytes<%llu> less than page size.", bytes);
		return NULL;
	}

	s32 order = log2_ceil(bytes);
	void* ptr = STAllocPages(order);
	if (NULL == ptr)
	{
		error_tlog("alloc pages failed, order<%d>.", order);
		return NULL;
	}

	return ptr;
}

s32 STSetPageOpPtrs(AllocPagesPtr alloc_ptr, FreePagesPtr free_ptr)
{
	return 0;
}

void* STAllocPages(s32 order)
{
	return new char[1ull << order];
}

s32 STFreePages(void* ptr, size_t size)
{
	delete ptr;
	return 0;
}
