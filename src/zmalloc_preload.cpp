#include "zmalloc_mt.h"
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <new>
#include <thread>

// Platform-specific headers: kept to a small, localized block.
#if defined(_WIN32)
// only win
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// Export macro, same style as PRELOAD_DUMMY_API.
#if defined(_WIN32)
#define ZMALLOC_PRELOAD_API extern "C" __declspec(dllexport)
#else
#define ZMALLOC_PRELOAD_API extern "C"
#endif

static zmalloc_mt g_zmalloc_mt_instance;
static std::atomic<bool> g_inited{false};
static std::atomic<bool> g_initializing{false};

static void ensure_init()
{
    if (g_inited.load(std::memory_order_acquire))
    {
        return;
    }

    bool expected = false;
    if (!g_initializing.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
    {
        while (!g_inited.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
        return;
    }

    memset(&g_zmalloc_mt_instance, 0, sizeof(g_zmalloc_mt_instance));
    zmalloc_mt::set_global(&g_zmalloc_mt_instance);
    g_zmalloc_mt_instance.init();
    g_inited.store(true, std::memory_order_release);
}

static inline bool is_zmalloc_ptr(void* ptr)
{
    if (ptr == nullptr)
    {
        return false;
    }
    zmalloc::chunk_type* chunk = zmalloc_chunk_cast(zmalloc_u64_cast(ptr) - zmalloc::kChunkPaddingSize);
    return chunk->fence == zmalloc::kChunkFence;
}

ZMALLOC_PRELOAD_API void* malloc(size_t size)
{
    if (!g_inited.load(std::memory_order_acquire))
    {
        ensure_init();
    }
    return g_zmalloc_mt_instance.alloc_memory(size);
}

ZMALLOC_PRELOAD_API void free(void* ptr)
{
    if (ptr == nullptr)
    {
        return;
    }
    if (!g_inited.load(std::memory_order_acquire))
    {
        return;
    }
    if (!is_zmalloc_ptr(ptr))
    {
        return;
    }
    g_zmalloc_mt_instance.free_memory(ptr);
}

ZMALLOC_PRELOAD_API void* calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr)
    {
        memset(ptr, 0, total);
    }
    return ptr;
}

ZMALLOC_PRELOAD_API void* realloc(void* ptr, size_t size)
{
    if (ptr == nullptr)
    {
        return malloc(size);
    }
    if (size == 0)
    {
        free(ptr);
        return nullptr;
    }

    if (!is_zmalloc_ptr(ptr))
    {
        void* new_ptr = malloc(size);
        return new_ptr;
    }

    void* new_ptr = malloc(size);
    if (new_ptr == nullptr)
    {
        return nullptr;
    }

    zmalloc::chunk_type* chunk = zmalloc_chunk_cast(zmalloc_u64_cast(ptr) - zmalloc::kChunkPaddingSize);
    u32 old_size = chunk->this_size - zmalloc::kChunkPaddingSize;
    size_t copy_size = old_size < size ? old_size : size;
    memcpy(new_ptr, ptr, copy_size);
    free(ptr);
    return new_ptr;
}

// POSIX-style aligned allocators. zmalloc_mt::alloc_memory already guarantees
// 16-byte alignment, which satisfies typical alignment requests in practice;
// these forwarders therefore intentionally ignore the alignment argument and
// share a single body across platforms (exported with the same names on Win32
// for API parity).
ZMALLOC_PRELOAD_API void* memalign(size_t alignment, size_t size)
{
    (void)alignment;
    return malloc(size);
}

ZMALLOC_PRELOAD_API int posix_memalign(void** memptr, size_t alignment, size_t size)
{
    (void)alignment;
    void* ptr = malloc(size);
    if (ptr == nullptr)
    {
        if (memptr)
        {
            *memptr = nullptr;
        }
        return ENOMEM;
    }
    *memptr = ptr;
    return 0;
}

ZMALLOC_PRELOAD_API void* valloc(size_t size)
{
    return malloc(size);
}

ZMALLOC_PRELOAD_API void* pvalloc(size_t size)
{
    return malloc(size);
}
