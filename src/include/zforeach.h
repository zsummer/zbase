/*
* zforeach License
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


#include <memory>

#ifndef ZFOREACH_H
#define ZFOREACH_H

#ifdef WIN32
#pragma warning( push )
#pragma warning(disable : 4200)
#endif // WIN32
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

    //核心目标: 分帧分段驱动大间隔轮询 借此削峰   
    //需求环境:  不要求间隔精度, 甚至允许多触发 少触发.  例如超时管理. 
    //实现环境: 静态数组软边界  
    //  - 非静态数组会遇到多触发 漏触发问题, 特别是轮询中大规模批量插入删除等情况  
    //  - 其他容器类 迭代器失效 引用数据失效 以及多触发漏触发问题

    namespace zforeach_impl
    {
        struct subframe
        {
            using Trigger = s32(*)(u64, u64, s64);
            u64 key_;
            Trigger trigger_;
            u32 segments_;
            u32 hard_begin_;
            u32 hard_end_;
            u32 soft_begin_;
            u32 soft_end_;
            u32 tick_;
            u32 next_id_; //总量动态变化时 起点在老的节点上连续, 保证每轮驱动不会漏掉或者重复.  (前提是索引不会发生平移变化)  
        };

        static inline s32 init(subframe& sub)
        {
            memset(&sub, 0, sizeof(subframe));
            sub.segments_ = 1;
            return 0;
        }

        static inline bool is_valid(const subframe& sub)
        {
            if (sub.trigger_ == NULL)
            {
                return false;
            }
            if (sub.segments_ == 0)
            {
                return false;
            }
            if (sub.soft_begin_ < sub.hard_begin_ || sub.soft_end_ > sub.hard_end_)
            {
                return false;
            }
            return true;
        }

        static inline s32 window_tick(subframe& sub, s64 now_ms)
        {
            if (!is_valid(sub))
            {
                return -1;
            }
            sub.tick_ = (sub.tick_ + 1) % sub.segments_;
            u32 begin_id = sub.next_id_;
            u32 end_id = sub.next_id_ + (sub.soft_end_ - sub.soft_begin_) / sub.segments_ + 1;
            if (end_id > sub.soft_end_)
            {
                end_id = sub.soft_end_;
            }

            //当前为最后一轮, 设置下一次开始位置 
            if (sub.tick_ == 0)
            {
                sub.next_id_ = sub.soft_begin_;
                end_id = sub.soft_end_;
            }
            else
            {
                sub.next_id_ = end_id;
            }

            for (u32 i = begin_id; i < end_id; i++)
            {
                sub.trigger_(sub.key_, (u64)i, now_ms);
            }
            return 0;
        }
        static inline s32 root_tick(subframe& sub, s64 now_ms)
        {
            sub.soft_begin_ = sub.hard_begin_;
            sub.soft_end_ = sub.hard_end_;
            return window_tick(sub, now_ms);
        }
    }

    struct zforeach
    {
        inline s32 init(u64 key, u32 begin_id, u32 end_id, zforeach_impl::subframe::Trigger trigger, u32 base_tick, u32 foreach_tick)
        {
            zforeach_impl::init(subframe_);
            if (base_tick == 0 || foreach_tick == 0)
            {
                return -1;
            }
            if ((foreach_tick % base_tick) != 0)
            {
                return -2;
            }
            if (foreach_tick < base_tick)
            {
                return -3;
            }
            if (trigger == NULL)
            {
                return -4;
            }
            if (end_id < begin_id)
            {
                return -5;
            }
            subframe_.hard_begin_ = begin_id;
            subframe_.hard_end_ = end_id;
            subframe_.next_id_ = subframe_.hard_begin_;
            subframe_.segments_ = foreach_tick / base_tick;
            subframe_.soft_begin_ = subframe_.hard_begin_;
            subframe_.soft_end_ = subframe_.hard_end_;
            subframe_.tick_ = 0;
            subframe_.key_ = key;
            subframe_.trigger_ = trigger;
            return 0;
        }
        inline s32 window_tick(u32 begin_id, u32 end_id, s64 now_ms)
        { 
            subframe_.soft_begin_ = begin_id;
            subframe_.soft_end_ = end_id;
            return zforeach_impl::window_tick(subframe_, now_ms);
        }
        zforeach_impl::subframe subframe_;
    };



    //foreach



}


#ifdef WIN32
#pragma warning( pop  )
#endif // WIN32
#endif