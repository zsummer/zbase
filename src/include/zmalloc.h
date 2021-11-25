
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
#define ZMALLOC_OPEN_FENCE 1

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

#define zmalloc_floor_power_of_2(num)   (zmalloc_fill_right((num) >> 1) + 1)
#define zmalloc_ceil_power_of_2(num)   (zmalloc_fill_right((num - 1) ) + 1)
#define zmalloc_is_power_of_2(num)  (!(num & (num-1)))
#define zmalloc_last_bit_size(x) ((x) & -(x))

#define zmalloc_order_size(shift) (1U << (shift))
#define zmalloc_order_size_64(shift) (1ULL << (shift))
#define zmalloc_order_mask(shift) (zmalloc_order_size(shift) -1U)
#define zmalloc_order_mask_64(shift) (zmalloc_order_size_64(shift) -1ULL)

#define zmalloc_align_value(bytes, up) ( ( (bytes) + ((up) - 1U) ) & ~((up) - 1U) )  
#define zmalloc_is_align_value(bytes, up) (!((bytes) & ((up) - 1U)))

static_assert(zmalloc_align_value(0, 4) == 0, "");
static_assert(zmalloc_align_value(1, 4) == 4, "");
static_assert(zmalloc_align_value(1, 4096) == 4096, "");
static_assert(zmalloc_align_value((1ULL << 50) + 1, (1ULL << 50)) == (1ULL << 50) * 2, "");
static_assert(zmalloc_align_value((1ULL << 50) * 2 + 1, (1ULL << 50)) == (1ULL << 50) * 3, "");
static_assert(zmalloc_is_align_value(0, 4), "");
static_assert(!zmalloc_is_align_value(1, 4), "");
static_assert(zmalloc_is_align_value(4, 4), "");

#define zmalloc_align_default_value(bytes) zmalloc_align_value(bytes, sizeof(std::max_align_t)) 
static_assert(zmalloc_align_default_value(1) == sizeof(std::max_align_t), "");
static_assert(zmalloc_align_default_value(0) == 0, "");

#define zmalloc_align_up_value(bytes, shift) (((bytes) + zmalloc_order_mask_64(shift)) >> (shift))
static_assert(zmalloc_align_up_value(0, 10) == 0, "");
static_assert(zmalloc_align_up_value(1, 10) == 1, "");
static_assert(zmalloc_align_up_value(1 << 10, 10) == 1, "");
static_assert(zmalloc_align_up_value((1 << 10) + 1, 10) == 2, "");
static_assert(zmalloc_align_up_value(1 << 10, 10) == 1, "");
static_assert(zmalloc_align_up_value((1ULL << 50) + 1, 50) == 2, "");

#define zmalloc_align_third_bit_value(value)  (value + (zmalloc_fill_right(value) >> 3) )
#define zmalloc_align_third_bit_order(value)  (zmalloc_first_bit_index(value ) - 2)
#define zmalloc_third_sequence(third_order, value) (    ((third_order) << 2)   +    (((value) >> (third_order)) & 0x3)   )

static constexpr u32 geo_sequence_test_1 = zmalloc_third_sequence(0, 7);
static_assert(zmalloc_third_sequence(0, 4) == 0, "");
static_assert(zmalloc_third_sequence(0, 5) == 1, "");
static_assert(zmalloc_third_sequence(1, 8) == 4, "");
static_assert(zmalloc_third_sequence(1, 15) == 7, "");
static_assert(zmalloc_third_sequence(2, 16) == 8, "");
static_assert(zmalloc_third_sequence(8, 1024) == 32, "");
static_assert(zmalloc_third_sequence(8, 1280) == 33, "");
static_assert(zmalloc_third_sequence(9, 2048) == 36, "");
#define zmalloc_third_sequence_compress(sequence) (sequence - 32)
#define zmalloc_resolve_order_size(index )  ((((index + 32) & 0x3) | 0x4) << (((index +32) >> 2) ))

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
        using block_alloc_func = void* (*)(u64);
        using block_free_func = u64(*)(void*);
        static const u32 BINMAP_SIZE = (sizeof(u64) * 8U);
        static const u32 BITMAP_LEVEL = 2;
        static const u32 DEFAULT_BLOCK_SIZE = (8 * 1024 * 1024);
    public:
        inline static zmalloc& instance();
        inline static zmalloc* instance_ptr();
        inline static void* default_block_alloc(u64 );
        inline static u64 default_block_free(void*);
        inline static void set_global(zmalloc* state);
        inline void set_block_callback(block_alloc_func block_alloc, block_free_func block_free);
        inline void* alloc_memory(u64 bytes);
        inline u64  free_memory(void* addr);

        inline void check_health();
        inline void clear_cache();
        inline const char* debug_string();

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

        struct block_type
        {
            u32 block_size;
            u32 fence;
            u64 reserve;
            block_type* next;
            block_type* front;
        };

        static const u32 LEAST_ALIGN_SHIFT = 4U;
        enum chunk_flags : u32
        {
            CHUNK_IS_BIG = 0x1,
            CHUNK_IS_IN_USED = 0x2,
            CHUNK_IS_DIRECT = 0x4,
        };
        static const u32 CHUNK_LEVEL_MASK = 0x01;
        static const u32 CHUNK_FENCE = 0xdeadbeaf;
        static const u32 CHUNK_PADDING_SIZE = sizeof(chunk_type);
        static const u32 CHUNK_FREE_SIZE = sizeof(free_chunk_type);
        static const u32  FINE_GRAINED_SHIFT = 4U;
        static const u32  FINE_GRAINED_SIZE = zmalloc_order_size(FINE_GRAINED_SHIFT);
        static const u32  FINE_GRAINED_MASK = zmalloc_order_mask(FINE_GRAINED_SHIFT);
        static const u32  SMALL_LEAST_SIZE = FINE_GRAINED_SIZE + CHUNK_PADDING_SIZE;
        static const u32  SMALL_MAX_REQUEST = (BINMAP_SIZE << FINE_GRAINED_SHIFT);
        static const u32  BIG_LEAST_SIZE = SMALL_MAX_REQUEST + CHUNK_PADDING_SIZE;
        static const u32  BIG_MAX_REQUEST = 512 * 1024;

        static_assert(CHUNK_FREE_SIZE - CHUNK_PADDING_SIZE == sizeof(void*) * 2, "check memory layout.");
        static_assert(zmalloc_order_size(LEAST_ALIGN_SHIFT) == sizeof(void*) * 2, "check memory layout.");
        static_assert(sizeof(chunk_type) == zmalloc_order_size(LEAST_ALIGN_SHIFT), "payload addr in chunk must align the least align.");

        static_assert(SMALL_LEAST_SIZE >= sizeof(zmalloc::free_chunk_type), "");
        static_assert(zmalloc_order_size(LEAST_ALIGN_SHIFT) >= sizeof(void*) * 2, "");
        static_assert(FINE_GRAINED_SHIFT >= LEAST_ALIGN_SHIFT, "");
        static_assert(zmalloc_is_power_of_2(FINE_GRAINED_SHIFT), "");
        static_assert(BINMAP_SIZE == sizeof(u64) * 8, "");
        static_assert(DEFAULT_BLOCK_SIZE >= BIG_MAX_REQUEST + sizeof(block_type) + sizeof(chunk_type), "");
        static_assert(zmalloc_is_power_of_2(DEFAULT_BLOCK_SIZE), "");
        static_assert(sizeof(zmalloc::block_type) == zmalloc_order_size(zmalloc::LEAST_ALIGN_SHIFT + 1), "block align");
        static const u32 BLOCK_TYPE_SIZE = sizeof(zmalloc::block_type);
    private:
        inline free_chunk_type* alloc_block(u32 bytes, u32 flag);
        inline u64 free_block(block_type* block);
        inline void push_chunk(free_chunk_type* chunk, u32 bin_id);
        inline void push_small_chunk(free_chunk_type* chunk);
        inline void push_big_chunk(free_chunk_type* chunk);
        inline bool pick_chunk(free_chunk_type* chunk);
        inline free_chunk_type* exploit_new_chunk(free_chunk_type* devide_chunk, u32 new_chunk_size);
    public:
        u32 inited_;
        u32 block_power_is_2_;
        block_alloc_func block_alloc_;
        block_free_func block_free_;

        u32 max_reserve_block_count_;
        u64 req_total_count_;
        u64 free_total_count_;
        u64 req_total_bytes_;
        u64 alloc_total_bytes_;
        u64 free_total_bytes_;

        u64 alloc_block_count_;
        u64 free_block_count_;

        u64 alloc_block_cached_;
        u64 free_block_cached_;

        u64 alloc_block_bytes_;
        u64 free_block_bytes_;


        u32 used_block_count_;
        block_type* used_block_list_;
        u32 reserve_block_count_;
        block_type* reserve_block_list_;
        u64 bitmap_[BITMAP_LEVEL];

        free_chunk_type* dv_[BITMAP_LEVEL];
        free_chunk_type bin_[BITMAP_LEVEL][BINMAP_SIZE];
        free_chunk_type bin_end_[BITMAP_LEVEL][BINMAP_SIZE];
        u64 alloc_counter_[BITMAP_LEVEL][BINMAP_SIZE];
        u64 free_counter_[BITMAP_LEVEL][BINMAP_SIZE];
    };

#define global_zmalloc(bytes) zmalloc::instance().alloc_memory(bytes)
#define global_zfree(addr) zmalloc::instance().free_memory(addr)





#define ZMALLOC_OPEN_CHECK 0
    inline void zmalloc_debug_chunk(zmalloc::chunk_type* c);
    inline void zmalloc_debug_free_chunk(zmalloc::free_chunk_type* c);
    inline void zmalloc_debug_free_chunk_list(zmalloc& zstate, zmalloc::free_chunk_type* c);
    inline void zmalloc_debug_block_list(zmalloc::block_type* block_list, u32 block_list_size, u32 max_list_size);
    inline void zmalloc_debug_block(zmalloc& zstate);
    inline void zmalloc_debug_bitmap(zmalloc& zstate);
#if ZMALLOC_OPEN_CHECK
#define CHECK_STATE(state) zmalloc_debug_block(state)
#define CHECK_C(c) zmalloc_debug_chunk(c)
#define CHECK_FC(c) zmalloc_debug_free_chunk(c)
#define CHECK_FCL(state, c) zmalloc_debug_free_chunk_list(state, c)
#else
#define CHECK_STATE(state)  (void)(state)
#define CHECK_C(c) (void)(c)
#define CHECK_FC(c) (void)(c)
#define CHECK_FCL(state, c) (void)(c); (void)(state);
#endif




#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
    static_assert(offsetof(zmalloc::free_chunk_type, prev_node) == sizeof(zmalloc::chunk_type), "struct memory layout is impl-defined. so need this test.");
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#define zmalloc_chunk_cast(p) ((zmalloc::chunk_type*)(p))
#define zmalloc_u64_cast(p) ((u64)(p))
#define zmalloc_free_chunk_cast(p) ((zmalloc::free_chunk_type*)(p))

#define zmalloc_unset_bitmap(bitmap, shift)  ((bitmap) &= ~zmalloc_order_size_64(shift))
#define zmalloc_set_bitmap(bitmap, shift) ((bitmap) |= zmalloc_order_size_64(shift))
#define zmalloc_has_bitmap(bitmap, shift)  ((bitmap) & zmalloc_order_size_64(shift))

#define zmalloc_unset_chunk(chunk, val)  ((chunk)->flags &= ~(val))
#define zmalloc_set_chunk(chunk, val) ((chunk)->flags |= (val))
#define zmalloc_has_chunk(chunk, val)  ((chunk)->flags &  (val))

#define zmalloc_chunk_in_use(chunk) zmalloc_has_chunk((chunk), zmalloc::CHUNK_IS_IN_USED)
#define zmalloc_chunk_is_dirct(chunk) zmalloc_has_chunk((chunk), zmalloc::CHUNK_IS_DIRECT)
#define zmalloc_check_fence(chunk) ((chunk)->fence == zmalloc::CHUNK_FENCE)


#define zmalloc_front_chunk(p)  zmalloc_free_chunk_cast(zmalloc_u64_cast(p)-zmalloc_chunk_cast(p)->prev_size)
#define zmalloc_next_chunk(p)  zmalloc_free_chunk_cast(zmalloc_u64_cast(p)+zmalloc_chunk_cast(p)->this_size)
#define zmalloc_chunk_level(p) (zmalloc_free_chunk_cast(p)->flags & zmalloc::CHUNK_LEVEL_MASK)

#define zmalloc_get_block(firstp) ((zmalloc::block_type*)(zmalloc_u64_cast(firstp) - zmalloc::BLOCK_TYPE_SIZE - sizeof(zmalloc::free_chunk_type)))
#define zmalloc_get_first_chunk(block) zmalloc_chunk_cast( zmalloc_u64_cast(block)+zmalloc::BLOCK_TYPE_SIZE + sizeof(zmalloc::free_chunk_type))
#define zmalloc_get_block_head(block) zmalloc_chunk_cast( zmalloc_u64_cast(block)+zmalloc::BLOCK_TYPE_SIZE)


    zmalloc* g_zmalloc_state = NULL;


    zmalloc& zmalloc::instance()
    {
        return *g_zmalloc_state;
    }
    zmalloc* zmalloc::instance_ptr()
    {
        return g_zmalloc_state;
    }

    void* zmalloc::default_block_alloc(u64 req_size)
    {
        char* addr = (char*)malloc(req_size + 8);
        if (addr == NULL)
        {
            return NULL;
        }
        *((u64*)addr) = req_size;
        return addr + sizeof(u64);
   }

    u64 zmalloc::default_block_free(void* addr)
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

    void zmalloc::set_block_callback(block_alloc_func block_alloc, block_free_func block_free)
    {
        block_alloc_ = block_alloc;
        block_free_ = block_free;
    }















    zmalloc::free_chunk_type* zmalloc::alloc_block(u32 bytes, u32 flag)
    {
        bytes += sizeof(block_type) + CHUNK_PADDING_SIZE + sizeof(free_chunk_type) * 2;
        static_assert(BIG_MAX_REQUEST + CHUNK_PADDING_SIZE + sizeof(free_chunk_type) * 2 + sizeof(block_type) <= DEFAULT_BLOCK_SIZE, "");
        static_assert(DEFAULT_BLOCK_SIZE >= 1024 * 4, "");
        static_assert(zmalloc_is_power_of_2(DEFAULT_BLOCK_SIZE), "");
        alloc_block_count_++;
        bool dirct = flag & CHUNK_IS_DIRECT;
        if (!dirct)
        {
            bytes = DEFAULT_BLOCK_SIZE;
        }
        else if(block_power_is_2_)
        {
            bytes = zmalloc_ceil_power_of_2(bytes);
        }
        block_type* block = NULL;
        if (!dirct && reserve_block_count_ > 0)
        {
            block = reserve_block_list_;
            reserve_block_count_--;
            if (block->next)
            {
                reserve_block_list_ = block->next;
                block->next->front = NULL;
            }
            else
            {
                reserve_block_list_ = NULL;
            }
            alloc_block_cached_++;
            CHECK_STATE(*this);
        }
        else
        {
            if (block_alloc_)
            {
                block = (block_type*)block_alloc_(bytes);
            }
            else
            {
                block = (block_type*)default_block_alloc(bytes);
            }
            
            alloc_block_bytes_ += bytes;
        }
        if (block == NULL)
        {
            return NULL;
        }
        block->block_size = bytes;
        block->fence = CHUNK_FENCE;
        block->next = used_block_list_;
        used_block_list_ = block;
        block->front = NULL;
        if (block->next)
        {
            block->next->front = block;
        }
        used_block_count_++;
        free_chunk_type* head_chunk = zmalloc_free_chunk_cast(zmalloc_get_block_head(block));
        head_chunk->flags = flag | CHUNK_IS_IN_USED;
        head_chunk->prev_size = 0;
        head_chunk->bin_id = 0;
        head_chunk->fence = CHUNK_FENCE;
        head_chunk->this_size = sizeof(free_chunk_type);
        head_chunk->next_node = NULL;
        head_chunk->prev_node = NULL;

        free_chunk_type* chunk = zmalloc_free_chunk_cast(zmalloc_get_first_chunk(block));
        chunk->flags = flag;
        chunk->prev_size = sizeof(free_chunk_type);
        chunk->this_size = bytes - sizeof(block_type) - sizeof(free_chunk_type) * 2;
        chunk->bin_id = 0;
        chunk->fence = CHUNK_FENCE;
        chunk->next_node = NULL;
        chunk->prev_node = NULL;


        free_chunk_type* tail_chunk = zmalloc_free_chunk_cast(zmalloc_next_chunk(chunk));
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


    u64 zmalloc::free_block(block_type* block)
    {
        if (used_block_list_ == block)
        {
            used_block_list_ = block->next;
            if (block->next)
            {
                block->next->front = NULL;
            }
        }
        else
        {
            if (block->front)
            {
                block->front->next = block->next;
            }
            if (block->next)
            {
                block->next->front = block->front;
            }
        }
        used_block_count_--;
        free_block_count_++;

        if (!zmalloc_has_chunk(zmalloc_get_first_chunk(block), CHUNK_IS_DIRECT) && reserve_block_count_ < max_reserve_block_count_)
        {
            reserve_block_count_++;
            if (reserve_block_list_ == NULL)
            {
                reserve_block_list_ = block;
                block->next = NULL;
                block->front = NULL;
            }
            else
            {
                block->next = reserve_block_list_;
                block->next->front = block;
                block->front = NULL;
                reserve_block_list_ = block;
            }
            CHECK_STATE(*this);
            free_block_cached_++;
            return block->block_size;
        }
        CHECK_STATE(*this);
        free_block_bytes_ += block->block_size;
        if (block_free_)
        {
            return block_free_(block);
        }
         return default_block_free(block);
    }

    void zmalloc::push_chunk(free_chunk_type* chunk, u32 bin_id)
    {
        u32 level = zmalloc_chunk_level(chunk);
        zmalloc_set_bitmap(bitmap_[level], bin_id);
        chunk->next_node = bin_[level][bin_id].next_node;
        chunk->next_node->prev_node = chunk;
        bin_[level][bin_id].next_node = chunk;
        chunk->prev_node = &bin_[level][bin_id];
        chunk->bin_id = bin_id;
        zmalloc_unset_chunk(chunk, CHUNK_IS_IN_USED);
        CHECK_FCL(*this, chunk);
    }

    void zmalloc::push_small_chunk(free_chunk_type* chunk)
    {
        u32 bin_id = ((chunk->this_size - CHUNK_PADDING_SIZE) >> FINE_GRAINED_SHIFT);
        if (bin_id >= BINMAP_SIZE)
        {
            bin_id = BINMAP_SIZE - 1;
        }
        push_chunk(chunk, bin_id);
    }
    void zmalloc::push_big_chunk(free_chunk_type* chunk)
    {
        u32 bytes = chunk->this_size - CHUNK_PADDING_SIZE;
        u32 third_order = zmalloc_align_third_bit_order(bytes);
        u32 seq_id = zmalloc_third_sequence(third_order, bytes);
        u32 bin_id = zmalloc_third_sequence_compress(seq_id);
        push_chunk(chunk, bin_id);
    }

    //typedef void (*InsertFree)(free_chunk_type* chunk);
    //static const InsertFree push_chunkFunc[] = { &push_small_chunk , &push_big_chunk };

    bool zmalloc::pick_chunk(free_chunk_type* chunk)
    {
        CHECK_FCL(*this, chunk);
        u32 bin_id = chunk->bin_id;
        u32 level = zmalloc_chunk_level(chunk);
        chunk->prev_node->next_node = chunk->next_node;
        chunk->next_node->prev_node = chunk->prev_node;
        if (bin_[level][bin_id].next_node == &bin_end_[level][bin_id])
        {
            zmalloc_unset_bitmap(bitmap_[level], bin_id);
        }
        CHECK_C(chunk);
        return true;
    }

    zmalloc::free_chunk_type* zmalloc::exploit_new_chunk(free_chunk_type* devide_chunk, u32 new_chunk_size)
    {
        u32 new_devide_size = devide_chunk->this_size - new_chunk_size;
        devide_chunk->this_size = new_devide_size;
        free_chunk_type* new_chunk = zmalloc_free_chunk_cast(zmalloc_u64_cast(devide_chunk) + new_devide_size);
        new_chunk->flags = devide_chunk->flags;
        new_chunk->fence = CHUNK_FENCE;
        new_chunk->prev_size = new_devide_size;
        new_chunk->this_size = new_chunk_size;
        new_chunk->bin_id = 0;
        zmalloc_next_chunk(new_chunk)->prev_size = new_chunk_size;
        return new_chunk;
    }

 
    void* zmalloc::alloc_memory(u64 req_bytes)
    {
        if (!inited_)
        {
            auto cache_max_reserve_block_count = max_reserve_block_count_;
            auto cache_block_alloc = block_alloc_;
            auto cache_block_free = block_free_;
            auto block_allloc_power_of_2 = block_power_is_2_;
            memset(this, 0, sizeof(zmalloc));
            max_reserve_block_count_= cache_max_reserve_block_count;
            block_alloc_ = cache_block_alloc;
            block_free_ = cache_block_free;
            block_power_is_2_ = block_allloc_power_of_2;
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
        if (req_bytes < SMALL_MAX_REQUEST - FINE_GRAINED_SIZE)
        {
            constexpr u32 level = 0;
            u32 padding = req_bytes < FINE_GRAINED_SIZE ? FINE_GRAINED_SIZE : (u32)req_bytes + FINE_GRAINED_MASK;
            u32 small_id = padding >> FINE_GRAINED_SHIFT;
            padding = (small_id << FINE_GRAINED_SHIFT) + CHUNK_PADDING_SIZE;
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
                u32 bin_id = small_id + zmalloc_first_bit_index(bitmap & -bitmap);
                chunk = bin_[level][bin_id].next_node;
                pick_chunk(chunk);
            }
            else
            {
                chunk = alloc_block(padding, 0);
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
            zmalloc_set_chunk(chunk, CHUNK_IS_IN_USED);
            chunk->fence = CHUNK_FENCE;
            chunk->bin_id = small_id;
            alloc_counter_[!CHUNK_IS_BIG][chunk->bin_id] ++;
            //RECORD_ALLOC(ALLOC_ZMALLOC, req_bytes);
            //RECORD_REQ_ALLOC_BYTES(ALLOC_ZMALLOC, req_bytes, chunk->this_size);
            alloc_total_bytes_ += chunk->this_size;
            return (void*)(zmalloc_u64_cast(chunk) + CHUNK_PADDING_SIZE);
        }
        //(1008~BIG_MAX_REQUEST)
        if (req_bytes < BIG_MAX_REQUEST)
        {
            constexpr u32 level = 1;
            u32 padding = ((u32)req_bytes + 255) & ~255U;
            padding = zmalloc_align_third_bit_value(padding);
            u32 third_order = zmalloc_align_third_bit_order(padding);
            u32 align_id = zmalloc_third_sequence(third_order, padding);
            u32 compress_id = zmalloc_third_sequence_compress(align_id);

            padding += CHUNK_PADDING_SIZE;
            u64 bitmap = bitmap_[level] >> compress_id;
            if (bitmap & 0x1)
            {
                u32 bin_id = compress_id;
                chunk = bin_[level][bin_id].next_node;
                pick_chunk(chunk);
                goto BIG_RETURN;
            }

            if (dv_[level])
            {
                if (dv_[level]->this_size >= padding + BIG_LEAST_SIZE)
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
                u32 bin_id = compress_id + zmalloc_first_bit_index(bitmap & -bitmap);
                chunk = bin_[level][bin_id].next_node;
                pick_chunk(chunk);
            }
            else
            {
                chunk = alloc_block(padding, CHUNK_IS_BIG);
                if (chunk == NULL)
                {
                    //LogWarn() << "no more memory";
                    return NULL;
                }
            }

            if (chunk->this_size >= padding + BIG_LEAST_SIZE)
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
            zmalloc_set_chunk(chunk, CHUNK_IS_IN_USED);
            chunk->fence = CHUNK_FENCE;
            chunk->bin_id = compress_id;
            alloc_counter_[CHUNK_IS_BIG][chunk->bin_id] ++;
            //RECORD_ALLOC(ALLOC_ZMALLOC, req_bytes);
            //RECORD_REQ_ALLOC_BYTES(ALLOC_ZMALLOC, req_bytes, chunk->this_size);
            alloc_total_bytes_ += chunk->this_size;
            return (void*)(zmalloc_u64_cast(chunk) + CHUNK_PADDING_SIZE);
        }

        chunk = alloc_block((u32)req_bytes + CHUNK_PADDING_SIZE + SMALL_LEAST_SIZE, CHUNK_IS_DIRECT);
        if (chunk == NULL)
        {
            //LogWarn() << "no more memory";
            return NULL;
        }

        zmalloc_set_chunk(chunk, CHUNK_IS_IN_USED);
        //RECORD_ALLOC(ALLOC_ZMALLOC, req_bytes);
        //RECORD_REQ_ALLOC_BYTES(ALLOC_ZMALLOC, req_bytes, chunk->this_size);
        alloc_total_bytes_ += chunk->this_size;
        return (void*)(zmalloc_u64_cast(chunk) + CHUNK_PADDING_SIZE);
    }


    u64 zmalloc::free_memory(void* addr)
    {
        if (addr == NULL)
        {
            //LogError() << "free null";
            return 0;
        }
        free_chunk_type* chunk = zmalloc_free_chunk_cast(zmalloc_u64_cast(addr) - CHUNK_PADDING_SIZE);
        if (!zmalloc_chunk_in_use(chunk))
        {
            //LogError() << "free error";
            return 0;
        }
        CHECK_C(chunk);
        zmalloc_unset_chunk(chunk, CHUNK_IS_IN_USED);
#if ZMALLOC_OPEN_FENCE
        if (!zmalloc_check_fence(chunk) || !zmalloc_check_fence(zmalloc_next_chunk(chunk)))
        {
            LogError() << "fence error";
            return 0;
        }
#endif
        //RECORD_FREE(ALLOC_ZMALLOC, chunk->this_size);
        free_total_count_++;
        free_total_bytes_ += chunk->this_size;

        u32 level = zmalloc_chunk_level(chunk);
        u64 bytes = chunk->this_size - CHUNK_PADDING_SIZE;
        (void)level;
        if (chunk->flags & CHUNK_IS_DIRECT)
        {
            zmalloc_unset_chunk(chunk, CHUNK_IS_IN_USED);
            block_type* block = zmalloc_get_block(chunk);
            free_block(block);
            return bytes;
        }

        free_counter_[chunk->flags & CHUNK_IS_BIG][chunk->bin_id]++;
        if (!zmalloc_chunk_in_use(zmalloc_front_chunk(chunk)))
        {
            free_chunk_type* prev_node = zmalloc_front_chunk(chunk);
            if (prev_node != dv_[level])
            {
                pick_chunk(prev_node);
            }
            prev_node->this_size += chunk->this_size;
            prev_node->flags |= chunk->flags;
            chunk = prev_node;
            zmalloc_next_chunk(chunk)->prev_size = chunk->this_size;
            CHECK_C(chunk);
        }

        if (!zmalloc_chunk_in_use(zmalloc_next_chunk(chunk)))
        {
            free_chunk_type* next_node = zmalloc_free_chunk_cast(zmalloc_next_chunk(chunk));
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
            zmalloc_next_chunk(chunk)->prev_size = chunk->this_size;
            CHECK_C(chunk);
        }

        if (chunk == dv_[level])
        {
            return bytes;
        }

        if (chunk->this_size >= DEFAULT_BLOCK_SIZE - sizeof(block_type) - sizeof(free_chunk_type) * 2)
        {
            block_type* block = zmalloc_get_block(chunk);
            free_block(block);
            return bytes;
        }

        //push_chunkFunc[zmalloc_chunk_level(chunk)](chunk);
        if (zmalloc_has_chunk(chunk, CHUNK_IS_BIG))
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
        zmalloc_debug_block(instance());
        zmalloc_debug_bitmap(instance());
    }

    void zmalloc::clear_cache()
    {
        for (size_t i = 0; i < BITMAP_LEVEL; i++)
        {
            /**/
            if (dv_[i])
            {
                zmalloc_set_chunk(dv_[i], CHUNK_IS_IN_USED);
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
        while (reserve_block_list_)
        {
            block_type* release_block = reserve_block_list_;
            reserve_block_list_ = reserve_block_list_->next;
            reserve_block_count_--;
            free_block_bytes_ += release_block->block_size;
            if (block_free_)
            {
                block_free_(release_block);
            }
            else
            {
                default_block_free(release_block);
            }
            
        }
    }
    const char* zmalloc::debug_string()
    {
        static const size_t bufsz = 10 * 1024;
        static char buffer[bufsz] = { 0 };
        int used = 0;
        int ret = snprintf(buffer, bufsz, "zmalloc summary: block size:%u, max reserve block:%u \n"
            "used block:%u, cur reserve block:%u, in hold:%0.4lfm, in real used:%0.4lfm\n"
            "total alloc block count:%.03lfk, total free block count:%.03lfk\n"
            "total alloc cache count:%.03lfk, total free cache count:%.03lfk\n"
            "total req count:%.03lfk, free count:%.03lfk\n"
            "total req:%.04lfm, total alloc:%.04lfm, total free:%.04lfm \n"
            "total sale:%.04lfm, total return:%.04lfm\n"
            "sale real:%.04lfm, return real:%.04lfm\n"
            "avg inner frag:%llu%%.\n",
            DEFAULT_BLOCK_SIZE, max_reserve_block_count_,
            used_block_count_, reserve_block_count_, (alloc_block_bytes_ - free_block_bytes_) / 1024.0 / 1024.0, (alloc_total_bytes_ - free_total_bytes_) / 1024.0 / 1024.0,
            alloc_block_count_ / 1000.0, free_block_count_ / 1000.0, 
            alloc_block_cached_ / 1000.0, free_block_cached_ / 1000.0,
            req_total_count_ / 1000.0, free_total_count_ / 1000.0,
            req_total_bytes_ / 1024.0 / 1024.0, alloc_total_bytes_ / 1024.0 / 1024.0, free_total_bytes_ / 1024.0 / 1024.0,
            (alloc_block_cached_ * DEFAULT_BLOCK_SIZE + alloc_block_bytes_) / 1024.0 / 1024.0, (free_block_cached_ * DEFAULT_BLOCK_SIZE + free_block_bytes_) / 1024.0 / 1024.0,
            alloc_block_bytes_ / 1024.0 / 1024.0, free_block_bytes_ / 1024.0 / 1024.0,
            req_total_bytes_ * 100 / (alloc_total_bytes_ == 0 ? 1 : alloc_total_bytes_));
        u32 c = 0;
        while (ret > 0 && c < BINMAP_SIZE)
        {
            used += ret;
            ret = snprintf(buffer + used, bufsz - used, "[%03u]\t[%u byte]:\t alloc:%llu  \tfree:%llu \tused:%llu\n", 
                c, ((c) << FINE_GRAINED_SHIFT), alloc_counter_[0][c], free_counter_[0][c], alloc_counter_[0][c]- free_counter_[0][c]);
            c++;
        }
        c = 0;
        while (ret > 0 && c < BINMAP_SIZE)
        {
            used += ret;
            u64 bytes = zmalloc_resolve_order_size(c);
            if (bytes > BIG_MAX_REQUEST)
            {
                break;
            }
            ret = snprintf(buffer + used, bufsz - used, "[%03u]\t[%llu byte]:\t alloc:%llu  \tfree:%llu \tused:%llu\n", 
                c + 64, bytes, alloc_counter_[CHUNK_IS_BIG][c], free_counter_[CHUNK_IS_BIG][c], alloc_counter_[CHUNK_IS_BIG][c] - free_counter_[CHUNK_IS_BIG][c]);
            c++;
        }
        return buffer;
    }



    #define ZMALLOC_ASSERT(expr, desc) if (!(expr)) \
            if (true) \
            {\
                std::this_thread::sleep_for(std::chrono::milliseconds(1000)); \
                 *(volatile u64*)NULL; \
            }

    void zmalloc_debug_chunk(zmalloc::chunk_type* c)
    {
        ZMALLOC_ASSERT(c, "chunk is NULL");
        ZMALLOC_ASSERT(zmalloc_check_fence(c), "good fence");
        ZMALLOC_ASSERT(zmalloc_check_fence(zmalloc_next_chunk(c)), "good next fence");
        ZMALLOC_ASSERT(zmalloc_check_fence(zmalloc_front_chunk(c)), "good front fence");
        ZMALLOC_ASSERT(zmalloc_next_chunk(c)->prev_size == c->this_size, "good prev_size");
        ZMALLOC_ASSERT(c->prev_size == zmalloc_front_chunk(c)->this_size, "good prev_size");

        ZMALLOC_ASSERT(zmalloc_chunk_level(c) == zmalloc_chunk_level(zmalloc_next_chunk(c)), "good level");
        ZMALLOC_ASSERT(zmalloc_chunk_level(c) == zmalloc_chunk_level(zmalloc_front_chunk(c)), "good level");
        ZMALLOC_ASSERT(c->this_size >= zmalloc::SMALL_LEAST_SIZE, "good this size");
        if (!zmalloc_has_chunk(c, zmalloc::CHUNK_IS_DIRECT))
        {
            ZMALLOC_ASSERT(c->this_size <= zmalloc::DEFAULT_BLOCK_SIZE - sizeof(zmalloc::block_type) - sizeof(zmalloc::free_chunk_type) * 2, "good this size");
            ZMALLOC_ASSERT(c->this_size + zmalloc_next_chunk(c)->this_size + zmalloc_front_chunk(c)->this_size <= zmalloc::DEFAULT_BLOCK_SIZE - (u32)sizeof(zmalloc::block_type), "good this size");
        }

        if (zmalloc_chunk_level(c) > 0)
        {
            ZMALLOC_ASSERT(c->this_size >= zmalloc::SMALL_MAX_REQUEST + zmalloc::CHUNK_PADDING_SIZE, "good big size");
            ZMALLOC_ASSERT(c->this_size >= 1024 + zmalloc::CHUNK_PADDING_SIZE, "good big size");
        }
    }

    void zmalloc_debug_free_chunk(zmalloc::free_chunk_type* c)
    {
        zmalloc_debug_chunk(c);
        ZMALLOC_ASSERT(!zmalloc_chunk_in_use(c), "free chunk");
        ZMALLOC_ASSERT(c->bin_id < 64, "bin id");
        ZMALLOC_ASSERT(!(!zmalloc_chunk_in_use(c) && !zmalloc_chunk_in_use(zmalloc_next_chunk(c))), "good in use");
        ZMALLOC_ASSERT(!(!zmalloc_chunk_in_use(c) && !zmalloc_chunk_in_use(zmalloc_front_chunk(c))), "good in use");
    }

    void zmalloc_debug_free_chunk_list(zmalloc& zstate, zmalloc::free_chunk_type* c)
    {
        zmalloc_debug_chunk(c);
        if (c != zstate.dv_[zmalloc_chunk_level(c)])
        {
            ZMALLOC_ASSERT((u64)c->next_node < ((~0ULL) >> 0x4), "free pointer");
            ZMALLOC_ASSERT((u64)c->prev_node < ((~0ULL) >> 0x4), "free pointer");
            ZMALLOC_ASSERT(zstate.bitmap_[zmalloc_chunk_level(c)] & 1ULL << c->bin_id, "has bitmap flag");
            ZMALLOC_ASSERT(zstate.bin_[zmalloc_chunk_level(c)][c->bin_id].next_node, "has bin pointer");
            bool found_this = false;
            zmalloc::free_chunk_type* head = zstate.bin_[zmalloc_chunk_level(c)][c->bin_id].next_node;
            while (head)
            {
                if (c == head)
                {
                    found_this = true;
                    break;
                }
                if (c == &zstate.bin_end_[zmalloc_chunk_level(c)][c->bin_id])
                {
                    break;
                }
                head = head->next_node;
            }
            ZMALLOC_ASSERT(found_this, "found in bin");
            ZMALLOC_ASSERT(c->this_size > zmalloc::CHUNK_PADDING_SIZE, "chunk size too small");
            if (zmalloc_chunk_level(c))
            {
                u32 index_size = zmalloc_resolve_order_size(c->bin_id);
                (void)index_size;
                ZMALLOC_ASSERT(c->this_size >= zmalloc_resolve_order_size(c->bin_id) + zmalloc::CHUNK_PADDING_SIZE, "chunk size too small");
                ZMALLOC_ASSERT(c->this_size < zmalloc_resolve_order_size(c->bin_id + 1) + zmalloc::CHUNK_PADDING_SIZE, "chunk size too large");
            }
            else
            {
                ZMALLOC_ASSERT((c->this_size - zmalloc::CHUNK_PADDING_SIZE) >= (u32)(c->bin_id) << zmalloc::FINE_GRAINED_SHIFT, "chunk size too large");
                if (c->this_size < 63 * zmalloc_order_size(zmalloc::FINE_GRAINED_SHIFT) + zmalloc::CHUNK_PADDING_SIZE)
                {
                    ZMALLOC_ASSERT((c->this_size - zmalloc::CHUNK_PADDING_SIZE) < (u32)(c->bin_id + 1) << zmalloc::FINE_GRAINED_SHIFT, "chunk size too small");
                }
            }
        }
    }
    void zmalloc_debug_block_list(zmalloc::block_type* block_list, u32 block_list_size, u32 max_list_size)
    {
        if (block_list == NULL)
        {
            ZMALLOC_ASSERT(block_list_size == 0, "reserve size");
        }
        else
        {
            ZMALLOC_ASSERT(block_list_size > 0, "reserve size");
        }
        ZMALLOC_ASSERT(block_list_size <= max_list_size, "reserve size");


        u32 detect_size = 0;
        zmalloc::block_type* block = block_list;
        zmalloc::block_type* front_block = block;
        while (block)
        {
            ZMALLOC_ASSERT(zmalloc_is_power_of_2(block->block_size), "align");


            if (zmalloc_has_chunk(zmalloc_get_first_chunk(block), zmalloc::CHUNK_IS_DIRECT))
            {

            }
            else
            {
                ZMALLOC_ASSERT(block->block_size >= zmalloc::DEFAULT_BLOCK_SIZE, "align");
                ZMALLOC_ASSERT(block->block_size >= zmalloc::BIG_MAX_REQUEST, "align");
            }

            detect_size++;
            front_block = block;
            block = block->next;
        }
        ZMALLOC_ASSERT(detect_size == block_list_size, "reserve size");

        block = front_block;
        while (front_block)
        {
            detect_size--;
            block = front_block;
            front_block = front_block->front;
        }
        ZMALLOC_ASSERT(detect_size == 0, "reserve size");
        ZMALLOC_ASSERT(block == block_list, "reserve size");
    }

    void zmalloc_debug_block(zmalloc& zstate)
    {
        zmalloc_debug_block_list(zstate.reserve_block_list_, zstate.reserve_block_count_, zstate.max_reserve_block_count_);
        zmalloc_debug_block_list(zstate.used_block_list_, zstate.used_block_count_, ~0U);

        zmalloc::block_type* block = zstate.used_block_list_;
        zmalloc::block_type* last_block = block;
        u32 c_count = 0;
        u32 fc_count = 0;
        block = last_block;
        while (block)
        {
            u32 block_bytes = 0;
            u32 block_c_count = 0;
            u32 block_fc_count = 0;
            zmalloc::chunk_type* c = zmalloc_get_first_chunk(block);
            while (c)
            {
                c_count++;
                block_c_count++;
                block_fc_count += zmalloc_chunk_in_use(c) ? 0 : 1;
                fc_count += zmalloc_chunk_in_use(c) ? 0 : 1;
                block_bytes += c->this_size;
                if (zmalloc_chunk_is_dirct(c))
                {
                    ZMALLOC_ASSERT(zmalloc_chunk_in_use(zmalloc_front_chunk(c)) && zmalloc_chunk_in_use(zmalloc_next_chunk(c)), "bound chunk");
                    ZMALLOC_ASSERT(zmalloc_front_chunk(c)->this_size == zmalloc_next_chunk(c)->this_size, "bound size");
                }
                else if (!zmalloc_chunk_in_use(c) && c != zstate.dv_[zmalloc_chunk_level(c)])
                {
                    ZMALLOC_ASSERT((1ULL << c->bin_id) & zstate.bitmap_[zmalloc_chunk_level(c)], "bitmap");
                    ZMALLOC_ASSERT(zstate.bin_[zmalloc_chunk_level(c)][c->bin_id].next_node, "bin");
                    zmalloc::free_chunk_type* fcb = zstate.bin_[zmalloc_chunk_level(c)][c->bin_id].next_node;
                    bool found = false;
                    while (fcb != &zstate.bin_end_[zmalloc_chunk_level(c)][c->bin_id])
                    {
                        CHECK_FCL(zstate, fcb);
                        if (fcb == c)
                        {
                            found = true;
                        }
                        fcb = fcb->next_node;
                    }
                    ZMALLOC_ASSERT(found, "found in bin");
                }

                if (block_bytes + zmalloc::BLOCK_TYPE_SIZE + (u32)sizeof(zmalloc::free_chunk_type) * 2U == block->block_size)
                {
                    if (zmalloc_free_chunk_cast(zmalloc_next_chunk(c))->this_size == sizeof(zmalloc::free_chunk_type)
                        && zmalloc_chunk_in_use(zmalloc_free_chunk_cast(zmalloc_next_chunk(c))))
                    {
                        break;
                    }
                }
                ZMALLOC_ASSERT(block_bytes + zmalloc::BLOCK_TYPE_SIZE + (u32)sizeof(zmalloc::free_chunk_type) * 2U <= block->block_size, "max block");
                c = zmalloc_next_chunk(c);
            };
            last_block = block;
            block = block->front;
        }
        //LogInfo() << "check all block success. block count:" << block_count << ", total chunk:" << c_count <<", free chunk:" << fc_count;
    }

    void zmalloc_debug_bitmap(zmalloc& zstate)
    {
        for (u32 small_type = 0; small_type < 2; small_type++)
        {
            for (u32 bin_id = 0; bin_id < zmalloc::BINMAP_SIZE; bin_id++)
            {
                zmalloc::free_chunk_type* zmalloc_free_chunk_cast = zstate.bin_[small_type][bin_id].next_node;
                if (zmalloc_free_chunk_cast != &zstate.bin_end_[small_type][bin_id])
                {
                    ZMALLOC_ASSERT(zmalloc_has_bitmap(zstate.bitmap_[small_type], bin_id), "has bitmap");
                }
                else
                {
                    ZMALLOC_ASSERT(!zmalloc_has_bitmap(zstate.bitmap_[small_type], bin_id), "has bitmap");
                }
                while (zmalloc_free_chunk_cast != &zstate.bin_end_[small_type][bin_id])
                {
                    ZMALLOC_ASSERT(zmalloc_free_chunk_cast->bin_id == bin_id, "bin id");
                    ZMALLOC_ASSERT(!zmalloc_chunk_in_use(zmalloc_free_chunk_cast), "free");
                    zmalloc_free_chunk_cast = zmalloc_free_chunk_cast->next_node;
                }
            }
        }
    }

  
    }
#endif