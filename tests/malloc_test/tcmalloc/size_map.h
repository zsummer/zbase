#pragma once


#include "st_common.h"

// 对于大的分配尺寸， 用的大的内存分配对齐来减少size class 的数目
unsigned int AlignmentForSize(size_t size);

// size-class的映射关系信息
class SizeMap
{
public:
    SizeMap();
    // 初始化所有的映射关系
    int Init();

    void Dump();

    // 根据size 获取他对应的外部Class索引
    inline int SizeClass(int size)
    {
        return class_array_[ClassArrayIndex(size)];
    }
    inline size_t ByteSizeForClass(size_t cl)
    {
        return class_to_size_[cl];
    }
    inline size_t class_to_pages(size_t cl)
    {
        return class_to_pages_[cl];
    }
    inline int num_objects_to_move(size_t cl)
    {
        return num_objects_to_move_[cl];
    }

private:
    int num_objects_to_move_[kNumClasses];
    // 小尺寸对象的最大大小
    static const int kMaxSmallSize = 1024;
    static const size_t kClassArraySize = ((kMaxSize + 127 + (120 << 7)) >> 7) + 1;

    unsigned char class_array_[kClassArraySize];

    static inline size_t ClassArrayIndex(int s)
    {
        /*if (0 > s || s > kMaxSize)
        {
            return 0;
        }
        */

        if (s <= kMaxSmallSize) // 1024一下。按照8字节对齐
        {
            return (static_cast<unsigned int>(s) + 7) >> 3;
        }
        else // 1024 到256k之间，按照128对齐
        {
            return (static_cast<unsigned int>(s) + 127 + (120 << 7)) >> 7;
        }
    }

    // 计算某个大小一次批量移动的数量
    size_t NumMoveSize(size_t size);

    // 从size class 到该class 可以存储的最大尺寸映射
    size_t class_to_size_[kNumClasses];

    // 从size class 到该class 每次需要分配的Page数量的映射
    size_t class_to_pages_[kNumClasses];
};
