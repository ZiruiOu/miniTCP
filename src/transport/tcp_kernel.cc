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
                         port_t local_port) {
  return std::make_pair(std::make_pair(remote_ip.s_addr, remote_port),
                        std::make_pair(local_ip.s_addr, local_port));
}

static std::mutex kernel_mutex_;
// TODO : search establish_map first, and then listen_map.
static std::map<connection_key_t, class Socket*> establish_map;
static std::map<std::pair<std::uint32_t, std::uint16_t>, class Socket*>
    listen_map;
static std::map<std::uint32_t, class Socket*> fd_to_socket;

int insertListen(ip_t local_ip, port_t local_port, class Socket* request) {
  std::pair<std::uint32_t, std::uint16_t> key =
      std::make_pair(local_ip.s_addr, local_port);

  std::scoped_lock lock(kernel_mutex_);
  if (listen_map.find(key) != listen_map.end()) {
    MINITCP_LOG(ERROR) << " insert request socket : duplicated insertion."
                       << std::endl;
    return 1;
  }

  listen_map.insert(std::make_pair(key, request));
  return 0;
}

class Socket* findListen(ip_t remote_ip, ip_t local_ip, port_t remote_port,
                         port_t local_port) {
  std::pair<std::uint32_t, std::uint16_t> key =
      std::make_pair(local_ip.s_addr, local_port);

  std::scoped_lock lock(kernel_mutex_);
  auto iter = listen_map.find(key);
  if (iter == listen_map.end()) {
    return nullptr;
  } else {
    return iter->second;
  }
}

int deleteListen(ip_t remote_ip, ip_t local_ip, port_t remote_port,
                 port_t local_port) {
  std::pair<std::uint32_t, std::uint16_t> key =
      std::make_pair(local_ip.s_addr, local_port);

  std::scoped_lock lock(kernel_mutex_);
  auto iter = listen_map.find(key);
  if (iter == listen_map.end()) {
    return 1;
  }

  listen_map.erase(iter);
  return 0;
}

class Socket* findEstablish(ip_t remote_ip, ip_t local_ip, port_t remote_port,
                            port_t local_port) {
  auto key = makeKey(remote_ip, local_ip, remote_port, local_port);

  std::scoped_lock lock(kernel_mutex_);
  auto iter = establish_map.find(key);
  if (iter == establish_map.end()) {
    return nullptr;
  } else {
    return iter->second;
  }
}

int insertEstablish(ip_t remote_ip, ip_t local_ip, port_t remote_port,
                    port_t local_port, class Socket* socket) {
  auto key = makeKey(remote_ip, local_ip, remote_port, local_port);

  std::scoped_lock lock(kernel_mutex_);
  establish_map.insert(std::make_pair(key, socket));
  return 0;
}

class Socket* findSocket(ip_t remote_ip, ip_t local_ip, port_t remote_port,
                         port_t local_port) {
  connection_key_t socket_key =
      makeKey(remote_ip, local_ip, remote_port, local_port);

  std::scoped_lock lock(kernel_mutex_);

  auto established_iter = establish_map.find(socket_key);
  if (established_iter != establish_map.end()) {
    return established_iter->second;
  }

  std::pair<std::uint32_t, std::uint16_t> listen_key =
      std::make_pair(local_ip.s_addr, local_port);

  auto listen_iter = listen_map.find(listen_key);
  if (listen_iter != listen_map.end()) {
    return listen_iter->second;
  }

  return nullptr;
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
  int status = network::sendIPPacket(src_ip, dest_ip, kIpProtoTcp, packet,
                                     sizeof(struct tcphdr) + len);
  delete[] packet;
  return status;
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

  port_t remote_port = ntohs(tcp_header->th_sport);
  port_t local_port = ntohs(tcp_header->th_dport);

  // MINITCP_LOG(DEBUG) << "remote ip = " << inet_ntoa(remote_ip) << std::endl
  //                    << "local ip = " << inet_ntoa(local_ip) << std::endl
  //                    << "remote port = " << remote_port << std::endl
  //                    << "local port = " << local_port << std::endl;

  class Socket* socket =
      findSocket(remote_ip, local_ip, remote_port, local_port);

  if (socket != nullptr) {
    return socket->ReceiveStateProcess(remote_ip, local_ip, tcp_header,
                                       tcp_payload,
                                       length - sizeof(struct tcphdr));
  }

  MINITCP_LOG(ERROR)
      << "tcpReceiveCallback : something wrong upon receiving an ACK."
      << std::endl;

  return 0;
}

}  // namespace transport
}  // namespace minitcp