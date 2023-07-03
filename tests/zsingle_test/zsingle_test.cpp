
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zprof.h"
#include "zsingle.h"
#include "test_common.h"


using ga = zsingle<zarray<u32, 100>>;


void push_one(u32);

int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    push_one(99);

    ASSERT_TEST(!ga::instance().empty());
    ASSERT_TEST(ga::instance().front() == 99);
    zsingle<zarray<u32, 100>>::instance().push_back(100);
    ASSERT_TEST(ga::instance().back() == 100);

    

    LogInfo() << "all test finish .";
    return 0;
}


