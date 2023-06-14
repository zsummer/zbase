/*
* base_con License
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


