#include "ethkernel.h"

#include <unistd.h>

#include "../common/constant.h"

namespace minitcp {
namespace ethernet {
EthernetKernel::EthernetKernel() {
    epoll_fd_ = epoll_create1(0);
    MINITCP_ASSERT(epoll_fd_ != -1)
        << "Ethernet Kernel error: cannot initialize epoll instance."
        << std::endl;

    // register stdin
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = kEpollStdinFd;
    int status = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, 0, &event);
    MINITCP_ASSERT(status != -1)
        << "Ethernet kernel: "
        << " cannot register stdin into epoll instance. " << std::endl;
}

EthernetKernel::~EthernetKernel() {
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

    int pcap_fd = pcap_get_selectable_fd(new_device->pcap_handler_);
    MINITCP_ASSERT(pcap_fd != -1)
        << "Ethernet Kernel: pcap_get_selectable_fd fail. " << std::endl;

    struct epoll_event device_event;
    device_event.events = EPOLLIN | EPOLLET;
    device_event.data.fd = device_id;
    int status = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, pcap_fd, &device_event);
    MINITCP_ASSERT(status != -1)
        << "Ethernet Kernel AddDevice error: cannot register device "
        << device_name << " into epoll instance." << std::endl;

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
                         start_with_prefix.size()) == 0) {
            // MINITCP_LOG(INFO)
            //     << "EthernetKernel: adding device " << netif->name <<
            //     std::endl;
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
    return -1;
}

class EthernetDevice* EthernetKernel::GetDevicePointer(int id) {
    if (id >= 0 && id < devices.size()) {
        return devices[id];
    } else {
        return nullptr;
    }
}

int EthernetKernel::SendFrame(const void* buf, int len, int ethtype,
                              const void* destmac, int id) {
    MINITCP_ASSERT(id >= 0 && id < devices.size())
        << "EthernetKernel Error: device id invalid." << std::endl;

    return devices[id]->SendFrame((std::uint8_t*)(buf), len, ethtype, destmac);
}

int EthernetKernel::SetFrameReceiveCallback(frameReceiveCallback callback) {
    kernel_callback_ = callback;
    return 0;
}

void EthernetKernel::Start() {
    // std::vector<std::thread> worker_threads_;
    // for (int i = 0; i < devices.size(); i++) {
    //     worker_threads_.emplace_back(std::thread([this, i]() {
    //         auto device = this->GetDevicePointer(i);
    //         MINITCP_ASSERT(device) << "Ethernet Kernel: device with id = " <<
    //         i
    //                                << " does not exist. " << std::endl;
    //         MINITCP_LOG(INFO) << "thread " << device->device_name_
    //                           << " starts ." << std::endl;
    //
    //         int result;
    //         struct pcap_pkthdr packet_header;
    //         const u_char* packet_data;

    //        while (true) {
    //            packet_data = pcap_next(device->pcap_handler_,
    //            &packet_header);
    //            if (packet_data != NULL) {
    //                std::cout << device->device_name_ << " Receive "
    //                          << packet_header.caplen << " bytes "
    //                          << packet_data << std::endl;
    //                if (this->kernel_callback_) {
    //                    this->kernel_callback_(packet_data,
    //                                           packet_header.caplen, i);
    //                }
    //            }
    //        }
    //    }));
    //}
    // for (auto& worker : worker_threads_) {
    //    worker.detach();
    //}

    std::thread epoll_worker = std::thread([this]() {
        int result;
        struct pcap_pkthdr packet_header;
        const u_char* packet_data;
        struct epoll_event events[kMaxConcurrentEvents];

        while (true) {
            int num_ready =
                epoll_wait(epoll_fd_, events, kMaxConcurrentEvents, -1);

            for (int i = 0; i < num_ready; i++) {
                int device_id = events[i].data.fd;

                if (device_id == kEpollStdinFd) {
                    std::string device_name, address;
                    std::cin >> device_name >> address;
                    char content[1000];
                    scanf("%[^\n\n\r]", content);
                    content[999] = 0;

                    mac_t dest_address;
                    ether_aton_r(address.c_str(), &dest_address);

                    device_id = FindDevice(device_name);
                    this->SendFrame(content, std::strlen(content), 0x1551,
                                    &dest_address, device_id);

                } else {
                    auto device_ptr = this->devices[device_id];
                    pcap_t* device_handler = device_ptr->pcap_handler_;

                    packet_data = pcap_next(device_handler, &packet_header);
                    if (packet_data != NULL) {
                        if (this->kernel_callback_) {
                            this->kernel_callback_(
                                packet_data, packet_header.caplen, device_id);
                        }
                    }
                }
            }
        }
    });
    epoll_worker.detach();
}

}  // namespace ethernet
}  // namespace minitcp