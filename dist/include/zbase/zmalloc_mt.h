

/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once 
#ifndef ZMALLOC_MT_H
#define ZMALLOC_MT_H

#include "zmalloc.h"
#include <mutex>
#include <atomic>
#include <thread>


// 强制 initial-exec TLS 模型: 让 thread_local 直接走 %fs:offset,
// 不再调 __tls_get_addr, 从而不可能间接触发 malloc.
// 仅适用于启动期 load 的 so (LD_PRELOAD 注入或主程序直接链接),
// 这正是 libzmalloc_preload.so 的使用场景.
// MSVC / Windows 下退化为空 (无 __tls_get_addr 也无 LD_PRELOAD 链路).
#if (defined(__GNUC__) || defined(__clang__)) && !defined(_WIN32)
#define ZMALLOC_TLS_MODEL_IE __attribute__((tls_model("initial-exec")))
#else
#define ZMALLOC_TLS_MODEL_IE
#endif


/* ============================================================
 *  基于zmalloc进行多线程支持, 思路借鉴参考ptmalloc的多线程方案
 *
 *  核心思路:
 *    1. 多 arena: 每个 arena 内含一个独立的 zmalloc 实例 + 一把 mutex
 *    2. thread-local 绑定: 每个线程通过 thread_local 缓存绑定到某个 arena,
 *       alloc 时优先 trylock 自己的 arena, 失败则遍历或创建新 arena
 *    3. 自动创建 arena: arena 数量随线程竞争自动增长, 上限可配置
 *    4. free 路径: 通过遍历 arena 的 block 列表找到所属 arena, 加锁后归还
 *    5. thread-local cache: 每个线程维护一个小型 free chunk 缓存,
 *       小对象 free 时先放入 cache, alloc 时先从 cache 取, 减少锁操作
 * ============================================================ */


//todo 
// 3. 日志打印需要优化  


// 最大 arena 数量
#ifndef ZMALLOC_MT_MAX_ARENAS
#define ZMALLOC_MT_MAX_ARENAS 64
#endif

// 每线程 cache 的 bin 数量 (覆盖 small 分配的 bin 范围)
#ifndef ZMALLOC_MT_CACHE_BIN_COUNT
#define ZMALLOC_MT_CACHE_BIN_COUNT 64
#endif

// 每个 cache bin 的最大缓存 chunk 数
#ifndef ZMALLOC_MT_CACHE_BIN_MAX
#define ZMALLOC_MT_CACHE_BIN_MAX 8
#endif

// cache 总字节上限 (超过后触发 flush)
#ifndef ZMALLOC_MT_CACHE_MAX_BYTES
#define ZMALLOC_MT_CACHE_MAX_BYTES (256 * 1024)
#endif



struct zmalloc_arena
{
    std::mutex          mtx;            
    zmalloc             allocator;      
    std::atomic<u32>    thread_count;   
    u32                 arena_id;      
    u32                 inited;         
    u32                 pad_[1];     

    inline s32 init(u32 id)
    {
        arena_id = id;
        thread_count.store(0, std::memory_order_relaxed);
        inited = 1;
        return allocator.init();
    }
};

struct zmalloc_cache_bin
{
    zmalloc::free_chunk_type*   head;       
    u32                         count;     
    u32                         pad_;
};

class zmalloc_mt; 

class zmalloc_thread_cache
{
public:
    zmalloc_mt*         owner_;
    zmalloc_arena*      bound_arena_;   
    u64                 cached_bytes_;
    u64                 alloc_count_;
    u64                 free_count_;
    u64                 alloc_bytes_;
    u64                 free_bytes_;
    u32                 inited_;
    u32                 pad_;
    zmalloc_cache_bin   bins_[ZMALLOC_MT_CACHE_BIN_COUNT];

    zmalloc_thread_cache()
    {
        memset(this, 0, sizeof(zmalloc_thread_cache));
    }
    zmalloc_thread_cache(const zmalloc_thread_cache&) = delete;
    zmalloc_thread_cache& operator=(const zmalloc_thread_cache&) = delete;
    zmalloc_thread_cache(zmalloc_thread_cache&&) = delete;
    zmalloc_thread_cache& operator=(zmalloc_thread_cache&&) = delete;


    inline ~zmalloc_thread_cache();


    inline void init(zmalloc_mt* owner)
    {
        owner_ = owner;
        bound_arena_ = nullptr;
        cached_bytes_ = 0;
        alloc_count_ = 0;
        free_count_ = 0;
        alloc_bytes_ = 0;
        free_bytes_ = 0;
        memset(bins_, 0, sizeof(bins_));
        inited_ = 1;
    }

    inline bool push(zmalloc::free_chunk_type* chunk, u32 bin_id)
    {
        if (bin_id >= ZMALLOC_MT_CACHE_BIN_COUNT)
        {
            return false;
        }
        if (bins_[bin_id].count >= ZMALLOC_MT_CACHE_BIN_MAX)
        {
            return false;
        }
        if (cached_bytes_ >= ZMALLOC_MT_CACHE_MAX_BYTES)
        {
            return false;
        }

        // no use pre node; needn't merge free chunk. 
        chunk->next_node = bins_[bin_id].head;
        bins_[bin_id].head = chunk;
        bins_[bin_id].count++;
        alloc_count_++;
        alloc_bytes_ += chunk->this_size;
        cached_bytes_ += chunk->this_size;
        return true;
    }


    inline zmalloc::free_chunk_type* pop(u32 bin_id)
    {
        if (bin_id >= ZMALLOC_MT_CACHE_BIN_COUNT)
        {
            return nullptr;
        }
            
        if (bins_[bin_id].count == 0)
        {
            return nullptr;
        }

        zmalloc::free_chunk_type* chunk = bins_[bin_id].head;
        bins_[bin_id].head = chunk->next_node;
        bins_[bin_id].count--;
        cached_bytes_ -= chunk->this_size;
        free_count_++;
		free_bytes_ += chunk->this_size;
        chunk->next_node = nullptr;
        return chunk;
    }
};



class zmalloc_mt
{
    friend class zmalloc_thread_cache;
public:
    using block_alloc_func = zmalloc::block_alloc_func;
    using block_free_func = zmalloc::block_free_func;



    inline static u32 zmalloc_size() { return sizeof(zmalloc_mt); }
    inline static zmalloc_mt& instance() { return *instance_ptr(); }
    inline static zmalloc_mt*& instance_ptr() { static zmalloc_mt* g_zmalloc_state = NULL; return g_zmalloc_state; }
    inline static void set_global(zmalloc_mt* state) { instance_ptr() = state; }


    inline static void* default_block_alloc(u64 bytes) { return zmalloc::default_block_alloc(bytes); }
    inline static u64 default_block_free(void* addr, u64 bytes) { (void)bytes; return zmalloc::default_block_free(addr); }
    inline void set_block_callback(block_alloc_func block_alloc, block_free_func block_free);


    inline s32 init();


    template<u16 COLOR = 0>
    inline void* alloc_memory(u64 bytes);
    inline u64  free_memory(void* addr);


    inline void* alloc_slot(u16 slot_id, u64 bytes, u64 limit_block_size);
    inline u64 free_slot(void* addr);


    inline s32 check_health();
    inline void check_panic();
    inline void clear_cache();
    inline void flush_and_reset_thread_cache();
    template<class StreamLog>
    inline void debug_state_log(StreamLog logwrap);
    template<class StreamLog>
    inline void debug_color_log(StreamLog logwrap, u32 begin_color, u32 end_color);


private:

    inline zmalloc_arena* create_arena();
    inline zmalloc_arena* find_arena();
    inline zmalloc_arena* find_arena_for_chunk(zmalloc::chunk_type* chunk);


    inline static zmalloc_thread_cache& get_thread_cache();
    inline void flush_cache(zmalloc_thread_cache& cache);
    inline void flush_cache_bin(zmalloc_thread_cache& cache, u32 bin_id);

public:
    std::mutex          arena_list_mtx_;        
    zmalloc_arena       arenas_[ZMALLOC_MT_MAX_ARENAS];  
    std::atomic<u32>    arena_count_;           
    u32                 max_arena_count_;       

    block_alloc_func    block_alloc_;
    block_free_func     block_free_;
    u32                 inited_;
    u32                 error_count_;
};



#define global_zmalloc_mt(bytes) zmalloc_mt::instance().alloc_memory(bytes)
#define global_zfree_mt(addr) zmalloc_mt::instance().free_memory(addr)



inline zmalloc_thread_cache& zmalloc_mt::get_thread_cache()
{
    static thread_local zmalloc_thread_cache tls_cache ZMALLOC_TLS_MODEL_IE;
    return tls_cache;
}



inline s32 zmalloc_mt::init()
{
    if (inited_)
    {
        return 0;
    }

    new (&arena_list_mtx_) std::mutex();
    for (u32 i = 0; i < ZMALLOC_MT_MAX_ARENAS; i++)
    {
        new (&arenas_[i].mtx) std::mutex();
        new (&arenas_[i].thread_count) std::atomic<u32>(0);
        arenas_[i].inited = 0;
    }
    new (&arena_count_) std::atomic<u32>(0);

    if (!block_alloc_)
    {
        block_alloc_ = default_block_alloc;
    }

    if (!block_free_)
    {
        block_free_ = default_block_free;
    }


    u32 hw_threads = std::thread::hardware_concurrency();
    if (hw_threads == 0)
    {
        hw_threads = 4;
    }
    max_arena_count_ = hw_threads * 2;
    if (max_arena_count_ > ZMALLOC_MT_MAX_ARENAS)
    {
        max_arena_count_ = ZMALLOC_MT_MAX_ARENAS;
    }


    inited_ = 1;
    error_count_ = 0;

    create_arena();
    return 0;
}

inline void zmalloc_mt::set_block_callback(block_alloc_func block_alloc, block_free_func block_free)
{
    block_alloc_ = block_alloc;
    block_free_ = block_free;
    u32 count = arena_count_.load(std::memory_order_acquire);
    for (u32 i = 0; i < count; i++)
    {
        std::lock_guard<std::mutex> lock(arenas_[i].mtx);
        arenas_[i].allocator.set_block_callback(block_alloc, block_free);
    }
}


inline zmalloc_arena* zmalloc_mt::create_arena()
{
    std::lock_guard<std::mutex> lock(arena_list_mtx_);
    u32 count = arena_count_.load(std::memory_order_relaxed);
    if (count >= max_arena_count_)
    {
        return nullptr;
    }

    zmalloc_arena* arena = &arenas_[count];
    if (block_alloc_)
    {
        arena->allocator.set_block_callback(block_alloc_, block_free_);
    }

    arena->init(count);
    arena_count_.store(count + 1, std::memory_order_release);
    return arena;
}


inline zmalloc_arena* zmalloc_mt::find_arena()
{
    zmalloc_thread_cache& cache = get_thread_cache();
    if (!cache.inited_)
    {
        cache.init(this);
    }

    if (cache.bound_arena_ != nullptr)
    {
        if (cache.bound_arena_->mtx.try_lock())
        {
            return cache.bound_arena_;
        }
    }

    u32 count = arena_count_.load(std::memory_order_acquire);
    u32 min_threads = ~0U;
    zmalloc_arena* best_arena = nullptr;

    for (u32 i = 0; i < count; i++)
    {
        // not best 
        if (arenas_[i].mtx.try_lock())
        {
            if (cache.bound_arena_ && cache.bound_arena_ != &arenas_[i])
            {
                cache.bound_arena_->thread_count.fetch_sub(1, std::memory_order_relaxed);
            }
            cache.bound_arena_ = &arenas_[i];
            cache.bound_arena_->thread_count.fetch_add(1, std::memory_order_relaxed);
            return &arenas_[i];
        }

        u32 tc = arenas_[i].thread_count.load(std::memory_order_relaxed);
        if (tc < min_threads)
        {
            min_threads = tc;
            best_arena = &arenas_[i];
        }
    }

    if (count < max_arena_count_)
    {
        zmalloc_arena* new_arena = create_arena();
        if (new_arena != nullptr)
        {
            new_arena->mtx.lock();
            if (cache.bound_arena_)
            {
                cache.bound_arena_->thread_count.fetch_sub(1, std::memory_order_relaxed);
            }

            cache.bound_arena_ = new_arena;
            cache.bound_arena_->thread_count.fetch_add(1, std::memory_order_relaxed);
            return new_arena;
        }
    }

    //worst 
    if (best_arena == nullptr)
    {
        best_arena = &arenas_[0];
    }

    best_arena->mtx.lock();
    if (cache.bound_arena_ && cache.bound_arena_ != best_arena)
    {
        cache.bound_arena_->thread_count.fetch_sub(1, std::memory_order_relaxed);
    }

    cache.bound_arena_ = best_arena;
    cache.bound_arena_->thread_count.fetch_add(1, std::memory_order_relaxed);
    return best_arena;
}


inline zmalloc_arena* zmalloc_mt::find_arena_for_chunk(zmalloc::chunk_type* chunk)
{
    u32 id = chunk->arena_id;
    u32 count = arena_count_.load(std::memory_order_acquire);
    if (id < count)
    {
        return &arenas_[id];
    }
    return nullptr;
}


template<u16 COLOR>
inline void* zmalloc_mt::alloc_memory(u64 bytes)
{
    if (!inited_)
    {
        init();
    }

    zmalloc_thread_cache& cache = get_thread_cache();
    if (!cache.inited_)
    {
        cache.init(this);
    }


    if (bytes < zmalloc::kSmallMaxRequest - zmalloc::kFineGrainedSize)
    {
        u32 padding = bytes < zmalloc::kFineGrainedSize ? zmalloc::kFineGrainedSize : (u32)bytes + zmalloc::kFineGrainedMask;
        u32 bin_id = padding >> zmalloc::kFineGrainedShift;

        zmalloc::free_chunk_type* cached_chunk = cache.pop(bin_id);
        if (cached_chunk != nullptr)
        {
            cached_chunk->fence = zmalloc::kChunkFence;
#ifdef ZDEBUG_UNINIT_MEMORY
            memset((void*)(zmalloc_u64_cast(cached_chunk) + zmalloc::kChunkPaddingSize), 0xfd,
                   cached_chunk->this_size - zmalloc::kChunkPaddingSize);
#endif
            return (void*)(zmalloc_u64_cast(cached_chunk) + zmalloc::kChunkPaddingSize);
        }
    }

    zmalloc_arena* arena = find_arena();
    void* ptr = arena->allocator.alloc_memory<COLOR>(bytes);
    if (ptr != nullptr)
    {
        zmalloc::chunk_type* chunk = zmalloc_chunk_cast(zmalloc_u64_cast(ptr) - zmalloc::kChunkPaddingSize);
        chunk->arena_id = (u8)arena->arena_id;
    }
    arena->mtx.unlock();
    return ptr;
}


inline u64 zmalloc_mt::free_memory(void* addr)
{
    if (addr == nullptr)
    {
        return 0;
    }

    zmalloc::free_chunk_type* chunk = zmalloc_free_chunk_cast(zmalloc_u64_cast(addr) - zmalloc::kChunkPaddingSize);

    if (chunk->fence != zmalloc::kChunkFence)
    {
        error_count_++;
        return 0;
    }

    if (chunk->arena_id >= arena_count_.load(std::memory_order_acquire))
    {
        error_count_++;
        return 0;
    }

    if (!zmalloc_chunk_is_dirct(chunk))
    {
        u32 level = zmalloc_chunk_level(chunk);
        if (level == 0) // small chunk
        {
            zmalloc_thread_cache& cache = get_thread_cache();
            if (!cache.inited_)
            {
                cache.init(this);
            }

            u32 bin_id = chunk->bin_id;
            u64 bytes = chunk->this_size - zmalloc::kChunkPaddingSize;

#ifdef ZDEBUG_DEATH_MEMORY
            {
                char* clean_addr = ((char*)chunk) + sizeof(zmalloc::chunk_type);
                u32 size = chunk->this_size - sizeof(zmalloc::chunk_type);
                memset(clean_addr, 0xdf, size);
            }
#endif
            if (cache.push(chunk, bin_id))
            {
                if (cache.cached_bytes_ >= ZMALLOC_MT_CACHE_MAX_BYTES)
                {
                    flush_cache(cache);
                }
                return bytes;
            }
        }
    }


    zmalloc_arena* arena = find_arena_for_chunk( zmalloc_chunk_cast(zmalloc_u64_cast(addr) - zmalloc::kChunkPaddingSize));
    if (arena == nullptr)
    {
        error_count_++;
        return 0;
    }

    std::lock_guard<std::mutex> lock(arena->mtx);
    return arena->allocator.free_memory(addr);
}



inline void zmalloc_mt::flush_cache(zmalloc_thread_cache& cache)
{
    for (u32 i = 0; i < ZMALLOC_MT_CACHE_BIN_COUNT; i++)
    {
        flush_cache_bin(cache, i);
    }
}

inline void zmalloc_mt::flush_cache_bin(zmalloc_thread_cache& cache, u32 bin_id)
{
    while (cache.bins_[bin_id].count > 0)
    {
        zmalloc::free_chunk_type* chunk = cache.pop(bin_id);
        if (chunk == nullptr)
        {
            break;
        }

        void* addr = (void*)(zmalloc_u64_cast(chunk) + zmalloc::kChunkPaddingSize);
        zmalloc_arena* arena = find_arena_for_chunk(zmalloc_chunk_cast(chunk));
        if (arena != nullptr)
        {
            std::lock_guard<std::mutex> lock(arena->mtx);
            arena->allocator.free_memory(addr);
        }
        else
        {
            error_count_++;
        }
    }
}



inline void* zmalloc_mt::alloc_slot(u16 slot_id, u64 bytes, u64 limit_block_size)
{
    if (!inited_)
    {
        init();
    }
    zmalloc_arena* arena = find_arena();  // 返回时 arena->mtx 已锁
    void* ptr = arena->allocator.alloc_slot(slot_id, bytes, limit_block_size);
    if (ptr != nullptr)
    {
        zmalloc::chunk_type* chunk = zmalloc_chunk_cast(zmalloc_u64_cast(ptr) - zmalloc::kChunkPaddingSize);
        chunk->arena_id = (u8)arena->arena_id;
    }
    arena->mtx.unlock();
    return ptr;
}


inline u64 zmalloc_mt::free_slot(void* addr)
{
    if (addr == nullptr)
    {
        return 0;
    }

    zmalloc::chunk_type* chunk = zmalloc_chunk_cast(zmalloc_u64_cast(addr) - zmalloc::kChunkPaddingSize);
    zmalloc_arena* arena = find_arena_for_chunk(chunk);
    if (arena == nullptr)
    {
        return 0;
    }
    std::lock_guard<std::mutex> lock(arena->mtx);
    return arena->allocator.free_slot(addr);
}



inline s32 zmalloc_mt::check_health()
{
    s32 total_errors = 0;
    u32 count = arena_count_.load(std::memory_order_acquire);
    for (u32 i = 0; i < count; i++)
    {
        std::lock_guard<std::mutex> lock(arenas_[i].mtx);
        s32 ret = arenas_[i].allocator.check_health();
        if (ret > 0)
        {
            total_errors += ret;
        }
    }
    return total_errors;
}

inline void zmalloc_mt::check_panic()
{
    u32 count = arena_count_.load(std::memory_order_acquire);
    for (u32 i = 0; i < count; i++)
    {
        std::lock_guard<std::mutex> lock(arenas_[i].mtx);
        arenas_[i].allocator.check_panic();
    }
}

inline void zmalloc_mt::clear_cache()
{
    u32 count = arena_count_.load(std::memory_order_acquire);
    for (u32 i = 0; i < count; i++)
    {
        std::lock_guard<std::mutex> lock(arenas_[i].mtx);
        arenas_[i].allocator.clear_cache();
    }
}

inline void zmalloc_mt::flush_and_reset_thread_cache()
{
    zmalloc_thread_cache& cache = get_thread_cache();
    if (!cache.inited_)
    {
        return;
    }

    flush_cache(cache);

    if (cache.bound_arena_ != nullptr)
    {
        cache.bound_arena_->thread_count.fetch_sub(1, std::memory_order_relaxed);
        cache.bound_arena_ = nullptr;
    }
    cache.inited_ = 0;
}


inline zmalloc_thread_cache::~zmalloc_thread_cache()
{
    if (!inited_)
    {
        return;
    }
    


    zmalloc_mt* mt = owner_;
    if (mt == nullptr || !mt->inited_)
    {
        // zmalloc_mt already destroyed or not initialized,
        // cannot safely flush cached chunks, just abandon them
        bound_arena_ = nullptr;
        inited_ = 0;
        return;
    }

    mt->flush_cache(*this);

    if (bound_arena_ != nullptr)
    {
        bound_arena_->thread_count.fetch_sub(1, std::memory_order_relaxed);
        bound_arena_ = nullptr;
    }
    inited_ = 0;
}



template<class StreamLog>
inline void zmalloc_mt::debug_state_log(StreamLog logwrap)
{
    u32 count = arena_count_.load(std::memory_order_acquire);
    logwrap() << "========== zmalloc_mt: " << count << " arenas ==========";
    for (u32 i = 0; i < count; i++)
    {
        logwrap() << "--- arena[" << i << "] threads:"
                  << arenas_[i].thread_count.load(std::memory_order_relaxed) << " ---";
        std::lock_guard<std::mutex> lock(arenas_[i].mtx);
        arenas_[i].allocator.debug_state_log(logwrap);
    }
    logwrap() << "========== zmalloc_mt end ==========";
}

template<class StreamLog>
inline void zmalloc_mt::debug_color_log(StreamLog logwrap, u32 begin_color, u32 end_color)
{
    u32 count = arena_count_.load(std::memory_order_acquire);
    logwrap() << "========== zmalloc_mt color log: " << count << " arenas ==========";
    for (u32 i = 0; i < count; i++)
    {
        logwrap() << "--- arena[" << i << "] ---";
        std::lock_guard<std::mutex> lock(arenas_[i].mtx);
        arenas_[i].allocator.debug_color_log(logwrap, begin_color, end_color);
    }
    logwrap() << "========== zmalloc_mt color log end ==========";
}


#endif
