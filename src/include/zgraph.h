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


template<class Node, class Link>
class zgraph
{
    //may be all constexpr value as template args;
public:
    static constexpr s32 kMaxLinkCnt = 5000;
    static constexpr s32 kMaxNodeCnt = 5000;
    static constexpr s32 kMaxGridCnt = 100 * 100;


    static constexpr s32 kGridP90LinkCnt = 5;

    static constexpr s32 kGridSize = 1000; //cm
    static constexpr f32 kMergeDist = 0.001f; //cm


public:
    struct path_node
    {
        Node node;
        zpoint pos;
        s32 x;
        s32 y;
        s32 refs;
    };
    
    struct path_link
    {
        Link link;
        s32 source;
        s32 target;
        s32 refs;
    };


    struct terrain
    {
        // 空间索引只登记 link(边), 不登记 node(点): node 稠密数组下标本身已是最紧凑的定位方式,
        // 需要"node 的空间位置"时, 走它所属的 link 的两端点(source/target)间接得到即可,
        // 没必要为 node 单独维护一份终归会与 link 端点重复的空间索引.
        zarray<s32, kGridP90LinkCnt> links;
        std::vector<s32> ext_links; //expire empty
    };

private:
    using node_list = zlist<path_node, kMaxNodeCnt>;
    node_list nodes_;

    using link_list = zlist<path_link, kMaxLinkCnt>;
    link_list links_;

    zhash_map<u64, terrain, kMaxGridCnt> terrain_map_;

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


    s32 new_node(const zpoint& pos, Node v)
    {
        auto xy = to_int_pos(pos);
        if (nodes_.full())
        {
            return -1;
        }
        path_node node;
        node.node = v;
        node.pos = pos;
        node.x = xy.first;
        node.y = xy.second;
        node.refs = 0; // must init: path_node is a plain struct, refs is otherwise indeterminate garbage
        nodes_.push_back(node);

        s32 inner_node_id = nodes_.data()[node_list::END_ID].front;

        return inner_node_id;
    }
    bool is_valid_node(s32 node_id) const 
    {
        return nodes_.is_valid_node(nodes_.data() + node_id);
    }

    path_node* ref_node(s32 node_id)
    {
        if (!is_valid_node(node_id))
        {
            return nullptr;
        }
        return nodes_.node_cast(nodes_.data()[node_id]);
    }

    s32 free_node(s32 node_id)
    {
        path_node* node = ref_node(node_id);
        if (node == nullptr)
        {
            return -1;
        }

        if (node->refs > 0)
        {
            return -2;
        }

        nodes_.erase(node_list::iterator(nodes_.data(), node_id));
        return 0;
    }



    s32 new_link(s32 source, s32 target, const Link& v)
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

        path_link link;
        link.link = v;
        link.source = source;
        link.target = target;
        link.refs = 0; // must init: path_link is a plain struct, refs is otherwise indeterminate garbage

        links_.push_back(link);

        s32 inner_link_id = links_.data()[link_list::END_ID].front;

        return inner_link_id;
    }

    bool is_valid_link(s32 link_id) const
    {
        return links_.is_valid_node(links_.data() + link_id);
    }

    path_link* ref_link(s32 link_id)
    {
        if (!is_valid_link(link_id))
        {
            return nullptr;
        }
        return links_.node_cast(links_.data()[link_id]);
    }

    s32 free_link(s32 link_id)
    {
        path_link* link = ref_link(link_id);
        if (link == nullptr)
        {
            return -1;
        }

        if (link->refs > 0)
        {
            return -2;
        }

        path_node* source = ref_node(link->source);
        if (source == nullptr)
        {
            return -3;
        }
        path_node* target = ref_node(link->target);
        if (target == nullptr)
        {
            return -4;
        }


        source->refs--;
        target->refs--;

        links_.erase(link_list::iterator(links_.data(), link_id));
        return 0;
    }

    s32 push_link(s32 link_id, s32& affects)
    {
        affects = 0;
        path_link* link = ref_link(link_id);
        if (link == nullptr)
        {
            return -2;
        }

        path_node* source_node = ref_node(link->source);
        if (source_node == nullptr)
        {
            return -3;
        }
        path_node* target_node = ref_node(link->target);
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
        path_link* link = ref_link(link_id);
        if (link == nullptr)
        {
            return -2;
        }

        path_node* source_node = ref_node(link->source);
        if (source_node == nullptr)
        {
            return -3;
        }
        path_node* target_node = ref_node(link->target);
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
                if (iter->second.links.empty() && iter->second.ext_links.empty())
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

    s32 find_nearest_node(const zpoint& pos, s32& out_link_id, bool& out_is_source, s32 exclude_node_id = -1) const
    {
        auto xy = to_int_pos(pos);
        s32 cx = xy.first;
        s32 cy = xy.second;

        s32 best_link_id = -1;
        bool best_is_source = false;
        f32 best_sq_dist = 0.0f;

        auto try_link = [&](s32 link_id)
        {
            const path_link* link = const_cast<zgraph*>(this)->ref_link(link_id);
            if (link == nullptr)
            {
                return;
            }
            s32 node_ids[2] = {link->source, link->target};
            bool is_source_flags[2] = {true, false};
            for (s32 i = 0; i < 2; i++)
            {
                s32 node_id = node_ids[i];
                if (node_id == exclude_node_id)
                {
                    continue;
                }
                const path_node* node = const_cast<zgraph*>(this)->ref_node(node_id);
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
                    best_is_source = is_source_flags[i];
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


    s32 find_nearest_link(const zpoint& pos, s32& out_link_id, zpoint& out_nearest_pos, bool& out_is_source_side,
                           s32 exclude_link_id = -1) const
    {
        auto xy = to_int_pos(pos);
        s32 cx = xy.first;
        s32 cy = xy.second;

        s32 best_link_id = -1;
        zpoint best_pos;
        bool best_is_source_side = false;
        f32 best_sq_dist = 0.0f;

        auto try_link = [&](s32 link_id)
        {
            if (link_id == exclude_link_id)
            {
                return;
            }
            const path_link* link = const_cast<zgraph*>(this)->ref_link(link_id);
            if (link == nullptr)
            {
                return;
            }
            const path_node* source = const_cast<zgraph*>(this)->ref_node(link->source);
            const path_node* target = const_cast<zgraph*>(this)->ref_node(link->target);
            if (source == nullptr || target == nullptr)
            {
                return;
            }

            f32 ex = target->pos.x - source->pos.x;
            f32 ey = target->pos.y - source->pos.y;
            f32 len_sq = ex * ex + ey * ey;

            f32 t = 0.0f;
            if (len_sq > kMergeDist * kMergeDist) // source/target 几乎重合(退化线段)时直接取 t=0(source端)
            {
                t = ((pos.x - source->pos.x) * ex + (pos.y - source->pos.y) * ey) / len_sq;
                t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); // clamp到线段范围内, 垂足落在线段外则退化为端点距离
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
                best_is_source_side = (t <= 0.5f);
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
        out_nearest_pos = best_pos;
        out_is_source_side = best_is_source_side;
        return 0;
    }



};




#endif
