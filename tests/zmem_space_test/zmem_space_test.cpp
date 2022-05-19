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
#include "zforeach.h"
#include "zlist.h"
#include "zarray.h"
#include "zmem_space.h"
using namespace zsummer;




int main(int argc, char *argv[])
{
    PROF_INIT("inner prof");
    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";
    if (true)
    {
        zstatic_trivial_pool<int, 100> ds;
        zarray<int*, 100> store;
        for (int loop = 0; loop < 2; loop++)
        {
            for (int i = 0; i < 100; i++)
            {
                int* p = ds.exploit();
                if (p == NULL)
                {
                    LogError() << "has error";
                    return -1;
                }
                *p = i;
                store.push_back(p);
            }
            if (!ds.full())
            {
                LogError() << "has error";
                return -2;
            }
            if (ds.exploit() != NULL)
            {
                LogError() << "has error";
                return -3;
            }
            for (auto p : store)
            {
                ds.back(p);
            }
            store.clear();
            if (ds.used_count() != 0)
            {
                LogError() << "has error";
                return -4;
            }
        }
        

    }
    


    LogInfo() << "all test finish .";
    return 0;
}


