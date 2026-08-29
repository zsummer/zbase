
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

static const int THREAD_COUNT = 8;
static const int ALLOC_COUNT = 100000;
static const int MAX_ALLOC_SIZE = 4096;

static std::atomic<u_int64_t> g_total_alloc{0};
static std::atomic<u_int64_t> g_total_free{0};
static std::atomic<int> g_errors{0};

void thread_small_alloc(int tid)
{
    void* ptrs[ALLOC_COUNT];
    for (int i = 0; i < ALLOC_COUNT; i++)
    {
        size_t sz = (i % 256) + 1;
        ptrs[i] = malloc(sz);
        if (ptrs[i] == nullptr)
        {
            g_errors.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        memset(ptrs[i], 0xab, sz);
        g_total_alloc.fetch_add(1, std::memory_order_relaxed);
    }
    for (int i = 0; i < ALLOC_COUNT; i++)
    {
        if (ptrs[i])
        {
            free(ptrs[i]);
            g_total_free.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void thread_mixed_alloc(int tid)
{
    std::vector<void*> ptrs;
    ptrs.reserve(ALLOC_COUNT);
    for (int i = 0; i < ALLOC_COUNT; i++)
    {
        size_t sz = (i * 37 + tid * 13) % MAX_ALLOC_SIZE + 1;
        void* p = malloc(sz);
        if (p == nullptr)
        {
            g_errors.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        memset(p, 0xcd, sz);
        ptrs.push_back(p);
        g_total_alloc.fetch_add(1, std::memory_order_relaxed);

        if (ptrs.size() > 100 && (i % 3 == 0))
        {
            free(ptrs.back());
            ptrs.pop_back();
            g_total_free.fetch_add(1, std::memory_order_relaxed);
        }
    }
    for (auto p : ptrs)
    {
        free(p);
        g_total_free.fetch_add(1, std::memory_order_relaxed);
    }
}

void thread_realloc_test(int tid)
{
    for (int i = 0; i < ALLOC_COUNT / 10; i++)
    {
        size_t sz = (i % 128) + 1;
        void* p = malloc(sz);
        if (p == nullptr)
        {
            g_errors.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        memset(p, 0xef, sz);
        g_total_alloc.fetch_add(1, std::memory_order_relaxed);

        size_t new_sz = sz * 2 + tid;
        void* np = realloc(p, new_sz);
        if (np == nullptr)
        {
            free(p);
            g_total_free.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        memset(np, 0xfe, new_sz);

        free(np);
        g_total_free.fetch_add(1, std::memory_order_relaxed);
    }
}

void thread_calloc_test(int tid)
{
    for (int i = 0; i < ALLOC_COUNT / 10; i++)
    {
        size_t nmemb = (i % 64) + 1;
        size_t sz = (tid % 8) + 8;
        void* p = calloc(nmemb, sz);
        if (p == nullptr)
        {
            g_errors.fetch_add(1, std::memory_order_relaxed);
            continue;
        }

        unsigned char* bp = (unsigned char*)p;
        bool all_zero = true;
        for (size_t j = 0; j < nmemb * sz; j++)
        {
            if (bp[j] != 0)
            {
                all_zero = false;
                break;
            }
        }
        if (!all_zero)
        {
            g_errors.fetch_add(1, std::memory_order_relaxed);
        }

        g_total_alloc.fetch_add(1, std::memory_order_relaxed);
        free(p);
        g_total_free.fetch_add(1, std::memory_order_relaxed);
    }
}

void thread_large_alloc(int tid)
{
    for (int i = 0; i < 1000; i++)
    {
        size_t sz = 64 * 1024 + (i * 1024) + tid;
        void* p = malloc(sz);
        if (p == nullptr)
        {
            g_errors.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        memset(p, 0xaa, 1024);
        g_total_alloc.fetch_add(1, std::memory_order_relaxed);
        free(p);
        g_total_free.fetch_add(1, std::memory_order_relaxed);
    }
}

int main(int argc, char* argv[])
{
    printf("=== zmalloc_mt preload multi-thread test ===\n");
    printf("threads: %d, allocs per thread: %d\n\n", THREAD_COUNT, ALLOC_COUNT);

    auto start = std::chrono::high_resolution_clock::now();

    {
        printf("[phase 1] small alloc test ...\n");
        std::vector<std::thread> threads;
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            threads.emplace_back(thread_small_alloc, i);
        }
        for (auto& t : threads)
        {
            t.join();
        }
        printf("  alloc: %lu, free: %lu, errors: %d\n",
               g_total_alloc.load(), g_total_free.load(), g_errors.load());
    }

    g_total_alloc.store(0);
    g_total_free.store(0);
    g_errors.store(0);

    {
        printf("[phase 2] mixed alloc test ...\n");
        std::vector<std::thread> threads;
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            threads.emplace_back(thread_mixed_alloc, i);
        }
        for (auto& t : threads)
        {
            t.join();
        }
        printf("  alloc: %lu, free: %lu, errors: %d\n",
               g_total_alloc.load(), g_total_free.load(), g_errors.load());
    }

    g_total_alloc.store(0);
    g_total_free.store(0);
    g_errors.store(0);

    {
        printf("[phase 3] realloc test ...\n");
        std::vector<std::thread> threads;
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            threads.emplace_back(thread_realloc_test, i);
        }
        for (auto& t : threads)
        {
            t.join();
        }
        printf("  alloc: %lu, free: %lu, errors: %d\n",
               g_total_alloc.load(), g_total_free.load(), g_errors.load());
    }

    g_total_alloc.store(0);
    g_total_free.store(0);
    g_errors.store(0);

    {
        printf("[phase 4] calloc test ...\n");
        std::vector<std::thread> threads;
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            threads.emplace_back(thread_calloc_test, i);
        }
        for (auto& t : threads)
        {
            t.join();
        }
        printf("  alloc: %lu, free: %lu, errors: %d\n",
               g_total_alloc.load(), g_total_free.load(), g_errors.load());
    }

    g_total_alloc.store(0);
    g_total_free.store(0);
    g_errors.store(0);

    {
        printf("[phase 5] large alloc test ...\n");
        std::vector<std::thread> threads;
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            threads.emplace_back(thread_large_alloc, i);
        }
        for (auto& t : threads)
        {
            t.join();
        }
        printf("  alloc: %lu, free: %lu, errors: %d\n",
               g_total_alloc.load(), g_total_free.load(), g_errors.load());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    printf("\n=== all phases done, total time: %ld ms ===\n", ms);
    printf("final errors: %d\n", g_errors.load());

    return g_errors.load() > 0 ? 1 : 0;
}
