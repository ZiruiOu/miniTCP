#ifndef MINITCP_SRC_NETWORK_ROUTING_IMPL_H_
#define MINITCP_SRC_NETWORK_ROUTING_IMPL_H_

#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#include "../common/types.h"

namespace minitcp {
namespace network {

#pragma pack(4)
struct DVEntry {
    ip_t dest;     // network destination
    ip_t netmask;  //
    int distance;  // distance
};
#pragma pack()

enum RoutingEntryStatus { kPersist = 0, kStable, kGarbage };

struct RoutingEntry {
    enum RoutingEntryStatus status;  // entry status
    int age_timer;                   // aging timer
    int gc_timer;                    // gc timer

    ip_t dest;        // local ip
    ip_t netmask;     // netmask for local ip
    ip_t nexthop_ip;  // nexthop
    int distance;     // distance
};

class RoutingTable {
   public:
    static RoutingTable& GetInstance() {
        static std::once_flag routing_table_flag_;
        std::call_once(routing_table_flag_,
                       [&]() { routing_singleton_ = new RoutingTable(); });
        return *routing_singleton_;
    }

    // disallow copy and move
    RoutingTable(const RoutingTable&) = delete;
    RoutingTable& operator=(const RoutingTable&) = delete;
    RoutingTable(RoutingTable&&) = delete;
    RoutingTable& operator=(RoutingTable&&) = delete;

    void InitRoutingTable();
    /**
     * @brief Insert a new route entry into route_table_.
     * @param dest The local ip of the device.
     * @param netmask The netmask of the local device.
     * @param peer_ip The peer ip corresponding to the local device.
     */
    void Insert(ip_t dest, ip_t netmask, ip_t nexthop_ip, int distance);
    void Remove(ip_t dest, ip_t netmask);
    std::optional<ip_t> Query(ip_t dest_ip);
    /**
     * @brief Parse the distance vector received from nexthop_ip.
     * @param nexthop_ip The ip of the sender.
     * @param buffer  The payload of the ethernet frame.
     * @return 0 on successfully parse the buffer, -1 on failure.
     */
    int UpdateByDistanceVector(ip_t nexthop_ip, const void* buffer, int length);
    std::pair<std::uint8_t*, int> SerializeDistanceVector(ip_t local_ip,
                                                          ip_t nexthop_ip);
    int Aging();
    void GarbageCollection();

    void ShowInformation(std::ostream& os);
    friend std::ostream& operator<<(std::ostream& os, RoutingTable&);

   private:
    RoutingTable() = default;
    ~RoutingTable() = default;
    // some unsafe function without lock, serve as internal helpers.
    int unsafeInsert(ip_t dest, ip_t netmask, ip_t nexthop_ip, int distance);
    void unsafeRemove(ip_t dest, ip_t netmask);
    void unsafeGarbageCollection();

    static RoutingTable* routing_singleton_;

    std::mutex routing_mutex_;
    std::vector<struct RoutingEntry> routing_table_{};
    std::map<std::uint32_t, std::uint32_t> routing_timer_;
};

}  // namespace network
}  // namespace minitcp

#endif  // ! MINITCP_SRC_NETWORK_ROUTING_IMPL_H_