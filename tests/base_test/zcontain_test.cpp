

#include "fn_log.h"
#include <string>
#include "zarray.h"
#include "zlist.h"
#include "zlist_ext.h"
#include "zhash_map.h"
#include "zcontain_test.h"
#include "zobj_pool.h"
using namespace zsummer;


static const int MAX_SIZE = 1000;
int rand_array[MAX_SIZE];
int sort_array[MAX_SIZE];

template<class T>
s32 CheckPushArray(T& a)
{
    for (int i = 0; i < MAX_SIZE; i++)
    {
        a.push_back(rand_array[i]);
    }
    if (a.size() != MAX_SIZE)
    {
        LogError() << "  has error";
        return -1;
    }
    return 0;
}
template<class T>
s32 CheckEmplacePushArray(T& a)
{
    for (int i = 0; i < MAX_SIZE; i++)
    {
        a.emplace_back(rand_array[i]);
    }
    if (a.size() != MAX_SIZE)
    {
        LogError() << "  has error";
        return -1;
    }
    return 0;
}

template<class T>
s32 CheckInsertBeginArray(T& a)
{
    for (int i = 0; i < MAX_SIZE; i++)
    {
        a.insert(a.begin(), rand_array[i]);
    }
    if (a.size() != MAX_SIZE)
    {
        LogError() << "  has error";
        return -1;
    }
    return 0;
}

template<class T>
s32 CheckEmplaceInsertBeginArray(T& a)
{
    for (int i = 0; i < MAX_SIZE; i++)
    {
        a.emplace(a.begin(), rand_array[i]);
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
        for (auto v : a)
        {
            if (v != rand_array[i])
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
s32 CheckRandArrayAt(T& a)
{
    if (true)
    {
        int i = 0;
        for (auto v : a)
        {
            if (v != rand_array[i])
            {
                LogError() << "  has error";
                return -1;
            }
            if (v != a.at(i))
            {
                LogError() << "  has error";
                return -1;
            }
            if (v != a[i])
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
s32 CheckSortArray(T& a)
{
    if (true)
    {
        int i = 0;
        for (auto v : a)
        {
            if (v != sort_array[i])
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
    int count = a.size();
    if (a.size() != 0)
    {
        a.pop_back();
        count--;
    }
    if (count != 0)
    {
        LogError() << " pop error";
    }
    return 0;
}

template<class T>
s32 CheckEraseArray(T& a)
{
    int count = a.size();
    a.erase(a.begin(), a.end());
    if (count != 0)
    {
        LogError() << " pop error";
    }
    return 0;
}

s32 ArrayTest()
{
    for (int i = 0; i < MAX_SIZE; i++)
    {
        rand_array[i] = rand() % MAX_SIZE;
    }

    memcpy(sort_array, rand_array, sizeof(int) * MAX_SIZE);
    std::sort(&sort_array[0], &sort_array[0] + MAX_SIZE);

    if (true)
    {
        zarray<int, MAX_SIZE> numbers;
        CheckPushArray(numbers);
        CheckRandArray(numbers);
        CheckPopArray(numbers);

        CheckEmplacePushArray(numbers);
        CheckRandArray(numbers);
        CheckPopArray(numbers);

        CheckInsertBeginArray(numbers);
        CheckRandArrayAt(numbers);
        CheckEraseArray(numbers);

        CheckEmplaceInsertBeginArray(numbers);
        CheckRandArrayAt(numbers);
        CheckEraseArray(numbers);

        CheckEmplaceInsertBeginArray(numbers);
        std::sort(numbers.begin(), numbers.end());
        CheckSortArray(numbers);
        CheckPushArray(numbers);
        CheckSortArray(numbers);
    }

    if (true)
    {
        std::vector<int> numbers;
        CheckPushArray(numbers);
        CheckRandArray(numbers);
        CheckPopArray(numbers);

        CheckEmplacePushArray(numbers);
        CheckRandArray(numbers);
        CheckPopArray(numbers);

        CheckInsertBeginArray(numbers);
        CheckRandArrayAt(numbers);
        CheckEraseArray(numbers);

        CheckEmplaceInsertBeginArray(numbers);
        CheckRandArrayAt(numbers);
        CheckEraseArray(numbers);

        CheckEmplaceInsertBeginArray(numbers);
        std::sort(numbers.begin(), numbers.end());
        CheckSortArray(numbers);
    }

    if (true)
    {
        zlist<int, MAX_SIZE> numbers;
        CheckPushArray(numbers);
        CheckRandArray(numbers);
        CheckPopArray(numbers);

        CheckEmplacePushArray(numbers);
        CheckRandArray(numbers);
        CheckPopArray(numbers);

        CheckInsertBeginArray(numbers);
        CheckRandArray(numbers);
        CheckEraseArray(numbers);

        CheckEmplaceInsertBeginArray(numbers);
        CheckRandArray(numbers);
        CheckEraseArray(numbers);

        CheckEmplaceInsertBeginArray(numbers);
    }

    if (true)
    {
        zlist_ext<int, MAX_SIZE, MAX_SIZE> numbers;
        CheckPushArray(numbers);
        CheckRandArray(numbers);
        CheckPopArray(numbers);

        CheckEmplacePushArray(numbers);
        CheckRandArray(numbers);
        CheckPopArray(numbers);

        CheckInsertBeginArray(numbers);
        CheckRandArray(numbers);
        CheckEraseArray(numbers);

        CheckEmplaceInsertBeginArray(numbers);
        CheckRandArray(numbers);
        CheckEraseArray(numbers);

        CheckEmplaceInsertBeginArray(numbers);
    }

    if (true)
    {
        zlist_ext<int, MAX_SIZE, 1> numbers;
        CheckPushArray(numbers);
        CheckRandArray(numbers);
        CheckPopArray(numbers);

        CheckEmplacePushArray(numbers);
        CheckRandArray(numbers);
        CheckPopArray(numbers);

        CheckInsertBeginArray(numbers);
        CheckRandArray(numbers);
        CheckEraseArray(numbers);

        CheckEmplaceInsertBeginArray(numbers);
        CheckRandArray(numbers);
        CheckEraseArray(numbers);

        CheckEmplaceInsertBeginArray(numbers);
    }


    zarray<std::string, 100> strings = { "123", "2" };
    zarray<std::string, 50> strings2;
    strings2.assign(strings.begin(), strings.end());
    AssertTest(strings2.size(), 2ULL, "");
    strings2.emplace_back("888");

    if (true)
    {
        RAIIVal<> rv;
        rv.reset();
        zarray<RAIIVal<>, 100> raii_array;
        AssertTest(RAIIVal<>::now_live_count_, 0U, "");
        raii_array.push_back(rv);
        AssertTest(RAIIVal<>::now_live_count_, 1U, "");
        raii_array.insert(raii_array.begin(), rv);
        AssertTest(RAIIVal<>::now_live_count_, 2U, "");
        AssertTest(RAIIVal<>::construct_count_, 3U, "");
        raii_array.clear();
        AssertTest(RAIIVal<>::now_live_count_, 0U, "");
        AssertTest(RAIIVal<>::construct_count_, 3U, "");
        AssertTest(RAIIVal<>::destroy_count_, 3U, "");

    }

    return 0;
}



template<typename _Ty, typename ToTy, typename FromTy>
s32 zlistBoundTest(ToTy to, FromTy from)
{
    using meta_list = zlist<_Ty, 0>;
    constexpr u32 obj_count = 100;
    constexpr u32 buf_size = meta_list::static_buf_size(obj_count);

    //std::unique_ptr<char> zbuf11 = std::make_unique<char>(buf_size);
    //std::unique_ptr<char> zbuf22 = std::make_unique<char>(buf_size);
    //char* zbuf1 = zbuf11.get();
    //char* zbuf2 = zbuf22.get();

    //char zbuf1[buf_size];
    //char zbuf2[buf_size];
    //memset(zbuf1, 0, buf_size);
    //memset(zbuf2, 0, buf_size);
    char* large = new char[sizeof(u64) * 3 + buf_size * 2];
    u64* fence[3] = { (u64*)large, (u64*)(large + sizeof(u64) + buf_size), (u64*)(large + sizeof(u64) * 2 + buf_size * 2) };
    *fence[0] = meta_list::FENCE_VAL;
    *fence[1] = meta_list::FENCE_VAL;
    *fence[2] = meta_list::FENCE_VAL;
    meta_list* p1 = (meta_list*)(large + sizeof(u64));
    meta_list* p2 = (meta_list*)(large + sizeof(u64) * 2 + buf_size);
    p1->init(obj_count);
    for (u32 i = 0; i < obj_count; i++)
    {
        p1->push_back(to(i));
    }
    if (std::is_trivial<_Ty>::value)
    {
#if __GNUG__ && __GNUC__ >= 5
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
        memcpy(p2, p1, buf_size);
#if __GNUG__ && __GNUC__ >= 5
#pragma GCC diagnostic pop
#endif
        
    }
    else
    {
        p2->init(obj_count);
        p2->assign(p1->begin(), p1->end());
    }
    
    AssertTest(p2->size(), p1->size(), "");
    AssertTest(p2->size(), obj_count, "");
    for (u32 i = 0; i < 100; i++)
    {
        if (from(p2->front()) != i)
        {
            AssertTest(1, 0, "");
        }
        p2->pop_front();
    }
    p2->clear();
    if (std::is_trivial<_Ty>::value)
    {
#if __GNUG__ && __GNUC__ >= 5
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
        memcpy(p2, p1, buf_size);
#if __GNUG__ && __GNUC__ >= 5
#pragma GCC diagnostic pop
#endif
        
    }
    else
    {
        *p2 = *p1;
    }
    AssertTest(p2->size(), p1->size(), "");
    AssertTest(p2->size(), obj_count, "");
    for (u32 i = 0; i < 100; i++)
    {
        if (from(p2->front()) != i)
        {
            AssertTest(1, 0, "");
        }
        p2->pop_front();
    }
    p2->clear();
    p1->clear();
    for (u32 i = 0; i < 3; i++)
    {
        AssertTest(*fence[i], meta_list::FENCE_VAL, "");
    }
    static_assert(meta_list::static_buf_size(0) == sizeof(meta_list), "");
    static_assert(meta_list::static_buf_size(100) == sizeof(zlist<_Ty, 100>), "");
    static_assert(zlist<std::string, 0>::static_buf_size(0) == sizeof(zlist<std::string, 0>), "");
    static_assert(zlist<std::string, 0>::static_buf_size(1) == sizeof(zlist<std::string, 1>), "");
    static_assert(zlist<std::string, 0>::static_buf_size(5) == sizeof(zlist<std::string, 5>), "");
    return 0;
}



#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()                                

s32 ZListTest()
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


    zlistBoundTest<int>([](u32 v) {return (int)v; }, [](int v) {return (u32)v; }); 
    zlistBoundTest<std::string>([](int v) {return std::to_string(v); }, [](std::string v)->u32 {return std::stoi(v); });

    
    zlist<int, 100> bound_test;
    AssertTest(bound_test.is_valid_node((void*)((u64)&bound_test - 1 )), false, "");

    AssertTest(bound_test.is_valid_node((void*)((u64)&bound_test + sizeof(bound_test))), false, "");
    AssertTest(!bound_test.is_valid_node((void*)((u64)&bound_test + sizeof(zlist<int, 100>::Node) * 99 )), false, "");


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

s32 ZHashMapTest()
{
    zhash_map<int, int, 2> hash = { {1,1}, {2,2}, {3,3}, {4,4} };
    AssertTest(hash.size(), 4U, "");
    AssertTest(hash.size(), 4U, "");
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
s32 ZContainCopyTest(C* ptr)
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

s32 ZSortInsertTest()
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

    return 0;
}
s32 ZObjPoolTest()
{
    std::unique_ptr<char[]> zspace(new char[zobj_pool<int>::static_buf_size(200U)]);
    ((zobj_pool<int>*)zspace.get())->init(200U, zobj_pool<int>::static_buf_size(200U));
    zobj_pool<int>& zp = *((zobj_pool<int>*)zspace.get());
    zlist<int*, 200> zl;
    for (int i = 0; i < 200; i++)
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
    for (int i = 0; i < 200; i++)
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

s32 ZContainTest()
{
    AssertTest(ArrayTest(), 0, " ArrayTest()");
    AssertTest(ZListTest(), 0, " ZListTest()");
    AssertTest(ZSortInsertTest(), 0, " ZSortInsertTest()");
    AssertTest(ZContainCopyTest((zlist<size_t, 100>*)NULL), 0, "ZContainCopyTest((zlist<size_t, 100>*)NULL)");
    AssertTest(ZContainCopyTest((zarray<size_t, 100>*)NULL), 0, "ZContainCopyTest((zarray<size_t, 100>*)NULL)");
    AssertTest(ZHashMapTest(), 0, " ZHashMapTest()");
    AssertTest(ZObjPoolTest(), 0, " ZObjPoolTest()");
    return 0;
}

