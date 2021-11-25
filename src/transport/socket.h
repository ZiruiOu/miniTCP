#ifndef MINITCP_SRC_TRANSPORT_SOCKET_H_
#define MINITCP_SRC_TRANSPORT_SOCKET_H_

#include <stdlib.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>

#include "../common/intrusive.h"
#include "../common/timer.h"
#include "../common/timer_impl.h"
#include "../common/types.h"
#include "../ethernet/device.h"
#include "channel.h"

namespace minitcp {
namespace transport {
class Socket;

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
  char* buffer;
  std::size_t length;

  std::uint8_t flags;
  std::uint32_t seq;
  std::uint32_t ack;
  std::uint32_t recv_window;

  struct list_head link {
    nullptr, nullptr
  };

  std::chrono::time_point<std::chrono::high_resolution_clock> send_time;
  std::chrono::time_point<std::chrono::high_resolution_clock> ack_time;
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

  // disallow copy and move
  SocketBase(const SocketBase&) = delete;
  SocketBase& operator=(const SocketBase&) = delete;

  SocketBase(SocketBase&&) = delete;
  SocketBase& operator=(SocketBase&&) = delete;

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

class Socket : public SocketBase {
 public:
  // TODO : arbitary bind a local ip and local port.
  Socket() : SocketBase(ip_t{0}, ip_t{ethernet::getLocalIP()}, 0, 0) {
    // FIXME : not a good idea to use rand() ?
    send_window_ = 10000;
    recv_window_ = 4096;

    send_seq_num_ = rand();
    next_seq_num_ = send_seq_num_;
    send_unack_base_ = send_seq_num_;
    send_unack_nextseq_ = send_seq_num_;

    recv_seq_num_ = 0;
  }

  // FIXME : only estiblished socket can use.
  Socket(ip_t dest_ip, ip_t src_ip, port_t dest_port, port_t src_port,
         std::uint32_t send_seq_num, std::uint32_t recv_seq_num)
      : SocketBase(dest_ip, src_ip, dest_port, src_port),
        send_seq_num_(send_seq_num),
        recv_seq_num_(recv_seq_num) {
    // TODO : add into constant.
    // TODO : receiver would better use ring buffer.
    send_window_ = 10000;
    recv_window_ = 4096;

    next_seq_num_ = send_seq_num_;
    send_unack_base_ = send_unack_nextseq_ = send_seq_num_;
  }

  ~Socket() = default;

  bool IsValid() const override {
    // TODO : check state
    return true;
  }

  int ReceiveRequest(ip_t remote_ip, ip_t local_ip, struct tcphdr* tcp_header);
  int ReceiveConnection(struct tcphdr* tcp_header);
  int ReceiveStateProcess(ip_t remote_ip, ip_t local_ip,
                          struct tcphdr* tcp_header, const void* buffer,
                          int length);

  bool AddToListenQueue(class Socket* request) {
    std::scoped_lock lock(socket_mutex_);
    if (backlog_count_ >= max_backlog_) {
      return false;
    }
    // backlog_count_++;
    //  listen_queue_.push_front(request);
    //  request->lietsn_link_ = listen_queue_.begin();
    return true;
  }

  void RemoveFromListenQueue(class Socket* request) {
    std::scoped_lock lock(socket_mutex_);
    // backlog_count_--;
    // listen_queue_.erase(request->listen_link_);
  }

  void MoveToAcceptQueue(class Socket* request) {
    std::scoped_lock lock(socket_mutex_);
    // listen_queue_.erase(request->GetLink());
    accept_queue_.push_front(request);
    request->accept_link_ = accept_queue_.begin();
    socket_cv_.notify_one();
  }

  void AddToAcceptQueue(class Socket* request) {
    std::scoped_lock lock(socket_mutex_);
    accept_queue_.push_front(request);
    request->accept_link_ = accept_queue_.begin();
    socket_cv_.notify_one();
  }

  class Socket* PopAcceptQueue() {
    std::scoped_lock lock(socket_mutex_);
    class Socket* child_socket = accept_queue_.front();
    backlog_count_--;
    accept_queue_.pop_front();
    return child_socket;
  }

  // socket send functions.
  // send Syn, SynAck or data packets and set up timer for these packets.
  int SendTCPPacketImpl(std::uint8_t flags, const void* buffer, int length);
  // send Ack/RST flags
  // NOTIMPLEMENTED : piggyback some data when the pending queue is not null.
  int SendTCPPacketPush(std::uint32_t sequence, std::uint32_t ack,
                        std::uint8_t flags, const void* buffer, int length);

  // Socket Actions: send and receive
  int Bind(struct sockaddr* address, socklen_t address_len);
  int Listen(int backlog);
  class Socket* Accept();
  int Connect(struct sockaddr* address, socklen_t address_len);
  int Close();

 private:
  /* TODO : RTT estimation */
  /* TODO : congestion control */
  /* TODO : flow control */

  // retransmission management
  void RtxEnqueue(struct SocketBuffer* buffer);
  struct SocketBuffer* RtxDeque();
  void RetransmitCallback();

  // send queue management
  void TxEnqueue(struct SocketBuffer* buffer);
  void TxDeque();

  // RAII reference counting.
  std::atomic<std::uint32_t> ref_cnt_;

  // socket mutex
  std::mutex socket_mutex_;
  // socket condition variable
  std::condition_variable socket_cv_;

  // The socket backlog, i.e. the maximum of listen_queue_.length +
  // accept_queue_.length
  int max_backlog_{1};
  int backlog_count_{0};

  // iterator for listen and accept.
  // can be only used in state of kSynSent/kSynReceived.
  std::list<class Socket*>::iterator accept_link_;

  // the listen socket this socket belongs to.
  class Socket* listen_socket_{nullptr};

  // queue for accept.
  // can only be used in the state of kListen.
  std::list<class Socket*> accept_queue_;

  // for sending and receiving.
  // std::uint32_t seq_num_;
  // std::uint32_t ack_num_;
  // std::uint32_t next_seq_num_;

  std::uint32_t send_window_;
  std::uint32_t recv_window_;
  std::uint32_t send_seq_num_;
  std::uint32_t recv_seq_num_;
  std::uint32_t next_seq_num_;

  std::uint32_t send_unack_base_;
  std::uint32_t send_unack_nextseq_;

  // [send_unack_base_, send_unack_nextseq_)
  struct list_head retransmit_queue_ {
    &retransmit_queue_, &retransmit_queue_
  };
  // [send_seq_num_, next_seq_num_)
  struct list_head send_queue_ {
    &send_queue_, &send_queue_
  };
  // for sampling.
  std::vector<SocketBuffer*> send_history_;

  // Sender queue
  // Channel<SocketBuffer> send_queue_;
  // Receiver queue
  // Channel<SocketBuffer> recv_queue_;

  // keepalive timer or Syn Ack retransmission.
  handler_t keepalive_timer_;
  // retransmit timer
  handler_t retransmit_timer_;
};
}  // namespace transport
}  // namespace minitcp
#endif  // ! MINITCP_SRC_TRANSPORT_SOCKET_H_