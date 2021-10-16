#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <memory>

#include <net/ethernet.h>
#include "../ethernet/macaddr.h"
#include "../ethernet/device.h"

using namespace minitcp::ethernet;

int main(int argc, char* argv[]) {
    EthernetDevice device(argv[1]);
    char buffer[] = "Hello World! This is the first message send to you!";
    if (strcmp(argv[2], "send") == 0) {
        device.SendFrame(reinterpret_cast<std::uint8_t*>(buffer), 79, 1520,
                         MacAddress(argv[3]));
    } else {
        device.ReceivePoll();
    }
}