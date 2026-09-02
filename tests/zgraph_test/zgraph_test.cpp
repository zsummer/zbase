
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#include <random>
#include <vector>
#include <array>
#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zgraph.h"
#include "zclock.h"
#include "zstat_loghist.h"

#define LOGRAW(fmt, ...) LOG_FORMAT(0, FNLog::PRIORITY_INFO, 0, 0, FNLog::LOG_PREFIX_NULL, fmt, ##__VA_ARGS__)

using test_graph = zgraph<s32, s32>;

struct narrow_open_config : public DefaultGraphConfig
{
    static constexpr s32 kMaxOpenCnt = 8;
};

struct narrow_grid_config : public DefaultGraphConfig
{
    static constexpr s32 kMaxGridCnt = 4;
};

static constexpr f32 kMapSizeCm = (f32)test_graph::kGridSize * 30.0f;

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

static s32 zgraph_build_chain_links_test()
{
    std::mt19937 rng(20260826u);
    const int LINK_CNT = 400;
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
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, LINK_CNT, "zgraph: push 400 links (zprof overall)");

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

    dump_histogram("push_link (400 links, new_link+push_link each) latency", hist);
    return 0;
}

static s32 zgraph_build_extra_nodes_test()
{
    ASSERT_TEST(!g_node_ids.empty(), "graph not built, run zgraph_build_chain_links_test first");

    std::mt19937 rng(20260830u);
    const int NODE_CNT = 400;

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

    dump_histogram("build 400 standalone nodes (new_node+push_node each, link-independent) latency", hist);
    return 0;
}

static s32 zgraph_find_nearest_and_neighbor_bench_test()
{
    ASSERT_TEST(!g_node_ids.empty(), "graph not built, run zgraph_build_chain_links_test first");
    ASSERT_TEST(!g_extra_node_ids.empty(), "extra nodes not built, run zgraph_build_extra_nodes_test first");

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
            s32 out_slot = -1;
            zpoint out_nearest_pos;
            f32 out_slot_sq_dist = 0.0f;
            s32 ret = g_graph.find_nearest_link(points[i], out_link_id, out_slot, out_nearest_pos, out_slot_sq_dist);
            salt += (ret == 0) ? (out_link_id + out_slot + (s32)out_slot_sq_dist) : 0;
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
            s32 v = g_graph.ref_node(nid)->data;
            ASSERT_TEST((v % 2) != 0, "filter should have removed even-value node id=", nid, " v=", v);
        }
    }

    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

static s32 zgraph_add_and_remove_one_bench_test()
{
    ASSERT_TEST(!g_node_ids.empty(), "graph not built, run zgraph_build_chain_links_test first");

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

static s32 astar_group_report(const char* group_name, s32 loops, s32 ok_cnt, s64 step_total, s64 cost_ns)
{
    LOGFMTI("group[%s] avg cost: %.2f ns/op  (calls=%d, ok=%d, avg_steps=%.1f)",
            group_name, (f64)cost_ns / (f64)loops, loops, ok_cnt,
            ok_cnt > 0 ? (f64)step_total / (f64)ok_cnt : 0.0);
    LOGFMTI("group[%s] open peak: %u / %d   visit peak: %u / %d",
            group_name, g_graph.path_open_peak(), test_graph::kMaxOpenCnt,
            g_graph.path_visit_peak(), test_graph::kMaxNodeCnt);
    LOGFMTI("group[%s] nodes: %d / %d   links: %d / %d   grids: %d / %d",
            group_name, g_graph.node_count(), test_graph::kMaxNodeCnt,
            g_graph.link_count(), test_graph::kMaxLinkCnt,
            g_graph.grid_count(), test_graph::kMaxGridCnt);
    return 0;
}

static s32 astar_turns_group_run(const char* group_name, std::mt19937& rng, s32 loops,
                                 s32 gap_min, s32 gap_max, volatile s32& salt)
{
    std::uniform_int_distribution<s32> gap_dist(gap_min, gap_max);
    std::uniform_int_distribution<s32> head_dist(0, (s32)g_node_ids.size() - 1 - gap_max);
    std::vector<test_graph::graph_path_step> steps;
    g_graph.path_peak_reset();
    zclock<> cost;
    cost.start();
    s32 ok_cnt = 0;
    s64 step_total = 0;
    for (s32 i = 0; i < loops; i++)
    {
        s32 head = head_dist(rng);
        s32 tail = head + gap_dist(rng);
        s32 ret = g_graph.find_path(g_node_ids[head], g_node_ids[tail], steps);
        ASSERT_TEST_NOLOG(ret != -4, "open heap overflow in group ", group_name, " at i=", i);
        if (ret == 0)
        {
            ok_cnt++;
            step_total += (s64)steps.size();
            salt += (s32)steps.size();
        }
    }
    cost.stop_and_save();
    return astar_group_report(group_name, loops, ok_cnt, step_total, cost.cost_ns());
}

static s32 astar_diagonal_group_run(std::mt19937& rng, s32 loops, volatile s32& salt)
{
    std::uniform_real_distribution<float> low_dist(0.0f, kMapSizeCm * 0.05f);
    std::uniform_real_distribution<float> high_dist(kMapSizeCm * 0.95f, kMapSizeCm - 1.0f);
    std::uniform_int_distribution<s32> flip_dist(0, 1);
    std::vector<test_graph::graph_path_step> steps;
    g_graph.path_peak_reset();
    zclock<> cost;
    cost.start();
    s32 ok_cnt = 0;
    s64 step_total = 0;
    for (s32 i = 0; i < loops; )
    {
        zpoint from(low_dist(rng), low_dist(rng), 0.0f);
        zpoint to(high_dist(rng), high_dist(rng), 0.0f);
        if (flip_dist(rng) == 1)
        {
            from.y = high_dist(rng);
            to.y = low_dist(rng);
        }
        s32 ret = g_graph.find_path(from, to, steps);
        if (ret == -1 || ret == -2)
        {
            continue;
        }
        i++;
        ASSERT_TEST_NOLOG(ret != -4, "open heap overflow in group diagonal at i=", i);
        if (ret == 0)
        {
            ok_cnt++;
            step_total += (s64)steps.size();
            salt += (s32)steps.size();
        }
    }
    cost.stop_and_save();
    return astar_group_report("diagonal", loops, ok_cnt, step_total, cost.cost_ns());
}

static s32 zgraph_astar_bench_test()
{
    ASSERT_TEST(!g_node_ids.empty(), "graph not built, run zgraph_build_chain_links_test first");
    const s32 N = 20000;
    std::mt19937 rng(20260830u);
    volatile s32 salt = 0;
    {
        std::vector<test_graph::graph_path_step> warm_steps;
        for (s32 i = 0; i < 100; i++)
        {
            g_graph.find_path(g_node_ids[i], g_node_ids[g_node_ids.size() - 1 - i], warm_steps);
        }
    }
    ASSERT_TEST(astar_turns_group_run("turns03", rng, N, 3, 5, salt) == 0, "group turns03 fail");
    ASSERT_TEST(astar_turns_group_run("turns10", rng, N, 10, 12, salt) == 0, "group turns10 fail");
    ASSERT_TEST(astar_turns_group_run("turns20", rng, N, 20, 22, salt) == 0, "group turns20 fail");
    ASSERT_TEST(astar_diagonal_group_run(rng, N, salt) == 0, "group diagonal fail");
    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

static s32 zgraph_destroy_test()
{
    ASSERT_TEST(!g_node_ids.empty(), "graph not built, run zgraph_build_chain_links_test first");
    ASSERT_TEST(!g_extra_node_ids.empty(), "extra nodes not built, run zgraph_build_extra_nodes_test first");

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
        dump_histogram("destroy 400 standalone nodes (pop_node+free_node each) latency", hist);
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
        dump_histogram("destroy 400 links (pop_link+free_link each) latency", hist);
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


static s32 zgraph_astar_grid_test()
{
    test_graph g;
    const s32 W = 8;
    const s32 H = 8;
    const f32 STEP = (f32)test_graph::kGridSize;
    s32 ids[8][8];
    for (s32 y = 0; y < H; y++)
    {
        for (s32 x = 0; x < W; x++)
        {
            ids[x][y] = g.new_node(zpoint((f32)x * STEP, (f32)y * STEP, 0.0f), y * W + x);
            ASSERT_TEST_NOLOG(ids[x][y] >= 0, "new_node fail at x=", x, " y=", y);
        }
    }
    for (s32 y = 0; y < H; y++)
    {
        for (s32 x = 0; x < W; x++)
        {
            if (x + 1 < W)
            {
                s32 lid = g.new_link(ids[x][y], ids[x + 1][y], 0);
                ASSERT_TEST_NOLOG(lid >= 0, "new_link fail at x=", x, " y=", y);
            }
            if (y + 1 < H)
            {
                s32 lid = g.new_link(ids[x][y], ids[x][y + 1], 0);
                ASSERT_TEST_NOLOG(lid >= 0, "new_link fail at x=", x, " y=", y);
            }
        }
    }

    std::vector<test_graph::graph_path_step> steps;
    s32 ret = g.find_path(ids[0][0], ids[7][7], steps);
    ASSERT_TEST(ret == 0, "find_path fail ret=", ret);
    ASSERT_TEST((s32)steps.size() == 14, "expect 14 steps, got=", (s32)steps.size());

    f32 total = 0.0f;
    s32 curr = ids[0][0];
    for (size_t i = 0; i < steps.size(); i++)
    {
        const test_graph::graph_path_step& step = steps[i];
        ASSERT_TEST(step.link >= 0, "grid step link_id invalid at i=", (s32)i);
        test_graph::graph_link* link = g.ref_link(step.link);
        ASSERT_TEST(link->node[0] == curr || link->node[1] == curr, "path discontinuity at i=", (s32)i);
        s32 depart_slot = link->node[0] == curr ? 0 : 1;
        s32 next = link->node[1 - depart_slot];
        ASSERT_TEST(step.slot == 1 - depart_slot, "step slot mismatch at i=", (s32)i);
        f32 dx = g.ref_node(next)->pos.x - g.ref_node(curr)->pos.x;
        f32 dy = g.ref_node(next)->pos.y - g.ref_node(curr)->pos.y;
        ASSERT_TEST(fabsf(step.pos.x - g.ref_node(next)->pos.x) < 0.001f
                    && fabsf(step.pos.y - g.ref_node(next)->pos.y) < 0.001f,
                    "step pos mismatch at i=", (s32)i);
        total += sqrtf(dx * dx + dy * dy);
        curr = next;
    }
    ASSERT_TEST(curr == ids[7][7], "path endpoint mismatch");
    ASSERT_TEST(fabsf(total - 14.0f * STEP) < 1.0f, "expect total len 14000, got=", total);

    ret = g.find_path(ids[3][3], ids[3][3], steps);
    ASSERT_TEST(ret == 0 && steps.empty(), "source==target expect 0 with empty steps, ret=", ret);
    ret = g.find_path(-1, ids[0][0], steps);
    ASSERT_TEST(ret == -1, "invalid source expect -1, ret=", ret);
    ret = g.find_path(ids[0][0], 99999, steps);
    ASSERT_TEST(ret == -2, "invalid target expect -2, ret=", ret);
    return 0;
}

static s32 astar_corridor_build(test_graph& g, s32 short_cost, s32& out_a, s32& out_c)
{
    out_a = g.new_node(zpoint(0, 0, 0), 0);
    s32 b = g.new_node(zpoint(1000, 0, 0), 1);
    out_c = g.new_node(zpoint(2000, 0, 0), 2);
    s32 d = g.new_node(zpoint(0, 1000, 0), 3);
    s32 e = g.new_node(zpoint(1000, 1000, 0), 4);
    s32 f = g.new_node(zpoint(2000, 1000, 0), 5);
    ASSERT_TEST_NOLOG(out_a >= 0 && b >= 0 && out_c >= 0 && d >= 0 && e >= 0 && f >= 0, "corridor new_node fail");
    s32 l1 = g.new_link(out_a, b, 0, 0, short_cost);
    s32 l2 = g.new_link(b, out_c, 0);
    s32 l3 = g.new_link(out_a, d, 0);
    s32 l4 = g.new_link(d, e, 0);
    s32 l5 = g.new_link(e, f, 0);
    s32 l6 = g.new_link(f, out_c, 0);
    ASSERT_TEST_NOLOG(l1 >= 0 && l2 >= 0 && l3 >= 0 && l4 >= 0 && l5 >= 0 && l6 >= 0, "corridor new_link fail");
    return 0;
}

static s32 zgraph_astar_cost_test()
{
    test_graph g;
    s32 a = -1;
    s32 c = -1;
    ASSERT_TEST(astar_corridor_build(g, 30000, a, c) == 0, "corridor build fail");
    std::vector<test_graph::graph_path_step> steps;
    s32 ret = g.find_path(a, c, steps);
    ASSERT_TEST(ret == 0, "find_path fail ret=", ret);
    ASSERT_TEST((s32)steps.size() == 4, "expensive short corridor should be avoided, steps=", (s32)steps.size());

    test_graph g2;
    s32 a2 = -1;
    s32 c2 = -1;
    ASSERT_TEST(astar_corridor_build(g2, 0, a2, c2) == 0, "corridor build fail");
    std::vector<test_graph::graph_path_step> steps2;
    ret = g2.find_path(a2, c2, steps2);
    ASSERT_TEST(ret == 0, "find_path fail ret=", ret);
    ASSERT_TEST((s32)steps2.size() == 2, "cheap short corridor should be taken, steps=", (s32)steps2.size());
    return 0;
}

static s32 zgraph_astar_unreachable_test()
{
    test_graph g;
    s32 a = g.new_node(zpoint(0, 0, 0), 0);
    s32 b = g.new_node(zpoint(1000, 0, 0), 1);
    s32 c = g.new_node(zpoint(5000, 0, 0), 2);
    s32 d = g.new_node(zpoint(6000, 0, 0), 3);
    ASSERT_TEST_NOLOG(a >= 0 && b >= 0 && c >= 0 && d >= 0, "unreachable new_node fail");
    ASSERT_TEST_NOLOG(g.new_link(a, b, 0) >= 0, "unreachable new_link fail");
    ASSERT_TEST_NOLOG(g.new_link(c, d, 0) >= 0, "unreachable new_link fail");
    std::vector<test_graph::graph_path_step> steps;
    s32 ret = g.find_path(a, d, steps);
    ASSERT_TEST(ret == -3, "unreachable should return -3, ret=", ret);
    ASSERT_TEST(steps.empty(), "unreachable steps should be empty");
    return 0;
}

static s32 zgraph_astar_composite_test()
{
    test_graph g;
    s32 a = g.new_node(zpoint(0, 0, 0), 0);
    s32 b = g.new_node(zpoint(1000, 0, 0), 1);
    s32 c = g.new_node(zpoint(2000, 0, 0), 2);
    ASSERT_TEST_NOLOG(a >= 0 && b >= 0 && c >= 0, "composite new_node fail");
    s32 lab = g.new_link(a, b, 0);
    s32 lbc = g.new_link(b, c, 0);
    ASSERT_TEST_NOLOG(lab >= 0 && lbc >= 0, "composite new_link fail");
    s32 affects = 0;
    ASSERT_TEST_NOLOG(g.push_link(lab, affects) == 0, "composite push_link fail");
    ASSERT_TEST_NOLOG(g.push_link(lbc, affects) == 0, "composite push_link fail");

    std::vector<test_graph::graph_path_step> steps;
    zpoint from(200.0f, -500.0f, 0.0f);
    zpoint to(1800.0f, 300.0f, 0.0f);
    s32 ret = g.find_path(from, to, steps);
    ASSERT_TEST(ret == 0, "composite find_path fail ret=", ret);
    ASSERT_TEST((s32)steps.size() == 4, "composite expect 4 steps, got=", (s32)steps.size());
    if (steps.size() == 4)
    {
        ASSERT_TEST(steps[0].link == -1 && steps[0].slot == -1
                    && fabsf(steps[0].pos.x - 200.0f) < 0.001f && fabsf(steps[0].pos.y) < 0.001f,
                    "composite walk-a step mismatch");
        ASSERT_TEST(steps[1].link == lab && steps[1].slot == 1
                    && fabsf(steps[1].pos.x - 1000.0f) < 0.001f,
                    "composite seed step mismatch");
        ASSERT_TEST(steps[2].link == lbc && steps[2].slot == 1
                    && fabsf(steps[2].pos.x - 1800.0f) < 0.001f,
                    "composite truncated step mismatch");
        ASSERT_TEST(steps[3].link == -1 && steps[3].slot == -1
                    && fabsf(steps[3].pos.x - 1800.0f) < 0.001f && fabsf(steps[3].pos.y - 300.0f) < 0.001f,
                    "composite walk-b step mismatch");
    }

    zpoint from2(300.0f, -400.0f, 0.0f);
    zpoint to2(700.0f, 400.0f, 0.0f);
    ret = g.find_path(from2, to2, steps);
    ASSERT_TEST(ret == 0, "same-link find_path fail ret=", ret);
    ASSERT_TEST((s32)steps.size() == 3, "same-link expect 3 steps, got=", (s32)steps.size());
    if (steps.size() == 3)
    {
        ASSERT_TEST(steps[0].link == -1 && fabsf(steps[0].pos.x - 300.0f) < 0.001f,
                    "same-link walk-a step mismatch");
        ASSERT_TEST(steps[1].link == lab && steps[1].slot == 1
                    && fabsf(steps[1].pos.x - 700.0f) < 0.001f,
                    "same-link ride step mismatch");
        ASSERT_TEST(steps[2].link == -1 && fabsf(steps[2].pos.x - 700.0f) < 0.001f
                    && fabsf(steps[2].pos.y - 400.0f) < 0.001f,
                    "same-link walk-b step mismatch");
    }

    test_graph::graph_find_option opt;
    opt.link_color = 2;
    ret = g.find_path(a, c, steps, opt);
    ASSERT_TEST(ret == -3, "color filter should make path unreachable, ret=", ret);

    ret = g.find_path(zpoint(50000.0f, 50000.0f, 0.0f), to, steps);
    ASSERT_TEST(ret == -1, "no nearby link for from expect -1, ret=", ret);
    ret = g.find_path(from, zpoint(50000.0f, 50000.0f, 0.0f), steps);
    ASSERT_TEST(ret == -2, "no nearby link for to expect -2, ret=", ret);
    return 0;
}

static s32 zgraph_adjacency_chain_test()
{
    static test_graph g;

    s32 n0 = g.new_node(zpoint(100, 100, 0), 0);
    s32 n1 = g.new_node(zpoint(200, 100, 0), 1);
    s32 n2 = g.new_node(zpoint(300, 100, 0), 2);
    s32 n3 = g.new_node(zpoint(400, 100, 0), 3);
    ASSERT_TEST(n0 >= 0 && n1 >= 0 && n2 >= 0 && n3 >= 0, "new_node fail");

    ASSERT_TEST(g.ref_node(n0)->first_link[0] == -1 && g.ref_node(n0)->first_link[1] == -1,
                "fresh node first_link should be -1");

    s32 l01 = g.new_link(n0, n1, 0);
    s32 l02 = g.new_link(n0, n2, 1);
    s32 l03 = g.new_link(n0, n3, 2);
    s32 l21 = g.new_link(n2, n1, 3);
    ASSERT_TEST(l01 >= 0 && l02 >= 0 && l03 >= 0 && l21 >= 0, "new_link fail");

    {
        std::vector<s32> out_chain;
        s32 curr = g.ref_node(n0)->first_link[0];
        while (curr != -1)
        {
            out_chain.push_back(curr);
            curr = g.ref_link(curr)->next[0];
        }
        std::vector<s32> expect = { l03, l02, l01 };
        ASSERT_TEST(out_chain == expect, "n0 slot0 (out) chain order broken");
    }
    {
        std::vector<s32> in_chain;
        s32 curr = g.ref_node(n1)->first_link[1];
        while (curr != -1)
        {
            in_chain.push_back(curr);
            curr = g.ref_link(curr)->next[1];
        }
        std::vector<s32> expect = { l21, l01 };
        ASSERT_TEST(in_chain == expect, "n1 slot1 (in) chain order broken");
    }
    {
        std::vector<s32> out_chain;
        s32 curr = g.ref_node(n2)->first_link[0];
        while (curr != -1)
        {
            out_chain.push_back(curr);
            curr = g.ref_link(curr)->next[0];
        }
        std::vector<s32> expect = { l21 };
        ASSERT_TEST(out_chain == expect, "n2 slot0 (out) chain order broken");
    }
    {
        ASSERT_TEST(g.ref_node(n0)->first_link[1] == -1, "n0 has no in-links, slot1 should be empty");
        ASSERT_TEST(g.ref_node(n2)->first_link[1] == l02, "n2 slot1 should hold its only in-link l02");
        ASSERT_TEST(g.ref_node(n1)->first_link[0] == -1, "n1 has no out-links, slot0 should be empty");
        ASSERT_TEST(g.ref_node(n3)->first_link[0] == -1, "n3 has no out-links, slot0 should be empty");
    }

    {
        s32 ret = g.free_link(l02);
        ASSERT_TEST(ret == 0, "free middle link fail ret=", ret);
        std::vector<s32> out_chain;
        s32 curr = g.ref_node(n0)->first_link[0];
        while (curr != -1)
        {
            out_chain.push_back(curr);
            curr = g.ref_link(curr)->next[0];
        }
        std::vector<s32> expect = { l03, l01 };
        ASSERT_TEST(out_chain == expect, "n0 slot0 chain broken after removing middle link");

        std::vector<s32> in_chain;
        curr = g.ref_node(n1)->first_link[1];
        while (curr != -1)
        {
            in_chain.push_back(curr);
            curr = g.ref_link(curr)->next[1];
        }
        std::vector<s32> in_expect = { l21, l01 };
        ASSERT_TEST(in_chain == in_expect, "n1 slot1 chain broken after removing middle link");
    }

    {
        s32 ret = g.free_link(l03);
        ASSERT_TEST(ret == 0, "free head link fail ret=", ret);
        std::vector<s32> out_chain;
        s32 curr = g.ref_node(n0)->first_link[0];
        while (curr != -1)
        {
            out_chain.push_back(curr);
            curr = g.ref_link(curr)->next[0];
        }
        std::vector<s32> expect = { l01 };
        ASSERT_TEST(out_chain == expect, "n0 slot0 chain broken after removing head link");
    }

    {
        s32 ret = g.free_link(l01);
        ASSERT_TEST(ret == 0, "free last link fail ret=", ret);
        ASSERT_TEST(g.ref_node(n0)->first_link[0] == -1, "n0 slot0 chain should be empty");
        ASSERT_TEST(g.ref_node(n1)->first_link[1] == l21, "n1 slot1 chain should still hold l21");
        ASSERT_TEST(g.ref_node(n0)->refs == 0, "n0 refs should drop to 0");
    }

    {
        s32 l_new = g.new_link(n0, n2, 9);
        ASSERT_TEST(l_new >= 0, "new_link after chain empty fail");
        std::vector<s32> out_chain;
        s32 curr = g.ref_node(n0)->first_link[0];
        while (curr != -1)
        {
            out_chain.push_back(curr);
            curr = g.ref_link(curr)->next[0];
        }
        ASSERT_TEST(out_chain.size() == 1 && out_chain[0] == l_new, "reused slot should be the only out-link on n0");
        s32 ret = g.free_link(l_new);
        ASSERT_TEST(ret == 0, "free reused link fail ret=", ret);
    }

    {
        s32 l_self = g.new_link(n3, n3, 10);
        ASSERT_TEST(l_self >= 0, "new self-loop link fail");
        ASSERT_TEST(g.ref_node(n3)->first_link[0] == l_self && g.ref_node(n3)->first_link[1] == l_self,
                    "self-loop should sit on both chains of n3");
        ASSERT_TEST(g.ref_node(n3)->refs == 2, "self-loop should add 2 refs");
        s32 ret = g.free_link(l_self);
        ASSERT_TEST(ret == 0, "free self-loop fail ret=", ret);
        ASSERT_TEST(g.ref_node(n3)->first_link[0] == -1 && g.ref_node(n3)->first_link[1] == -1,
                    "n3 both chains should be empty after self-loop free");
        ASSERT_TEST(g.ref_node(n3)->refs == 0, "n3 refs should drop to 0");
    }

    return 0;
}


static s32 zgraph_negative_coord_test()
{
    ASSERT_TEST(test_graph::to_terrain_key(0, -1) != test_graph::to_terrain_key(1, -1),
                "negative y must not swallow x in terrain key");
    ASSERT_TEST(test_graph::to_terrain_key(-1, 0) != test_graph::to_terrain_key(-1, -1),
                "negative x rows must keep y apart");
    ASSERT_TEST(test_graph::to_terrain_key(-1, -1) != test_graph::to_terrain_key(0, 0),
                "negative quadrant must not alias origin");

    static test_graph g;
    const f32 step = (f32)test_graph::kGridSize;
    s32 a = g.new_node(zpoint(-3.5f * step, -3.5f * step, 0), 0);
    s32 b = g.new_node(zpoint(-2.5f * step, -3.5f * step, 0), 1);
    s32 c = g.new_node(zpoint(-1.5f * step, -3.5f * step, 0), 2);
    ASSERT_TEST(a >= 0 && b >= 0 && c >= 0, "negative quadrant new_node fail");

    s32 lab = g.new_link(a, b, 0);
    s32 lbc = g.new_link(b, c, 0);
    ASSERT_TEST(lab >= 0 && lbc >= 0, "negative quadrant new_link fail");

    s32 affects = 0;
    ASSERT_TEST(g.push_link(lab, affects) == 0, "negative quadrant push_link lab fail");
    ASSERT_TEST(affects == 2, "lab should cover 2 grids, affects=", affects);
    ASSERT_TEST(g.push_link(lbc, affects) == 0, "negative quadrant push_link lbc fail");
    ASSERT_TEST(affects == 2, "lbc should cover 2 grids, affects=", affects);
    ASSERT_TEST(g.grid_count() == 3, "3 distinct negative grids expect 3, got=", g.grid_count());

    ASSERT_TEST(g.push_node(a) == 0, "negative quadrant push_node a fail");
    ASSERT_TEST(g.push_node(c) == 0, "negative quadrant push_node c fail");

    s32 near_link = -1;
    s32 near_slot = -1;
    zpoint near_pos;
    f32 near_sq = 0.0f;
    s32 ret = g.find_nearest_link(zpoint(-3.4f * step, -3.4f * step, 0), near_link, near_slot, near_pos, near_sq);
    ASSERT_TEST(ret == 0 && near_link == lab, "negative quadrant nearest link expect lab, ret=", ret);

    zarray<s32, 8> neighbors;
    ASSERT_TEST(g.find_neighbor_nodes(zpoint(-3.4f * step, -3.4f * step, 0), neighbors) == 0,
                "negative quadrant find_neighbor_nodes fail");
    ASSERT_TEST(neighbors.size() == 1 && neighbors[0] == a,
                "negative quadrant neighbor expect only a, size=", (s32)neighbors.size());

    std::vector<test_graph::graph_path_step> steps;
    ret = g.find_path(zpoint(-3.5f * step, -3.5f * step, 0), zpoint(-1.5f * step, -3.5f * step, 0), steps);
    ASSERT_TEST(ret == 0, "negative quadrant find_path by pos fail ret=", ret);

    ASSERT_TEST(g.pop_node(a) == 0, "negative quadrant pop_node a fail");
    ASSERT_TEST(g.pop_node(c) == 0, "negative quadrant pop_node c fail");
    ASSERT_TEST(g.pop_link(lab, affects) == 0, "negative quadrant pop_link lab fail");
    ASSERT_TEST(affects == 2, "pop lab affects expect 2, got=", affects);
    ASSERT_TEST(g.pop_link(lbc, affects) == 0, "negative quadrant pop_link lbc fail");
    ASSERT_TEST(g.grid_count() == 0, "all negative grids should be recycled, got=", g.grid_count());
    return 0;
}

static s32 zgraph_stale_handle_test()
{
    static test_graph g;
    s32 a = g.new_node(zpoint(100, 100, 0), 0);
    s32 b = g.new_node(zpoint(200, 100, 0), 1);
    ASSERT_TEST(a >= 0 && b >= 0, "stale handle new_node fail");
    s32 lab = g.new_link(a, b, 0);
    ASSERT_TEST(lab >= 0, "stale handle new_link fail");

    ASSERT_TEST(g.is_valid_link(lab), "live link should be valid");
    ASSERT_TEST(g.free_link(lab) == 0, "stale handle free_link fail");
    ASSERT_TEST(!g.is_valid_link(lab), "freed link handle must be invalid");
    ASSERT_TEST(g.ref_link(lab) == nullptr, "freed link ref must be nullptr");
    ASSERT_TEST(g.free_link(lab) == -1, "double free_link expect -1");

    ASSERT_TEST(g.is_valid_node(a), "live node should be valid");
    ASSERT_TEST(g.free_node(a) == 0, "stale handle free_node fail");
    ASSERT_TEST(!g.is_valid_node(a), "freed node handle must be invalid");
    ASSERT_TEST(g.ref_node(a) == nullptr, "freed node ref must be nullptr");
    ASSERT_TEST(g.free_node(a) == -1, "double free_node expect -1");
    ASSERT_TEST(g.new_link(a, b, 0) == -2, "link from freed node expect -2");

    ASSERT_TEST(!g.is_valid_node(-1), "negative node id must be invalid");
    ASSERT_TEST(!g.is_valid_node(test_graph::kMaxNodeCnt), "out of range node id must be invalid");
    ASSERT_TEST(!g.is_valid_link(-1), "negative link id must be invalid");
    ASSERT_TEST(!g.is_valid_link(test_graph::kMaxLinkCnt), "out of range link id must be invalid");

    s32 reuse = g.new_node(zpoint(300, 100, 0), 2);
    ASSERT_TEST(reuse >= 0 && g.is_valid_node(reuse), "reused node slot should be valid");
    ASSERT_TEST(g.free_node(reuse) == 0, "reuse free_node fail");
    ASSERT_TEST(g.free_node(b) == 0, "stale handle cleanup free_node b fail");
    return 0;
}

static s32 zgraph_negative_cost_test()
{
    static test_graph g;
    s32 a = g.new_node(zpoint(100, 100, 0), 0);
    s32 b = g.new_node(zpoint(200, 100, 0), 1);
    ASSERT_TEST(a >= 0 && b >= 0, "negative cost new_node fail");

    ASSERT_TEST(g.new_link(a, b, 0, 0, -1) == -3, "cost -1 must be rejected with -3");
    ASSERT_TEST(g.new_link(a, b, 0, 0, -test_graph::kCostScaleN) == -3, "cost -kCostScaleN must be rejected");
    ASSERT_TEST(g.ref_node(a)->refs == 0 && g.ref_node(b)->refs == 0,
                "rejected link must not leak refs");
    ASSERT_TEST(g.link_count() == 0, "rejected link must not enter graph, got=", g.link_count());

    s32 ok = g.new_link(a, b, 0, 0, 0);
    ASSERT_TEST(ok >= 0, "cost 0 must be accepted");
    ASSERT_TEST(g.free_link(ok) == 0, "negative cost cleanup free_link fail");
    ASSERT_TEST(g.free_node(a) == 0 && g.free_node(b) == 0, "negative cost cleanup free_node fail");
    return 0;
}

static s32 zgraph_decrease_key_test()
{
    static test_graph g;
    s32 s = g.new_node(zpoint(0, 0, 0), 0);
    s32 hub = g.new_node(zpoint(1000, 0, 0), 1);
    s32 detour = g.new_node(zpoint(500, 4000, 0), 2);
    s32 t = g.new_node(zpoint(2000, 0, 0), 3);
    ASSERT_TEST(s >= 0 && hub >= 0 && detour >= 0 && t >= 0, "decrease-key new_node fail");

    ASSERT_TEST(g.new_link(s, hub, 0, 0, 200000) >= 0, "decrease-key expensive direct link fail");
    ASSERT_TEST(g.new_link(s, detour, 0) >= 0, "decrease-key s->detour fail");
    ASSERT_TEST(g.new_link(detour, hub, 0) >= 0, "decrease-key detour->hub fail");
    s32 l_ht = g.new_link(hub, t, 0);
    ASSERT_TEST(l_ht >= 0, "decrease-key hub->t fail");

    std::vector<test_graph::graph_path_step> steps;
    s32 ret = g.find_path(s, t, steps);
    ASSERT_TEST(ret == 0, "decrease-key find_path fail ret=", ret);
    ASSERT_TEST((s32)steps.size() == 3, "decrease-key expect 3-step detour, got=", (s32)steps.size());
    if (steps.size() == 3)
    {
        ASSERT_TEST(steps[0].link != -1 && g.ref_link(steps[0].link)->node[1] == detour,
                    "decrease-key first hop should reach detour");
        ASSERT_TEST(steps[2].link == l_ht, "decrease-key last hop should be hub->t");
    }
    ASSERT_TEST((s32)g.path_open_peak() <= test_graph::kMaxOpenCnt,
                "open peak must stay within cap, got=", (s32)g.path_open_peak());
    return 0;
}

static s32 zgraph_open_cap_test()
{
    using narrow_config_graph = zgraph<s32, s32, narrow_open_config>;
    static narrow_config_graph g;
    const s32 kFanout = narrow_open_config::kMaxOpenCnt + 4;
    s32 hub = g.new_node(zpoint(0, 0, 0), 0);
    ASSERT_TEST(hub >= 0, "open cap hub new_node fail");
    s32 last = -1;
    for (s32 i = 0; i < kFanout; i++)
    {
        s32 leaf = g.new_node(zpoint(100.0f + (f32)i, 100.0f, 0), i + 1);
        ASSERT_TEST_NOLOG(leaf >= 0, "open cap leaf new_node fail i=", i);
        ASSERT_TEST_NOLOG(g.new_link(hub, leaf, 0) >= 0, "open cap new_link fail i=", i);
        last = leaf;
    }
    s32 island = g.new_node(zpoint(9000, 9000, 0), 9999);
    ASSERT_TEST(island >= 0, "open cap island new_node fail");
    ASSERT_TEST(g.new_link(last, island, 0) >= 0, "open cap island link fail");

    std::vector<narrow_config_graph::graph_path_step> steps;
    s32 ret = g.find_path(hub, island, steps);
    ASSERT_TEST(ret == -4, "open heap overflow must return -4, ret=", ret);
    ASSERT_TEST(steps.empty(), "overflow steps must be empty");
    ASSERT_TEST((s32)g.path_open_peak() <= narrow_open_config::kMaxOpenCnt,
                "peak must never exceed cap, got=", (s32)g.path_open_peak());
    return 0;
}

static s32 zgraph_terrain_full_test()
{
    using narrow_grid_graph = zgraph<s32, s32, narrow_grid_config>;
    static narrow_grid_graph g;
    const f32 step = (f32)narrow_grid_config::kGridSize;
    s32 prev = -1;
    s32 filled = 0;
    for (s32 i = 0; i < narrow_grid_config::kMaxGridCnt; i++)
    {
        s32 nid = g.new_node(zpoint(((f32)i + 0.5f) * step, 0.5f * step, 0), i);
        ASSERT_TEST_NOLOG(nid >= 0, "terrain full new_node fail i=", i);
        ASSERT_TEST_NOLOG(g.push_node(nid) == 0, "terrain full push_node fail i=", i);
        filled++;
        prev = nid;
    }
    ASSERT_TEST(g.grid_count() == narrow_grid_config::kMaxGridCnt,
                "grid map should be full, got=", g.grid_count());

    s32 spill = g.new_node(zpoint(((f32)filled + 0.5f) * step, 0.5f * step, 0), filled);
    ASSERT_TEST(spill >= 0, "spill new_node fail");
    ASSERT_TEST(g.push_node(spill) == -2, "push_node into full terrain map must return -2");

    s32 spill_link = g.new_link(prev, spill, 0);
    ASSERT_TEST(spill_link >= 0, "spill new_link fail");
    s32 affects = 0;
    ASSERT_TEST(g.push_link(spill_link, affects) == -5, "push_link into full terrain map must return -5");
    ASSERT_TEST(affects == 0, "failed push_link must not report affects, got=", affects);
    return 0;
}

static s32 zgraph_dijkstra_reference(test_graph& g, const std::vector<s32>& node_ids,
                                     const std::vector<std::array<s32, 3>>& edges,
                                     s32 source_index, std::vector<f32>& out_cost)
{
    const f32 kInf = 1e30f;
    out_cost.assign(node_ids.size(), kInf);
    std::vector<s32> done(node_ids.size(), 0);
    out_cost[source_index] = 0.0f;
    for (size_t round = 0; round < node_ids.size(); round++)
    {
        s32 pick = -1;
        for (size_t i = 0; i < node_ids.size(); i++)
        {
            if (done[i] == 0 && out_cost[i] < kInf && (pick == -1 || out_cost[i] < out_cost[pick]))
            {
                pick = (s32)i;
            }
        }
        if (pick == -1)
        {
            break;
        }
        done[pick] = 1;
        for (size_t e = 0; e < edges.size(); e++)
        {
            for (s32 dir = 0; dir < 2; dir++)
            {
                s32 from = edges[e][dir];
                s32 to = edges[e][1 - dir];
                if (from != pick)
                {
                    continue;
                }
                f32 w = g.ref_link(edges[e][2])->weight;
                if (out_cost[pick] + w < out_cost[to])
                {
                    out_cost[to] = out_cost[pick] + w;
                }
            }
        }
    }
    return 0;
}

static s32 zgraph_dijkstra_crosscheck_test()
{
    const s32 kNodeCnt = 48;
    const s32 kRounds = 200;
    std::mt19937 rng(20260901u);
    std::uniform_real_distribution<f32> pos_dist(0.0f, 8000.0f);
    std::uniform_int_distribution<s32> cost_dist(0, 40000);
    s32 checked = 0;
    s32 unreachable = 0;
    for (s32 round = 0; round < kRounds; round++)
    {
        test_graph g;
        std::vector<s32> node_ids;
        for (s32 i = 0; i < kNodeCnt; i++)
        {
            s32 nid = g.new_node(zpoint(pos_dist(rng), pos_dist(rng), 0.0f), i);
            ASSERT_TEST_NOLOG(nid >= 0, "crosscheck new_node fail round=", round, " i=", i);
            node_ids.push_back(nid);
        }
        std::vector<std::array<s32, 3>> edges;
        std::uniform_int_distribution<s32> pick(0, kNodeCnt - 1);
        const s32 kEdgeCnt = kNodeCnt * 3;
        for (s32 e = 0; e < kEdgeCnt; e++)
        {
            s32 a = pick(rng);
            s32 b = pick(rng);
            if (a == b)
            {
                continue;
            }
            s32 lid = g.new_link(node_ids[a], node_ids[b], e, 0, cost_dist(rng));
            ASSERT_TEST_NOLOG(lid >= 0, "crosscheck new_link fail round=", round, " e=", e);
            std::array<s32, 3> rec = { a, b, lid };
            edges.push_back(rec);
        }

        s32 source_index = pick(rng);
        std::vector<f32> ref_cost;
        zgraph_dijkstra_reference(g, node_ids, edges, source_index, ref_cost);

        for (s32 target_index = 0; target_index < kNodeCnt; target_index++)
        {
            if (target_index == source_index)
            {
                continue;
            }
            std::vector<test_graph::graph_path_step> steps;
            s32 ret = g.find_path(node_ids[source_index], node_ids[target_index], steps);
            if (ref_cost[target_index] > 1e29f)
            {
                ASSERT_TEST_NOLOG(ret == -3, "crosscheck expect unreachable round=", round,
                                  " target=", target_index, " ret=", ret);
                unreachable++;
                continue;
            }
            ASSERT_TEST_NOLOG(ret == 0, "crosscheck find_path fail round=", round,
                              " target=", target_index, " ret=", ret);
            f32 got = 0.0f;
            for (size_t i = 0; i < steps.size(); i++)
            {
                got += g.ref_link(steps[i].link)->weight;
            }
            f32 tolerance = ref_cost[target_index] * 1e-4f + 0.01f;
            ASSERT_TEST_NOLOG(fabsf(got - ref_cost[target_index]) < tolerance,
                              "crosscheck cost mismatch round=", round, " target=", target_index,
                              " astar=", got, " dijkstra=", ref_cost[target_index]);
            checked++;
        }
    }
    LOGFMTI("zgraph dijkstra crosscheck: rounds=%d reachable=%d unreachable=%d", kRounds, checked, unreachable);
    ASSERT_TEST(checked > kRounds * 20, "crosscheck coverage too thin, checked=", checked);
    return 0;
}

int main(int argc, char* argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zgraph_adjacency_chain_test()                     == 0);
    ASSERT_TEST(zgraph_negative_coord_test()                      == 0);
    ASSERT_TEST(zgraph_stale_handle_test()                        == 0);
    ASSERT_TEST(zgraph_negative_cost_test()                       == 0);
    ASSERT_TEST(zgraph_decrease_key_test()                        == 0);
    ASSERT_TEST(zgraph_dijkstra_crosscheck_test()                 == 0);
    ASSERT_TEST(zgraph_open_cap_test()                            == 0);
    ASSERT_TEST(zgraph_terrain_full_test()                        == 0);
    ASSERT_TEST(zgraph_astar_grid_test()                          == 0);
    ASSERT_TEST(zgraph_astar_cost_test()                          == 0);
    ASSERT_TEST(zgraph_astar_unreachable_test()                   == 0);
    ASSERT_TEST(zgraph_astar_composite_test()                     == 0);
    ASSERT_TEST(zgraph_build_chain_links_test()                   == 0);
    ASSERT_TEST(zgraph_build_extra_nodes_test()                   == 0);
    ASSERT_TEST(zgraph_find_nearest_and_neighbor_bench_test()      == 0);
    ASSERT_TEST(zgraph_astar_bench_test()                         == 0);
    ASSERT_TEST(zgraph_add_and_remove_one_bench_test()            == 0);
    ASSERT_TEST(zgraph_destroy_test()                             == 0);

    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();

    LogInfo() << "all test finish .";
    return 0;
}
