#include "arp_impl.h"

#include <pcap.h>

#include "../common/logging.h"
#include "device.h"
#include "packetio.h"

namespace minitcp {
namespace ethernet {

ArpTable *ArpTable::arp_singleton_ = nullptr;

void ArpTable::Update(ip_t *dest_ip, mac_t *dest_mac, int local_id) {
    std::scoped_lock lock(arp_mutex_);
    for (auto &entry : arp_table_) {
        if (entry.dest_ip.s_addr == (*dest_ip).s_addr) {
            // MINITCP_LOG(INFO)
            //     << "UpdateArpTable: an entry updated." << std::endl;
            if (entry.aging != -1) {
                entry.aging = kMaxArpAging;
            }
            return;
        }
    }
    arp_table_.push_back(ArpTableEntry{
        *dest_ip,
        *dest_mac,
        local_id,
        kMaxArpAging,
    });
}

int ArpTable::Persist(ip_t *dest_ip, mac_t *dest_mac, int local_id) {
    std::scoped_lock lock(arp_mutex_);
    for (auto &entry : arp_table_) {
        if (entry.dest_ip.s_addr == (*dest_ip).s_addr) {
            MINITCP_LOG(INFO)
                << "ArpTable.Persist: persist an entry." << std::endl;
            entry.aging = -1;
        }
    }
}

std::optional<ArpTableEntry> ArpTable::Query(ip_t dest_ip) {
    std::scoped_lock lock(arp_mutex_);
    for (auto &entry : arp_table_) {
        if (entry.dest_ip.s_addr == dest_ip.s_addr) {
            return entry;
        }
    }
    // MINITCP_LOG(ERROR) << "QueryArpTable: arp entry not found." << std::endl;
    return {};
}

std::optional<ArpTableEntry> ArpTable::QueryByDeviceId(int id) {
    std::scoped_lock lock(arp_mutex_);
    for (auto &entry : arp_table_) {
        if (entry.local_id == id) {
            return entry;
        }
    }
    // MINITCP_LOG(ERROR) << "QueryByDeviceId: arp entry not found." <<
    // std::endl;
    return {};
}

// int ArpTable::SendArp(ip_t dest_ip, std::uint16_t arp_type, bool broadcast) {
//     broadcast = (dest_ip.s_addr == 0xffffffff) || broadcast;
//
//     std::scoped_lock lock(arp_mutex_);
//
//     int status = 0;
//
//     for (auto &entry : arp_table_) {
//         if (broadcast || entry.dest_ip.s_addr == dest_ip.s_addr) {
//            auto buffer = new std::uint8_t[28]();
//            *(std::uint16_t *)buffer = htons(1);
//            *(std::uint16_t *)(buffer + 2) = htons(0x0800);
//            *(std::uint8_t *)(buffer + 4) = 6;
//            *(std::uint8_t *)(buffer + 5) = 4;
//            *(std::uint16_t *)(buffer + 6) = htons(arp_type);
//            *(mac_t *)(buffer + 8) = entry.local_mac;
//            *(ip_t *)(buffer + 14) = entry.local_ip;
//            if (arp_type == kArpTypeRequest) {
//                *(mac_t *)(buffer + 18) =
//                    mac_t{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
//            } else {
//                *(mac_t *)(buffer + 18) = entry.dest_mac;
//            }
//            *(ip_t *)(buffer + 24) = entry.dest_ip;
//            if (status == 0) {
//                status = ethernet::sendFrame(buffer, 28, kEtherArpType,
//                                             &(entry.dest_mac),
//                                             entry.local_id);
//            }
//            delete[] buffer;
//        }
//    }
//
//    return status;
//}

int ArpTable::Aging() {
    std::scoped_lock lock(arp_mutex_);
    auto iter = arp_table_.begin();

    int modified = 0;
    while (iter != arp_table_.end()) {
        if ((*iter).aging > 0) {
            (*iter).aging--;
            iter++;
        } else if ((*iter).aging == 0) {
            modified = 1;
            // MINITCP_LOG(INFO)
            //     << "AgingArpTable: arp entry with ip address "
            //     << inet_ntoa((*iter).dest_ip)
            //     << " is removing from the arp table." << std::endl;
            iter = arp_table_.erase(iter);
        } else {
            iter++;
        }
    }

    return modified;
}

}  // namespace ethernet
}  // namespace minitcp