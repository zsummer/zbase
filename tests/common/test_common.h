
/*
* test_common License
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




#include "ztest.h"
#include "zarray.h"
#include "zlist.h"
#include "zhash_map.h"



#ifndef  TEST_COMMON_H
#define TEST_COMMON_H






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



#endif