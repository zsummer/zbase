
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
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




#ifdef WIN32
/*
* left to right scan  
* num:<0>  ill 
* num:<1>  return 0
* num:<2>  return 1
* num:<3>  return 1
* num:<4>  return 2
* num:<0>  return (u32)-1
*/

inline u32 zbuddy_first_bit_index(u64 num)
{
    DWORD index = (DWORD)-1;
    _BitScanReverse64(&index, num);
    return (u32)index;
}

#else
#define zbuddy_first_bit_index(num) ((u32)(sizeof(u64) * 8 - __builtin_clzll((u64)num) - 1))
#endif


template<class Integer>
inline Integer zbuddy_fill_right_u32(Integer num)
{
    static_assert(std::is_same<Integer, u32>::value, "only support u32 type");
    num |= num >> 1U;
    num |= num >> 2U;
    num |= num >> 4U;
    num |= num >> 8U;
    num |= num >> 16U;
    return num;
}

//基本术语和抽象  
// shift: bit位下标   
// space: 地址空间   
// order: 数的大小 order和shift都是2为底的的指数值 
// node: 用于管理地址空间的buddy节点  
// ability: -1后为当前node空间下的最大连续空间; 0为使用中  
// index: 若无特别说明则指node的下标  
// page: zbuddy管理的平坦地址空间的最小单位;  在zbuddy中 page的数量为node的叶子节点数量  



//
#define ZBUDDY_POLICY_LEFT_FIRST 1



//基础位操作函数  
#define zbuddy_max(v1, v2)  ((v1) > (v2) ? (v1) : (v2))

#define zbuddy_is_power_of_2(num)  (!(num & (num-1)))
#define zbuddy_shift_size(shift) (1U << (shift))
#define zbuddy_high_bit_index(num) zbuddy_first_bit_index(num) 
#define zbuddy_fill_right(num) zbuddy_fill_right_u32(num)  //zbuddy管理的是page; 以page单位为4k推算 管理的连续空间约为:16 T   

//伙伴结构算法  index 指的node序号;  
#define ZBUDDY_INVALID_INDEX 0U
#define ZBUDDY_ROOT_INDEX 1U

#define zbuddy_parent(index) ((index) >> 1U)
#define zbuddy_left(index) (((index) << 1U))
#define zbuddy_right(index) (((index) << 1U) + 1)
#define zbuddy_depth(index)  zbuddy_high_bit_index(index)  //root depth is 0  
#define zbuddy_horizontal_offset(index) ((index) - zbuddy_shift_size(zbuddy_depth(index)))  //index在当前深度下的offset 


//返回码  
constexpr static u32 ZBUDDY_INVALID_PAGE_INDEX = (~0U);

//
enum ZBUDDY_ERROR_CODE  : s32
{
    ZBUDDY_EC_SUCCESS = 0,
    ZBUDDY_EC_ERROR = 1,
    ZBUDDY_EC_NO_PAGES = 2,
    ZBUDDY_EC_NO_CONTINUOUS_PAGES = 3,
    ZBUDDY_EC_ILL_PAGE_INDEX = 4,
    ZBUDDY_EC_ILL_PAGE_STATE,

    ZBUDDY_EC_ILL_PARAM = 10,
    ZBUDDY_EC_ADDR_NO_ALIGN = 11,
    ZBUDDY_EC_VERSION_MISMATCH,
    ZBUDDY_EC_WRONG_HEAD_STATE,
    ZBUDDY_EC_WRONG_NODE_STATE,
};


/* type_traits:
*
* is_trivially_copyable: safely
    * memset: safely
    * memcpy: safely
* shm resume : safely
    * has vptr:     no
    * static var:   no
    * has heap ptr: no
    * has code ptr: no
* thread safe: read safe
*
*/


/*
    char * mem = new char [zbuddy::zbuddy_size(16)];
    zbuddy* buddy = build_zbuddy(mem, zbuddy::zbuddy_size(16), 16);
    //todo used buddy  
    delete mem;  
*/

//flattened buddy tree 
class zbuddy
{
public:
    struct buddy_node
    {
        u32 ability_;
    };
public:
    inline static u32 zbuddy_size(u32 space_order);
    inline static zbuddy& instance() { return *instance_ptr(); }
    inline static zbuddy*& instance_ptr() { static zbuddy* g_zbuddy_state = NULL; return g_zbuddy_state; }
    inline static void set_global(zbuddy* state) { instance_ptr() = state; }

    inline static zbuddy* build_zbuddy(void* addr, u64 bytes, u32 space_order, s32* error_code = nullptr);
    inline static zbuddy* rebuild_zbuddy(void* addr, u64 bytes, u32 space_order, s32* error_code = nullptr);

    inline u32 alloc_page(u32 pages);
    inline u32 free_page(u32 page_index);

    //status  
    u32 get_max_space_order()  const { return space_order_; }
    u32 get_max_space_pages()  const { return zbuddy_shift_size(space_order_); }
    u32 get_max_space_nodes()  const { return zbuddy_shift_size(space_order_ + 1); }
    u32 get_first_leaf_node_index() const { return zbuddy_shift_size(space_order_); }
    u32 get_now_continuous_order()  const { return (nodes_[ZBUDDY_ROOT_INDEX].ability_ > 0 ? nodes_[ZBUDDY_ROOT_INDEX].ability_ - 1 : 0); }
    u32 get_now_continuous_pages()  const { return  (nodes_[ZBUDDY_ROOT_INDEX].ability_ > 0 ? zbuddy_shift_size(nodes_[ZBUDDY_ROOT_INDEX].ability_ - 1) : 0); }
    u32 get_now_free_pages() const { return free_pages_; }
    inline u32 get_right_bound_used() const;

    //status
    s32 get_last_error()const { return last_error_; }
    s32 get_error_count()const { return error_count_; }
    void clean_error() { last_error_ = 0; error_count_ = 0; }

    //check 
    inline bool check_node_in_used(u32 index) const;
    template<class StreamLog>
    inline void debug_state_log(StreamLog logwrap) const;
    template<class StreamLog>
    inline void debug_fragment_log(StreamLog logwrap) const;

public:
    u32 space_order_;  
    u32 free_pages_;  
    s32 last_error_;
    s32 error_count_;
    buddy_node nodes_[2]; //flexible array: buddy tree    
};





u32 zbuddy::alloc_page(u32 pages)
{
    u32 ability = zbuddy_high_bit_index(pages);
    if (!zbuddy_is_power_of_2(pages))
    {
        ability += 1;
    }
    ability += 1;

    if (free_pages_ < pages)
    {
        last_error_ = ZBUDDY_EC_NO_PAGES;
        error_count_++;
        return ZBUDDY_INVALID_PAGE_INDEX;
    }

    if (nodes_[ZBUDDY_ROOT_INDEX].ability_ < ability)
    {
        last_error_ = ZBUDDY_EC_NO_CONTINUOUS_PAGES;
        error_count_++;
        //LogError() << "no enough pages . ability:" << ability << ". buddy_state:" << this;
        return ZBUDDY_INVALID_PAGE_INDEX;
    }

    auto& tree = this->nodes_;
    u32 target_index = ZBUDDY_ROOT_INDEX;

    for (u32 cur_ability = this->space_order_ + 1; cur_ability != ability; cur_ability--)
    {
#if ZBUDDY_POLICY_LEFT_FIRST 
        target_index = tree[zbuddy_left(target_index)].ability_ >= ability ? zbuddy_left(target_index) : zbuddy_right(target_index);
#else   //碎片程度相同时左优先, 否则优先拆分更碎的伙伴树;     
        u32 left_child_index = zbuddy_left(target_index);
        u32 right_child_index = zbuddy_right(target_index);

        u32 left_ability = tree[left_child_index].ability_;
        u32 right_ability = tree[right_child_index].ability_;
        u32 min_child_index = left_ability <= right_ability ? left_child_index : right_child_index;
        u32 max_child_index = left_ability >= right_ability ? left_child_index : right_child_index;
        target_index = (tree[min_child_index].ability_ >= ability) ? min_child_index : max_child_index;
#endif
    }

    nodes_[target_index].ability_ = 0U; //hold, no page alloc ability

    free_pages_ -= 1U << (ability - 1);

    u32 page_index = zbuddy_horizontal_offset(target_index << (ability - 1));

    while ((target_index = zbuddy_parent(target_index)))
    {
        tree[target_index].ability_ = zbuddy_max(tree[zbuddy_left(target_index)].ability_, tree[zbuddy_right(target_index)].ability_);
    }

    return page_index;
}

u32 zbuddy::free_page(u32 page_index)
{
    u32 leaf_size = zbuddy_shift_size(space_order_);
    if (page_index >= leaf_size)
    {
        last_error_ = ZBUDDY_EC_ILL_PAGE_INDEX;
        error_count_++;
        //LogError() << "page_index:" << page_index << " too big than leaf_index:" << (leaf_size - 1) << ", head: " << this;
        return 0;
    }

    u32 node_index = leaf_size + page_index;  //leaf size == non-leaf size;  view the struct in file header comments  
    u32 free_order = 0U;
    auto& nodes = nodes_;

    for (; free_order < space_order_; free_order++)
    {
        if (nodes[node_index].ability_ == 0)
        {
            break;
        }
        node_index = zbuddy_parent(node_index);
    }

    if (node_index == 0)
    {
        last_error_ = ZBUDDY_EC_ILL_PAGE_STATE;
        error_count_++;
        //LogError() << "no page:" << page_index << " info head:" << this;
        return 0;
    }

    u32 ability = free_order + 1;
    nodes[node_index].ability_ = ability;
    free_pages_ += zbuddy_shift_size(free_order);

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
        u32 left_ability = nodes[zbuddy_left(node_index)].ability_;
        u32 right_ability = nodes[zbuddy_right(node_index)].ability_;
        u32 max_ability = zbuddy_max(left_ability, right_ability);
        u32 parrent_ability = (left_ability == right_ability && left_ability == ability - 1) ? ability : max_ability;
        nodes[node_index].ability_ = parrent_ability;
    }
    return zbuddy_shift_size(free_order);
}

u32 zbuddy::zbuddy_size(u32 space_order)
{
    return sizeof(zbuddy) + (sizeof(zbuddy::buddy_node) << (space_order + 1));
}


zbuddy* zbuddy::rebuild_zbuddy(void*addr, u64 bytes, u32 space_order, s32* error_code)
{
    s32 local_error_code = 0;
    if (error_code == nullptr)
    {
        //easy debug and code  
        error_code = &local_error_code;
    }

    if (addr == nullptr)
    {
        *error_code = ZBUDDY_EC_ILL_PARAM;
        //LogError() << "the addr is null." << (void*)addr;
        return NULL;
    }

    u32 buddy_tree_size = zbuddy::zbuddy_size(space_order);
    if (bytes < buddy_tree_size)
    {
        *error_code = ZBUDDY_EC_ILL_PARAM;
        //LogError() << "addr:" << (void*)addr << " may be too small: "<< "need least:" << buddy_tree_size << ", give size:" << bytes;
        return NULL;
    }

    u64 addr_mask = sizeof(void*) - 1U;
    if ((u64)addr & addr_mask)
    {
        *error_code = ZBUDDY_EC_ADDR_NO_ALIGN;
        //LogError() << "addr not align:" << (void*)addr;
        return NULL;
    }


    zbuddy* buddy_state = (zbuddy*)addr;
    //LogDebug() << "dump buddy head:" << buddy_state;

    if (buddy_state->space_order_ != space_order)
    {
        *error_code = ZBUDDY_EC_VERSION_MISMATCH;
        //LogError() << "expire space_order is:" << space_order << ", dump buddy head:" << buddy_state;
        return NULL;
    }

    if (buddy_state->free_pages_ > zbuddy_shift_size(space_order))
    {
        *error_code = ZBUDDY_EC_WRONG_HEAD_STATE;
        //LogError() << "page_free_count over the tree manager size."
        //    "max page is:" << zbuddy_shift_size(space_order) << ", target page_free_count :" << buddy_state->free_pages_;
        return NULL;
    }

    if (buddy_state->nodes_[ZBUDDY_INVALID_INDEX].ability_ != 0)
    {
        *error_code = ZBUDDY_EC_VERSION_MISMATCH;
        //LogError() << " index 0 ability not 0";
        return NULL;
    }

    for (u32 depth = 0; depth <= space_order; depth++)
    {
        u32 begin_index = zbuddy_shift_size(depth);
        u32 end_index = begin_index << 1U;
        u32 ability_ = buddy_state->space_order_ - depth + 1U;

        for (u32 index = begin_index; index < end_index; index++)
        {
            if (buddy_state->nodes_[index].ability_ > ability_)
            {
                *error_code = ZBUDDY_EC_WRONG_NODE_STATE;
                //LogError() << "ability:" << buddy_state->nodes_[index].ability_
                //    << " over the depth max ability:" << ability_ << ", depth:" << depth << ", index:" << index;
                return NULL;
            }
        }
    }
    return buddy_state;
}

zbuddy* zbuddy::build_zbuddy(void* addr, u64 bytes, u32 space_order, s32* error_code)
{
    s32 local_error_code = 0;
    if (error_code == nullptr)
    {
        //easy debug and code  
        error_code = &local_error_code;
    }


    u64 addr_mask = sizeof(void*) - 1U;
    if ((u64)addr & addr_mask)
    {
        *error_code = ZBUDDY_EC_ADDR_NO_ALIGN;
        //LogError() << "addr not align:" << (void*)addr;
        return NULL;
    }

    if (addr == NULL)
    {
        *error_code = ZBUDDY_EC_ILL_PARAM;
        //LogError() << "addr is null:" << (void*)addr;
        return NULL;
    }

    if (bytes < zbuddy::zbuddy_size(space_order))
    {
        *error_code = ZBUDDY_EC_ILL_PARAM;
        //LogError() << "target addr no enough memory to build space order:<"
        //    << space_order << "> tree.  need least:<:" << zbuddy::zbuddy_size(space_order) << ">.";
        return NULL;
    }

    zbuddy* buddy_state = (zbuddy*)addr;
    buddy_state->space_order_ = space_order;
    buddy_state->free_pages_ = zbuddy_shift_size(space_order);
    buddy_state->error_count_ = 0;
    buddy_state->last_error_ = 0;
    buddy_state->nodes_[ZBUDDY_INVALID_INDEX].ability_ = 0;
    for (u32 depth = 0; depth <= space_order; depth++)
    {
        u32 begin_index = zbuddy_shift_size(depth);
        u32 end_index = begin_index << 1U;
        u32 ability_ = buddy_state->space_order_ - depth + 1U;

        for (u32 index = begin_index; index < end_index; index++)
        {
            buddy_state->nodes_[index].ability_ = ability_;
        }
    }
    return buddy_state;
}

u32 zbuddy::get_right_bound_used() const
{
    u32 right_bound = ZBUDDY_ROOT_INDEX;
    u32 ability = space_order_ + 1;
    u32 end_page_index = 0;
    while (nodes_[right_bound].ability_ < ability && ability > 0)
    {
        if (nodes_[right_bound].ability_ == 0)
        {
            end_page_index = zbuddy_horizontal_offset(right_bound);
            end_page_index = (end_page_index + 1) << (ability - 1);
            return end_page_index;
        }
        ability--;
        if (nodes_[zbuddy_right(right_bound)].ability_ < ability)
        {
            right_bound = zbuddy_right(right_bound);
            continue;
        }
        right_bound = zbuddy_left(right_bound);
        continue;
    }
    return 0;
}


bool zbuddy::check_node_in_used(u32 index) const
{
    while (index >= ZBUDDY_ROOT_INDEX && index < get_max_space_nodes())
    {
        if (nodes_[index].ability_ == 0)
        {
            return true;
        }
        index = zbuddy_parent(index);
    }
    return false;
}

template<class StreamLog>
void zbuddy::debug_state_log(StreamLog logwrap)  const
{
    auto log = std::move(logwrap());
    log << "["
        << "  max_space_order:" << get_max_space_order()
        << ", max_space_pages:" << get_max_space_pages()
        << ", max_space_nodes:" << get_max_space_nodes()
        << ", now_continuous_order:" << get_now_continuous_order()
        << ", now_continuous_pages:" << get_now_continuous_pages()
        << ", now_free_pages:" << get_now_free_pages()
        << ", now_right_used_pages:" << get_right_bound_used()
        << ", now_continuous_pages:" << get_now_continuous_pages()
        << "]";
}

template<class StreamLog>
void zbuddy::debug_fragment_log(StreamLog logwrap) const
{
    logwrap() << "zbuddy::debug_fragment_log:";
    constexpr static u32 line_page_size = 128;
    constexpr static u32 limit_line_count = 80;

    u32 first_leaf_id = get_first_leaf_node_index();


    u32 max_line = get_max_space_pages() / line_page_size;
    if (max_line > limit_line_count)
    {
        logwrap() << "zbuddy has pages:" << get_max_space_nodes() << ", this log only show the head:" << line_page_size * limit_line_count << " pages.";
        max_line = limit_line_count;
    }
    else
    {
        if (get_max_space_pages() % line_page_size != 0)
        {
            max_line += 1;
        }
    }

    u32 line = 0;
    for (; line < max_line; line++)
    {
        auto log = std::move(logwrap());
        log << "[" << line * line_page_size << "-" << (line + 1) * line_page_size - 1 <<"]:";
        log << FNLog::LogBlankAlign<14>();
        for (u32 i = 0; i < line_page_size; i++)
        {
            u32 index = line * line_page_size + i + first_leaf_id;
            if (index >= get_max_space_nodes())
            {
                continue;
            }
            else
            {
                log << (check_node_in_used(index) ? "1" : "0");
            }
        }
    }

}



#ifdef WIN32
#pragma warning( pop  )
#endif // WIN32

#endif