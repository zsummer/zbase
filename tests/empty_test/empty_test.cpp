
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zprof.h"
#include "zarray.h"
#include "test_common.h"
#include "zpoint.h"

struct MyStruct
{
public:
    const static u32 MODULE_TYPE = 5;
};

#define D(m)   z.push_back(decltype(m)::MODULE_TYPE);




static char md5_num2hex[] = {
    '0','1','2','3','4','5','6','7','8','9',
    'a','b','c','d','e','f'
};

static char md5_num2HEX[] = {
    '0','1','2','3','4','5','6','7','8','9',
    'A','B','C','D','E','F'
};

static std::string _md5_result2string(unsigned char(&result)[16],
    char(&num2hex)[16])
{
    std::string str;
    str.reserve(64);
    for (int i = 0; i < 16; ++i)
    {
        char c = result[i];
        str.push_back(num2hex[(c & 0xf0) >> 4]);
        str.push_back(num2hex[c & 0x0f]);
    }
    //return std::move(str);
    return str;
}

void md5_to_str(unsigned char md[], char str[])
{
    int i = 0, offset = 0;
    for (; i < 16; ++i)
    {
        offset += sprintf(str + offset, "%02x", md[i]);
    }
}

int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");


    u64 salt = std::atoll("");
    unsigned char md[16] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

    char buf[33] = { 0 };
    md5_to_str(md, buf);

    std::string r = _md5_result2string(md, md5_num2hex);

    if (true)
    {
        zpoint a = { 1, 0, 0 };
        zpoint b = { 0, 1, 0 };

        zpoint right = a.cross(b);


    }

    LogInfo() << "all test finish .salt:" << salt;
    return 0;
}




