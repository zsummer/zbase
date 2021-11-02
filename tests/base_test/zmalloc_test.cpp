

#include "fn_log.h"
#include <string>
#include <zhash_map.h>
#include "zcontain_test.h"
#include "zmalloc.h"
#include "zprof.h"

#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()


using namespace zsummer;


s32 ZMallocIOTest()
{
    std::unique_ptr<zmalloc> zstate(new zmalloc());
    memset(zstate.get(), 0, sizeof(zmalloc));
    zstate->max_reserve_block_count_ = 20;
    zstate->set_global(zstate.get());

    static const u32 rand_size = 1000 * 10000;
    static_assert(rand_size > zmalloc::DEFAULT_BLOCK_SIZE, "");
    u32* rand_array = new u32[rand_size];
    for (u32 i = 0; i < zmalloc::DEFAULT_BLOCK_SIZE; i++)
    {
        rand_array[i] = i;
    }
    for (u32 i = zmalloc::DEFAULT_BLOCK_SIZE; i < rand_size; i++)
    {
        rand_array[i] = rand();
    }


    PROF_DEFINE_COUNTER(cost);
    PROF_START_COUNTER(cost);
    for (u64 i = 0; i < rand_size; i++)
    {
        global_zfree(global_zmalloc(1));
    }
    PROF_OUTPUT_MULTI_COUNT_CPU("global_zfree(global_zmalloc(1))", rand_size, cost.stop_and_save().cycles());

    PROF_START_COUNTER(cost);
    for (u64 i = 0; i < rand_size; i++)
    {
        u32 test_size = rand_array[i] % (1024);
        void* p = global_zmalloc(test_size);
        global_zfree(p);
    }
    PROF_OUTPUT_MULTI_COUNT_CPU("global_zfree(global_zmalloc(0~1024))", rand_size, cost.stop_and_save().cycles());

    PROF_START_COUNTER(cost);
    for (u64 i = 0; i < rand_size; i++)
    {
        u32 test_size = (rand_array[i] % (zmalloc::BIG_MAX_REQUEST - zmalloc::SMALL_MAX_REQUEST)) + zmalloc::SMALL_MAX_REQUEST;
        void* p = global_zmalloc(test_size);
        global_zfree(p);
    }
    PROF_OUTPUT_MULTI_COUNT_CPU("global_zfree(global_zmalloc(1024~512k))", rand_size, cost.stop_and_save().cycles());

    PROF_START_COUNTER(cost);
    for (u64 i = rand_size/2; i < rand_size; i++)
    {
        u32 test_size = rand_array[i] % (zmalloc::DEFAULT_BLOCK_SIZE/2);
        void* p = global_zmalloc(test_size);
        global_zfree(p);
    }
    PROF_OUTPUT_MULTI_COUNT_CPU("global_zfree(global_zmalloc(0~2M))", rand_size, cost.stop_and_save().cycles());


    for (u64 i = 0; i < zmalloc::BIG_MAX_REQUEST * 2; i++)
    {
        u32 rand_size = rand_array[i] % (zmalloc::DEFAULT_BLOCK_SIZE * 2);
        void* p = global_zmalloc(rand_size);
        memset(p, 0, rand_size);
        global_zfree(p);
        zstate->check_health();
    }

    void* pz = global_zmalloc(0);
    zstate->check_health();
    global_zfree(pz);

    zstate->clear_cache();
    AssertTest(zstate->used_block_count_ + zstate->reserve_block_count_, 0U, "");
    delete[]rand_array;
    return 0;
}



s32 zmalloc_base_test()
{
    PROF_DEFINE_COUNTER(cost);
    PROF_START_COUNTER(cost);
    for (u32 i = 1024; i < 1000*10000; i++)
    {
        u32 align_value = zmalloc_align_third_bit_value(i);
        u32 bit_order = zmalloc_align_third_bit_order(align_value);
        u32 align_id = zmalloc_third_sequence(bit_order, align_value);
        u32 compress_id = zmalloc_third_sequence_compress(align_id);
        u32 resolve = zmalloc_resolve_order_size(compress_id);
        AssertBoolTest(i <= resolve, "");
    }
    PROF_OUTPUT_MULTI_COUNT_CPU("zmalloc third used", 1000 * 10000 - 1024, cost.stop_and_save().cycles());



    return 0;
}




s32 ZMallocTest()
{
    AssertTest(zmalloc_base_test(), 0, " zmalloc_base_test()");
    AssertTest(ZMallocIOTest(), 0, " ZMallocIOTest()");
    return 0;
}

