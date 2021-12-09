#include "socket_impl.h"

#include "../common/logging.h"
#include "../common/timer.h"
#include "tcp_kernel.h"

namespace minitcp {
namespace transport {

// WARNING : hold socket lock before calling this function.
void Socket::TxEnqueue(struct list_head* node) {
  list_insert_after(node, send_queue_.prev);
  RetransmitExtend();
}

void Socket::RetransmitExtend() {
  int status = 0;
  std::size_t bytes_in_flight = send_unack_nextseq_ - send_unack_base_;

  struct list_head* node = send_queue_.next;
  while (node != &send_queue_) {
    struct SocketBuffer* buffer = container_of(node, struct SocketBuffer, link);

    std::size_t length = calculatePacketBytes(buffer->flags, buffer->length);

    if (length + bytes_in_flight <= send_window_) {
      // (1) record sending time.
      buffer->send_time = std::chrono::high_resolution_clock::now();
      // (2) send.
      // MINITCP_LOG(INFO) << " retransmit extend : send out a packet."
      //                  << std::endl;
      if (buffer->length > 1456ul) {
        MINITCP_LOG(ERROR) << "TCP send packet error : " << std::endl
                           << " packet sequence number = " << buffer->seq
                           << std::endl
                           << "packet size = " << buffer->length << std::endl;
      }

      status = sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_,
                             buffer->seq, buffer->ack, buffer->flags,
                             recv_window_, buffer->buffer, buffer->length);
      // (3) add into retransmit queue.
      struct list_head* tmp = node->next;
      list_remove(node);
      RtxEnqueue(node);
      node = tmp;
      bytes_in_flight += buffer->length;

    } else {
      // the window is full, quit.
      // MINITCP_LOG(INFO) << "Retransmit Extend() : the sending_window = "
      //                  << send_window_ << std::endl
      //                  << " the bytes_in_flight = " << bytes_in_flight
      //                  << " quit. " << std::endl;
      break;
    }
  }

  send_unack_nextseq_ = send_unack_base_ + bytes_in_flight;
}

int Socket::RetransmitShrink(std::uint32_t received_ack) {
  int status = 0;
  struct list_head* node = retransmit_queue_.next;

  timestamp_t now = std::chrono::high_resolution_clock::now();
  while (node != &retransmit_queue_) {
    struct SocketBuffer* buffer = container_of(node, struct SocketBuffer, link);

    int length = calculatePacketBytes(buffer->flags, buffer->length);

    if (buffer->seq + length <= received_ack) {
      status = 0;
      send_unack_base_ += length;
      send_buffer_size_ += length;
      socket_cv_.notify_one();

      rtt_estimator_.AddSample(buffer->send_time, now);

      struct list_head* tmp = node->next;
      RtxDequeue();
      node = tmp;

      if (buffer->buffer) {
        delete[] buffer->buffer;
      }
      delete buffer;

    } else {
      break;
    }
  }
  return status;
}

void Socket::RtxEnqueue(struct list_head* node) {
  int should_setup = list_empty(&retransmit_queue_);

  list_insert_after(node, retransmit_queue_.prev);
}

// WARNING: hold socket lock before calling this function.
void Socket::RtxDequeue() {
  if (!list_empty(&retransmit_queue_)) {
    struct list_head* head = retransmit_queue_.next;
    list_remove(head);
  }
}

void Socket::SetUpTimer() {
  retransmit_timer_ = new TimerHandler(/*is_persist = */ true);
  std::function<void()> retransmit_wrapper = [this]() {
    // TODO : add into constant.h
    this->RetransmitCallback();
    setTimerAfter(rtt_estimator_.GetTimeoutInterval(), retransmit_timer_);
  };
  retransmit_timer_->RegisterCallback(std::move(retransmit_wrapper));
  setTimerAfter(1000, retransmit_timer_);
}

void Socket::CancellTimer() {
  cancellTimer(retransmit_timer_);
  retransmit_timer_ = nullptr;
}

void Socket::RetransmitFront() {
  struct list_head* node = &retransmit_queue_;
  if (!list_empty(node)) {
    struct SocketBuffer* buffer = container_of(node, struct SocketBuffer, link);
    buffer->send_time = std::chrono::high_resolution_clock::now();
    sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_, buffer->seq,
                  buffer->ack, buffer->flags, recv_window_, buffer->buffer,
                  buffer->length);
  }
}

void Socket::RetransmitCallback() {
  std::scoped_lock lock(socket_mutex_);

  struct list_head* node = &retransmit_queue_;
  struct list_head* last = &retransmit_queue_;

  bool is_syn = false;
  for (node = node->next; node != last; node = node->next) {
    struct SocketBuffer* buffer = container_of(node, struct SocketBuffer, link);
    buffer->send_time = std::chrono::high_resolution_clock::now();
    sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_, buffer->seq,
                  buffer->ack, buffer->flags, buffer->recv_window,
                  buffer->buffer, buffer->length);
    if (buffer->flags & TH_SYN) {
      is_syn = true;
    }
  }
  if (!list_empty(&retransmit_queue_)) {
    if (!is_syn) {
      congestion_->OnTimeout();
    }
    rtt_estimator_.TimeoutCallback();
  }
}

void Socket::SetupKeepalive() {
  keepalive_timer_ = new TimerHandler(/*is_persist=*/true);
  std::function<void()> keepalive_wrapper = [this]() {
    // keepalive timeout : 5 seconds.
    std::scoped_lock lock(this->socket_mutex_);
    timestamp_t expired = keepalive_time_ + std::chrono::milliseconds(10000);
    timestamp_t current = std::chrono::high_resolution_clock::now();
    if (current < expired) {
      keepalive_probe_ = 0;
      setTimerAfter(10000, this->keepalive_timer_);
    } else if (keepalive_probe_ < 4) {
      MINITCP_LOG(DEBUG) << "probe = " << keepalive_probe_ << std::endl;
      keepalive_probe_++;
      setTimerAfter(10000, this->keepalive_timer_);
    } else {
      if (this->state_ == ConnectionState::kEstablished) {
        this->state_ = ConnectionState::kClosed;
        this->socket_cv_.notify_one();
      }
    }
  };
  keepalive_timer_->RegisterCallback(std::move(keepalive_wrapper));
  setTimerAfter(5000, keepalive_timer_);
}

void Socket::CancellKeepalive() {
  if (keepalive_timer_) {
    cancellTimer(keepalive_timer_);
  }
}

// WARNING : hold socket lock before calling this function.
int Socket::SendTCPPacketImpl(std::uint8_t flags, const void* buffer,
                              std::size_t length) {
  if (length > 1456ul) {
    MINITCP_LOG(ERROR) << "Send TCP Packet Impl :  "
                       << " packet with sequence number = " << send_seq_num_
                       << std::endl
                       << " has length = " << length << std::endl
                       << " which is larger than a MSS." << std::endl;
    return 1;
  }

  struct SocketBuffer* socket_buffer = new struct SocketBuffer;

  socket_buffer->flags = flags;
  socket_buffer->seq = send_seq_num_;
  socket_buffer->ack = recv_seq_num_;
  socket_buffer->recv_window = recv_window_;
  if (buffer) {
    socket_buffer->buffer = new char[length]();
    socket_buffer->length = length;
    std::memcpy(socket_buffer->buffer, buffer, length);
  } else {
    socket_buffer->buffer = 0;
    socket_buffer->length = 0ul;
  }

  send_seq_num_ +=
      static_cast<std::uint32_t>(calculatePacketBytes(flags, length));

  // add into send queue.
  TxEnqueue(&socket_buffer->link);

  return 0;
}

// Send data without retransmission.
// WARNING : hold socket lock before calling this function.
int Socket::SendTCPPacketPush(std::uint32_t sequence, std::uint32_t ack,
                              std::uint8_t flags, const void* buffer,
                              std::size_t length) {
  int status = sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_, sequence,
                             ack, flags, recv_window_, buffer, length);
  return status;
}

// WARNING: hold lock before calling this function.
int Socket::ReceiveRequest(ip_t remote_ip, ip_t local_ip,
                           struct tcphdr* tcp_header) {
  // (0) see if the backlog_ is full.
  if (backlog_count_ >= max_backlog_) {
    return 1;
  }

  // (1) create new request socket.
  port_t remote_port = ntohs(tcp_header->th_sport);
  port_t local_port = ntohs(tcp_header->th_dport);

  // std::uint32_t local_seqnum = rand();
  std::uint32_t local_seqnum = 0;
  std::uint32_t remote_seqnum = ntohl(tcp_header->th_seq) + 1;

  class Socket* request_socket =
      new Socket(remote_ip, local_ip, remote_port, local_port, local_seqnum,
                 remote_seqnum);

  request_socket->state_ = ConnectionState::kSynReceived;
  request_socket->socket_state_ = SocketState::kEstablishSocket;

  // (2) insert this new request socket into my listen_queue_.
  request_socket->listen_socket_ = this;

  // (3) insert this new request socket into establish map.
  insertEstablish(remote_ip, local_ip, remote_port, local_port, request_socket);

  // (4) send SYNACK and setup timer for this request socket.
  request_socket->SendTCPPacketImpl(TH_SYN | TH_ACK, nullptr, 0);

  return 0;
}

// WARNING : hold lock before calling this function.
int Socket::ReceiveConnection(struct tcphdr* tcp_header) {
  std::uint32_t received_ack = ntohl(tcp_header->th_ack);
  std::uint32_t received_seqnum = ntohl(tcp_header->th_seq);

  recv_seq_num_ = received_seqnum + 1;

  int status =
      SendTCPPacketPush(send_seq_num_, recv_seq_num_, TH_ACK, nullptr, 0);

  if (!status) {
    state_ = ConnectionState::kEstablished;
    // SetupKeepalive();
    socket_cv_.notify_one();
  }

  return status;
}

int Socket::ReceiveData(struct tcphdr* tcp_header, const void* buffer,
                        int length) {
  std::uint32_t remote_seqnum = ntohl(tcp_header->th_seq);

  std::uint32_t new_length = calculatePacketBytes(tcp_header->th_flags, length);

  bool is_finish = (tcp_header->th_flags == (TH_FIN | TH_ACK));

  if (new_length > 0) {
    if (remote_seqnum >= recv_seq_num_) {
      if (remote_seqnum == recv_seq_num_) {
        if ((tcp_header->th_flags & (TH_FIN | TH_SYN)) ||
            ring_buffer_->Write((const char*)buffer, length)) {
          recv_seq_num_ += new_length;
          socket_cv_.notify_one();
        }
      }
      SendTCPPacketPush(send_seq_num_, recv_seq_num_, TH_ACK, NULL, 0);
      if (is_finish) {
        ReceiveShutDown();
      }
      // received_ = true;
    } else if (is_finish && remote_seqnum == recv_seq_num_ - 1) {
      SendTCPPacketPush(send_seq_num_, recv_seq_num_, TH_ACK, NULL, 0);
    }
  }
}

int Socket::ReceiveAck(struct tcphdr* tcp_header) {
  int received_ack = ntohl(tcp_header->th_ack);
  bool retransmit = congestion_->OnReceiveAck(received_ack);

  if (retransmit) {
    RetransmitFront();
  }

  RetransmitShrink(received_ack);
  RetransmitExtend();

  if (received_ack == send_seq_num_) {
    switch (state_) {
      case ConnectionState::kFinWait1:
        state_ = ConnectionState::kFinWait2;
        break;
      case ConnectionState::kLastAck:
        state_ = ConnectionState::kClosed;
        break;
      default:
        break;
    }
  }
}

int Socket::ReceiveShutDown() {
  handler_t timewait_handler;
  std::optional<handler_t> result;
  std::function<void()> timewait_wrapper;

  switch (state_) {
    case ConnectionState::kEstablished:
      state_ = ConnectionState::kCloseWait;
      ShutDown();
      break;
    case ConnectionState::kFinWait2:
      state_ = ConnectionState::kTimeWait;
      break;
    default:
      break;
  }
  return 0;
}

void Socket::ShutDown() {
  switch (state_) {
    case ConnectionState::kEstablished:
      state_ = ConnectionState::kFinWait1;
      break;
    case ConnectionState::kCloseWait:
      state_ = ConnectionState::kLastAck;
      break;
    default:
      MINITCP_LOG(ERROR) << " socket shutdown() : illegal state for shutdown."
                         << std::endl;
      break;
  }
  SendTCPPacketImpl(TH_FIN | TH_ACK, nullptr, 0ul);
}

int Socket::ReceiveStateProcess(ip_t remote_ip, ip_t local_ip,
                                struct tcphdr* tcp_header, const void* buffer,
                                int length) {
  std::scoped_lock lock(socket_mutex_);

  // MINITCP_LOG(INFO) << "entering packet handler with state = "
  //                   << static_cast<std::uint32_t>(state_)
  //                   << " and the next sequence number is " << send_seq_num_
  //                   << std::endl;

  int status = 0;

  std::uint32_t remote_seqnum = ntohl(tcp_header->th_seq);
  std::uint32_t received_ack = ntohl(tcp_header->th_ack);
  std::uint8_t received_flags = tcp_header->th_flags;

  keepalive_probe_ = 0;
  keepalive_time_ = std::chrono::high_resolution_clock::now();

  switch (SocketBase::state_) {
    case ConnectionState::kSynSent:
      if (received_flags & TH_SYN) {
        if (received_flags & TH_ACK) {
          ReceiveAck(tcp_header);
          if (send_seq_num_ == received_ack) {
            status = ReceiveConnection(tcp_header);
          } else {
            SendTCPPacketPush(received_ack, recv_seq_num_ + 1, TH_RST, NULL,
                              0ul);
          }
        } else {
          RtxDequeue();
          send_seq_num_ -= 1;
          recv_seq_num_ = remote_seqnum + 1;
          SendTCPPacketImpl(TH_SYN | TH_ACK, nullptr, 0ul);
          state_ = ConnectionState::kSynReceived;
        }
      }
      break;
    case ConnectionState::kSynReceived:
      if (tcp_header->th_flags == TH_SYN) {
        status = 1;
      } else if (tcp_header->th_flags & TH_ACK) {
        status = ReceiveAck(tcp_header);
        if (received_ack == send_seq_num_) {
          if (listen_socket_) {
            listen_socket_->AddToAcceptQueue(this);
          }
          state_ = ConnectionState::kEstablished;
          // SetupKeepalive();
          socket_cv_.notify_one();
        }
        if (received_flags & TH_SYN) {
          recv_seq_num_ = remote_seqnum;
        }
        ReceiveData(tcp_header, buffer, length);
      } else if (tcp_header->th_flags == TH_RST) {
        // TODO : clean up this request socket.
        status = 1;
      }
      break;
    case ConnectionState::kListen:
      if (tcp_header->th_flags == TH_SYN) {
        status = ReceiveRequest(remote_ip, local_ip, tcp_header);
      } else {
        status = 1;
      }
      break;
    case ConnectionState::kFinWait1:
    case ConnectionState::kEstablished:
    case ConnectionState::kTimeWait:
    case ConnectionState::kCloseWait:
    case ConnectionState::kLastAck:
      if (tcp_header->th_flags & TH_SYN) {
        status = 1;
      } else {
        if (tcp_header->th_flags & TH_ACK) {
          status = ReceiveAck(tcp_header);
        }
        ReceiveData(tcp_header, buffer, length);
      }
      break;
    case ConnectionState::kFinWait2:
      if (tcp_header->th_flags & TH_ACK) {
        status = ReceiveAck(tcp_header);
      }
      ReceiveData(tcp_header, buffer, length);
      break;
    default:
      MINITCP_LOG(ERROR) << "socket ReceiveStateProcess: state not implemented."
                         << std::endl;
      status = 1;
      break;
  }
  return status;
}

int Socket::Bind(const struct sockaddr* address, socklen_t address_len) {
  if (address->sa_family != AF_INET) {
    MINITCP_LOG(ERROR)
        << " socket.Bind() : fail to bind an socket address which is not "
           "AF_INET."
        << std::endl;
    return -1;
  }

  auto address_in = reinterpret_cast<const struct sockaddr_in*>(address);
  if (address_in->sin_addr.s_addr != 0) {
    src_ip_ = address_in->sin_addr;
  }
  src_port_ = ntohs(address_in->sin_port);

  return 0;
}

int Socket::Listen(int backlog) {
  // TODO : backlog
  max_backlog_ = 1;
  state_ = ConnectionState::kListen;
  socket_state_ = SocketState::kListenSocket;
  return 0;
}

class Socket* Socket::Accept(sockaddr* address, socklen_t* address_len) {
  std::unique_lock<std::mutex> lock(socket_mutex_);

  // block until accept_queue_ is not empty
  socket_cv_.wait(lock, [this]() { return !this->accept_queue_.empty(); });

  class Socket* request = PopAcceptQueue();

  return request;
}

int Socket::Connect(const struct sockaddr* address, socklen_t address_len) {
  std::unique_lock<std::mutex> lock(socket_mutex_);

  auto address_in = reinterpret_cast<const struct sockaddr_in*>(address);

  dest_ip_ = address_in->sin_addr;
  dest_port_ = ntohs(address_in->sin_port);

  int status;
  status = SendTCPPacketImpl(TH_SYN, nullptr, 0);

  state_ = ConnectionState::kSynSent;
  socket_state_ = SocketState::kEstablishSocket;

  socket_cv_.wait(lock, [this]() {
    return this->state_ == ConnectionState::kEstablished ||
           this->state_ == ConnectionState::kClosed;
  });

  return state_ == ConnectionState::kClosed;
}

ssize_t Socket::Read(void* buffer, size_t length) {
  std::unique_lock lock(socket_mutex_);

  socket_cv_.wait(lock, [this]() {
    return (this->ring_buffer_->GetAvailableBytes() > 0) ||
           (this->state_ == ConnectionState::kClosed);
  });

  return ring_buffer_->Read((char*)buffer, length);
}

ssize_t Socket::Write(const void* buffer, size_t length) {
  std::unique_lock<std::mutex> lock(socket_mutex_);

  if (state_ == ConnectionState::kClosed) {
    return 0;
  }

  int status = 0;
  std::size_t start_segment = 0;
  std::size_t segment_length = 0;

  while (start_segment < length) {
    segment_length = std::min(1456ul, length - start_segment);
    // MINITCP_LOG(DEBUG) << "Socket Write : " << std::endl
    //                    << " segment_length = " << segment_length << std::endl
    //                    << " start_segment = " << start_segment << std::endl
    //                    << " length = " << length << std::endl;
    if (send_buffer_size_ < segment_length) {
      socket_cv_.wait(lock, [this, segment_length]() {
        return this->send_buffer_size_ > segment_length;
      });
    }
    send_buffer_size_ -= segment_length;
    status |= SendTCPPacketImpl(0, buffer + start_segment, segment_length);
    start_segment += segment_length;
  }

  return length;
}

// ShutDown
int Socket::Close() {
  std::unique_lock<std::mutex> lock(socket_mutex_);

  if (state_ != ConnectionState::kClosed &&
      state_ != ConnectionState::kListen) {
    ShutDown();
    socket_cv_.wait(lock, [this]() {
      return (this->state_ == ConnectionState::kClosed) ||
             (this->state_ == ConnectionState::kTimeWait);
    });
    if (this->state_ == ConnectionState::kTimeWait) {
      socket_cv_.wait_until(lock, std::chrono::high_resolution_clock::now() +
                                      std::chrono::milliseconds(2 * 10000));
    }
    state_ = ConnectionState::kClosed;
  }

  return 0;
}

}  // namespace transport
}  // namespace minitcp