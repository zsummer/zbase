/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once
#ifndef  ZGRAPH_H
#define ZGRAPH_H

#include <math.h>
#include <cmath>
#include <cstring>
#include <type_traits>
#include "zpoint.h"
#include <vector>

//default use format compatible short type .
#if !defined(ZBASE_USE_OUTSIDE_TYPE) && !defined(ZBASE_USE_AHEAD_TYPE) && !defined(ZBASE_USE_DEFAULT_TYPE)
#define ZBASE_USE_DEFAULT_TYPE
#endif

//win & unix format incompatible
#ifdef ZBASE_USE_AHEAD_TYPE
using s8 = int8_t;
using u8 = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using s64 = int64_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;
#endif

#ifdef ZBASE_USE_DEFAULT_TYPE
using s8 = char;
using u8 = unsigned char;
using s16 = short int;
using u16 = unsigned short int;
using s32 = int;
using u32 = unsigned int;
using s64 = long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;
#endif


#if __GNUG__
#define ZBASE_ALIAS __attribute__((__may_alias__))
#else
#define ZBASE_ALIAS
#endif

/*
zgraph面向小规模图寻路

zpoint 提供向量计算 坐标封装

这个graph有非常强的动态调整能力

空间索引不再拆成独立组件 直接内置实现 因为它对 item_id 必须是稠密数组下标这个要求
本质上就是复用 zgraph 自己的 link 池形状 拆出去只是形式上的解耦 实际是结构硬拆
真正想要跟 zhash_map 一样通用的话 得换成 zhash_map<cell_key, zhash_set<link_id>> 那种实现
代价是多一层哈希和一份反查索引 现在的场景不需要 见类内 空间索引 分区注释

*/


class zgraph
{
public:
    struct link
    {
        u32 left_vex_id;
        u32 right_vex_id;
        u32 flag;
    };

    struct ref
    {
        u32 vertex_id;
        u32 link_id;
        u32 flag; //插值点或者真实点
    };

public:
    static constexpr u32 kMaxLinks = 4096;
    static constexpr u32 kMaxLinkCoverage = kMaxLinks * 8;
    static constexpr u32 kGridCellBuckets = 1024;
    static constexpr f32 kGridCellSize = 500.0f;
    static_assert((kGridCellBuckets & (kGridCellBuckets - 1)) == 0, "kGridCellBuckets must be power of 2");

public:
    zgraph()
    {
        vertex_using_cnt_ = 0;
        link_using_cnt_ = 0;
        grid_reset();
    }

public:
    std::vector<zpoint>  vertexs_;
    std::vector<u32>  vertex_frees_;
    u32 vertex_using_cnt_;

    std::vector<link> links_;
    std::vector<u32>  link_frees_;
    u32 link_using_cnt_;


public:

    //候选 link 先由内置空间索引按方形范围筛出来 再逐条做点到线段投影 取真正最近的一条
    //out_ref.flag 暂时借用来存归一化 t 的 u16 定点 0 到 65535 独立的 t 字段是提案里待改项 这里先不动 struct 布局
    //kMaxCandidates 是查询候选缓冲区的容量 命中数超过它时 hit 仍会数满实际命中总数 调用方可据此判断是否发生截断
    s32 find_nearst_ref(zpoint center, f32 max_radius, ref& out_ref)
    {
        static constexpr u32 kMaxCandidates = 64;
        u32 candidates[kMaxCandidates];
        u32 hit = grid_query(center, max_radius, candidates, kMaxCandidates);
        u32 loop_count = hit < kMaxCandidates ? hit : kMaxCandidates;

        u32 best_link_id = link_using_cnt_ + link_frees_.size(); //links_.size() 的哨兵值 代表未命中
        f32 best_dist_sq = max_radius * max_radius;
        u16 best_t_fixed = 0;

        for (u32 i = 0; i < loop_count; i++)
        {
            u32 link_id = candidates[i];
            const link& l = links_[link_id];
            zpoint a = vertexs_[l.left_vex_id];
            zpoint b = vertexs_[l.right_vex_id];
            zpoint ab = b - a;
            f32 len_sq = ab.square_length_2d();
            f32 t = len_sq > zpoint::kFloatPrecision ? (center - a).dot_2d(ab) / len_sq : 0.0f;
            t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
            zpoint proj = a + ab * t;
            f32 dist_sq = (center - proj).square_length_2d();
            if (dist_sq <= best_dist_sq)
            {
                best_dist_sq = dist_sq;
                best_link_id = link_id;
                best_t_fixed = (u16)(t * 65535.0f + 0.5f);
            }
        }

        if (best_link_id >= links_.size())
        {
            return -1;
        }
        out_ref.vertex_id = links_[best_link_id].left_vex_id;
        out_ref.link_id = best_link_id;
        out_ref.flag = best_t_fixed;
        return 0;
    }

    //A* 还没上 属于第一阶后续内容 这里先占位
    s32 path_find(ref start, ref end)
    {
        return -1;
    }

    s32 path_find(zpoint start, zpoint end)
    {
        ref start_ref;
        ref end_ref;
        if (find_nearst_ref(start, kGridCellSize * 4, start_ref) != 0)
        {
            return -1;
        }
        if (find_nearst_ref(end, kGridCellSize * 4, end_ref) != 0)
        {
            return -1;
        }
        return path_find(start_ref, end_ref);
    }

    s32 remove_link(u32 link_id)
    {
        if (link_id >= links_.size())
        {
            return -1;
        }
        grid_remove(link_id);
        link_frees_.push_back(link_id);
        link_using_cnt_--;
        return 0;
    }

    s32 remove_vertex(u32 vertex_id)
    {
        if (vertex_id >= vertexs_.size())
        {
            return -1;
        }
        vertex_frees_.push_back(vertex_id);
        vertex_using_cnt_--;
        return 0;
    }

    //ref 是否要支持独立的编辑期落点 owner 反查等语义还没定 先占位
    s32 remove_ref(ref& ref)
    {
        return -1;
    }

    s32 add_vertex(zpoint point, u32 flag)
    {
        u32 id;
        if (!vertex_frees_.empty())
        {
            id = vertex_frees_.back();
            vertex_frees_.pop_back();
            vertexs_[id] = point;
        }
        else
        {
            id = (u32)vertexs_.size();
            vertexs_.push_back(point);
        }
        vertex_using_cnt_++;
        return (s32)id;
    }

    //空间索引容量是编译期常量 link id 超出 kMaxLinks 时直接失败 不做静默截断
    s32 add_link(u32 left_vex_id, u32 right_vex_id, u32 flag)
    {
        if (left_vex_id >= vertexs_.size() || right_vex_id >= vertexs_.size())
        {
            return -1;
        }

        u32 id;
        if (!link_frees_.empty())
        {
            id = link_frees_.back();
            link_frees_.pop_back();
        }
        else
        {
            id = (u32)links_.size();
            links_.push_back({});
        }

        if (id >= kMaxLinks)
        {
            link_frees_.push_back(id);
            return -2;
        }

        s32 grid_ret = grid_insert(id, vertexs_[left_vex_id], vertexs_[right_vex_id]);
        if (grid_ret != 0)
        {
            link_frees_.push_back(id);
            return grid_ret;
        }

        links_[id].left_vex_id = left_vex_id;
        links_[id].right_vex_id = right_vex_id;
        links_[id].flag = flag;
        link_using_cnt_++;
        return (s32)id;
    }

    //同上 语义未定 先占位
    s32 add_ref(u32 vertex_id, u32 link_id, u32 flag)
    {
        return -1;
    }


    // ============================================================================
    // 空间索引 内部实现 只服务本类自己的 link 池 不是通用组件 不要指望脱离 zgraph 复用
    // item_id 就是 link 池的 u32 下标 上限就是 kMaxLinks 跟外部完全没有独立身份概念
    // 光栅化 加 节点池 加 位掩码桶 的做法参见提案 zgraph 动态导航图 的空间索引决策一节
    // ============================================================================
private:
    struct grid_node_type
    {
        u32 item_id;
        u64 cell_key;
        u32 next_in_cell;
        u32 next_in_item;
    };

    static const u32 kGridFreePoolSize = 0;

    grid_node_type grid_node_pool_[kMaxLinkCoverage + 1];
    u32 grid_buckets_[kGridCellBuckets];
    u32 grid_item_head_[kMaxLinks];
    mutable u32 grid_item_stamp_[kMaxLinks];
    mutable u32 grid_cur_stamp_;
    u32 grid_exploit_offset_;
    u32 grid_count_;

    void grid_reset()
    {
        grid_exploit_offset_ = 0;
        grid_count_ = 0;
        grid_cur_stamp_ = 0;
        grid_node_pool_[kGridFreePoolSize].next_in_item = kGridFreePoolSize;
        memset(grid_buckets_, 0, sizeof(grid_buckets_));
        memset(grid_item_head_, 0, sizeof(grid_item_head_));
        memset(grid_item_stamp_, 0, sizeof(grid_item_stamp_));
    }

    static s32 grid_quantize(f32 v)
    {
        return (s32)std::floor(v / kGridCellSize);
    }

    static u64 grid_make_cell_key(s32 cx, s32 cy)
    {
        return (((u64)(u32)cx) << 32) | (u32)cy;
    }

    static u32 grid_bucket_of(u64 key)
    {
        s32 cx = (s32)(key >> 32);
        s32 cy = (s32)(key & 0xFFFFFFFFu);
        //经典空间哈希的乘数 来自 Optimized Spatial Hashing (Teschner et al.) 目的只是把量化坐标打散 不追求密码学强度
        u32 h = ((u32)cx * 73856093u) ^ ((u32)cy * 19349663u);
        return h & (kGridCellBuckets - 1);
    }

    //Bresenham 风格的整数格子遍历 每 next() 前进一格 首次调用返回起点 走完返回 false
    //不用回调 因为 grid_insert 要在遍历途中随时可能因为节点池耗尽而中断并回滚 用 next() 逐格拉取更直接
    struct grid_cell_walker
    {
        void init(zpoint a, zpoint b)
        {
            s32 x0 = grid_quantize(a.x);
            s32 y0 = grid_quantize(a.y);
            s32 x1 = grid_quantize(b.x);
            s32 y1 = grid_quantize(b.y);

            s32 dx = x1 - x0;
            s32 dy = y1 - y0;
            nx_ = dx < 0 ? -dx : dx;
            ny_ = dy < 0 ? -dy : dy;
            step_x_ = dx > 0 ? 1 : -1;
            step_y_ = dy > 0 ? 1 : -1;

            x_ = x0;
            y_ = y0;
            ix_ = 0;
            iy_ = 0;
            step_done_ = 0;
            step_total_ = nx_ + ny_;
        }

        bool next(s32& cx, s32& cy)
        {
            if (step_done_ > step_total_)
            {
                return false;
            }
            if (step_done_ > 0)
            {
                if ((1 + 2 * ix_) * ny_ < (1 + 2 * iy_) * nx_)
                {
                    x_ += step_x_;
                    ++ix_;
                }
                else
                {
                    y_ += step_y_;
                    ++iy_;
                }
            }
            cx = x_;
            cy = y_;
            ++step_done_;
            return true;
        }

    private:
        s32 x_, y_;
        s32 nx_, ny_;
        s32 step_x_, step_y_;
        s32 ix_, iy_;
        u32 step_done_;
        u32 step_total_;
    };

    u32 grid_pop_free()
    {
        if (grid_node_pool_[kGridFreePoolSize].next_in_item != kGridFreePoolSize)
        {
            u32 ret = grid_node_pool_[kGridFreePoolSize].next_in_item;
            grid_node_pool_[kGridFreePoolSize].next_in_item = grid_node_pool_[ret].next_in_item;
            grid_count_++;
            return ret;
        }
        if (grid_exploit_offset_ < kMaxLinkCoverage)
        {
            grid_count_++;
            return ++grid_exploit_offset_;
        }
        return kGridFreePoolSize;
    }

    void grid_push_free(u32 node_id)
    {
        grid_node_pool_[node_id].next_in_item = grid_node_pool_[kGridFreePoolSize].next_in_item;
        grid_node_pool_[kGridFreePoolSize].next_in_item = node_id;
        grid_count_--;
    }

    void grid_unlink_from_bucket(u32 node_id)
    {
        grid_node_type& node = grid_node_pool_[node_id];
        u32 bucket = grid_bucket_of(node.cell_key);
        u32 cur = grid_buckets_[bucket];
        if (cur == node_id)
        {
            grid_buckets_[bucket] = node.next_in_cell;
            return;
        }
        while (cur != kGridFreePoolSize)
        {
            u32 next = grid_node_pool_[cur].next_in_cell;
            if (next == node_id)
            {
                grid_node_pool_[cur].next_in_cell = node.next_in_cell;
                return;
            }
            cur = next;
        }
    }

    //按 kGridCellSize 把线段 a->b 光栅化进格子 每条覆盖到的格子记一份归属节点
    //item_id 必须小于 kMaxLinks 且当前未被 insert 过 重复 insert 前必须先 remove
    //节点池耗尽时整条 item 回滚 不留半条 item 的归属
    s32 grid_insert(u32 item_id, zpoint a, zpoint b)
    {
        if (item_id >= kMaxLinks || grid_item_head_[item_id] != kGridFreePoolSize)
        {
            return -1;
        }

        grid_cell_walker walker;
        walker.init(a, b);

        s32 cx = 0;
        s32 cy = 0;
        while (walker.next(cx, cy))
        {
            u64 key = grid_make_cell_key(cx, cy);
            u32 node_id = grid_pop_free();
            if (node_id == kGridFreePoolSize)
            {
                grid_remove(item_id);
                return -2;
            }
            grid_node_type& node = grid_node_pool_[node_id];
            node.item_id = item_id;
            node.cell_key = key;

            u32 bucket = grid_bucket_of(key);
            node.next_in_cell = grid_buckets_[bucket];
            grid_buckets_[bucket] = node_id;

            node.next_in_item = grid_item_head_[item_id];
            grid_item_head_[item_id] = node_id;
        }
        return 0;
    }

    s32 grid_remove(u32 item_id)
    {
        if (item_id >= kMaxLinks)
        {
            return -1;
        }
        u32 node_id = grid_item_head_[item_id];
        while (node_id != kGridFreePoolSize)
        {
            u32 next_item_node = grid_node_pool_[node_id].next_in_item;
            grid_unlink_from_bucket(node_id);
            grid_push_free(node_id);
            node_id = next_item_node;
        }
        grid_item_head_[item_id] = kGridFreePoolSize;
        return 0;
    }

    //以 center 为中心 边长 2*radius 的方形范围做候选查询 不是精确圆 精确裁剪由调用方对拿到的 item 再做一次几何判定
    //同一个 item 覆盖多个候选格子时只写一次 用时间戳标记法去重 不清数组
    //out_items 由调用方提供 装不下时仍然继续数 返回值可能大于 max_out 调用方据此判断是否发生截断 不做静默丢弃
    u32 grid_query(zpoint center, f32 radius, u32* out_items, u32 max_out) const
    {
        s32 cx0 = grid_quantize(center.x - radius);
        s32 cx1 = grid_quantize(center.x + radius);
        s32 cy0 = grid_quantize(center.y - radius);
        s32 cy1 = grid_quantize(center.y + radius);

        u32 stamp = ++grid_cur_stamp_;
        u32 hit = 0;
        for (s32 cx = cx0; cx <= cx1; cx++)
        {
            for (s32 cy = cy0; cy <= cy1; cy++)
            {
                u64 key = grid_make_cell_key(cx, cy);
                u32 bucket = grid_bucket_of(key);
                u32 node_id = grid_buckets_[bucket];
                while (node_id != kGridFreePoolSize)
                {
                    const grid_node_type& node = grid_node_pool_[node_id];
                    if (node.cell_key == key && grid_item_stamp_[node.item_id] != stamp)
                    {
                        grid_item_stamp_[node.item_id] = stamp;
                        if (hit < max_out)
                        {
                            out_items[hit] = node.item_id;
                        }
                        hit++;
                    }
                    node_id = node.next_in_cell;
                }
            }
        }
        return hit;
    }
    // ============================================================================
    // 空间索引实现结束
    // ============================================================================
};




#endif
