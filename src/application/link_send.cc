#include <arpa/inet.h>
#include <net/ethernet.h>

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>

#include "../common/constant.h"
#include "../common/ipaddr.h"
#include "../common/logging.h"
#include "../common/macaddr.h"
#include "../ethernet/device.h"
#include "../ethernet/packetio.h"

using namespace minitcp;

int LinkCallback(const void *buffer, int length, int device_id) {
    struct ether_addr *dest_mac = (struct ether_addr *)buffer;
    struct ether_addr *src_mac = (struct ether_addr *)(buffer + 6);
    std::uint16_t ethernet_type = ntohs(*(std::uint16_t *)(buffer + 12));
    char *packet_content = (char *)(buffer + 14);
    std::cout << "device " << device_id << " receive " << length
              << " bytes, and the message is " << packet_content << std::endl;
}

int main(int argc, char *argv[]) {
    ethernet::setFrameReceiveCallback(LinkCallback);
    std::cout << sizeof(class MacAddress) << " " << sizeof(mac_t) << std::endl;
    std::cout << sizeof(class IpAddress) << " " << sizeof(ip_t) << std::endl;
    if (strcmp(argv[1], "send") == 0) {
        int fd = ethernet::addDevice(argv[2]);

        MINITCP_LOG(DEBUG) << "ethernet kernel register" << argv[2]
                           << " with device id " << fd << std::endl;

        char buffer[60] = {};
        sprintf(buffer, "Hello World! Greetings from %s", argv[2]);

        mac_t dest_mac;
        MINITCP_ASSERT(ether_aton_r(argv[3], &dest_mac))
            << "ether_aton_r error: parse address " << argv[3] << " fail."
            << std::endl;

        ethernet::sendFrame(buffer, std::strlen(buffer),
                            0x5555 /* just for test */, (const void *)&dest_mac,
                            fd);
    } else {
        int fd1 = ethernet::addDevice("veth3-0");
        int fd2 = ethernet::addDevice("veth3-2");
        int fd3 = ethernet::addDevice("veth3-4");

        ethernet::start();
        while (1)
            ;
    }
}