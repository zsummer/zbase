/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

#ifdef __APPLE__
#define ZCLOCK_NO_RDTSC
#endif

#include <algorithm>
#include <vector>
#include <random>
#include <cmath>
#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zclock.h"
#include "zstat_p2.h"

// 纯数据输出（无 timestamp/priority/file/function 前缀），便于表格对齐与外部解析
#define LOGRAW(fmt, ...) LOG_FORMAT(0, FNLog::PRIORITY_INFO, 0, 0, FNLog::LOG_PREFIX_NULL, fmt, ##__VA_ARGS__)

// v is taken by value: sorted in place below.
static f64 exact_percentile(std::vector<f64> v, f64 p)
{
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    if (p <= 0.0) return v.front();
    if (p >= 1.0) return v.back();
    f64 idx  = p * (f64)(v.size() - 1);
    size_t lo = (size_t)idx;
    if (lo >= v.size() - 1) return v.back();
    f64 frac = idx - (f64)lo;
    return v[lo] + frac * (v[lo + 1] - v[lo]);
}

// bar length ~ log2(value) position within [0, log2(max_value)], width chars.
static void emit_log_bar(f64 value, f64 max_value, int width, char* buf, int cap)
{
    int n = 0;
    if (max_value > 0.0 && value > 0.0)
    {
        f64 denom = log2(max_value + 1.0);
        if (denom > 0.0) n = (int)(log2(value + 1.0) / denom * (f64)width + 0.5);
    }
    if (n < 0) n = 0;
    if (n > cap - 1) n = cap - 1;
    for (int k = 0; k < n; ++k) buf[k] = '#';
    buf[n] = '\0';
}

// dump the 5 P² markers: estimated height + actual/desired position, bar ~ height (log-range).
static void zstat_p2_marker_dump(const char* tag, const zstat_p2& s)
{
    LOGRAW("  ---- markers [%s]  target_p=%.2f  count=%lld ----",
           tag, s.target_p(), (long long)s.count());
    LOGRAW("%s", "     mk | height        actual_pos  want_pos    bar(height,log)#...");
    f64 hmax = s.marker_height(4);
    for (int i = 0; i < 5; ++i)
    {
        char bar[64];
        emit_log_bar(s.marker_height(i), hmax, 40, bar, sizeof(bar));
        LOGRAW("    [%d] | %12.2f  %8.2f  %8.2f    %s",
               i, s.marker_height(i), s.marker_pos(i), s.marker_want_pos(i), bar);
    }
}

// run a percentile grid over one sample set; print est vs exact truth + bars, then dump p95 markers.
// raw is taken by value: fed to estimators in stream order, then reused for exact truth.
static s32 zstat_p2_curve_compare_on(const char* tag, std::vector<f64> raw)
{
    static const int kPTable[] = { 5, 25, 50, 75, 90, 95, 99 };
    const int NP = (int)(sizeof(kPTable) / sizeof(kPTable[0]));

    std::vector<zstat_p2> est;
    est.reserve(NP);
    for (int i = 0; i < NP; ++i) est.emplace_back((f64)kPTable[i] / 100.0);

    for (f64 v : raw)
        for (auto& e : est) e.add((s64)v);

    f64 est_max = 0.0;
    for (int i = 0; i < NP; ++i)
        if (est[i].quantile() > est_max) est_max = est[i].quantile();

    LOGFMTI("===== P2 quantile curve on %s (N=%d) =====", tag, (int)raw.size());
    LOGRAW("%s", "     p%   | est_height    truth         err%    bar(est,log)#...");
    for (int i = 0; i < NP; ++i)
    {
        f64 e     = est[i].quantile();
        f64 truth = exact_percentile(raw, (f64)kPTable[i] / 100.0);
        f64 err   = (truth > 0.0) ? fabs(e - truth) / truth * 100.0 : 0.0;
        char bar[64];
        emit_log_bar(e, est_max, 40, bar, sizeof(bar));
        LOGRAW("    p%-3d | %12.2f  %12.2f  %6.2f%%  %s", kPTable[i], e, truth, err, bar);
    }

    // markers belong to the p95 estimator (index 5 in kPTable).
    zstat_p2_marker_dump(tag, est[5]);
    return 0;
}

static s32 zstat_p2_curve_spike_test()
{
    struct pair_t { s64 value; int count; };
    pair_t cases[] = {
        {     100, 200 }, {  10000,  50 }, { 100000,  30 },
        { 200000,  10 }, { 500000,   5 }, { 800000,   4 }, {1000000,   1 },
    };
    std::vector<f64> raw;
    for (auto& c : cases)
        for (int i = 0; i < c.count; ++i) raw.push_back((f64)c.value);
    return zstat_p2_curve_compare_on("discrete-spike(7 spikes,N=300)", raw);
}

static s32 zstat_p2_curve_lognormal_test()
{
    const int N = 5000;
    std::mt19937 rng(20260714u);
    std::lognormal_distribution<double> dist(6.0, 1.5);
    std::vector<f64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        double x = dist(rng);
        if (x < 1.0) x = 1.0;
        raw.push_back(x);
    }
    return zstat_p2_curve_compare_on("log-normal(mu=6,sigma=1.5,N=5000)", raw);
}

static s32 zstat_p2_curve_pareto_test()
{
    const int N = 5000;
    const double xm    = 100.0;
    const double alpha = 1.2;
    std::mt19937 rng(20260715u);
    std::uniform_real_distribution<double> u01(1e-6, 1.0);
    std::vector<f64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        double x = xm / std::pow(u01(rng), 1.0 / alpha);
        if (x < 1.0) x = 1.0;
        if (x > 1e12) x = 1e12;
        raw.push_back(x);
    }
    return zstat_p2_curve_compare_on("pareto(xm=100,alpha=1.2,N=5000)", raw);
}

static s32 zstat_p2_curve_exponential_test()
{
    const int N = 5000;
    std::mt19937 rng(20260716u);
    std::exponential_distribution<double> dist(1.0 / 5000.0);
    std::vector<f64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        double x = dist(rng);
        if (x < 1.0) x = 1.0;
        raw.push_back(x);
    }
    return zstat_p2_curve_compare_on("exponential(mean=5000,N=5000)", raw);
}

static s32 zstat_p2_curve_uniform_test()
{
    const int N = 100000;
    std::mt19937 rng(20260713u);
    std::uniform_int_distribution<int> dist(0, 100000);
    std::vector<f64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i) raw.push_back((f64)dist(rng));
    return zstat_p2_curve_compare_on("uniform(0..1e5,N=100000)", raw);
}


static s32 zstat_p2_uniform_test()
{
    zstat_p2 s95(0.95);
    zstat_p2 s99(0.99);
    zstat_p2 s50(0.50);

    const int N = 10000;
    std::vector<int> raw(N);
    std::mt19937 rng(20260713u);
    std::uniform_int_distribution<int> dist(0, 9999);
    for (int i = 0; i < N; ++i)
    {
        raw[i] = dist(rng);
        s95.add(raw[i]);
        s99.add(raw[i]);
        s50.add(raw[i]);
    }
    std::sort(raw.begin(), raw.end());
    f64 truth_p50 = raw[(int)(N * 0.50)];
    f64 truth_p95 = raw[(int)(N * 0.95)];
    f64 truth_p99 = raw[(int)(N * 0.99)];

    LOGFMTI("uniform[0,9999) N=%d  p50 est=%.2f truth=%.2f", N, s50.quantile(), truth_p50);
    LOGFMTI("uniform[0,9999) N=%d  p95 est=%.2f truth=%.2f", N, s95.quantile(), truth_p95);
    LOGFMTI("uniform[0,9999) N=%d  p99 est=%.2f truth=%.2f", N, s99.quantile(), truth_p99);

    ASSERT_TEST(std::abs(s50.quantile() - truth_p50) < truth_p50 * 0.02 + 5.0);
    ASSERT_TEST(std::abs(s95.quantile() - truth_p95) < truth_p95 * 0.02 + 5.0);
    ASSERT_TEST(std::abs(s99.quantile() - truth_p99) < truth_p99 * 0.02 + 5.0);
    return 0;
}

static s32 zstat_p2_bootstrap_test()
{
    zstat_p2 s0(0.95);
    ASSERT_TEST(s0.count() == 0);
    ASSERT_TEST(s0.quantile() == 0.0);

    zstat_p2 s1(0.95);
    s1.add(42);
    ASSERT_TEST(s1.count() == 1);
    ASSERT_TEST(s1.quantile() == 42.0);

    zstat_p2 s4(0.50);
    s4.add(10); s4.add(20); s4.add(30); s4.add(40);
    ASSERT_TEST(s4.count() == 4);
    ASSERT_TEST(s4.quantile() == 30.0);

    zstat_p2 s5(0.50);
    s5.add(5); s5.add(1); s5.add(4); s5.add(2); s5.add(3);
    ASSERT_TEST(s5.count() == 5);
    ASSERT_TEST(s5.quantile() == 3.0);
    return 0;
}

static s32 zstat_p2_monotonic_test()
{
    zstat_p2 asc(0.95);
    for (int i = 1; i <= 1000; ++i) asc.add(i);
    LOGFMTI("ascending 1..1000  p95 est=%.2f truth=950", asc.quantile());
    ASSERT_TEST(std::abs(asc.quantile() - 950.0) < 30.0);

    zstat_p2 desc(0.95);
    for (int i = 1000; i >= 1; --i) desc.add(i);
    LOGFMTI("descending 1000..1  p95 est=%.2f truth=950", desc.quantile());
    ASSERT_TEST(std::abs(desc.quantile() - 950.0) < 30.0);

    zstat_p2 c(0.99);
    for (int i = 0; i < 1000; ++i) c.add(777);
    LOGFMTI("constant 777  p99 est=%.2f", c.quantile());
    ASSERT_TEST(c.quantile() == 777.0);
    return 0;
}

static s32 zstat_p2_bench_test()
{
    zstat_p2 s(0.95);
    zclock<> cost;
    volatile s64 salt = 0;
    const int N = 100 * 10000;
    cost.start();
    for (int i = 0; i < N; ++i)
    {
        s.add(i);
        salt += (s64)s.count();
    }
    cost.stop_and_save();
    LogInfo() << "zstat_p2.add() avg cost: " << (f64)cost.cost_ns() / (f64)N << " ns/op";
    return 0;
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zstat_p2_bootstrap_test()  == 0);
    ASSERT_TEST(zstat_p2_uniform_test()    == 0);
    ASSERT_TEST(zstat_p2_monotonic_test()  == 0);
    ASSERT_TEST(zstat_p2_curve_spike_test()       == 0);
    ASSERT_TEST(zstat_p2_curve_lognormal_test()   == 0);
    ASSERT_TEST(zstat_p2_curve_pareto_test()      == 0);
    ASSERT_TEST(zstat_p2_curve_exponential_test() == 0);
    ASSERT_TEST(zstat_p2_curve_uniform_test()     == 0);
    ASSERT_TEST(zstat_p2_bench_test()      == 0);

    LogInfo() << "all test finish .";
    return 0;
}
