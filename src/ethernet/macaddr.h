#ifndef MINITCP_ETHERNET_MAC_ADDR_H_
#define MINITCP_ETHERNET_MAC_ADDR_H_

#include <ifaddrs.h>
#include <net/ethernet.h>
// #include <net/if_dl.h>
#include <linux/if_packet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/ether.h>
#include <string>
#include <cstring>

#include "../common/logging.h"

namespace minitcp {
namespace ethernet {

typedef struct ether_addr mac_t;

class MacAddress {
   public:
    MacAddress();
    // three ways to store a mac address
    // 1. by device name
    // 2. by char
    // 3. by struct ether_addr
    explicit MacAddress(const char*);
    explicit MacAddress(const std::string&);
    explicit MacAddress(const mac_t&);

    // get device mac address by device name
    int GetAddressByDevice(const char*);
    const mac_t* GetAddressPointer() const { return &mac_addr_; }

    bool operator==(const MacAddress& other) const;

   private:
    // store the mac address with std::string
    // then we can use ether_aton_r / ether_ntoa_r to convert it to the mac
    // address
    mac_t mac_addr_;
};
}  // namespace ethernet
}  // namespace minitcp

#endif  // ! MINITCP_ETHERNET_MAC_ADDR_H_