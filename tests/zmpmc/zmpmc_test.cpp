
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zprof.h"
#include "zforeach.h"
#include "zlist.h"
#include "zarray.h"
#include "test_common.h"
#include "ztrace.h"
#include <future>
#include <thread>

#include "zhazard_pointer.h"
#include "zmpmc.h"
#include "zarray.h"

std::atomic<int> has_error(0);

std::atomic<zmpmc::mpmc_node*> mp_head;
std::atomic<zmpmc::mpmc_node*> mp_tail;
std::atomic<zmpmc::mpmc_node*> dummy(new zmpmc::mpmc_node());

std::atomic<u64>  alloc_cnt(0);
std::atomic<u64>  free_cnt(0);

constexpr int kMaxThread = 1000;
int cur_epoch = 0;
int cur_win = 0;
std::array< std::atomic<u64>, kMaxThread> hold_epoch;
std::mutex epoch_lock;





void job()
{
    //thread_local std::atomic<u64> this_hold;

    thread_local int local_hold_idx = -1;
    thread_local zmpmc::mpmc_node local_head;
    local_head.next = nullptr;
    if (local_hold_idx == -1)
    {
        epoch_lock.lock();
        if (cur_epoch == 0)
        {
            cur_epoch = 1;
            for (size_t i = 0; i < kMaxThread; i++)
            {
                hold_epoch[i] = -1;
            }

        }

        for (int i = 0; i < hold_epoch.size(); i++)
        {
            if (hold_epoch[i] == -1)
            {
                cur_epoch++;
                local_hold_idx = (cur_epoch/kMaxThread + 1) * kMaxThread + i;
                hold_epoch[i] = local_hold_idx;
                if (cur_win <= i)
                {
                    cur_win = i+1;
                }
                break;
            }
        }
        if (local_hold_idx == -1)
        {
            epoch_lock.unlock();
            LogError() << ":";
            return;
        }

        epoch_lock.unlock();
    }


    for(int i=0; i<1000; i++)
    {
        void* ptr = (void*)(u64)((rand()% 100000) + 100) ;

        for (size_t k = 0; k < 100; k++)
        {
            zmpmc::push(mp_head, mp_tail, alloc_cnt, free_cnt, hold_epoch[local_hold_idx% kMaxThread], new zmpmc::mpmc_node());
        }
        for (size_t k = 0; k < 100; k++)
        {
            zmpmc::mpmc_node* ret = zmpmc::pop(mp_head, mp_tail, alloc_cnt, free_cnt, hold_epoch[local_hold_idx% kMaxThread]);
            if (ret)
            {
                // need check all thread epoch. 
                //delete ret; //danger 

                ret->next.store(local_head.next);
                local_head.next.store(ret);
            }
        }

        //gc  
        u64 cur_max_epoch = 0;
        for (int i = 0; i < cur_win; i++)
        {
            u64 cur = hold_epoch[i].load();
            if (cur_max_epoch < cur)
            {
                cur_max_epoch = cur;
            }
        }
        if (cur_max_epoch == 0)
        {
            cur_max_epoch = -1;
        }

        zmpmc::mpmc_node* first = &local_head;
        zmpmc::mpmc_node* second = local_head.next;
        while (second)
        {
            if (second->epoch < cur_max_epoch)
            {
                first->next.store(second->next);
                delete second;
                second = first->next;
                continue;
            }
            first = first->next;
            second = second->next;
        }


    } 
    
    if (local_hold_idx != -1)
    {
        hold_epoch[local_hold_idx% kMaxThread] = -1;
    }
}



int main(int argc, char *argv[])
{
    ztest_init();



    if (true)
    {
        u64 epoch = 0;
        u64 i = 0;
        u64 max_epoch = 0;

        while (epoch >= max_epoch)
        {
            i++;
            epoch = ((epoch) / kMaxThread +1)* kMaxThread + i;
            if (epoch > max_epoch)
            {
                max_epoch = epoch;
            }
            if (i> 1000)
            {
                break;
            }
        }

        LogInfo() << " max epoch idx:" << i << " epoch:" << epoch;

    }







    zmpmc::init(mp_head, mp_tail, alloc_cnt, free_cnt, dummy);

    std::vector<std::thread> v;
    LogInfo() << "begin:  alloc_cnt:" << alloc_cnt << ", free_cnt:" << free_cnt;
    for (size_t i = 0; i < 10; i++)
    {
        v.emplace_back(&job);
    }

    for (auto& work: v)
    {
        work.join();
    }
    v.clear();

    LogInfo() << "second:  alloc_cnt:" << alloc_cnt << ", free_cnt:" << free_cnt;
    for (size_t i = 0; i < 100; i++)
    {
        v.emplace_back(&job);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    for (auto& work : v)
    {
        work.join();
        
    }
    v.clear();

    LogInfo() << "finish: alloc_cnt:" << alloc_cnt << ", free_cnt:" << free_cnt;

    return has_error;
}


