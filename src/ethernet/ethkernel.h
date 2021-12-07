#ifndef MINITCP_SRC_ETHERNET_ETH_KERNEL_H_
#define MINITCP_SRC_ETHERNET_ETH_KERNEL_H_
#include <sys/epoll.h>

#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include "../common/logging.h"
#include "device.h"
#include "device_impl.h"
#include "packetio.h"

namespace minitcp {
namespace ethernet {
struct EthernetBuffer {
  char* buffer;
  std::uint32_t length;
  int device_id;

  EthernetBuffer(char* buf, std::uint32_t len, int id) {
    buffer = buf;
    length = len;
    device_id = id;
  }
};

class EthernetKernel {
 public:
  static EthernetKernel& GetInstance() {
    static std::once_flag ethernet_kernel_flag_;
    std::call_once(ethernet_kernel_flag_,
                   [&]() { ethernet_singleton_ = new EthernetKernel; });
    return *ethernet_singleton_;
  }

  // no copy and no move
  EthernetKernel(const EthernetKernel&) = delete;
  EthernetKernel& operator=(const EthernetKernel&) = delete;
  EthernetKernel(EthernetKernel&&) = delete;
  EthernetKernel& operator=(EthernetKernel&&) = delete;

  int AddDevice(const std::string& device_name);
  int AddAllDevices(const std::string& start_with_prefix);
  int FindDevice(const std::string& device_name);
  class EthernetDevice* GetDevicePointer(int id);
  int SendFrame(const void* buf, int len, int ethtype, const void* destmac,
                int id);
  int BroadcastArp();
  int SetFrameReceiveCallback(frameReceiveCallback);
  int GetDeviceNumber() const { return devices.size(); }
  void Start();

 private:
  EthernetKernel();
  ~EthernetKernel();
  int epoll_fd_;
  bool stop_{false};
  std::vector<class EthernetDevice*> devices;
  frameReceiveCallback kernel_callback_{0};

  static EthernetKernel* ethernet_singleton_;
};
}  // namespace ethernet
}  // namespace minitcp

#endif  //! MINITCP_SRC_ETHERNET_ETH_KERNEL_H_