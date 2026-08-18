
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zgraph.h"

s32 graph_add_and_find_nearst_ref_test()
{
    zgraph g;
    u32 v0 = g.add_vertex(zpoint(0, 0, 0), 0);
    u32 v1 = g.add_vertex(zpoint(100, 0, 0), 0);
    s32 link_id = g.add_link(v0, v1, 0);
    ASSERT_TEST(link_id >= 0);

    zgraph::ref out_ref;
    //查询点落在 link 中点附近 t 应该接近归一化 0.5 也就是定点 32767 左右
    ASSERT_TEST_EQ(g.find_nearst_ref(zpoint(50, 5, 0), 20.0f, out_ref), 0);
    ASSERT_TEST_EQ(out_ref.link_id, (u32)link_id);
    ASSERT_TEST(out_ref.flag > 30000 && out_ref.flag < 35000);

    return 0;
}

s32 graph_long_link_middle_query_test()
{
    zgraph g;
    u32 v0 = g.add_vertex(zpoint(0, 0, 0), 0);
    u32 v1 = g.add_vertex(zpoint(2000, 0, 0), 0);
    s32 link_id = g.add_link(v0, v1, 0);
    ASSERT_TEST(link_id >= 0);

    //查询点落在长 link 中段 远离两端顶点所在的格子 验证光栅化确实覆盖了中间格子 不是只有端点两个格子
    zgraph::ref out_ref;
    ASSERT_TEST_EQ(g.find_nearst_ref(zpoint(1000, 5, 0), 20.0f, out_ref), 0);
    ASSERT_TEST_EQ(out_ref.link_id, (u32)link_id);

    return 0;
}

s32 graph_remove_link_test()
{
    zgraph g;
    u32 v0 = g.add_vertex(zpoint(0, 0, 0), 0);
    u32 v1 = g.add_vertex(zpoint(100, 0, 0), 0);
    s32 link_id = g.add_link(v0, v1, 0);
    ASSERT_TEST(link_id >= 0);

    zgraph::ref out_ref;
    ASSERT_TEST_EQ(g.find_nearst_ref(zpoint(50, 5, 0), 20.0f, out_ref), 0);

    ASSERT_TEST_EQ(g.remove_link((u32)link_id), 0);
    //remove 之后同一块空间不应该再命中已删除的 link
    ASSERT_TEST_EQ(g.find_nearst_ref(zpoint(50, 5, 0), 20.0f, out_ref), -1);

    return 0;
}

s32 graph_reuse_link_id_test()
{
    zgraph g;
    u32 v0 = g.add_vertex(zpoint(0, 0, 0), 0);
    u32 v1 = g.add_vertex(zpoint(100, 0, 0), 0);
    s32 link_id = g.add_link(v0, v1, 0);
    ASSERT_TEST(link_id >= 0);
    ASSERT_TEST_EQ(g.remove_link((u32)link_id), 0);

    //link id 从 freelist 复用后 空间索引那一份归属必须是全新的 不能残留旧几何的痕迹
    u32 v2 = g.add_vertex(zpoint(500, 500, 0), 0);
    u32 v3 = g.add_vertex(zpoint(600, 500, 0), 0);
    s32 new_link_id = g.add_link(v2, v3, 0);
    ASSERT_TEST(new_link_id >= 0);

    zgraph::ref out_ref;
    ASSERT_TEST_EQ(g.find_nearst_ref(zpoint(50, 5, 0), 20.0f, out_ref), -1);
    ASSERT_TEST_EQ(g.find_nearst_ref(zpoint(550, 500, 0), 20.0f, out_ref), 0);
    ASSERT_TEST_EQ(out_ref.link_id, (u32)new_link_id);

    return 0;
}

s32 graph_add_link_bad_vertex_test()
{
    zgraph g;
    u32 v0 = g.add_vertex(zpoint(0, 0, 0), 0);
    ASSERT_TEST_EQ(g.add_link(v0, v0 + 999, 0), -1);
    return 0;
}

int main(int argc, char* argv[])
{
    ztest_init();

    LogDebug() << " main begin test. ";

    ASSERT_TEST(graph_add_and_find_nearst_ref_test() == 0);
    ASSERT_TEST(graph_long_link_middle_query_test() == 0);
    ASSERT_TEST(graph_remove_link_test() == 0);
    ASSERT_TEST(graph_reuse_link_id_test() == 0);
    ASSERT_TEST(graph_add_link_bad_vertex_test() == 0);

    LogInfo() << "all test finish .";
    return 0;
}
