

#include "fn_log.h"
#include <string>
#include "zarray.h"
#include "zlist.h"
#include "zlist_ext.h"
#include "zhash_map.h"
#include "zcontain_test.h"
#include "zobj_pool.h"
using namespace zsummer;


static const u32 MAX_SIZE = 1000;
int rand_array[MAX_SIZE];
int sort_array[MAX_SIZE];

RAIIVal<> rand_obj_array[MAX_SIZE];
RAIIVal<> sort_obj_array[MAX_SIZE];


template<class T, class V>
s32 CheckPushArray(T& a, const V & rand_a)
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        a.push_back(rand_a[i]);
        if (a.back() != rand_a[i])
        {
            LogError() << "  has error";
            return -1;
        }
    }
    if (a.size() != MAX_SIZE)
    {
        LogError() << "  has error";
        return -1;
    }
    return 0;
}
template<class T, class V>
s32 CheckEmplacePushArray(T& a, const V& rand_a)
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        a.emplace_back(rand_a[i]);
        if (a.back() != rand_a[i])
        {
            LogError() << "  has error";
            return -1;
        }
    }
    if (a.size() != MAX_SIZE)
    {
        LogError() << "  has error";
        return -1;
    }
    return 0;
}

template<class T, class V>
s32 CheckInsertBeginArray(T& a, const V& rand_a)
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        a.insert(a.begin(), rand_a[i]);
        if (a.front() != rand_a[i])
        {
            LogError() << "  has error";
            return -1;
        }
    }
    if (a.size() != MAX_SIZE)
    {
        LogError() << "  has error";
        return -1;
    }
    return 0;
}

template<class T, class V>
s32 CheckInsertEndArray(T& a, const V& rand_a)
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        a.insert(a.end(), rand_a[i]);
        if (a.back() != rand_a[i])
        {
            LogError() << "  has error";
            return -1;
        }
    }
    if (a.size() != MAX_SIZE)
    {
        LogError() << "  has error";
        return -1;
    }
    return 0;
}


template<class T, class V>
s32 CheckEmplaceInsertBeginArray(T& a, const V& rand_a)
{
    for (u32 i = 0; i < MAX_SIZE; i++)
    {
        a.emplace(a.begin(), rand_a[i]);
        if (a.front() != rand_a[i])
        {
            LogError() << "  has error";
            return -1;
        }
    }
    if (a.size() != MAX_SIZE)
    {
        LogError() << "  has error";
        return -1;
    }
    return 0;
}

template<class T>
s32 CheckRandArray(T& a)
{
    if (true)
    {
        int i = 0;
        for (auto& v : a)
        {
            if ((int)v != rand_array[i])
            {
                LogError() << "  has error";
                return -1;
            }
            i++;
        }
    }
    return 0;
}

template<class T>
s32 CheckRevertRandArray(T& a)
{
    if (true)
    {
        u32 i = MAX_SIZE - 1;
        for (auto& v : a)
        {
            if ((int)v != rand_array[i])
            {
                LogError() << "  has error";
                return -1;
            }
            i--;
        }
    }
    return 0;
}

template<class T>
s32 CheckRandArrayAt(T& a)
{
    if (true)
    {
        u32 i = 0;
        for (auto& v : a)
        {
            if ((int)v != rand_array[i])
            {
                LogError() << "  has error";
                return -1;
            }
            if ((int)v != (int)a.at(i))
            {
                LogError() << "  has error";
                return -1;
            }
            if ((int)v != (int)a[i])
            {
                LogError() << "  has error";
                return -1;
            }
            i++;
        }
    }
    return 0;
}

template<class T>
s32 CheckRevertRandArrayAt(T& a)
{
    if (true)
    {
        u32 i = MAX_SIZE - 1;
        for (auto& v : a)
        {
            if ((int)v != rand_array[i])
            {
                LogError() << "  has error";
                return -1;
            }
            if ((int)v != (int)a.at(MAX_SIZE - i - 1))
            {
                LogError() << "  has error";
                return -1;
            }
            if ((int)v != (int)a[MAX_SIZE - i - 1])
            {
                LogError() << "  has error";
                return -1;
            }
            i--;
        }
    }
    return 0;
}


template<class T>
s32 CheckSortArray(T& a)
{
    if (true)
    {
        u32 i = 0;
        for (auto& v : a)
        {
            if ((int)v != sort_array[i])
            {
                LogError() << "  has error";
                return -1;
            }
            i++;
        }
    }
    return 0;
}

template<class T>
s32 CheckPopArray(T& a)
{
    while (a.size() != 0)
    {
        a.pop_back();
    }
    if (a.size() != 0)
    {
        LogError() << " pop error";
    }
    return 0;
}

template<class T>
s32 CheckEraseArray(T& a)
{
    a.erase(a.begin(), a.end());
    if (a.size() != 0)
    {
        LogError() << " erase fail";
    }
    return 0;
}

template<class A, class VS>
s32 ArrayBaseTest(A& a, VS& v)
{
    a = { 1, 2,3 };
    if ((int)a[0] != 1  || (int)a[1] != 2 || (int)a[2] != 3  || a.size() != 3)
    {
        LogError() << " pop error";
    }
    CheckPopArray(a);
    CheckRAIIVal("ArrayBaseTest");

    CheckPushArray(a, v);
    CheckRandArray(a);
    CheckPopArray(a);
    CheckRAIIVal("ArrayBaseTest");

    CheckEmplacePushArray(a, v);
    CheckRandArray(a);
    CheckPopArray(a);
    CheckRAIIVal("ArrayBaseTest");

    CheckInsertEndArray(a, v);
    CheckRandArrayAt(a);
    CheckEraseArray(a);
    CheckRAIIVal("ArrayBaseTest");

    CheckInsertBeginArray(a, v);
    CheckRevertRandArrayAt(a);
    CheckEraseArray(a);
    CheckRAIIVal("ArrayBaseTest");

    CheckEmplaceInsertBeginArray(a, v);
    CheckRevertRandArrayAt(a);
    CheckEraseArray(a);
    CheckRAIIVal("ArrayBaseTest");

    CheckEmplaceInsertBeginArray(a, v);
    std::sort(a.begin(), a.end());
    CheckSortArray(a);
    auto m1 = a.begin();
    m1++;
    auto m2 = a.end();
    m2--;
    a.erase(m1, m2);
    return 0;
}

template<class A, class VS>
s32 ListBaseTest(A& a, VS& v)
{
    a = { 1, 2,3 };
    if (true)
    {
        u32 i = 0;
        char vvv[] = { 1,2,3 };
        for (auto& v:a)
        {
            if ((int)v != vvv[i])
            {
                LogError() << " assign error";
                return 1;
            }
            i++;
        }
        if (a.size() != 3)
        {
            LogError() << " assign error";
        }
        CheckPopArray(a);
    }


    CheckPushArray(a, v);
    CheckRandArray(a);
    CheckPopArray(a);
    CheckRAIIVal("ListBaseTest");

    CheckEmplacePushArray(a, v);
    CheckRandArray(a);
    CheckPopArray(a);
    CheckRAIIVal("ListBaseTest");

    CheckInsertEndArray(a, v);
    CheckRandArray(a);
    CheckEraseArray(a);
    CheckRAIIVal("ListBaseTest");

    CheckInsertBeginArray(a, v);
    CheckRevertRandArray(a);
    CheckEraseArray(a);
    CheckRAIIVal("ListBaseTest");

    CheckEmplaceInsertBeginArray(a, v);
    CheckRevertRandArray(a);
    CheckEraseArray(a);
    CheckRAIIVal("ListBaseTest");

    CheckEmplaceInsertBeginArray(a, v);
    auto m1 = a.begin();
    m1++;
    auto m2 = a.end();
    m2--;
    a.erase(m1, m2);
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
    RAIIVal<>::reset();

    if (true)
    {
        std::vector<int> a;
        ArrayBaseTest(a, rand_array);
    }

    if (true)
    {
        zarray<int, MAX_SIZE> a;
        ArrayBaseTest(a, rand_array);
    }

    if (true)
    {
        RAIIVal<>::reset();
        std::vector<RAIIVal<>> a;
        ArrayBaseTest(a, rand_array);
    }

    if (true)
    {
        RAIIVal<>::reset();
        zarray<RAIIVal<>, MAX_SIZE> a;
        ArrayBaseTest(a, rand_array);
    }




    if (true)
    {
        zlist<int, MAX_SIZE> a;
        ListBaseTest(a, rand_array);
    }
    if (true)
    {
        zlist_ext<int, MAX_SIZE, MAX_SIZE> a;
        ListBaseTest(a, rand_array);
    }
    if (true)
    {
        zlist_ext<int, MAX_SIZE, 1> a;
        ListBaseTest(a, rand_array);
    }


    zarray<std::string, 100> strings = { "123", "2" };
    zarray<std::string, 50> strings2;
    strings2.assign(strings.begin(), strings.end());
    AssertTest(strings2.size(), 2ULL, "");
    strings2.emplace_back("888");

    if (true)
    {
        RAIIVal<>& rv = sort_obj_array[0];
        zarray<RAIIVal<>, 100> raii_array;
        AssertTest(RAIIVal<>::now_live_count_, 0U, "");
        raii_array.push_back(rv);
        AssertTest(RAIIVal<>::now_live_count_, 1U, "");
        raii_array.insert(raii_array.begin(), rv);
        AssertTest(RAIIVal<>::now_live_count_, 2U, "");
        AssertTest(RAIIVal<>::construct_count_, RAIIVal<>::destroy_count_ + 2, "");
        raii_array.clear();
        AssertTest(RAIIVal<>::now_live_count_, 0U, "");
        AssertTest(RAIIVal<>::construct_count_, RAIIVal<>::destroy_count_, "");


    }

    return 0;
}






#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()                                

s32 list_test()
{
    zlist<int, 20> numbers = { 1,9,8,7 };
    AssertTest(numbers.size(), 4ULL, "");
    zarray<int, 100> a = { 1,9,8,7 };
    u32 i = 0;
    for (auto& n : numbers)
    {
        AssertTest(a[i++], n, "");
    }
    numbers.push_back(9);
    numbers.push_front(9);
    AssertTest(numbers.size(), 6ULL, "");
    AssertTest(numbers.front(), 9, "");
    AssertTest(numbers.back(), 9, "");
    numbers.pop_back();
    numbers.pop_front();
    AssertTest(numbers.size(), 4ULL, "");
    AssertTest(numbers.front(), 1, "");
    AssertTest(numbers.back(), 7, "");
    numbers.erase(++numbers.begin(), --numbers.end());
    AssertTest(numbers.size(), 2ULL, "");
    AssertTest(numbers.front(), 1, "");
    AssertTest(numbers.back(), 7, "");
    numbers.insert(++numbers.begin(), 9);
    numbers.insert(--numbers.end(), 8);
    AssertTest(numbers.size(), 4ULL, "");
    AssertTest(numbers.front(), 1, "");
    AssertTest(numbers.back(), 7, "");

    zlist<std::string, 20> strlist;
    strlist.push_back("sss");
    strlist.clear();

    zlist<int, 3> clear_test = { 1,3,5 };
    clear_test.clear();
    clear_test.push_back(2);
    clear_test.push_back(3);
    clear_test.push_back(1);
    clear_test.clear();
    AssertTest(clear_test.size(), 0ULL, "");



    
    zlist<int, 100> bound_test;
    AssertTest(bound_test.is_valid_node((void*)((u64)&bound_test - 1 )), false, "");

    AssertTest(bound_test.is_valid_node((void*)((u64)&bound_test + sizeof(bound_test))), false, "");
    AssertTest(!bound_test.is_valid_node((void*)((u64)&bound_test + sizeof(zlist<int, 100>::node_type) * 99 )), false, "");


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

s32 hash_map_test()
{
    zhash_map<int, int, 2> hash = { {1,1}, {2,2}, {3,3}, {4,4} };
    AssertTest(hash.size(), 2U, "");
    AssertTest(hash.insert({ 8,8 }).second, false, "");
    AssertTest(hash.insert({ 8,8 }).first == hash.end(), true, "");
    hash[1] = 111;
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
    AssertTest(c.size(), s, "");
    for (auto& e : c)
    {
        if (!c.is_valid_node(&e))
        {
            volatile int a = 0;
            (void)a;
        }
        AssertTest(!c.is_valid_node(&e), false, "");
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
    AssertTest((src + 1)->size(), 10ULL, "");

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
        AssertTest(ZSortInsertLogN(za, 200), 0, "");
    }
    if (true)
    {
        zlist<size_t, 200> zl;
        AssertTest(ZSortInsertLine(zl, 200), 0, "");
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
s32 object_test()
{
    std::unique_ptr<char[]> zspace(new char[zobj_pool<u32>::static_buf_size(200U)]);
    ((zobj_pool<u32>*)zspace.get())->init(200U, zobj_pool<u32>::static_buf_size(200U));
    zobj_pool<u32>& zp = *((zobj_pool<u32>*)zspace.get());
    zlist<u32*, 200> zl;
    for (u32 i = 0; i < 200; i++)
    {
        zl.push_back(zp.create());
        if (zl.back() == NULL)
        {
            LogError();
            return -1;
        }
        *zl.back() = i;
        if (zl.size() != zp.size())
        {
            LogError();
            return -2;
        }
    }
    for (u32 i = 0; i < 200; i++)
    {
        if (*zl.front() != i)
        {
            LogError();
            return -3;
        }
        zp.destroy(zl.front());
        zl.pop_front();
        if (zl.size() != zp.size())
        {
            LogError();
            return -4;
        }
    }
    if (!zl.empty() || !zp.empty())
    {
        return -5;
    }


    for (u32 i = 1; i < 10; i++)
    {
        u32 obj_count = i;
        std::unique_ptr<char[]> space(new char[zobj_pool<size_t>::static_buf_size(i)]);
        u32 buff_size = zobj_pool<size_t>::static_buf_size(i);
        char* buff = new char[buff_size];
        zobj_pool<size_t>* op = (zobj_pool<size_t>*) space.get();
        if (op->init(obj_count, zobj_pool<size_t>::static_buf_size(i)) != 0)
        {
            return -9;
        }
        for (size_t j = 0; j < i; j++)
        {
            if (op->create() == NULL)
            {
                LogError() << "error";
                return -6;
            }
            if (op->size() != j+1)
            {
                LogError() << "error";
                return -6;
            }
        }
        if (op->create() != NULL)
        {
            LogError() << "error";
            return -7;
        }
        op->clear();
        if (!op->empty())
        {
            LogError() << "error";
            return -7;
        }
        delete[] buff;
    }
    return 0;
}

s32 contiainer_base_test()
{
    AssertTest(array_test(), 0, " array_test()");
    AssertTest(list_test(), 0, " list_test()");
    AssertTest(by_order_test(), 0, " by_order_test()");
    AssertTest(copy_test((zlist<size_t, 100>*)NULL), 0, "copy_test((zlist<size_t, 100>*)NULL)");
    AssertTest(copy_test((zarray<size_t, 100>*)NULL), 0, "copy_test((zarray<size_t, 100>*)NULL)");
    AssertTest(hash_map_test(), 0, " hash_map_test()");
    AssertTest(object_test(), 0, " object_test()");
    return 0;
}

