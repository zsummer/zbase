

/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

#pragma once
#ifndef ZSHM_PTR_H_
#define ZSHM_PTR_H_

#include <stdint.h>
#include <type_traits>


#if __GNUG__
#include <type_traits>
#endif




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




//auto fix object's vptr on single-inheritance polymorphic  
//  warn(1): don't use this ptr to fix multi-inheritance polymorphic   
//  warn(2): don't use the real object's bases class. 
//  warn(3): shm_ptr not control object life,  it's same not unique_ptr/shared_ptr
//  warn(4): InstType must has default and public construct function.  get vptr used a temporary obj instance ;

//marks(1):  gcc tr2 bases can collect all base class type. used dynamic_cast get the right offset and vtable can suport multi-inheritance.  more cost;   

template<class InstType>  
class zshm_ptr
{
public:
    zshm_ptr(void* addr){ obj_ = reinterpret_cast<InstType*>(addr);}

    InstType* reset(void* addr){obj_ = reinterpret_cast<InstType*>(addr);}
    InstType* get() { return fix_vptr(); }
    const InstType* get() const { return fix_vptr(); }

    InstType* operator ->()
    {
        return fix_vptr();
    }
    const InstType* operator ->() const
    {
        return fix_vptr();
    }

    InstType& operator *()
    {
        return *fix_vptr();
    }
    const InstType& operator *() const
    {
        return *fix_vptr();
    }


    template<class _Ty = InstType>
    typename std::enable_if<std::is_polymorphic<_Ty>::value, _Ty>::type* fix_vptr()
    {
        //这里使用局部静态变量, 局部变量call_once时机在首次调用;  
        //** 因此可以避免resume框架中底层内存分配器未准备好时, 因修复带有动态内存分配的虚表对象而产生问题  
        static u64 vtable_adress = get_vtable_address<_Ty>();
        //O2下直接范围obj_会因strict-aliasing假定导致上层直接cache旧的vtable 
        //(1) volatile阻止strict-aliasing 
        u64 obj_vtable_adrees = *reinterpret_cast<u64*>(reinterpret_cast<char*>(obj_));
        if (obj_vtable_adrees != vtable_adress)
        {
            //(2) memcpy (char*  阻止 
            memcpy((char*)obj_, &vtable_adress, sizeof(vtable_adress));
            //obj_vtable_adrees = vtable_adress;
        }
        //(1)(2) cost= org ptr's cost *2
        return obj_;
    }

    
    template<class _Ty = InstType>
    typename std::enable_if<!std::is_polymorphic<_Ty>::value, _Ty>::type* fix_vptr()
    {
        //not polymorphic: do nothing  
        return obj_;
    }

private:
    InstType* obj_;

    //helpers  
private:
    template<class _Ty>
    struct zshm_is_big_polymorphic
    {
        static const bool value = std::is_polymorphic<_Ty>::value && sizeof(_Ty) > 1024;
    };

    template<class _Ty>
    struct zshm_is_small_polymorphic
    {
        static const bool value = std::is_polymorphic<_Ty>::value && sizeof(_Ty) <= 1024;
    };

    template<class _Ty>
    static inline typename std::enable_if< zshm_is_big_polymorphic<_Ty>::value, u64>::type
        get_vtable_address()
    {
        auto get_vptr_lambda = []()
        {
            _Ty* p = new _Ty();
            u64 vptr = 0;
            //char* can safly clean warn: strict-aliasing rules  
            memcpy(&vptr, (char*)p, sizeof(vptr));
            delete p;
            return vptr;
        };
        //similar behavior can be obtained for arbitrary functions with std::call_once;  from C++11;  
        static u64 vptr = get_vptr_lambda();
        return vptr;
    }

    template<class _Ty>
    static inline typename std::enable_if< zshm_is_small_polymorphic<_Ty>::value, u64>::type
        get_vtable_address()
    {
        auto get_vptr_lambda = []()
        {
            _Ty t;
            u64 vptr = 0;
            //char* can safly clean warn: strict-aliasing rules  
            memcpy(&vptr, (char*)&t, sizeof(vptr));
            return vptr;
        };
        //similar behavior can be obtained for arbitrary functions with std::call_once;  from C++11;  
        static u64 vptr = get_vptr_lambda();
        return vptr;
    }

    template<class _Ty>
    static inline
        typename std::enable_if <!std::is_polymorphic<_Ty>::value, u64>::type
        get_vtable_address()
    {
        return 0;
    }



};


#endif

