#include "socket.h"

#include "../common/logging.h"
#include "tcp_kernel.h"

namespace minitcp {
namespace transport {

int RequestSocket::SendSynAck() {
    return sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_,
                         tx_init_seq_number_, rx_init_seq_number_ + 1,
                         TH_SYN | TH_ACK, 4096, NULL, 0);
}

int RequestSocket::SynAckRetransmit() {
    int result = SendSynAck();
    if (!result) {
        retransmission_count_++;
    }
    if (retransmission_count_ <= 10) {
        setTimerAfter(1000, retransmit_timer_);
    } else {
        // TODO : delete it from the listen_queue_ and global hash table.

        // TODO : delete timer.
        delete retransmit_timer_;
    }
    return result;
}

// FIXME : this function should only be called once.
void RequestSocket::SetUpRetransmitTimer() {
    retransmit_timer_ = new TimerHandler(/* is_persist */ = true);
    std::function<void()> retransmit_wrapper_ = [this]() {
        this->SynAckRetransmit();
    };
    retransmit_timer_->RegisterCallback(std::move(retransmit_wrapper_));
    setTimerAfter(5000, retransmit_timer_);
}

void RequestSocket::CancellTimer() {
    if (retransmission_count_ <= 10) {
        cancellTimer(retransmit_timer_);
    }
}

int RequestSocket::ReceiveAck() {
    if (!listen_socket_) {
        MINITCP_LOG(ERROR)
            << "RequestSocket.ReceiveAck(): listen_socket_ is null."
            << std::endl;
        return -1;
    }
    listen_socket_->RemoveFromListenQueue(this->iter);
    listen_socket_->AddToAcceptQueue(this);
}

int Socket::Bind(struct sockaddr* address, socklen_t* address_len) {
    if (address->sa_family != AF_INET) {
        MINITCP_ASSERT(ERROR)
            << " socket.Bind() : fail to bind an socket address which is not "
               "AF_INET."
            << std::endl;
        return -1;
    }

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

    // TODO : remove from the request_map and release request
    return request.GetSocket();
}

// block or timeout
int Socket::Connect() {}

// ShutDown
int Socket::Close() { std::scoped_lock lock(socket_mutex_); }

}  // namespace transport
}  // namespace minitcp