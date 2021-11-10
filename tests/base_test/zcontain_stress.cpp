


#include "fn_log.h"
#include "zarray.h"
#include <string>
#include "zlist.h"
#include "zlist_ext.h"
#include "zhash_map.h"
#include "zcontain_test.h"
#include "zobj_pool.h"
#include "zlist_stress.h"
#include <unordered_map>
#include "zprof.h"
#include "static_vector.h"

using namespace zsummer;




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
        AssertCheck((int)l.size(), LOAD_CAPACITY, desc + ": error");

        int i = 0;
        for (auto &v : l)
        {
            if ((v) != i)
            {
                volatile int a = i;
                (void)a;
            }
            AssertCheck((int)v, i, desc + ": error");
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
        AssertCheck((int)l.size(), 0, desc + ": error");
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
        AssertCheck((int)l.size(), 0, desc + ": error");
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
        AssertCheck((int)l.size(), 0, desc + ": error");

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
        AssertCheck((int)l.size(), 0, desc + ": error");

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
        AssertCheck((int)l.size(), 0, desc + ": error");

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
        AssertCheck((int)l.size(), 0, desc + ": error");

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
        AssertCheck((int)l.size(), 0, desc + ": error");

    }

    return 0;
}

template<class Map, class V>
s32 MapStress(Map& m, const std::string& desc, bool is_static = false)
{
    std::stringstream ss;
    ss << desc << ":" <<  LOAD_CAPACITY << ": ";
    PROF_DEFINE_COUNTER(cost);
    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i, V(i) });
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "insert").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        if (is_static)
        {
            m.insert({ LOAD_CAPACITY, V(LOAD_CAPACITY) });
            m.insert({ LOAD_CAPACITY + 1,  V(LOAD_CAPACITY + 1) });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ":has error");

    }

    if (true)
    {
        PROF_START_COUNTER(cost);
        m.clear();
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "clear").c_str(), 1, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");

    }

    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i,  V(i) });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(i);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(i);
        }
        if (is_static)
        {
            m.erase(m.end());
        }
    }
    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i,  V(i) });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(LOAD_CAPACITY - i - 1);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "revert erase key (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(i);
        }
        if (is_static)
        {
            m.erase(m.end());
        }
    }
    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i,  V(i) });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(m.begin());
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase begin (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");
    }


    if (true)
    {

        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i,  V(i) });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < 1000 * 10000; i++)
        {
            if (m.find(i % LOAD_CAPACITY) == m.end())
            {
                LogError() << " stress test error";
            }
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "find(capacity)").c_str(), 1000 * 10000, cost.stop_and_save().cycles());
        PROF_START_COUNTER(cost);
        for (int i = 0; i < 1000 * 10000; i++)
        {
            if (m[i % LOAD_CAPACITY] != i % LOAD_CAPACITY)
            {
                LogError() << " stress test error";
            }
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "[](capacity)").c_str(), 1000 * 10000, cost.stop_and_save().cycles());
    }


    return 0;
}




template<class Map, class V>
s32 MapStringStress(Map& m, const std::string& desc, bool is_static = false)
{
    std::stringstream ss;
    ss << desc << ":" <<  LOAD_CAPACITY << ": ";
    PROF_DEFINE_COUNTER(cost);
    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ string_data[i], string_data[i] });
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "insert").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ":has error");

    }

    if (true)
    {
        PROF_START_COUNTER(cost);
        m.clear();
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "clear").c_str(), 1, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");

    }

    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ string_data[i],  string_data[i] });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(string_data[i]);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(string_data[i]);
        }
        if (is_static)
        {
            m.erase(m.end());
        }
    }
    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ string_data[i],   string_data[i] });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(string_data[LOAD_CAPACITY - i - 1]);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "revert erase key (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(string_data[i]);
        }
        if (is_static)
        {
            m.erase(m.end());
        }
    }
    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ string_data[i],  string_data[i] });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(m.begin());
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase begin (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");
    }


    if (true)
    {

        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ string_data[i],   string_data[i] });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < 1000 * 10000; i++)
        {
            if (m.find(string_data[i % LOAD_CAPACITY]) == m.end())
            {
                LogError() << " stress test error";
            }
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "find(capacity)").c_str(), 1000 * 10000, cost.stop_and_save().cycles());
    }


    return 0;
}



template<class Map, class V>
s32 SetStress(Map& m, const std::string& desc, bool is_static = false)
{
    std::stringstream ss;
    ss << desc << ":" << LOAD_CAPACITY << ": ";
    PROF_DEFINE_COUNTER(cost);
    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert(V(i));
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "insert").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        if (is_static)
        {
            m.insert(V(0));
            m.insert(V(0));
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ":has error");

    }

    if (true)
    {
        PROF_START_COUNTER(cost);
        m.clear();
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "clear").c_str(), 1, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");

    }

    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert(V(i));
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(i);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(i);
        }
        if (is_static)
        {
            m.erase(m.end());
        }
    }
    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert( V(i) );
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(V(i));
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "revert erase key (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(V(i));
        }
        if (is_static)
        {
            m.erase(m.end());
        }
    }
    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert(V(i));
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(m.begin());
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase begin (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        AssertCheck((int)m.size(), 0, desc + ":has error");
    }


    if (true)
    {

        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert(V(i));
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < 1000 * 10000; i++)
        {
            if (m.find(i % LOAD_CAPACITY) == m.end())
            {
                LogError() << " stress test error";
            }
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "find(capacity)").c_str(), 1000 * 10000, cost.stop_and_save().cycles());
    }


    return 0;
}




#define LinerStressWrap(T, V, is_static, out_prof) \
do\
{\
RAIIVal<>::reset(); \
T* c = new T; \
LinerStress<T,V>(*c, #T, is_static, out_prof); \
delete c; \
CheckRAIIValByType(T);\
}\
while(0)


template<class T, class V = RAIIVal<>, bool IS_STATIC = false>
void LinerDestroyWrap()
{
    RAIIVal<>::reset();
    T* c = new T;
    for (int i = 0; i < LOAD_CAPACITY; i++)
    {
        c->push_back(V(i));
    }
    delete c;
    CheckRAIIValByType(T);
}

#define MapStressWrap(T, V, is_static) \
do\
{\
RAIIVal<>::reset(); \
T* c = new T; \
MapStress<T,V>(*c, #T, is_static); \
delete c; \
CheckRAIIValByType(T);\
}\
while(0)

#define MapStringStressWrap(T, V, is_static) \
do\
{\
RAIIVal<>::reset(); \
T* c = new T; \
MapStringStress<T,V>(*c, #T, is_static); \
delete c; \
CheckRAIIValByType(T);\
}\
while(0)

#define SetStressWrap(T, V, is_static) \
do\
{\
RAIIVal<>::reset(); \
T* c = new T; \
SetStress<T,V>(*c, #T, is_static); \
delete c; \
CheckRAIIValByType(T);\
}\
while(0)

template<class T, class V = RAIIVal<>, bool IS_STATIC = false>
void MapDestroyWrap()
{
    RAIIVal<>::reset();
    T* c = new T;
    for (int i = 0; i < LOAD_CAPACITY; i++)
    {
        c->insert(std::make_pair(i, V(i)));
    }
    delete c;
    CheckRAIIValByType(T);
}

template<int M, int F>
void TestDynSpaceMemoryLeak()
{
    for (int i = 0; i < M; i++)
    {
        zlist_ext<RAIIVal<>, M, F>  z; 
        for (int j = 0; j < i; j++)
        {
            z.push_back(RAIIVal<>(i * 10000 + j)); 
        }
    }
}

template<int M>
void TestStaticSpaceMemoryLeak()
{
    for (int i = 0; i < M; i++)
    {
        zlist<RAIIVal<>, M>  z;
        for (int j = 0; j < i; j++)
        {
            z.push_back(RAIIVal<>(i * 10000 + j));
        }
    }
}



s32 contiainer_stress_test()
{
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
        CheckRAIIVal("empty  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zlist_ext<RAIIVal<>, 1000, 200>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(RAIIVal<>(i * 10000 + j));
            }
        }
        CheckRAIIVal("zlist_ext  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zlist<RAIIVal<>, 1000>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(RAIIVal<>(i * 10000 + j));
            }
        }
        CheckRAIIVal("zlist  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zarray<RAIIVal<>, 1000>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(RAIIVal<>(i * 10000 + j));
            }
        }
        CheckRAIIVal("zlist  memory over test");
    }



    LogDebug() << "================";

    using int_zarray = zarray<int, LOAD_CAPACITY>;
    using raii_zarray = zarray<RAIIVal<>, LOAD_CAPACITY>;
    using raii_svector = StaticVector<RAIIVal<>, LOAD_CAPACITY>;
    using int_svector = StaticVector<int, LOAD_CAPACITY>;

    LinerStressWrap(std::vector<int>, int, false, true);
    LinerStressWrap(int_zarray, int, true, true);
    LinerStressWrap(int_svector, int, false, true);


    LinerStressWrap(std::vector<int>, RAIIVal<>, false, false);
    LinerStressWrap(int_zarray, RAIIVal<>, true, false);

    LinerDestroyWrap<std::vector<int>, RAIIVal<>>();
    LinerDestroyWrap<zarray<int, LOAD_CAPACITY>, RAIIVal<>, true>();

    LinerStressWrap(std::vector<RAIIVal<>>, RAIIVal<>, false, true);
    LinerStressWrap(raii_zarray, RAIIVal<>, true, true);
    LinerStressWrap(raii_svector, int, false, true);


    LinerStressWrap(std::vector<RAIIVal<>>, int, false, false);
    LinerStressWrap(raii_zarray, int, true, false);

    LinerDestroyWrap<std::vector<RAIIVal<>>, RAIIVal<>>();
    LinerDestroyWrap<zarray<RAIIVal<>, LOAD_CAPACITY>, RAIIVal<>, true>();
    

    LogDebug() << "================";
    using int_zlist = zlist<int, LOAD_CAPACITY>;
    using int_fixed_zlist_ext = zlist_ext<int, LOAD_CAPACITY, LOAD_CAPACITY>;
    using int_dyn_zlist_ext = zlist_ext<int, LOAD_CAPACITY, 1>;
    LinerStressWrap(std::deque<int>, int, false, true);
    LinerStressWrap(std::list<int>, int, false, true);
    LinerStressWrap(int_zlist, int, true, true);
    LinerStressWrap(int_fixed_zlist_ext, int, true, true);
    LinerStressWrap(int_dyn_zlist_ext, int, true, true);

    using raii_zlist = zlist<RAIIVal<>, LOAD_CAPACITY>;
    using raii_fixed_zlist_ext = zlist_ext<RAIIVal<>, LOAD_CAPACITY, LOAD_CAPACITY>;
    using raii_dyn_zlist_ext = zlist_ext<RAIIVal<>, LOAD_CAPACITY, 1>;
    LinerStressWrap(std::deque<RAIIVal<>>, RAIIVal<>, false, true);
    LinerStressWrap(std::list<RAIIVal<>>, RAIIVal<>, false, true);
    LinerStressWrap(raii_zlist, RAIIVal<>, true, true);
    LinerStressWrap(raii_fixed_zlist_ext, RAIIVal<>, true, true);
    LinerStressWrap(raii_dyn_zlist_ext, RAIIVal<>, true, true);

    LinerDestroyWrap<std::deque<RAIIVal<>>, RAIIVal<>>();
    LinerDestroyWrap<std::list<RAIIVal<>>, RAIIVal<>>();
    LinerDestroyWrap<zlist<RAIIVal<>, LOAD_CAPACITY>, RAIIVal<>, true>();
    LinerDestroyWrap<zlist_ext<RAIIVal<>, LOAD_CAPACITY, LOAD_CAPACITY>, RAIIVal<>, true>();
    LinerDestroyWrap<zlist_ext<RAIIVal<>, LOAD_CAPACITY, 1>, RAIIVal<>, true>();

    LogDebug() << "================";
    for (u32 i = 0; i < LOAD_CAPACITY; i++)
    {
        char buf[50];
        sprintf(buf, "%u", i);
        string_data[i] = buf;
    }

    using std_int_int_map = std::map<int, int>;
    using std_int_int_unordered_map = std::unordered_map<int, int>;
    using z_int_int_hash_map = zhash_map<int, int, LOAD_CAPACITY>;
    using z_int_int_hash_map_zhash = zhash_map<int, int, LOAD_CAPACITY, zhash<int>>;

    using std_int_raii_map = std::map<int, RAIIVal<>>;
    using std_int_raii_unordered_map = std::unordered_map<int, RAIIVal<>>;
    using z_int_raii_hash_map = zhash_map<int, RAIIVal<>, LOAD_CAPACITY>;
    using z_int_raii_hash_map_zhash = zhash_map<int, RAIIVal<>, LOAD_CAPACITY, zhash<int>>;

    MapStressWrap(std_int_int_map, int, false);
    MapStressWrap(std_int_int_unordered_map, int, false);
    MapStressWrap(z_int_int_hash_map, int, true);
    MapStressWrap(z_int_int_hash_map_zhash, int, true);

    MapStressWrap(std_int_raii_map, RAIIVal<>, false);
    MapStressWrap(std_int_raii_unordered_map, RAIIVal<>, false);
    MapStressWrap(z_int_raii_hash_map, RAIIVal<>, true);
    MapStressWrap(z_int_raii_hash_map_zhash, RAIIVal<>, true);


    using std_str_str_map = std::map<std::string, std::string>;
    using std_str_str_unordered_map = zhash_map<std::string, std::string, LOAD_CAPACITY>;
    using z_str_str_hash_map = zhash_map<std::string, std::string, LOAD_CAPACITY>;

    MapStringStressWrap(std_str_str_map, std::string, false);
    MapStringStressWrap(std_str_str_unordered_map, std::string, false);
    MapStringStressWrap(z_str_str_hash_map, std::string, true);

    using std_int_set = std::set<int>;
    using std_int_unordered_set = zhash_set<int, LOAD_CAPACITY>;
    using z_int_hash_set = zhash_set<int, LOAD_CAPACITY>;
    using z_int_hash_set_zhash = zhash_set<int, LOAD_CAPACITY, zhash<int>>;

    SetStressWrap(std_int_set, int, false);
    SetStressWrap(std_int_unordered_set, int, false);
    SetStressWrap(z_int_hash_set, int, true);
    SetStressWrap(z_int_hash_set_zhash, int, true);


    MapDestroyWrap<std::map<int, RAIIVal<>>, RAIIVal<>, false>();
    MapDestroyWrap<std::unordered_map<int, RAIIVal<>>, RAIIVal<>, false>();
    MapDestroyWrap<zhash_map<int, RAIIVal<>, LOAD_CAPACITY>, RAIIVal<>, true>();
    LogDebug() << "================";

    if (true)
    {
        zhash_map<int, int, LOAD_CAPACITY> z_hashmap;
        for (int i = 0; i < LOAD_CAPACITY/3; i++)
        {
            z_hashmap.insert(std::make_pair(i, i));
        }
        AssertTest(z_hashmap.size(), (u32)LOAD_CAPACITY / 3, "");
        std::unordered_map<int, int> sys_hashmap;
        for (int i = 0; i < LOAD_CAPACITY / 3; i++)
        {
            sys_hashmap.insert(std::make_pair(i, i));
        }
        AssertTest(sys_hashmap.size(), (u32)LOAD_CAPACITY / 3, "");

        int sys_count = 0;
        int z_count = 0;
        if (true)
        {
            
            PROF_DEFINE_AUTO_SINGLE_RECORD(cost, LOOP_CAPACITY, PROF_LEVEL_NORMAL, "sys hash_map foreach");
            for (int i = 0; i < LOOP_CAPACITY; i++)
            {
                for (auto&kv: sys_hashmap)
                {
                    sys_count +=kv.second;
                }
            }
        }
        if (true)
        {
            
            PROF_DEFINE_AUTO_SINGLE_RECORD(cost, LOOP_CAPACITY, PROF_LEVEL_NORMAL, "z hash_map foreach");
            for (int i = 0; i < LOOP_CAPACITY; i++)
            {
                for (auto& kv : z_hashmap)
                {
                    z_count += kv.second;
                }
            }
        }
        AssertTest(sys_count, z_count, "");
    }

    return 0;
}
