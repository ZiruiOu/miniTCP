#include <gtest/gtest.h>

#include "logging.h"

using namespace minitcp;

TEST(logging, trace) {
    int a = 1, b = 2;
    MINITCP_LOG(TRACE) << "assigning variable: a = 1 " << std::endl;
    MINITCP_LOG(TRACE) << "aasigning variable: b = 2 " << std::endl;
}

TEST(logging, info) {
    int a, b, c;
    MINITCP_LOG(INFO) << "assigining variable a and b " << std::endl;
    a = 1;
    b = 2;
    MINITCP_LOG(INFO) << "assigning variable finish " << std::endl;
    MINITCP_LOG(INFO) << "adding variable a and b " << std::endl;
    c = a + b;
    MINITCP_LOG(INFO) << "adding variable finish " << std::endl;
}

TEST(logging, debug) {
    int a, b, c;
    a = 1;
    MINITCP_LOG(DEBUG) << "a = " << a << std::endl;
    b = 2;
    MINITCP_LOG(DEBUG) << "b = " << b << std::endl;
    c = a + b;
    MINITCP_LOG(DEBUG) << "c = " << c << std::endl;
    EXPECT_EQ(c, 3);
    MINITCP_LOG(DEBUG) << "end of logging.debug " << std::endl;
}

TEST(logging, warning) {
    int *a, *b, *c;
    a = 0;
    if (!a) {
        MINITCP_LOG(WARNING) << "a is a null pointer " << std::endl;
    }
}

TEST(logging, error) {
    int *a, *b, *c;
    a = 0;
    if (!a) {
        MINITCP_LOG(ERROR) << "a is a null pointer " << std::endl;
    }
}

TEST(logging, assert_eq) {
    int a = 1, b = 2;
    int c = a + b;
    MINITCP_ASSERT_EQ(c, 3) << std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}