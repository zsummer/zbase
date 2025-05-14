/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once
#ifndef  ZPOINT_CAPSULE_H
#define ZPOINT_CAPSULE_H

#include "zpoint.h"


class zpoint_capsule
{
public:
    zpoint center_;
    float half_hight_;
    float radius_;
public:

public:
    zpoint_capsule(zpoint center, float half_hight, float radius): center_(center), half_hight_(half_hight), radius_(radius) {}
    zpoint_capsule(const zpoint_capsule& ) = default;
    zpoint_capsule(): zpoint_capsule(zpoint(), 0.0f, 0.0f) {};
    zpoint_capsule& operator =(const zpoint_capsule& v3) = default;

    void reset() { center_.reset(); half_hight_ = 0.0f; radius_ = 0.0f; }


};


#endif