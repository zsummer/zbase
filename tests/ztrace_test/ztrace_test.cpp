
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


void job(std::promise<int>& pms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    LogInfo() << "do pms set val";
    pms.set_value(1);
    LogInfo() << "done pms set val";
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}



int main(int argc, char *argv[])
{
    ztest_init();

    std::promise<int> pms;
    std::future<int> ft = pms.get_future();

    std::thread work(&job, std::ref(pms));

    work.join();
    LogInfo() << ft.get();
    return 0;
}


