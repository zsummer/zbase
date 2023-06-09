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

    u32 mem_size = sizeof(shm_header);

    zshm_loader loader_svr;
    zshm_loader loader_clt;


    ASSERT_TEST(!loader_svr.check_exist(198709, mem_size));
    ASSERT_TEST(loader_svr.create_from_shm() == 0);
    new (loader_svr.attach_addr()) shm_header;
    shm_header* header = (shm_header*)loader_svr.attach_addr();
    for (u32 i = 0; i < 100; i++)
    {
        header->push_back(i);
    }
    loader_svr.detach();


    ASSERT_TEST(loader_clt.check_exist(198709, mem_size));
    ASSERT_TEST(loader_clt.load_from_shm() == 0);
    
    header = (shm_header*)loader_clt.attach_addr();

    ASSERT_TEST(header->size() == 100);

    for (u32 i = 0; i < 100; i++)
    {
        ASSERT_TEST_NOLOG(header->at(i) == i);
    }
    ASSERT_TEST(loader_svr.destroy() == 0);
    ASSERT_TEST(loader_clt.destroy() == 0);

    loader_clt.reset();
    ASSERT_TEST(!loader_clt.check_exist(198709, mem_size));
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

