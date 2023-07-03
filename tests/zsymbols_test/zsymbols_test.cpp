
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
        ASSERT_TEST_NOLOG((s32)strlen(name) == symbols.len(id));
    }

    s32 test_id = symbols.add("test", 0, true);
    ASSERT_TEST(test_id != zsymbols_fast_static<100>::invalid_symbols_id);
    ASSERT_TEST(test_id == symbols.add("test", 0, true));
    ASSERT_TEST((s32)strlen("test") == symbols.len(test_id));

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
        s32 rand_val = rand()/ (rand()%1000 + 1);
        sprintf(buf, "%d", rand_val);
        s32 id = base->add(buf, 0, false);
        rands->push_back(id);
        ASSERT_TEST_NOLOG(id != zsymbols::invalid_symbols_id);
    }
    LogInfo() << "test rand count:" << rands->size() <<", base string total size:" << base->exploit_;
    zsymbols_fast_static<symbols * 20>* fast = new zsymbols_fast_static<symbols * 20>;
    zarray<s32, symbols>* fast_ids = new zarray<s32, symbols>;
    zsymbols_solid_static<symbols * 20>* solid = new zsymbols_solid_static<symbols * 20>;
    zarray<s32, symbols>* solid_ids = new zarray<s32, symbols>;
    if (true)
    {
        //add  
        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "no reuse add");
            for (auto id : *rands)
            {
                s32 symbol_id = base_no_reuse->add(base->at(id), 0, false);
                ASSERT_TEST_NOLOG(symbol_id != zsymbols_static<symbols * 20>::invalid_symbols_id);
            }
        }
        
    }


    if (true)
    {
        //add  
        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "fast reuse add");
            for (auto id : *rands)
            {
                s32 symbol_id = fast->add(base->at(id), 0, true);
                ASSERT_TEST_NOLOG(symbol_id != zsymbols::invalid_symbols_id);
                fast_ids->push_back(symbol_id);
            }
        }
        
    }

    if (true)
    {
        //add  
        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "solid reuse add");
            for (auto id : *rands)
            {
                s32 symbol_id = solid->add(base->at(id), 0, true);
                ASSERT_TEST_NOLOG(symbol_id != zsymbols_solid_static<symbols * 20>::invalid_symbols_id);
                solid_ids->push_back(symbol_id);
            }
        }
        
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "fast at");
        char buf[50];
        volatile u32 se = 0;
        for (auto id : *fast_ids)
        {
            const char* p = fast->at(id);
            memcpy(buf, p, fast->len(id));
            se += (u32)buf[0];
        }
    }
    if (true)
    {

        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "solid at");
        char buf[50];
        volatile u32 se = 0;
        for (auto id : *solid_ids)
        {
            const char* p = solid->at(id);
            memcpy(buf, p, strlen(p)+1);
            se += (u32)buf[0];
        }
    }
    LogInfo() << "base_no_reuse used bytes:" << base_no_reuse->exploit_;
    LogInfo() << "fast used bytes:" << fast->exploit_;
    LogInfo() << "solid used bytes:" << solid->exploit_;
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


