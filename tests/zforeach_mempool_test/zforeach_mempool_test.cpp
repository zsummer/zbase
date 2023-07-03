
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zprof.h"
#include "zforeach.h"
#include "zlist.h"
#include "zarray.h"
#include "test_common.h"
#include "zmem_pool.h"





static constexpr s32 kMaxForeachs = 10;



static s32 foreach_insts_;
static s32 base_frame_len_;

static s64 total_tick_count_;
static s32 foreach_user_frame_len_[kMaxForeachs];

static s32 foreach_real_frame_len_[kMaxForeachs];

static s64 foreach_tick_count_[kMaxForeachs];


static s32 has_error_ = 0;
struct TickRecord
{
    s64 last_;
    s64 tick_count_;
};


struct Object
{
    TickRecord record_[kMaxForeachs];
    s32 OnTick(u64 tick_id, s64 now_ms)
    {
        if (record_[tick_id].last_ != 0)
        {
            s64 duration = now_ms - record_[tick_id].last_;
            if (duration != foreach_real_frame_len_[tick_id])
            {
                has_error_++;
                ASSERT_TEST_NOLOG(has_error_ == 0);
                return has_error_;
            }
        }
        record_[tick_id].last_ = now_ms;
        record_[tick_id].tick_count_++;
        return 0;
    }
};



static zmem_pool object_pool_;



class TestForeach
{
public:
    inline s32 hook(const zforeach_impl::subframe& sub, u32 begin_id, u32 end_id, s64 now_ms)
    {
        for (u32 i = begin_id; i < end_id; i++)
        {
            Object* obj = object_pool_.user_data<Object>(i);
            obj->OnTick(sub.userkey_, now_ms);
            if (has_error_ != 0)
            {
                return has_error_;
            }
        }
        return has_error_;
    }

private:

};



s32 base_test()
{
    constexpr static s32 kForeachInsts = 5;
    constexpr static s32 kTickCount = 800 * 90;
    constexpr static s32 kObjects = 9999;
    constexpr static s32 kBaseFrameLen = 10;

    foreach_insts_ = kForeachInsts;
    base_frame_len_ = kBaseFrameLen;
    foreach_user_frame_len_[0] = 10;
    foreach_user_frame_len_[1] = 50;
    foreach_user_frame_len_[2] = 800;
    foreach_user_frame_len_[3] = 93;
    foreach_user_frame_len_[4] = 49;

    foreach_real_frame_len_[0] = 10;
    foreach_real_frame_len_[1] = 50;
    foreach_real_frame_len_[2] = 800;
    foreach_real_frame_len_[3] = 100;
    foreach_real_frame_len_[4] = 50;

    total_tick_count_ = kTickCount;

    foreach_tick_count_[0] = total_tick_count_ / (foreach_real_frame_len_[0]/ base_frame_len_);
    foreach_tick_count_[1] = total_tick_count_ / (foreach_real_frame_len_[1]/ base_frame_len_);
    foreach_tick_count_[2] = total_tick_count_ / (foreach_real_frame_len_[2]/ base_frame_len_);
    foreach_tick_count_[3] = total_tick_count_ / (foreach_real_frame_len_[3]/ base_frame_len_);
    foreach_tick_count_[4] = total_tick_count_ / (foreach_real_frame_len_[4]/ base_frame_len_);
    
    s64 space_size = zmem_pool::calculate_space_size(sizeof(Object), kObjects);
    s32 ret = object_pool_.init(sizeof(Object), kObjects, new char[space_size], space_size);
    ASSERT_TEST(ret == 0);
    memset(object_pool_.space_addr_, 0, object_pool_.space_size_);

    zforeach<TestForeach> inst[kForeachInsts];
    for (s32 i = 0; i < foreach_insts_; i++)
    {
        s32 ret = inst[i].init(i, 0, kObjects, base_frame_len_, foreach_user_frame_len_[i]);
        ASSERT_TEST(ret == 0);
    }
    
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, kTickCount, "base frame ticks(9999obj)");
        s64 now_ms = 10000;
        for (s32 i = 0; i < total_tick_count_; i++)
        {
            for (s32 inst_id = 0; inst_id < foreach_insts_; inst_id++)
            {
                s32 ret = inst[inst_id].window_foreach(0, kObjects, now_ms);
                ASSERT_TEST_NOLOG(ret == 0);
                ASSERT_TEST_NOLOG(has_error_ == 0);
            }
            now_ms += base_frame_len_;
        }
    }

    ASSERT_TEST(has_error_ == 0);

    s64 all_ticks = 0;
    for (s32 i = 0; i < kObjects; i++)
    {
        for (s32 j = 0; j < foreach_insts_; j++)
        {
            s64 real = object_pool_.at<Object>(i).record_[j].tick_count_;
            s64 expect = foreach_tick_count_[j];
            all_ticks += real;
            ASSERT_TEST_NOLOG(real == expect, "real:", real, ", expect:", expect);
        }
        
    }
    LogInfo() << " all ticks:" << all_ticks;
    delete object_pool_.space_addr_;
    return 0;
}



int main(int argc, char *argv[])
{
    PROF_INIT("inner prof");
    PROF_SET_OUTPUT(&FNLogFunc);

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";
    
    ASSERT_TEST(base_test() == 0);


    LogInfo() << "all test finish ." ;
    return 0;
}


