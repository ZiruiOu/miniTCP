#ifndef MINITCP_SRC_ETHERNET_DEVICE_IMPL_H_
#define MINITCP_SRC_ETHERNET_DEVICE_IMPL_H_

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <pcap.h>
#include <sys/types.h>

#include <cstring>
#include <iostream>
#include <queue>
#include <string>

#include "../common/constant.h"
#include "../common/logging.h"
#include "../common/types.h"
#include "arp_impl.h"
#include "ethkernel.h"
#include "packetio.h"

namespace minitcp {
namespace ethernet {
class DeviceBase {
 public:
  DeviceBase(){};
  ~DeviceBase(){};

  virtual int SendFrame(const std::uint8_t* buffer, std::size_t length,
                        int ethernet_type, const void* dest_mac) = 0;
  virtual void ReceivePoll() = 0;

  virtual const std::string& GetName() const = 0;
};

class EthernetDevice : public DeviceBase {
 public:
  explicit EthernetDevice(const std::string&);
  EthernetDevice(const std::string&, const mac_t&, const ip_t&);
  ~EthernetDevice();

  // no copy and no move, in case of double free caused by copy & move.
  EthernetDevice(const EthernetDevice&) = delete;
  EthernetDevice& operator=(const EthernetDevice&) = delete;
  EthernetDevice(EthernetDevice&&) = delete;
  EthernetDevice& operator=(EthernetDevice&&) = delete;

  int SendFrame(const std::uint8_t* buffer, std::size_t length,
                int ethernet_type, const void* dest_mac) override;
  void ReceivePoll() override;

  int SendArp(std::uint16_t arp_type, const void* dest_mac,
              const void* dest_ip);

  const std::string& GetName() const override { return device_name_; }
  mac_t GetMacAddress() const { return mac_addr_; }
  ip_t GetIpAddress() const { return ip_addr_; }
  ip_t GetNetmask() const { return netmask_; }

  friend class EthernetKernel;

 protected:
  int InitMacAddress();
  int InitIpAddress();
  int InitPcapHandler();

 private:
  mac_t mac_addr_;
  ip_t ip_addr_;
  ip_t netmask_;
  std::string device_name_;
  // frameReceiveCallback callback_public_{0};
  // frameReceiveCallback callback_private_{0};
  pcap_t* pcap_handler_;
};

}  // namespace ethernet
}  // namespace minitcp

#endif  //! MINITCP_SRC_ETHERNET_DEVICE_IMPL_H_