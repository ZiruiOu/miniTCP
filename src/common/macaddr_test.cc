#include "macaddr.h"

#include <gtest/gtest.h>

using namespace minitcp;

TEST(MacAddress, AssignWithString) {
    MacAddress address("11:22:33:44:55:66");
    MacAddress other("11:22:33:44:55:66");

    EXPECT_EQ(address, other);
}

TEST(MacAddress, AssignWithEtherAddr) {
    struct ether_addr my_address = {0xff, 0xff, 0xde, 0xad, 0xbe, 0xef};
    MacAddress address(my_address);
    MacAddress other("ff:ff:de:ad:be:ef");

    EXPECT_EQ(address, other);
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}