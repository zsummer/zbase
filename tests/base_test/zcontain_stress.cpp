

#include "fn_log.h"
#include "zarray.h"
#include <string>
#include "zlist.h"
#include "zhash_map.h"
#include "zcontain_test.h"
#include "zobj_pool.h"
#include "zlist_stress.h"

using namespace zsummer::shm_arena;


#define AssertTest(val1, val2, desc)   \
{\
    auto v1 = (val1); \
    auto v2 = (val2); \
    if ((v1)==(v2)) \
    { \
        LogDebug() << (v1) << " " << (v2) <<" " << desc << " pass.";  \
    } \
    else  \
    { \
        LogError() << (v1) << " " << (v2) <<" " << desc << " failed.";  \
        return 1U;  \
    } \
}

#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()                                

static const size_t LOOP_CAPACITY = 1000;
static const size_t LOAD_CAPACITY = 10000;
template<class List, class Ty, AllocStatusType AS>
s32 ZcontainLinerStress(List& l, const std::string& desc)
{
    double begin = Now();
    for (size_t i = 0; i < LOAD_CAPACITY; i++)
    {
        l.push_back(i);
    }
    double fill_used = Now() - begin;
    size_t fill_count = l.size();
    begin = Now();
    l.clear();
    double clear_used = Now() - begin;

    begin = Now();
    for (size_t loop = 0; loop < 1; loop++)
    {
        for (size_t i = 0; i < LOAD_CAPACITY; i++)
        {
            l.insert(l.begin(), i);
        }
        if (l.size() != LOAD_CAPACITY)
        {
            LogError() << " stress test error";
        }
        for (size_t i = 0; i < LOAD_CAPACITY; i++)
        {
            l.erase(l.begin());
        }
    }
    double front_push_pop_used = Now() - begin;

    begin = Now();
    for (size_t loop = 0; loop < 1; loop++)
    {
        for (size_t i = 0; i < LOAD_CAPACITY; i++)
        {
            l.push_back(i);
        }
        if (l.size() != LOAD_CAPACITY)
        {
            LogError() << " stress test error";
        }
        for (size_t i = 0; i < LOAD_CAPACITY; i++)
        {
            l.pop_back();
        }
    }
    double back_push_pop_used = Now() - begin;

    begin = Now();
    for (size_t i = 0; i < LOAD_CAPACITY * LOOP_CAPACITY; i++)
    {
        l.push_back(i);
        l.pop_back();
    }
    double stress_back = (Now() - begin);

    begin = Now();
    for (size_t j = 0; j < 1000; j++)
    {
        for (size_t i = 0; i < LOAD_CAPACITY; i++)
        {
            l.push_back(i);
        }
        for (size_t i = 0; i < LOAD_CAPACITY; i++)
        {
            l.pop_back();
        }
    }
    double stress_fill = (Now() - begin);

    for (size_t i = 0; i < LOAD_CAPACITY; i++)
    {
        l.push_back(i);
    }

    LogDebug() << desc << ALLOC_INFO(sizeof(Ty) * LOAD_CAPACITY, AS)
        << ":[" << fill_count << "] , fill:" << fill_used << ", clear:" << clear_used
        << ", front_push_pop:" << front_push_pop_used
        << ", back_push_pop:" << back_push_pop_used
        << ", 1000w back: " << stress_back
        << ", 1000 * (10000 push, 10000 free): " << stress_fill;
    return 0;
}

template<class Map, class Ty, AllocStatusType AS>
s32 ZContainMapStress(Map& m, const std::string& desc)
{
    double begin = Now();
    for (size_t i = 0; i < LOAD_CAPACITY; i++)
    {
        m.insert({ i, i });
    }
    double fill_used = Now() - begin;
    size_t fill_count = m.size();
    begin = Now();
    m.clear();
    double clear_used = Now() - begin;

    begin = Now();
    for (size_t i = 0; i < LOAD_CAPACITY; i++)
    {
        m.insert({ i, i });
    }
    if (m.size() != LOAD_CAPACITY)
    {
        LogError() << " stress test error";
    }
    for (size_t i = 0; i < LOAD_CAPACITY; i++)
    {
        m.erase(i);
    }
    if (m.size() != 0)
    {
        LogError() << " stress test error";
    }
    double insert_erase_used = Now() - begin;

    
    for (size_t i = 0; i < LOAD_CAPACITY; i++)
    {
        m.insert({ i, i });
    }
    if (m.size() != LOAD_CAPACITY)
    {
        LogError() << " stress test error";
    }
    begin = Now();
    for (size_t i = 0; i < 1000*10000; i++)
    {
        if (m.find(rand() % LOAD_CAPACITY) == m.end())
        {
            LogError() << " stress test error";
        }
    }
    double find_used = Now() - begin;


    LogDebug() << desc << ALLOC_INFO(sizeof(Ty) * LOAD_CAPACITY, AS)
        << ":[" << fill_count << "] static:" << sizeof(m) <<", payload:" << LOAD_CAPACITY * sizeof(Ty)
        << "b, fill:" << fill_used << ", clear:" << clear_used
        << ", insert_erase:" << insert_erase_used
        << ", full find 1000*10000:" << find_used;
    return 0;
}


static u64 SysFree(void* addr)
{
    u64 bytes = *(u64*)((u64)addr - 16);
    free((void*)((u64)addr - 16));
    return bytes;
}
static void* SysMalloc(u64 bytes)
{
    char* ret = (char*)malloc(bytes + 16);
    *(u64*)(ret) = bytes;
    return ret + 16;
}
s32 ZContainStress()
{
    zstatus<0>::reset();
    auto zstate = std::make_unique<ZMalloc>();
    memset(zstate.get(), 0, sizeof(ZMalloc));
    zstate->max_reserve_seg_count_ = 200;
    zstate->SetGlobal(zstate.get(), &SysMalloc, &SysFree);


    std::unique_ptr<char[]> dlstate(new char[1024 * 4 * 2]);
    memset(dlstate.get(), 0, 1024 * 4 * 2);
    set_global_malloc_state_ptr(dlstate.get(), 1024*4,
        dlstate.get() + 1024*4, 1024 * 4,
        &SysMalloc, &SysFree);

    if (true)
    {
        ShmSysVector<size_t> stdsys;
        AllocStatus<ALLOC_SYS>::reset();
        ZcontainLinerStress<ShmSysVector<size_t>, size_t, ALLOC_SYS>(stdsys, "ShmSysVector<size_t>()");


        ShmZVector<size_t> zv;
        ZMalloc::Instance().Release();
        AllocStatus<ALLOC_ZMALLOC>::reset();
        ZcontainLinerStress<ShmZVector<size_t>, size_t, ALLOC_ZMALLOC>(zv, "ShmZVector<size_t>()");

        ShmDLVector<size_t> dlv;
        release_unused_segments_1();
        AllocStatus<ALLOC_DLMALLOC>::reset();
        ZcontainLinerStress<ShmDLVector<size_t>, size_t, ALLOC_DLMALLOC>(dlv, "ShmDLVector<size_t>()");

        auto zl = std::make_shared<zarray<size_t, LOAD_CAPACITY>>();
        ZcontainLinerStress<zarray<size_t, LOAD_CAPACITY>, size_t, ALLOC_ZMALLOC>(*zl, "zarray<size_t>");
        LogInfo() << "zarray used memory:" << sizeof(zarray<size_t, LOAD_CAPACITY>)
            << ", payload rate:" << sizeof(size_t) * LOAD_CAPACITY * 1.0 / sizeof(zarray<size_t, LOAD_CAPACITY>);


    }
    if (true)
    {
        ShmSysList<size_t> stdsys;
        AllocStatus<ALLOC_SYS>::reset();
        ZcontainLinerStress<ShmSysList<size_t>, size_t, ALLOC_SYS>(stdsys, "ShmSysList<size_t>()");

        ShmZList<size_t> szl;
        ZMalloc::Instance().Release();
        AllocStatus<ALLOC_ZMALLOC>::reset();
        ZcontainLinerStress<ShmZList<size_t>, size_t, ALLOC_ZMALLOC>(szl, "ShmZList<size_t>()");


        ShmDLList<size_t> dll;
        release_unused_segments_1();
        AllocStatus<ALLOC_DLMALLOC>::reset();
        ZcontainLinerStress<ShmDLList<size_t>, size_t, ALLOC_DLMALLOC>(dll, "ShmDLList<size_t>()");

        auto zl = std::make_shared<zlist<size_t, LOAD_CAPACITY>>();
        ZcontainLinerStress<zlist<size_t, LOAD_CAPACITY>, size_t, ALLOC_ZMALLOC>(*zl, "zlist<size_t>");
        LogInfo() << "zlist used memory:" << sizeof(zlist<size_t, LOAD_CAPACITY>)
            << ", payload rate:" << sizeof(size_t) * LOAD_CAPACITY * 1.0 / sizeof(zlist<size_t, LOAD_CAPACITY>);

    }
    if (true)
    {

        ShmSysHashMap<size_t, size_t> stdsys;
        AllocStatus<ALLOC_SYS>::reset();
        ZContainMapStress<ShmSysHashMap<size_t, size_t>, std::pair<size_t, size_t>, ALLOC_SYS>(stdsys, "ShmSysHashMap<size_t,size_t>()");


        ShmZHashMap<size_t, size_t> zh;
        ZMalloc::Instance().Release();
        AllocStatus<ALLOC_ZMALLOC>::reset();
        ZContainMapStress<ShmZHashMap<size_t, size_t>, std::pair<size_t, size_t>, ALLOC_ZMALLOC>(zh, "ShmZHashMap<size_t,size_t>()");
        LogInfo() << "ShmZHashMap :" << ALLOC_INFO(sizeof(std::pair<size_t, size_t>) * LOAD_CAPACITY, ALLOC_ZMALLOC);

        ShmDLHashMap<size_t, size_t> dlh;
        ZMalloc::Instance().Release();
        AllocStatus<ALLOC_DLMALLOC>::reset();
        ZContainMapStress<ShmDLHashMap<size_t, size_t>, std::pair<size_t, size_t>, ALLOC_DLMALLOC>(dlh, "ShmZHashMap<size_t,size_t>()");
        LogInfo() << "ShmDLHashMap :" << ALLOC_INFO(sizeof(std::pair<size_t, size_t>) * LOAD_CAPACITY, ALLOC_DLMALLOC);

        auto zmap = std::make_shared<zhash_map<size_t,size_t, LOAD_CAPACITY>>();
        ZContainMapStress<zhash_map<size_t, size_t, LOAD_CAPACITY>, std::pair<size_t, size_t>, ALLOC_ZMALLOC>(*zmap, "zhash_map<size_t, size_t, LOAD_CAPACITY>");
        LogInfo() << "zhash_map used memory:" << sizeof(zhash_map<size_t, size_t, LOAD_CAPACITY>)
            << ", payload rate:" << sizeof(std::pair<size_t, size_t>) * LOAD_CAPACITY * 1.0 / sizeof(zhash_map<size_t, size_t, LOAD_CAPACITY>);

    }

    if (true)
    {
        u32 space_bytes = zobj_pool<size_t>::static_buf_size(LOAD_CAPACITY);
        std::unique_ptr<char[]> space(new char[space_bytes]);
        zobj_pool<size_t>* zp = (zobj_pool<size_t>*)space.get();
        zp->init(LOAD_CAPACITY, space_bytes);

        double begin = Now();
        for (size_t i = 0; i < 10000000; i++)
        {
            zp->destroy(zp->create());
        }
        double duration = Now() - begin;
        LogInfo() << " ------------  obj pool ----------";
        LogInfo() << "objpool create/destroy 1000w used:" << duration;

        begin = Now();
        for (size_t i = 0; i < 10000000; i++)
        {
            zp->destroy(zp->create(i));
        }
        duration = Now() - begin;
        LogInfo() << " ------------  obj pool ----------";
        LogInfo() << "objpool create/destroy 1000w has inited used:" << duration;
    }
    if (true)
    {
        using Pool = zlist <size_t, LOAD_CAPACITY>;
        auto pool = std::make_unique<Pool>();
        Pool* zp = pool.get();
        double begin = Now();
        for (size_t i = 0; i < 10000000; i++)
        {
            zp->push_back(i);
            zp->pop_back();
        }
        double duration = Now() - begin;
        LogInfo() << " ------------  obj pool ----------";
        LogInfo() << "zlist push_back/pop_back 1000w used:" << duration;
    }
    zstatus<ALLOC_ZMALLOC>::chart();
    AssertTest(ZListLowerBoundTest(), 0, "");
    return 0;
}
