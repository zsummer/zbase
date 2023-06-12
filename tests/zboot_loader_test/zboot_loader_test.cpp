/*
* zshm_loader License
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
#include "zshm_loader.h"
#include <memory>
#include <zarray.h>


using shm_header = zarray<u32, 100>;



s32 shm_loader_base_test()
{

    return 0;
}




s32 shm_loader_stress_test()
{

    return 0;
}


int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    PROF_INIT("shm_loader");
    PROF_SET_OUTPUT(&FNLogFunc);

    ASSERT_TEST(shm_loader_base_test() == 0);
    ASSERT_TEST(shm_loader_stress_test() == 0);

    LogInfo() << "all test finish .";
    return 0;
}


