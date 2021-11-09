#ifndef MINITCP_SRC_NETWORK_ARP_H_
#define MINITCP_SRC_NETWORK_ARP_H_

#include "../common/timer.h"
#include "arp_impl.h"
#include "device.h"

namespace minitcp {
namespace ethernet {
#ifdef __cplusplus
extern "C" {
#endif  // ! __cplusplus

void updateArpTable(ip_t *dest_ip, mac_t *dest_mac, int device_id);

std::optional<ArpTableEntry> queryArpTable(ip_t dest_ip);

int receiveArpCallback(const void *arp_packet, int length, int device_id);

void timerArpHandler();

#ifdef __cplusplus
}
#endif  // ! __cplusplus
}  // namespace ethernet
}  // namespace minitcp
#endif  // ! MINITCP_SRC_NETWORK_ARP_H_