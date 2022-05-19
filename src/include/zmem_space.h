
/*
* zlist License
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



#ifndef  ZMEM_BLOCK_H
#define ZMEM_BLOCK_H
#include <string.h>
namespace zsummer
{
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

    class zmem_space
    {
    public:
        u32 chunk_size_;
        u32 chunk_count_;
        u32 chunk_exploit_offset_;
        u32 chunk_used_count_;
        u32 chunk_free_id_;
        u32 placeholder_;
        char space_[1];
    public:
        //the real size need minus sizeof(block_[1]);  
        constexpr static u32 align_chunk_size(u32 mem_size) { return ((mem_size == 0 ? 1 : mem_size) + 3) / 4 * 4; }
        constexpr static u32 calculate_total_size(u32 mem_size, u32 mem_count) { return align_chunk_size(mem_size) * mem_count + sizeof(zmem_space); }
        inline void init(u32 mem_size, u32 mem_count)
        {
            //clear head. 
            memset(this, 0, sizeof(zmem_space));
            chunk_size_ = align_chunk_size(mem_size);
            chunk_count_ = mem_count;
            chunk_free_id_ = -1;
        }
        inline void* exploit()
        {
            if (chunk_free_id_ != -1)
            {
                u32* p = (u32*)&space_[chunk_free_id_ * chunk_size_];
                chunk_free_id_ = *p;
                chunk_used_count_++;
                return (void*)p;
            }
            if (chunk_exploit_offset_ < chunk_count_)
            {
                void* p = &space_[chunk_exploit_offset_ * chunk_size_];
                chunk_exploit_offset_++;
                chunk_used_count_++;
                return (void*)p;
            }
            return NULL;
        }

        inline void back(void* addr)
        {
            u32 id = (u32)((char*)addr - &space_[0]) / chunk_size_;
            u32* p = (u32*)addr;
            *p = chunk_free_id_;
            chunk_free_id_ = id;
            chunk_used_count_--;
        }
    };

    
    template<u32 ChunkSize, u32 ChunkCount>
    class zstatic_mem_block
    {
    public:
        
        inline void init()
        {
            ref().init(ChunkSize, ChunkCount);
        }
        inline void* exploit() { return ref().exploit(); }
        inline void back(void* addr) { return ref().back(addr); }
        inline u32 used_count() { return ref().chunk_used_count_; }
        inline bool full() { return used_count() == ChunkCount; }
    private:
        inline zmem_space& ref() { return  *((zmem_space*)solo_); }
        constexpr static u32 SPACE_SIZE = zmem_space::calculate_total_size(ChunkSize, ChunkCount);
        char solo_[SPACE_SIZE];
    };

    template<class T, u32 ChunkCount>
    class zstatic_trivial_pool : public zstatic_mem_block<sizeof(T), ChunkCount>
    {
    public:
        using zsuper = zstatic_mem_block<sizeof(T), ChunkCount>;
        zstatic_trivial_pool()
        {
            zsuper::init();
        }
        T* exploit() { return (T*)zsuper::exploit(); }
    };

}


#endif