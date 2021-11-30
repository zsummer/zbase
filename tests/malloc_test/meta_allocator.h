#pragma once

#include "st_common.h"
#include "st_error_define.h"
// 用来分配一些管理对象,如span
template<class T>
class MetaAllocator
{
public:
	MetaAllocator()
	{
		inuse_ = 0;
		free_area_ = NULL;
		free_avail_ = 0;
		free_list_ = NULL;
	}

	s32 Init()
	{
		inuse_ = 0;
		free_area_ = NULL;
		free_avail_ = 0;
		free_list_ = NULL;
		// 先分配一个，把空间开辟出来,避免空间碎片化
		T* ptr = New();
		if (NULL == ptr)
		{
			return ST_ALLOC_SHM_META_ALLOC_FAILED;
		}
		Delete(ptr);
		return 0;
	}
	T* New()
	{
		void* result;
		if (free_list_ != NULL)
		{
			result = free_list_;
			free_list_ = *(reinterpret_cast<void**>(result));
		}
		else
		{
			// 空间不足，需要重新分配一块
			if (free_avail_ < sizeof(T))
			{
				free_area_ = reinterpret_cast<char*>(MetaDataChunkAlloc(kAllocIncrement));
				if (free_area_ == NULL)
				{
					return NULL;
				}
				free_avail_ = kAllocIncrement;
			}
			result = free_area_;
			free_area_ += sizeof(T);
			free_avail_ -= sizeof(T);
		}
		inuse_++;
		return reinterpret_cast<T*>(result);
	}

	void Delete(T* p)
	{
		if (NULL == p)
		{
			return;
		}

		*(reinterpret_cast<void**>(p)) = free_list_;
		free_list_ = p;
		inuse_--;
	}

	// 正在使用的obj数目
	s32 inuse() const { return inuse_; }

private:
	static const s32 kAllocIncrement = 128 << 10; //128k
	char* free_area_; // free area, 用来切割出对象
	size_t free_avail_;
	void *free_list_; // 已经切割出的对象free list
	int inuse_; // 已经分配，但没有释放的对象数量
};