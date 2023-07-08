

/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/




#pragma once 
#ifndef ZSINGLE_H
#define ZSINGLE_H

#include <stdint.h>
#include <type_traits>
#include <iterator>
#include <cstddef>
#include <memory>
#include <algorithm>

#ifndef ZBASE_SHORT_TYPE
#define ZBASE_SHORT_TYPE
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




template<class T>
class zsingle
{
public:
    zsingle(zsingle const&) = delete;
    zsingle& operator=(zsingle const&) = delete;

    static T& instance()
    {
        static T inst;
        return inst;
    }

protected:
    zsingle() = default;
    ~zsingle() = default;
};



#endif