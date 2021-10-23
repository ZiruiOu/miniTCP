#include "device_impl.h"

#include <memory>

namespace minitcp {
namespace ethernet {
EthernetDevice::EthernetDevice(const std::string& device_name)
    : device_name_(device_name) {
    int status;
    char err_buffer[PCAP_ERRBUF_SIZE];

    status = InitMacAddress();
    MINITCP_ASSERT(status == 0)
        << "InitMacAddress error: " << device_name_ << std::endl;

    status = InitIpAddress();
    MINITCP_ASSERT(status == 0)
        << "InitIpAddress error: " << device_name_ << std::endl;

    InitPcapHandler();

    MINITCP_LOG(INFO) << " EthernetDevice " << device_name
                      << " registered successfully " << std::endl;
}

EthernetDevice::EthernetDevice(const std::string& device_name,
                               const mac_t& mac_addr, const ip_t& ip_addr)
    : device_name_(device_name), mac_addr_(mac_addr), ip_addr_(ip_addr) {
    InitPcapHandler();
    MINITCP_LOG(INFO) << " EthernetDevice " << device_name
                      << " registered successfully " << std::endl;
}

EthernetDevice::~EthernetDevice() { pcap_close(pcap_handler_); }

int EthernetDevice::SendFrame(const std::uint8_t* buffer, std::size_t length,
                              int ethernet_type, const void* dest_mac) {
    // TODO : Use buffer writer + frame factory.
    // TODO : Allow zero-copy instead of allocating new buffer at each layer and
    // copy multiple times.
    int status;

    {
        auto frame = std::shared_ptr<std::uint8_t>(
            new std::uint8_t[14 + length](),
            [](std::uint8_t* ptr) { delete[] ptr; });

        auto type_field = htons(static_cast<std::uint16_t>(ethernet_type));

        std::uint32_t checksum = 0;
        std::memcpy(frame.get(), dest_mac, sizeof(mac_t));
        std::memcpy(frame.get() + 6, &mac_addr_, sizeof(mac_t));
        std::memcpy(frame.get() + 12, &type_field, sizeof(type_field));
        std::memcpy(frame.get() + 14, buffer, length);

        status = pcap_sendpacket(pcap_handler_, frame.get(), 14 + length);
    }

    if (status == -1) {
        char err_buffer[PCAP_ERRBUF_SIZE];
        pcap_perror(pcap_handler_, err_buffer);
        MINITCP_LOG(ERROR) << "sendframe: " << device_name_
                           << " send frame fail " << err_buffer << std::endl;
    }
    return status;
}

void EthernetDevice::ReceivePoll() {
    int result;
    struct pcap_pkthdr packet_header;
    const u_char* packet_data;

    MINITCP_LOG(DEBUG) << "Ethernet Device : Polling device " << device_name_
                       << std::endl;
    while (true) {
        packet_data = pcap_next(pcap_handler_, &packet_header);
        std::cout << device_name_ << std::endl;
        if (packet_data != NULL) {
            std::cout << device_name_ << " Receive " << packet_header.caplen
                      << " bytes " << packet_data << std::endl;
        }
    }
}

int EthernetDevice::InitMacAddress() {
    // use getifaddrs
    int found = 0;
    struct ifaddrs *ifa_begin, *ifa;
    getifaddrs(&ifa_begin);
    for (ifa = ifa_begin; ifa != NULL; ifa = ifa->ifa_next) {
        int sa_family = ifa->ifa_addr->sa_family;
        if (sa_family == AF_PACKET &&
            std::strcmp(ifa->ifa_name, device_name_.c_str()) == 0) {
            struct sockaddr_ll* link_addr =
                reinterpret_cast<struct sockaddr_ll*>(ifa->ifa_addr);
            struct ether_addr* ifa_mac_addr =
                reinterpret_cast<struct ether_addr*>(link_addr->sll_addr);
            std::memcpy(&mac_addr_, ifa_mac_addr, sizeof(mac_t));
            MINITCP_LOG(INFO) << "EthernetDevice: Device " << device_name_
                              << " successfully get mac address " << std::endl;
            found = 1;
            break;
        }
    }
    freeifaddrs(ifa_begin);

    if (!found) {
        MINITCP_LOG(ERROR)
            << "EthernetDevice: cannot initialize mac address for "
            << device_name_ << std::endl;
    }
    return -(found == 0);
}

int EthernetDevice::InitIpAddress() {
    int found = 0;
    struct ifaddrs *ifa_begin, *ifa;
    getifaddrs(&ifa_begin);
    for (ifa = ifa_begin; ifa != NULL; ifa = ifa->ifa_next) {
        int sa_family = ifa->ifa_addr->sa_family;
        // ipv4 only
        if (sa_family == AF_INET && device_name_ == ifa->ifa_name) {
            struct sockaddr_in* inet_addr =
                reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            ip_addr_ = inet_addr->sin_addr;
            MINITCP_LOG(INFO) << "EthernetDevice: Device " << device_name_
                              << " successfully get ip address " << std::endl;
            found = 1;
            break;
        }
    }
    freeifaddrs(ifa_begin);
    if (!found) {
        MINITCP_LOG(ERROR)
            << "EthernetDevice : cannot initialize ip address for "
            << device_name_ << std::endl;
    }
    return -(found == 0);
}

int EthernetDevice::InitPcapHandler() {
    int status;
    char err_buffer[PCAP_ERRBUF_SIZE];

    pcap_handler_ = pcap_create(device_name_.c_str(), err_buffer);
    MINITCP_ASSERT(pcap_handler_) << "pcap_create error: " << device_name_
                                  << " " << err_buffer << std::endl;

    // timeout
    status = pcap_set_timeout(pcap_handler_, kPcapTimeout);
    MINITCP_ASSERT(status == 0)
        << "pcap_set_timeout error: " << device_name_ << " "
        << pcap_geterr(pcap_handler_) << std::endl;

    // pcap buffer length
    status = pcap_set_snaplen(pcap_handler_, kPcapSnapLen);
    MINITCP_ASSERT(status == 0)
        << "pcap_set_spanlen error: " << device_name_ << " "
        << pcap_geterr(pcap_handler_) << std::endl;

    // set immediate mode
    status = pcap_set_immediate_mode(pcap_handler_, 1);
    MINITCP_ASSERT(status == 0)
        << "pcap_set_immediate_mode error: " << device_name_ << " "
        << pcap_geterr(pcap_handler_) << std::endl;

    // set promisc mode
    status = pcap_set_promisc(pcap_handler_, 1);
    MINITCP_ASSERT(status == 0)
        << "pcap_set_promisc error: " << device_name_ << " "
        << pcap_geterr(pcap_handler_) << std::endl;

    // activate the pcap handler
    status = pcap_activate(pcap_handler_);
    MINITCP_ASSERT(status == 0) << "pcap_activate error: " << device_name_
                                << pcap_geterr(pcap_handler_) << std::endl;

    // set non blocking
    status = pcap_setnonblock(pcap_handler_, 1, err_buffer);
    MINITCP_ASSERT(status == 0) << "pcap_setnonblock error: " << device_name_
                                << err_buffer << std::endl;

    return 0;
}

}  // namespace ethernet
}  // namespace minitcp