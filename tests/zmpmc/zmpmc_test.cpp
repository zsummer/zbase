
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
std::atomic<int> list_size(0);
std::atomic_int in;

std::atomic<zmpmc::mpmc_node*> head;
std::atomic<zmpmc::mpmc_node*> tail;
std::atomic<zmpmc::mpmc_node*> dummy(new zmpmc::mpmc_node());

std::atomic<u64>  alloc_cnt(0);
std::atomic<u64>  free_cnt(0);

constexpr int kMaxThread = 1000;
int global_hold_epoch = 0;
std::array< std::atomic<u64>, kMaxThread> global_hold;
std::mutex mtx;

void job()
{
    //thread_local std::atomic<u64> this_hold;

    thread_local int global_hold_idx = -1;
    if (global_hold_idx == -1)
    {
        mtx.lock();
        if (global_hold_epoch == 0)
        {
            global_hold_epoch = 1;
            for (size_t i = 0; i < kMaxThread; i++)
            {
                global_hold[i] = -1;
            }

        }

        for (int i = 0; i < global_hold.size(); i++)
        {
            if (global_hold[i] == -1)
            {
                global_hold_epoch++;
                global_hold_idx = global_hold_epoch * kMaxThread + i;
                global_hold[i] = global_hold_idx;
                break;
            }
        }
        if (global_hold_idx == -1)
        {
            mtx.unlock();
            LogError() << ":";
            return;
        }

        mtx.unlock();
    }


    for(int i=0; i<1000; i++)
    {
        void* ptr = (void*)(u64)((rand()% 100000) + 100) ;

        for (size_t k = 0; k < 100; k++)
        {
            zmpmc::push(head, tail, alloc_cnt, free_cnt, global_hold[global_hold_idx% kMaxThread], new zmpmc::mpmc_node());
        }
        for (size_t k = 0; k < 100; k++)
        {
            zmpmc::mpmc_node* ret = zmpmc::pop(head, tail, alloc_cnt, free_cnt, global_hold[global_hold_idx% kMaxThread]);
            if (ret)
            {
                // need check all thread epoch. 
                delete ret; //danger 
            }
        }
    } 
    


    if (global_hold_idx != -1)
    {
        global_hold[global_hold_idx% kMaxThread] = -1;
    }
}



int main(int argc, char *argv[])
{
    ztest_init();

    zmpmc::init(head, tail, alloc_cnt, free_cnt, dummy);

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


