


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

using namespace zsummer;




#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()                                

static const int LOOP_CAPACITY = 1000;
static const int LOAD_CAPACITY = 1024*8;
template<class List, class V>
s32 LinerStress(List& l, const std::string& desc,  bool is_static = false, V*p = NULL)
{
    std::stringstream ss;
    ss << desc << ":" << LOAD_CAPACITY  << ": ";

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "push_back").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            l.push_back(V(i));
        }
        prof_cost.record_current<LOAD_CAPACITY>();
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
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "clear").c_str());
        prof_cost.start();
        l.clear();
        prof_cost.record_current<LOAD_CAPACITY>();
        AssertCheck((int)l.size(), 0, desc + ": error");
        if (is_static)
        {
            l.erase(l.end());
        }
    }



    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "insert begin & pop begin(capacity)").c_str());
        prof_cost.start();
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
        prof_cost.record_current<LOAD_CAPACITY * 2>();
        AssertCheck((int)l.size(), 0, desc + ": error");
    }

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "insert begin ").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            l.insert(l.begin(), V(i));
        }
        prof_cost.record_current<LOAD_CAPACITY * 2>();
    }

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "pop begin ").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            l.erase(l.begin());
        }
        prof_cost.record_current<LOAD_CAPACITY * 2>();
        AssertCheck((int)l.size(), 0, desc + ": error");
    }


    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "insert end & pop end(capacity)").c_str());
        prof_cost.start();
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
        prof_cost.record_current<LOAD_CAPACITY * 2>();
        if (is_static)
        {
            l.erase(l.end());
        }
        AssertCheck((int)l.size(), 0, desc + ": error");
    }



    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "push back & pop back(capacity)").c_str());
        prof_cost.start();
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
        prof_cost.record_current<LOAD_CAPACITY * 2>();
        AssertCheck((int)l.size(), 0, desc + ": error");
    }


    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "push back & pop back (one item)").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY * LOOP_CAPACITY; i++)
        {
            l.push_back(V(i));
            l.pop_back();
        }
        prof_cost.record_current<LOAD_CAPACITY * LOOP_CAPACITY>();
        AssertCheck((int)l.size(), 0, desc + ": error");
    }


    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "push back & pop back(capacity loop 1000)").c_str());
        prof_cost.start();
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
        prof_cost.record_current<LOAD_CAPACITY * 2 * 1000>();
        AssertCheck((int)l.size(), 0, desc + ": error");
    }

    return 0;
}

template<class Map, class V>
s32 MapStress(Map& m, const std::string& desc, bool is_static = false, V* p = NULL)
{
    std::stringstream ss;
    ss << desc << ":" <<  LOAD_CAPACITY << ": ";

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "insert").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i, V(i) });
        }
        prof_cost.record_current<LOAD_CAPACITY>();
       
        if (is_static)
        {
            m.insert({ LOAD_CAPACITY, V(LOAD_CAPACITY) });
            m.insert({ LOAD_CAPACITY + 1,  V(LOAD_CAPACITY + 1) });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ":has error");
    }

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "clear").c_str());
        prof_cost.start();
        m.clear();
        prof_cost.record_current<LOAD_CAPACITY>();
        AssertCheck((int)m.size(), 0, desc + ":has error");

    }

    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i,  V(i) });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "erase key (capacity)").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(i);
        }
        prof_cost.record_current<LOAD_CAPACITY>();
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
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "revert erase key (capacity)").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(LOAD_CAPACITY - i - 1);
        }
        prof_cost.record_current<LOAD_CAPACITY>();
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
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "erase begin (capacity)").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(m.begin());
        }
        prof_cost.record_current<LOAD_CAPACITY>();
        AssertCheck((int)m.size(), 0, desc + ":has error");
    }


    if (true)
    {

        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i,  V(i) });
        }
        AssertCheck((int)m.size(), LOAD_CAPACITY, desc + ": error");

        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "find(capacity)").c_str());
        prof_cost.start();
        for (int i = 0; i < 1000 * 10000; i++)
        {
            int r = rand() % LOAD_CAPACITY;
            if (m.find(r) == m.end())
            {
                LogError() << " stress test error";
            }
        }
        prof_cost.record_current<1000 * 10000>();
    }


    return 0;
}

template<class T,  class V = RAIIVal<>,  bool IS_STATIC = false>
void LinerStressWrap()
{
    T* c = new T;
    LinerStress(*c, TypeName<T>(), IS_STATIC, (V*)NULL);
    delete c; 
    
    CheckRAIIValByType(T);
}

template<class T, class V = RAIIVal<>, bool IS_STATIC = false>
void LinerDestroyWrap()
{
    T* c = new T;
    for (int i = 0; i < LOAD_CAPACITY; i++)
    {
        c->push_back(V(i));
    }
    delete c;
    CheckRAIIValByType(T);
}

template<class T, class V = RAIIVal<>, bool IS_STATIC = false>
void MapStressWrap()
{
    T* c = new T;
    MapStress(*c, TypeName<T>(), IS_STATIC, (V*)NULL);
    delete c;
    CheckRAIIValByType(T);
}

template<class T, class V = RAIIVal<>, bool IS_STATIC = false>
void MapDestroyWrap()
{
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



s32 ContainerStress()
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


    LinerStressWrap<std::vector<int>, int>();
    LinerStressWrap<zarray<int, LOAD_CAPACITY>, int, true>();
    LinerStressWrap<std::vector<int>, RAIIVal<>>();
    LinerStressWrap<zarray<int, LOAD_CAPACITY>, RAIIVal<>, true>();
    LinerDestroyWrap<std::vector<int>, RAIIVal<>>();
    LinerDestroyWrap<zarray<int, LOAD_CAPACITY>, RAIIVal<>, true>();

    LinerStressWrap<std::vector<RAIIVal<>>, int>();
    LinerStressWrap<zarray<RAIIVal<>, LOAD_CAPACITY>, int, true>();
    LinerStressWrap<std::vector<RAIIVal<>>, RAIIVal<>>();
    LinerStressWrap<zarray<RAIIVal<>, LOAD_CAPACITY>, RAIIVal<>, true>();
    LinerDestroyWrap<std::vector<RAIIVal<>>, RAIIVal<>>();
    LinerDestroyWrap<zarray<RAIIVal<>, LOAD_CAPACITY>, RAIIVal<>, true>();

    LogDebug() << "================";
    LinerStressWrap<std::deque<int>, int>();
    LinerStressWrap<zlist<int, LOAD_CAPACITY>, int, true>();
    LinerStressWrap<zlist_ext<int, LOAD_CAPACITY, LOAD_CAPACITY>, int, true>();
    LinerStressWrap<zlist_ext<int, LOAD_CAPACITY, 1>, int, true>();


    LinerStressWrap<std::deque<RAIIVal<>>, RAIIVal<>>();
    LinerStressWrap<zlist<RAIIVal<>, LOAD_CAPACITY>, RAIIVal<>, true>();
    LinerStressWrap<zlist_ext<RAIIVal<>, LOAD_CAPACITY, LOAD_CAPACITY>, RAIIVal<>, true>();
    LinerStressWrap<zlist_ext<RAIIVal<>, LOAD_CAPACITY, 1>, RAIIVal<>, true>();

    LinerDestroyWrap<std::deque<RAIIVal<>>, RAIIVal<>>();
    LinerDestroyWrap<zlist<RAIIVal<>, LOAD_CAPACITY>, RAIIVal<>, true>();
    LinerDestroyWrap<zlist_ext<RAIIVal<>, LOAD_CAPACITY, LOAD_CAPACITY>, RAIIVal<>, true>();
    LinerDestroyWrap<zlist_ext<RAIIVal<>, LOAD_CAPACITY, 1>, RAIIVal<>, true>();

    LogDebug() << "================";

    MapStressWrap<std::map<int, int>, int, false>();
    MapStressWrap<std::unordered_map<int, int>, int, false>();
    MapStressWrap<zhash_map<int, int, LOAD_CAPACITY>, int, true>();

    MapStressWrap<std::map<int, RAIIVal<>>, RAIIVal<>, false>();
    MapStressWrap<std::unordered_map<int, RAIIVal<>>, RAIIVal<>, false>();
    MapStressWrap<zhash_map<int, RAIIVal<>, LOAD_CAPACITY>, RAIIVal<>, true>();

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
        AssertTest(z_hashmap.size(), LOAD_CAPACITY / 3, "");
        std::unordered_map<int, int> sys_hashmap;
        for (int i = 0; i < LOAD_CAPACITY / 3; i++)
        {
            sys_hashmap.insert(std::make_pair(i, i));
        }
        AssertTest(sys_hashmap.size(), LOAD_CAPACITY / 3, "");

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
