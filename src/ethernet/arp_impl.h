#ifndef MINITCP_SRC_NETWORK_ARP_IMPL_H_
#define MINITCP_SRC_NETWORK_ARP_IMPL_H_

#include <mutex>
#include <optional>
#include <vector>

#include "../common/constant.h"
#include "../common/types.h"

namespace minitcp {
namespace ethernet {

struct ArpTableEntry {
    ip_t dest_ip;
    mac_t dest_mac;
    int local_id;  // local device id
    int aging;
};

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

#pragma pack(1)
struct ArpHeader {
    std::uint16_t hw_type;
    std::uint16_t proto_type;
    std::uint8_t hwaddr_length;
    std::uint8_t protoaddr_length;
    std::uint16_t operation;
    mac_t src_mac;
    ip_t src_ip;
    mac_t dest_mac;
    ip_t dest_ip;
};
#pragma pack()

class ArpTable {
   public:
    static ArpTable &GetInstance() {
        static std::once_flag arptable_flag_;
        std::call_once(arptable_flag_,
                       [&]() { arp_singleton_ = new ArpTable(); });
        return *arp_singleton_;
    }

    // disallow copy and move
    ArpTable(const ArpTable &) = delete;
    ArpTable &operator=(const ArpTable &) = delete;
    ArpTable(ArpTable &&) = delete;
    ArpTable &operator=(ArpTable &&) = delete;

    void Update(ip_t *dest_ip, mac_t *dest_mac, int local_id);
    int Persist(ip_t *dest_ip, mac_t *dest_mac, int local_id);
    std::optional<ArpTableEntry> Query(ip_t dest_ip);
    std::optional<ArpTableEntry> QueryByDeviceId(int id);
    // int SendArp(ip_t peer_ip, std::uint16_t arp_type, bool broadcast);
    int Announce();
    int Aging();

   private:
    ArpTable() = default;
    ~ArpTable() = default;
    static ArpTable *arp_singleton_;
    std::mutex arp_mutex_;
    std::vector<ArpTableEntry> arp_table_ = {};
};

}  // namespace ethernet
}  // namespace minitcp

#endif  // ! MINITCP_SRC_NETWORK_ARP_IMPL_H_