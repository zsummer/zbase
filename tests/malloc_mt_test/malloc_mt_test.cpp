
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zmalloc_mt_test.h"


int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin zmalloc_mt test. ";
    volatile double cycles = 0.0f;

    ASSERT_TEST_EQ(zmalloc_mt_test(), 0, "zmalloc_mt main error");

    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();

    LogInfo() << "all zmalloc_mt test finish .salt:" << cycles;
    return 0;
}
