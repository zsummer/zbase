
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
#include "zmem_pool.h"
#include "test_common.h"



constexpr static s32 NODE_COUNT = 50;
constexpr static s32 SEQ_COUNT = 5;

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
    u64 seq[SEQ_COUNT] = { 0 };
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
        zmem_obj_pool<Node, NODE_COUNT> ds;
        zarray<Node*, NODE_COUNT> store;
        for (int loop = 0; loop < 2; loop++)
        {
            for (int i = 0; i < NODE_COUNT; i++)
            {
                ASSERT_TEST(Node::node_seq_id == (u64)loop*NODE_COUNT + i, "");
                Node* p = ds.create();
                ASSERT_TEST(p != NULL, "");
                for (u32 j = 0; j < SEQ_COUNT; j++)
                {
                    p->seq[j] = (u64)loop * NODE_COUNT + i;
                }
                ASSERT_TEST(Node::node_seq_id == (u64)loop * NODE_COUNT + i + 1, "");
                store.push_back(p);
            }
            for (int i = 0; i < NODE_COUNT; i++)
            {
                for (u32 j = 0; j < SEQ_COUNT; j++)
                {
                    ASSERT_TEST(store[i]->seq[j] == (u64)loop * NODE_COUNT + i, "");
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
        zmem_obj_pool<Node, NODE_COUNT> ds;
        zarray<Node*, NODE_COUNT> store;
        for (int loop = 0; loop < 2; loop++)
        {
            for (int i = 0; i < NODE_COUNT; i++)
            {
                ASSERT_TEST(Node::node_seq_id == (u64)loop * NODE_COUNT * SEQ_COUNT + i * SEQ_COUNT, "");
                Node* p = ds.create(5);
                ASSERT_TEST(p != NULL, "");
                for (u32 j = 0; j < SEQ_COUNT; j++)
                {
                    p->seq[j] = (u64)loop * NODE_COUNT * SEQ_COUNT + i * SEQ_COUNT;
                }
                ASSERT_TEST(Node::node_seq_id == (u64)loop * NODE_COUNT* SEQ_COUNT + i* SEQ_COUNT + SEQ_COUNT, "");
                store.push_back(p);
            }
            for (int i = 0; i < NODE_COUNT; i++)
            {
                for (u32 j = 0; j < SEQ_COUNT; j++)
                {
                    ASSERT_TEST(store[i]->seq[j] == (u64)loop * NODE_COUNT * SEQ_COUNT + i * SEQ_COUNT, "");
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
        zmem_obj_pool<int, NODE_COUNT> ds;
        zarray<int*, NODE_COUNT> store;
        for (int loop = 0; loop < 2; loop++)
        {
            for (int i = 0; i < NODE_COUNT; i++)
            {
                int* p = ds.create();
                ASSERT_TEST(p != NULL, "");
                *p = i;
                store.push_back(p);
            }
            for (int i = 0; i < NODE_COUNT; i++)
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
        zmem_obj_pool<int, NODE_COUNT> ds;
        zarray<int*, NODE_COUNT> store;
        for (int loop = 0; loop < 2; loop++)
        {
            for (int i = 0; i < NODE_COUNT; i++)
            {
                int* p = ds.create(0);
                ASSERT_TEST(p != NULL, "");
                ASSERT_TEST(*p == 0, "");
                *p = i;
                store.push_back(p);
            }
            for (int i = 0; i < NODE_COUNT; i++)
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


