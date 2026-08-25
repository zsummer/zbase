
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zgraph.h"


int main(int argc, char* argv[])
{
    ztest_init();

    LogDebug() << " main begin test. ";



    LogInfo() << "all test finish .";
    return 0;
}
