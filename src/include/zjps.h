/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once
#ifndef  ZJPS_H
#define ZJPS_H

#include "zpoint.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>

static constexpr s32 kZjpsDirX[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static constexpr s32 kZjpsDirY[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

class zjps_grid
{
public:
    static constexpr s32 kCostStraight = 1000;
    static constexpr s32 kCostDiagonal = 1414;
    static constexpr s32 kDefaultOpenCnt = 1024;
    static constexpr u8 kDefaultVoxelHeight = 1;
    static constexpr s32 kTierAstar = -1;
    static constexpr s32 kTierScan = 0;
    static constexpr s32 kTierLight = 1;
    static constexpr s32 kTierPlus = 2;
    static constexpr s32 kDirEast = 0;
    static constexpr s32 kDirNorth = 2;
    static constexpr s32 kDirWest = 4;
    static constexpr s32 kDirSouth = 6;
    static constexpr size_t kPlusSlotCnt = 4;

    zjps_grid() = default;

    s32 init(s32 width, s32 height, f32 cell_size, bool walkable_default);

    s32 width() const { return width_; }
    s32 height() const { return height_; }
    f32 cell_size() const { return cell_size_; }
    u32 map_version() const { return map_version_; }
    u32 payload_version() const { return payload_version_; }

    bool cell_walkable(s32 x, s32 y) const
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_)
        {
            return false;
        }
        return cell_flag_[(size_t)y * (size_t)width_ + (size_t)x] != 0;
    }

    s32 set_cell(s32 x, s32 y, bool walkable);

    s32 set_rect_cell(s32 x0, s32 y0, s32 x1, s32 y1, bool walkable);

    s32 set_triangle_cell(const zpoint& a, const zpoint& b, const zpoint& c, bool walkable);

    s32 set_blocked(s32 x, s32 y)
    {
        return set_cell(x, y, false);
    }

    s32 set_walkable(s32 x, s32 y)
    {
        return set_cell(x, y, true);
    }

    s32 cell_height(s32 x, s32 y) const
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_)
        {
            return -1;
        }
        return (s32)cell_voxel_[(size_t)y * (size_t)width_ + (size_t)x];
    }

    s32 set_cell_height(s32 x, s32 y, u8 voxel_height);

    s32 pos_to_cell(f32 px, f32 py, s32& out_x, s32& out_y) const;

    s32 cell_to_pos(s32 x, s32 y, f32& out_x, f32& out_y) const
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_)
        {
            return -1;
        }
        out_x = ((f32)x + 0.5f) * cell_size_;
        out_y = ((f32)y + 0.5f) * cell_size_;
        return 0;
    }

    bool move_valid(s32 x, s32 y, s32 dx, s32 dy) const;

    s32 build_jps_light();

    s32 drop_jps_plus()
    {
        plus_table_.clear();
        plus_table_version_ = 0;
        return 0;
    }

    s32 build_jps_plus();

    s32 astar_search(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells);

    s32 jps_search(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells);

    s32 find_path(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
    {
        return jps_search(start_x, start_y, target_x, target_y, out_cells);
    }

    s32 set_open_capacity(s32 max_open_cnt)
    {
        if (max_open_cnt <= 0)
        {
            return -1;
        }
        open_capacity_ = max_open_cnt;
        open_heap_.reserve((size_t)max_open_cnt);
        return 0;
    }

    s32 open_capacity() const { return open_capacity_; }
    s32 light_dirty() const { return light_dirty_cnt_; }
    s32 last_tier() const { return last_tier_; }
    size_t jps_plus_table_bytes() const { return plus_table_.size() * sizeof(plus_ray); }
    s32 open_push_count() const { return open_push_count_; }
    s32 open_pop_count() const { return open_pop_count_; }
    s32 open_peak() const { return open_peak_; }
    s32 visit_count() const { return visit_count_; }
    s32 last_path_cost() const { return last_path_cost_; }

private:
    static constexpr s32 kFastProbeSteps = 8;
    static constexpr s32 kDirCnt = 8;
    static constexpr s32 kNoCell = -1;
    static constexpr s8 kNoDir = -1;

    struct search_state
    {
        s32 gone_cost = 0;
        s32 came_from = kNoCell;
        u32 stamp = 0;
        u8 closed = 0;
        s8 entry_dir = kNoDir;
    };

    struct plus_ray
    {
        s32 first_turn_cell = kNoCell;
        s32 first_block_cell = kNoCell;
    };

    struct block_line
    {
        const s32* blocks;
        s32 cnt;
    };

    struct block_line_mut
    {
        s32* blocks;
        s32* cnt;
    };

    struct heap_entry
    {
        s32 full_cost;
        s32 gone_cost;
        s32 cell;
    };

    static s32 dir_x(s32 d)
    {
        return kZjpsDirX[d];
    }

    static s32 dir_y(s32 d)
    {
        return kZjpsDirY[d];
    }

    static s32 dir_index(s32 dx, s32 dy);

    static bool dir_is_axis(s32 dx, s32 dy)
    {
        return dx == 0 || dy == 0;
    }

    static s32 octile_to(s32 x, s32 y, s32 target_x, s32 target_y)
    {
        s32 dx = x > target_x ? x - target_x : target_x - x;
        s32 dy = y > target_y ? y - target_y : target_y - y;
        s32 long_axis = dx > dy ? dx : dy;
        s32 short_axis = dx > dy ? dy : dx;
        return kCostStraight * long_axis + (kCostDiagonal - kCostStraight) * short_axis;
    }

    static bool heap_before(const heap_entry& a, const heap_entry& b)
    {
        if (a.full_cost != b.full_cost)
        {
            return a.full_cost < b.full_cost;
        }
        return a.gone_cost > b.gone_cost;
    }

    static s32 heap_push(std::vector<heap_entry>& heap, s32 capacity, s32 full_cost, s32 gone_cost, s32 cell);

    static s32 heap_pop(std::vector<heap_entry>& heap, heap_entry& out);

    s32 open_push(s32 full_cost, s32 gone_cost, s32 cell)
    {
        if (heap_push(open_heap_, open_capacity_, full_cost, gone_cost, cell) != 0)
        {
            return -1;
        }
        open_push_count_++;
        if ((s32)open_heap_.size() > open_peak_)
        {
            open_peak_ = (s32)open_heap_.size();
        }
        return 0;
    }

    bool open_pop(heap_entry& out)
    {
        if (heap_pop(open_heap_, out) != 0)
        {
            return false;
        }
        open_pop_count_++;
        return true;
    }

    static s32 sorted_lower_bound(const s32* vals, s32 cnt, s32 v);

    static s32 sorted_upper_bound(const s32* vals, s32 cnt, s32 v);

    static s32 sorted_insert(s32* vals, s32& cnt, s32 cap, s32 v);

    static s32 sorted_erase(s32* vals, s32& cnt, s32 v);

    static size_t block_run_end(const s32* blocks, size_t i, size_t cnt);

    static size_t block_run_start(const s32* blocks, size_t i);

    static s32* light_row_head(s32* rows, s32 width, s32 y)
    {
        return rows + (size_t)y * (size_t)(width + 1);
    }

    static s32* light_col_head(s32* cols, s32 height, s32 x)
    {
        return cols + (size_t)x * (size_t)(height + 1);
    }

    static block_line light_row_line(const s32* rows, s32 width, s32 y)
    {
        const s32* head = rows + (size_t)y * (size_t)(width + 1);
        return block_line{ head + 1, head[0] };
    }

    static block_line light_col_line(const s32* cols, s32 height, s32 x)
    {
        const s32* head = cols + (size_t)x * (size_t)(height + 1);
        return block_line{ head + 1, head[0] };
    }

    static block_line_mut light_row_line_mut(s32* rows, s32 width, s32 y)
    {
        s32* head = light_row_head(rows, width, y);
        return block_line_mut{ head + 1, head };
    }

    static block_line_mut light_col_line_mut(s32* cols, s32 height, s32 x)
    {
        s32* head = light_col_head(cols, height, x);
        return block_line_mut{ head + 1, head };
    }

    static s32 refill_light_row(const u8* cell_flag, s32* rows, s32 width, s32 y);

    static s32 refill_light_col(const u8* cell_flag, s32* cols, s32 width, s32 height, s32 x);

    static s32 mark_light_dirty_row(u8* dirty_rows, s32& dirty_cnt, s32 y)
    {
        if (dirty_rows[(size_t)y] == 0)
        {
            dirty_rows[(size_t)y] = 1;
            dirty_cnt++;
        }
        return 0;
    }

    static s32 mark_light_dirty_col(u8* dirty_cols, s32& dirty_cnt, s32 x)
    {
        if (dirty_cols[(size_t)x] == 0)
        {
            dirty_cols[(size_t)x] = 1;
            dirty_cnt++;
        }
        return 0;
    }

    static_assert(kZjpsDirX[kDirEast] == 1 && kZjpsDirY[kDirEast] == 0
        && kZjpsDirX[kDirNorth] == 0 && kZjpsDirY[kDirNorth] == 1
        && kZjpsDirX[kDirWest] == -1 && kZjpsDirY[kDirWest] == 0
        && kZjpsDirX[kDirSouth] == 0 && kZjpsDirY[kDirSouth] == -1,
        "axis dir codes must pair with kZjpsDirX/kZjpsDirY layout");

    static_assert(kDirEast / 2 == 0 && kDirNorth / 2 == 1
        && kDirWest / 2 == 2 && kDirSouth / 2 == 3 && kPlusSlotCnt == 4,
        "axis dirs must fill distinct plus slots by d/2");

    static size_t plus_slot(s32 cell_idx, s32 d)
    {
        return (size_t)cell_idx * kPlusSlotCnt + (size_t)(d / 2);
    }

    bool side_forced_row(s32 x, s32 y, s32 entry_dx, s32 side) const
    {
        return cell_walkable(x, y + side) && !cell_walkable(x - entry_dx, y + side);
    }

    bool side_forced_col(s32 x, s32 y, s32 entry_dy, s32 side) const
    {
        return cell_walkable(x + side, y) && !cell_walkable(x + side, y - entry_dy);
    }

    bool at_forced_turn(s32 x, s32 y, s32 entry_dx, s32 entry_dy) const;

    s32 successor_dirs(s32 x, s32 y, s32 entry_dir, s32* out_dirs) const;

    s32 rebuild_path(s32 target, std::vector<s32>& out_cells);

    s32 jump(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y, s32 tier) const;

    s32 probe_next_cell(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y, s32 tier) const;

    bool probe_has_next_cell(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y, s32 tier) const
    {
        return probe_next_cell(x, y, dx, dy, target_x, target_y, tier) >= 0;
    }

    s32 probe_next_cell_by_scan(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y) const;

    s32 probe_next_cell_by_light(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y) const;

    s32 probe_next_cell_by_real_light(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y) const;

    s32 probe_next_cell_by_real_light_row(s32 x, s32 y, s32 dx, s32 target_x, s32 target_y) const;

    s32 probe_next_cell_by_real_light_col(s32 x, s32 y, s32 dy, s32 target_x, s32 target_y) const;

    s32 straight_forced_turn_row(s32 y, s32 x, s32 dx, s32 reach_col) const;

    s32 straight_forced_turn_col(s32 x, s32 y, s32 dy, s32 reach_row) const;

    s32 probe_next_cell_by_plus(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y) const;

    bool probe_dir_hit_target_by_plus(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y) const;

    s32 width_ = 0;
    s32 height_ = 0;
    f32 cell_size_ = 0.0f;
    u8 default_walkable_ = 0;
    u32 map_version_ = 0;
    u32 payload_version_ = 0;
    std::vector<u8> cell_flag_;
    std::vector<u8> cell_voxel_;

    bool light_built_ = false;
    std::vector<u8> light_dirty_row_;
    std::vector<u8> light_dirty_col_;
    s32 light_dirty_cnt_ = 0;
    std::vector<s32> light_row_;
    std::vector<s32> light_col_;

    std::vector<plus_ray> plus_table_;
    u32 plus_table_version_ = 0;

    s32 open_capacity_ = kDefaultOpenCnt;
    std::vector<heap_entry> open_heap_;
    u32 search_stamp_ = 0;
    std::vector<search_state> search_states_;

    s32 open_push_count_ = 0;
    s32 open_pop_count_ = 0;
    s32 open_peak_ = 0;
    s32 visit_count_ = 0;
    s32 last_path_cost_ = 0;
    s32 last_tier_ = kTierScan;
};

inline s32 zjps_grid::init(s32 width, s32 height, f32 cell_size, bool walkable_default)
{
    if (width <= 0 || height <= 0 || cell_size <= 0.0f)
    {
        return -1;
    }
    width_ = width;
    height_ = height;
    cell_size_ = cell_size;
    default_walkable_ = walkable_default ? 1 : 0;

    size_t cell_cnt = (size_t)width_ * (size_t)height_;
    cell_flag_.assign(cell_cnt, default_walkable_);
    u8 voxel_init = kDefaultVoxelHeight;
    cell_voxel_.assign(cell_cnt, voxel_init);
    light_dirty_row_.assign((size_t)height_, 0);
    light_dirty_col_.assign((size_t)width_, 0);
    light_dirty_cnt_ = 0;
    
    light_row_.assign((size_t)height_ * (size_t)(width_ + 1), 0);
    light_col_.assign((size_t)width_ * (size_t)(height_ + 1), 0);

    search_states_.resize(cell_cnt);
    open_heap_.reserve((size_t)open_capacity_);

    map_version_++;
    light_built_ = false;
    plus_table_.clear();
    plus_table_version_ = 0;
    return 0;
}

inline s32 zjps_grid::set_cell(s32 x, s32 y, bool walkable)
{
    if (x < 0 || y < 0 || x >= width_ || y >= height_)
    {
        return -1;
    }
    size_t idx = (size_t)width_ * (size_t)y  + (size_t)x;
    u8 old = cell_flag_[idx];
    u8 next = walkable ? 1 : 0;
    if (old == next)
    {
        return 0;
    }
    cell_flag_[idx] = next;
    map_version_++;
    block_line_mut row = light_row_line_mut(light_row_.data(), width_, y);
    block_line_mut col = light_col_line_mut(light_col_.data(), height_, x);
    if (!light_built_)
    {
        //nothing todo
    }
    else if (light_dirty_row_[(size_t)y] != 0 || light_dirty_col_[(size_t)x] != 0)
    {
        mark_light_dirty_row(light_dirty_row_.data(), light_dirty_cnt_, y);
        mark_light_dirty_col(light_dirty_col_.data(), light_dirty_cnt_, x);
    }
    else if (next != 0)
    {
        sorted_erase(row.blocks, *row.cnt, x);
        sorted_erase(col.blocks, *col.cnt, y);
    }
    else if (sorted_insert(row.blocks, *row.cnt, width_, x) != 0
             || sorted_insert(col.blocks, *col.cnt, height_, y) != 0)
    {
        mark_light_dirty_row(light_dirty_row_.data(), light_dirty_cnt_, y);
        mark_light_dirty_col(light_dirty_col_.data(), light_dirty_cnt_, x);
    }
    return 0;
}

inline s32 zjps_grid::set_rect_cell(s32 x0, s32 y0, s32 x1, s32 y1, bool walkable)
{
    if (x0 < 0 || y0 < 0 || x0 > x1 || y0 > y1 || x1 >= width_ || y1 >= height_)
    {
        return -1;
    }
    u8 next = walkable ? 1 : 0;
    bool changed = false;
    for (s32 y = y0; y <= y1; y++)
    {
        u8* row = cell_flag_.data() + (size_t)y * (size_t)width_;
        for (s32 x = x0; x <= x1; x++)
        {
            if (row[x] != next)
            {
                row[x] = next;
                changed = true;
            }
        }
    }
    if (!changed)
    {
        return 0;
    }
    map_version_++;
    if (light_built_)
    {
        for (s32 y = y0; y <= y1; y++)
        {
            mark_light_dirty_row(light_dirty_row_.data(), light_dirty_cnt_, y);
        }
        for (s32 x = x0; x <= x1; x++)
        {
            mark_light_dirty_col(light_dirty_col_.data(), light_dirty_cnt_, x);
        }
    }
    return 0;
}

inline s32 zjps_grid::set_triangle_cell(const zpoint& a, const zpoint& b, const zpoint& c, bool walkable)
{
    if (cell_size_ <= 0.0f)
    {
        return -1;
    }
    f32 area = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
    if (area == 0.0f)
    {
        return -1;
    }
    f32 lo_x = a.x < b.x ? a.x : b.x;
    if (c.x < lo_x) lo_x = c.x;
    f32 hi_x = a.x > b.x ? a.x : b.x;
    if (c.x > hi_x) hi_x = c.x;
    f32 lo_y = a.y < b.y ? a.y : b.y;
    if (c.y < lo_y) lo_y = c.y;
    f32 hi_y = a.y > b.y ? a.y : b.y;
    if (c.y > hi_y) hi_y = c.y;
    s32 x0 = (s32)ceilf(lo_x / cell_size_ - 0.5f);
    s32 x1 = (s32)floorf(hi_x / cell_size_ - 0.5f);
    s32 y0 = (s32)ceilf(lo_y / cell_size_ - 0.5f);
    s32 y1 = (s32)floorf(hi_y / cell_size_ - 0.5f);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= width_) x1 = width_ - 1;
    if (y1 >= height_) y1 = height_ - 1;
    if (x0 > x1 || y0 > y1)
    {
        return 0;
    }
    u8 next = walkable ? 1 : 0;
    bool changed = false;
    for (s32 y = y0; y <= y1; y++)
    {
        f32 center_y = ((f32)y + 0.5f) * cell_size_;
        u8* row = cell_flag_.data() + (size_t)y * (size_t)width_;
        for (s32 x = x0; x <= x1; x++)
        {
            f32 center_x = ((f32)x + 0.5f) * cell_size_;
            f32 w0 = (b.x - a.x) * (center_y - a.y) - (center_x - a.x) * (b.y - a.y);
            f32 w1 = (c.x - b.x) * (center_y - b.y) - (center_x - b.x) * (c.y - b.y);
            f32 w2 = (a.x - c.x) * (center_y - c.y) - (center_x - c.x) * (a.y - c.y);
            bool inside = (area > 0.0f) ? (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
                                        : (w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f);
            if (inside && row[x] != next)
            {
                row[x] = next;
                changed = true;
            }
        }
    }
    if (!changed)
    {
        return 0;
    }
    map_version_++;
    if (light_built_)
    {
        for (s32 y = y0; y <= y1; y++)
        {
            mark_light_dirty_row(light_dirty_row_.data(), light_dirty_cnt_, y);
        }
        for (s32 x = x0; x <= x1; x++)
        {
            mark_light_dirty_col(light_dirty_col_.data(), light_dirty_cnt_, x);
        }
    }
    return 0;
}

inline s32 zjps_grid::set_cell_height(s32 x, s32 y, u8 voxel_height)
{
    if (x < 0 || y < 0 || x >= width_ || y >= height_)
    {
        return -1;
    }
    size_t idx = (size_t)y * (size_t)width_ + (size_t)x;
    if (cell_voxel_[idx] == voxel_height)
    {
        return 0;
    }
    cell_voxel_[idx] = voxel_height;
    payload_version_++;
    return 0;
}

inline s32 zjps_grid::pos_to_cell(f32 px, f32 py, s32& out_x, s32& out_y) const
{
    if (cell_size_ <= 0.0f)
    {
        return -1;
    }
    s32 x = (s32)floorf(px / cell_size_);
    s32 y = (s32)floorf(py / cell_size_);
    if (x < 0 || y < 0 || x >= width_ || y >= height_)
    {
        return -2;
    }
    out_x = x;
    out_y = y;
    return 0;
}

inline bool zjps_grid::move_valid(s32 x, s32 y, s32 dx, s32 dy) const
{
    if (dx == 0 && dy == 0)
    {
        return false;
    }
    if (!cell_walkable(x + dx, y + dy))
    {
        return false;
    }
    if (dx != 0 && dy != 0)
    {
        return cell_walkable(x + dx, y) && cell_walkable(x, y + dy);
    }
    return true;
}

inline s32 zjps_grid::astar_search(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
{
    out_cells.clear();
    if (start_x < 0 || start_y < 0 || start_x >= width_ || start_y >= height_
        || target_x < 0 || target_y < 0 || target_x >= width_ || target_y >= height_)
    {
        return -1;
    }
    s32 start_idx = start_y * width_ + start_x;
    s32 target_idx = target_y * width_ + target_x;
    if (cell_flag_[start_idx] == 0 || cell_flag_[target_idx] == 0)
    {
        return -2;
    }
    last_path_cost_ = 0;
    last_tier_ = kTierAstar;
    if (start_idx == target_idx)
    {
        out_cells.push_back(start_idx);
        return 0;
    }
    search_stamp_++;
    if (search_stamp_ == 0)
    {
        for (size_t i = 0; i < search_states_.size(); i++)
        {
            search_states_[i].stamp = 0;
        }
        search_stamp_ = 1;
    }
    open_heap_.clear();
    open_push_count_ = 0;
    open_pop_count_ = 0;
    open_peak_ = 0;
    visit_count_ = 0;

    search_states_[start_idx].gone_cost = 0;
    search_states_[start_idx].came_from = kNoCell;
    search_states_[start_idx].stamp = search_stamp_;
    search_states_[start_idx].closed = 0;
    visit_count_ = 1;
    s32 start_heuristic_cost = octile_to(start_x, start_y, target_x, target_y);
    if (open_push(start_heuristic_cost, 0, start_idx) != 0)
    {
        return -3;
    }

    heap_entry entry;
    while (open_pop(entry))
    {
        s32 cur = entry.cell;
        if (search_states_[cur].closed != 0)
        {
            continue;
        }
        search_states_[cur].closed = 1;
        if (cur == target_idx)
        {
            last_path_cost_ = search_states_[cur].gone_cost;
            while (cur >= 0)
            {
                out_cells.push_back(cur);
                cur = search_states_[cur].came_from;
            }
            std::reverse(out_cells.begin(), out_cells.end());
            return 0;
        }
        s32 x = cur % width_;
        s32 y = cur / width_;
        for (s32 d = 0; d < kDirCnt; d++)
        {
            s32 dx = dir_x(d);
            s32 dy = dir_y(d);
            if (!move_valid(x, y, dx, dy))
            {
                continue;
            }
            s32 next = (y + dy) * width_ + (x + dx);
            s32 next_gone_cost = search_states_[cur].gone_cost + ((dx != 0 && dy != 0) ? kCostDiagonal : kCostStraight);
            if (search_states_[next].stamp != search_stamp_ || next_gone_cost < search_states_[next].gone_cost)
            {
                search_states_[next].gone_cost = next_gone_cost;
                search_states_[next].came_from = cur;
                search_states_[next].stamp = search_stamp_;
                search_states_[next].closed = 0;
                visit_count_++;
                s32 heuristic_cost = octile_to(x + dx, y + dy, target_x, target_y);
                if (open_push(next_gone_cost + heuristic_cost, next_gone_cost, next) != 0)
                {
                    return -3;
                }
            }
        }
    }
    return -2;
}

inline s32 zjps_grid::build_jps_light()
{
    if (width_ <= 0 || height_ <= 0)
    {
        return -1;
    }
    if (light_built_ && light_dirty_cnt_ == 0)
    {
        return 0;
    }
    if (!light_built_)
    {
        for (s32 y = 0; y < height_; y++)
        {
            refill_light_row(cell_flag_.data(), light_row_.data(), width_, y);
        }
        for (s32 x = 0; x < width_; x++)
        {
            refill_light_col(cell_flag_.data(), light_col_.data(), width_, height_, x);
        }
        light_built_ = true;
        return 0;
    }
    for (s32 y = 0; y < height_; y++)
    {
        if (light_dirty_row_[(size_t)y] != 0)
        {
            refill_light_row(cell_flag_.data(), light_row_.data(), width_, y);
            light_dirty_row_[(size_t)y] = 0;
            light_dirty_cnt_--;
        }
    }
    for (s32 x = 0; x < width_; x++)
    {
        if (light_dirty_col_[(size_t)x] != 0)
        {
            refill_light_col(cell_flag_.data(), light_col_.data(), width_, height_, x);
            light_dirty_col_[(size_t)x] = 0;
            light_dirty_cnt_--;
        }
    }
    return 0;
}

inline s32 zjps_grid::build_jps_plus()
{
    if (width_ <= 0 || height_ <= 0)
    {
        return -1;
    }
    size_t cell_cnt = (size_t)width_ * (size_t)height_;
    size_t slot_cnt = cell_cnt * kPlusSlotCnt;
    if (plus_table_.capacity() < slot_cnt)
    {
        plus_table_.reserve(slot_cnt);
    }
    plus_table_.assign(slot_cnt, plus_ray());
    for (s32 y = 0; y < height_; y++)
    {
        for (s32 x = width_ - 2; x >= 0; x--)
        {
            s32 next = x + 1;
            size_t base = plus_slot(y * width_ + x, kDirEast);
            size_t next_base = plus_slot(y * width_ + next, kDirEast);
            if (cell_flag_[(size_t)y * (size_t)width_ + (size_t)next] == 0)
            {
                plus_table_[base].first_block_cell = y * width_ + next;
                continue;
            }
            if (at_forced_turn(next, y, 1, 0))
            {
                plus_table_[base].first_turn_cell = y * width_ + next;
            }
            else
            {
                plus_table_[base].first_turn_cell = plus_table_[next_base].first_turn_cell;
            }
            plus_table_[base].first_block_cell = plus_table_[next_base].first_block_cell;
        }
        for (s32 x = 1; x < width_; x++)
        {
            s32 next = x - 1;
            size_t base = plus_slot(y * width_ + x, kDirWest);
            size_t next_base = plus_slot(y * width_ + next, kDirWest);
            if (cell_flag_[(size_t)y * (size_t)width_ + (size_t)next] == 0)
            {
                plus_table_[base].first_block_cell = y * width_ + next;
                continue;
            }
            if (at_forced_turn(next, y, -1, 0))
            {
                plus_table_[base].first_turn_cell = y * width_ + next;
            }
            else
            {
                plus_table_[base].first_turn_cell = plus_table_[next_base].first_turn_cell;
            }
            plus_table_[base].first_block_cell = plus_table_[next_base].first_block_cell;
        }
    }
    for (s32 x = 0; x < width_; x++)
    {
        for (s32 y = height_ - 2; y >= 0; y--)
        {
            s32 next = y + 1;
            size_t base = plus_slot(y * width_ + x, kDirNorth);
            size_t next_base = plus_slot(next * width_ + x, kDirNorth);
            if (cell_flag_[(size_t)next * (size_t)width_ + (size_t)x] == 0)
            {
                plus_table_[base].first_block_cell = next * width_ + x;
                continue;
            }
            if (at_forced_turn(x, next, 0, 1))
            {
                plus_table_[base].first_turn_cell = next * width_ + x;
            }
            else
            {
                plus_table_[base].first_turn_cell = plus_table_[next_base].first_turn_cell;
            }
            plus_table_[base].first_block_cell = plus_table_[next_base].first_block_cell;
        }
        for (s32 y = 1; y < height_; y++)
        {
            s32 next = y - 1;
            size_t base = plus_slot(y * width_ + x, kDirSouth);
            size_t next_base = plus_slot(next * width_ + x, kDirSouth);
            if (cell_flag_[(size_t)next * (size_t)width_ + (size_t)x] == 0)
            {
                plus_table_[base].first_block_cell = next * width_ + x;
                continue;
            }
            if (at_forced_turn(x, next, 0, -1))
            {
                plus_table_[base].first_turn_cell = next * width_ + x;
            }
            else
            {
                plus_table_[base].first_turn_cell = plus_table_[next_base].first_turn_cell;
            }
            plus_table_[base].first_block_cell = plus_table_[next_base].first_block_cell;
        }
    }
    plus_table_version_ = map_version_;
    return 0;
}

inline s32 zjps_grid::rebuild_path(s32 target, std::vector<s32>& out_cells)
{
    s32 node = target;
    while (node >= 0)
    {
        out_cells.push_back(node);
        s32 parent = search_states_[node].came_from;
        if (parent < 0)
        {
            break;
        }
        s32 entry_dir = search_states_[node].entry_dir;
        s32 dx = dir_x(entry_dir);
        s32 dy = dir_y(entry_dir);
        s32 cx = node % width_ - dx;
        s32 cy = node / width_ - dy;
        while (cy * width_ + cx != parent)
        {
            if (cx < 0 || cy < 0 || cx >= width_ || cy >= height_)
            {
                return -1;
            }
            out_cells.push_back(cy * width_ + cx);
            cx -= dx;
            cy -= dy;
        }
        node = parent;
    }
    std::reverse(out_cells.begin(), out_cells.end());
    return 0;
}

inline s32 zjps_grid::jps_search(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
{
    out_cells.clear();
    if (start_x < 0 || start_y < 0 || start_x >= width_ || start_y >= height_
        || target_x < 0 || target_y < 0 || target_x >= width_ || target_y >= height_)
    {
        return -1;
    }
    s32 start_idx = start_y * width_ + start_x;
    s32 target_idx = target_y * width_ + target_x;
    if (cell_flag_[start_idx] == 0 || cell_flag_[target_idx] == 0)
    {
        return -2;
    }
    last_path_cost_ = 0;
    bool plus_ready = !plus_table_.empty() && plus_table_version_ == map_version_;
    bool light_ready = light_built_ && light_dirty_cnt_ == 0;
    last_tier_ = plus_ready ? kTierPlus : (light_ready ? kTierLight : kTierScan);
    if (start_idx == target_idx)
    {
        out_cells.push_back(start_idx);
        return 0;
    }
    search_stamp_++;
    if (search_stamp_ == 0)
    {
        for (size_t i = 0; i < search_states_.size(); i++)
        {
            search_states_[i].stamp = 0;
        }
        search_stamp_ = 1;
    }
    open_heap_.clear();
    open_push_count_ = 0;
    open_pop_count_ = 0;
    open_peak_ = 0;
    visit_count_ = 0;

    search_states_[start_idx].gone_cost = 0;
    search_states_[start_idx].came_from = kNoCell;
    search_states_[start_idx].stamp = search_stamp_;
    search_states_[start_idx].closed = 0;
    search_states_[start_idx].entry_dir = kNoDir;
    visit_count_ = 1;
    s32 start_heuristic_cost = octile_to(start_x, start_y, target_x, target_y);
    if (open_push(start_heuristic_cost, 0, start_idx) != 0)
    {
        return -3;
    }

    s32 dirs[8];
    heap_entry entry;
    while (open_pop(entry))
    {
        s32 cur = entry.cell;
        if (search_states_[cur].closed != 0)
        {
            continue;
        }
        search_states_[cur].closed = 1;
        if (cur == target_idx)
        {
            last_path_cost_ = search_states_[cur].gone_cost;
            return rebuild_path(cur, out_cells);
        }
        s32 x = cur % width_;
        s32 y = cur / width_;
        s32 entry_dir = search_states_[cur].entry_dir;
        s32 dir_cnt = successor_dirs(x, y, entry_dir, dirs);
        for (s32 k = 0; k < dir_cnt; k++)
        {
            s32 d = dirs[k];
            s32 dx = dir_x(d);
            s32 dy = dir_y(d);
            s32 jump_cell = jump(x, y, dx, dy, target_x, target_y, last_tier_);
            if (jump_cell < 0)
            {
                continue;
            }
            s32 jump_x = jump_cell % width_;
            s32 jump_y = jump_cell / width_;
            s32 next_gone_cost = search_states_[cur].gone_cost + octile_to(x, y, jump_x, jump_y);
            if (search_states_[jump_cell].stamp != search_stamp_ || next_gone_cost < search_states_[jump_cell].gone_cost)
            {
                search_states_[jump_cell].gone_cost = next_gone_cost;
                search_states_[jump_cell].came_from = cur;
                search_states_[jump_cell].entry_dir = (s8)d;
                search_states_[jump_cell].stamp = search_stamp_;
                search_states_[jump_cell].closed = 0;
                visit_count_++;
                s32 heuristic_cost = octile_to(jump_x, jump_y, target_x, target_y);
                if (open_push(next_gone_cost + heuristic_cost, next_gone_cost, jump_cell) != 0)
                {
                    return -3;
                }
            }
        }
    }
    return -2;
}

inline s32 zjps_grid::sorted_lower_bound(const s32* vals, s32 cnt, s32 v)
{
    s32 lo = 0;
    s32 hi = cnt;
    while (lo < hi)
    {
        s32 mid = lo + (hi - lo) / 2;
        if (vals[mid] < v)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    return lo;
}

inline s32 zjps_grid::sorted_upper_bound(const s32* vals, s32 cnt, s32 v)
{
    s32 lo = 0;
    s32 hi = cnt;
    while (lo < hi)
    {
        s32 mid = lo + (hi - lo) / 2;
        if (vals[mid] <= v)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    return lo;
}

inline s32 zjps_grid::refill_light_row(const u8* cell_flag, s32* rows, s32 width, s32 y)
{
    const u8* row = cell_flag + (size_t)y * (size_t)width;
    block_line_mut line = light_row_line_mut(rows, width, y);
    s32 cnt = 0;
    for (s32 x = 0; x < width; x++)
    {
        if (row[x] == 0)
        {
            line.blocks[cnt++] = x;
        }
    }
    *line.cnt = cnt;
    return 0;
}

inline s32 zjps_grid::refill_light_col(const u8* cell_flag, s32* cols, s32 width, s32 height, s32 x)
{
    block_line_mut line = light_col_line_mut(cols, height, x);
    s32 cnt = 0;
    for (s32 y = 0; y < height; y++)
    {
        if (cell_flag[(size_t)y * (size_t)width + (size_t)x] == 0)
        {
            line.blocks[cnt++] = y;
        }
    }
    *line.cnt = cnt;
    return 0;
}

inline s32 zjps_grid::sorted_insert(s32* vals, s32& cnt, s32 cap, s32 v)
{
    s32 pos = sorted_lower_bound(vals, cnt, v);
    if (pos < cnt && vals[pos] == v)
    {
        return 0;
    }
    if (cnt >= cap)
    {
        return -1;
    }
    if (pos < cnt)
    {
        memmove(vals + pos + 1, vals + pos, (size_t)(cnt - pos) * sizeof(s32));
    }
    vals[pos] = v;
    cnt++;
    return 0;
}

inline s32 zjps_grid::sorted_erase(s32* vals, s32& cnt, s32 v)
{
    s32 pos = sorted_lower_bound(vals, cnt, v);
    if (pos >= cnt || vals[pos] != v)
    {
        return 0;
    }
    if (pos + 1 < cnt)
    {
        memmove(vals + pos, vals + pos + 1, (size_t)(cnt - pos - 1) * sizeof(s32));
    }
    cnt--;
    return 0;
}

inline s32 zjps_grid::heap_push(std::vector<heap_entry>& heap, s32 capacity, s32 full_cost, s32 gone_cost, s32 cell)
{
    if ((s32)heap.size() >= capacity)
    {
        return -1;
    }
    heap_entry entry;
    entry.full_cost = full_cost;
    entry.gone_cost = gone_cost;
    entry.cell = cell;
    heap.push_back(entry);
    size_t child = heap.size() - 1;
    while (child > 0)
    {
        size_t parent = (child - 1) / 2;
        if (!heap_before(heap[child], heap[parent]))
        {
            break;
        }
        heap_entry tmp = heap[parent];
        heap[parent] = heap[child];
        heap[child] = tmp;
        child = parent;
    }
    return 0;
}

inline s32 zjps_grid::heap_pop(std::vector<heap_entry>& heap, heap_entry& out)
{
    if (heap.empty())
    {
        return -1;
    }
    out = heap.front();
    heap.front() = heap.back();
    heap.pop_back();
    size_t parent = 0;
    while (true)
    {
        size_t left = parent * 2 + 1;
        size_t right = left + 1;
        size_t best = parent;
        if (left < heap.size() && heap_before(heap[left], heap[best]))
        {
            best = left;
        }
        if (right < heap.size() && heap_before(heap[right], heap[best]))
        {
            best = right;
        }
        if (best == parent)
        {
            break;
        }
        heap_entry tmp = heap[parent];
        heap[parent] = heap[best];
        heap[best] = tmp;
        parent = best;
    }
    return 0;
}

inline s32 zjps_grid::dir_index(s32 dx, s32 dy)
{
    if (dx > 0)
    {
        return dy > 0 ? 1 : (dy < 0 ? 7 : 0);
    }
    if (dx < 0)
    {
        return dy > 0 ? 3 : (dy < 0 ? 5 : 4);
    }
    return dy > 0 ? 2 : 6;
}

inline bool zjps_grid::at_forced_turn(s32 x, s32 y, s32 entry_dx, s32 entry_dy) const
{
    if (!dir_is_axis(entry_dx, entry_dy))
    {
        return false;
    }
    if (entry_dx != 0)
    {
        return side_forced_row(x, y, entry_dx, -1)
            || side_forced_row(x, y, entry_dx, 1);
    }
    return side_forced_col(x, y, entry_dy, -1)
        || side_forced_col(x, y, entry_dy, 1);
}

inline s32 zjps_grid::jump(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y, s32 tier) const
{
    if (dir_is_axis(dx, dy))
    {
        //轴方向 返回目标或拐点中先遇到者 探空则该方向无后继 
        return probe_next_cell(x, y, dx, dy, target_x, target_y, tier);
    }

    //斜线 踏上目标返回目标 每步轴探测看见拐点或目标则返回当前步 看见的点由下一跳重新发现  
    while (true)
    {
        if (!move_valid(x, y, dx, dy))
        {
            return kNoCell;
        }
        x += dx;
        y += dy;
        if (x == target_x && y == target_y)
        {
            return y * width_ + x;
        }
        if (at_forced_turn(x, y, dx, dy))
        {
            return y * width_ + x;
        }
        if (probe_has_next_cell(x, y, dx, 0, target_x, target_y, tier))
        {
            return y * width_ + x;
        }
        if (probe_has_next_cell(x, y, 0, dy, target_x, target_y, tier))
        {
            return y * width_ + x;
        }
    }
}

inline s32 zjps_grid::probe_next_cell(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y, s32 tier) const
{
    if (!dir_is_axis(dx, dy))
    {
        return kNoCell;
    }
    if (tier == kTierPlus)
    {
        return probe_next_cell_by_plus(x, y, dx, dy, target_x, target_y);
    }
    if (tier == kTierLight)
    {
        return probe_next_cell_by_light(x, y, dx, dy, target_x, target_y);
    }
    return probe_next_cell_by_scan(x, y, dx, dy, target_x, target_y);
}

inline s32 zjps_grid::probe_next_cell_by_scan(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y) const
{
    while (true)
    {
        if (!move_valid(x, y, dx, dy))
        {
            return kNoCell;
        }
        x += dx;
        y += dy;
        if (x == target_x && y == target_y)
        {
            return y * width_ + x;
        }
        if (at_forced_turn(x, y, dx, dy))
        {
            return y * width_ + x;
        }
    }
}

inline s32 zjps_grid::probe_next_cell_by_light(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y) const
{
    s32 fast_probe_left = kFastProbeSteps;
    while (fast_probe_left > 0)
    {
        if (!move_valid(x, y, dx, dy))
        {
            return kNoCell;
        }
        x += dx;
        y += dy;
        if (x == target_x && y == target_y)
        {
            return y * width_ + x;
        }
        if (at_forced_turn(x, y, dx, dy))
        {
            return y * width_ + x;
        }
        fast_probe_left--;
    }
    return probe_next_cell_by_real_light(x, y, dx, dy, target_x, target_y);
}

inline s32 zjps_grid::probe_next_cell_by_real_light(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y) const
{
    if (dx != 0)
    {
        return probe_next_cell_by_real_light_row(x, y, dx, target_x, target_y);
    }
    return probe_next_cell_by_real_light_col(x, y, dy, target_x, target_y);
}

inline s32 zjps_grid::probe_next_cell_by_real_light_row(s32 x, s32 y, s32 dx, s32 target_x, s32 target_y) const
{
    block_line row = light_row_line(light_row_.data(), width_, y);
    s32 reach_col;
    if (true)
    {
        s32 block_at;
        if (dx > 0)
        {
            s32 pos = sorted_lower_bound(row.blocks, row.cnt, x + 1);
            block_at = pos < row.cnt ? row.blocks[pos] : -1;
        }
        else
        {
            s32 pos = sorted_upper_bound(row.blocks, row.cnt, x - 1);
            block_at = pos > 0 ? row.blocks[pos - 1] : -1;
        }
        reach_col = block_at < 0 ? (dx > 0 ? width_ - 1 : 0) : block_at - dx;
        if (dx > 0 && reach_col > width_ - 1)
        {
            reach_col = width_ - 1;
        }
        if (dx < 0 && reach_col < 0)
        {
            reach_col = 0;
        }
    }
    s32 jump_col;
    if (true)
    {
        jump_col = -1;
        if (y == target_y
            && ((dx > 0 && target_x > x && target_x <= reach_col) || (dx < 0 && target_x < x && target_x >= reach_col)))
        {
            jump_col = target_x;
        }
    }
    if (true)
    {
        s32 forced = straight_forced_turn_row(y, x, dx, reach_col);
        if (forced >= 0 && (jump_col < 0 || (dx > 0 ? forced < jump_col : forced > jump_col)))
        {
            jump_col = forced;
        }
    }
    if (jump_col < 0)
    {
        return kNoCell;
    }
    return y * width_ + jump_col;
}

inline s32 zjps_grid::probe_next_cell_by_real_light_col(s32 x, s32 y, s32 dy, s32 target_x, s32 target_y) const
{
    block_line col = light_col_line(light_col_.data(), height_, x);
    s32 reach_row;
    if (true)
    {
        s32 block_at;
        if (dy > 0)
        {
            s32 pos = sorted_lower_bound(col.blocks, col.cnt, y + 1);
            block_at = pos < col.cnt ? col.blocks[pos] : -1;
        }
        else
        {
            s32 pos = sorted_upper_bound(col.blocks, col.cnt, y - 1);
            block_at = pos > 0 ? col.blocks[pos - 1] : -1;
        }
        reach_row = block_at < 0 ? (dy > 0 ? height_ - 1 : 0) : block_at - dy;
        if (dy > 0 && reach_row > height_ - 1)
        {
            reach_row = height_ - 1;
        }
        if (dy < 0 && reach_row < 0)
        {
            reach_row = 0;
        }
    }
    s32 jump_row;
    if (true)
    {
        jump_row = -1;
        if (x == target_x
            && ((dy > 0 && target_y > y && target_y <= reach_row) || (dy < 0 && target_y < y && target_y >= reach_row)))
        {
            jump_row = target_y;
        }
    }
    if (true)
    {
        s32 forced = straight_forced_turn_col(x, y, dy, reach_row);
        if (forced >= 0 && (jump_row < 0 || (dy > 0 ? forced < jump_row : forced > jump_row)))
        {
            jump_row = forced;
        }
    }
    if (jump_row < 0)
    {
        return kNoCell;
    }
    return jump_row * width_ + x;
}

inline s32 zjps_grid::straight_forced_turn_row(s32 y, s32 x, s32 dx, s32 reach_col) const
{
    s32 nearest_turn = -1;
    for (s32 side = -1; side <= 1; side += 2)
    {
        s32 side_y = y + side;
        if (side_y < 0 || side_y >= height_)
        {
            continue;
        }
        block_line line = light_row_line(light_row_.data(), width_, side_y);
        if (dx > 0)
        {
            s32 block_idx = sorted_lower_bound(line.blocks, line.cnt, x);
            if (block_idx >= line.cnt)
            {
                continue;
            }
            size_t run_end = block_run_end(line.blocks, (size_t)block_idx, (size_t)line.cnt);
            s32 stop_col = line.blocks[run_end] + 1;
            if (stop_col >= width_ || stop_col > reach_col)
            {
                continue;
            }
            if (nearest_turn < 0 || stop_col < nearest_turn)
            {
                nearest_turn = stop_col;
            }
        }
        else
        {
            s32 block_idx = sorted_upper_bound(line.blocks, line.cnt, x);
            if (block_idx <= 0)
            {
                continue;
            }
            size_t run_start = block_run_start(line.blocks, (size_t)(block_idx - 1));
            s32 stop_col = line.blocks[run_start] - 1;
            if (stop_col < 0 || stop_col < reach_col)
            {
                continue;
            }
            if (nearest_turn < 0 || stop_col > nearest_turn)
            {
                nearest_turn = stop_col;
            }
        }
    }
    return nearest_turn;
}

inline s32 zjps_grid::straight_forced_turn_col(s32 x, s32 y, s32 dy, s32 reach_row) const
{
    s32 nearest_turn = -1;
    for (s32 side = -1; side <= 1; side += 2)
    {
        s32 side_x = x + side;
        if (side_x < 0 || side_x >= width_)
        {
            continue;
        }
        block_line line = light_col_line(light_col_.data(), height_, side_x);
        if (dy > 0)
        {
            s32 block_idx = sorted_lower_bound(line.blocks, line.cnt, y);
            if (block_idx >= line.cnt)
            {
                continue;
            }
            size_t run_end = block_run_end(line.blocks, (size_t)block_idx, (size_t)line.cnt);
            s32 stop_row = line.blocks[run_end] + 1;
            if (stop_row >= height_ || stop_row > reach_row)
            {
                continue;
            }
            if (nearest_turn < 0 || stop_row < nearest_turn)
            {
                nearest_turn = stop_row;
            }
        }
        else
        {
            s32 block_idx = sorted_upper_bound(line.blocks, line.cnt, y);
            if (block_idx <= 0)
            {
                continue;
            }
            size_t run_start = block_run_start(line.blocks, (size_t)(block_idx - 1));
            s32 stop_row = line.blocks[run_start] - 1;
            if (stop_row < 0 || stop_row < reach_row)
            {
                continue;
            }
            if (nearest_turn < 0 || stop_row > nearest_turn)
            {
                nearest_turn = stop_row;
            }
        }
    }
    return nearest_turn;
}

inline s32 zjps_grid::successor_dirs(s32 x, s32 y, s32 entry_dir, s32* out_dirs) const
{
    s32 cnt = 0;
    if (entry_dir < 0)
    {
        for (s32 k = 0; k < kDirCnt; k++)
        {
            out_dirs[cnt++] = k;
        }
        return cnt;
    }
    s32 dx = dir_x(entry_dir);
    s32 dy = dir_y(entry_dir);
    if (!dir_is_axis(dx, dy))
    {
        bool walk_x = cell_walkable(x + dx, y);
        bool walk_y = cell_walkable(x, y + dy);
        if (walk_x)
        {
            out_dirs[cnt++] = dir_index(dx, 0);
        }
        if (walk_y)
        {
            out_dirs[cnt++] = dir_index(0, dy);
        }
        if (walk_x && walk_y)
        {
            out_dirs[cnt++] = entry_dir;
        }
        return cnt;
    }
    if (dx != 0)
    {
        bool walk_ahead = cell_walkable(x + dx, y);
        if (walk_ahead)
        {
            out_dirs[cnt++] = entry_dir;
        }
        for (s32 side = -1; side <= 1; side += 2)
        {
            if (side_forced_row(x, y, dx, side))
            {
                out_dirs[cnt++] = dir_index(0, side);
                if (walk_ahead)
                {
                    out_dirs[cnt++] = dir_index(dx, side);
                }
            }
        }
        return cnt;
    }
    bool walk_ahead = cell_walkable(x, y + dy);
    if (walk_ahead)
    {
        out_dirs[cnt++] = entry_dir;
    }
    for (s32 side = -1; side <= 1; side += 2)
    {
        if (side_forced_col(x, y, dy, side))
        {
            out_dirs[cnt++] = dir_index(side, 0);
            if (walk_ahead)
            {
                out_dirs[cnt++] = dir_index(side, dy);
            }
        }
    }
    return cnt;
}

inline s32 zjps_grid::probe_next_cell_by_plus(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y) const
{
    if (probe_dir_hit_target_by_plus(x, y, dx, dy, target_x, target_y))
    {
        return target_y * width_ + target_x;
    }
    return plus_table_[plus_slot(y * width_ + x, dir_index(dx, dy))].first_turn_cell;
}

inline bool zjps_grid::probe_dir_hit_target_by_plus(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y) const
{
    if (!dir_is_axis(dx, dy))
    {
        return false;
    }
    if (dx != 0)
    {
        if (target_y != y || (target_x - x) * dx <= 0)
        {
            return false;
        }
    }
    else
    {
        if (target_x != x || (target_y - y) * dy <= 0)
        {
            return false;
        }
    }
    size_t slot = plus_slot(y * width_ + x, dir_index(dx, dy));
    s32 jump_cell = plus_table_[slot].first_turn_cell;
    s32 stop = plus_table_[slot].first_block_cell;
    s32 target_axis = (dx != 0) ? target_x : target_y;
    s32 step = (dx != 0) ? dx : dy;
    if (jump_cell >= 0)
    {
        s32 jump_coord = (dx != 0) ? (jump_cell % width_) : (jump_cell / width_);
        return (target_axis - jump_coord) * step <= 0;
    }
    if (stop >= 0)
    {
        s32 stop_axis = (dx != 0) ? (stop % width_) : (stop / width_);
        return (target_axis - stop_axis) * step < 0;
    }
    return true;
}

inline size_t zjps_grid::block_run_end(const s32* blocks, size_t i, size_t cnt)
{
    s32 target = blocks[i] - (s32)i;
    size_t lo = i + 1;
    size_t hi = cnt;
    while (lo < hi)
    {
        size_t mid = lo + (hi - lo) / 2;
        if (blocks[mid] - (s32)mid == target)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    return lo - 1;
}

inline size_t zjps_grid::block_run_start(const s32* blocks, size_t i)
{
    s32 target = blocks[i] - (s32)i;
    size_t lo = 0;
    size_t hi = i;
    while (lo < hi)
    {
        size_t mid = lo + (hi - lo) / 2;
        if (blocks[mid] - (s32)mid == target)
        {
            hi = mid;
        }
        else
        {
            lo = mid + 1;
        }
    }
    return lo;
}


#endif
