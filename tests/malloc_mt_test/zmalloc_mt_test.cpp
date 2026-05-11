

#include "fn_log.h"
#include <string>
#include "zmalloc_mt.h"
#include "zprof.h"
#include "zarray.h"
#include "zclock.h"
#include "test_common.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdlib>

#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()


// ====================================================================
//  1. 基础功能测试: 单线程下的 alloc/free 正确性
// ====================================================================
s32 zmalloc_mt_base_test()
{
    LogInfo() << "========== zmalloc_mt_base_test begin ==========";

    // 测试1: 基本 alloc/free (size=0)
    {
        std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
        memset(zstate.get(), 0, sizeof(zmalloc_mt));
        zstate->init();
        zstate->set_global(zstate.get());

        void* p = global_zmalloc_mt(0);
        ASSERT_TEST_NOLOG(p != nullptr, "alloc(0) should return non-null");
        global_zfree_mt(p);

        zstate->check_panic();
        zstate->flush_and_reset_thread_cache();
        LogDebug() << "  base_test: alloc(0) ok";
    }


    // 测试2: 小对象分配与释放
    {
        std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
        memset(zstate.get(), 0, sizeof(zmalloc_mt));
        zstate->init();
        zstate->set_global(zstate.get());

        void* p = global_zmalloc_mt(64);
        ASSERT_TEST_NOLOG(p != nullptr, "alloc(64) should return non-null");
        memset(p, 0xAB, 64); // 写入测试
        global_zfree_mt(p);

        zstate->check_panic();
        zstate->flush_and_reset_thread_cache();
        LogDebug() << "  base_test: small alloc(64) ok";
    }


    // 测试3: 中等对象 (1024~512K 范围)
    {
        std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
        memset(zstate.get(), 0, sizeof(zmalloc_mt));
        zstate->init();
        zstate->set_global(zstate.get());

        void* p = global_zmalloc_mt(1020);
        ASSERT_TEST_NOLOG(p != nullptr, "alloc(1020) should return non-null");
        memset(p, 0xCD, 1020);
        global_zfree_mt(p);

        void* p2 = global_zmalloc_mt(1024);
        ASSERT_TEST_NOLOG(p2 != nullptr, "alloc(1024) should return non-null");
        memset(p2, 0xEF, 1024);
        global_zfree_mt(p2);

        zstate->check_panic();
        zstate->flush_and_reset_thread_cache();
        LogDebug() << "  base_test: medium alloc ok";
    }


    // 测试4: 大对象 (>512K direct alloc)
    {
        std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
        memset(zstate.get(), 0, sizeof(zmalloc_mt));
        zstate->init();
        zstate->set_global(zstate.get());

        void* p = global_zmalloc_mt(zmalloc::kBigMaxRequest + 1024);
        ASSERT_TEST_NOLOG(p != nullptr, "alloc(big) should return non-null");
        memset(p, 0xAA, zmalloc::kBigMaxRequest + 1024);
        global_zfree_mt(p);

        zstate->check_panic();
        zstate->flush_and_reset_thread_cache();
        LogDebug() << "  base_test: big alloc ok";
    }


    // 测试5: 连续分配和释放多种大小
    {
        std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
        memset(zstate.get(), 0, sizeof(zmalloc_mt));
        zstate->init();
        zstate->set_global(zstate.get());

        static const u32 kTestCount = 2000;
        void* ptrs[kTestCount];
        for (u32 i = 0; i < kTestCount; i++)
        {
            u32 size = (i * 37 + 7) % (zmalloc::kBigMaxRequest + 2000); // 各种大小
            ptrs[i] = global_zmalloc_mt(size);
            ASSERT_TEST_NOLOG(ptrs[i] != nullptr, "alloc should return non-null");
            // 写入首尾字节验证
            if (size > 0)
            {
                ((char*)ptrs[i])[0] = (char)(i & 0xFF);
                if (size > 1)
                    ((char*)ptrs[i])[size - 1] = (char)(i & 0xFF);
            }
        }
        for (u32 i = 0; i < kTestCount; i++)
        {
            global_zfree_mt(ptrs[i]);
        }

        zstate->check_panic();
        zstate->clear_cache();
        zstate->flush_and_reset_thread_cache();
        LogDebug() << "  base_test: sequential alloc/free ok";
    }


    // 测试6: thread-local cache 命中测试 (同一 size 反复 alloc/free)
    {
        std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
        memset(zstate.get(), 0, sizeof(zmalloc_mt));
        zstate->init();
        zstate->set_global(zstate.get());

        static const u32 kCacheTestCount = 10000;
        for (u32 i = 0; i < kCacheTestCount; i++)
        {
            void* p = global_zmalloc_mt(128);
            ASSERT_TEST_NOLOG(p != nullptr, "cache test alloc should return non-null");
            *(u32*)p = i;
            global_zfree_mt(p);
        }

        zstate->check_panic();
        zstate->flush_and_reset_thread_cache();
        LogDebug() << "  base_test: thread-local cache hit test ok";
    }


    LogInfo() << "========== zmalloc_mt_base_test passed ==========";
    return 0;
}


// ====================================================================
//  2. 单线程压测: 与 zmalloc_stress 类似的性能压测
// ====================================================================
s32 zmalloc_mt_single_thread_stress()
{
    LogInfo() << "========== zmalloc_mt_single_thread_stress begin ==========";

    std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
    memset(zstate.get(), 0, sizeof(zmalloc_mt));
    zstate->init();
    zstate->set_global(zstate.get());

    static const u32 rand_size = 100 * 10000;
    u32* rand_array = new u32[rand_size];
    for (u32 i = 0; i < rand_size; i++)
    {
        rand_array[i] = rand() % (zmalloc::kBigMaxRequest * 4 / 3);
    }

    using Addr = void*;
    static const u32 kBufMax = 8192;
    zarray<Addr, kBufMax>* buffers = new zarray<Addr, kBufMax>();

    PROF_DEFINE_COUNTER(cost);
    zclock<> zc;

    // 即时 alloc/free (小对象)
    PROF_START_COUNTER(cost);
    zc.start();
    for (u64 i = 0; i < rand_size; i++)
    {
        global_zfree_mt(global_zmalloc_mt(1));
    }
    zc.stop_and_save();
    PROF_OUTPUT_MULTI_COUNT_CPU("mt_st: zfree_mt(zmalloc_mt(1))", rand_size, cost.StopAndSave().cost());
    LogInfo() << "  avg_ns/op=" << (rand_size > 0 ? (double)zc.cost_ns() / rand_size : 0.0);

    // 即时 alloc/free (0~1024 随机)
    PROF_START_COUNTER(cost);
    zc.start();
    for (u64 i = 0; i < rand_size; i++)
    {
        u32 test_size = rand_array[i] % (1024);
        void* p = global_zmalloc_mt(test_size);
        global_zfree_mt(p);
    }
    zc.stop_and_save();
    PROF_OUTPUT_MULTI_COUNT_CPU("mt_st: zfree_mt(zmalloc_mt(0~1024))", rand_size, cost.StopAndSave().cost());
    LogInfo() << "  avg_ns/op=" << (rand_size > 0 ? (double)zc.cost_ns() / rand_size : 0.0);

    // 即时 alloc/free (1024~512k)
    PROF_START_COUNTER(cost);
    zc.start();
    for (u64 i = 0; i < rand_size; i++)
    {
        u32 test_size = (rand_array[i] % (zmalloc::kBigMaxRequest - zmalloc::kSmallMaxRequest)) + zmalloc::kSmallMaxRequest;
        void* p = global_zmalloc_mt(test_size);
        global_zfree_mt(p);
    }
    zc.stop_and_save();
    PROF_OUTPUT_MULTI_COUNT_CPU("mt_st: zfree_mt(zmalloc_mt(1024~512k))", rand_size, cost.StopAndSave().cost());
    LogInfo() << "  avg_ns/op=" << (rand_size > 0 ? (double)zc.cost_ns() / rand_size : 0.0);

    // 批量 alloc + 批量 free (连续分段覆盖)
    LogInfo() << "";
    LogInfo() << "mt_st: begin batch alloc/free segmented coverage";
    LogInfo() << "-------------------------------------------------------------------";
    static const u32 kSegCount = 20;
    static const u32 kSegSize = kBufMax;
    for (u32 loop = 0; loop < kSegCount; loop++)
    {
        u32 begin_idx = kSegSize * loop;
        char mbuf[80];
        sprintf(mbuf, "mt_st: zmalloc_mt seg[%u]", loop);
        char fbuf[80];
        sprintf(fbuf, "mt_st: zfree_mt seg[%u]", loop);

        PROF_START_COUNTER(cost);
        for (u32 i = 0; i < kSegSize; i++)
        {
            u32 test_size = rand_array[(begin_idx + i) % rand_size] % (zmalloc::kBigMaxRequest);
            void* p = global_zmalloc_mt(test_size);
            if (test_size >= 4)
                *(u32*)p = i;
            buffers->push_back(p);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU(mbuf, buffers->size(), cost.StopAndSave().cost());

        PROF_START_COUNTER(cost);
        for (auto p : *buffers)
        {
            global_zfree_mt(p);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU(fbuf, buffers->size(), cost.StopAndSave().cost());
        buffers->clear();
    }

    // 随机 alloc/free 混合 (0~2k)
    LogInfo() << "";
    LogInfo() << "mt_st: begin rand alloc/free mix (0~2k)";
    LogInfo() << "-------------------------------------------------------------------";
    {
        buffers->clear();
        PROF_START_COUNTER(cost);
        u64 alloc_count = 0;
        u64 free_count = 0;
        for (u64 i = 0; i < rand_size; i++)
        {
            if (rand() % 5 == 0)
                continue;

            if (buffers->full())
            {
                for (u32 j = 0; j < buffers->max_size() / 2; j++)
                {
                    global_zfree_mt(buffers->back());
                    buffers->pop_back();
                    free_count++;
                }
            }

            u32 push_size = rand_array[i] % (2048);
            if (push_size % 3 == 0 && !buffers->empty())
            {
                global_zfree_mt(buffers->back());
                buffers->pop_back();
                free_count++;
            }

            buffers->push_back(global_zmalloc_mt(push_size));
            alloc_count++;
        }
        PROF_OUTPUT_MULTI_COUNT_CPU("mt_st: rand zmalloc_mt/zfree_mt(0~2k)", alloc_count + free_count, cost.StopAndSave().cost());

        for (auto p : *buffers)
        {
            global_zfree_mt(p);
        }
        buffers->clear();
    }

    zstate->check_panic();
    zstate->clear_cache();
    zstate->flush_and_reset_thread_cache();

    // ================================================================
    //  zmalloc (单线程版) 对比测试 — 复用相同的 rand_array
    // ================================================================
    LogInfo() << "";
    LogInfo() << "========== zmalloc (single-thread) comparison ==========";
    LogInfo() << "-------------------------------------------------------------------";
    {
        std::unique_ptr<zmalloc> zst(new zmalloc());
        memset(zst.get(), 0, sizeof(zmalloc));
        zst->init();
        zst->set_global(zst.get());

        zclock<> zc;

        // 即时 alloc/free (小对象)
        PROF_START_COUNTER(cost);
        zc.start();
        for (u64 i = 0; i < rand_size; i++)
        {
            global_zfree(global_zmalloc(1));
        }
        zc.stop_and_save();
        PROF_OUTPUT_MULTI_COUNT_CPU("st: zfree(zmalloc(1))", rand_size, cost.StopAndSave().cost());
        LogInfo() << "  avg_ns/op=" << (rand_size > 0 ? (double)zc.cost_ns() / rand_size : 0.0);

        // 即时 alloc/free (0~1024 随机)
        PROF_START_COUNTER(cost);
        zc.start();
        for (u64 i = 0; i < rand_size; i++)
        {
            u32 test_size = rand_array[i] % (1024);
            void* p = global_zmalloc(test_size);
            global_zfree(p);
        }
        zc.stop_and_save();
        PROF_OUTPUT_MULTI_COUNT_CPU("st: zfree(zmalloc(0~1024))", rand_size, cost.StopAndSave().cost());
        LogInfo() << "  avg_ns/op=" << (rand_size > 0 ? (double)zc.cost_ns() / rand_size : 0.0);

        // 即时 alloc/free (1024~512k)
        PROF_START_COUNTER(cost);
        zc.start();
        for (u64 i = 0; i < rand_size; i++)
        {
            u32 test_size = (rand_array[i] % (zmalloc::kBigMaxRequest - zmalloc::kSmallMaxRequest)) + zmalloc::kSmallMaxRequest;
            void* p = global_zmalloc(test_size);
            global_zfree(p);
        }
        zc.stop_and_save();
        PROF_OUTPUT_MULTI_COUNT_CPU("st: zfree(zmalloc(1024~512k))", rand_size, cost.StopAndSave().cost());
        LogInfo() << "  avg_ns/op=" << (rand_size > 0 ? (double)zc.cost_ns() / rand_size : 0.0);

        zst->check_panic();
    }
    LogInfo() << "========== zmalloc comparison done ==========";

    delete[] rand_array;
    delete buffers;

    LogInfo() << "========== zmalloc_mt_single_thread_stress passed ==========";

    return 0;
}


// ====================================================================
//  3. 多线程并发压测
// ====================================================================

// 线程工作函数: 每个线程独立做 alloc/free
static void mt_stress_worker(zmalloc_mt* zstate, u32 thread_id, u32 iterations,
                              std::atomic<u64>& total_alloc_ops, std::atomic<u64>& total_free_ops,
                              std::atomic<s32>& error_flag)
{
    static const u32 kLocalBufMax = 1024;
    void* local_buf[kLocalBufMax];
    u32 local_count = 0;

    u32 seed = thread_id * 1234567 + 42;
    auto simple_rand = [&seed]() -> u32 {
        seed = seed * 1103515245 + 12345;
        return (seed >> 16) & 0x7FFF;
    };

    u64 alloc_ops = 0;
    u64 free_ops = 0;

    for (u32 i = 0; i < iterations; i++)
    {
        u32 action = simple_rand() % 10;
        u32 alloc_size = simple_rand() % 2048;

        if (action < 6 || local_count == 0)
        {
            // alloc (60% 概率 或 buffer为空)
            if (local_count >= kLocalBufMax)
            {
                // buffer满了, 先释放一半
                for (u32 j = 0; j < kLocalBufMax / 2; j++)
                {
                    local_count--;
                    zstate->free_memory(local_buf[local_count]);
                    free_ops++;
                }
            }

            void* p = zstate->alloc_memory(alloc_size);
            if (p == nullptr)
            {
                error_flag.store(1, std::memory_order_relaxed);
                LogError() << "thread " << thread_id << " alloc(" << alloc_size << ") returned null at iter " << i;
                break;
            }

            // 写入验证数据
            if (alloc_size >= 4)
                *(u32*)p = thread_id;

            local_buf[local_count++] = p;
            alloc_ops++;
        }
        else
        {
            // free (40% 概率)
            if (local_count > 0)
            {
                u32 idx = simple_rand() % local_count;
                zstate->free_memory(local_buf[idx]);
                // 用最后一个填充空位
                local_buf[idx] = local_buf[local_count - 1];
                local_count--;
                free_ops++;
            }
        }
    }

    // 清理剩余
    for (u32 j = 0; j < local_count; j++)
    {
        zstate->free_memory(local_buf[j]);
        free_ops++;
    }

    total_alloc_ops.fetch_add(alloc_ops, std::memory_order_relaxed);
    total_free_ops.fetch_add(free_ops, std::memory_order_relaxed);
}


s32 zmalloc_mt_concurrent_stress()
{
    LogInfo() << "========== zmalloc_mt_concurrent_stress begin ==========";

    std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
    memset(zstate.get(), 0, sizeof(zmalloc_mt));
    zstate->init();
    zstate->set_global(zstate.get());

    // 测试不同线程数: 2, 4, 8
    u32 thread_counts[] = { 2, 4, 8 };
    static const u32 kIterationsPerThread = 50 * 10000;

    for (u32 tc_idx = 0; tc_idx < 3; tc_idx++)
    {
        u32 num_threads = thread_counts[tc_idx];
        LogInfo() << "";
        LogInfo() << "--- concurrent stress: " << num_threads << " threads, "
                  << kIterationsPerThread << " iters/thread ---";

        // 每次重新创建 zmalloc_mt 实例
        std::unique_ptr<zmalloc_mt> local_zstate(new zmalloc_mt());
        memset(local_zstate.get(), 0, sizeof(zmalloc_mt));
        local_zstate->init();
        local_zstate->set_global(local_zstate.get());

        std::atomic<u64> total_alloc_ops(0);
        std::atomic<u64> total_free_ops(0);
        std::atomic<s32> error_flag(0);

        PROF_DEFINE_COUNTER(cost);
        zclock<> zcost;
        PROF_START_COUNTER(cost);
        zcost.start();

        std::vector<std::thread> threads;
        for (u32 t = 0; t < num_threads; t++)
        {
            threads.emplace_back(mt_stress_worker, local_zstate.get(), t,
                                  kIterationsPerThread, std::ref(total_alloc_ops),
                                  std::ref(total_free_ops), std::ref(error_flag));
        }

        for (auto& t : threads)
        {
            t.join();
        }

        zcost.stop_and_save();
        u64 total_ops = total_alloc_ops.load() + total_free_ops.load();
        long long total_ns = zcost.cost_ns();
        double avg_ns_per_op = total_ops > 0 ? (double)total_ns / (double)total_ops : 0.0;

        char buf[128];
        sprintf(buf, "mt_concurrent(%u threads)", num_threads);
        PROF_OUTPUT_MULTI_COUNT_CPU(buf, total_ops, cost.StopAndSave().cost());

        ASSERT_TEST_NOLOG(error_flag.load() == 0, "no error in concurrent stress");

        LogInfo() << "  alloc_ops=" << total_alloc_ops.load()
                  << " free_ops=" << total_free_ops.load()
                  << " total_ops=" << total_ops
                  << " cost_ns=" << total_ns
                  << " avg_ns/op=" << avg_ns_per_op
                  << " arena_count=" << local_zstate->arena_count_.load();

        s32 health = local_zstate->check_health();
        ASSERT_TEST_NOLOG(health == 0, "check_health after concurrent stress");

        local_zstate->clear_cache();
        local_zstate->flush_and_reset_thread_cache();
    }

    LogInfo() << "========== zmalloc_mt_concurrent_stress passed ==========";

    return 0;
}


// ====================================================================
//  4. 混合压测: 多线程 + 不同大小对象 + 跨线程 free
// ====================================================================

// 共享队列: 一个线程 alloc, 另一个线程 free (跨线程 free 场景)
struct shared_queue
{
    static const u32 kQueueSize = 4096;
    std::atomic<void*>  slots[kQueueSize];
    std::atomic<u32>    head;   // 生产者写入位置
    std::atomic<u32>    tail;   // 消费者读取位置

    void init()
    {
        head.store(0, std::memory_order_relaxed);
        tail.store(0, std::memory_order_relaxed);
        for (u32 i = 0; i < kQueueSize; i++)
            slots[i].store(nullptr, std::memory_order_relaxed);
    }

    bool try_push(void* p)
    {
        u32 h = head.load(std::memory_order_relaxed);
        u32 next_h = (h + 1) % kQueueSize;
        if (next_h == tail.load(std::memory_order_acquire))
            return false; // 满了
        slots[h].store(p, std::memory_order_relaxed);
        head.store(next_h, std::memory_order_release);
        return true;
    }

    void* try_pop()
    {
        u32 t = tail.load(std::memory_order_relaxed);
        if (t == head.load(std::memory_order_acquire))
            return nullptr; // 空的
        void* p = slots[t].load(std::memory_order_relaxed);
        tail.store((t + 1) % kQueueSize, std::memory_order_release);
        return p;
    }
};


// 生产者线程: alloc 并推入共享队列
static void producer_worker(zmalloc_mt* zstate, shared_queue* queue, u32 thread_id,
                             u32 iterations, std::atomic<u64>& alloc_ops, std::atomic<s32>& error_flag)
{
    u32 seed = thread_id * 7654321 + 13;
    auto simple_rand = [&seed]() -> u32 {
        seed = seed * 1103515245 + 12345;
        return (seed >> 16) & 0x7FFF;
    };

    u64 ops = 0;
    u32 local_buf_count = 0;
    static const u32 kLocalMax = 512;
    void* local_buf[kLocalMax];

    for (u32 i = 0; i < iterations; i++)
    {
        u32 size_class = simple_rand() % 100;
        u32 alloc_size;
        if (size_class < 60)
            alloc_size = simple_rand() % 512;         // 60%: 小对象 (0~512)
        else if (size_class < 85)
            alloc_size = 512 + simple_rand() % 4096;  // 25%: 中对象 (512~4608)
        else if (size_class < 95)
            alloc_size = 4096 + simple_rand() % (zmalloc::kBigMaxRequest - 4096); // 10%: 大对象
        else
            alloc_size = zmalloc::kBigMaxRequest + simple_rand() % 10000; // 5%: 超大对象 (direct)

        void* p = zstate->alloc_memory(alloc_size);
        if (p == nullptr)
        {
            error_flag.store(1, std::memory_order_relaxed);
            break;
        }

        // 写入验证数据
        if (alloc_size >= sizeof(u32))
            *(u32*)p = thread_id;

        ops++;

        // 尝试推入共享队列 (跨线程 free)
        if (!queue->try_push(p))
        {
            // 队列满了, 本地缓存或直接 free
            if (local_buf_count < kLocalMax)
            {
                local_buf[local_buf_count++] = p;
            }
            else
            {
                zstate->free_memory(p);
            }
        }
    }

    // 清理本地缓存 (推入队列或直接 free)
    for (u32 j = 0; j < local_buf_count; j++)
    {
        if (!queue->try_push(local_buf[j]))
            zstate->free_memory(local_buf[j]);
    }

    alloc_ops.fetch_add(ops, std::memory_order_relaxed);
}


// 消费者线程: 从共享队列取出并 free
static void consumer_worker(zmalloc_mt* zstate, shared_queue* queue,
                             u32 iterations, std::atomic<u64>& free_ops,
                             std::atomic<bool>& producers_done)
{
    u64 ops = 0;

    while (!producers_done.load(std::memory_order_acquire) || true)
    {
        void* p = queue->try_pop();
        if (p != nullptr)
        {
            zstate->free_memory(p);
            ops++;
        }
        else
        {
            if (producers_done.load(std::memory_order_acquire))
            {
                // 再尝试清空队列
                while (true)
                {
                    p = queue->try_pop();
                    if (p == nullptr)
                        break;
                    zstate->free_memory(p);
                    ops++;
                }
                break;
            }
            std::this_thread::yield();
        }
    }

    free_ops.fetch_add(ops, std::memory_order_relaxed);
}


s32 zmalloc_mt_cross_thread_stress()
{
    LogInfo() << "========== zmalloc_mt_cross_thread_stress begin ==========";
    LogInfo() << "  (producer alloc, consumer free — 跨线程 free 场景)";

    std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
    memset(zstate.get(), 0, sizeof(zmalloc_mt));
    zstate->init();
    zstate->set_global(zstate.get());

    static const u32 kProducerCount = 4;
    static const u32 kConsumerCount = 2;
    static const u32 kProducerIters = 20 * 10000;

    shared_queue* queues = new shared_queue[kConsumerCount];
    for (u32 i = 0; i < kConsumerCount; i++)
        queues[i].init();

    std::atomic<u64> total_alloc_ops(0);
    std::atomic<u64> total_free_ops(0);
    std::atomic<s32> error_flag(0);
    std::atomic<bool> producers_done(false);

    PROF_DEFINE_COUNTER(cost);
    zclock<> zcost;
    PROF_START_COUNTER(cost);
    zcost.start();

    // 启动消费者
    std::vector<std::thread> consumers;
    for (u32 c = 0; c < kConsumerCount; c++)
    {
        consumers.emplace_back(consumer_worker, zstate.get(), &queues[c],
                                kProducerIters, std::ref(total_free_ops),
                                std::ref(producers_done));
    }

    // 启动生产者 (每个生产者轮询推入不同队列)
    std::vector<std::thread> producers;
    for (u32 p = 0; p < kProducerCount; p++)
    {
        // 每个生产者推入 p % kConsumerCount 对应的队列
        producers.emplace_back(producer_worker, zstate.get(), &queues[p % kConsumerCount],
                                p, kProducerIters, std::ref(total_alloc_ops),
                                std::ref(error_flag));
    }

    for (auto& t : producers)
        t.join();

    producers_done.store(true, std::memory_order_release);

    for (auto& t : consumers)
        t.join();

    zcost.stop_and_save();
    u64 total_ops = total_alloc_ops.load() + total_free_ops.load();
    long long total_ns = zcost.cost_ns();
    double avg_ns_per_op = total_ops > 0 ? (double)total_ns / (double)total_ops : 0.0;

    char buf[128];
    sprintf(buf, "mt_cross_thread(%u producers + %u consumers)", kProducerCount, kConsumerCount);
    PROF_OUTPUT_MULTI_COUNT_CPU(buf, total_ops, cost.StopAndSave().cost());

    ASSERT_TEST_NOLOG(error_flag.load() == 0, "no error in cross-thread stress");

    LogInfo() << "  alloc_ops=" << total_alloc_ops.load()
              << " free_ops=" << total_free_ops.load()
              << " total_ops=" << total_ops
              << " cost_ns=" << total_ns
              << " avg_ns/op=" << avg_ns_per_op
              << " arena_count=" << zstate->arena_count_.load();

    s32 health = zstate->check_health();
    ASSERT_TEST_NOLOG(health == 0, "check_health after cross-thread stress");

    zstate->clear_cache();
    zstate->flush_and_reset_thread_cache();
    delete[] queues;

    LogInfo() << "========== zmalloc_mt_cross_thread_stress passed ==========";

    return 0;
}


// ====================================================================
//  5. 混合大小 + 多线程同时读写验证
// ====================================================================

static void mixed_size_worker(zmalloc_mt* zstate, u32 thread_id, u32 iterations,
                               std::atomic<u64>& total_ops, std::atomic<s32>& error_flag)
{
    u32 seed = thread_id * 999983 + 7;
    auto simple_rand = [&seed]() -> u32 {
        seed = seed * 1103515245 + 12345;
        return (seed >> 16) & 0x7FFF;
    };

    static const u32 kLocalBufMax = 256;
    struct alloc_entry
    {
        void* ptr;
        u32   size;
        u32   magic;
    };
    alloc_entry local_buf[kLocalBufMax];
    u32 local_count = 0;
    u64 ops = 0;

    for (u32 i = 0; i < iterations; i++)
    {
        u32 action = simple_rand() % 10;

        if (action < 6 || local_count == 0)
        {
            // alloc
            if (local_count >= kLocalBufMax)
            {
                // 释放一半
                u32 half = kLocalBufMax / 2;
                for (u32 j = 0; j < half; j++)
                {
                    local_count--;
                    auto& entry = local_buf[local_count];
                    // 验证 magic
                    if (entry.size >= 8)
                    {
                        u32 magic_check = *(u32*)entry.ptr;
                        if (magic_check != entry.magic)
                        {
                            error_flag.store(1, std::memory_order_relaxed);
                            LogError() << "thread " << thread_id << " data corruption! expected=" << entry.magic
                                       << " got=" << magic_check;
                        }
                    }
                    zstate->free_memory(entry.ptr);
                    ops++;
                }
            }

            // 选择分配大小
            u32 size_class = simple_rand() % 100;
            u32 alloc_size;
            if (size_class < 50)
                alloc_size = simple_rand() % 256;              // 50%: 极小对象
            else if (size_class < 75)
                alloc_size = 256 + simple_rand() % 768;        // 25%: 小对象
            else if (size_class < 90)
                alloc_size = 1024 + simple_rand() % (32 * 1024); // 15%: 中对象
            else
                alloc_size = 32 * 1024 + simple_rand() % (zmalloc::kBigMaxRequest - 32 * 1024); // 10%: 大对象

            void* p = zstate->alloc_memory(alloc_size);
            if (p == nullptr)
            {
                error_flag.store(1, std::memory_order_relaxed);
                break;
            }

            u32 magic = (thread_id << 16) | (i & 0xFFFF);
            if (alloc_size >= 8)
            {
                *(u32*)p = magic;
                // 尾部也写入
                if (alloc_size >= 8)
                    *((u32*)((char*)p + alloc_size - 4)) = magic;
            }
            else if (alloc_size >= 1)
            {
                ((char*)p)[0] = (char)(magic & 0xFF);
            }

            local_buf[local_count].ptr = p;
            local_buf[local_count].size = alloc_size;
            local_buf[local_count].magic = magic;
            local_count++;
            ops++;
        }
        else
        {
            // free
            if (local_count > 0)
            {
                u32 idx = simple_rand() % local_count;
                auto& entry = local_buf[idx];

                // 验证数据完整性
                if (entry.size >= 8)
                {
                    u32 magic_check = *(u32*)entry.ptr;
                    if (magic_check != entry.magic)
                    {
                        error_flag.store(1, std::memory_order_relaxed);
                        LogError() << "thread " << thread_id << " data corruption on free!";
                    }
                }

                zstate->free_memory(entry.ptr);
                local_buf[idx] = local_buf[local_count - 1];
                local_count--;
                ops++;
            }
        }
    }

    // 清理
    for (u32 j = 0; j < local_count; j++)
    {
        zstate->free_memory(local_buf[j].ptr);
        ops++;
    }

    total_ops.fetch_add(ops, std::memory_order_relaxed);
}


s32 zmalloc_mt_mixed_stress()
{
    LogInfo() << "========== zmalloc_mt_mixed_stress begin ==========";
    LogInfo() << "  (多线程 + 混合大小 + 数据完整性验证)";

    u32 thread_counts[] = { 2, 4, 8 };
    static const u32 kIterationsPerThread = 30 * 10000;

    for (u32 tc_idx = 0; tc_idx < 3; tc_idx++)
    {
        u32 num_threads = thread_counts[tc_idx];
        LogInfo() << "";
        LogInfo() << "--- mixed stress: " << num_threads << " threads, "
                  << kIterationsPerThread << " iters/thread ---";

        std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
        memset(zstate.get(), 0, sizeof(zmalloc_mt));
        zstate->init();
        zstate->set_global(zstate.get());

        std::atomic<u64> total_ops(0);
        std::atomic<s32> error_flag(0);

        PROF_DEFINE_COUNTER(cost);
        zclock<> zcost;
        PROF_START_COUNTER(cost);
        zcost.start();

        std::vector<std::thread> threads;
        for (u32 t = 0; t < num_threads; t++)
        {
            threads.emplace_back(mixed_size_worker, zstate.get(), t,
                                  kIterationsPerThread, std::ref(total_ops),
                                  std::ref(error_flag));
        }

        for (auto& t : threads)
            t.join();

        zcost.stop_and_save();
        long long total_ns = zcost.cost_ns();
        double avg_ns_per_op = total_ops.load() > 0 ? (double)total_ns / (double)total_ops.load() : 0.0;

        char buf[128];
        sprintf(buf, "mt_mixed(%u threads)", num_threads);
        PROF_OUTPUT_MULTI_COUNT_CPU(buf, total_ops.load(), cost.StopAndSave().cost());

        ASSERT_TEST_NOLOG(error_flag.load() == 0, "no data corruption in mixed stress");

        LogInfo() << "  total_ops=" << total_ops.load()
                  << " cost_ns=" << total_ns
                  << " avg_ns/op=" << avg_ns_per_op
                  << " arena_count=" << zstate->arena_count_.load();

        // 输出状态
        auto new_log = []() { return std::move(LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL)); };
        zstate->debug_state_log(new_log);

        s32 health = zstate->check_health();
        ASSERT_TEST_NOLOG(health == 0, "check_health after mixed stress");

        zstate->clear_cache();
        zstate->flush_and_reset_thread_cache();
    }

    LogInfo() << "========== zmalloc_mt_mixed_stress passed ==========";

    return 0;
}


// ====================================================================
//  6. 对比压测: zmalloc_mt vs sys malloc 多线程性能
// ====================================================================

static void sys_malloc_worker(u32 thread_id, u32 iterations,
                               std::atomic<u64>& total_ops)
{
    u32 seed = thread_id * 1234567 + 42;
    auto simple_rand = [&seed]() -> u32 {
        seed = seed * 1103515245 + 12345;
        return (seed >> 16) & 0x7FFF;
    };

    static const u32 kLocalBufMax = 1024;
    void* local_buf[kLocalBufMax];
    u32 local_count = 0;
    u64 ops = 0;

    for (u32 i = 0; i < iterations; i++)
    {
        u32 action = simple_rand() % 10;
        u32 alloc_size = simple_rand() % 2048 + 1;

        if (action < 6 || local_count == 0)
        {
            if (local_count >= kLocalBufMax)
            {
                for (u32 j = 0; j < kLocalBufMax / 2; j++)
                {
                    local_count--;
                    free(local_buf[local_count]);
                    ops++;
                }
            }
            local_buf[local_count++] = malloc(alloc_size);
            ops++;
        }
        else
        {
            if (local_count > 0)
            {
                u32 idx = simple_rand() % local_count;
                free(local_buf[idx]);
                local_buf[idx] = local_buf[local_count - 1];
                local_count--;
                ops++;
            }
        }
    }

    for (u32 j = 0; j < local_count; j++)
    {
        free(local_buf[j]);
        ops++;
    }
    total_ops.fetch_add(ops, std::memory_order_relaxed);
}


static void zmalloc_mt_bench_worker(zmalloc_mt* zstate, u32 thread_id, u32 iterations,
                                     std::atomic<u64>& total_ops)
{
    u32 seed = thread_id * 1234567 + 42;
    auto simple_rand = [&seed]() -> u32 {
        seed = seed * 1103515245 + 12345;
        return (seed >> 16) & 0x7FFF;
    };

    static const u32 kLocalBufMax = 1024;
    void* local_buf[kLocalBufMax];
    u32 local_count = 0;
    u64 ops = 0;

    for (u32 i = 0; i < iterations; i++)
    {
        u32 action = simple_rand() % 10;
        u32 alloc_size = simple_rand() % 2048;

        if (action < 6 || local_count == 0)
        {
            if (local_count >= kLocalBufMax)
            {
                for (u32 j = 0; j < kLocalBufMax / 2; j++)
                {
                    local_count--;
                    zstate->free_memory(local_buf[local_count]);
                    ops++;
                }
            }
            local_buf[local_count++] = zstate->alloc_memory(alloc_size);
            ops++;
        }
        else
        {
            if (local_count > 0)
            {
                u32 idx = simple_rand() % local_count;
                zstate->free_memory(local_buf[idx]);
                local_buf[idx] = local_buf[local_count - 1];
                local_count--;
                ops++;
            }
        }
    }

    for (u32 j = 0; j < local_count; j++)
    {
        zstate->free_memory(local_buf[j]);
        ops++;
    }
    total_ops.fetch_add(ops, std::memory_order_relaxed);
}


s32 zmalloc_mt_vs_sys_benchmark()
{
    LogInfo() << "========== zmalloc_mt vs sys malloc benchmark ==========";

    u32 thread_counts[] = { 1, 2, 4, 8 };
    static const u32 kIterationsPerThread = 50 * 10000;

    for (u32 tc_idx = 0; tc_idx < 4; tc_idx++)
    {
        u32 num_threads = thread_counts[tc_idx];
        LogInfo() << "";
        LogInfo() << "--- benchmark: " << num_threads << " threads ---";

        PROF_DEFINE_COUNTER(cost);

        // zmalloc_mt
        {
            std::unique_ptr<zmalloc_mt> zstate(new zmalloc_mt());
            memset(zstate.get(), 0, sizeof(zmalloc_mt));
            zstate->init();
            zstate->set_global(zstate.get());

            std::atomic<u64> total_ops(0);
            zclock<> zcost;

            PROF_START_COUNTER(cost);
            zcost.start();
            std::vector<std::thread> threads;
            for (u32 t = 0; t < num_threads; t++)
            {
                threads.emplace_back(zmalloc_mt_bench_worker, zstate.get(), t,
                                      kIterationsPerThread, std::ref(total_ops));
            }
            for (auto& t : threads)
                t.join();

            zcost.stop_and_save();
            long long total_ns = zcost.cost_ns();
            double avg_ns_per_op = total_ops.load() > 0 ? (double)total_ns / (double)total_ops.load() : 0.0;

            char buf[128];
            sprintf(buf, "zmalloc_mt(%u threads)", num_threads);
            PROF_OUTPUT_MULTI_COUNT_CPU(buf, total_ops.load(), cost.StopAndSave().cost());
            LogInfo() << "  total_ops=" << total_ops.load() << " cost_ns=" << total_ns << " avg_ns/op=" << avg_ns_per_op;

            zstate->clear_cache();
            zstate->flush_and_reset_thread_cache();
        }

        // sys malloc

        {
            std::atomic<u64> total_ops(0);
            zclock<> zcost;

            PROF_START_COUNTER(cost);
            zcost.start();
            std::vector<std::thread> threads;
            for (u32 t = 0; t < num_threads; t++)
            {
                threads.emplace_back(sys_malloc_worker, t,
                                      kIterationsPerThread, std::ref(total_ops));
            }
            for (auto& t : threads)
                t.join();

            zcost.stop_and_save();
            long long total_ns = zcost.cost_ns();
            double avg_ns_per_op = total_ops.load() > 0 ? (double)total_ns / (double)total_ops.load() : 0.0;

            char buf[128];
            sprintf(buf, "sys_malloc(%u threads)", num_threads);
            PROF_OUTPUT_MULTI_COUNT_CPU(buf, total_ops.load(), cost.StopAndSave().cost());
            LogInfo() << "  total_ops=" << total_ops.load() << " cost_ns=" << total_ns << " avg_ns/op=" << avg_ns_per_op;
        }
    }

    LogInfo() << "========== benchmark finished ==========";
    return 0;
}


// ====================================================================
//  总入口
// ====================================================================
s32 zmalloc_mt_test()
{
    ASSERT_TEST_EQ(zmalloc_mt_base_test(), 0, "zmalloc_mt_base_test");
    ASSERT_TEST_EQ(zmalloc_mt_single_thread_stress(), 0, "zmalloc_mt_single_thread_stress");
    ASSERT_TEST_EQ(zmalloc_mt_concurrent_stress(), 0, "zmalloc_mt_concurrent_stress");
    ASSERT_TEST_EQ(zmalloc_mt_cross_thread_stress(), 0, "zmalloc_mt_cross_thread_stress");
    ASSERT_TEST_EQ(zmalloc_mt_mixed_stress(), 0, "zmalloc_mt_mixed_stress");
    ASSERT_TEST_EQ(zmalloc_mt_vs_sys_benchmark(), 0, "zmalloc_mt_vs_sys_benchmark");

    LogInfo() << "";
    LogInfo() << "========== ALL zmalloc_mt tests PASSED ==========";
    return 0;
}
