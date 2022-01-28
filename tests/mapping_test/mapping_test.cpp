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
#include "zmapping.h"


using namespace zsummer;

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

    if (true)
    {
        zmapping res;
        res.mapping_res("./make.sh");
        if (!res.is_mapped())
        {
            LogError() << "not mapped!";
            return 1;
        }

        int a = 0;
        for (size_t i = 0; i < res.data_len(); i++)
        {
            a += (unsigned char)res.data()[i];
        }
        LogInfo() << " auto test ./make.sh success";
    }
    if (argc > 1)
    {

        zmapping res;
        s32 ret = res.mapping_res(argv[1]);
        if (ret != 0)
        {
            LogError() << "mapping [" << argv[1] << "] has error";
            return 2;
        }
        int a = 0;
        u64 read_bytes = 0;
        for (size_t i = 0; i < res.data_len(); i++)
        {
            a += (unsigned char)res.data()[i];
            read_bytes++;
            if (read_bytes % (500*1024*1024) == 0)
            {
                LogInfo() << "now read bytes:" << read_bytes / 1024.0 / 1024.0 << "M. please putchar to continue...";
                getchar();
            }
        }
        LogInfo() << "now read bytes:" << read_bytes / 1024.0 / 1024.0 << "M. please putchar to continue...";
        getchar();
    }
    LogInfo() << "please putchar to continue...";
    getchar();
    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


