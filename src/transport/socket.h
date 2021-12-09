/**
* @file socket.h
* @brief POSIX-compatible socket library supporting TCP protocol on
IPv4. */
#ifndef MINITCP_SRC_TRANSPORT_SOCKET_H_
#define MINITCP_SRC_TRANSPORT_SOCKET_H_

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef __cplusplus
namespace minitcp {
namespace transport {
extern "C" {
#endif  // ! __cplusplus

// "real" functions for link-time interposition
int __real_socket(int domain, int type, int protocol);
int __real_bind(int socket, const struct sockaddr *address,
                socklen_t address_len);
int __real_listen(int socket, int backlog);
int __real_connect(int socket, const struct sockaddr *address,
                   socklen_t address_len);
int __real_accept(int socket, struct sockaddr *address, socklen_t *address_len);
ssize_t __real_read(int fildes, void *buf, size_t nbyte);
ssize_t __real_write(int fildes, const void *buf, size_t nbyte);
int __real_close(int fildes);
int __real_getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints, struct addrinfo **res);
/**
 * @see [POSIX.1-2017:socket](http://pubs.opengroup.org/onlinepubs/ *
 * 9699919799/functions/socket.html)
 */
int __wrap_socket(int domain, int type, int protocol);
/**
 * @see [POSIX.1-2017:bind](http://pubs.opengroup.org/onlinepubs/ *
 * 9699919799/functions/bind.html)
 */
int __wrap_bind(int socket, const struct sockaddr *address,
                socklen_t address_len);

/**
 * @see [POSIX.1-2017:listen](http://pubs.opengroup.org/onlinepubs/ *
 * 9699919799/functions/listen.html)
 */
int __wrap_listen(int socket, int backlog);
/**
 * @see [POSIX.1-2017:connect](http://pubs.opengroup.org/onlinepubs/ *
 * 9699919799/functions/connect.html)
 */
int __wrap_connect(int socket, const struct sockaddr *address,
                   socklen_t address_len);
/**
 * @see [POSIX.1-2017:accept](http://pubs.opengroup.org/onlinepubs/ *
 * 9699919799/functions/accept.html)
 */
int __wrap_accept(int socket, struct sockaddr *address, socklen_t *address_len);
/**
 * @see [POSIX.1-2017:read](http://pubs.opengroup.org/onlinepubs/ *
 * 9699919799/functions/read.html)
 */
ssize_t __wrap_read(int fildes, void *buf, size_t nbyte);
/**
 * @see [POSIX.1-2017:write](http://pubs.opengroup.org/onlinepubs/ *
 * 9699919799/functions/write.html)
 */
ssize_t __wrap_write(int fildes, const void *buf, size_t nbyte);

/**
 * @see [POSIX.1-2017:close](http://pubs.opengroup.org/onlinepubs/ *
 * 9699919799/functions/close.html)
 */
int __wrap_close(int fildes);

/**
* @see [POSIX.1-2017:getaddrinfo](http://pubs.opengroup.org/
onlinepubs/
* 9699919799/functions/getaddrinfo.html) */
int __wrap_getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints, struct addrinfo **res);
#ifdef __cplusplus
}
}
}
#endif  //! __cplusplus
#endif  //! MINITCP_SRC_TRANSPORT_SOCKET_H_