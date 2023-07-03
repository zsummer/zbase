
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"

s32 contiainer_stress_test();

s32 sort_test();


int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");


    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;
    ASSERT_TEST(sort_test() == 0);
    ASSERT_TEST(contiainer_stress_test() == 0);
    

    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();



    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


