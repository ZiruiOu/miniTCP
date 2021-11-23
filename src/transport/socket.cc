#include "socket.h"

#include "../common/logging.h"
#include "tcp_kernel.h"

namespace minitcp {
namespace transport {

int RequestSocket::ReceiveStateProcess(struct tcphdr* tcp_header) {
    if (tcp_header->th_flags == TH_SYN) {
        // duplicated SYN.
        return 1;
    } else if (tcp_header->th_flags == TH_ACK) {
        if (tx_init_seq_num_ + 1 == ntohl(tcp_header->th_ack)) {
            // accept.
            ReceiveAck();
            return 0;
        }
    } else if (tcp_header->th_flags == TH_RST) {
        // reset request socket.
        // remove this request socket by calling clean up function.
        // CleanUp();
        return 0;
    }
}

int RequestSocket::SendSynAck() {
    MINITCP_LOG(INFO) << "Syn Ack packet : seq = " << tx_init_seq_num_
                      << std::endl
                      << "ack = " << rx_init_seq_num_ + 1 << std::endl;
    return sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_,
                         tx_init_seq_num_, rx_init_seq_num_ + 1,
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
        retransmit_timer_ = nullptr;
    }
    return result;
}

// FIXME : this function should only be called once.
void RequestSocket::SetUpRetransmitTimer() {
    if (retransmit_timer_) {
        CancellTimer();
    }
    retransmit_timer_ = new TimerHandler(/* is_persist = */ true);
    std::function<void()> retransmit_wrapper_ = [this]() {
        this->SynAckRetransmit();
    };
    retransmit_timer_->RegisterCallback(std::move(retransmit_wrapper_));
    setTimerAfter(5000, retransmit_timer_);
}

void RequestSocket::CancellTimer() {
    if (retransmission_count_ <= 10) {
        cancellTimer(retransmit_timer_);
        retransmission_count_ = 0;
        retransmit_timer_ = nullptr;
    }
}

int RequestSocket::ReceiveAck() {
    if (!listen_socket_) {
        MINITCP_LOG(ERROR)
            << "RequestSocket.ReceiveAck(): listen_socket_ is null."
            << std::endl;
        return -1;
    }

    socket_ = new Socket(dest_ip_, src_ip_, dest_port_, src_port_,
                         tx_init_seq_num_ + 1, rx_init_seq_num_);
    socket_->SetState(ConnectionState::kEstablished);

    auto key = makeKey(dest_ip_, src_ip_, dest_port_, src_port_,
                       /* is_listen */ = false);

    insertSocket(key, socket_);

    // set up retransmission timer for new socket.
    listen_socket_->MoveToAcceptQueue(this);
}

int Socket::ReceiveRequest(ip_t remote_ip, ip_t local_ip,
                           struct tcphdr* tcp_header) {
    // TODO
    // (1) create new request socket.
    // (2) see if the backlog_ is full.
    // (3) insert this new request socket into my listen_queue_.
    // (4) insert this new request socket into kernel.
    // (5) send SYNACK and setup timer for this request socket.
    if (backlog_count_ >= max_backlog_) {
        return 1;
    }

    port_t remote_port = ntohs(tcp_header->th_sport);
    port_t local_port = ntohs(tcp_header->th_dport);
    class RequestSocket* request =
        new RequestSocket(remote_ip, local_ip, remote_port, local_port,
                          ntohl(tcp_header->th_seq));

    listen_queue_.push_front(request);
    request->SetLink(listen_queue_.begin());

    connection_key_t key = makeKey(remote_ip, local_ip, remote_port, local_port,
                                   /*is_listen = */ false);
    insertRequest(key, request);

    request->SendSynAck();

    return 0;
}

int Socket::ReceiveConnection(struct tcphdr* tcp_header) {
    ack_num_ = ntohl(tcp_header->th_seq);

    // TODO : implement a function sendPacketInternal to
    // (1) send TCP packet.
    // (2) maintain seq_num_ and next_seq_num_.
    // (3) setup new timer.
    sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_, next_seq_num_,
                  ack_num_, TH_ACK, 0, NULL, 0);
    seq_num_ = next_seq_num_;
    next_seq_num_ = seq_num_ + 1;

    state_ = ConnectionState::kEstablished;
    socket_cv_.notify_one();
}

int Socket::ReceiveStateProcess(ip_t remote_ip, ip_t local_ip,
                                struct tcphdr* tcp_header, const void* buffer,
                                int length) {
    std::scoped_lock lock(socket_mutex_);

    int status = 0;
    switch (SocketBase::state_) {
        case ConnectionState::kListen:
            if (tcp_header->th_flags == TH_SYN) {
                status = ReceiveRequest(remote_ip, local_ip, tcp_header);
            }
            break;
        case ConnectionState::kSynSent:
            if (tcp_header->th_flags == TH_SYN | TH_ACK) {
                if (ntohl(tcp_header->th_ack) == next_seq_num_) {
                    status = ReceiveConnection(tcp_header);
                } else {
                    // reset remote
                    // (1) send RESET
                    // (2) setup timer.
                    sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_,
                                  ntohl(tcp_header->th_ack), ack_num_, TH_RST,
                                  0, NULL, 0);
                }
            }
            break;
        case ConnectionState::kEstablished:
            // may piggyback some data.
            // so you need to process some data.
            if (tcp_header->th_flags == TH_ACK) {
                // simple go back-n scheme.
            }
        default:
            MINITCP_LOG(ERROR)
                << "socket ReceiveStateProcess: state not implemented."
                << std::endl;
            status = 1;
            break;
    }
    return status;
}

int Socket::Bind(struct sockaddr* address, socklen_t address_len) {
    if (address->sa_family != AF_INET) {
        MINITCP_LOG(ERROR)
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
    max_backlog_ = 2;
    SocketBase::SetState(ConnectionState::kListen);
    return 0;
}

class RequestSocket* Socket::Accept() {
    std::unique_lock<std::mutex> lock(socket_mutex_);

    // block until accept_queue_ is not empty
    socket_cv_.wait(lock, [this]() { return !this->accept_queue_.empty(); });

    class RequestSocket* request = PopAcceptQueue();
    // class Socket* new_socket = request->GetSocket();
    //  request->CleanUp();
    return request;
}

// block or timeout
int Socket::Connect(struct sockaddr* address, socklen_t address_len) {
    std::unique_lock<std::mutex> lock(socket_mutex_);

    auto address_in = reinterpret_cast<struct sockaddr_in*>(address);

    dest_ip_ = address_in->sin_addr;
    dest_port_ = address_in->sin_port;

    // send SYN datagram.
    int status = sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_,
                               seq_num_, 0, TH_SYN, 4096, NULL, 0);

    // TODO : setup retransmission timer.

    state_ = ConnectionState::kSynSent;

    if (status) {
        return status;
    }

    socket_cv_.wait(lock, [this]() {
        return this->state_ == ConnectionState::kEstablished;
    });

    return 0;
}

// ShutDown
int Socket::Close() { std::scoped_lock lock(socket_mutex_); }

}  // namespace transport
}  // namespace minitcp