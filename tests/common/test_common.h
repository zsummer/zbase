
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






template<int None = 0>
class raii_object_impl
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
    raii_object_impl<None>()
    {
        val_ = 0;
        construct_count_++;
        now_live_count_++;
    }
    raii_object_impl<None>(const raii_object_impl& v)
    {
        val_ = v.val_;
        construct_count_++;
        now_live_count_++;
    }
    raii_object_impl<None>(int v)
    {
        val_ = v;
        construct_count_++;
        now_live_count_++;
    }
    ~raii_object_impl<None>()
    {
        destroy_count_++;
        now_live_count_--;
    }


    raii_object_impl<None>& operator=(const raii_object_impl<None> v)
    {
        val_ = v.val_;
        return *this;
    }
    raii_object_impl<None>& operator=(int v)
    {
        val_ = v;
        return *this;
    }

};
template<int None = 0>
bool operator <(const raii_object_impl<None>& v1, const raii_object_impl<None>& v2)
{
    return v1.val_ < v2.val_;
}

template<int None>
u32 raii_object_impl<None>::construct_count_ = 0;
template<int None>
u32 raii_object_impl<None>::destroy_count_ = 0;
template<int None>
u32 raii_object_impl<None>::now_live_count_ = 0;

using raii_object = raii_object_impl<0>;


template<class _Ty>
inline std::string readable_class_name()
{
#ifdef WIN32
    return typeid(_Ty).name();
#else
    int status = 0;   
    char *p = abi::__cxa_demangle(typeid(_Ty).name(), 0, 0, &status);
    std::string dname = p;
    free(p);
    return dname;
#endif
}



#define ASSERT_RAII_EQUAL(name)   \
do \
{\
    if(raii_object::construct_count_  != raii_object::destroy_count_ )\
    {\
        LogError() << name << ": ASSERT_RAII_EQUAL has error.  destroy / construct:" << raii_object::destroy_count_ << "/" << raii_object::construct_count_;\
        return -2; \
    } \
    else \
    {\
        LogInfo() << name << ": ASSERT_RAII_EQUAL.  destroy / construct:" << raii_object::destroy_count_ << "/" << raii_object::construct_count_; \
    }\
} while(0)

#define ASSERT_TNAME_RAII_EQUAL(_Ty)   ASSERT_RAII_EQUAL(readable_class_name<_Ty>());



#endif