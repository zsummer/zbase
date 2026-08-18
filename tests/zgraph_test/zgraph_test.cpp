
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

#ifdef __APPLE__
#define ZCLOCK_NO_RDTSC
#endif

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
    zclock<zclock_impl::kClockSystem>             c01;
    zclock<zclock_impl::kClockClock>           c02;
    zclock<zclock_impl::kClockChrono>          c03;
    zclock<zclock_impl::kClockSteadyChrono>    c04;
    zclock<zclock_impl::kClockSystemChrono>       c05;
    zclock<zclock_impl::kClockPureRDTSC>       c06;
    zclock<zclock_impl::kClockVolatileRDTSC>   c07;
    zclock<zclock_impl::kClockFenceRDTSC>      c08;
    zclock<zclock_impl::kClockBTBFenceRDTSC>   c09;
    zclock<zclock_impl::kClockRDTSCP>          c10;
    zclock<zclock_impl::kClockMFenceRDTSC>     c11;
    zclock<zclock_impl::kClockBTBMFenceRDTSC>  c12;
    zclock<zclock_impl::kClockLockRDTSC>       c13;
    zclock<zclock_impl::kClockSystemMS>       c14;

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
    c14.start();

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
    c14.stop_and_save();


    ASSERT_TEST(std::abs(c01.cost_ms() - 1000) < 200, " dev ms:", c01.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c02.cost_ms() - 1000) < 200, " dev ms:", c02.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c03.cost_ms() - 1000) < 200, " dev ms:", c03.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c04.cost_ms() - 1000) < 200, " dev ms:", c04.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c05.cost_ms() - 1000) < 200, " dev ms:", c05.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c06.cost_ms() - 1000) < 200, " dev ms:", c06.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c07.cost_ms() - 1000) < 200, " dev ms:", c07.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c08.cost_ms() - 1000) < 200, " dev ms:", c08.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c09.cost_ms() - 1000) < 200, " dev ms:", c09.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c10.cost_ms() - 1000) < 200, " dev ms:", c10.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c11.cost_ms() - 1000) < 200, " dev ms:", c11.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c12.cost_ms() - 1000) < 200, " dev ms:", c12.cost_ms()- 1000);
    ASSERT_TEST(std::abs(c13.cost_ms() - 1000) < 200, " dev ms:", c13.cost_ms() - 1000);
    ASSERT_TEST(std::abs(c14.cost_ms() - 1000) < 200, " dev ms:", c14.cost_ms() - 1000);


    return 0;
}

template<zclock_impl::clock_type ctype>
s32 zclock_cost_test(const std::string& desc)
{
    zclock<ctype> c;
    zclock<> cost;
    volatile s64 salt = 0;
    cost.start();
    for (size_t i = 0; i < 10*10000; i++)
    {
        c.start();
        salt += c.stop_and_save().cost();
    }
    cost.stop_and_save();
    LogInfo() << desc << " start + stop cost:" << (double)cost.cost_ns() / (10.0 * 10000) <<"ns";
    return 0;
}

s32 zclock_bench_test()
{
#define bench(x)  zclock_cost_test<x>(#x)
    bench(zclock_impl::kClockSystem);
    bench(zclock_impl::kClockClock);
    bench(zclock_impl::kClockChrono);
    bench(zclock_impl::kClockSteadyChrono);
    bench(zclock_impl::kClockSystemChrono);
    bench(zclock_impl::kClockSystemMS);
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

FNLog::LogStream& operator<<(FNLog::LogStream& ls, const zclock_impl::vmdata& vm)
{
    ls << "vm:" << vm.vm_size << ", rss:" << vm.rss_size << ", shm:" << vm.shr_size;
    return ls;
}

int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zclock_test() == 0);
    ASSERT_TEST(zclock_bench_test() == 0);

    
    LogInfo() << zclock_impl::get_self_mem();
    LogInfo() << zclock_impl::get_sys_mem();

    LogInfo() << "all test finish .";
    return 0;
}


