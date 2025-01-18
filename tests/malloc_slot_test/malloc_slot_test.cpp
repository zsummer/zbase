
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

#define ZMALLOC_OPEN_FENCE 1

#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zmalloc.h"

int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;


    if (true)
    {
        std::unique_ptr<zmalloc> zstate(new zmalloc());
        memset(zstate.get(), 0, sizeof(zmalloc));
        zstate->set_global(zstate.get());
        void* p = nullptr;

        p = zmalloc::instance().alloc_slot(1, 100, 100);
        ASSERT_TEST_NOLOG(p == NULL, "");
        ASSERT_TEST_NOLOG(zstate->req_total_count_ == 1, "");

        p = zmalloc::instance().alloc_slot(1, 100, 1000);
        ASSERT_TEST_NOLOG(p != NULL, "");
        ASSERT_TEST_NOLOG(zstate->req_total_count_ == 2, "");
        ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 1, "");

        ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(p) != 0, "");
        ASSERT_TEST_NOLOG(zstate->free_total_count_ == 1, "");
        ASSERT_TEST_NOLOG(zstate->free_block_count_ == 1, "");


        if (true)
        {
            void* pp[8];
            for (u32 i = 0; i < 8; i++)
            {
                pp[i] = zmalloc::instance().alloc_slot(1, 100, 1000);
                memset(pp[i], 0, 100);
                ASSERT_TEST_NOLOG(pp[i] != NULL, "");
            }
            for (u32 i = 0; i < 8; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[i]) != 0, "");
            }
            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 2, "");
            ASSERT_TEST_NOLOG(zstate->free_block_count_ == 2, "");
        }

        if (true)
        {
            void* pp[8];
            for (u32 i = 0; i < 8; i++)
            {
                pp[i] = zmalloc::instance().alloc_slot(2, 100, 1000);
                memset(pp[i], 0, 100);
                ASSERT_TEST_NOLOG(pp[i] != NULL, "");
            }
            for (u32 i = 0; i < 8; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[7-i]) != 0, "");
            }
            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 3, "");
            ASSERT_TEST_NOLOG(zstate->free_block_count_ == 3, "");
        }

        if (true)
        {
            void* pp[8];
            for (u32 i = 0; i < 8; i++)
            {
                pp[i] = zmalloc::instance().alloc_slot(2, 100, 1000);
                memset(pp[i], 0, 100);
                ASSERT_TEST_NOLOG(pp[i] != NULL, "");
            }
            for (u32 i = 0; i < 4; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[i]) != 0, "");
            }
            for (u32 i = 0; i < 3; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[7 - i]) != 0, "");
            }
            ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[4]) != 0, "");

            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 4, "");
            ASSERT_TEST_NOLOG(zstate->free_block_count_ == 4, "");
        }

        if (true)
        {
            void* pp[16];
            for (u32 i = 0; i < 16; i++)
            {
                pp[i] = zmalloc::instance().alloc_slot(1, 100, 1000);
                memset(pp[i], 0, 100);
                ASSERT_TEST_NOLOG(pp[i] != NULL, "");
            }
            for (u32 i = 0; i < 16; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[i]) != 0, "");
            }
            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 6, "");
            ASSERT_TEST_NOLOG(zstate->free_block_count_ == 6, "");
        }

        if (true)
        {
            void* pp[16];
            for (u32 i = 0; i < 16; i++)
            {
                pp[i] = zmalloc::instance().alloc_slot(3, 200, 2000);
                memset(pp[i], 0, 200);
                ASSERT_TEST_NOLOG(pp[i] != NULL, "");
            }
            for (u32 i = 0; i < 16; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[i]) != 0, "");
            }
            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 8, "");
            ASSERT_TEST_NOLOG(zstate->free_block_count_ == 8, "");
        }

    }


    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();


    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


