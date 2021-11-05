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

typedef char s8;
typedef unsigned char u8;
typedef short int s16;
typedef unsigned short int u16;
typedef int s32;
typedef unsigned int u32;
typedef long long s64;
typedef unsigned long long u64;
typedef unsigned long pointer;
typedef float f32;

#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zprof.h"
#include "zcontain_stress.h"
#include "zlist_stress.h"
#include "zcontain_test.h"
#include "zmalloc_test.h"


int main(int argc, char *argv[])
{
    PROF_INIT("inner prof");
    PROF_DEFINE_AUTO_SINGLE_RECORD(delta, 1, PROF_LEVEL_NORMAL, "self use mem in main func begin and exit");
    PROF_REGISTER_REFRESH_VM(delta.reg(), prof_get_mem_use());
    if (true)
    {
        PROF_DEFINE_AUTO_SINGLE_RECORD(guard, 1, PROF_LEVEL_NORMAL, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;
    //zmalloc_test();
    //sort_test();
    contiainer_base_test();
    contiainer_stress_test();
    

    PROF_UPDATE_MERGE();
    PROF_SERIALIZE_FN_LOG();


    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


