#include "logging.h"

#include <gtest/gtest.h>

using namespace minitcp;

TEST(logging, trace) {
    int a = 1, b = 2;
    MINITCP_LOG(TRACE) << "assigning variable: a = 1" << std::endl;
    MINITCP_LOG(TRACE) << "aasigning variable: b = 2" << std::endl;
}

TEST(logging, assert_eq) {
    int a = 1, b = 2;
    int c = a + b;
    MINITCP_ASSERT_EQ(c, 3) << std::endl;
}