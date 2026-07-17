/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once
#ifndef  ZSTAT_P2_H
#define ZSTAT_P2_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#if !defined(ZBASE_USE_OUTSIDE_TYPE) && !defined(ZBASE_USE_AHEAD_TYPE) && !defined(ZBASE_USE_DEFAULT_TYPE)
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


// P-square (P²) streaming quantile estimator.
//
// Tracks a single target quantile with 5 markers and O(1) state / update,
// without storing samples. target_p selects the estimated quantile (e.g.
// 0.95 for p95); values outside (0,1) fall back to 0.95.
class zstat_p2
{
public:
    zstat_p2() { reset(0.95); }
    explicit zstat_p2(f64 target_p) { reset(target_p); }

    void reset(f64 target_p = 0.95);
    void add(s64 height);

    s64 count() const { return count_; }
    f64 target_p() const { return target_p_; }
    f64 quantile() const;

    f64 marker_height(int i)   const { return (i >= 0 && i < 5) ? marker_heights_[i]   : 0.0; }
    f64 marker_pos(int i)      const { return (i >= 0 && i < 5) ? marker_pos_[i]        : 0.0; }
    f64 marker_want_pos(int i) const { return (i >= 0 && i < 5) ? marker_want_pos_[i]   : 0.0; }

private:
    f64 target_p_;
    s64 count_;

    f64 marker_heights_[5];   // estimated height at each marker
    f64 marker_pos_[5];       // actual marker position (rank)
    f64 marker_want_pos_[5];  // desired marker position
    f64 marker_want_step_[5]; // per-sample increment of the desired position

    static f64 parabolic(f64 q_prev, f64 q, f64 q_next,
                         f64 n_prev, f64 n, f64 n_next, f64 dir);
    static f64 linear(f64 q, f64 q_side, f64 n, f64 n_side);
    void bootstrap_sort();
};

inline void zstat_p2::reset(f64 target_p)
{
    target_p_ = (target_p <= 0.0 || target_p >= 1.0) ? 0.95 : target_p;
    count_    = 0;
    for (int i = 0; i < 5; ++i)
    {
        marker_heights_[i]   = 0.0;
        marker_pos_[i]       = 0.0;
        marker_want_pos_[i]  = 0.0;
        marker_want_step_[i] = 0.0;
    }
}

inline f64 zstat_p2::parabolic(f64 q_prev, f64 q, f64 q_next,
                               f64 n_prev, f64 n, f64 n_next, f64 dir)
{
    f64 a = dir / (n_next - n_prev);
    f64 b = (n - n_prev + dir) * (q_next - q) / (n_next - n)
          + (n_next - n - dir) * (q - q_prev) / (n - n_prev);
    return q + a * b;
}

inline f64 zstat_p2::linear(f64 q, f64 q_side, f64 n, f64 n_side)
{
    return q + ((q_side - q) / (n_side - n));
}

inline void zstat_p2::bootstrap_sort()
{
    for (int i = 1; i < 5; ++i)
    {
        f64 v = marker_heights_[i];
        int j = i - 1;
        while (j >= 0 && marker_heights_[j] > v) { marker_heights_[j + 1] = marker_heights_[j]; --j; }
        marker_heights_[j + 1] = v;
    }
}

inline void zstat_p2::add(s64 height)
{
    f64 sample = (f64)height;
    count_++;

    if (count_ <= 5)
    {
        marker_heights_[count_ - 1] = sample;
        if (count_ == 5)
        {
            bootstrap_sort();
            for (int i = 0; i < 5; ++i) marker_pos_[i] = (f64)(i + 1);

            // want_step[i] = target cumulative prob q_i of marker i (also the per-sample
            // advance of its desired position). want_pos初值 = n' = 1 + q_i*(N-1), N = count_.
            marker_want_step_[0] = 0.0;
            marker_want_step_[1] = target_p_ / 2.0;
            marker_want_step_[2] = target_p_;
            marker_want_step_[3] = (1.0 + target_p_) / 2.0;
            marker_want_step_[4] = 1.0;
            for (int i = 0; i < 5; ++i)
                marker_want_pos_[i] = 1.0 + marker_want_step_[i] * (f64)(count_ - 1);
        }
        return;
    }

    int cell;
    if (sample < marker_heights_[0]) { marker_heights_[0] = sample; cell = 0; }
    else if (sample < marker_heights_[1]) cell = 0;
    else if (sample < marker_heights_[2]) cell = 1;
    else if (sample < marker_heights_[3]) cell = 2;
    else if (sample <= marker_heights_[4]) cell = 3;
    else { marker_heights_[4] = sample; cell = 3; }

    for (int i = cell + 1; i < 5; ++i) marker_pos_[i] += 1.0;
    for (int i = 0; i < 5; ++i) marker_want_pos_[i] += marker_want_step_[i];

    for (int i = 1; i <= 3; ++i)
    {
        f64 gap = marker_want_pos_[i] - marker_pos_[i];
        if ((gap >=  1.0 && marker_pos_[i + 1] - marker_pos_[i] >  1.0) ||
            (gap <= -1.0 && marker_pos_[i - 1] - marker_pos_[i] < -1.0))
        {
            f64 dir   = (gap >= 0.0) ? 1.0 : -1.0;
            f64 q_new = parabolic(marker_heights_[i - 1], marker_heights_[i], marker_heights_[i + 1],
                                  marker_pos_[i - 1], marker_pos_[i], marker_pos_[i + 1], dir);
            if (marker_heights_[i - 1] < q_new && q_new < marker_heights_[i + 1])
            {
                marker_heights_[i] = q_new;
            }
            else
            {
                marker_heights_[i] = linear(marker_heights_[i], marker_heights_[i + (int)dir],
                                            marker_pos_[i], marker_pos_[i + (int)dir]);
            }
            marker_pos_[i] += dir;
        }
    }
}

inline f64 zstat_p2::quantile() const
{
    if (count_ == 0) return 0.0;
    if (count_ < 5)
    {
        f64 tmp[5];
        int n = (int)count_;
        for (int i = 0; i < n; ++i) tmp[i] = marker_heights_[i];
        for (int i = 1; i < n; ++i)
        {
            f64 v = tmp[i]; int j = i - 1;
            while (j >= 0 && tmp[j] > v) { tmp[j + 1] = tmp[j]; --j; }
            tmp[j + 1] = v;
        }
        int idx = (int)((f64)(n - 1) * target_p_ + 0.5);
        if (idx < 0) idx = 0;
        if (idx > n - 1) idx = n - 1;
        return tmp[idx];
    }
    return marker_heights_[2];
}

#endif
