
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>. 
* All rights reserved
* This file is part of the zbase, used MIT License.   
*/



#pragma once 
#ifndef ZSYMBOLS_H
#define ZSYMBOLS_H

#include <stdint.h>
#include <type_traits>
#include <iterator>
#include <cstddef>
#include <memory>
#include <algorithm>


//default use format compatible short type .  
#if !defined(ZBASE_USE_AHEAD_TYPE) && !defined(ZBASE_USE_DEFAULT_TYPE)
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

#ifndef WIN32
#include <cxxabi.h>
#endif // !WIN32


/* type_traits:
*
* is_trivially_copyable: in part
    * memset: uninit or no attach any array_data;
    * memcpy: uninit or no attach any array_data;
* shm resume : safely, require heap address fixed
    * has vptr:     no
    * static var:   no
    * has heap ptr: yes  (attach memory)
    * has code ptr: no
    * has sys ptr:  no
* thread safe: read safe
*
*/


//两套实现均提供相同的读符号名的O(1)小常数性能  

//对烧录符号入库的性能消耗不敏感并且不要求每个符号的地址独立 则应当开启resuse;   

//短字符串(对照长度8字节)优先solid方案 也是适合大多数场景的方案       
//长字符串优先fast并使用len获取字符串长度.      

//reuse模式会遍历对比 对于大型符号库而言需要进行辅助字典优化构建速度  

class zsymbols
{
public:
    //每个符号有一个s32的头部用于存储长度. 该长度包含字符串'\0'的大小
    using SymbolHead = s32;
    constexpr static s32 HEAD_SIZE = sizeof(SymbolHead);


public:
    char* space_;
    s32 space_len_;
    s32 exploit_;

    s32 attach(char* space, s32 space_len, s32 exploit_offset = 0)
    {
        if (space == nullptr)
        {
            return -1;
        }
        if (space_len <= 0)
        {
            return -2;
        }
        if (exploit_offset < 0)
        {
            return -3;
        }
        if (exploit_offset> space_len)
        {
            return -4;
        }

        space_ = space;
        space_len_ = space_len;
        exploit_ = exploit_offset;

        return 0;
    }



    const char* at(s32 name_id) const 
    {
        if (name_id >= HEAD_SIZE && name_id < exploit_)
        {
            return &space_[name_id];
        }
        return "";
    }

    s32 len(s32 name_id) const
    {
        if (name_id >= HEAD_SIZE && name_id < exploit_)
        {
            SymbolHead head;
            memcpy(&head, &space_[name_id - HEAD_SIZE], HEAD_SIZE); //adapt address align .  
            return head;
        }
        return 0;
    }

    s32 add(const char* name, s32 name_len, bool reuse_same_name) 
    {
        if (name == nullptr)
        {
            return -1;
        }
        if (space_ == nullptr)
        {
            return -2;
        }
        if (space_len_ < HEAD_SIZE)
        {
            return -3;
        }

        if (name_len == 0)
        {
            name_len = (s32)strlen(name);
        }

        if (reuse_same_name)
        {
            s32 offset = 0;
            SymbolHead head = 0;

            //有效的符号长度至少是HEAD_SIZE +  一个字符长度'\0'   
            while (offset + HEAD_SIZE + 1 <= exploit_)
            {
                memcpy(&head, space_ + offset, HEAD_SIZE);
                if (head != name_len)
                {
                    offset += HEAD_SIZE + head + 1;
                    continue;
                }
                if (strcmp(space_ + offset + HEAD_SIZE, name) == 0)
                {
                    return offset + HEAD_SIZE;
                }
                offset += HEAD_SIZE + head + 1;
            }
        }



        s32 new_symbol_len = HEAD_SIZE + name_len + 1;
        s32 new_symbol_id = exploit_ + HEAD_SIZE;

        if (exploit_ + new_symbol_len > space_len_)
        {
            return -5;
        }

        SymbolHead head = name_len;
        memcpy(space_ + exploit_, &head, HEAD_SIZE);
        memcpy(space_ + exploit_ + HEAD_SIZE, name, (u64)name_len + 1);
        space_[exploit_ + new_symbol_len - 1] = '\0';

        exploit_ += new_symbol_len;
        return new_symbol_id;
    }

    s32 clone_from(const zsymbols& from)
    {
        if (space_ == nullptr)
        {
            return -1;
        }
        if (space_ == from.space_)
        {
            return -2;
        }

        if (from.space_ == nullptr || from.space_len_ < HEAD_SIZE + 1)
        {
            return -3;
        }

        //support shrink  
        if (space_len_ < from.exploit_)
        {
            return -4;
        }
        memcpy(space_, from.space_, (s64)from.exploit_);
        exploit_ = from.exploit_;
        return 0;
    }

    s32 clear()
    {
        exploit_ = 0;
        return 0;
    }
    s32 reset()
    {
        space_ = nullptr;
        space_len_ = 0;
        exploit_ = 0;
        return 0;
    }

    s32 swap(zsymbols& from)
    {
        if (space_ == from.space_)
        {
            return -1;
        }

        char* space = space_;
        s32 space_len = space_len_;
        s32 exploit = exploit_;
        space_ = from.space_;
        space_len_ = from.space_len_;
        exploit_ = exploit;
        from.space_ = space;
        from.space_len_ = space_len;
        from.exploit_ = exploit;
        return 0;
    }


    template<class _Ty>
    static inline std::string readable_class_name()
    {
#ifdef WIN32
        return typeid(_Ty).name();
#else
        int status = 0;
        char* p = abi::__cxa_demangle(typeid(_Ty).name(), 0, 0, &status);
        std::string dname = p;
        free(p);
        return dname;
#endif
    }
};




template<s32 BuffSize>
class zsymbols_static :public zsymbols
{
public:
    zsymbols_static()
    {
        static_assert(BuffSize >= zsymbols::HEAD_SIZE + 1, "");
        zsymbols::space_ = nullptr;
        zsymbols::space_len_ = 0;
        attach(space_, BuffSize);
    }
    s32 swap(zsymbols& from)
    {
        //静态buff不允许swap  持有和使用分离会有生命周期管理问题  
        return -1;
    }

private:
    char space_[BuffSize];
};



#endif