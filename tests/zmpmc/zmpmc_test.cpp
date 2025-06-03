
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


std::atomic<int> has_error(0);
std::atomic<int> list_size(0);
std::atomic_int in;

std::atomic<zmpmc::mpmc_node*> head;
std::atomic<zmpmc::mpmc_node*> tail;
std::atomic<zmpmc::mpmc_node*> dummy = new zmpmc::mpmc_node();

std::atomic<long long>  alloc_cnt(0);
std::atomic<long long>  free_cnt(0);

void job()
{
    for(int i=0; i<1000; i++)
    {
        void* ptr = (void*)(u64)((rand()% 100000) + 100) ;

        for (size_t k = 0; k < 100; k++)
        {
            alloc_cnt++;
            zmpmc::push(head, tail, new zmpmc::mpmc_node());
        }
        for (size_t k = 0; k < 100; k++)
        {
            zmpmc::mpmc_node* ret = zmpmc::pop(head, tail);
            if (ret)
            {
                free_cnt++;
                delete ret; //danger 
            }
        }
    } 
}



int main(int argc, char *argv[])
{
    ztest_init();

    zmpmc::init(head, tail, dummy);

    std::vector<std::thread> v;
    LogInfo() << "begin:  list_size:" << list_size;
    for (size_t i = 0; i < 10; i++)
    {
        v.emplace_back(&job);
    }

    for (auto& work: v)
    {
        work.join();
    }
    v.clear();

    LogInfo() << "second:  list_size:" << list_size;
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

    LogInfo() << "finish: list_size:" << list_size;

    return has_error;
}


