

#include "fn_log.h"

#include "zarray.h"
#include <string>
#include "zlist.h"
#include "zhash_map.h"
#include "zcontain_test.h"
#include "zobj_pool.h"

using namespace zsummer;


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

struct Element
{
    int v;
    char buf[12800];
};

template<class V>
s32 LowerBoundStress(V& v, const char* desc)
{
    auto comp = [](const Element& f, const Element& s) {return f.v > s.v; };
    
    double now = Now();
    for (size_t i = 0; i < 10000; i++)
    {
        v.clear();
        for (u32 i = 0; i < 100; i++)
        {
            Element e;
            e.v = rand() % 50;
            v.insert(std::lower_bound(v.begin(), v.end(), e, comp), e);
        }
        for (auto iter = v.begin(); iter != v.end(); ++iter)
        {
            auto iter2 = iter;
            iter2++;
            if (iter2 != v.end())
            {
                if (iter->v < iter2->v)
                {
                    *(u64*)NULL = 0;
                }
            }
        }
        for (u32 i = 0; i < 100; i++)
        {
            v.pop_back();
        }
    }
    LogInfo() << desc << " lower_bound test(10000*100) used:" << Now() - now << "s.";
    return 0;
}

s32 ZListLowerBoundTest()
{
    std::list<Element> sys_list;
    std::unique_ptr< zlist<Element, 100>> zlist_ptr(new  zlist<Element, 100>());
    zlist<Element, 100>& zlist_bound = *zlist_ptr;
    std::vector<Element> sys_vector;
    auto comp = [](const Element& f, const Element& s) {return f.v > s.v; };

    LowerBoundStress(zlist_bound, "zlist bound");
    LowerBoundStress(sys_list, "sys_list bound");
    LowerBoundStress(sys_vector, "sys_vector bound");

    double now = Now();
    for (size_t i = 0; i < 10000; i++)
    {
        zlist_bound.clear();
        for (u32 i = 0; i < 100; i++)
        {
            Element e;
            e.v = rand() % 50;
            zlist_bound.insert(zlist_bound.lower_bound(zlist_bound.begin(), zlist_bound.end(), e, comp), e);
        }
        for (auto iter = zlist_bound.begin(); iter != zlist_bound.end(); ++iter)
        {
            auto iter2 = iter;
            iter2++;
            if (iter2 != zlist_bound.end())
            {
                if (iter->v < iter2->v)
                {
                    *(u64*)NULL = 0;
                }
            }
        }
        for (u32 i = 0; i < 100; i++)
        {
            zlist_bound.pop_back();
        }
    }
    LogInfo() << "zlist_bound inner lower_bound test(10000*100) used:" << Now() - now << "s.";

    return 0;
}
