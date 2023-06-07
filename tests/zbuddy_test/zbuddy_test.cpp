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



int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    u32 memory_size = zbuddy::buddy_state_size(24);
    char* mem = new char[memory_size];
    zbuddy * buddy = zbuddy::build_zbuddy(mem, memory_size, 24);

    u32 page_index = buddy->alloc_page(10);
    ASSERT_TEST(page_index != (u32)-1);

    u32 page_size = buddy->free_page(page_index);
    ASSERT_TEST(page_size == zbuddy_fill_right(10U)+1);
    delete mem;

    LogInfo() << "all test finish .";
    return 0;
}


