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

/*
zjps 面向小规模低精度的多层玩法
- 动态ugc类底座
- 2D 分层寻路和移动方案

zjps的grid相比navmesh不需要烘焙过程 ugc过程直接落点cache即可
相比navmesh通常会降低精度 例如从20~30cm的cell可以调制0.5m~1m 以常规游戏中的小型餐厅/农田级别的巡逻移动场景而言 navmesh 10~100us+量级 jps约20~50us附近

多层是拓扑关系而非几何距离关系 层的分割按上下重叠关系而非高度, 因此无法排除 不同层 在同一个高度, 相同层有大跨度斜坡 因此没有层高的说法
每个cell可以set一个中心高度z 用于在用户只知道pos坐标而不知道格子时 辅助定位要选择哪一层的cell z不参与连通性判断 属于用户payload    
层内搜索与单层完全一致 跳跃不跨层 层间只能靠外部显式建立的link连通

kDefaultLinkCost 是权重 由于当前层不假设和目标点的关系 并且存在性能截然相反但却常见的两种场景  
    多大平层+少楼梯   单底座+多高层建筑 因此默认是1414 即不鼓励优先选link  
    基于1414的基准下 在调整f/替换f的多种构思上 均未得到理想的改善 以业务下的场景和寻路两点 以平地a到悬崖上下bc两点来说 
        命中斜线直达目标点不绕路 而恰好这一点又需要高度启发且高度启发有效的情况很低 recast navmesh 也一样, 而当前分层图阶段用z大概率会带来全局劣化 

*/

class zjps_grid
{
public:
    static constexpr s32 kCostStraight = 1000;
    static constexpr s32 kCostDiagonal = 1414;

  
    static constexpr s32 kDefaultLinkCost = kCostDiagonal;

    static constexpr s32 kDefaultOpenCnt = 1024;

    static constexpr u8 kDefaultVoxelZ = 0;

    static constexpr s32 kTierPlus = 2;
    static constexpr s32 kTierLight = 1;
    static constexpr s32 kTierScan = 0;
    static constexpr s32 kTierAstar = -1;

    static constexpr s32 kDirRight = 0;
    static constexpr s32 kDirDown = 2;
    static constexpr s32 kDirLeft = 4;
    static constexpr s32 kDirUp = 6;

    static constexpr size_t kPlusSlotCnt = 4;


    zjps_grid() = default;

    s32 init(s32 layer_cnt, s32 width, s32 height, f32 cell_size, bool walkable_default);

    s32 layer_cnt() const { return layer_cnt_; }
    s32 width() const { return width_; }
    s32 height() const { return height_; }
    f32 cell_size() const { return cell_size_; }
    u32 map_version() const { return map_version_; }
    u32 payload_version() const { return payload_version_; }

    bool cell_walkable(s32 layer, s32 x, s32 y) const
    {
        if ((u32)layer >= (u32)layer_cnt_ || (u32)x >= (u32)width_ || (u32)y >= (u32)height_)
        {
            return false;
        }
        return cell_flag_[(size_t)layer * (size_t)plane_ + (size_t)y * (size_t)width_ + (size_t)x] != 0;
    }

    s32 set_cell(s32 layer, s32 x, s32 y, bool walkable);

    s32 set_rect_cell(s32 layer, s32 x0, s32 y0, s32 x1, s32 y1, bool walkable);

    s32 set_triangle_cell(s32 layer, const zpoint& a, const zpoint& b, const zpoint& c, bool walkable);

    s32 set_blocked(s32 layer, s32 x, s32 y)
    {
        return set_cell(layer, x, y, false);
    }

    s32 set_walkable(s32 layer, s32 x, s32 y)
    {
        return set_cell(layer, x, y, true);
    }

    s32 cell_z(s32 layer, s32 x, s32 y) const
    {
        if (!cell_valid(layer, x, y))
        {
            return -1;
        }
        return (s32)cell_voxel_[cell_index(layer, x, y)];
    }

    s32 set_cell_z(s32 layer, s32 x, s32 y, u8 voxel_z);

    //轴向偏移一格最多让h降kCostStraight 斜向最多降kCostDiagonal 代价低于此则启发高估 最优性丢失
    static s32 link_cost_floor(s32 abs_dx, s32 abs_dy)
    {
        if (abs_dx != 0 && abs_dy != 0)
        {
            return kCostDiagonal;
        }
        if (abs_dx != 0 || abs_dy != 0)
        {
            return kCostStraight;
        }
        return 1;
    }


    s32 set_link(s32 from_layer, s32 from_x, s32 from_y, s32 to_layer, s32 to_x, s32 to_y, s32 cost);

    s32 erase_link(s32 from_layer, s32 from_x, s32 from_y, s32 to_layer, s32 to_x, s32 to_y);

    s32 link_count() const { return (s32)link_edges_.size(); }

    bool has_link(s32 layer, s32 x, s32 y) const
    {
        if (!cell_valid(layer, x, y))
        {
            return false;
        }
        return link_flag_[cell_index(layer, x, y)] != 0;
    }

 
    s32 pos_to_cell(f32 px, f32 py, s32& out_x, s32& out_y) const;

    s32 cell_to_pos(s32 layer, s32 x, s32 y, f32& out_x, f32& out_y, f32& out_z) const
    {
        if (!cell_valid(layer, x, y))
        {
            return -1;
        }
        out_x = ((f32)x + 0.5f) * cell_size_;
        out_y = ((f32)y + 0.5f) * cell_size_;
        out_z = (f32)cell_voxel_[cell_index(layer, x, y)] * cell_size_;
        return 0;
    }


    s32 cell_layer_of(s32 cell_idx) const { return cell_idx / plane_; }
    s32 cell_x_of(s32 cell_idx) const { return cell_idx % width_; }
    s32 cell_y_of(s32 cell_idx) const { return cell_idx % plane_ / width_; }

    bool move_valid(s32 layer, s32 x, s32 y, s32 dx, s32 dy) const;

    s32 build_jps_light();

    s32 drop_jps_plus()
    {
        plus_table_.clear();
        plus_table_version_ = 0;
        return 0;
    }

    s32 build_jps_plus();

    s32 astar_search(s32 start_layer, s32 start_x, s32 start_y, s32 target_layer, s32 target_x, s32 target_y, std::vector<s32>& out_cells);

    s32 jps_search(s32 start_layer, s32 start_x, s32 start_y, s32 target_layer, s32 target_x, s32 target_y, std::vector<s32>& out_cells);

    s32 find_path(s32 start_layer, s32 start_x, s32 start_y, s32 target_layer, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
    {
        return jps_search(start_layer, start_x, start_y, target_layer, target_x, target_y, out_cells);
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
    //link边的入向哨兵 同样满足<0 因此successor_dirs会展开全部8方向 换层等于在新层重新起步 没有可用来剪枝的入向
    static constexpr s8 kDirLink = -2;

    struct search_state
    {
        s32 gone_cost = 0;
        s32 came_from = kNoCell;
        u32 stamp = 0;
        u8 closed = 0;
        s8 entry_dir = kNoDir;
    };

    struct link_edge
    {
        s32 from_cell;
        s32 to_cell;
        s32 cost;
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

    bool cell_valid(s32 layer, s32 x, s32 y) const
    {
        return (u32)layer < (u32)layer_cnt_ && (u32)x < (u32)width_ && (u32)y < (u32)height_;
    }

    size_t cell_index(s32 layer, s32 x, s32 y) const
    {
        return (size_t)layer * (size_t)plane_ + (size_t)y * (size_t)width_ + (size_t)x;
    }

    s32 cell_code(s32 layer, s32 x, s32 y) const
    {
        return layer * plane_ + y * width_ + x;
    }

    //层已知时的解码 减掉层基址后一次除法同时得到xy 与单层实现同代价
    void cell_xy_in_layer(s32 cell_idx, s32 layer, s32& out_x, s32& out_y) const
    {
        s32 local = cell_idx - layer * plane_;
        out_x = local % width_;
        out_y = local / width_;
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

    //松弛一条边并入堆 A*与JPS的层内步和link步共用 返回非0表示open溢出
    //next的xy由调用方给出 层内步能直接算出来 省掉这里的两次除法
    s32 relax_to(s32 cur, s32 next, s32 next_x, s32 next_y, s32 step_cost, s8 entry_dir, s32 target_x, s32 target_y)
    {
        s32 next_gone_cost = search_states_[cur].gone_cost + step_cost;
        if (search_states_[next].stamp == search_stamp_ && next_gone_cost >= search_states_[next].gone_cost)
        {
            return 0;
        }
        search_states_[next].gone_cost = next_gone_cost;
        search_states_[next].came_from = cur;
        search_states_[next].entry_dir = entry_dir;
        search_states_[next].stamp = search_stamp_;
        search_states_[next].closed = 0;
        visit_count_++;
        s32 heuristic_cost = octile_to(next_x, next_y, target_x, target_y);
        return open_push(next_gone_cost + heuristic_cost, next_gone_cost, next);
    }

    //展开当前节点的全部link出边 A*与JPS共用
    s32 relax_links(s32 cur, s32 target_x, s32 target_y)
    {
        for (size_t k = link_edge_lower_bound(cur); k < link_edges_.size() && link_edges_[k].from_cell == cur; k++)
        {
            s32 next = link_edges_[k].to_cell;
            if (cell_flag_[next] == 0)
            {
                continue;
            }
            if (relax_to(cur, next, cell_x_of(next), cell_y_of(next), link_edges_[k].cost, kDirLink, target_x, target_y) != 0)
            {
                return -3;
            }
        }
        return 0;
    }

    static s32 sorted_lower_bound(const s32* vals, s32 cnt, s32 v);

    static s32 sorted_upper_bound(const s32* vals, s32 cnt, s32 v);

    static s32 sorted_insert(s32* vals, s32& cnt, s32 cap, s32 v);

    static s32 sorted_erase(s32* vals, s32& cnt, s32 v);

    static size_t block_run_end(const s32* blocks, size_t i, size_t cnt);

    static size_t block_run_start(const s32* blocks, size_t i);

    //每层各占一段连续的行/列表 层内布局与单层完全一致 因此refill与探测的内部逻辑不需要感知层
    s32* light_row_head(s32 layer, s32 y) const
    {
        return const_cast<s32*>(light_row_.data()) + ((size_t)layer * (size_t)height_ + (size_t)y) * (size_t)(width_ + 1);
    }

    s32* light_col_head(s32 layer, s32 x) const
    {
        return const_cast<s32*>(light_col_.data()) + ((size_t)layer * (size_t)width_ + (size_t)x) * (size_t)(height_ + 1);
    }

    block_line light_row_line(s32 layer, s32 y) const
    {
        const s32* head = light_row_head(layer, y);
        return block_line{ head + 1, head[0] };
    }

    block_line light_col_line(s32 layer, s32 x) const
    {
        const s32* head = light_col_head(layer, x);
        return block_line{ head + 1, head[0] };
    }

    block_line_mut light_row_line_mut(s32 layer, s32 y)
    {
        s32* head = light_row_head(layer, y);
        return block_line_mut{ head + 1, head };
    }

    block_line_mut light_col_line_mut(s32 layer, s32 x)
    {
        s32* head = light_col_head(layer, x);
        return block_line_mut{ head + 1, head };
    }

    s32 refill_light_row(s32 layer, s32 y);

    s32 refill_light_col(s32 layer, s32 x);

    void mark_light_dirty_row(s32 layer, s32 y)
    {
        size_t idx = (size_t)layer * (size_t)height_ + (size_t)y;
        if (light_dirty_row_[idx] == 0)
        {
            light_dirty_row_[idx] = 1;
            light_dirty_cnt_++;
        }
    }

    void mark_light_dirty_col(s32 layer, s32 x)
    {
        size_t idx = (size_t)layer * (size_t)width_ + (size_t)x;
        if (light_dirty_col_[idx] == 0)
        {
            light_dirty_col_[idx] = 1;
            light_dirty_cnt_++;
        }
    }

    static_assert(kZjpsDirX[kDirRight] == 1 && kZjpsDirY[kDirRight] == 0
        && kZjpsDirX[kDirDown] == 0 && kZjpsDirY[kDirDown] == 1
        && kZjpsDirX[kDirLeft] == -1 && kZjpsDirY[kDirLeft] == 0
        && kZjpsDirX[kDirUp] == 0 && kZjpsDirY[kDirUp] == -1,
        "axis dir codes must pair with kZjpsDirX/kZjpsDirY layout");

    static_assert(kDirRight / 2 == 0 && kDirDown / 2 == 1
        && kDirLeft / 2 == 2 && kDirUp / 2 == 3 && kPlusSlotCnt == 4,
        "axis dirs must fill distinct plus slots by d/2");

    static size_t plus_slot(s32 cell_idx, s32 d)
    {
        return (size_t)cell_idx * kPlusSlotCnt + (size_t)(d / 2);
    }

    bool side_forced_row(s32 layer, s32 x, s32 y, s32 entry_step, s32 side) const
    {
        return cell_walkable(layer, x, y + side) && !cell_walkable(layer, x - entry_step, y + side);
    }

    bool side_forced_col(s32 layer, s32 x, s32 y, s32 entry_step, s32 side) const
    {
        return cell_walkable(layer, x + side, y) && !cell_walkable(layer, x + side, y - entry_step);
    }

    bool at_forced_turn(s32 layer, s32 x, s32 y, s32 entry_dx, s32 entry_dy) const;


    bool at_jump_stop(s32 layer, s32 x, s32 y, s32 entry_dx, s32 entry_dy) const
    {
        if (has_link_edges_ && link_flag_[cell_index(layer, x, y)] != 0)
        {
            return true;
        }
        return at_forced_turn(layer, x, y, entry_dx, entry_dy);
    }

    s32 successor_dirs(s32 layer, s32 x, s32 y, s32 entry_dir, s32* out_dirs) const;

    s32 rebuild_path(s32 target, std::vector<s32>& out_cells);

    s32 jump(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y, s32 tier) const;

    s32 probe_next_cell(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y, s32 tier) const;

    bool probe_has_next_cell(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y, s32 tier) const
    {
        return probe_next_cell(layer, x, y, dx, dy, target_layer, target_x, target_y, tier) >= 0;
    }

    s32 probe_next_cell_by_scan(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y) const;

    s32 probe_next_cell_by_light(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y) const;

    s32 probe_next_cell_by_real_light(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y) const;

    s32 probe_next_cell_by_real_light_row(s32 layer, s32 x, s32 y, s32 step, s32 target_layer, s32 target_x, s32 target_y) const;

    s32 probe_next_cell_by_real_light_col(s32 layer, s32 x, s32 y, s32 step, s32 target_layer, s32 target_x, s32 target_y) const;

    s32 light_forced_turn_row(s32 layer, s32 y, s32 x, s32 step, s32 reach_col) const;

    s32 light_forced_turn_col(s32 layer, s32 x, s32 y, s32 step, s32 reach_row) const;

    //light档二分会直接跳到reach处 因此需要把范围内最近的link格作为第三候选补进来
    s32 light_link_stop_row(s32 layer, s32 y, s32 x, s32 step, s32 reach_col) const;

    s32 light_link_stop_col(s32 layer, s32 x, s32 y, s32 step, s32 reach_row) const;

    s32 probe_next_cell_by_plus(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y) const;

    bool probe_dir_hit_target_by_plus(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y) const;

    size_t link_edge_lower_bound(s32 from_cell) const;

    size_t link_col_key_lower_bound(s32 col_key) const;

    s32 col_key_of(s32 layer, s32 x, s32 y) const
    {
        return layer * plane_ + x * height_ + y;
    }

    s32 layer_cnt_ = 0;
    s32 width_ = 0;
    s32 height_ = 0;
    s32 plane_ = 0;
    f32 cell_size_ = 0.0f;
    u8 default_walkable_ = 0;
    u32 map_version_ = 0;
    u32 payload_version_ = 0;
    std::vector<u8> cell_flag_;
    std::vector<u8> cell_voxel_;

    //link数量很少 有序表直接增量维护 不需要dirty与rebuild
    //link_flag_是纯索引缓存 服务scan热路径的O(1)谓词
    std::vector<u8> link_flag_;
    std::vector<link_edge> link_edges_;
    std::vector<s32> link_col_keys_;
    bool has_link_edges_ = false;

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

inline s32 zjps_grid::init(s32 layer_cnt, s32 width, s32 height, f32 cell_size, bool walkable_default)
{
    if (layer_cnt <= 0 || width <= 0 || height <= 0 || cell_size <= 0.0f)
    {
        return -1;
    }
    layer_cnt_ = layer_cnt;
    width_ = width;
    height_ = height;
    plane_ = width * height;
    cell_size_ = cell_size;
    default_walkable_ = walkable_default ? 1 : 0;

    size_t cell_cnt = (size_t)plane_ * (size_t)layer_cnt_;
    cell_flag_.assign(cell_cnt, default_walkable_);
    u8 voxel_init = kDefaultVoxelZ;
    cell_voxel_.assign(cell_cnt, voxel_init);
    link_flag_.assign(cell_cnt, 0);
    link_edges_.clear();
    link_col_keys_.clear();
    has_link_edges_ = false;
    light_dirty_row_.assign((size_t)height_ * (size_t)layer_cnt_, 0);
    light_dirty_col_.assign((size_t)width_ * (size_t)layer_cnt_, 0);
    light_dirty_cnt_ = 0;

    light_row_.assign((size_t)height_ * (size_t)layer_cnt_ * (size_t)(width_ + 1), 0);
    light_col_.assign((size_t)width_ * (size_t)layer_cnt_ * (size_t)(height_ + 1), 0);

    search_states_.resize(cell_cnt);
    open_heap_.reserve((size_t)open_capacity_);

    map_version_++;
    light_built_ = false;
    plus_table_.clear();
    plus_table_version_ = 0;
    return 0;
}

inline s32 zjps_grid::set_cell(s32 layer, s32 x, s32 y, bool walkable)
{
    if (!cell_valid(layer, x, y))
    {
        return -1;
    }
    size_t idx = cell_index(layer, x, y);
    u8 old = cell_flag_[idx];
    u8 next = walkable ? 1 : 0;
    if (old == next)
    {
        return 0;
    }
    cell_flag_[idx] = next;
    map_version_++;
    block_line_mut row = light_row_line_mut(layer, y);
    block_line_mut col = light_col_line_mut(layer, x);
    if (!light_built_)
    {
        //nothing todo
    }
    else if (light_dirty_row_[(size_t)layer * (size_t)height_ + (size_t)y] != 0
             || light_dirty_col_[(size_t)layer * (size_t)width_ + (size_t)x] != 0)
    {
        mark_light_dirty_row(layer, y);
        mark_light_dirty_col(layer, x);
    }
    else if (next != 0)
    {
        sorted_erase(row.blocks, *row.cnt, x);
        sorted_erase(col.blocks, *col.cnt, y);
    }
    else if (sorted_insert(row.blocks, *row.cnt, width_, x) != 0
             || sorted_insert(col.blocks, *col.cnt, height_, y) != 0)
    {
        mark_light_dirty_row(layer, y);
        mark_light_dirty_col(layer, x);
    }
    return 0;
}

inline s32 zjps_grid::set_rect_cell(s32 layer, s32 x0, s32 y0, s32 x1, s32 y1, bool walkable)
{
    if (layer < 0 || layer >= layer_cnt_
        || x0 < 0 || y0 < 0 || x0 > x1 || y0 > y1 || x1 >= width_ || y1 >= height_)
    {
        return -1;
    }
    u8 next = walkable ? 1 : 0;
    bool changed = false;
    for (s32 y = y0; y <= y1; y++)
    {
        u8* row = cell_flag_.data() + cell_index(layer, 0, y);
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
            mark_light_dirty_row(layer, y);
        }
        for (s32 x = x0; x <= x1; x++)
        {
            mark_light_dirty_col(layer, x);
        }
    }
    return 0;
}

inline s32 zjps_grid::set_triangle_cell(s32 layer, const zpoint& a, const zpoint& b, const zpoint& c, bool walkable)
{
    if (layer < 0 || layer >= layer_cnt_ || cell_size_ <= 0.0f)
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
        u8* row = cell_flag_.data() + cell_index(layer, 0, y);
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
            mark_light_dirty_row(layer, y);
        }
        for (s32 x = x0; x <= x1; x++)
        {
            mark_light_dirty_col(layer, x);
        }
    }
    return 0;
}

inline s32 zjps_grid::set_cell_z(s32 layer, s32 x, s32 y, u8 voxel_z)
{
    if (!cell_valid(layer, x, y))
    {
        return -1;
    }
    size_t idx = cell_index(layer, x, y);
    if (cell_voxel_[idx] == voxel_z)
    {
        return 0;
    }
    cell_voxel_[idx] = voxel_z;
    //z是纯payload 不参与连通性 因此不能bump map_version_ 否则会让plus表无谓失效
    payload_version_++;
    return 0;
}

inline size_t zjps_grid::link_edge_lower_bound(s32 from_cell) const
{
    size_t lo = 0;
    size_t hi = link_edges_.size();
    while (lo < hi)
    {
        size_t mid = lo + (hi - lo) / 2;
        if (link_edges_[mid].from_cell < from_cell)
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

inline size_t zjps_grid::link_col_key_lower_bound(s32 col_key) const
{
    size_t lo = 0;
    size_t hi = link_col_keys_.size();
    while (lo < hi)
    {
        size_t mid = lo + (hi - lo) / 2;
        if (link_col_keys_[mid] < col_key)
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

inline s32 zjps_grid::set_link(s32 from_layer, s32 from_x, s32 from_y, s32 to_layer, s32 to_x, s32 to_y, s32 cost)
{
    if (!cell_valid(from_layer, from_x, from_y) || !cell_valid(to_layer, to_x, to_y))
    {
        return -1;
    }

    s32 layer_step = to_layer > from_layer ? to_layer - from_layer : from_layer - to_layer;
    s32 abs_dx = to_x > from_x ? to_x - from_x : from_x - to_x;
    s32 abs_dy = to_y > from_y ? to_y - from_y : from_y - to_y;
    if (layer_step != 1 || abs_dx > 1 || abs_dy > 1)
    {
        return -2;
    }

    if (cost < link_cost_floor(abs_dx, abs_dy))
    {
        return -3;
    }
    s32 from_cell = cell_code(from_layer, from_x, from_y);
    s32 to_cell = cell_code(to_layer, to_x, to_y);
    size_t pos = link_edge_lower_bound(from_cell);
    while (pos < link_edges_.size() && link_edges_[pos].from_cell == from_cell)
    {
        if (link_edges_[pos].to_cell == to_cell)
        {
            link_edges_[pos].cost = cost;
            return 0;
        }
        pos++;
    }
    link_edge edge;
    edge.from_cell = from_cell;
    edge.to_cell = to_cell;
    edge.cost = cost;
    link_edges_.insert(link_edges_.begin() + (std::ptrdiff_t)pos, edge);

    s32 col_key = col_key_of(from_layer, from_x, from_y);
    size_t col_pos = link_col_key_lower_bound(col_key);
    link_col_keys_.insert(link_col_keys_.begin() + (std::ptrdiff_t)col_pos, col_key);

    link_flag_[cell_index(from_layer, from_x, from_y)] = 1;
    has_link_edges_ = true;
    map_version_++;
    return 0;
}

inline s32 zjps_grid::erase_link(s32 from_layer, s32 from_x, s32 from_y, s32 to_layer, s32 to_x, s32 to_y)
{
    if (!cell_valid(from_layer, from_x, from_y) || !cell_valid(to_layer, to_x, to_y))
    {
        return -1;
    }
    s32 from_cell = cell_code(from_layer, from_x, from_y);
    s32 to_cell = cell_code(to_layer, to_x, to_y);
    size_t pos = link_edge_lower_bound(from_cell);
    size_t hit = link_edges_.size();
    s32 left_cnt = 0;
    while (pos < link_edges_.size() && link_edges_[pos].from_cell == from_cell)
    {
        if (link_edges_[pos].to_cell == to_cell)
        {
            hit = pos;
        }
        else
        {
            left_cnt++;
        }
        pos++;
    }
    if (hit == link_edges_.size())
    {
        return 0;
    }
    link_edges_.erase(link_edges_.begin() + (std::ptrdiff_t)hit);

    s32 col_key = col_key_of(from_layer, from_x, from_y);
    size_t col_pos = link_col_key_lower_bound(col_key);
    if (col_pos < link_col_keys_.size() && link_col_keys_[col_pos] == col_key)
    {
        link_col_keys_.erase(link_col_keys_.begin() + (std::ptrdiff_t)col_pos);
    }
    if (left_cnt == 0)
    {
        link_flag_[cell_index(from_layer, from_x, from_y)] = 0;
    }
    has_link_edges_ = !link_edges_.empty();
    map_version_++;
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

inline bool zjps_grid::move_valid(s32 layer, s32 x, s32 y, s32 dx, s32 dy) const
{
    if (dx == 0 && dy == 0)
    {
        return false;
    }
    if (!cell_walkable(layer, x + dx, y + dy))
    {
        return false;
    }
    if (dx != 0 && dy != 0)
    {
        return cell_walkable(layer, x + dx, y) && cell_walkable(layer, x, y + dy);
    }
    return true;
}

inline s32 zjps_grid::astar_search(s32 start_layer, s32 start_x, s32 start_y, s32 target_layer, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
{
    out_cells.clear();
    if (!cell_valid(start_layer, start_x, start_y) || !cell_valid(target_layer, target_x, target_y))
    {
        return -1;
    }
    s32 start_idx = cell_code(start_layer, start_x, start_y);
    s32 target_idx = cell_code(target_layer, target_x, target_y);
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
        s32 layer = cell_layer_of(cur);
        s32 x = 0;
        s32 y = 0;
        cell_xy_in_layer(cur, layer, x, y);
        for (s32 d = 0; d < kDirCnt; d++)
        {
            s32 dx = dir_x(d);
            s32 dy = dir_y(d);
            if (!move_valid(layer, x, y, dx, dy))
            {
                continue;
            }
            s32 next = cell_code(layer, x + dx, y + dy);
            s32 step_cost = (dx != 0 && dy != 0) ? kCostDiagonal : kCostStraight;
            if (relax_to(cur, next, x + dx, y + dy, step_cost, (s8)d, target_x, target_y) != 0)
            {
                return -3;
            }
        }
        if (relax_links(cur, target_x, target_y) != 0)
        {
            return -3;
        }
    }
    return -2;
}

inline s32 zjps_grid::build_jps_light()
{
    if (layer_cnt_ <= 0 || width_ <= 0 || height_ <= 0)
    {
        return -1;
    }
    if (light_built_ && light_dirty_cnt_ == 0)
    {
        return 0;
    }
    if (!light_built_)
    {
        for (s32 layer = 0; layer < layer_cnt_; layer++)
        {
            for (s32 y = 0; y < height_; y++)
            {
                refill_light_row(layer, y);
            }
            for (s32 x = 0; x < width_; x++)
            {
                refill_light_col(layer, x);
            }
        }
        light_built_ = true;
        return 0;
    }
    for (s32 layer = 0; layer < layer_cnt_; layer++)
    {
        for (s32 y = 0; y < height_; y++)
        {
            size_t idx = (size_t)layer * (size_t)height_ + (size_t)y;
            if (light_dirty_row_[idx] != 0)
            {
                refill_light_row(layer, y);
                light_dirty_row_[idx] = 0;
                light_dirty_cnt_--;
            }
        }
        for (s32 x = 0; x < width_; x++)
        {
            size_t idx = (size_t)layer * (size_t)width_ + (size_t)x;
            if (light_dirty_col_[idx] != 0)
            {
                refill_light_col(layer, x);
                light_dirty_col_[idx] = 0;
                light_dirty_cnt_--;
            }
        }
    }
    return 0;
}

inline s32 zjps_grid::build_jps_plus()
{
    if (layer_cnt_ <= 0 || width_ <= 0 || height_ <= 0)
    {
        return -1;
    }
    size_t cell_cnt = (size_t)plane_ * (size_t)layer_cnt_;
    size_t slot_cnt = cell_cnt * kPlusSlotCnt;
    if (plus_table_.capacity() < slot_cnt)
    {
        plus_table_.reserve(slot_cnt);
    }
    plus_table_.assign(slot_cnt, plus_ray());
    for (s32 layer = 0; layer < layer_cnt_; layer++)
    {
        for (s32 y = 0; y < height_; y++)
        {
            for (s32 x = width_ - 2; x >= 0; x--)
            {
                s32 next = x + 1;
                size_t base = plus_slot(cell_code(layer, x, y), kDirRight);
                size_t next_base = plus_slot(cell_code(layer, next, y), kDirRight);
                if (cell_flag_[cell_index(layer, next, y)] == 0)
                {
                    plus_table_[base].first_block_cell = cell_code(layer, next, y);
                    continue;
                }
                if (at_jump_stop(layer, next, y, 1, 0))
                {
                    plus_table_[base].first_turn_cell = cell_code(layer, next, y);
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
                size_t base = plus_slot(cell_code(layer, x, y), kDirLeft);
                size_t next_base = plus_slot(cell_code(layer, next, y), kDirLeft);
                if (cell_flag_[cell_index(layer, next, y)] == 0)
                {
                    plus_table_[base].first_block_cell = cell_code(layer, next, y);
                    continue;
                }
                if (at_jump_stop(layer, next, y, -1, 0))
                {
                    plus_table_[base].first_turn_cell = cell_code(layer, next, y);
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
                size_t base = plus_slot(cell_code(layer, x, y), kDirDown);
                size_t next_base = plus_slot(cell_code(layer, x, next), kDirDown);
                if (cell_flag_[cell_index(layer, x, next)] == 0)
                {
                    plus_table_[base].first_block_cell = cell_code(layer, x, next);
                    continue;
                }
                if (at_jump_stop(layer, x, next, 0, 1))
                {
                    plus_table_[base].first_turn_cell = cell_code(layer, x, next);
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
                size_t base = plus_slot(cell_code(layer, x, y), kDirUp);
                size_t next_base = plus_slot(cell_code(layer, x, next), kDirUp);
                if (cell_flag_[cell_index(layer, x, next)] == 0)
                {
                    plus_table_[base].first_block_cell = cell_code(layer, x, next);
                    continue;
                }
                if (at_jump_stop(layer, x, next, 0, -1))
                {
                    plus_table_[base].first_turn_cell = cell_code(layer, x, next);
                }
                else
                {
                    plus_table_[base].first_turn_cell = plus_table_[next_base].first_turn_cell;
                }
                plus_table_[base].first_block_cell = plus_table_[next_base].first_block_cell;
            }
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
        if (entry_dir == kDirLink)
        {
            //link边不是直线 没有中间格可插值 直接接上父节点
            node = parent;
            continue;
        }
        s32 dx = dir_x(entry_dir);
        s32 dy = dir_y(entry_dir);
        s32 layer = cell_layer_of(node);
        s32 cx = 0;
        s32 cy = 0;
        cell_xy_in_layer(node, layer, cx, cy);
        cx -= dx;
        cy -= dy;
        while (cell_code(layer, cx, cy) != parent)
        {
            if (cx < 0 || cy < 0 || cx >= width_ || cy >= height_)
            {
                return -1;
            }
            out_cells.push_back(cell_code(layer, cx, cy));
            cx -= dx;
            cy -= dy;
        }
        node = parent;
    }
    std::reverse(out_cells.begin(), out_cells.end());
    return 0;
}

inline s32 zjps_grid::jps_search(s32 start_layer, s32 start_x, s32 start_y, s32 target_layer, s32 target_x, s32 target_y, std::vector<s32>& out_cells)
{
    out_cells.clear();
    if (!cell_valid(start_layer, start_x, start_y) || !cell_valid(target_layer, target_x, target_y))
    {
        return -1;
    }
    s32 start_idx = cell_code(start_layer, start_x, start_y);
    s32 target_idx = cell_code(target_layer, target_x, target_y);
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
        s32 layer = cell_layer_of(cur);
        s32 x = 0;
        s32 y = 0;
        cell_xy_in_layer(cur, layer, x, y);
        s32 entry_dir = search_states_[cur].entry_dir;
        s32 dir_cnt = successor_dirs(layer, x, y, entry_dir, dirs);
        for (s32 k = 0; k < dir_cnt; k++)
        {
            s32 d = dirs[k];
            s32 dx = dir_x(d);
            s32 dy = dir_y(d);
            s32 jump_cell = jump(layer, x, y, dx, dy, target_layer, target_x, target_y, last_tier_);
            if (jump_cell < 0)
            {
                continue;
            }
            s32 jump_x = 0;
            s32 jump_y = 0;
            cell_xy_in_layer(jump_cell, layer, jump_x, jump_y);
            if (relax_to(cur, jump_cell, jump_x, jump_y, octile_to(x, y, jump_x, jump_y), (s8)d, target_x, target_y) != 0)
            {
                return -3;
            }
        }
        if (relax_links(cur, target_x, target_y) != 0)
        {
            return -3;
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

inline s32 zjps_grid::refill_light_row(s32 layer, s32 y)
{
    const u8* row = cell_flag_.data() + cell_index(layer, 0, y);
    block_line_mut line = light_row_line_mut(layer, y);
    s32 cnt = 0;
    for (s32 x = 0; x < width_; x++)
    {
        if (row[x] == 0)
        {
            line.blocks[cnt++] = x;
        }
    }
    *line.cnt = cnt;
    return 0;
}

inline s32 zjps_grid::refill_light_col(s32 layer, s32 x)
{
    block_line_mut line = light_col_line_mut(layer, x);
    s32 cnt = 0;
    for (s32 y = 0; y < height_; y++)
    {
        if (cell_flag_[cell_index(layer, x, y)] == 0)
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

inline bool zjps_grid::at_forced_turn(s32 layer, s32 x, s32 y, s32 entry_dx, s32 entry_dy) const
{
    if (!dir_is_axis(entry_dx, entry_dy))
    {
        return false;
    }
    if (entry_dx != 0)
    {
        return side_forced_row(layer, x, y, entry_dx, -1)
            || side_forced_row(layer, x, y, entry_dx, 1);
    }
    return side_forced_col(layer, x, y, entry_dy, -1)
        || side_forced_col(layer, x, y, entry_dy, 1);
}

inline s32 zjps_grid::jump(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y, s32 tier) const
{
    if (dir_is_axis(dx, dy))
    {
        //轴方向 返回目标或拐点中先遇到者 探空则该方向无后继
        return probe_next_cell(layer, x, y, dx, dy, target_layer, target_x, target_y, tier);
    }

    //斜线 踏上目标返回目标 每步轴探测看见拐点或目标则返回当前步 看见的点由下一跳重新发现
    while (true)
    {
        if (!move_valid(layer, x, y, dx, dy))
        {
            return kNoCell;
        }
        x += dx;
        y += dy;
        if (layer == target_layer && x == target_x && y == target_y)
        {
            return cell_code(layer, x, y);
        }
        if (at_jump_stop(layer, x, y, dx, dy))
        {
            return cell_code(layer, x, y);
        }
        if (probe_has_next_cell(layer, x, y, dx, 0, target_layer, target_x, target_y, tier))
        {
            return cell_code(layer, x, y);
        }
        if (probe_has_next_cell(layer, x, y, 0, dy, target_layer, target_x, target_y, tier))
        {
            return cell_code(layer, x, y);
        }
    }
}

inline s32 zjps_grid::probe_next_cell(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y, s32 tier) const
{
    if (!dir_is_axis(dx, dy))
    {
        return kNoCell;
    }
    if (tier == kTierPlus)
    {
        return probe_next_cell_by_plus(layer, x, y, dx, dy, target_layer, target_x, target_y);
    }
    if (tier == kTierLight)
    {
        return probe_next_cell_by_light(layer, x, y, dx, dy, target_layer, target_x, target_y);
    }
    return probe_next_cell_by_scan(layer, x, y, dx, dy, target_layer, target_x, target_y);
}

inline s32 zjps_grid::probe_next_cell_by_scan(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y) const
{
    while (true)
    {
        if (!move_valid(layer, x, y, dx, dy))
        {
            return kNoCell;
        }
        x += dx;
        y += dy;
        if (layer == target_layer && x == target_x && y == target_y)
        {
            return cell_code(layer, x, y);
        }
        if (at_jump_stop(layer, x, y, dx, dy))
        {
            return cell_code(layer, x, y);
        }
    }
}

inline s32 zjps_grid::probe_next_cell_by_light(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y) const
{
    s32 fast_probe_left = kFastProbeSteps;
    while (fast_probe_left > 0)
    {
        if (!move_valid(layer, x, y, dx, dy))
        {
            return kNoCell;
        }
        x += dx;
        y += dy;
        if (layer == target_layer && x == target_x && y == target_y)
        {
            return cell_code(layer, x, y);
        }
        if (at_jump_stop(layer, x, y, dx, dy))
        {
            return cell_code(layer, x, y);
        }
        fast_probe_left--;
    }
    return probe_next_cell_by_real_light(layer, x, y, dx, dy, target_layer, target_x, target_y);
}

inline s32 zjps_grid::probe_next_cell_by_real_light(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y) const
{
    if (dx != 0)
    {
        return probe_next_cell_by_real_light_row(layer, x, y, dx, target_layer, target_x, target_y);
    }
    return probe_next_cell_by_real_light_col(layer, x, y, dy, target_layer, target_x, target_y);
}

inline s32 zjps_grid::probe_next_cell_by_real_light_row(s32 layer, s32 x, s32 y, s32 step, s32 target_layer, s32 target_x, s32 target_y) const
{
    block_line row = light_row_line(layer, y);
    s32 reach_col;
    if (true)
    {
        s32 block_at;
        if (step > 0)
        {
            s32 pos = sorted_lower_bound(row.blocks, row.cnt, x + 1);
            block_at = pos < row.cnt ? row.blocks[pos] : -1;
        }
        else
        {
            s32 pos = sorted_upper_bound(row.blocks, row.cnt, x - 1);
            block_at = pos > 0 ? row.blocks[pos - 1] : -1;
        }
        reach_col = block_at < 0 ? (step > 0 ? width_ - 1 : 0) : block_at - step;
        if (step > 0 && reach_col > width_ - 1)
        {
            reach_col = width_ - 1;
        }
        if (step < 0 && reach_col < 0)
        {
            reach_col = 0;
        }
    }
    s32 jump_col;
    if (true)
    {
        jump_col = -1;
        if (layer == target_layer && y == target_y
            && ((step > 0 && target_x > x && target_x <= reach_col) || (step < 0 && target_x < x && target_x >= reach_col)))
        {
            jump_col = target_x;
        }
    }
    if (true)
    {
        s32 forced = light_forced_turn_row(layer, y, x, step, reach_col);
        if (forced >= 0 && (jump_col < 0 || (step > 0 ? forced < jump_col : forced > jump_col)))
        {
            jump_col = forced;
        }
    }
    if (true)
    {
        s32 link_stop = light_link_stop_row(layer, y, x, step, reach_col);
        if (link_stop >= 0 && (jump_col < 0 || (step > 0 ? link_stop < jump_col : link_stop > jump_col)))
        {
            jump_col = link_stop;
        }
    }
    if (jump_col < 0)
    {
        return kNoCell;
    }
    return cell_code(layer, jump_col, y);
}

inline s32 zjps_grid::probe_next_cell_by_real_light_col(s32 layer, s32 x, s32 y, s32 step, s32 target_layer, s32 target_x, s32 target_y) const
{
    block_line col = light_col_line(layer, x);
    s32 reach_row;
    if (true)
    {
        s32 block_at;
        if (step > 0)
        {
            s32 pos = sorted_lower_bound(col.blocks, col.cnt, y + 1);
            block_at = pos < col.cnt ? col.blocks[pos] : -1;
        }
        else
        {
            s32 pos = sorted_upper_bound(col.blocks, col.cnt, y - 1);
            block_at = pos > 0 ? col.blocks[pos - 1] : -1;
        }
        reach_row = block_at < 0 ? (step > 0 ? height_ - 1 : 0) : block_at - step;
        if (step > 0 && reach_row > height_ - 1)
        {
            reach_row = height_ - 1;
        }
        if (step < 0 && reach_row < 0)
        {
            reach_row = 0;
        }
    }
    s32 jump_row;
    if (true)
    {
        jump_row = -1;
        if (layer == target_layer && x == target_x
            && ((step > 0 && target_y > y && target_y <= reach_row) || (step < 0 && target_y < y && target_y >= reach_row)))
        {
            jump_row = target_y;
        }
    }
    if (true)
    {
        s32 forced = light_forced_turn_col(layer, x, y, step, reach_row);
        if (forced >= 0 && (jump_row < 0 || (step > 0 ? forced < jump_row : forced > jump_row)))
        {
            jump_row = forced;
        }
    }
    if (true)
    {
        s32 link_stop = light_link_stop_col(layer, x, y, step, reach_row);
        if (link_stop >= 0 && (jump_row < 0 || (step > 0 ? link_stop < jump_row : link_stop > jump_row)))
        {
            jump_row = link_stop;
        }
    }
    if (jump_row < 0)
    {
        return kNoCell;
    }
    return cell_code(layer, x, jump_row);
}

inline s32 zjps_grid::light_link_stop_row(s32 layer, s32 y, s32 x, s32 step, s32 reach_col) const
{
    if (link_edges_.empty())
    {
        return -1;
    }
    //行内cell编号连续 因此from_cell有序表直接支持行范围查询
    s32 lo_col = step > 0 ? x + 1 : reach_col;
    s32 hi_col = step > 0 ? reach_col : x - 1;
    if (lo_col > hi_col)
    {
        return -1;
    }
    if (step > 0)
    {
        size_t pos = link_edge_lower_bound(cell_code(layer, lo_col, y));
        if (pos >= link_edges_.size())
        {
            return -1;
        }
        s32 hit = link_edges_[pos].from_cell;
        return hit <= cell_code(layer, hi_col, y) ? cell_x_of(hit) : -1;
    }
    size_t pos = link_edge_lower_bound(cell_code(layer, hi_col, y) + 1);
    if (pos == 0)
    {
        return -1;
    }
    s32 hit = link_edges_[pos - 1].from_cell;
    return hit >= cell_code(layer, lo_col, y) ? cell_x_of(hit) : -1;
}

inline s32 zjps_grid::light_link_stop_col(s32 layer, s32 x, s32 y, s32 step, s32 reach_row) const
{
    if (link_col_keys_.empty())
    {
        return -1;
    }
    //列内cell编号不连续 因此另用一份按列主序的键表支持列范围查询
    s32 lo_row = step > 0 ? y + 1 : reach_row;
    s32 hi_row = step > 0 ? reach_row : y - 1;
    if (lo_row > hi_row)
    {
        return -1;
    }
    if (step > 0)
    {
        size_t pos = link_col_key_lower_bound(col_key_of(layer, x, lo_row));
        if (pos >= link_col_keys_.size())
        {
            return -1;
        }
        s32 hit = link_col_keys_[pos];
        return hit <= col_key_of(layer, x, hi_row) ? hit % plane_ % height_ : -1;
    }
    size_t pos = link_col_key_lower_bound(col_key_of(layer, x, hi_row) + 1);
    if (pos == 0)
    {
        return -1;
    }
    s32 hit = link_col_keys_[pos - 1];
    return hit >= col_key_of(layer, x, lo_row) ? hit % plane_ % height_ : -1;
}

inline s32 zjps_grid::light_forced_turn_row(s32 layer, s32 y, s32 x, s32 step, s32 reach_col) const
{
    s32 nearest_turn = -1;
    for (s32 side = -1; side <= 1; side += 2)
    {
        s32 side_y = y + side;
        if (side_y < 0 || side_y >= height_)
        {
            continue;
        }
        block_line line = light_row_line(layer, side_y);
        if (step > 0)
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

inline s32 zjps_grid::light_forced_turn_col(s32 layer, s32 x, s32 y, s32 step, s32 reach_row) const
{
    s32 nearest_turn = -1;
    for (s32 side = -1; side <= 1; side += 2)
    {
        s32 side_x = x + side;
        if (side_x < 0 || side_x >= width_)
        {
            continue;
        }
        block_line line = light_col_line(layer, side_x);
        if (step > 0)
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

inline s32 zjps_grid::successor_dirs(s32 layer, s32 x, s32 y, s32 entry_dir, s32* out_dirs) const
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
        bool walk_x = cell_walkable(layer, x + dx, y);
        bool walk_y = cell_walkable(layer, x, y + dy);
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
        bool walk_ahead = cell_walkable(layer, x + dx, y);
        if (walk_ahead)
        {
            out_dirs[cnt++] = entry_dir;
        }
        for (s32 side = -1; side <= 1; side += 2)
        {
            if (side_forced_row(layer, x, y, dx, side))
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
    bool walk_ahead = cell_walkable(layer, x, y + dy);
    if (walk_ahead)
    {
        out_dirs[cnt++] = entry_dir;
    }
    for (s32 side = -1; side <= 1; side += 2)
    {
        if (side_forced_col(layer, x, y, dy, side))
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

inline s32 zjps_grid::probe_next_cell_by_plus(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y) const
{
    if (probe_dir_hit_target_by_plus(layer, x, y, dx, dy, target_layer, target_x, target_y))
    {
        return cell_code(target_layer, target_x, target_y);
    }
    return plus_table_[plus_slot(cell_code(layer, x, y), dir_index(dx, dy))].first_turn_cell;
}

inline bool zjps_grid::probe_dir_hit_target_by_plus(s32 layer, s32 x, s32 y, s32 dx, s32 dy, s32 target_layer, s32 target_x, s32 target_y) const
{
    if (!dir_is_axis(dx, dy) || layer != target_layer)
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
    size_t slot = plus_slot(cell_code(layer, x, y), dir_index(dx, dy));
    s32 jump_cell = plus_table_[slot].first_turn_cell;
    s32 stop = plus_table_[slot].first_block_cell;
    s32 target_axis = (dx != 0) ? target_x : target_y;
    s32 step = (dx != 0) ? dx : dy;
    if (jump_cell >= 0)
    {
        s32 jump_x = 0;
        s32 jump_y = 0;
        cell_xy_in_layer(jump_cell, layer, jump_x, jump_y);
        s32 jump_coord = (dx != 0) ? jump_x : jump_y;
        return (target_axis - jump_coord) * step <= 0;
    }
    if (stop >= 0)
    {
        s32 stop_x = 0;
        s32 stop_y = 0;
        cell_xy_in_layer(stop, layer, stop_x, stop_y);
        s32 stop_axis = (dx != 0) ? stop_x : stop_y;
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
