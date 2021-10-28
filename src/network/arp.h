#ifndef MINITCP_SRC_NETWORK_ARP_H_
#define MINITCP_SRC_NETWORK_ARP_H_

#include "../ethernet/device.h"
#include "arp_impl.h"

namespace minitcp {
namespace network {
#ifdef __cplusplus
extern "C" {
#endif  // ! __cplusplus

void initArp();

int updateArpTable(mac_t *peer_mac, ip_t *peer_ip, mac_t *local_mac,
                   ip_t *local_ip);

std::optional<mac_t> queryArpTable(ip_t peer_ip);

int sendArp(ip_t peer_ip, std::uint16_t arp_type, bool broadcast);

int receiveArpCallback(const void *arp_packet, int length, int device_id);

void timerArpHandler();

#ifdef __cplusplus
}
#endif  // ! __cplusplus
}  // namespace network
}  // namespace minitcp
#endif  // ! MINITCP_SRC_NETWORK_ARP_H_