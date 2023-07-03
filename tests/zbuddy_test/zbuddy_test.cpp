
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zbuddy.h"
#include <memory>
#include <zarray.h>


void debug_zbuddy_log(zbuddy* buddy, const std::string& debug_info = "")
{
    auto new_log = []() { return std::move(LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL)); };
    new_log() << debug_info;
    buddy->debug_state_log(new_log);
    buddy->debug_fragment_log(new_log);
}

s32 zbuddy_base_macro_test()
{
    ASSERT_TEST(zbuddy_horizontal_offset(2) == 0, "zbuddy_horizontal_offset(2)=", zbuddy_horizontal_offset(2));
    ASSERT_TEST(zbuddy_horizontal_offset(8) == 0, "zbuddy_horizontal_offset(8)=", zbuddy_horizontal_offset(8));
    ASSERT_TEST(zbuddy_horizontal_offset(9) == 1, "zbuddy_horizontal_offset(9)=", zbuddy_horizontal_offset(9));
    return 0;
}

s32 zbuddy_base_test()
{
    static constexpr u32 space_order = 10;
    u32 memory_size = zbuddy::zbuddy_size(space_order);
    char* mem = new char[memory_size];
    zbuddy* buddy = zbuddy::build_zbuddy(mem, memory_size, space_order);

    debug_zbuddy_log(buddy, "begin alloc ");
    u32 page_index = buddy->alloc_page(zbuddy_shift_size(space_order+1));
    ASSERT_TEST(page_index == ZBUDDY_INVALID_PAGE_INDEX, page_index);
    ASSERT_TEST(buddy->get_error_count() == 1, buddy->get_error_count());
    buddy->clean_error();


    page_index = buddy->alloc_page(100U);
    ASSERT_TEST(page_index != ZBUDDY_INVALID_PAGE_INDEX);
    debug_zbuddy_log(buddy, "alloced 100 pages");


    u32 page_size = buddy->free_page(page_index);
    ASSERT_TEST(page_size == zbuddy_fill_right(100U) + 1);
    debug_zbuddy_log(buddy, "freed 100 pages ");
    ASSERT_TEST(buddy->get_error_count() == 0, buddy->get_error_count());

    delete mem;
    return 0;
}



//page size: 2m;  manager 64G memory; (zbuddy manager 32k page, order 15). 
s32 zbuddy_stress_test()
{
    static constexpr u32 space_order = 15;
    u32 memory_size = zbuddy::zbuddy_size(space_order);
    char* mem = new char[memory_size];
    zbuddy* buddy = zbuddy::build_zbuddy(mem, memory_size, space_order);


    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000 * 10000 * 2, "1000*10000 alloc/free");
        for (u32 i = 0; i < 1000*10000; i++)
        {
            buddy->free_page(buddy->alloc_page(10000));
        }
    }
    debug_zbuddy_log(buddy, "after 1000*10000 alloc/free");

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000 * 10000 * 2, "1000*10000 alloc/free rand size 1~10000");
        for (u32 i = 0; i < 1000 * 10000; i++)
        {
            buddy->free_page(buddy->alloc_page((rand()%10000) + 1));
        }
    }
    debug_zbuddy_log(buddy, "after 1000*10000 alloc/free rand size 1~10000");


    if (true)
    {
        zarray<u32, 50> used_set_1;
        zarray<u32, 50> used_set_2;
        for (u32 i = 0; i < 50; i++)
        {
            used_set_1.push_back(buddy->alloc_page((rand() % 100) + 1));
            ASSERT_TEST_NOLOG(used_set_1.back() != ZBUDDY_INVALID_PAGE_INDEX);
            used_set_2.push_back(buddy->alloc_page((rand() % 100) + 1));
            ASSERT_TEST_NOLOG(used_set_2.back() != ZBUDDY_INVALID_PAGE_INDEX);
        }
        for (auto page : used_set_1)
        {
            ASSERT_TEST_NOLOG(buddy->free_page(page) != 0);
        }
        used_set_1.clear();

        for (u32 i = 0; i < 100; i++)
        {
            if (rand()%10 < 5)
            {
                if (rand()%10 < 5)
                {
                    if (!used_set_1.full())
                    {
                        used_set_1.push_back(buddy->alloc_page((rand() % 100) + 1));
                        ASSERT_TEST_NOLOG(used_set_1.back() != ZBUDDY_INVALID_PAGE_INDEX);
                    }
                    
                }
                else
                {
                    if (!used_set_2.full())
                    {
                        used_set_2.push_back(buddy->alloc_page((rand() % 100) + 1));
                        ASSERT_TEST_NOLOG(used_set_2.back() != ZBUDDY_INVALID_PAGE_INDEX);
                    }
                }
            }
            else
            {
                if (rand() % 10 < 5)
                {
                    if (rand() % 10 < 5)
                    {
                        if (!used_set_1.empty())
                        {
                            ASSERT_TEST_NOLOG(buddy->free_page(used_set_1.back()) != 0);
                            used_set_1.pop_back();
                        }

                    }
                    else
                    {
                        if (!used_set_2.empty())
                        {
                            ASSERT_TEST_NOLOG(buddy->free_page(used_set_2.back()) != 0);
                            used_set_2.pop_back();
                        }
                    }
                }
            }
        }
        debug_zbuddy_log(buddy, "all randed");

        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000 * 10000 * 2, "1000*10000 alloc/free rand size 1~10000  on chaos");
            for (u32 i = 0; i < 1000 * 10000; i++)
            {
                buddy->free_page(buddy->alloc_page((rand() % 10000) + 1));
            }
        }









        for (auto page: used_set_1)
        {
            ASSERT_TEST_NOLOG(buddy->free_page(page) != 0);
        }
        used_set_1.clear();
        for (auto page : used_set_2)
        {
            ASSERT_TEST_NOLOG(buddy->free_page(page) != 0);
        }
        used_set_2.clear();
    }

    ASSERT_TEST(buddy->get_error_count() == 0, buddy->get_error_count());
    delete mem;
    return 0;
}


int main(int argc, char *argv[])
{
    ztest_init();

    ASSERT_TEST(zbuddy_base_macro_test() == 0);
    ASSERT_TEST(zbuddy_base_test() == 0);
    ASSERT_TEST(zbuddy_stress_test() == 0);
    LogInfo() << "all test finish .";
    return 0;
}


