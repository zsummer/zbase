

#include "fn_log.h"

#include "zarray.h"
#include <string>
#include "zlist.h"
#include "zhash_map.h"
#include "zcontain_test.h"
#include "zobj_pool.h"

using namespace zsummer;




#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()                                

struct Element
{
    int v;
    char buf[8];
    //char buf[1280];
};
static const int list_size = 200;
static const int rand_size = 1000;
static int rand_array[rand_size] = { 0 };

inline bool comp(const Element& f, const Element& s)
{
    return f.v > s.v;
}

template<class V>
s32 LowerBoundStress(V& v, bool is_test, const char* desc)
{
    Element e;
    double now = Now();
    for (size_t i = 0; i < 10000; i++)
    {
        v.clear();
        for (u32 i = 0; i < list_size; i++)
        {
            e.v = rand_array[i];
            v.insert(std::lower_bound(v.begin(), v.end(), e, comp), e);
        }
        if (is_test)
        {
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
            for (u32 i = 0; i < list_size; i++)
            {
                v.pop_back();
            }
        }

    }
    LogInfo() << desc << " lower_bound test used:" << (Now() - now)/10000 *1000*1000  << "us.";
    now = Now();
    for (size_t i = 0; i < 10000; i++)
    {
        v.clear();
        for (u32 i = 0; i < list_size; i++)
        {
            e.v = rand_array[i];
            v.push_back(e);
        }
        std::sort(v.begin(), v.end(), comp);
        if (is_test)
        {
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
            for (u32 i = 0; i < list_size; i++)
            {
                v.pop_back();
            }
        }

    }
    LogInfo() << desc << " sort test used:" << (Now() - now) / 10000 * 1000 * 1000 << "us.";

    return 0;
}


template<class V>
s32 LowerBoundListStress(V& v, bool is_test, const char* desc)
{
    Element e;
    double now = Now();
    for (size_t i = 0; i < 10000; i++)
    {
        v.clear();
        for (u32 i = 0; i < list_size; i++)
        {
            e.v = rand_array[i];
            v.insert(v.lower_bound(v.begin(), v.end(), e, comp), e);
        }
        if (is_test)
        {
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
            for (u32 i = 0; i < list_size; i++)
            {
                v.pop_back();
            }
        }

    }
    LogInfo() << desc << " lower_bound used:" << (Now() - now) / 10000 * 1000 * 1000 << "us.";

    return 0;
}

#ifndef _WINDOWS
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif // !_WINDOWS

s32 likely_test()
{
    volatile int likely_count = 0;
    volatile int no_likely_count = 0;
    if (true)
    {
        PROF_DEFINE_AUTO_SINGLE_RECORD(prof_cost, 1, PROF_LEVEL_NORMAL, "nolikey");
        for (size_t i = 0; i < 10000; i++)
        {
            for (size_t i = 0; i < rand_size; i++)
            {
                if (rand_array[i]%1024 < 10)
                {
                    no_likely_count++;
                }
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_SINGLE_RECORD(prof_cost, 1, PROF_LEVEL_NORMAL, "likey");
        for (size_t i = 0; i < 10000; i++)
        {
            for (size_t i = 0; i < rand_size; i++)
            {
                if (likely(rand_array[i]%1024 < 10))
                {
                    likely_count++;
                }
            }
        }
    }
    LogDebug() << "no likely hit:" << no_likely_count << "/" << 10000 * rand_size << ",  likely hit:" << likely_count << "/" << 10000 * rand_size;
    return 0;
}



s32 ZListLowerBoundTest()
{
    std::list<Element> sys_list;
    std::unique_ptr< zlist<Element, list_size>> zlist_ptr(new  zlist<Element, list_size>());
    zlist<Element, list_size>& zlist_bound = *zlist_ptr;
    std::vector<Element> sys_vector;

    std::unique_ptr< zarray<Element, list_size>> zarray_ptr(new  zarray<Element, list_size>());
    zarray<Element, list_size>& zarray_ref = *zarray_ptr;
    for (size_t i = 0; i < rand_size; i++)
    {
        rand_array[i] = rand();
    }

    LowerBoundStress(sys_vector, true, "sys_vector");
    LowerBoundStress(zarray_ref, true, "zarray");

    LowerBoundStress(sys_vector, false, "sys_vector");
    LowerBoundStress(zarray_ref, false, "zarray");


    LowerBoundListStress(zlist_bound, true,  "zlist");
    LowerBoundListStress(zlist_bound, false,  "zlist");


    likely_test();

    return 0;
}
