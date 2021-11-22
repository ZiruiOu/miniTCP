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

static std::map<connection_key_t, class RequestSocket*> request_map;
static std::map<connection_key_t, class Socket*> socket_map;
static std::map<std::uint32_t, class Socket*> fd_to_socket;

int getNextFd() {
    int fd = open("/dev/null", O_RDONLY);
    return fd;
}

// TODO : implement right function
static std::uint16_t calculateTCPCheckSum() { return 0; }

// NOTE : assume that src_port and dest_port are int big endian
// FIXME : ensure that len <= kTCPMss
std::uint8_t* createTCPPacket(port_t src_port, port_t dest_port,
                              std::uint32_t sequence, std::uint32_t ack,
                              std::uint8_t flags, std::uint16_t window,
                              const void* buffer, int len) {
    std::uint8_t* tcp_packet = new std::uint8_t[sizeof(struct tcphdr) + len]();
    auto tcp_header = reinterpret_cast<struct tcphdr*>(tcp_packet);
    tcp_header->th_sport = src_port;
    tcp_header->th_dport = dest_port;

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

}  // namespace transport
}  // namespace minitcp