#ifndef MINITCP_SRC_COMMON_TYPE_H_
#define MINITCP_SRC_COMMON_TYPE_H_

#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace minitcp {
using mac_t = struct ether_addr;
using ip_t = struct in_addr;
}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_TYPE_H_