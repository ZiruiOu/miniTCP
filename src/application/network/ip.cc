#include "../network/ip.h"

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
#include "../network/routing.h"

using namespace minitcp;

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
            char *packet_content = (char *)(buffer + 14);
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
        MINITCP_LOG(INFO) << "NetworkCallback: " << inet_ntoa(ip_header->ip_dst)
                          << " receive a message from "
                          << inet_ntoa(ip_header->ip_src) << " the content is "
                          << ip_content << std::endl;
    } else {
        status = network::forwardIPPacket(ip_header->ip_src, ip_header->ip_dst,
                                          buffer, length);
    }
    return status;
}

void userPrompt() {
    printf(
        "Usage : \n \
            To send message from s_ip to d_ip: send s_ip d_ip no-blank-space-message\n \
            To see the routing table: routing\n \
            To see the arp table: arp\n \
            To set the routing table: add s_ip netmask_ip nexthop_ip\n");
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
    char operation[30] = {};
    char src[30] = {};
    char dest[30] = {};

    userPrompt();
    while (scanf("%s", operation) == 1) {
        if (std::strcmp(operation, "send") == 0) {
            char message[200] = {};
            scanf("%s %s %s", src, dest, message);
            ip_t src_ip, dest_ip;
            inet_aton(src, &src_ip);
            inet_aton(dest, &dest_ip);
            network::sendIPPacket(src_ip, dest_ip, 253, message,
                                  std::strlen(message));
        } else if (std::strcmp(operation, "routing") == 0) {
            network::RoutingTable &table = network::RoutingTable::GetInstance();
            MINITCP_LOG(INFO) << " current routing table " << std::endl
                              << table << std::endl;
        } else if (std::strcmp(operation, "arp") == 0) {
            // TODO : pretty print ARP table
            MINITCP_LOG(INFO) << " current arp table " << std::endl;
        } else if (std::strcmp(operation, "add") == 0) {
            MINITCP_LOG(INFO) << "adding a new item into routing table";

            char netmask[30] = {};
            ip_t dest_ip, netmask_ip, nexthop_ip;

            scanf("%s %s %s", dest, netmask, src);
            inet_aton(dest, &dest_ip);
            inet_aton(netmask, &netmask_ip);
            inet_aton(src, &nexthop_ip);

            MINITCP_LOG(INFO)
                << "netmask = " << inet_ntoa(netmask_ip) << std::endl;

            network::RoutingTable &table = network::RoutingTable::GetInstance();
            table.Insert(dest_ip, netmask_ip, nexthop_ip, 0, network::kPersist);
        }
        userPrompt();
        std::memset(operation, 0, sizeof(operation));
    }
    return 0;
}