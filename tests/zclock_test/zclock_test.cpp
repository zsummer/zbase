
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
#include "zclock.h"

s32 zclock_test()
{
    zclock_base<zclock_impl::kClockSys>             c01;
    zclock_base<zclock_impl::kClockClock>           c02;
    zclock_base<zclock_impl::kClockChrono>          c03;
    zclock_base<zclock_impl::kClockSteadyChrono>    c04;
    zclock_base<zclock_impl::kClockSysChrono>       c05;
    zclock_base<zclock_impl::kClockPureRDTSC>       c06;
    zclock_base<zclock_impl::kClockVolatileRDTSC>   c07;
    zclock_base<zclock_impl::kClockFenceRDTSC>      c08;
    zclock_base<zclock_impl::kClockBTBFenceRDTSC>   c09;
    zclock_base<zclock_impl::kClockRDTSCP>          c10;
    zclock_base<zclock_impl::kClockMFenceRDTSC>     c11;
    zclock_base<zclock_impl::kClockBTBMFenceRDTSC>  c12;
    zclock_base<zclock_impl::kClockLockRDTSC>       c13;

    c01.start();
    c02.start();
    c03.start();
    c04.start();
    c05.start();
    c06.start();
    c07.start();
    c08.start();
    c09.start();
    c10.start();
    c11.start();
    c12.start();
    c13.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    c01.stop_and_save();
    c02.stop_and_save();
    c03.stop_and_save();
    c04.stop_and_save();
    c05.stop_and_save();
    c06.stop_and_save();
    c07.stop_and_save();
    c08.stop_and_save();
    c09.stop_and_save();
    c10.stop_and_save();
    c11.stop_and_save();
    c12.stop_and_save();
    c13.stop_and_save();


    ASSERT_TEST(std::abs(c01.duration_ms() - 1000) < 100, " dev ms:", c01.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c02.duration_ms() - 1000) < 100, " dev ms:", c02.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c03.duration_ms() - 1000) < 100, " dev ms:", c03.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c04.duration_ms() - 1000) < 100, " dev ms:", c04.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c05.duration_ms() - 1000) < 100, " dev ms:", c05.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c06.duration_ms() - 1000) < 100, " dev ms:", c06.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c07.duration_ms() - 1000) < 100, " dev ms:", c07.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c08.duration_ms() - 1000) < 100, " dev ms:", c08.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c09.duration_ms() - 1000) < 100, " dev ms:", c09.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c10.duration_ms() - 1000) < 100, " dev ms:", c10.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c11.duration_ms() - 1000) < 100, " dev ms:", c11.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c12.duration_ms() - 1000) < 100, " dev ms:", c12.duration_ms()- 1000);
    ASSERT_TEST(std::abs(c13.duration_ms() - 1000) < 100, " dev ms:", c13.duration_ms()- 1000);


    return 0;
}

template<zclock_impl::clock_type ctype>
s32 zclock_cost_test(const std::string& desc)
{
    zclock_base<ctype> c;
    zclock cost;
    volatile s64 salt = 0;
    cost.start();
    for (size_t i = 0; i < 10*10000; i++)
    {
        c.start();
        salt += c.stop_and_save().duration_cycles();
    }
    cost.stop_and_save();
    LogInfo() << desc << " start + stop cost:" << (double)cost.duration_ns() / (10.0 * 10000) <<"ns";
    return 0;
}

s32 zclock_bench_test()
{
#define bench(x)  zclock_cost_test<x>(#x)
    bench(zclock_impl::kClockSys);
    bench(zclock_impl::kClockClock);
    bench(zclock_impl::kClockChrono);
    bench(zclock_impl::kClockSteadyChrono);
    bench(zclock_impl::kClockSysChrono);
    bench(zclock_impl::kClockPureRDTSC);
    bench(zclock_impl::kClockVolatileRDTSC);
    bench(zclock_impl::kClockFenceRDTSC);
    bench(zclock_impl::kClockBTBFenceRDTSC);
    bench(zclock_impl::kClockRDTSCP);
    bench(zclock_impl::kClockMFenceRDTSC);
    bench(zclock_impl::kClockBTBMFenceRDTSC);
    bench(zclock_impl::kClockLockRDTSC);
    return 0;
}

int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zclock_test() == 0);
    ASSERT_TEST(zclock_bench_test() == 0);


    LogInfo() << "all test finish .";
    return 0;
}

