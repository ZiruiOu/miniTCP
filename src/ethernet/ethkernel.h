#ifndef MINITCP_SRC_ETHERNET_ETH_KERNEL_H_
#define MINITCP_SRC_ETHERNET_ETH_KERNEL_H_
#include <sys/epoll.h>

#include <memory>
#include <thread>
#include <vector>

#include "../common/logging.h"
#include "device.h"
#include "device_impl.h"
#include "packetio.h"

namespace minitcp {
namespace ethernet {
class EthernetKernel {
   public:
    EthernetKernel();
    ~EthernetKernel();

    // no copy and no move
    EthernetKernel(const EthernetKernel&) = delete;
    EthernetKernel& operator=(const EthernetKernel&) = delete;
    EthernetKernel(EthernetKernel&&) = delete;
    EthernetKernel& operator=(EthernetKernel&&) = delete;

    int AddDevice(std::string& device_name);
    int FindDevice(std::string& device_name);
    class EthernetDevice* GetDevicePointer(int id);
    int SendFrame(const void* buf, int len, int ethtype, const void* destmac,
                  int id);
    int SetFrameReceiveCallback(frameReceiveCallback);
    int GetDeviceNumber() const { return devices.size(); }
    void Start();

   private:
    int epoll_fd_;
    std::vector<class EthernetDevice*> devices;
    frameReceiveCallback kernel_callback_{0};
};
}  // namespace ethernet
}  // namespace minitcp

#endif  //! MINITCP_SRC_ETHERNET_ETH_KERNEL_H_