
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
#include "zstream.h"

s32 zstream_test()
{
    char* buf = new char[1000];
    if (true)
    {
        buf[0] = '1';
        zstream s(buf, 1000);
        ASSERT_TEST(strlen(buf) == 0);

        s << (u32)100;
        ASSERT_TEST(s.offset_ == 3);
        ASSERT_TEST(atoi(buf) == 100);

        s.offset_ = 0;
        s << (s32)-100;
        ASSERT_TEST(s.offset_ == 4);
        ASSERT_TEST(atoi(buf) == -100);


        s.offset_ = 0;
        s << -100.001;
        ASSERT_TEST(atof(buf) * 1000 == -100001);


        for (s32 i = 0; i < 10000; i++)
        {
            s << i;
        }

        ASSERT_TEST(s.offset_ < 1000);
    }

    if (true)
    {
        zstream s(buf, 1000);
        s.fmt("%d", 100);
        s.fmt("100");
        ASSERT_TEST(std::string("100100") == s.buf_);
    }
    if (true)
    {
        zstream s(buf, 6);
        s.fmt("%d", 100);
        s.fmt("100");
        ASSERT_TEST(std::string("10010") == s.buf_);
    }

    delete buf;
    return 0;
}


s32 zstream_bench_test()
{

    return 0;
}

int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zstream_test() == 0);
    ASSERT_TEST(zstream_bench_test() == 0);

    LogInfo() << "all test finish .";
    return 0;
}


