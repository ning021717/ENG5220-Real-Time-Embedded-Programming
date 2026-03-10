#include <gtest/gtest.h>
#include "SignDatabase.hpp"

// Test case: Verify that templates can be loaded correctly
TEST(SignDatabaseTest, LoadValidTemplate) {
    SignDatabase db;
    // Assume you have a test binary file: test_data/test_a.bin
    bool result = db.loadTemplate("ActionA", "test_data/test_a.bin");

    EXPECT_TRUE(result); // Expect the template to load successfully
    // Verify the dimension of the loaded template
    EXPECT_EQ(db.getTemplate("ActionA").size(), 30 * 21 * 3);
}

// Test case: Verify loading a non-existent file returns failure
TEST(SignDatabaseTest, LoadNonExistentFile) {
    SignDatabase db;
    bool result = db.loadTemplate("Error", "non_existent.bin");
    EXPECT_FALSE(result); // Expect the load operation to fail
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv); // Initialize Google Test framework
    return RUN_ALL_TESTS(); // Execute all registered test cases
}

//
