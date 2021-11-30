#pragma once
#include "link_list.h"
#include "st_common.h"

class FreeList
{
public:
	FreeList()
	{
		list_ = NULL;
		length_ = 0;
		lowater_ = 0;
		max_length_ = kDefaultTransferNumObjects; // 直接提升到128， 避免慢分配
		length_overages_ = 0;
	}
	void Init()
	{
		list_ = NULL;
		length_ = 0;
		lowater_ = 0;
		max_length_ = kDefaultTransferNumObjects; // 直接提升到128， 避免慢分配
		length_overages_ = 0;
	}

	s32 length() const { return length_; }
	s32 max_length() { return max_length_; }
	void set_max_length(s32 new_max)
	{
		max_length_ = new_max;
	}

	u32 length_overages() const
	{
		return length_overages_;
	}

	void set_length_overages(s32 new_count)
	{
		length_overages_ = new_count;
	}

	bool empty() const
	{
		return (list_ == NULL);
	}

	u32 lowwater_mark() { return lowater_; }
	void clear_lowwater_mark() { lowater_ = length_; }

	void Push(void* ptr)
	{
		SLL_Push(&list_, ptr);
		length_++;
	}

	void* Pop()
	{
		if (list_ == NULL)
		{
			return NULL;
		}
		length_--;
		if (length_ < lowater_)
		{
			lowater_ = length_;
		}
		return SLL_Pop(&list_);
	}

	void* Next()
	{
		return SLL_Next(&list_);
	}

	void PushRange(s32 N, void* start, void* end)
	{
		SLL_PushRange(&list_, start, end);
		length_ += N;
	}

	void PopRange(int num, void** start, void ** end)
	{
		SLL_PopRange(&list_, num, start, end);
		//ASSERT(length_ >= N);
		length_ -= num;
		if (length_ < lowater_)
		{
			lowater_ = length_;
		}
	}

private:
	void* list_; // 链表头
	s32 length_; // 当前长度
	s32 max_length_; // 动态最大长度
	s32 lowater_; // 低水线长度
	s32 length_overages_; // 统计在释放时length_ > max_length_ 的次数
};
