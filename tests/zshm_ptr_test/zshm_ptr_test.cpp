
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/




#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "ztest.h"
#include "zshm_ptr.h"
#include <memory>

class pod
{
public:
    int value_ =1;
    int value() { return value_; }
    int id() { return 1; }
};

class small_vtable
{
public:
    virtual ~small_vtable(){}
    int value_ = 2;
    virtual int value() { return value_; }
    virtual int id() { return 2; }

private:
};

class big_vtable
{
public:
    virtual ~big_vtable() {}
    int value_ = 3;
    virtual int value() { return value_; }
    virtual int id() { return 3; }
    char b[1024 * 1024 * 10];
private:
};

s32 shm_ptr_base_test()
{
    static_assert(!std::is_polymorphic<pod>::value, "");
    static_assert(std::is_polymorphic<small_vtable>::value, "");
    static_assert(std::is_polymorphic<big_vtable>::value, "");

    pod pod_val;
    small_vtable small_val;
    big_vtable* big_val = new big_vtable;

    zshm_ptr<pod> pod_val_ptr(&pod_val);
    pod_val_ptr->value_;
    pod_val_ptr->id();
    (*pod_val_ptr).value_;

    zshm_ptr<small_vtable> small_val_ptr(&small_val);
    small_val_ptr->value_;
    small_val_ptr->id();
    (*small_val_ptr).value_;

    zshm_ptr<big_vtable> big_val_ptr(big_val);
    big_val_ptr->value_;
    big_val_ptr->id();
    (*big_val_ptr).value_;


    delete big_val_ptr.get();
    return 0;
}

s32 shm_ptr_fixed_test()
{
    big_vtable* big_val = new big_vtable;


    if (true)
    {
        ASSERT_TEST(big_val->id() == 3);
        ASSERT_TEST(big_val->value() == 3);

        LogDebug() << "vtable:" << (void*)*(u64*)big_val;
        zshm_ptr<small_vtable> vptr(big_val);
        //int id = vptr->id();
        //LogDebug() << "id:" << id << "vtable:" << (void*)*(u64*)big_val;
        
        //-O2will return old func before fixed  
        ASSERT_TEST(vptr->id() == 2);
        ASSERT_TEST(vptr->value() == 3);

        ASSERT_TEST(big_val->id() == 2);
        ASSERT_TEST(big_val->value() == 3);


        zshm_ptr<big_vtable>(big_val).fix_vptr();
        //id = big_val->id();
        //LogDebug() << "id:" << id << "vtable:" << (void*)*(u64*)big_val;

        //-O2will return old func before fixed  
        ASSERT_TEST(big_val->id() == 3, big_val->id());
        ASSERT_TEST(big_val->value() == 3, big_val->value());

    }
    delete big_val;
    return 0;
}

s32 shm_ptr_stress_test()
{

    big_vtable* big_val = new big_vtable;
    constexpr u32 count = 10000 * 10000;
    volatile int value = 0;
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, count, "volatile ->");
        for (u32 i = 0; i < count; i++)
        {
            value = big_val->value();
        }
    }

    if (true)
    {
        big_vtable* big_val = new big_vtable;
        std::unique_ptr<big_vtable> uptr(big_val);
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, count, "volatile unique_ptr->");
        for (u32 i = 0; i < count; i++)
        {
            value = uptr->value();
        }
    }


    if (true)
    {
        big_vtable* big_val = new big_vtable;
        std::shared_ptr<big_vtable> uptr(big_val);
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, count, "volatile shared_ptr->");
        for (u32 i = 0; i < count; i++)
        {
            value = uptr->value();
        }
    }


    if (true)
    {
        zshm_ptr<big_vtable> shm_ptr(big_val);
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, count, "volatile zshm_ptr->");
        for (u32 i = 0; i < count; i++)
        {
            value = shm_ptr->value();
        }
    }

    (void)value;
    delete big_val;



    return 0;
}



int main(int argc, char *argv[])
{
    ztest_init();

    ASSERT_TEST(shm_ptr_base_test() == 0);
    ASSERT_TEST(shm_ptr_fixed_test() == 0);
    ASSERT_TEST(shm_ptr_stress_test() == 0);

    

    LogInfo() << "all test finish .";
    return 0;
}


