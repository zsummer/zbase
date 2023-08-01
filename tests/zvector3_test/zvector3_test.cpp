
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
#include "zvector3.h"


s32 zvector3_test()
{
    zvector3<> pos;
    ASSERT_TEST(pos.is_zero());
    return 0;
}




s32 zvector3_bench_test()
{
    zvector3<> pos;
    ASSERT_TEST(pos.is_zero());
    return 0;
}

int main(int argc, char *argv[])
{
    ztest_init();
    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zvector3_test() == 0);

    ASSERT_TEST(zvector3_bench_test() == 0);

    LogInfo() << "all test finish .";
    return 0;
}


