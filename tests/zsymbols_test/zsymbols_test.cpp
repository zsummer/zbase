
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
    constexpr s32 HEAD_SIZE = zsymbols::HEAD_SIZE;
    if (true)
    {
        zsymbols symbols;
        symbols.reset();
        ASSERT_TEST(symbols.clear() == 0, "");
        ASSERT_TEST(symbols.add("", 0, false) <= 0, "");
        ASSERT_TEST(strcmp(symbols.at(-1), "") == 0, "");
        ASSERT_TEST(strcmp(symbols.at(0), "") == 0, "");
        ASSERT_TEST(symbols.exploit_ == 0, "");

        symbols.attach(nullptr, HEAD_SIZE);
        ASSERT_TEST(symbols.clear() == 0, "");
        ASSERT_TEST(symbols.add("", 0, false) <= 0, "");
        ASSERT_TEST(strcmp(symbols.at(-1), "") == 0, "");
        ASSERT_TEST(strcmp(symbols.at(0), "") == 0, "");
        ASSERT_TEST(symbols.exploit_ == 0, "");

        symbols.attach((char*)0x5555, HEAD_SIZE);
        ASSERT_TEST(symbols.clear() == 0, "");
        ASSERT_TEST(symbols.add("", 0, false) <= 0, "");
        ASSERT_TEST(strcmp(symbols.at(-1), "") == 0, "");
        ASSERT_TEST(strcmp(symbols.at(0), "") == 0, "");
        ASSERT_TEST(symbols.exploit_ == 0, "");

        ASSERT_TEST(symbols.attach((char*)0x55, 4, 4) == 0, "");
        ASSERT_TEST(symbols.attach(nullptr, 4, 4) != 0, "");
        ASSERT_TEST(symbols.attach((char*)0x55, 4, 5) != 0, "");
    }

    if (true)
    {
        zsymbols_static<HEAD_SIZE+1> symbols;
        ASSERT_TEST(symbols.add("", 0, false) > 0, "");
        ASSERT_TEST(symbols.add("", 0, false) <= 0, "");
        ASSERT_TEST(symbols.add("", 0, true) == HEAD_SIZE, "");
    }

    if (true)
    {
        zsymbols_static<HEAD_SIZE+3> symbols;
        ASSERT_TEST(symbols.add("1", 0, false) == HEAD_SIZE, "");
        ASSERT_TEST(symbols.add("1", 0, false) <= 0, "");
        ASSERT_TEST(symbols.add("1", 0, true) == HEAD_SIZE, "");
        symbols.clear();
        ASSERT_TEST(symbols.add("1322", 1, false) == HEAD_SIZE, "");
        ASSERT_TEST(symbols.zsymbols::space_[HEAD_SIZE] == '1', "");
        ASSERT_TEST(symbols.zsymbols::space_[HEAD_SIZE+1] == '\0', "");
        ASSERT_TEST(symbols.len(HEAD_SIZE) == 1, "");
        ASSERT_TEST(strcmp(symbols.at(HEAD_SIZE), "1") ==0, "");
    }
    if (true)
    {
        zsymbols  symbols1;
        symbols1.attach((char*)0x55, 55, 55);

        zsymbols  symbols2;
        symbols2.attach((char*)0x11, 11, 0);


        ASSERT_TEST(symbols1.swap(symbols1) != 0, "");
        ASSERT_TEST(symbols1.swap(symbols2) == 0, "");

        ASSERT_TEST(symbols1.space_len_ == 11, "");

    }





    zsymbols_static<100> symbols;
    for (s32 i = 0; i < 10; i++)
    {
        char buf[10];
        sprintf(buf, "%d", i);
        s32 id = symbols.add(buf, 0, true);
        ASSERT_TEST_NOLOG(id > 0);
        const char* name = symbols.at(id);
        ASSERT_TEST_NOLOG(strcmp(name, buf) == 0);
        ASSERT_TEST_NOLOG((s32)strlen(name) == symbols.len(id));
    }

    s32 test_id = symbols.add("test", 0, true);
    ASSERT_TEST(test_id > 0);
    ASSERT_TEST(test_id == symbols.add("test", 0, true));
    ASSERT_TEST((s32)strlen("test") == symbols.len(test_id));

    for (s32 i = 0; i < 50; i++)
    {
        symbols.add("test", 0, false);
    }
    ASSERT_TEST(symbols.add("test", 0, false) < 0);
    ASSERT_TEST(symbols.add("test", 0, true) > 0);

    zsymbols_static<100> clone_test;
    s32 ret = clone_test.clone_from(symbols);
    ASSERT_TEST(ret == 0);
    ASSERT_TEST(clone_test.add("test", 0, false) < 0);
    ASSERT_TEST(clone_test.add("test", 0, true) > 0);



    ASSERT_TEST(zsymbols::readable_class_name<zsymbols>().find("zsymbols") != std::string::npos, zsymbols::readable_class_name<zsymbols>());



    return 0;
}



template<size_t SCOUNT, size_t SLEN>
s32 zsymbols_bench_test(std::array<s32, SCOUNT>* p1, std::array<s32, SLEN>* p2)
{
    static constexpr s32 symbols = SCOUNT;
    static constexpr s32 slen = SLEN;
    zsymbols_static<symbols * slen>* base = new zsymbols_static<symbols * slen>;
    zarray<s32, symbols>* rands = new zarray<s32, symbols>;
    for (s32 i = 0; i < symbols; i++)
    {
        char buf[50];
        s32 rand_val = rand()%10000;
        sprintf(buf, "asdfqwea%dasdfqwea", rand_val);
        s32 id = base->add(buf, 0, false);
        rands->push_back(id);
        ASSERT_TEST_NOLOG(id > 0);
        ASSERT_TEST_NOLOG(buf[0] != '\0');
        ASSERT_TEST_NOLOG(base->at(id)[0] != '\0');

    }
    LogInfo() << SCOUNT <<"x" << SLEN << " test rand count:" << rands->size() <<", base string total size:" << base->exploit_;




    zsymbols_static<symbols * slen>* fast = new zsymbols_static<symbols * slen>;

    if (true)
    {
        fast->clear();
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "no reuse add");
        for (auto id : *rands)
        {
            s32 symbol_id = fast->add(base->at(id), 0, false);
            ASSERT_TEST_NOLOG(symbol_id > 0);
        }
    }
    LogInfo() << SCOUNT << "x" << SLEN << " base_no_reuse used bytes:" << fast->exploit_;

    if (true)
    {
        fast->clear();
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "fast reuse add");
        for (auto id : *rands)
        {
            s32 symbol_id = fast->add(base->at(id), 0, true);
            ASSERT_TEST_NOLOG(symbol_id > 0);
        }
    }
    LogInfo() << SCOUNT << "x" << SLEN << " fast reuse used bytes:" << fast->exploit_;

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, symbols, "fast at");
        for (auto id : *rands)
        {
            const char* p = base->at(id);

                ASSERT_TEST_NOLOG((*p != '\0'), "");

        }
    }

    
   
    delete base;
    delete rands;
    delete fast;

    return 0;
}

int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zsymbols_test() == 0);
    std::array<s32, 10000>* first_null = nullptr;
    std::array<s32, 10000>* first_null2 = nullptr;
    std::array<s32, 30>* second_null = nullptr;
    ASSERT_TEST(zsymbols_bench_test(first_null, second_null) == 0);
    ASSERT_TEST(zsymbols_bench_test(first_null2, second_null) == 0);

    LogInfo() << "all test finish .";
    return 0;
}


