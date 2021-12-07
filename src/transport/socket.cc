#include "socket.h"

#include <unistd.h>

#include <mutex>

#include "callbacks.h"
#include "tcp_kernel.h"

static std::once_flag tcp_kernel_flag;

static inline void start_my_tcp() {
  std::call_once(tcp_kernel_flag, minitcp::transport::startTCPKernel);
}

int minitcp::transport::__wrap_socket(int domain, int type, int protocol) {
  start_my_tcp();

  if (domain != AF_INET && type != SOCK_STREAM && protocol != IPPROTO_TCP) {
    return __real_socket(domain, type, protocol);
  }

  int fd = getNextFd();
  if (fd == -1) {
    MINITCP_LOG(ERROR) << "__wrap_socket : fail to allocate new fd."
                       << std::endl;
    return -1;
  }
  class Socket *socket = new Socket();
  if (!socket) {
    __real_close(fd);
    return -1;
  }
  int result = insertSocketByFd(fd, socket);
  if (result) {
    delete socket;
    __real_close(fd);
    return -1;
  } else {
    return fd;
  }
}

int minitcp::transport::__wrap_bind(int socket, const struct sockaddr *address,
                                    socklen_t address_len) {
  start_my_tcp();
  class Socket *my_socket = findSocketByFd(socket);
  if (!my_socket) {
    return __real_bind(socket, address, address_len);
  }
  // TODO : check local binding.
  return my_socket->Bind(address, address_len);
}

int minitcp::transport::__wrap_listen(int socket, int backlog) {
  start_my_tcp();
  class Socket *my_socket = findSocketByFd(socket);
  if (!socket) {
    return __real_listen(socket, backlog);
  }
  // (1) move the socket to listen socket.
  // int result = insertListen()
  int result = insertListen(my_socket->GetLocalIp(), my_socket->GetLocalPort(),
                            my_socket);
  return my_socket->Listen(backlog);
}

int minitcp::transport::__wrap_connect(int socket,
                                       const struct sockaddr *address,
                                       socklen_t address_len) {
  start_my_tcp();
  class Socket *my_socket = findSocketByFd(socket);
  if (address->sa_family != AF_INET || !my_socket) {
    return __real_connect(socket, address, address_len);
  }
  const struct sockaddr_in *address_in =
      reinterpret_cast<const struct sockaddr_in *>(address);
  ip_t remote_ip = address_in->sin_addr;
  port_t remote_port = ntohs(address_in->sin_port);
  int result = insertEstablish(remote_ip, my_socket->GetLocalIp(), remote_port,
                               my_socket->GetLocalPort(), my_socket);
  return my_socket->Connect(address, address_len);
}

int minitcp::transport::__wrap_accept(int socket, struct sockaddr *address,
                                      socklen_t *address_len) {
  start_my_tcp();

  class Socket *my_socket = findSocketByFd(socket);

  if (!my_socket) {
    return __real_accept(socket, address, address_len);
  }

  class Socket *child_socket = my_socket->Accept(address, address_len);

  int child_fd = getNextFd();

  insertSocketByFd(child_fd, child_socket);

  return child_fd;
}

ssize_t minitcp::transport::__wrap_read(int fildes, void *buf, size_t nbyte) {
  start_my_tcp();

  class Socket *socket = findSocketByFd(fildes);

  if (!socket) {
    return __real_read(fildes, buf, nbyte);
  }

  return socket->Read(buf, nbyte);
}

ssize_t minitcp::transport::__wrap_write(int fildes, const void *buf,
                                         size_t nbyte) {
  start_my_tcp();

  class Socket *socket = findSocketByFd(fildes);

  if (!socket) {
    return __real_write(fildes, buf, nbyte);
  }

  return socket->Write(buf, nbyte);
}

int minitcp::transport::__wrap_close(int fildes) {
  start_my_tcp();

  class Socket *socket = findSocketByFd(fildes);

  if (!socket) {
    return __real_close(fildes);
  }

  int status = socket->Close();

  if (!status) {
    cleanupSocket(fildes);
    __real_close(fildes);
    delete socket;

    return 0;
  } else {
    return status;
  }
}

int minitcp::transport::__wrap_getaddrinfo(const char *node,
                                           const char *service,
                                           const struct addrinfo *hints,
                                           struct addrinfo **res) {
  start_my_tcp();
  // class Socket *socket = findSocketByFd(fildes);
  __real_getaddrinfo(node, service, hints, res);
}