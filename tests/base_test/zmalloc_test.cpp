

#include "fn_log.h"
#include <string>
#include <zhash_map.h>
#include "zcontain_test.h"
#include "zmalloc.h"


#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()



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
    std::unique_ptr<ZMalloc> zstate(new ZMalloc());
    memset(zstate.get(), 0, sizeof(ZMalloc));
    zstate->max_reserve_seg_count_ = 20;
    zstate->SetGlobal(zstate.get(), &SysMalloc, &SysFree);


    double  last_time = Now();
    for (u64 i = 0; i < 10 * 100; i++)
    {
        zfree(zmalloc(1));
    }
    double  now = Now();
    LogDebug() << "zalloc & zfree " << 10 * 1000 / 10000 << " w  fixed 512B  use:" << now - last_time;

    for (u64 i = 0; i < 10 * 1000; i++)
    {
        u32 rand_size = rand() % (ZMalloc::SALE_SYS_ALLOC_SIZE / 2);
        void* p = zmalloc(rand_size);
        memset(p, 0, rand_size);
        zfree(p);
    }



    void* pz = zmalloc(0);
    zstate->Check();
    zfree(pz);
    zstate->Check();
    pz = zmalloc(0);
    zstate->Check();
    zfree(pz);
    zstate->Check();
    pz = zmalloc(10 * 1024);
    zstate->Check();
    void* pz2 = zmalloc(2 * 1024);
    zstate->Check();
    zfree(pz);
    zstate->Check();
    pz = zmalloc(10 * 1024);
    zstate->Check();
    zfree(pz);
    zstate->Check();
    zfree(pz2);
    zstate->Check();
    pz = zmalloc(10 * 1024);
    zstate->Check();
    zfree(pz);
    zstate->Check();
    pz = zmalloc(102 * 1024);
    zstate->Check();
    zfree(pz);
    zstate->Check();
    zfree(zmalloc(1012));
    zstate->Check();
    zstate->Release();


    zfree(zmalloc(234));
    zfree(zmalloc(666));
    zfree(zmalloc(555));
    zfree(zmalloc(888));
    zfree(zmalloc(111));
    zfree(zmalloc(7 * 1024 * 1024));
    zfree(zmalloc(50 * 1024 * 1024));
    zstate->Release();
    AssertTest(zstate->used_seg_count_ + zstate->reserve_seg_count_, 0U, "");

    return 0;
}


s32 ZMallocTest()
{
    AssertTest(ZMallocIOTest(), 0, " ZMallocIOTest()");
    return 0;
}

