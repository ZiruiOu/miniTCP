#ifndef MINITCP_SRC_TRANSPORT_SOCKET_H_
#define MINITCP_SRC_TRANSPORT_SOCKET_H_

#include <stdlib.h>

#include <condition_variable>
#include <list>
#include <mutex>

#include "../common/timer.h"
#include "../common/timer_impl.h"
#include "../common/types.h"
#include "../ethernet/device.h"
#include "channel.h"

namespace minitcp {
namespace transport {
using listnode_t = std::list<class RequestSocket*>::iterator;

class Socket;
class RequestSocket;

enum class ConnectionState {
    kClosed,
    kListen,
    kSynSent,
    kSynReceived,
    kEstablished,
    kFinWait1,
    kFinWait2,
    kClosing,
    kLastAck,
    kTimeWait,
};

struct SocketBuffer {
    std::size_t length;
    char* buffer;

    ip_t dest_ip;
    ip_t src_ip;
    port_t dest_port;
    port_t src_port;
};

// Assumption : the dest_port and src_port should be in big endian.
class SocketBase {
   public:
    SocketBase() : state_(ConnectionState::kClosed) {}
    // TODO : sensure network endian for dest_port and src_port
    SocketBase(ip_t dest_ip, ip_t src_ip, port_t dest_port, port_t src_port)
        : dest_ip_(dest_ip),
          src_ip_(src_ip),
          dest_port_(dest_port),
          src_port_(src_port),
          state_(ConnectionState::kClosed) {}
    ~SocketBase() = default;

    void SetState(enum ConnectionState new_state) {
        // TODO : implement the state machine transition.
        state_ = new_state;
    }

    const enum ConnectionState GetState() const { return state_; }

    virtual bool IsValid() const = 0;

   protected:
    ip_t dest_ip_;
    ip_t src_ip_;
    port_t dest_port_;
    port_t src_port_;

    enum ConnectionState state_;
};

class RequestSocket : public SocketBase {
   public:
    RequestSocket(ip_t dest_ip, ip_t src_ip, port_t dest_port, port_t src_port,
                  std::uint32_t rx_init_seq_num)
        : SocketBase(dest_ip, src_ip, dest_port, src_port) {
        SocketBase::SetState(ConnectionState::kSynReceived);
        // FIXME : not a good idea to use rand() ?
        tx_init_seq_num_ = rand();
        rx_init_seq_num_ = rx_init_seq_num;
    }
    ~RequestSocket() = default;

    bool IsValid() const override {
        // TODO : check state
        return true;
    }

    int ReceiveStateProcess(struct tcphdr* tcp_header);

    // state transition:
    // kSynReceived -> kEstablished
    // Actions : SendSynAck, SyncAck Retransmission, Receive Ack.

    int SendSynAck();
    int SynAckRetransmit();
    int ReceiveAck();
    void SetUpRetransmitTimer();
    void CancellTimer();

    std::list<class RequestSocket*>::iterator GetLink() { return link_; }

    void SetLink(std::list<class RequestSocket*>::iterator iter) {
        link_ = iter;
    }

    class Socket* GetSocket() {
        return socket_;
    }

    void SetSocket(class Socket* socket) { socket_ = socket; }

   private:
    // the connection socket established by this RequestSocket.
    class Socket* socket_{nullptr};
    // the pointer of the listen socket
    class Socket* listen_socket_{nullptr};
    // the iterator for listen_queue_ or accept_queue_
    std::list<class RequestSocket*>::iterator link_;

    // local initial sequence number
    std::uint32_t tx_init_seq_num_;
    // remote initial sequence number
    std::uint32_t rx_init_seq_num_;

    // the syn ack retransmission timer.
    handler_t retransmit_timer_;
    int retransmission_count_{0};
};

class Socket : public SocketBase {
   public:
    // TODO : arbitary bind a local ip and local port.
    Socket() : SocketBase(ip_t{0}, ip_t{ethernet::getLocalIP()}, 0, 0) {
        // FIXME : not a good idea to use rand() ?
        seq_num_ = rand();
        ack_num_ = 0;
        next_seq_num_ = seq_num_ + 1;
    }

    // FIXME : only estiblished socket can use.
    Socket(ip_t dest_ip, ip_t src_ip, port_t dest_port, port_t src_port,
           std::uint32_t seq_num, std::uint32_t ack_num)
        : SocketBase(dest_ip, src_ip, dest_port, src_port),
          seq_num_(seq_num),
          ack_num_(ack_num) {
        // TODO : add into constant.
        // TODO : use link list instead of fix size sender queue.
        // TODO : receiver would better use ring buffer.
        // send_queue_(5000);
        // recv_queue_.reset(5000);
        next_seq_num_ = seq_num_ + 1;
    }
    ~Socket() = default;

    Socket& operator=(const Socket&) = delete;
    Socket(const Socket&) = delete;

    Socket&& operator=(Socket&&) = delete;
    Socket(Socket&&) = delete;

    bool IsValid() const override {
        // TODO : check state
        return true;
    }

    int ReceiveRequest(ip_t remote_ip, ip_t local_ip,
                       struct tcphdr* tcp_header);
    int ReceiveConnection(struct tcphdr* tcp_header);
    int ReceiveStateProcess(ip_t remote_ip, ip_t local_ip,
                            struct tcphdr* tcp_header, const void* buffer,
                            int length);

    bool AddToListenQueue(class RequestSocket* request) {
        std::scoped_lock lock(socket_mutex_);
        if (backlog_count_ >= max_backlog_) {
            return false;
        }
        backlog_count_++;
        listen_queue_.push_front(request);
        request->SetLink(listen_queue_.begin());
        return true;
    }

    void RemoveFromListenQueue(std::list<class RequestSocket*>::iterator iter) {
        std::scoped_lock lock(socket_mutex_);
        backlog_count_--;
        listen_queue_.erase(iter);
    }

    void MoveToAcceptQueue(class RequestSocket* request) {
        std::scoped_lock lock(socket_mutex_);
        listen_queue_.erase(request->GetLink());
        accept_queue_.push_front(request);
        request->SetLink(accept_queue_.begin());
        socket_cv_.notify_one();
    }

    void AddToAcceptQueue(class RequestSocket* request) {
        std::scoped_lock lock(socket_mutex_);
        accept_queue_.push_front(request);
        request->SetLink(accept_queue_.begin());
        socket_cv_.notify_one();
    }

    class RequestSocket* PopAcceptQueue() {
        std::scoped_lock lock(socket_mutex_);
        class RequestSocket* request = accept_queue_.front();
        backlog_count_--;
        accept_queue_.pop_front();

        request->CancellTimer();
        return request;
    }

    // Socket Actions: send and receive
    int Bind(struct sockaddr* address, socklen_t address_len);
    int Listen(int backlog);
    class RequestSocket* Accept();
    int Connect(struct sockaddr* address, socklen_t address_len);
    int Close();

   private:
    /* TODO : RTT estimation */
    /* TODO : congestion control */
    /* TODO : flow control */

    // socket mutex
    std::mutex socket_mutex_;
    // socket condition variable
    std::condition_variable socket_cv_;

    // The socket backlog, i.e. the maximum of listen_queue_.length +
    // accept_queue_.length
    int max_backlog_{1};
    int backlog_count_{0};

    // Request Socket with state = SYN_RECV
    std::list<class RequestSocket*> listen_queue_;
    // Request Socket with state = ESTABLISH.
    std::list<class RequestSocket*> accept_queue_;

    std::uint32_t seq_num_;
    std::uint32_t ack_num_;
    std::uint32_t next_seq_num_;

    // Sender queue
    // Channel<SocketBuffer> send_queue_;
    // Receiver queue
    // Channel<SocketBuffer> recv_queue_;

    // keepalive timer
    // retransmit timer
};
}  // namespace transport
}  // namespace minitcp
#endif  // ! MINITCP_SRC_TRANSPORT_SOCKET_H_