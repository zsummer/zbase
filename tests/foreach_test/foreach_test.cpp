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
using namespace zsummer;
s32 tick1000(u64 key, u64 id, s64 now_ms)
{
    LogDebug() << "tick1000: id:" << id ;
    return 0;
}
s32 tick5000(u64 key, u64 id, s64 now_ms)
{
    LogDebug() << "tick5000: id:" << id;
    return 0;
}


enum TRIGGER_TYPE
{
    TT_10MS,
    TT_50_1MS,
    TT_50_2MS,
    TT_800MS,
    TT_MAX,
};
s64 g_trigger_interval[TT_MAX] = {
    10, 50, 50, 800
};


struct trigger_info
{
    s64 total_delta;
    s64 total_count;
    s64 dutaion[5];
    s64 first_time;
    s64 last_time;
};

struct node
{
    bool valid;
    trigger_info triggers[TT_MAX];
};

void trigger_check_init(node& node, s64 now_ms)
{
    memset(&node, 0, sizeof(node));
    node.valid = true;
    for (size_t j = 0; j < TT_MAX; j++)
    {
        node.triggers[j].dutaion[0] = g_trigger_interval[j];
        node.triggers[j].dutaion[1] = g_trigger_interval[j];
        node.triggers[j].dutaion[2] = g_trigger_interval[j];
        node.triggers[j].dutaion[3] = g_trigger_interval[j];
        node.triggers[j].dutaion[4] = g_trigger_interval[j];
        node.triggers[j].first_time = now_ms;
        node.triggers[j].last_time = now_ms;
    }
}
static const u32 node_count = 10000;
using obj_pool = zarray<node, node_count>;

static obj_pool g_pool;

template<TRIGGER_TYPE TT, u32 INTERVAL>
static inline s32 trigger_check(u64 key, u64 id, s64 now_ms)
{
    obj_pool* p = (obj_pool*)key;
    if (!p->at(id).valid)
    {
        return 0;
    }
    trigger_info& info = p->at(id).triggers[TT];
    s64 duration = now_ms - info.last_time;
    if (true)
    {
        s64 total = info.dutaion[0] + info.dutaion[1] + info.dutaion[2] + info.dutaion[3] + info.dutaion[4] + duration;
        if (abs(total / 6 - INTERVAL) > INTERVAL * 10 / 2)
        {
            s64 s = total / 6 - INTERVAL;
            s64 m = INTERVAL * 10 / 2;
            LogError() << "id:" << id<<" " << INTERVAL << "ms shaking:" << s <<" over max:" << m;
        }
        info.dutaion[4] = info.dutaion[3];
        info.dutaion[3] = info.dutaion[2];
        info.dutaion[2] = info.dutaion[1];
        info.dutaion[1] = info.dutaion[0];
        info.dutaion[0] = duration;
    }
    info.last_time = now_ms;
    info.total_count++;
    info.total_delta += duration;


    if (true)
    {
        if (abs(info.last_time - info.first_time - info.total_delta) > INTERVAL * 2)
        {
            LogError() << "leak";
        }
        if (abs(info.total_delta / INTERVAL - info.total_count) > 2)
        {
            LogError() << "leak";
        }
    }
    return 0;
}











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
    volatile double cycles = 0.0f;
    if (true)
    {
        g_pool.clear();
        zforeach fe[TT_MAX];
        s32 ret = fe[TT_10MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_10MS, 10>, 10, 10);
        ret |= fe[TT_50_1MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_1MS, 50>, 10, 50);
        ret |= fe[TT_50_2MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_2MS, 50>, 10, 50);
        ret |= fe[TT_800MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_800MS, 800>, 10, 800);
        if (ret != 0)
        {
            LogError() << "";
            return -2;
        }
        s64 now_ms = 1000000;
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000, "1000 auto inc");
        for (u32 i = 0; i < 1000; i++)
        {
            now_ms += 10;
            for (size_t tt = 0; tt < TT_MAX; tt++)
            {
                fe[tt].window_tick(0, g_pool.size(), now_ms);
            }
            if (!g_pool.full() && rand() % 2 == 0)
            {
                node n;
                trigger_check_init(n, now_ms);
                g_pool.push_back(n);
            }
        }
    }

    if (true)
    {
        g_pool.clear();
        zforeach fe[TT_MAX];
        s32 ret = fe[TT_10MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_10MS, 10>, 10, 10);
        ret |= fe[TT_50_1MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_1MS, 50>, 10, 50);
        ret |= fe[TT_50_2MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_2MS, 50>, 10, 50);
        ret |= fe[TT_800MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_800MS, 800>, 10, 800);
        if (ret != 0)
        {
            LogError() << "";
            return -2;
        }
        s64 now_ms = 1000;
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000, "1000 full");
        for (u32 i = 0; i < 1000; i++)
        {
            now_ms += 10;
            for (size_t tt = 0; tt < TT_MAX; tt++)
            {
                fe[tt].window_tick(0, g_pool.size(), now_ms);
            }
            while (!g_pool.full())
            {
                node n;
                trigger_check_init(n, now_ms);
                g_pool.push_back(n);
            }
        }
    }

    if (true)
    {
        g_pool.clear();
        zforeach fe[TT_MAX];
        s32 ret = fe[TT_10MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_10MS, 10>, 10, 10);
        ret |= fe[TT_50_1MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_1MS, 50>, 10, 50);
        ret |= fe[TT_50_2MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_2MS, 50>, 10, 50);
        ret |= fe[TT_800MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_800MS, 800>, 10, 800);
        if (ret != 0)
        {
            LogError() << "";
            return -2;
        }
        s64 now_ms = 1000000;
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 100000, "100000 TT_10MS");
        for (u32 i = 0; i < 100000; i++)
        {
            now_ms += 10;
            for (size_t tt = 0; tt < TT_10MS + 1; tt++)
            {
                fe[tt].window_tick(0, g_pool.size(), now_ms);
            }
            if (!g_pool.full() && rand() % 2 == 0)
            {
                node n;
                trigger_check_init(n, now_ms);
                g_pool.push_back(n);
            }
        }
    }
    if (true)
    {
        g_pool.clear();
        zforeach fe[TT_MAX];
        s32 ret = fe[TT_10MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_10MS, 10>, 10, 10);
        ret |= fe[TT_50_1MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_1MS, 50>, 10, 50);
        ret |= fe[TT_50_2MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_2MS, 50>, 10, 50);
        ret |= fe[TT_800MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_800MS, 800>, 10, 800);
        if (ret != 0)
        {
            LogError() << "";
            return -2;
        }
        s64 now_ms = 1000000;
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 100000, "100000 TT_50_1MS");
        for (u32 i = 0; i < 100000; i++)
        {
            now_ms += 10;
            for (size_t tt = TT_50_1MS; tt < TT_50_1MS + 1; tt++)
            {
                fe[tt].window_tick(0, g_pool.size(), now_ms);
            }
            if (!g_pool.full() && rand() % 2 == 0)
            {
                node n;
                trigger_check_init(n, now_ms);
                g_pool.push_back(n);
            }
        }
    }
    if (true)
    {
        g_pool.clear();
        zforeach fe[TT_MAX];
        s32 ret = fe[TT_10MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_10MS, 10>, 10, 10);
        ret |= fe[TT_50_1MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_1MS, 50>, 10, 50);
        ret |= fe[TT_50_2MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_2MS, 50>, 10, 50);
        ret |= fe[TT_800MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_800MS, 800>, 10, 800);
        if (ret != 0)
        {
            LogError() << "";
            return -2;
        }
        s64 now_ms = 1000000;
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 100000, "100000 TT_800MS");
        for (u32 i = 0; i < 100000; i++)
        {
            now_ms += 10;
            for (size_t tt = TT_800MS; tt < TT_800MS + 1; tt++)
            {
                fe[tt].window_tick(0, g_pool.size(), now_ms);
            }
            if (!g_pool.full() && rand() % 2 == 0)
            {
                node n;
                trigger_check_init(n, now_ms);
                g_pool.push_back(n);
            }
        }
    }

    if (true)
    {
        g_pool.clear();
        zforeach fe[TT_MAX];
        s32 ret = fe[TT_10MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_10MS, 10>, 10, 10);
        ret |= fe[TT_50_1MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_1MS, 50>, 10, 50);
        ret |= fe[TT_50_2MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_50_2MS, 50>, 10, 50);
        ret |= fe[TT_800MS].init((u64)&g_pool, 0, g_pool.max_size(), trigger_check<TT_800MS, 800>, 10, 800);
        if (ret != 0)
        {
            LogError() << "";
            return -2;
        }
        s64 now_ms = 1000000;
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 100000, "100000");
        for (u32 i = 0; i < 100000; i++)
        {
            now_ms += 10;
            for (size_t tt = 0; tt < TT_MAX; tt++)
            {
                fe[tt].window_tick(0, g_pool.size(), now_ms);
            }
            if (!g_pool.full() && rand() % 2 == 0)
            {
                node n;
                trigger_check_init(n, now_ms);
                g_pool.push_back(n);
            }
        }
    }




    if (true)
    {
        zforeach t[2];
        s32 ret = t[0].init(0, 100, 110, &tick1000, 100, 1000);
        ret |= t[1].init(0, 100, 110, &tick5000, 100, 5000);
        if (ret != 0)
        {
            LogError() << "init error:" << ret;
            return -2;
        }

        for (size_t i = 0; i < 100; i++)
        {
            t[0].window_tick(100, 110, i);
            t[1].window_tick(100, 110, i);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    }






    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


