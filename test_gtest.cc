#include <gtest/gtest.h>

#include <iostream>

TEST(binary_operation, apulsb) {
    int a = 1, b = 2;
    int c;
    c = a + b;
    ASSERT_EQ(c, 3);
}

TEST(binary_operation, aminusb) {
    int a = 2, b = 1;
    int c;
    c = a - b;
    ASSERT_EQ(c, 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}