
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
        ASSERT_TEST(s.size() == 3);
        ASSERT_TEST(atoi(buf) == 100);

        s.attach(buf, 1000, 0);
        s << (s32)-100;
        ASSERT_TEST(s.size() == 4);
        ASSERT_TEST(atoi(buf) == -100);


        s.attach(buf, 1000, 0);
        s << -100.001;
        ASSERT_TEST(atof(buf) * 1000 == -100001);


        for (s32 i = 0; i < 10000; i++)
        {
            s << i;
        }

        ASSERT_TEST(s.size() < 1000);
    }

    if (true)
    {
        zstream s(buf, 1000);
        s.fmt("%d", 100);
        s.fmt("100");
        ASSERT_TEST(std::string("100100") == s.data());
    }
    if (true)
    {
        zstream s(buf, 6);
        s.fmt("%d", 100);
        s.fmt("100");
        ASSERT_TEST(std::string("10010") == s.data());
    }
    if (true)
    {
        zstream s(buf, 1);
        ASSERT_TEST(s.size() == 0, "size:", s.size());
        s.fmt("%d", 100);
        ASSERT_TEST(s.size() == 0, "size:", s.size());
        s << 1;
        s << 1.1;
        s << 1.1f;
        s << 'c';
        s << (u8)1;
        s << (u64)1;
        s << (s64)-1;
        ASSERT_TEST(s.size() == 0, "size:", s.size());
        s.write_date(time(NULL), 0);
        s.write_date(0, 0);
        s.write_block(buf, 100);
        s.write_block(buf, 1);
        ASSERT_TEST(s.size() == 0, "size:", s.size());
        ASSERT_TEST(strlen(s.data()) == 0);
    }

    delete buf;
    return 0;
}


s32 zstream_bench_test()
{
    volatile s32 count = 0;
    if (true)
    {
        zstream_static<1000> ss;
        ss.write_date(time(NULL), 0);
        ss << "bench test[" << 0 << "] double:" << 3234.23 << " ull" << (u64)324234 << " pointer:" << (void*)&ss;
        LogInfo() << ss.data();
    }
    static constexpr s32 loop_count = 10 * 10000;
    PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, loop_count, "write stream");
    for (s32 i = 0; i < loop_count; i++)
    {
        zstream_static<1000> ss;
        ss.write_date(time(NULL), 0);
        ss << "bench test[" << i << "] double:" << 3234.23 << " ull" << (u64)324234 << " pointer:" << (void*)&ss;
        count += ss.size();
    }
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


