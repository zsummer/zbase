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
zgraph面向小规模图寻路

zpoint 提供向量计算 坐标封装

这个graph有非常强的动态调整能力

空间索引不再拆成独立组件 直接内置实现 因为它对 item_id 必须是稠密数组下标这个要求
本质上就是复用 zgraph 自己的 link 池形状 拆出去只是形式上的解耦 实际是结构硬拆
真正想要跟 zhash_map 一样通用的话 得换成 zhash_map<cell_key, zhash_set<link_id>> 那种实现
代价是多一层哈希和一份反查索引 现在的场景不需要 见类内 空间索引 分区注释

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
    static constexpr s32 kGridP90NodeCnt = 5;

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
    static  u64 to_terrain_key(s32 x, s32 y){return ((u64)x << 32) | (u64)y;}


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
                t.links.push(link_id);
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



};




#endif
