
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

        p = zmalloc::instance().alloc_slot(1, 100, 900);
        ASSERT_TEST_NOLOG(p != NULL, "");
        ASSERT_TEST_NOLOG(zstate->req_total_count_ == 2, "");
        ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 1, "");

        ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(p) != 0, "");
        ASSERT_TEST_NOLOG(zstate->free_total_count_ == 1, "");
        ASSERT_TEST_NOLOG(zstate->free_block_count_ == 1, "");


        if (true)
        {
            void* pp[7];
            for (u32 i = 0; i < 7; i++)
            {
                pp[i] = zmalloc::instance().alloc_slot(1, 100, 900);
                memset(pp[i], 0, 100);
                ASSERT_TEST_NOLOG(pp[i] != NULL, "");
            }
            for (u32 i = 0; i < 7; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[i]) != 0, "");
            }
            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 2, "");
            ASSERT_TEST_NOLOG(zstate->free_block_count_ == 2, "");
        }

        if (true)
        {
            void* pp[7];
            for (u32 i = 0; i < 7; i++)
            {
                pp[i] = zmalloc::instance().alloc_slot(2, 100, 900);
                memset(pp[i], 0, 100);
                ASSERT_TEST_NOLOG(pp[i] != NULL, "");
            }
            for (u32 i = 0; i < 7; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[6-i]) != 0, "");
            }
            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 3, "");
            ASSERT_TEST_NOLOG(zstate->free_block_count_ == 3, "");
        }

        if (true)
        {
            void* pp[7];
            for (u32 i = 0; i < 7; i++)
            {
                pp[i] = zmalloc::instance().alloc_slot(2, 100, 900);
                memset(pp[i], 0, 100);
                ASSERT_TEST_NOLOG(pp[i] != NULL, "");
            }
            for (u32 i = 0; i < 3; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[i]) != 0, "");
            }
            for (u32 i = 0; i < 3; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[6 - i]) != 0, "");
            }
            ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[3]) != 0, "");

            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 4, "");
            ASSERT_TEST_NOLOG(zstate->free_block_count_ == 4, "");
        }

        if (true)
        {
            void* pp[14];
            for (u32 i = 0; i < 14; i++)
            {
                pp[i] = zmalloc::instance().alloc_slot(1, 100, 900);
                memset(pp[i], 0, 100);
                ASSERT_TEST_NOLOG(pp[i] != NULL, "");
            }
            for (u32 i = 0; i < 14; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[i]) != 0, "");
            }
            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 6, "");
            ASSERT_TEST_NOLOG(zstate->free_block_count_ == 6, "");
        }

        if (true)
        {
            void* pp[14];
            for (u32 i = 0; i < 14; i++)
            {
                pp[i] = zmalloc::instance().alloc_slot(3, 200, 1500);
                memset(pp[i], 0, 200);
                ASSERT_TEST_NOLOG(pp[i] != NULL, "");
            }
            for (u32 i = 0; i < 14; i++)
            {
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[i]) != 0, "");
            }
            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == 8, "");
            ASSERT_TEST_NOLOG(zstate->free_block_count_ == 8, "");
        }


        if (true)
        {

            memset(zstate.get(), 0, sizeof(zmalloc));
            zstate->set_global(zstate.get());


            void* pp[14];
            for (u32 i = 0; i < zmalloc::kSlotBinMapSize; i++)
            {
                pp[0] = zmalloc::instance().alloc_slot(i, 200, zmalloc_align_default_value(200) + zmalloc::kChunkPaddingSize + sizeof(zmalloc::free_chunk_type) * 2 + sizeof(zmalloc::block_type) +i *5);
                ASSERT_TEST_NOLOG(pp[0] != NULL, i);
                memset(pp[0], 0, 200);
                ASSERT_TEST_NOLOG(zmalloc::instance().free_slot(pp[0]) != 0, "");
            }

            ASSERT_TEST_NOLOG(zstate->alloc_block_count_ == zstate->free_block_count_, "");
 
        }
        LogDebug() << "zmalloc state log:";
        auto new_log = []() { return std::move(LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL)); };
        zmalloc::instance().debug_state_log(new_log);
        zmalloc::instance().debug_color_log(new_log, 0, (zmalloc::kChunkColorMaskWithLevel + 1) / 2);


    }




    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();


    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


