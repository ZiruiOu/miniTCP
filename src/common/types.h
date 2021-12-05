#ifndef MINITCP_SRC_COMMON_TYPE_H_
#define MINITCP_SRC_COMMON_TYPE_H_

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <chrono>
#include <cstdint>
#include <utility>

namespace minitcp {
using timestamp_t = std::chrono::time_point<std::chrono::high_resolution_clock>;
using mac_t = struct ether_addr;
using ip_t = struct in_addr;
using port_t = std::uint16_t;

using connection_key_t = std::pair<std::pair<std::uint32_t, std::uint16_t>,
                                   std::pair<std::uint32_t, std::uint16_t>>;
}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_TYPE_H_