
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
#include "zclock_diagnostic.h"



bool g_bark = false;
void test(const char* desc, double ms)
{
    desc = desc ? desc : "";
    g_bark = true;
    LogDebug() << desc << "bark:" << ms << "ms";
}

s32 zclock_test()
{

    if (true)
    {
        g_bark = false;
        if (true)
        {
            zclock_diagnostic<const char*> c("test", 1.0, &test);
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
        ASSERT_TEST(g_bark);
    }

    if (true)
    {
        g_bark = false;
        if (true)
        {
            zclock_diagnostic<const char*> c("test ", 1.0, &test);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            c.diagnostic("1");
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            c.diagnostic("2");
            g_bark = false;
            c.reset_clock();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        ASSERT_TEST(!g_bark);
    }

    return 0;
}


int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zclock_test() == 0);
    
    LogInfo() << "all test finish .";
    return 0;
}


