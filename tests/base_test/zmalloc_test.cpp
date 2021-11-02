

#include "fn_log.h"
#include <string>
#include <zhash_map.h>
#include "zcontain_test.h"
#include "zmalloc.h"
#include "zprof.h"

#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()


using namespace zsummer;
static u64 SysFree(void* addr)
{
    free(addr);
    return 0;
}
static void* SysMalloc(u64 bytes)
{
    return malloc(bytes);
}

s32 ZMallocIOTest()
{
    std::unique_ptr<zmalloc> zstate(new zmalloc());
    memset(zstate.get(), 0, sizeof(zmalloc));
    zstate->max_reserve_block_count_ = 20;
    zstate->set_global(zstate.get());
    zstate->set_block_callback(&SysMalloc, &SysFree);


    double  last_time = Now();
    for (u64 i = 0; i < 10 * 100; i++)
    {
        global_zfree(global_zmalloc(1));
    }
    double  now = Now();
    LogDebug() << "zalloc & global_zfree " << 10 * 1000 / 10000 << " w  fixed 512B  use:" << now - last_time;

    for (u64 i = 0; i < 10 * 1000; i++)
    {
        u32 rand_size = rand() % (zmalloc::DEFAULT_PAGE_SIZE / 2);
        void* p = global_zmalloc(rand_size);
        memset(p, 0, rand_size);
        global_zfree(p);
    }



    void* pz = global_zmalloc(0);
    zstate->check_health();
    global_zfree(pz);
    zstate->check_health();
    pz = global_zmalloc(0);
    zstate->check_health();
    global_zfree(pz);
    zstate->check_health();
    pz = global_zmalloc(10 * 1024);
    zstate->check_health();
    void* pz2 = global_zmalloc(2 * 1024);
    zstate->check_health();
    global_zfree(pz);
    zstate->check_health();
    pz = global_zmalloc(10 * 1024);
    zstate->check_health();
    global_zfree(pz);
    zstate->check_health();
    global_zfree(pz2);
    zstate->check_health();
    pz = global_zmalloc(10 * 1024);
    zstate->check_health();
    global_zfree(pz);
    zstate->check_health();
    pz = global_zmalloc(102 * 1024);
    zstate->check_health();
    global_zfree(pz);
    zstate->check_health();
    global_zfree(global_zmalloc(1012));
    zstate->check_health();
    zstate->clear_cache();


    global_zfree(global_zmalloc(234));
    global_zfree(global_zmalloc(666));
    global_zfree(global_zmalloc(555));
    global_zfree(global_zmalloc(888));
    global_zfree(global_zmalloc(111));
    global_zfree(global_zmalloc(7 * 1024 * 1024));
    global_zfree(global_zmalloc(50 * 1024 * 1024));
    zstate->clear_cache();
    AssertTest(zstate->used_block_count_ + zstate->reserve_block_count_, 0U, "");

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

