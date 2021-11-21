#include "tcp_kernel.h"

#include <map>
#include <utility>

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

// fast lookup for callback function
static std::map<connection_key_t, class RequestSocket*> request_map;
// listen socket : remote_ip == 0.0.0.0 && remote_port = 0
// connection socket
static std::map<connection_key_t, class Socket*> socket_map;
// fast lookup for file descrptor
static std::map<std::uint32_t, class Socket*> fd_to_socket;

}  // namespace transport
}  // namespace minitcp