#include "packetio.h"
#include "ethkernel.h"

namespace minitcp {
namespace ethernet {

extern class EthernetKernel ethernet_kernel_;

int sendFrame(const void* buf, int len, int ethtype, const void* destmac,
              int id) {
    return ethernet_kernel_.SendFrame(buf, len, ethtype, destmac, id);
}

int setFrameReceiveCallback(frameReceiveCallback callback) {
    return ethernet_kernel_.SetFrameReceiveCallback(callback);
}

}  // namespace ethernet
}  // namespace minitcp