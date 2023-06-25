/*
* zsummer License
* Copyright (C) 2014-2021 YaweiZhang <yawei.zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/



#include <unordered_map>
#include <unordered_set>
#include "fn_log.h"
#include "zprof.h"
#include "zforeach.h"
#include "zlist.h"
#include "zarray.h"
#include "zpool.h"
#include "test_common.h"



struct Node
{
    Node()
    {
        node_seq_id++;
    }
    Node(u32 n)
    {
        node_seq_id+= n;
    }
    u64 seq[5] = { 0 };
    static u64 node_seq_id;
};
u64 Node::node_seq_id = 0;



int main(int argc, char *argv[])
{
    PROF_INIT("inner prof");
    PROF_SET_OUTPUT(&FNLogFunc);

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";

    if (true)
    {
        Node::node_seq_id = 0;
        zpool_obj_static<Node, 100> ds;
        zarray<Node*, 100> store;
        for (int loop = 0; loop < 2; loop++)
        {
            for (int i = 0; i < 100; i++)
            {
                ASSERT_TEST(Node::node_seq_id == (u64)loop*100 + i, "");
                Node* p = ds.create();
                ASSERT_TEST(p != NULL, "");
                for (u32 j = 0; j < 5; j++)
                {
                    p->seq[j] = (u64)loop * 100 + i;
                }
                ASSERT_TEST(Node::node_seq_id == (u64)loop * 100 + i + 1, "");
                store.push_back(p);
            }
            for (int i = 0; i < 100; i++)
            {
                for (u32 j = 0; j < 5; j++)
                {
                    ASSERT_TEST(store[i]->seq[j] == (u64)loop * 100 + i, "");
                }
            }
            ASSERT_TEST(ds.full(), "");
            ASSERT_TEST(ds.exploit() == NULL, "");
            for (auto p : store)
            {
                ds.back(p);
            }
            store.clear();
            ASSERT_TEST(ds.empty(), "");
        }
    }


    if (true)
    {
        Node::node_seq_id = 0;
        zpool_obj_static<Node, 100> ds;
        zarray<Node*, 100> store;
        for (int loop = 0; loop < 2; loop++)
        {
            for (int i = 0; i < 100; i++)
            {
                ASSERT_TEST(Node::node_seq_id == (u64)loop * 100 * 5 + i * 5, "");
                Node* p = ds.create(5);
                ASSERT_TEST(p != NULL, "");
                for (u32 j = 0; j < 5; j++)
                {
                    p->seq[j] = (u64)loop * 100 * 5 + i * 5;
                }
                ASSERT_TEST(Node::node_seq_id == (u64)loop * 100*5 + i*5 + 5, "");
                store.push_back(p);
            }
            for (int i = 0; i < 100; i++)
            {
                for (u32 j = 0; j < 5; j++)
                {
                    ASSERT_TEST(store[i]->seq[j] == (u64)loop * 100 * 5 + i * 5, "");
                }
            }
            ASSERT_TEST(ds.full(), "");
            ASSERT_TEST(ds.exploit() == NULL, "");
            for (auto p : store)
            {
                ds.back(p);
            }
            store.clear();
            ASSERT_TEST(ds.empty(), "");
        }
    }

    if (true)
    {
        zpool_obj_static<int, 100> ds;
        zarray<int*, 100> store;
        for (int loop = 0; loop < 2; loop++)
        {
            for (int i = 0; i < 100; i++)
            {
                int* p = ds.create();
                ASSERT_TEST(p != NULL, "");
                *p = i;
                store.push_back(p);
            }
            for (int i = 0; i < 100; i++)
            {
                ASSERT_TEST(*store[i] == i, "");
            }
            ASSERT_TEST(ds.full(), "");
            ASSERT_TEST(ds.exploit() == NULL, "");
            for (auto p : store)
            {
                ds.back(p);
            }
            store.clear();
            ASSERT_TEST(ds.empty(), "");
        }
    }



    if (true)
    {
        zpool_obj_static<int, 100> ds;
        zarray<int*, 100> store;
        for (int loop = 0; loop < 2; loop++)
        {
            for (int i = 0; i < 100; i++)
            {
                int* p = ds.create(0);
                ASSERT_TEST(p != NULL, "");
                ASSERT_TEST(*p == 0, "");
                *p = i;
                store.push_back(p);
            }
            for (int i = 0; i < 100; i++)
            {
                ASSERT_TEST(*store[i] == i, "");
            }
            ASSERT_TEST(ds.full(), "");
            ASSERT_TEST(ds.exploit() == NULL, "");
            for (auto p : store)
            {
                ds.back(p);
            }
            store.clear();
            ASSERT_TEST(ds.empty(), "");
        }
    }




    LogInfo() << "all test finish .";
    return 0;
}

