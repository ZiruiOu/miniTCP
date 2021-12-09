#include "packetio.h"

#include "ethkernel.h"

namespace minitcp {
namespace ethernet {

extern class EthernetKernel ethernet_kernel_;

int sendFrame(const void* buf, std::size_t len, int ethtype,
              const void* destmac, int id) {
  class EthernetKernel& kernel = EthernetKernel::GetInstance();
  return kernel.SendFrame(buf, len, ethtype, destmac, id);
}

int setFrameReceiveCallback(frameReceiveCallback callback) {
  class EthernetKernel& kernel = EthernetKernel::GetInstance();
  return kernel.SetFrameReceiveCallback(callback);
}

}  // namespace ethernet
}  // namespace minitcp