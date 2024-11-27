#pragma once
#include "fn_log.h"






typedef unsigned long long PageID;
typedef unsigned long long Length;
typedef unsigned int SizeClass;

#define  error_tlog LOGFMTE
#define  debug_tlog LOGFMTD
#define  infor_tlog LOGFMTI

#ifdef WIN32
/*
* left to right scan
* num:<1>  return 0
* num:<2>  return 1
* num:<3>  return 1
* num:<4>  return 2
* num:<0>  return (unsigned int)-1
*/

inline unsigned int test_first_bit_index(unsigned long long num)
{
    DWORD index = (DWORD)-1;
    _BitScanReverse64(&index, num);
    return (unsigned int)index;
}
/*
* right to left scan
*/

inline unsigned int test_last_bit_index(unsigned long long num)
{
    DWORD index = -1;
    _BitScanForward64(&index, num);
    return (unsigned int)index;
}

#else
#define test_first_bit_index(num) ((unsigned int)(sizeof(unsigned long long) * 8 - __builtin_clzll((unsigned long long)num) - 1))
#define test_last_bit_index(num) ((unsigned int)(__builtin_ctzll((unsigned long long)num)))
#endif


// 计算一个数以2为底的对数, 如果value不是2^N次方，那么会向上取整
inline int log2_ceil(unsigned long long value)
{
    int n = test_first_bit_index(value);
    unsigned long long mask = (1ull << n) - 1;
    unsigned long long remainder = (mask & value) + mask;
    return n + ((remainder >> n) & 1);
}



static const  size_t kPageShift = 13;
// 每页的尺寸
static const size_t kPageSize = 1 << kPageShift;


static const  size_t kNumClasses = 65;
// 对齐可以根据需要修改，在内存和速度之间做折中
static const size_t kMinAlign = 16;

// thread cache拥有的最大内存大小
static const size_t kMaxThreadCacheSize = 4 << 20;

static const size_t kMaxSize = 256 * 1024;
static const size_t kAlignment = 8;
static const size_t kLargeSizeClass = 0;

// 1M以下的页都保持精确的列表,暂时不用，我们不做page的缓存机制
static const size_t kMaxPages = 1 << (20 - kPageShift);
// 所有的thread cache内存总量
static const size_t kDefaultOverallThreadCacheSize = 8u * kMaxThreadCacheSize;
// 每个thread cache最小尺寸
static const  size_t kMinThreadCacheSize = kMaxSize * 2;

// 可以容忍几次length 超过max_length, 超过这个次数后，就会收缩free list
static const unsigned int kMaxOverages = 3;

// 每个FreeList允许的最大长度
static const int kMaxDynamicFreeListLength = 8192;

static const Length kMaxValidPages = (~static_cast<Length>(0)) >> kPageShift;

// 64位系统下面实际使用的只有48位，所以不需要那么多位来做PageMap
static const int kAddressBits = (sizeof(void*) < 8 ? (8 * sizeof(void*)) : 48);
// 一个线程可以从另外一个线程获取的内存数量
static const size_t kStealAmount = 1 << 16;

// 最大只可能取到2的48次方的地址。大于他的地址肯定是无效的
static const unsigned long long kMaxValidAddr = 1ull << 48;

// 获取指定的byte数量到所需的page数量的转换
inline Length GetPagesNum(size_t bytes)
{
    return ((bytes >> kPageShift) + (bytes & (kPageSize - 1)) > 0 ? 1 : 0);
}

// 每次在thread cache 和center cache之间传递的对象数量
static const int kDefaultTransferNumObjects = 1024;


// 分配用来分配meta data的chunk, 必须是页数的整数倍数
// 这里为了和buddy system配合，最少不能分配少于buddy system最小元素大小
void* MetaDataChunkAlloc(size_t bytes);

typedef void* (*AllocPagesPtr)(int order);
typedef int (*FreePagesPtr)(void* ptr, size_t size);

// 设置分配页面和销毁页面的函数指针。
int STSetPageOpPtrs(AllocPagesPtr alloc_ptr, FreePagesPtr free_ptr);

void* STAllocPages(int order);
int STFreePages(void* ptr, size_t size);

