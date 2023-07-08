#pragma once


#include "st_common.h"
#include "span.h"

class CentralFreeList
{
public:
	CentralFreeList();
	~CentralFreeList();

public:
	int Init(SizeClass cl);
	int InsertRange(void* start, void* end, int num);
	int RemoveRange(void** start, void** end, int num);

	// 返回从span中取得了多少对象
	int FetchFromOneSpan(int num, void** start, void** end);
	// 先尝试从span获取。span中不足，从page heap分配span,然后再获取
	int FetchFromOneSpanSafe(int num, void** start, void** end);

	int ReleaseListToSpans(void* start);
	int ReleaseToSpans(void* object);

	// 从page heap 中分配span,切分为cache对象
	int Populate();

	// 试图增大tc entry cache的数量
	bool TryMakeCacheSpace();
	// 收缩cache大小
	int ShrinkCache(bool is_force);

	// 统计所有碎片大小，数量的总和
	unsigned long long OverheadBytes() const;
	int TCTotalLength() const;

	unsigned long long free_obj_num() const { return free_obj_num_; }

	static bool EvictRandomSizeClass(SizeClass lock_cl, bool is_force);

public:
	struct TCEntry
	{
		void* head;
		void* tail;
	};

	// 最大允许缓存多少条链表
	static const int kMaxNumTransferEntries = 64;

private:
	SizeClass cl_;
	Span empty_; // 已经全部分配出去的span
	Span noempty_; // 部分分配出去的span, 还可以继续分配
	unsigned long long num_spans_; // empty + noempty的span总和
	TCEntry tc_slots_[kMaxNumTransferEntries];
	int used_slots_; // 使用的tc_slots数目
	int cache_size_; // 当前允许缓存slot大小
	int max_cache_size_; // 最大允许缓存slot大小
	unsigned long long free_obj_num_; // 空闲对象数量,包含span内的对象数量
};
