#include "size_map.h"
#include "st_common.h"

SizeMap::SizeMap()
{

}

void SizeMap::Dump()
{
    infor_tlog("SizeMap::Dump begin!");
    for (size_t i = 0; i < kNumClasses; ++i)
    {
        infor_tlog("index<%llu> class to size<%llu>  num to move<%d> class to page<%llu>.",
                   i, class_to_size_[i], num_objects_to_move_[i], class_to_pages_[i]);
    }
    infor_tlog("SizeMap::Dump end!");
}

size_t SizeMap::NumMoveSize(size_t size)
{
    if (size == 0)
    {
        return 0;
    }

    // 按照64KB去除以size, 得到一个每次要移动的数量.也就是说大部分时候尽量一次移动64KB
    size_t num = static_cast<size_t>(64.0f * 1024.0f / size);
    if (num < 2)
    {
        num = 2;
    }

    if (num > kDefaultTransferNumObjects)
    {
        num = kDefaultTransferNumObjects;
    }
    return num;
}

s32 SizeMap::Init()
{
    s32 sc = 1;
    u32 alignment = kAlignment;

    // 每次步进aligment个单位,alignment会随着size不断的增加
    for (size_t size = kAlignment; size <= kMaxSize; size += alignment)
    {
        alignment = AlignmentForSize(size);
        size_t block_to_move = NumMoveSize(size) / 4;
        size_t psize = 0;
        do
        {
            psize += kPageSize;
            // 分配剩余的空间如果大于1/8, 就不断的累加，直到小于1/8为止
            while ((psize % size) > (psize >> 3))
            {
                psize += kPageSize;
            }
        } while ((psize / size) < (block_to_move));

        // 页面数对齐到2的N次方，方便buddy system分配
        s32 page_order = log2_ceil(psize);
        if (page_order < 16)
        {
            page_order = 16;
        }
        const size_t need_pages = 1ull<<(page_order - kPageShift);
        //const size_t  need_pages = psize >> kPageShift;

        // 如果和前1个需要的页面一致, 在不增加碎片的情况下，直接把前一个size略过
        if (sc > 1 && need_pages == class_to_pages_[sc - 1])
        {
            const size_t alloc_objs = (need_pages << kPageShift) / size;
            const size_t prev_objs = (class_to_pages_[sc - 1] << kPageShift) / class_to_size_[sc - 1];
            if (alloc_objs == prev_objs)
            {
                class_to_size_[sc - 1] = size;
                continue;
            }
        }
        class_to_pages_[sc] = need_pages;
        class_to_size_[sc] = size;
        ++sc;
    }

    if (sc != kNumClasses)
    {
        error_tlog("sc is <%d>, kNumClass<%llu>.", sc, kNumClasses);
        return -2;
    }

    s32 next_size = 0;
    for (size_t cl = 1; cl < kNumClasses; ++cl)
    {
        const s32 max_size_in_class = class_to_size_[cl];
        for (s32 s = next_size; s <= max_size_in_class; s += kAlignment)
        {
            class_array_[ClassArrayIndex(s)] = cl;
        }
        next_size = max_size_in_class + kAlignment;
    }

    // 这个地方加个check, 防止前面初始化不正确
    for (size_t size = 0; size <= kMaxSize;)
    {
        const s32 sc = SizeClass(size);
        if (sc <= 0 || (size_t) sc >= kNumClasses)
        {
            error_tlog("bad size class sc<%d>, size<%llu>.", sc, size);
            return -3;
        }

        if (sc > 1 && size <= class_to_size_[sc - 1])
        {
            error_tlog("size not in right sc, sc<%d>, size<%llu>.", sc, size);
            return -3;
        }
        size_t sc_size = class_to_size_[sc];
        if (size > sc_size || sc_size == 0)
        {
            error_tlog(" sc<%d>, size<%llu> more than sc size<%llu>.", sc, size, sc_size);
            return -4;
        }
        if (size <= kMaxSmallSize)
        {
            size += 8;
        }
        else
        {
            size += 128;
        }
    }

    for (size_t cl = 1; cl < kNumClasses; ++cl)
    {
        size_t size = ByteSizeForClass(cl);
        num_objects_to_move_[cl] = NumMoveSize(size);
    }
    return 0;
}

static s32 LogFloor(size_t n)
{
    s32 log = 0;
    for (int i = 4; i >= 0; --i)
    {
        s32 shift = (1 << i);
        size_t x = n >> shift;
        if (x != 0)
        {
            n = x;
            log += shift;
        }
    }
    return log;
}

u32 AlignmentForSize(size_t size)
{
    u32 alignment = kAlignment;
    if (size > kMaxSize) // 大于256k, 按照页尺寸来对齐
    {
        alignment = kPageSize;
    }
    else if (size >= 128)
    {
        alignment = (1 << LogFloor(size)) / 8;
    }
    else if (size >= kMinAlign)
    {
        alignment = kMinAlign;
    }

    // 最大对其就是按照页来对齐
    if (alignment > kPageSize)
    {
        alignment = kPageSize;
    }
    return alignment;
}
