
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



#define OI 0

#if defined(__GCC__) && (OI)
#pragma GCC push_options
#pragma GCC optimize(2)
#pragma GCC optimize(3)
#pragma GCC optimize("Ofast")
#pragma GCC optimize("inline")
#pragma GCC optimize("-fgcse")
#pragma GCC optimize("-fgcse-lm")
#pragma GCC optimize("-fipa-sra")
#pragma GCC optimize("-ftree-pre")
#pragma GCC optimize("-ftree-vrp")
#pragma GCC optimize("-fpeephole2")
#pragma GCC optimize("-ffast-math")
#pragma GCC optimize("-fsched-spec")
#pragma GCC optimize("unroll-loops")
#pragma GCC optimize("-falign-jumps")
#pragma GCC optimize("-falign-loops")
#pragma GCC optimize("-falign-labels")
#pragma GCC optimize("-fdevirtualize")
#pragma GCC optimize("-fcaller-saves")
#pragma GCC optimize("-fcrossjumping")
#pragma GCC optimize("-fthread-jumps")
#pragma GCC optimize("-funroll-loops")
#pragma GCC optimize("-fwhole-program")
#pragma GCC optimize("-freorder-blocks")
#pragma GCC optimize("-fschedule-insns")
#pragma GCC optimize("inline-functions")
#pragma GCC optimize("-ftree-tail-merge")
#pragma GCC optimize("-fschedule-insns2")
#pragma GCC optimize("-fstrict-aliasing")
#pragma GCC optimize("-fstrict-overflow")
#pragma GCC optimize("-falign-functions")
#pragma GCC optimize("-fcse-skip-blocks")
#pragma GCC optimize("-fcse-follow-jumps")
#pragma GCC optimize("-fsched-interblock")
#pragma GCC optimize("-fpartial-inlining")
#pragma GCC optimize("no-stack-protector")
#pragma GCC optimize("-freorder-functions")
#pragma GCC optimize("-findirect-inlining")
#pragma GCC optimize("-fhoist-adjacent-loads")
#pragma GCC optimize("-frerun-cse-after-loop")
#pragma GCC optimize("inline-small-functions")
#pragma GCC optimize("-finline-small-functions")
#pragma GCC optimize("-ftree-switch-conversion")
#pragma GCC optimize("-foptimize-sibling-calls")
#pragma GCC optimize("-fexpensive-optimizations")
#pragma GCC optimize("-funsafe-loop-optimizations")
#pragma GCC optimize("inline-functions-called-once")
#pragma GCC optimize("-fdelete-null-pointer-checks")

#pragma GCC target("avx", "sse2")

#endif


#include "fn_log.h"
#include "zarray.h"
#include "zlist.h"
#include "zhash_map.h"
#ifndef WIN32
#include <cxxabi.h>
#endif
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





template<typename ... Args>
FNLog::LogStream& ASSERT_ARGS_LOG(FNLog::LogStream&& ls, const std::string& head, Args&& ... args)
{
    ls << head << " ";
    std::initializer_list<int>{ (ls << args, '\0') ... };
    return ls;
}

#define ASSERT_TEST(expr, ...)  \
do \
{\
    if(expr) \
    { \
        std::string expr_str = #expr; \
        ASSERT_ARGS_LOG(LogDebug(), expr_str, ##__VA_ARGS__, " ok."); \
    }\
    else \
    {\
        std::string expr_str = #expr; \
        ASSERT_ARGS_LOG(LogError(), expr_str, ##__VA_ARGS__); \
        return -1; \
    }\
}\
while(0)

#define ASSERT_TEST_NOLOG(expr, ...)  \
do \
{\
    if(!(expr)) \
    {\
        std::string expr_str = #expr; \
        ASSERT_ARGS_LOG(LogError(), expr_str, ##__VA_ARGS__); \
        return -1; \
    }\
}\
while(0)



#define ASSERT_TEST_EQ(val1, val2, ...)   \
{\
    auto v1 = (val1); \
    auto v_tmp = (val2); \
    auto v2 = static_cast<decltype(v1)>(v_tmp); \
    ASSERT_TEST_NOLOG((v1)==(v2), (v1) , " ",  (v2), ##__VA_ARGS__);\
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
    unsigned long long hold_1_;
    unsigned long long hold_2_;

    operator int() const  { return val_; }
public:
    static void reset()
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
bool operator <(const RAIIVal<CLASS>& v1, const RAIIVal<CLASS>& v2)
{
    return v1.val_ < v2.val_;
}
template<int CLASS>
u32 RAIIVal<CLASS>::construct_count_ = 0;
template<int CLASS>
u32 RAIIVal<CLASS>::destroy_count_ = 0;
template<int CLASS>
u32 RAIIVal<CLASS>::now_live_count_ = 0;

template<class T>
inline std::string TypeName()
{
#ifdef WIN32
    return typeid(T).name();
#else
    int status = 0;   
    char *p = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);  
    std::string dname = p;
    free(p);
    return dname;
#endif
}



#define ASSERT_RAII_VAL(name)   \
do \
{\
    if(RAIIVal<>::construct_count_  != RAIIVal<>::destroy_count_ )\
    {\
        LogError() << name << ": ASSERT_RAII_VAL has error.  destroy / construct:" << RAIIVal<>::destroy_count_ << "/" << RAIIVal<>::construct_count_;\
        return -2; \
    } \
    else \
    {\
        LogInfo() << name << ": ASSERT_RAII_VAL.  destroy / construct:" << RAIIVal<>::destroy_count_ << "/" << RAIIVal<>::construct_count_; \
    }\
} while(0)
//#define CheckRAIIValByType(t)   ASSERT_RAII_VAL(TypeName<decltype(t)>());
#define CheckRAIIValByType(t)   ASSERT_RAII_VAL(TypeName<t>());



static inline void FNLogFunc(const ProfSerializer& serializer)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(serializer.buff(), (int)serializer.offset());
}


#if defined(__GCC__) && (OI)
#pragma GCC pop_options
#endif

#endif