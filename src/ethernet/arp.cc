#include "arp.h"

#include "arp_impl.h"

namespace minitcp {
namespace ethernet {

static std::size_t local_arp_timer_counter_ = 0;

void updateArpTable(ip_t *dest_ip, mac_t *dest_mac, int device_id) {
    class ArpTable &local_arp_ = ArpTable::GetInstance();
    local_arp_.Update(dest_ip, dest_mac, device_id);
}

std::optional<ArpTableEntry> queryArpTable(ip_t dest_ip) {
    class ArpTable &local_arp_ = ArpTable::GetInstance();
    return local_arp_.Query(dest_ip);
}

int receiveArpCallback(const void *packet, int length, int device_id) {
    // TODO : check
    struct ArpHeader *arp_packet = (struct ArpHeader *)packet;

    auto device = getDevicePointer(device_id);
    mac_t rx_mac = device->GetMacAddress();
    ip_t rx_ip = device->GetIpAddress();

    updateArpTable(&(arp_packet->src_ip), &(arp_packet->src_mac), device_id);

    int status = 0;
    if (arp_packet->operation == kArpTypeRequest) {
        status = device->SendArp(kArpTypeReply, &(arp_packet->src_mac),
                                 &(arp_packet->src_ip));
    }
    return status;
}

void timerArpHandler() {
    if (local_arp_timer_counter_ % 2 == 0) {
        broadcastArp();
    }
    class ArpTable &local_arp = ArpTable::GetInstance();
    local_arp.Aging();
    local_arp_timer_counter_++;
    if (local_arp_timer_counter_ >= 4) {
        local_arp_timer_counter_ = 0;
    }
}

}  // namespace ethernet
}  // namespace minitcp