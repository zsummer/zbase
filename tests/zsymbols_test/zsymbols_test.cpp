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
    zsymbols_fast_static<100> symbols;
    for (s32 i = 0; i < 10; i++)
    {
        char buf[10];
        sprintf(buf, "%d", i);
        s32 id = symbols.add(buf, 0, true);
        ASSERT_TEST_NOLOG(id != zsymbols_fast_static<100>::invalid_symbols_id);
        const char* name = symbols.at(id);
        ASSERT_TEST_NOLOG(strcmp(name, buf) == 0);
    }

    s32 test_id = symbols.add("test", 0, true);
    ASSERT_TEST(test_id != zsymbols_fast_static<100>::invalid_symbols_id);
    ASSERT_TEST(test_id == symbols.add("test", 0, true));

    for (s32 i = 0; i < 50; i++)
    {
        symbols.add("test", 0, false);
    }
    ASSERT_TEST(symbols.add("test", 0, false) == zsymbols_fast_static<100>::invalid_symbols_id);
    ASSERT_TEST(symbols.add("test", 0, true) != zsymbols_fast_static<100>::invalid_symbols_id);

    zsymbols_fast_static<100> clone_test;
    s32 ret = clone_test.clone_from(symbols);
    ASSERT_TEST(ret == 0);
    ASSERT_TEST(clone_test.add("test", 0, false) == zsymbols_fast_static<100>::invalid_symbols_id);
    ASSERT_TEST(clone_test.add("test", 0, true) != zsymbols_fast_static<100>::invalid_symbols_id);

    return 0;
}

s32 zsymbols_solid_test()
{
    zsymbols_solid_static<100> symbols;
    for (s32 i = 0; i < 10; i++)
    {
        char buf[10];
        sprintf(buf, "%d", i);
        s32 id = symbols.add(buf, 0, true);
        ASSERT_TEST_NOLOG(id != zsymbols_solid_static<100>::invalid_symbols_id);
        const char* name = symbols.at(id);
        ASSERT_TEST_NOLOG(strcmp(name, buf) == 0);
    }
    s32 test_id = symbols.add("test", 0, true);
    ASSERT_TEST(test_id != zsymbols_solid_static<100>::invalid_symbols_id);
    ASSERT_TEST(test_id == symbols.add("test", 0, true));

    for (s32 i = 0; i < 50; i++)
    {
        symbols.add("test", 0, false);
    }
    ASSERT_TEST(symbols.add("test", 0, false) == zsymbols_solid_static<100>::invalid_symbols_id);
    ASSERT_TEST(symbols.add("test", 0, true) != zsymbols_solid_static<100>::invalid_symbols_id);

    zsymbols_solid_static<100> clone_test;
    s32 ret = clone_test.clone_from(symbols);
    ASSERT_TEST(ret == 0);
    ASSERT_TEST(clone_test.add("test", 0, false) == zsymbols_solid_static<100>::invalid_symbols_id);
    ASSERT_TEST(clone_test.add("test", 0, true) != zsymbols_solid_static<100>::invalid_symbols_id);

    return 0;
}





s32 zsymbols_bench_test()
{
    static constexpr s32 symbols = 10000;
    zsymbols_static<symbols * 20>* base = new zsymbols_static<symbols * 20>;
    zsymbols_static<symbols * 20>* base_no_reuse = new zsymbols_static<symbols * 20>;
    zarray<s32, symbols>* rands = new zarray<s32, symbols>;
    for (s32 i = 0; i < symbols; i++)
    {
        char buf[50];
        rands->push_back(rand());
        sprintf(buf, "%d", rands->back());
        s32 ret = base->add(buf, 0, true);
        ASSERT_TEST_NOLOG(ret != zsymbols::invalid_symbols_id);
    }

    zsymbols_fast_static<symbols * 10>* fast = new zsymbols_fast_static<symbols * 10>;
    zsymbols_solid_static<symbols * 10>* solid = new zsymbols_solid_static<symbols * 10>;
    
    if (true)
    {
        //add  
        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "noreuse");
            for (auto id : *rands)
            {
                s32 symbol_id = base_no_reuse->add(base->at(id), 0, false);
                ASSERT_TEST_NOLOG(symbol_id != zsymbols_static<symbols * 20>::invalid_symbols_id);
            }
        }
        LogInfo() << "used bytes:" << base_no_reuse->exploit_;
    }


    if (true)
    {
        //add  
        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "fast");
            for (auto id : *rands)
            {
                s32 symbol_id = fast->add(base->at(id), 0, true);
                ASSERT_TEST_NOLOG(symbol_id != zsymbols::invalid_symbols_id);
            }
        }
        LogInfo() << "used bytes:" << fast->exploit_;
    }

    if (true)
    {
        //add  
        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "solid");
            for (auto id : *rands)
            {
                s32 symbol_id = solid->add(base->at(id), 0, true);
                ASSERT_TEST_NOLOG(symbol_id != zsymbols_solid_static<symbols * 10>::invalid_symbols_id);
            }
        }
        LogInfo() << "used bytes:" << solid->exploit_;
    }

    delete base;
    delete base_no_reuse;
    delete rands;
    delete fast;
    delete solid;

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
    ASSERT_TEST(zsymbols_solid_test() == 0);
    ASSERT_TEST(zsymbols_bench_test() == 0);

    LogInfo() << "all test finish .";
    return 0;
}


