#include <arpa/inet.h>
#include <net/ethernet.h>
#include <stdio.h>

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
#include "../transport/socket.h"
#include "../transport/tcp_kernel.h"

using namespace minitcp;

int TCPCallback(const struct ip *ip_header, const void *buffer, int length);

int LinkCallback(const void *buffer, int length, int device_id) {
    mac_t *dest_mac = (struct ether_addr *)buffer;
    mac_t *src_mac = (struct ether_addr *)(buffer + 6);
    std::uint16_t ethernet_type = ntohs(*(std::uint16_t *)(buffer + 12));

    auto device_ptr = ethernet::getDevicePointer(device_id);
    MINITCP_ASSERT(device_ptr) << "getDevicePointer error" << std::endl;

    mac_t device_mac = device_ptr->GetMacAddress();

    char *packet_content = (char *)(buffer + 14);
    MINITCP_ASSERT(device_ptr) << "getDevicePointer error" << std::endl;

    if (std::strncmp((const char *)src_mac, (const char *)&device_mac,
                     sizeof(device_mac)) != 0) {
        if (ethernet_type == kEtherRouteType) {
            network::receiveDVTableCallback(buffer + 14);
        } else if (ethernet_type == kEtherArpType) {
            ethernet::receiveArpCallback(
                buffer + 14, sizeof(ethernet::ArpHeader), device_id);
        } else if (ethernet_type == kEtherIPv4Type) {
            network::kernel_callback(buffer + 14, length - 14 - 4);
        } else {
            MINITCP_ASSERT(device_ptr) << "getDevicePointer error" << std::endl;
            std::cout << "device " << device_ptr->GetName() << " receive "
                      << length << " bytes, and the message is "
                      << packet_content << std::endl;
        }
    }
    return 0;
}

int NetworkCallback(const void *buffer, int length) {
    auto ip_header = reinterpret_cast<const struct ip *>(buffer);
    auto ip_content = (std::uint8_t *)(buffer + sizeof(struct ip));

    // upon receiving an IP packet
    // 1. if it fits a local Ip address, accept it.
    // 2. else find a good
    int status = 0;
    if (network::isLocalIP(ip_header->ip_dst)) {
        if (ip_header->ip_p == kIpProtoTcp) {
            transport::tcpReceiveCallback(ip_header, buffer + sizeof(struct ip),
                                          length - sizeof(struct ip));
        } else {
            MINITCP_LOG(INFO)
                << "NetworkCallback: " << inet_ntoa(ip_header->ip_dst)
                << " receive a message from " << inet_ntoa(ip_header->ip_src)
                << " the content is " << ip_content << std::endl;
        }
    } else {
        status = network::forwardIPPacket(ip_header->ip_src, ip_header->ip_dst,
                                          buffer, length);
    }
    return status;
}

int main(int argc, char *argv[]) {
    ethernet::addAllDevices("veth");
    network::initRoutingTable();

    ethernet::setFrameReceiveCallback(LinkCallback);
    network::setIPPacketReceiveCallback(NetworkCallback);

    ethernet::start();
    ethernet::timerArpHandler();
    network::timerRIPHandler();

    timerStart();

    char src_ip_str[] = "10.100.1.1";
    char dest_ip_str[] = "10.100.2.1";

    ip_t src_ip, dest_ip;
    inet_aton(src_ip_str, &src_ip);
    inet_aton(dest_ip_str, &dest_ip);

    struct sockaddr_in address;

    class transport::Socket socket;

    connection_key_t key = transport::makeKey(dest_ip, src_ip, 80, 80, false);

    insertSocket(key, &socket);

    address.sin_family = AF_INET;
    address.sin_addr = src_ip;
    address.sin_port = 80;
    socket.Bind((struct sockaddr *)&address, sizeof(address));

    address.sin_addr = dest_ip;
    std::this_thread::sleep_for(std::chrono::seconds(30));

    socket.Connect((struct sockaddr *)&address, sizeof(address));

    MINITCP_LOG(INFO) << "nice boat!" << std::endl;

    while (true)
        ;
    return 0;
}