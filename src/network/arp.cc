#include "arp.h"

#include "arp_impl.h"

namespace minitcp {
namespace network {

static std::size_t local_arp_timer_counter_ = 0;
static class ArpTable local_arp_;

void initArp() { local_arp_.InitArpTable(); }

int updateArpTable(mac_t *peer_mac, ip_t *peer_ip, mac_t *local_mac,
                   ip_t *local_ip) {
    return local_arp_.UpdateArpTable(peer_mac, peer_ip, local_mac, local_ip);
}

std::optional<mac_t> queryArpTable(ip_t peer_ip) {
    return local_arp_.QueryArpTable(peer_ip);
}

int sendArp(ip_t peer_ip, std::uint16_t arp_type, bool broadcast) {
    return local_arp_.SendArp(peer_ip, arp_type, broadcast);
}

int receiveArpCallback(const void *arp_packet, int length, int device_id) {
    // TODO
    std::uint16_t hw_type = ntohs(*(std::uint16_t *)arp_packet);
    std::uint16_t proto_type = ntohs(*(std::uint16_t *)(arp_packet + 2));
    std::uint16_t arp_type = ntohs(*(std::uint16_t *)(arp_packet + 6));

    mac_t *tx_mac = (mac_t *)(arp_packet + 8);
    ip_t *tx_ip = (ip_t *)(arp_packet + 14);

    // mac_t *rx_mac = (mac_t *)(arp_packet + 18);
    // ip_t *rx_ip = (ip_t *)(arp_packet + 24);

    auto device_block = ethernet::getDevicePointer(device_id);
    mac_t rx_mac = device_block->GetMacAddress();
    ip_t rx_ip = device_block->GetIpAddress();

    updateArpTable(tx_mac, tx_ip, &rx_mac, &rx_ip);

    int status = 0;
    if (arp_type == kArpTypeRequest) {
        status = sendArp(*tx_ip, kArpTypeReply, 0);
    }
    return status;
}

void timerArpHandler() {
    if (local_arp_timer_counter_ % 2 == 0) {
        sendArp(ip_t{0xffffffff}, kArpTypeRequest, 1);
    }
    local_arp_.AgingArpTable();
    local_arp_timer_counter_++;
    if (local_arp_timer_counter_ >= 4) {
        local_arp_timer_counter_ = 0;
    }
}

}  // namespace network
}  // namespace minitcp