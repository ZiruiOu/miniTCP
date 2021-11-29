#include "ip.h"

#include "../common/constant.h"
#include "../common/logging.h"
#include "../common/types.h"
#include "../ethernet/arp.h"
#include "../ethernet/device.h"
#include "routing.h"

namespace minitcp {
namespace network {
// struct ip in <netinet/ip.h>

std::uint16_t calculateCheckSum(const void* header) {
  auto ip_header = reinterpret_cast<const struct ip*>(header);
  int length = ip_header->ip_hl;
  if (length < 5) {
    MINITCP_LOG(ERROR) << "calculate IP Ckecksum: receive a bad ip packet "
                          "with length less than 20 bytes."
                       << std::endl;
    return -1;
  }
  const std::uint16_t* as_uint16_buffer =
      static_cast<const std::uint16_t*>(header);
  std::uint32_t sum = 0;
  for (int i = 0; i < length * 2; i++) {
    sum += as_uint16_buffer[i];
  }
  // set the old checksum as 0
  sum -= as_uint16_buffer[5];
  while ((sum >> 16)) {
    sum = (sum >> 16) + (sum & 0xffff);
  }
  return ~static_cast<std::uint16_t>(sum);
}

int sendIPPacketInternal(const ip_t src, const ip_t dest, const void* ip_packet,
                         int ip_length) {
  int status = 0;

  // MINITCP_LOG(INFO) << "destination is " << inet_ntoa(dest) << std::endl;

  // find the routing table
  auto routing_ip = queryRoutingTable(dest);
  if (routing_ip) {
    // find the arp entry
    auto arp_entry = ethernet::queryArpTable(*routing_ip);
    if (arp_entry) {
      // MINITCP_LOG(INFO) << "A packet is sending from " << inet_ntoa(src)
      //                   << " to " << inet_ntoa(arp_entry->dest_ip) <<
      //                   std::endl;
      status = ethernet::sendFrame(ip_packet, ip_length, kEtherIPv4Type,
                                   (const void*)&(arp_entry->dest_mac),
                                   arp_entry->local_id);
    } else {
      // MINITCP_LOG(ERROR) << "sendIPPacket : nexthop ip "
      //                    << inet_ntoa(*routing_ip)
      //                    << " is not found in the routing table. " <<
      //                    std::endl;
      status = 1;
    }
  } else {
    // MINITCP_LOG(ERROR) << "sendIPPacket : destination ip " << inet_ntoa(dest)
    //                    << " is not found in the routing table. " <<
    //                    std::endl;
    status = 1;
  }

  return status;
}
/*
 *
 * defaults
 * ip_version(4 bits) = 0x0100 as default, the protocol for ip.
 * ip_ihl(4 bits) = 0x0101 as default, length of the Ip header in 32 bit words
 * IN THE SCALE OF 4 BYTES.
 * ip_tos(8 bits) = 0
 * ip_len(16 bits) = < 1480 as default, length of datagram.
 * ip_id(16 bits) = identification for help reassemble the datagram
 * ip_flags(3 bits) =
 * ip_off(13 bits) = offset of the datagram IN THE SCALE OF 8 BYTES.
 * ip_ttl(8 bits) = time to live, MAX = 225
 * ip_p(8 bits) = protocol 1-255, ICMP = 1, TCP = 6, OSPF = 89
 * ip_sum(16 bits) = checksum of the
 * ip_src = source ip address
 * ip_dst = destination ip address
 */
int sendIPPacket(const struct in_addr src, const struct in_addr dest, int proto,
                 const void* buf, int len) {
  // TODO : add constant into ../common/constant.h
  int ip_length = 20 + len;
  auto ip_packet = new std::uint8_t[ip_length]();
  auto ip_header = reinterpret_cast<struct ip*>(ip_packet);

  // default value
  ip_header->ip_v = 0x04;
  ip_header->ip_hl = 0x05;
  ip_header->ip_tos = 0;
  // TODO :
  ip_header->ip_len = htons(20 + len);
  ip_header->ip_id = htons(0x0);
  ip_header->ip_off = 1 << 6;  // don't fragment
  // TODO : add into constant.h
  ip_header->ip_ttl = 255;
  ip_header->ip_p = static_cast<std::uint8_t>(proto);
  ip_header->ip_sum = 0x0;
  ip_header->ip_src = src;
  ip_header->ip_dst = dest;

  ip_header->ip_sum = calculateCheckSum(ip_header);

  std::memcpy(ip_packet + 20, buf, len);

  int status = sendIPPacketInternal(src, dest, ip_packet, ip_length);

  delete[] ip_packet;
  return status;
}

int forwardIPPacket(const struct in_addr src, const struct in_addr dest,
                    const void* ip_packet, int len) {
  int status = 0;
  auto buffer = new std::uint8_t[len]();
  std::memcpy(buffer, ip_packet, len);

  auto ip_header = reinterpret_cast<struct ip*>(buffer);
  // TODO : recalculate the checksum
  std::uint16_t ip_checksum = calculateCheckSum(ip_header);
  // TODO : verify the checksum

  if (ip_checksum != ip_header->ip_sum) {
    MINITCP_LOG(ERROR) << "forwardIPPacket: drop error packet." << std::endl;
    // TODO : send icmp message.
    delete[] buffer;
    return 1;
  }

  ip_header->ip_ttl--;
  if (ip_header->ip_ttl == 0) {
    status = 1;
    // TODO : send back an ICMP message.
  } else {
    std::uint16_t new_checksum = calculateCheckSum(ip_header);
    ip_header->ip_sum = new_checksum;
    status = sendIPPacketInternal(src, dest, buffer, len);
  }

  delete[] buffer;
  return status;
}

IPPacketReceiveCallback kernel_callback;

int setIPPacketReceiveCallback(IPPacketReceiveCallback callback) {
  kernel_callback = callback;
  return 0;
}

int setRoutingTable(const struct in_addr dest, const struct in_addr mask,
                    const void* nextHopMAC, const char* device) {
  // TODO : persistent in arp table and routing table
  return 0;
}

bool isLocalIP(ip_t address) {
  auto routing_ip = queryRoutingTable(address);
  // If it is default gateway, then it should be a local ip address.
  return routing_ip && (routing_ip->s_addr == 0);
}

}  // namespace network
}  // namespace minitcp