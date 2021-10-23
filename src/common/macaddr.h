#ifndef MINITCP_SRC_COMMON_MACADDR_H_
#define MINITCP_SRC_COMMON_MACADDR_H_

#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstring>
#include <string>

#include "logging.h"
#include "types.h"

namespace minitcp {

class MacAddress {
   public:
    MacAddress() { std::memset(&mac_addr_, 0, sizeof(mac_addr_)); }
    // two ways to store a mac address
    // 1. by char
    // 2. by struct ether_addr
    explicit MacAddress(const char* mac_addr) {
        auto buffer = ether_aton_r(mac_addr, &mac_addr_);
        MINITCP_ASSERT(buffer)
            << "MacAddress : ether_aton_r error." << std::endl;
    }
    explicit MacAddress(const std::string& mac_addr) {
        auto buffer = ether_aton_r(mac_addr.c_str(), &mac_addr_);
        MINITCP_ASSERT(buffer)
            << "MacAddress : ether_aton_r error." << std::endl;
    }
    explicit MacAddress(const mac_t& other) {
        for (int i = 0; i < 6; i++) {
            mac_addr_.ether_addr_octet[i] = other.ether_addr_octet[i];
        }
    }

    MacAddress(const MacAddress& other) {
        memcpy(&mac_addr_, &other.mac_addr_, sizeof(mac_addr_));
    }

    // get the raw pointer of the struct ether_addr;
    mac_t* GetPointer() { return &mac_addr_; }

    bool operator==(const MacAddress& other) const {
        for (int i = 0; i < 6; i++) {
            if (mac_addr_.ether_addr_octet[i] !=
                other.mac_addr_.ether_addr_octet[i]) {
                return false;
            }
        }
        return true;
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const MacAddress& address) {
        char buffer[30] = {};
        MINITCP_ASSERT(ether_ntoa_r(&address.mac_addr_, buffer))
            << " ether_ntoa_r error " << std::endl;
        os << buffer;
        return os;
    }

   private:
    // directly store the mac address
    mac_t mac_addr_;
};
}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_MACADDR_H_