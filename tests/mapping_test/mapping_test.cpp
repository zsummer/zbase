
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zprof.h"
#include "zfile_mapping.h"
#include "test_common.h"
#include "zfile.h"


int main(int argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");


    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;

    if (true)
    {
        const char* test_file_name = "./make.sh";
        zfile_mapping res;
        res.mapping_res(test_file_name);
        ASSERT_TEST(res.is_mapped());


        ASSERT_TEST(res.file_size() == zfile::file_size(test_file_name));
        ASSERT_TEST(res.file_size() > 0, res.file_size());

        zfile file;
        struct stat s;
        file.open(test_file_name, "rb", s);
        ASSERT_TEST(file.is_open());
        std::string content = file.read_content();
        ASSERT_TEST((long long)content.size() == res.file_size(), content.size());

        for (long long i = 0; i < res.file_size(); i++)
        {
            ASSERT_TEST_NOLOG(res.file_data()[i] == content[i]);
        }

        LogInfo() << " base test ./make.sh success";
    }


    if (argc > 1)
    {
        zfile_mapping res;
        s32 ret = res.mapping_res(argv[1]);
        if (ret != 0)
        {
            LogError() << "mapping [" << argv[1] << "] has error";
            return 2;
        }
        if (!res.is_mapped())
        {
            LogError() << "mapping [" << argv[1] << "] has error";
            return 3;
        }
        if (res.file_size() == 0 || res.file_data() == NULL)
        {
            LogError() << "mapping [" << argv[1] << "] has error";
            return 4;
        }
        LogInfo() << "test:" << argv[1] << "  mapping file size : " << res.file_size();

        volatile int a = 0;
        u64 read_bytes = 0;
        for (long long i = 0; i < res.file_size(); i++)
        {
            a += (unsigned char)res.file_data()[i];
            read_bytes++;
            if (read_bytes % (500*1024*1024) == 0)
            {
                LogInfo() << "now read bytes:" << read_bytes / 1024.0 / 1024.0 << "M. all:" << res.file_size()/1024.0/1024.0 <<"M please putchar to continue...";
                //getchar();
            }
        }
        LogInfo() << "now read bytes:" << read_bytes / 1024.0 / 1024.0 << "M. please putchar to continue...";
        //getchar();
    }
    LogInfo() << "please putchar to continue...";
    //getchar();
    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


