#ifndef MINITCP_SRC_NETWORK_IP_H_
#define MINITCP_SRC_NETWORK_IP_H_
/**
 * @file ip.h
 * @brief Library supporting sending/receiving IP packets encapsulated in an
 * Ethernet II frame.
 */

#include <netinet/ip.h>

#include "../common/types.h"

namespace minitcp {
namespace network {
#ifdef __cplusplus
extern "C" {
#endif  // ! __cplusplus

/**
 * @brief Send an IP packet to specified host.
 *
 * @param src Source IP address.
 * @param dest Destination IP address.
 * @param proto Value of 'protocol' field in IP header
 * @param buf  Pointer to IP payload.
 * @param len Length of IP payload.
 * @return 0 on success, -1 on error.
 */
int sendIPPacket(const struct in_addr src, const struct in_addr dest, int proto,
                 const void* buf, int len);

/**
 * @brief  When receiving a packet from some neighbours, forward the packet to
 * the destination.
 * @param  src  Source IP address.
 * @param  dest Destination IP address.
 * @param  buf  Pointer to IP datagram (IP header + IP payload).
 * @param  len  Length of the IP datagram.
 * @return 0 on success, -1 on error.
 */
int forwardIPPacket(const struct in_addr src, const struct in_addr dest,
                    const void* ip_packet, int len);

/**
 *  @brief Proecss an IP packet upon receiving it.
 *
 *  @param buf Pointer to the packet.
 *  @param len Length of the packet.
 *  @return 0 on success, -1 on error.
 *  @see addDevice
 */
typedef int (*IPPacketReceiveCallback)(const void* buf, int len);

/**
 *  @brief The callback function that needs to call once an IP packet is
 * received.
 *
 */
extern IPPacketReceiveCallback kernel_callback;

/**
 *  @brief Register a callback function to be called each time an IP packet was
 * received.
 *
 *  @param callback The callback function.
 *  @param 0 on success, -1 on error.
 *  @see   IPPacketReceiveCallback
 */
int setIPPacketReceiveCallback(IPPacketReceiveCallback callback);

/**
 *  @brief Manully add an item to routing table. Useful when talking with real
 * Linux machines.
 *  @param dest The destination IP prefix.
 *  @param mask The subnet mask of the destination IP prefix.
 *  @param nextHopMAC MAC address of the next hop.
 *  @param device Name of device to send packets on.
 *  @return 0 on success, -1 on error.
 */
int setRoutingTable(const struct in_addr dest, const struct in_addr mask,
                    const void* nextHopMAC, const char* device);

/**
 *  @brief Judge whether an ip address is an local ip of not, helpful when
 * writing callback function in network layer.
 *  @param address An ip address.
 *  @return True on local ip address, false otherwise.
 */
bool isLocalIP(ip_t address);

#ifdef __cplusplus
}
#endif  // ! __cplusplus
}  // namespace network
}  // namespace minitcp

#endif  // ! MINITCP_SRC_NETWORK_IP_H_