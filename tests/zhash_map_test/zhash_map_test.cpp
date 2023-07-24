
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zallocator.h"


s32 hash_map_base_test()
{
    zhash_map<int, int, 2> hash = { {1,1}, {2,2}, {3,3}, {4,4} };
    ASSERT_TEST_EQ(hash.size(), 2U, "");
    ASSERT_TEST_EQ(hash.insert({ 8,8 }).second, false, "");
    ASSERT_TEST_EQ(hash.insert({ 8,8 }).first == hash.end(), true, "");
    hash[1] = 111;
    return 0;
}


static const int LOOP_CAPACITY = 1000;
static const int LOAD_CAPACITY = 1024 * 8;

std::string string_data[LOAD_CAPACITY];



template<class Map, class V>
s32 MapStress(Map& m, const std::string& desc, bool is_static = false)
{
    std::stringstream ss;
    ss << desc << ":" << LOAD_CAPACITY << ": ";
    PROF_DEFINE_COUNTER(cost);
    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ i, V(i) });
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "insert").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "clear").c_str(), 1, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "revert erase key (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase begin (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "find(capacity)").c_str(), 1000 * 10000, cost.stop_and_save().cost());
        PROF_START_COUNTER(cost);
        for (int i = 0; i < 1000 * 10000; i++)
        {
            if (m[i % LOAD_CAPACITY] != i % LOAD_CAPACITY)
            {
                LogError() << " stress test error";
            }
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "[](capacity)").c_str(), 1000 * 10000, cost.stop_and_save().cost());
    }


    return 0;
}




template<class Map, class V>
s32 MapStringStress(Map& m, const std::string& desc, bool is_static = false)
{
    std::stringstream ss;
    ss << desc << ":" << LOAD_CAPACITY << ": ";
    PROF_DEFINE_COUNTER(cost);
    if (true)
    {
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.insert({ string_data[i], string_data[i] });
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "insert").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ":has error");

    }

    if (true)
    {
        PROF_START_COUNTER(cost);
        m.clear();
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "clear").c_str(), 1, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "revert erase key (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase begin (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "find(capacity)").c_str(), 1000 * 10000, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "insert").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "clear").c_str(), 1, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
            m.insert(V(i));
        }
        ASSERT_TEST_EQ((int)m.size(), LOAD_CAPACITY, desc + ": error");
        PROF_START_COUNTER(cost);
        for (int i = 0; i < LOAD_CAPACITY; i++)
        {
            m.erase(V(i));
        }
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "revert erase key (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "erase begin (capacity)").c_str(), LOAD_CAPACITY, cost.stop_and_save().cost());
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
        PROF_OUTPUT_MULTI_COUNT_CPU((ss.str() + "find(capacity)").c_str(), 1000 * 10000, cost.stop_and_save().cost());
    }


    return 0;
}

#define MapStressWrap(T, V, is_static) \
do\
{\
    raii_object::reset(); \
    T* c = new T; \
    MapStress<T,V>(*c, #T, is_static); \
    delete c; \
    ASSERT_TNAME_RAII_EQUAL(T);\
}\
while(0)




#define MapStringStressWrap(T, V, is_static) \
do\
{\
    raii_object::reset(); \
    T* c = new T; \
    MapStringStress<T,V>(*c, #T, is_static); \
    delete c; \
    ASSERT_TNAME_RAII_EQUAL(T);\
}\
while(0)

#define SetStressWrap(T, V, is_static) \
do\
{\
    raii_object::reset(); \
    T* c = new T; \
    SetStress<T,V>(*c, #T, is_static); \
    delete c; \
    ASSERT_TNAME_RAII_EQUAL(T);\
}\
while(0)

template<class T, class V = raii_object, bool IS_STATIC = false>
s32 MapDestroyWrap()
{
    raii_object::reset();
    T* c = new T;
    for (int i = 0; i < LOAD_CAPACITY; i++)
    {
        c->insert(std::make_pair(i, V(i)));
    }
    delete c;
    ASSERT_TNAME_RAII_EQUAL(T);
    return 0;
}

s32 contiainer_stress_test()
{
    std::unique_ptr<zmalloc> zstate(new zmalloc());
    memset(zstate.get(), 0, sizeof(zmalloc));
    zstate->max_reserve_block_count_ = 100;
    zstate->set_global(zstate.get());

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

    using std_int_raii_map = std::map<int, raii_object>;
    using std_int_raii_unordered_map = std::unordered_map<int, raii_object>;
    using z_int_raii_hash_map = zhash_map<int, raii_object, LOAD_CAPACITY>;
    using z_int_raii_hash_map_zhash = zhash_map<int, raii_object, LOAD_CAPACITY, zhash<int>>;

    MapStressWrap(std_int_int_map, int, false);
    MapStressWrap(std_int_int_map_zallocator, int, false);
    MapStressWrap(std_int_int_unordered_map, int, false);
    MapStressWrap(z_int_int_hash_map, int, true);
    MapStressWrap(z_int_int_hash_map_zhash, int, true);

    MapStressWrap(std_int_raii_map, raii_object, false);
    MapStressWrap(std_int_raii_unordered_map, raii_object, false);
    MapStressWrap(z_int_raii_hash_map, raii_object, true);
    MapStressWrap(z_int_raii_hash_map_zhash, raii_object, true);


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


    MapDestroyWrap<std::map<int, raii_object>, raii_object, false>();
    MapDestroyWrap<std::unordered_map<int, raii_object>, raii_object, false>();
    MapDestroyWrap<zhash_map<int, raii_object, LOAD_CAPACITY>, raii_object, true>();
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
                for (auto& kv : sys_hashmap)
                {
                    sys_count += kv.second;
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


int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");


    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;
    ASSERT_TEST(hash_map_base_test() == 0);
    ASSERT_TEST(contiainer_stress_test() == 0);

    

    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();



    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


