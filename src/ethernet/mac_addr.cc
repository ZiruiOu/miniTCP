#include "mac_addr.h"

namespace minitcp {
namespace ethernet {
MacAddress::MacAddress() { memset(&mac_addr_, 0, sizeof(mac_addr_)); }

MacAddress::MacAddress(const char* mac_addr) {
    // FIXME : ether_aton is not a re-entrant function.
    auto _buffer = ether_aton(mac_addr);
    memcpy(&mac_addr_, _buffer, sizeof(mac_addr_));
}

MacAddress::MacAddress(const std::string& mac_addr) {
    // FIXME : ether_aton is not a re-entrant function.
    auto _buffer = ether_aton(mac_addr.c_str());
    memcpy(&mac_addr_, _buffer, sizeof(mac_addr_));
}

MacAddress::MacAddress(const struct ether_addr& other) {
    for (int i = 0; i < 6; i++) {
        mac_addr_.octet[i] = other.octet[i];
    }
}

int MacAddress::GetAddressByDevice(const char* device_name) {
    struct ifaddrs *ifa_begin, *ifaddr;
    int status = getifaddrs(&ifa_begin);

    MINITCP_ASSERT_EQ(status, 0) << "getifaddrs error " << std::endl;

    for (ifaddr = ifa_begin; ifaddr != NULL; ifaddr = ifaddr->ifa_next) {
        if ((ifaddr->ifa_addr->sa_family == AF_LINK) &&
            strcmp(ifaddr->ifa_name, device_name) == 0) {
            u_char* address = reinterpret_cast<u_char*>(
                LLADDR(reinterpret_cast<sockaddr_dl*>(ifaddr->ifa_addr)));

            for (int i = 0; i < 6; i++) {
                mac_addr_.octet[i] = address[i];
            }

            return 0;
        }
    }

    MINITCP_LOG(ERROR) << "get mac address by device error: " << device_name
                       << " not found." << std::endl;
    return -1;
}

bool MacAddress::operator==(const MacAddress& other) const {
    for (int i = 0; i < 6; i++) {
        if (mac_addr_.octet[i] != other.mac_addr_.octet[i]) {
            return false;
        }
    }
    return true;
}

}  // namespace ethernet
}  // namespace minitcp