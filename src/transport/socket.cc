#include "socket.h"

#include "../common/logging.h"

namespace minitcp {
namespace transport {

int Socket::Bind(struct sockaddr* address, socklen_t* address_len) {
    // FIXME : set errno in stead of ASSERTION.
    MINITC_ASSERT(address->sa_family == AF_INET)
        << " socket.Bind : fail to bind an socket address which is not AF_INET."
        << std::endl;

    auto address_in = reinterpret_cast<struct sockaddr_in*>(address);
    src_ip_ = address_in->sin_addr;
    src_port_ = address_in->sin_port;

    return 0;
}

int Socket::Listen(int backlog) {
    // TODO : backlog
    max_backlog_ = 1;
    SocketBase::SetState(ConnectionState::kListen);
    return 0;
}

class RequestSocket* Socket::Accept() {
    std::unique_lock<std::mutex> lock(socket_mutex_);

    // FIXME : add timeout

    // block until accept_queue_ is not empty
    socket_cv_->wait(lock, [this]() { return !this->accept_queue_.empty(); });

    class RequestSocket* request = PopAcceptQueue();

    // TODO : remove from the request_map
    // TODO : release request
    return request.GetSocket();
}

// block or timeout
int Socket::Connect() {}

// ShutDown
int Socket::Close() {}

}  // namespace transport
}  // namespace minitcp