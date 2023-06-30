


#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zarray.h"
#include "zvector.h"
#include <string>
#include "zlist.h"
#include "zlist_ext.h"
#include "zprof.h"
#include "zmalloc.h"
#include "zallocator.h"


template<class _Ty, size_t _Size>
class StaticVector : public std::vector<_Ty>
{
public:
    StaticVector()
    {
        std::vector<_Ty>::reserve(_Size);
    }
};

#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()                                

static const int LOOP_CAPACITY = 1000;
static const int LOAD_CAPACITY = 1024*8;

std::string string_data[LOAD_CAPACITY];


template<class List, class V>
s32 LinerStress(List& l, const std::string& desc,  bool is_static, bool out_prof)
{
    std::stringstream ss;
    ss << desc << ":" << LOAD_CAPACITY  << ": ";
    PROF_DEFINE_COUNTER(cost);
    
    if (true)
    {
        PROF_START_COUNTER(cost);

        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            l.push_back(V(i));
        }
        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "push_back").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        }
        if (is_static)
        {
            l.push_back(V(LOAD_CAPACITY));
            l.push_back(V(LOAD_CAPACITY));
        }
        ASSERT_TEST_EQ((int)l.size(), LOAD_CAPACITY, desc + ": error");

        int i = 0;
        for (auto &v : l)
        {
            if ((v) != i)
            {
                volatile int a = i;
                (void)a;
            }
            ASSERT_TEST_EQ((int)v, i, desc + ": error");
            i++;
        }
        
    }

    if (true)
    {
        PROF_START_COUNTER(cost);
        l.clear();
        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "clear").c_str(), 1, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), 0, desc + ": error");
        if (is_static)
        {
            l.erase(l.end());
        }
    }



    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int loop = 0; loop < 1; loop++)
        {
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.insert(l.begin(), V(i));
            }
            if (l.size() != LOAD_CAPACITY)
            {
                LogError() << " stress test error";
            }
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.erase(l.begin());
            }
        }
        ASSERT_TEST_EQ((int)l.size(), 0, desc + ": error");
        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "insert begin &erase begin").c_str(), LOAD_CAPACITY * 2, cost.stop_and_save().cycles());
        }
    }

    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            l.insert(l.begin(), V(i));
        }
        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "insert begin").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        }
    }

    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            l.erase(l.begin());
        }
        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "pop begin").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), 0, desc + ": error");

    }


    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int loop = 0; loop < 1; loop++)
        {
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.insert(l.end(), V(i));
            }
            if (l.size() != LOAD_CAPACITY)
            {
                LogError() << " stress test error";
            }
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                auto iter = l.end();
                iter--;
                l.erase(iter);
            }
        }
        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "insert end & pop end(capacity)").c_str(), LOAD_CAPACITY * 2, cost.stop_and_save().cycles());
        }
        if (is_static)
        {
            l.erase(l.end());
        }
        ASSERT_TEST_EQ((int)l.size(), 0, desc + ": error");

    }



    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int loop = 0; loop < 1; loop++)
        {
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.push_back(V(i));
            }
            if (l.size() != LOAD_CAPACITY)
            {
                LogError() << " stress test error";
            }
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.pop_back();
            }
        }
        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "push back & pop back(capacity)").c_str(), LOAD_CAPACITY * 2, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), 0, desc + ": error");

    }


    if (true)
    {
        PROF_START_COUNTER(cost);
        cost.start();
        for (int i = 0; i < LOAD_CAPACITY * LOOP_CAPACITY; i++)
        {
            l.push_back(V(i));
            l.pop_back();
        }
        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "push back & pop back (one item)").c_str(), LOAD_CAPACITY * LOOP_CAPACITY * 2, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), 0, desc + ": error");

    }


    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.push_back(V(i));
            }
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.pop_back();
            }
        }
        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "push back & pop back(capacity loop 1000)").c_str(), LOAD_CAPACITY * 2 * 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), 0, desc + ": error");

    }


    if (true)
    {
        l.clear();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            l.push_back(V(i));
        }
        auto p = std::make_unique<List>();
        List& other = *p.get();
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            other = l;
            l = other;
        }

        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "full cap copy from other(other =l):size" + std::to_string(LOAD_CAPACITY) ).c_str(), 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), LOAD_CAPACITY, desc + ": error");
        ASSERT_TEST_EQ(*l.begin(), *other.begin(), "");
        ASSERT_TEST_EQ(*l.rbegin(), *other.rbegin(), "");
        l.clear();
    }

    if (true)
    {
        l.clear();
        for (int i = 0; i < LOAD_CAPACITY/2; i++)
        {
            l.push_back(V(i));
        }
        auto p = std::make_unique<List>();
        List& other = *p.get();
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            other = l;
            l = other;
        }

        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "half cap copy from other(other =l):size" + std::to_string(LOAD_CAPACITY/2)).c_str(), 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), LOAD_CAPACITY/2, desc + ": error");
        ASSERT_TEST_EQ(*l.begin(), *other.begin(), "");
        ASSERT_TEST_EQ(*l.rbegin(), *other.rbegin(), "");
        l.clear();
    }

    if (true)
    {
        l.clear();
        for (int i = 0; i < LOAD_CAPACITY / 3; i++)
        {
            l.push_back(V(i));
        }
        auto p = std::make_unique<List>();
        List& other = *p.get();
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            other = l;
            l = other;
        }

        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + " 1/3 cap copy from other(other =l):size" + std::to_string(LOAD_CAPACITY / 2)).c_str(), 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), LOAD_CAPACITY / 3, desc + ": error");
        ASSERT_TEST_EQ(*l.begin(), *other.begin(), "");
        ASSERT_TEST_EQ(*l.rbegin(), *other.rbegin(), "");
        l.clear();
    }

    if (true)
    {
        l.clear();
        for (int i = 0; i < LOAD_CAPACITY / 5; i++)
        {
            l.push_back(V(i));
        }
        auto p = std::make_unique<List>();
        List& other = *p.get();
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            other = l;
            l = other;
        }

        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + " 1/5 cap copy from other(other =l):size" + std::to_string(LOAD_CAPACITY / 2)).c_str(), 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), LOAD_CAPACITY / 5, desc + ": error");
        ASSERT_TEST_EQ(*l.begin(), *other.begin(), "");
        ASSERT_TEST_EQ(*l.rbegin(), *other.rbegin(), "");
        l.clear();
    }


    if (true)
    {
        l.clear();
        auto p = std::make_unique<List>();
        List& other = *p.get();
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            other = l;
            l = other;
        }

        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "empty cap copy from other(other =l):size" + std::to_string(0)).c_str(), 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), 0, desc + ": error");
        l.clear();
    }


    if (true)
    {
        l.clear();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            l.push_back(V(i));
        }
        auto p = std::make_unique<List>();
        List& other = *p.get();
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            other = std::move(l);
            l = std::move(other);
        }

        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "full cap std::move() from other(other =l):size" + std::to_string(LOAD_CAPACITY)).c_str(), 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), LOAD_CAPACITY, desc + ": error");
        l.clear();
    }

    if (true)
    {
        l.clear();
        for (int i = 0; i < LOAD_CAPACITY / 2; i++)
        {
            l.push_back(V(i));
        }
        auto p = std::make_unique<List>();
        List& other = *p.get();
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            other = std::move(l);
            l = std::move(other);
        }

        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "half cap std::move() from other(other =l):size" + std::to_string(LOAD_CAPACITY / 2)).c_str(), 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), LOAD_CAPACITY / 2, desc + ": error");
        //ASSERT_TEST_EQ(*l.begin(), *other.begin(), "");
        //ASSERT_TEST_EQ(*l.rbegin(), *other.rbegin(), "");
        l.clear();
    }

    if (true)
    {
        l.clear();
        for (int i = 0; i < LOAD_CAPACITY / 3; i++)
        {
            l.push_back(V(i));
        }
        auto p = std::make_unique<List>();
        List& other = *p.get();
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            other = std::move(l);
            l = std::move(other);
        }

        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + " 1/3 cap std::move() from other(other =l):size" + std::to_string(LOAD_CAPACITY / 2)).c_str(), 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), LOAD_CAPACITY / 3, desc + ": error");
        //ASSERT_TEST_EQ(*l.begin(), *other.begin(), "");
        //ASSERT_TEST_EQ(*l.rbegin(), *other.rbegin(), "");
        l.clear();
    }

    if (true)
    {
        l.clear();
        for (int i = 0; i < LOAD_CAPACITY / 5; i++)
        {
            l.push_back(V(i));
        }
        auto p = std::make_unique<List>();
        List& other = *p.get();
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            other = std::move(l);
            l = std::move(other);
        }

        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + " 1/5 cap std::move() from other(other =l):size" + std::to_string(LOAD_CAPACITY / 2)).c_str(), 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), LOAD_CAPACITY / 5, desc + ": error");
        l.clear();
    }

    if (true)
    {
        l.clear();
        auto p = std::make_unique<List>();
        List& other = *p.get();
        PROF_START_COUNTER(cost);
        for (int j = 0; j < 1000; j++)
        {
            other = std::move(l);
            l = std::move(other);
        }

        if (out_prof)
        {
            PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "empty list std::move() from other(other =l):size" + std::to_string(0)).c_str(), 1000, cost.stop_and_save().cycles());
        }
        ASSERT_TEST_EQ((int)l.size(), 0, desc + ": error");
        l.clear();
    }


    return 0;
}


template<class T>
T* InstT(T* = nullptr)
{
    T* p = new T;
    if (p == NULL)
    {
        return NULL;
    }
    (void)*p;
    return p;
}

template<class T>
void DestroyT(T*p)
{
    delete p;
}


#define LinerStressWrap(T, V, is_static, out_prof) \
do\
{\
    raii_object::reset(); \
    T* c = InstT<T>(); \
    s32 ret = LinerStress<T,V>(*c, #T, is_static, out_prof); \
    DestroyT<T>(c); \
    if (ret != 0) {LogError() <<"error."; return 1;} else {LogDebug() << #T <<"tested.";  std::this_thread::sleep_for(std::chrono::milliseconds(100));}\
    ASSERT_TNAME_RAII_EQUAL(T);\
}\
while(0)


template<class T, class V = raii_object, bool IS_STATIC = false>
s32 LinerDestroyWrap()
{
    raii_object::reset();
    T* c = new T;
    for (int i = 0; i < LOAD_CAPACITY; i++)
    {
        c->push_back(V(i));
    }
    delete c;
    ASSERT_TNAME_RAII_EQUAL(T);
    return 0;
}


template<int M, int F>
s32 TestDynSpaceMemoryLeak()
{
    for (int i = 0; i < M; i++)
    {
        zlist_ext<raii_object, M, F>  z; 
        for (int j = 0; j < i; j++)
        {
            z.push_back(raii_object(i * 10000 + j)); 
        }
    }
    return 0;
}

template<int M>
s32 TestStaticSpaceMemoryLeak()
{
    for (int i = 0; i < M; i++)
    {
        zlist<raii_object, M>  z;
        for (int j = 0; j < i; j++)
        {
            z.push_back(raii_object(i * 10000 + j));
        }
    }
    return 0;
}



s32 contiainer_stress_test()
{
    std::unique_ptr<zmalloc> zstate(new zmalloc());
    memset(zstate.get(), 0, sizeof(zmalloc));
    zstate->max_reserve_block_count_ = 100;
    zstate->set_global(zstate.get());

    TestDynSpaceMemoryLeak<1, 1>();
    TestDynSpaceMemoryLeak<2, 1>();
    TestDynSpaceMemoryLeak<2, 2>();
    TestDynSpaceMemoryLeak<3, 1>();
    TestDynSpaceMemoryLeak<3, 2>();
    TestDynSpaceMemoryLeak<3, 3>();
    TestDynSpaceMemoryLeak<4, 1>();
    TestDynSpaceMemoryLeak<4, 2>();
    TestDynSpaceMemoryLeak<4, 3>();
    TestDynSpaceMemoryLeak<4, 4>();
    TestDynSpaceMemoryLeak<5, 1>();
    TestDynSpaceMemoryLeak<5, 2>();
    TestDynSpaceMemoryLeak<5, 3>();
    TestDynSpaceMemoryLeak<5, 4>();
    TestDynSpaceMemoryLeak<5, 5>();

    TestStaticSpaceMemoryLeak<1>();
    TestStaticSpaceMemoryLeak<2>();
    TestStaticSpaceMemoryLeak<3>();
    TestStaticSpaceMemoryLeak<4>();
    TestStaticSpaceMemoryLeak<5>();
    TestStaticSpaceMemoryLeak<6>();
    TestStaticSpaceMemoryLeak<7>();
    TestStaticSpaceMemoryLeak<8>();

    if (true)
    {
        ASSERT_RAII_EQUAL("empty  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zlist_ext<raii_object, 1000, 200>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(raii_object(i * 10000 + j));
            }
        }
        ASSERT_RAII_EQUAL("zlist_ext  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zlist<raii_object, 1000>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(raii_object(i * 10000 + j));
            }
        }
        ASSERT_RAII_EQUAL("zlist  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zarray<raii_object, 1000>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(raii_object(i * 10000 + j));
            }
        }
        ASSERT_RAII_EQUAL("zarray  memory over test");

        for (int i = 0; i < 1008; i++)
        {
            zvector<raii_object, 1000, 1000>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(raii_object(i * 10000 + j));
            }
        }
        ASSERT_RAII_EQUAL("zvector  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zvector<raii_object, 1000, 0>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(raii_object(i * 10000 + j));
            }
        }
        ASSERT_RAII_EQUAL("zvector  memory over test");
    }



    LogDebug() << "================";

    using int_zarray = zarray<int, LOAD_CAPACITY>;
    using raii_zarray = zarray<raii_object, LOAD_CAPACITY>;
    using int_zvector_0_fixed = zvector<int, LOAD_CAPACITY, 0>;
    using raii_zvector_0_fixed = zvector<raii_object, LOAD_CAPACITY, 0>;
    using int_zvector_half_fixed = zvector<int, LOAD_CAPACITY, LOAD_CAPACITY/2>;
    using raii_zvector_half_fixed = zvector<raii_object, LOAD_CAPACITY, LOAD_CAPACITY/2>;
    using int_zvector_full_fixed = zvector<int, LOAD_CAPACITY, LOAD_CAPACITY>;
    using raii_zvector_full_fixed = zvector<raii_object, LOAD_CAPACITY, LOAD_CAPACITY>;

    using raii_svector = StaticVector<raii_object, LOAD_CAPACITY>;
    using int_svector = StaticVector<int, LOAD_CAPACITY>;

    LinerStressWrap(std::vector<int>, int, false, true);
    LinerStressWrap(int_zarray, int, true, true);
    LinerStressWrap(int_zvector_0_fixed, int, true, true);
    LinerStressWrap(int_zvector_half_fixed, int, true, true);
    LinerStressWrap(int_zvector_full_fixed, int, true, true);
    LinerStressWrap(int_svector, int, false, true);


    LinerStressWrap(std::vector<int>, raii_object, false, false);
    LinerStressWrap(int_zarray, raii_object, true, false);
    LinerStressWrap(int_zvector_0_fixed, raii_object, true, false);
    LinerStressWrap(int_zvector_half_fixed, raii_object, true, false);
    LinerStressWrap(int_zvector_full_fixed, raii_object, true, false);

    LinerDestroyWrap<std::vector<int>, raii_object>();
    LinerDestroyWrap<int_zarray, raii_object, true>();
    LinerDestroyWrap<int_zvector_0_fixed, raii_object, true>();
    LinerDestroyWrap<int_zvector_half_fixed, raii_object, true>();
    LinerDestroyWrap<int_zvector_full_fixed, raii_object, true>();

    LinerStressWrap(std::vector<raii_object>, raii_object, false, true);
    LinerStressWrap(raii_zarray, raii_object, true, true);
    LinerStressWrap(raii_zvector_0_fixed, raii_object, true, true);
    LinerStressWrap(raii_zvector_half_fixed, raii_object, true, true);
    LinerStressWrap(raii_zvector_full_fixed, raii_object, true, true);
    LinerStressWrap(raii_svector, int, false, true);


    LinerStressWrap(std::vector<raii_object>, int, false, false);
    LinerStressWrap(raii_zarray, int, true, false);
    LinerStressWrap(raii_zvector_0_fixed, int, true, false);
    LinerStressWrap(raii_zvector_half_fixed, int, true, false);
    LinerStressWrap(raii_zvector_full_fixed, int, true, false);

    LinerDestroyWrap<std::vector<raii_object>, raii_object>();
    LinerDestroyWrap<raii_zarray, raii_object, true>();
    LinerDestroyWrap<raii_zvector_0_fixed, raii_object, true>();
    LinerDestroyWrap<raii_zvector_half_fixed, raii_object, true>();
    LinerDestroyWrap<raii_zvector_full_fixed, raii_object, true>();


    LogDebug() << "================";
    using int_zlist = zlist<int, LOAD_CAPACITY>;
    using int_fixed_zlist_ext = zlist_ext<int, LOAD_CAPACITY, LOAD_CAPACITY>;
    using int_dyn_zlist_ext = zlist_ext<int, LOAD_CAPACITY, 1>;
    LinerStressWrap(std::deque<int>, int, false, true);
    LinerStressWrap(std::list<int>, int, false, true);
    LinerStressWrap(int_zlist, int, true, true);
    LinerStressWrap(int_fixed_zlist_ext, int, true, true);
    LinerStressWrap(int_dyn_zlist_ext, int, true, true);

    using raii_zlist = zlist<raii_object, LOAD_CAPACITY>;
    using raii_fixed_zlist_ext = zlist_ext<raii_object, LOAD_CAPACITY, LOAD_CAPACITY>;
    using raii_dyn_zlist_ext = zlist_ext<raii_object, LOAD_CAPACITY, 1>;
    LinerStressWrap(std::deque<raii_object>, raii_object, false, true);
    LinerStressWrap(std::list<raii_object>, raii_object, false, true);
    LinerStressWrap(raii_zlist, raii_object, true, true);
    LinerStressWrap(raii_fixed_zlist_ext, raii_object, true, true);
    LinerStressWrap(raii_dyn_zlist_ext, raii_object, true, true);

    LinerDestroyWrap<std::deque<raii_object>, raii_object>();
    LinerDestroyWrap<std::list<raii_object>, raii_object>();
    LinerDestroyWrap<zlist<raii_object, LOAD_CAPACITY>, raii_object, true>();
    LinerDestroyWrap<zlist_ext<raii_object, LOAD_CAPACITY, LOAD_CAPACITY>, raii_object, true>();
    LinerDestroyWrap<zlist_ext<raii_object, LOAD_CAPACITY, 1>, raii_object, true>();

    LogDebug() << "================";
    for (u32 i = 0; i < LOAD_CAPACITY; i++)
    {
        char buf[50];
        sprintf(buf, "%u", i);
        string_data[i] = buf;
    }


    return 0;
}
