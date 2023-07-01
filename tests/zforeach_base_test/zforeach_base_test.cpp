/*
* zsummer License
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
#include "zforeach.h"
#include "zlist.h"
#include "zarray.h"
#include "test_common.h"



class foreach_bound_case
{
public:
    inline s32 hook(const zforeach_impl::subframe& sub, u32 begin_id, u32 end_id, s64 now_ms)
    {
        ticks_++;
        cur_count_ = end_id - begin_id;
        total_count_ += cur_count_;
        return 0;
    }
    int ticks_ = 0;
    int cur_count_ = 0;
    int total_count_ = 0;
};



s32 foreach_bound_case_test(int max_count)
{
    zforeach<foreach_bound_case> empty_foreach;
    s32 ret = empty_foreach.init(0, 0, max_count, 10, 50);
    ASSERT_TEST_NOLOG(ret == 0);
    ret = empty_foreach.window_foreach(0, max_count, 0);
    ASSERT_TEST_NOLOG(ret == 0);
    if (max_count >= 1)
    {
        ASSERT_TEST_NOLOG(empty_foreach.foreach_inst_.cur_count_ > 0);
    }



    ret = empty_foreach.window_foreach(0, max_count, 0);
    ASSERT_TEST_NOLOG(ret == 0);
    if (max_count >= 2)
    {
        ASSERT_TEST_NOLOG(empty_foreach.foreach_inst_.cur_count_ > 0);
    }


    ret = empty_foreach.window_foreach(0, max_count, 0);
    ASSERT_TEST_NOLOG(ret == 0);
    if (max_count >= 5)
    {
        ASSERT_TEST_NOLOG(empty_foreach.foreach_inst_.cur_count_ > 0);
    }


    ret = empty_foreach.window_foreach(0, max_count, 0);
    ASSERT_TEST_NOLOG(ret == 0);
    if (max_count >= 7)
    {
        ASSERT_TEST_NOLOG(empty_foreach.foreach_inst_.cur_count_ > 0);
    }
    if (max_count == 7)
    {
        ASSERT_TEST_NOLOG(empty_foreach.foreach_inst_.cur_count_ == 1);
    }

    ret = empty_foreach.window_foreach(0, max_count, 0);
    ASSERT_TEST_NOLOG(ret == 0);
    if (max_count >= 9)
    {
        ASSERT_TEST_NOLOG(empty_foreach.foreach_inst_.cur_count_ > 0, " max:", max_count);
    }
    

    ASSERT_TEST(empty_foreach.foreach_inst_.ticks_ == 5);
    ASSERT_TEST(empty_foreach.foreach_inst_.total_count_ == max_count, max_count);
    return 0;
}

s32 foreach_scala_bound_case_test(int max_count)
{
    zforeach<foreach_bound_case> empty_foreach;
    s32 ret = empty_foreach.init(0, 0, max_count, 10, 50);
    ASSERT_TEST_NOLOG(ret == 0);
    ret = empty_foreach.window_foreach(0, 10, 0);
    ASSERT_TEST_NOLOG(ret == 0);
    ret = empty_foreach.window_foreach(0, 20, 0);
    ASSERT_TEST_NOLOG(ret == 0);
    ret = empty_foreach.window_foreach(0, 80, 0);
    ASSERT_TEST_NOLOG(ret == 0);
    ret = empty_foreach.window_foreach(0, 1000, 0);
    ASSERT_TEST_NOLOG(ret == 0);
    ret = empty_foreach.window_foreach(0, 1000, 0);
    ASSERT_TEST_NOLOG(ret == 0);

    ASSERT_TEST(empty_foreach.foreach_inst_.ticks_ == 5);
    ASSERT_TEST(empty_foreach.foreach_inst_.total_count_ == max_count, max_count);
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
    ASSERT_TEST(foreach_bound_case_test(100) == 0);
    ASSERT_TEST(foreach_bound_case_test(99) == 0);
    ASSERT_TEST(foreach_bound_case_test(1) == 0);
    ASSERT_TEST(foreach_bound_case_test(0) == 0);
    ASSERT_TEST(foreach_bound_case_test(4) == 0);
    ASSERT_TEST(foreach_bound_case_test(7) == 0);

    ASSERT_TEST(foreach_scala_bound_case_test(100) == 0);
    ASSERT_TEST(foreach_scala_bound_case_test(0) == 0);
    ASSERT_TEST(foreach_scala_bound_case_test(1) == 0);
    ASSERT_TEST(foreach_scala_bound_case_test(4) == 0);
    ASSERT_TEST(foreach_scala_bound_case_test(7) == 0);
    ASSERT_TEST(foreach_scala_bound_case_test(49) == 0);
    ASSERT_TEST(foreach_scala_bound_case_test(999) == 0);


    LogInfo() << "all test finish .";
    return 0;
}

