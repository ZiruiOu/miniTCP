//
// Created by Zirui Ou on 2021/10/5
//

#include <ifaddrs.h>
#include <net/ethernet.h>
#include <net/if_dl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

int main() {
    struct ifaddrs *ifa_begin;
    uint8_t *device_macaddr;

    // struct ether_addr mac_addr;

    // if (getifaddrs(&ifa_begin) == 0) {
    //  struct ifaddrs *ifa_iter;
    //  for (ifa_iter = ifa_begin; ifa_iter != NULL;
    //       ifa_iter = ifa_iter->ifa_next) {
    //    if ((ifa_iter->ifa_addr)->sa_family == AF_LINK &&
    //        strcmp("en1", ifa_iter->ifa_name) == 0) {
    //      device_macaddr = reinterpret_cast<uint8_t *>(
    //          LLADDR(reinterpret_cast<struct sockaddr_dl
    //          *>(ifa_iter->ifa_addr)));
    //
    //      memcpy(&mac_addr, device_macaddr, sizeof(mac_addr));

    //      printf("%s : %02x:%02x:%02x:%02x:%02x:%02x\n", ifa_iter->ifa_name,
    //             mac_addr.octet[0], mac_addr.octet[1], mac_addr.octet[2],
    //             mac_addr.octet[3], mac_addr.octet[4], mac_addr.octet[5]);
    //    }
    //  }
    //  freeifaddrs(ifa_begin);
    //}
    std::map<std::string, int> my_map;
    my_map["nice"] = 200;
    std::cout << my_map["nice"] << std::endl;
    return 0;
}