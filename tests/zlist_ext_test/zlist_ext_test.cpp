
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zlist_ext.h"
#include "test_common.h"

s32 coverage_test()
{
    if (true)
    {
        zlist_ext<int, 2, 1>  c1;
        ASSERT_TEST(c1.empty());
        c1.push_back(1);
        ASSERT_TEST(c1.size() == 1);
        ASSERT_TEST(c1.front() == 1);
        c1.push_back(2);

        ASSERT_TEST(c1.size() == 2);
        ASSERT_TEST(c1.full());
        ASSERT_TEST(c1.back() == 2);
        if (true)
        {
            zlist_ext<int, 2, 1> c2(c1);
            ASSERT_TEST(c2.size() == 2);
            ASSERT_TEST(c2.full());
            ASSERT_TEST(c2.back() == 2);

            ASSERT_TEST(c1.size() == 2);
            ASSERT_TEST(c1.full());
            ASSERT_TEST(c1.back() == 2);
        }


        if (true)
        {
            zlist_ext<int, 2, 1> c2; 
            c2 =(std::move(c1));
            ASSERT_TEST(c2.size() == 2);
            ASSERT_TEST(c2.full());
            ASSERT_TEST(c2.back() == 2);

            ASSERT_TEST(c1.empty());
        }
        if (true)
        {
            c1.push_back(1);
            c1.push_back(2);

            zlist_ext<int, 2, 1> c2 = std::move(c1);
            ASSERT_TEST(c2.size() == 2);
            ASSERT_TEST(c2.full());
            ASSERT_TEST(c2.back() == 2);

            ASSERT_TEST(c1.empty());
        }
    }

    if (true)
    {
        zlist_ext<std::string, 2, 1>  c1;
        ASSERT_TEST(c1.empty());
        c1.push_back("1");
        ASSERT_TEST(c1.size() == 1);
        ASSERT_TEST(c1.front() == "1");
        c1.push_back("2");

        ASSERT_TEST(c1.size() == 2);
        ASSERT_TEST(c1.full());
        ASSERT_TEST(c1.back() == "2");
        if (true)
        {
            zlist_ext<std::string, 2, 1> c2(c1);
            ASSERT_TEST(c2.size() == 2);
            ASSERT_TEST(c2.full());
            ASSERT_TEST(c2.back() == "2");

            ASSERT_TEST(c1.size() == 2);
            ASSERT_TEST(c1.full());
            ASSERT_TEST(c1.back() == "2");
        }


        if (true)
        {
            zlist_ext<std::string, 2, 1> c2;
            c2 = (std::move(c1));
            ASSERT_TEST(c2.size() == 2);
            ASSERT_TEST(c2.full());
            ASSERT_TEST(c2.back() == "2");

            ASSERT_TEST(c1.empty());
        }
        if (true)
        {
            c1.push_back("1");
            c1.push_back("2");

            zlist_ext<std::string, 2, 1> c2 = std::move(c1);
            ASSERT_TEST(c2.size() == 2);
            ASSERT_TEST(c2.full());
            ASSERT_TEST(c2.back() == "2");

            ASSERT_TEST(c1.empty());
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(coverage_test() == 0);
   

    LogInfo() << "all test finish .";

    return 0;
}


