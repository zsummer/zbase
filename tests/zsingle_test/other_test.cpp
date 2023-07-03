
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

#include "zsingle.h"
#include "test_common.h"
#include "fn_log.h"
#include "zprof.h"

void push_one(u32 v)
{
    zsingle<zarray<u32, 100>>::instance().push_back(v);
}

