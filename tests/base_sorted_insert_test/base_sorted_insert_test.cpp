/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

#include <unordered_map>
#include <unordered_set>
#include <set>
#include "fn_log.h"
#include "zprof.h"
#include "zforeach.h"
#include "zarray.h"
#include "zlist.h"
#include "zvector.h"
#include "zhash_map.h"
#include "test_common.h"


template<class Container>
bool is_ascending(const Container& c) { return std::is_sorted(c.begin(), c.end()); }

template<class Container>
bool is_descending(const Container& c) { return std::is_sorted(c.begin(), c.end(), std::greater<typename Container::value_type>()); }


// ============================================================================
// 覆盖测试: zarray
// ============================================================================
s32 zsort_zarray_coverage_test()
{
    LogInfo() << "---- zsort_zarray_coverage_test begin ----";

    // 基础升序插入
    if (true)
    {
        zarray<int, 100> arr;
        for (int v : {5, 3, 7, 1, 9, 4}) arr.sorted_insert(v);
        ASSERT_TEST((int)arr.size() == 6);
        ASSERT_TEST(is_ascending(arr), "zarray asc order");
        ASSERT_TEST(arr[0] == 1 && arr[5] == 9);
    }

    // 空容器插入
    if (true)
    {
        zarray<int, 100> arr;
        arr.sorted_insert(42);
        ASSERT_TEST((int)arr.size() == 1 && arr[0] == 42);
    }

    // 重复元素插入
    if (true)
    {
        zarray<int, 100> arr;
        for (int v : {5, 5, 5, 3, 3}) arr.sorted_insert(v);
        ASSERT_TEST((int)arr.size() == 5);
        ASSERT_TEST(is_ascending(arr), "zarray dup asc");
        ASSERT_TEST(arr[0] == 3 && arr[2] == 5);
    }

    // 逆序插入
    if (true)
    {
        zarray<int, 100> arr;
        for (int i = 50; i > 0; i--) arr.sorted_insert(i);
        ASSERT_TEST((int)arr.size() == 50);
        ASSERT_TEST(is_ascending(arr), "zarray reverse insert asc");
        ASSERT_TEST(arr[0] == 1 && arr[49] == 50);
    }

    // 降序插入 (自定义比较器)
    if (true)
    {
        zarray<int, 100> arr;
        for (int v : {5, 3, 7, 1, 9}) arr.sorted_insert(v, std::greater<int>());
        ASSERT_TEST((int)arr.size() == 5);
        ASSERT_TEST(is_descending(arr), "zarray desc order");
        ASSERT_TEST(arr[0] == 9 && arr[4] == 1);
    }

    // 容器满时插入
    if (true)
    {
        zarray<int, 5> arr;
        for (int i = 0; i < 5; i++) arr.sorted_insert(i * 2);
        ASSERT_TEST(arr.full());
        auto it = arr.sorted_insert(100);
        ASSERT_TEST(it == arr.end(), "zarray full insert returns end");
        ASSERT_TEST((int)arr.size() == 5);
    }

    LogInfo() << "---- zsort_zarray_coverage_test passed ----";
    return 0;
}

// ============================================================================
// 覆盖测试: zlist
// ============================================================================
s32 zsort_zlist_coverage_test()
{
    LogInfo() << "---- zsort_zlist_coverage_test begin ----";

    // 基础升序插入
    if (true)
    {
        zlist<int, 100> lst;
        for (int v : {5, 3, 7, 1, 9, 4}) lst.sorted_insert(v);
        ASSERT_TEST((int)lst.size() == 6);
        ASSERT_TEST(is_ascending(lst), "zlist asc order");
    }

    // 空容器 + 重复元素
    if (true)
    {
        zlist<int, 100> lst;
        lst.sorted_insert(42);
        ASSERT_TEST((int)lst.size() == 1 && *lst.begin() == 42);
        for (int v : {5, 5, 3, 3}) lst.sorted_insert(v);
        ASSERT_TEST((int)lst.size() == 5);
        ASSERT_TEST(is_ascending(lst), "zlist dup asc");
    }

    // 逆序插入
    if (true)
    {
        zlist<int, 100> lst;
        for (int i = 50; i > 0; i--) lst.sorted_insert(i);
        ASSERT_TEST((int)lst.size() == 50);
        ASSERT_TEST(is_ascending(lst), "zlist reverse insert asc");
    }

    // 降序插入
    if (true)
    {
        zlist<int, 100> lst;
        for (int v : {5, 3, 7, 1, 9}) lst.sorted_insert(v, std::greater<int>());
        ASSERT_TEST((int)lst.size() == 5);
        ASSERT_TEST(is_descending(lst), "zlist desc order");
    }

    // 容器满时插入
    if (true)
    {
        zlist<int, 5> lst;
        for (int i = 0; i < 5; i++) lst.sorted_insert(i * 2);
        ASSERT_TEST(lst.full());
        auto it = lst.sorted_insert(100);
        ASSERT_TEST(it == lst.end(), "zlist full insert returns end");
        ASSERT_TEST((int)lst.size() == 5);
    }

    LogInfo() << "---- zsort_zlist_coverage_test passed ----";
    return 0;
}

// ============================================================================
// 覆盖测试: zvector
// ============================================================================
s32 zsort_zvector_coverage_test()
{
    LogInfo() << "---- zsort_zvector_coverage_test begin ----";

    // 全固定大小
    if (true)
    {
        zvector<int, 100, 100> vec;
        for (int v : {5, 3, 7, 1, 9, 4}) vec.sorted_insert(v);
        ASSERT_TEST((int)vec.size() == 6);
        ASSERT_TEST(is_ascending(vec), "zvector full_fixed asc");
        ASSERT_TEST(vec[0] == 1 && vec[5] == 9);
    }

    // 半固定 + 零固定
    if (true)
    {
        zvector<int, 100, 50> v1;
        zvector<int, 100, 0>  v2;
        for (int v : {5, 3, 7, 1, 9}) { v1.sorted_insert(v); v2.sorted_insert(v); }
        ASSERT_TEST(is_ascending(v1), "zvector half_fixed asc");
        ASSERT_TEST(is_ascending(v2), "zvector 0_fixed asc");
    }

    // 逆序插入
    if (true)
    {
        zvector<int, 200, 100> vec;
        for (int i = 100; i > 0; i--) vec.sorted_insert(i);
        ASSERT_TEST((int)vec.size() == 100);
        ASSERT_TEST(is_ascending(vec), "zvector reverse insert asc");
    }

    // 降序插入
    if (true)
    {
        zvector<int, 100, 100> vec;
        for (int v : {5, 3, 7, 1, 9}) vec.sorted_insert(v, std::greater<int>());
        ASSERT_TEST((int)vec.size() == 5);
        ASSERT_TEST(is_descending(vec), "zvector desc order");
    }

    // 容器满时插入
    if (true)
    {
        zvector<int, 5, 5> vec;
        for (int i = 0; i < 5; i++) vec.sorted_insert(i * 2);
        ASSERT_TEST(vec.full());
        auto it = vec.sorted_insert(100);
        ASSERT_TEST(it == vec.end(), "zvector full insert returns end");
        ASSERT_TEST((int)vec.size() == 5);
    }

    LogInfo() << "---- zsort_zvector_coverage_test passed ----";
    return 0;
}

// ============================================================================
// 覆盖测试: raii_object (非平凡类型, 构造/析构泄漏检测)
// ============================================================================
s32 zsort_raii_coverage_test()
{
    LogInfo() << "---- zsort_raii_coverage_test begin ----";

    // zarray
    if (true)
    {
        raii_object::reset();
        {
            zarray<raii_object, 100> arr;
            for (int v : {5, 3, 7, 1, 9}) arr.sorted_insert(raii_object(v));
            ASSERT_TEST((int)arr.size() == 5);
            ASSERT_TEST(is_ascending(arr), "zarray raii asc");
        }
        ASSERT_RAII_EQUAL("zarray<raii_object> sorted_insert");
    }

    // zlist
    if (true)
    {
        raii_object::reset();
        {
            zlist<raii_object, 100> lst;
            for (int v : {5, 3, 7, 1, 9}) lst.sorted_insert(raii_object(v));
            ASSERT_TEST((int)lst.size() == 5);
            ASSERT_TEST(is_ascending(lst), "zlist raii asc");
        }
        ASSERT_RAII_EQUAL("zlist<raii_object> sorted_insert");
    }

    // zvector
    if (true)
    {
        raii_object::reset();
        {
            zvector<raii_object, 100, 100> vec;
            for (int v : {5, 3, 7, 1, 9}) vec.sorted_insert(raii_object(v));
            ASSERT_TEST((int)vec.size() == 5);
            ASSERT_TEST(is_ascending(vec), "zvector raii asc");
        }
        ASSERT_RAII_EQUAL("zvector<raii_object> sorted_insert");
    }

    // raii_object 降序

    if (true)
    {
        raii_object::reset();
        {
            zarray<raii_object, 100> arr;
            for (int v : {5, 3, 7, 1, 9}) arr.sorted_insert(raii_object(v), std::greater<raii_object>());
            ASSERT_TEST((int)arr.size() == 5);
            ASSERT_TEST(is_descending(arr), "zarray raii desc");
        }
        ASSERT_RAII_EQUAL("zarray<raii_object> sorted_insert desc");
    }

    // 大量 raii_object 验证无泄漏
    if (true)
    {
        raii_object::reset();
        {
            zarray<raii_object, 1000> arr;
            srand(54321);
            for (int i = 0; i < 500; i++) arr.sorted_insert(raii_object(rand() % 10000));
            ASSERT_TEST((int)arr.size() == 500);
            ASSERT_TEST(is_ascending(arr), "zarray raii 500 random asc");
        }
        ASSERT_RAII_EQUAL("zarray<raii_object> sorted_insert 500");
    }

    LogInfo() << "---- zsort_raii_coverage_test passed ----";
    return 0;
}

// ============================================================================
// 覆盖测试: 交叉验证各容器一致性
// ============================================================================
s32 zsort_cross_validation_test()
{
    LogInfo() << "---- zsort_cross_validation_test begin ----";

    static const int TEST_COUNT = 200;
    int rand_data[TEST_COUNT];
    srand(99999);
    for (int i = 0; i < TEST_COUNT; i++) rand_data[i] = rand() % 50000;

    zarray<int, TEST_COUNT> zarr;
    zvector<int, TEST_COUNT, TEST_COUNT> zvec;
    zlist<int, TEST_COUNT> zlst;

    for (int i = 0; i < TEST_COUNT; i++)
    {
        zarr.sorted_insert(rand_data[i]);
        zvec.sorted_insert(rand_data[i]);
        zlst.sorted_insert(rand_data[i]);
    }

    ASSERT_TEST((int)zarr.size() == TEST_COUNT);
    ASSERT_TEST((int)zvec.size() == TEST_COUNT);
    ASSERT_TEST((int)zlst.size() == TEST_COUNT);

    auto it_vec = zvec.begin();
    auto it_lst = zlst.begin();
    for (int i = 0; i < TEST_COUNT; i++)
    {
        ASSERT_TEST_NOLOG(zarr[i] == *it_vec, "cross vec mismatch at:", i);
        ASSERT_TEST_NOLOG(zarr[i] == *it_lst, "cross lst mismatch at:", i);
        ++it_vec; ++it_lst;
    }


    LogInfo() << "---- zsort_cross_validation_test passed ----";
    return 0;
}

// ============================================================================
// 性能测试: 多量级对比 (100 / 1000 / 10000)
// ============================================================================
static const int MAX_BENCH_SIZE = 10000;
static int bench_rand_data[MAX_BENCH_SIZE];


static void init_bench_rand_data()
{
    srand(42);
    for (int i = 0; i < MAX_BENCH_SIZE; i++) bench_rand_data[i] = rand();
}

template<class Fn>
void bench(const char* tag, int n, int loops, Fn fn)
{
    char label[128];
    snprintf(label, sizeof(label), "[N=%5d] %s", n, tag);
    s64 total_ops = (s64)n * loops;
    PROF_DEFINE_COUNTER(cost);
    PROF_START_COUNTER(cost);
    for (int loop = 0; loop < loops; loop++)
    {
        fn(n);
    }
    PROF_OUTPUT_MULTI_COUNT_CPU(label, total_ops, cost.StopAndSave().cost());
}



template<int N>
s32 zsort_bench_n(int loops)
{
    LogInfo() << "";
    LogInfo() << "==== zsort_bench N=" << N << ", loops=" << loops << " ====";


    // --- 逐元素 sorted_insert ---
    bench("zarray sorted_insert", N, loops, [](int n) {
        zarray<int, N> c;
        for (int i = 0; i < n; i++) c.sorted_insert(bench_rand_data[i]);
    });
    bench("zvector(fix)  sorted_insert", N, loops, [](int n) {
        zvector<int, N, N> c;
        for (int i = 0; i < n; i++) c.sorted_insert(bench_rand_data[i]);
    });
    bench("zhash_set     insert(unique)", N, loops, [](int n) {
        zhash_set<int, N> c;
        for (int i = 0; i < n; i++) c.insert(bench_rand_data[i]);
        });
    bench("zlist         sorted_insert", N, loops, [](int n) {
        zlist<int, N> c;
        for (int i = 0; i < n; i++) c.sorted_insert(bench_rand_data[i]);
    });

    bench("std::set      insert(unique)", N, loops, [](int n) {
        std::set<int> c;
        for (int i = 0; i < n; i++) c.insert(bench_rand_data[i]);
    });
    bench("std::multiset insert(dup)  ", N, loops, [](int n) {
        std::multiset<int> c;
        for (int i = 0; i < n; i++) c.insert(bench_rand_data[i]);
    });



    // --- 整批排序方案 ---
    bench("zarray push_back+sort", N, loops, [](int n) {
        zarray<int, N> c;
        for (int i = 0; i < n; i++) c.push_back(bench_rand_data[i]);
        std::sort(c.begin(), c.end()); });


    bench("vec push_back+sort  (batch)", N, loops, [](int n) {
        std::vector<int> c; c.reserve(n);
        for (int i = 0; i < n; i++) c.push_back(bench_rand_data[i]);
        std::sort(c.begin(), c.end());
    });

    bench("zhash_set insert    (batch)", N, loops, [](int n) {
        zhash_set<int, N> c;
        for (int i = 0; i < n; i++) c.insert(bench_rand_data[i]);
        });


    bench("set insert          (batch)", N, loops, [](int n) {
        std::set<int> c;
        for (int i = 0; i < n; i++) c.insert(bench_rand_data[i]);
    });
    bench("multiset insert     (batch)", N, loops, [](int n) {
        std::multiset<int> c;
        for (int i = 0; i < n; i++) c.insert(bench_rand_data[i]);
    });
    bench("multiset+copy to vec(batch)", N, loops, [](int n) {
        std::multiset<int> ms;
        for (int i = 0; i < n; i++) ms.insert(bench_rand_data[i]);
        std::vector<int> v(ms.begin(), ms.end()); (void)v;
    });



    LogInfo() << "==== zsort_bench N=" << N << " done ====";
    return 0;
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char *argv[])
{
    ztest_init();
    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");
    LogDebug() << " main begin test. ";

    // 覆盖测试
    ASSERT_TEST(zsort_zarray_coverage_test() == 0);
    ASSERT_TEST(zsort_zlist_coverage_test() == 0);
    ASSERT_TEST(zsort_zvector_coverage_test() == 0);
    ASSERT_TEST(zsort_raii_coverage_test() == 0);

    ASSERT_TEST(zsort_cross_validation_test() == 0);

    // 性能测试: 100 / 1000 / 10000
    init_bench_rand_data();
    ASSERT_TEST(zsort_bench_n<100>(100) == 0);
    ASSERT_TEST(zsort_bench_n<1000>(100) == 0);
    ASSERT_TEST(zsort_bench_n<10000>(100) == 0);

    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();
    LogInfo() << "all test finish .";
    return 0;
}