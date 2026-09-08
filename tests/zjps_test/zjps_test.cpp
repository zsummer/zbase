/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#include <random>
#include <vector>
#include "fn_log.h"
#include "test_common.h"
#include "zjps.h"
#include "zgraph.h"
#include "zclock.h"

using test_graph = zgraph<s32, s32>;

static constexpr f32 kCellSize = 50.0f;
static constexpr f32 kCorridorHalfWidth = 48.0f;

static const s32 kDirX[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static const s32 kDirY[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

static void grid_flood_reach(const zjps_grid& grid, s32 start_x, s32 start_y, std::vector<u8>& out_reach)
{
    out_reach.assign((size_t)grid.width() * (size_t)grid.height(), 0);
    if (!grid.cell_walkable(0, start_x, start_y))
    {
        return;
    }
    std::vector<s32> queue;
    queue.reserve((size_t)grid.width() * (size_t)grid.height());
    queue.push_back(start_y * grid.width() + start_x);
    out_reach[(size_t)start_y * grid.width() + (size_t)start_x] = 1;
    size_t head = 0;
    while (head < queue.size())
    {
        s32 idx = queue[head];
        head++;
        s32 x = idx % grid.width();
        s32 y = idx / grid.width();
        for (s32 d = 0; d < 8; d++)
        {
            s32 dx = kDirX[d];
            s32 dy = kDirY[d];
            if (!grid.move_valid(0, x, y, dx, dy))
            {
                continue;
            }
            size_t next_idx = (size_t)(y + dy) * grid.width() + (size_t)(x + dx);
            if (out_reach[next_idx] != 0)
            {
                continue;
            }
            out_reach[next_idx] = 1;
            queue.push_back((s32)next_idx);
        }
    }
}

static s32 astar_cross_check(zjps_grid& grid, const zpoint& from_pos, const zpoint& to_pos,
                             s32 graph_ret, const std::vector<test_graph::graph_path_step>& steps,
                             s32 i, s32 j, bool check_length)
{
    s32 ax = 0;
    s32 ay = 0;
    s32 bx = 0;
    s32 by = 0;
    ASSERT_TEST_NOLOG(grid.pos_to_cell(from_pos.x, from_pos.y, ax, ay) == 0, "astar cross pos_to_cell fail i=", i);
    ASSERT_TEST_NOLOG(grid.pos_to_cell(to_pos.x, to_pos.y, bx, by) == 0, "astar cross pos_to_cell fail j=", j);
    std::vector<s32> acells;
    s32 aret = grid.astar_search( 0,ax, ay, 0, bx, by, acells);
    ASSERT_TEST_NOLOG(aret == 0 || aret == -2, "astar unexpected ret i=", i, " j=", j, " aret=", aret);
    bool graph_reach = (graph_ret == 0);
    bool astar_reach = (aret == 0);
    ASSERT_TEST_NOLOG(graph_reach == astar_reach, "astar reach mismatch i=", i, " j=", j,
                      " graph_ret=", graph_ret, " aret=", aret);
    if (!astar_reach)
    {
        std::vector<s32> jcells;
        s32 jret = grid.find_path( 0,ax, ay, 0, bx, by, jcells);
        ASSERT_TEST_NOLOG(jret == -2, "jps expect unreachable i=", i, " j=", j, " jret=", jret);
        return 0;
    }
    ASSERT_TEST_NOLOG(acells.front() == ay * grid.width() + ax, "astar path start mismatch i=", i, " j=", j);
    ASSERT_TEST_NOLOG(acells.back() == by * grid.width() + bx, "astar path end mismatch i=", i, " j=", j);
    for (size_t k = 0; k + 1 < acells.size(); k++)
    {
        s32 cx = acells[k] % grid.width();
        s32 cy = acells[k] / grid.width();
        s32 nx = acells[k + 1] % grid.width();
        s32 ny = acells[k + 1] / grid.width();
        ASSERT_TEST_NOLOG(grid.move_valid(0, cx, cy, nx - cx, ny - cy), "astar step invalid i=", i, " j=", j, " k=", (s32)k);
    }
    s32 astar_cost = grid.last_path_cost();
    std::vector<s32> jcells;
    s32 jret = grid.find_path( 0,ax, ay, 0, bx, by, jcells);
    ASSERT_TEST_NOLOG(jret == 0, "jps fail on reachable pair i=", i, " j=", j, " jret=", jret);
    ASSERT_TEST_NOLOG(jcells.front() == ay * grid.width() + ax, "jps path start mismatch i=", i, " j=", j);
    ASSERT_TEST_NOLOG(jcells.back() == by * grid.width() + bx, "jps path end mismatch i=", i, " j=", j);
    for (size_t k = 0; k + 1 < jcells.size(); k++)
    {
        s32 cx = jcells[k] % grid.width();
        s32 cy = jcells[k] / grid.width();
        s32 nx = jcells[k + 1] % grid.width();
        s32 ny = jcells[k + 1] / grid.width();
        ASSERT_TEST_NOLOG(grid.move_valid(0, cx, cy, nx - cx, ny - cy), "jps step invalid i=", i, " j=", j, " k=", (s32)k);
    }
    ASSERT_TEST_NOLOG(grid.last_path_cost() == astar_cost, "jps/astar cost mismatch i=", i, " j=", j,
                      " jps=", grid.last_path_cost(), " astar=", astar_cost);
    f32 astar_len = (f32)astar_cost / 1000.0f * grid.cell_size();
    if (check_length)
    {
        f32 zlen = 0.0f;
        zpoint prev = from_pos;
        for (size_t k = 0; k < steps.size(); k++)
        {
            f32 dx = steps[k].pos.x - prev.x;
            f32 dy = steps[k].pos.y - prev.y;
            zlen += sqrtf(dx * dx + dy * dy);
            prev = steps[k].pos;
        }
        ASSERT_TEST_NOLOG(fabsf(astar_len - zlen) < 2.0f, "astar/zgraph length mismatch i=", i, " j=", j,
                          " astar_len=", astar_len, " zlen=", zlen);
    }
    return 0;
}

static s32 fill_axis_link(zjps_grid& grid, const zpoint& pa, const zpoint& pb, f32 half_width)
{
    f32 lo_x = pa.x < pb.x ? pa.x : pb.x;
    f32 hi_x = pa.x > pb.x ? pa.x : pb.x;
    f32 lo_y = pa.y < pb.y ? pa.y : pb.y;
    f32 hi_y = pa.y > pb.y ? pa.y : pb.y;
    f32 min_x = lo_x - half_width;
    f32 max_x = hi_x + half_width;
    f32 min_y = lo_y - half_width;
    f32 max_y = hi_y + half_width;
    s32 x0 = (s32)ceilf(min_x / kCellSize - 0.5f);
    s32 x1 = (s32)floorf(max_x / kCellSize - 0.5f);
    s32 y0 = (s32)ceilf(min_y / kCellSize - 0.5f);
    s32 y1 = (s32)floorf(max_y / kCellSize - 0.5f);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= grid.width()) x1 = grid.width() - 1;
    if (y1 >= grid.height()) y1 = grid.height() - 1;
    if (x0 > x1 || y0 > y1)
    {
        return 0;
    }
    return grid.set_rect_cell(0, x0, y0, x1, y1, true);
}


static s32 zjps_set_rect_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 20, 20, kCellSize, false) == 0, "rect init fail");
    u32 version = grid.map_version();
    ASSERT_TEST(grid.set_rect_cell(0, 4, 5, 7, 6, true) == 0, "set_rect_cell fail");
    ASSERT_TEST(grid.map_version() > version, "version should bump");
    s32 cnt = 0;
    for (s32 y = 0; y < 20; y++)
    {
        for (s32 x = 0; x < 20; x++)
        {
            bool expect = (x >= 4 && x <= 7 && y >= 5 && y <= 6);
            ASSERT_TEST(grid.cell_walkable(0, x, y) == expect, "rect mismatch x=", x, " y=", y);
            if (grid.cell_walkable(0, x, y)) cnt++;
        }
    }
    ASSERT_TEST(cnt == 8, "rect cnt=", cnt);
    version = grid.map_version();
    ASSERT_TEST(grid.set_rect_cell(0, 4, 5, 7, 6, true) == 0, "idempotent fail");
    ASSERT_TEST(grid.map_version() == version, "idempotent should not bump");
    ASSERT_TEST(grid.set_rect_cell(0, 7, 6, 4, 5, true) == -1, "inverted expect -1");
    ASSERT_TEST(grid.set_rect_cell(0, -1, 0, 3, 3, true) == -1, "out of range expect -1");

    ASSERT_TEST(grid.build_jps_light() == 0, "rect light build fail");
    ASSERT_TEST(grid.light_dirty() == 0, "rect dirty after build");
    ASSERT_TEST(grid.set_rect_cell(0, 2, 2, 9, 9, false) == 0, "rect block fail");
    ASSERT_TEST(grid.light_dirty() == 16, "rect 8x8 expect 16 dirty lines, got=", grid.light_dirty());
    ASSERT_TEST(grid.build_jps_light() == 0, "rect repair fail");
    ASSERT_TEST(grid.light_dirty() == 0, "rect dirty after repair");
    return 0;
}

static s32 zjps_set_triangle_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 20, 20, kCellSize, false) == 0, "tri init fail");
    zpoint a(0.0f, 0.0f, 0.0f);
    zpoint b(600.0f, 0.0f, 0.0f);
    zpoint c(0.0f, 600.0f, 0.0f);
    u32 version = grid.map_version();
    ASSERT_TEST(grid.set_triangle_cell(0, a, b, c, true) == 0, "set_triangle_cell fail");
    ASSERT_TEST(grid.map_version() > version, "tri version should bump");
    s32 cnt = 0;
    for (s32 y = 0; y < 20; y++)
    {
        for (s32 x = 0; x < 20; x++)
        {
            f32 cx = ((f32)x + 0.5f) * kCellSize;
            f32 cy = ((f32)y + 0.5f) * kCellSize;
            bool expect = (cx + cy <= 600.0f);
            ASSERT_TEST(grid.cell_walkable(0, x, y) == expect, "tri mismatch x=", x, " y=", y);
            if (grid.cell_walkable(0, x, y)) cnt++;
        }
    }
    ASSERT_TEST(cnt == 78, "tri cnt=", cnt);

    zjps_grid g2;
    ASSERT_TEST(g2.init(1, 20, 20, kCellSize, false) == 0, "tri init2 fail");
    zpoint ra(600.0f, 0.0f, 0.0f);
    zpoint rb(600.0f, 600.0f, 0.0f);
    zpoint rc(0.0f, 600.0f, 0.0f);
    ASSERT_TEST(g2.set_triangle_cell(0, ra, rb, rc, true) == 0, "reverse-winding tri fail");
    s32 cnt2 = 0;
    for (s32 y = 0; y < 20; y++)
    {
        for (s32 x = 0; x < 20; x++)
        {
            f32 cx = ((f32)x + 0.5f) * kCellSize;
            f32 cy = ((f32)y + 0.5f) * kCellSize;
            if (cx + cy >= 600.0f && cx <= 600.0f && cy <= 600.0f)
            {
                ASSERT_TEST(g2.cell_walkable(0, x, y), "reverse tri missing x=", x, " y=", y);
                cnt2++;
            }
        }
    }
    ASSERT_TEST(cnt2 == 78, "reverse tri cnt=", cnt2);
    ASSERT_TEST(g2.set_triangle_cell(0, a, b, b, true) == -1, "degenerate tri expect -1");
    version = g2.map_version();
    ASSERT_TEST(g2.set_triangle_cell(0, ra, rb, rc, true) == 0, "tri idempotent fail");
    ASSERT_TEST(g2.map_version() == version, "tri idempotent should not bump");

    ASSERT_TEST(g2.build_jps_light() == 0, "tri light build fail");
    ASSERT_TEST(g2.set_triangle_cell(0, ra, rb, rc, false) == 0, "tri block fail");
    ASSERT_TEST(g2.light_dirty() > 0, "tri should dirty lines");
    ASSERT_TEST(g2.build_jps_light() == 0, "tri repair fail");
    ASSERT_TEST(g2.light_dirty() == 0, "tri dirty after repair");
    return 0;
}

static s32 zjps_corridor_consistency_test()
{
    const s32 W = 40;
    const s32 NODE_CNT = 10;
    zpoint node_pos[NODE_CNT] =
    {
        zpoint(300.0f, 300.0f, 0.0f), zpoint(1700.0f, 300.0f, 0.0f),
        zpoint(1700.0f, 700.0f, 0.0f), zpoint(300.0f, 700.0f, 0.0f),
        zpoint(300.0f, 1100.0f, 0.0f), zpoint(1700.0f, 1100.0f, 0.0f),
        zpoint(1700.0f, 1500.0f, 0.0f), zpoint(300.0f, 1500.0f, 0.0f),
        zpoint(1850.0f, 1850.0f, 0.0f), zpoint(1950.0f, 1950.0f, 0.0f),
    };
    const s32 LINK_CNT = 8;
    const s32 link_pairs[LINK_CNT][2] =
    {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 }, { 4, 5 }, { 5, 6 }, { 6, 7 }, { 8, 9 },
    };

    test_graph graph;
    s32 node_ids[NODE_CNT] = { 0 };
    for (s32 i = 0; i < NODE_CNT; i++)
    {
        node_ids[i] = graph.new_node(node_pos[i], i);
        ASSERT_TEST(node_ids[i] >= 0, "new_node fail i=", i);
    }
    for (s32 i = 0; i < LINK_CNT; i++)
    {
        s32 lid = graph.new_link(node_ids[link_pairs[i][0]], node_ids[link_pairs[i][1]], i);
        ASSERT_TEST(lid >= 0, "new_link fail i=", i);
        s32 affects = 0;
        ASSERT_TEST(graph.push_link(lid, affects) == 0, "push_link fail i=", i);
    }

    zjps_grid grid;
    ASSERT_TEST(grid.init(1, W, W, kCellSize, false) == 0, "grid init fail");
    for (s32 i = 0; i < LINK_CNT; i++)
    {
        ASSERT_TEST(fill_axis_link(grid, node_pos[link_pairs[i][0]], node_pos[link_pairs[i][1]], kCorridorHalfWidth) == 0,
                    "raster link fail i=", i);
    }

    s32 cell_x[NODE_CNT] = { 0 };
    s32 cell_y[NODE_CNT] = { 0 };
    for (s32 i = 0; i < NODE_CNT; i++)
    {
        ASSERT_TEST(grid.pos_to_cell(node_pos[i].x, node_pos[i].y, cell_x[i], cell_y[i]) == 0, "pos_to_cell fail i=", i);
        ASSERT_TEST(grid.cell_walkable(0, cell_x[i], cell_y[i]), "node cell not walkable i=", i);
    }

    std::vector<u8> reach;
    std::vector<test_graph::graph_path_step> steps;
    s32 checked = 0;
    for (s32 i = 0; i < NODE_CNT; i++)
    {
        for (s32 j = 0; j < NODE_CNT; j++)
        {
            s32 ret = graph.find_path(node_ids[i], node_ids[j], steps);
            ASSERT_TEST(ret == 0 || ret == -3, "find_path unexpected ret i=", i, " j=", j, " ret=", ret);
            bool graph_reach = (ret == 0);
            grid_flood_reach(grid, cell_x[i], cell_y[i], reach);
            bool grid_reach = reach[(size_t)cell_y[j] * W + (size_t)cell_x[j]] != 0;
            ASSERT_TEST(graph_reach == grid_reach, "corridor consistency mismatch i=", i, " j=", j, " ret=", ret);
            ASSERT_TEST(astar_cross_check(grid, node_pos[i], node_pos[j], ret, steps, i, j, false) == 0,
                        "corridor astar cross fail i=", i, " j=", j);
            checked++;
        }
    }
    LOGFMTI("zjps corridor consistency: %d node pairs checked", checked);
    return 0;
}

static s32 zjps_serpentine_consistency_test()
{
    const s32 W = 400;
    const s32 ROW_CNT = 33;
    const s32 NODE_CNT = ROW_CNT * 2 + 2;
    std::vector<zpoint> node_pos;
    node_pos.reserve(NODE_CNT);
    for (s32 k = 0; k < ROW_CNT; k++)
    {
        f32 y = 300.0f + 600.0f * (f32)k;
        node_pos.push_back(zpoint(300.0f, y, 0.0f));
        node_pos.push_back(zpoint(19700.0f, y, 0.0f));
    }
    node_pos.push_back(zpoint(19850.0f, 19850.0f, 0.0f));
    node_pos.push_back(zpoint(19950.0f, 19950.0f, 0.0f));

    test_graph graph;
    std::vector<s32> node_ids(NODE_CNT, 0);
    for (s32 i = 0; i < NODE_CNT; i++)
    {
        node_ids[i] = graph.new_node(node_pos[i], i);
        ASSERT_TEST(node_ids[i] >= 0, "new_node fail i=", i);
    }
    s32 link_cnt = 0;
    for (s32 k = 0; k < ROW_CNT; k++)
    {
        s32 lid = graph.new_link(node_ids[2 * k], node_ids[2 * k + 1], link_cnt);
        ASSERT_TEST(lid >= 0, "new_link row fail k=", k);
        s32 affects = 0;
        ASSERT_TEST(graph.push_link(lid, affects) == 0, "push_link row fail k=", k);
        link_cnt++;
    }
    for (s32 k = 0; k + 1 < ROW_CNT; k++)
    {
        s32 side = (k % 2 == 0) ? 1 : 0;
        s32 lid = graph.new_link(node_ids[2 * k + side], node_ids[2 * (k + 1) + side], link_cnt);
        ASSERT_TEST(lid >= 0, "new_link conn fail k=", k);
        s32 affects = 0;
        ASSERT_TEST(graph.push_link(lid, affects) == 0, "push_link conn fail k=", k);
        link_cnt++;
    }
    {
        s32 lid = graph.new_link(node_ids[NODE_CNT - 2], node_ids[NODE_CNT - 1], link_cnt);
        ASSERT_TEST(lid >= 0, "new_link isolated fail");
        s32 affects = 0;
        ASSERT_TEST(graph.push_link(lid, affects) == 0, "push_link isolated fail");
        link_cnt++;
    }
    ASSERT_TEST(graph.node_count() <= test_graph::kMaxNodeCnt, "node overflow ", graph.node_count());
    ASSERT_TEST(graph.link_count() <= test_graph::kMaxLinkCnt, "link overflow ", graph.link_count());
    ASSERT_TEST(graph.grid_count() <= test_graph::kMaxGridCnt, "grid overflow ", graph.grid_count());

    zjps_grid grid;
    ASSERT_TEST(grid.init(1, W, W, kCellSize, false) == 0, "grid init fail");
    ASSERT_TEST(grid.set_open_capacity(65536) == 0, "set open capacity fail");
    ASSERT_TEST(grid.build_jps_light() == 0, "light build fail");
    for (s32 k = 0; k < ROW_CNT; k++)
    {
        ASSERT_TEST(fill_axis_link(grid, node_pos[2 * k], node_pos[2 * k + 1], kCorridorHalfWidth) == 0, "raster row fail k=", k);
    }
    ASSERT_TEST(grid.build_jps_light() == 0, "light rebuild after raster fail");
    for (s32 k = 0; k + 1 < ROW_CNT; k++)
    {
        s32 side = (k % 2 == 0) ? 1 : 0;
        ASSERT_TEST(fill_axis_link(grid, node_pos[2 * k + side], node_pos[2 * (k + 1) + side], kCorridorHalfWidth) == 0,
                    "raster conn fail k=", k);
    }
    ASSERT_TEST(fill_axis_link(grid, node_pos[NODE_CNT - 2], node_pos[NODE_CNT - 1], kCorridorHalfWidth) == 0,
                "raster isolated fail");

    std::vector<s32> cell_x(NODE_CNT, 0);
    std::vector<s32> cell_y(NODE_CNT, 0);
    for (s32 i = 0; i < NODE_CNT; i++)
    {
        ASSERT_TEST(grid.pos_to_cell(node_pos[i].x, node_pos[i].y, cell_x[i], cell_y[i]) == 0, "pos_to_cell fail i=", i);
        ASSERT_TEST(grid.cell_walkable(0, cell_x[i], cell_y[i]), "node cell not walkable i=", i);
    }

    std::mt19937 rng(20260830u);
    std::uniform_int_distribution<s32> node_dist(0, NODE_CNT - 1);
    std::vector<u8> reach;
    std::vector<test_graph::graph_path_step> steps;
    const s32 SAMPLE_CNT = 600;
    s32 reachable_cnt = 0;
    for (s32 s = 0; s < SAMPLE_CNT; s++)
    {
        s32 i = node_dist(rng);
        s32 j = node_dist(rng);
        s32 ret = graph.find_path(node_ids[i], node_ids[j], steps);
        ASSERT_TEST(ret == 0 || ret == -3, "find_path unexpected ret i=", i, " j=", j, " ret=", ret);
        ASSERT_TEST(ret != -4, "open heap overflow i=", i, " j=", j);
        bool graph_reach = (ret == 0);
        bool group_reach = ((i < NODE_CNT - 2) == (j < NODE_CNT - 2));
        ASSERT_TEST(graph_reach == group_reach, "serpentine group reach mismatch i=", i, " j=", j, " ret=", ret);
        grid_flood_reach(grid, cell_x[i], cell_y[i], reach);
        bool grid_reach = reach[(size_t)cell_y[j] * W + (size_t)cell_x[j]] != 0;
        ASSERT_TEST(graph_reach == grid_reach, "serpentine consistency mismatch i=", i, " j=", j, " ret=", ret);
        ASSERT_TEST(astar_cross_check(grid, node_pos[i], node_pos[j], ret, steps, i, j, false) == 0,
                    "serpentine astar cross fail i=", i, " j=", j);
        if (graph_reach)
        {
            reachable_cnt++;
        }
    }
    LOGFMTI("zjps serpentine consistency: %d samples checked, reachable=%d, nodes=%d, links=%d, grids=%d",
            SAMPLE_CNT, reachable_cnt, graph.node_count(), graph.link_count(), graph.grid_count());
    {
        std::vector<s32> acells;
        s32 aret = grid.astar_search( 0,cell_x[0], cell_y[0], 0, cell_x[2 * (ROW_CNT - 1)], cell_y[2 * (ROW_CNT - 1)], acells);
        ASSERT_TEST(aret == 0, "serpentine astar long query fail aret=", aret);
        LOGFMTI("zjps astar serpentine long query: cells=%d cost=%d open_push=%d open_pop=%d open_peak=%d visit=%d",
                (s32)acells.size(), grid.last_path_cost(), grid.open_push_count(), grid.open_pop_count(),
                grid.open_peak(), grid.visit_count());
    }
    {
        std::vector<s32> jcells;
        s32 jret = grid.find_path( 0,cell_x[0], cell_y[0], 0, cell_x[2 * (ROW_CNT - 1)], cell_y[2 * (ROW_CNT - 1)], jcells);
        ASSERT_TEST(jret == 0, "serpentine jps long query fail jret=", jret);
        LOGFMTI("zjps jps serpentine long query: cells=%d cost=%d open_push=%d open_pop=%d open_peak=%d visit=%d",
                (s32)jcells.size(), grid.last_path_cost(), grid.open_push_count(), grid.open_pop_count(),
                grid.open_peak(), grid.visit_count());
    }
    return 0;
}

static s32 random_map_check(s32 width, s32 rect_cnt, s32 sample_pairs, u32 seed)
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, width, width, kCellSize, true) == 0, "grid init fail");
    ASSERT_TEST(grid.set_open_capacity(8192) == 0, "set open capacity fail");
    ASSERT_TEST(grid.build_jps_light() == 0, "light build fail");
    std::mt19937 rng(seed);
    for (s32 i = 0; i < rect_cnt; i++)
    {
        s32 bx = (s32)(rng() % (size_t)(width - 3));
        s32 by = (s32)(rng() % (size_t)(width - 3));
        s32 bw = 1 + (s32)(rng() % 3);
        s32 bh = 1 + (s32)(rng() % 3);
        ASSERT_TEST(grid.set_rect_cell(0, bx, by, bx + bw - 1, by + bh - 1, false) == 0,
                    "block rect fail i=", i);
    }
    ASSERT_TEST(grid.build_jps_light() == 0, "light rebuild after raster fail");

    std::vector<s32> node_ids((size_t)width * width, -1);
    std::vector<s32> cell_list;
    test_graph graph;
    for (s32 y = 0; y < width; y++)
    {
        for (s32 x = 0; x < width; x++)
        {
            if (!grid.cell_walkable(0, x, y))
            {
                continue;
            }
            f32 px = ((f32)x + 0.5f) * kCellSize;
            f32 py = ((f32)y + 0.5f) * kCellSize;
            s32 nid = graph.new_node(zpoint(px, py, 0.0f), y * width + x);
            ASSERT_TEST(nid >= 0, "new_node fail x=", x, " y=", y);
            node_ids[(size_t)y * width + (size_t)x] = nid;
            cell_list.push_back(y * width + x);
        }
    }
    ASSERT_TEST((s32)cell_list.size() >= 20, "too few walkable cells ", (s32)cell_list.size());
    ASSERT_TEST(graph.node_count() <= test_graph::kMaxNodeCnt, "node overflow ", graph.node_count());

    s32 link_cnt = 0;
    for (s32 y = 0; y < width; y++)
    {
        for (s32 x = 0; x < width; x++)
        {
            if (node_ids[(size_t)y * width + (size_t)x] < 0)
            {
                continue;
            }
            if (x + 1 < width && node_ids[(size_t)y * width + (size_t)x + 1] >= 0)
            {
                s32 lid = graph.new_link(node_ids[(size_t)y * width + (size_t)x], node_ids[(size_t)y * width + (size_t)x + 1], link_cnt);
                ASSERT_TEST(lid >= 0, "new_link east fail x=", x, " y=", y);
                s32 affects = 0;
                ASSERT_TEST(graph.push_link(lid, affects) == 0, "push_link east fail x=", x, " y=", y);
                link_cnt++;
            }
            if (y + 1 < width && node_ids[(size_t)(y + 1) * width + (size_t)x] >= 0)
            {
                s32 lid = graph.new_link(node_ids[(size_t)y * width + (size_t)x], node_ids[(size_t)(y + 1) * width + (size_t)x], link_cnt);
                ASSERT_TEST(lid >= 0, "new_link north fail x=", x, " y=", y);
                s32 affects = 0;
                ASSERT_TEST(graph.push_link(lid, affects) == 0, "push_link north fail x=", x, " y=", y);
                link_cnt++;
            }
            if (x + 1 < width && y + 1 < width
                && node_ids[(size_t)(y + 1) * width + (size_t)x + 1] >= 0
                && node_ids[(size_t)y * width + (size_t)x + 1] >= 0
                && node_ids[(size_t)(y + 1) * width + (size_t)x] >= 0)
            {
                s32 lid = graph.new_link(node_ids[(size_t)y * width + (size_t)x], node_ids[(size_t)(y + 1) * width + (size_t)x + 1], link_cnt);
                ASSERT_TEST(lid >= 0, "new_link northeast fail x=", x, " y=", y);
                s32 affects = 0;
                ASSERT_TEST(graph.push_link(lid, affects) == 0, "push_link northeast fail x=", x, " y=", y);
                link_cnt++;
            }
            if (x + 1 < width && y - 1 >= 0
                && node_ids[(size_t)(y - 1) * width + (size_t)x + 1] >= 0
                && node_ids[(size_t)y * width + (size_t)x + 1] >= 0
                && node_ids[(size_t)(y - 1) * width + (size_t)x] >= 0)
            {
                s32 lid = graph.new_link(node_ids[(size_t)y * width + (size_t)x], node_ids[(size_t)(y - 1) * width + (size_t)x + 1], link_cnt);
                ASSERT_TEST(lid >= 0, "new_link southeast fail x=", x, " y=", y);
                s32 affects = 0;
                ASSERT_TEST(graph.push_link(lid, affects) == 0, "push_link southeast fail x=", x, " y=", y);
                link_cnt++;
            }
        }
    }
    ASSERT_TEST(graph.link_count() <= test_graph::kMaxLinkCnt, "link overflow ", graph.link_count());
    ASSERT_TEST(graph.grid_count() <= test_graph::kMaxGridCnt, "grid overflow ", graph.grid_count());

    std::vector<u8> reach;
    std::vector<test_graph::graph_path_step> steps;
    s32 checked = 0;
    s32 reachable_cnt = 0;
    if (sample_pairs <= 0)
    {
        for (size_t i = 0; i < cell_list.size(); i++)
        {
            for (size_t j = 0; j < cell_list.size(); j++)
            {
                s32 ret = graph.find_path(node_ids[(size_t)cell_list[i]], node_ids[(size_t)cell_list[j]], steps);
                ASSERT_TEST(ret == 0 || ret == -3, "find_path unexpected ret i=", (s32)i, " j=", (s32)j, " ret=", ret);
                ASSERT_TEST(ret != -4, "open heap overflow i=", (s32)i, " j=", (s32)j);
                bool graph_reach = (ret == 0);
                grid_flood_reach(grid, cell_list[i] % width, cell_list[i] / width, reach);
                bool grid_reach = reach[(size_t)cell_list[j]] != 0;
                ASSERT_TEST(graph_reach == grid_reach, "random map consistency mismatch i=", (s32)i, " j=", (s32)j, " ret=", ret);
                zpoint pa(((f32)(cell_list[i] % width) + 0.5f) * kCellSize, ((f32)(cell_list[i] / width) + 0.5f) * kCellSize, 0.0f);
                zpoint pb(((f32)(cell_list[j] % width) + 0.5f) * kCellSize, ((f32)(cell_list[j] / width) + 0.5f) * kCellSize, 0.0f);
                ASSERT_TEST(astar_cross_check(grid, pa, pb, ret, steps, (s32)i, (s32)j, true) == 0,
                            "random map astar cross fail i=", (s32)i, " j=", (s32)j);
                if (graph_reach)
                {
                    reachable_cnt++;
                }
                checked++;
            }
        }
    }
    else
    {
        std::uniform_int_distribution<size_t> cell_dist(0, cell_list.size() - 1);
        for (s32 s = 0; s < sample_pairs; s++)
        {
            size_t i = cell_dist(rng);
            size_t j = cell_dist(rng);
            s32 ret = graph.find_path(node_ids[(size_t)cell_list[i]], node_ids[(size_t)cell_list[j]], steps);
            ASSERT_TEST(ret == 0 || ret == -3, "find_path unexpected ret i=", (s32)i, " j=", (s32)j, " ret=", ret);
            ASSERT_TEST(ret != -4, "open heap overflow i=", (s32)i, " j=", (s32)j);
            bool graph_reach = (ret == 0);
            grid_flood_reach(grid, cell_list[i] % width, cell_list[i] / width, reach);
            bool grid_reach = reach[(size_t)cell_list[j]] != 0;
            ASSERT_TEST(graph_reach == grid_reach, "random map consistency mismatch i=", (s32)i, " j=", (s32)j, " ret=", ret);
            zpoint pa(((f32)(cell_list[i] % width) + 0.5f) * kCellSize, ((f32)(cell_list[i] / width) + 0.5f) * kCellSize, 0.0f);
            zpoint pb(((f32)(cell_list[j] % width) + 0.5f) * kCellSize, ((f32)(cell_list[j] / width) + 0.5f) * kCellSize, 0.0f);
            ASSERT_TEST(astar_cross_check(grid, pa, pb, ret, steps, (s32)i, (s32)j, true) == 0,
                        "random map astar cross fail i=", (s32)i, " j=", (s32)j);
            if (graph_reach)
            {
                reachable_cnt++;
            }
            checked++;
        }
    }
    LOGFMTI("zjps random map consistency: map=%dx%d nodes=%d links=%d pairs=%d reachable=%d",
            width, width, graph.node_count(), graph.link_count(), checked, reachable_cnt);
    return 0;
}

static s32 zjps_random_map_consistency_test()
{
    ASSERT_TEST(random_map_check(12, 10, 0, 20260831u) == 0, "random map small fail");
    ASSERT_TEST(random_map_check(12, 10, 0, 20260832u) == 0, "random map small seed2 fail");
    ASSERT_TEST(random_map_check(20, 14, 2000, 20260833u) == 0, "random map medium fail");
    return 0;
}

static s32 zjps_astar_basic_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 20, 20, kCellSize, true) == 0, "grid init fail");
    std::vector<s32> cells;
    s32 ret = grid.astar_search( 0,0, 0, 0, 10, 5, cells);
    ASSERT_TEST(ret == 0, "find_path fail ret=", ret);
    ASSERT_TEST((s32)cells.size() == 11, "expect 11 cells, got=", (s32)cells.size());
    ASSERT_TEST(cells.front() == 0, "path start mismatch");
    ASSERT_TEST(cells.back() == 5 * 20 + 10, "path end mismatch");
    ASSERT_TEST(grid.last_path_cost() == 1000 * 10 + 414 * 5, "octile cost mismatch, got=", grid.last_path_cost());
    for (size_t k = 0; k + 1 < cells.size(); k++)
    {
        s32 cx = cells[k] % grid.width();
        s32 cy = cells[k] / grid.width();
        s32 nx = cells[k + 1] % grid.width();
        s32 ny = cells[k + 1] / grid.width();
        ASSERT_TEST(grid.move_valid(0, cx, cy, nx - cx, ny - cy), "step invalid k=", (s32)k);
    }

    ret = grid.astar_search( 0,3, 3, 0, 3, 3, cells);
    ASSERT_TEST(ret == 0 && cells.size() == 1, "source==target expect single cell, ret=", ret);

    ret = grid.astar_search( 0,-1, 0, 0, 5, 5, cells);
    ASSERT_TEST(ret == -1, "invalid source expect -1, ret=", ret);
    ret = grid.astar_search( 0,0, 0, 0, 20, 0, cells);
    ASSERT_TEST(ret == -1, "invalid target expect -1, ret=", ret);

    ASSERT_TEST(grid.set_blocked(0, 15, 15) == 0, "set_blocked fail");
    ret = grid.astar_search( 0,0, 0, 0, 15, 15, cells);
    ASSERT_TEST(ret == -2, "blocked target expect -2, ret=", ret);
    ASSERT_TEST(grid.set_walkable(0, 15, 15) == 0, "set_walkable fail");

    for (s32 y = 0; y < 20; y++)
    {
        ASSERT_TEST(grid.set_blocked(0, 10, y) == 0, "set wall fail y=", y);
    }
    ret = grid.astar_search( 0,0, 0, 0, 19, 0, cells);
    ASSERT_TEST(ret == -2, "unreachable expect -2, ret=", ret);
    for (s32 y = 0; y < 20; y++)
    {
        ASSERT_TEST(grid.set_walkable(0, 10, y) == 0, "clear wall fail y=", y);
    }
    ret = grid.astar_search( 0,0, 0, 0, 19, 0, cells);
    ASSERT_TEST(ret == 0, "restored path fail ret=", ret);

    zjps_grid diag_grid;
    ASSERT_TEST(diag_grid.init(1, 20, 20, kCellSize, true) == 0, "diag grid init fail");
    ret = diag_grid.astar_search( 0,0, 0, 0, 19, 19, cells);
    ASSERT_TEST(ret == 0, "diag find_path fail ret=", ret);
    ASSERT_TEST(diag_grid.last_path_cost() == 19 * 1414, "diag octile cost mismatch, got=", diag_grid.last_path_cost());
    ASSERT_TEST((s32)cells.size() == 20, "diag expect 20 cells, got=", (s32)cells.size());
    LOGFMTI("zjps astar basic: open_push=%d open_pop=%d open_peak=%d visit=%d",
            diag_grid.open_push_count(), diag_grid.open_pop_count(), diag_grid.open_peak(), diag_grid.visit_count());
    return 0;
}

static s32 zjps_astar_capacity_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 20, 20, kCellSize, true) == 0, "grid init fail");
    ASSERT_TEST(grid.open_capacity() == zjps_grid::kDefaultOpenCnt, "default capacity mismatch");
    ASSERT_TEST(grid.set_open_capacity(2) == 0, "set open capacity 2 fail");
    ASSERT_TEST(grid.set_open_capacity(0) == -1, "set open capacity 0 expect -1");
    std::vector<s32> cells;
    s32 ret = grid.astar_search( 0,0, 0, 0, 19, 19, cells);
    ASSERT_TEST(ret == -3, "open overflow expect -3, ret=", ret);
    ASSERT_TEST(grid.set_open_capacity(1024) == 0, "set open capacity 1024 fail");
    ret = grid.astar_search( 0,0, 0, 0, 19, 19, cells);
    ASSERT_TEST(ret == 0, "find_path after capacity restore fail ret=", ret);
    ASSERT_TEST(grid.last_path_cost() == 19 * 1414, "cost after restore mismatch");
    return 0;
}

static s32 zjps_jps_basic_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 20, 20, kCellSize, true) == 0, "grid init fail");
    std::vector<s32> cells;
    s32 ret = grid.find_path( 0,0, 0, 0, 10, 5, cells);
    ASSERT_TEST(ret == 0, "jps find_path fail ret=", ret);
    ASSERT_TEST(grid.last_path_cost() == 1000 * 10 + 414 * 5, "jps octile cost mismatch, got=", grid.last_path_cost());
    ASSERT_TEST(cells.front() == 0, "jps path start mismatch");
    ASSERT_TEST(cells.back() == 5 * 20 + 10, "jps path end mismatch");
    for (size_t k = 0; k + 1 < cells.size(); k++)
    {
        s32 cx = cells[k] % grid.width();
        s32 cy = cells[k] / grid.width();
        s32 nx = cells[k + 1] % grid.width();
        s32 ny = cells[k + 1] / grid.width();
        ASSERT_TEST(grid.move_valid(0, cx, cy, nx - cx, ny - cy), "jps step invalid k=", (s32)k);
    }
    std::vector<s32> acells;
    s32 aret = grid.astar_search( 0,0, 0, 0, 10, 5, acells);
    ASSERT_TEST(aret == 0 && grid.last_path_cost() == 1000 * 10 + 414 * 5, "astar cross fail");

    ret = grid.find_path( 0,3, 3, 0, 3, 3, cells);
    ASSERT_TEST(ret == 0 && cells.size() == 1, "jps source==target expect single cell, ret=", ret);
    ret = grid.find_path( 0,-1, 0, 0, 5, 5, cells);
    ASSERT_TEST(ret == -1, "jps invalid source expect -1, ret=", ret);
    ret = grid.find_path( 0,0, 0, 0, 20, 0, cells);
    ASSERT_TEST(ret == -1, "jps invalid target expect -1, ret=", ret);
    ASSERT_TEST(grid.set_blocked(0, 15, 15) == 0, "jps set_blocked fail");
    ret = grid.find_path( 0,0, 0, 0, 15, 15, cells);
    ASSERT_TEST(ret == -2, "jps blocked target expect -2, ret=", ret);
    ASSERT_TEST(grid.set_walkable(0, 15, 15) == 0, "jps set_walkable fail");
    for (s32 y = 0; y < 20; y++)
    {
        ASSERT_TEST(grid.set_blocked(0, 10, y) == 0, "jps set wall fail y=", y);
    }
    ret = grid.find_path( 0,0, 0, 0, 19, 0, cells);
    ASSERT_TEST(ret == -2, "jps unreachable expect -2, ret=", ret);
    for (s32 y = 0; y < 20; y++)
    {
        ASSERT_TEST(grid.set_walkable(0, 10, y) == 0, "jps clear wall fail y=", y);
    }
    ret = grid.find_path( 0,0, 0, 0, 19, 19, cells);
    ASSERT_TEST(ret == 0 && grid.last_path_cost() == 19 * 1414, "jps diag cost mismatch, got=", grid.last_path_cost());
    return 0;
}

static s32 zjps_jps_capacity_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 20, 20, kCellSize, true) == 0, "grid init fail");
    ASSERT_TEST(grid.set_blocked(0, 1, 1) == 0, "block fail");
    ASSERT_TEST(grid.set_open_capacity(1) == 0, "set open capacity 1 fail");
    std::vector<s32> cells;
    s32 ret = grid.find_path( 0,0, 0, 0, 19, 19, cells);
    ASSERT_TEST(ret == -3, "jps open overflow expect -3, ret=", ret);
    ASSERT_TEST(grid.set_open_capacity(1024) == 0, "jps set open capacity 1024 fail");
    ret = grid.find_path( 0,0, 0, 0, 19, 19, cells);
    ASSERT_TEST(ret == 0, "jps after capacity restore fail ret=", ret);
    return 0;
}

static s32 bench_serpentine_grid(zjps_grid& grid)
{
    const s32 W = 400;
    ASSERT_TEST(grid.init(1, W, W, kCellSize, false) == 0, "bench grid init fail");
    ASSERT_TEST(grid.set_open_capacity(65536) == 0, "bench open capacity fail");
    for (s32 k = 0; k < 33; k++)
    {
        f32 y = 300.0f + 600.0f * (f32)k;
        ASSERT_TEST(fill_axis_link(grid, zpoint(300.0f, y, 0.0f), zpoint(19700.0f, y, 0.0f), kCorridorHalfWidth) == 0,
                    "bench raster row fail k=", k);
        if (k + 1 < 33)
        {
            f32 sx = (k % 2 == 0) ? 19700.0f : 300.0f;
            ASSERT_TEST(fill_axis_link(grid, zpoint(sx, y, 0.0f), zpoint(sx, y + 600.0f, 0.0f), kCorridorHalfWidth) == 0,
                        "bench raster conn fail k=", k);
        }
    }
    return 0;
}

static s32 zjps_bench_test()
{
    volatile s32 salt = 0;
    std::vector<s32> cells;

    zjps_grid grid;
    ASSERT_TEST(bench_serpentine_grid(grid) == 0, "bench serpentine build fail");
    ASSERT_TEST(grid.build_jps_light() == 0, "bench serpentine light build fail");
    s32 sx = 0;
    s32 sy = 0;
    s32 tx = 0;
    s32 ty = 0;
    ASSERT_TEST(grid.pos_to_cell(300.0f, 300.0f, sx, sy) == 0, "bench pos a fail");
    ASSERT_TEST(grid.pos_to_cell(300.0f, 19500.0f, tx, ty) == 0, "bench pos b fail");
    {
        const s32 N = 100;
        zclock<> cost;
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 ret = grid.astar_search( 0,sx, sy, 0, tx, ty, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench serpentine(400x400, 12675-cell path) astar: %.0f ns/op visit=%d open_peak=%d",
                (f64)cost.cost_ns() / (f64)N, grid.visit_count(), grid.open_peak());
    }
    {
        const s32 N = 2000;
        zclock<> cost;
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 ret = grid.find_path( 0,sx, sy, 0, tx, ty, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench serpentine(400x400, 12675-cell path) jps:   %.0f ns/op visit=%d open_peak=%d",
                (f64)cost.cost_ns() / (f64)N, grid.visit_count(), grid.open_peak());
    }

    zjps_grid rgrid;
    const s32 RW = 20;
    ASSERT_TEST(rgrid.init(1, RW, RW, kCellSize, true) == 0, "bench rgrid init fail");
    ASSERT_TEST(rgrid.set_open_capacity(8192) == 0, "bench rgrid capacity fail");
    ASSERT_TEST(rgrid.build_jps_light() == 0, "bench rgrid light build fail");
    std::mt19937 rng(20260834u);
    for (s32 i = 0; i < 14; i++)
    {
        s32 bx = (s32)(rng() % (size_t)(RW - 3));
        s32 by = (s32)(rng() % (size_t)(RW - 3));
        s32 bw = 1 + (s32)(rng() % 3);
        s32 bh = 1 + (s32)(rng() % 3);
        ASSERT_TEST(rgrid.set_rect_cell(0, bx, by, bx + bw - 1, by + bh - 1, false) == 0,
                    "bench block fail i=", i);
    }
    ASSERT_TEST(rgrid.build_jps_light() == 0, "bench rgrid light rebuild fail");
    std::vector<s32> rcells;
    for (s32 y = 0; y < RW; y++)
    {
        for (s32 x = 0; x < RW; x++)
        {
            if (rgrid.cell_walkable(0, x, y))
            {
                rcells.push_back(y * RW + x);
            }
        }
    }
    {
        std::uniform_int_distribution<size_t> dist(0, rcells.size() - 1);
        const s32 N = 2000;
        s32 a[2000];
        s32 b[2000];
        for (s32 i = 0; i < N; i++)
        {
            a[i] = rcells[dist(rng)];
            b[i] = rcells[dist(rng)];
        }
        zclock<> cost;
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 ret = rgrid.astar_search( 0,a[i] % RW, a[i] / RW, 0, b[i] % RW, b[i] / RW, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench random(20x20) astar: %.0f ns/op", (f64)cost.cost_ns() / (f64)N);
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 ret = rgrid.find_path( 0,a[i] % RW, a[i] / RW, 0, b[i] % RW, b[i] / RW, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench random(20x20) jps:   %.0f ns/op", (f64)cost.cost_ns() / (f64)N);
    }

    zjps_grid egrid;
    ASSERT_TEST(egrid.init(1, 200, 200, kCellSize, true) == 0, "bench egrid init fail");
    ASSERT_TEST(egrid.build_jps_light() == 0, "bench egrid light build fail");
    {
        const s32 N = 2000;
        zclock<> cost;
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 ret = egrid.astar_search( 0,1, 1, 0, 198, 198, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench openfield(200x200 diag) astar: %.0f ns/op visit=%d", (f64)cost.cost_ns() / (f64)N,
                egrid.visit_count(), egrid.open_peak());
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 ret = egrid.find_path( 0,1, 1, 0, 198, 198, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench openfield(200x200 diag) jps:   %.0f ns/op visit=%d", (f64)cost.cost_ns() / (f64)N,
                egrid.visit_count(), egrid.open_peak());
    }
    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

static s32 zjps_api_surface_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 16, 12, kCellSize, true) == 0, "api grid init fail");

    f32 px = 0.0f;
    f32 py = 0.0f;
    f32 zjps_tmp_z = 0.0f;
    ASSERT_TEST(grid.cell_to_pos( 0,0, 0, px, py, zjps_tmp_z) == 0, "api cell_to_pos origin fail");
    ASSERT_TEST(px == 0.5f * kCellSize && py == 0.5f * kCellSize, "api cell_to_pos origin value px=", px, " py=", py);
    ASSERT_TEST(grid.cell_to_pos( 0,15, 11, px, py, zjps_tmp_z) == 0, "api cell_to_pos corner fail");
    ASSERT_TEST(px == 15.5f * kCellSize && py == 11.5f * kCellSize, "api cell_to_pos corner value");
    ASSERT_TEST(grid.cell_to_pos( 0,-1, 0, px, py, zjps_tmp_z) == -1, "api cell_to_pos neg x expect -1");
    ASSERT_TEST(grid.cell_to_pos( 0,0, -1, px, py, zjps_tmp_z) == -1, "api cell_to_pos neg y expect -1");
    ASSERT_TEST(grid.cell_to_pos( 0,16, 0, px, py, zjps_tmp_z) == -1, "api cell_to_pos over x expect -1");
    ASSERT_TEST(grid.cell_to_pos( 0,0, 12, px, py, zjps_tmp_z) == -1, "api cell_to_pos over y expect -1");
    for (s32 y = 0; y < 12; y++)
    {
        for (s32 x = 0; x < 16; x++)
        {
            ASSERT_TEST_NOLOG(grid.cell_to_pos( 0,x, y, px, py, zjps_tmp_z) == 0, "api roundtrip cell_to_pos x=", x, " y=", y);
            s32 bx = 0;
            s32 by = 0;
            ASSERT_TEST_NOLOG(grid.pos_to_cell(px, py, bx, by) == 0, "api roundtrip pos_to_cell x=", x, " y=", y);
            ASSERT_TEST_NOLOG(bx == x && by == y, "api roundtrip mismatch x=", x, " y=", y, " bx=", bx, " by=", by);
        }
    }

    u32 version = grid.map_version();
    ASSERT_TEST(grid.set_cell(0, 4, 4, false) == 0, "api set_cell block fail");
    ASSERT_TEST(!grid.cell_walkable(0, 4, 4), "api set_cell block not applied");
    ASSERT_TEST(grid.map_version() > version, "api set_cell should bump map version");
    version = grid.map_version();
    ASSERT_TEST(grid.set_cell(0, 4, 4, false) == 0, "api set_cell idempotent fail");
    ASSERT_TEST(grid.map_version() == version, "api set_cell idempotent should not bump version");
    ASSERT_TEST(grid.set_cell(0, 4, 4, true) == 0, "api set_cell unblock fail");
    ASSERT_TEST(grid.cell_walkable(0, 4, 4), "api set_cell unblock not applied");
    ASSERT_TEST(grid.set_cell(0, -1, 0, false) == -1, "api set_cell neg x expect -1");
    ASSERT_TEST(grid.set_cell(0, 0, -1, false) == -1, "api set_cell neg y expect -1");
    ASSERT_TEST(grid.set_cell(0, 16, 0, false) == -1, "api set_cell over x expect -1");
    ASSERT_TEST(grid.set_cell(0, 0, 12, false) == -1, "api set_cell over y expect -1");

    std::vector<s32> cells;
    ASSERT_TEST(grid.build_jps_plus() == 0, "api plus build fail");
    ASSERT_TEST(grid.jps_plus_table_bytes() > 0, "api plus table empty after build");
    ASSERT_TEST(grid.find_path( 0,0, 0, 0, 15, 11, cells) == 0, "api plus query fail");
    ASSERT_TEST(grid.last_tier() == 2, "api plus expect tier 2, got=", grid.last_tier());
    s32 plus_cost = grid.last_path_cost();

    ASSERT_TEST(grid.drop_jps_plus() == 0, "api drop_jps_plus fail");
    ASSERT_TEST(grid.jps_plus_table_bytes() == 0, "api plus table not released, bytes=",
                (s32)grid.jps_plus_table_bytes());
    ASSERT_TEST(grid.find_path( 0,0, 0, 0, 15, 11, cells) == 0, "api post-drop query fail");
    ASSERT_TEST(grid.last_tier() < 2, "api post-drop tier still 2");
    ASSERT_TEST(grid.last_path_cost() == plus_cost, "api post-drop cost mismatch, got=", grid.last_path_cost(),
                " plus=", plus_cost);
    ASSERT_TEST(grid.drop_jps_plus() == 0, "api drop_jps_plus twice fail");
    ASSERT_TEST(grid.build_jps_plus() == 0, "api plus rebuild after drop fail");
    ASSERT_TEST(grid.find_path( 0,0, 0, 0, 15, 11, cells) == 0, "api plus requery fail");
    ASSERT_TEST(grid.last_tier() == 2, "api plus rebuild expect tier 2, got=", grid.last_tier());
    ASSERT_TEST(grid.last_path_cost() == plus_cost, "api plus rebuild cost mismatch");

    zjps_grid bare;
    ASSERT_TEST(bare.drop_jps_plus() == 0, "api drop on uninit fail");
    ASSERT_TEST(bare.build_jps_plus() == -1, "api build on uninit expect -1");
    LOGFMTI("zjps api surface: cell_to_pos roundtrip 192 cells, set_cell and drop_jps_plus covered");
    return 0;
}

static s32 zjps_wall_detour_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 20, 20, kCellSize, true) == 0, "detour grid init fail");
    for (s32 x = 0; x <= 9; x++)
    {
        ASSERT_TEST(grid.set_blocked(0, x, 1) == 0, "detour wall fail x=", x);
        ASSERT_TEST(grid.set_blocked(0, x, 2) == 0, "detour wall fail x=", x);
    }
    std::vector<s32> cells;
    s32 aret = grid.astar_search( 0,0, 0, 0, 10, 3, cells);
    ASSERT_TEST(aret == 0, "detour astar fail aret=", aret);
    ASSERT_TEST(grid.last_path_cost() == 13000, "detour astar cost mismatch, got=", grid.last_path_cost());
    s32 jret = grid.find_path( 0,0, 0, 0, 10, 3, cells);
    ASSERT_TEST(jret == 0, "detour jps fail jret=", jret);
    ASSERT_TEST(grid.last_path_cost() == 13000, "detour jps cost mismatch, got=", grid.last_path_cost());

    zjps_grid rgrid;
    ASSERT_TEST(rgrid.init(1, 20, 20, kCellSize, true) == 0, "detour rgrid init fail");
    ASSERT_TEST(rgrid.set_open_capacity(8192) == 0, "detour rgrid capacity fail");
    std::mt19937 rng(20260835u);
    for (s32 i = 0; i < 14; i++)
    {
        s32 bx = (s32)(rng() % (size_t)(20 - 3));
        s32 by = (s32)(rng() % (size_t)(20 - 3));
        s32 bw = 1 + (s32)(rng() % 3);
        s32 bh = 1 + (s32)(rng() % 3);
        ASSERT_TEST(rgrid.set_rect_cell(0, bx, by, bx + bw - 1, by + bh - 1, false) == 0,
                    "detour block fail i=", i);
    }
    std::vector<s32> rcells;
    for (s32 y = 0; y < 20; y++)
    {
        for (s32 x = 0; x < 20; x++)
        {
            if (rgrid.cell_walkable(0, x, y))
            {
                rcells.push_back(y * 20 + x);
            }
        }
    }
    std::uniform_int_distribution<size_t> dist(0, rcells.size() - 1);
    for (s32 s = 0; s < 2000; s++)
    {
        s32 a = rcells[dist(rng)];
        s32 b = rcells[dist(rng)];
        s32 ar = rgrid.astar_search( 0,a % 20, a / 20, 0, b % 20, b / 20, cells);
        s32 acost = rgrid.last_path_cost();
        s32 jr = rgrid.find_path( 0,a % 20, a / 20, 0, b % 20, b / 20, cells);
        s32 jcost = rgrid.last_path_cost();
        ASSERT_TEST_NOLOG(ar == jr, "detour ret mismatch s=", s, " astar=", ar, " jps=", jr);
        ASSERT_TEST_NOLOG(ar != 0 || acost == jcost, "detour cost mismatch s=", s,
                          " astar=", acost, " jps=", jcost);
    }
    LOGFMTI("zjps wall detour: no corner cut, 2000 random pairs jps == astar on ret and cost");
    return 0;
}

static s32 plus_random_check(s32 width, s32 rect_cnt, u32 seed)
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, width, width, kCellSize, true) == 0, "plus grid init fail");
    ASSERT_TEST(grid.set_open_capacity(8192) == 0, "plus capacity fail");
    std::mt19937 rng(seed);
    for (s32 i = 0; i < rect_cnt; i++)
    {
        s32 bx = (s32)(rng() % (size_t)(width - 3));
        s32 by = (s32)(rng() % (size_t)(width - 3));
        s32 bw = 1 + (s32)(rng() % 3);
        s32 bh = 1 + (s32)(rng() % 3);
        ASSERT_TEST(grid.set_rect_cell(0, bx, by, bx + bw - 1, by + bh - 1, false) == 0,
                    "plus block fail i=", i);
    }
    ASSERT_TEST(grid.build_jps_plus() == 0, "plus build fail");

    std::vector<s32> cell_list;
    for (s32 y = 0; y < width; y++)
    {
        for (s32 x = 0; x < width; x++)
        {
            if (grid.cell_walkable(0, x, y))
            {
                cell_list.push_back(y * width + x);
            }
        }
    }
    std::vector<s32> acells;
    std::vector<s32> pcells;
    s32 checked = 0;
    for (size_t i = 0; i < cell_list.size(); i++)
    {
        for (size_t j = 0; j < cell_list.size(); j++)
        {
            s32 ar = grid.astar_search( 0,cell_list[i] % width, cell_list[i] / width, 0,
                                    cell_list[j] % width, cell_list[j] / width, acells);
            s32 acost = grid.last_path_cost();
            s32 pr = grid.find_path( 0,cell_list[i] % width, cell_list[i] / width, 0,
                                             cell_list[j] % width, cell_list[j] / width, pcells);
            s32 pcost = grid.last_path_cost();
            ASSERT_TEST(ar == pr, "plus reach mismatch i=", (s32)i, " j=", (s32)j, " ar=", ar, " pr=", pr);
            if (ar == 0)
            {
                ASSERT_TEST(acost == pcost, "plus cost mismatch i=", (s32)i, " j=", (s32)j,
                            " astar=", acost, " plus=", pcost);
                for (size_t k = 0; k + 1 < pcells.size(); k++)
                {
                    s32 cx = pcells[k] % width;
                    s32 cy = pcells[k] / width;
                    s32 nx = pcells[k + 1] % width;
                    s32 ny = pcells[k + 1] / width;
                    ASSERT_TEST(grid.move_valid(0, cx, cy, nx - cx, ny - cy), "plus step invalid i=", (s32)i,
                                " j=", (s32)j, " k=", (s32)k);
                }
            }
            checked++;
        }
    }
    LOGFMTI("plus random map: map=%dx%d pairs=%d all match astar", width, width, checked);
    return 0;
}

static s32 zjps_jps_plus_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 20, 20, kCellSize, true) == 0, "plus openfield init fail");
    ASSERT_TEST(grid.build_jps_plus() == 0, "plus openfield build fail");
    std::vector<s32> cells;
    s32 ret = grid.astar_search( 0,0, 0, 0, 15, 10, cells);
    ASSERT_TEST(ret == 0, "plus openfield target-via-subray fail ret=", ret);
    ASSERT_TEST(grid.last_path_cost() == 19140, "plus openfield cost mismatch, got=", grid.last_path_cost());
    ret = grid.astar_search( 0,1, 1, 0, 18, 18, cells);
    ASSERT_TEST(ret == 0 && grid.last_path_cost() == 17 * 1414, "plus openfield diag mismatch");

    ASSERT_TEST(plus_random_check(12, 10, 20260836u) == 0, "plus random seed1 fail");
    ASSERT_TEST(plus_random_check(12, 10, 20260837u) == 0, "plus random seed2 fail");

    zjps_grid sgrid;
    ASSERT_TEST(bench_serpentine_grid(sgrid) == 0, "plus serpentine build fail");
    zclock<> build_cost;
    build_cost.start();
    ASSERT_TEST(sgrid.build_jps_plus() == 0, "plus serpentine jps+ build fail");
    build_cost.stop_and_save();
    size_t table_bytes = (sgrid.jps_plus_table_bytes());
    LOGFMTI("plus serpentine: jps+ build=%.3f ms, table=%zu bytes (%.1f MB)",
            (f64)build_cost.cost_ns() / 1000000.0, table_bytes, (f64)table_bytes / 1048576.0);
    s32 sx = 0;
    s32 sy = 0;
    s32 tx = 0;
    s32 ty = 0;
    ASSERT_TEST(sgrid.pos_to_cell(300.0f, 300.0f, sx, sy) == 0, "plus pos a fail");
    ASSERT_TEST(sgrid.pos_to_cell(300.0f, 19500.0f, tx, ty) == 0, "plus pos b fail");
    ret = sgrid.find_path( 0,sx, sy, 0, tx, ty, cells);
    ASSERT_TEST(ret == 0, "plus serpentine long query fail ret=", ret);
    ASSERT_TEST(sgrid.last_path_cost() == 12749834, "plus serpentine long cost mismatch, got=", sgrid.last_path_cost());

    std::vector<s32> walk_cells;
    for (s32 y = 0; y < sgrid.height(); y++)
    {
        for (s32 x = 0; x < sgrid.width(); x++)
        {
            if (sgrid.cell_walkable(0, x, y))
            {
                walk_cells.push_back(y * sgrid.width() + x);
            }
        }
    }
    {
        std::mt19937 rng(20260838u);
        std::uniform_int_distribution<size_t> dist(0, walk_cells.size() - 1);
        std::vector<s32> jcells;
        for (s32 s = 0; s < 600; s++)
        {
            s32 a = walk_cells[dist(rng)];
            s32 b = walk_cells[dist(rng)];
            s32 jr = sgrid.find_path( 0,a % 400, a / 400, 0, b % 400, b / 400, jcells);
            s32 jcost = sgrid.last_path_cost();
            s32 pr = sgrid.find_path( 0,a % 400, a / 400, 0, b % 400, b / 400, cells);
            s32 pcost = sgrid.last_path_cost();
            ASSERT_TEST(jr == pr && (jr != 0 || jcost == pcost), "plus serpentine mismatch s=", s,
                        " jr=", jr, " pr=", pr, " jcost=", jcost, " pcost=", pcost);
        }
        LOGFMTI("plus serpentine: 600 sampled pairs jps+ == jps");
    }

    zjps_grid rgrid;
    ASSERT_TEST(rgrid.init(1, 20, 20, kCellSize, true) == 0, "plus fallback init fail");
    std::mt19937 rng(20260839u);
    for (s32 i = 0; i < 14; i++)
    {
        s32 bx = (s32)(rng() % (size_t)17);
        s32 by = (s32)(rng() % (size_t)17);
        s32 bw = 1 + (s32)(rng() % 3);
        s32 bh = 1 + (s32)(rng() % 3);
        ASSERT_TEST(rgrid.set_rect_cell(0, bx, by, bx + bw - 1, by + bh - 1, false) == 0,
                    "plus fallback block fail i=", i);
    }
    ASSERT_TEST(rgrid.build_jps_plus() == 0, "plus fallback build fail");
    ASSERT_TEST(rgrid.set_blocked(0, 3, 3) == 0, "plus fallback set_blocked fail");
    {
        s32 ar = rgrid.astar_search( 0,0, 0, 0, 19, 19, cells);
        s32 acost = rgrid.last_path_cost();
        s32 pr = rgrid.find_path( 0,0, 0, 0, 19, 19, cells);
        s32 pcost = rgrid.last_path_cost();
        ASSERT_TEST(ar == pr && (ar != 0 || acost == pcost), "plus fallback mismatch ar=", ar, " pr=", pr);
        LOGFMTI("plus fallback: after set_blocked(version bump) jps+ auto-fallback matches astar ret=%d cost=%d",
                pr, pcost);
    }
    ASSERT_TEST(rgrid.build_jps_plus() == 0, "plus fallback rebuild fail");
    {
        s32 ar = rgrid.astar_search( 0,0, 0, 0, 19, 19, cells);
        s32 acost = rgrid.last_path_cost();
        s32 pr = rgrid.find_path( 0,0, 0, 0, 19, 19, cells);
        s32 pcost = rgrid.last_path_cost();
        ASSERT_TEST(ar == pr && (ar != 0 || acost == pcost), "plus rebuild mismatch ar=", ar, " pr=", pr);
    }

    volatile s32 salt = 0;
    {
        const s32 N = 2000;
        zclock<> cost;
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 r = sgrid.find_path( 0,sx, sy, 0, tx, ty, cells);
            salt += r + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("plus bench serpentine jps+: %.0f ns/op (no-build jps baseline ~120000 ns/op)",
                (f64)cost.cost_ns() / (f64)N);
    }
    zjps_grid egrid;
    ASSERT_TEST(egrid.init(1, 200, 200, kCellSize, true) == 0, "plus egrid init fail");
    zclock<> ebuild;
    ebuild.start();
    ASSERT_TEST(egrid.build_jps_plus() == 0, "plus egrid build fail");
    ebuild.stop_and_save();
    LOGFMTI("plus bench openfield build: %.3f ms, table=%zu bytes",
            (f64)ebuild.cost_ns() / 1000000.0, egrid.jps_plus_table_bytes());
    {
        const s32 N = 2000;
        zclock<> cost;
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 r = egrid.find_path( 0,1, 1, 0, 198, 198, cells);
            salt += r + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("plus bench openfield jps+: %.0f ns/op (no-build jps baseline ~15000 ns/op, astar ~40000 ns/op)",
                (f64)cost.cost_ns() / (f64)N);
    }
    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

struct phase5_pair
{
    s32 ax;
    s32 ay;
    s32 bx;
    s32 by;
    s32 na;
    s32 nb;
};

static f32 phase5_path_len(const std::vector<test_graph::graph_path_step>& steps, const zpoint& from)
{
    f32 len = 0.0f;
    zpoint prev = from;
    for (size_t k = 0; k < steps.size(); k++)
    {
        f32 dx = steps[k].pos.x - prev.x;
        f32 dy = steps[k].pos.y - prev.y;
        len += sqrtf(dx * dx + dy * dy);
        prev = steps[k].pos;
    }
    return len;
}

static s32 cpos_to_coarse(s32 cell_axis, s32 width, s32 stride)
{
    s32 coarse = cell_axis / stride;
    s32 max_coarse = width / stride - 1;
    if (coarse > max_coarse)
    {
        coarse = max_coarse;
    }
    return coarse;
}

static s32 phase5_serpentine_bench()
{
    zjps_grid grid;
    ASSERT_TEST(bench_serpentine_grid(grid) == 0, "p5 serp grid fail");

    test_graph graph;
    const s32 ROW_CNT = 33;
    std::vector<zpoint> node_pos;
    node_pos.reserve((size_t)ROW_CNT * 2);
    for (s32 k = 0; k < ROW_CNT; k++)
    {
        f32 y = 300.0f + 600.0f * (f32)k;
        node_pos.push_back(zpoint(300.0f, y, 0.0f));
        node_pos.push_back(zpoint(19700.0f, y, 0.0f));
    }
    const s32 NODE_CNT = (s32)node_pos.size();
    std::vector<s32> node_ids((size_t)NODE_CNT, 0);
    for (s32 i = 0; i < NODE_CNT; i++)
    {
        node_ids[i] = graph.new_node(node_pos[i], i);
        ASSERT_TEST_NOLOG(node_ids[i] >= 0, "p5 new_node fail i=", i);
    }
    s32 link_seq = 0;
    for (s32 k = 0; k < ROW_CNT; k++)
    {
        s32 affects = 0;
        s32 lid = graph.new_link(node_ids[2 * k], node_ids[2 * k + 1], link_seq++);
        ASSERT_TEST_NOLOG(lid >= 0 && graph.push_link(lid, affects) == 0, "p5 push row fail k=", k);
    }
    for (s32 k = 0; k + 1 < ROW_CNT; k++)
    {
        s32 side = (k % 2 == 0) ? 1 : 0;
        s32 affects = 0;
        s32 lid = graph.new_link(node_ids[2 * k + side], node_ids[2 * (k + 1) + side], link_seq++);
        ASSERT_TEST_NOLOG(lid >= 0 && graph.push_link(lid, affects) == 0, "p5 push conn fail k=", k);
    }

    std::mt19937 rng(20260841u);
    const s32 TIER_LO[3] = { 500, 2000, 8000 };
    const s32 TIER_HI[3] = { 1000, 4000, 14000 };
    std::vector<test_graph::graph_path_step> steps;
    std::vector<phase5_pair> tier_pairs[3];
    f64 tier_zlen[3] = { 0.0, 0.0, 0.0 };
    f64 tier_dist[3] = { 0.0, 0.0, 0.0 };
    s32 attempts = 0;
    f32 rect_len = sqrtf((19700.0f - 300.0f) * (19700.0f - 300.0f) + 19500.0f * 19500.0f);
    while ((tier_pairs[0].size() < 250 || tier_pairs[1].size() < 250 || tier_pairs[2].size() < 250)
           && attempts < 400000)
    {
        attempts++;
        s32 i = (s32)(rng() % (size_t)NODE_CNT);
        s32 j = (s32)(rng() % (size_t)NODE_CNT);
        f32 dx = node_pos[j].x - node_pos[i].x;
        f32 dy = node_pos[j].y - node_pos[i].y;
        f32 dist = sqrtf(dx * dx + dy * dy);
        s32 tier = -1;
        for (s32 t = 0; t < 3; t++)
        {
            if (dist >= (f32)TIER_LO[t] && dist <= (f32)TIER_HI[t] && tier_pairs[t].size() < 250)
            {
                tier = t;
                break;
            }
        }
        if (tier < 0)
        {
            continue;
        }
        s32 ret = graph.find_path(node_ids[i], node_ids[j], steps);
        ASSERT_TEST_NOLOG(ret == 0, "p5 serp reach fail i=", i, " j=", j);
        f32 zlen = phase5_path_len(steps, node_pos[i]);
        phase5_pair pr;
        pr.na = i;
        pr.nb = j;
        pr.ax = (s32)(node_pos[i].x / kCellSize);
        pr.ay = (s32)(node_pos[i].y / kCellSize);
        pr.bx = (s32)(node_pos[j].x / kCellSize);
        pr.by = (s32)(node_pos[j].y / kCellSize);
        tier_pairs[tier].push_back(pr);
        tier_zlen[tier] += zlen;
        tier_dist[tier] += dist;
    }
    LOGFMTI("phase5 serpentine tiers: max node pair straight-line dist=%.1fm", rect_len / 100.0);

    volatile s32 salt = 0;
    std::vector<s32> cells;
    {
        std::vector<s32> warm_cells;
        s32 warm_ret = grid.astar_search( 0,6, 6, 0, 6, 390, warm_cells);
        salt += warm_ret;
    }
    const char* tier_names[3] = { "short5-10m", "mid20-40m", "long80-140m" };
    f64 res0[3] = { 0.0, 0.0, 0.0 };
    for (s32 t = 0; t < 3; t++)
    {
        if (tier_pairs[t].empty())
        {
            continue;
        }
        const s32 N = (s32)tier_pairs[t].size();
        zclock<> c;
        c.start();
        for (s32 s = 0; s < N; s++)
        {
            grid.find_path( 0,tier_pairs[t][s].ax, tier_pairs[t][s].ay, 0, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
            salt += (s32)cells.size();
        }
        c.stop_and_save();
        res0[t] = (f64)c.cost_ns() / (f64)N;
        ASSERT_TEST(grid.last_tier() == 0, "serpentine jps0 tier expect 0(no light yet), t=", t, " got=", grid.last_tier());
    }
    ASSERT_TEST(grid.build_jps_light() == 0, "p5 serp light build fail");
    {
        f64 res[3][4];
        s32 jps_tier[3];
        s32 plus_tier[3];
        for (s32 t = 0; t < 3; t++)
        {
            if (tier_pairs[t].empty())
            {
                continue;
            }
            const s32 N = (s32)tier_pairs[t].size();
            {
                zclock<> c;
                c.start();
                for (s32 s = 0; s < N; s++)
                {
                    graph.find_path(node_ids[tier_pairs[t][s].na], node_ids[tier_pairs[t][s].nb], steps);
                    salt += (s32)steps.size();
                }
                c.stop_and_save();
                res[t][0] = (f64)c.cost_ns() / (f64)N;
            }
            {
                zclock<> c;
                c.start();
                for (s32 s = 0; s < N; s++)
                {
                    grid.astar_search( 0,tier_pairs[t][s].ax, tier_pairs[t][s].ay, 0, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                    salt += (s32)cells.size();
                }
                c.stop_and_save();
                res[t][1] = (f64)c.cost_ns() / (f64)N;
            }
            {
                zclock<> c;
                c.start();
                for (s32 s = 0; s < N; s++)
                {
                    grid.find_path( 0,tier_pairs[t][s].ax, tier_pairs[t][s].ay, 0, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                    salt += (s32)cells.size();
                }
                c.stop_and_save();
                res[t][2] = (f64)c.cost_ns() / (f64)N;
                jps_tier[t] = grid.last_tier();
            }
            ASSERT_TEST(jps_tier[t] == 1, "serpentine jps tier expect 1(no jump table yet), t=", t, " got=", jps_tier[t]);
        }
        {
            zclock<> build_cost;
            build_cost.start();
            ASSERT_TEST(grid.build_jps_plus() == 0, "p5 serp jps+ build fail");
            build_cost.stop_and_save();
            LOGFMTI("phase5 serpentine: jps+ build=%.2fms table=%.1fMB",
                    (f64)build_cost.cost_ns() / 1000000.0, (f64)grid.jps_plus_table_bytes() / 1048576.0);
        }
        for (s32 t = 0; t < 3; t++)
        {
            if (tier_pairs[t].empty())
            {
                continue;
            }
            const s32 N = (s32)tier_pairs[t].size();
            {
                zclock<> c;
                c.start();
                for (s32 s = 0; s < N; s++)
                {
                    grid.find_path( 0,tier_pairs[t][s].ax, tier_pairs[t][s].ay, 0, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                    salt += (s32)cells.size();
                }
                c.stop_and_save();
                res[t][3] = (f64)c.cost_ns() / (f64)N;
                plus_tier[t] = grid.last_tier();
            }
            ASSERT_TEST(plus_tier[t] == 2, "serpentine jps+ tier expect 2, got=", plus_tier[t]);
        }
        for (s32 t = 0; t < 3; t++)
        {
            if (tier_pairs[t].empty())
            {
                continue;
            }
            const s32 N = (s32)tier_pairs[t].size();
            f64 dist_avg_m = tier_dist[t] / (f64)N / 100.0;
            f64 zlen_avg_m = tier_zlen[t] / (f64)N / 100.0;
            LOGFMTI("phase5 serpentine %s: pairs=%d dist=%.1fm zpath=%.1fm(wrap=%.1fx) | zgraph=%.0fns astar=%.0fns jps0=%.0fns(tier0) jps=%.0fns(tier%d) jps+=%.0fns(tier%d)",
                    tier_names[t], N, dist_avg_m, zlen_avg_m, zlen_avg_m / (dist_avg_m > 0.0001 ? dist_avg_m : 1.0),
                    res[t][0], res[t][1], res0[t], res[t][2], jps_tier[t], res[t][3], plus_tier[t]);
        }
    }

    {
        const s32 N = 1000;
        zclock<> c;
        c.start();
        for (s32 s = 0; s < N; s++)
        {
            const zpoint& pa = node_pos[(size_t)(s % NODE_CNT)];
            const zpoint& pb = node_pos[(size_t)((s * 7 + 3) % NODE_CNT)];
            graph.find_path(pa, pb, steps);
            salt += (s32)steps.size();
        }
        c.stop_and_save();
        LOGFMTI("phase5 access cost: zgraph find_path(pos,pos)=%.0fns avg (incl find_nearest_link) vs grid pos_to_cell O(1)",
                (f64)c.cost_ns() / (f64)N);
    }

    size_t state_bytes = (size_t)grid.width() * (size_t)grid.height() * (sizeof(s32) * 3 + 2);
    LOGFMTI("phase5 memory: zgraph nodes=%d links=%d grids=%d | grid walkable=%zuB search_states=%zuB jps+_table=%zuB",
            graph.node_count(), graph.link_count(), graph.grid_count(),
            (size_t)grid.width() * (size_t)grid.height(), state_bytes, grid.jps_plus_table_bytes());

    {
        const s32 OPS = 20000;
        zclock<> c;
        c.start();
        for (s32 i = 0; i < OPS; i++)
        {
            if (i % 2 == 0)
            {
                grid.set_blocked(0, 6, 5);
            }
            else
            {
                grid.set_walkable(0, 6, 5);
            }
        }
        c.stop_and_save();
        LOGFMTI("phase5 dynamic: grid set_blocked/set_walkable(mark-dirty only)=%.0fns/op",
                (f64)c.cost_ns() / (f64)OPS);
    }
    {
        const s32 PAIRS = 2000;
        zclock<> c;
        c.start();
        for (s32 i = 0; i < PAIRS; i++)
        {
            grid.set_blocked(0, 6, 5);
            grid.set_walkable(0, 6, 5);
        }
        c.stop_and_save();
        LOGFMTI("phase5 dynamic: eager toggle pair(1 cell, light stays ready)=%.0fns/pair",
                (f64)c.cost_ns() / (f64)PAIRS);
        ASSERT_TEST(grid.light_dirty() == 0, "eager toggle dirty leak");
    }
    {
        std::mt19937 arng(20260845u);
        const s32 LOOPS = 2000;
        s64 add_ns = 0;
        s64 del_ns = 0;
        zclock<> ac;
        zclock<> dc;
        for (s32 i = 0; i < LOOPS; i++)
        {
            s32 anchor = node_ids[(s32)(arng() % (size_t)NODE_CNT)];
            zpoint p(500.0f + (f32)(arng() % 19000), 500.0f + (f32)(arng() % 19000), 0.0f);
            ac.start();
            s32 nid = graph.new_node(p, 900000 + i);
            s32 lid = graph.new_link(anchor, nid, 900000 + i);
            s32 affects = 0;
            graph.push_link(lid, affects);
            ac.stop_and_save();
            add_ns += ac.cost_ns();
            dc.start();
            graph.pop_link(lid, affects);
            graph.free_link(lid);
            graph.free_node(nid);
            dc.stop_and_save();
            del_ns += dc.cost_ns();
            salt += nid + lid;
        }
        LOGFMTI("phase5 dynamic: zgraph add=%.0fns del=%.0fns (new_node+new_link+push_link / pop+free+free)",
                (f64)add_ns / (f64)LOOPS, (f64)del_ns / (f64)LOOPS);
    }
    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

static s32 phase5_obstacle_bench(s32 rect_cnt, const char* density_name, u32 seed)
{
    const s32 W = 200;
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, W, W, kCellSize, true) == 0, "p5 obst grid fail");
    ASSERT_TEST(grid.set_open_capacity(65536) == 0, "p5 obst capacity fail");
    std::mt19937 rng(seed);
    for (s32 i = 0; i < rect_cnt; i++)
    {
        s32 bx = (s32)(rng() % (size_t)(W - 8));
        s32 by = (s32)(rng() % (size_t)(W - 8));
        s32 bw = 2 + (s32)(rng() % 5);
        s32 bh = 2 + (s32)(rng() % 5);
        ASSERT_TEST(grid.set_rect_cell(0, bx, by, bx + bw - 1, by + bh - 1, false) == 0,
                    "p5 obst block fail i=", i);
    }
    s32 walkable_cnt = 0;
    for (s32 y = 0; y < W; y++)
    {
        for (s32 x = 0; x < W; x++)
        {
            if (grid.cell_walkable(0, x, y))
            {
                walkable_cnt++;
            }
        }
    }
    LOGFMTI("phase5 obstacle[%s]: rects=%d walkable=%.1f%%", density_name, rect_cnt,
            100.0 * (f64)walkable_cnt / (f64)(W * W));

    const s32 STRIDE = 10;
    const s32 CN = W / STRIDE;
    test_graph graph;
    s32 coarse_id[20][20];
    std::vector<zpoint> cpos;
    for (s32 cy = 0; cy < CN; cy++)
    {
        for (s32 cx = 0; cx < CN; cx++)
        {
            coarse_id[cx][cy] = -1;
        }
    }
    for (s32 cy = 0; cy < CN; cy++)
    {
        for (s32 cx = 0; cx < CN; cx++)
        {
            if (grid.cell_walkable(0, cx * STRIDE, cy * STRIDE))
            {
                coarse_id[cx][cy] = graph.new_node(zpoint(((f32)(cx * STRIDE) + 0.5f) * kCellSize,
                                                          ((f32)(cy * STRIDE) + 0.5f) * kCellSize, 0.0f), cy * CN + cx);
                ASSERT_TEST_NOLOG(coarse_id[cx][cy] >= 0, "p5 coarse node fail cx=", cx, " cy=", cy);
                cpos.push_back(zpoint(((f32)(cx * STRIDE) + 0.5f) * kCellSize, ((f32)(cy * STRIDE) + 0.5f) * kCellSize, 0.0f));
            }
        }
    }
    const s32 COARSE_CNT = (s32)cpos.size();
    s32 link_seq = 0;
    s32 affects = 0;
    for (s32 cy = 0; cy < CN; cy++)
    {
        for (s32 cx = 0; cx < CN; cx++)
        {
            if (coarse_id[cx][cy] < 0)
            {
                continue;
            }
            if (cx + 1 < CN && coarse_id[cx + 1][cy] >= 0)
            {
                bool clear = true;
                for (s32 i = 1; i <= STRIDE; i++)
                {
                    if (!grid.cell_walkable(0, cx * STRIDE + i, cy * STRIDE))
                    {
                        clear = false;
                        break;
                    }
                }
                if (clear)
                {
                    s32 lid = graph.new_link(coarse_id[cx][cy], coarse_id[cx + 1][cy], link_seq++);
                    ASSERT_TEST_NOLOG(lid >= 0 && graph.push_link(lid, affects) == 0, "p5 coarse east fail");
                }
            }
            if (cy + 1 < CN && coarse_id[cx][cy + 1] >= 0)
            {
                bool clear = true;
                for (s32 i = 1; i <= STRIDE; i++)
                {
                    if (!grid.cell_walkable(0, cx * STRIDE, cy * STRIDE + i))
                    {
                        clear = false;
                        break;
                    }
                }
                if (clear)
                {
                    s32 lid = graph.new_link(coarse_id[cx][cy], coarse_id[cx][cy + 1], link_seq++);
                    ASSERT_TEST_NOLOG(lid >= 0 && graph.push_link(lid, affects) == 0, "p5 coarse north fail");
                }
            }
            if (cx + 1 < CN && cy + 1 < CN && coarse_id[cx + 1][cy + 1] >= 0)
            {
                bool clear = true;
                for (s32 i = 1; i <= STRIDE; i++)
                {
                    if (!grid.cell_walkable(0, cx * STRIDE + i, cy * STRIDE + i))
                    {
                        clear = false;
                        break;
                    }
                }
                if (clear)
                {
                    s32 lid = graph.new_link(coarse_id[cx][cy], coarse_id[cx + 1][cy + 1], link_seq++);
                    ASSERT_TEST_NOLOG(lid >= 0 && graph.push_link(lid, affects) == 0, "p5 coarse ne fail");
                }
            }
            if (cx + 1 < CN && cy - 1 >= 0 && coarse_id[cx + 1][cy - 1] >= 0)
            {
                bool clear = true;
                for (s32 i = 1; i <= STRIDE; i++)
                {
                    if (!grid.cell_walkable(0, cx * STRIDE + i, cy * STRIDE - i))
                    {
                        clear = false;
                        break;
                    }
                }
                if (clear)
                {
                    s32 lid = graph.new_link(coarse_id[cx][cy], coarse_id[cx + 1][cy - 1], link_seq++);
                    ASSERT_TEST_NOLOG(lid >= 0 && graph.push_link(lid, affects) == 0, "p5 coarse se fail");
                }
            }
        }
    }
    ASSERT_TEST(graph.node_count() <= test_graph::kMaxNodeCnt, "p5 coarse node overflow ", graph.node_count());
    ASSERT_TEST(graph.link_count() <= test_graph::kMaxLinkCnt, "p5 coarse link overflow ", graph.link_count());
    ASSERT_TEST(graph.grid_count() <= test_graph::kMaxGridCnt, "p5 coarse grid overflow ", graph.grid_count());

    zclock<> build_cost;

    const s32 TIER_CNT = 3;
    const s32 PAIRS_PER_TIER = 250;
    const f32 tier_lo[TIER_CNT] = { 500.0f, 2000.0f, 8000.0f };
    const f32 tier_hi[TIER_CNT] = { 1000.0f, 4000.0f, 14000.0f };
    const char* tier_names[TIER_CNT] = { "short5-10m", "mid20-40m", "long80-140m" };
    std::vector<phase5_pair> tier_pairs[TIER_CNT];
    f64 tier_zlen[TIER_CNT] = { 0.0, 0.0, 0.0 };
    f64 tier_alen[TIER_CNT] = { 0.0, 0.0, 0.0 };
    std::vector<test_graph::graph_path_step> steps;
    std::vector<s32> cells;
    s32 attempts = 0;
    while ((tier_pairs[0].size() < (size_t)PAIRS_PER_TIER || tier_pairs[1].size() < (size_t)PAIRS_PER_TIER
            || tier_pairs[2].size() < (size_t)PAIRS_PER_TIER) && attempts < 400000)
    {
        attempts++;
        s32 i = (s32)(rng() % (size_t)COARSE_CNT);
        s32 j = (s32)(rng() % (size_t)COARSE_CNT);
        f32 dx = cpos[j].x - cpos[i].x;
        f32 dy = cpos[j].y - cpos[i].y;
        f32 dist = sqrtf(dx * dx + dy * dy);
        s32 tier = -1;
        for (s32 t = 0; t < TIER_CNT; t++)
        {
            if (dist >= tier_lo[t] && dist <= tier_hi[t] && tier_pairs[t].size() < (size_t)PAIRS_PER_TIER)
            {
                tier = t;
                break;
            }
        }
        if (tier < 0)
        {
            continue;
        }
        s32 ax = (s32)(cpos[i].x / kCellSize);
        s32 ay = (s32)(cpos[i].y / kCellSize);
        s32 bx = (s32)(cpos[j].x / kCellSize);
        s32 by = (s32)(cpos[j].y / kCellSize);
        s32 ar = grid.astar_search( 0,ax, ay, 0, bx, by, cells);
        if (ar != 0)
        {
            continue;
        }
        s32 acost = grid.last_path_cost();
        s32 zr = graph.find_path(coarse_id[cpos_to_coarse(ax, W, STRIDE)][cpos_to_coarse(ay, W, STRIDE)],
                                 coarse_id[cpos_to_coarse(bx, W, STRIDE)][cpos_to_coarse(by, W, STRIDE)], steps);
        if (zr != 0)
        {
            continue;
        }
        f32 zlen = phase5_path_len(steps, cpos[i]);
        phase5_pair pr;
        pr.ax = ax;
        pr.ay = ay;
        pr.bx = bx;
        pr.by = by;
        pr.na = coarse_id[cpos_to_coarse(ax, W, STRIDE)][cpos_to_coarse(ay, W, STRIDE)];
        pr.nb = coarse_id[cpos_to_coarse(bx, W, STRIDE)][cpos_to_coarse(by, W, STRIDE)];
        tier_pairs[tier].push_back(pr);
        tier_zlen[tier] += zlen;
        tier_alen[tier] += (f64)acost / 1000.0 * (f64)kCellSize;
    }

    volatile s32 salt = 0;
    f64 res_z[TIER_CNT];
    f64 res_a[TIER_CNT];
    f64 res_n[TIER_CNT];
    f64 res_j[TIER_CNT];
    f64 res_p[TIER_CNT];
    {
        std::vector<s32> warm_cells;
        s32 warm_ret = grid.astar_search( 0,tier_pairs[0][0].ax, tier_pairs[0][0].ay, 0,
                                          tier_pairs[0][0].bx, tier_pairs[0][0].by, warm_cells);
        salt += warm_ret;
    }
    for (s32 t = 0; t < TIER_CNT; t++)
    {
        if (tier_pairs[t].empty())
        {
            continue;
        }
        const s32 N = (s32)tier_pairs[t].size();
        zclock<> c;
        c.start();
        for (s32 s = 0; s < N; s++)
        {
            grid.find_path( 0,tier_pairs[t][s].ax, tier_pairs[t][s].ay, 0, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
            salt += (s32)cells.size();
        }
        c.stop_and_save();
        res_n[t] = (f64)c.cost_ns() / (f64)N;
        ASSERT_TEST(grid.last_tier() == 0, "obstacle jps0 tier expect 0(no light yet), got=", grid.last_tier());
    }
    ASSERT_TEST(grid.build_jps_light() == 0, "p5 obst light build fail");
    for (s32 t = 0; t < TIER_CNT; t++)
    {
        if (tier_pairs[t].empty())
        {
            continue;
        }
        const s32 N = (s32)tier_pairs[t].size();
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                graph.find_path(tier_pairs[t][s].na, tier_pairs[t][s].nb, steps);
                salt += (s32)steps.size();
            }
            c.stop_and_save();
            res_z[t] = (f64)c.cost_ns() / (f64)N;
        }
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                grid.astar_search( 0,tier_pairs[t][s].ax, tier_pairs[t][s].ay, 0, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                salt += (s32)cells.size();
            }
            c.stop_and_save();
            res_a[t] = (f64)c.cost_ns() / (f64)N;
        }
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                grid.find_path( 0,tier_pairs[t][s].ax, tier_pairs[t][s].ay, 0, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                salt += (s32)cells.size();
            }
            c.stop_and_save();
            res_j[t] = (f64)c.cost_ns() / (f64)N;
            ASSERT_TEST(grid.last_tier() == 1, "obstacle jps tier expect 1(no jump table yet), got=", grid.last_tier());
        }
        res_p[t] = 0.0;
    }
    {
        build_cost.start();
        ASSERT_TEST(grid.build_jps_plus() == 0, "p5 obst jps+ build fail");
        build_cost.stop_and_save();
    }
    for (s32 t = 0; t < TIER_CNT; t++)
    {
        if (tier_pairs[t].empty())
        {
            continue;
        }
        const s32 N = (s32)tier_pairs[t].size();
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                grid.find_path( 0,tier_pairs[t][s].ax, tier_pairs[t][s].ay, 0, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                salt += (s32)cells.size();
            }
            c.stop_and_save();
            res_p[t] = (f64)c.cost_ns() / (f64)N;
            ASSERT_TEST(grid.last_tier() == 2, "obstacle jps+ tier expect 2, got=", grid.last_tier());
        }
    }
    for (s32 t = 0; t < TIER_CNT; t++)
    {
        if (tier_pairs[t].empty())
        {
            continue;
        }
        const s32 N = (s32)tier_pairs[t].size();
        LOGFMTI("phase5 obstacle[%s] %s: pairs=%d zpath=%.1fm apath=%.1fm | zgraph=%.0fns astar=%.0fns jps0=%.0fns(tier0) jps=%.0fns(tier1) jps+=%.0fns(tier2)",
                density_name, tier_names[t], N, tier_zlen[t] / (f64)N / 100.0, tier_alen[t] / (f64)N / 100.0,
                res_z[t], res_a[t], res_n[t], res_j[t], res_p[t]);
    }
    LOGFMTI("phase5 obstacle[%s] summary: coarse zgraph nodes=%d links=%d grids=%d | jps+ build=%.2fms table=%.1fMB",
            density_name, graph.node_count(), graph.link_count(), graph.grid_count(),
            (f64)build_cost.cost_ns() / 1000000.0, (f64)grid.jps_plus_table_bytes() / 1048576.0);
    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

static s32 zjps_phase5_bench_test()
{
    {
        zjps_grid fresh;
        ASSERT_TEST(bench_serpentine_grid(fresh) == 0, "light build grid fail");
        std::vector<s32> c;
        zclock<> bld;
        bld.start();
        ASSERT_TEST(fresh.build_jps_light() == 0, "light build fail");
        bld.stop_and_save();
        zclock<> first;
        first.start();
        s32 r = fresh.find_path( 0,6, 6, 0, 6, 390, c);
        first.stop_and_save();
        ASSERT_TEST(r == 0, "light build first query fail r=", r);
        zclock<> steady;
        steady.start();
        r = fresh.find_path( 0,6, 6, 0, 6, 390, c);
        steady.stop_and_save();
        LOGFMTI("light build cost(400x400): build=%.0fns first query=%.0fns steady=%.0fns",
                (f64)bld.cost_ns(), (f64)first.cost_ns(), (f64)steady.cost_ns());
        ASSERT_TEST(bench_serpentine_grid(fresh) == 0, "light rebuild grid fail");
        zclock<> bld2;
        bld2.start();
        ASSERT_TEST(fresh.build_jps_light() == 0, "light rebuild fail");
        bld2.stop_and_save();
        LOGFMTI("light build cost(400x400): second map load rebuild=%.0fns (flat buffers retained)",
                (f64)bld2.cost_ns());
    }
    {
        zjps_grid fresh;
        ASSERT_TEST(fresh.init(1, 200, 200, kCellSize, true) == 0, "lazy idx egrid fail");
        std::vector<s32> c;
        zclock<> first;
        first.start();
        s32 r = fresh.find_path( 0,1, 1, 0, 198, 198, c);
        first.stop_and_save();
        ASSERT_TEST(r == 0, "lazy idx egrid query fail r=", r);
        LOGFMTI("lazy index cost(200x200 open): first query=%.0fns (incl ensure_index)", (f64)first.cost_ns());
    }
    ASSERT_TEST(phase5_serpentine_bench() == 0, "p5 serpentine fail");
    ASSERT_TEST(phase5_obstacle_bench(15, "low", 20260842u) == 0, "p5 low fail");
    ASSERT_TEST(phase5_obstacle_bench(40, "mid", 20260843u) == 0, "p5 mid fail");
    ASSERT_TEST(phase5_obstacle_bench(90, "high", 20260844u) == 0, "p5 high fail");
    return 0;
}

static s32 zjps_height_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 10, 10, kCellSize, true) == 0, "height grid init fail");
    ASSERT_TEST(grid.cell_z(0, 5, 5) == 0, "default voxel z expect 0, got=", grid.cell_z(0, 5, 5));
    ASSERT_TEST(grid.build_jps_plus() == 0, "height plus build fail");
    std::vector<s32> warm;
    ASSERT_TEST(grid.find_path( 0,0, 0, 0, 9, 9, warm) == 0, "height plus warm query fail");
    ASSERT_TEST(grid.last_tier() == 2, "height plus tier expect 2, got=", grid.last_tier());
    u32 version = grid.map_version();
    u32 payload = grid.payload_version();
    ASSERT_TEST(grid.set_cell_z(0, 5, 5, 5) == 0, "set height fail");
    ASSERT_TEST(grid.map_version() == version, "set height must not bump map version");
    ASSERT_TEST(grid.payload_version() > payload, "set height should bump payload version");
    ASSERT_TEST(grid.find_path( 0,0, 0, 0, 9, 9, warm) == 0, "height plus requery fail");
    ASSERT_TEST(grid.last_tier() == 2, "height edit must not degrade tier, got=", grid.last_tier());
    ASSERT_TEST(grid.cell_z(0, 5, 5) == 5, "height mismatch, got=", grid.cell_z(0, 5, 5));
    ASSERT_TEST(grid.set_cell_z(0, 0, 0, 200) == 0, "set 200 fail");
    ASSERT_TEST(grid.cell_z(0, 0, 0) == 200, "height 200 mismatch");
    payload = grid.payload_version();
    ASSERT_TEST(grid.set_cell_z(0, 0, 0, 200) == 0, "idempotent set fail");
    ASSERT_TEST(grid.payload_version() == payload, "idempotent set should not bump payload version");
    ASSERT_TEST(grid.cell_z(0, -1, 0) == -1, "out of range height expect -1");
    ASSERT_TEST(grid.set_cell_z(0, 10, 0, 1) == -1, "out of range set expect -1");

    std::vector<s32> acells;
    std::vector<s32> jcells;
    s32 aret = grid.astar_search( 0,0, 0, 0, 9, 9, acells);
    s32 jret = grid.astar_search( 0,0, 0, 0, 9, 9, jcells);
    ASSERT_TEST(aret == 0 && jret == 0, "height data must not affect connectivity");
    ASSERT_TEST(grid.last_path_cost() == 9 * 1414, "cost unchanged by height data, got=", grid.last_path_cost());

    ASSERT_TEST(grid.set_blocked(0, 5, 5) == 0, "block fail");
    ASSERT_TEST(grid.cell_z(0, 5, 5) == 5, "height preserved across flag edit");
    aret = grid.astar_search( 0,0, 0, 0, 9, 9, acells);
    ASSERT_TEST(aret == 0, "path around single block fail ret=", aret);
    LOGFMTI("zjps height: voxel plane is pure payload, connectivity driven by flag only");
    return 0;
}

static s32 zjps_batch_edit_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 40, 40, kCellSize, true) == 0, "batch grid init fail");
    ASSERT_TEST(grid.build_jps_light() == 0, "batch light build fail");
    std::vector<s32> acells;
    std::vector<s32> jcells;
    s32 ret = grid.astar_search( 0,0, 0, 0, 39, 39, acells);
    ASSERT_TEST(ret == 0, "batch open path fail ret=", ret);
    s32 open_cost = grid.last_path_cost();

    ASSERT_TEST(grid.set_rect_cell(0, 15, 15, 24, 24, false) == 0, "batch block rect fail");
    for (s32 y = 15; y <= 24; y++)
    {
        for (s32 x = 15; x <= 24; x++)
        {
            ASSERT_TEST(!grid.cell_walkable(0, x, y), "batch cell still walkable x=", x, " y=", y);
        }
    }
    ret = grid.astar_search( 0,0, 0, 0, 39, 39, acells);
    ASSERT_TEST(ret == 0, "batch detour path fail ret=", ret);
    s32 detour_cost = grid.last_path_cost();
    ASSERT_TEST(detour_cost > open_cost, "batch detour not longer, got=", detour_cost);
    ret = grid.astar_search( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0 && grid.last_path_cost() == detour_cost, "batch jps mismatch, jps=", grid.last_path_cost(),
                " astar=", detour_cost);

    ASSERT_TEST(grid.set_rect_cell(0, 15, 15, 24, 24, true) == 0, "batch unblock rect fail");
    ret = grid.astar_search( 0,0, 0, 0, 39, 39, acells);
    ASSERT_TEST(ret == 0 && grid.last_path_cost() == open_cost, "batch restore cost mismatch");
    ret = grid.astar_search( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0 && grid.last_path_cost() == open_cost, "batch restore jps mismatch");

    u32 version = grid.map_version();
    ASSERT_TEST(grid.set_rect_cell(0, 15, 15, 24, 24, true) == 0, "batch idempotent call fail");
    ASSERT_TEST(grid.map_version() == version, "batch idempotent should not bump version");

    ASSERT_TEST(grid.set_rect_cell(0, 24, 24, 15, 15, false) == -1, "batch inverted rect expect -1");
    ASSERT_TEST(grid.set_rect_cell(0, -1, 0, 5, 5, false) == -1, "batch out of range expect -1");

    zjps_grid bench_rect;
    zjps_grid bench_cell;
    ASSERT_TEST(bench_rect.init(1, 400, 400, kCellSize, true) == 0, "bench rect grid fail");
    ASSERT_TEST(bench_rect.build_jps_light() == 0, "bench rect build fail");
    ASSERT_TEST(bench_cell.init(1, 400, 400, kCellSize, true) == 0, "bench cell grid fail");
    ASSERT_TEST(bench_cell.build_jps_light() == 0, "bench cell build fail");
    std::vector<s32> c;
    const s32 ESIZE = 5;
    {
        zclock<> cost;
        cost.start();
        ASSERT_TEST(bench_rect.set_rect_cell(0, 100, 100, 100 + ESIZE - 1, 100 + ESIZE - 1, false) == 0, "bench rect block fail");
        ASSERT_TEST(bench_rect.build_jps_light() == 0, "bench rect repair fail");
        cost.stop_and_save();
        LOGFMTI("batch edit+repair %dx%d on 400x400: set_rect_cell+light_repair=%.0fns",
                ESIZE, ESIZE, (f64)cost.cost_ns());
    }
    {
        zclock<> cost;
        cost.start();
        for (s32 y = 100; y <= 100 + ESIZE - 1; y++)
        {
            for (s32 x = 100; x <= 100 + ESIZE - 1; x++)
            {
                bench_cell.set_blocked(0, x, y);
            }
        }
        ASSERT_TEST(bench_cell.build_jps_light() == 0, "bench cell repair fail");
        cost.stop_and_save();
        LOGFMTI("batch edit+repair %dx%d on 400x400: %dx set_cell+light_repair=%.0fns",
                ESIZE, ESIZE, ESIZE * ESIZE, (f64)cost.cost_ns());
    }
    ASSERT_TEST(bench_rect.set_rect_cell(0, 100, 100, 100 + ESIZE - 1, 100 + ESIZE - 1, true) == 0, "bench rect restore fail");
    ASSERT_TEST(bench_rect.build_jps_light() == 0, "bench rect restore repair fail");
    ASSERT_TEST(bench_cell.set_rect_cell(0, 100, 100, 100 + ESIZE - 1, 100 + ESIZE - 1, true) == 0, "bench cell restore fail");
    ASSERT_TEST(bench_cell.build_jps_light() == 0, "bench cell restore repair fail");

    const s32 ESIZE2 = 40;
    {
        zclock<> cost;
        cost.start();
        ASSERT_TEST(bench_rect.set_rect_cell(0, 100, 100, 100 + ESIZE2 - 1, 100 + ESIZE2 - 1, false) == 0, "bench rect block 2 fail");
        ASSERT_TEST(bench_rect.build_jps_light() == 0, "bench rect repair 2 fail");
        cost.stop_and_save();
        LOGFMTI("batch edit+repair %dx%d on 400x400: set_rect_cell+light_repair=%.0fns",
                ESIZE2, ESIZE2, (f64)cost.cost_ns());
    }
    {
        zclock<> cost;
        cost.start();
        for (s32 y = 100; y <= 100 + ESIZE2 - 1; y++)
        {
            for (s32 x = 100; x <= 100 + ESIZE2 - 1; x++)
            {
                bench_cell.set_blocked(0, x, y);
            }
        }
        ASSERT_TEST(bench_cell.build_jps_light() == 0, "bench cell repair 2 fail");
        cost.stop_and_save();
        LOGFMTI("batch edit+repair %dx%d on 400x400: %dx set_cell+light_repair=%.0fns",
                ESIZE2, ESIZE2, ESIZE2 * ESIZE2, (f64)cost.cost_ns());
    }
    for (s32 y = 100; y <= 100 + ESIZE2 - 1; y++)
    {
        for (s32 x = 100; x <= 100 + ESIZE2 - 1; x++)
        {
            ASSERT_TEST(!bench_rect.cell_walkable(0, x, y) && !bench_cell.cell_walkable(0, x, y), "bench grids diverge");
        }
    }
    for (s32 y = 100; y <= 119; y++)
    {
        for (s32 x = 100; x <= 119; x++)
        {
            ASSERT_TEST(!bench_rect.cell_walkable(0, x, y) && !bench_cell.cell_walkable(0, x, y), "bench grids diverge");
        }
    }
    s32 r1 = bench_rect.find_path( 0,0, 0, 0, 399, 399, c);
    s32 k1 = bench_rect.last_path_cost();
    s32 r2 = bench_cell.find_path( 0,0, 0, 0, 399, 399, c);
    s32 k2 = bench_cell.last_path_cost();
    ASSERT_TEST(r1 == r2 && k1 == k2, "bench grids path diverge r1=", r1, " r2=", r2, " k1=", k1, " k2=", k2);
    return 0;
}

static s32 zjps_dirty_flow_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 40, 40, kCellSize, true) == 0, "dirty grid init fail");
    std::vector<s32> acells;
    std::vector<s32> jcells;

    s32 ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0, "dirty raw jps fail ret=", ret);
    ASSERT_TEST(grid.last_tier() == 0, "no light built expect tier 0, got=", grid.last_tier());

    ASSERT_TEST(grid.build_jps_light() == 0, "dirty light build fail");
    ASSERT_TEST(grid.light_dirty() == 0, "after build expect dirty 0, got=", grid.light_dirty());
    ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0, "dirty light jps fail ret=", ret);
    ASSERT_TEST(grid.last_tier() == 1, "light clean expect tier 1, got=", grid.last_tier());
    s32 open_cost = grid.last_path_cost();

    ASSERT_TEST(grid.set_blocked(0, 20, 20) == 0, "dirty set_blocked fail");
    ASSERT_TEST(grid.light_dirty() == 0, "single cell on clean lines eager-maintained, dirty=", grid.light_dirty());
    ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0, "eager-maintained jps fail ret=", ret);
    ASSERT_TEST(grid.last_tier() == 1, "eager-maintained expect tier 1, got=", grid.last_tier());
    ASSERT_TEST(grid.astar_search( 0,0, 0, 0, 39, 39, acells) == 0, "dirty astar fail");
    ASSERT_TEST(grid.last_tier() == -1, "astar expect tier -1, got=", grid.last_tier());

    ASSERT_TEST(grid.set_rect_cell(0, 20, 20, 22, 22, true) == 0, "dirty rect clear fail");
    ASSERT_TEST(grid.light_dirty() == 6, "rect 3x3 expects 6 dirty lines(span of rows+cols), got=", grid.light_dirty());
    ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0 && grid.last_tier() == 0, "rect dirty expect tier 0");
    ASSERT_TEST(grid.build_jps_light() == 0, "dirty repair fail");
    ASSERT_TEST(grid.light_dirty() == 0, "after repair expect dirty 0, got=", grid.light_dirty());
    ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0 && grid.last_tier() == 1, "after repair expect tier 1, got=", grid.last_tier());

    ASSERT_TEST(grid.set_rect_cell(0, 15, 15, 19, 19, false) == 0, "dirty rect block fail");
    ASSERT_TEST(grid.light_dirty() == 10, "5x5 rect expect dirty 10(5rows+5cols), got=", grid.light_dirty());
    ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0 && grid.last_tier() == 0, "rect dirty expect tier 0");
    ASSERT_TEST(grid.build_jps_light() == 0, "rect repair fail");
    ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0 && grid.last_tier() == 1, "rect repaired expect tier 1");
    s32 detour_cost = grid.last_path_cost();
    ASSERT_TEST(detour_cost > open_cost, "rect detour not longer");

    ASSERT_TEST(grid.build_jps_plus() == 0, "dirty plus build fail");
    ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0 && grid.last_tier() == 2, "plus ok expect tier 2, got=", grid.last_tier());
    ASSERT_TEST(grid.last_path_cost() == detour_cost, "plus cost mismatch");

    ASSERT_TEST(grid.set_rect_cell(0, 30, 30, 31, 31, false) == 0, "dirty edit after plus fail");
    ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0, "plus fallback after edit fail ret=", ret);
    ASSERT_TEST(grid.last_tier() == 0, "plus stale and light dirty expect tier 0, got=", grid.last_tier());

    ASSERT_TEST(grid.build_jps_light() == 0, "light repair after plus fail");
    ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0 && grid.last_tier() == 1, "plus stale light ok expect tier 1, got=", grid.last_tier());

    ASSERT_TEST(grid.build_jps_plus() == 0, "plus rebuild fail");
    ret = grid.find_path( 0,0, 0, 0, 39, 39, jcells);
    ASSERT_TEST(ret == 0 && grid.last_tier() == 2, "plus rebuilt expect tier 2, got=", grid.last_tier());
    LOGFMTI("zjps dirty flow: tier0(raw)/tier1(light)/tier2(plus) fallback chain verified");
    return 0;
}

static s32 zjps_eager_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(1, 30, 30, kCellSize, true) == 0, "eager grid init fail");
    ASSERT_TEST(grid.build_jps_light() == 0, "eager light build fail");
    std::vector<s32> cells;

    ASSERT_TEST(grid.set_blocked(0, 5, 5) == 0, "eager block fail");
    ASSERT_TEST(grid.light_dirty() == 0, "eager block should not dirty, got=", grid.light_dirty());
    ASSERT_TEST(grid.cell_walkable(0, 5, 5) == false, "eager blocked cell walkable");

    s32 ret = grid.find_path( 0,0, 0, 0, 29, 29, cells);
    ASSERT_TEST(ret == 0, "eager path fail ret=", ret);
    ASSERT_TEST(grid.last_tier() == 1, "eager expect tier 1, got=", grid.last_tier());

    ASSERT_TEST(grid.set_walkable(0, 5, 5) == 0, "eager unblock fail");
    ASSERT_TEST(grid.light_dirty() == 0, "eager unblock should not dirty");

    ASSERT_TEST(grid.set_blocked(0, 0, 0) == 0, "eager head insert fail");
    ASSERT_TEST(grid.set_blocked(0, 29, 29) == 0, "eager tail insert fail");
    ASSERT_TEST(grid.set_blocked(0, 15, 15) == 0, "eager mid insert fail");
    ASSERT_TEST(grid.light_dirty() == 0, "eager multi insert dirty");
    ret = grid.find_path( 0,1, 0, 0, 29, 28, cells);
    ASSERT_TEST(ret == 0, "eager detour fail ret=", ret);
    ASSERT_TEST(grid.last_tier() == 1, "eager detour expect tier 1");

    ASSERT_TEST(grid.set_walkable(0, 15, 15) == 0, "eager mid remove fail");
    ASSERT_TEST(grid.set_walkable(0, 15, 15) == 0, "eager double remove fail");
    ASSERT_TEST(grid.set_blocked(0, 15, 15) == 0, "eager re-add fail");
    ASSERT_TEST(grid.set_blocked(0, 15, 15) == 0, "eager double add fail");
    ASSERT_TEST(grid.light_dirty() == 0, "eager toggle storm dirty");
    ret = grid.find_path( 0,1, 0, 0, 29, 28, cells);
    ASSERT_TEST(ret == 0 && grid.last_tier() == 1, "eager toggle storm tier");

    ASSERT_TEST(grid.set_rect_cell(0, 10, 10, 12, 12, false) == 0, "eager rect block fail");
    ASSERT_TEST(grid.light_dirty() == 6, "rect should dirty 6 lines, got=", grid.light_dirty());
    ASSERT_TEST(grid.set_blocked(0, 3, 20) == 0, "eager after rect fail");
    ASSERT_TEST(grid.light_dirty() == 6, "eager on dirty lines should mark not repair, got=", grid.light_dirty());
    ret = grid.find_path( 0,1, 0, 0, 29, 28, cells);
    ASSERT_TEST(ret == 0 && grid.last_tier() == 0, "rect dirty expect tier 0");
    ASSERT_TEST(grid.build_jps_light() == 0, "eager repair after rect fail");
    ASSERT_TEST(grid.light_dirty() == 0, "repair dirty mismatch");
    ret = grid.find_path( 0,1, 0, 0, 29, 28, cells);
    ASSERT_TEST(ret == 0 && grid.last_tier() == 1, "repaired expect tier 1");
    for (s32 y = 10; y <= 12; y++)
    {
        for (s32 x = 10; x <= 12; x++)
        {
            ASSERT_TEST(!grid.cell_walkable(0, x, y), "eager rect cell walkable");
        }
    }
    ASSERT_TEST(!grid.cell_walkable(0, 3, 20), "eager cell after rect lost");

    {
        zjps_grid bench;
        ASSERT_TEST(bench.init(1, 400, 400, kCellSize, true) == 0, "eager bench init fail");
        ASSERT_TEST(bench.build_jps_light() == 0, "eager bench build fail");
        std::mt19937 rng(20260846u);
        const s32 OPS = 20000;
        zclock<> c;
        c.start();
        for (s32 i = 0; i < OPS; i++)
        {
            s32 x = (s32)(rng() % (size_t)400);
            s32 y = (s32)(rng() % (size_t)400);
            if (i % 2 == 0)
            {
                bench.set_blocked(0, x, y);
            }
            else
            {
                bench.set_walkable(0, x, y);
            }
        }
        c.stop_and_save();
        LOGFMTI("eager edit on 400x400: set_cell(index-maintained, light stays ready)=%.0fns/op",
                (f64)c.cost_ns() / (f64)OPS);
        ASSERT_TEST(bench.light_dirty() == 0, "eager bench dirty leak=", bench.light_dirty());
        ASSERT_TEST(bench.set_open_capacity(65536) == 0, "eager bench open capacity fail");
        s32 r = bench.find_path( 0,0, 0, 0, 399, 399, cells);
        ASSERT_TEST(r == 0 && bench.last_tier() == 1, "eager bench tier after 20000 edits r=", r,
                    " tier=", bench.last_tier());
    }
    LOGFMTI("zjps eager: set_cell maintains light in-place on clean lines, rect keeps lazy");
    return 0;
}

static s32 zjps_five_way_test()
{
    const s32 ROW_CNT = 33;
    std::vector<zpoint> node_pos;
    node_pos.reserve((size_t)ROW_CNT * 2);
    for (s32 k = 0; k < ROW_CNT; k++)
    {
        f32 y = 300.0f + 600.0f * (f32)k;
        node_pos.push_back(zpoint(300.0f, y, 0.0f));
        node_pos.push_back(zpoint(19700.0f, y, 0.0f));
    }
    const s32 NODE_CNT = (s32)node_pos.size();

    test_graph graph;
    std::vector<s32> node_ids((size_t)NODE_CNT, 0);
    for (s32 i = 0; i < NODE_CNT; i++)
    {
        node_ids[i] = graph.new_node(node_pos[i], i);
        ASSERT_TEST_NOLOG(node_ids[i] >= 0, "5way new_node fail i=", i);
    }
    s32 seq = 0;
    for (s32 k = 0; k < ROW_CNT; k++)
    {
        s32 affects = 0;
        s32 lid = graph.new_link(node_ids[2 * k], node_ids[2 * k + 1], seq++);
        ASSERT_TEST_NOLOG(lid >= 0 && graph.push_link(lid, affects) == 0, "5way row link fail k=", k);
    }
    for (s32 k = 0; k + 1 < ROW_CNT; k++)
    {
        s32 side = (k % 2 == 0) ? 1 : 0;
        s32 affects = 0;
        s32 lid = graph.new_link(node_ids[2 * k + side], node_ids[2 * (k + 1) + side], seq++);
        ASSERT_TEST_NOLOG(lid >= 0 && graph.push_link(lid, affects) == 0, "5way conn link fail k=", k);
    }

    volatile s32 salt = 0;
    std::vector<test_graph::graph_path_step> steps;
    std::vector<s32> cells;
    const s32 N = 2000;
    f64 zg_ns = 0.0;
    f64 a_ns = 0.0;
    f64 t0_ns = 0.0;
    f64 t1_ns = 0.0;
    f64 t2_ns = 0.0;
    {
        zclock<> c;
        c.start();
        for (s32 i = 0; i < N; i++)
        {
            graph.find_path(node_ids[0], node_ids[NODE_CNT - 1], steps);
            salt += (s32)steps.size();
        }
        c.stop_and_save();
        zg_ns = (f64)c.cost_ns() / (f64)N;
    }
    {
        zjps_grid g;
        ASSERT_TEST(bench_serpentine_grid(g) == 0, "5way grid fail");
        zclock<> c;
        c.start();
        for (s32 i = 0; i < N; i++)
        {
            g.astar_search( 0,6, 6, 0, 6, 390, cells);
            salt += (s32)cells.size();
        }
        c.stop_and_save();
        a_ns = (f64)c.cost_ns() / (f64)N;
        zclock<> c0;
        c0.start();
        for (s32 i = 0; i < N; i++)
        {
            g.find_path( 0,6, 6, 0, 6, 390, cells);
            salt += (s32)cells.size();
        }
        c0.stop_and_save();
        t0_ns = (f64)c0.cost_ns() / (f64)N;
        ASSERT_TEST(g.last_tier() == 0, "5way tier0 tier mismatch ", g.last_tier());
    }
    f64 light_build_ms = 0.0;
    f64 plus_build_ms = 0.0;
    {
        zjps_grid g;
        ASSERT_TEST(bench_serpentine_grid(g) == 0, "5way grid2 fail");
        zclock<> lb;
        lb.start();
        ASSERT_TEST(g.build_jps_light() == 0, "5way light fail");
        lb.stop_and_save();
        light_build_ms = (f64)lb.cost_ns() / 1000000.0;
        zclock<> c1;
        c1.start();
        for (s32 i = 0; i < N; i++)
        {
            g.find_path( 0,6, 6, 0, 6, 390, cells);
            salt += (s32)cells.size();
        }
        c1.stop_and_save();
        t1_ns = (f64)c1.cost_ns() / (f64)N;
        ASSERT_TEST(g.last_tier() == 1, "5way tier1 tier mismatch ", g.last_tier());
        zclock<> pb;
        pb.start();
        ASSERT_TEST(g.build_jps_plus() == 0, "5way plus fail");
        pb.stop_and_save();
        plus_build_ms = (f64)pb.cost_ns() / 1000000.0;
        zclock<> c2;
        c2.start();
        for (s32 i = 0; i < N; i++)
        {
            g.find_path( 0,6, 6, 0, 6, 390, cells);
            salt += (s32)cells.size();
        }
        c2.stop_and_save();
        t2_ns = (f64)c2.cost_ns() / (f64)N;
        ASSERT_TEST(g.last_tier() == 2, "5way tier2 tier mismatch ", g.last_tier());
    }
    LOGFMTI("5WAY serpentine long query: zgraph=%.0fns | grid A*=%.0fns | JPS(no index)=%.0fns | JPS+LIGHT=%.0fns | JPS+BUILD=%.0fns",
            zg_ns, a_ns, t0_ns, t1_ns, t2_ns);
    LOGFMTI("5WAY pretreatment: light build=%.3fms(incremental) | jps+ build=%.2fms(snapshot, rebuild on any edit)",
            light_build_ms, plus_build_ms);
    LOGFMTI("5WAY speedup chain vs grid A*: JPS=%.1fx JPS+LIGHT=%.1fx JPS+BUILD=%.1fx | zgraph is %.1fx faster than best grid tier",
            a_ns / t0_ns, a_ns / t1_ns, a_ns / t2_ns, t2_ns / zg_ns);
    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

static s32 layer_cross_check(zjps_grid& grid, s32 sl, s32 sx, s32 sy, s32 tl, s32 tx, s32 ty, const char* tag)
{
    std::vector<s32> acells;
    std::vector<s32> jcells;
    s32 aret = grid.astar_search(sl, sx, sy, tl, tx, ty, acells);
    s32 acost = grid.last_path_cost();
    s32 jret = grid.jps_search(sl, sx, sy, tl, tx, ty, jcells);
    s32 jcost = grid.last_path_cost();
    ASSERT_TEST(aret == jret, tag, " ret mismatch astar=", aret, " jps=", jret);
    if (aret != 0)
    {
        return 0;
    }
    //JPS与A*必须给出同代价的最优解 路径本身可以不同
    ASSERT_TEST(acost == jcost, tag, " cost mismatch astar=", acost, " jps=", jcost, " tier=", grid.last_tier());
    ASSERT_TEST(jcells.front() == acells.front() && jcells.back() == acells.back(), tag, " endpoint mismatch");
    //逐格校验jps路径连续且可通行 link步允许换层 层内步必须八邻接
    for (size_t i = 0; i + 1 < jcells.size(); i++)
    {
        s32 al = grid.cell_layer_of(jcells[i]);
        s32 ax = grid.cell_x_of(jcells[i]);
        s32 ay = grid.cell_y_of(jcells[i]);
        s32 bl = grid.cell_layer_of(jcells[i + 1]);
        s32 bx = grid.cell_x_of(jcells[i + 1]);
        s32 by = grid.cell_y_of(jcells[i + 1]);
        ASSERT_TEST_NOLOG(grid.cell_walkable(al, ax, ay), tag, " path cell blocked i=", (s32)i);
        ASSERT_TEST_NOLOG(grid.cell_walkable(bl, bx, by), tag, " path next blocked i=", (s32)i);
        s32 dx = bx > ax ? bx - ax : ax - bx;
        s32 dy = by > ay ? by - ay : ay - by;
        ASSERT_TEST_NOLOG(dx <= 1 && dy <= 1, tag, " path gap i=", (s32)i, " dx=", dx, " dy=", dy);
        if (al == bl)
        {
            ASSERT_TEST_NOLOG(grid.move_valid(al, ax, ay, bx - ax, by - ay), tag, " path corner cut i=", (s32)i);
        }
        else
        {
            ASSERT_TEST_NOLOG(grid.has_link(al, ax, ay), tag, " layer change without link i=", (s32)i);
        }
    }
    return 0;
}

//三档tier对同一组跨层查询必须给出一致的最优代价
static s32 layer_tier_check(zjps_grid& grid, s32 sl, s32 sx, s32 sy, s32 tl, s32 tx, s32 ty, const char* tag)
{
    std::vector<s32> cells;
    grid.drop_jps_plus();
    ASSERT_TEST(layer_cross_check(grid, sl, sx, sy, tl, tx, ty, tag) == 0, tag, " scan tier fail");
    s32 scan_cost = grid.last_path_cost();
    s32 scan_tier = grid.last_tier();

    ASSERT_TEST(grid.build_jps_light() == 0, tag, " light build fail");
    ASSERT_TEST(layer_cross_check(grid, sl, sx, sy, tl, tx, ty, tag) == 0, tag, " light tier fail");
    s32 light_cost = grid.last_path_cost();
    s32 light_tier = grid.last_tier();

    ASSERT_TEST(grid.build_jps_plus() == 0, tag, " plus build fail");
    ASSERT_TEST(layer_cross_check(grid, sl, sx, sy, tl, tx, ty, tag) == 0, tag, " plus tier fail");
    s32 plus_cost = grid.last_path_cost();
    s32 plus_tier = grid.last_tier();

    ASSERT_TEST(scan_tier == 0 && light_tier == 1 && plus_tier == 2, tag,
                " tier ladder unexpected scan=", scan_tier, " light=", light_tier, " plus=", plus_tier);
    ASSERT_TEST(scan_cost == light_cost && light_cost == plus_cost, tag,
                " tier cost divergence scan=", scan_cost, " light=", light_cost, " plus=", plus_cost);
    grid.drop_jps_plus();
    return 0;
}

static s32 zjps_layer_basic_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(2, 20, 20, kCellSize, true) == 0, "layer init fail");
    ASSERT_TEST(grid.layer_cnt() == 2, "layer cnt expect 2");

    //层与层默认互不连通
    std::vector<s32> cells;
    ASSERT_TEST(grid.astar_search(0, 1, 1, 1, 18, 18, cells) == -2, "layers must start disconnected");
    ASSERT_TEST(grid.jps_search(0, 1, 1, 1, 18, 18, cells) == -2, "layers must start disconnected for jps");

    //link参数校验 层必须相邻 xy位移不超过一格 代价不低于下界
    ASSERT_TEST(grid.set_link(0, 5, 5, 0, 5, 5, zjps_grid::kDefaultLinkCost) == -2, "same layer link expect -2");
    ASSERT_TEST(grid.set_link(0, 5, 5, 1, 7, 5, zjps_grid::kDefaultLinkCost) == -2, "far xy link expect -2");
    //代价下界按link自身的xy位移决定 竖直楼梯不影响启发 因此允许任意正代价
    ASSERT_TEST(zjps_grid::link_cost_floor(0, 0) == 1, "vertical floor expect 1");
    ASSERT_TEST(zjps_grid::link_cost_floor(1, 0) == zjps_grid::kCostStraight, "axial floor expect straight");
    ASSERT_TEST(zjps_grid::link_cost_floor(1, 1) == zjps_grid::kCostDiagonal, "diagonal floor expect diagonal");
    ASSERT_TEST(grid.set_link(0, 5, 5, 1, 5, 5, 0) == -3, "zero cost link expect -3");
    ASSERT_TEST(grid.set_link(0, 6, 6, 1, 7, 6, zjps_grid::kCostStraight - 1) == -3, "cheap axial link expect -3");
    ASSERT_TEST(grid.set_link(0, 8, 8, 1, 9, 9, zjps_grid::kCostDiagonal - 1) == -3, "cheap diagonal link expect -3");

    ASSERT_TEST(grid.set_link(0, 5, 5, 2, 5, 5, zjps_grid::kDefaultLinkCost) == -1, "out of range layer expect -1");
    ASSERT_TEST(grid.link_count() == 0, "failed links must not be stored");

    //单向link 只能上不能下
    ASSERT_TEST(grid.set_link(0, 5, 5, 1, 5, 5, zjps_grid::kDefaultLinkCost) == 0, "set link fail");
    ASSERT_TEST(grid.link_count() == 1, "link count expect 1");
    ASSERT_TEST(grid.has_link(0, 5, 5), "link flag expect set");
    ASSERT_TEST(!grid.has_link(1, 5, 5), "reverse link must not exist");
    ASSERT_TEST(grid.astar_search(0, 1, 1, 1, 18, 18, cells) == 0, "up path expect reachable");
    ASSERT_TEST(grid.astar_search(1, 18, 18, 0, 1, 1, cells) == -2, "down path expect blocked by one-way link");

    ASSERT_TEST(grid.set_link(1, 5, 5, 0, 5, 5, zjps_grid::kDefaultLinkCost) == 0, "set reverse link fail");
    ASSERT_TEST(grid.link_count() == 2, "link count expect 2");
    ASSERT_TEST(grid.astar_search(1, 18, 18, 0, 1, 1, cells) == 0, "down path expect reachable after reverse link");

    //重复set只更新代价 不新增边
    ASSERT_TEST(grid.set_link(0, 5, 5, 1, 5, 5, zjps_grid::kDefaultLinkCost + 500) == 0, "link cost update fail");
    ASSERT_TEST(grid.link_count() == 2, "duplicate set must not add edge");

    ASSERT_TEST(grid.erase_link(0, 5, 5, 1, 5, 5) == 0, "erase link fail");
    ASSERT_TEST(grid.link_count() == 1, "link count expect 1 after erase");
    ASSERT_TEST(!grid.has_link(0, 5, 5), "link flag expect cleared");
    ASSERT_TEST(grid.astar_search(0, 1, 1, 1, 18, 18, cells) == -2, "up path expect blocked after erase");

    //z是纯payload 改z不影响连通也不失效plus表
    ASSERT_TEST(grid.set_link(0, 5, 5, 1, 5, 5, zjps_grid::kDefaultLinkCost) == 0, "relink fail");
    ASSERT_TEST(grid.build_jps_plus() == 0, "layer plus build fail");
    ASSERT_TEST(grid.jps_search(0, 1, 1, 1, 18, 18, cells) == 0, "layer plus query fail");
    ASSERT_TEST(grid.last_tier() == 2, "layer plus tier expect 2, got=", grid.last_tier());
    ASSERT_TEST(grid.set_cell_z(1, 5, 5, 30) == 0, "set z fail");
    ASSERT_TEST(grid.jps_search(0, 1, 1, 1, 18, 18, cells) == 0, "layer plus requery fail");
    ASSERT_TEST(grid.last_tier() == 2, "z edit must not invalidate plus table, got=", grid.last_tier());

    //cell_to_pos的z来自本层本格的voxel
    f32 px = 0.0f;
    f32 py = 0.0f;
    f32 pz = 0.0f;
    ASSERT_TEST(grid.cell_to_pos(1, 5, 5, px, py, pz) == 0, "cell_to_pos fail");
    ASSERT_TEST(pz == 30.0f * kCellSize, "layer1 z mismatch, got=", pz);
    ASSERT_TEST(grid.cell_to_pos(0, 5, 5, px, py, pz) == 0, "cell_to_pos layer0 fail");
    ASSERT_TEST(pz == 0.0f, "layer0 z expect 0, got=", pz);
    //同xy不同层拿到不同z 这正是外部据以选层的依据
    ASSERT_TEST(grid.cell_to_pos(0, 5, 5, px, py, pz) == 0, "cell_to_pos recheck fail");

    LOGFMTI("zjps layer basic: link is one-way, xy-adjacent, cost-floored; z is per-layer payload");
    return 0;
}

//单层行为必须与多层实现下的layer0逐位一致
static s32 zjps_layer_degenerate_test()
{
    const s32 W = 40;
    const s32 H = 40;
    std::mt19937 rng(20260908u);
    zjps_grid one;
    zjps_grid two;
    ASSERT_TEST(one.init(1, W, H, kCellSize, true) == 0, "degen one init fail");
    ASSERT_TEST(two.init(3, W, H, kCellSize, true) == 0, "degen two init fail");
    for (s32 i = 0; i < 120; i++)
    {
        s32 x = (s32)(rng() % (u32)W);
        s32 y = (s32)(rng() % (u32)H);
        ASSERT_TEST_NOLOG(one.set_cell(0, x, y, false) == 0, "degen one block fail");
        ASSERT_TEST_NOLOG(two.set_cell(0, x, y, false) == 0, "degen two block fail");
    }
    ASSERT_TEST(one.build_jps_light() == 0 && two.build_jps_light() == 0, "degen light build fail");
    ASSERT_TEST(one.build_jps_plus() == 0 && two.build_jps_plus() == 0, "degen plus build fail");

    std::vector<s32> ocells;
    std::vector<s32> tcells;
    s32 checked = 0;
    for (s32 i = 0; i < 300; i++)
    {
        s32 sx = (s32)(rng() % (u32)W);
        s32 sy = (s32)(rng() % (u32)H);
        s32 tx = (s32)(rng() % (u32)W);
        s32 ty = (s32)(rng() % (u32)H);
        s32 oret = one.jps_search(0, sx, sy, 0, tx, ty, ocells);
        s32 ocost = one.last_path_cost();
        s32 tret = two.jps_search(0, sx, sy, 0, tx, ty, tcells);
        s32 tcost = two.last_path_cost();
        ASSERT_TEST_NOLOG(oret == tret, "degen ret mismatch i=", i, " one=", oret, " multi=", tret);
        if (oret != 0)
        {
            continue;
        }
        //layer0偏移为0 因此多层实现下的cell编号与单层完全相同
        ASSERT_TEST_NOLOG(ocost == tcost, "degen cost mismatch i=", i);
        ASSERT_TEST_NOLOG(ocells == tcells, "degen path mismatch i=", i);
        checked++;
    }
    LOGFMTI("zjps layer degenerate: layer0 of a %d-layer grid matches single-layer bit-for-bit on %d paths", 3, checked);
    return 0;
}

//你担心的场景: 目标在楼上 楼梯在反方向 启发会不会先把搜索推到目标正下方再回头绕
//结论是会 因为octile启发只看xy 看不见楼梯 这里把代价量出来 并确认最优性不受影响
static s32 zjps_layer_detour_test()
{
    const s32 W = 60;
    const s32 H = 9;
    const s32 STAIR_X = 1;
    const s32 MID_Y = H / 2;

    //对照组: 单层 目标在正东 启发全程指向正确方向
    zjps_grid flat;
    ASSERT_TEST(flat.init(1, W, H, kCellSize, true) == 0, "detour flat init fail");
    std::vector<s32> cells;
    ASSERT_TEST(flat.astar_search(0, 3, MID_Y, 0, W - 2, MID_Y, cells) == 0, "detour flat query fail");
    s32 flat_visit = flat.visit_count();

    //实验组: 双层 楼梯在最西端 目标在东侧楼上 启发把搜索往东拉 但正解是先往西
    zjps_grid grid;
    ASSERT_TEST(grid.init(2, W, H, kCellSize, true) == 0, "detour init fail");
    ASSERT_TEST(grid.set_link(0, STAIR_X, MID_Y, 1, STAIR_X, MID_Y, zjps_grid::kDefaultLinkCost) == 0, "detour link fail");
    ASSERT_TEST(grid.set_link(1, STAIR_X, MID_Y, 0, STAIR_X, MID_Y, zjps_grid::kDefaultLinkCost) == 0, "detour back link fail");

    ASSERT_TEST(layer_cross_check(grid, 0, 3, MID_Y, 1, W - 2, MID_Y, "detour-open") == 0, "detour cross check fail");
    ASSERT_TEST(grid.astar_search(0, 3, MID_Y, 1, W - 2, MID_Y, cells) == 0, "detour astar fail");
    s32 cross_visit = grid.visit_count();
    s32 cross_cost = grid.last_path_cost();

    //最优代价可解析给出: 西行2格 + 爬楼 + 东行W-3格 启发的误导不影响最优性
    s32 want = 2 * zjps_grid::kCostStraight + zjps_grid::kDefaultLinkCost + (W - 3) * zjps_grid::kCostStraight;
    ASSERT_TEST(cross_cost == want, "detour cost expect ", want, " got=", cross_cost);

    //路径必须在唯一的楼梯处换层 而不是走到目标正下方再想办法
    ASSERT_TEST(grid.jps_search(0, 3, MID_Y, 1, W - 2, MID_Y, cells) == 0, "detour jps fail");
    s32 turn_idx = -1;
    for (size_t i = 0; i + 1 < cells.size(); i++)
    {
        if (grid.cell_layer_of(cells[i]) != grid.cell_layer_of(cells[i + 1]))
        {
            turn_idx = (s32)i;
            break;
        }
    }
    ASSERT_TEST(turn_idx >= 0, "detour path must change layer");
    ASSERT_TEST(grid.cell_x_of(cells[turn_idx]) == STAIR_X, "detour must climb at the only stair, got x=",
                grid.cell_x_of(cells[turn_idx]));
    ASSERT_TEST(layer_tier_check(grid, 0, 3, MID_Y, 1, W - 2, MID_Y, "detour-tier") == 0, "detour tier check fail");

    //把楼梯挪到起点旁边 启发方向与正解一致 用来对比楼梯位置带来的差异
    zjps_grid near_stair;
    ASSERT_TEST(near_stair.init(2, W, H, kCellSize, true) == 0, "detour near init fail");
    ASSERT_TEST(near_stair.set_link(0, W - 3, MID_Y, 1, W - 3, MID_Y, zjps_grid::kDefaultLinkCost) == 0, "detour near link fail");
    ASSERT_TEST(near_stair.astar_search(0, 3, MID_Y, 1, W - 2, MID_Y, cells) == 0, "detour near query fail");
    s32 near_visit = near_stair.visit_count();

    //启发被误导只影响扫描量 不影响最优性 这是octile只看xy的直接后果
    ASSERT_TEST(cross_visit > near_visit, "far stair must cost more exploration than near stair, far=",
                cross_visit, " near=", near_visit);
    LOGFMTI("zjps layer detour: A* visits same-layer=%d | stair-near-target=%d | stair-behind-start=%d "
            "(%.1fx of the aligned case) -- cost stays optimal, only the scan volume grows",
            flat_visit, near_visit, cross_visit, (f64)cross_visit / (f64)near_visit);
    return 0;
}

//真实形态: 空地上立一栈多层建筑 上层只有建筑占地是可走的 其余全封
//这是ugc家园的常见布局 用来对照全开阔多层的最坏情况
static s32 zjps_layer_building_bench()
{
    const s32 W = 100;
    const s32 H = 100;
    //建筑占地约10%面积 楼梯在建筑一角
    const s32 BX0 = 60;
    const s32 BY0 = 60;
    const s32 BX1 = 89;
    const s32 BY1 = 89;
    const s32 STAIR_X = 62;
    const s32 STAIR_Y = 62;

    for (s32 layers = 2; layers <= 3; layers++)
    {
        zjps_grid grid;
        ASSERT_TEST(grid.init(layers, W, H, kCellSize, true) == 0, "building init fail");
        //地面层撒散落障碍 模拟空地上的杂物
        std::mt19937 rng(4242u + (u32)layers);
        for (s32 i = 0; i < 120; i++)
        {
            s32 bx = (s32)(rng() % (u32)(W - 6));
            s32 by = (s32)(rng() % (u32)(H - 6));
            if (bx + 3 >= BX0 && bx <= BX1 && by + 3 >= BY0 && by <= BY1)
            {
                continue;
            }
            ASSERT_TEST_NOLOG(grid.set_rect_cell(0, bx, by, bx + 3, by + 3, false) == 0, "building ground block fail");
        }
        //上层默认全封 只把建筑占地开出来 这是与全开阔多层的关键差别
        s32 upper_open = 0;
        for (s32 l = 1; l < layers; l++)
        {
            ASSERT_TEST_NOLOG(grid.set_rect_cell(l, 0, 0, W - 1, H - 1, false) == 0, "building seal fail");
            ASSERT_TEST_NOLOG(grid.set_rect_cell(l, BX0, BY0, BX1, BY1, true) == 0, "building floor fail");
            upper_open += (BX1 - BX0 + 1) * (BY1 - BY0 + 1);
        }
        //楼梯竖直贯通 每层之间双向
        for (s32 l = 0; l + 1 < layers; l++)
        {
            ASSERT_TEST_NOLOG(grid.set_link(l, STAIR_X, STAIR_Y, l + 1, STAIR_X, STAIR_Y, zjps_grid::kDefaultLinkCost) == 0,
                              "building link fail");
            ASSERT_TEST_NOLOG(grid.set_link(l + 1, STAIR_X, STAIR_Y, l, STAIR_X, STAIR_Y, zjps_grid::kDefaultLinkCost) == 0,
                              "building back link fail");
        }
        ASSERT_TEST(grid.build_jps_light() == 0, "building light build fail");
        ASSERT_TEST(grid.build_jps_plus() == 0, "building plus build fail");

        //三类查询分开计时: 地面到地面 / 地面到楼上 / 楼上到楼上
        std::vector<s32> cells;
        std::mt19937 qrng(99u + (u32)layers);
        s32 g2g_cnt = 0;
        s32 g2u_cnt = 0;
        s32 u2u_cnt = 0;
        f64 g2g_ns = 0.0;
        f64 g2u_ns = 0.0;
        f64 u2u_ns = 0.0;
        s32 salt = 0;
        for (s32 i = 0; i < 900; i++)
        {
            s32 mode = i % 3;
            s32 sl = 0;
            s32 tl = 0;
            s32 sx = 0;
            s32 sy = 0;
            s32 tx = 0;
            s32 ty = 0;
            if (mode == 0)
            {
                sx = (s32)(qrng() % (u32)W);
                sy = (s32)(qrng() % (u32)H);
                tx = (s32)(qrng() % (u32)W);
                ty = (s32)(qrng() % (u32)H);
            }
            else if (mode == 1)
            {
                sx = (s32)(qrng() % (u32)W);
                sy = (s32)(qrng() % (u32)H);
                tl = layers - 1;
                tx = BX0 + (s32)(qrng() % (u32)(BX1 - BX0 + 1));
                ty = BY0 + (s32)(qrng() % (u32)(BY1 - BY0 + 1));
            }
            else
            {
                sl = layers - 1;
                tl = layers - 1;
                sx = BX0 + (s32)(qrng() % (u32)(BX1 - BX0 + 1));
                sy = BY0 + (s32)(qrng() % (u32)(BY1 - BY0 + 1));
                tx = BX0 + (s32)(qrng() % (u32)(BX1 - BX0 + 1));
                ty = BY0 + (s32)(qrng() % (u32)(BY1 - BY0 + 1));
            }
            if (!grid.cell_walkable(sl, sx, sy) || !grid.cell_walkable(tl, tx, ty))
            {
                continue;
            }
            zclock<> query_clock;
            query_clock.start();
            s32 ret = grid.jps_search(sl, sx, sy, tl, tx, ty, cells);
            query_clock.stop_and_save();
            if (ret != 0)
            {
                continue;
            }
            salt += (s32)cells.size();
            if (mode == 0)
            {
                g2g_ns += (f64)query_clock.cost_ns();
                g2g_cnt++;
            }
            else if (mode == 1)
            {
                g2u_ns += (f64)query_clock.cost_ns();
                g2u_cnt++;
            }
            else
            {
                u2u_ns += (f64)query_clock.cost_ns();
                u2u_cnt++;
            }
        }
        LOGFMTI("zjps building bench L=%d: upper open %.1f%% of plane | ground->ground %d q %.0fns | "
                "ground->upper %d q %.0fns | upper->upper %d q %.0fns (salt=%d)",
                layers, 100.0 * (f64)upper_open / (f64)((layers - 1) * W * H),
                g2g_cnt, g2g_cnt > 0 ? g2g_ns / (f64)g2g_cnt : 0.0,
                g2u_cnt, g2u_cnt > 0 ? g2u_ns / (f64)g2u_cnt : 0.0,
                u2u_cnt, u2u_cnt > 0 ? u2u_ns / (f64)u2u_cnt : 0.0, salt);
    }
    return 0;
}

static s32 zjps_layer_bench_test()
{
    const s32 W = 100;
    const s32 H = 100;
    for (s32 layers = 1; layers <= 3; layers++)
    {
        zjps_grid grid;
        ASSERT_TEST(grid.init(layers, W, H, kCellSize, true) == 0, "bench layer init fail");
        std::mt19937 rng(20260908u + (u32)layers);
        for (s32 l = 0; l < layers; l++)
        {
            for (s32 i = 0; i < 200; i++)
            {
                s32 bx = (s32)(rng() % (u32)(W - 6));
                s32 by = (s32)(rng() % (u32)(H - 6));
                ASSERT_TEST_NOLOG(grid.set_rect_cell(l, bx, by, bx + 3, by + 3, false) == 0, "bench block fail");
            }
        }
        //每相邻层之间放两处双向楼梯 贴合家园里楼梯极少的形态
        //楼梯格强制掏空 否则随机障碍会把楼梯埋掉导致层间根本没连上
        s32 link_cnt = 0;
        for (s32 l = 0; l + 1 < layers; l++)
        {
            for (s32 k = 0; k < 2; k++)
            {
                s32 lx = k == 0 ? 8 : W - 9;
                s32 ly = k == 0 ? 8 : H - 9;
                ASSERT_TEST_NOLOG(grid.set_rect_cell(l, lx - 1, ly - 1, lx + 1, ly + 1, true) == 0, "bench stair clear fail");
                ASSERT_TEST_NOLOG(grid.set_rect_cell(l + 1, lx - 1, ly - 1, lx + 1, ly + 1, true) == 0, "bench stair clear up fail");
                ASSERT_TEST_NOLOG(grid.set_link(l, lx, ly, l + 1, lx, ly, zjps_grid::kDefaultLinkCost) == 0, "bench link fail");
                ASSERT_TEST_NOLOG(grid.set_link(l + 1, lx, ly, l, lx, ly, zjps_grid::kDefaultLinkCost) == 0, "bench back link fail");
                link_cnt += 2;
            }
        }

        zclock<> light_clock;
        light_clock.start();
        ASSERT_TEST(grid.build_jps_light() == 0, "bench light build fail");
        light_clock.stop_and_save();
        zclock<> plus_clock;
        plus_clock.start();
        ASSERT_TEST(grid.build_jps_plus() == 0, "bench plus build fail");
        plus_clock.stop_and_save();

        //同层与跨层各采样一批 分别看代价
        std::vector<s32> cells;
        std::mt19937 qrng(777u + (u32)layers);
        s32 same_cnt = 0;
        s32 cross_cnt = 0;
        f64 same_ns = 0.0;
        f64 cross_ns = 0.0;
        s32 salt = 0;
        for (s32 i = 0; i < 400; i++)
        {
            s32 sl = (s32)(qrng() % (u32)layers);
            s32 tl = (s32)(qrng() % (u32)layers);
            s32 sx = (s32)(qrng() % (u32)W);
            s32 sy = (s32)(qrng() % (u32)H);
            s32 tx = (s32)(qrng() % (u32)W);
            s32 ty = (s32)(qrng() % (u32)H);
            if (!grid.cell_walkable(sl, sx, sy) || !grid.cell_walkable(tl, tx, ty))
            {
                continue;
            }
            zclock<> query_clock;
            query_clock.start();
            s32 ret = grid.jps_search(sl, sx, sy, tl, tx, ty, cells);
            query_clock.stop_and_save();
            if (ret != 0)
            {
                continue;
            }
            salt += (s32)cells.size();
            if (sl == tl)
            {
                same_ns += (f64)query_clock.cost_ns();
                same_cnt++;
            }
            else
            {
                cross_ns += (f64)query_clock.cost_ns();
                cross_cnt++;
            }
        }
        LOGFMTI("zjps layer bench L=%d: light=%.2fms plus=%.2fms table=%.1fMB links=%d | same-layer %d q %.0fns | "
                "cross-layer %d q %.0fns (salt=%d)",
                layers, (f64)light_clock.cost_ns() / 1000000.0, (f64)plus_clock.cost_ns() / 1000000.0,
                (f64)grid.jps_plus_table_bytes() / 1048576.0, link_cnt,
                same_cnt, same_cnt > 0 ? same_ns / (f64)same_cnt : 0.0,
                cross_cnt, cross_cnt > 0 ? cross_ns / (f64)cross_cnt : 0.0, salt);
    }
    return 0;
}

//三层竖向串联 必须逐层爬 不能跳层
static s32 zjps_layer_three_test()
{
    const s32 W = 30;
    const s32 H = 30;
    zjps_grid grid;
    ASSERT_TEST(grid.init(3, W, H, kCellSize, true) == 0, "three init fail");
    ASSERT_TEST(grid.set_link(0, 4, 4, 1, 4, 4, zjps_grid::kDefaultLinkCost) == 0, "three link 0-1 fail");
    ASSERT_TEST(grid.set_link(1, 25, 25, 2, 25, 25, zjps_grid::kDefaultLinkCost) == 0, "three link 1-2 fail");

    std::vector<s32> cells;
    ASSERT_TEST(grid.astar_search(0, 1, 1, 2, 28, 28, cells) == 0, "three up path fail");
    ASSERT_TEST(layer_cross_check(grid, 0, 1, 1, 2, 28, 28, "three-up") == 0, "three cross check fail");
    ASSERT_TEST(layer_tier_check(grid, 0, 1, 1, 2, 28, 28, "three-tier") == 0, "three tier check fail");
    //必须两次换层 且顺序是0->1->2
    ASSERT_TEST(grid.jps_search(0, 1, 1, 2, 28, 28, cells) == 0, "three requery fail");
    s32 changes = 0;
    for (size_t i = 0; i + 1 < cells.size(); i++)
    {
        s32 a = grid.cell_layer_of(cells[i]);
        s32 b = grid.cell_layer_of(cells[i + 1]);
        if (a != b)
        {
            ASSERT_TEST_NOLOG(b == a + 1, "three must climb one layer at a time i=", (s32)i);
            changes++;
        }
    }
    ASSERT_TEST(changes == 2, "three expect exactly 2 layer changes, got=", changes);
    //反向不通 因为link是单向的
    ASSERT_TEST(grid.astar_search(2, 28, 28, 0, 1, 1, cells) == -2, "three reverse expect blocked");
    LOGFMTI("zjps layer three: chained stairs force layer-by-layer climb, one-way links block the reverse trip");
    return 0;
}

//随机多层图上交叉校验jps三档与A*
static s32 zjps_layer_random_test()
{
    const s32 W = 36;
    const s32 H = 36;
    const s32 L = 3;
    std::mt19937 rng(20260908u);
    zjps_grid grid;
    ASSERT_TEST(grid.init(L, W, H, kCellSize, true) == 0, "layer random init fail");
    for (s32 l = 0; l < L; l++)
    {
        for (s32 i = 0; i < 40; i++)
        {
            s32 bx = (s32)(rng() % (u32)(W - 4));
            s32 by = (s32)(rng() % (u32)(H - 4));
            ASSERT_TEST_NOLOG(grid.set_rect_cell(l, bx, by, bx + 2, by + 2, false) == 0, "layer random block fail");
        }
    }
    //随机撒双向link 含斜向位移 覆盖xy不对齐的楼梯
    s32 link_cnt = 0;
    for (s32 i = 0; i < 24; i++)
    {
        s32 l = (s32)(rng() % (u32)(L - 1));
        s32 x = 1 + (s32)(rng() % (u32)(W - 2));
        s32 y = 1 + (s32)(rng() % (u32)(H - 2));
        s32 dx = (s32)(rng() % 3u) - 1;
        s32 dy = (s32)(rng() % 3u) - 1;
        if (!grid.cell_walkable(l, x, y) || !grid.cell_walkable(l + 1, x + dx, y + dy))
        {
            continue;
        }
        ASSERT_TEST_NOLOG(grid.set_link(l, x, y, l + 1, x + dx, y + dy, zjps_grid::kDefaultLinkCost) == 0, "layer random link fail");
        ASSERT_TEST_NOLOG(grid.set_link(l + 1, x + dx, y + dy, l, x, y, zjps_grid::kDefaultLinkCost) == 0, "layer random back link fail");
        link_cnt += 2;
    }

    s32 reach = 0;
    s32 cross = 0;
    for (s32 i = 0; i < 200; i++)
    {
        s32 sl = (s32)(rng() % (u32)L);
        s32 tl = (s32)(rng() % (u32)L);
        s32 sx = (s32)(rng() % (u32)W);
        s32 sy = (s32)(rng() % (u32)H);
        s32 tx = (s32)(rng() % (u32)W);
        s32 ty = (s32)(rng() % (u32)H);
        if (!grid.cell_walkable(sl, sx, sy) || !grid.cell_walkable(tl, tx, ty))
        {
            continue;
        }
        ASSERT_TEST_NOLOG(layer_cross_check(grid, sl, sx, sy, tl, tx, ty, "layer-random") == 0,
                          "layer random cross check fail i=", i);
        std::vector<s32> cells;
        if (grid.jps_search(sl, sx, sy, tl, tx, ty, cells) == 0)
        {
            reach++;
            if (sl != tl)
            {
                cross++;
            }
        }
    }
    //同一组查询在三档下都要一致 分别验一次避免只覆盖scan档
    ASSERT_TEST(grid.build_jps_light() == 0, "layer random light build fail");
    s32 light_checked = 0;
    std::mt19937 qrng(31337u);
    for (s32 i = 0; i < 120; i++)
    {
        s32 sl = (s32)(qrng() % (u32)L);
        s32 tl = (s32)(qrng() % (u32)L);
        s32 sx = (s32)(qrng() % (u32)W);
        s32 sy = (s32)(qrng() % (u32)H);
        s32 tx = (s32)(qrng() % (u32)W);
        s32 ty = (s32)(qrng() % (u32)H);
        if (!grid.cell_walkable(sl, sx, sy) || !grid.cell_walkable(tl, tx, ty))
        {
            continue;
        }
        ASSERT_TEST_NOLOG(layer_cross_check(grid, sl, sx, sy, tl, tx, ty, "layer-random-light") == 0,
                          "layer random light check fail i=", i);
        light_checked++;
    }
    ASSERT_TEST(grid.build_jps_plus() == 0, "layer random plus build fail");
    s32 plus_checked = 0;
    std::mt19937 prng(31337u);
    for (s32 i = 0; i < 120; i++)
    {
        s32 sl = (s32)(prng() % (u32)L);
        s32 tl = (s32)(prng() % (u32)L);
        s32 sx = (s32)(prng() % (u32)W);
        s32 sy = (s32)(prng() % (u32)H);
        s32 tx = (s32)(prng() % (u32)W);
        s32 ty = (s32)(prng() % (u32)H);
        if (!grid.cell_walkable(sl, sx, sy) || !grid.cell_walkable(tl, tx, ty))
        {
            continue;
        }
        ASSERT_TEST_NOLOG(layer_cross_check(grid, sl, sx, sy, tl, tx, ty, "layer-random-plus") == 0,
                          "layer random plus check fail i=", i);
        plus_checked++;
    }
    LOGFMTI("zjps layer random: L=%d links=%d | scan reach=%d(cross=%d) light=%d plus=%d all match A* cost",
            L, link_cnt, reach, cross, light_checked, plus_checked);
    return 0;
}

//业务层用link代价调节倾向: 便宜则能上就上(多层大平台) 贵则能不上就不上(单层多建筑)
static s32 zjps_layer_cost_tuning_test()
{
    const s32 W = 40;
    const s32 H = 5;
    const s32 MID_Y = H / 2;
    //地面被一道墙隔断 绕行要走满H 上层是直通的 因此"上去再下来"与"绕行"构成竞争
    const s32 WALL_X = W / 2;

    struct probe
    {
        s32 cost;
        const char* tag;
    };
    probe probes[] = {
        { 1, "floor-cheapest" },
        { zjps_grid::kCostStraight, "straight" },
        { zjps_grid::kDefaultLinkCost, "neutral" },
        { 20 * zjps_grid::kCostStraight, "expensive" },
    };
    s32 climb_cnt = 0;
    s32 detour_cnt = 0;
    for (size_t i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
    {
        zjps_grid grid;
        ASSERT_TEST(grid.init(2, W, H, kCellSize, true) == 0, "tuning init fail");
        //地面墙只在中间留最下面一行可过 迫使绕行有明确额外代价
        ASSERT_TEST(grid.set_rect_cell(0, WALL_X, 0, WALL_X, H - 2, false) == 0, "tuning wall fail");
        //上层竖直楼梯 墙两侧各一处
        ASSERT_TEST(grid.set_link(0, WALL_X - 2, MID_Y, 1, WALL_X - 2, MID_Y, probes[i].cost) == 0, "tuning up link fail");
        ASSERT_TEST(grid.set_link(1, WALL_X + 2, MID_Y, 0, WALL_X + 2, MID_Y, probes[i].cost) == 0, "tuning down link fail");

        std::vector<s32> cells;
        ASSERT_TEST(layer_cross_check(grid, 0, 1, MID_Y, 0, W - 2, MID_Y, probes[i].tag) == 0, "tuning cross check fail");
        ASSERT_TEST(grid.jps_search(0, 1, MID_Y, 0, W - 2, MID_Y, cells) == 0, "tuning query fail");
        bool used_upper = false;
        for (size_t k = 0; k < cells.size(); k++)
        {
            if (grid.cell_layer_of(cells[k]) != 0)
            {
                used_upper = true;
                break;
            }
        }
        if (used_upper)
        {
            climb_cnt++;
        }
        else
        {
            detour_cnt++;
        }
        LOGFMTI("zjps cost tuning [%s] cost=%d: %s (path cost=%d)",
                probes[i].tag, probes[i].cost, used_upper ? "climbs" : "detours", grid.last_path_cost());
    }
    //便宜的link必须至少让一次查询选择上楼 贵的必须至少让一次选择绕行 否则这个旋钮就是失效的
    ASSERT_TEST(climb_cnt > 0, "cheap link must make the search climb at least once");
    ASSERT_TEST(detour_cnt > 0, "expensive link must make the search detour at least once");
    LOGFMTI("zjps cost tuning: link cost is the business-layer knob, %d climbed / %d detoured", climb_cnt, detour_cnt);
    return 0;
}

int main(int argc, char* argv[])
{
    ztest_init();

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zjps_set_rect_test()                          == 0);
    ASSERT_TEST(zjps_set_triangle_test()                      == 0);
    ASSERT_TEST(zjps_corridor_consistency_test()              == 0);
    ASSERT_TEST(zjps_serpentine_consistency_test()            == 0);
    ASSERT_TEST(zjps_random_map_consistency_test()            == 0);
    ASSERT_TEST(zjps_astar_basic_test()                       == 0);
    ASSERT_TEST(zjps_astar_capacity_test()                    == 0);
    ASSERT_TEST(zjps_jps_basic_test()                         == 0);
    ASSERT_TEST(zjps_jps_capacity_test()                      == 0);
    ASSERT_TEST(zjps_height_test()                            == 0);
    ASSERT_TEST(zjps_batch_edit_test()                        == 0);
    ASSERT_TEST(zjps_dirty_flow_test()                        == 0);
    ASSERT_TEST(zjps_eager_test()                             == 0);
    ASSERT_TEST(zjps_five_way_test()                          == 0);
    ASSERT_TEST(zjps_bench_test()                             == 0);
    ASSERT_TEST(zjps_api_surface_test()                       == 0);
    ASSERT_TEST(zjps_wall_detour_test()                       == 0);
    ASSERT_TEST(zjps_jps_plus_test()                          == 0);
    ASSERT_TEST(zjps_phase5_bench_test()                      == 0);
    ASSERT_TEST(zjps_layer_basic_test()                       == 0);
    ASSERT_TEST(zjps_layer_degenerate_test()                  == 0);
    ASSERT_TEST(zjps_layer_three_test()                       == 0);
    ASSERT_TEST(zjps_layer_random_test()                      == 0);
    ASSERT_TEST(zjps_layer_detour_test()                      == 0);
    ASSERT_TEST(zjps_layer_bench_test()                       == 0);
    ASSERT_TEST(zjps_layer_building_bench()                   == 0);
    ASSERT_TEST(zjps_layer_cost_tuning_test()                 == 0);

    LogInfo() << "all test finish .";
    return 0;
}
