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
#include <algorithm>
#include "zpoint.h"
#include <vector>
#include "zlist_ext.h"
#include "zarray.h"
#include "zlist.h"
#include "zhash_map.h"

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
zgraph面向小规模动态图寻路

link和node独立入图 用作视野管理时 可不入link   用作寻路时候 可不入node  

*/


struct DefaultGraphConfig
{
    static constexpr s32 kMaxLinkCnt = 2000;
    static constexpr s32 kMaxNodeCnt = 1000;
    static constexpr s32 kMaxGridCnt = 30 * 30;
    static constexpr s32 kMaxOpenCnt = 200;

    static constexpr s32 kGridP90LinkCnt = 5;
    static constexpr s32 kGridP90NodeCnt = 10;

    static constexpr s32 kGridSize = 1000; //cm
    static constexpr f32 kMergeDist = 0.001f; //cm

    static constexpr s32 kCostScaleN = 10000;
    static constexpr s32 kDefaultLinkCost = 0;
};

template<class Node, class Link, typename Config = DefaultGraphConfig>
class zgraph
{
public:
    static constexpr s32 kMaxLinkCnt = Config::kMaxLinkCnt;
    static constexpr s32 kMaxNodeCnt = Config::kMaxNodeCnt;
    static constexpr s32 kMaxGridCnt = Config::kMaxGridCnt;
    static constexpr s32 kMaxOpenCnt = Config::kMaxOpenCnt;

    static constexpr s32 kGridP90LinkCnt = Config::kGridP90LinkCnt;
    static constexpr s32 kGridP90NodeCnt = Config::kGridP90NodeCnt;

    static constexpr s32 kGridSize = Config::kGridSize;
    static constexpr f32 kMergeDist = Config::kMergeDist;

    static constexpr s32 kCostScaleN = Config::kCostScaleN;
    static constexpr s32 kDefaultLinkCost = Config::kDefaultLinkCost;

    static constexpr s32 kSlotS = 0;
    static constexpr s32 kSlotT = 1;
    static constexpr s32 kSlotMax = 2;


public:
    struct graph_node
    {
        Node node;
        zpoint pos;
        s32 x;
        s32 y;
        s32 refs;
        s32 cls;
        s32 first_link[kSlotMax]; //invalid: -1 
    };
    
    struct graph_link
    {
        Link link;
        s32 node[kSlotMax];
        s32 refs;
        s32 color;
        s32 cost;
        s32 next[kSlotMax];
    };

    struct graph_find_option
    {
        s32 node_cls = 0;
        s32 link_color = 0;
    };

    struct graph_path_step
    {
        s32 link_id;
        s32 slot;
        zpoint pos;
    };


    struct terrain
    {
        zarray<s32, kGridP90LinkCnt> links;
        std::vector<s32> ext_links; //expire empty
        zarray<s32, kGridP90NodeCnt> nodes;
        std::vector<s32> ext_nodes; //expire empty
    };

private:
    struct default_node_filter
    {
        bool operator()(const Node&) const { return false; }
    };

    struct path_node_state
    {
        f32 cost;
        f32 estimate;
        s32 came_from_link;
        u32 stamp;
    };

private:
    using node_list = zlist<graph_node, kMaxNodeCnt>;
    node_list nodes_;

    using link_list = zlist<graph_link, kMaxLinkCnt>;
    link_list links_;

    zhash_map<u64, terrain, kMaxGridCnt> terrain_map_;

    zarray<path_node_state, kMaxNodeCnt> path_node_states_;
    zarray<s32, kMaxOpenCnt> path_open_;
    u32 path_current_stamp_ = 0;
    u32 path_open_peak_ = 0;
    u32 path_visit_count_ = 0;
    u32 path_visit_peak_ = 0;

private:
    static bool match_node(const graph_node* node, const graph_find_option& option)
    {
        return option.node_cls == 0 || node->cls == option.node_cls;
    }
    static bool match_link(const graph_link* link, const graph_find_option& option)
    {
        return option.link_color == 0 || link->color == option.link_color;
    }

    s32 unlink_from_chain(s32 link_id, s32 slot)
    {
        graph_link* link = ref_link(link_id);
        if (link == nullptr)
        {
            return -1;
        }
        graph_node* node = ref_node(link->node[slot]);
        if (node == nullptr)
        {
            return -2;
        }
        s32 prev_id = -1;
        s32 curr_id = node->first_link[slot];
        while (curr_id != -1)
        {
            if (curr_id == link_id)
            {
                break;
            }
            prev_id = curr_id;
            curr_id = ref_link(curr_id)->next[slot];
        }
        if (curr_id == -1)
        {
            return -5;
        }
        if (prev_id == -1)
        {
            node->first_link[slot] = link->next[slot];
        }
        else
        {
            ref_link(prev_id)->next[slot] = link->next[slot];
        }
        return 0;
    }

    static f32 dist_2d(const zpoint& a, const zpoint& b)
    {
        f32 dx = a.x - b.x;
        f32 dy = a.y - b.y;
        return sqrtf(dx * dx + dy * dy);
    }

    s32 path_heap_push(s32 node_id)
    {
        if (path_open_.full())
        {
            return -1;
        }
        path_open_.push_back(node_id);
        if (path_open_.size() > path_open_peak_)
        {
            path_open_peak_ = path_open_.size();
        }
        s32 hole = (s32)path_open_.size() - 1;
        while (hole > 0)
        {
            s32 parent = (hole - 1) / 2;
            if (path_node_states_[path_open_[parent]].estimate <= path_node_states_[path_open_[hole]].estimate)
            {
                break;
            }
            std::swap(path_open_[parent], path_open_[hole]);
            hole = parent;
        }
        return 0;
    }

    s32 path_heap_pop(s32& out_node_id)
    {
        if (path_open_.empty())
        {
            return -1;
        }
        out_node_id = path_open_[0];
        path_open_[0] = path_open_.back();
        path_open_.pop_back();
        size_t cnt = path_open_.size();
        size_t hole = 0;
        while (true)
        {
            size_t left = hole * 2 + 1;
            size_t right = left + 1;
            size_t best = hole;
            if (left < cnt && path_node_states_[path_open_[left]].estimate < path_node_states_[path_open_[best]].estimate)
            {
                best = left;
            }
            if (right < cnt && path_node_states_[path_open_[right]].estimate < path_node_states_[path_open_[best]].estimate)
            {
                best = right;
            }
            if (best == hole)
            {
                break;
            }
            std::swap(path_open_[hole], path_open_[best]);
            hole = best;
        }
        return 0;
    }

    s32 path_visit_touch()
    {
        path_visit_count_++;
        if (path_visit_count_ > path_visit_peak_)
        {
            path_visit_peak_ = path_visit_count_;
        }
        return 0;
    }

    s32 path_search(s32 source_node_id, s32 target_node_id, const graph_find_option& option,
                    std::vector<graph_path_step>& out_steps)
    {
        out_steps.clear();
        if (path_current_stamp_ == 0 || path_current_stamp_ == 0xFFFFFFFFu)
        {
            for (s32 i = 0; i < kMaxNodeCnt; i++)
            {
                path_node_states_[i].stamp = 0;
            }
            path_current_stamp_ = 1;
        }
        else
        {
            path_current_stamp_++;
        }
        u32 stamp = path_current_stamp_;
        path_open_.clear();
        path_visit_count_ = 0;
        const graph_node* target_node = ref_node(target_node_id);
        if (target_node == nullptr)
        {
            return -3;
        }
        graph_node* source_node = ref_node(source_node_id);
        if (source_node == nullptr)
        {
            return -3;
        }
        path_node_states_[source_node_id].cost = 0.0f;
        path_node_states_[source_node_id].estimate = dist_2d(source_node->pos, target_node->pos);
        path_node_states_[source_node_id].came_from_link = -1;
        path_node_states_[source_node_id].stamp = stamp;
        path_visit_touch();
        if (path_heap_push(source_node_id) != 0)
        {
            return -4;
        }
        while (!path_open_.empty())
        {
            s32 current_node_id = -1;
            path_heap_pop(current_node_id);
            if (current_node_id == target_node_id)
            {
                s32 cursor = current_node_id;
                while (path_node_states_[cursor].came_from_link != -1)
                {
                    graph_link* link = ref_link(path_node_states_[cursor].came_from_link);
                    graph_path_step step;
                    step.link_id = path_node_states_[cursor].came_from_link;
                    step.slot = link->node[kSlotS] == cursor ? kSlotS : kSlotT;
                    step.pos = ref_node(cursor)->pos;
                    out_steps.push_back(step);
                    cursor = link->node[1 - step.slot];
                }
                std::reverse(out_steps.begin(), out_steps.end());
                return 0;
            }
            graph_node* current_node = ref_node(current_node_id);
            for (s32 slot = 0; slot < kSlotMax; slot++)
            {
                for (s32 link_id = current_node->first_link[slot]; link_id != -1; link_id = ref_link(link_id)->next[slot])
                {
                    graph_link* link = ref_link(link_id);
                    if (!match_link(link, option))
                    {
                        continue;
                    }
                    s32 next_node_id = link->node[1 - slot];
                    if (next_node_id == current_node_id)
                    {
                        continue;
                    }
                    graph_node* next_node = ref_node(next_node_id);
                    if (next_node == nullptr || !match_node(next_node, option))
                    {
                        continue;
                    }
                    graph_node* link_source = ref_node(link->node[kSlotS]);
                    graph_node* link_target = ref_node(link->node[kSlotT]);
                    f32 link_length = dist_2d(link_source->pos, link_target->pos);
                    f32 link_weight = link_length * (f32)(kCostScaleN + link->cost) / (f32)kCostScaleN;
                    f32 next_cost = path_node_states_[current_node_id].cost + link_weight;
                    if (path_node_states_[next_node_id].stamp == stamp && next_cost >= path_node_states_[next_node_id].cost)
                    {
                        continue;
                    }
                    path_node_states_[next_node_id].cost = next_cost;
                    path_node_states_[next_node_id].estimate = next_cost + dist_2d(next_node->pos, target_node->pos);
                    path_node_states_[next_node_id].came_from_link = link_id;
                    path_node_states_[next_node_id].stamp = stamp;
                    path_visit_touch();
                    if (path_heap_push(next_node_id) != 0)
                    {
                        return -4;
                    }
                }
            }
        }
        return -3;
    }

public:
    static std::pair<s32, s32> to_int_pos(const zpoint& pos)
    {
        return std::make_pair((s32)std::floor(pos.x / kGridSize), (s32)std::floor(pos.y / kGridSize));
    }
    static zpoint to_f32_pos(s32 x, s32 y, f32 z)
    {
        return zpoint((f32)x*kGridSize + kGridSize/2, (f32)y*kGridSize + kGridSize/2,  z);
    }
    static  u64 to_terrain_key(s64 x, s64 y){return ((u64)x << 32) | (u64)y;}


    s32 new_node(const zpoint& pos, Node v, s32 cls = 0)
    {
        auto xy = to_int_pos(pos);
        if (nodes_.full())
        {
            return -1;
        }
        graph_node node;
        node.node = v;
        node.pos = pos;
        node.x = xy.first;
        node.y = xy.second;
        node.refs = 0;
        node.cls = cls;
        node.first_link[0] = -1;
        node.first_link[1] = -1;
        nodes_.push_back(node);

        s32 inner_node_id = nodes_.data()[node_list::END_ID].front;

        return inner_node_id;
    }
    bool is_valid_node(s32 node_id) const 
    {
        return nodes_.is_valid_node(nodes_.data() + node_id);
    }

    graph_node* ref_node(s32 node_id)
    {
        if (!is_valid_node(node_id))
        {
            return nullptr;
        }
        return nodes_.node_cast(nodes_.data()[node_id]);
    }

    s32 free_node(s32 node_id)
    {
        graph_node* node = ref_node(node_id);
        if (node == nullptr)
        {
            return -1;
        }

        if (node->refs > 0)
        {
            return -2;
        }

        nodes_.erase(typename node_list::iterator(nodes_.data(), node_id));
        return 0;
    }

    s32 push_node(s32 node_id)
    {
        graph_node* node = ref_node(node_id);
        if (node == nullptr)
        {
            return -1;
        }
        u64 key = to_terrain_key(node->x, node->y);
        if (terrain_map_.full())
        {
            auto iter = terrain_map_.find(key);
            if (iter == terrain_map_.end())
            {
                return -2;
            }
        }
        terrain& t = terrain_map_[key];
        if (!t.nodes.full())
        {
            t.nodes.push_back(node_id);
        }
        else
        {
            t.ext_nodes.push_back(node_id);
        }
        node->refs++;
        return 0;
    }

    s32 pop_node(s32 node_id)
    {
        graph_node* node = ref_node(node_id);
        if (node == nullptr)
        {
            return -1;
        }
        u64 key = to_terrain_key(node->x, node->y);
        auto iter = terrain_map_.find(key);
        if (iter == terrain_map_.end())
        {
            return -2;
        }
        terrain& t = iter->second;
        auto nit = std::find(t.nodes.begin(), t.nodes.end(), node_id);
        if (nit != t.nodes.end())
        {
            t.nodes.erase(nit);
            node->refs--;
        }
        else
        {
            auto eit = std::find(t.ext_nodes.begin(), t.ext_nodes.end(), node_id);
            if (eit == t.ext_nodes.end())
            {
                return -2;
            }
            t.ext_nodes.erase(eit);
            node->refs--;
        }
        if (iter->second.links.empty() && iter->second.ext_links.empty()
            && iter->second.nodes.empty() && iter->second.ext_nodes.empty())
        {
            terrain_map_.erase(iter);
        }
        return 0;
    }





    s32 new_link(s32 source, s32 target, const Link& v, s32 color = 0, s32 cost = kDefaultLinkCost)
    {
        if (links_.full())
        {
            return -1;
        }
        if (!is_valid_node(source) || !is_valid_node(target))
        {
            return -2;
        }

        ref_node(source)->refs++;
        ref_node(target)->refs++;

        graph_link link;
        link.link = v;
        link.node[kSlotS] = source;
        link.node[kSlotT] = target;
        link.refs = 0;
        link.color = color;
        link.cost = cost;
        link.next[kSlotS] = ref_node(source)->first_link[kSlotS];
        link.next[kSlotT] = ref_node(target)->first_link[kSlotT];

        links_.push_back(link);

        s32 inner_link_id = links_.data()[link_list::END_ID].front;

        ref_node(source)->first_link[kSlotS] = inner_link_id;
        ref_node(target)->first_link[kSlotT] = inner_link_id;

        return inner_link_id;
    }

    bool is_valid_link(s32 link_id) const
    {
        return links_.is_valid_node(links_.data() + link_id);
    }

    graph_link* ref_link(s32 link_id)
    {
        if (!is_valid_link(link_id))
        {
            return nullptr;
        }
        return links_.node_cast(links_.data()[link_id]);
    }

    s32 free_link(s32 link_id)
    {
        graph_link* link = ref_link(link_id);
        if (link == nullptr)
        {
            return -1;
        }

        if (link->refs > 0)
        {
            return -2;
        }

        graph_node* source = ref_node(link->node[kSlotS]);
        if (source == nullptr)
        {
            return -3;
        }
        graph_node* target = ref_node(link->node[kSlotT]);
        if (target == nullptr)
        {
            return -4;
        }

        s32 ret = unlink_from_chain(link_id, kSlotS);
        if (ret != 0)
        {
            return ret;
        }
        ret = unlink_from_chain(link_id, kSlotT);
        if (ret != 0)
        {
            return ret;
        }

        source->refs--;
        target->refs--;

        links_.erase(typename link_list::iterator(links_.data(), link_id));
        return 0;
    }

    s32 push_link(s32 link_id, s32& affects)
    {
        affects = 0;
        graph_link* link = ref_link(link_id);
        if (link == nullptr)
        {
            return -2;
        }

        graph_node* source_node = ref_node(link->node[kSlotS]);
        if (source_node == nullptr)
        {
            return -3;
        }
        graph_node* target_node = ref_node(link->node[kSlotT]);
        if (target_node == nullptr)
        {
            return -4;
        }

        s64 begin_x = source_node->x;
        s64 begin_y = source_node->y;
        s64 end_x = target_node->x;
        s64 end_y = target_node->y;

        s64 dx = (s64)end_x - begin_x;
        s64 dy = (s64)end_y - begin_y;



        s32 stepx = dx >= 0 ? 1 : -1;
        s32 stepy = dy >= 0 ? 1 : -1;


        s64 adx = std::abs(dx);
        s64 ady = std::abs(dy);


        s64 err = adx - ady;

        do
        {

            u64 key = to_terrain_key(begin_x, begin_y);
            if (terrain_map_.full()/* && terrain_map_.find(key) == terrain_map_.end() */ ) // don't fix
            {
                return -5; // least need one
            }
            terrain& t = terrain_map_[key];
            affects++;

            if (!t.links.full())
            {
                t.links.push_back(link_id);
            }
            else
            {
                t.ext_links.push_back(link_id);
            }
            link->refs++;


            if (begin_x == end_x && begin_y == end_y)
            {
                break; //finish
            }

            if (err > 0) 
            {
                if (begin_x == end_x)
                {
                    return -6; //nerver reachable
                }
                begin_x += stepx;
                err -= ady;
            }
            else if (err < 0)
            {
                if (begin_y == end_y)
                {
                    return -7; //nerver reachable
                }
                begin_y += stepy;
                err += adx;
            }
            else  
            {
                begin_x += stepx;
                begin_y += stepy;
                err += adx - ady;
            }
        } while (true);

        return 0;
    }

    s32 pop_link(s32 link_id, s32& affects)
    {
        affects = 0;
        graph_link* link = ref_link(link_id);
        if (link == nullptr)
        {
            return -2;
        }

        graph_node* source_node = ref_node(link->node[kSlotS]);
        if (source_node == nullptr)
        {
            return -3;
        }
        graph_node* target_node = ref_node(link->node[kSlotT]);
        if (target_node == nullptr)
        {
            return -4;
        }

        s64 begin_x = source_node->x;
        s64 begin_y = source_node->y;
        s64 end_x = target_node->x;
        s64 end_y = target_node->y;

        s64 dx = (s64)end_x - begin_x;
        s64 dy = (s64)end_y - begin_y;



        s32 stepx = dx >= 0 ? 1 : -1;
        s32 stepy = dy >= 0 ? 1 : -1;


        s64 adx = std::abs(dx);
        s64 ady = std::abs(dy);


        s64 err = adx - ady;

        do
        {

            u64 key = to_terrain_key(begin_x, begin_y);
            auto iter = terrain_map_.find(key);
            if (iter != terrain_map_.end())
            {
                terrain& t = iter->second;


                auto lit = std::find(t.links.begin(), t.links.end(), link_id);
                if (lit != t.links.end())
                {
                    t.links.erase(lit);
                    link->refs--;
                    affects++;
                }
                else
                {
                    auto eit = std::find(t.ext_links.begin(), t.ext_links.end(), link_id);
                    if (eit != t.ext_links.end())
                    {
                        t.ext_links.erase(eit);
                        link->refs--;
                        affects++;
                    }
                }
                if (iter->second.links.empty() && iter->second.ext_links.empty()
                    && iter->second.nodes.empty() && iter->second.ext_nodes.empty())
                {
                    terrain_map_.erase(iter);
                }
            }

            


            if (begin_x == end_x && begin_y == end_y)
            {
                break; //finish
            }

            if (err > 0) 
            {
                if (begin_x == end_x)
                {
                    return -6; //nerver reachable
                }
                begin_x += stepx;
                err -= ady;
            }
            else if (err < 0)
            {
                if (begin_y == end_y)
                {
                    return -7; //nerver reachable
                }
                begin_y += stepy;
                err += adx;
            }
            else  
            {
                begin_x += stepx;
                begin_y += stepy;
                err += adx - ady;
            }
        } while (true);

        return 0;
    }


    s32 find_nearest_link_sample(const zpoint& pos, s32& out_link_id, bool& out_is_source, s32 exclude_node_id = -1,
                                  const graph_find_option& option = graph_find_option()) const
    {
        auto xy = to_int_pos(pos);
        s32 cx = xy.first;
        s32 cy = xy.second;

        s32 best_link_id = -1;
        bool best_is_source = false;
        f32 best_sq_dist = 0.0f;

        auto try_link = [&](s32 link_id)
        {
            const graph_link* link = const_cast<zgraph*>(this)->ref_link(link_id);
            if (link == nullptr || !match_link(link, option))
            {
                return;
            }
            for (s32 i = 0; i < 2; i++)
            {
                s32 node_id = link->node[i];
                if (node_id == exclude_node_id)
                {
                    continue;
                }
                const graph_node* node = const_cast<zgraph*>(this)->ref_node(node_id);
                if (node == nullptr)
                {
                    continue;
                }
                f32 ddx = node->pos.x - pos.x;
                f32 ddy = node->pos.y - pos.y;
                f32 sq_dist = ddx * ddx + ddy * ddy;
                if (best_link_id == -1 || sq_dist < best_sq_dist)
                {
                    best_link_id = link_id;
                    best_is_source = (i == kSlotS);
                    best_sq_dist = sq_dist;
                }
            }
        };

        for (s32 dx = -1; dx <= 1; dx++)
        {
            for (s32 dy = -1; dy <= 1; dy++)
            {
                u64 key = to_terrain_key(cx + dx, cy + dy);
                auto iter = terrain_map_.find(key);
                if (iter == terrain_map_.end())
                {
                    continue;
                }
                const terrain& t = iter->second;

                for (s32 link_id : t.links)
                {
                    try_link(link_id);
                }
                for (s32 link_id : t.ext_links)
                {
                    try_link(link_id);
                }
            }
        }

        if (best_link_id == -1)
        {
            return -1;
        }
        out_link_id = best_link_id;
        out_is_source = best_is_source;
        return 0;
    }


    s32 find_nearest_link(const zpoint& pos, s32& out_link_id, s32& out_slot, zpoint& out_nearest_pos,
                          f32& out_slot_sq_dist,
                          s32 exclude_link_id = -1, const graph_find_option& option = graph_find_option()) const
    {
        auto xy = to_int_pos(pos);
        s32 cx = xy.first;
        s32 cy = xy.second;

        s32 best_link_id = -1;
        zpoint best_pos;
        s32 best_slot = kSlotS;
        f32 best_sq_dist = 0.0f;

        auto try_link = [&](s32 link_id)
        {
            if (link_id == exclude_link_id)
            {
                return;
            }
            const graph_link* link = const_cast<zgraph*>(this)->ref_link(link_id);
            if (link == nullptr || !match_link(link, option))
            {
                return;
            }
            const graph_node* ends[2] =
            {
                const_cast<zgraph*>(this)->ref_node(link->node[kSlotS]),
                const_cast<zgraph*>(this)->ref_node(link->node[kSlotT])
            };
            if (ends[0] == nullptr || ends[1] == nullptr)
            {
                return;
            }
            const graph_node* source = ends[0];
            const graph_node* target = ends[1];

            f32 ex = target->pos.x - source->pos.x;
            f32 ey = target->pos.y - source->pos.y;
            f32 len_sq = ex * ex + ey * ey;

            f32 t = 0.0f;
            if (len_sq > kMergeDist * kMergeDist)
            {
                t = ((pos.x - source->pos.x) * ex + (pos.y - source->pos.y) * ey) / len_sq;
                t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
            }

            zpoint nearest(source->pos.x + t * ex, source->pos.y + t * ey,
                            source->pos.z + t * (target->pos.z - source->pos.z));

            f32 ddx = nearest.x - pos.x;
            f32 ddy = nearest.y - pos.y;
            f32 sq_dist = ddx * ddx + ddy * ddy;
            if (best_link_id == -1 || sq_dist < best_sq_dist)
            {
                best_link_id = link_id;
                best_pos = nearest;
                best_slot = (t <= 0.5f) ? kSlotS : kSlotT;
                best_sq_dist = sq_dist;
            }
        };

        for (s32 dx = -1; dx <= 1; dx++)
        {
            for (s32 dy = -1; dy <= 1; dy++)
            {
                u64 key = to_terrain_key(cx + dx, cy + dy);
                auto iter = terrain_map_.find(key);
                if (iter == terrain_map_.end())
                {
                    continue;
                }
                const terrain& t = iter->second;
                for (s32 link_id : t.links)
                {
                    try_link(link_id);
                }
                for (s32 link_id : t.ext_links)
                {
                    try_link(link_id);
                }
            }
        }

        if (best_link_id == -1)
        {
            return -1;
        }
        out_link_id = best_link_id;
        out_slot = best_slot;
        out_nearest_pos = best_pos;
        out_slot_sq_dist = best_sq_dist;
        return 0;
    }

    template<size_t MaxOut, class Filter = default_node_filter>
    s32 find_neighbor_nodes(const zpoint& pos, zarray<s32, MaxOut>& out_node_ids, s32 exclude_node_id = -1,
                             Filter filter = Filter(), const graph_find_option& option = graph_find_option()) const
    {
        auto xy = to_int_pos(pos);
        s32 cx = xy.first;
        s32 cy = xy.second;

        auto try_node = [&](s32 node_id)
        {
            if (out_node_ids.full() || node_id == exclude_node_id)
            {
                return;
            }
            const graph_node* node = const_cast<zgraph*>(this)->ref_node(node_id);
            if (node == nullptr || !match_node(node, option) || filter(node->node))
            {
                return;
            }
            out_node_ids.push_back(node_id);
        };

        for (s32 dx = -1; dx <= 1 && !out_node_ids.full(); dx++)
        {
            for (s32 dy = -1; dy <= 1 && !out_node_ids.full(); dy++)
            {
                u64 key = to_terrain_key(cx + dx, cy + dy);
                auto iter = terrain_map_.find(key);
                if (iter == terrain_map_.end())
                {
                    continue;
                }
                const terrain& t = iter->second;
                for (s32 node_id : t.nodes)
                {
                    try_node(node_id);
                }
                for (s32 node_id : t.ext_nodes)
                {
                    try_node(node_id);
                }
            }
        }
        return 0;
    }

    const u32 path_open_peak() const { return path_open_peak_; }
    const u32 path_visit_peak() const { return path_visit_peak_; }
    const s32 node_count() const { return (s32)nodes_.size(); }
    const s32 link_count() const { return (s32)links_.size(); }
    const s32 grid_count() const { return (s32)terrain_map_.size(); }
    void path_peak_reset() { path_open_peak_ = 0; path_visit_peak_ = 0; }

    s32 find_path(s32 source_node_id, s32 target_node_id, std::vector<graph_path_step>& out_steps,
                  const graph_find_option& option = graph_find_option())
    {
        out_steps.clear();
        if (!is_valid_node(source_node_id))
        {
            return -1;
        }
        if (!is_valid_node(target_node_id))
        {
            return -2;
        }
        return path_search(source_node_id, target_node_id, option, out_steps);
    }

    s32 find_path(const zpoint& from, const zpoint& to, std::vector<graph_path_step>& out_steps,
                  const graph_find_option& option = graph_find_option())
    {
        out_steps.clear();
        s32 entry_link_id = -1;
        s32 entry_slot = -1;
        zpoint entry_pos;
        f32 entry_sq_dist = 0.0f;
        if (find_nearest_link(from, entry_link_id, entry_slot, entry_pos, entry_sq_dist) != 0)
        {
            return -1;
        }
        s32 exit_link_id = -1;
        s32 exit_slot = -1;
        zpoint exit_pos;
        f32 exit_sq_dist = 0.0f;
        if (find_nearest_link(to, exit_link_id, exit_slot, exit_pos, exit_sq_dist) != 0)
        {
            return -2;
        }
        graph_link* entry_link = ref_link(entry_link_id);
        graph_link* exit_link = ref_link(exit_link_id);
        if (entry_link_id == exit_link_id)
        {
            graph_path_step step_walk_to_entry;
            step_walk_to_entry.link_id = -1;
            step_walk_to_entry.slot = -1;
            step_walk_to_entry.pos = entry_pos;
            out_steps.push_back(step_walk_to_entry);
            graph_path_step step_ride;
            step_ride.link_id = entry_link_id;
            step_ride.slot = exit_slot;
            step_ride.pos = exit_pos;
            out_steps.push_back(step_ride);
            graph_path_step step_walk_to_target;
            step_walk_to_target.link_id = -1;
            step_walk_to_target.slot = -1;
            step_walk_to_target.pos = to;
            out_steps.push_back(step_walk_to_target);
            return 0;
        }
        s32 entry_node_id = entry_link->node[entry_slot];
        s32 target_node_id = exit_link->node[exit_slot];
        std::vector<graph_path_step> ride_steps;
        s32 ret = path_search(entry_node_id, target_node_id, option, ride_steps);
        if (ret != 0)
        {
            return ret;
        }
        graph_path_step step_walk_to_entry;
        step_walk_to_entry.link_id = -1;
        step_walk_to_entry.slot = -1;
        step_walk_to_entry.pos = entry_pos;
        out_steps.push_back(step_walk_to_entry);
        if (ride_steps.empty() || ride_steps.front().link_id != entry_link_id)
        {
            graph_path_step step_ride_to_entry;
            step_ride_to_entry.link_id = entry_link_id;
            step_ride_to_entry.slot = entry_slot;
            step_ride_to_entry.pos = ref_node(entry_node_id)->pos;
            out_steps.push_back(step_ride_to_entry);
        }
        for (size_t i = 0; i < ride_steps.size(); i++)
        {
            out_steps.push_back(ride_steps[i]);
        }
        if (!ride_steps.empty() && ride_steps.back().link_id == exit_link_id)
        {
            out_steps.back().pos = exit_pos;
        }
        graph_path_step step_walk_to_target;
        step_walk_to_target.link_id = -1;
        step_walk_to_target.slot = -1;
        step_walk_to_target.pos = to;
        out_steps.push_back(step_walk_to_target);
        return 0;
    }



};




#endif
