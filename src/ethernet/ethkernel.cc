#include "ethkernel.h"

#include <unistd.h>

#include "../common/constant.h"

namespace minitcp {
namespace ethernet {
EthernetKernel* EthernetKernel::ethernet_singleton_ = nullptr;

EthernetKernel::EthernetKernel() {
  epoll_fd_ = epoll_create1(0);
  MINITCP_ASSERT(epoll_fd_ != -1)
      << "Ethernet Kernel error: cannot initialize epoll instance."
      << std::endl;
}

EthernetKernel::~EthernetKernel() {
  stop_ = true;
  for (int i = 0; i < devices.size(); i++) {
    delete devices[i];
  }
}

int EthernetKernel::AddDevice(const std::string& device_name) {
  int device_id = devices.size();

  auto new_device = new EthernetDevice(device_name);
  MINITCP_ASSERT(new_device)
      << "Ethernet Kernel: allocate device error" << std::endl;

  devices.push_back(new_device);

  return device_id;
}

int EthernetKernel::AddAllDevices(const std::string& start_with_prefix) {
  pcap_if_t *netif, *netif_devices;

  int status = 0;
  char err_buffer[PCAP_ERRBUF_SIZE];
  status = pcap_findalldevs(&netif_devices, err_buffer);

  MINITCP_ASSERT(status == 0)
      << "EthernetKernel: pcap_findalldevs error " << err_buffer << std::endl;

  for (netif = netif_devices; netif != NULL; netif = netif->next) {
    if (std::strncmp(netif->name, start_with_prefix.c_str(),
                     start_with_prefix.size()) == 0 &&
        (netif->flags & PCAP_IF_UP)) {
      std::string device_name = std::string(netif->name);
      AddDevice(device_name);
    }
  }

  pcap_freealldevs(netif_devices);
  return 0;
}

int EthernetKernel::FindDevice(const std::string& device) {
  for (int i = 0; i < devices.size(); i++) {
    if (devices[i]->GetName() == device) {
      return i;
    }
  }
  MINITCP_LOG(ERROR) << "EthernetKernel FindDevice: device " << device
                     << " not found. " << std::endl;
  return -1;
}

class EthernetDevice* EthernetKernel::GetDevicePointer(int id) {
  if (id >= 0 && id < devices.size()) {
    return devices[id];
  } else {
    return nullptr;
  }
}

int EthernetKernel::SendFrame(const void* buf, std::size_t len, int ethtype,
                              const void* destmac, int id) {
  MINITCP_ASSERT(id >= 0 && id < devices.size())
      << "EthernetKernel Error: device id invalid." << std::endl;

  return devices[id]->SendFrame((std::uint8_t*)(buf), len, ethtype, destmac);
}

int EthernetKernel::BroadcastArp() {
  mac_t broadcast_mac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  ip_t broadcast_ip = {0xffffffff};
  int status = 0;
  for (int i = 0; i < devices.size(); i++) {
    status |=
        devices[i]->SendArp(kArpTypeRequest, &broadcast_mac, &broadcast_ip);
  }
  return status;
}

int EthernetKernel::SetFrameReceiveCallback(frameReceiveCallback callback) {
  kernel_callback_ = callback;
  return 0;
}

void EthernetKernel::Start() {
  std::vector<std::thread> workers;
  for (int i = 0; i < devices.size(); i++) {
    workers.emplace_back([this, i]() {
      int result;
      struct pcap_pkthdr* packet_header;
      const u_char* packet_data;

      auto device_ptr = this->devices[i];
      pcap_t* device_handler = device_ptr->pcap_handler_;

      while (true) {
        if (stop_) {
          return;
        }
        result = pcap_next_ex(device_handler, &packet_header, &packet_data);
        if (packet_data != NULL && result > 0) {
          char* new_data = new char[packet_header->caplen]();
          int new_len = packet_header->caplen;
          std::memcpy(new_data, packet_data, new_len);

          auto new_buffer = std::shared_ptr<struct EthernetBuffer>(
              new EthernetBuffer(new_data, new_len, i),
              [](struct EthernetBuffer* pointer) {
                delete[] pointer->buffer;
                delete pointer;
              });

          kernel_callback_(new_buffer.get()->buffer, new_buffer.get()->length,
                           new_buffer.get()->device_id);
        }
      }
    });
  }

  for (int i = 0; i < workers.size(); i++) {
    workers[i].detach();
  }
}

}  // namespace ethernet
}  // namespace minitcp