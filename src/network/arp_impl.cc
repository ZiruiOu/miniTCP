#include "arp_impl.h"

#include <pcap.h>

#include "../common/logging.h"
#include "../ethernet/device.h"
#include "../ethernet/packetio.h"

namespace minitcp {
namespace network {

void ArpTable::InitArpTable() {
    char err_buffer[PCAP_ERRBUF_SIZE];
    pcap_if_t *device, *devices;
    MINITCP_ASSERT_EQ(pcap_findalldevs(&devices, err_buffer), 0)
        << "InitARpTable error: " << err_buffer << std::endl;
    for (device = devices; device != NULL; device = device->next) {
        int id = ethernet::findDevice(device->name);
        if (id != -1) {
            MINITCP_LOG(INFO) << "InitArpTable: adding device " << device->name
                              << " into arp table." << std::endl;
            auto device_block = ethernet::getDevicePointer(id);
            arp_table_.push_back(ArpTableEntry{
                ip_t{0xffffffff},
                mac_t{0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
                device_block->GetIpAddress(),
                device_block->GetMacAddress(),
                id,
                -1,
            });
        }
    }
    MINITCP_LOG(INFO) << "InitArpTable: initialize arp table with "
                      << arp_table_.size() << " items." << std::endl;
    pcap_freealldevs(devices);
}

int ArpTable::UpdateArpTable(mac_t *peer_mac, ip_t *peer_ip, mac_t *local_mac,
                             ip_t *local_ip) {
    std::scoped_lock lock(arp_mutex_);
    for (auto &entry : arp_table_) {
        if (entry.local_ip.s_addr == (*local_ip).s_addr) {
            MINITCP_LOG(INFO)
                << "UpdateArpTable: an entry updated." << std::endl;
            entry.peer_ip = *peer_ip;
            entry.peer_mac = *peer_mac;
            entry.aging = kMaxArpAging;
            return 0;
        }
    }
    // TODO : add a new arp entry.
    MINITCP_LOG(ERROR) << "UpdateArpTable: arp entry not found." << std::endl;
    return -1;
}

std::optional<mac_t> ArpTable::QueryArpTable(ip_t peer_ip) {
    std::scoped_lock lock(arp_mutex_);
    for (auto &entry : arp_table_) {
        if (entry.peer_ip.s_addr == peer_ip.s_addr) {
            return entry.peer_mac;
        }
    }
    MINITCP_LOG(ERROR) << "QueryArpTable: arp entry not found." << std::endl;
    return {};
}

int ArpTable::SendArp(ip_t peer_ip, std::uint16_t arp_type, bool broadcast) {
    broadcast = (peer_ip.s_addr == 0xffffffff) || broadcast;

    std::scoped_lock lock(arp_mutex_);

    int status = 0;

    for (auto &entry : arp_table_) {
        if (broadcast || entry.peer_ip.s_addr == peer_ip.s_addr) {
            // TODO : use a buffer factory to read/write the buffer
            auto buffer = new std::uint8_t[28]();
            *(std::uint16_t *)buffer = htons(1);
            *(std::uint16_t *)(buffer + 2) = htons(0x0800);
            *(std::uint8_t *)(buffer + 4) = 6;
            *(std::uint8_t *)(buffer + 5) = 4;
            // TODO : add into constant.h
            *(std::uint16_t *)(buffer + 6) = htons(arp_type);
            *(mac_t *)(buffer + 8) = entry.local_mac;
            *(ip_t *)(buffer + 14) = entry.local_ip;
            if (arp_type == kArpTypeRequest) {
                *(mac_t *)(buffer + 18) =
                    mac_t{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
            } else {
                *(mac_t *)(buffer + 18) = entry.peer_mac;
            }
            *(ip_t *)(buffer + 24) = entry.peer_ip;
            if (status == 0) {
                status = ethernet::sendFrame(buffer, 28, kEtherArpType,
                                             &entry.peer_mac, entry.local_id);
            }
            delete[] buffer;
        }
    }

    return status;
}

int ArpTable::AgingArpTable() {
    std::scoped_lock lock(arp_mutex_);
    auto iter = arp_table_.begin();

    int modified = 0;
    while (iter != arp_table_.end()) {
        if ((*iter).aging > 0) {
            (*iter).aging--;
            iter++;
        } else if ((*iter).aging == 0) {
            modified = 1;
            MINITCP_LOG(INFO)
                << "AgingArpTable: arp entry with ip address "
                << inet_ntoa((*iter).peer_ip)
                << " is removing from the arp table." << std::endl;
            arp_table_.erase(iter++);
        } else {
            iter++;
        }
    }

    return modified;
}

}  // namespace network
}  // namespace minitcp