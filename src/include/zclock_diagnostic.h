
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/




#pragma once 
#ifndef ZCLOCK_DIAGNOSTIC_H
#define ZCLOCK_DIAGNOSTIC_H

#include "zclock.h"



template<class Desc, zclock_impl::clock_type _C = zclock_impl::T_CLOCK_VOLATILE_RDTSC>
class zclock_diagnostic_ns
{
public:
public:
    //watchdog  ms  
    explicit zclock_diagnostic_ns(const Desc& desc, long long watchdog, std::function<void(const Desc&, long long)> dog) :desc_(desc)
    {
        watchdog_ = watchdog;
        dog_ = dog;
        clock_.start();
    }
    ~zclock_diagnostic_ns()
    {
        if (dog_ == nullptr)
        {
            return;
        }
        long long ns = clock_.save().cost_ns();
        if (ns >= watchdog_)
        {
            dog_(desc_, ns);
        }
    }
private:
    const Desc desc_;
    zclock<_C> clock_;
    long long watchdog_;
    std::function<void(const Desc&, long long)> dog_;
};


template<class Desc, zclock_impl::clock_type _C = zclock_impl::T_CLOCK_VOLATILE_RDTSC>
class zclock_diagnostic_ms 
{
public:
public:
    //watchdog  ms  
    explicit zclock_diagnostic_ms(const Desc& desc, long long watchdog, std::function<void(const Desc&, long long)> dog) :desc_(desc)
    {
        watchdog_ = watchdog * 1000 * 1000;
        dog_ = dog;
        clock_.start();
    }
    ~zclock_diagnostic_ms()
    {
        if (dog_ == nullptr)
        {
            return;
        }
        long long ns = clock_.save().cost_ns();
        if (ns >= watchdog_)
        {
            dog_(desc_, ns/1000/1000);
        }
    }

    void reset_clock() 
    { 
        clock_.start(); 
    }

    void diagnostic_ns()
    {
        if (dog_ == nullptr)
        {
            return;
        }
        long long ns = clock_.save().cost_ns();
        dog_(desc_, ns);
    }

    void diagnostic_ms()
    {
        if (dog_ == nullptr)
        {
            return;
        }
        long long ms = clock_.save().cost_ms();
        dog_(desc_, ms);
    }

private:
    const Desc desc_;
    zclock<_C> clock_;
    long long watchdog_;
    std::function<void(const Desc&, long long)> dog_;
};





#endif