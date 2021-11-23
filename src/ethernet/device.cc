#include "device.h"

#include "ethkernel.h"

namespace minitcp {
namespace ethernet {
int addDevice(const char* device) {
    class EthernetKernel& kernel = EthernetKernel::GetInstance();
    std::string device_name = std::string(device);
    return kernel.AddDevice(device_name);
}

int findDevice(const char* device) {
    class EthernetKernel& kernel = EthernetKernel::GetInstance();
    std::string device_name = std::string(device);
    return kernel.FindDevice(device_name);
}

int addAllDevices(const char* start_with_prefix) {
    class EthernetKernel& kernel = EthernetKernel::GetInstance();
    std::string prefix = std::string(start_with_prefix);
    return kernel.AddAllDevices(prefix);
}

int broadcastArp() {
    class EthernetKernel& kernel = EthernetKernel::GetInstance();
    return kernel.BroadcastArp();
}

int getDeviceNumber() {
    class EthernetKernel& kernel = EthernetKernel::GetInstance();
    return kernel.GetDeviceNumber();
}

class EthernetDevice* getDevicePointer(int id) {
    class EthernetKernel& kernel = EthernetKernel::GetInstance();
    return kernel.GetDevicePointer(id);
}

ip_t getLocalIP() {
    class EthernetDevice* device = getDevicePointer(0);
    MINITCP_ASSERT(device) << "getLocalIP : no available device. " << std::endl;
    return device->GetIpAddress();
}

void start() {
    class EthernetKernel& kernel = EthernetKernel::GetInstance();
    kernel.Start();
}
}  // namespace ethernet
}  // namespace minitcp