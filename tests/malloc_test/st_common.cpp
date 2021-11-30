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
	if (true)
	{
		s32 align_size = 1u << (order + 1);
		align_size += 16;
		char* p = new char[align_size];
		u64 addr = (u64)(p);
		addr &= ~((1ull << order) - 1);
		addr += (1ull << order);
		*(u64*)(addr - 16) = (u64)p;
		*(u64*)(addr - 8) = 1ull << (order);
		return (void*)addr;
	}


	if (true)
	{
		s32 align_size = 1u << (order);
		align_size += 1u << kPageShift;
		align_size += 16;
		char* p = new char[align_size];
		u64 addr = (u64)(p);
		addr &= ~((1ull << kPageShift) - 1);
		addr += (1ull << kPageShift);
		*(u64*)(addr - 16) = (u64)p;
		*(u64*)(addr - 8) = 1ull << (order);
		return (void*)addr;
	}
	return NULL;
}

s32 STFreePages(void* ptr, size_t size)
{
	u64* addr = (u64*)ptr;
	void* old_addr = (void*)*(addr - 2);
	delete old_addr;
	//delete ptr;
	return 0;
}
