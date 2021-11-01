#include "routing_impl.h"

#include <pcap.h>

#include <iomanip>
#include <optional>

#include "../common/logging.h"
#include "../ethernet/device.h"

namespace minitcp {
namespace network {

RoutingTable* RoutingTable::routing_singleton_ = nullptr;

void RoutingTable::InitRoutingTable() {
    pcap_if_t *netif, *netif_all;
    char err_buffer[PCAP_ERRBUF_SIZE];

    MINITCP_ASSERT_EQ(pcap_findalldevs(&netif_all, err_buffer), 0)
        << "RoutingTable: initialize error " << err_buffer << std::endl;
    for (netif = netif_all; netif != NULL; netif = netif->next) {
        int device_id = ethernet::findDevice(netif->name);
        if (device_id != -1) {
            auto device = ethernet::getDevicePointer(device_id);
            routing_table_.push_back(RoutingEntry{
                kPersist,
                0,
                0,
                device->GetIpAddress(),
                device->GetNetmask(),
                device->GetIpAddress(),
                0,
            });
        }
    }

    pcap_freealldevs(netif_all);
}

void RoutingTable::Insert(ip_t dest, ip_t netmask, ip_t nexthop_ip,
                          int distance) {
    std::scoped_lock lock(routing_mutex_);
    this->unsafeInsert(dest, netmask, nexthop_ip, distance);
}

void RoutingTable::Remove(ip_t dest, ip_t netmask) {
    std::scoped_lock lock(routing_mutex_);
    for (auto iter = routing_table_.begin(); iter != routing_table_.end();
         iter++) {
        if ((*iter).dest.s_addr == dest.s_addr &&
            (*iter).netmask.s_addr == netmask.s_addr) {
            routing_table_.erase(iter);
            return;
        }
    }
}

std::optional<ip_t> RoutingTable::Query(ip_t dest_ip) {
    std::scoped_lock lock(routing_mutex_);

    int found = 0;
    ip_t nexthop{0};
    ip_t prefix{0};
    for (auto& entry : routing_table_) {
        if (((dest_ip.s_addr & entry.netmask.s_addr) ==
             (entry.netmask.s_addr & entry.netmask.s_addr)) &&
            ntohl(prefix.s_addr) < ntohl(entry.netmask.s_addr) &&
            entry.status != kGarbage) {
            found = 1;
            nexthop = entry.nexthop_ip;
            prefix = entry.netmask;
        }
    }
    if (found) {
        return nexthop;
    } else {
        return {};
    }
}

int RoutingTable::UpdateByDistanceVector(ip_t nexthop_ip, const void* buffer,
                                         int length) {
    auto distance_vector = (struct DVEntry*)buffer;

    std::scoped_lock lock(routing_mutex_);
    int modified = 0;
    for (int i = 0; i < length; i++) {
        modified |= this->unsafeInsert(distance_vector[i].dest,
                                       distance_vector[i].netmask, nexthop_ip,
                                       ntohl(distance_vector[i].distance) + 1);
    }
    return modified;
}

std::pair<std::uint8_t*, int> RoutingTable::SerializeDistanceVector(
    ip_t local_ip, ip_t nexthop_ip) {
    std::scoped_lock lock(routing_mutex_);
    int dv_length =
        sizeof(int) + sizeof(ip_t) + sizeof(DVEntry) * routing_table_.size();

    auto dv_ptr = new std::uint8_t[dv_length]();
    *(ip_t*)dv_ptr = local_ip;
    *(int*)(dv_ptr + 4) = htonl(routing_table_.size());
    struct DVEntry* vector_ptr = (struct DVEntry*)(dv_ptr + 8);

    for (int i = 0; i < routing_table_.size(); i++) {
        vector_ptr[i].dest = routing_table_[i].dest;
        vector_ptr[i].netmask = routing_table_[i].netmask;
        if (routing_table_[i].nexthop_ip.s_addr == nexthop_ip.s_addr) {
            // poison reverse
            vector_ptr[i].distance = htonl(1 + kRipPoisonThresh);
        } else {
            vector_ptr[i].distance = htonl(routing_table_[i].distance);
        }
    }

    return std::make_pair(dv_ptr, dv_length);
}

int RoutingTable::Aging() {
    int modified = 0;
    std::scoped_lock lock(routing_mutex_);
    for (auto iter = routing_table_.begin(); iter != routing_table_.end();) {
        if (iter->status == kStable) {
            iter->age_timer--;
            if (iter->age_timer == 0) {
                modified = 1;
                // mark as inreachable
                iter->distance = 1 + kRipPoisonThresh;
                iter->status = kGarbage;
                iter->gc_timer = kRipMaxGarbage;
            }
            iter++;
        } else if (iter->status == kGarbage) {
            iter->gc_timer--;
            if (iter->gc_timer == 0) {
                // timeout for garbage, directly remove from routing table.
                iter = routing_table_.erase(iter);
            }
        } else {
            iter++;
        }
    }
    return modified;
}

void RoutingTable::GarbageCollection() {
    std::scoped_lock lock(routing_mutex_);
    this->unsafeGarbageCollection();
}

void RoutingTable::ShowInformation(std::ostream& os) {
    os << " destination "
       << " netmask "
       << " gateway "
       << " distance " << std::endl;
    std::scoped_lock lock(routing_mutex_);
    for (auto& entry : routing_table_) {
        os << inet_ntoa(entry.dest) << " " << inet_ntoa(entry.netmask) << " "
           << inet_ntoa(entry.nexthop_ip) << " " << entry.distance << std::endl;
    }
}

std::ostream& operator<<(std::ostream& os, RoutingTable& table) {
    table.ShowInformation(os);
    return os;
}

int RoutingTable::unsafeInsert(ip_t dest, ip_t netmask, ip_t nexthop_ip,
                               int distance) {
    int modified = 0;
    for (auto& entry : routing_table_) {
        if (entry.dest.s_addr == dest.s_addr &&
            entry.netmask.s_addr == entry.netmask.s_addr) {
            if (entry.nexthop_ip.s_addr == nexthop_ip.s_addr) {
                if (distance > kRipPoisonThresh) {
                    entry.distance = kRipPoisonThresh + 1;
                    if (entry.status == kStable) {
                        modified = 1;
                        entry.status = kGarbage;
                        entry.age_timer = 0;
                        entry.gc_timer = 4;
                    }
                } else {
                    entry.distance = distance;
                    // reset status
                    entry.status = kStable;
                    entry.age_timer = kRipMaxAging;
                    entry.gc_timer = 0;
                }
            } else if (distance < entry.distance && entry.status == kStable) {
                entry.distance = distance;
                entry.nexthop_ip = nexthop_ip;
                entry.age_timer = kRipMaxAging;
            }
            return modified;
        }
    }
    if (distance < kRipPoisonThresh) {
        routing_table_.push_back(RoutingEntry{kStable, kRipMaxAging, 0, dest,
                                              netmask, nexthop_ip, distance});
    }
    return modified;
}

void RoutingTable::unsafeRemove(ip_t dest, ip_t netmask) {
    for (auto iter = routing_table_.begin(); iter != routing_table_.end();
         iter++) {
        if ((*iter).dest.s_addr == dest.s_addr &&
            (*iter).netmask.s_addr == netmask.s_addr) {
            routing_table_.erase(iter);
            return;
        }
    }
}

void RoutingTable::unsafeGarbageCollection() {
    auto iter = routing_table_.begin();
    while (iter != routing_table_.end()) {
        if (iter->status == kGarbage && iter->gc_timer == 0) {
            iter = routing_table_.erase(iter);
        } else {
            iter++;
        }
    }
}

}  // namespace network
}  // namespace minitcp