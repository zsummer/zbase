
/*
* zcontain License
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#include "fn_log.h"
#include "zarray.h"
#include "zlist.h"
#include "zhash_map.h"

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

#ifndef  ZCONTAIN_H
#define ZCONTAIN_H


#define AssertCheck(val1, val2, desc)   \
{\
    auto v1 = (val1); \
    auto v2 = (val2); \
    if ((v1)!=(v2)) \
    { \
        LogError() << (v1) << " " << (v2) <<" " << desc << " failed.";  \
    } \
}


#define AssertTest(val1, val2, desc)   \
{\
    auto v1 = (val1); \
    auto v2 = (val2); \
    if ((v1)==(v2)) \
    { \
        LogDebug() << (v1) << " " << (v2) <<" " << desc << " pass.";  \
    } \
    else  \
    { \
        LogError() << (v1) << " " << (v2) <<" " << desc << " failed.";  \
        return 1U;  \
    } \
}


template<int CLASS = 0>
class RAIIVal
{
public:
    static u32 construct_count_;
    static u32 destroy_count_;
    static u32 now_live_count_;
public:
    int val_;
    operator int() { return val_; }
public:
    void reset()
    {
        construct_count_ = 0;
        destroy_count_ = 0;
        now_live_count_ = 0;
    }
    RAIIVal()
    {
        val_ = 0;
        construct_count_++;
        now_live_count_++;
    }
    RAIIVal(const RAIIVal& v)
    {
        val_ = v.val_;
        construct_count_++;
        now_live_count_++;
    }
    RAIIVal(int v)
    {
        val_ = v;
        construct_count_++;
        now_live_count_++;
    }
    ~RAIIVal()
    {
        destroy_count_++;
        now_live_count_--;
    }

    RAIIVal<CLASS>& operator=(const RAIIVal<CLASS>& v)
    {
        val_ = v.val_;
        return *this;
    }
    RAIIVal<CLASS>& operator=(int v)
    {
        val_ = v;
        return *this;
    }
};
template<int CLASS>
u32 RAIIVal<CLASS>::construct_count_ = 0;
template<int CLASS>
u32 RAIIVal<CLASS>::destroy_count_ = 0;
template<int CLASS>
u32 RAIIVal<CLASS>::now_live_count_ = 0;

inline s32 CheckRAIIVal(const std::string& name)
{
    if (RAIIVal<>::construct_count_  != RAIIVal<>::destroy_count_ )
    {
        LogError() << name << ": CheckRAIIVal has error.  destroy / construct:" << RAIIVal<>::destroy_count_ << "/" << RAIIVal<>::construct_count_;
    }
    return 0;
}



#endif