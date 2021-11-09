#ifndef MINITCP_SRC_ETHERNET_DEVICE_H_
#define MINITCP_SRC_ETHERNET_DEVICE_H_

#include <string>

#include "device_impl.h"

namespace minitcp {
namespace ethernet {

#ifdef __cplusplus
extern "C" {
#endif  // ! __cplusplus

/**
 * Add a device to the library for sending/receiving packets. *
 * @param device Name of network device to send/receive packet on.
 * @return A non-negative _device-ID_ on success, -1 on error. */
int addDevice(const char* device);

/**
 * Find a device added by ‘addDevice‘. *
 * @param device Name of the network device.
 * @return A non-negative _device-ID_ on success, -1 if no such device
 * was found. */
int findDevice(const char* device);

/**
 * Get the pointer of the DeviceCOntrolBlock
 * @param id id of the device
 * @return The pointer of the corresponding device on success, NULL if no such
 * device exists. */
class EthernetDevice* getDevicePointer(int id);

/**
 * Add all network interface into the ethernet kernel
 * @param start_with_prefix the name of the devices should start with
 * start_wit_prefix
 * @return 0 on success, -1 if fail.
 **/
int addAllDevices(const char* start_with_prefix);

int broadcastArp();

int getDeviceNumber();

/**
 * Start the ethernet kernel
 *
 */
void start();

#ifdef __cplusplus
}
#endif  // ! __cplusplus
}  // namespace ethernet
}  // namespace minitcp
#endif  // ! MINITCP_SRC_ETHERNET_DEVICE_H_