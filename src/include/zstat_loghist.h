/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once
#ifndef  ZSTAT_LOGHIST_H
#define ZSTAT_LOGHIST_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <utility>

#if !defined(ZBASE_USE_AHEAD_TYPE) && !defined(ZBASE_USE_DEFAULT_TYPE)
#define ZBASE_USE_DEFAULT_TYPE
#endif

#ifdef ZBASE_USE_AHEAD_TYPE
using s8 = int8_t;
using u8 = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using s64 = int64_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;
#endif

#ifdef ZBASE_USE_DEFAULT_TYPE
using s8 = char;
using u8 = unsigned char;
using s16 = short int;
using u16 = unsigned short int;
using s32 = int;
using u32 = unsigned int;
using s64 = long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;
#endif

#ifdef _WIN32
#include <intrin.h>
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

enum zstat_loghist_centroid_mode
{
    kCentroidOff              = 0,   // (low, high)                — no centroid
    kCentroidLocalLinear      = 1,   // (low,    c_hit, high)      — 2-seg linear
    kCentroidNeighborLinear   = 2,   // (c_left, c_hit, c_right)   — 2-seg linear, rank-space
    kCentroidLocalLagrange    = 3,   // (low,    c_hit, high)      — Lagrange parabola
    kCentroidNeighborLagrange = 4,   // (c_left, c_hit, c_right)   — Lagrange parabola, rank-space
    kCentroidModeCount        = 5,
};

struct zstat_loghist_hit_result
{
    f64  height;             // estimated quantile value
    int  bucket_idx;         // index of the bucket hit by p
    s64  bucket_low;         // low  edge of the hit bucket
    s64  bucket_high;        // high edge of the hit bucket
    f64  centroid;           // centroid of the hit bucket
    s64  bucket_valid_count; // sample count in the hit bucket
    int  log2_height;        // bar length, log2-normalized to [0,60]
};

// Template parameters:
//
//   SubBits    - Sub-bucket resolution (bits per octave).
//                Each power-of-two octave is subdivided into 2^SubBits linear
//                buckets, controlling the relative error:
//                  1 -> 2 buckets/octave (~50% relative error)
//                  2 -> 4 buckets/octave (~25%)
//                  3 -> 8 buckets/octave (~12.5%)
//                  6 -> 64 buckets/octave (~1.56%)
//                Total bucket count scales as (octaves) * (2^SubBits).
//
//   TotalBits  - Dynamic range of the shifted domain (in bits).
//                Values in [0, 2^TotalBits) are tracked; anything larger falls
//                into the overflow counter. E.g. range 0..1024 requires
//                TotalBits >= 10, range 0..1M requires TotalBits >= 20.
//
//   unit_shift - Number of low bits stripped from the input before bucketing.
//                Use it to skip octaves smaller than your measurement floor,
//                trading low-end precision for a smaller bucket table without
//                changing the bucket count. E.g. if the sampling granularity
//                is 32 ns, setting unit_shift = 5 discards the 5 lowest
//                octaves that would otherwise be wasted.

template <int SubBits = 2, int TotalBits = 32, int CentroidMode = kCentroidOff>
class zstat_loghist
{
public:
    static_assert(SubBits >= 1 && SubBits <= 6,
                  "SubBits must be in [1,6]  (1=coarse 50%, 3=default 6.25%, 6=fine 0.78%)");
    static_assert(TotalBits > SubBits && TotalBits <= 62,
                  "TotalBits must satisfy SubBits < TotalBits <= 62");
    static_assert(CentroidMode >= 0 && CentroidMode < kCentroidModeCount,
                  "CentroidMode must be a valid zstat_loghist_centroid_mode value");

    static constexpr int  kSubBits       = SubBits;
    static constexpr int  kTotalBits     = TotalBits;
    static constexpr int  kSubCount      = 1 << SubBits;
    static constexpr int  kSubMask       = kSubCount - 1;
    static constexpr int  kBucketCount   = (TotalBits - SubBits + 1) << SubBits;
    static constexpr int  kCentroidMode  = CentroidMode;

    zstat_loghist() { reset(0); }
    explicit zstat_loghist(int unit_shift) { reset(unit_shift); }

    void reset(int unit_shift = 0);
    void add(s64 height);

    s64 req_count() const { return req_count_; }
    s64 req_underflow_count() const { return req_underflow_count_; }
    s64 req_overflow_count() const { return req_overflow_count_; }
    f64 req_underflow_sum() const { return req_underflow_sum_; }
    f64 req_overflow_sum() const { return req_overflow_sum_; }
    s64 valid_count() const { return valid_count_; }
    s64 valid_low() const { return (valid_count_ == 0) ? 0 : valid_low_; }
    s64 valid_high() const { return (valid_count_ == 0) ? 0 : valid_high_; }
    f64 valid_sum() const { return valid_sum_; }
    f64 valid_avg() const { return (valid_count_ == 0) ? 0.0 : valid_sum_ / (f64)valid_count_; }
    int valid_low_bucket_idx() const { return valid_low_bucket_idx_; }
    int valid_high_bucket_idx() const { return valid_high_bucket_idx_; }
    s64 valid_bucket_low() const { return valid_low_bucket_idx_ < 0 ? 0 : bucket_range(valid_low_bucket_idx_).first; }
    s64 valid_bucket_high() const { return valid_high_bucket_idx_ < 0 ? 0 : bucket_range(valid_high_bucket_idx_).second; }
    int unit_shift() const { return unit_shift_; }
    int bucket_count() const { return kBucketCount; }

    std::pair<s64, s64> bucket_range(int idx) const;
    s64 bucket_valid_count(int idx) const
    {
        if (idx < 0 || idx >= kBucketCount) return 0;
        return bucket_counts_[idx];
    }
    zstat_loghist_hit_result quantile(f64 p) const;
    zstat_loghist_hit_result bucket_info(int idx) const;

private:
    int         unit_shift_;

    s64         req_count_;
    s64         req_underflow_count_;
    s64         req_overflow_count_;
    s64         valid_count_;

    f64         req_underflow_sum_;
    f64         req_overflow_sum_;
    f64         valid_sum_;

    s64         valid_low_;
    s64         valid_high_;

    int         valid_low_bucket_idx_;
    int         valid_high_bucket_idx_;

    s64         bucket_counts_[kBucketCount];
    f32         bucket_sums_[CentroidMode != kCentroidOff ? kBucketCount : 1];

    static u32 first_bit_index_u64(u64 num);
    int compute_log2_height(f64 height) const;
    f64 bucket_centroid(int idx) const;
};

template <int SubBits, int TotalBits, int CentroidMode>
void zstat_loghist<SubBits, TotalBits, CentroidMode>::reset(int unit_shift)
{
    if (unit_shift < 0) unit_shift = 0;
    if (unit_shift > 63 - TotalBits) unit_shift = 63 - TotalBits;
    unit_shift_          = unit_shift;
    req_count_           = 0;
    req_underflow_count_ = 0;
    req_overflow_count_  = 0;
    valid_count_         = 0;
    req_underflow_sum_   = 0.0;
    req_overflow_sum_    = 0.0;
    valid_sum_           = 0.0;
    valid_low_           = LLONG_MAX;
    valid_high_          = LLONG_MIN;
    valid_low_bucket_idx_  = -1;
    valid_high_bucket_idx_ = -1;
    for (int i = 0; i < kBucketCount; ++i) bucket_counts_[i] = 0;
    if (CentroidMode != kCentroidOff)
    {
        for (int i = 0; i < kBucketCount; ++i) bucket_sums_[i] = 0.0f;
    }
}

template <int SubBits, int TotalBits, int CentroidMode>
void zstat_loghist<SubBits, TotalBits, CentroidMode>::add(s64 height)
{
    req_count_++;
    if (height < 0)
    {
        req_underflow_count_++;
        req_underflow_sum_ += (f64)height;
        return;
    }

    u64 shifted_height = (u64)height >> unit_shift_;
    int idx;
    if (shifted_height < (u64)kSubCount)
    {
        idx = (int)shifted_height;
    }
    else
    {
        u32 bit_idx = first_bit_index_u64(shifted_height);
        if ((int)bit_idx >= TotalBits)
        {
            req_overflow_count_++;
            req_overflow_sum_ += (f64)height;
            return;
        }
        u32 sub = (u32)((shifted_height >> (bit_idx - SubBits)) & (u32)kSubMask);
        idx = (int)(((bit_idx - (u32)SubBits + 1u) << SubBits) + sub);
    }

    if (height < valid_low_) valid_low_ = height;
    if (height > valid_high_) valid_high_ = height;
    if (bucket_counts_[idx] == 0)
    {
        if (valid_low_bucket_idx_ < 0 || idx < valid_low_bucket_idx_) valid_low_bucket_idx_ = idx;
        if (idx > valid_high_bucket_idx_) valid_high_bucket_idx_ = idx;
    }
    bucket_counts_[idx]++;
    valid_count_++;
    valid_sum_ += (f64)height;
    if (CentroidMode != kCentroidOff)
    {
        bucket_sums_[idx] += (f32)height;
    }
}

template <int SubBits, int TotalBits, int CentroidMode>
std::pair<s64, s64> zstat_loghist<SubBits, TotalBits, CentroidMode>::bucket_range(int idx) const
{
    if (idx < 0 || idx >= kBucketCount) return std::pair<s64, s64>(0, 0);
    u64 low, high;
    if (idx < kSubCount)
    {
        low  = (u64)idx;
        high = (u64)(idx + 1);
    }
    else
    {
        int octave_off = (idx - kSubCount) >> SubBits;
        int sub        = (idx - kSubCount) & kSubMask;
        u64 base = (u64)1 << (SubBits + octave_off);
        u64 step = (u64)1 << octave_off;
        low  = base + (u64)sub * step;
        high = low + step;
    }
    return std::pair<s64, s64>((s64)(low << unit_shift_), (s64)(high << unit_shift_));
}

template <int SubBits, int TotalBits, int CentroidMode>
zstat_loghist_hit_result zstat_loghist<SubBits, TotalBits, CentroidMode>::quantile(f64 p) const
{
    zstat_loghist_hit_result out = {};
    out.bucket_idx = -1;
    if (valid_count_ == 0) return out;
    if (p <= 0.0) p = 0.0;
    if (p >  1.0) p = 1.0;
    f64 target = p * (f64)valid_count_;
    if (target < 1.0) target = 1.0;
    s64 cum = 0;
    for (int i = 0; i < kBucketCount; ++i)
    {
        if (bucket_counts_[i] == 0) continue;
        s64 next_cum = cum + bucket_counts_[i];
        if ((f64)next_cum < target)
        {
            cum = next_cum;
            continue;
        }

        std::pair<s64, s64> r = bucket_range(i);
        f64 low  = (f64)r.first;
        f64 high = (f64)r.second;
        f64 frac = (target - (f64)cum) / (f64)bucket_counts_[i];

        out.bucket_idx   = i;
        out.bucket_low   = r.first;
        out.bucket_high  = r.second;
        out.bucket_valid_count = bucket_counts_[i];
        out.centroid     = bucket_centroid(i);

        if (CentroidMode == kCentroidOff)
        {
            out.height       = low + frac * (high - low);
            out.log2_height  = compute_log2_height(out.height);
            return out;
        }

        f64 c_hit = (f64)bucket_sums_[i] / (f64)bucket_counts_[i];

        if (CentroidMode == kCentroidLocalLinear)
        {
            if (frac <= 0.5)
            {
                out.height      = low   + (frac * 2.0)         * (c_hit - low);
                out.log2_height = compute_log2_height(out.height);
                return out;
            }
            out.height      = c_hit + ((frac - 0.5) * 2.0) * (high  - c_hit);
            out.log2_height = compute_log2_height(out.height);
            return out;
        }

        if (CentroidMode == kCentroidLocalLagrange)
        {
            // 3-point Lagrange on equidistant nodes (0, 0.5, 1) -> (low, c_hit, high)
            f64 t  = frac;
            f64 r0 = 0.0, r1 = 0.5, r2 = 1.0;
            f64 L0 = (t - r1) * (t - r2) / ((r0 - r1) * (r0 - r2));
            f64 L1 = (t - r0) * (t - r2) / ((r1 - r0) * (r1 - r2));
            f64 L2 = (t - r0) * (t - r1) / ((r2 - r0) * (r2 - r1));
            out.height      = L0 * low + L1 * c_hit + L2 * high;
            out.log2_height = compute_log2_height(out.height);
            return out;
        }

        // Neighbor modes: pull centroids of the previous / next non-empty
        // buckets; fall back to (low / high) when a neighbor is missing.
        f64 c_left  = low;
        f64 c_right = high;
        f64 n_left  = 0.0;
        f64 n_right = 0.0;
        if (i - 1 >= 0 && bucket_counts_[i - 1] > 0)
        {
            c_left = (f64)bucket_sums_[i - 1] / (f64)bucket_counts_[i - 1];
            n_left = (f64)bucket_counts_[i - 1];
        }
        if (i + 1 < kBucketCount && bucket_counts_[i + 1] > 0)
        {
            c_right = (f64)bucket_sums_[i + 1] / (f64)bucket_counts_[i + 1];
            n_right = (f64)bucket_counts_[i + 1];
        }

        // Rank-space positions of the three anchors:
        //   c_left  at cum      - 0.5 * n_left   (= cum      when no left  neighbor)
        //   c_hit   at cum      + 0.5 * n_hit
        //   c_right at next_cum + 0.5 * n_right  (= next_cum when no right neighbor)
        f64 n_hit     = (f64)bucket_counts_[i];
        f64 t         = (f64)target;
        f64 rank_left = (f64)cum      - 0.5 * n_left;
        f64 rank_hit  = (f64)cum      + 0.5 * n_hit;
        f64 rank_rt   = (f64)next_cum + 0.5 * n_right;

        if (CentroidMode == kCentroidNeighborLinear)
        {
            if (t <= rank_hit)
            {
                f64 u = (t - rank_left) / (rank_hit - rank_left);
                out.height      = c_left + u * (c_hit - c_left);
                out.log2_height = compute_log2_height(out.height);
                return out;
            }
            f64 u = (t - rank_hit) / (rank_rt - rank_hit);
            out.height      = c_hit + u * (c_right - c_hit);
            out.log2_height = compute_log2_height(out.height);
            return out;
        }

        if (CentroidMode == kCentroidNeighborLagrange)
        {
            // General 3-point Lagrange over non-equidistant rank nodes.
            f64 r0 = rank_left, r1 = rank_hit, r2 = rank_rt;
            f64 L0 = (t - r1) * (t - r2) / ((r0 - r1) * (r0 - r2));
            f64 L1 = (t - r0) * (t - r2) / ((r1 - r0) * (r1 - r2));
            f64 L2 = (t - r0) * (t - r1) / ((r2 - r0) * (r2 - r1));
            out.height      = L0 * c_left + L1 * c_hit + L2 * c_right;
            out.log2_height = compute_log2_height(out.height);
            return out;
        }
    }
    out.height      = (f64)valid_high_;
    out.log2_height = compute_log2_height(out.height);
    return out;
}

template <int SubBits, int TotalBits, int CentroidMode>
zstat_loghist_hit_result zstat_loghist<SubBits, TotalBits, CentroidMode>::bucket_info(int idx) const
{
    zstat_loghist_hit_result out = {};
    out.bucket_idx = -1;
    if (valid_count_ == 0) return out;
    if (idx < 0 || idx >= kBucketCount) return out;
    std::pair<s64, s64> r = bucket_range(idx);
    out.bucket_idx   = idx;
    out.bucket_low   = r.first;
    out.bucket_high  = r.second;
    out.bucket_valid_count = bucket_counts_[idx];
    out.centroid     = bucket_centroid(idx);

    if (CentroidMode != kCentroidOff && bucket_counts_[idx] > 0)
        out.height = out.centroid;
    else
        out.height = 0.5 * ((f64)r.first + (f64)r.second);
    out.log2_height = compute_log2_height(out.height);
    return out;
}

template <int SubBits, int TotalBits, int CentroidMode>
u32 zstat_loghist<SubBits, TotalBits, CentroidMode>::first_bit_index_u64(u64 num)
{
#ifdef _WIN32
    unsigned long index = 0;
    _BitScanReverse64(&index, num);
    return (u32)index;
#else
    return (u32)(63u - (u32)__builtin_clzll(num));
#endif
}

template <int SubBits, int TotalBits, int CentroidMode>
int zstat_loghist<SubBits, TotalBits, CentroidMode>::compute_log2_height(f64 height) const
{
    if (valid_count_ == 0 || valid_high_bucket_idx_ < 0) return 0;
    std::pair<s64, s64> vr = bucket_range(valid_high_bucket_idx_);
    f64 vmax = (f64)vr.second;
    if (vmax <= 0.0) return 0;
    f64 denom = log2(vmax + 1.0);
    if (denom <= 0.0) return 0;
    f64 num   = (height > 0.0) ? log2(height + 1.0) : 0.0;
    f64 r     = num / denom * 60.0;
    int n     = (int)(r + 0.5);
    if (n < 0)  n = 0;
    if (n > 60) n = 60;
    return n;
}

template <int SubBits, int TotalBits, int CentroidMode>
f64 zstat_loghist<SubBits, TotalBits, CentroidMode>::bucket_centroid(int idx) const
{
    if (idx < 0 || idx >= kBucketCount) return 0.0;
    if (CentroidMode != kCentroidOff && bucket_counts_[idx] > 0)
    {
        return (f64)bucket_sums_[idx] / (f64)bucket_counts_[idx];
    }
    std::pair<s64, s64> r = bucket_range(idx);
    return 0.5 * ((f64)r.first + (f64)r.second);
}

#endif
