
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/




#pragma once 
#ifndef ZCLOCK_DIAGNOSTIC_H
#define ZCLOCK_DIAGNOSTIC_H

#include "zclock.h"



template<class Desc, zclock_impl::clock_type _C = zclock_impl::kClockVolatileRDTSC>
class zclock_diagnostic
{
public:
public:
    //watchdog  second
    explicit zclock_diagnostic(const Desc& desc, double watchdog, std::function<void(const Desc&, double)> dog) : desc_(desc)
    {
        watchdog_ = watchdog;
        dog_ = dog;
        clock_.start();
        last_diagnostic_ = 0.0;
    }
    ~zclock_diagnostic()
    {
        if (dog_ == nullptr)
        {
            return;
        }
        double s = clock_.save().cost_ns() * 1.0 / 1000 / 1000 / 1000;
        if (s > watchdog_)
        {
            dog_(desc_, s);
        }
    }

    void reset_clock()
    {
        clock_.start();
        last_diagnostic_ = 0;
    }

    void diagnostic(const Desc& desc, bool use_delta = true)
    {
        if (dog_ == nullptr)
        {
            return;
        }
        double s = clock_.save().cost_ns() * 1.0 / 1000 / 1000 / 1000;
        if (use_delta)
        {
            dog_(desc, s - last_diagnostic_);
            last_diagnostic_ = s;
        }
        else
        {
            dog_(desc, s);
        }
    }

private:
    const Desc desc_;
    zclock<_C> clock_;
    double watchdog_;
    double last_diagnostic_;
    std::function<void(const Desc&, double)> dog_;
};





#endif