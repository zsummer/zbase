
/*
* zmem_pool License
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



#ifndef  ZMEM_POOL_H
#define ZMEM_POOL_H
#include <string.h>
#include <type_traits>
#include <cstddef>

#ifndef ZBASE_SHORT_TYPE
#define ZBASE_SHORT_TYPE
using s8 = char;
using u8 = unsigned char;
using s16 = short int;
using u16 = unsigned short int;
using s32 = int;
using s32 = unsigned int;
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



class zmem_pool
{
public:
    s32 user_size_;
    s32 chunk_size_;
    s32 chunk_count_;
    s32 chunk_exploit_offset_;
    s32 chunk_used_count_;
    s32 chunk_free_id_;
    char* space_addr_;
    s64  space_size_;
    static constexpr s32 FENCE_4 = 0xbeafbeaf;
    static constexpr s64 FENCE_8 = 0xbeafbeafbeafbeaf;
    static constexpr s32 FENCE_SIZE = 8;
    static constexpr s32 ALIGN_SIZE = FENCE_SIZE;

    //8×Ö½Ú¶ÔÆë 
    static constexpr s32 align_size(s32 input_size) { return ((input_size == 0 ? 1 : input_size) + ALIGN_SIZE - 1) / ALIGN_SIZE * ALIGN_SIZE; }
    constexpr static s64 calculate_space_size(s32 user_size, s32 total_count) { return (FENCE_SIZE + align_size(user_size)) * 1ULL * total_count + FENCE_SIZE; }
public:
    union chunk
    {
        struct
        {
            s32 small_fence_;
            s32 free_id_;
        };
        struct
        {
            s64 fence_;
            char data_;
        };
    };

public:
    inline chunk* chunk_ref(s32 chunk_id) { return  reinterpret_cast<chunk*>(space_addr_ + chunk_size_ * chunk_id); }
    inline char* user_ref(s32 chunk_id) { return  &chunk_ref(chunk_id)->data_; }

    inline s32   chunk_size() const { return chunk_size_; }
    inline s32   max_size()const { return chunk_count_; }
    inline s32   window_size() const { return chunk_exploit_offset_; } //history's largest end id for used.     
    inline s32   size()const { return chunk_used_count_; }
    inline s32   empty()const { return chunk_used_count_ == 0; }
    inline s32   full()const { return chunk_used_count_ == chunk_count_; }


    inline s32 init(s32 user_size, s32 total_count, void* space_addr, s64  space_size)
    {
        static_assert(sizeof(zmem_pool) % sizeof(s64) == 0, "");
        static_assert(align_size(0) == align_size(8), "");
        static_assert(align_size(7) == align_size(8), "");

        s64 min_space_size = calculate_space_size(user_size, total_count);
        if (space_size < min_space_size)
        {
            return -1;
        }
        if (space_addr == nullptr)
        {
            return -2;
        }
        if (total_count <= 0)
        {
            return -3;
        }
        if ((u64) space_addr  % FENCE_SIZE != 0)
        {
            return -4;
        }
        user_size_ = user_size;
        chunk_size_ = FENCE_SIZE + align_size(user_size);
        chunk_count_ = total_count;
        chunk_exploit_offset_ = 0;
        chunk_used_count_ = 0;
        chunk_free_id_ = chunk_count_;
        space_addr_ = (char*)space_addr;
        space_size_ = space_size;
        chunk* end_chunk = (chunk*)(space_addr_ + chunk_size_ * chunk_exploit_offset_);
        end_chunk->fence_ = FENCE_8;
        return 0;
    }



    inline void* exploit()
    {
        if (chunk_free_id_ != chunk_count_)
        {
            chunk* c = chunk_ref(chunk_free_id_);
            chunk_free_id_ = c->free_id_;
            chunk_used_count_++;

            c->fence_ = FENCE_8;
            chunk_ref(chunk_free_id_)->small_fence_ = FENCE_4;
#ifdef ZDEBUG_UNINIT_MEMORY
            memset(&c->data_, 0xfd, chunk_size_ - FENCE_8);
#endif // ZDEBUG_UNINIT_MEMORY
            return &c->data_;
        }
        if (chunk_exploit_offset_ < chunk_count_)
        {
            chunk* c = chunk_ref(chunk_exploit_offset_ ++);
            chunk_used_count_++;

            c->fence_ = FENCE_8;
            chunk_ref(chunk_exploit_offset_)->fence_ = FENCE_8;
#ifdef ZDEBUG_UNINIT_MEMORY
            memset(&c->data_, 0xfd, chunk_size_ - FENCE_8);
#endif // ZDEBUG_UNINIT_MEMORY
            return &c->data_;
        }
        return NULL;
    }

    inline void back(void* addr)
    {
#ifdef ZDEBUG_DEATH_MEMORY
        memset(addr, 0xfd, chunk_size_);
#endif // ZDEBUG_DEATH_MEMORY
        s32 id = (s32)((char*)addr - space_addr_) / chunk_size_;
        chunk* c = chunk_ref(id);
        c->small_fence_ = FENCE_4;
        c->free_id_ = chunk_free_id_;
        chunk_free_id_ = id;
        chunk_used_count_--;
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
    inline s32 init()
    {
        return pool_.init(USER_SIZE, TOTAL_COUNT, space_, SPACE_SIZE);
    }

    inline void* exploit() { return pool_.exploit(); }
    inline void back(void* addr) { return pool_.back(addr); }


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
        zsuper::init();
    }

    template<class... Args >
    inline _Ty* create(Args&&... args) { return zsuper::template create<_Ty, Args ...>(std::forward<Args>(args) ...); }

    inline void destroy(const _Ty* obj) { zsuper::destory(obj); }
};




#endif