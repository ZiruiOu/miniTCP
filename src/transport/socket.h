#ifndef MINITCP_SRC_TRANSPORT_SOCKET_H_
#define MINITCP_SRC_TRANSPORT_SOCKET_H_

#include <condition_variable>
#include <list>
#include <mutex>

#include "../common/timer.h"
#include "../common/timer_impl.h"
#include "../common/types.h"
#include "channel.h"

namespace minitcp {
namespace transport {
using listnode_t = std::list<class RequestSocket*>::iterator;

class Socket;
class RequestSocket;

// enum class SocketState {
//     kFree = 0,
//     kUnConnected,
// };

enum class ConectionState {
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
    SocketBase(ip_t dest_ip, ip_t src_ip, port_t dest_port, port_t src_port_)
        : dest_ip_(dest_ip),
          src_ip_(src_ip),
          dest_port_(dest_port),
          src_port_(src_port),
          state_(ConnectionState::kClosed) {}
    ~SocketBase() = default;

    bool SetState(enum class ConnectionState new_state) {
        // TODO : implement the state machine transition.
        // state_ = new_state;

        bool flag = false;
        switch (state_) {
            case ConnectionState::kClosed:
                if (new_state == ConnectionState::kListen ||
                    new_state == ConnectionState::kSynSent) {
                    flag = true;
                    state_ = new_state;
                }
                break;
            case ConnectionState::kSynSent:
                // FIXME : what about receving reset or close?
                if (new_state == ConnectionState::kEstablished ||
                    new_state == ConnectionState::kFinWait1) {
                    flag = true;
                    state_ = new_state;
                }
                break;
            case ConnectionState::kSynReceived:
                // FIXME : what about receving reset or close?
                if (new_state == ConnectionState::kEstablished) {
                    flag = true;
                    state_ = new_state;
                }
                break;
            case ConnectionState::kEstablished:
                if (new_state == ConnectionState::kFinWait1 ||
                    new_state == ConnectionState::kClosing) {
                    flag = true;
                    state_ = new_state;
                }
                break;
            case ConnectionState::kFinWait1:
                if (new_state == ConnectionState::kFinWait2) {
                    flag = true;
                    state_ = new_state;
                }
                break;
            case ConnectionState::kFinWait2:
                if (new_state == ConnectionState::kTimeWait) {
                    flag = true;
                    state_ = new_state;
                }
                break;
            case ConnectionState::kClosing:
                // TODO : rename to kCloseWait
                if (new_state == ConnectionState::kLastAck) {
                    flag = true;
                    state = new_state_;
                }
                break;
            case ConnectionState::kTimeWait:
            case ConnectionState::kLastAck:
                if (new_state == ConnectionState::kClosed) {
                    flag = true;
                    state = new_state_;
                }
                break;
            default:
                break;
        }
        return flag;
    }

    const enum class ConnectionState GetState() const { return state_; }

    virtual bool IsValid() const = 0;

   protected:
    ip_t dest_ip_;
    ip_t stc_ip_;
    port_t dest_port_;
    port_t src_port_;

    enum ConnectionState state_;
};

class RequestSocket : public SocketBase {
   public:
    RequestSocket(ip_t dest_ip, ip_t src_ip, port_t dest_port, port_t src_port)
        : SocketBase(dest_ip, src_ip, dest_port, src_port) {}
    ~RequestSocket() = default;

    bool IsValid() const override {
        // TODO : check state
        return true;
    }

    // state transition:
    // kSynReceived -> kEstablished
    // Actions : SendSynAck, SyncAck Retransmission, Receive Ack.

    int SendSynAck();
    int SynAckRetransmission();

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

    // the syn ack retransmission timer.
    handler_t retransmit_timer_;
    // the keepalive timer.
};

class Socket : public SocketBase {
   public:
    // TODO : arbitary bind a local ip and local port.
    Socket() : SocketBase(ip_t{0}, ip_t{0}, 0, 0) {}

    // FIXME : only estiblished socket can use.
    Socket(ip_t dest_ip, ip_t src_ip, port_t dest_port, port_t src_port)
        : SocketBase(dest_ip, src_ip, dest_port, src_port) {
        // TODO : add into constant.
        send_queue_.reset(5000);
        recv_queue_.reset(5000);
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
        listen_queue_.erase(iter);
    }

    void AddToAcceptQueue(class RequestSocket* request) {
        std::scoped_lock lock(socket_mutex_);
        accept_queue_.push_front(request);
        request->SetLink(accept_queue_.begin());
    }

    class RequestSocket* PopAcceptQueue() {
        std::scoped_lock lock(socket_mutex_);
        class RequestSocket* request = accept_queue_.front();
        backlog_count_--;
        accept_queue_.pop_front();

        // TODO : keepalive timer.
        return request;
    }

    // Socket Actions: send and receive
    int Bind(struct sockaddr* address, socklen_t address_len);
    int Listen(int backlog);
    class RequestSocket* Accept();
    int Connect();
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
    std::list<class RequestSock*> listen_queue_;
    // Request Socket with state = ESTABLISH.
    std::list<class RequestSock*> accept_queue_;

    // Sender queue
    Channel<SocketBuffer> send_queue_;
    // Receiver queue
    Channel<SocketBuffer> recv_queue_;
};
}  // namespace transport
}  // namespace minitcp
#endif  // ! MINITCP_SRC_TRANSPORT_SOCKET_H_