/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once
#ifndef  ZPOINT_H
#define ZPOINT_H

#include <math.h>
#include <cmath>
#include <type_traits>

//default use format compatible short type .  
#if !defined(ZBASE_USE_AHEAD_TYPE) && !defined(ZBASE_USE_DEFAULT_TYPE)
#define ZBASE_USE_DEFAULT_TYPE
#endif 

//win & unix format incompatible   
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


#if __GNUG__
#define ZBASE_ALIAS __attribute__((__may_alias__))
#else
#define ZBASE_ALIAS
#endif



class zpoint
{
public:
    float x_, y_, z_;
public:
    static constexpr float kPI = (float)3.1415926535897932;
    static constexpr float kAngle180 = 180.0f;
    static constexpr float kPerAnglePI = kPI / kAngle180;
    static constexpr float kPerPIAngle = kAngle180 / kPI;
    static constexpr float kLowPrecision = 0.0002f;
    static constexpr float kFloatPrecision = 0.000001f;
public:
    zpoint(float fx, float fy, float fz):x_(fx),y_(fy),z_(fz){}
    zpoint(const zpoint & ) = default;
    zpoint() :zpoint(0.0f, 0.0f, 0.0f) {};
    zpoint & operator =(const zpoint & v3) = default;

    void reset() { x_ = 0.0f; y_ = 0.0f; z_ = 0.0f; }

    bool is_zero(float prcs = kLowPrecision) const { return fabs(x_) < prcs && fabs(y_) < prcs && fabs(z_) < prcs; }
    bool is_valid() const { return !std::isnan(x_) && !std::isnan(y_) && !std::isnan(z_) && !std::isinf(x_) && !std::isinf(y_) && !std::isinf(z_); }

    float dot(const zpoint& v) const { return x_ * v.x_ + y_ * v.y_ + z_ * v.z_; };
    float dot_2d(const zpoint& v) const { return x_ * v.x_ + y_ * v.y_; };

    zpoint det(const zpoint& v) const { return { y_ * v.z_ - z_ * v.y_ , z_ * v.x_ - x_ * v.z_ , x_ * v.y_ - y_ * v.x_ }; };
    zpoint det_2d(const zpoint& v) const { return { 0.0f , 0.0 , x_ * v.y_ - y_ * v.x_ }; };

    zpoint cross(const zpoint& v) const { return det(v); }
    zpoint cross_2d(const zpoint& v) const { return det_2d(v); }

    float square_length()const { return dot(*this); }
    float square_length_2d()const { return dot_2d(*this); }

    float square_distance()const { return square_length(); }
    float square_distance_2d()const { return square_length_2d(); }

    float length()const { return sqrtf(square_length()); }
    float length_2d()const { return sqrtf(square_length_2d()); }

    float distance()const { return length(); }
    float distance_2d()const { return length_2d(); }


    bool normalize();
    zpoint const_normalize() const { zpoint ret = *this; ret.normalize(); return ret; }

    bool normalize_2d(){ z_ = 0.0f; return normalize(); }
    zpoint const_normalize_2d() const { zpoint ret = *this; ret.normalize_2d(); return ret; }

    bool from_angle(float angle);
    float to_agnle() const;
    bool from_uv(float u, float v);


    static zpoint new_from_uv(float u, float v)
    {
        return zpoint(sin(kPI * v) * cos(kPI * 2 * u), sin(kPI * v) * sin(kPI  * 2 * u), cos(kPI * v));
    }
    static zpoint new_from_uv2(float u, float v)
    {
        return zpoint(cos(kPI * v) * cos(kPI * u), sin(kPI * v) * cos(kPI * u), sin(kPI * v));
    }

    zpoint operator + (const zpoint & v) const { return { x_ + v.x_, y_ + v.y_, z_ + v.z_ }; }
    zpoint & operator += (const zpoint & v) { x_ += v.x_, y_ += v.y_, z_ += v.z_; return *this; }
    zpoint operator - (const zpoint & v) const { return { x_ - v.x_, y_ - v.y_, z_ - v.z_ }; }
    zpoint & operator -= (const zpoint & v) { x_ -= v.x_, y_ -= v.y_, z_ -= v.z_; return *this; }
    zpoint operator * (const zpoint & v) const { return { x_ * v.x_, y_ * v.y_, z_ * v.z_ }; }
    zpoint & operator *= (const zpoint & v) { x_ *= v.x_, y_ *= v.y_, z_ *= v.z_; return *this; }
    zpoint operator / (const zpoint & v) const { return { x_ / v.x_, y_ / v.y_, z_ / v.z_ }; }
    zpoint & operator /= (const zpoint & v) { x_ /= v.x_, y_ /= v.y_, z_ /= v.z_; return *this; }

    zpoint operator + (float val) const { return { x_ + val, y_ + val, z_ + val }; }
    zpoint & operator += (float val) { x_ += val, y_ += val, z_ += val; return *this; }
    zpoint operator - (float val) const { return { x_ - val, y_ - val, z_ - val }; }
    zpoint & operator -= (float val) { x_ -= val, y_ -= val, z_ -= val; return *this; }
    zpoint operator * (float scalar) const { return { x_ * scalar, y_ * scalar, z_ * scalar }; }
    zpoint & operator *= (float scalar) { x_ *= scalar, y_ *= scalar, z_ *= scalar; return *this; }
    zpoint operator / (float scalar) const { return { x_ / scalar, y_ / scalar, z_ / scalar }; }
    zpoint & operator /= (float scalar) { x_ /= scalar, y_ /= scalar, z_ /= scalar; return *this; }


    //utils
public:
    static inline float INVERSE_SQRT(float val)
    {
        float xhalf = 0.5f * val;
        int i = *(int*)&val;
        i = 0x5f3759df - (i >> 1);
        val = *(float*)&i;
        val = val * (1.5f - xhalf * val * val);
        return val;
    }

};

inline bool zpoint::normalize()
{
    float len = length();
    if (fabs(len) < zpoint::kFloatPrecision)
    {
        return false;
    }
    *this /= len;
    return true;
}

inline bool zpoint::from_angle(float angle)
{
    float radian = angle * zpoint::kPerAnglePI;
    x_ = cos(radian);
    y_ = sin(radian);
    z_ = 0.0f;
    return true;
}

inline float zpoint::to_agnle() const
{
    float radian = dot({ 1.0f, 0.0f, 0.0f });
    if (y_ < 0.0f)
    {
        return (zpoint::kPI * 2 - radian) * zpoint::kPerPIAngle;
    }
    return radian * zpoint::kPerPIAngle;
}

inline bool zpoint::from_uv(float u, float v)
{
    x_ = sin(zpoint::kPI * v) * cos(zpoint::kPI * u);
    y_ = sin(zpoint::kPI * v) * sin(zpoint::kPI * u);
    z_ = cos(zpoint::kPI * v);
    return true;
}



#endif