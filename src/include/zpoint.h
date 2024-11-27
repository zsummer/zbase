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





template<class _Flt = float>
struct zpoint
{
public:
    static_assert(std::is_same<_Flt, float>::value || std::is_same<_Flt, double>::value, "");
    static constexpr _Flt PI = (_Flt)3.1415926535897932;
    static constexpr _Flt ANGLE_180 = (_Flt)180.0;
    static constexpr _Flt PI_PER_ANGLE = PI / ANGLE_180;
    static constexpr _Flt ANGLE_PER_PI = ANGLE_180 / PI;
    static constexpr _Flt LOW_PRECISION = 0.0002f;
    static constexpr float F32_PRECISION = 0.000001f;
    static constexpr double F64_PRECISION = 0.0000000000000001;
public:
    _Flt x, y, z;
    zpoint(_Flt fx, _Flt fy, _Flt fz):x(fx),y(fy),z(fz){}
    zpoint(const zpoint & ) = default;
    zpoint() :zpoint(0.0f, 0.0f, 0.0f) {};
    zpoint & operator =(const zpoint & v3) = default;

    void reset() { x = 0.0f; y = 0.0f; z = 0.0f; }
    _Flt dot(const zpoint& v) const { return x * v.x + y * v.y + z * v.z; };
    _Flt dot_2d(const zpoint& v) const { return x * v.x + y * v.y; };
    zpoint det(const zpoint& v) const { return { y * v.z - z * v.y , z * v.x - x * v.z , x * v.y - y * v.x }; };
    zpoint det_2d(const zpoint& v) const { return { 0.0f , 0.0 , x * v.y - y * v.x }; };
    zpoint cross(const zpoint& v) const { return det(v); }
    zpoint cross_2d(const zpoint& v) const { return det_2d(v); }
    _Flt square_length()const { return dot(*this); }
    _Flt square_length_2d()const { return dot_2d(*this); }
    _Flt square_distance()const { return square_length(); }
    _Flt square_distance_2d()const { return square_length_2d(); }
    _Flt length()const { return sqrtf(square_length()); }
    _Flt length_2d()const { return sqrtf(square_length_2d()); }
    _Flt distance()const { return length(); }
    _Flt distance_2d()const { return length_2d(); }


    bool is_zero(_Flt prcs = LOW_PRECISION) const { return fabs(x) < prcs && fabs(y) < prcs && fabs(z) < prcs; }
    bool is_valid() const{return !std::isnan(x) && !std::isnan(y) && !std::isnan(z) && !std::isinf(x) && !std::isinf(y) && !std::isinf(z);}

    template<class Flt = _Flt>
    bool normalize(const typename std::enable_if<std::is_same<float, Flt>::value, Flt>::type = 0.0f)
    {
        _Flt len = length();
        if (fabs(len) < F32_PRECISION)
        {
            return false;
        }
        *this /= len;
        return true;
    }
    template<class Flt = _Flt>
    bool normalize(const typename std::enable_if<std::is_same<double, Flt>::value, Flt>::type = 0.0f)
    {
        _Flt len = length();
        if (fabs(len) < F64_PRECISION)
        {
            return false;
        }
        *this /= len;
        return true;
    }

    zpoint const_normalize() const { zpoint ret = *this; ret.normalize(); return ret; }

    bool normalize_2d(){z = 0.0f;return normalize(); }
    zpoint const_normalize_2d() const { zpoint ret = *this; ret.normalize_2d(); return ret; }



    bool from_angle(_Flt angle)
    {
        float radian = angle * PI_PER_ANGLE;
        x = cos(radian);
        y = sin(radian);
        z = 0.0f;
        return true;
    }

    _Flt to_agnle() const
    {
        _Flt radian = dot({ 1.0f, 0.0f, 0.0f });
        if(y < 0.0f)
        {
            return (PI * 2 - radian) * ANGLE_PER_PI;
        }
        return radian * ANGLE_PER_PI;
    }

    bool from_uv(_Flt u, _Flt v)
    {
        x = sin(PI * v) * cos(PI * u);
        y = sin(PI * v) * sin(PI * u);
        z = cos(PI * v);
        return true;
    }
    static zpoint<_Flt> new_from_uv(_Flt u, _Flt v)
    {
        return zpoint<_Flt>(sin(PI * v) * cos(PI * 2 * u), sin(PI * v) * sin(PI  * 2 * u), cos(PI * v));
    }
    static zpoint<_Flt> new_from_uv2(_Flt u, _Flt v)
    {
        return zpoint<_Flt>(cos(PI * v) * cos(PI * u), sin(PI * v) * cos(PI * u), sin(PI * v));
    }

    zpoint operator + (const zpoint & v) const { return { x + v.x, y + v.y, z + v.z }; }
    zpoint & operator += (const zpoint & v) { x += v.x, y += v.y, z += v.z; return *this; }
    zpoint operator - (const zpoint & v) const { return { x - v.x, y - v.y, z - v.z }; }
    zpoint & operator -= (const zpoint & v) { x -= v.x, y -= v.y, z -= v.z; return *this; }
    zpoint operator * (const zpoint & v) const { return { x * v.x, y * v.y, z * v.z }; }
    zpoint & operator *= (const zpoint & v) { x *= v.x, y *= v.y, z *= v.z; return *this; }
    zpoint operator / (const zpoint & v) const { return { x / v.x, y / v.y, z / v.z }; }
    zpoint & operator /= (const zpoint & v) { x /= v.x, y /= v.y, z /= v.z; return *this; }

    zpoint operator + (_Flt val) const { return { x + val, y + val, z + val }; }
    zpoint & operator += (_Flt val) { x += val, y += val, z += val; return *this; }
    zpoint operator - (_Flt val) const { return { x - val, y - val, z - val }; }
    zpoint & operator -= (_Flt val) { x -= val, y -= val, z -= val; return *this; }
    zpoint operator * (_Flt scalar) const { return { x * scalar, y * scalar, z * scalar }; }
    zpoint & operator *= (_Flt scalar) { x *= scalar, y *= scalar, z *= scalar; return *this; }
    zpoint operator / (_Flt scalar) const { return { x / scalar, y / scalar, z / scalar }; }
    zpoint & operator /= (_Flt scalar) { x /= scalar, y /= scalar, z /= scalar; return *this; }


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




#endif