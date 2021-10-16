#include "macaddr.h"

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
        mac_addr_.ether_addr_octet[i] = other.ether_addr_octet[i];
    }
}

int MacAddress::GetAddressByDevice(const char* device_name) {
    struct ifaddrs *ifa_begin, *ifaddr;

    int status = getifaddrs(&ifa_begin);
    MINITCP_ASSERT_EQ(status, 0) << "getifaddrs error " << std::endl;

    int found = 0;
    for (ifaddr = ifa_begin; ifaddr != NULL; ifaddr = ifaddr->ifa_next) {
        int sa_family = ifaddr->ifa_addr->sa_family;
        // if (strcmp(ifaddr->ifa_name, device_name) == 0) {
        //     u_char* address = reinterpret_cast<u_char*>(
        //         LLADDR(reinterpret_cast<sockaddr_dl*>(ifaddr->ifa_addr)));

        //    for (int i = 0; i < 6; i++) {
        //        mac_addr_.octet[i] = address[i];
        //    }

        //    found = 1;
        //    break;
        //}

        if (sa_family == AF_PACKET &&
            strcmp(ifaddr->ifa_name, device_name) == 0) {
            struct sockaddr_ll* socket_addr =
                reinterpret_cast<sockaddr_ll*>(ifaddr->ifa_addr);
            struct ether_addr* ifa_mac_addr =
                reinterpret_cast<ether_addr*>(socket_addr->sll_addr);
            for (int i = 0; i < 6; i++) {
                mac_addr_.ether_addr_octet[i] =
                    ifa_mac_addr->ether_addr_octet[i];
            }
            found = 1;
            break;
        }
    }

    freeifaddrs(ifa_begin);

    MINITCP_ASSERT(found) << "get mac address by device error: " << device_name
                          << " not found." << std::endl;
    return (found == 1);
}

bool MacAddress::operator==(const MacAddress& other) const {
    for (int i = 0; i < 6; i++) {
        if (mac_addr_.ether_addr_octet[i] !=
            other.mac_addr_.ether_addr_octet[i]) {
            return false;
        }
    }
    return true;
}

}  // namespace ethernet
}  // namespace minitcp