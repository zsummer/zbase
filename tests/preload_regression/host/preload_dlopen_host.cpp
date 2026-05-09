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

// 仅在 host exe 自身的 IAT 上做轻量改写, 把 CRT 导出的
// malloc/free/calloc/realloc 重定向到 zmalloc_preload.dll 的同名导出.
// 仅 host 自身使用, 不进入 src/ 公共代码, 因此就地放在本 cpp 内.
// 非 Windows 平台直接返回 0, 让 main 无需再做平台分支.
//
// 注意: 本函数及其调用链中禁止使用 std 容器, new/malloc, 以及任何
// 可能间接触发 CRT malloc 的 API. 因为我们正在改写的就是这些 API
// 的 IAT 项, 一旦在改写过程中递归触发 zmalloc, 极易死循环或重入异常.
// 仅允许使用栈缓冲, GetModuleHandleA / GetProcAddress / VirtualProtect
// 这些不走 C 堆的 Win32 API, 以及 fprintf(stderr, ...) (写 FILE* stderr
// 自身不会触发 malloc, 仅 stdio 内部缓冲, 其首块缓冲已在 CRT 启动时分配).
int install_iat_patch()
{
    int patched = 0;
#if defined(_WIN32)
    HMODULE host = GetModuleHandleA(NULL);
    if (host == NULL)
    {
        fprintf(stderr, "[win_iat_patch] GetModuleHandleA(NULL) failed\n");
        return 0;
    }

    HMODULE preload = GetModuleHandleA("libzmalloc_preload.dll");
    if (preload == NULL)
    {
        // 兜底: 当未来产物名变回不带 lib 前缀时仍能工作.
        preload = GetModuleHandleA("zmalloc_preload.dll");
    }
    if (preload == NULL)
    {
        fprintf(stderr, "[win_iat_patch] zmalloc_preload.dll not loaded; skip\n");
        return 0;
    }

    void* tgt_malloc  = (void*)GetProcAddress(preload, "malloc");
    void* tgt_free    = (void*)GetProcAddress(preload, "free");
    void* tgt_calloc  = (void*)GetProcAddress(preload, "calloc");
    void* tgt_realloc = (void*)GetProcAddress(preload, "realloc");
    void* tgt_malloc_dbg  = (void*)GetProcAddress(preload, "_malloc_dbg");
    void* tgt_free_dbg    = (void*)GetProcAddress(preload, "_free_dbg");
    void* tgt_calloc_dbg  = (void*)GetProcAddress(preload, "_calloc_dbg");
    void* tgt_realloc_dbg = (void*)GetProcAddress(preload, "_realloc_dbg");
    if ((tgt_malloc == NULL && tgt_malloc_dbg == NULL) ||
        (tgt_free == NULL && tgt_free_dbg == NULL) ||
        (tgt_calloc == NULL && tgt_calloc_dbg == NULL) ||
        (tgt_realloc == NULL && tgt_realloc_dbg == NULL))
    {
        fprintf(stderr,
                "[win_iat_patch] GetProcAddress on zmalloc_preload.dll failed: "
                "malloc=%p free=%p calloc=%p realloc=%p "
                "_malloc_dbg=%p _free_dbg=%p _calloc_dbg=%p _realloc_dbg=%p\n",
                tgt_malloc, tgt_free, tgt_calloc, tgt_realloc,
                tgt_malloc_dbg, tgt_free_dbg, tgt_calloc_dbg, tgt_realloc_dbg);
        return 0;
    }

    BYTE* base = reinterpret_cast<BYTE*>(host);
    PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
    {
        fprintf(stderr, "[win_iat_patch] bad DOS signature\n");
        return 0;
    }
    PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
    {
        fprintf(stderr, "[win_iat_patch] bad NT signature\n");
        return 0;
    }
    IMAGE_DATA_DIRECTORY& imp_dir =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imp_dir.VirtualAddress == 0 || imp_dir.Size == 0)
    {
        fprintf(stderr, "[win_iat_patch] no import directory\n");
        return 0;
    }

    // 同时匹配 ucrtbase.dll 与 api-ms-win-crt-heap-l1-1-0(.dll): UCRT 在
    // Win10+ 通过 API set 转发名做拆分, 不同链接器/不同 SDK 可能把 malloc
    // 落到任一个名字下, 因此两类名字都需要参与匹配.
    static const char* const kCrtNames[] = {
        "ucrtbase.dll",
        "ucrtbased.dll",
        "api-ms-win-crt-heap-l1-1-0.dll",
        "api-ms-win-crt-heap-l1-1-0",
        "msvcrt.dll",
        "vcruntime140.dll",
        "vcruntime140d.dll",
    };
    const int kCrtNameCount = sizeof(kCrtNames) / sizeof(kCrtNames[0]);

    auto stricmp_local = [](const char* a, const char* b) -> int {
        while (*a && *b)
        {
            char ca = *a;
            char cb = *b;
            if (ca >= 'A' && ca <= 'Z') ca = char(ca + ('a' - 'A'));
            if (cb >= 'A' && cb <= 'Z') cb = char(cb + ('a' - 'A'));
            if (ca != cb) return (int)(unsigned char)ca - (int)(unsigned char)cb;
            ++a; ++b;
        }
        return (int)(unsigned char)*a - (int)(unsigned char)*b;
    };

    auto is_crt_module = [&](const char* name) -> bool {
        for (int i = 0; i < kCrtNameCount; ++i)
        {
            if (stricmp_local(name, kCrtNames[i]) == 0) return true;
        }
        return false;
    };

    PIMAGE_IMPORT_DESCRIPTOR desc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        base + imp_dir.VirtualAddress);

    for (; desc->Name != 0; ++desc)
    {
        const char* mod_name = reinterpret_cast<const char*>(base + desc->Name);
        if (!is_crt_module(mod_name)) continue;

        // OriginalFirstThunk 给名字; FirstThunk 是真正在调用点被读的 IAT.
        DWORD oft_rva = desc->OriginalFirstThunk ? desc->OriginalFirstThunk
                                                 : desc->FirstThunk;
        if (oft_rva == 0 || desc->FirstThunk == 0) continue;

        PIMAGE_THUNK_DATA name_thunk =
            reinterpret_cast<PIMAGE_THUNK_DATA>(base + oft_rva);
        PIMAGE_THUNK_DATA iat_thunk =
            reinterpret_cast<PIMAGE_THUNK_DATA>(base + desc->FirstThunk);

        for (; name_thunk->u1.AddressOfData != 0; ++name_thunk, ++iat_thunk)
        {
            if (IMAGE_SNAP_BY_ORDINAL(name_thunk->u1.Ordinal)) continue;

            PIMAGE_IMPORT_BY_NAME ibn =
                reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                    base + name_thunk->u1.AddressOfData);
            const char* sym_name = reinterpret_cast<const char*>(ibn->Name);

            void* new_addr = NULL;
            if      (strcmp(sym_name, "malloc")  == 0) new_addr = tgt_malloc;
            else if (strcmp(sym_name, "free")    == 0) new_addr = tgt_free;
            else if (strcmp(sym_name, "calloc")  == 0) new_addr = tgt_calloc;
            else if (strcmp(sym_name, "realloc") == 0) new_addr = tgt_realloc;
            else if (strcmp(sym_name, "_malloc_dbg")  == 0) new_addr = tgt_malloc_dbg ? tgt_malloc_dbg : tgt_malloc;
            else if (strcmp(sym_name, "_free_dbg")    == 0) new_addr = tgt_free_dbg ? tgt_free_dbg : tgt_free;
            else if (strcmp(sym_name, "_calloc_dbg")  == 0) new_addr = tgt_calloc_dbg ? tgt_calloc_dbg : tgt_calloc;
            else if (strcmp(sym_name, "_realloc_dbg") == 0) new_addr = tgt_realloc_dbg ? tgt_realloc_dbg : tgt_realloc;
            else continue;

            DWORD old_prot = 0;
            if (!VirtualProtect(&iat_thunk->u1.Function, sizeof(void*),
                                PAGE_READWRITE, &old_prot))
            {
                fprintf(stderr,
                        "[win_iat_patch] VirtualProtect RW failed for %s in %s\n",
                        sym_name, mod_name);
                continue;
            }
            iat_thunk->u1.Function = reinterpret_cast<ULONGLONG>(new_addr);
            DWORD tmp_prot = 0;
            VirtualProtect(&iat_thunk->u1.Function, sizeof(void*),
                           old_prot, &tmp_prot);
            ++patched;
        }
    }
#endif
    return patched;
}

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

// CMake 全平台给 preload_dummy 统一加了 PREFIX "lib", 实际产物是 libpreload_dummy.dll,
// 与 Linux 侧 libpreload_dummy.so 命名保持一致.
static inline const char* dummy_lib_name() { return "libpreload_dummy.dll"; }

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

    // 必须先于任何 std 容器 / 线程 / I/O 之前完成 IAT 改写,
    // 这样后续 host 内部走的所有 CRT malloc 才会被路由到
    // zmalloc_preload.dll 的导出实现; Linux 下该调用恒返回 0.
    int patched = install_iat_patch();
    fprintf(stderr, "[preload_dlopen_host] win_iat_patch: patched=%d\n", patched);

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
#if defined(_WIN32)
    // 跨映像 inline static 分裂: host.exe 与 zmalloc_preload.dll 各自持有
    // zmalloc_mt::instance_ptr() 的一份独立 .data 副本, 因此 host 侧直接
    // 读 inst 永远是 NULL. 通过 dll 显式导出的诊断函数取 dll 那一侧看到
    // 的真实统计量, 这才是回归判定应当依据的数据.
    struct preload_stats_t
    {
        void* instance;
        unsigned int arena_count;
        unsigned int error_count;
        unsigned long long runtime_errors;
    };
    typedef void (*get_stats_fn)(preload_stats_t*);
    HMODULE dll = GetModuleHandleA("libzmalloc_preload.dll");
    if (dll == NULL) dll = GetModuleHandleA("zmalloc_preload.dll");
    get_stats_fn get_stats = dll ? (get_stats_fn)GetProcAddress(dll, "zmalloc_preload_get_stats")
                                 : NULL;
    if (get_stats != NULL)
    {
        preload_stats_t s;
        get_stats(&s);
        inst = (zmalloc_mt*)s.instance;
        arena_count = s.arena_count;
        err_count = s.error_count;
        rt_errors = s.runtime_errors;
    }
#else
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
#endif
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
