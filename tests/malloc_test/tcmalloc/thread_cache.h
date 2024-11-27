#pragma once

#include "free_list.h"
#include "st_common.h"

class FreeList;

class ThreadCache
{
public:
    ThreadCache();
    ~ThreadCache();

public:
    int Init();
    void CleanUp();

    int freelist_length(int cl) const
    {
        return list_[cl].length();
    }
    size_t size() { return size_; }
    void* Allocate(SizeClass cl, size_t size);
    int Deallocate(void* ptr, SizeClass cl);

private:
    // 从Central取一大块对象列表，然后返回一个给调用者
    void* FetchFromCentralCache(SizeClass cl, size_t size);
    // 把缓存返回Central Cache
    int ReleaseToCentralCache(FreeList* src, SizeClass cl, int num);

    // 清理多余的内存
    int Scavenge();
    // 链表过长，做清理
    int ReduceTooLongList(FreeList* list, size_t cl);

    // 增加thread cache缓存总上限大小
    int IncreaseCacheLimit();

private:
    size_t size_;     // 空闲位分配出去的数据总大小
    size_t max_size_; // 允许的最大大小
    FreeList list_[kNumClasses];
};
