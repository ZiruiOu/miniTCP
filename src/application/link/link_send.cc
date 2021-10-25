#include <arpa/inet.h>
#include <net/ethernet.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include "../common/constant.h"
#include "../common/ipaddr.h"
#include "../common/logging.h"
#include "../common/macaddr.h"
#include "../ethernet/device.h"
#include "../ethernet/device_impl.h"
#include "../ethernet/packetio.h"

using namespace minitcp;

int LinkCallback(const void *buffer, int length, int device_id) {
    mac_t *dest_mac = (struct ether_addr *)buffer;
    mac_t *src_mac = (struct ether_addr *)(buffer + 6);
    std::uint16_t ethernet_type = ntohs(*(std::uint16_t *)(buffer + 12));
    char *packet_content = (char *)(buffer + 14);
    auto device_ptr = ethernet::getDevicePointer(device_id);
    MINITCP_ASSERT(device_ptr) << "getDevicePointer error" << std::endl;
    std::cout << "device " << device_ptr->GetName() << " receive " << length
              << " bytes, and the message is " << packet_content << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    ethernet::setFrameReceiveCallback(LinkCallback);

    int fd = ethernet::addDevice(argv[1]);

    MINITCP_LOG(DEBUG) << "ethernet kernel register " << argv[1]
                       << " with device id " << fd << std::endl;

    char buffer[60] = {};
    sprintf(buffer, "Hello World! This is greeting from %s", argv[1]);

    mac_t dest_mac;
    MINITCP_ASSERT(ether_aton_r(argv[2], &dest_mac))
        << "ether_aton_r error: parse address " << argv[2] << " fail."
        << std::endl;

    ethernet::sendFrame(buffer, std::strlen(buffer), 0x5555 /* just for test */,
                        (const void *)&dest_mac, fd);

    return 0;
}