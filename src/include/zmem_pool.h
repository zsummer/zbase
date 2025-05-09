

/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once 
#ifndef ZMEM_POOL_H
#define ZMEM_POOL_H

#include <stdint.h>
#include <string.h>
#include <type_traits>
#include <cstddef>


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


// init new memory with 0xfd    
//#define ZDEBUG_UNINIT_MEMORY

// backed memory immediately fill 0xdf 
//#define ZDEBUG_DEATH_MEMORY  

// open and check fence 
//#define ZDEBUG_CHECK_POOL_HEALTH


/* type_traits:
*
* is_trivially_copyable: in part
    * memset: uninit or no dync heap
    * memcpy: uninit or no dync heap
* shm resume : safely, require heap address fixed
    * has vptr:     no
    * static var:   no
    * has heap ptr: yes
    * has code ptr: no
* thread safe: read safe
*
*/




class zmem_pool
{
public:
    s32 obj_size_;
    s32 name_id_;
    s32 debug_error_;
    u64 obj_vptr_;
    s32 obj_count_;
    s32 exploit_;
    s32 chunk_size_;
    s32 used_count_;
    s32 free_id_;
    char* space_;
    s64  space_size_;

    

    static constexpr u32 FENCE_4 = 0xbeafbeaf;
    static constexpr s32 HEAD_SIZE = 8;
    static constexpr u64 HEAD_USED = (1ULL << 63) | FENCE_4;
    static constexpr u64 HEAD_UNUSED = FENCE_4;
    static constexpr s32 ALIGN_SIZE = HEAD_SIZE;

    static_assert(ALIGN_SIZE >= 8, "min vptr size");

    //8字节对齐 
    static constexpr s32 align_size(s32 input_size) { return ((input_size == 0 ? 1 : input_size) + ALIGN_SIZE - 1) / ALIGN_SIZE * ALIGN_SIZE; }
    constexpr static s64 calculate_space_size(s32 obj_size, s32 total_count) { return (HEAD_SIZE + align_size(obj_size)) * 1ULL * total_count + HEAD_SIZE; }

    //utils: 
    template<class _Ty, class... Args>
    static inline u64 get_vptr(Args&&... args)
    {
        if (std::is_polymorphic<_Ty>::value)
        {
            auto get_vptr_lambda = [&]()
            {
                _Ty* p = new _Ty(std::forward<Args>(args) ...);
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
        return 0;
    }


public:
    union chunk
    {
        struct
        {
            u32 fence_;
            u32 free_id_: 31;
            u32 used_ : 1;
            char data_[8];
        };
        struct
        {
            u64 head_;
            char _[8];
        };
    };

public:
    inline chunk* ref(s32 chunk_id) 
    { 
        return  reinterpret_cast<chunk*>(space_ + chunk_size_ * chunk_id); 
    }
    inline char* at(s32 chunk_id)
    { 
        return  &ref(chunk_id)->data_[0]; 
    }

    inline s32 resolve_chunk_id_from_obj(void* obj)
    {
        char* addr = (char*)obj - HEAD_SIZE;
        if (addr < space_ || addr + chunk_size_ >= space_ + space_size_)
        {
            return obj_count_;
        }
        return (s32)((addr - space_) / chunk_size_);
    }
    inline s32 resolve_chunk_id_from_chunk(void* chunk)
    {
        char* addr = (char*)chunk;
        if (addr < space_ || addr + chunk_size_ >= space_ + space_size_)
        {
            return obj_count_;
        }
        return (s32)((addr - space_) / chunk_size_);
    }

    inline chunk* safe_ref(s32 chunk_id)
    {
        if (chunk_id >= exploit_)
        {
            return nullptr;
        }
        return  reinterpret_cast<chunk*>(space_ + chunk_size_ * chunk_id);
    }

    inline char* safe_at(s32 chunk_id)
    {
        chunk* c = safe_ref(chunk_id);
        if (c == nullptr)
        {
            return nullptr;
        }
        if (!c->used_)
        {
            return nullptr;
        }
        return  &c->data_[0];
    }
    
    inline char* fixed(s32 chunk_id) 
    { 
        static_assert(ALIGN_SIZE >= 8, "min vptr size");

        char* p = safe_at(chunk_id);
        if (p == nullptr)
        {
            return nullptr;
        }
        if (obj_vptr_ == 0)
        {
            return p;
        }
        if (obj_vptr_ != *(u64*)p)
        {
            *(u64*)p = obj_vptr_;
        }
        return  p;
    }
    template<class _Ty>
    inline _Ty* cast(s32 chunk_id) { return reinterpret_cast<_Ty*>(fixed(chunk_id)); }
    


    inline s32   chunk_size() const { return chunk_size_; }
    inline s32   max_size()const { return obj_count_; }
    inline s32   window_size() const { return exploit_; } //history's largest end id for used.     
    inline s32   size()const { return used_count_; }
    inline s32   empty()const { return used_count_ == 0; }
    inline s32   full()const { return used_count_ == obj_count_; }

    template <class _Ty>
    inline s32 init_with_object(s32 name_id, s32 total_count, void* space, s64  space_size)
    {
        u64 vptr = get_vptr<_Ty>();
        s32 ret = init(sizeof(_Ty), name_id, vptr, total_count, space, space_size);
        return ret;
    }

    inline s32 init(s32 obj_size, s32 name_id, u64 obj_vptr, s32 total_count, void* space, s64  space_size)
    {
        static_assert(sizeof(zmem_pool) % sizeof(s64) == 0, "");
        static_assert(align_size(0) == align_size(8), "");
        static_assert(align_size(7) == align_size(8), "");

        s64 MIN_SPACE_SIZE = calculate_space_size(obj_size, total_count);
        if (space_size < MIN_SPACE_SIZE)
        {
            return -1;
        }
        if (space == nullptr)
        {
            return -2;
        }
        if (total_count <= 0)
        {
            return -3;
        }
        if ((u64) space  % HEAD_SIZE != 0)
        {
            return -4;
        }
        debug_error_ = 0;
        obj_size_ = obj_size;
        name_id_ = name_id;
        obj_vptr_ = obj_vptr;
        chunk_size_ = HEAD_SIZE + align_size(obj_size);
        obj_count_ = total_count;
        exploit_ = 0;
        used_count_ = 0;
        free_id_ = obj_count_;
        space_ = (char*)space;
        space_size_ = space_size;
        chunk* end_chunk = (chunk*)(space_ + chunk_size_ * exploit_);
        //end_chunk->fence_ = FENCE_4;
        end_chunk->head_ = HEAD_UNUSED;
        return 0;
    }

    s32 health(void* obj, bool is_used) const 
    {
        char* addr = (char*)obj - HEAD_SIZE;
        s64 offset = addr - space_;
        if (offset < 0)
        {
            return -1;
        }
        if (offset + chunk_size_ > space_size_)
        {
            return -2;
        }
        if (offset % chunk_size_ != 0)
        {
            return -3;
        }
        if (offset + chunk_size_ > chunk_size_ * exploit_)
        {
            return -4;
        }

        chunk* head = (chunk*)addr;
        if (is_used && ! head->used_)
        {
            return -5;
        }

        if (head->fence_ != FENCE_4)
        {
            return -6;
        }

        head = (chunk*)(addr + chunk_size_);
        if (head->fence_ != FENCE_4)
        {
            return -7;
        }
        return 0;
    }


    /*
    * 恢复虚表(多态下) 
    * 多态只支持单继承, 保证只有一个虚表, 以简化恢复虚表的复杂度.   
    * 这里限制exploit窗口和使用标记 减少恢复的量级  
    * 先修复obj_vptr_指针值  再执行该函数  
    */
    s32 resume(u64 vptr)
    {
        if (obj_vptr_ == 0 || vptr == 0)
        {
            //no vptr;  
            return 0;
        }
        obj_vptr_ = vptr;
        s32 fixed_count = 0;
        for (s32 i = 0; i < exploit_; i++)
        {
            chunk* c = ref(i);
            if (!c->used_)
            {
                continue;
            }
            fixed(i);
            fixed_count++;
        }
        if (fixed_count != used_count_)
        {
            return -1;
        }
        return 0;
    }


    inline void* exploit()
    {
        if (free_id_ != obj_count_)
        {
            chunk* c = ref(free_id_);
            free_id_ = c->free_id_;
            used_count_++;

            c->head_ = HEAD_USED;
            //c->fence_ = FENCE_4;
            //c->used_ = 1;
            //c->free_id_ = 0;

#ifdef ZDEBUG_UNINIT_MEMORY
            memset(&c->data_, 0xfd, chunk_size_ - HEAD_SIZE);
#endif 
            return &c->data_;
        }
        if (exploit_ < obj_count_)
        {
            chunk* c = ref(exploit_ ++);
            used_count_++;
            c->head_ = HEAD_USED;
            //c->fence_ = FENCE_4;
            //c->used_ = 1;
            //c->free_id_ = 0;
            ref(exploit_)->fence_ = FENCE_4;
#ifdef ZDEBUG_UNINIT_MEMORY
            memset(&c->data_, 0xfd, chunk_size_ - HEAD_SIZE);
#endif 
            return &c->data_;
        }
        return NULL;
    }

    inline s32 back(void* obj)
    {

#ifdef ZDEBUG_CHECK_POOL_HEALTH
        s32 ret = health(obj, true);
        if (ret != 0)
        {
            //memory leak when health check fail  
            debug_error_++;
        }
#endif 
        //check success 
        char* addr = (char*)obj - HEAD_SIZE;
        chunk* c = (chunk*)addr;
        if (c->fence_ != FENCE_4)
        {
            debug_error_++;
        }

        c->head_ = HEAD_UNUSED;
        //c->fence_ = FENCE_4;
        //c->used_ = 0;
        c->free_id_ = free_id_;
        free_id_ = (s32)((addr - space_)/chunk_size_);
        used_count_--;
#ifdef ZDEBUG_DEATH_MEMORY
        memset(obj, 0xdf, chunk_size_ - HEAD_SIZE);
#endif 
        return 0;
    }


    template<class _Ty>
    inline _Ty* create_without_construct()
    {
        return (_Ty*)exploit();
    }

    template<class _Ty >
    inline typename std::enable_if <std::is_trivial<_Ty>::value, _Ty>::type* create()
    {
        return (_Ty*)exploit();
    }

    template<class _Ty >
    inline typename std::enable_if <!std::is_trivial<_Ty>::value, _Ty>::type* create()
    {
        void* p = exploit();
        if (p == NULL)
        {
            return NULL;
        }
        return new (p) _Ty();
    }


    template<class _Ty, class... Args >
    inline _Ty* create(Args&&... args)
    {
        void* p = exploit();
        if (p == NULL)
        {
            return NULL;
        }
        return new (p) _Ty(std::forward<Args>(args) ...);
    }

    template<class _Ty>
    inline void destroy(const typename std::enable_if <std::is_trivial<_Ty>::value, _Ty>::type* obj)
    {
        back(obj);
    }
    template<class _Ty>
    inline void destroy(const typename std::enable_if <!std::is_trivial<_Ty>::value, _Ty>::type* obj)
    {
        obj->~_Ty();
        back(obj);
    }
};



template<s32 USER_SIZE, s32 TOTAL_COUNT>
class zmemory_static_pool
{
public:
    inline s32 init(s32 name_id, u64 vptr)
    {
        return pool_.init(USER_SIZE, name_id, vptr, TOTAL_COUNT, space_, SPACE_SIZE);
    }
    s32 resume(u64 vptr)
    {
        pool_.resume(vptr);
        return 0;
    }

    inline void* exploit() { return pool_.exploit(); }
    inline s32 back(void* obj) { return pool_.back(obj); }
    inline s32 health(void* obj, bool is_used) const { return pool_.health(obj, is_used); }

    inline s32   chunk_size() const { return pool_.chunk_size(); }
    inline s32   max_size()const { return pool_.max_size(); }
    inline s32   window_size() const { return pool_.window_size(); } //history's largest end id for used.     
    inline s32   size()const { return pool_.size(); }
    inline s32   empty()const { return pool_.empty(); }
    inline s32   full()const { return pool_.full(); }

    template<class _Ty>
    inline _Ty* create() { return pool_.template create<_Ty>(); }
    template<class _Ty, class... Args >
    inline _Ty* create(Args&&... args) { return pool_.template create<_Ty, Args ...>(std::forward<Args>(args) ...); }

    template<class _Ty>
    inline void destroy(const _Ty* obj) { pool_.destroy(obj); }

    zmem_pool& orgin_pool() { return pool_; }
private:
    constexpr static s64 SPACE_SIZE = zmem_pool::calculate_space_size(USER_SIZE, TOTAL_COUNT);
    zmem_pool pool_;
    char space_[SPACE_SIZE];
};


template<class _Ty, s32 TotalCount>
class zmem_obj_pool : public zmemory_static_pool<sizeof(_Ty), TotalCount>
{
public:
    using zsuper = zmemory_static_pool<sizeof(_Ty), TotalCount>;
    zmem_obj_pool()
    {
        u64 vptr = zmem_pool::get_vptr<_Ty>();
        zsuper::init(0, vptr);
    }
    s32 resume()
    {
        return zsuper::resume(zmem_pool::get_vptr<_Ty>());
    }
    template<class... Args >
    inline _Ty* create(Args&&... args) { return zsuper::template create<_Ty, Args ...>(std::forward<Args>(args) ...); }

    inline void destroy(const _Ty* obj) { zsuper::destory(obj); }
};




#endif