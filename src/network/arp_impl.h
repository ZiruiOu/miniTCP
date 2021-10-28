#ifndef MINITCP_SRC_NETWORK_ARP_IMPL_H_
#define MINITCP_SRC_NETWORK_ARP_IMPL_H_

#include <mutex>
#include <optional>
#include <vector>

#include "../common/constant.h"
#include "../common/types.h"

namespace minitcp {
namespace network {

struct ArpTableEntry {
    ip_t peer_ip;
    mac_t peer_mac;
    ip_t local_ip;    // from which local device?
    mac_t local_mac;  // from which local device?
    int local_id;     // local device id
    int aging;
};

//@brief: to detect the cost to neighbours
class ArpTable {
   public:
    ArpTable() = default;
    ~ArpTable() = default;

    void InitArpTable();
    int UpdateArpTable(mac_t *peer_mac, ip_t *peer_ip, mac_t *local_mac,
                       ip_t *local_ip);
    std::optional<mac_t> QueryArpTable(ip_t peer_ip);
    int SendArp(ip_t peer_ip, std::uint16_t arp_type, bool broadcast);

    /* arp
     * 0-2: hardware type = 1 for ethernet
     * 2-4: protocol type = 0x0800 for IpV4
     * 4-5: hardware address length = 6
     * 5-6: protocol address length = 4
     * 6-8: operation, REQUEST = 1, REPLY = 2
     * 8-14: sender hadrware address : sender mac address
     * 14-18: sender protocol address : sender ipv4 address
     * 18-24: receiver hardware address : receiver mac address
     * 24-28: receiver protocol address : receiver ipv4 address
     */

    int AgingArpTable();

   private:
    // Unsafe functions, some internal helper
    void unsafeUpdateArpTable();
    void unsafeQueryArpTable();

    std::mutex arp_mutex_;
    std::vector<ArpTableEntry> arp_table_;
};

}  // namespace network
}  // namespace minitcp

#endif  // ! MINITCP_SRC_NETWORK_ARP_IMPL_H_