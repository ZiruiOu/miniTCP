#include <memory>

#include "./device.h"

namespace minitcp {
namespace ethernet {
EthernetDevice::EthernetDevice(const std::string& device_name)
    : device_name_(device_name) {
    char err_buffer[PCAP_ERRBUF_SIZE];

    mac_addr_.GetAddressByDevice(device_name_.c_str());
    pcap_handler_ =
        pcap_open_live(device_name_.c_str(), 1520, 1, 1000, err_buffer);

    MINITCP_ASSERT(pcap_handler_) << "pcap_open_live error: " << device_name_
                                  << " " << err_buffer << std::endl;
    MINITCP_LOG(DEBUG) << " EthernetDevice " << device_name
                       << " registered successfully with mac address"
                       << std::endl;
}

EthernetDevice::EthernetDevice(const std::string& device_name,
                               const mac_t& mac_addr)
    : device_name_(device_name), mac_addr_(mac_addr) {
    char err_buffer[PCAP_ERRBUF_SIZE];
    pcap_handler_ =
        pcap_open_live(device_name_.c_str(), 1520, 1, 1000, err_buffer);

    MINITCP_ASSERT(pcap_handler_) << "pcap_open_live error: " << device_name_
                                  << " " << err_buffer << std::endl;
}

EthernetDevice::~EthernetDevice() { pcap_close(pcap_handler_); }

int EthernetDevice::SendFrame(const std::uint8_t* buffer, std::size_t length,
                              int ethernet_type, MacAddress dest_mac) {
    // TODO : use buffer writer + frame factory
    auto frame =
        std::shared_ptr<std::uint8_t>(new std::uint8_t[14 + length + 4]());
    auto type_field = htons(static_cast<std::uint16_t>(ethernet_type));
    std::uint32_t checksum = 0;
    std::memcpy(frame.get(), dest_mac.GetAddressPointer(), sizeof(mac_t));
    std::memcpy(frame.get() + 6, mac_addr_.GetAddressPointer(), sizeof(mac_t));
    std::memcpy(frame.get() + 12, &type_field, sizeof(type_field));
    std::memcpy(frame.get() + 14, buffer, length);

    int status = pcap_sendpacket(pcap_handler_, frame.get(), 18 + length);
    if (status == -1) {
        MINITCP_LOG(ERROR) << "sendframe: " << device_name_
                           << " send frame fail " << std::endl;
    }
}

void EthernetDevice::ReceivePoll() {
    int result;
    struct pcap_pkthdr* packet_header;
    const u_char* packet_data;
    while ((result = pcap_next_ex(pcap_handler_, &packet_header,
                                  &packet_data)) >= 0) {
        // A timeout
        if (result == 0) {
            continue;
        }
        std::cout << "Receive " << packet_header->len << " bytes "
                  << packet_data << std::endl;
    }
}
}  // namespace ethernet
}  // namespace minitcp