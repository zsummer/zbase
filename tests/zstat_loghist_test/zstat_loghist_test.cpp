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
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zclock.h"
#include "zstat_loghist.h"

// 纯数据输出（无 timestamp/priority/file/function 前缀），用于表格与 CSV，便于外部工具直接解析
#define LOGRAW(fmt, ...) LOG_FORMAT(0, FNLog::PRIORITY_INFO, 0, 0, FNLog::LOG_PREFIX_NULL, fmt, ##__VA_ARGS__)

static inline f64 exact_percentile(std::vector<s64>& v, f64 p)
{
    std::sort(v.begin(), v.end());
    if (v.empty()) return 0.0;
    if (p <= 0.0) return (f64)v.front();
    if (p >= 1.0) return (f64)v.back();
    f64 idx_f = p * (f64)(v.size() - 1);
    size_t lo = (size_t)idx_f;
    if (lo >= v.size() - 1) return (f64)v.back();
    f64 frac = idx_f - (f64)lo;
    return (f64)v[lo] + frac * (f64)(v[lo + 1] - v[lo]);
}

template<class Hist>
static void dump_histogram(const char* title, const Hist& h)
{
    LogInfo() << "==== histogram: " << title
              << "  buckets=" << h.bucket_count()
              << "  unit_shift=" << h.unit_shift()
              << "  req_count=" << h.req_count()
              << "  valid_count=" << h.valid_count()
              << "  req_underflow_count=" << h.req_underflow_count()
              << "  req_overflow_count=" << h.req_overflow_count()
              << "  valid_low=" << h.valid_low()
              << "  valid_high=" << h.valid_high()
              << "  valid_avg=" << h.valid_avg();

    s64 max_bucket_count = 0;
    for (int i = 0; i < h.bucket_count(); ++i)
    {
        if (h.bucket_valid_count(i) > max_bucket_count) max_bucket_count = h.bucket_valid_count(i);
    }
    if (max_bucket_count == 0)
    {
        LogInfo() << "  (all buckets empty)";
        return;
    }

    const int BAR_MAX = 40;
    for (int i = 0; i < h.bucket_count(); ++i)
    {
        s64 bucket_valid_count = h.bucket_valid_count(i);
        if (bucket_valid_count == 0) continue;
        int bar = (int)((bucket_valid_count * BAR_MAX + max_bucket_count - 1) / max_bucket_count);
        char bar_buf[64] = {0};
        for (int k = 0; k < bar && k < 60; ++k) bar_buf[k] = '#';
        auto r = h.bucket_range(i);
        LOGFMTI("  bkt[%3d] [%12lld, %12lld) %s %lld",
                i,
                r.first, r.second,
                bar_buf, (long long)bucket_valid_count);
    }
}

static s32 zstat_loghist_boundary_test()
{
    using H = zstat_loghist<3, 57>;
    static_assert(H::kBucketCount == 440, "bucket count for <3,57> must be 440");
    static_assert(H::kSubCount    == 8,   "sub-count for SubBits=3 must be 8");

    H h;
    h.reset(0);
    for (int i = 0; i < 8; ++i)
    {
        auto r = h.bucket_range(i);
        ASSERT_TEST(r.first  == i);
        ASSERT_TEST(r.second == i + 1);
    }
    {
        auto r8  = h.bucket_range(8);
        auto r15 = h.bucket_range(15);
        auto r16 = h.bucket_range(16);
        ASSERT_TEST(r8.first   == 8);
        ASSERT_TEST(r8.second  == 9);
        ASSERT_TEST(r15.first  == 15);
        ASSERT_TEST(r15.second == 16);
        ASSERT_TEST(r16.first  == 16);
        ASSERT_TEST(r16.second == 18);
    }

    h.reset(6);
    LogInfo() << "unit_shift=6  sequence of first 12 bucket lows:";
    for (int i = 0; i < 12; ++i)
    {
        auto r = h.bucket_range(i);
        LOGFMTI("  bkt[%d] low=%lld high=%lld", i, r.first, r.second);
    }
    s64 expect[12] = {0, 64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704};
    for (int i = 0; i < 12; ++i)
    {
        ASSERT_TEST(h.bucket_range(i).first == expect[i]);
    }

    h.reset(-5);      ASSERT_TEST(h.unit_shift() == 0);
    h.reset(100);     ASSERT_TEST(h.unit_shift() == 63 - 57);
    h.reset(0);
    return 0;
}

static s32 zstat_loghist_edge_test()
{
    zstat_loghist<> h;
    h.reset(0);

    ASSERT_TEST(h.req_count() == 0);
    ASSERT_TEST(h.valid_count() == 0);
    ASSERT_TEST(h.valid_sum() == 0.0);
    ASSERT_TEST(h.valid_avg() == 0.0);
    ASSERT_TEST(h.valid_low() == 0);
    ASSERT_TEST(h.valid_high() == 0);
    ASSERT_TEST(h.valid_low_bucket_idx() == -1);
    ASSERT_TEST(h.valid_high_bucket_idx() == -1);
    ASSERT_TEST(h.valid_bucket_low() == 0);
    ASSERT_TEST(h.valid_bucket_high() == 0);
    ASSERT_TEST(h.quantile(0.5).height == 0.0);

    h.add(-1);
    h.add(-100);
    ASSERT_TEST(h.req_count() == 2);
    ASSERT_TEST(h.req_underflow_count() == 2);
    ASSERT_TEST(h.req_underflow_sum() == -101.0);
    ASSERT_TEST(h.valid_count() == 0);
    ASSERT_TEST(h.valid_sum() == 0.0);
    ASSERT_TEST(h.valid_low() == 0);
    ASSERT_TEST(h.valid_high() == 0);
    ASSERT_TEST(h.valid_low_bucket_idx() == -1);
    ASSERT_TEST(h.valid_high_bucket_idx() == -1);
    ASSERT_TEST(h.quantile(0.5).height == 0.0);

    h.add(LLONG_MAX);
    ASSERT_TEST(h.req_overflow_count() == 1);
    ASSERT_TEST(h.req_overflow_sum() == (f64)LLONG_MAX);
    ASSERT_TEST(h.valid_high() == 0);

    h.add(5);
    ASSERT_TEST(h.valid_count() == 1);
    ASSERT_TEST(h.valid_sum() == 5.0);
    ASSERT_TEST(h.valid_avg() == 5.0);
    ASSERT_TEST(h.valid_low() == 5);
    ASSERT_TEST(h.valid_high() == 5);
    ASSERT_TEST(h.valid_low_bucket_idx() == 5);
    ASSERT_TEST(h.valid_high_bucket_idx() == 5);
    ASSERT_TEST(h.valid_bucket_low() == 5);
    ASSERT_TEST(h.valid_bucket_high() == 6);
    ASSERT_TEST(h.quantile(0.5).height > 4.99 && h.quantile(0.5).height < 6.01);

    h.reset(3);
    ASSERT_TEST(h.req_count() == 0);
    ASSERT_TEST(h.req_underflow_count() == 0);
    ASSERT_TEST(h.req_overflow_count() == 0);
    ASSERT_TEST(h.req_underflow_sum() == 0.0);
    ASSERT_TEST(h.req_overflow_sum() == 0.0);
    ASSERT_TEST(h.valid_sum() == 0.0);
    ASSERT_TEST(h.valid_low_bucket_idx() == -1);
    ASSERT_TEST(h.valid_high_bucket_idx() == -1);
    ASSERT_TEST(h.unit_shift() == 3);
    return 0;
}

static s32 zstat_loghist_window_safety_test()
{
    zstat_loghist<> h;
    h.reset(0);
    for (int i = 0; i < h.bucket_count(); ++i)
    {
        h.reset(0);
        auto r   = h.bucket_range(i);
        s64 mid  = r.first + (r.second - r.first) / 2;
        h.add(mid);
        ASSERT_TEST(h.valid_count() == 1);
        ASSERT_TEST(h.bucket_valid_count(i) == 1, "expected only bucket ", i, " has valid count. mid=", mid);
    }
    LogInfo() << "window safety: every one of "
              << zstat_loghist<>::kBucketCount
              << " bucket-midpoints lands in its own bucket. OK.";
    return 0;
}

static s32 zstat_loghist_userdist_test()
{
    struct pair_t { s64 value; int count; };
    pair_t cases[] = {
        {     100, 200 },
        {  10000,  50 },
        { 100000,  30 },
        { 200000,  10 },
        { 500000,   5 },
        { 800000,   4 },
        {1000000,   1 },
    };
    const int TOTAL = 200 + 50 + 30 + 10 + 5 + 4 + 1;

    zstat_loghist<> h;
    h.reset(0);
    std::vector<s64> raw;
    raw.reserve(TOTAL);
    for (auto& c : cases)
    {
        for (int i = 0; i < c.count; ++i)
        {
            h.add(c.value);
            raw.push_back(c.value);
        }
    }
    ASSERT_TEST(h.req_count() == TOTAL);
    ASSERT_TEST(h.valid_count() == TOTAL);
    ASSERT_TEST(h.valid_low() == 100);
    ASSERT_TEST(h.valid_high() == 1000000);

    f64 truth_p50 = exact_percentile(raw, 0.50);
    f64 truth_p90 = exact_percentile(raw, 0.90);
    f64 truth_p95 = exact_percentile(raw, 0.95);
    f64 truth_p99 = exact_percentile(raw, 0.99);
    f64 est_p50 = h.quantile(0.50).height;
    f64 est_p90 = h.quantile(0.90).height;
    f64 est_p95 = h.quantile(0.95).height;
    f64 est_p99 = h.quantile(0.99).height;

    LOGFMTI("userdist p50: est=%.1f truth=%.1f  rel_err=%.2f%%",
            est_p50, truth_p50, (truth_p50 > 0 ? fabs(est_p50 - truth_p50) / truth_p50 * 100.0 : 0.0));
    LOGFMTI("userdist p90: est=%.1f truth=%.1f  rel_err=%.2f%%",
            est_p90, truth_p90, (truth_p90 > 0 ? fabs(est_p90 - truth_p90) / truth_p90 * 100.0 : 0.0));
    LOGFMTI("userdist p95: est=%.1f truth=%.1f  rel_err=%.2f%%",
            est_p95, truth_p95, (truth_p95 > 0 ? fabs(est_p95 - truth_p95) / truth_p95 * 100.0 : 0.0));
    LOGFMTI("userdist p99: est=%.1f truth=%.1f  rel_err=%.2f%%",
            est_p99, truth_p99, (truth_p99 > 0 ? fabs(est_p99 - truth_p99) / truth_p99 * 100.0 : 0.0));

    ASSERT_TEST(truth_p50 > 0 && fabs(est_p50 - truth_p50) / truth_p50 < 0.15);
    ASSERT_TEST(truth_p90 > 0 && fabs(est_p90 - truth_p90) / truth_p90 < 0.15);
    ASSERT_TEST(truth_p95 > 0 && fabs(est_p95 - truth_p95) / truth_p95 < 0.15);
    ASSERT_TEST(truth_p99 > 0 && fabs(est_p99 - truth_p99) / truth_p99 < 0.15);

    dump_histogram("user distribution [200x100ns,50x10us,30x100us,10x200us,5x500us,4x800us,1x1ms]", h);
    return 0;
}

static s32 zstat_loghist_userdist_shifted_test()
{
    struct pair_t { s64 value; int count; };
    pair_t cases[] = {
        {     100, 200 },
        {  10000,  50 },
        { 100000,  30 },
        { 200000,  10 },
        { 500000,   5 },
        { 800000,   4 },
        {1000000,   1 },
    };
    zstat_loghist<> h;
    h.reset(6);

    for (auto& c : cases)
        for (int i = 0; i < c.count; ++i) h.add(c.value);

    ASSERT_TEST(h.req_count() == 300);
    ASSERT_TEST(h.valid_low() == 100);
    ASSERT_TEST(h.valid_high() == 1000000);
    ASSERT_TEST(h.valid_low_bucket_idx() == 1);
    ASSERT_TEST(h.valid_high_bucket_idx() == 51);
    ASSERT_TEST(h.valid_bucket_low() == 64);
    ASSERT_TEST(h.valid_bucket_high() == 1048576);

    ASSERT_TEST(h.bucket_valid_count(1) == 200);
    {
        auto r = h.bucket_range(1);
        ASSERT_TEST(r.first  == 64);
        ASSERT_TEST(r.second == 128);
    }

    dump_histogram("user distribution (unit_shift=6)", h);
    return 0;
}

static s32 zstat_loghist_uniform_stress_test()
{
    zstat_loghist<> h;
    h.reset(0);
    const int N = 100000;
    std::vector<s64> raw;
    raw.reserve(N);
    std::mt19937 rng(20260713u);
    std::uniform_int_distribution<int> dist(0, 100000);
    for (int i = 0; i < N; ++i)
    {
        s64 v = dist(rng);
        h.add(v);
        raw.push_back(v);
    }
    f64 truth = exact_percentile(raw, 0.95);
    f64 est   = h.quantile(0.95).height;
    LOGFMTI("uniform stress N=%d p95: est=%.1f truth=%.1f rel_err=%.2f%%",
            N, est, truth, fabs(est - truth) / truth * 100.0);
    ASSERT_TEST(fabs(est - truth) / truth < 0.10);
    return 0;
}

static s32 zstat_loghist_template_test()
{
    using H4 = zstat_loghist<4, 32>;
    static_assert(H4::kBucketCount == 464, "");
    static_assert(H4::kSubCount    == 16,  "");

    H4 h;
    h.reset(0);
    h.add(100);
    h.add(1000);
    h.add(1000000);
    ASSERT_TEST(h.valid_count() == 3);

    using H1 = zstat_loghist<1, 16>;
    static_assert(H1::kBucketCount == 32, "");
    static_assert(H1::kSubCount    == 2,  "");
    return 0;
}

static s32 zstat_loghist_centroid_compare_on(const char* tag, std::vector<s64> raw);

static s32 zstat_loghist_centroid_compare_test()
{
    struct pair_t { s64 value; int count; };
    pair_t cases[] = {
        {     100, 200 },
        {  10000,  50 },
        { 100000,  30 },
        { 200000,  10 },
        { 500000,   5 },
        { 800000,   4 },
        {1000000,   1 },
    };
    std::vector<s64> raw;
    for (auto& c : cases)
        for (int i = 0; i < c.count; ++i) raw.push_back(c.value);
    return zstat_loghist_centroid_compare_on("discrete-spike(7 spikes,N=300)", raw);
}

// -----------------------------------------------------------------------------
//   Continuous-distribution centroid compare helper
// -----------------------------------------------------------------------------
// Runs the 5-mode comparison over a dense percentile grid (p1..p99, step=1),
// prints:
//   (1) per-distribution table       (20 rows)
//   (2) per-distribution summary     (avg / max err% per mode + coverage)
//   (3) per-distribution CSV block   (raw matrix, ready to plot)
// and accumulates results into g_dist_summaries for the cross-distribution
// summary matrix printed at the end of the run.

struct dist_summary_t
{
    char tag[128];
    int  n_samples;
    int  nonempty_buckets;
    int  max_empty_gap;
    f64  avg_err[5];
    f64  max_err[5];
};

static std::vector<dist_summary_t> g_dist_summaries;

// raw is taken by value: exact_percentile() sorts it in place below.
static s32 zstat_loghist_centroid_compare_on(const char* tag, std::vector<s64> raw)
{
    zstat_loghist<2, 32, kCentroidOff>              h0;
    zstat_loghist<2, 32, kCentroidLocalLinear>      h1;
    zstat_loghist<2, 32, kCentroidNeighborLinear>   h2;
    zstat_loghist<2, 32, kCentroidLocalLagrange>    h3;
    zstat_loghist<2, 32, kCentroidNeighborLagrange> h4;
    h0.reset(0); h1.reset(0); h2.reset(0); h3.reset(0); h4.reset(0);

    for (s64 v : raw)
    {
        h0.add(v); h1.add(v); h2.add(v); h3.add(v); h4.add(v);
    }

    // reduced percentile grid: 5,10,15,...,90,95,99  (20 points)
    static const int kPTable[] = {
        5, 10, 15, 20, 25, 30, 35, 40, 45, 50,
        55, 60, 65, 70, 75, 80, 85, 90, 95, 99
    };
    const int NP = (int)(sizeof(kPTable) / sizeof(kPTable[0]));
    const char* names[5] = {
        "M0 uniform          ",
        "M1 local-linear     ",
        "M2 neighbor-linear  ",
        "M3 local-parabola   ",
        "M4 neighbor-parabola"
    };

    LOGFMTI("===== quantile modes compare  (2,32) on %s (%d samples) =====",
            tag, (int)raw.size());
    LOGRAW("  %-4s | %-12s | %-14s %-7s | %-14s %-7s | %-14s %-7s | %-14s %-7s | %-14s %-7s",
           "p", "truth",
           "M0 uniform",           "err%",
           "M1 local-linear",      "err%",
           "M2 neighbor-linear",   "err%",
           "M3 local-parabola",    "err%",
           "M4 neighbor-parabola", "err%");

    f64 sum_err[5] = {0,0,0,0,0};
    f64 max_err[5] = {0,0,0,0,0};
    // full-table rows
    for (int pi = 0; pi < NP; ++pi)
    {
        int ip    = kPTable[pi];
        f64 p     = (f64)ip / 100.0;
        f64 truth = exact_percentile(raw, p);
        f64 est[5] = {
            h0.quantile(p).height, h1.quantile(p).height, h2.quantile(p).height,
            h3.quantile(p).height, h4.quantile(p).height
        };
        f64 err[5];
        for (int k = 0; k < 5; ++k)
        {
            err[k] = truth > 0 ? fabs(est[k] - truth) / truth * 100.0 : 0.0;
            sum_err[k] += err[k];
            if (err[k] > max_err[k]) max_err[k] = err[k];
        }
        LOGRAW("  p%-3d | %-12.1f | %-14.1f %6.2f%% | %-14.1f %6.2f%% | %-14.1f %6.2f%% | %-14.1f %6.2f%% | %-14.1f %6.2f%%",
               ip, truth,
               est[0], err[0], est[1], err[1],
               est[2], err[2], est[3], err[3], est[4], err[4]);
    }
    LOGRAW("%s", "  ---- summary (avg / max err%) ----");
    for (int k = 0; k < 5; ++k)
    {
        LOGRAW("    mode %s  avg_err=%6.2f%%  max_err=%6.2f%%",
               names[k], sum_err[k] / (f64)NP, max_err[k]);
    }

    // report how many non-empty buckets got covered (=  neighbor prerequisite)
    int nonempty = 0, max_gap = 0, cur_gap = 0, prev_i = -2;
    for (int i = 0; i < h0.bucket_count(); ++i)
    {
        if (h0.bucket_valid_count(i) > 0)
        {
            if (prev_i >= 0)
            {
                cur_gap = i - prev_i - 1;
                if (cur_gap > max_gap) max_gap = cur_gap;
            }
            prev_i = i;
            ++nonempty;
        }
    }
    LOGRAW("  coverage: non-empty buckets=%d  max-empty-gap=%d", nonempty, max_gap);

    // ---- accumulate into global cross-distribution summary
    dist_summary_t s;
    memset(&s, 0, sizeof(s));
    size_t tag_len = strlen(tag);
    if (tag_len >= sizeof(s.tag)) tag_len = sizeof(s.tag) - 1;
    memcpy(s.tag, tag, tag_len);
    s.tag[tag_len]         = '\0';
    s.n_samples            = (int)raw.size();
    s.nonempty_buckets     = nonempty;
    s.max_empty_gap        = max_gap;
    for (int k = 0; k < 5; ++k)
    {
        s.avg_err[k] = sum_err[k] / (f64)NP;
        s.max_err[k] = max_err[k];
    }
    g_dist_summaries.push_back(s);
    return 0;
}

static s32 zstat_loghist_centroid_lognormal_test()
{
    // log-normal: ln(X) ~ N(mu, sigma).  choose mu/sigma so range spans ~5 orders of magnitude.
    const int N = 5000;
    std::mt19937 rng(20260714u);
    std::lognormal_distribution<double> dist(6.0 /*mu*/, 1.5 /*sigma*/);
    std::vector<s64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        double x = dist(rng);
        if (x < 1.0) x = 1.0;
        raw.push_back((s64)x);
    }
    return zstat_loghist_centroid_compare_on("log-normal(mu=6,sigma=1.5,N=5000)", raw);
}

static s32 zstat_loghist_centroid_pareto_test()
{
    // Pareto(alpha): X = xm / U^(1/alpha), U ~ U(0,1).  heavy right tail but continuous.
    const int N = 5000;
    const double xm    = 100.0;
    const double alpha = 1.2;
    std::mt19937 rng(20260715u);
    std::uniform_real_distribution<double> u01(1e-6, 1.0);
    std::vector<s64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        double u = u01(rng);
        double x = xm / std::pow(u, 1.0 / alpha);
        if (x < 1.0) x = 1.0;
        if (x > 1e12) x = 1e12;
        raw.push_back((s64)x);
    }
    return zstat_loghist_centroid_compare_on("pareto(xm=100,alpha=1.2,N=5000)", raw);
}

static s32 zstat_loghist_centroid_exponential_test()
{
    // Exponential(lambda): light tail, dense small values.
    const int N = 5000;
    std::mt19937 rng(20260716u);
    std::exponential_distribution<double> dist(1.0 / 5000.0);   // mean = 5000
    std::vector<s64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        double x = dist(rng);
        if (x < 1.0) x = 1.0;
        raw.push_back((s64)x);
    }
    return zstat_loghist_centroid_compare_on("exponential(mean=5000,N=5000)", raw);
}

static s32 zstat_loghist_centroid_uniform_test()
{
    // Uniform integer distribution over a wide range: continuous coverage,
    // dense adjacent non-empty buckets, ideal case for neighbor interpolation.
    const int N = 100000;
    std::mt19937 rng(20260713u);
    std::uniform_int_distribution<int> dist(0, 100000);
    std::vector<s64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i) raw.push_back((s64)dist(rng));
    return zstat_loghist_centroid_compare_on("uniform(0..1e5,N=100000)", raw);
}

static s32 zstat_loghist_centroid_cross_summary()
{
    if (g_dist_summaries.empty())
    {
        LogInfo() << "cross summary: no distributions collected.";
        return 0;
    }

    const char* names[5] = {
        "M0 uniform          ",
        "M1 local-linear     ",
        "M2 neighbor-linear  ",
        "M3 local-parabola   ",
        "M4 neighbor-parabola"
    };

    LogInfo() << "============ CROSS-DISTRIBUTION SUMMARY  (avg err% / max err%) ============";
    LOGRAW("  %-40s | %-16s | %-16s | %-16s | %-16s | %-16s | %-14s",
           "distribution",
           names[0], names[1], names[2], names[3], names[4],
           "coverage(gap)");
    for (const auto& s : g_dist_summaries)
    {
        LOGRAW("  %-40s | %6.2f / %-6.2f  | %6.2f / %-6.2f  | %6.2f / %-6.2f  | %6.2f / %-6.2f  | %6.2f / %-6.2f  | %5d / %-6d",
               s.tag,
               s.avg_err[0], s.max_err[0],
               s.avg_err[1], s.max_err[1],
               s.avg_err[2], s.max_err[2],
               s.avg_err[3], s.max_err[3],
               s.avg_err[4], s.max_err[4],
               s.nonempty_buckets, s.max_empty_gap);
    }

    // column-wise avg across distributions (each dist weighted equally)
    f64 col_avg_avg[5] = {0,0,0,0,0};
    f64 col_avg_max[5] = {0,0,0,0,0};
    for (const auto& s : g_dist_summaries)
    {
        for (int k = 0; k < 5; ++k)
        {
            col_avg_avg[k] += s.avg_err[k];
            col_avg_max[k] += s.max_err[k];
        }
    }
    f64 nd = (f64)g_dist_summaries.size();
    for (int k = 0; k < 5; ++k)
    {
        col_avg_avg[k] /= nd;
        col_avg_max[k] /= nd;
    }
    LOGRAW("  %-40s | %6.2f / %-6.2f  | %6.2f / %-6.2f  | %6.2f / %-6.2f  | %6.2f / %-6.2f  | %6.2f / %-6.2f  | %-14s",
           "-- mean across distributions --",
           col_avg_avg[0], col_avg_max[0],
           col_avg_avg[1], col_avg_max[1],
           col_avg_avg[2], col_avg_max[2],
           col_avg_avg[3], col_avg_max[3],
           col_avg_avg[4], col_avg_max[4],
           "");

    return 0;
}

// -----------------------------------------------------------------------------
//   bucket_info visual dump (two figures, all 5 centroid modes)
// -----------------------------------------------------------------------------
// For every distribution we print, per centroid-mode, two ASCII figures:
//   (A) BUCKET view    : one row per non-empty bucket. Bar width is
//                        proportional to bucket_valid_count / total (the fraction of
//                        traffic in this bucket, i.e. the quantile *width* it
//                        occupies). We also print the cumulative quantile
//                        interval [p_lo, p_hi) this bucket covers.
//   (B) QUANTILE view  : 33 rows, p = 0/32, 1/32 .. 32/32. For each p we call
//                        histogram.quantile(p) and plot the estimated height.
//                        Bar length reflects the position of log2(height)
//                        inside the run's own value-range
//                        [quant(0).height, quant(1).height] on a log2 scale,
//                        so p=0 -> empty bar, p=1 -> full 40-# bar, and the
//                        middle rows expose the shape of the log-CDF^-1
//                        (S-curve for log-normal, concave for pareto, etc.).
// Together they let you eyeball, from the same histogram:
//   * shape of density (fig A: wider bar == more traffic in that bucket)
//   * shape of the quantile-value curve (fig B: bar length grows with value)
// -----------------------------------------------------------------------------
static s32 zstat_loghist_bucket_info_compare_on(const char* tag, const std::vector<s64>& raw)
{
    zstat_loghist<2, 32, kCentroidOff>              h0;
    zstat_loghist<2, 32, kCentroidLocalLinear>      h1;
    zstat_loghist<2, 32, kCentroidNeighborLinear>   h2;
    zstat_loghist<2, 32, kCentroidLocalLagrange>    h3;
    zstat_loghist<2, 32, kCentroidNeighborLagrange> h4;
    h0.reset(0); h1.reset(0); h2.reset(0); h3.reset(0); h4.reset(0);
    for (s64 v : raw) { h0.add(v); h1.add(v); h2.add(v); h3.add(v); h4.add(v); }

    const char* mode_names[5] = {
        "M0 uniform",
        "M1 local-linear",
        "M2 neighbor-linear",
        "M3 local-parabola",
        "M4 neighbor-parabola"
    };

    // enumerate non-empty buckets from M0 (shape is mode-independent)
    std::vector<int> non_empty;
    for (int i = 0; i < h0.bucket_count(); ++i)
        if (h0.bucket_valid_count(i) > 0) non_empty.push_back(i);

    // total count for the bucket-fraction bar in figure A.
    s64 total_valid_count = 0;
    for (int i : non_empty) total_valid_count += h0.bucket_valid_count(i);

    // for figure A: use the max bucket_valid_count so the fullest bucket == 40-wide bar
    s64 max_bucket_count = 0;
    for (int i : non_empty)
        if (h0.bucket_valid_count(i) > max_bucket_count) max_bucket_count = h0.bucket_valid_count(i);
    f64 max_frac = (total_valid_count > 0) ? (f64)max_bucket_count / (f64)total_valid_count : 1.0;

    // per-mode dispatch shim: bucket_info() and quantile() switching by m.
    auto bkt_info = [&](int m, int i) -> zstat_loghist_hit_result
    {
        switch (m)
        {
        case 0: return h0.bucket_info(i);
        case 1: return h1.bucket_info(i);
        case 2: return h2.bucket_info(i);
        case 3: return h3.bucket_info(i);
        default: return h4.bucket_info(i);
        }
    };
    auto quant = [&](int m, f64 p) -> zstat_loghist_hit_result
    {
        switch (m)
        {
        case 0: return h0.quantile(p);
        case 1: return h1.quantile(p);
        case 2: return h2.quantile(p);
        case 3: return h3.quantile(p);
        default: return h4.quantile(p);
        }
    };

    // "#-bar" emitter shared by both figures. n = round(value / max * width).
    auto emit_bar = [](f64 value, f64 max_value, int width, char* buf, int cap)
    {
        int n = 0;
        if (max_value > 0.0 && value > 0.0)
        {
            n = (int)(value / max_value * (f64)width + 0.5);
            if (n < 1) n = 1;   // ensure any non-zero row shows at least one '#'
        }
        if (n > cap - 1) n = cap - 1;
        for (int k = 0; k < n; ++k) buf[k] = '#';
        buf[n] = '\0';
    };

    LOGFMTI("===== bucket_info dump on %s (N=%d, non_empty=%d, total=%lld) =====",
            tag, (int)raw.size(), (int)non_empty.size(), (long long)total_valid_count);

    for (int m = 0; m < 5; ++m)
    {
        LOGRAW("  ==== mode [%s] ====", mode_names[m]);

        // ------- Figure A: per-bucket density + quantile interval -------
        LOGRAW("%s",
               "  [A] BUCKET view     : bar ~ bucket_frac / max_frac  (max_frac == fullest bucket)");
        LOGRAW("%s",
               "     idx | [        low,        high)   height valid_count [p_lo   , p_hi   )  bar#... p%");
        s64 cum = 0;
        for (int i : non_empty)
        {
            zstat_loghist_hit_result info = bkt_info(m, i);
            s64 bucket_valid_count = (s64)info.bucket_valid_count;
            f64 p_lo = (total_valid_count > 0) ? (f64)cum          / (f64)total_valid_count : 0.0;
            cum += bucket_valid_count;
            f64 p_hi = (total_valid_count > 0) ? (f64)cum          / (f64)total_valid_count : 0.0;
            f64 frac = p_hi - p_lo;

            char bar[80];
            emit_bar(frac, max_frac, 40, bar, sizeof(bar));
            LOGRAW("    [%3d] | [%11lld, %11lld) %10.2f  %6lld   [%7.4f, %7.4f)  %s %6.2f%%",
                   info.bucket_idx,
                   (long long)info.bucket_low, (long long)info.bucket_high,
                   info.height, (long long)bucket_valid_count, p_lo, p_hi, bar, p_hi * 100.0);
        }

        // ------- Figure B: quantile curve at 1/32 step -------
        // Bar length is taken directly from res.log2_height (0..60), rescaled
        // to width 40 == 60 * 40 / 60, so we do NOT redo any log2 math here;
        // the histogram already encodes the exact log-range position we want.
        const int STEPS = 32;
        LOGRAW("%s",
               "  [B] QUANTILE view   : bar ~ res.log2_height rescaled to 40 (log-range position of height within valid_bucket_high)");
        LOGRAW("%s",
               "     p%     | est_height    bucket_idx  [   low,    high)  bar(log-range)#... p%");
        for (int s = 0; s <= STEPS; ++s)
        {
            f64 p = (f64)s / (f64)STEPS;
            zstat_loghist_hit_result q = quant(m, p);
            int n = q.log2_height * 40 / 60;
            char bar[80];
            for (int k = 0; k < n; ++k) bar[k] = '#';
            bar[n] = '\0';
            LOGRAW("    %6.2f%% | %11.2f   [%3d]     [%6lld, %7lld)  %-40s %6.2f%%",
                   p * 100.0, q.height, q.bucket_idx,
                   (long long)q.bucket_low, (long long)q.bucket_high, bar, p * 100.0);
        }
    }

    return 0;
}

// -----------------------------------------------------------------------------
//  5-distribution drivers for the bucket_info dump (two-figure view).
//  Each one just builds the sample vector then delegates to the helper above,
//  mirroring the 5 quantile-mode compare tests.
// -----------------------------------------------------------------------------
static s32 zstat_loghist_bucket_info_spike_test()
{
    struct pair_t { s64 value; int count; };
    pair_t cases[] = {
        {     100, 200 }, {  10000,  50 }, { 100000,  30 },
        { 200000,  10 }, { 500000,   5 }, { 800000,   4 }, {1000000,   1 },
    };
    std::vector<s64> raw;
    for (auto& c : cases)
        for (int i = 0; i < c.count; ++i) raw.push_back(c.value);
    return zstat_loghist_bucket_info_compare_on("discrete-spike(7 spikes,N=300)", raw);
}

static s32 zstat_loghist_bucket_info_lognormal_test()
{
    const int N = 5000;
    std::mt19937 rng(20260714u);
    std::lognormal_distribution<double> dist(6.0, 1.5);
    std::vector<s64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        double x = dist(rng);
        if (x < 1.0) x = 1.0;
        raw.push_back((s64)x);
    }
    return zstat_loghist_bucket_info_compare_on("log-normal(mu=6,sigma=1.5,N=5000)", raw);
}

static s32 zstat_loghist_bucket_info_pareto_test()
{
    const int N = 5000;
    const double xm    = 100.0;
    const double alpha = 1.2;
    std::mt19937 rng(20260715u);
    std::uniform_real_distribution<double> u01(1e-6, 1.0);
    std::vector<s64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        double u = u01(rng);
        double x = xm / std::pow(u, 1.0 / alpha);
        if (x < 1.0) x = 1.0;
        if (x > 1e12) x = 1e12;
        raw.push_back((s64)x);
    }
    return zstat_loghist_bucket_info_compare_on("pareto(xm=100,alpha=1.2,N=5000)", raw);
}

static s32 zstat_loghist_bucket_info_exponential_test()
{
    const int N = 5000;
    std::mt19937 rng(20260716u);
    std::exponential_distribution<double> dist(1.0 / 5000.0);
    std::vector<s64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        double x = dist(rng);
        if (x < 1.0) x = 1.0;
        raw.push_back((s64)x);
    }
    return zstat_loghist_bucket_info_compare_on("exponential(mean=5000,N=5000)", raw);
}

static s32 zstat_loghist_bucket_info_uniform_test()
{
    const int N = 100000;
    std::mt19937 rng(20260713u);
    std::uniform_int_distribution<int> dist(0, 100000);
    std::vector<s64> raw;
    raw.reserve(N);
    for (int i = 0; i < N; ++i) raw.push_back((s64)dist(rng));
    return zstat_loghist_bucket_info_compare_on("uniform(0..1e5,N=100000)", raw);
}

static s32 zstat_loghist_bench_test()
{
    zstat_loghist<> h;
    h.reset(0);
    zclock<> cost;
    volatile s64 salt = 0;
    const int N = 100 * 10000;
    cost.start();
    for (int i = 0; i < N; ++i)
    {
        h.add((s64)i);
        salt += (s64)h.valid_count();
    }
    cost.stop_and_save();
    LogInfo() << "zstat_loghist.add() avg cost: "
              << (f64)cost.cost_ns() / (f64)N << " ns/op";
    return 0;
}

static s32 zstat_loghist_bucket_info_bench_test()
{
    // Fill a histogram with a realistic-ish distribution so that many buckets are
    // non-empty; this makes bucket_info() do actual work on non-degenerate paths.
    zstat_loghist<> h;
    h.reset(0);
    std::mt19937 rng(20260717u);
    std::lognormal_distribution<double> dist(6.0, 1.5);
    const int N_FILL = 50000;
    for (int i = 0; i < N_FILL; ++i)
    {
        double x = dist(rng);
        if (x < 1.0) x = 1.0;
        h.add((s64)x);
    }

    const int B = h.bucket_count();
    volatile s64 salt = 0;

    // Benchmark bucket_info(idx) across all bucket indices, looped enough
    // times to average out timer noise (~1e7 total calls).
    const int LOOPS = 10000000 / B + 1;
    zclock<> cost;
    cost.start();
    for (int lp = 0; lp < LOOPS; ++lp)
    {
        for (int i = 0; i < B; ++i)
        {
            zstat_loghist_hit_result r = h.bucket_info(i);
            salt += r.bucket_low + r.log2_height;
        }
    }
    cost.stop_and_save();
    s64 total_calls = (s64)LOOPS * (s64)B;
    LOGFMTI("zstat_loghist.bucket_info() avg cost: %.2f ns/op   (calls=%lld, buckets=%d, loops=%d)",
            (f64)cost.cost_ns() / (f64)total_calls,
            (long long)total_calls, B, LOOPS);

    // Also benchmark quantile(p) for reference (it scans buckets internally,
    // so it's expected to be O(bucket_count) rather than O(1)).
    const int Q_STEPS = 33;
    const int Q_LOOPS = 10000000 / Q_STEPS + 1;
    zclock<> cost2;
    cost2.start();
    for (int lp = 0; lp < Q_LOOPS; ++lp)
    {
        for (int s = 0; s < Q_STEPS; ++s)
        {
            f64 p = (f64)s / (f64)(Q_STEPS - 1);
            zstat_loghist_hit_result r = h.quantile(p);
            salt += (s64)r.height + r.log2_height;
        }
    }
    cost2.stop_and_save();
    s64 total_q = (s64)Q_LOOPS * (s64)Q_STEPS;
    LOGFMTI("zstat_loghist.quantile()    avg cost: %.2f ns/op   (calls=%lld, steps=%d, loops=%d)",
            (f64)cost2.cost_ns() / (f64)total_q,
            (long long)total_q, Q_STEPS, Q_LOOPS);
    return 0;
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    ASSERT_TEST(zstat_loghist_boundary_test()         == 0);
    ASSERT_TEST(zstat_loghist_edge_test()             == 0);
    ASSERT_TEST(zstat_loghist_window_safety_test()    == 0);
    ASSERT_TEST(zstat_loghist_userdist_test()         == 0);
    ASSERT_TEST(zstat_loghist_userdist_shifted_test() == 0);
    ASSERT_TEST(zstat_loghist_centroid_compare_test()     == 0);
    ASSERT_TEST(zstat_loghist_centroid_lognormal_test()   == 0);
    ASSERT_TEST(zstat_loghist_centroid_pareto_test()      == 0);
    ASSERT_TEST(zstat_loghist_centroid_exponential_test() == 0);
    ASSERT_TEST(zstat_loghist_centroid_uniform_test()     == 0);
    ASSERT_TEST(zstat_loghist_centroid_cross_summary()    == 0);
    ASSERT_TEST(zstat_loghist_bucket_info_spike_test()       == 0);
    ASSERT_TEST(zstat_loghist_bucket_info_lognormal_test()   == 0);
    ASSERT_TEST(zstat_loghist_bucket_info_pareto_test()      == 0);
    ASSERT_TEST(zstat_loghist_bucket_info_exponential_test() == 0);
    ASSERT_TEST(zstat_loghist_bucket_info_uniform_test()     == 0);
    ASSERT_TEST(zstat_loghist_uniform_stress_test()   == 0);
    ASSERT_TEST(zstat_loghist_template_test()         == 0);
    ASSERT_TEST(zstat_loghist_bench_test()            == 0);
    ASSERT_TEST(zstat_loghist_bucket_info_bench_test()== 0);

    LogInfo() << "all test finish .";
    return 0;
}
