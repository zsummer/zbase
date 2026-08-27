
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#include <random>
#include <vector>
#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zgraph.h"
#include "zclock.h"
#include "zstat_loghist.h"

// 纯数据输出（无 timestamp/priority 前缀），用于直方图柱状打印
#define LOGRAW(fmt, ...) LOG_FORMAT(0, FNLog::PRIORITY_INFO, 0, 0, FNLog::LOG_PREFIX_NULL, fmt, ##__VA_ARGS__)

// Node/Link 用 s32 即可(trivial 类型, zarray/zlist 走 pod 分支, 性能更优)
using test_graph = zgraph<s32, s32>;

// 场景常量: 1km x 1km 地图, 10m 格子, 坐标用 f32 厘米单位
// zgraph::kGridSize == 1000cm(10m), kMaxGridCnt == 100*100, 恰好铺满 1km x 1km, 无需改动模板常量
static constexpr f32 kMapSizeCm = (f32)test_graph::kGridSize * 100.0f; // 100 * 10m = 1000m = 1km

static inline zpoint random_pos(std::mt19937& rng)
{
    std::uniform_real_distribution<float> dist(0.0f, kMapSizeCm - 1.0f);
    return zpoint(dist(rng), dist(rng), 0.0f);
}

template<class Hist>
static void dump_histogram(const char* title, const Hist& h)
{
    LogInfo() << "==== histogram: " << title
              << "  buckets=" << h.bucket_count()
              << "  req_count=" << h.req_count()
              << "  valid_count=" << h.valid_count()
              << "  valid_low(ns)=" << h.valid_low()
              << "  valid_high(ns)=" << h.valid_high()
              << "  valid_avg(ns)=" << h.valid_avg();

    LOGFMTI("  p50=%.1fns  p90=%.1fns  p95=%.1fns  p99=%.1fns",
            h.quantile(0.50).height, h.quantile(0.90).height,
            h.quantile(0.95).height, h.quantile(0.99).height);

    s64 max_bucket_count = 0;
    for (int i = 0; i < h.bucket_count(); ++i)
    {
        if (h.bucket_valid_count(i) > max_bucket_count) max_bucket_count = h.bucket_valid_count(i);
    }
    if (max_bucket_count == 0)
    {
        LogInfo() << "  (all buckets empty)";
        return;
    }
    const int BAR_MAX = 40;
    for (int i = 0; i < h.bucket_count(); ++i)
    {
        s64 bucket_valid_count = h.bucket_valid_count(i);
        if (bucket_valid_count == 0) continue;
        int bar = (int)((bucket_valid_count * BAR_MAX + max_bucket_count - 1) / max_bucket_count);
        char bar_buf[64] = { 0 };
        for (int k = 0; k < bar && k < 60; ++k) bar_buf[k] = '#';
        auto r = h.bucket_range(i);
        f64 mid_ns = h.bucket_info(i).height; // 当前桶的中心高度(ns)
        LOGFMTI("  bkt[%3d] [%12lld, %12lld) ns %s cnt=%lld mid=%.0fns",
                i, (long long)r.first, (long long)r.second, bar_buf, (long long)bucket_valid_count, mid_ns);
    }
}

// ---------------------------------------------------------------------------
// 1) 构建 1km x 1km / 10m 格子场景, 添加 2000 个 link, 用 zstat_loghist 记录
//    每次 (new_link + push_link) 的延迟分布; 同时用 zprof 对整体过程做一次性统计输出.
// ---------------------------------------------------------------------------
static test_graph g_graph;                 // 供后续 benchmark 复用(避免重复建场景)
static std::vector<s32> g_node_ids;         // 初始链上的 2001 个节点id, 供最近邻/新增点测试使用anchor

static s32 zgraph_build_2000_links_test()
{
    std::mt19937 rng(20260826u);
    const int LINK_CNT = 2000;
    const int NODE_CNT = LINK_CNT + 1;

    g_node_ids.reserve(NODE_CNT);
    for (int i = 0; i < NODE_CNT; i++)
    {
        zpoint p = random_pos(rng);
        s32 nid = g_graph.new_node(p, i);
        ASSERT_TEST_NOLOG(nid >= 0, "new_node at i=", i);
        g_node_ids.push_back(nid);
    }

    zstat_loghist<> hist;
    hist.reset(0); // 单位是 ns, 覆盖 1ns ~ 数十ms 足够

    // 整体用 zprof 统计: 用 RAII 匿名计时器包住"添加2000个link"全过程,
    // 作用域结束时自动把 (总耗时 / 次数) 输出到日志(cnt=LINK_CNT 触发均摊输出分支).
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, LINK_CNT, "zgraph: push 2000 links (zprof overall)");

        for (int i = 0; i < LINK_CNT; i++)
        {
            zclock<> one;
            one.start();

            s32 lid = g_graph.new_link(g_node_ids[i], g_node_ids[i + 1], i);
            s32 affects = 0;
            s32 ret = g_graph.push_link(lid, affects);

            one.stop_and_save();
            hist.add(one.cost_ns());

            // 校验放在计时区间之外, 避免日志I/O污染耗时统计; 用 _NOLOG 版本, 只在真失败时才打日志(避免刷屏)
            ASSERT_TEST_NOLOG(lid >= 0, "new_link at i=", i);
            ASSERT_TEST_NOLOG(ret == 0, "push_link at i=", i, " ret=", ret);
        }
    }
    // guard 析构时已经通过 PROF_OUTPUT_TEMP_RECORD 输出了一次 "zprof" 汇总

    dump_histogram("push_link (2000 links, new_link+push_link each) latency", hist);
    return 0;
}

// ---------------------------------------------------------------------------
// 2) 九宫格找最近邻 性能对比: node 不再单独占用空间索引(见 zgraph.h terrain 注释),
//    find_nearest_node 改为遍历候选link的两个端点比距离; find_nearest_link 则对
//    候选link做"点到线段"投影(画垂线求垂足), 计算量比只比端点更大.
//    两组测试复用同一份随机查询点序列(独立拷贝rng状态), 保证对比公平.
// ---------------------------------------------------------------------------
static s32 zgraph_find_nearest_bench_test()
{
    ASSERT_TEST(!g_node_ids.empty(), "graph not built, run zgraph_build_2000_links_test first");

    std::mt19937 rng_seed(20260827u);
    const int N = 200000;
    volatile s32 salt = 0;

    // 2a) find_nearest_node: 只比较候选link的两个端点(source/target)与查询点的距离
    {
        std::mt19937 rng = rng_seed;
        zclock<> cost;
        cost.start();
        for (int i = 0; i < N; i++)
        {
            zpoint q = random_pos(rng);
            s32 out_link_id = -1;
            bool out_is_source = false;
            s32 ret = g_graph.find_nearest_node(q, out_link_id, out_is_source);
            salt += (ret == 0) ? out_link_id : 0;
        }
        cost.stop_and_save();
        LOGFMTI("zgraph.find_nearest_node() avg cost: %.2f ns/op  (calls=%d, links_in_graph=%zu)",
                (f64)cost.cost_ns() / (f64)N, N, g_node_ids.size() - 1);
    }

    // 2b) find_nearest_link: 对每条候选link做点到线段(垂线)投影求真正最近点, 而不是只比端点
    {
        std::mt19937 rng = rng_seed; // 与2a相同的查询点序列, 保证两者可比
        zclock<> cost;
        cost.start();
        for (int i = 0; i < N; i++)
        {
            zpoint q = random_pos(rng);
            s32 out_link_id = -1;
            zpoint out_nearest_pos;
            bool out_is_source_side = false;
            s32 ret = g_graph.find_nearest_link(q, out_link_id, out_nearest_pos, out_is_source_side);
            salt += (ret == 0) ? (out_link_id + (out_is_source_side ? 1 : 0)) : 0;
        }
        cost.stop_and_save();
        LOGFMTI("zgraph.find_nearest_link() avg cost: %.2f ns/op  (calls=%d, links_in_graph=%zu)",
                (f64)cost.cost_ns() / (f64)N, N, g_node_ids.size() - 1);
    }

    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

// ---------------------------------------------------------------------------
// 3) 新增一个点 + 与已有点 new_link + push_link 的总开销
//    4) 对应删除该 link 与该顶点(pop_link + free_link + free_node)的总开销
//    两者在同一循环里配对进行(增->测时A, 删->测时B), 保证图规模在整个 benchmark
//    期间维持在"2001节点/2000link"这个规模附近, 不会无限增长撞容量上限.
// ---------------------------------------------------------------------------
static s32 zgraph_add_and_remove_one_bench_test()
{
    ASSERT_TEST(!g_node_ids.empty(), "graph not built, run zgraph_build_2000_links_test first");

    std::mt19937 rng(20260828u);
    std::uniform_int_distribution<size_t> anchor_dist(0, g_node_ids.size() - 1);

    const int LOOPS = 2000;
    zstat_loghist<> add_hist;
    add_hist.reset(0);
    zstat_loghist<> del_hist;
    del_hist.reset(0);

    zclock<> add_cost;
    zclock<> del_cost;
    s64 add_ticks_ns = 0;
    s64 del_ticks_ns = 0;

    for (int i = 0; i < LOOPS; i++)
    {
        s32 anchor = g_node_ids[anchor_dist(rng)];
        zpoint new_pos = random_pos(rng);

        // ---- A: 新增一个点 + 与已有点建立link + push_link ----
        add_cost.start();
        s32 new_node_id = g_graph.new_node(new_pos, 1000000 + i);
        s32 link_id = g_graph.new_link(anchor, new_node_id, 2000000 + i);
        s32 affects = 0;
        s32 ret = g_graph.push_link(link_id, affects);
        add_cost.stop_and_save();

        add_ticks_ns += add_cost.cost_ns();
        add_hist.add(add_cost.cost_ns());

        // 校验放在计时区间之外, 用 _NOLOG 版本, 只在真失败时才打日志(避免刷屏)
        ASSERT_TEST_NOLOG(new_node_id >= 0, "new_node at i=", i);
        ASSERT_TEST_NOLOG(link_id >= 0, "new_link at i=", i);
        ASSERT_TEST_NOLOG(ret == 0, "push_link at i=", i, " ret=", ret);

        // ---- B: 删除对应的这一份 link 和顶点 ----
        del_cost.start();
        s32 pop_affects = 0;
        s32 pop_ret = g_graph.pop_link(link_id, pop_affects);
        s32 free_link_ret = g_graph.free_link(link_id);
        s32 free_node_ret = g_graph.free_node(new_node_id);
        del_cost.stop_and_save();

        del_ticks_ns += del_cost.cost_ns();
        del_hist.add(del_cost.cost_ns());

        // 校验放在计时区间之外, 用 _NOLOG 版本, 只在真失败时才打日志(避免刷屏)
        ASSERT_TEST_NOLOG(pop_ret == 0, "pop_link at i=", i, " ret=", pop_ret);
        ASSERT_TEST_NOLOG(free_link_ret == 0, "free_link at i=", i, " ret=", free_link_ret);
        ASSERT_TEST_NOLOG(free_node_ret == 0, "free_node at i=", i, " ret=", free_node_ret);
    }

    LOGFMTI("zgraph: add(new_node+new_link+push_link)    avg cost: %.2f ns/op  (loops=%d)",
            (f64)add_ticks_ns / (f64)LOOPS, LOOPS);
    LOGFMTI("zgraph: del(pop_link+free_link+free_node)    avg cost: %.2f ns/op  (loops=%d)",
            (f64)del_ticks_ns / (f64)LOOPS, LOOPS);

    dump_histogram("add one point+link+push_link latency", add_hist);
    dump_histogram("del one link+node (pop_link+free_link+free_node) latency", del_hist);
    return 0;
}


int main(int argc, char* argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zgraph_build_2000_links_test()          == 0);
    ASSERT_TEST(zgraph_find_nearest_bench_test()        == 0);
    ASSERT_TEST(zgraph_add_and_remove_one_bench_test()  == 0);

    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();

    LogInfo() << "all test finish .";
    return 0;
}
