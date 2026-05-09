#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

#include "zmalloc_mt.h"

// ---- platform abstraction (small, localized block) -------------------------
#if defined(_WIN32)
// only win
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#include <limits.h>
#endif

namespace dyn {

#if defined(_WIN32)
using handle = HMODULE;

static inline handle open_lib(const char* path)
{
    // only win
    return LoadLibraryA(path);
}

static inline void* sym(handle h, const char* name)
{
    // only win
    return reinterpret_cast<void*>(GetProcAddress(h, name));
}

static inline int close_lib(handle h)
{
    // only win: FreeLibrary returns nonzero on success, mirror dlclose's
    // "0 == success" convention so the caller can stay platform-agnostic.
    return FreeLibrary(h) ? 0 : -1;
}

static inline std::string last_error()
{
    // only win
    DWORD code = GetLastError();
    char buf[64];
    snprintf(buf, sizeof(buf), "GetLastError=%lu", (unsigned long)code);
    return std::string(buf);
}

static inline std::string self_dir()
{
    // only win
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf))
    {
        return std::string();
    }
    std::string exe(buf, n);
    size_t pos = exe.find_last_of("\\/");
    if (pos == std::string::npos)
    {
        return std::string();
    }
    return exe.substr(0, pos);
}

static inline const char* dummy_lib_name() { return "preload_dummy.dll"; }

#else  // !_WIN32

using handle = void*;

static inline handle open_lib(const char* path)
{
    return dlopen(path, RTLD_NOW);
}

static inline void* sym(handle h, const char* name)
{
    return dlsym(h, name);
}

static inline int close_lib(handle h)
{
    return dlclose(h);
}

static inline std::string last_error()
{
    const char* e = dlerror();
    return e ? std::string(e) : std::string();
}

static inline std::string self_dir()
{
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0)
    {
        return std::string();
    }
    buf[n] = '\0';
    std::string exe(buf);
    size_t pos = exe.find_last_of('/');
    if (pos == std::string::npos)
    {
        return std::string();
    }
    return exe.substr(0, pos);
}

static inline const char* dummy_lib_name() { return "libpreload_dummy.so"; }

#endif

}  // namespace dyn
// ---------------------------------------------------------------------------

static const int kSequentialRounds = 200;
static const int kConcurrentThreads = 4;
static const int kRoundsPerThread = 50;

static std::string resolve_dummy_path()
{
    std::string dir = dyn::self_dir();
    if (dir.empty())
    {
        return std::string(dyn::dummy_lib_name());
    }
    return dir + "/" + dyn::dummy_lib_name();
}

static int do_one_round(const std::string& so_path, int seed)
{
    dyn::handle h = dyn::open_lib(so_path.c_str());
    if (h == nullptr)
    {
        h = dyn::open_lib(dyn::dummy_lib_name());
        if (h == nullptr)
        {
            fprintf(stderr, "open_lib failed: %s | %s\n",
                    so_path.c_str(), dyn::last_error().c_str());
            return -1;
        }
    }
    typedef int (*add_fn)(int, int);
    add_fn fn = reinterpret_cast<add_fn>(dyn::sym(h, "preload_dummy_add"));
    if (fn == nullptr)
    {
        fprintf(stderr, "sym failed: %s\n", dyn::last_error().c_str());
        dyn::close_lib(h);
        return -1;
    }
    int r = fn(seed, seed + 1);
    if (r != seed + seed + 1)
    {
        dyn::close_lib(h);
        return -1;
    }
    void* tmp = malloc(64 + (seed & 0xff));
    if (tmp != nullptr)
    {
        memset(tmp, 0xa5, 64);
        free(tmp);
    }
    if (dyn::close_lib(h) != 0)
    {
        fprintf(stderr, "close_lib failed: %s\n", dyn::last_error().c_str());
        return -1;
    }
    return 0;
}

static std::atomic<int> g_thread_errors{0};

static void thread_proc(const std::string& so_path, int tid)
{
    for (int i = 0; i < kRoundsPerThread; i++)
    {
        if (do_one_round(so_path, tid * 1000 + i) != 0)
        {
            g_thread_errors.fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::string so_path = resolve_dummy_path();
    printf("[preload_dlopen_host] dummy so: %s\n", so_path.c_str());

    // On Linux the host may or may not be launched under LD_PRELOAD; only
    // when the env var is present do we strictly require a non-null instance.
    // On Windows the host explicitly links zmalloc_preload.dll, so the dll's
    // global instance is constructed at load time and must always be present.
#if defined(_WIN32)
    bool preloaded = true;
    printf("[preload_dlopen_host] zmalloc_preload.dll: linked\n");
#else
    bool preloaded = (getenv("LD_PRELOAD") != nullptr);
    printf("[preload_dlopen_host] LD_PRELOAD: %s\n", preloaded ? "yes" : "no");
#endif

    printf("[preload_dlopen_host] sequential phase: %d rounds\n", kSequentialRounds);
    for (int i = 0; i < kSequentialRounds; i++)
    {
        if (do_one_round(so_path, i) != 0)
        {
            fprintf(stderr, "[preload_dlopen_host] sequential round %d failed\n", i);
            return 1;
        }
    }

    printf("[preload_dlopen_host] concurrent phase: %d threads x %d rounds\n",
           kConcurrentThreads, kRoundsPerThread);
    {
        std::vector<std::thread> ts;
        ts.reserve(kConcurrentThreads);
        for (int i = 0; i < kConcurrentThreads; i++)
        {
            ts.emplace_back(thread_proc, std::cref(so_path), i);
        }
        for (auto& t : ts)
        {
            t.join();
        }
    }
    if (g_thread_errors.load() != 0)
    {
        fprintf(stderr, "[preload_dlopen_host] concurrent phase: %d errors\n",
                g_thread_errors.load());
        return 1;
    }

    zmalloc_mt* inst = zmalloc_mt::instance_ptr();
    u64 rt_errors = 0;
    u32 err_count = 0;
    u32 arena_count = 0;
    if (inst != nullptr)
    {
        err_count = inst->error_count_;
        arena_count = inst->arena_count_.load(std::memory_order_relaxed);
        for (u32 i = 0; i < ZMALLOC_MT_MAX_ARENAS; i++)
        {
            if (inst->arenas_[i].inited)
            {
                rt_errors += inst->arenas_[i].allocator.runtime_errors_;
            }
        }
    }
    printf("[preload_dlopen_host] instance=%p arenas=%u error_count=%u runtime_errors=%llu\n",
           (void*)inst, arena_count, err_count, (unsigned long long)rt_errors);

    if (preloaded && inst == nullptr)
    {
        fprintf(stderr, "[preload_dlopen_host] expected non-null instance\n");
        return 1;
    }

    if (rt_errors != 0 || err_count != 0)
    {
        fprintf(stderr, "[preload_dlopen_host] runtime_errors=%llu error_count=%u (expected 0)\n",
                (unsigned long long)rt_errors, err_count);
        return 1;
    }

    printf("[preload_dlopen_host] all phases done, exit 0\n");
    return 0;
}
