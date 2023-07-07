
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

s32 base_test()
{
    //base create/health/back test 
    if (true)
    {
        zmem_obj_pool<s32, NODE_COUNT> ds;
        zarray<s32*, NODE_COUNT> store;
        for (s32 loop = 0; loop < 2; loop++)
        {
            for (s32 i = 0; i < NODE_COUNT; i++)
            {
                s32* p = ds.create();
                ASSERT_TEST_NOLOG(p != NULL, "");
                *p = i;
                store.push_back(p);
            }
            for (s32 i = 0; i < NODE_COUNT; i++)
            {
                ASSERT_TEST_NOLOG(*store[i] == i, "");
            }
            ASSERT_TEST(ds.full(), "");
            ASSERT_TEST(ds.exploit() == NULL, "");
            for (auto p : store)
            {
                ASSERT_TEST_NOLOG(ds.health(p, true) == 0);
                ASSERT_TEST_NOLOG(ds.back(p) == 0);
            }
            store.clear();
            ASSERT_TEST(ds.empty(), "");
        }
    }

    //create(...)
    if (true)
    {
        zmem_obj_pool<s32, NODE_COUNT> ds;
        zarray<s32*, NODE_COUNT> store;
        for (s32 loop = 0; loop < 2; loop++)
        {
            for (s32 i = 0; i < NODE_COUNT; i++)
            {
                s32* p = ds.create(0);
                ASSERT_TEST_NOLOG(p != NULL, "");
                ASSERT_TEST_NOLOG(*p == 0, "");
                *p = i;
                store.push_back(p);
            }
            for (s32 i = 0; i < NODE_COUNT; i++)
            {
                ASSERT_TEST_NOLOG(*store[i] == i, "");
            }
            ASSERT_TEST(ds.full(), "");
            ASSERT_TEST(ds.exploit() == NULL, "");
            for (auto p : store)
            {
                ASSERT_TEST_NOLOG(ds.health(p, true) == 0);
                ASSERT_TEST_NOLOG(ds.back(p) == 0);
            }
            store.clear();
            ASSERT_TEST(ds.empty(), "");
        }
    }


    //create(...) 
    if (true)
    {
        Node::node_seq_id = 0;
        zmem_obj_pool<Node, NODE_COUNT> ds;
        zarray<Node*, NODE_COUNT> store;
        for (s32 loop = 0; loop < 2; loop++)
        {
            for (s32 i = 0; i < NODE_COUNT; i++)
            {
                ASSERT_TEST_NOLOG(Node::node_seq_id == (u64)loop * NODE_COUNT * SEQ_COUNT + i * SEQ_COUNT, "");
                Node* p = ds.create(SEQ_COUNT);
                ASSERT_TEST_NOLOG(p != NULL, "");
                for (u32 j = 0; j < SEQ_COUNT; j++)
                {
                    p->seq[j] = (u64)loop * NODE_COUNT * SEQ_COUNT + i * SEQ_COUNT;
                }
                ASSERT_TEST_NOLOG(Node::node_seq_id == (u64)loop * NODE_COUNT * SEQ_COUNT + i * SEQ_COUNT + SEQ_COUNT, "");
                store.push_back(p);
            }
            for (s32 i = 0; i < NODE_COUNT; i++)
            {
                for (u32 j = 0; j < SEQ_COUNT; j++)
                {
                    ASSERT_TEST_NOLOG(store[i]->seq[j] == (u64)loop * NODE_COUNT * SEQ_COUNT + i * SEQ_COUNT, "");
                }
            }
            ASSERT_TEST(ds.full(), "");
            ASSERT_TEST(ds.exploit() == NULL, "");
            for (auto p : store)
            {
                ASSERT_TEST_NOLOG(ds.health(p, true) == 0);
                ASSERT_TEST_NOLOG(ds.back(p) == 0);
            }
            store.clear();
            ASSERT_TEST(ds.empty(), "");
        }
    }


    return 0;
}

s32 stress_test()
{
    zmem_obj_pool<s32, NODE_COUNT> ds;
    zarray<s32, NODE_COUNT> store;
    volatile s32 salt = 0;
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000 * 10000, "obj pool");
        for (s32 i = 0; i < 1000 * 10000; i++)
        {
            void* addr = ds.exploit();
            salt += (u32)(u64)addr;
            ds.back(addr);
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000 * 10000, "array");
        for (s32 i = 0; i < 1000 * 10000; i++)
        {
            store.push_back(1);
            salt += store.back();
            store.pop_back();
        }
    }
    return 0;
}




class NodePod
{
public:
    s32 pod_ = 0;
};

class ChildNode : public NodePod
{
public:
    ChildNode()
    {
        pod_ = 1;
    }
    virtual s32 ouput()
    {
        return pod_;
    }
};

class LeafNode : public ChildNode
{
public:
    LeafNode()
    {
        pod_ = 2;
    }
    virtual s32 ouput()
    {
        return pod_;
    }
};




s32 resume_test()
{
    if (true)
    {
        zmem_obj_pool<LeafNode, NODE_COUNT> ds;
        zarray<LeafNode*, NODE_COUNT> store;
        for (s32 i = 0; i < NODE_COUNT/2; i++)
        {
            LeafNode* p = ds.create();
            ASSERT_TEST_NOLOG(p != NULL, "");
            ASSERT_TEST_NOLOG(p->ouput() == 2);
            store.push_back(p);
        }

        for (auto p : store)
        {
            *(u64*)p = zmem_pool::get_vptr<ChildNode>();
        }

        for (auto p : store)
        {
            ASSERT_TEST_NOLOG(*(u64*)p == zmem_pool::get_vptr<ChildNode>());
            ASSERT_TEST_NOLOG(p->ouput() == 2);
        }
        ds.resume();
        for (auto p : store)
        {
            ASSERT_TEST_NOLOG(*(u64*)p == zmem_pool::get_vptr<LeafNode>());
            ASSERT_TEST_NOLOG(p->ouput() == 2);
        }
    }

    if (true)
    {
        zmem_pool ds;
        char pd[zmem_pool::calculate_space_size(sizeof(s32), NODE_COUNT)];
        ds.init(sizeof(s32), 0, 0, NODE_COUNT, &pd[0], zmem_pool::calculate_space_size(sizeof(s32), NODE_COUNT));
        zarray<s32*, NODE_COUNT> store;

        for (s32 i = 0; i < NODE_COUNT; i++)
        {
            s32* p = ds.create<s32>(i);
            ASSERT_TEST_NOLOG(p != NULL, "");
            *p = i;
            store.push_back(p);
        }
        for (s32 i = 0; i < NODE_COUNT; i++)
        {
            ASSERT_TEST_NOLOG(*store[i] == i, "");
            ASSERT_TEST_NOLOG(*ds.cast<s32>(i) == i, " expect:", i , ", geted:", *ds.cast<s32>(i));
        }
        ASSERT_TEST(ds.full(), "");
        ASSERT_TEST(ds.exploit() == NULL, "");
        for (auto p : store)
        {
            ASSERT_TEST_NOLOG(ds.health(p, true) == 0);
            ASSERT_TEST_NOLOG(ds.back(p) == 0);
        }
        store.clear();
        ASSERT_TEST(ds.empty(), "");
 
    }

    return 0;
}








s32 main(s32 argc, char *argv[])
{
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");


    LogDebug() << " main begin test. ";


    ASSERT_TEST(base_test() == 0);
    ASSERT_TEST(resume_test() == 0);
    ASSERT_TEST(stress_test() == 0);



    LogInfo() << "all test finish .";
    return 0;
}


