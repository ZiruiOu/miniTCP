#include <gtest/gtest.h>

#include "macaddr.h"

using namespace minitcp::ethernet;

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

TEST(MacAddress, GetAddressByDevice) {
    MacAddress device1;
    device1.GetAddressByDevice("en0");
    MacAddress device1_other("8c:85:90:0f:2f:27");
    EXPECT_EQ(device1, device1_other);
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}