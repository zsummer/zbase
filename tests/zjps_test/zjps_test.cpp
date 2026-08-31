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
    if (!grid.cell_walkable(start_x, start_y))
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
            if (!grid.move_valid(x, y, dx, dy))
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
    s32 aret = grid.find_path(ax, ay, bx, by, acells);
    ASSERT_TEST_NOLOG(aret == 0 || aret == -2, "astar unexpected ret i=", i, " j=", j, " aret=", aret);
    bool graph_reach = (graph_ret == 0);
    bool astar_reach = (aret == 0);
    ASSERT_TEST_NOLOG(graph_reach == astar_reach, "astar reach mismatch i=", i, " j=", j,
                      " graph_ret=", graph_ret, " aret=", aret);
    if (!astar_reach)
    {
        std::vector<s32> jcells;
        s32 jret = grid.find_path_jps(ax, ay, bx, by, jcells);
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
        ASSERT_TEST_NOLOG(grid.move_valid(cx, cy, nx - cx, ny - cy), "astar step invalid i=", i, " j=", j, " k=", (s32)k);
    }
    s32 astar_cost = grid.last_path_cost();
    std::vector<s32> jcells;
    s32 jret = grid.find_path_jps(ax, ay, bx, by, jcells);
    ASSERT_TEST_NOLOG(jret == 0, "jps fail on reachable pair i=", i, " j=", j, " jret=", jret);
    ASSERT_TEST_NOLOG(jcells.front() == ay * grid.width() + ax, "jps path start mismatch i=", i, " j=", j);
    ASSERT_TEST_NOLOG(jcells.back() == by * grid.width() + bx, "jps path end mismatch i=", i, " j=", j);
    for (size_t k = 0; k + 1 < jcells.size(); k++)
    {
        s32 cx = jcells[k] % grid.width();
        s32 cy = jcells[k] / grid.width();
        s32 nx = jcells[k + 1] % grid.width();
        s32 ny = jcells[k + 1] / grid.width();
        ASSERT_TEST_NOLOG(grid.move_valid(cx, cy, nx - cx, ny - cy), "jps step invalid i=", i, " j=", j, " k=", (s32)k);
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

static s32 raster_zgraph_link(zjps_grid& grid, const zpoint& pa, const zpoint& pb, f32 half_width)
{
    if (pa.y == pb.y)
    {
        f32 lo = pa.x < pb.x ? pa.x : pb.x;
        f32 hi = pa.x < pb.x ? pb.x : pa.x;
        return grid.raster_rect(lo - half_width, pa.y - half_width, hi + half_width, pa.y + half_width, true);
    }
    if (pa.x == pb.x)
    {
        f32 lo = pa.y < pb.y ? pa.y : pb.y;
        f32 hi = pa.y < pb.y ? pb.y : pa.y;
        return grid.raster_rect(pa.x - half_width, lo - half_width, pa.x + half_width, hi + half_width, true);
    }
    return grid.raster_segment(pa, pb, half_width, true);
}

static s32 zjps_raster_rect_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(20, 20, kCellSize, false) == 0, "init fail");
    u32 version_before = grid.map_version();
    ASSERT_TEST(grid.raster_rect(200.0f, 252.0f, 400.0f, 348.0f, true) == 0, "raster_rect fail");
    ASSERT_TEST(grid.map_version() > version_before, "map_version not bumped");
    s32 walkable_cnt = 0;
    for (s32 y = 0; y < grid.height(); y++)
    {
        for (s32 x = 0; x < grid.width(); x++)
        {
            bool expect = (x >= 4 && x <= 7 && y >= 5 && y <= 6);
            ASSERT_TEST(grid.cell_walkable(x, y) == expect, "rect cell mismatch x=", x, " y=", y);
            if (grid.cell_walkable(x, y))
            {
                walkable_cnt++;
            }
        }
    }
    ASSERT_TEST(walkable_cnt == 8, "rect walkable cnt=", walkable_cnt);
    return 0;
}

static s32 zjps_raster_segment_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(20, 20, kCellSize, false) == 0, "init fail");
    zpoint a(200.0f, 300.0f, 0.0f);
    zpoint b(400.0f, 300.0f, 0.0f);
    ASSERT_TEST(grid.raster_segment(a, b, kCorridorHalfWidth, true) == 0, "raster_segment fail");
    s32 walkable_cnt = 0;
    for (s32 y = 0; y < grid.height(); y++)
    {
        for (s32 x = 0; x < grid.width(); x++)
        {
            bool expect = (x >= 3 && x <= 8 && y >= 5 && y <= 6);
            ASSERT_TEST(grid.cell_walkable(x, y) == expect, "segment cell mismatch x=", x, " y=", y);
            if (grid.cell_walkable(x, y))
            {
                walkable_cnt++;
            }
        }
    }
    ASSERT_TEST(walkable_cnt == 12, "segment walkable cnt=", walkable_cnt);

    zjps_grid diag_grid;
    ASSERT_TEST(diag_grid.init(20, 20, kCellSize, false) == 0, "init fail");
    zpoint da(200.0f, 200.0f, 0.0f);
    zpoint db(400.0f, 400.0f, 0.0f);
    ASSERT_TEST(diag_grid.raster_segment(da, db, kCorridorHalfWidth, true) == 0, "raster_segment diag fail");
    s32 diag_cnt = 0;
    for (s32 y = 0; y < diag_grid.height(); y++)
    {
        for (s32 x = 0; x < diag_grid.width(); x++)
        {
            if (x >= 3 && x <= 8 && y >= 3 && y <= 8 && (x - y <= 1 && y - x <= 1))
            {
                ASSERT_TEST(diag_grid.cell_walkable(x, y), "diag expect walkable x=", x, " y=", y);
                diag_cnt++;
            }
            else
            {
                ASSERT_TEST(!diag_grid.cell_walkable(x, y), "diag expect blocked x=", x, " y=", y);
            }
        }
    }
    ASSERT_TEST(diag_cnt == 16, "diag walkable cnt=", diag_cnt);
    return 0;
}

static s32 zjps_raster_circle_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(20, 20, kCellSize, false) == 0, "init fail");
    ASSERT_TEST(grid.raster_circle(250.0f, 250.0f, 100.0f, true) == 0, "raster_circle fail");
    s32 walkable_cnt = 0;
    for (s32 y = 0; y < grid.height(); y++)
    {
        for (s32 x = 0; x < grid.width(); x++)
        {
            f32 cx = ((f32)x + 0.5f) * kCellSize - 250.0f;
            f32 cy = ((f32)y + 0.5f) * kCellSize - 250.0f;
            bool expect = (cx * cx + cy * cy <= 100.0f * 100.0f);
            ASSERT_TEST(grid.cell_walkable(x, y) == expect, "circle cell mismatch x=", x, " y=", y);
            if (grid.cell_walkable(x, y))
            {
                walkable_cnt++;
            }
        }
    }
    ASSERT_TEST(walkable_cnt == 12, "circle walkable cnt=", walkable_cnt);
    return 0;
}

static s32 zjps_raster_polygon_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(20, 20, kCellSize, false) == 0, "init fail");
    std::vector<zpoint> vertexes;
    vertexes.push_back(zpoint(50.0f, 50.0f, 0.0f));
    vertexes.push_back(zpoint(550.0f, 50.0f, 0.0f));
    vertexes.push_back(zpoint(550.0f, 250.0f, 0.0f));
    vertexes.push_back(zpoint(250.0f, 250.0f, 0.0f));
    vertexes.push_back(zpoint(250.0f, 550.0f, 0.0f));
    vertexes.push_back(zpoint(50.0f, 550.0f, 0.0f));
    ASSERT_TEST(grid.raster_polygon(vertexes, true) == 0, "raster_polygon fail");
    s32 walkable_cnt = 0;
    for (s32 y = 0; y < grid.height(); y++)
    {
        for (s32 x = 0; x < grid.width(); x++)
        {
            f32 center_x = ((f32)x + 0.5f) * kCellSize;
            f32 center_y = ((f32)y + 0.5f) * kCellSize;
            bool in_bottom_bar = (center_x >= 50.0f && center_x <= 550.0f && center_y >= 50.0f && center_y <= 250.0f);
            bool in_left_bar = (center_x >= 50.0f && center_x <= 250.0f && center_y >= 50.0f && center_y <= 550.0f);
            bool expect = in_bottom_bar || in_left_bar;
            ASSERT_TEST(grid.cell_walkable(x, y) == expect, "polygon cell mismatch x=", x, " y=", y);
            if (grid.cell_walkable(x, y))
            {
                walkable_cnt++;
            }
        }
    }
    ASSERT_TEST(walkable_cnt == 64, "polygon walkable cnt=", walkable_cnt);
    ASSERT_TEST(!grid.cell_walkable(8, 8), "concave notch cell should be blocked");
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
    ASSERT_TEST(grid.init(W, W, kCellSize, false) == 0, "grid init fail");
    for (s32 i = 0; i < LINK_CNT; i++)
    {
        ASSERT_TEST(raster_zgraph_link(grid, node_pos[link_pairs[i][0]], node_pos[link_pairs[i][1]], kCorridorHalfWidth) == 0,
                    "raster link fail i=", i);
    }

    s32 cell_x[NODE_CNT] = { 0 };
    s32 cell_y[NODE_CNT] = { 0 };
    for (s32 i = 0; i < NODE_CNT; i++)
    {
        ASSERT_TEST(grid.pos_to_cell(node_pos[i].x, node_pos[i].y, cell_x[i], cell_y[i]) == 0, "pos_to_cell fail i=", i);
        ASSERT_TEST(grid.cell_walkable(cell_x[i], cell_y[i]), "node cell not walkable i=", i);
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
    ASSERT_TEST(grid.init(W, W, kCellSize, false) == 0, "grid init fail");
    ASSERT_TEST(grid.set_open_capacity(65536) == 0, "set open capacity fail");
    ASSERT_TEST(grid.build_jps_light() == 0, "light build fail");
    for (s32 k = 0; k < ROW_CNT; k++)
    {
        ASSERT_TEST(raster_zgraph_link(grid, node_pos[2 * k], node_pos[2 * k + 1], kCorridorHalfWidth) == 0, "raster row fail k=", k);
    }
    ASSERT_TEST(grid.build_jps_light() == 0, "light rebuild after raster fail");
    for (s32 k = 0; k + 1 < ROW_CNT; k++)
    {
        s32 side = (k % 2 == 0) ? 1 : 0;
        ASSERT_TEST(raster_zgraph_link(grid, node_pos[2 * k + side], node_pos[2 * (k + 1) + side], kCorridorHalfWidth) == 0,
                    "raster conn fail k=", k);
    }
    ASSERT_TEST(raster_zgraph_link(grid, node_pos[NODE_CNT - 2], node_pos[NODE_CNT - 1], kCorridorHalfWidth) == 0,
                "raster isolated fail");

    std::vector<s32> cell_x(NODE_CNT, 0);
    std::vector<s32> cell_y(NODE_CNT, 0);
    for (s32 i = 0; i < NODE_CNT; i++)
    {
        ASSERT_TEST(grid.pos_to_cell(node_pos[i].x, node_pos[i].y, cell_x[i], cell_y[i]) == 0, "pos_to_cell fail i=", i);
        ASSERT_TEST(grid.cell_walkable(cell_x[i], cell_y[i]), "node cell not walkable i=", i);
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
        s32 aret = grid.find_path(cell_x[0], cell_y[0], cell_x[2 * (ROW_CNT - 1)], cell_y[2 * (ROW_CNT - 1)], acells);
        ASSERT_TEST(aret == 0, "serpentine astar long query fail aret=", aret);
        LOGFMTI("zjps astar serpentine long query: cells=%d cost=%d open_push=%d open_pop=%d open_peak=%d visit=%d",
                (s32)acells.size(), grid.last_path_cost(), grid.open_push_count(), grid.open_pop_count(),
                grid.open_peak(), grid.visit_count());
    }
    {
        std::vector<s32> jcells;
        s32 jret = grid.find_path_jps(cell_x[0], cell_y[0], cell_x[2 * (ROW_CNT - 1)], cell_y[2 * (ROW_CNT - 1)], jcells);
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
    ASSERT_TEST(grid.init(width, width, kCellSize, true) == 0, "grid init fail");
    ASSERT_TEST(grid.set_open_capacity(8192) == 0, "set open capacity fail");
    ASSERT_TEST(grid.build_jps_light() == 0, "light build fail");
    std::mt19937 rng(seed);
    for (s32 i = 0; i < rect_cnt; i++)
    {
        s32 bx = (s32)(rng() % (size_t)(width - 3));
        s32 by = (s32)(rng() % (size_t)(width - 3));
        s32 bw = 1 + (s32)(rng() % 3);
        s32 bh = 1 + (s32)(rng() % 3);
        ASSERT_TEST(grid.raster_rect((f32)bx * kCellSize, (f32)by * kCellSize,
                                     (f32)(bx + bw) * kCellSize, (f32)(by + bh) * kCellSize, false) == 0,
                    "block rect fail i=", i);
    }

    std::vector<s32> node_ids((size_t)width * width, -1);
    std::vector<s32> cell_list;
    test_graph graph;
    for (s32 y = 0; y < width; y++)
    {
        for (s32 x = 0; x < width; x++)
        {
            if (!grid.cell_walkable(x, y))
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
                && (node_ids[(size_t)y * width + (size_t)x + 1] >= 0 || node_ids[(size_t)(y + 1) * width + (size_t)x] >= 0))
            {
                s32 lid = graph.new_link(node_ids[(size_t)y * width + (size_t)x], node_ids[(size_t)(y + 1) * width + (size_t)x + 1], link_cnt);
                ASSERT_TEST(lid >= 0, "new_link northeast fail x=", x, " y=", y);
                s32 affects = 0;
                ASSERT_TEST(graph.push_link(lid, affects) == 0, "push_link northeast fail x=", x, " y=", y);
                link_cnt++;
            }
            if (x + 1 < width && y - 1 >= 0
                && node_ids[(size_t)(y - 1) * width + (size_t)x + 1] >= 0
                && (node_ids[(size_t)y * width + (size_t)x + 1] >= 0 || node_ids[(size_t)(y - 1) * width + (size_t)x] >= 0))
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
    ASSERT_TEST(grid.init(20, 20, kCellSize, true) == 0, "grid init fail");
    std::vector<s32> cells;
    s32 ret = grid.find_path(0, 0, 10, 5, cells);
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
        ASSERT_TEST(grid.move_valid(cx, cy, nx - cx, ny - cy), "step invalid k=", (s32)k);
    }

    ret = grid.find_path(3, 3, 3, 3, cells);
    ASSERT_TEST(ret == 0 && cells.size() == 1, "source==target expect single cell, ret=", ret);

    ret = grid.find_path(-1, 0, 5, 5, cells);
    ASSERT_TEST(ret == -1, "invalid source expect -1, ret=", ret);
    ret = grid.find_path(0, 0, 20, 0, cells);
    ASSERT_TEST(ret == -1, "invalid target expect -1, ret=", ret);

    ASSERT_TEST(grid.set_blocked(15, 15) == 0, "set_blocked fail");
    ret = grid.find_path(0, 0, 15, 15, cells);
    ASSERT_TEST(ret == -2, "blocked target expect -2, ret=", ret);
    ASSERT_TEST(grid.set_walkable(15, 15) == 0, "set_walkable fail");

    for (s32 y = 0; y < 20; y++)
    {
        ASSERT_TEST(grid.set_blocked(10, y) == 0, "set wall fail y=", y);
    }
    ret = grid.find_path(0, 0, 19, 0, cells);
    ASSERT_TEST(ret == -2, "unreachable expect -2, ret=", ret);
    for (s32 y = 0; y < 20; y++)
    {
        ASSERT_TEST(grid.set_walkable(10, y) == 0, "clear wall fail y=", y);
    }
    ret = grid.find_path(0, 0, 19, 0, cells);
    ASSERT_TEST(ret == 0, "restored path fail ret=", ret);

    zjps_grid diag_grid;
    ASSERT_TEST(diag_grid.init(20, 20, kCellSize, true) == 0, "diag grid init fail");
    ret = diag_grid.find_path(0, 0, 19, 19, cells);
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
    ASSERT_TEST(grid.init(20, 20, kCellSize, true) == 0, "grid init fail");
    ASSERT_TEST(grid.open_capacity() == zjps_grid::kDefaultOpenCnt, "default capacity mismatch");
    ASSERT_TEST(grid.set_open_capacity(2) == 0, "set open capacity 2 fail");
    ASSERT_TEST(grid.set_open_capacity(0) == -1, "set open capacity 0 expect -1");
    std::vector<s32> cells;
    s32 ret = grid.find_path(0, 0, 19, 19, cells);
    ASSERT_TEST(ret == -3, "open overflow expect -3, ret=", ret);
    ASSERT_TEST(grid.set_open_capacity(1024) == 0, "set open capacity 1024 fail");
    ret = grid.find_path(0, 0, 19, 19, cells);
    ASSERT_TEST(ret == 0, "find_path after capacity restore fail ret=", ret);
    ASSERT_TEST(grid.last_path_cost() == 19 * 1414, "cost after restore mismatch");
    return 0;
}

static s32 zjps_jps_basic_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(20, 20, kCellSize, true) == 0, "grid init fail");
    std::vector<s32> cells;
    s32 ret = grid.find_path_jps(0, 0, 10, 5, cells);
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
        ASSERT_TEST(grid.move_valid(cx, cy, nx - cx, ny - cy), "jps step invalid k=", (s32)k);
    }
    std::vector<s32> acells;
    s32 aret = grid.find_path(0, 0, 10, 5, acells);
    ASSERT_TEST(aret == 0 && grid.last_path_cost() == 1000 * 10 + 414 * 5, "astar cross fail");

    ret = grid.find_path_jps(3, 3, 3, 3, cells);
    ASSERT_TEST(ret == 0 && cells.size() == 1, "jps source==target expect single cell, ret=", ret);
    ret = grid.find_path_jps(-1, 0, 5, 5, cells);
    ASSERT_TEST(ret == -1, "jps invalid source expect -1, ret=", ret);
    ret = grid.find_path_jps(0, 0, 20, 0, cells);
    ASSERT_TEST(ret == -1, "jps invalid target expect -1, ret=", ret);
    ASSERT_TEST(grid.set_blocked(15, 15) == 0, "jps set_blocked fail");
    ret = grid.find_path_jps(0, 0, 15, 15, cells);
    ASSERT_TEST(ret == -2, "jps blocked target expect -2, ret=", ret);
    ASSERT_TEST(grid.set_walkable(15, 15) == 0, "jps set_walkable fail");
    for (s32 y = 0; y < 20; y++)
    {
        ASSERT_TEST(grid.set_blocked(10, y) == 0, "jps set wall fail y=", y);
    }
    ret = grid.find_path_jps(0, 0, 19, 0, cells);
    ASSERT_TEST(ret == -2, "jps unreachable expect -2, ret=", ret);
    for (s32 y = 0; y < 20; y++)
    {
        ASSERT_TEST(grid.set_walkable(10, y) == 0, "jps clear wall fail y=", y);
    }
    ret = grid.find_path_jps(0, 0, 19, 19, cells);
    ASSERT_TEST(ret == 0 && grid.last_path_cost() == 19 * 1414, "jps diag cost mismatch, got=", grid.last_path_cost());
    return 0;
}

static s32 zjps_jps_capacity_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(20, 20, kCellSize, true) == 0, "grid init fail");
    ASSERT_TEST(grid.set_blocked(1, 1) == 0, "block fail");
    ASSERT_TEST(grid.set_open_capacity(1) == 0, "set open capacity 1 fail");
    std::vector<s32> cells;
    s32 ret = grid.find_path_jps(0, 0, 19, 19, cells);
    ASSERT_TEST(ret == -3, "jps open overflow expect -3, ret=", ret);
    ASSERT_TEST(grid.set_open_capacity(1024) == 0, "jps set open capacity 1024 fail");
    ret = grid.find_path_jps(0, 0, 19, 19, cells);
    ASSERT_TEST(ret == 0, "jps after capacity restore fail ret=", ret);
    return 0;
}

static s32 bench_serpentine_grid(zjps_grid& grid)
{
    const s32 W = 400;
    ASSERT_TEST(grid.init(W, W, kCellSize, false) == 0, "bench grid init fail");
    ASSERT_TEST(grid.set_open_capacity(65536) == 0, "bench open capacity fail");
    for (s32 k = 0; k < 33; k++)
    {
        f32 y = 300.0f + 600.0f * (f32)k;
        ASSERT_TEST(raster_zgraph_link(grid, zpoint(300.0f, y, 0.0f), zpoint(19700.0f, y, 0.0f), kCorridorHalfWidth) == 0,
                    "bench raster row fail k=", k);
        if (k + 1 < 33)
        {
            f32 sx = (k % 2 == 0) ? 19700.0f : 300.0f;
            ASSERT_TEST(raster_zgraph_link(grid, zpoint(sx, y, 0.0f), zpoint(sx, y + 600.0f, 0.0f), kCorridorHalfWidth) == 0,
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
            s32 ret = grid.find_path(sx, sy, tx, ty, cells);
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
            s32 ret = grid.find_path_jps(sx, sy, tx, ty, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench serpentine(400x400, 12675-cell path) jps:   %.0f ns/op visit=%d open_peak=%d",
                (f64)cost.cost_ns() / (f64)N, grid.visit_count(), grid.open_peak());
    }

    zjps_grid rgrid;
    const s32 RW = 20;
    ASSERT_TEST(rgrid.init(RW, RW, kCellSize, true) == 0, "bench rgrid init fail");
    ASSERT_TEST(rgrid.set_open_capacity(8192) == 0, "bench rgrid capacity fail");
    ASSERT_TEST(rgrid.build_jps_light() == 0, "bench rgrid light build fail");
    std::mt19937 rng(20260834u);
    for (s32 i = 0; i < 14; i++)
    {
        s32 bx = (s32)(rng() % (size_t)(RW - 3));
        s32 by = (s32)(rng() % (size_t)(RW - 3));
        s32 bw = 1 + (s32)(rng() % 3);
        s32 bh = 1 + (s32)(rng() % 3);
        ASSERT_TEST(rgrid.raster_rect((f32)bx * kCellSize, (f32)by * kCellSize,
                                       (f32)(bx + bw) * kCellSize, (f32)(by + bh) * kCellSize, false) == 0,
                    "bench block fail i=", i);
    }
    std::vector<s32> rcells;
    for (s32 y = 0; y < RW; y++)
    {
        for (s32 x = 0; x < RW; x++)
        {
            if (rgrid.cell_walkable(x, y))
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
            s32 ret = rgrid.find_path(a[i] % RW, a[i] / RW, b[i] % RW, b[i] / RW, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench random(20x20) astar: %.0f ns/op", (f64)cost.cost_ns() / (f64)N);
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 ret = rgrid.find_path_jps(a[i] % RW, a[i] / RW, b[i] % RW, b[i] / RW, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench random(20x20) jps:   %.0f ns/op", (f64)cost.cost_ns() / (f64)N);
    }

    zjps_grid egrid;
    ASSERT_TEST(egrid.init(200, 200, kCellSize, true) == 0, "bench egrid init fail");
    ASSERT_TEST(egrid.build_jps_light() == 0, "bench egrid light build fail");
    {
        const s32 N = 2000;
        zclock<> cost;
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 ret = egrid.find_path(1, 1, 198, 198, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench openfield(200x200 diag) astar: %.0f ns/op visit=%d", (f64)cost.cost_ns() / (f64)N,
                egrid.visit_count(), egrid.open_peak());
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 ret = egrid.find_path_jps(1, 1, 198, 198, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("bench openfield(200x200 diag) jps:   %.0f ns/op visit=%d", (f64)cost.cost_ns() / (f64)N,
                egrid.visit_count(), egrid.open_peak());
    }
    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

static s32 zjps_fcut_experiment_test()
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(20, 20, kCellSize, true) == 0, "fcut grid init fail");
    for (s32 x = 0; x <= 9; x++)
    {
        ASSERT_TEST(grid.set_blocked(x, 1) == 0, "fcut wall fail x=", x);
        ASSERT_TEST(grid.set_blocked(x, 2) == 0, "fcut wall fail x=", x);
    }
    std::vector<s32> cells;
    s32 aret = grid.find_path(0, 0, 10, 3, cells);
    ASSERT_TEST(aret == 0, "fcut astar fail aret=", aret);
    ASSERT_TEST(grid.last_path_cost() == 12414, "fcut astar cost mismatch, got=", grid.last_path_cost());
    s32 jret = grid.find_path_jps(0, 0, 10, 3, cells);
    ASSERT_TEST(jret == 0, "fcut jps fail jret=", jret);
    ASSERT_TEST(grid.last_path_cost() == 12414, "fcut jps cost mismatch, got=", grid.last_path_cost());

    ASSERT_TEST(grid.set_scan_f_cut(true) == 0, "fcut enable fail");
    s32 cret = grid.find_path_jps(0, 0, 10, 3, cells);
    LOGFMTI("fcut counterexample(wall detour): astar=12414 jps_nocut=12414 jps_fcut ret=%d cost=%d",
            cret, grid.last_path_cost());
    ASSERT_TEST(cret != 0 || grid.last_path_cost() != 12414,
                "fcut counterexample NOT reproduced: cut variant still optimal");
    ASSERT_TEST(grid.set_scan_f_cut(false) == 0, "fcut disable fail");

    zjps_grid rgrid;
    ASSERT_TEST(rgrid.init(20, 20, kCellSize, true) == 0, "fcut rgrid init fail");
    ASSERT_TEST(rgrid.set_open_capacity(8192) == 0, "fcut rgrid capacity fail");
    std::mt19937 rng(20260835u);
    for (s32 i = 0; i < 14; i++)
    {
        s32 bx = (s32)(rng() % (size_t)(20 - 3));
        s32 by = (s32)(rng() % (size_t)(20 - 3));
        s32 bw = 1 + (s32)(rng() % 3);
        s32 bh = 1 + (s32)(rng() % 3);
        ASSERT_TEST(rgrid.raster_rect((f32)bx * kCellSize, (f32)by * kCellSize,
                                       (f32)(bx + bw) * kCellSize, (f32)(by + bh) * kCellSize, false) == 0,
                    "fcut block fail i=", i);
    }
    std::vector<s32> rcells;
    for (s32 y = 0; y < 20; y++)
    {
        for (s32 x = 0; x < 20; x++)
        {
            if (rgrid.cell_walkable(x, y))
            {
                rcells.push_back(y * 20 + x);
            }
        }
    }
    ASSERT_TEST(rgrid.set_scan_f_cut(true) == 0, "fcut rgrid enable fail");
    std::uniform_int_distribution<size_t> dist(0, rcells.size() - 1);
    s32 mismatch = 0;
    for (s32 s = 0; s < 2000; s++)
    {
        s32 a = rcells[dist(rng)];
        s32 b = rcells[dist(rng)];
        s32 ar = rgrid.find_path(a % 20, a / 20, b % 20, b / 20, cells);
        s32 acost = rgrid.last_path_cost();
        s32 jr = rgrid.find_path_jps(a % 20, a / 20, b % 20, b / 20, cells);
        s32 jcost = rgrid.last_path_cost();
        if (ar != jr || (ar == 0 && acost != jcost))
        {
            mismatch++;
        }
    }
    LOGFMTI("fcut random map: 2000 pairs checked, %d mismatch vs astar (ret or cost differs)", mismatch);
    ASSERT_TEST(rgrid.set_scan_f_cut(false) == 0, "fcut rgrid disable fail");

    zjps_grid egrid;
    ASSERT_TEST(egrid.init(200, 200, kCellSize, true) == 0, "fcut egrid init fail");
    ASSERT_TEST(egrid.set_scan_f_cut(true) == 0, "fcut egrid enable fail");
    volatile s32 salt = 0;
    {
        const s32 N = 2000;
        zclock<> cost;
        cost.start();
        for (s32 i = 0; i < N; i++)
        {
            s32 ret = egrid.find_path_jps(1, 1, 198, 198, cells);
            salt += ret + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("fcut openfield(200x200 diag) jps_with_fcut: %.0f ns/op (nocut baseline ~90000 ns/op)",
                (f64)cost.cost_ns() / (f64)N);
    }
    LOGFMTI("(anti-optimize salt=%d)", (int)salt);
    return 0;
}

static s32 plus_random_check(s32 width, s32 rect_cnt, u32 seed)
{
    zjps_grid grid;
    ASSERT_TEST(grid.init(width, width, kCellSize, true) == 0, "plus grid init fail");
    ASSERT_TEST(grid.set_open_capacity(8192) == 0, "plus capacity fail");
    std::mt19937 rng(seed);
    for (s32 i = 0; i < rect_cnt; i++)
    {
        s32 bx = (s32)(rng() % (size_t)(width - 3));
        s32 by = (s32)(rng() % (size_t)(width - 3));
        s32 bw = 1 + (s32)(rng() % 3);
        s32 bh = 1 + (s32)(rng() % 3);
        ASSERT_TEST(grid.raster_rect((f32)bx * kCellSize, (f32)by * kCellSize,
                                     (f32)(bx + bw) * kCellSize, (f32)(by + bh) * kCellSize, false) == 0,
                    "plus block fail i=", i);
    }
    ASSERT_TEST(grid.build_jps_plus() == 0, "plus build fail");

    std::vector<s32> cell_list;
    for (s32 y = 0; y < width; y++)
    {
        for (s32 x = 0; x < width; x++)
        {
            if (grid.cell_walkable(x, y))
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
            s32 ar = grid.find_path(cell_list[i] % width, cell_list[i] / width,
                                    cell_list[j] % width, cell_list[j] / width, acells);
            s32 acost = grid.last_path_cost();
            s32 pr = grid.find_path_jps_plus(cell_list[i] % width, cell_list[i] / width,
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
                    ASSERT_TEST(grid.move_valid(cx, cy, nx - cx, ny - cy), "plus step invalid i=", (s32)i,
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
    ASSERT_TEST(grid.init(20, 20, kCellSize, true) == 0, "plus openfield init fail");
    ASSERT_TEST(grid.build_jps_plus() == 0, "plus openfield build fail");
    std::vector<s32> cells;
    s32 ret = grid.find_path_jps_plus(0, 0, 15, 10, cells);
    ASSERT_TEST(ret == 0, "plus openfield target-via-subray fail ret=", ret);
    ASSERT_TEST(grid.last_path_cost() == 19140, "plus openfield cost mismatch, got=", grid.last_path_cost());
    ret = grid.find_path_jps_plus(1, 1, 18, 18, cells);
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
    ret = sgrid.find_path_jps_plus(sx, sy, tx, ty, cells);
    ASSERT_TEST(ret == 0, "plus serpentine long query fail ret=", ret);
    ASSERT_TEST(sgrid.last_path_cost() == 12712916, "plus serpentine long cost mismatch, got=", sgrid.last_path_cost());

    std::vector<s32> walk_cells;
    for (s32 y = 0; y < sgrid.height(); y++)
    {
        for (s32 x = 0; x < sgrid.width(); x++)
        {
            if (sgrid.cell_walkable(x, y))
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
            s32 jr = sgrid.find_path_jps(a % 400, a / 400, b % 400, b / 400, jcells);
            s32 jcost = sgrid.last_path_cost();
            s32 pr = sgrid.find_path_jps_plus(a % 400, a / 400, b % 400, b / 400, cells);
            s32 pcost = sgrid.last_path_cost();
            ASSERT_TEST(jr == pr && (jr != 0 || jcost == pcost), "plus serpentine mismatch s=", s,
                        " jr=", jr, " pr=", pr, " jcost=", jcost, " pcost=", pcost);
        }
        LOGFMTI("plus serpentine: 600 sampled pairs jps+ == jps");
    }

    zjps_grid rgrid;
    ASSERT_TEST(rgrid.init(20, 20, kCellSize, true) == 0, "plus fallback init fail");
    std::mt19937 rng(20260839u);
    for (s32 i = 0; i < 14; i++)
    {
        s32 bx = (s32)(rng() % (size_t)17);
        s32 by = (s32)(rng() % (size_t)17);
        s32 bw = 1 + (s32)(rng() % 3);
        s32 bh = 1 + (s32)(rng() % 3);
        ASSERT_TEST(rgrid.raster_rect((f32)bx * kCellSize, (f32)by * kCellSize,
                                       (f32)(bx + bw) * kCellSize, (f32)(by + bh) * kCellSize, false) == 0,
                    "plus fallback block fail i=", i);
    }
    ASSERT_TEST(rgrid.build_jps_plus() == 0, "plus fallback build fail");
    ASSERT_TEST(rgrid.set_blocked(3, 3) == 0, "plus fallback set_blocked fail");
    {
        s32 ar = rgrid.find_path(0, 0, 19, 19, cells);
        s32 acost = rgrid.last_path_cost();
        s32 pr = rgrid.find_path_jps_plus(0, 0, 19, 19, cells);
        s32 pcost = rgrid.last_path_cost();
        ASSERT_TEST(ar == pr && (ar != 0 || acost == pcost), "plus fallback mismatch ar=", ar, " pr=", pr);
        LOGFMTI("plus fallback: after set_blocked(version bump) jps+ auto-fallback matches astar ret=%d cost=%d",
                pr, pcost);
    }
    ASSERT_TEST(rgrid.build_jps_plus() == 0, "plus fallback rebuild fail");
    {
        s32 ar = rgrid.find_path(0, 0, 19, 19, cells);
        s32 acost = rgrid.last_path_cost();
        s32 pr = rgrid.find_path_jps_plus(0, 0, 19, 19, cells);
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
            s32 r = sgrid.find_path_jps_plus(sx, sy, tx, ty, cells);
            salt += r + (s32)cells.size();
        }
        cost.stop_and_save();
        LOGFMTI("plus bench serpentine jps+: %.0f ns/op (no-build jps baseline ~120000 ns/op)",
                (f64)cost.cost_ns() / (f64)N);
    }
    zjps_grid egrid;
    ASSERT_TEST(egrid.init(200, 200, kCellSize, true) == 0, "plus egrid init fail");
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
            s32 r = egrid.find_path_jps_plus(1, 1, 198, 198, cells);
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
    ASSERT_TEST(grid.build_jps_light() == 0, "p5 serp light build fail");

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

    zclock<> build_cost;
    build_cost.start();
    ASSERT_TEST(grid.build_jps_plus() == 0, "p5 serp jps+ build fail");
    build_cost.stop_and_save();
    LOGFMTI("phase5 serpentine: jps+ build=%.2fms table=%.1fMB", (f64)build_cost.cost_ns() / 1000000.0,
            (f64)grid.jps_plus_table_bytes() / 1048576.0);

    std::mt19937 rng(20260841u);
    const s32 PAIR_CNT = 1500;
    std::vector<test_graph::graph_path_step> steps;
    std::vector<phase5_pair> tier_pairs[3];
    f64 tier_zlen[3] = { 0.0, 0.0, 0.0 };
    for (s32 s = 0; s < PAIR_CNT; s++)
    {
        s32 i = (s32)(rng() % (size_t)NODE_CNT);
        s32 j = (s32)(rng() % (size_t)NODE_CNT);
        s32 ret = graph.find_path(node_ids[i], node_ids[j], steps);
        ASSERT_TEST_NOLOG(ret == 0, "p5 serp reach fail i=", i, " j=", j);
        f32 zlen = phase5_path_len(steps, node_pos[i]);
        s32 tier = zlen < 8000.0f ? 0 : (zlen < 40000.0f ? 1 : 2);
        phase5_pair pr;
        pr.na = i;
        pr.nb = j;
        pr.ax = (s32)(node_pos[i].x / kCellSize);
        pr.ay = (s32)(node_pos[i].y / kCellSize);
        pr.bx = (s32)(node_pos[j].x / kCellSize);
        pr.by = (s32)(node_pos[j].y / kCellSize);
        tier_pairs[tier].push_back(pr);
        tier_zlen[tier] += zlen;
    }

    volatile s32 salt = 0;
    std::vector<s32> cells;
    {
        std::vector<s32> warm_cells;
        s32 warm_ret = grid.find_path_jps(6, 6, 6, 390, warm_cells);
        salt += warm_ret;
    }
    const char* tier_names[3] = { "short<80m", "mid80-400m", "long>400m" };
    for (s32 t = 0; t < 3; t++)
    {
        if (tier_pairs[t].empty())
        {
            continue;
        }
        const s32 N = (s32)tier_pairs[t].size();
        f64 zlen_avg_m = tier_zlen[t] / (f64)N / 100.0;
        f64 zg_ns = 0.0;
        f64 a_ns = 0.0;
        f64 j_ns = 0.0;
        f64 p_ns = 0.0;
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                graph.find_path(node_ids[tier_pairs[t][s].na], node_ids[tier_pairs[t][s].nb], steps);
                salt += (s32)steps.size();
            }
            c.stop_and_save();
            zg_ns = (f64)c.cost_ns() / (f64)N;
        }
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                grid.find_path(tier_pairs[t][s].ax, tier_pairs[t][s].ay, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                salt += (s32)cells.size();
            }
            c.stop_and_save();
            a_ns = (f64)c.cost_ns() / (f64)N;
        }
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                grid.find_path_jps(tier_pairs[t][s].ax, tier_pairs[t][s].ay, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                salt += (s32)cells.size();
            }
            c.stop_and_save();
            j_ns = (f64)c.cost_ns() / (f64)N;
        }
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                grid.find_path_jps_plus(tier_pairs[t][s].ax, tier_pairs[t][s].ay, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                salt += (s32)cells.size();
            }
            c.stop_and_save();
            p_ns = (f64)c.cost_ns() / (f64)N;
        }
        LOGFMTI("phase5 serpentine %s: pairs=%d zpath=%.1fm | zgraph=%.0fns astar=%.0fns jps=%.0fns jps+=%.0fns",
                tier_names[t], N, zlen_avg_m, zg_ns, a_ns, j_ns, p_ns);
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
                grid.set_blocked(6, 5);
            }
            else
            {
                grid.set_walkable(6, 5);
            }
        }
        c.stop_and_save();
        LOGFMTI("phase5 dynamic: grid set_blocked/set_walkable(incl incremental index)=%.0fns/op",
                (f64)c.cost_ns() / (f64)OPS);
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
    ASSERT_TEST(grid.init(W, W, kCellSize, true) == 0, "p5 obst grid fail");
    ASSERT_TEST(grid.set_open_capacity(65536) == 0, "p5 obst capacity fail");
    std::mt19937 rng(seed);
    for (s32 i = 0; i < rect_cnt; i++)
    {
        s32 bx = (s32)(rng() % (size_t)(W - 8));
        s32 by = (s32)(rng() % (size_t)(W - 8));
        s32 bw = 2 + (s32)(rng() % 5);
        s32 bh = 2 + (s32)(rng() % 5);
        ASSERT_TEST(grid.raster_rect((f32)bx * kCellSize, (f32)by * kCellSize,
                                     (f32)(bx + bw) * kCellSize, (f32)(by + bh) * kCellSize, false) == 0,
                    "p5 obst block fail i=", i);
    }
    s32 walkable_cnt = 0;
    for (s32 y = 0; y < W; y++)
    {
        for (s32 x = 0; x < W; x++)
        {
            if (grid.cell_walkable(x, y))
            {
                walkable_cnt++;
            }
        }
    }
    ASSERT_TEST(grid.build_jps_light() == 0, "p5 obst light build fail");
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
            if (grid.cell_walkable(cx * STRIDE, cy * STRIDE))
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
                    if (!grid.cell_walkable(cx * STRIDE + i, cy * STRIDE))
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
                    if (!grid.cell_walkable(cx * STRIDE, cy * STRIDE + i))
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
                    if (!grid.cell_walkable(cx * STRIDE + i, cy * STRIDE + i))
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
                    if (!grid.cell_walkable(cx * STRIDE + i, cy * STRIDE - i))
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
    build_cost.start();
    ASSERT_TEST(grid.build_jps_plus() == 0, "p5 obst jps+ build fail");
    build_cost.stop_and_save();

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
        s32 ar = grid.find_path(ax, ay, bx, by, cells);
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
    {
        std::vector<s32> warm_cells;
        s32 warm_ret = grid.find_path_jps(tier_pairs[0][0].ax, tier_pairs[0][0].ay,
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
        f64 zg_ns = 0.0;
        f64 a_ns = 0.0;
        f64 j_ns = 0.0;
        f64 p_ns = 0.0;
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                graph.find_path(tier_pairs[t][s].na, tier_pairs[t][s].nb, steps);
                salt += (s32)steps.size();
            }
            c.stop_and_save();
            zg_ns = (f64)c.cost_ns() / (f64)N;
        }
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                grid.find_path(tier_pairs[t][s].ax, tier_pairs[t][s].ay, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                salt += (s32)cells.size();
            }
            c.stop_and_save();
            a_ns = (f64)c.cost_ns() / (f64)N;
        }
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                grid.find_path_jps(tier_pairs[t][s].ax, tier_pairs[t][s].ay, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                salt += (s32)cells.size();
            }
            c.stop_and_save();
            j_ns = (f64)c.cost_ns() / (f64)N;
        }
        {
            zclock<> c;
            c.start();
            for (s32 s = 0; s < N; s++)
            {
                grid.find_path_jps_plus(tier_pairs[t][s].ax, tier_pairs[t][s].ay, tier_pairs[t][s].bx, tier_pairs[t][s].by, cells);
                salt += (s32)cells.size();
            }
            c.stop_and_save();
            p_ns = (f64)c.cost_ns() / (f64)N;
        }
        LOGFMTI("phase5 obstacle[%s] %s: pairs=%d zpath=%.1fm apath=%.1fm | zgraph=%.0fns astar=%.0fns jps=%.0fns jps+=%.0fns",
                density_name, tier_names[t], N, tier_zlen[t] / (f64)N / 100.0, tier_alen[t] / (f64)N / 100.0,
                zg_ns, a_ns, j_ns, p_ns);
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
        s32 r = fresh.find_path_jps(6, 6, 6, 390, c);
        first.stop_and_save();
        ASSERT_TEST(r == 0, "light build first query fail r=", r);
        zclock<> steady;
        steady.start();
        r = fresh.find_path_jps(6, 6, 6, 390, c);
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
        ASSERT_TEST(fresh.init(200, 200, kCellSize, true) == 0, "lazy idx egrid fail");
        std::vector<s32> c;
        zclock<> first;
        first.start();
        s32 r = fresh.find_path_jps(1, 1, 198, 198, c);
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

int main(int argc, char* argv[])
{
    ztest_init();

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zjps_raster_rect_test()                       == 0);
    ASSERT_TEST(zjps_raster_segment_test()                    == 0);
    ASSERT_TEST(zjps_raster_circle_test()                     == 0);
    ASSERT_TEST(zjps_raster_polygon_test()                    == 0);
    ASSERT_TEST(zjps_corridor_consistency_test()              == 0);
    ASSERT_TEST(zjps_serpentine_consistency_test()            == 0);
    ASSERT_TEST(zjps_random_map_consistency_test()            == 0);
    ASSERT_TEST(zjps_astar_basic_test()                       == 0);
    ASSERT_TEST(zjps_astar_capacity_test()                    == 0);
    ASSERT_TEST(zjps_jps_basic_test()                         == 0);
    ASSERT_TEST(zjps_jps_capacity_test()                      == 0);
    ASSERT_TEST(zjps_bench_test()                             == 0);
    ASSERT_TEST(zjps_fcut_experiment_test()                   == 0);
    ASSERT_TEST(zjps_jps_plus_test()                          == 0);
    ASSERT_TEST(zjps_phase5_bench_test()                      == 0);

    LogInfo() << "all test finish .";
    return 0;
}
