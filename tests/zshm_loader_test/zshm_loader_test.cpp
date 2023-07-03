
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
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

    zshm_loader loader_svr(false, 1987090, mem_size);
    zshm_loader loader_clt(false, 1987090, mem_size);


    ASSERT_TEST(loader_svr.check() != 0);
    ASSERT_TEST(loader_svr.create() == 0);
    new (loader_svr.shm_mnt_addr()) shm_header;
    shm_header* header = (shm_header*)loader_svr.shm_mnt_addr();
    for (u32 i = 0; i < 100; i++)
    {
        header->push_back(i);
    }
    ASSERT_TEST(loader_svr.detach() == 0);



    ASSERT_TEST(loader_clt.check() == 0);
    ASSERT_TEST(loader_clt.attach() == 0);
    
    header = (shm_header*)loader_clt.shm_mnt_addr();

    ASSERT_TEST(header->size() == 100);

    for (u32 i = 0; i < 100; i++)
    {
        ASSERT_TEST_NOLOG(header->at(i) == i);
    }
    ASSERT_TEST(loader_svr.destroy() == 0);
    ASSERT_TEST(loader_clt.destroy() == 0);


    loader_clt.init(false, 1987090, mem_size);
    ASSERT_TEST(loader_clt.check() != 0);

    return 0;
}



s32 shm_loader_unix_test()
{
    u32 mem_size = sizeof(shm_header);

    zshm_loader loader_svr(false, 1987090, mem_size);

    ASSERT_TEST(loader_svr.check()!= 0);
    ASSERT_TEST(loader_svr.create(0x0000700000000000) == 0);
    if (loader_svr.shm_mnt_addr() != (void*)0x0000700000000000ULL)
    {
        LogError() << "mnt addr:" << loader_svr.shm_mnt_addr();
    }

    new (loader_svr.shm_mnt_addr()) shm_header;
    shm_header* header = (shm_header*)loader_svr.shm_mnt_addr();
    for (u32 i = 0; i < 100; i++)
    {
        header->push_back(i);
    }
    ASSERT_TEST(loader_svr.detach() == 0);
    ASSERT_TEST(loader_svr.destroy() == 0);
    ASSERT_TEST(loader_svr.check() != 0);
    return 0;
}




s32 shm_loader_stress_test()
{
    u32 mem_size = sizeof(shm_header);

    zshm_loader shm_loader(false, 198709, mem_size);
    zshm_loader heap_loader(true, 198709, 2000 * 1024 * 1024);
    ASSERT_TEST(shm_loader.check() != 0);
    ASSERT_TEST(shm_loader.create() == 0);
    ASSERT_TEST(heap_loader.check() != 0);
    ASSERT_TEST(heap_loader.create() == 0);

    shm_header& shm = *(shm_header*)shm_loader.shm_mnt_addr();
    shm_header& heap = *(shm_header*)heap_loader.shm_mnt_addr();
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000 * 10000, "shm push pop");
        for (u32 i = 0; i < 1000*10000; i++)
        {
            shm.push_back(i);
            volatile int a = shm.size();
            (void)a;
            shm.pop_back();
            
        }
    }

    
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000 * 10000, "heap push pop");
        for (u32 i = 0; i < 1000 * 10000; i++)
        {
            heap.push_back(i);
            volatile int a = shm.size();
            (void)a;
            heap.pop_back();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000 * 10000, "shm push pop");
        for (u32 i = 0; i < 1000 * 10000; i++)
        {
            shm.push_back(i);
            volatile int a = shm.size();
            (void)a;
            shm.pop_back();
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000 * 10000, "heap push pop");
        for (u32 i = 0; i < 1000 * 10000; i++)
        {
            heap.push_back(i);
            volatile int a = shm.size();
            (void)a;
            heap.pop_back();
        }
    }


    

    ASSERT_TEST(shm_loader.detach() == 0);
    ASSERT_TEST(shm_loader.destroy() == 0);
    ASSERT_TEST(heap_loader.detach() == 0);
    ASSERT_TEST(heap_loader.destroy() == 0);
    return 0;
}


int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    PROF_INIT("shm_loader");
    PROF_SET_OUTPUT(&FNLogFunc);

    ASSERT_TEST(shm_loader_base_test() == 0);
    ASSERT_TEST(shm_loader_stress_test() == 0);

#ifdef WIN32
#else
    ASSERT_TEST(shm_loader_unix_test() == 0);
#endif // WIN32

    (void)zshm_errno::str(zshm_errno::kSuccess);

    LogInfo() << "all test finish .";
    return 0;
}


