#ifndef MINITCP_SRC_TRANSPORT_SOCKET_IMPL_H_
#define MINITCP_SRC_TRANSPORT_SOCKET_IMPL_H_

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
#include "congestion.h"
#include "ringbuffer.h"
#include "rtomanager.h"

namespace minitcp {
namespace transport {

enum class SocketState {
  kCloseSocket,
  kListenSocket,
  kEstablishSocket,
};

enum class ConnectionState {
  kClosed,
  kListen,
  kSynSent,
  kSynReceived,
  kEstablished,
  kFinWait1,
  kFinWait2,
  kCloseWait,
  kClosing,
  kLastAck,
  kTimeWait,
};

port_t getFreePort();

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

  enum SocketState socket_state_ { SocketState::kCloseSocket };
  enum ConnectionState state_;
};

class Socket : public SocketBase {
 public:
  // TODO : arbitary bind a local ip and local port.
  Socket()
      : SocketBase(ip_t{0}, ip_t{ethernet::getLocalIP()}, 0, getFreePort()) {
    // FIXME : not a good idea to use rand() ?
    // send_seq_num_ = rand();
    send_seq_num_ = 0;
    next_seq_num_ = send_seq_num_;
    send_unack_base_ = send_seq_num_;
    send_unack_nextseq_ = send_seq_num_;

    recv_seq_num_ = 0;

    congestion_ = new class TCPReno(this);
    ring_buffer_ = new class RingBuffer(262144);

    SetUpTimer();
  }

  Socket(ip_t dest_ip, ip_t src_ip, port_t dest_port, port_t src_port,
         std::uint32_t send_seq_num, std::uint32_t recv_seq_num)
      : SocketBase(dest_ip, src_ip, dest_port, src_port),
        send_seq_num_(send_seq_num),
        recv_seq_num_(recv_seq_num) {
    next_seq_num_ = send_seq_num_;
    send_unack_base_ = send_unack_nextseq_ = send_seq_num_;

    congestion_ = new class TCPReno(this);
    ring_buffer_ = new class RingBuffer(262144);

    SetUpTimer();
  }

  ~Socket() {
    delete congestion_;
    delete ring_buffer_;

    CancellTimer();
    CancellKeepalive();
  }

  ip_t GetLocalIp() const { return src_ip_; }
  port_t GetLocalPort() const { return src_port_; }
  ip_t GetRemoteIp() const { return dest_ip_; }
  port_t GetRemotePort() const { return dest_port_; }

  bool IsPassive() const { return socket_state_ == SocketState::kListenSocket; }
  bool IsActive() const {
    return socket_state_ == SocketState::kEstablishSocket;
  }

  bool IsValid() const override {
    // TODO : check state
    return true;
  }

  int ReceiveRequest(ip_t remote_ip, ip_t local_ip, struct tcphdr* tcp_header);
  int ReceiveConnection(struct tcphdr* tcp_header);
  int ReceiveAck(struct tcphdr* tcp_header);
  int ReceiveData(struct tcphdr* tcp_header, const void* buffer, int length);
  int ReceiveStateProcess(ip_t remote_ip, ip_t local_ip,
                          struct tcphdr* tcp_header, const void* buffer,
                          int length);

  void AddToAcceptQueue(class Socket* request) {
    std::scoped_lock lock(socket_mutex_);
    backlog_count_++;
    accept_queue_.push_front(request);
    request->accept_link_ = accept_queue_.begin();
    socket_cv_.notify_one();
  }

  class Socket* PopAcceptQueue() {
    backlog_count_--;
    class Socket* child_socket = accept_queue_.front();
    accept_queue_.pop_front();
    return child_socket;
  }

  // socket send functions.
  int SendTCPPacketImpl(std::uint8_t flags, const void* buffer,
                        std::size_t length);
  // NOTIMPLEMENTED : piggyback some data when the pending queue is not null.
  int SendTCPPacketPush(std::uint32_t sequence, std::uint32_t ack,
                        std::uint8_t flags, const void* buffer,
                        std::size_t length);

  // congestion control
  void RetransmitFront();
  void SetCongestionWindow(std::uint32_t cwnd) {
    send_window_ = cwnd;
    RetransmitExtend();
  }

  // Socket Actions: send and receive
  class Socket* Accept(sockaddr* address, socklen_t* address_len);
  int Bind(const struct sockaddr* address, socklen_t address_len);
  int Listen(int backlog);
  int Connect(const struct sockaddr* address, socklen_t address_len);
  int Send(void* buffer, int length);
  ssize_t Read(void* buffer, std::size_t length);
  ssize_t Write(const void* buffer, std::size_t length);
  int Close();

 private:
  /* TODO : RTT estimation */
  /* TODO : congestion control */
  /* TODO : flow control */

  // retransmission management
  void RtxEnqueue(struct list_head* node);
  void RtxDequeue();
  void RetransmitExtend();
  int RetransmitShrink(std::uint32_t received_ack);
  void RetransmitCallback();

  // send queue management
  void TxEnqueue(struct list_head* node);
  void TxDequeue();

  // connection management
  void ShutDown();
  int ReceiveShutDown();

  // timer management
  void SetUpTimer();
  void CancellTimer();

  // keepalive managment
  void SetupKeepalive();
  void CancellKeepalive();

  // TODO : RAII reference counting.
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
  std::list<class Socket*> accept_queue_;

  std::size_t send_buffer_size_{8192};
  std::size_t send_window_{1ul * kTCPMss};
  std::uint16_t recv_window_{8192};

  std::uint32_t recv_seq_num_;

  std::size_t send_unack_base_;
  std::size_t send_unack_nextseq_;
  struct list_head retransmit_queue_ {
    &retransmit_queue_, &retransmit_queue_
  };

  std::uint32_t send_seq_num_;
  std::uint32_t next_seq_num_;
  struct list_head send_queue_ {
    &send_queue_, &send_queue_
  };

  // congestion control
  class CCManagerBase* congestion_;

  // receive queue
  class RingBuffer* ring_buffer_;

  // keepalive timer.
  int keepalive_probe_;         // maximum = 4.
  timestamp_t keepalive_time_;  // last time receive info from peer.
  handler_t keepalive_timer_{nullptr};

  // rto estimator
  class RttEstimator rtt_estimator_;
  // retransmit timer
  handler_t retransmit_timer_{nullptr};

  // socket timewait timer
  handler_t timewait_timer_{nullptr};
};
}  // namespace transport
}  // namespace minitcp
#endif  // ! MINITCP_SRC_TRANSPORT_SOCKET_IMPL_H_