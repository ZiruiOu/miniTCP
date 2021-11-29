#include "callbacks.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <thread>

#include "../common/constant.h"
#include "../common/ipaddr.h"
#include "../common/logging.h"
#include "../common/macaddr.h"
#include "../ethernet/arp.h"
#include "../ethernet/device.h"
#include "../ethernet/packetio.h"
#include "../network/ip.h"
#include "../network/routing.h"
#include "socket.h"
#include "tcp_kernel.h"

namespace minitcp {
namespace transport {

int TCPCallback(const struct ip *ip_header, const void *buffer, int length);

int LinkCallback(const void *buffer, int length, int device_id) {
  mac_t *dest_mac = (struct ether_addr *)buffer;
  mac_t *src_mac = (struct ether_addr *)(buffer + 6);
  std::uint16_t ethernet_type = ntohs(*(std::uint16_t *)(buffer + 12));
  char *packet_content = (char *)(buffer + 14);

  auto device_ptr = ethernet::getDevicePointer(device_id);
  MINITCP_ASSERT(device_ptr) << "getDevicePointer error" << std::endl;

  mac_t device_mac = device_ptr->GetMacAddress();

  if (std::strncmp((const char *)src_mac, (const char *)&device_mac,
                   sizeof(device_mac)) != 0) {
    if (ethernet_type == kEtherRouteType) {
      network::receiveDVTableCallback(buffer + 14);
    } else if (ethernet_type == kEtherArpType) {
      ethernet::receiveArpCallback(buffer + 14, sizeof(ethernet::ArpHeader),
                                   device_id);
    } else if (ethernet_type == kEtherIPv4Type) {
      network::kernel_callback(buffer + 14, length - 14 - 4);
    } else {
      std::cout << "device " << device_ptr->GetName() << " receive " << length
                << " bytes, and the message is " << packet_content << std::endl;
    }
  }
  return 0;
}

int NetworkCallback(const void *buffer, int length) {
  auto ip_header = reinterpret_cast<const struct ip *>(buffer);
  auto ip_content = (std::uint8_t *)(buffer + 20);

  int status = 0;
  if (network::isLocalIP(ip_header->ip_dst)) {
    if (ip_header->ip_p == kIpProtoTcp) {
      length -= 20;
      transport::tcpReceiveCallback(ip_header, ip_content, length);
    } else {
      MINITCP_LOG(INFO) << "NetworkCallback: " << inet_ntoa(ip_header->ip_dst)
                        << " receive a message from "
                        << inet_ntoa(ip_header->ip_src) << " the content is "
                        << ip_content << std::endl;
    }
  } else {
    status = network::forwardIPPacket(ip_header->ip_src, ip_header->ip_dst,
                                      buffer, length);
  }
  return status;
}

void startTCPKernel() {
  ethernet::addAllDevices("veth");
  network::initRoutingTable();

  ethernet::setFrameReceiveCallback(LinkCallback);
  network::setIPPacketReceiveCallback(NetworkCallback);

  ethernet::start();
  ethernet::timerArpHandler();
  network::timerRIPHandler();

  timerStart();
}

}  // namespace transport
}  // namespace minitcp
