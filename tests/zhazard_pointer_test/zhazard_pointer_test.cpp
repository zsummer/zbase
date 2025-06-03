
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


zhazard_pointer::hazard_node  head;
std::atomic<int> has_error(0);
std::atomic<int> list_size(0);
std::atomic_int in;

zhazard_pointer::hazard_node* node_alloc()
{
    list_size++;
    return new zhazard_pointer::hazard_node;
}

void job()
{
    for(int i=0; i<100*10000; i++)
    {
        void* ptr = (void*)(u64)((rand()% 100000) + 100) ;
        zhazard_pointer::hazard_node* keep = zhazard_pointer::push(&head, ptr, node_alloc);
        if (keep == nullptr || keep->hazard_ptr != ptr)
        {
            LogError() << "has error";
            has_error++;
            return;
        }
        volatile s64 tick = FNLog::LogStream::get_tick();
        for (size_t i = 0; i < 5; i++)
        {
            tick += FNLog::LogStream::get_tick();
        }
        (void)tick;
        if (keep->hazard_ptr != ptr)
        {
            LogError() << "has error";
            has_error++;
            return;
        }
        zhazard_pointer::release(keep);
    } 
}



int main(int argc, char *argv[])
{
    ztest_init();
    std::vector<std::thread> v;

    LogInfo() << "begin:  list_size:" << list_size;
    for (size_t i = 0; i < 60; i++)
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
    zhazard_pointer::clear(head.next);
    LogInfo() << "finish: list_size:" << list_size;

    return has_error;
}


