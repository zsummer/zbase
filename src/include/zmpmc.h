/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once
#ifndef  ZMPMC_H
#define ZMPMC_H


//Michael & Scott Lock-Free Queue


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




class zmpmc
{
public:
    struct mpmc_node
    {
        std::atomic<mpmc_node*> next;
        u64 epoch = 0;
    };

    static mpmc_node* default_alloc()
    {
        return new mpmc_node;
    }

    static void default_dealloc(mpmc_node* node)
    {
        delete node;
    }

    static mpmc_node* init(std::atomic<mpmc_node*>& head, std::atomic<mpmc_node*>& tail, std::atomic<u64>& push_no, std::atomic<u64>& pop_no, mpmc_node* dummy)
    {
        head = dummy;
        head.load()->next = nullptr;
        tail.store(head);
        push_no.store(0);
        pop_no.store(0);
        return head;
    }


    static mpmc_node* push(std::atomic<mpmc_node*>& head, std::atomic<mpmc_node*>& tail, std::atomic<u64>& push_no, std::atomic<u64>& pop_no, std::atomic<u64>& hold, mpmc_node* new_node)
    {
        new_node->next = nullptr;
        hold.store(pop_no);
        while (true)
        {
            mpmc_node* last = tail;
            mpmc_node* next = tail.load()->next;
            if (last != tail)
            {
                //next not last->next 
                continue;
            }

            if (next != nullptr)
            {
                //try move tail and retry    
                mpmc_node* exp= last;
                tail.compare_exchange_weak(exp, next);
                continue;
            }

            //try append  
            mpmc_node* exp = nullptr;
            bool ok = last->next.compare_exchange_weak(exp, new_node);
            if (!ok)
            {
                //retry 
                continue;
            }
            push_no++;
            hold.store(0);
            break;

        }
        return new_node;
    }

    static mpmc_node* pop(std::atomic<mpmc_node*>& head, std::atomic<mpmc_node*>& tail, std::atomic<u64>& push_no, std::atomic<u64>& pop_no, std::atomic<u64>& hold)
    {
        hold.store(pop_no);
        while (true)
        {
            mpmc_node* first = head.load();
            mpmc_node* second = tail.load();
            //try move tail 
            if (first == second)
            {
                mpmc_node* next = second->next;
                if (next != nullptr)
                {
                    mpmc_node* expr = second;
                    tail.compare_exchange_weak(expr, next);
                    continue;
                }
                hold.store(0);
                return nullptr;
            }

            mpmc_node* next = first->next;
            if (next == nullptr)
            {
                hold.store(0);
                return nullptr;
            }
            bool ok = head.compare_exchange_weak(first, next);
            if (ok)
            {
                pop_no++;
                hold.store(0);
                first->epoch = pop_no;
                return first;
            }
        }
        hold.store(0);
        return nullptr;
    }

};



#endif