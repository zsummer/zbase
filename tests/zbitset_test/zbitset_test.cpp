
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
#include "zbitset.h"

s32 zbitset_test()
{
    u64 array_data[100];
    zbitset bs;
    bs.attach(array_data, 100, true);
    ASSERT_TEST(bs.win_size() == 0);
    ASSERT_TEST(!bs.has(1));
    ASSERT_TEST(bs.win_size() == 0);
    ASSERT_TEST(bs.dirty_count() == 0);
    ASSERT_TEST(bs.empty());



    bs.set_with_win(1);
    ASSERT_TEST(bs.has(1));
    ASSERT_TEST(bs.win_size() == 1);
    ASSERT_TEST(bs.dirty_count() == 1);
    ASSERT_TEST(!bs.empty());

    bs.set_with_win(3);
    ASSERT_TEST(bs.has(3));
    ASSERT_TEST(bs.dirty_count() == 2);

    bs.set_with_win(700);
    ASSERT_TEST(bs.has(700));
    ASSERT_TEST(bs.dirty_count() == 3);


    bs.set_with_win(6399);
    ASSERT_TEST(bs.has(6399));
    ASSERT_TEST(bs.has_error() == 0);


    bs.set_with_win(6400);
    ASSERT_TEST(bs.has_error() == 1);
    ASSERT_TEST(!bs.has(6400));


    u32 bit_id = bs.first_bit();
    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == 1);
    ASSERT_TEST(!bs.has(1));


    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == 3);
    ASSERT_TEST(!bs.has(3));



    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == 700);
    ASSERT_TEST(!bs.has(700));



    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == 6399);
    ASSERT_TEST(!bs.has(6399));
    ASSERT_TEST(bs.has_error() == 1);

    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == bs.bit_count());


    for (u32 i = 0; i < 100; i++)
    {
        ASSERT_TEST_NOLOG(bs.array_data()[i] == 0, i);
    }

    bs.light_clear();
    ASSERT_TEST(bs.has_error() == 0);
    ASSERT_TEST(bs.win_size() == 0);
    ASSERT_TEST(bs.dirty_count() == 0);
    ASSERT_TEST(bs.empty());


    ASSERT_TEST(!bs.has(0));
    bs.set_with_win(0);
    ASSERT_TEST(bs.has(0));
    bit_id = bs.first_bit();
    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == 0);
    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == bs.bit_count());


    return 0;
}



s32 zbitset_test_clone_test()
{
    u64 array_data[100];
    u64 clone_data[100];
    
    zbitset bs;
    bs.attach(array_data, 100, true);
    bs.set_with_win(1);
    bs.set_with_win(3);
    bs.set_with_win(700);
    bs.set_with_win(6399);
    bs.set_with_win(6400);

    zbitset clone_bs;
    clone_bs.attach(clone_data, 100, false);
    clone_bs.clone_from(bs);

    u32 bit_id = clone_bs.first_bit();
    ASSERT_TEST((bit_id = clone_bs.pick_next_with_win(bit_id)) == 1);
    ASSERT_TEST(!clone_bs.has(1));


    ASSERT_TEST((bit_id = clone_bs.pick_next_with_win(bit_id)) == 3);
    ASSERT_TEST(!clone_bs.has(3));



    ASSERT_TEST((bit_id = clone_bs.pick_next_with_win(bit_id)) == 700);
    ASSERT_TEST(!clone_bs.has(700));



    ASSERT_TEST((bit_id = clone_bs.pick_next_with_win(bit_id)) == 6399);
    ASSERT_TEST(!clone_bs.has(6399));
    ASSERT_TEST(clone_bs.has_error() == 1);

    ASSERT_TEST((bit_id = clone_bs.pick_next_with_win(bit_id)) == clone_bs.bit_count());


    for (u32 i = 0; i < 100; i++)
    {
        ASSERT_TEST_NOLOG(clone_bs.array_data()[i] == 0, i);
    }

    clone_bs.light_clear();
    ASSERT_TEST(clone_bs.has_error() == 0);
    ASSERT_TEST(clone_bs.win_size() == 0);
    ASSERT_TEST(clone_bs.dirty_count() == 0);
    ASSERT_TEST(clone_bs.empty());


    ASSERT_TEST(!clone_bs.has(0));
    clone_bs.set_with_win(0);
    ASSERT_TEST(clone_bs.has(0));
    bit_id = clone_bs.first_bit();
    ASSERT_TEST((bit_id = clone_bs.pick_next_with_win(bit_id)) == 0);
    ASSERT_TEST((bit_id = clone_bs.pick_next_with_win(bit_id)) == clone_bs.bit_count());


    return 0;
}


s32 zbitset_bench_win()
{
    u64 array_[100];
    constexpr u32 bit_count = zbitset::max_bit_count(100);
    zbitset bs;
    bs.attach(array_, 100, true);

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, bit_count / 2,"set_with_win * 3200");
        for (u32 i = 0; i < bit_count / 2; i++)
        {
            bs.set_with_win(rand() % bit_count);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(cost, "pick_next_with_win");
        volatile u32 add = 0;
        u32 bit_id = bs.first_bit();
        while ((bit_id = bs.pick_next_with_win(bit_id)) < bs.bit_count())
        {
            add++;
        }
    }

    bs.light_clear();
    return 0;
}

s32 zbitset_bench()
{
    u64 array_[100];
    constexpr u32 bit_count = zbitset::max_bit_count(100);
    zbitset bs;
    bs.attach(array_, 100, true);

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, bit_count / 2, "set * 3200");
        for (u32 i = 0; i < bit_count / 2; i++)
        {
            bs.set(rand() % bit_count);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(cost, "pick_next");
        volatile u32 add = 0;
        u32 bit_id = 0;
        while ((bit_id = bs.pick_next(bit_id)) < bs.bit_count())
        {
            add++;
        }
    }

    bs.light_clear();
    return 0;
}


s32 zbitset_bench_static()
{
    constexpr u32 bit_count = zbitset::max_bit_count(100);
    zbitset_static<bit_count> bs;
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, bit_count / 2, "set * 3200");
        for (u32 i = 0; i < bit_count / 2; i++)
        {
            bs.set(rand() % bit_count);
        }
    }

    if (true)
    {
        s32 ret = memcmp(zbitset_static<bit_count>(bs).array_data(), bs.array_data(), bs.array_size());
        ASSERT_TEST(ret == 0);
    }

    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(cost, "pick copy next");
        volatile u32 add = 0;
        u32 bit_id = 0;
        zbitset_static<bit_count> copy_bs(bs);
        while ((bit_id = bs.pick_next(bit_id)) < copy_bs.bit_count())
        {
            add++;
        }
    }



    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(cost, "pick_next");
        volatile u32 add = 0;
        u32 bit_id = 0;
        while ((bit_id = bs.pick_next(bit_id)) < bs.bit_count())
        {
            add++;
        }
    }



    bs.light_clear();
    return 0;
}

int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zbitset_test() == 0);
    ASSERT_TEST(zbitset_test_clone_test() == 0);
    ASSERT_TEST(zbitset_bench_win() == 0);
    ASSERT_TEST(zbitset_bench() == 0);
    ASSERT_TEST(zbitset_bench_static() == 0);

    LogInfo() << "all test finish .";
    return 0;
}


