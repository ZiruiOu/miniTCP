/**
 * @file packetio.h
 * @brief Library supporting sending/receiving Ethernet II frames. */
#ifndef MINITCP_SRC_ETHERNET_PACKETIO_H_
#define MINITCP_SRC_ETHERNET_PACKETIO_H_
#include <netinet/ether.h>

namespace minitcp {
namespace ethernet {

#ifdef __cplusplus
extern "C" {
#endif  // ! __cplusplus

/**
 * @brief Encapsulate some data into an Ethernet II frame and send it. *
 * @param buf Pointer to the payload.
 * @param len Length of the payload.
 * @param ethtype EtherType field value of this frame.
 * @param destmac MAC address of the destination.
 * @param id ID of the device(returned by ‘addDevice‘) to send on. * @return
 * 0 on success, -1 on error.
 * @see addDevice
 */
int sendFrame(const void* buf, int len, int ethtype, const void* destmac,
              int id);

/**
 * @brief Process a frame upon receiving it.*
 *
 * @param buf Pointer to the frame.
 * @param len Length of the frame.
 * @param id ID of the device (returned by ‘addDevice‘) receiving current
 * frame.
 * @return 0 on success, -1 on error.
 * @see addDevice */
typedef int (*frameReceiveCallback)(const void*, int, int);

/**
 * @brief Register a callback function to be called each time an Ethernet II
 * frame was received.*
 *
 * @param callback the callback function.
 * @return 0 on success, -1 on error.
 * @see frameReceiveCallback */
int setFrameReceiveCallback(frameReceiveCallback callback);

#ifdef __cplusplus
}
#endif  // ! __cplusplus

}  // namespace ethernet
}  // namespace minitcp

#endif  // ! MINITCP_SRC_ETHERNET_PACKETIO_H_