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
    size_t jps_plus_table_bytes() const { return jump_table_.size() * sizeof(jump_entry); }
    s32 open_push_count() const { return open_push_count_; }
    s32 open_pop_count() const { return open_pop_count_; }
    s32 open_peak() const { return open_peak_; }
    s32 visit_count() const { return visit_count_; }
    s32 last_path_cost() const { return last_path_cost_; }

    s32 path_search(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells);

    s32 find_path(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
    {
        return jps_search(start_x, start_y, target_x, target_y, out_cells);
    }

    s32 build_jps_light();

    s32 drop_jps_plus()
    {
        jump_table_.clear();
        jump_build_version_ = 0;
        return 0;
    }

    s32 build_jps_plus();

    s32 jps_search(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells);

private:
    static constexpr s32 kScanProbeSteps = 8;

    static s32 line_lower(const s32* vals, s32 cnt, s32 v);

    static s32 line_upper(const s32* vals, s32 cnt, s32 v);

    s32 mark_light_dirty_row(s32 y)
    {
        if (light_dirty_row_[(size_t)y] == 0)
        {
            light_dirty_row_[(size_t)y] = 1;
            light_dirty_cnt_++;
        }
        return 0;
    }

    s32 mark_light_dirty_col(s32 x)
    {
        if (light_dirty_col_[(size_t)x] == 0)
        {
            light_dirty_col_[(size_t)x] = 1;
            light_dirty_cnt_++;
        }
        return 0;
    }

    s32 refill_light_row(s32 y);

    s32 refill_light_col(s32 x);

    static s32 line_insert(s32* vals, s32& cnt, s32 cap, s32 v);

    static s32 line_remove(s32* vals, s32& cnt, s32 v);

    s32* light_row_head(s32 y)
    {
        return light_row_.data() + (size_t)y * (size_t)(width_ + 1);
    }

    s32* light_col_head(s32 x)
    {
        return light_col_.data() + (size_t)x * (size_t)(height_ + 1);
    }

    struct search_state
    {
        s32 cost = 0;
        s32 came_from = -1;
        u32 stamp = 0;
        u8 closed = 0;
        u8 dir = 0;
    };

    struct jump_entry
    {
        s32 jump = -1;
        s32 stop = -1;
    };

    struct heap_entry
    {
        s32 f;
        s32 g;
        s32 cell;
    };

    static constexpr s32 kDirCnt = 8;

    static s32 dir_x(s32 d)
    {
        const s32 dirs[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
        return dirs[d];
    }

    static s32 dir_y(s32 d)
    {
        const s32 dirs[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
        return dirs[d];
    }

    static s32 octile_to(s32 x, s32 y, s32 target_x, s32 target_y)
    {
        s32 dx = x > target_x ? x - target_x : target_x - x;
        s32 dy = y > target_y ? y - target_y : target_y - y;
        s32 mx = dx > dy ? dx : dy;
        s32 mn = dx > dy ? dy : dx;
        return kCostStraight * mx + (kCostDiagonal - kCostStraight) * mn;
    }

    static bool heap_before(const heap_entry& a, const heap_entry& b)
    {
        if (a.f != b.f)
        {
            return a.f < b.f;
        }
        return a.g > b.g;
    }

    static s32 heap_push(std::vector<heap_entry>& heap, s32 capacity, s32 f, s32 g, s32 cell);

    static s32 heap_pop(std::vector<heap_entry>& heap, heap_entry& out);

    static s32 dir_index(s32 dx, s32 dy);

    static size_t plus_slot(s32 cell_idx, s32 d)
    {
        return (size_t)cell_idx * kPlusSlotCnt + (size_t)(d / 2);
    }

    bool has_forced(s32 x, s32 y, s32 dx, s32 dy);

    s32 jump(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y);

    s32 jump_straight(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y);

    s32 jump_straight_indexed(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y);

    s32 straight_forced_stop_row(s32 y, s32 x, s32 dx, s32 c_max);

    s32 straight_forced_stop_col(s32 x, s32 y, s32 dy, s32 r_max);

    s32 successor_dirs(s32 x, s32 y, s32 d, s32* out_dirs);

    s32 jump_table_lookup(s32 cell_idx, s32 d, s32 target_x, s32 target_y);

    bool ray_reaches_target(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y);

    s32 jump_diag_plus(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y);

    static size_t block_run_end(const s32* blocks, size_t i, size_t cnt);

    static size_t block_run_start(const s32* blocks, size_t i);

    s32 width_ = 0;
    s32 height_ = 0;
    f32 cell_size_ = 0.0f;
    u8 default_walkable_ = 0;
    u32 map_version_ = 0;
    u32 payload_version_ = 0;
    std::vector<u8> cell_flag_;
    std::vector<u8> cell_voxel_;

    s32 open_capacity_ = kDefaultOpenCnt;
    u32 search_stamp_ = 0;
    bool index_built_ = false;
    std::vector<u8> light_dirty_row_;
    std::vector<u8> light_dirty_col_;
    s32 light_dirty_cnt_ = 0;
    s32 last_tier_ = kTierScan;
    std::vector<s32> light_row_;
    std::vector<s32> light_col_;
    std::vector<jump_entry> jump_table_;
    u32 jump_build_version_ = 0;
    std::vector<heap_entry> open_heap_;
    std::vector<search_state> states_;
    s32 open_push_count_ = 0;
    s32 open_pop_count_ = 0;
    s32 open_peak_ = 0;
    s32 visit_count_ = 0;
    s32 last_path_cost_ = 0;
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

    states_.resize(cell_cnt);
    open_heap_.reserve((size_t)open_capacity_);

    map_version_++;
    index_built_ = false;
    jump_table_.clear();
    jump_build_version_ = 0;
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
    if (index_built_)
    {
        if (light_dirty_row_[(size_t)y] == 0 && light_dirty_col_[(size_t)x] == 0)
        {
            s32* row = light_row_head(y);
            s32* col = light_col_head(x);
            if (next != 0)
            {
                line_remove(row + 1, row[0], x);
                line_remove(col + 1, col[0], y);
            }
            else if (line_insert(row + 1, row[0], width_, x) != 0
                     || line_insert(col + 1, col[0], height_, y) != 0)
            {
                mark_light_dirty_row(y);
                mark_light_dirty_col(x);
            }
        }
        else
        {
            mark_light_dirty_row(y);
            mark_light_dirty_col(x);
        }
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
    if (index_built_)
    {
        for (s32 y = y0; y <= y1; y++)
        {
            mark_light_dirty_row(y);
        }
        for (s32 x = x0; x <= x1; x++)
        {
            mark_light_dirty_col(x);
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
    if (index_built_)
    {
        for (s32 y = y0; y <= y1; y++)
        {
            mark_light_dirty_row(y);
        }
        for (s32 x = x0; x <= x1; x++)
        {
            mark_light_dirty_col(x);
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

inline s32 zjps_grid::path_search(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
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
        for (size_t i = 0; i < states_.size(); i++)
        {
            states_[i].stamp = 0;
        }
        search_stamp_ = 1;
    }
    open_heap_.clear();
    open_push_count_ = 0;
    open_pop_count_ = 0;
    open_peak_ = 0;
    visit_count_ = 0;

    states_[start_idx].cost = 0;
    states_[start_idx].came_from = -1;
    states_[start_idx].stamp = search_stamp_;
    states_[start_idx].closed = 0;
    visit_count_ = 1;
    if (heap_push(open_heap_, open_capacity_, octile_to(start_x, start_y, target_x, target_y), 0, start_idx) != 0)
    {
        return -3;
    }
    open_push_count_++;
    if ((s32)open_heap_.size() > open_peak_)
    {
        open_peak_ = (s32)open_heap_.size();
    }

    heap_entry entry;
    while (heap_pop(open_heap_, entry) == 0)
    {
        open_pop_count_++;
        s32 cur = entry.cell;
        if (states_[cur].closed != 0)
        {
            continue;
        }
        states_[cur].closed = 1;
        if (cur == target_idx)
        {
            last_path_cost_ = states_[cur].cost;
            while (cur >= 0)
            {
                out_cells.push_back(cur);
                cur = states_[cur].came_from;
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
            s32 next_cost = states_[cur].cost + ((dx != 0 && dy != 0) ? kCostDiagonal : kCostStraight);
            if (states_[next].stamp != search_stamp_ || next_cost < states_[next].cost)
            {
                states_[next].cost = next_cost;
                states_[next].came_from = cur;
                states_[next].stamp = search_stamp_;
                states_[next].closed = 0;
                visit_count_++;
                if (heap_push(open_heap_, open_capacity_, next_cost + octile_to(x + dx, y + dy, target_x, target_y), next_cost, next) != 0)
                {
                    return -3;
                }
                open_push_count_++;
                if ((s32)open_heap_.size() > open_peak_)
                {
                    open_peak_ = (s32)open_heap_.size();
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
    if (index_built_ && light_dirty_cnt_ == 0)
    {
        return 0;
    }
    if (!index_built_)
    {
        for (s32 y = 0; y < height_; y++)
        {
            refill_light_row(y);
        }
        for (s32 x = 0; x < width_; x++)
        {
            refill_light_col(x);
        }
        index_built_ = true;
        return 0;
    }
    for (s32 y = 0; y < height_; y++)
    {
        if (light_dirty_row_[(size_t)y] != 0)
        {
            refill_light_row(y);
            light_dirty_row_[(size_t)y] = 0;
            light_dirty_cnt_--;
        }
    }
    for (s32 x = 0; x < width_; x++)
    {
        if (light_dirty_col_[(size_t)x] != 0)
        {
            refill_light_col(x);
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
    if (jump_table_.capacity() < slot_cnt)
    {
        jump_table_.reserve(slot_cnt);
    }
    jump_table_.assign(slot_cnt, jump_entry());
    for (s32 y = 0; y < height_; y++)
    {
        for (s32 x = width_ - 2; x >= 0; x--)
        {
            s32 next = x + 1;
            size_t base = plus_slot(y * width_ + x, kDirEast);
            size_t next_base = plus_slot(y * width_ + next, kDirEast);
            if (cell_flag_[(size_t)y * (size_t)width_ + (size_t)next] == 0)
            {
                jump_table_[base].stop = y * width_ + next;
                continue;
            }
            if (has_forced(next, y, 1, 0))
            {
                jump_table_[base].jump = y * width_ + next;
            }
            else
            {
                jump_table_[base].jump = jump_table_[next_base].jump;
            }
            jump_table_[base].stop = jump_table_[next_base].stop;
        }
        for (s32 x = 1; x < width_; x++)
        {
            s32 next = x - 1;
            size_t base = plus_slot(y * width_ + x, kDirWest);
            size_t next_base = plus_slot(y * width_ + next, kDirWest);
            if (cell_flag_[(size_t)y * (size_t)width_ + (size_t)next] == 0)
            {
                jump_table_[base].stop = y * width_ + next;
                continue;
            }
            if (has_forced(next, y, -1, 0))
            {
                jump_table_[base].jump = y * width_ + next;
            }
            else
            {
                jump_table_[base].jump = jump_table_[next_base].jump;
            }
            jump_table_[base].stop = jump_table_[next_base].stop;
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
                jump_table_[base].stop = next * width_ + x;
                continue;
            }
            if (has_forced(x, next, 0, 1))
            {
                jump_table_[base].jump = next * width_ + x;
            }
            else
            {
                jump_table_[base].jump = jump_table_[next_base].jump;
            }
            jump_table_[base].stop = jump_table_[next_base].stop;
        }
        for (s32 y = 1; y < height_; y++)
        {
            s32 next = y - 1;
            size_t base = plus_slot(y * width_ + x, kDirSouth);
            size_t next_base = plus_slot(next * width_ + x, kDirSouth);
            if (cell_flag_[(size_t)next * (size_t)width_ + (size_t)x] == 0)
            {
                jump_table_[base].stop = next * width_ + x;
                continue;
            }
            if (has_forced(x, next, 0, -1))
            {
                jump_table_[base].jump = next * width_ + x;
            }
            else
            {
                jump_table_[base].jump = jump_table_[next_base].jump;
            }
            jump_table_[base].stop = jump_table_[next_base].stop;
        }
    }
    jump_build_version_ = map_version_;
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
    bool jump_table_ok = !jump_table_.empty() && jump_build_version_ == map_version_;
    bool light_ok = index_built_ && light_dirty_cnt_ == 0;
    last_tier_ = jump_table_ok ? kTierPlus : (light_ok ? kTierLight : kTierScan);
    if (start_idx == target_idx)
    {
        out_cells.push_back(start_idx);
        return 0;
    }
    search_stamp_++;
    if (search_stamp_ == 0)
    {
        for (size_t i = 0; i < states_.size(); i++)
        {
            states_[i].stamp = 0;
        }
        search_stamp_ = 1;
    }
    open_heap_.clear();
    open_push_count_ = 0;
    open_pop_count_ = 0;
    open_peak_ = 0;
    visit_count_ = 0;

    states_[start_idx].cost = 0;
    states_[start_idx].came_from = -1;
    states_[start_idx].stamp = search_stamp_;
    states_[start_idx].closed = 0;
    states_[start_idx].dir = 0;
    visit_count_ = 1;
    if (heap_push(open_heap_, open_capacity_, octile_to(start_x, start_y, target_x, target_y), 0, start_idx) != 0)
    {
        return -3;
    }
    open_push_count_++;
    if ((s32)open_heap_.size() > open_peak_)
    {
        open_peak_ = (s32)open_heap_.size();
    }

    s32 dirs[8];
    heap_entry entry;
    while (heap_pop(open_heap_, entry) == 0)
    {
        open_pop_count_++;
        s32 cur = entry.cell;
        if (states_[cur].closed != 0)
        {
            continue;
        }
        states_[cur].closed = 1;
        if (cur == target_idx)
        {
            last_path_cost_ = states_[cur].cost;
            s32 node = cur;
            while (node >= 0)
            {
                out_cells.push_back(node);
                s32 parent = states_[node].came_from;
                if (parent < 0)
                {
                    break;
                }
                s32 d = (s32)states_[node].dir - 1;
                s32 dx = dir_x(d);
                s32 dy = dir_y(d);
                s32 cx = node % width_ - dx;
                s32 cy = node / width_ - dy;
                while (cy * width_ + cx != parent)
                {
                    out_cells.push_back(cy * width_ + cx);
                    cx -= dx;
                    cy -= dy;
                }
                node = parent;
            }
            std::reverse(out_cells.begin(), out_cells.end());
            return 0;
        }
        s32 x = cur % width_;
        s32 y = cur / width_;
        s32 entry_dir = (s32)states_[cur].dir - 1;
        s32 dir_cnt = successor_dirs(x, y, entry_dir, dirs);
        for (s32 k = 0; k < dir_cnt; k++)
        {
            s32 d = dirs[k];
            s32 dx = dir_x(d);
            s32 dy = dir_y(d);
            s32 t = jump_table_ok ? (dx != 0 && dy != 0 ? jump_diag_plus(x, y, dx, dy, target_x, target_y)
                                                        : jump_table_lookup(cur, d, target_x, target_y))
                                  : jump(x, y, dx, dy, target_x, target_y);
            if (t < 0)
            {
                continue;
            }
            s32 tx = t % width_;
            s32 ty = t / width_;
            s32 next_cost = states_[cur].cost + octile_to(x, y, tx, ty);
            if (states_[t].stamp != search_stamp_ || next_cost < states_[t].cost)
            {
                states_[t].cost = next_cost;
                states_[t].came_from = cur;
                states_[t].dir = (u8)(d + 1);
                states_[t].stamp = search_stamp_;
                states_[t].closed = 0;
                visit_count_++;
                if (heap_push(open_heap_, open_capacity_, next_cost + octile_to(tx, ty, target_x, target_y), next_cost, t) != 0)
                {
                    return -3;
                }
                open_push_count_++;
                if ((s32)open_heap_.size() > open_peak_)
                {
                    open_peak_ = (s32)open_heap_.size();
                }
            }
        }
    }
    return -2;
}

inline s32 zjps_grid::line_lower(const s32* vals, s32 cnt, s32 v)
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

inline s32 zjps_grid::line_upper(const s32* vals, s32 cnt, s32 v)
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

inline s32 zjps_grid::refill_light_row(s32 y)
{
    const u8* row = cell_flag_.data() + (size_t)y * (size_t)width_;
    s32* ln = light_row_head(y);
    s32* vals = ln + 1;
    s32 cnt = 0;
    for (s32 x = 0; x < width_; x++)
    {
        if (row[x] == 0)
        {
            vals[cnt++] = x;
        }
    }
    ln[0] = cnt;
    return 0;
}

inline s32 zjps_grid::refill_light_col(s32 x)
{
    s32* ln = light_col_head(x);
    s32* vals = ln + 1;
    s32 cnt = 0;
    for (s32 y = 0; y < height_; y++)
    {
        if (cell_flag_[(size_t)y * (size_t)width_ + (size_t)x] == 0)
        {
            vals[cnt++] = y;
        }
    }
    ln[0] = cnt;
    return 0;
}

inline s32 zjps_grid::line_insert(s32* vals, s32& cnt, s32 cap, s32 v)
{
    s32 pos = line_lower(vals, cnt, v);
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

inline s32 zjps_grid::line_remove(s32* vals, s32& cnt, s32 v)
{
    s32 pos = line_lower(vals, cnt, v);
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

inline s32 zjps_grid::heap_push(std::vector<heap_entry>& heap, s32 capacity, s32 f, s32 g, s32 cell)
{
    if ((s32)heap.size() >= capacity)
    {
        return -1;
    }
    heap_entry entry;
    entry.f = f;
    entry.g = g;
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

inline bool zjps_grid::has_forced(s32 x, s32 y, s32 dx, s32 dy)
{
    if (dx != 0 && dy != 0)
    {
        return false;
    }
    if (dx != 0)
    {
        if (cell_walkable(x, y + 1) && !cell_walkable(x - dx, y + 1))
        {
            return true;
        }
        if (cell_walkable(x, y - 1) && !cell_walkable(x - dx, y - 1))
        {
            return true;
        }
        return false;
    }
    if (cell_walkable(x + 1, y) && !cell_walkable(x + 1, y - dy))
    {
        return true;
    }
    if (cell_walkable(x - 1, y) && !cell_walkable(x - 1, y - dy))
    {
        return true;
    }
    return false;
}

inline s32 zjps_grid::jump(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y)
{
    if (dx != 0 && dy != 0)
    {
        while (true)
        {
            if (!move_valid(x, y, dx, dy))
            {
                return -1;
            }
            x += dx;
            y += dy;
            if (x == target_x && y == target_y)
            {
                return y * width_ + x;
            }
            if (has_forced(x, y, dx, dy))
            {
                return y * width_ + x;
            }
            if (jump_straight(x, y, dx, 0, target_x, target_y) >= 0)
            {
                return y * width_ + x;
            }
            if (jump_straight(x, y, 0, dy, target_x, target_y) >= 0)
            {
                return y * width_ + x;
            }
        }
    }
    return jump_straight(x, y, dx, dy, target_x, target_y);
}

inline s32 zjps_grid::jump_straight(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y)
{
    if (!(index_built_ && light_dirty_cnt_ == 0))
    {
        while (true)
        {
            if (!move_valid(x, y, dx, dy))
            {
                return -1;
            }
            x += dx;
            y += dy;
            if (x == target_x && y == target_y)
            {
                return y * width_ + x;
            }
            if (has_forced(x, y, dx, dy))
            {
                return y * width_ + x;
            }
        }
    }
    s32 probe = kScanProbeSteps;
    while (probe > 0)
    {
        if (!move_valid(x, y, dx, dy))
        {
            return -1;
        }
        x += dx;
        y += dy;
        if (x == target_x && y == target_y)
        {
            return y * width_ + x;
        }
        if (has_forced(x, y, dx, dy))
        {
            return y * width_ + x;
        }
        probe--;
    }
    return jump_straight_indexed(x, y, dx, dy, target_x, target_y);
}

inline s32 zjps_grid::jump_straight_indexed(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y)
{
    if (dx != 0)
    {
        const s32* row_ln = light_row_head(y);
        const s32* row_vals = row_ln + 1;
        s32 row_cnt = row_ln[0];
        s32 hard;
        if (dx > 0)
        {
            s32 pos = line_lower(row_vals, row_cnt, x + 1);
            hard = pos < row_cnt ? row_vals[pos] : -1;
        }
        else
        {
            s32 pos = line_upper(row_vals, row_cnt, x - 1);
            hard = pos > 0 ? row_vals[pos - 1] : -1;
        }
        s32 c_max = hard < 0 ? (dx > 0 ? width_ - 1 : 0) : hard - dx;
        if (dx > 0 && c_max > width_ - 1)
        {
            c_max = width_ - 1;
        }
        if (dx < 0 && c_max < 0)
        {
            c_max = 0;
        }
        s32 stop = -1;
        if (y == target_y)
        {
            if ((dx > 0 && target_x > x && target_x <= c_max) || (dx < 0 && target_x < x && target_x >= c_max))
            {
                stop = target_x;
            }
        }
        s32 forced = straight_forced_stop_row(y, x, dx, c_max);
        if (forced >= 0 && (stop < 0 || (dx > 0 ? forced < stop : forced > stop)))
        {
            stop = forced;
        }
        if (stop < 0)
        {
            return -1;
        }
        return y * width_ + stop;
    }
    const s32* col_ln = light_col_head(x);
    const s32* col_vals = col_ln + 1;
    s32 col_cnt = col_ln[0];
    s32 hard;
    if (dy > 0)
    {
        s32 pos = line_lower(col_vals, col_cnt, y + 1);
        hard = pos < col_cnt ? col_vals[pos] : -1;
    }
    else
    {
        s32 pos = line_upper(col_vals, col_cnt, y - 1);
        hard = pos > 0 ? col_vals[pos - 1] : -1;
    }
    s32 r_max = hard < 0 ? (dy > 0 ? height_ - 1 : 0) : hard - dy;
    if (dy > 0 && r_max > height_ - 1)
    {
        r_max = height_ - 1;
    }
    if (dy < 0 && r_max < 0)
    {
        r_max = 0;
    }
    s32 stop = -1;
    if (x == target_x)
    {
        if ((dy > 0 && target_y > y && target_y <= r_max) || (dy < 0 && target_y < y && target_y >= r_max))
        {
            stop = target_y;
        }
    }
    s32 forced = straight_forced_stop_col(x, y, dy, r_max);
    if (forced >= 0 && (stop < 0 || (dy > 0 ? forced < stop : forced > stop)))
    {
        stop = forced;
    }
    if (stop < 0)
    {
        return -1;
    }
    return stop * width_ + x;
}

inline s32 zjps_grid::straight_forced_stop_row(s32 y, s32 x, s32 dx, s32 c_max)
{
    s32 best = -1;
    for (s32 side = -1; side <= 1; side += 2)
    {
        s32 ys = y + side;
        if (ys < 0 || ys >= height_)
        {
            continue;
        }
        const s32* row_ln = light_row_head(ys);
        const s32* blocks = row_ln + 1;
        s32 block_cnt = row_ln[0];
        if (dx > 0)
        {
            s32 i = line_lower(blocks, block_cnt, x);
            if (i >= block_cnt)
            {
                continue;
            }
            size_t e = block_run_end(blocks, (size_t)i, (size_t)block_cnt);
            s32 c = blocks[e] + 1;
            if (c >= width_ || c > c_max)
            {
                continue;
            }
            if (best < 0 || c < best)
            {
                best = c;
            }
        }
        else
        {
            s32 i = line_upper(blocks, block_cnt, x);
            if (i <= 0)
            {
                continue;
            }
            size_t s = block_run_start(blocks, (size_t)(i - 1));
            s32 c = blocks[s] - 1;
            if (c < 0 || c < c_max)
            {
                continue;
            }
            if (best < 0 || c > best)
            {
                best = c;
            }
        }
    }
    return best;
}

inline s32 zjps_grid::straight_forced_stop_col(s32 x, s32 y, s32 dy, s32 r_max)
{
    s32 best = -1;
    for (s32 side = -1; side <= 1; side += 2)
    {
        s32 xs = x + side;
        if (xs < 0 || xs >= width_)
        {
            continue;
        }
        const s32* col_ln = light_col_head(xs);
        const s32* blocks = col_ln + 1;
        s32 block_cnt = col_ln[0];
        if (dy > 0)
        {
            s32 i = line_lower(blocks, block_cnt, y);
            if (i >= block_cnt)
            {
                continue;
            }
            size_t e = block_run_end(blocks, (size_t)i, (size_t)block_cnt);
            s32 r = blocks[e] + 1;
            if (r >= height_ || r > r_max)
            {
                continue;
            }
            if (best < 0 || r < best)
            {
                best = r;
            }
        }
        else
        {
            s32 i = line_upper(blocks, block_cnt, y);
            if (i <= 0)
            {
                continue;
            }
            size_t s = block_run_start(blocks, (size_t)(i - 1));
            s32 r = blocks[s] - 1;
            if (r < 0 || r < r_max)
            {
                continue;
            }
            if (best < 0 || r > best)
            {
                best = r;
            }
        }
    }
    return best;
}

inline s32 zjps_grid::successor_dirs(s32 x, s32 y, s32 d, s32* out_dirs)
{
    s32 cnt = 0;
    if (d < 0)
    {
        for (s32 k = 0; k < kDirCnt; k++)
        {
            out_dirs[cnt++] = k;
        }
        return cnt;
    }
    s32 dx = dir_x(d);
    s32 dy = dir_y(d);
    if (dx != 0 && dy != 0)
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
            out_dirs[cnt++] = d;
        }
        return cnt;
    }
    if (dx != 0)
    {
        bool walk_ahead = cell_walkable(x + dx, y);
        if (walk_ahead)
        {
            out_dirs[cnt++] = d;
        }
        for (s32 side = -1; side <= 1; side += 2)
        {
            if (!cell_walkable(x, y + side) || cell_walkable(x - dx, y + side))
            {
                continue;
            }
            out_dirs[cnt++] = dir_index(0, side);
            if (walk_ahead)
            {
                out_dirs[cnt++] = dir_index(dx, side);
            }
        }
        return cnt;
    }
    bool walk_ahead = cell_walkable(x, y + dy);
    if (walk_ahead)
    {
        out_dirs[cnt++] = d;
    }
    for (s32 side = -1; side <= 1; side += 2)
    {
        if (!cell_walkable(x + side, y) || cell_walkable(x + side, y - dy))
        {
            continue;
        }
        out_dirs[cnt++] = dir_index(side, 0);
        if (walk_ahead)
        {
            out_dirs[cnt++] = dir_index(side, dy);
        }
    }
    return cnt;
}

inline s32 zjps_grid::jump_table_lookup(s32 cell_idx, s32 d, s32 target_x, s32 target_y)
{
    s32 x = cell_idx % width_;
    s32 y = cell_idx / width_;
    s32 dx = dir_x(d);
    s32 dy = dir_y(d);
    if (ray_reaches_target(x, y, dx, dy, target_x, target_y))
    {
        return target_y * width_ + target_x;
    }
    return jump_table_[plus_slot(cell_idx, d)].jump;
}

inline bool zjps_grid::ray_reaches_target(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y)
{
    if (dx != 0 && dy != 0)
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
    s32 j = jump_table_[slot].jump;
    s32 stop = jump_table_[slot].stop;
    s32 t_axis = (dx != 0) ? target_x : target_y;
    s32 s = (dx != 0) ? dx : dy;
    if (j >= 0)
    {
        s32 j_axis = (dx != 0) ? (j % width_) : (j / width_);
        return (t_axis - j_axis) * s <= 0;
    }
    if (stop >= 0)
    {
        s32 stop_axis = (dx != 0) ? (stop % width_) : (stop / width_);
        return (t_axis - stop_axis) * s < 0;
    }
    return true;
}

inline s32 zjps_grid::jump_diag_plus(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y)
{
    while (true)
    {
        if (!move_valid(x, y, dx, dy))
        {
            return -1;
        }
        x += dx;
        y += dy;
        if (x == target_x && y == target_y)
        {
            return y * width_ + x;
        }
        if (has_forced(x, y, dx, dy))
        {
            return y * width_ + x;
        }
        s32 cell = y * width_ + x;
        if (jump_table_[plus_slot(cell, dir_index(dx, 0))].jump >= 0
            || jump_table_[plus_slot(cell, dir_index(0, dy))].jump >= 0)
        {
            return cell;
        }
        if (ray_reaches_target(x, y, dx, 0, target_x, target_y)
            || ray_reaches_target(x, y, 0, dy, target_x, target_y))
        {
            return cell;
        }
    }
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
