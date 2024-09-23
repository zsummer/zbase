
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

class DeriveBB : public DeriveB
{
public:
	DeriveBB()
	{

	}
	virtual ~DeriveBB()
	{

	}
	float a = 0.0f;
};

class DeriveAA : public DeriveA
{
public:
	DeriveAA()
	{

	}
	virtual ~DeriveAA()
	{

	}
	float a = 0.0f;
};


class DeriveAAA : public DeriveAA
{
public:
	DeriveAAA()
	{

	}
	virtual ~DeriveAAA()
	{

	}
	float a = 0.0f;
};

class DeriveAAAA : public DeriveAAA
{
public:
	DeriveAAAA()
	{

	}
	virtual ~DeriveAAAA()
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
	if (a == 3)
	{
		return new DeriveAAAA();
	}
	return new Base();
}



int main(int argc, char* argv[])
{
    ztest_init();

	Base* a = genObj(3);

	volatile int c = 10000;

	volatile int sum = 0;

	if (true)
	{
		PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, c * 2, "empty");
		for (int i = 0; i < c; i++)
		{
			sum++;
			sum--;
		}
	}


	if (true)
	{
		PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, c*2, "dynamic_cast");
		for (int i = 0; i < c; i++)
		{
			DeriveA* aa = dynamic_cast<DeriveA*>(a);
			if (aa)
			{
				sum++;
			}
			DeriveB* bb = dynamic_cast<DeriveB*>(a);
			if (bb)
			{
				sum--;
			}
		}
	}

	if (true)
	{
		PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, c * 2, "static_cast");
		for (int i = 0; i < c; i++)
		{
			DeriveA* aa = static_cast<DeriveA*>(a);
			if (aa)
			{
				sum++;
			}
			DeriveB* bb = static_cast<DeriveB*>(a);
			if (bb)
			{
				sum--;
			}
		}
	}

	if (true)
	{
		PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, c * 2, "dynamic_cast AABB ");
		for (int i = 0; i < c; i++)
		{
			DeriveAAAA* aa = dynamic_cast<DeriveAAAA*>(a);
			if (aa)
			{
				sum++;
			}
			DeriveBB* bb = dynamic_cast<DeriveBB*>(a);
			if (bb)
			{
				sum--;
			}
		}
	}







    return 0;
}


