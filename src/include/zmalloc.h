
/*
* zmalloc License
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <vector>
#include <iostream>
#include <sstream>



#ifndef  ZMALLOC_H
#define ZMALLOC_H


namespace zsummer
{
    using s8 = char;
    using u8 = unsigned char;
    using s16 = short int;
    using u16 = unsigned short int;
    using s32 = int;
    using u32 = unsigned int;
    using s64 = long long;
    using u64 = unsigned long long;
    using f32 = float;
    using f64 = double;

    class zmalloc
    {
    public:
        using page_alloc_func = void* (*)(u64);
        using page_free_func = u64(*)(void*);
        static const u32 BINMAP_SIZE = (sizeof(u64) * 8U);
        static const u32 BITMAP_LEVEL = 2;
        static const u32 SALE_SYS_ALLOC_SIZE = (4 * 1024 * 1024);
    public:
        inline static zmalloc& instance();
        inline static zmalloc* instance_ptr();
        inline static void* default_page_alloc(u64 );
        inline static u64 default_page_free(void*);
        inline static void set_global(zmalloc* state);
        inline void set_page_callback(page_alloc_func page_alloc, page_free_func page_free);
        inline void* alloc_memory(u64 bytes);
        inline u64  free_memory(void* addr);

        inline void check_health();
        inline void clear_cache();
        inline const char* SummaryStatic();
        inline void Summary();

    public:
        struct chunk_type
        {
            u32 fence;
            u32 prev_size;
            u32 this_size;
            u16 flags;
            u16 bin_id;
        };

        struct free_chunk_type :public chunk_type
        {
            free_chunk_type* prev_node;
            free_chunk_type* next_node;
        };

        struct page_type
        {
            u32 page_size;
            u32 fence;
            u64 reserve;
            page_type* next;
            page_type* front;
        };
    private:
        inline free_chunk_type* alloc_page(u32 bytes, u32 flag);
        inline u64 free_page(page_type* page);
        inline void push_chunk(free_chunk_type* chunk, u32 bin_id);
        inline void push_small_chunk(free_chunk_type* chunk);
        inline void push_big_chunk(free_chunk_type* chunk);
        inline bool pick_chunk(free_chunk_type* chunk);
        inline free_chunk_type* exploit_new_chunk(free_chunk_type* devide_chunk, u32 new_chunk_size);
    public:
        u32 inited_;
        page_alloc_func page_alloc_;
        page_free_func page_free_;

        u32 max_reserve_page_count_;
        u64 req_total_count_;
        u64 free_total_count_;
        u64 req_total_bytes_;
        u64 alloc_total_bytes_;
        u64 free_total_bytes_;

        u64 sale_total_count_;
        u64 return_total_count_;

        u64 sale_cache_count_;
        u64 return_cache_count_;

        u64 sale_real_bytes_;
        u64 return_real_bytes_;


        u32 used_page_count_;
        page_type* used_page_list_;
        u32 reserve_page_count_;
        page_type* reserve_page_list_;
        u64 bitmap_[BITMAP_LEVEL];

        free_chunk_type* dv_[BITMAP_LEVEL];
        free_chunk_type bin_[BITMAP_LEVEL][BINMAP_SIZE];
        free_chunk_type bin_end_[BITMAP_LEVEL][BINMAP_SIZE];
        u64 alloc_counter_[BITMAP_LEVEL][BINMAP_SIZE];
        u64 free_counter_[BITMAP_LEVEL][BINMAP_SIZE];
    };

#define global_zmalloc(bytes) zmalloc::instance().alloc_memory(bytes)
#define global_zfree(addr) zmalloc::instance().free_memory(addr)



#ifdef WIN32
    /*
    * left to right scan
    * num:<1>  return 0
    * num:<2>  return 1
    * num:<3>  return 1
    * num:<4>  return 2
    * num:<0>  return (u32)-1
    */

    inline u32 zmalloc_first_bit_index(u64 num)
    {
        DWORD index = (DWORD)-1;
        _BitScanReverse64(&index, num);
        return (u32)index;
    }
    /*
    * right to left scan
    */

    inline u32 zmalloc_last_bit_index(u64 num)
    {
        DWORD index = -1;
        _BitScanForward64(&index, num);
        return (u32)index;
    }

#else
#define zmalloc_first_bit_index(num) ((u32)(sizeof(u64) * 8 - __builtin_clzll((u64)num) - 1))
#define zmalloc_last_bit_index(num) ((u32)(__builtin_ctzll((u64)num)))
#endif



    static const u32 ZMALLOC_FIRST_BIT_INDEX_TABLE[] =
    {
        /*
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
        */
        0,
        0, 1, 1, 2, 2, 2, 2, 3, 3, 3,  3,  3,  3,  3,  3,  4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  4,  4,  4,  4,  4,  5, //32
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5,  5,  5,  5,  5,  5,  5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5,  5,  5,  5,  5,  5,  6, //64
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  6,  6,  6,  6,  6,  6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  6,  6,  6,  6,  6,  6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  6,  6,  6,  6,  6,  6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  6,  6,  6,  6,  6,  7, //128
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7,  7,  7,  7,  7,  7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7,  7,  7,  7,  7,  7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7,  7,  7,  7,  7,  7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7,  7,  7,  7,  7,  7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7,  7,  7,  7,  7,  7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7,  7,  7,  7,  7,  7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7,  7,  7,  7,  7,  7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  7,  7,  7,  7,  7,//8, //256
    };
    static constexpr u32 ZMALLOC_FIRST_BIT_INDEX_TABLE_BYTES = sizeof(ZMALLOC_FIRST_BIT_INDEX_TABLE) / sizeof(u32);

#define first_bit_index(bytes) ( (bytes) < ZMALLOC_FIRST_BIT_INDEX_TABLE_BYTES ? ZMALLOC_FIRST_BIT_INDEX_TABLE[bytes] : zmalloc_first_bit_index(bytes) )





    template<class Integer>
    inline Integer zmalloc_fill_right(Integer num)
    {
        static_assert(std::is_same<Integer, u32>::value, "only support u32 type");
        num |= num >> 1U;
        num |= num >> 2U;
        num |= num >> 4U;
        num |= num >> 8U;
        num |= num >> 16U;
        return num;
    }

#define FirstBit001(num)   (zmalloc_fill_right((num) >> 1) + 1)
#define CeilFirstBit001(num)   (zmalloc_fill_right((num) -1) + 1) 
#define CeilFirstBit CeilFirstBit001   //next power of 2
#define FirstBit FirstBit001   //next power of 2
#define NextPowerOf2 CeilFirstBit
#define IsPowerOf2(num)  (!(num & (num-1)))

#define LastBit(x) ((x) & -(x))
    //#define LastBit(num)  (num &(~(num & (num-1))))

#define BSMax(x, y) ((x) > (y) ? (x) : (y))
#define BSMin(x, y) ((x) < (y) ? (x) : (y))




#define ShiftSize(shift) (1U << (shift))
#define ShiftSize64(shift) (1ULL << (shift))
#define ShiftRightMask(shift) (ShiftSize(shift) -1U)
#define ShiftRightMask64(shift) (ShiftSize64(shift) -1ULL)



#define AlignBytesSize(bytes, up) ( ( (bytes) + ((up) - 1U) ) & ~((up) - 1U) )  
#define IsAlignBytesSize(bytes, up) (!((bytes) & ((up) - 1U)))

    static_assert(AlignBytesSize(0, 4) == 0, "");
    static_assert(AlignBytesSize(1, 4) == 4, "");
    static_assert(AlignBytesSize(1, 4096) == 4096, "");
    static_assert(AlignBytesSize((1ULL << 50) + 1, (1ULL << 50)) == (1ULL << 50) * 2, "");
    static_assert(AlignBytesSize((1ULL << 50) * 2 + 1, (1ULL << 50)) == (1ULL << 50) * 3, "");

    static_assert(IsAlignBytesSize(0, 4), "");
    static_assert(!IsAlignBytesSize(1, 4), "");
    static_assert(IsAlignBytesSize(4, 4), "");




#define AlignUpUnitSize(bytes, shift) (((bytes) + ShiftRightMask64(shift)) >> (shift))
    static_assert(AlignUpUnitSize(0, 10) == 0, "");
    static_assert(AlignUpUnitSize(1, 10) == 1, "");
    static_assert(AlignUpUnitSize(1 << 10, 10) == 1, "");
    static_assert(AlignUpUnitSize((1 << 10) + 1, 10) == 2, "");
    static_assert(AlignUpUnitSize(1 << 10, 10) == 1, "");
    static_assert(AlignUpUnitSize((1ULL << 50) + 1, 50) == 2, "");

#define AlignObjSize(bytes) AlignBytesSize(bytes, sizeof(std::max_align_t)) 
    static_assert(AlignObjSize(1) == sizeof(std::max_align_t), "");
    static_assert(AlignObjSize(0) == 0, "");


#define MIN_PAGE_SHIFT 12






#define ThirdBitIndex(bytes) ((first_bit_index(bytes >> 10) + 10) - 2)
#define GeoPostPart(third_index, bytes) (((bytes) >> (third_index)) & 0x3)
#define GeoSequence(third_index, bytes) (((third_index) << 2) + GeoPostPart(third_index, bytes))
    static constexpr u32 geo_sequence_test_1 = GeoSequence(0, 7);
    static_assert(GeoSequence(0, 4) == 0, "");
    static_assert(GeoSequence(0, 5) == 1, "");
    static_assert(GeoSequence(0, 6) == 2, "");
    static_assert(GeoSequence(0, 7) == 3, "");

    static_assert(GeoSequence(1, 8) == 4, "");
    static_assert(GeoSequence(1, 9) == 4, "");
    static_assert(GeoSequence(1, 10) == 5, "");
    static_assert(GeoSequence(1, 11) == 5, "");
    static_assert(GeoSequence(1, 12) == 6, "");
    static_assert(GeoSequence(1, 13) == 6, "");
    static_assert(GeoSequence(1, 14) == 7, "");
    static_assert(GeoSequence(1, 15) == 7, "");
    static_assert(GeoSequence(2, 16) == 8, "");

    static_assert(GeoSequence(8, 1024) == 32, "");
    static_assert(GeoSequence(8, 1280) == 33, "");
    static_assert(GeoSequence(8, 1536) == 34, "");
    static_assert(GeoSequence(8, 1792) == 35, "");
    static_assert(GeoSequence(9, 2048) == 36, "");

#define GeoSequenceZip(third_index, bytes) (GeoSequence(third_index, bytes) - 32)
    static_assert(GeoSequenceZip(8, 1024) == 0, "");

#define BigPadIndex(bytes)   GeoSequenceZip(ThirdBitIndex(bytes), bytes) 

#define BigIndexSize(index )  ((((index + 32) & 0x3) | 0x4) << (((index +32) >> 2) ))
#define CeilGeoBytes(third_index, bytes) ((ShiftRightMask(third_index) + (bytes))& ~ShiftRightMask(third_index))
    static_assert(CeilGeoBytes(8, 1024) == 1024, "");
    static_assert(CeilGeoBytes(8, 1025) == 1280, "");
    static_assert(CeilGeoBytes(8, 1279) == 1280, "");
    static_assert(CeilGeoBytes(8, 1280) == 1280, "");
    static_assert(CeilGeoBytes(8, 1281) == 1536, "");
    static_assert(CeilGeoBytes(8, 1535) == 1536, "");

#define CeilThirdIndex(third_index, bytes) (bytes >= (1ULL << (third_index+3)) ? (third_index)+1 : (third_index))
    static_assert(CeilThirdIndex(8, 1024) == 8, "");
    static_assert(CeilThirdIndex(8, 1025) == 8, "");
    static_assert(CeilThirdIndex(8, 2047) == 8, "");
    static_assert(CeilThirdIndex(8, 2048) == 9, "");
    static_assert(CeilThirdIndex(8, 2049) == 9, "");


#if ZMALLOC_OPEN_CHECK
#define CHECK_STATE(state) DebugAssertAllpage_type(state)
#define CHECK_C(c) DebugAssertChunk(c)
#define CHECK_FC(c) DebugAssertFreeChunk(c)
#define CHECK_FCL(state, c) DebugAssertFreeChunkList(state, c)

#else
#define CHECK_STATE(state)  (void)(state)
#define CHECK_C(c) (void)(c)
#define CHECK_FC(c) (void)(c)
#define CHECK_FCL(state, c) (void)(c); (void)(state);
#endif


    using chunk_type = zmalloc::chunk_type;
    using free_chunk_type = zmalloc::free_chunk_type;
    using page_type = zmalloc::page_type;
    static const u32 BINMAP_SIZE = zmalloc::BINMAP_SIZE;
    static const u32 BITMAP_LEVEL = zmalloc::BITMAP_LEVEL;
    static const u32 SALE_SYS_ALLOC_SIZE = zmalloc::SALE_SYS_ALLOC_SIZE;
    static const u32 LEAST_ALIGN_SHIFT = 4U;

    enum ChunkFlags : u32
    {
        CHUNK_IS_BIG = 0x1,
        CHUNK_IS_IN_USED = 0x2,
        CHUNK_IS_DIRECT = 0x4,
    };
    static const u32 CHUNK_LEVEL_MASK = 0x01;
    static const u32 CHUNK_FENCE = 0xdeadbeaf;

    static const u32 CHUNK_PADDING_SIZE = sizeof(chunk_type);
    static const u32 CHUNK_FREE_SIZE = sizeof(free_chunk_type);


    static_assert(CHUNK_FREE_SIZE - CHUNK_PADDING_SIZE == sizeof(void*) * 2, "check memory layout.");
    static_assert(ShiftSize(LEAST_ALIGN_SHIFT) == sizeof(void*) * 2, "check memory layout.");
    static_assert(sizeof(chunk_type) == ShiftSize(LEAST_ALIGN_SHIFT), "payload addr in chunk must align the least align.");

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
    static_assert(offsetof(free_chunk_type, prev_node) == sizeof(chunk_type), "struct memory layout is impl-defined. so need this test.");
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#define c(p) ((chunk_type*)(p))
#define u(p) ((u64)(p))
#define fc(p) ((free_chunk_type*)(p))

#define UnsetShift(bitmap, shift)  ((bitmap) &= ~ShiftSize64(shift))
#define SetShift(bitmap, shift) ((bitmap) |= ShiftSize64(shift))
#define HasShift(bitmap, shift)  ((bitmap) & ShiftSize64(shift))

#define CUnsetBit(chunk, val)  ((chunk)->flags &= ~(val))
#define CSetBit(chunk, val) ((chunk)->flags |= (val))
#define CHasBit(chunk, val)  ((chunk)->flags &  (val))

#define InUse(chunk) CHasBit((chunk), CHUNK_IS_IN_USED)
#define IsDirect(chunk) CHasBit((chunk), CHUNK_IS_DIRECT)
#define IsGoodFence(chunk) ((chunk)->fence == CHUNK_FENCE)


#define Front(p)  fc(u(p)-c(p)->prev_size)
#define Next(p)  fc(u(p)+c(p)->this_size)
#define Level(p) (fc(p)->flags & CHUNK_LEVEL_MASK)


    zmalloc* g_zmalloc_state = NULL;


    zmalloc& zmalloc::instance()
    {
        return *g_zmalloc_state;
    }
    zmalloc* zmalloc::instance_ptr()
    {
        return g_zmalloc_state;
    }

    void* zmalloc::default_page_alloc(u64 req_size)
    {
        char* addr = (char*)malloc(req_size + 8);
        if (addr == NULL)
        {
            return NULL;
        }
        *((u64*)addr) = req_size;
        return addr + sizeof(u64);
   }

    u64 zmalloc::default_page_free(void* addr)
    {
        if (addr == NULL)
        {
            return 0;
        }
        char* req_addr = ((char*)addr) - sizeof(u64);
        u64 req_size = *((u64*)req_addr);
        if (req_size > 1*1024*1024*1024*1024ULL) //1 T
        {
            //will memory leak;  
            return 0;
        }
        free(req_addr);
        return req_size;
    }


    void zmalloc::set_global(zmalloc* state)
    {
        g_zmalloc_state = state;
    }

    void zmalloc::set_page_callback(page_alloc_func page_alloc, page_free_func page_free)
    {
        page_alloc_ = page_alloc;
        page_free_ = page_free;
    }
    /*
    static void DebugAssertChunk(chunk_type* c);
    static void DebugAssertAllpage_type(zmalloc& zstate);
    static void DebugAssertBitmap(zmalloc& zstate);
    static void DebugAssertFreeChunk(free_chunk_type* c);
    static void DebugAssertFreeChunkList(zmalloc& zstate, free_chunk_type* c);
    */

    static_assert(sizeof(page_type) == ShiftSize(LEAST_ALIGN_SHIFT + 1), "page align");
    static const u32 FIELD_SIZE = sizeof(page_type);

#define Thispage_type(firstp) ((page_type*)(u(firstp) - FIELD_SIZE - sizeof(free_chunk_type)))
#define FirstChunk(page) c( u(page)+FIELD_SIZE + sizeof(free_chunk_type))
#define SegHeadChunk(page) c( u(page)+FIELD_SIZE)
    //static_assert(Thispage_type(FirstChunk(NULL)) == NULL, "");


    static const u32  SMALL_GRADE_SHIFT = 4U;
    static const u32  SMALL_GRADE_SIZE = ShiftSize(SMALL_GRADE_SHIFT);
    static const u32  SMALL_GRADE_MASK = ShiftRightMask(SMALL_GRADE_SHIFT);
    static const u32  SMALL_LEAST_SIZE = SMALL_GRADE_SIZE + CHUNK_PADDING_SIZE;
    static const u32  SMALL_MAX_REQUEST = (BINMAP_SIZE << SMALL_GRADE_SHIFT);

    static const u32  BIG_MAX_REQUEST = 512 * 1024;


    static_assert(SMALL_LEAST_SIZE >= sizeof(free_chunk_type), "");
    static_assert(ShiftSize(LEAST_ALIGN_SHIFT) >= sizeof(void*) * 2, "");
    static_assert(SMALL_GRADE_SHIFT >= LEAST_ALIGN_SHIFT, "");
    static_assert(IsPowerOf2(SMALL_GRADE_SHIFT), "");
    static_assert(BINMAP_SIZE == sizeof(u64) * 8, "");
    static_assert(SALE_SYS_ALLOC_SIZE >= BIG_MAX_REQUEST + sizeof(page_type) + sizeof(chunk_type), "");
    static_assert(IsPowerOf2(SALE_SYS_ALLOC_SIZE), "");

#define AlignBytes(bytes, grade_shift)  (((bytes) + ShiftRightMask(grade_shift) ) & ~ShiftRightMask(grade_shift)) 
#define FloorGradeIndex(bytes, grade_shift)   ((bytes) >> (grade_shift))
#define BinIndex(bytes, grade_shift )  (FloorGradeIndex((bytes), (grade_shift)) -1)

    static_assert(AlignBytes(1, 4) == 16, "");
    static_assert(AlignBytes(16, 4) == 16, "");
    static_assert(AlignBytes(17, 4) == 32, "");






    
    free_chunk_type* zmalloc::alloc_page(u32 bytes, u32 flag)
    {
        bytes += sizeof(page_type) + CHUNK_PADDING_SIZE + sizeof(free_chunk_type) * 2;
        static_assert(BIG_MAX_REQUEST + CHUNK_PADDING_SIZE + sizeof(free_chunk_type) * 2 + sizeof(page_type) <= SALE_SYS_ALLOC_SIZE, "");
        static_assert(SALE_SYS_ALLOC_SIZE >= 1024 * 4, "");
        static_assert(IsPowerOf2(SALE_SYS_ALLOC_SIZE), "");
        sale_total_count_++;
        bool dirct = flag & CHUNK_IS_DIRECT;
        if (!dirct)
        {
            bytes = SALE_SYS_ALLOC_SIZE;
        }
        else
        {
            bytes = NextPowerOf2(bytes);
        }
        page_type* page = NULL;
        if (!dirct && reserve_page_count_ > 0)
        {
            page = reserve_page_list_;
            reserve_page_count_--;
            if (page->next)
            {
                reserve_page_list_ = page->next;
                page->next->front = NULL;
            }
            else
            {
                reserve_page_list_ = NULL;
            }
            sale_cache_count_++;
            CHECK_STATE(*this);
        }
        else
        {
            if (page_alloc_)
            {
                page = (page_type*)page_alloc_(bytes);
            }
            else
            {
                page = (page_type*)default_page_alloc(bytes);
            }
            
            sale_real_bytes_ += bytes;
        }
        if (page == NULL)
        {
            return NULL;
        }
        page->page_size = bytes;
        page->fence = CHUNK_FENCE;
        page->next = used_page_list_;
        used_page_list_ = page;
        page->front = NULL;
        if (page->next)
        {
            page->next->front = page;
        }
        used_page_count_++;
        free_chunk_type* head_chunk = fc(SegHeadChunk(page));
        head_chunk->flags = flag | CHUNK_IS_IN_USED;
        head_chunk->prev_size = 0;
        head_chunk->bin_id = 0;
        head_chunk->fence = CHUNK_FENCE;
        head_chunk->this_size = sizeof(free_chunk_type);
        head_chunk->next_node = NULL;
        head_chunk->prev_node = NULL;

        free_chunk_type* chunk = fc(FirstChunk(page));
        chunk->flags = flag;
        chunk->prev_size = sizeof(free_chunk_type);
        chunk->this_size = bytes - sizeof(page_type) - sizeof(free_chunk_type) * 2;
        chunk->bin_id = 0;
        chunk->fence = CHUNK_FENCE;
        chunk->next_node = NULL;
        chunk->prev_node = NULL;


        free_chunk_type* tail_chunk = fc(Next(chunk));
        tail_chunk->flags = flag | CHUNK_IS_IN_USED;
        tail_chunk->prev_size = chunk->this_size;
        tail_chunk->bin_id = 0;
        tail_chunk->fence = CHUNK_FENCE;
        tail_chunk->this_size = sizeof(free_chunk_type);
        tail_chunk->next_node = NULL;
        tail_chunk->prev_node = NULL;

        CHECK_C(chunk);
        return chunk;
    }


    u64 zmalloc::free_page(page_type* page)
    {
        if (used_page_list_ == page)
        {
            used_page_list_ = page->next;
            if (page->next)
            {
                page->next->front = NULL;
            }
        }
        else
        {
            if (page->front)
            {
                page->front->next = page->next;
            }
            if (page->next)
            {
                page->next->front = page->front;
            }
        }
        used_page_count_--;
        return_total_count_++;

        if (!CHasBit(FirstChunk(page), CHUNK_IS_DIRECT) && reserve_page_count_ < max_reserve_page_count_)
        {
            reserve_page_count_++;
            if (reserve_page_list_ == NULL)
            {
                reserve_page_list_ = page;
                page->next = NULL;
                page->front = NULL;
            }
            else
            {
                page->next = reserve_page_list_;
                page->next->front = page;
                page->front = NULL;
                reserve_page_list_ = page;
            }
            CHECK_STATE(*this);
            return_cache_count_++;
            return page->page_size;
        }
        CHECK_STATE(*this);
        return_real_bytes_ += page->page_size;
        if (page_free_)
        {
            return page_free_(page);
        }
         return default_page_free(page);
    }

    void zmalloc::push_chunk(free_chunk_type* chunk, u32 bin_id)
    {
        u32 level = Level(chunk);
        SetShift(bitmap_[level], bin_id);
        chunk->next_node = bin_[level][bin_id].next_node;
        chunk->next_node->prev_node = chunk;
        bin_[level][bin_id].next_node = chunk;
        chunk->prev_node = &bin_[level][bin_id];
        chunk->bin_id = bin_id;
        CUnsetBit(chunk, CHUNK_IS_IN_USED);
        CHECK_FCL(*this, chunk);
    }

    void zmalloc::push_small_chunk(free_chunk_type* chunk)
    {
        u32 bin_id = ((chunk->this_size - CHUNK_PADDING_SIZE) >> SMALL_GRADE_SHIFT);
        if (bin_id >= BINMAP_SIZE)
        {
            bin_id = BINMAP_SIZE - 1;
        }
        push_chunk(chunk, bin_id);
    }
    void zmalloc::push_big_chunk(free_chunk_type* chunk)
    {
        u32 bytes = chunk->this_size - CHUNK_PADDING_SIZE;
        u32 third_index = ThirdBitIndex(bytes);
        u32 bin_id = GeoSequenceZip(third_index, bytes);
        push_chunk(chunk, bin_id);
    }

    //typedef void (*InsertFree)(free_chunk_type* chunk);
    //static const InsertFree push_chunkFunc[] = { &push_small_chunk , &push_big_chunk };

    bool zmalloc::pick_chunk(free_chunk_type* chunk)
    {
        CHECK_FCL(*this, chunk);
        u32 bin_id = chunk->bin_id;
        u32 level = Level(chunk);
        chunk->prev_node->next_node = chunk->next_node;
        chunk->next_node->prev_node = chunk->prev_node;
        if (bin_[level][bin_id].next_node == &bin_end_[level][bin_id])
        {
            UnsetShift(bitmap_[level], bin_id);
        }
        CHECK_C(chunk);
        return true;
    }

    free_chunk_type* zmalloc::exploit_new_chunk(free_chunk_type* devide_chunk, u32 new_chunk_size)
    {
        u32 new_devide_size = devide_chunk->this_size - new_chunk_size;
        devide_chunk->this_size = new_devide_size;
        free_chunk_type* new_chunk = fc(u(devide_chunk) + new_devide_size);
        new_chunk->flags = devide_chunk->flags;
        new_chunk->fence = CHUNK_FENCE;
        new_chunk->prev_size = new_devide_size;
        new_chunk->this_size = new_chunk_size;
        new_chunk->bin_id = 0;
        Next(new_chunk)->prev_size = new_chunk_size;
        return new_chunk;
    }

 
    void* zmalloc::alloc_memory(u64 req_bytes)
    {
        if (!inited_)
        {
            auto cache_max_reserve_page_count = max_reserve_page_count_;
            auto cache_page_alloc = page_alloc_;
            auto cache_page_free = page_free_;
            memset(this, 0, sizeof(zmalloc));
            max_reserve_page_count_= cache_max_reserve_page_count;
            page_alloc_ = cache_page_alloc;
            page_free_ = cache_page_free;
            inited_ = 1;
            for (u32 level = 0; level < BITMAP_LEVEL; level++)
            {
                for (u32 bin_id = 0; bin_id < BINMAP_SIZE; bin_id++)
                {
                    bin_[level][bin_id].next_node = &bin_end_[level][bin_id];
                    bin_end_[level][bin_id].prev_node = &bin_[level][bin_id];
                }
            }
        }
        req_total_bytes_ += req_bytes;
        req_total_count_++;
        free_chunk_type* chunk = NULL;
        if (req_bytes < SMALL_MAX_REQUEST - SMALL_GRADE_SIZE)
        {
            constexpr u32 level = 0;
            u32 padding = req_bytes < SMALL_GRADE_SIZE ? SMALL_GRADE_SIZE : (u32)req_bytes + SMALL_GRADE_MASK;
            u32 small_id = padding >> SMALL_GRADE_SHIFT;
            padding = (small_id << SMALL_GRADE_SHIFT) + CHUNK_PADDING_SIZE;
            u64 bitmap = bitmap_[level] >> small_id;
            if (bitmap & 0x3)
            {
                u32 bin_id = small_id + ((~bitmap) & 0x1);
                chunk = bin_[level][bin_id].next_node;
                pick_chunk(chunk);
                goto SMALL_RETURN;
            }
            if (dv_[level])
            {
                if (dv_[level]->this_size >= padding + SMALL_LEAST_SIZE)
                {
                    chunk = exploit_new_chunk(dv_[level], padding);
                    goto SMALL_RETURN;
                }
                else if (dv_[level]->this_size >= padding)
                {
                    chunk = dv_[level];
                    dv_[level] = NULL;
                    goto SMALL_RETURN;
                }
            }

            if (bitmap != 0)
            {
                u32 bin_id = small_id + first_bit_index(bitmap & -bitmap);
                chunk = bin_[level][bin_id].next_node;
                pick_chunk(chunk);
            }
            else
            {
                chunk = alloc_page(padding, 0);
                if (chunk == NULL)
                {
                    //LogWarn() << "no more memory";
                    return NULL;
                }
            }
            if (dv_[level] == NULL)
            {
                dv_[level] = chunk;
            }
            else
            {
                push_small_chunk(dv_[level]);
                dv_[level] = chunk;
            }

            chunk = exploit_new_chunk(chunk, padding);

        SMALL_RETURN:
            CSetBit(chunk, CHUNK_IS_IN_USED);
            chunk->fence = CHUNK_FENCE;
            chunk->bin_id = small_id;
            //alloc_counter_[!CHUNK_IS_BIG][chunk->bin_id] ++;
            //RECORD_ALLOC(ALLOC_ZMALLOC, req_bytes);
            //RECORD_REQ_ALLOC_BYTES(ALLOC_ZMALLOC, req_bytes, chunk->this_size);
            //alloc_total_bytes_ += chunk->this_size;
            return (void*)(u(chunk) + CHUNK_PADDING_SIZE);
        }
        //(1008~BIG_MAX_REQUEST)
        if (req_bytes < BIG_MAX_REQUEST)
        {
            constexpr u32 level = 1;
            u32 padding = ((u32)req_bytes + 255) & ~255U;
            u32 third_index = ThirdBitIndex(padding);
            padding = CeilGeoBytes(third_index, padding);
            third_index = CeilThirdIndex(third_index, padding);

            u32 id = GeoSequenceZip(third_index, padding);
            static_assert(GeoSequenceZip(8, 1024) == 0, "least id test");
            static_assert(GeoSequenceZip(8, 1280) == 1, "least id test");
            static_assert(GeoSequenceZip(8, 1536) == 2, "least id test");

            padding += CHUNK_PADDING_SIZE;
            u64 bitmap = bitmap_[level] >> id;
            if (bitmap & 0x1)
            {
                u32 bin_id = id + ((~bitmap) & 0x1);
                chunk = bin_[level][bin_id].next_node;
                pick_chunk(chunk);
                goto BIG_RETURN;
            }

            if (dv_[level])
            {
                if (dv_[level]->this_size >= padding + SMALL_MAX_REQUEST + CHUNK_PADDING_SIZE)
                {
                    chunk = exploit_new_chunk(dv_[level], padding);
                    goto BIG_RETURN;
                }
                else if (dv_[level]->this_size >= padding)
                {
                    chunk = dv_[level];
                    dv_[level] = NULL;
                    goto BIG_RETURN;
                }
            }

            if (bitmap != 0)
            {
                u32 bin_id = id + first_bit_index(bitmap & -bitmap);
                chunk = bin_[level][bin_id].next_node;
                pick_chunk(chunk);
            }
            else
            {
                chunk = alloc_page(padding, CHUNK_IS_BIG);
                if (chunk == NULL)
                {
                    //LogWarn() << "no more memory";
                    return NULL;
                }
            }

            if (chunk->this_size >= padding + SMALL_MAX_REQUEST + CHUNK_PADDING_SIZE)
            {
                free_chunk_type* dv_chunk = chunk;
                chunk = exploit_new_chunk(dv_chunk, padding);
                if (dv_[level] == NULL)
                {
                    dv_[level] = dv_chunk;
                }
                else
                {
                    push_big_chunk(dv_[level]);
                    dv_[level] = dv_chunk;
                }
            }


        BIG_RETURN:
            CSetBit(chunk, CHUNK_IS_IN_USED);
            chunk->fence = CHUNK_FENCE;
            chunk->bin_id = id;
            //alloc_counter_[CHUNK_IS_BIG][chunk->bin_id] ++;
            //RECORD_ALLOC(ALLOC_ZMALLOC, req_bytes);
            //RECORD_REQ_ALLOC_BYTES(ALLOC_ZMALLOC, req_bytes, chunk->this_size);
            alloc_total_bytes_ += chunk->this_size;
            return (void*)(u(chunk) + CHUNK_PADDING_SIZE);
        }

        chunk = alloc_page((u32)req_bytes + CHUNK_PADDING_SIZE + SMALL_LEAST_SIZE, CHUNK_IS_DIRECT);
        if (chunk == NULL)
        {
            //LogWarn() << "no more memory";
            return NULL;
        }

        CSetBit(chunk, CHUNK_IS_IN_USED);
        //RECORD_ALLOC(ALLOC_ZMALLOC, req_bytes);
        //RECORD_REQ_ALLOC_BYTES(ALLOC_ZMALLOC, req_bytes, chunk->this_size);
        alloc_total_bytes_ += chunk->this_size;
        return (void*)(u(chunk) + CHUNK_PADDING_SIZE);
    }


    u64 zmalloc::free_memory(void* addr)
    {
        zmalloc& zstate = *this;
        if (addr == NULL)
        {
            //LogError() << "free null";
            return 0;
        }
        free_chunk_type* chunk = fc(u(addr) - CHUNK_PADDING_SIZE);
        if (!InUse(chunk))
        {
            //LogError() << "free error";
            return 0;
        }
        CHECK_C(chunk);
        CUnsetBit(chunk, CHUNK_IS_IN_USED);
#if ZMALLOC_OPEN_FENCE
        if (!IsGoodFence(chunk) || !IsGoodFence(Next(chunk)))
        {
            LogError() << "fence error";
            return 0;
        }
#endif
        //RECORD_FREE(ALLOC_ZMALLOC, chunk->this_size);
        //free_total_count_++;
        //free_total_bytes_ += chunk->this_size;

        u32 level = Level(chunk);
        u64 bytes = chunk->this_size - CHUNK_PADDING_SIZE;
        (void)level;
        if (chunk->flags & CHUNK_IS_DIRECT)
        {
            CUnsetBit(chunk, CHUNK_IS_IN_USED);
            page_type* page = Thispage_type(chunk);
            free_page(page);
            return bytes;
        }

        free_counter_[chunk->flags & CHUNK_IS_BIG][chunk->bin_id]++;
        if (!InUse(Front(chunk)))
        {
            free_chunk_type* prev_node = Front(chunk);
            if (prev_node != dv_[level])
            {
                pick_chunk(prev_node);
            }
            prev_node->this_size += chunk->this_size;
            prev_node->flags |= chunk->flags;
            chunk = prev_node;
            Next(chunk)->prev_size = chunk->this_size;
            CHECK_C(chunk);
        }

        if (!InUse(Next(chunk)))
        {
            free_chunk_type* next_node = fc(Next(chunk));
            if (next_node == dv_[level])
            {
                dv_[level] = chunk;
            }
            else
            {
                pick_chunk(next_node);
            }
            chunk->this_size += next_node->this_size;
            chunk->flags |= next_node->flags;
            Next(chunk)->prev_size = chunk->this_size;
            CHECK_C(chunk);
        }

        if (chunk == dv_[level])
        {
            return bytes;
        }

        if (chunk->this_size >= SALE_SYS_ALLOC_SIZE - sizeof(page_type) - sizeof(free_chunk_type) * 2)
        {
            page_type* page = Thispage_type(chunk);
            free_page(page);
            return bytes;
        }

        //push_chunkFunc[Level(chunk)](chunk);
        if (CHasBit(chunk, CHUNK_IS_BIG))
        {
            push_big_chunk(chunk);
        }
        else
        {
            push_small_chunk(chunk);
        }
        return bytes;
        }




    void zmalloc::check_health()
    {
        //DebugAssertAllpage_type(instance());
        //DebugAssertBitmap(instance());
    }

    void zmalloc::clear_cache()
    {
        Summary();
        for (size_t i = 0; i < BITMAP_LEVEL; i++)
        {
            /**/
            if (dv_[i])
            {
                CSetBit(dv_[i], CHUNK_IS_IN_USED);
                req_total_count_++;
                req_total_bytes_ += dv_[i]->this_size;
                alloc_total_bytes_ += dv_[i]->this_size;
                dv_[i]->bin_id = 0;
                void* addr = (char*)dv_[i] + CHUNK_PADDING_SIZE;
                dv_[i] = NULL;
                free_memory(addr);
            }

            dv_[i] = NULL;

        }
        while (reserve_page_list_)
        {
            page_type* release_field = reserve_page_list_;
            reserve_page_list_ = reserve_page_list_->next;
            reserve_page_count_--;
            return_real_bytes_ += release_field->page_size;
            if (page_free_)
            {
                page_free_(release_field);
            }
            else
            {
                default_page_free(release_field);
            }
            
        }
        Summary();
    }
    const char* zmalloc::SummaryStatic()
    {
        static const size_t bufsz = 10 * 1024;
        static char buffer[bufsz] = { 0 };
        int used = 0;
        int ret = snprintf(buffer, bufsz, "zmalloc summary: page size:%u, page cache:%u \n"
            "used page:%u, cache page:%u, in hold:%0.4lfm, in used:%0.4lfm\n"
            "sale total count:%.03lfk, return total count:%.03lfk\n"
            "sale cache count:%.03lfk, return cache count:%.03lfk\n"
            "total req count:%.03lfk, free count:%.03lfk\n"
            "total req:%.04lfm, total alloc:%.04lfm, total free:%.04lfm \n"
            "total sale:%.04lfm, total return:%.04lfm\n"
            "sale real:%.04lfm, return real:%.04lfm\n"
            "avg inner frag:%llu%%.\n",
            SALE_SYS_ALLOC_SIZE, max_reserve_page_count_,
            used_page_count_, reserve_page_count_, (sale_real_bytes_ - return_real_bytes_) / 1024.0 / 1024.0, (alloc_total_bytes_ - free_total_bytes_) / 1024.0 / 1024.0,
            sale_total_count_ / 1000.0, return_total_count_ / 1000.0, sale_cache_count_ / 1000.0, return_cache_count_ / 1000.0,
            req_total_count_ / 1000.0, free_total_count_ / 1000.0,
            req_total_bytes_ / 1024.0 / 1024.0, alloc_total_bytes_ / 1024.0 / 1024.0, free_total_bytes_ / 1024.0 / 1024.0,
            (sale_cache_count_ * SALE_SYS_ALLOC_SIZE + sale_real_bytes_) / 1024.0 / 1024.0, (return_cache_count_ * SALE_SYS_ALLOC_SIZE + return_real_bytes_) / 1024.0 / 1024.0,
            sale_real_bytes_ / 1024.0 / 1024.0, return_real_bytes_ / 1024.0 / 1024.0,
            req_total_bytes_ * 100 / (alloc_total_bytes_ == 0 ? 1 : alloc_total_bytes_));
        u32 c = 0;
        while (ret > 0 && c < BINMAP_SIZE)
        {
            used += ret;
            ret = snprintf(buffer + used, bufsz - used, "[%03u]\t[%u byte]:\t alloc:%llu  \tfree:%llu\n", c, ((c) << SMALL_GRADE_SHIFT), alloc_counter_[0][c], free_counter_[0][c]);
            c++;
        }
        c = 0;
        while (ret > 0 && c < BINMAP_SIZE)
        {
            used += ret;
            u32 first = (c + 30) / 3;
            u32 m = first - 2;
            u64 bytes = (1ULL << first) + ((((u64)c + 30) % 3) << m);
            if (bytes >= BIG_MAX_REQUEST)
            {
                break;
            }
            ret = snprintf(buffer + used, bufsz - used, "[%03u]\t[%llu byte]:\t alloc:%llu  \tfree:%llu\n", c + BINMAP_SIZE, bytes, alloc_counter_[CHUNK_IS_BIG][c], free_counter_[CHUNK_IS_BIG][c]);
            c++;
        }
        return buffer;
    }
    void zmalloc::Summary()
    {
        //LOGFMTD("%s", SummaryStatic());
    }

    /*

    #define DebugAssert(expr, desc) if (!(expr)) \
            if (true) \
            {\
                //////LogError() << "DebugAssert " << desc <<" fail"; \
                std::this_thread::sleep_for(std::chrono::milliseconds(1000)); \
                 *(volatile u64*)NULL; \
            }

    void DebugAssertChunk(chunk_type* c)
    {
        DebugAssert(c, "chunk is NULL");
        DebugAssert(IsGoodFence(c), "good fence");
        DebugAssert(IsGoodFence(Next(c)), "good next fence");
        DebugAssert(IsGoodFence(Front(c)), "good front fence");
        DebugAssert(Next(c)->prev_size == c->this_size, "good prev_size");
        DebugAssert(c->prev_size == Front(c)->this_size, "good prev_size");

        DebugAssert(Level(c) == Level(Next(c)), "good level");
        DebugAssert(Level(c) == Level(Front(c)), "good level");
        DebugAssert(c->this_size >= SMALL_LEAST_SIZE, "good this size");
        if (!CHasBit(c, CHUNK_IS_DIRECT))
        {
            DebugAssert(c->this_size <= SALE_SYS_ALLOC_SIZE - sizeof(page_type) - sizeof(free_chunk_type) * 2, "good this size");
            DebugAssert(c->this_size + Next(c)->this_size + Front(c)->this_size <= SALE_SYS_ALLOC_SIZE - (u32)sizeof(page_type), "good this size");
        }

        if (Level(c) > 0)
        {
            DebugAssert(c->this_size >= SMALL_MAX_REQUEST + CHUNK_PADDING_SIZE, "good big size");
            DebugAssert(c->this_size >= 1024 + CHUNK_PADDING_SIZE, "good big size");
        }
    }

    void DebugAssertFreeChunk(free_chunk_type* c)
    {
        DebugAssertChunk(c);
        DebugAssert(!InUse(c), "free chunk");
        DebugAssert(c->bin_id < 64, "bin id");
        DebugAssert(!(!InUse(c) && !InUse(Next(c))), "good in use");
        DebugAssert(!(!InUse(c) && !InUse(Front(c))), "good in use");
    }

    void DebugAssertFreeChunkList(zmalloc& zstate, free_chunk_type* c)
    {
        DebugAssertChunk(c);
        if (c != zstate.dv_[Level(c)])
        {
            DebugAssert((u64)c->next_node < ((~0ULL) >> 0x4), "free pointer");
            DebugAssert((u64)c->prev_node < ((~0ULL) >> 0x4), "free pointer");
            DebugAssert(zstate.bitmap_[Level(c)] & 1ULL << c->bin_id, "has bitmap flag");
            DebugAssert(zstate.bin_[Level(c)][c->bin_id].next_node, "has bin pointer");
            bool found_this = false;
            free_chunk_type* head = zstate.bin_[Level(c)][c->bin_id].next_node;
            while (head)
            {
                if (c == head)
                {
                    found_this = true;
                    break;
                }
                if (c == &zstate.bin_end_[Level(c)][c->bin_id])
                {
                    break;
                }
                head = head->next_node;
            }
            DebugAssert(found_this, "found in bin");
            DebugAssert(c->this_size > CHUNK_PADDING_SIZE, "chunk size too small");
            if (Level(c))
            {
                u32 index_size = BigIndexSize(c->bin_id);
                (void)index_size;
                DebugAssert(c->this_size >= BigIndexSize(c->bin_id) + CHUNK_PADDING_SIZE, "chunk size too small");
                DebugAssert(c->this_size < BigIndexSize(c->bin_id + 1) + CHUNK_PADDING_SIZE, "chunk size too large");
            }
            else
            {
                DebugAssert((c->this_size - CHUNK_PADDING_SIZE) >= (u32)(c->bin_id) << SMALL_GRADE_SHIFT, "chunk size too large");
                if (c->this_size < 63 * ShiftSize(SMALL_GRADE_SHIFT) + CHUNK_PADDING_SIZE)
                {
                    DebugAssert((c->this_size - CHUNK_PADDING_SIZE) < (u32)(c->bin_id + 1) << SMALL_GRADE_SHIFT, "chunk size too small");
                }
            }
        }
    }
    void DebugAssertpage_type(page_type* field_list, u32 field_list_size, u32 max_list_size)
    {
        if (field_list == NULL)
        {
            DebugAssert(field_list_size == 0, "reserve size");
        }
        else
        {
            DebugAssert(field_list_size > 0, "reserve size");
        }
        DebugAssert(field_list_size <= max_list_size, "reserve size");


        u32 detect_size = 0;
        page_type* page = field_list;
        page_type* front_field = page;
        while (page)
        {
            DebugAssert(IsPowerOf2(page->page_size), "align");


            if (CHasBit(FirstChunk(page), CHUNK_IS_DIRECT))
            {

            }
            else
            {
                DebugAssert(page->page_size >= SALE_SYS_ALLOC_SIZE, "align");
                DebugAssert(page->page_size >= BIG_MAX_REQUEST, "align");
            }

            detect_size++;
            front_field = page;
            page = page->next;
        }
        DebugAssert(detect_size == field_list_size, "reserve size");

        page = front_field;
        while (front_field)
        {
            detect_size--;
            page = front_field;
            front_field = front_field->front;
        }
        DebugAssert(detect_size == 0, "reserve size");
        DebugAssert(page == field_list, "reserve size");
    }

    void DebugAssertAllpage_type(zmalloc& zstate)
    {
        DebugAssertpage_type(zstate.reserve_page_list_, zstate.reserve_page_count_, zstate.max_reserve_page_count_);
        DebugAssertpage_type(zstate.used_page_list_, zstate.used_page_count_, ~0U);

        page_type* page = zstate.used_page_list_;
        page_type* last_field = page;
        u32 c_count = 0;
        u32 fc_count = 0;
        page = last_field;
        while (page)
        {
            u32 field_bytes = 0;
            u32 field_c_count = 0;
            u32 field_fc_count = 0;
            chunk_type* c = FirstChunk(page);
            while (c)
            {
                c_count++;
                field_c_count++;
                field_fc_count += InUse(c) ? 0 : 1;
                fc_count += InUse(c) ? 0 : 1;
                field_bytes += c->this_size;
                if (IsDirect(c))
                {
                    DebugAssert(InUse(Front(c)) && InUse(Next(c)), "bound chunk");
                    DebugAssert(Front(c)->this_size == Next(c)->this_size, "bound size");
                }
                else if (!InUse(c) && c != zstate.dv_[Level(c)])
                {
                    DebugAssert((1ULL << c->bin_id) & zstate.bitmap_[Level(c)], "bitmap");
                    DebugAssert(zstate.bin_[Level(c)][c->bin_id].next_node, "bin");
                    free_chunk_type* fcb = zstate.bin_[Level(c)][c->bin_id].next_node;
                    bool found = false;
                    while (fcb != &zstate.bin_end_[Level(c)][c->bin_id])
                    {
                        CHECK_FCL(zstate, fcb);
                        if (fcb == c)
                        {
                            found = true;
                        }
                        fcb = fcb->next_node;
                    }
                    DebugAssert(found, "found in bin");
                }

                if (field_bytes + FIELD_SIZE + (u32)sizeof(free_chunk_type) * 2U == page->page_size)
                {
                    if (fc(Next(c))->this_size == sizeof(free_chunk_type)
                        && InUse(fc(Next(c))))
                    {
                        break;
                    }
                }
                DebugAssert(field_bytes + FIELD_SIZE + (u32)sizeof(free_chunk_type) * 2U <= page->page_size, "max page");
                c = Next(c);
            };
            last_field = page;
            page = page->front;
        }
        //LogInfo() << "check all page success. page count:" << field_count << ", total chunk:" << c_count <<", free chunk:" << fc_count;
    }

    void DebugAssertBitmap(zmalloc& zstate)
    {
        for (u32 small_type = 0; small_type < 2; small_type++)
        {
            for (u32 bin_id = 0; bin_id < BINMAP_SIZE; bin_id++)
            {
                free_chunk_type* fc = zstate.bin_[small_type][bin_id].next_node;
                if (fc != &zstate.bin_end_[small_type][bin_id])
                {
                    DebugAssert(HasShift(zstate.bitmap_[small_type], bin_id), "has bitmap");
                }
                else
                {
                    DebugAssert(!HasShift(zstate.bitmap_[small_type], bin_id), "has bitmap");
                }
                while (fc != &zstate.bin_end_[small_type][bin_id])
                {
                    DebugAssert(fc->bin_id == bin_id, "bin id");
                    DebugAssert(!InUse(fc), "free");
                    fc = fc->next_node;
                }
            }
        }
    }

    */
    }
#endif