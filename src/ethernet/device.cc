#include "device.h"

#include "ethkernel.h"

namespace minitcp {
namespace ethernet {
class EthernetKernel ethernet_kernel_;

int addDevice(const char* device) {
    std::string device_name = std::string(device);
    return ethernet_kernel_.AddDevice(device_name);
}

int findDevice(const char* device) {
    std::string device_name = std::string(device);
    return ethernet_kernel_.FindDevice(device_name);
}

int addAllDevices(const char* start_with_prefix) {
    std::string prefix = std::string(start_with_prefix);
    return ethernet_kernel_.AddAllDevices(prefix);
}

class EthernetDevice* getDevicePointer(int id) {
    return ethernet_kernel_.GetDevicePointer(id);
}

void start() { ethernet_kernel_.Start(); }
}  // namespace ethernet
}  // namespace minitcp