

#include "fn_log.h"
#include <string>
#include "zarray.h"
#include "zvector.h"
#include "zlist.h"
#include "zlist_ext.h"
#include "test_common.h"
#include "zmalloc.h"


static const u32 MAX_SIZE = 1000;
int rand_array[MAX_SIZE];
int sort_array[MAX_SIZE];

raii_object rand_obj_array[MAX_SIZE];
raii_object sort_obj_array[MAX_SIZE];


template<class Target, class DataSets>
s32 CheckPushArray(Target& target, const DataSets& datasets)
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        target.push_back(datasets[i]);
        ASSERT_TEST_NOLOG(target.back() == datasets[i]);
    }
    ASSERT_TEST(target.size() == MAX_SIZE);
    return 0;
}

template<class Target, class DataSets>
s32 CheckEmplacePushArray(Target& target, const DataSets& datasets)
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        target.emplace_back(datasets[i]);
        ASSERT_TEST_NOLOG(target.back() == datasets[i]);
    }
    ASSERT_TEST(target.size() == MAX_SIZE);
    return 0;
}

template<class Target, class DataSets>
s32 CheckInsertBeginArray(Target& target, const DataSets& datasets)
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        target.insert(target.begin(), datasets[i]);
        ASSERT_TEST_NOLOG(target.front() == datasets[i]);
    }
    ASSERT_TEST(target.size() == MAX_SIZE);

    return 0;
}

template<class Target, class DataSets>
s32 CheckInsertEndArray(Target& target, const DataSets& datasets)
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        target.insert(target.end(), datasets[i]);
        ASSERT_TEST_NOLOG(target.back() == datasets[i]);
    }
    ASSERT_TEST(target.size() == MAX_SIZE);
    return 0;
}


template<class Target, class DataSets>
s32 CheckEmplaceInsertBeginArray(Target& target, const DataSets& datasets)
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        target.emplace(target.begin(), datasets[i]);
        ASSERT_TEST_NOLOG(target.front() == datasets[i]);
    }
    ASSERT_TEST(target.size() == MAX_SIZE);
    return 0;
}

template<class Target>
s32 CheckRandArray(Target& target)
{
    if (true)
    {
        int i = 0;
        for (auto& v : target)
        {
            ASSERT_TEST_NOLOG(v == rand_array[i]);
            i++;
        }
    }
    if (true)
    {
        int i = 0;
        const Target& const_target = target;
        for (auto& v : const_target)
        {
            ASSERT_TEST_NOLOG(v == rand_array[i]);
            i++;
        }
    }
    return 0;
}

template<class Target>
s32 CheckRandArrayRevertIter(Target& target)
{
    if (true)
    {
        u32 i = MAX_SIZE - 1;
        for (auto iter = target.rbegin(); iter != target.rend(); iter++)
        {
            ASSERT_TEST_NOLOG(*iter == rand_array[i]);
            i--;
        }
    }
    
    if (true)
    {
        u32 i = MAX_SIZE - 1;
        const Target& const_target = target;
        for (auto iter = const_target.rbegin(); iter != const_target.rend(); iter++)
        {
            ASSERT_TEST_NOLOG(*iter == rand_array[i]);
            i--;
        }
    }
    
    return 0;
}


template<class Target>
s32 CheckRevertRandArray(Target& target)
{
    if (true)
    {
        u32 i = MAX_SIZE - 1;
        for (auto& v : target)
        {
            ASSERT_TEST_NOLOG(v == rand_array[i]);
            i--;
        }
    }
    if (true)
    {
        u32 i = MAX_SIZE - 1;
        const Target& const_target = target;
        for (auto& v : const_target)
        {
            ASSERT_TEST_NOLOG(v == rand_array[i]);
            i--;
        }
    }
    return 0;
}




template<class Target>
s32 CheckRandArrayAt(Target& target)
{
    if (true)
    {
        u32 i = 0;
        for (auto& v : target)
        {
            ASSERT_TEST_NOLOG((int)v == rand_array[i]);
            ASSERT_TEST_NOLOG((int)v == (int)target.at(i));
            ASSERT_TEST_NOLOG((int)v == (int)target[i]);
            i++;
        }
    }
    if (true)
    {
        u32 i = 0;
        const Target& const_target = target;
        for (auto& v : const_target)
        {
            ASSERT_TEST_NOLOG((int)v == rand_array[i]);
            ASSERT_TEST_NOLOG((int)v == (int)const_target.at(i));
            ASSERT_TEST_NOLOG((int)v == (int)const_target[i]);
            i++;
        }
    }
    return 0;
}

template<class Target>
s32 CheckRevertRandArrayAt(Target& target)
{
    if (true)
    {
        u32 i = MAX_SIZE - 1;
        for (auto& v : target)
        {
            ASSERT_TEST_NOLOG((int)v == rand_array[i]);
            ASSERT_TEST_NOLOG((int)v == (int)target.at(MAX_SIZE - i - 1));
            ASSERT_TEST_NOLOG((int)v == (int)target[MAX_SIZE - i - 1]);
            i--;
        }
    }
    if (true)
    {
        u32 i = MAX_SIZE - 1;
        const Target& const_target = target;
        for (auto& v : const_target)
        {
            ASSERT_TEST_NOLOG((int)v == rand_array[i]);
            ASSERT_TEST_NOLOG((int)v == (int)const_target.at(MAX_SIZE - i - 1));
            ASSERT_TEST_NOLOG((int)v == (int)const_target[MAX_SIZE - i - 1]);
            i--;
        }
    }
    return 0;
}


template<class Target>
s32 CheckSortArray(Target& target)
{
    if (true)
    {
        u32 i = 0;
        for (auto& v : target)
        {
            ASSERT_TEST_NOLOG((int)v == sort_array[i]);
            i++;
        }
    }
    return 0;
}

template<class Target>
s32 CheckPopArray(Target& target)
{
    while (target.size() != 0)
    {
        target.pop_back();
    }

    ASSERT_TEST_NOLOG(target.size() == 0);

    return 0;
}

template<class Target>
s32 CheckEraseArray(Target& target)
{
    target.erase(target.begin(), target.end());
    ASSERT_TEST_NOLOG(target.size() == 0);
    return 0;
}




template<class Target, class DataSets>
s32 ArrayBaseTest(Target& target, const DataSets& datasets)
{
    target = { 1, 2,3 };
    if ((int)target[0] != 1  || (int)target[1] != 2 || (int)target[2] != 3  || target.size() != 3)
    {
        LogError() << " pop error";
    }
    ASSERT_TEST(CheckPopArray(target) == 0);
    ASSERT_RAII_EQUAL("ArrayBaseTest");

    ASSERT_TEST(CheckPushArray(target, datasets) == 0);
    ASSERT_TEST(CheckRandArray(target) == 0);
    ASSERT_TEST(CheckPopArray(target) == 0);
    ASSERT_RAII_EQUAL("ArrayBaseTest");

    ASSERT_TEST(CheckEmplacePushArray(target, datasets) == 0);
    ASSERT_TEST(CheckRandArray(target) == 0);
    ASSERT_TEST(CheckPopArray(target) == 0);
    ASSERT_RAII_EQUAL("ArrayBaseTest");

    ASSERT_TEST(CheckInsertEndArray(target, datasets) == 0);
    ASSERT_TEST(CheckRandArrayAt(target) == 0);
    ASSERT_TEST(CheckEraseArray(target) == 0);
    ASSERT_RAII_EQUAL("ArrayBaseTest");

    ASSERT_TEST(CheckInsertBeginArray(target, datasets) == 0);
    ASSERT_TEST(CheckRevertRandArrayAt(target) == 0);
    ASSERT_TEST(CheckEraseArray(target) == 0);
    ASSERT_RAII_EQUAL("ArrayBaseTest");

    ASSERT_TEST(CheckEmplaceInsertBeginArray(target, datasets) == 0);
    ASSERT_TEST(CheckRevertRandArrayAt(target) == 0);
    ASSERT_TEST(CheckEraseArray(target) == 0);
    ASSERT_RAII_EQUAL("ArrayBaseTest");

    ASSERT_TEST(CheckEmplaceInsertBeginArray(target, datasets) == 0);
    std::sort(target.begin(), target.end());
    ASSERT_TEST(CheckSortArray(target) == 0);
    auto m1 = target.begin();
    m1++;
    auto m2 = target.end();
    m2--;
    target.erase(m1, m2);
    return 0;
}

template<class Target, class DataSets>
s32 ListBaseTest(Target& target, const DataSets& datasets)
{
    target = { 1, 2,3 };
    if (true)
    {
        u32 i = 0;
        char vvv[] = { 1,2,3 };
        for (auto& v:target)
        {
            ASSERT_TEST((int)v == vvv[i]);
            i++;
        }
        ASSERT_TEST(target.size() == 3);

        ASSERT_TEST(CheckPopArray(target) == 0);
    }


    ASSERT_TEST(CheckPushArray(target, datasets) == 0);
    ASSERT_TEST(CheckRandArray(target) == 0);
    ASSERT_TEST(CheckRandArrayRevertIter(target) == 0);
    ASSERT_TEST(CheckPopArray(target) == 0);
    ASSERT_RAII_EQUAL("ListBaseTest");

    ASSERT_TEST(CheckEmplacePushArray(target, datasets) == 0);
    ASSERT_TEST(CheckRandArray(target) == 0);
    ASSERT_TEST(CheckPopArray(target) == 0);
    ASSERT_RAII_EQUAL("ListBaseTest");

    ASSERT_TEST(CheckInsertEndArray(target, datasets) == 0);
    ASSERT_TEST(CheckRandArray(target) == 0);
    ASSERT_TEST(CheckEraseArray(target) == 0);
    ASSERT_RAII_EQUAL("ListBaseTest");

    ASSERT_TEST(CheckInsertBeginArray(target, datasets) == 0);
    ASSERT_TEST(CheckRevertRandArray(target) == 0);

    ASSERT_TEST(CheckEraseArray(target) == 0);
    ASSERT_RAII_EQUAL("ListBaseTest");

    ASSERT_TEST(CheckEmplaceInsertBeginArray(target, datasets) == 0);
    ASSERT_TEST(CheckRevertRandArray(target) == 0);
    ASSERT_TEST(CheckEraseArray(target) == 0);
    ASSERT_RAII_EQUAL("ListBaseTest");

    ASSERT_TEST(CheckEmplaceInsertBeginArray(target, datasets) == 0);
    auto m1 = target.begin();
    m1++;
    auto m2 = target.end();
    m2--;
    target.erase(m1, m2);
    return 0;
}


s32 array_test()
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        rand_array[i] = rand() % MAX_SIZE;
        rand_obj_array[i] = rand_array[i];
    }
    memcpy(sort_array, rand_array, sizeof(int) * MAX_SIZE);
    std::sort(&sort_array[0], &sort_array[0] + MAX_SIZE);
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        sort_obj_array[i] = sort_array[i];
    }
    raii_object::reset();

    if (true)
    {
        std::vector<int> target;
        ASSERT_TEST(ArrayBaseTest(target, rand_array) == 0);
    }

    if (true)
    {
        zarray<int, MAX_SIZE> target;
        ASSERT_TEST(ArrayBaseTest(target, rand_array) == 0);
    }

    if (true)
    {
        zvector<int, MAX_SIZE, 0> target;
        ASSERT_TEST(ArrayBaseTest(target, rand_array) == 0, "0 fixed zvector");
    }

    if (true)
    {
        zvector<int, MAX_SIZE, MAX_SIZE / 2> target;
        ASSERT_TEST(ArrayBaseTest(target, rand_array) == 0, "half zvector");
    }

    if (true)
    {
        zvector<int, MAX_SIZE, MAX_SIZE> target;
        ASSERT_TEST(ArrayBaseTest(target, rand_array) == 0, "fixed zvector");
    }

    if (true)
    {
        raii_object::reset();
        std::vector<raii_object> target;
        ASSERT_TEST(ArrayBaseTest(target, rand_array) == 0);
    }

    if (true)
    {
        raii_object::reset();
        zarray<raii_object, MAX_SIZE> target;
        ASSERT_TEST(ArrayBaseTest(target, rand_array) == 0);

        
        if (true)
        {
            volatile u32 i = MAX_SIZE - 1;
            const auto& const_target = target;
            for (auto iter = const_target.rbegin(); iter != const_target.rend(); iter++)
            {
                i--;
            }
        }

    }

    if (true)
    {
        raii_object::reset();
        zvector<raii_object, MAX_SIZE, 0> target;
        ASSERT_TEST(ArrayBaseTest(target, rand_array) == 0, "zvector");

        if (true)
        {
            volatile u32 i = MAX_SIZE - 1;
            const auto& const_target = target;
            for (auto iter = const_target.rbegin(); iter != const_target.rend(); iter++)
            {
                i--;
            }
        }

    }

    if (true)
    {
        raii_object::reset();
        zvector<raii_object, MAX_SIZE, MAX_SIZE> target;
        ASSERT_TEST(ArrayBaseTest(target, rand_array) == 0, "zvector");

        if (true)
        {
            volatile u32 i = MAX_SIZE - 1;
            const auto& const_target = target;
            for (auto iter = const_target.rbegin(); iter != const_target.rend(); iter++)
            {
                i--;
            }
        }

    }


    if (true)
    {
        zlist<int, MAX_SIZE> target;
        ASSERT_TEST(ListBaseTest(target, rand_array) == 0);
        if (true)
        {
            volatile u32 i = MAX_SIZE - 1;
            const auto& const_target = target;
            for (auto iter = const_target.rbegin(); iter != const_target.rend(); iter++)
            {
                i--;
            }
        }
    }
    if (true)
    {
        zlist_ext<int, MAX_SIZE, MAX_SIZE> target;
        ASSERT_TEST(ListBaseTest(target, rand_array) == 0);
        if (true)
        {
            volatile u32 i = MAX_SIZE - 1;
            const auto& const_target = target;
            for (auto iter = const_target.rbegin(); iter != const_target.rend(); iter++)
            {
                i--;
            }
        }
    }
    if (true)
    {
        zlist_ext<int, MAX_SIZE, 1> target;
        ASSERT_TEST(ListBaseTest(target, rand_array) == 0);
    }

    if (true)
    {
        zarray<std::string, 100> strings = { "123", "2" };
        zarray<std::string, 50> strings2;
        strings2.assign(strings.begin(), strings.end());
        ASSERT_TEST_EQ(strings2.size(), 2ULL, "");
        strings2.emplace_back("888");

        if (true)
        {
            raii_object& rv = sort_obj_array[0];
            zarray<raii_object, 100> raii_array;
            ASSERT_TEST_EQ(raii_object::now_live_count_, 0U, "");
            raii_array.push_back(rv);
            ASSERT_TEST_EQ(raii_object::now_live_count_, 1U, "");
            raii_array.insert(raii_array.begin(), rv);
            ASSERT_TEST_EQ(raii_object::now_live_count_, 2U, "");
            ASSERT_TEST_EQ(raii_object::construct_count_, raii_object::destroy_count_ + 2, "");
            raii_array.clear();
            ASSERT_TEST_EQ(raii_object::now_live_count_, 0U, "");
            ASSERT_TEST_EQ(raii_object::construct_count_, raii_object::destroy_count_, "");
        }

    }


    if (true)
    {
        zvector<std::string, 100, 0> strings = { "123", "2" };
        zvector<std::string, 50, 0> strings2;
        strings2.assign(strings.begin(), strings.end());
        ASSERT_TEST_EQ(strings2.size(), 2ULL, "");
        strings2.emplace_back("888");

        if (true)
        {
            raii_object& rv = sort_obj_array[0];
            zvector<raii_object, 100, 0> raii_array;
            ASSERT_TEST_EQ(raii_object::now_live_count_, 0U, "");
            raii_array.push_back(rv);
            ASSERT_TEST_EQ(raii_object::now_live_count_, 1U, "");
            raii_array.insert(raii_array.begin(), rv);
            ASSERT_TEST_EQ(raii_object::now_live_count_, 2U, "");
            ASSERT_TEST_EQ(raii_object::construct_count_, raii_object::destroy_count_ + 2, "");
            raii_array.clear();
            ASSERT_TEST_EQ(raii_object::now_live_count_, 0U, "");
            ASSERT_TEST_EQ(raii_object::construct_count_, raii_object::destroy_count_, "");
        }

    }

    if (true)
    {
        zvector<std::string, 100, 100> strings = { "123", "2" };
        zvector<std::string, 50, 50> strings2;
        strings2.assign(strings.begin(), strings.end());
        ASSERT_TEST_EQ(strings2.size(), 2ULL, "");
        strings2.emplace_back("888");

        if (true)
        {
            raii_object& rv = sort_obj_array[0];
            zvector<raii_object, 100, 100> raii_array;
            ASSERT_TEST_EQ(raii_object::now_live_count_, 0U, "");
            raii_array.push_back(rv);
            ASSERT_TEST_EQ(raii_object::now_live_count_, 1U, "");
            raii_array.insert(raii_array.begin(), rv);
            ASSERT_TEST_EQ(raii_object::now_live_count_, 2U, "");
            ASSERT_TEST_EQ(raii_object::construct_count_, raii_object::destroy_count_ + 2, "");
            raii_array.clear();
            ASSERT_TEST_EQ(raii_object::now_live_count_, 0U, "");
            ASSERT_TEST_EQ(raii_object::construct_count_, raii_object::destroy_count_, "");
        }

    }



    return 0;
}



s32 list_test()
{
    zlist<int, 20> numbers = { 1,9,8,7 };
    ASSERT_TEST_EQ(numbers.size(), 4ULL, "");
    zarray<int, 100> target = { 1,9,8,7 };
    u32 i = 0;
    for (auto& n : numbers)
    {
        ASSERT_TEST_EQ(target[i++], n, "");
    }
    numbers.push_back(9);
    numbers.push_front(9);
    ASSERT_TEST_EQ(numbers.size(), 6ULL, "");
    ASSERT_TEST_EQ(numbers.front(), 9, "");
    ASSERT_TEST_EQ(numbers.back(), 9, "");
    numbers.pop_back();
    numbers.pop_front();
    ASSERT_TEST_EQ(numbers.size(), 4ULL, "");
    ASSERT_TEST_EQ(numbers.front(), 1, "");
    ASSERT_TEST_EQ(numbers.back(), 7, "");
    numbers.erase(++numbers.begin(), --numbers.end());
    ASSERT_TEST_EQ(numbers.size(), 2ULL, "");
    ASSERT_TEST_EQ(numbers.front(), 1, "");
    ASSERT_TEST_EQ(numbers.back(), 7, "");
    numbers.insert(++numbers.begin(), 9);
    numbers.insert(--numbers.end(), 8);
    ASSERT_TEST_EQ(numbers.size(), 4ULL, "");
    ASSERT_TEST_EQ(numbers.front(), 1, "");
    ASSERT_TEST_EQ(numbers.back(), 7, "");

    zlist<std::string, 20> strlist;
    strlist.push_back("sss");
    strlist.clear();

    zlist<int, 3> clear_test = { 1,3,5 };
    clear_test.clear();
    clear_test.push_back(2);
    clear_test.push_back(3);
    clear_test.push_back(1);
    clear_test.clear();
    ASSERT_TEST_EQ(clear_test.size(), 0ULL, "");



    
    zlist<int, 100> bound_test;
    ASSERT_TEST_EQ(bound_test.is_valid_node((void*)((u64)&bound_test - 1 )), false, "");

    ASSERT_TEST_EQ(bound_test.is_valid_node((void*)((u64)&bound_test + sizeof(bound_test))), false, "");
    ASSERT_TEST_EQ(!bound_test.is_valid_node((void*)((u64)&bound_test + sizeof(zlist<int, 100>::node_type) * 99 )), false, "");


    for (size_t i = 0; i < 100; i++)
    {
        int f = rand() % 50;
        bound_test.insert(bound_test.lower_bound(bound_test.begin(), bound_test.end(), f, [](int f, int s) {return f > s; }), f);
    }
    for (auto iter = bound_test.begin(); iter != bound_test.end(); ++iter)
    {
        auto iter2 = iter;
        iter2++;
        if (iter2 != bound_test.end())
        {
            if (*iter < *(iter2))
            {
                *(u64*)NULL = 0;
            }
        }
    }

    return 0;
}

s32 list_test_u32()
{
    zlist<u32, 20> numbers = { 1,9,8,7 };
    ASSERT_TEST_EQ(numbers.size(), 4ULL, "");
    zarray<u32, 100> target = { 1,9,8,7 };
    u32 i = 0;
    for (auto& n : numbers)
    {
        ASSERT_TEST_EQ(target[i++], n, "");
    }
    numbers.push_back(9);
    numbers.push_front(9);
    ASSERT_TEST_EQ(numbers.size(), 6ULL, "");
    ASSERT_TEST_EQ(numbers.front(), 9, "");
    ASSERT_TEST_EQ(numbers.back(), 9, "");
    numbers.pop_back();
    numbers.pop_front();
    ASSERT_TEST_EQ(numbers.size(), 4ULL, "");
    ASSERT_TEST_EQ(numbers.front(), 1, "");
    ASSERT_TEST_EQ(numbers.back(), 7, "");
    numbers.erase(++numbers.begin(), --numbers.end());
    ASSERT_TEST_EQ(numbers.size(), 2ULL, "");
    ASSERT_TEST_EQ(numbers.front(), 1, "");
    ASSERT_TEST_EQ(numbers.back(), 7, "");
    numbers.insert(++numbers.begin(), 9);
    numbers.insert(--numbers.end(), 8);
    ASSERT_TEST_EQ(numbers.size(), 4ULL, "");
    ASSERT_TEST_EQ(numbers.front(), 1, "");
    ASSERT_TEST_EQ(numbers.back(), 7, "");

    zlist<std::string, 20> strlist;
    strlist.push_back("sss");
    strlist.clear();

    zlist<u32, 3> clear_test = { 1,3,5 };
    clear_test.clear();
    clear_test.push_back(2);
    clear_test.push_back(3);
    clear_test.push_back(1);
    clear_test.clear();
    ASSERT_TEST_EQ(clear_test.size(), 0ULL, "");




    zlist<u32, 100> bound_test;
    ASSERT_TEST_EQ(bound_test.is_valid_node((void*)((u64)&bound_test - 1)), false, "");

    ASSERT_TEST_EQ(bound_test.is_valid_node((void*)((u64)&bound_test + sizeof(bound_test))), false, "");
    ASSERT_TEST_EQ(!bound_test.is_valid_node((void*)((u64)&bound_test + sizeof(zlist<u32, 100>::node_type) * 99)), false, "");


    for (size_t i = 0; i < 100; i++)
    {
        u32 f = rand() % 50;
        bound_test.insert(bound_test.lower_bound(bound_test.begin(), bound_test.end(), f, [](u32 f, u32 s) {return f > s; }), f);
    }
    for (auto iter = bound_test.begin(); iter != bound_test.end(); ++iter)
    {
        auto iter2 = iter;
        iter2++;
        if (iter2 != bound_test.end())
        {
            if (*iter < *(iter2))
            {
                *(u64*)NULL = 0;
            }
        }
    }

    return 0;
}


template<class C>
s32 ZSortInsertLogN(C& c, size_t s)
{
    for (size_t i = 0; i < s; i++)
    {
        size_t r = rand()%100;
        c.insert(std::lower_bound(c.begin(), c.end(), r), r);
    }
    size_t n = 0;
    for (auto& e : c)
    {
        if (e < n) return -1;
        n = e;
    }

    for (size_t i = 0; i < s; i++)
    {
        size_t r = rand() % 100;
        c.insert(std::lower_bound(c.begin(), c.end(), r, [](size_t l, size_t r) {return l < r; }), r);
    }
    n = 0;
    for (auto& e : c)
    {
        if (e < n) return -1;
        n = e;
    }


    return 0;
}


template<class C>
s32 ZListValidTest(C& c, size_t s)
{
    for (size_t i = 0; i < s; i++)
    {
        size_t r = rand() % 100;
        c.insert(c.begin(), r);
    }
    ASSERT_TEST_EQ(c.size(), s, "");
    for (auto& e : c)
    {
        if (!c.is_valid_node(&e))
        {
            volatile int target = 0;
            (void)target;
        }
        ASSERT_TEST_EQ(!c.is_valid_node(&e), false, "");
    }
    return 0;
}

template<class C>
s32 ZSortInsertLine(C& c, size_t s)
{
    for (size_t i = 0; i < s; i++)
    {
        size_t r = rand() % 100;
        c.insert(std::find_if(c.begin(), c.end(), [&r](const size_t& e) { return e >= r; }), r);
    }
    size_t n = 0;
    for (auto& e : c)
    {
        if (e < n) return -1;
        n = e;
    }
    return 0;
}

template<class C>
s32 copy_test(C* ptr)
{
    (void)ptr;
    void* test_addr = new char[sizeof(C)*2];
    C* src = new (test_addr) C();
    for (size_t i = 0; i < 10; i++)
    {
        src->push_back(i);
    }
    memcpy((char*)test_addr + sizeof(C), test_addr, sizeof(C));
    memset(test_addr, 0xdeadbeaf, sizeof(C));
    ASSERT_TEST_EQ((src + 1)->size(), 10ULL, "");

    size_t i = 0;
    for (auto & n: *(src+1))
    {
        if (n != i)
        {
            return -2;
        }
        i++;
    }
    delete[] (char*)test_addr;
    return 0;
}

s32 by_order_test()
{
    if (true)
    {
        zarray<size_t, 200> za;
        ASSERT_TEST_EQ(ZSortInsertLogN(za, 200), 0, "");
    }
    if (true)
    {
        zlist<size_t, 200> zl;
        ASSERT_TEST_EQ(ZSortInsertLine(zl, 200), 0, "");
    }
    if (true)
    {
        zlist<size_t, 200> zl;
        ZListValidTest(zl, 200);
    }
    if (true)
    {
        zlist_ext<size_t, 200, 100> zl;
        ZListValidTest(zl, 200);
    }
    return 0;
}







static const int LOOP_CAPACITY = 1000;
static const int LOAD_CAPACITY = 1024 * 8;




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



s32 destroy_test()
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
    using int_zvector_half_fixed = zvector<int, LOAD_CAPACITY, LOAD_CAPACITY / 2>;
    using raii_zvector_half_fixed = zvector<raii_object, LOAD_CAPACITY, LOAD_CAPACITY / 2>;
    using int_zvector_full_fixed = zvector<int, LOAD_CAPACITY, LOAD_CAPACITY>;
    using raii_zvector_full_fixed = zvector<raii_object, LOAD_CAPACITY, LOAD_CAPACITY>;





    LinerDestroyWrap<std::vector<int>, raii_object>();
    LinerDestroyWrap<int_zarray, raii_object, true>();
    LinerDestroyWrap<int_zvector_0_fixed, raii_object, true>();
    LinerDestroyWrap<int_zvector_half_fixed, raii_object, true>();
    LinerDestroyWrap<int_zvector_full_fixed, raii_object, true>();




    LinerDestroyWrap<std::vector<raii_object>, raii_object>();
    LinerDestroyWrap<raii_zarray, raii_object, true>();
    LinerDestroyWrap<raii_zvector_0_fixed, raii_object, true>();
    LinerDestroyWrap<raii_zvector_half_fixed, raii_object, true>();
    LinerDestroyWrap<raii_zvector_full_fixed, raii_object, true>();


    LogDebug() << "================";


    LinerDestroyWrap<std::deque<raii_object>, raii_object>();
    LinerDestroyWrap<std::list<raii_object>, raii_object>();
    LinerDestroyWrap<zlist<raii_object, LOAD_CAPACITY>, raii_object, true>();
    LinerDestroyWrap<zlist_ext<raii_object, LOAD_CAPACITY, LOAD_CAPACITY>, raii_object, true>();
    LinerDestroyWrap<zlist_ext<raii_object, LOAD_CAPACITY, 1>, raii_object, true>();



    return 0;
}

s32 zvector_find_test()
{
    if (true)
    {
        zvector<int, 100> z;
        z.push_back(3);
        z.push_back(1);
        z.push_back(2);
        z.push_back(1);
        z.push_back(2);
        z.push_back(3);
        ASSERT_TEST_EQ(*z.find(2), 2, "");
        ASSERT_TEST_EQ(*z.find(3), 3, "");
        ASSERT_TEST_NOLOG(z.find(3) == z.begin(), "");
        ASSERT_TEST_NOLOG(z.rfind(3) == z.rbegin(), "");
        ASSERT_TEST_NOLOG(z.rfind(5) == z.rend(), "");
        z.assign((u32)100, 1);
        ASSERT_TEST_NOLOG(z.full(), "");
        if (true)
        {
            zvector<int, 100> zz(std::move(z));
        }
        z.assign((u32)100, 1);
        ASSERT_TEST_NOLOG(z.full(), "");
    }
    if (true)
    {
        zvector<int, 100, 0> z;
        z.push_back(3);
        z.push_back(1);
        z.push_back(2);
        z.push_back(1);
        z.push_back(2);
        z.push_back(3);
        ASSERT_TEST_EQ(*z.find(2), 2, "");
        ASSERT_TEST_EQ(*z.find(3), 3, "");
        ASSERT_TEST_NOLOG(z.find(3)== z.begin(), "");
        ASSERT_TEST_NOLOG(z.rfind(3)== z.rbegin(), "");
        ASSERT_TEST_NOLOG(z.rfind(5) == z.rend(), "");
        z.assign((u32)100, 1);
        ASSERT_TEST_NOLOG(z.full(), "");
        if (true)
        {
            zvector<int, 100, 0> zz(std::move(z));
        }
        z.assign((u32)100, 1);
        ASSERT_TEST_NOLOG(z.full(), "");
    }
    return 0;
}

s32 coverage_test()
{

    ASSERT_TEST_EQ(array_test(), 0, " array_test()");
    ASSERT_TEST_EQ(list_test(), 0, " list_test()");
    ASSERT_TEST_EQ(list_test_u32(), 0, " list_test_u32()");
    ASSERT_TEST_EQ(by_order_test(), 0, " by_order_test()");
    ASSERT_TEST_EQ(copy_test((zlist<size_t, 100>*)NULL), 0, "copy_test((zlist<size_t, 100>*)NULL)");
    ASSERT_TEST_EQ(copy_test((zarray<size_t, 100>*)NULL), 0, "copy_test((zarray<size_t, 100>*)NULL)");
    ASSERT_TEST(destroy_test() == 0);
    ASSERT_TEST(zvector_find_test() == 0);
    return 0;
}


