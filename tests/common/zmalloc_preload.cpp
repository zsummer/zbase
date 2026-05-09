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

// Debug CRT wrappers for Windows /MDd builds
// These functions ignore the debug parameters and forward to the standard implementations
#if defined(_WIN32)
ZMALLOC_PRELOAD_API void* _malloc_dbg(size_t size, int blockType, const char* filename, int line)
{
    (void)blockType;
    (void)filename;
    (void)line;
    return malloc(size);
}

ZMALLOC_PRELOAD_API void _free_dbg(void* ptr, int blockType)
{
    (void)blockType;
    free(ptr);
}

ZMALLOC_PRELOAD_API void* _calloc_dbg(size_t nmemb, size_t size, int blockType, const char* filename, int line)
{
    (void)blockType;
    (void)filename;
    (void)line;
    return calloc(nmemb, size);
}

ZMALLOC_PRELOAD_API void* _realloc_dbg(void* ptr, size_t size, int blockType, const char* filename, int line)
{
    (void)blockType;
    (void)filename;
    (void)line;
    return realloc(ptr, size);
}
#endif

// On Windows host.exe and zmalloc_preload.dll each get their own copy of
// zmalloc_mt's inline-static members (instance_ptr / arena_count_ / ...) in
// their own .data section, so reading instance_ptr() from host always sees
// NULL. This exported diagnostic lets the host fetch the dll-side stats via
// GetProcAddress and use them for regression assertions. Linux LD_PRELOAD
// does not exhibit this split, so the helper is _WIN32-only.
#if defined(_WIN32)
struct zmalloc_preload_stats
{
    void* instance;
    unsigned int arena_count;
    unsigned int error_count;
    unsigned long long runtime_errors;
};

ZMALLOC_PRELOAD_API void zmalloc_preload_get_stats(zmalloc_preload_stats* out)
{
    if (out == NULL)
    {
        return;
    }
    out->instance = NULL;
    out->arena_count = 0;
    out->error_count = 0;
    out->runtime_errors = 0;

    if (!g_inited.load(std::memory_order_acquire))
    {
        return;
    }
    zmalloc_mt* inst = zmalloc_mt::instance_ptr();
    out->instance = (void*)inst;
    if (inst == NULL)
    {
        return;
    }
    out->arena_count = inst->arena_count_.load(std::memory_order_relaxed);
    out->error_count = inst->error_count_;
    unsigned long long rt = 0;
    for (unsigned int i = 0; i < ZMALLOC_MT_MAX_ARENAS; i++)
    {
        rt += inst->arenas_[i].allocator.runtime_errors_;
    }
    out->runtime_errors = rt;
}
#endif