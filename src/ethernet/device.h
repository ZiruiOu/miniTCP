#ifndef MINITCP_SRC_ETHERNET_DEVICE_H_
#define MINITCP_SRC_ETHERNET_DEVICE_H_

#include <arpa/inet.h>
#include <cstring>
#include <pcap.h>
#include <iostream>
#include <net/ethernet.h>
#include <string>

#include "../common/logging.h"
#include "macaddr.h"

namespace minitcp {
namespace ethernet {
class DeviceBase {
   public:
    DeviceBase(){};
    ~DeviceBase(){};

    virtual int SendFrame(const std::uint8_t* buffer, std::size_t length,
                          int ethernet_type, MacAddress dest_mac) = 0;
    virtual void ReceivePoll() = 0;
};

class EthernetDevice : public DeviceBase {
   public:
    explicit EthernetDevice(const std::string& device_name);
    EthernetDevice(const std::string& device_name, const mac_t& mac_addr);
    ~EthernetDevice();

    int SendFrame(const std::uint8_t* buffer, std::size_t length,
                  int ethernet_type, MacAddress dest_mac) override;
    void ReceivePoll() override;

    // no copy and move
    EthernetDevice(const EthernetDevice&) = delete;
    EthernetDevice& operator=(const EthernetDevice&) = delete;
    EthernetDevice(EthernetDevice&&) = delete;
    EthernetDevice& operator=(EthernetDevice&&) = delete;

   private:
    MacAddress mac_addr_;
    std::string device_name_;

    pcap_t* pcap_handler_;
};

}  // namespace ethernet
}  // namespace minitcp

#endif  //! MINITCP_SRC_ETHERNET_DEVICE_H_