
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

#define LOGRAW(fmt, ...) LOG_FORMAT(0, FNLog::PRIORITY_INFO, 0, 0, FNLog::LOG_PREFIX_NULL, fmt, ##__VA_ARGS__)

using test_graph = zgraph<s32, s32>;

static constexpr f32 kMapSizeCm = (f32)test_graph::kGridSize * 100.0f;

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
        f64 mid_ns = h.bucket_info(i).height;
        LOGFMTI("  bkt[%3d] [%12lld, %12lld) ns %s cnt=%lld mid=%.0fns",
                i, (long long)r.first, (long long)r.second, bar_buf, (long long)bucket_valid_count, mid_ns);
    }
}

static test_graph g_graph;
static std::vector<s32> g_node_ids;
static std::vector<s32> g_link_ids;
static std::vector<s32> g_extra_node_ids;

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
    hist.reset(0);

    g_link_ids.reserve(LINK_CNT);
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

            ASSERT_TEST_NOLOG(lid >= 0, "new_link at i=", i);
            ASSERT_TEST_NOLOG(ret == 0, "push_link at i=", i, " ret=", ret);
            g_link_ids.push_back(lid);
        }
    }

    dump_histogram("push_link (2000 links, new_link+push_link each) latency", hist);
    return 0;
}

static s32 zgraph_build_2000_nodes_test()
{
    ASSERT_TEST(!g_node_ids.empty(), "graph not built, run zgraph_build_2000_links_test first");

    std::mt19937 rng(20260830u);
    const int NODE_CNT = 2000;

    g_extra_node_ids.reserve(NODE_CNT);

    zstat_loghist<> hist;
    hist.reset(0);

    for (int i = 0; i < NODE_CNT; i++)
    {
        zpoint p = random_pos(rng);

        zclock<> one;
        one.start();

        s32 nid = g_graph.new_node(p, 3000000 + i);
        s32 ret = g_graph.push_node(nid);

        one.stop_and_save();
        hist.add(one.cost_ns());

        ASSERT_TEST_NOLOG(nid >= 0, "new_node at i=", i);
        ASSERT_TEST_NOLOG(ret == 0, "push_node at i=", i, " ret=", ret);
        g_extra_node_ids.push_back(nid);
    }

    dump_histogram("build 2000 standalone nodes (new_node+push_node each, link-independent) latency", hist);
    return 0;
}

static s32 zgraph_find_nearest_and_neighbor_bench_test()
{
    ASSERT_TEST(!g_node_ids.empty(), "graph not built, run zgraph_build_2000_links_test first");
    ASSERT_TEST(!g_extra_node_ids.empty(), "extra nodes not built, run zgraph_build_2000_nodes_test first");

    const int N = 200000;
    std::mt19937 rng(20260827u);
    std::vector<zpoint> points;
    points.reserve(N);
    for (int i = 0; i < N; i++)
    {
        points.push_back(random_pos(rng));
    }

    volatile s32 salt = 0;

    {
        zclock<> cost;
        cost.start();
        for (int i = 0; i < N; i++)
        {
            s32 out_link_id = -1;
            bool out_is_source = false;
            s32 ret = g_graph.find_nearest_node(points[i], out_link_id, out_is_source);
            salt += (ret == 0) ? out_link_id : 0;
        }
        cost.stop_and_save();
        LOGFMTI("zgraph.find_nearest_node() avg cost: %.2f ns/op  (calls=%d, links_in_graph=%zu)",
                (f64)cost.cost_ns() / (f64)N, N, g_node_ids.size() - 1);
    }

    {
        zclock<> cost;
        cost.start();
        for (int i = 0; i < N; i++)
        {
            s32 out_link_id = -1;
            zpoint out_nearest_pos;
            bool out_is_source_side = false;
            s32 ret = g_graph.find_nearest_link(points[i], out_link_id, out_nearest_pos, out_is_source_side);
            salt += (ret == 0) ? (out_link_id + (out_is_source_side ? 1 : 0)) : 0;
        }
        cost.stop_and_save();
        LOGFMTI("zgraph.find_nearest_link() avg cost: %.2f ns/op  (calls=%d, links_in_graph=%zu)",
                (f64)cost.cost_ns() / (f64)N, N, g_node_ids.size() - 1);
    }

    {
        zclock<> cost;
        cost.start();
        for (int i = 0; i < N; i++)
        {
            zarray<s32, 8> out_node_ids;
            g_graph.find_neighbor_nodes(points[i], out_node_ids);
            salt += (s32)out_node_ids.size();
        }
        cost.stop_and_save();
        LOGFMTI("zgraph.find_neighbor_nodes() avg cost: %.2f ns/op  (calls=%d, nodes_in_graph=%zu)",
                (f64)cost.cost_ns() / (f64)N, N, g_extra_node_ids.size());
    }

    {
        s32 anchor = g_extra_node_ids[0];
        zpoint anchor_pos = g_graph.ref_node(anchor)->pos;

        zarray<s32, 32> with_self;
        g_graph.find_neighbor_nodes(anchor_pos, with_self);
        bool has_self = std::find(with_self.begin(), with_self.end(), anchor) != with_self.end();
        ASSERT_TEST(has_self, "expect anchor itself in neighbor result without exclude");

        zarray<s32, 32> without_self;
        g_graph.find_neighbor_nodes(anchor_pos, without_self, anchor);
        bool still_has_self = std::find(without_self.begin(), without_self.end(), anchor) != without_self.end();
        ASSERT_TEST(!still_has_self, "expect anchor excluded from neighbor result via exclude_node_id");
    }

    {
        s32 anchor = g_extra_node_ids[0];
        zpoint anchor_pos = g_graph.ref_node(anchor)->pos;

        zarray<s32, 64> filtered;
        auto even_filter = [](const s32& v) { return (v % 2) == 0; };
        g_graph.find_neighbor_nodes(anchor_pos, filtered, -1, even_filter);
        for (s32 nid : filtered)
        {
            s32 v = g_graph.ref_node(nid)->node;
            ASSERT_TEST((v % 2) != 0, "filter should have removed even-value node id=", nid, " v=", v);
        }
    }

    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

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

        add_cost.start();
        s32 new_node_id = g_graph.new_node(new_pos, 1000000 + i);
        s32 link_id = g_graph.new_link(anchor, new_node_id, 2000000 + i);
        s32 affects = 0;
        s32 ret = g_graph.push_link(link_id, affects);
        add_cost.stop_and_save();

        add_ticks_ns += add_cost.cost_ns();
        add_hist.add(add_cost.cost_ns());

        ASSERT_TEST_NOLOG(new_node_id >= 0, "new_node at i=", i);
        ASSERT_TEST_NOLOG(link_id >= 0, "new_link at i=", i);
        ASSERT_TEST_NOLOG(ret == 0, "push_link at i=", i, " ret=", ret);

        del_cost.start();
        s32 pop_affects = 0;
        s32 pop_ret = g_graph.pop_link(link_id, pop_affects);
        s32 free_link_ret = g_graph.free_link(link_id);
        s32 free_node_ret = g_graph.free_node(new_node_id);
        del_cost.stop_and_save();

        del_ticks_ns += del_cost.cost_ns();
        del_hist.add(del_cost.cost_ns());

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

static s32 zgraph_destroy_test()
{
    ASSERT_TEST(!g_node_ids.empty(), "graph not built, run zgraph_build_2000_links_test first");
    ASSERT_TEST(!g_extra_node_ids.empty(), "extra nodes not built, run zgraph_build_2000_nodes_test first");

    {
        zstat_loghist<> hist;
        hist.reset(0);
        for (size_t i = 0; i < g_extra_node_ids.size(); i++)
        {
            zclock<> one;
            one.start();

            s32 pop_ret = g_graph.pop_node(g_extra_node_ids[i]);
            s32 free_ret = g_graph.free_node(g_extra_node_ids[i]);

            one.stop_and_save();
            hist.add(one.cost_ns());

            ASSERT_TEST_NOLOG(pop_ret == 0, "pop_node at i=", i, " ret=", pop_ret);
            ASSERT_TEST_NOLOG(free_ret == 0, "free_node at i=", i, " ret=", free_ret);
        }
        dump_histogram("destroy 2000 standalone nodes (pop_node+free_node each) latency", hist);
        g_extra_node_ids.clear();
    }

    {
        zstat_loghist<> hist;
        hist.reset(0);
        for (size_t i = 0; i < g_link_ids.size(); i++)
        {
            zclock<> one;
            one.start();

            s32 affects = 0;
            s32 pop_ret = g_graph.pop_link(g_link_ids[i], affects);
            s32 free_ret = g_graph.free_link(g_link_ids[i]);

            one.stop_and_save();
            hist.add(one.cost_ns());

            ASSERT_TEST_NOLOG(pop_ret == 0, "pop_link at i=", i, " ret=", pop_ret);
            ASSERT_TEST_NOLOG(free_ret == 0, "free_link at i=", i, " ret=", free_ret);
        }
        dump_histogram("destroy 2000 links (pop_link+free_link each) latency", hist);
        g_link_ids.clear();
    }

    {
        zstat_loghist<> hist;
        hist.reset(0);
        for (size_t i = 0; i < g_node_ids.size(); i++)
        {
            zclock<> one;
            one.start();

            s32 free_ret = g_graph.free_node(g_node_ids[i]);

            one.stop_and_save();
            hist.add(one.cost_ns());

            ASSERT_TEST_NOLOG(free_ret == 0, "free_node at i=", i, " ret=", free_ret);
        }
        dump_histogram("destroy 2001 link-topology nodes (free_node each) latency", hist);
        g_node_ids.clear();
    }

    return 0;
}


int main(int argc, char* argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zgraph_build_2000_links_test()                    == 0);
    ASSERT_TEST(zgraph_build_2000_nodes_test()                    == 0);
    ASSERT_TEST(zgraph_find_nearest_and_neighbor_bench_test()      == 0);
    ASSERT_TEST(zgraph_add_and_remove_one_bench_test()            == 0);
    ASSERT_TEST(zgraph_destroy_test()                             == 0);

    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();

    LogInfo() << "all test finish .";
    return 0;
}
