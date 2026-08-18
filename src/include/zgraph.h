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

zkdtree 提供空间查找 这里需要选型 什么样的空间查找更合适 

这个graph有非常强的动态调整能力 

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
    std::vector<zpoint>  vertexs_;
    std::vector<u32>  vertex_frees_;
    u32 vertex_using_cnt_;

    std::vector<link> links_;
    std::vector<u32>  link_frees_;
    u32 link_using_cnt_;


public:

    s32 find_nearst_ref(zpoint center, f32 max_radius, ref& out_ref)
    {
        //空间查找
        if (true)
        {
            return -1;
        }
        out_ref = {};
        return 0;
    }

    s32 path_find(ref start, ref end);

    s32 path_find(zpoint start, zpoint end)
    {
       // like return path_find(find_nearst_ref(start), find_nearst_ref(end));
    }

    s32 remove_link(u32 link_id);
    s32 remove_vertex(u32 vertex_id);
    s32 remove_ref(ref& ref);

    s32 add_vertex(zpoint point, u32 flag);
    s32 add_link(u32 left_vex_id, u32 right_vex_id, u32 flag);
    s32 add_ref(u32 vertex_id, u32 link_id, u32 flag);


};




#endif
