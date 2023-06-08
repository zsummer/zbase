/*
* zbuddy License
* Copyright (C) 2014-2021 YaweiZhang <yawei.zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zbuddy.h"
#include <memory>

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
    u32 memory_size = zbuddy::get_zbuddy_state_size(space_order);
    char* mem = new char[memory_size];
    zbuddy* buddy = zbuddy::build_zbuddy(mem, memory_size, space_order);

    if (true)
    {
        auto new_log = []() { return std::move(LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL)); };
        new_log() << "begin alloc ";
        buddy->debug_state_log(new_log);
        buddy->debug_fragment_log(new_log);
    }

    u32 page_index = buddy->alloc_page(zbuddy_shift_size(space_order+1));
    ASSERT_TEST(page_index == (u32)-1, page_index);

    page_index = buddy->alloc_page(100U);
    ASSERT_TEST(page_index != (u32)-1);

    if (true)
    {
        auto new_log = []() { return std::move(LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL)); };
        new_log() << "alloced 100 pages ";
        buddy->debug_state_log(new_log);
        buddy->debug_fragment_log(new_log);
    }

    u32 page_size = buddy->free_page(page_index);
    ASSERT_TEST(page_size == zbuddy_fill_right(100U) + 1);
    if (true)
    {
        auto new_log = []() { return std::move(LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL)); };
        new_log() << "freed 100 pages ";
        buddy->debug_state_log(new_log);
        buddy->debug_fragment_log(new_log);
    }

    delete mem;
    return 0;
}


int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();

    ASSERT_TEST(zbuddy_base_macro_test() == 0);
    ASSERT_TEST(zbuddy_base_test() == 0);
    LogInfo() << "all test finish .";
    return 0;
}


