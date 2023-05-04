


#include "fn_log.h"
#include "zarray.h"
#include "zvector.h"
#include <string>
#include "zlist.h"
#include "zlist_ext.h"
#include "zhash_map.h"
#include "zcontain_test.h"
#include "zlist_stress.h"
#include <unordered_map>
#include "zprof.h"
#include "static_vector.h"
#include "zmalloc.h"
#include "zallocator.h"




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
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ":has error");

    }

    if (true)
    {
        PROF_START_COUNTER(cost);
        m.clear();
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "clear").c_str(), 1, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");

    }

    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i,  V(i) });
        }
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(i);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");
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
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(LOAD_CAPACITY - i - 1);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "revert erase key (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");
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
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(m.begin());
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase begin (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");
    }


    if (true)
    {

        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i,  V(i) });
        }
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
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
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ":has error");

    }

    if (true)
    {
        PROF_START_COUNTER(cost);
        m.clear();
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "clear").c_str(), 1, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");

    }

    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ string_data[i],  string_data[i] });
        }
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(string_data[i]);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");
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
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(string_data[LOAD_CAPACITY - i - 1]);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "revert erase key (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");
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
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(m.begin());
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase begin (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");
    }


    if (true)
    {

        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ string_data[i],   string_data[i] });
        }
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
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
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ":has error");

    }

    if (true)
    {
        PROF_START_COUNTER(cost);
        m.clear();
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "clear").c_str(), 1, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");

    }

    if (true)
    {
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert(V(i));
        }
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(i);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");
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
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(V(i));
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "revert erase key (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");
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
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(m.begin());
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase begin (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cycles());
        ASSERT_TEST_EQ((int)m.size(), 0, desc + ":has error");
    }


    if (true)
    {

        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert(V(i));
        }
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
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
    RAIIVal<>::reset(); \
    T* c = InstT<T>(); \
    s32 ret = LinerStress<T,V>(*c, #T, is_static, out_prof); \
    DestroyT<T>(c); \
    if (ret != 0) {LogError() <<"error."; return 1;} else {LogDebug() << #T <<"tested.";  std::this_thread::sleep_for(std::chrono::milliseconds(100));}\
    CheckRAIIValByType(T);\
}\
while(0)


template<class T, class V = RAIIVal<>, bool IS_STATIC = false>
s32 LinerDestroyWrap()
{
    RAIIVal<>::reset();
    T* c = new T;
    for (int i = 0; i < LOAD_CAPACITY; i++)
    {
        c->push_back(V(i));
    }
    delete c;
    CheckRAIIValByType(T);
    return 0;
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
s32 MapDestroyWrap()
{
    RAIIVal<>::reset();
    T* c = new T;
    for (int i = 0; i < LOAD_CAPACITY; i++)
    {
        c->insert(std::make_pair(i, V(i)));
    }
    delete c;
    CheckRAIIValByType(T);
    return 0;
}

template<int M, int F>
s32 TestDynSpaceMemoryLeak()
{
    for (int i = 0; i < M; i++)
    {
        zlist_ext<RAIIVal<>, M, F>  z; 
        for (int j = 0; j < i; j++)
        {
            z.push_back(RAIIVal<>(i * 10000 + j)); 
        }
    }
    return 0;
}

template<int M>
s32 TestStaticSpaceMemoryLeak()
{
    for (int i = 0; i < M; i++)
    {
        zlist<RAIIVal<>, M>  z;
        for (int j = 0; j < i; j++)
        {
            z.push_back(RAIIVal<>(i * 10000 + j));
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
        ASSERT_RAII_VAL("empty  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zlist_ext<RAIIVal<>, 1000, 200>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(RAIIVal<>(i * 10000 + j));
            }
        }
        ASSERT_RAII_VAL("zlist_ext  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zlist<RAIIVal<>, 1000>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(RAIIVal<>(i * 10000 + j));
            }
        }
        ASSERT_RAII_VAL("zlist  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zarray<RAIIVal<>, 1000>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(RAIIVal<>(i * 10000 + j));
            }
        }
        ASSERT_RAII_VAL("zarray  memory over test");

        for (int i = 0; i < 1008; i++)
        {
            zvector<RAIIVal<>, 1000, 1000>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(RAIIVal<>(i * 10000 + j));
            }
        }
        ASSERT_RAII_VAL("zvector  memory over test");
        for (int i = 0; i < 1008; i++)
        {
            zvector<RAIIVal<>, 1000, 0>  z;
            for (int j = 0; j < i; j++)
            {
                z.push_back(RAIIVal<>(i * 10000 + j));
            }
        }
        ASSERT_RAII_VAL("zvector  memory over test");
    }



    LogDebug() << "================";

    using int_zarray = zarray<int, LOAD_CAPACITY>;
    using raii_zarray = zarray<RAIIVal<>, LOAD_CAPACITY>;
    using int_zvector_0_fixed = zvector<int, LOAD_CAPACITY, 0>;
    using raii_zvector_0_fixed = zvector<RAIIVal<>, LOAD_CAPACITY, 0>;
    using int_zvector_half_fixed = zvector<int, LOAD_CAPACITY, LOAD_CAPACITY/2>;
    using raii_zvector_half_fixed = zvector<RAIIVal<>, LOAD_CAPACITY, LOAD_CAPACITY/2>;
    using int_zvector_full_fixed = zvector<int, LOAD_CAPACITY, LOAD_CAPACITY>;
    using raii_zvector_full_fixed = zvector<RAIIVal<>, LOAD_CAPACITY, LOAD_CAPACITY>;

    using raii_svector = StaticVector<RAIIVal<>, LOAD_CAPACITY>;
    using int_svector = StaticVector<int, LOAD_CAPACITY>;

    LinerStressWrap(std::vector<int>, int, false, true);
    LinerStressWrap(int_zarray, int, true, true);
    LinerStressWrap(int_zvector_0_fixed, int, true, true);
    LinerStressWrap(int_zvector_half_fixed, int, true, true);
    LinerStressWrap(int_zvector_full_fixed, int, true, true);
    LinerStressWrap(int_svector, int, false, true);


    LinerStressWrap(std::vector<int>, RAIIVal<>, false, false);
    LinerStressWrap(int_zarray, RAIIVal<>, true, false);
    LinerStressWrap(int_zvector_0_fixed, RAIIVal<>, true, false);
    LinerStressWrap(int_zvector_half_fixed, RAIIVal<>, true, false);
    LinerStressWrap(int_zvector_full_fixed, RAIIVal<>, true, false);

    LinerDestroyWrap<std::vector<int>, RAIIVal<>>();
    LinerDestroyWrap<int_zarray, RAIIVal<>, true>();
    LinerDestroyWrap<int_zvector_0_fixed, RAIIVal<>, true>();
    LinerDestroyWrap<int_zvector_half_fixed, RAIIVal<>, true>();
    LinerDestroyWrap<int_zvector_full_fixed, RAIIVal<>, true>();

    LinerStressWrap(std::vector<RAIIVal<>>, RAIIVal<>, false, true);
    LinerStressWrap(raii_zarray, RAIIVal<>, true, true);
    LinerStressWrap(raii_zvector_0_fixed, RAIIVal<>, true, true);
    LinerStressWrap(raii_zvector_half_fixed, RAIIVal<>, true, true);
    LinerStressWrap(raii_zvector_full_fixed, RAIIVal<>, true, true);
    LinerStressWrap(raii_svector, int, false, true);


    LinerStressWrap(std::vector<RAIIVal<>>, int, false, false);
    LinerStressWrap(raii_zarray, int, true, false);
    LinerStressWrap(raii_zvector_0_fixed, int, true, false);
    LinerStressWrap(raii_zvector_half_fixed, int, true, false);
    LinerStressWrap(raii_zvector_full_fixed, int, true, false);

    LinerDestroyWrap<std::vector<RAIIVal<>>, RAIIVal<>>();
    LinerDestroyWrap<raii_zarray, RAIIVal<>, true>();
    LinerDestroyWrap<raii_zvector_0_fixed, RAIIVal<>, true>();
    LinerDestroyWrap<raii_zvector_half_fixed, RAIIVal<>, true>();
    LinerDestroyWrap<raii_zvector_full_fixed, RAIIVal<>, true>();


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
    using std_int_int_map_zallocator = std::map<int, int, std::less<int>, zallocator<std::pair<const int, int>>>;
    using std_int_int_unordered_map = std::unordered_map<int, int>;
    using z_int_int_hash_map = zhash_map<int, int, LOAD_CAPACITY>;
    using z_int_int_hash_map_zhash = zhash_map<int, int, LOAD_CAPACITY, zhash<int>>;

    using std_int_raii_map = std::map<int, RAIIVal<>>;
    using std_int_raii_unordered_map = std::unordered_map<int, RAIIVal<>>;
    using z_int_raii_hash_map = zhash_map<int, RAIIVal<>, LOAD_CAPACITY>;
    using z_int_raii_hash_map_zhash = zhash_map<int, RAIIVal<>, LOAD_CAPACITY, zhash<int>>;

    MapStressWrap(std_int_int_map, int, false);
    MapStressWrap(std_int_int_map_zallocator, int, false);
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
        std::unordered_map<int, int> sys_map;
        zhash_map<int, int, LOAD_CAPACITY> z_hashmap;
        std::unordered_map<int, int> sys_hashmap;
        for (int i = 0; i < LOAD_CAPACITY / 3; i++)
        {
            sys_hashmap.insert(std::make_pair(i, i));
            sys_map.insert(std::make_pair(i, i));
            z_hashmap.insert(std::make_pair(i, i));
        }
        ASSERT_TEST_EQ(sys_map.size(), (u32)LOAD_CAPACITY / 3, "");
        ASSERT_TEST_EQ(sys_hashmap.size(), (u32)LOAD_CAPACITY / 3, "");
        ASSERT_TEST_EQ(z_hashmap.size(), (u32)LOAD_CAPACITY / 3, "");

        int sys_count = 0;
        int z_count = 0;
        int sys_map_count = 0;
        if (true)
        {

            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, LOOP_CAPACITY, "sys map foreach");
            for (int i = 0; i < LOOP_CAPACITY; i++)
            {
                for (auto& kv : sys_map)
                {
                    sys_map_count += kv.second;
                }
            }
        }
        if (true)
        {
            
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, LOOP_CAPACITY, "sys hash_map foreach");
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
            
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, LOOP_CAPACITY, "z hash_map foreach");
            for (int i = 0; i < LOOP_CAPACITY; i++)
            {
                for (auto& kv : z_hashmap)
                {
                    z_count += kv.second;
                }
            }
        }
        ASSERT_TEST_EQ(sys_count, z_count, "");
        ASSERT_TEST_EQ(sys_map_count, z_count, "");
    }

    return 0;
}
