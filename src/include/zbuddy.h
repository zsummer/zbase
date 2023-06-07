/*
* zbuddy License
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


#ifndef  ZBUDDY_H
#define ZBUDDY_H


#ifndef ZBASE_SHORT_TYPE
#define ZBASE_SHORT_TYPE
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



#ifdef WIN32
#pragma warning( push )
#pragma warning(disable : 4200)
#endif // WIN32

#ifndef _FN_LOG_FILE_H_
#define LogError() std::cout <<"error:"
#define LogInfo() std::cout <<"info:"
#define LogDebug() std::cout <<"debug:"
#endif // !_FN_LOG_FILE_H_



#include <vector>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <string.h>
#include <stdlib.h>
#include <cstddef>
#include <type_traits>
#include <deque>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#endif


/*  perfect binary true
*                   0        -----------   : reserve
*                   1        -----------   :  root : depth 0:  space order is max_order - depth
*
*          2                 3
*
*     4        5         6        7
*
*   8   9   10   11   12   13  14   15
*
* ----------------------------------------
*
*   0   1   2    3    4    5    6    7     : page offset
*
* (max order)  is equal  (tree depth)   and is equal (root space order)
*/

inline u32 zbuddy_first_bit_index(u64 num)
{
    DWORD index = (DWORD)-1;
    _BitScanReverse64(&index, num);
    return (u32)index;
}
/*
* right to left scan
*/

inline u32 zbuddy_last_bit_index(u64 num)
{
    DWORD index = -1;
    _BitScanForward64(&index, num);
    return (u32)index;
}

template<class Integer>
inline Integer zbuddy_fill_right(Integer num)
{
    static_assert(std::is_same<Integer, u32>::value, "only support u32 type");
    num |= num >> 1U;
    num |= num >> 2U;
    num |= num >> 4U;
    num |= num >> 8U;
    num |= num >> 16U;
    return num;
}




#define zbuddy_is_power_of_2(num)  (!(num & (num-1)))
#define zbuddy_max(v1, v2)  ((v1) > (v2) ? (v1) : (v2))


#define zbuddy_shift_size(shift) (1U << (shift))
#define zbuddy_shift_size64(shift) (1ULL << (shift))


#define zbuddy_parent(index) ((index) >>  1U)
#define zbuddy_is_left_node(index) (!((index) & 1U))
#define zbuddy_is_right_node(index) ((index) & 1U)

#define zbuddy_left(index) (((index) << 1U))
#define zbuddy_right(index) (((index) << 1U) + 1)


#define zbuddy_tree_depth(index)  zbuddy_first_bit_index(index)  //root depth is 0  
//root space order指的完整连续空间.  
#define zbuddy_node_space_order(root_space_order, index)   ((root_space_order) - zbuddy_tree_depth(index) )

#define zbuddy_node_array_size(root_space_order) (zbuddy_shift_size(root_space_order+1))


#define zbuddy_index_2_level_offset_index(depth, index) ((index) - zbuddy_shift_size(depth) )

static_assert(zbuddy_index_2_level_offset_index(1, 2) == 0, "");
static_assert(zbuddy_index_2_level_offset_index(3, 8) == 0, "");
static_assert(zbuddy_index_2_level_offset_index(3, 9) == 1, "");


#define zbuddy_node_space(head, index)  ((head)->nodes_[index].space_ability)
#define zbuddy_root_space(head) zbuddy_node_space(head, 1U)



class zbuddy
{
public:
    struct buddy_node
    {
        u32 space_ability;
    };
public:
    inline static u32 buddy_state_size(u32 space_order);
    inline static zbuddy* build_zbuddy(void* addr, u64 bytes, u32 space_order);
    inline static zbuddy* rebuild_zbuddy(u64 addr, u64 bytes, u32 space_order);

    inline u32 right_page_bound() const;
    inline void summary() const;
    inline void summary(u64 page_shift) const;
    inline void dump() const;
    inline u32 alloc_page(u32 pages);
    inline u32 free_page(u32 page_index);
protected:
    inline u32 alloc_ablility(u32 ability);
public:
    u32 space_order_;  //Key Parameter: max alloc order (tree size)  
    u32 page_free_count_;  //alloc state counter   
    buddy_node nodes_[0]; //the buddy tree  
};



u32 zbuddy::alloc_ablility(u32 ability)
{
    if (zbuddy_root_space(this) < ability)
    {
        LogError() << "no enough pages . ability:" << ability
            << ". buddy_state:" << this;
        return -1U;
    }

    auto& tree = this->nodes_;
    u32 target_index = 1;

    for (u32 cur_ability = this->space_order_ + 1; cur_ability != ability; cur_ability--)
    {
#if OPEN_BUDDY_FIRST_FIT
        target_index = tree[zbuddy_left(target_index)].space_ability >= ability ? zbuddy_left(target_index) : zbuddy_right(target_index);
#else
        u32 left_child_index = zbuddy_left(target_index);
        u32 right_child_index = zbuddy_right(target_index);

        u32 left_ability = tree[left_child_index].space_ability;
        u32 right_ability = tree[right_child_index].space_ability;
        u32 min_child_index = left_ability <= right_ability ? left_child_index : right_child_index;
        u32 max_child_index = left_ability >= right_ability ? left_child_index : right_child_index;
        target_index = (tree[min_child_index].space_ability >= ability) ? min_child_index : max_child_index;
#endif
    }

    this->nodes_[target_index].space_ability = 0U; //hold, no page alloc ability

    this->page_free_count_ -= 1U << (ability - 1);

    u32 page_index = zbuddy_index_2_level_offset_index(this->space_order_, target_index << (ability - 1));

    while (target_index = zbuddy_parent(target_index))
    {
        tree[target_index].space_ability = zbuddy_max(tree[zbuddy_left(target_index)].space_ability, tree[zbuddy_right(target_index)].space_ability);
    }

    return page_index;
}

u32 zbuddy::alloc_page(u32 pages)
{
    u32 index = zbuddy_first_bit_index(pages);
    if (!zbuddy_is_power_of_2(pages))
    {
        index += 1;
    }
    index += 1;
    return alloc_ablility(index);
}

u32 zbuddy::free_page(u32 page_index)
{
    u32 leaf_size = zbuddy_shift_size(space_order_);
    if (page_index >= leaf_size)
    {
        LogError() << "page_index:" << page_index << " too big than leaf_index:" << (leaf_size - 1) << ", head: " << this;
        return 0;
    }

    u32 node_index = leaf_size + page_index;  //leaf size == non-leaf size;  view the struct in file header comments  
    u32 free_order = 0U;
    auto& nodes = nodes_;

    for (; free_order < space_order_; free_order++)
    {
        if (nodes[node_index].space_ability == 0)
        {
            break;
        }
        node_index = zbuddy_parent(node_index);
    }

    if (node_index == 0)
    {
        LogError() << "no page:" << page_index << " info head:" << this;
        return 0;
    }

    u32 ability = free_order + 1;
    nodes[node_index].space_ability = ability;
    page_free_count_ += zbuddy_shift_size(free_order);

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#endif
    while (node_index = zbuddy_parent(node_index))
    {
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
        ability++;
        u32 left_ability = nodes[zbuddy_left(node_index)].space_ability;
        u32 right_ability = nodes[zbuddy_right(node_index)].space_ability;
        u32 max_ability = zbuddy_max(left_ability, right_ability);
        u32 parrent_ability = (left_ability == right_ability && left_ability == ability - 1) ? ability : max_ability;
        nodes[node_index].space_ability = parrent_ability;
    }
    return zbuddy_shift_size(free_order);
}

u32 zbuddy::buddy_state_size(u32 space_order)
{
    return sizeof(zbuddy) + (sizeof(zbuddy::buddy_node) << (space_order + 1));
}


zbuddy* zbuddy::rebuild_zbuddy(u64 addr, u64 bytes, u32 space_order)
{
    if (addr == 0)
    {
        LogError() << "the addr is null." << (void*)addr;
        return NULL;
    }

    u32 buddy_tree_size = zbuddy::buddy_state_size(space_order);
    if (bytes < buddy_tree_size)
    {
        LogError() << "addr:" << (void*)addr << " may be too small: "
            << "need least:" << buddy_tree_size << ", give size:" << bytes;
        return NULL;
    }

    u64 addr_mask = sizeof(void*) - 1U;
    if ((u64)addr & addr_mask)
    {
        LogError() << "addr not align:" << (void*)addr;
        return NULL;
    }


    zbuddy* buddy_state = (zbuddy*)addr;
    LogDebug() << "dump buddy head:" << buddy_state;

    if (buddy_state->space_order_ != space_order)
    {
        LogError() << "expire space_order is:" << space_order << ", dump buddy head:" << buddy_state;
        return NULL;
    }

    if (buddy_state->page_free_count_ > zbuddy_shift_size(space_order))
    {
        LogError() << "page_free_count over the tree manager size."
            "max page is:" << zbuddy_shift_size(space_order) << ", target page_free_count :" << buddy_state->page_free_count_;
        return NULL;
    }

    if (buddy_state->nodes_[0].space_ability != 0)
    {
        LogError() << " index 0 ability not 0";
        return NULL;
    }

    for (u32 depth = 0; depth <= space_order; depth++)
    {
        u32 begin_index = zbuddy_shift_size(depth);
        u32 end_index = begin_index << 1U;
        u32 space_ability = buddy_state->space_order_ - depth + 1U;

        for (u32 index = begin_index; index < end_index; index++)
        {
            if (zbuddy_node_space(buddy_state, index) > space_ability)
            {
                LogError() << "ability:" << zbuddy_node_space(buddy_state, index)
                    << " over the depth max ability:" << space_ability << ", depth:" << depth << ", index:" << index;
                return NULL;
            }
        }
    }
    return buddy_state;
}

zbuddy* zbuddy::build_zbuddy(void* addr, u64 bytes, u32 space_order)
{
    u64 addr_mask = sizeof(void*) - 1U;
    if ((u64)addr & addr_mask)
    {
        LogError() << "addr not align:" << (void*)addr;
        return NULL;
    }

    if (addr == NULL)
    {
        LogError() << "addr is null:" << (void*)addr;
        return NULL;
    }

    if (bytes < zbuddy::buddy_state_size(space_order))
    {
        LogError() << "target addr no enough memory to build space order:<"
            << space_order << "> tree.  need least:<:" << zbuddy::buddy_state_size(space_order) << ">.";
        return NULL;
    }

    zbuddy* buddy_state = (zbuddy*)addr;
    buddy_state->space_order_ = space_order;
    buddy_state->page_free_count_ = zbuddy_shift_size(space_order);
    buddy_state->nodes_[0].space_ability = 0;
    for (u32 depth = 0; depth <= space_order; depth++)
    {
        u32 begin_index = zbuddy_shift_size(depth);
        u32 end_index = begin_index << 1U;
        u32 space_ability = buddy_state->space_order_ - depth + 1U;

        for (u32 index = begin_index; index < end_index; index++)
        {
            buddy_state->nodes_[index].space_ability = space_ability;
        }
    }
    return buddy_state;
}

u32 zbuddy::right_page_bound() const
{
    u32 right_bound = 1;
    u32 ability = space_order_ + 1;
    u32 end_page_index = 0;
    while (nodes_[right_bound].space_ability < ability && ability > 0)
    {
        if (nodes_[right_bound].space_ability == 0)
        {
            end_page_index = zbuddy_index_2_level_offset_index(space_order_ + 1 - ability, right_bound);
            end_page_index = (end_page_index + 1) << (ability - 1);
            return end_page_index;
        }
        ability--;
        if (nodes_[zbuddy_right(right_bound)].space_ability < ability)
        {
            right_bound = zbuddy_right(right_bound);
            continue;
        }
        right_bound = zbuddy_left(right_bound);
        continue;
    }
    return 0;
}

void zbuddy::summary() const
{
    LogInfo() << "["
        << "  space_order:" << space_order_
        << ", max pages:" << zbuddy_shift_size(space_order_)
        << ", free pages:" << page_free_count_
        << ", right bound page:" << right_page_bound()
        << ", root:" << (u32)nodes_[1].space_ability
        << ", root ability pages:"
        << ((nodes_[1].space_ability == 0) ? 0 : zbuddy_shift_size(nodes_[1].space_ability - 1))
        << "]";
}

void zbuddy::summary(u64 page_size) const
{
    auto mf = [](u64 n) {return n / 1024.0 / 1024.0; };

    LogInfo() << "["
        << "  space:" << mf(zbuddy_shift_size64(space_order_) * page_size)
        << "m, free:" << mf(page_free_count_ * page_size)
        << "m, right bound:" << mf(right_page_bound() * page_size)
        << "m, root max block:"
        << ((nodes_[1].space_ability == 0) ? 0 : mf(zbuddy_shift_size64(nodes_[1].space_ability - 1ULL)) * 1ULL * page_size)
        << "]";
}

void zbuddy::dump() const
{
    std::deque<u32> input;
    std::deque<u32> output;

    using MemInfos = std::deque<std::pair<u32, u32>>;
    std::deque<MemInfos>  statistic;
    statistic.resize(space_order_ + 1);

    input.push_back(1);
    while (!input.empty() || !output.empty())
    {
        for (auto index : input)
        {
            u32 depth = zbuddy_tree_depth(index);
            u32 ability = space_order_ - depth + 1;
            if (zbuddy_node_space(this, index) == ability)
            {
                continue;
            }
            else if (zbuddy_node_space(this, index) == 0)
            {
                MemInfos& mems = statistic[depth];
                u32 offset = zbuddy_index_2_level_offset_index(depth, index);
                bool inserted = false;
                for (MemInfos::iterator iter = mems.begin(); iter != mems.end(); ++iter)
                {
                    u32 begin_offset = iter->first;
                    u32 end_offset = iter->first + iter->second;  //second element is the linked node count. if no mege the second is 1

                    if (offset <= begin_offset && offset > end_offset)
                    {
                        LogError() << "has duplicate space. depth:" << depth << ", offset:" << offset;
                    }

                    if (offset == end_offset)
                    {
                        iter->second += 1;
                        inserted = true;
                        break;
                    }
                    else if (offset > begin_offset)
                    {
                        mems.insert(iter, std::make_pair(offset, 1));
                        inserted = true;
                        break;
                    }
                }

                if (!inserted)
                {
                    mems.push_back(std::make_pair(offset, 1));
                }
            }
            else if (depth < space_order_)
            {
                output.push_back(zbuddy_left(index));
                output.push_back(zbuddy_right(index));
            }
        }
        input.clear();
        input.swap(output);
    }


    std::string log_content;
    for (u32 depth = 0; depth < statistic.size(); depth++)
    {
        if (statistic[depth].empty())
        {
            continue;
        }
        /*
        auto log(LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_ALARM, 0, FNLog::LOG_PREFIX_NULL));
        log << "depth[";
        log.write_number<02>(depth) << "]: ";

        for (auto& mem : statistic[depth])
        {
            if (mem.first == 0)
            {
                log << "0x0";
            }
            else
            {
                log << (void*)(u64)mem.first;
            }
            log << ":" << mem.second << ",  ";
        }
        */
    }
}



#ifdef WIN32
#pragma warning( pop  )
#endif // WIN32

#endif