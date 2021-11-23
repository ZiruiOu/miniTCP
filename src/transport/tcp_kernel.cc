#include "tcp_kernel.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstring>
#include <map>
#include <utility>

#include "../common/constant.h"
#include "../common/logging.h"
#include "../network/ip.h"

namespace minitcp {
namespace transport {

// NOTE : both remote_port and local_port should be in big endian.
connection_key_t makeKey(ip_t remote_ip, ip_t local_ip, port_t remote_port,
                         port_t local_port, bool is_listen) {
    if (!is_listen) {
        return std::make_pair(std::make_pair(remote_ip.s_addr, remote_port),
                              std::make_pair(local_ip.s_addr, local_port));
    } else {
        return std::make_pair(std::make_pair(0, 0),
                              std::make_pair(local_ip.s_addr, local_port));
    }
}

static std::mutex kernel_mutex_;
static std::map<connection_key_t, class RequestSocket*> request_map;
static std::map<connection_key_t, class Socket*> socket_map;
static std::map<std::uint32_t, class Socket*> fd_to_socket;

int insertRequest(const connection_key_t& key, class RequestSocket* request) {
    std::scoped_lock lock(kernel_mutex_);
    if (request_map.find(key) != request_map.end()) {
        MINITCP_LOG(ERROR) << " insert request socket : duplicated insertion."
                           << std::endl;
        return 1;
    }
    request_map.insert(std::make_pair(key, request));
    return 0;
}

static class RequestSocket* findRequest(const connection_key_t& key) {
    std::scoped_lock lock(kernel_mutex_);
    auto iter = request_map.find(key);
    if (iter == request_map.end()) {
        return nullptr;
    } else {
        return iter->second;
    }
}

static int deleteRequest(const connection_key_t& key) {
    std::scoped_lock lock(kernel_mutex_);
    auto iter = request_map.find(key);
    if (iter == request_map.end()) {
        return 1;
    }
    request_map.erase(iter);
    return 0;
}

class Socket* findSocket(const connection_key_t& key) {
    std::scoped_lock lock(kernel_mutex_);
    auto iter = socket_map.find(key);
    if (iter == socket_map.end()) {
        return nullptr;
    } else {
        return iter->second;
    }
}

int insertSocket(const connection_key_t& key, class Socket* socket) {
    std::scoped_lock lock(kernel_mutex_);
    socket_map.insert(std::make_pair(key, socket));
    return 0;
}

int getNextFd() {
    int fd = open("/dev/null", O_RDONLY);
    return fd;
}

// TODO : implement right function
static std::uint16_t calculateTCPCheckSum() { return 0; }

// FIXME : ensure that len <= kTCPMss
std::uint8_t* createTCPPacket(port_t src_port, port_t dest_port,
                              std::uint32_t sequence, std::uint32_t ack,
                              std::uint8_t flags, std::uint16_t window,
                              const void* buffer, int len) {
    std::uint8_t* tcp_packet = new std::uint8_t[sizeof(struct tcphdr) + len]();
    auto tcp_header = reinterpret_cast<struct tcphdr*>(tcp_packet);
    tcp_header->th_sport = htons(src_port);
    tcp_header->th_dport = htons(dest_port);

    tcp_header->th_seq = htonl(sequence);
    tcp_header->th_ack = htonl(ack);

    tcp_header->th_off = 0x05;
    tcp_header->th_flags = flags;

    // TODO :  add support for window
    tcp_header->th_win = htons(4096);
    // TODO :  add checksum.
    tcp_header->th_sum = 0;
    tcp_header->th_urp = 0;

    if (buffer) {
        std::memcpy(tcp_packet + sizeof(*tcp_header), buffer, len);
    }

    return tcp_packet;
}

int sendTCPPacket(ip_t src_ip, ip_t dest_ip, port_t src_port, port_t dest_port,
                  std::uint32_t sequence, std::uint32_t ack, std::uint8_t flags,
                  std::uint16_t window, const void* buffer, int len) {
    if (len > kTCPMss) {
        MINITCP_LOG(ERROR)
            << "sendTCPPacket : size of the payload is larger than a MSS."
            << std::endl;
        // TODO : set errno.
        return -1;
    }
    std::uint8_t* packet = createTCPPacket(src_port, dest_port, sequence, ack,
                                           flags, window, buffer, len);
    return network::sendIPPacket(src_ip, dest_ip, kIpProtoTcp, packet,
                                 sizeof(struct tcphdr) + len);
}

int tcpReceiveCallback(const struct ip* ip_header, const void* buffer,
                       int length) {
    ip_t remote_ip = ip_header->ip_src;
    ip_t local_ip = ip_header->ip_dst;

    // leave other ip options alone
    // TODO : add other ip options into consideration.

    struct tcphdr* tcp_header =
        reinterpret_cast<struct tcphdr*>(const_cast<void*>(buffer));

    const std::uint8_t* tcp_payload =
        reinterpret_cast<const std::uint8_t*>(tcp_header + 1);

    port_t remote_port = htons(tcp_header->th_sport);
    port_t local_port = htons(tcp_header->th_dport);

    MINITCP_LOG(DEBUG) << "remote ip = " << inet_ntoa(remote_ip) << std::endl
                       << "local ip = " << inet_ntoa(local_ip) << std::endl
                       << "remote port = " << remote_port << std::endl
                       << "local port = " << local_port << std::endl;

    if (tcp_header->th_flags == TH_SYN) {
        // A SYN packet

        // 1. an active socket (remote socket) send SYN to active socket (local
        // socket).
        // 2. NOTIMPLEMENTED: two passive socket connect simutenaously.
        // 3. an active socket send duplicated SYN to passive socket (local
        // socket).

        // try to find local request socket.
        auto request_key = makeKey(remote_ip, local_ip, remote_port, local_port,
                                   /* is_listen= */ false);
        class RequestSocket* request_socket = findRequest(request_key);
        if (request_socket != nullptr) {
            return request_socket->ReceiveStateProcess(tcp_header);
        }

        // try to find local listen socket.
        auto socket_key = makeKey(remote_ip, local_ip, remote_port, local_port,
                                  /* is_listen = */ true);
        class Socket* listen_socket = findSocket(socket_key);
        if (listen_socket != nullptr) {
            // exists.
            return listen_socket->ReceiveStateProcess(
                remote_ip, local_ip, tcp_header, buffer, length);
        }

        MINITCP_LOG(ERROR) << "tcpReceivecallback: no such listen_socket."
                           << std::endl;
        // otherwise local had been crashed.
        // drop
        return 1;
    } else if (tcp_header->th_flags == TH_SYN | TH_ACK) {
        MINITCP_LOG(DEBUG) << " a SYNACK packet received " << std::endl;

        auto key = makeKey(remote_ip, local_ip, remote_port, local_port,
                           /* is_listen= */ false);

        class Socket* socket = findSocket(key);
        if (socket != nullptr) {
            return socket->ReceiveStateProcess(remote_ip, local_ip, tcp_header,
                                               buffer, length);
        }

        MINITCP_LOG(ERROR)
            << "tcpReceiveCallback : something wrong upon receiving a SYNACK."
            << std::endl;

    } else if (tcp_header->th_flags == TH_ACK) {
        auto key = makeKey(remote_ip, local_ip, remote_port, local_port,
                           /* is_listen= */ false);
        class Socket* socket = findSocket(key);
        if (socket != nullptr) {
            return socket->ReceiveStateProcess(remote_ip, local_ip, tcp_header,
                                               buffer, length);
        }

        MINITCP_LOG(ERROR)
            << "tcpReceiveCallback : something wrong upon receiving an ACK."
            << std::endl;
    }

    return 0;
}

}  // namespace transport
}  // namespace minitcp