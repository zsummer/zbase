
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
    zclock_base<zclock_impl::T_CLOCK_SYS>             c01;
    zclock_base<zclock_impl::T_CLOCK_CLOCK>           c02;
    zclock_base<zclock_impl::T_CLOCK_CHRONO>          c03;
    zclock_base<zclock_impl::T_CLOCK_STEADY_CHRONO>    c04;
    zclock_base<zclock_impl::T_CLOCK_SYS_CHRONO>       c05;
    zclock_base<zclock_impl::T_CLOCK_PURE_RDTSC>       c06;
    zclock_base<zclock_impl::T_CLOCK_VOLATILE_RDTSC>   c07;
    zclock_base<zclock_impl::T_CLOCK_FENCE_RDTSC>      c08;
    zclock_base<zclock_impl::T_CLOCK_BTB_FENCE_RDTSC>   c09;
    zclock_base<zclock_impl::T_CLOCK_RDTSCP>          c10;
    zclock_base<zclock_impl::T_CLOCK_MFENCE_RDTSC>     c11;
    zclock_base<zclock_impl::T_CLOCK_BTB_MFENCE_RDTSC>  c12;
    zclock_base<zclock_impl::T_CLOCK_LOCK_RDTSC>       c13;
    zclock_base<zclock_impl::T_CLOCK_SYS_MS>       c14;

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
    ASSERT_TEST(std::abs(c13.duration_ms() - 1000) < 100, " dev ms:", c13.duration_ms() - 1000);
    ASSERT_TEST(std::abs(c14.duration_ms() - 1000) < 100, " dev ms:", c14.duration_ms() - 1000);


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
        salt += c.stop_and_save().duration_ticks();
    }
    cost.stop_and_save();
    LogInfo() << desc << " start + stop cost:" << (double)cost.duration_ns() / (10.0 * 10000) <<"ns";
    return 0;
}

s32 zclock_bench_test()
{
#define bench(x)  zclock_cost_test<x>(#x)
    bench(zclock_impl::T_CLOCK_SYS);
    bench(zclock_impl::T_CLOCK_CLOCK);
    bench(zclock_impl::T_CLOCK_CHRONO);
    bench(zclock_impl::T_CLOCK_STEADY_CHRONO);
    bench(zclock_impl::T_CLOCK_SYS_CHRONO);
    bench(zclock_impl::T_CLOCK_SYS_MS);
    bench(zclock_impl::T_CLOCK_PURE_RDTSC);
    bench(zclock_impl::T_CLOCK_VOLATILE_RDTSC);
    bench(zclock_impl::T_CLOCK_FENCE_RDTSC);
    bench(zclock_impl::T_CLOCK_BTB_FENCE_RDTSC);
    bench(zclock_impl::T_CLOCK_RDTSCP);
    bench(zclock_impl::T_CLOCK_MFENCE_RDTSC);
    bench(zclock_impl::T_CLOCK_BTB_MFENCE_RDTSC);
    bench(zclock_impl::T_CLOCK_LOCK_RDTSC);
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

    
    LogInfo() << zclock_impl::get_vmdata();
    LogInfo() << zclock_impl::get_sys_mem();

    LogInfo() << "all test finish .";
    return 0;
}


