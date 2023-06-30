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
#include "zsymbols.h"

s32 zsymbols_test()
{
    
    return 0;
}



s32 zsymbols_clone_test()
{
    

    return 0;
}



s32 zsymbols_bench_test()
{
    
    return 0;
}

int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    PROF_INIT("inner prof");
    PROF_SET_OUTPUT(&FNLogFunc);

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zsymbols_test() == 0);
    ASSERT_TEST(zsymbols_clone_test() == 0);
    ASSERT_TEST(zsymbols_bench_test() == 0);

    LogInfo() << "all test finish .";
    return 0;
}


