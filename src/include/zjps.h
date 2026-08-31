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

    zjps_grid() = default;

    s32 init(s32 width, s32 height, f32 cell_size, bool walkable_default)
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
        walkable_.assign(cell_cnt, default_walkable_);
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

    s32 width() const { return width_; }
    s32 height() const { return height_; }
    f32 cell_size() const { return cell_size_; }
    u32 map_version() const { return map_version_; }

    bool cell_walkable(s32 x, s32 y) const
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_)
        {
            return false;
        }
        return walkable_[(size_t)y * (size_t)width_ + (size_t)x] != 0;
    }

    s32 set_cell(s32 x, s32 y, bool walkable)
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_)
        {
            return -1;
        }
        size_t idx = (size_t)y * (size_t)width_ + (size_t)x;
        u8 old = walkable_[idx];
        u8 next = walkable ? 1 : 0;
        if (old == next)
        {
            return 0;
        }
        walkable_[idx] = next;
        map_version_++;
        if (index_built_)
        {
            s32* row = light_row_head(y);
            s32* col = light_col_head(x);
            if (next != 0)
            {
                line_remove(row + 1, row[0], x);
                line_remove(col + 1, col[0], y);
            }
            else
            {
                line_insert(row + 1, row[0], width_, x);
                line_insert(col + 1, col[0], height_, y);
            }
        }
        return 0;
    }

    s32 set_blocked(s32 x, s32 y)
    {
        return set_cell(x, y, false);
    }

    s32 set_walkable(s32 x, s32 y)
    {
        return set_cell(x, y, true);
    }

    s32 pos_to_cell(f32 px, f32 py, s32& out_x, s32& out_y) const
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

    bool move_valid(s32 x, s32 y, s32 dx, s32 dy) const
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
            return cell_walkable(x + dx, y) || cell_walkable(x, y + dy);
        }
        return true;
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

    s32 set_scan_f_cut(bool enable)
    {
        scan_f_cut_ = enable;
        return 0;
    }

    s32 open_capacity() const { return open_capacity_; }
    size_t jps_plus_table_bytes() const { return jump_table_.size() * sizeof(jump_entry); }
    s32 open_push_count() const { return open_push_count_; }
    s32 open_pop_count() const { return open_pop_count_; }
    s32 open_peak() const { return open_peak_; }
    s32 visit_count() const { return visit_count_; }
    s32 last_path_cost() const { return last_path_cost_; }

    s32 find_path(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
    {
        out_cells.clear();
        if (start_x < 0 || start_y < 0 || start_x >= width_ || start_y >= height_
            || target_x < 0 || target_y < 0 || target_x >= width_ || target_y >= height_)
        {
            return -1;
        }
        s32 start_idx = start_y * width_ + start_x;
        s32 target_idx = target_y * width_ + target_x;
        if (walkable_[start_idx] == 0 || walkable_[target_idx] == 0)
        {
            return -2;
        }
        last_path_cost_ = 0;
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
        if (heap_push(octile_to(start_x, start_y, target_x, target_y), 0, start_idx) != 0)
        {
            return -3;
        }

        heap_entry entry;
        while (heap_pop(entry) == 0)
        {
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
                    if (heap_push(next_cost + octile_to(x + dx, y + dy, target_x, target_y), next_cost, next) != 0)
                    {
                        return -3;
                    }
                }
            }
        }
        return -2;
    }

    s32 find_path_jps(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
    {
        return jps_search(start_x, start_y, target_x, target_y, out_cells, false);
    }

    s32 find_path_jps_plus(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
    {
        if (jump_table_.empty() || jump_build_version_ != map_version_)
        {
            return find_path_jps(start_x, start_y, target_x, target_y, out_cells);
        }
        return jps_search(start_x, start_y, target_x, target_y, out_cells, true);
    }

    s32 build_jps_light()
    {
        if (width_ <= 0 || height_ <= 0)
        {
            return -1;
        }
        for (s32 y = 0; y < height_; y++)
        {
            const u8* row = walkable_.data() + (size_t)y * (size_t)width_;
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
        }
        for (s32 x = 0; x < width_; x++)
        {
            s32* ln = light_col_head(x);
            s32* vals = ln + 1;
            s32 cnt = 0;
            for (s32 y = 0; y < height_; y++)
            {
                if (walkable_[(size_t)y * (size_t)width_ + (size_t)x] == 0)
                {
                    vals[cnt++] = y;
                }
            }
            ln[0] = cnt;
        }
        index_built_ = true;
        return 0;
    }

    s32 build_jps_plus()
    {
        if (width_ <= 0 || height_ <= 0)
        {
            return -1;
        }
        size_t cell_cnt = (size_t)width_ * (size_t)height_;
        if (jump_table_.capacity() < cell_cnt * 8)
        {
            jump_table_.reserve(cell_cnt * 8);
        }
        jump_table_.assign(cell_cnt * 8, jump_entry());
        for (s32 y = 0; y < height_; y++)
        {
            for (s32 x = width_ - 2; x >= 0; x--)
            {
                s32 next = x + 1;
                size_t base = ((size_t)y * (size_t)width_ + (size_t)x) * 8;
                size_t next_base = ((size_t)y * (size_t)width_ + (size_t)next) * 8;
                if (walkable_[(size_t)y * (size_t)width_ + (size_t)next] == 0)
                {
                    jump_table_[base].stop = (s32)((size_t)y * (size_t)width_ + (size_t)next);
                    continue;
                }
                if (has_forced(next, y, 1, 0))
                {
                    jump_table_[base].jump = (s32)((size_t)y * (size_t)width_ + (size_t)next);
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
                size_t base = ((size_t)y * (size_t)width_ + (size_t)x) * 8;
                size_t next_base = ((size_t)y * (size_t)width_ + (size_t)next) * 8;
                if (walkable_[(size_t)y * (size_t)width_ + (size_t)next] == 0)
                {
                    jump_table_[base + 4].stop = (s32)((size_t)y * (size_t)width_ + (size_t)next);
                    continue;
                }
                if (has_forced(next, y, -1, 0))
                {
                    jump_table_[base + 4].jump = (s32)((size_t)y * (size_t)width_ + (size_t)next);
                }
                else
                {
                    jump_table_[base + 4].jump = jump_table_[next_base + 4].jump;
                }
                jump_table_[base + 4].stop = jump_table_[next_base + 4].stop;
            }
        }
        for (s32 x = 0; x < width_; x++)
        {
            for (s32 y = height_ - 2; y >= 0; y--)
            {
                s32 next = y + 1;
                size_t base = ((size_t)y * (size_t)width_ + (size_t)x) * 8;
                size_t next_base = ((size_t)next * (size_t)width_ + (size_t)x) * 8;
                if (walkable_[(size_t)next * (size_t)width_ + (size_t)x] == 0)
                {
                    jump_table_[base + 2].stop = (s32)((size_t)next * (size_t)width_ + (size_t)x);
                    continue;
                }
                if (has_forced(x, next, 0, 1))
                {
                    jump_table_[base + 2].jump = (s32)((size_t)next * (size_t)width_ + (size_t)x);
                }
                else
                {
                    jump_table_[base + 2].jump = jump_table_[next_base + 2].jump;
                }
                jump_table_[base + 2].stop = jump_table_[next_base + 2].stop;
            }
            for (s32 y = 1; y < height_; y++)
            {
                s32 next = y - 1;
                size_t base = ((size_t)y * (size_t)width_ + (size_t)x) * 8;
                size_t next_base = ((size_t)next * (size_t)width_ + (size_t)x) * 8;
                if (walkable_[(size_t)next * (size_t)width_ + (size_t)x] == 0)
                {
                    jump_table_[base + 6].stop = (s32)((size_t)next * (size_t)width_ + (size_t)x);
                    continue;
                }
                if (has_forced(x, next, 0, -1))
                {
                    jump_table_[base + 6].jump = (s32)((size_t)next * (size_t)width_ + (size_t)x);
                }
                else
                {
                    jump_table_[base + 6].jump = jump_table_[next_base + 6].jump;
                }
                jump_table_[base + 6].stop = jump_table_[next_base + 6].stop;
            }
        }
        jump_build_version_ = map_version_;
        return 0;
    }

    s32 jps_search(s32 start_x, s32 start_y, s32 target_x, s32 target_y, std::vector<s32>& out_cells, bool use_plus)
    {
        out_cells.clear();
        if (start_x < 0 || start_y < 0 || start_x >= width_ || start_y >= height_
            || target_x < 0 || target_y < 0 || target_x >= width_ || target_y >= height_)
        {
            return -1;
        }
        s32 start_idx = start_y * width_ + start_x;
        s32 target_idx = target_y * width_ + target_x;
        if (walkable_[start_idx] == 0 || walkable_[target_idx] == 0)
        {
            return -2;
        }
        last_path_cost_ = 0;
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
        if (heap_push(octile_to(start_x, start_y, target_x, target_y), 0, start_idx) != 0)
        {
            return -3;
        }

        s32 dirs[8];
        heap_entry entry;
        while (heap_pop(entry) == 0)
        {
            s32 cur = entry.cell;
            if (states_[cur].closed != 0)
            {
                continue;
            }
            states_[cur].closed = 1;
            if (cur == target_idx)
            {
                last_path_cost_ = states_[cur].cost;
                std::vector<s32> reversed;
                s32 node = cur;
                while (node >= 0)
                {
                    reversed.push_back(node);
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
                        reversed.push_back(cy * width_ + cx);
                        cx -= dx;
                        cy -= dy;
                    }
                    node = parent;
                }
                out_cells.assign(reversed.rbegin(), reversed.rend());
                return 0;
            }
            s32 x = cur % width_;
            s32 y = cur / width_;
            s32 f_cur = states_[cur].cost + octile_to(x, y, target_x, target_y);
            s32 entry_dir = (s32)states_[cur].dir - 1;
            s32 dir_cnt = successor_dirs(x, y, entry_dir, dirs);
            for (s32 k = 0; k < dir_cnt; k++)
            {
                s32 d = dirs[k];
                s32 dx = dir_x(d);
                s32 dy = dir_y(d);
                s32 t = use_plus ? plus_jump(cur, d, target_x, target_y)
                                 : jump(x, y, dx, dy, target_x, target_y, states_[cur].cost, f_cur);
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
                    if (heap_push(next_cost + octile_to(tx, ty, target_x, target_y), next_cost, t) != 0)
                    {
                        return -3;
                    }
                }
            }
        }
        return -2;
    }

    s32 raster_segment(const zpoint& source, const zpoint& target, f32 radius, bool walkable)
    {
        if (cell_size_ <= 0.0f || radius < 0.0f)
        {
            return -1;
        }
        f32 dx = target.x - source.x;
        f32 dy = target.y - source.y;
        f32 len = sqrtf(dx * dx + dy * dy);
        s32 steps = (s32)(len / cell_size_);
        if ((f32)steps * cell_size_ < len)
        {
            steps++;
        }
        if (steps < 1)
        {
            steps = 1;
        }
        s32 band = (s32)(radius / cell_size_) + 1;
        f32 radius_sq = radius * radius;
        for (s32 i = 0; i <= steps; i++)
        {
            f32 t = (f32)i / (f32)steps;
            zpoint sample(source.x + dx * t, source.y + dy * t, 0.0f);
            s32 sample_x = 0;
            s32 sample_y = 0;
            if (pos_to_cell(sample.x, sample.y, sample_x, sample_y) != 0)
            {
                continue;
            }
            for (s32 y = sample_y - band; y <= sample_y + band; y++)
            {
                for (s32 x = sample_x - band; x <= sample_x + band; x++)
                {
                    if (x < 0 || y < 0 || x >= width_ || y >= height_)
                    {
                        continue;
                    }
                    f32 center_x = ((f32)x + 0.5f) * cell_size_;
                    f32 center_y = ((f32)y + 0.5f) * cell_size_;
                    if (point_segment_dist_sq(center_x, center_y, source, target) <= radius_sq)
                    {
                        set_cell(x, y, walkable);
                    }
                }
            }
        }
        return 0;
    }

    s32 raster_rect(f32 min_x, f32 min_y, f32 max_x, f32 max_y, bool walkable)
    {
        if (cell_size_ <= 0.0f || min_x > max_x || min_y > max_y)
        {
            return -1;
        }
        s32 x0 = (s32)floorf(min_x / cell_size_);
        s32 x1 = (s32)floorf(max_x / cell_size_);
        s32 y0 = (s32)floorf(min_y / cell_size_);
        s32 y1 = (s32)floorf(max_y / cell_size_);
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 >= width_) x1 = width_ - 1;
        if (y1 >= height_) y1 = height_ - 1;
        for (s32 y = y0; y <= y1; y++)
        {
            for (s32 x = x0; x <= x1; x++)
            {
                f32 center_x = ((f32)x + 0.5f) * cell_size_;
                f32 center_y = ((f32)y + 0.5f) * cell_size_;
                if (center_x >= min_x && center_x <= max_x && center_y >= min_y && center_y <= max_y)
                {
                    set_cell(x, y, walkable);
                }
            }
        }
        return 0;
    }

    s32 raster_circle(f32 center_x, f32 center_y, f32 radius, bool walkable)
    {
        if (cell_size_ <= 0.0f || radius < 0.0f)
        {
            return -1;
        }
        s32 x0 = (s32)floorf((center_x - radius) / cell_size_);
        s32 x1 = (s32)floorf((center_x + radius) / cell_size_);
        s32 y0 = (s32)floorf((center_y - radius) / cell_size_);
        s32 y1 = (s32)floorf((center_y + radius) / cell_size_);
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 >= width_) x1 = width_ - 1;
        if (y1 >= height_) y1 = height_ - 1;
        f32 radius_sq = radius * radius;
        for (s32 y = y0; y <= y1; y++)
        {
            for (s32 x = x0; x <= x1; x++)
            {
                f32 cx = ((f32)x + 0.5f) * cell_size_ - center_x;
                f32 cy = ((f32)y + 0.5f) * cell_size_ - center_y;
                if (cx * cx + cy * cy <= radius_sq)
                {
                    set_cell(x, y, walkable);
                }
            }
        }
        return 0;
    }

    s32 raster_polygon(const std::vector<zpoint>& vertexes, bool walkable)
    {
        if (cell_size_ <= 0.0f || vertexes.size() < 3)
        {
            return -1;
        }
        f32 min_y = vertexes[0].y;
        f32 max_y = vertexes[0].y;
        for (size_t i = 1; i < vertexes.size(); i++)
        {
            if (vertexes[i].y < min_y) min_y = vertexes[i].y;
            if (vertexes[i].y > max_y) max_y = vertexes[i].y;
        }
        s32 y0 = (s32)floorf(min_y / cell_size_);
        s32 y1 = (s32)floorf(max_y / cell_size_);
        if (y0 < 0) y0 = 0;
        if (y1 >= height_) y1 = height_ - 1;
        std::vector<f32> crossings;
        for (s32 y = y0; y <= y1; y++)
        {
            f32 center_y = ((f32)y + 0.5f) * cell_size_;
            crossings.clear();
            for (size_t i = 0; i < vertexes.size(); i++)
            {
                const zpoint& a = vertexes[i];
                const zpoint& b = vertexes[(i + 1) % vertexes.size()];
                if (a.y == b.y)
                {
                    continue;
                }
                if ((a.y <= center_y && center_y < b.y) || (b.y <= center_y && center_y < a.y))
                {
                    f32 t = (center_y - a.y) / (b.y - a.y);
                    crossings.push_back(a.x + (b.x - a.x) * t);
                }
            }
            std::sort(crossings.begin(), crossings.end());
            for (size_t k = 0; k + 1 < crossings.size(); k += 2)
            {
                f32 span_min_x = crossings[k];
                f32 span_max_x = crossings[k + 1];
                s32 x0 = (s32)floorf(span_min_x / cell_size_);
                s32 x1 = (s32)floorf(span_max_x / cell_size_);
                if (x0 < 0) x0 = 0;
                if (x1 >= width_) x1 = width_ - 1;
                for (s32 x = x0; x <= x1; x++)
                {
                    f32 center_x = ((f32)x + 0.5f) * cell_size_;
                    if (center_x >= span_min_x && center_x <= span_max_x)
                    {
                        set_cell(x, y, walkable);
                    }
                }
            }
        }
        return 0;
    }

private:
    static constexpr s32 kScanProbeSteps = 8;

    static s32 line_lower(const s32* vals, s32 cnt, s32 v)
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

    static s32 line_upper(const s32* vals, s32 cnt, s32 v)
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

    static void line_insert(s32* vals, s32& cnt, s32 cap, s32 v)
    {
        s32 pos = line_lower(vals, cnt, v);
        if (pos < cnt && vals[pos] == v)
        {
            return;
        }
        if (cnt >= cap)
        {
            return;
        }
        if (pos < cnt)
        {
            memmove(vals + pos + 1, vals + pos, (size_t)(cnt - pos) * sizeof(s32));
        }
        vals[pos] = v;
        cnt++;
    }

    static void line_remove(s32* vals, s32& cnt, s32 v)
    {
        s32 pos = line_lower(vals, cnt, v);
        if (pos >= cnt || vals[pos] != v)
        {
            return;
        }
        if (pos + 1 < cnt)
        {
            memmove(vals + pos, vals + pos + 1, (size_t)(cnt - pos - 1) * sizeof(s32));
        }
        cnt--;
    }

    s32* light_row_head(s32 y)
    {
        return light_row_.data() + (size_t)y * (size_t)(width_ + 1);
    }

    s32* light_col_head(s32 x)
    {
        return light_col_.data() + (size_t)x * (size_t)(height_ + 1);
    }

    s32 next_block_in_row(s32 y, s32 from_x)
    {
        const s32* ln = light_row_head(y);
        const s32* vals = ln + 1;
        s32 cnt = ln[0];
        s32 pos = line_lower(vals, cnt, from_x);
        return pos < cnt ? vals[pos] : -1;
    }

    s32 prev_block_in_row(s32 y, s32 from_x)
    {
        const s32* ln = light_row_head(y);
        const s32* vals = ln + 1;
        s32 cnt = ln[0];
        s32 pos = line_upper(vals, cnt, from_x);
        return pos > 0 ? vals[pos - 1] : -1;
    }

    s32 next_block_in_col(s32 x, s32 from_y)
    {
        const s32* ln = light_col_head(x);
        const s32* vals = ln + 1;
        s32 cnt = ln[0];
        s32 pos = line_lower(vals, cnt, from_y);
        return pos < cnt ? vals[pos] : -1;
    }

    s32 prev_block_in_col(s32 x, s32 from_y)
    {
        const s32* ln = light_col_head(x);
        const s32* vals = ln + 1;
        s32 cnt = ln[0];
        s32 pos = line_upper(vals, cnt, from_y);
        return pos > 0 ? vals[pos - 1] : -1;
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

    s32 heap_push(s32 f, s32 g, s32 cell)
    {
        if ((s32)open_heap_.size() >= open_capacity_)
        {
            return -1;
        }
        heap_entry entry;
        entry.f = f;
        entry.g = g;
        entry.cell = cell;
        open_heap_.push_back(entry);
        size_t child = open_heap_.size() - 1;
        while (child > 0)
        {
            size_t parent = (child - 1) / 2;
            if (!heap_before(open_heap_[child], open_heap_[parent]))
            {
                break;
            }
            heap_entry tmp = open_heap_[parent];
            open_heap_[parent] = open_heap_[child];
            open_heap_[child] = tmp;
            child = parent;
        }
        open_push_count_++;
        if ((s32)open_heap_.size() > open_peak_)
        {
            open_peak_ = (s32)open_heap_.size();
        }
        return 0;
    }

    s32 heap_pop(heap_entry& out)
    {
        if (open_heap_.empty())
        {
            return -1;
        }
        out = open_heap_.front();
        open_pop_count_++;
        open_heap_.front() = open_heap_.back();
        open_heap_.pop_back();
        size_t parent = 0;
        while (true)
        {
            size_t left = parent * 2 + 1;
            size_t right = left + 1;
            size_t best = parent;
            if (left < open_heap_.size() && heap_before(open_heap_[left], open_heap_[best]))
            {
                best = left;
            }
            if (right < open_heap_.size() && heap_before(open_heap_[right], open_heap_[best]))
            {
                best = right;
            }
            if (best == parent)
            {
                break;
            }
            heap_entry tmp = open_heap_[parent];
            open_heap_[parent] = open_heap_[best];
            open_heap_[best] = tmp;
            parent = best;
        }
        return 0;
    }

    static s32 dir_index(s32 dx, s32 dy)
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

    bool has_forced(s32 x, s32 y, s32 dx, s32 dy)
    {
        if (dx != 0 && dy != 0)
        {
            if (!cell_walkable(x - dx, y))
            {
                if (cell_walkable(x - dx, y + dy) && cell_walkable(x, y + dy))
                {
                    return true;
                }
            }
            if (!cell_walkable(x, y - dy))
            {
                if (cell_walkable(x + dx, y - dy) && cell_walkable(x + dx, y))
                {
                    return true;
                }
            }
            return false;
        }
        if (dx != 0)
        {
            if (!cell_walkable(x, y + 1))
            {
                if (cell_walkable(x + dx, y + 1) && cell_walkable(x + dx, y))
                {
                    return true;
                }
            }
            if (!cell_walkable(x, y - 1))
            {
                if (cell_walkable(x + dx, y - 1) && cell_walkable(x + dx, y))
                {
                    return true;
                }
            }
            return false;
        }
        if (!cell_walkable(x + 1, y))
        {
            if (cell_walkable(x + 1, y + dy) && cell_walkable(x, y + dy))
            {
                return true;
            }
        }
        if (!cell_walkable(x - 1, y))
        {
            if (cell_walkable(x - 1, y + dy) && cell_walkable(x, y + dy))
            {
                return true;
            }
        }
        return false;
    }

    s32 jump(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y, s32 g_base, s32 f_parent)
    {
        if (dx != 0 && dy != 0)
        {
            s32 g_acc = 0;
            while (true)
            {
                if (!move_valid(x, y, dx, dy))
                {
                    return -1;
                }
                x += dx;
                y += dy;
                g_acc += kCostDiagonal;
                if (x == target_x && y == target_y)
                {
                    return y * width_ + x;
                }
                if (has_forced(x, y, dx, dy))
                {
                    return y * width_ + x;
                }
                if (scan_f_cut_ && g_base + g_acc + octile_to(x, y, target_x, target_y) > f_parent)
                {
                    return -1;
                }
                if (jump_straight(x, y, dx, 0, target_x, target_y, g_base + g_acc, f_parent) >= 0)
                {
                    return y * width_ + x;
                }
                if (jump_straight(x, y, 0, dy, target_x, target_y, g_base + g_acc, f_parent) >= 0)
                {
                    return y * width_ + x;
                }
            }
        }
        return jump_straight(x, y, dx, dy, target_x, target_y, g_base, f_parent);
    }

    s32 jump_straight(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y, s32 g_base, s32 f_parent)
    {
        s32 g_acc = 0;
        if (!index_built_)
        {
            while (true)
            {
                if (!move_valid(x, y, dx, dy))
                {
                    return -1;
                }
                x += dx;
                y += dy;
                g_acc += kCostStraight;
                if (x == target_x && y == target_y)
                {
                    return y * width_ + x;
                }
                if (has_forced(x, y, dx, dy))
                {
                    return y * width_ + x;
                }
                if (scan_f_cut_ && g_base + g_acc + octile_to(x, y, target_x, target_y) > f_parent)
                {
                    return -1;
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
            g_acc += kCostStraight;
            if (x == target_x && y == target_y)
            {
                return y * width_ + x;
            }
            if (has_forced(x, y, dx, dy))
            {
                return y * width_ + x;
            }
            if (scan_f_cut_ && g_base + g_acc + octile_to(x, y, target_x, target_y) > f_parent)
            {
                return -1;
            }
            probe--;
        }
        return jump_straight_indexed(x, y, dx, dy, target_x, target_y);
    }

    s32 jump_straight_indexed(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y)
    {
        if (dx != 0)
        {
            s32 hard = dx > 0 ? next_block_in_row(y, x + 1) : prev_block_in_row(y, x - 1);
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
        s32 hard = dy > 0 ? next_block_in_col(x, y + 1) : prev_block_in_col(x, y - 1);
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

    s32 straight_forced_stop_row(s32 y, s32 x, s32 dx, s32 c_max)
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
                size_t i = (size_t)line_lower(blocks, block_cnt, x + 1);
                while (i < (size_t)block_cnt)
                {
                    s32 o = blocks[i];
                    if (o > c_max)
                    {
                        break;
                    }
                    size_t e = block_run_end(blocks, i, (size_t)block_cnt);
                    o = blocks[e];
                    if (o > c_max)
                    {
                        break;
                    }
                    s32 ox = o + 1;
                    if (ox < width_ && walkable_[(size_t)ys * (size_t)width_ + (size_t)ox] != 0
                        && walkable_[(size_t)y * (size_t)width_ + (size_t)ox] != 0)
                    {
                        if (best < 0 || o < best)
                        {
                            best = o;
                        }
                        break;
                    }
                    i = e + 1;
                }
            }
            else
            {
                size_t i = (size_t)line_upper(blocks, block_cnt, x - 1);
                while (i > 0)
                {
                    s32 o = blocks[i - 1];
                    if (o < c_max)
                    {
                        break;
                    }
                    size_t s = block_run_start(blocks, i - 1);
                    o = blocks[s];
                    if (o < c_max)
                    {
                        break;
                    }
                    s32 ox = o - 1;
                    if (ox >= 0 && walkable_[(size_t)ys * (size_t)width_ + (size_t)ox] != 0
                        && walkable_[(size_t)y * (size_t)width_ + (size_t)ox] != 0)
                    {
                        if (best < 0 || o > best)
                        {
                            best = o;
                        }
                        break;
                    }
                    i = s;
                }
            }
        }
        return best;
    }

    s32 straight_forced_stop_col(s32 x, s32 y, s32 dy, s32 r_max)
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
                size_t i = (size_t)line_lower(blocks, block_cnt, y + 1);
                while (i < (size_t)block_cnt)
                {
                    s32 o = blocks[i];
                    if (o > r_max)
                    {
                        break;
                    }
                    size_t e = block_run_end(blocks, i, (size_t)block_cnt);
                    o = blocks[e];
                    if (o > r_max)
                    {
                        break;
                    }
                    s32 oy = o + 1;
                    if (oy < height_ && walkable_[(size_t)oy * (size_t)width_ + (size_t)xs] != 0
                        && walkable_[(size_t)oy * (size_t)width_ + (size_t)x] != 0)
                    {
                        if (best < 0 || o < best)
                        {
                            best = o;
                        }
                        break;
                    }
                    i = e + 1;
                }
            }
            else
            {
                size_t i = (size_t)line_upper(blocks, block_cnt, y - 1);
                while (i > 0)
                {
                    s32 o = blocks[i - 1];
                    if (o < r_max)
                    {
                        break;
                    }
                    size_t s = block_run_start(blocks, i - 1);
                    o = blocks[s];
                    if (o < r_max)
                    {
                        break;
                    }
                    s32 oy = o - 1;
                    if (oy >= 0 && walkable_[(size_t)oy * (size_t)width_ + (size_t)xs] != 0
                        && walkable_[(size_t)oy * (size_t)width_ + (size_t)x] != 0)
                    {
                        if (best < 0 || o > best)
                        {
                            best = o;
                        }
                        break;
                    }
                    i = s;
                }
            }
        }
        return best;
    }

    s32 successor_dirs(s32 x, s32 y, s32 d, s32* out_dirs)
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
            bool walk_x = cell_walkable(x + dx, y) != 0;
            bool walk_y = cell_walkable(x, y + dy) != 0;
            if (walk_x)
            {
                out_dirs[cnt++] = dir_index(dx, 0);
            }
            if (walk_y)
            {
                out_dirs[cnt++] = dir_index(0, dy);
            }
            if (walk_x || walk_y)
            {
                out_dirs[cnt++] = d;
            }
            if (!cell_walkable(x - dx, y) && cell_walkable(x - dx, y + dy) && cell_walkable(x, y + dy))
            {
                out_dirs[cnt++] = dir_index(-dx, dy);
            }
            if (!cell_walkable(x, y - dy) && cell_walkable(x + dx, y - dy) && cell_walkable(x + dx, y))
            {
                out_dirs[cnt++] = dir_index(dx, -dy);
            }
            return cnt;
        }
        if (dx != 0)
        {
            if (cell_walkable(x + dx, y))
            {
                out_dirs[cnt++] = d;
                if (!cell_walkable(x, y + 1) && cell_walkable(x + dx, y + 1))
                {
                    out_dirs[cnt++] = dir_index(dx, 1);
                }
                if (!cell_walkable(x, y - 1) && cell_walkable(x + dx, y - 1))
                {
                    out_dirs[cnt++] = dir_index(dx, -1);
                }
            }
            return cnt;
        }
        if (cell_walkable(x, y + dy))
        {
            out_dirs[cnt++] = d;
            if (!cell_walkable(x + 1, y) && cell_walkable(x + 1, y + dy))
            {
                out_dirs[cnt++] = dir_index(1, dy);
            }
            if (!cell_walkable(x - 1, y) && cell_walkable(x - 1, y + dy))
            {
                out_dirs[cnt++] = dir_index(-1, dy);
            }
        }
        return cnt;
    }

    s32 plus_jump(s32 cell_idx, s32 d, s32 target_x, s32 target_y)
    {
        s32 dx = dir_x(d);
        s32 dy = dir_y(d);
        if (dx != 0 && dy != 0)
        {
            return jump_diag_plus(cell_idx % width_, cell_idx / width_, dx, dy, target_x, target_y);
        }
        return jump_table_lookup(cell_idx, d, target_x, target_y);
    }

    s32 jump_table_lookup(s32 cell_idx, s32 d, s32 target_x, s32 target_y)
    {
        size_t base = (size_t)cell_idx * 8;
        s32 j = jump_table_[base + d].jump;
        s32 stop = jump_table_[base + d].stop;
        s32 x = cell_idx % width_;
        s32 y = cell_idx / width_;
        s32 dx = dir_x(d);
        s32 dy = dir_y(d);
        bool on_ray = false;
        if (dx != 0 && dy != 0)
        {
            on_ray = (target_x - x) == (target_y - y) && (target_x - x) * dx > 0;
        }
        else if (dx != 0)
        {
            on_ray = target_y == y && (target_x - x) * dx > 0;
        }
        else
        {
            on_ray = target_x == x && (target_y - y) * dy > 0;
        }
        if (on_ray)
        {
            if (j >= 0)
            {
                s32 j_axis = (dx != 0) ? (j % width_) : (j / width_);
                s32 t_axis = (dx != 0) ? target_x : target_y;
                s32 s = (dx != 0) ? dx : dy;
                if ((t_axis - j_axis) * s <= 0)
                {
                    return target_y * width_ + target_x;
                }
            }
            else if (stop >= 0)
            {
                s32 stop_axis = (dx != 0) ? (stop % width_) : (stop / width_);
                s32 t_axis = (dx != 0) ? target_x : target_y;
                s32 s = (dx != 0) ? dx : dy;
                if ((t_axis - stop_axis) * s < 0)
                {
                    return target_y * width_ + target_x;
                }
            }
            else
            {
                return target_y * width_ + target_x;
            }
        }
        return j;
    }

    s32 jump_diag_plus(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y)
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
            size_t base = ((size_t)y * (size_t)width_ + (size_t)x) * 8;
            if (jump_table_[base + dir_index(dx, 0)].jump >= 0 || jump_table_[base + dir_index(0, dy)].jump >= 0)
            {
                return y * width_ + x;
            }
            if (sub_ray_target(x, y, dx, 0, target_x, target_y) || sub_ray_target(x, y, 0, dy, target_x, target_y))
            {
                return y * width_ + x;
            }
        }
    }

    bool sub_ray_target(s32 x, s32 y, s32 dx, s32 dy, s32 target_x, s32 target_y)
    {
        if (dy == 0)
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
        s32 d = dir_index(dx, dy);
        size_t base = ((size_t)y * (size_t)width_ + (size_t)x) * 8;
        s32 j = jump_table_[base + d].jump;
        s32 stop = jump_table_[base + d].stop;
        s32 t_axis = (dy == 0) ? target_x : target_y;
        s32 s = (dy == 0) ? dx : dy;
        if (j >= 0)
        {
            s32 j_axis = (dy == 0) ? (j % width_) : (j / width_);
            return (t_axis - j_axis) * s <= 0;
        }
        if (stop >= 0)
        {
            s32 stop_axis = (dy == 0) ? (stop % width_) : (stop / width_);
            return (t_axis - stop_axis) * s < 0;
        }
        return true;
    }

    static size_t block_run_end(const s32* blocks, size_t i, size_t cnt)
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

    static size_t block_run_start(const s32* blocks, size_t i)
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

    static f32 point_segment_dist_sq(f32 px, f32 py, const zpoint& source, const zpoint& target)
    {
        f32 dx = target.x - source.x;
        f32 dy = target.y - source.y;
        f32 len_sq = dx * dx + dy * dy;
        if (len_sq <= 0.0f)
        {
            f32 ex = px - source.x;
            f32 ey = py - source.y;
            return ex * ex + ey * ey;
        }
        f32 t = ((px - source.x) * dx + (py - source.y) * dy) / len_sq;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        f32 closest_x = source.x + dx * t;
        f32 closest_y = source.y + dy * t;
        f32 ex = px - closest_x;
        f32 ey = py - closest_y;
        return ex * ex + ey * ey;
    }

private:
    s32 width_ = 0;
    s32 height_ = 0;
    f32 cell_size_ = 0.0f;
    u8 default_walkable_ = 0;
    u32 map_version_ = 0;
    std::vector<u8> walkable_;

    s32 open_capacity_ = kDefaultOpenCnt;
    u32 search_stamp_ = 0;
    bool scan_f_cut_ = false;
    bool index_built_ = false;
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

#endif
