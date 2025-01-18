
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


        if (true)
        {
            void* pp[100] = { 0 };
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000, "cost");
            for (size_t i = 0; i < 10000; i++)
            {
                int n = (rand() % 64) + 8;
                if (pp[n] != nullptr)
                {
                    zmalloc::instance().free_slot(pp[n]);
                }
                pp[n] = zmalloc::instance().alloc_slot(1, 100, 1000);
                
                if (pp[n - 2] != nullptr)
                {
                    zmalloc::instance().free_slot(pp[n-2]);
                }
                pp[n - 2] = zmalloc::instance().alloc_slot(1, 100, 1000);
              
                if (pp[n + 4] != nullptr)
                {
                    zmalloc::instance().free_slot(pp[n + 4]);
                }
                pp[n + 4] = zmalloc::instance().alloc_slot(1, 100, 1000);
              
            }
            for (size_t i = 0; i < 100; i++)
            {
                if (pp[i] != nullptr)
                {
                    zmalloc::instance().free_slot(pp[i]);
                }
            }

            cost.set_cnt(zmalloc::instance().free_total_count_ + zmalloc::instance().req_total_count_);

        }


        LogDebug() << "zmalloc state log:";
        auto new_log = []() { return std::move(LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL)); };
        zmalloc::instance().debug_state_log(new_log);
        zmalloc::instance().debug_color_log(new_log, 0, (zmalloc::CHUNK_COLOR_MASK_WITH_LEVEL + 1) / 2);

    }



    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();


    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


