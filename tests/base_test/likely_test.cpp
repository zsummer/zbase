

#include "fn_log.h"
#include "zprof.h"
#include "zarray.h"
#include <string>
#include "zlist.h"
#include "zhash_map.h"
#include "zlist_ext.h"



struct Element
{
    int v;
    //char buf[8];
    char buf[50];
};
static const int list_size = 100;
static const int rand_size = 1000;
static int rand_array[rand_size] = { 0 };
static Element rand_ele_array[rand_size] = { 0 };


#ifndef _WINDOWS
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif // !_WINDOWS

s32 likely_base_test()
{
    volatile int likely_count = 0;
    volatile int no_likely_count = 0;
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(prof_cost, 1, "nolikey");
        for (u32 loop_i = 0; loop_i < 10000; loop_i++)
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
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(prof_cost, 1, "likey");
        for (u32 loop_i = 0; loop_i < 10000; loop_i++)
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



s32 likely_test()
{
    std::vector<Element> sys_vector;
    std::list<Element> sys_list;


    std::unique_ptr<zarray<Element, list_size>> zarray_ptr(new  zarray<Element, list_size>());
    zarray<Element, list_size>& zarray_ref = *zarray_ptr;
    std::unique_ptr< zlist<Element, list_size>> zlist_ptr(new  zlist<Element, list_size>());
    zlist<Element, list_size>& zlist_bound = *zlist_ptr;
    std::unique_ptr< zlist_ext<Element, list_size, list_size / 2>> zlist_ext_ptr(new  zlist_ext<Element, list_size, list_size / 2>());
    zlist_ext<Element, list_size, list_size / 2 >& zlist_ext_bound = *zlist_ext_ptr;

    for (int i = 0; i < rand_size; i++)
    {
        rand_array[i] = rand();
        rand_ele_array[i].v = rand_array[i];
        sprintf(rand_ele_array[i].buf, "%d", i);
    }



    likely_base_test();

    return 0;
}
