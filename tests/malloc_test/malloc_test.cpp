
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zmalloc_test.h"
#include "tcmalloc/stmalloc.h"

int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    if (false)
    {
        g_st_malloc = new STMalloc;
        g_st_malloc->Init(NULL, NULL);
        void * p = st_malloc(300 * 1024);
        memset(p, 0, 300 * 1024);
        st_malloc(300 * 1024);

    }


    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;

    ASSERT_TEST_EQ(zmalloc_test(), 0, "main error");

    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();


    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


