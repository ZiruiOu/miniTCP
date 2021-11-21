#ifndef MINITCP_SRC_COMMON_TYPE_H_
#define MINITCP_SRC_COMMON_TYPE_H_

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdint>

namespace minitcp {
using mac_t = struct ether_addr;
using ip_t = struct in_addr;
using port_t = std::uint16_t;

using connection_key_t = std::pair<std::pair<std::uint32_t std::uint16_t>,
                                   std::pair<std::uint32_t, std::uint16_t>>;
}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_TYPE_H_