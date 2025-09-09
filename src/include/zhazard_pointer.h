/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once
#ifndef  ZHAZARD_POINTER_H
#define ZHAZARD_POINTER_H

#include <math.h>
#include <cmath>
#include <type_traits>
#include <atomic>


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




class zhazard_pointer
{
public:
    struct hazard_node
    {
        std::atomic<hazard_node*> next;
        std::atomic<void*> hazard_ptr;
    };

    static hazard_node* default_alloc()
    {
        return new hazard_node;
    }
    static void default_dealloc(hazard_node* node)
    {
        delete node;
    }
    //return node in list.   
    //the result maybe not is node .  
    //user need proc node  when result not equal node;    
    static hazard_node* push(hazard_node* head, void* hazard_ptr, hazard_node*(*alloc)()  = default_alloc)
    {
        if (head == nullptr)
        {
            return nullptr;
        }
        hazard_node* cur = head;
        //try reuse  
        do
        {
            while (cur && cur->hazard_ptr != nullptr)
            {
                cur = cur->next;
            }
            if (cur == nullptr)
            {
                break;
            }
            void* expected = nullptr;
            bool ret = std::atomic_compare_exchange_weak(&cur->hazard_ptr, &expected, hazard_ptr);
            if (ret)
            {
                return cur;
            }

        } while (true);


        //try append  
        hazard_node* new_node = alloc();
        if (new_node == nullptr)
        {
            return nullptr;
        }
        new_node->next = nullptr;
        new_node->hazard_ptr = hazard_ptr;
        cur = head;
        do
        {
            while (cur && cur->next != nullptr)
            {
                cur = cur->next;
            }

            hazard_node* expected = nullptr;
            bool ret = std::atomic_compare_exchange_weak(&cur->next, &expected, new_node);
            if (ret)
            {
                break;
            }
        } while (true);
        return new_node;
    }


    static hazard_node* release(hazard_node* node)
    {
        if (node == nullptr)
        {
            return nullptr;
        }
        node->hazard_ptr = nullptr;
        return node;
    }

    static hazard_node* clear(hazard_node* head, void(*dealloc)(hazard_node*) = default_dealloc)
    {
        if (head == nullptr)
        {
            return nullptr;
        }
        hazard_node* cur = head;
        while (cur)
        {
            if (cur->hazard_ptr != nullptr)
            {
                return cur;
            }
            cur = cur->next;
        }
        cur = head;
        hazard_node* old = head;
        do
        {
            old = cur;
            cur = cur->next;
            dealloc(old);
        } while (cur);
        return nullptr;
    }
};



#endif