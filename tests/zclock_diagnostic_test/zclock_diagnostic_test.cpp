
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
void test(const char* desc, long long ms)
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
            zclock_diagnostic_ns<const char*> c("ns ", 1000 * 1000 * 1000, &test);
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
        ASSERT_TEST(g_bark);
    }

    if (true)
    {
        g_bark = false;
        if (true)
        {
            zclock_diagnostic_ms<const char*> c("ms ", 1000, &test);
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
        ASSERT_TEST(g_bark);
    }

    if (true)
    {
        g_bark = false;
        if (true)
        {
            zclock_diagnostic_ms<const char*> c("ms ", 1000, &test);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            c.diagnostic_ms();
            g_bark = false;
            c.reset_clock();
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
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


