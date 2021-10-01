
/*
* zobj_pool License
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

#include "base_def.h"
#include "fn_log.h"


#ifndef ZOBJ_POOL_H
#define ZOBJ_POOL_H

#ifdef WIN32
#pragma warning( push )
#pragma warning(disable : 4200)
#endif // WIN32
namespace zsummer
{
    namespace shm_arena
    {

        //list need init all nodes.
        template<class _Ty, bool _Has_Vtpr = false>
        class alignas(std::max_align_t) zobj_pool
        {
        public:
            using space_type = typename std::aligned_storage<sizeof(_Ty), alignof(_Ty)>::type;
            static const u64 FENCE_VAL = 0xdeadbeafdeadbeaf;
            struct Node
            {
                u64 fence;
                u32 next;
                u32 front;
                space_type space;
            };
            
            u32 used_id_;
            u32 used_count_;
            u32 free_id_;
            u32 end_id_;
            u32 node_begin_;
            alignas(alignof(u64)) char space_[0];// used bit + node array
        public:
            constexpr static u32 node_begin(u32 obj_count) noexcept { return ((obj_count + 63U) >> 6U) << 3U; }
            constexpr static u32 static_buf_size(u32 obj_count) noexcept 
            {
                return (u32)sizeof(zobj_pool<_Ty>) + node_begin(obj_count) + (u32)sizeof(Node) * (obj_count+1U);
            }
        private:
            void set_bitmap(u32 node_index) { ((u64*)space_)[node_index >> 6] |= 1ULL << (node_index & 63); }
            void unset_bitmap(u32 node_index) { ((u64*)space_)[node_index >> 6] &= ~(1ULL << (node_index & 63)); }
            bool has_bitmap(u32 node_index) { return ((u64*)space_)[node_index >> 6] & (1ULL << (node_index & 63)); }
        public:
            s32 init(u32 real_size, u32 space_bytes)
            {
                if (real_size == 0)
                {
                    return -1;
                }
                if (static_buf_size(real_size) > space_bytes)
                {
                    return -2;
                }

                node_begin_ = node_begin(real_size);
                memset(space_, 0, node_begin_);


                end_id_ = real_size;
                used_id_ = end_id_;
                free_id_ = 0;
                used_count_ = 0;
                Node* data = (Node*)&space_[node_begin_];
                for (size_t i = 0; i <= real_size; i++)
                {
                    data[i].next = (u32)i + 1;
                    data[i].fence = FENCE_VAL;
                    data[i].front = end_id_;
                }
                data[end_id_].next = end_id_;
                return 0;
            }
        public:
            size_t size() { return used_count_; }
            size_t max_size() { return end_id_; }
            size_t empty() { return size() == 0; }
            void clear()
            {
                Node* data = (Node*)&space_[node_begin_];
                while (used_id_ != end_id_)
                {
                    _Ty* MAY_ALIAS pty = reinterpret_cast<_Ty*>(&data[used_id_].space);
                    pty->~_Ty();
                    u32 free_id = used_id_;
                    used_id_ = data[used_id_].next;
                    data[free_id].next = free_id_;
                    free_id_ = free_id;
                    unset_bitmap(free_id);
                    used_count_--;
                }
            }

            template< class... Args >
            _Ty* create(Args ... args)
            {
                Node* data = (Node*)&space_[node_begin_];
                u64* bitmap = (u64*)&space_[0];
                (void)bitmap;
                if (free_id_ == end_id_)
                {
                    return NULL;
                }
                u32 new_id = free_id_;
                free_id_ = data[free_id_].next;
                data[new_id].next = used_id_;
                data[new_id].front = end_id_;
                data[used_id_].front = new_id;
                used_id_ = new_id;
                used_count_++;
                
                new (&data[new_id].space) _Ty(args ...);
                set_bitmap(new_id);
                return (_Ty*)&data[new_id].space;
            }
            void destroy(_Ty* obj)
            {
                Node* data = (Node*)&space_[node_begin_];
                u64* bitmap = (u64*)&space_[0];
                (void)bitmap;
                u32 free_id = (u32)(((u64)obj - (u64)&data->space)/sizeof(Node));
                if (free_id >= end_id_)
                {
                    LogError() << "free id:<" << free_id << ">. end_id:<" << end_id_ <<">. over range.";
                    return;
                }
                if (!has_bitmap(free_id))
                {
                    LogError() << "not used error. " << "free id:" << free_id <<".";
                    return;
                }
                obj->~_Ty();
                unset_bitmap(free_id);
                data[data[free_id].next].front = data[free_id].front;
                if (free_id == used_id_)
                {
                    used_id_ = data[free_id].next;
                }
                else
                {
                    data[data[free_id].front].next = data[free_id].next;
                }

                data[free_id].next = free_id_;
                free_id_ = free_id;
                used_count_--;
                return;
            }

            void resume()
            {
                if (_Has_Vtpr)
                {
                    Node* data = (Node*)&space_[node_begin_];
                    std::unique_ptr<_Ty> temp_obj = std::make_unique<_Ty>();
                    u32 used_id = used_id_;
                    while (used_id != end_id_)
                    {
                        *(u64*)data[used_id].space = *(u64*)temp_obj.get();
                        used_id = data[used_id].next;
                    }
                }
            }

            void safely_foreach(u32 left, u32 right)
            {
                u64* bitmap = &space_[0];
                while (left != right)
                {
                    if (bitmap[left >> 6] & (left & 63))
                    {
                        //call
                    }
                    left++;
                }
            }
        };

        //foreach
    }
}

#ifdef WIN32
#pragma warning( pop  )
#endif // WIN32
#endif