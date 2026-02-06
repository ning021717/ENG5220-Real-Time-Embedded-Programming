#include <gtest/gtest.h>
#include "SignDatabase.hpp"

// 测试用例：验证模板是否能被正确加载
TEST(SignDatabaseTest, LoadValidTemplate) {
    SignDatabase db;
    // 假设你有一个测试用的二进制文件 test_a.bin
    bool result = db.loadTemplate("ActionA", "test_data/test_a.bin");

    EXPECT_TRUE(result); // 期望加载成功
    // 验证加载后的维度
    EXPECT_EQ(db.getTemplate("ActionA").size(), 30 * 21 * 3);
}

// 测试用例：验证加载不存在的文件
TEST(SignDatabaseTest, LoadNonExistentFile) {
    SignDatabase db;
    bool result = db.loadTemplate("Error", "non_existent.bin");
    EXPECT_FALSE(result); // 期望加载失败
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}//
// Created by 金扫帚奖最佳演员 on 2026/2/6.
//