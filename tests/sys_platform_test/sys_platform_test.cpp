
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zprof.h"
#include "zforeach.h"
#include "zlist.h"
#include "zarray.h"
#include "test_common.h"
#include "ztrace.h"
#include <future>
#include <thread>


class Base
{
public:
	Base()
	{

	}
	virtual ~Base()
	{

	}

private:

};

class DeriveA :public Base
{
public:
	DeriveA()
	{

	}
	virtual ~DeriveA()
	{

	}
	int a = 0;
};


class DeriveB :public Base
{
public:
	DeriveB()
	{

	}
	virtual ~DeriveB()
	{

	}
	float a = 0.0f;
};

Base* genObj(int a)
{
	if (a == 1)
	{
		return new DeriveA();
	}
	if (a == 2)
	{
		return new DeriveB();

	}
	return new Base();
}



int main(int argc, char* argv[])
{
    ztest_init();

	Base* a = genObj(1);

	volatile int c = 10000;

	volatile int sum = 0;


	if (true)
	{
		PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, c, "dynamic");
		for (int i = 0; i < c; i++)
		{
			DeriveB* aa = dynamic_cast<DeriveB*>(a);
			if (aa)
			{
				sum++;
			}
		}

	}






    return 0;
}


