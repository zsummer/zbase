
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
#include "ztrace.h"


using test_callstacker = zcallstacker<5>;
test_callstacker g_call_stacker_;
#define CALL_STACKER_GUARD() ztrace_guard<test_callstacker> trace_guard(g_call_stacker_, __FUNCTION__)


void func1();
void func2();
void func3();
void func4();
void func5();

void func1()
{
    CALL_STACKER_GUARD();
    func2();
}
void func2()
{
    CALL_STACKER_GUARD();
    func3();
}
void func3()
{
    CALL_STACKER_GUARD();
    func4();
}
void func4()
{
    CALL_STACKER_GUARD();
    func5();
}
void func5()
{
    CALL_STACKER_GUARD();
}

class MyClass1
{
public:
    void test()
    {
        CALL_STACKER_GUARD();
        func1();
    }

private:

};

class MyClass2
{
public:
    void test()
    {
        CALL_STACKER_GUARD();
        func5();
    }

private:

};


s32 ztrace_test()
{
    CALL_STACKER_GUARD();
    MyClass1 o;
    o.test();

    LogInfo() << "print last call stack. stacker good:" << g_call_stacker_.good();
    for (s32 i = 0; i <= g_call_stacker_.top(); i++)
    {
        FNLog::LogStream ls = std::move(LogInfo());

        ls.write_buffer("                                                                          ", i * 2);

        if (i == g_call_stacker_.top())
        {
            ls << "*";
        }
        ls << g_call_stacker_.at(i);
    }
    g_call_stacker_.reset_errcode();
    g_call_stacker_.set_top(1);
    MyClass2 o2;
    o2.test();
    LogInfo() << "print last call stack. stacker good:" << g_call_stacker_.good();
    for (s32 i = 0; i <= g_call_stacker_.top(); i++)
    {
        FNLog::LogStream ls = std::move(LogInfo());

        ls.write_buffer("                                                                          ", i * 2);

        if (i == g_call_stacker_.top())
        {
            ls << "*";
        }
        ls << g_call_stacker_.at(i);
    }

    return 0;
}




s32 ztrace_bench_test()
{
    MyClass2 o;
    PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000*10000, "used");
    for (s32 i = 0; i < 1000 * 10000; i++)
    {
        g_call_stacker_.reset();
        o.test();
    }
    return 0;
}

int main(int argc, char *argv[])
{
    ztest_init();
    CALL_STACKER_GUARD();
    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(ztrace_test() == 0);

    ASSERT_TEST(ztrace_bench_test() == 0);

    LogInfo() << "all test finish .";
    return 0;
}


