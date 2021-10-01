

#include "fn_log.h"
#include "zarray.h"
#include <string>
#include "zlist.h"
#include "zhash_map.h"
#include "zcontain_test.h"
#include "zobj_pool.h"
#include "zlist_stress.h"
#include <unordered_map>

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

static const int LOOP_CAPACITY = 1000;
static const int LOAD_CAPACITY = 10000;
template<class List>
s32 LinerStress(List& l, const std::string& desc)
{
    std::stringstream ss;
    ss << desc << ":" << LOOP_CAPACITY << "/" << LOAD_CAPACITY << ": ";

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "push_back").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            l.push_back(i);
        }
        prof_cost.record_current<LOAD_CAPACITY>();
    }

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "clear").c_str());
        prof_cost.start();
        l.clear();
        prof_cost.record_current<LOAD_CAPACITY>();
    }

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "insert begin & pop begin(capacity)").c_str());
        prof_cost.start();
        for (int loop = 0; loop < 1; loop++)
        {
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.insert(l.begin(), i);
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
    }




    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "push back & pop back(capacity)").c_str());
        prof_cost.start();
        for (int loop = 0; loop < 1; loop++)
        {
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.push_back(i);
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
    }


    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "push back & pop back (one item)").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY * LOOP_CAPACITY; i++)
        {
            l.push_back(i);
            l.pop_back();
        }
        prof_cost.record_current<LOAD_CAPACITY * LOOP_CAPACITY>();
    }


    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "push back & pop back(capacity loop 1000)").c_str());
        prof_cost.start();
        for (int j = 0; j < 1000; j++)
        {
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.push_back(i);
            }
            for (int i = 0; i < LOAD_CAPACITY; i++)
            {
                l.pop_back();
            }
        }
        prof_cost.record_current<LOAD_CAPACITY * 2 * 1000>();
    }

    return 0;
}

template<class Map>
s32 MapStress(Map& m, const std::string& desc)
{
    std::stringstream ss;
    ss << desc << ":" << LOOP_CAPACITY << "/" << LOAD_CAPACITY << ": ";

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "insert").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i, i });
        }
        prof_cost.record_current<LOAD_CAPACITY>();
    }

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "clear").c_str());
        prof_cost.start();
        m.clear();
        prof_cost.record_current<LOAD_CAPACITY>();
    }

    if (true)
    {
        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "insert begin & pop begin(capacity)").c_str());
        prof_cost.start();
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i, i });
        }
        if (m.size() != LOAD_CAPACITY)
        {
            LogError() << " stress test error";
        }
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(i);
        }
        if (m.size() != 0)
        {
            LogError() << " stress test error";
        }
        prof_cost.record_current<LOAD_CAPACITY * 2>();
    }




    if (true)
    {

        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i, i });
        }
        if (m.size() != LOAD_CAPACITY)
        {
            LogError() << " stress test error";
        }

        PROF_DEFINE_REGISTER_DEFAULT(prof_cost, (ss.str() + "find(capacity)").c_str());
        prof_cost.start();
        for (int i = 0; i < 1000 * 10000; i++)
        {
            if (m.find(rand() % LOAD_CAPACITY) == m.end())
            {
                LogError() << " stress test error";
            }
        }
        prof_cost.record_current<1000 * 10000>();
    }


    return 0;
}


s32 ZContainStress()
{
    if (true)
    {
        std::list<int> c;
        LinerStress(c, "std::list<int>");
    }
    if (true)
    {
        std::vector<int> c;
        LinerStress(c, "std::vector<int>");
    }
    if (true)
    {
        std::deque<int> c;
        LinerStress(c, "std::vector<int>");
    }
    if (true)
    {
        zlist<int, LOAD_CAPACITY> c;
        LinerStress(c, "zlist<int, LOAD_CAPACITY>");
    }
    if (true)
    {
        zarray<int, LOAD_CAPACITY> c;
        LinerStress(c, "zarray<int, LOAD_CAPACITY>");
    }
    if (true)
    {
        std::map<int, int>  m;
        MapStress(m, "std::map<int, int>");
    }
    if (true)
    {
        std::unordered_map<int, int>  m;
        MapStress(m, "std::unordered_map<int, int>");
    }
    if (true)
    {
        zhash_map<int, int, LOAD_CAPACITY> m;
        MapStress(m, "zhash_map<int, int, LOAD_CAPACITY>");
    }
    return 0;
}
