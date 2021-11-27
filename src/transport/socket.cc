#include "socket.h"

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

// TODO : optimizations.
void Socket::RetransmitExtend() {
  int status = 0;
  std::uint32_t bytes_in_flight = send_unack_nextseq_ - send_unack_base_;

  struct list_head* node = send_queue_.next;
  while (node != &send_queue_) {
    struct SocketBuffer* buffer = container_of(node, struct SocketBuffer, link);

    if (buffer->length + bytes_in_flight < send_window_) {
      // (1) record sending time.
      buffer->send_time = std::chrono::high_resolution_clock::now();
      // (2) send.
      MINITCP_LOG(INFO) << " retransmit extend : send out a packet."
                        << std::endl;
      status = sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_,
                             buffer->seq, buffer->ack, buffer->flags,
                             buffer->recv_window, buffer, buffer->length);
      // (3) add into retransmit queue.
      struct list_head* tmp = node->next;
      list_remove(node);
      RtxEnqueue(node);
      node = tmp;
      bytes_in_flight += buffer->length;

    } else {
      // the window is full, quit.
      break;
    }
  }

  send_unack_nextseq_ = send_unack_base_ + bytes_in_flight;
}

int Socket::RetransmitShrink(std::uint32_t received_ack) {
  int status = 0;
  struct list_head* node = retransmit_queue_.next;
  while (node != &retransmit_queue_) {
    struct SocketBuffer* buffer = container_of(node, struct SocketBuffer, link);
    if (buffer->seq + buffer->length <= received_ack) {
      status = 0;
      send_unack_base_ += buffer->length;
      // TODO : clean up socket buffer.
      buffer->ack_time = std::chrono::high_resolution_clock::now();

      struct list_head* tmp = node->next;
      RtxDequeue();
      node = tmp;
    } else {
      break;
    }
  }
  return status;
}

void Socket::RtxEnqueue(struct list_head* node) {
  int should_setup = list_empty(&retransmit_queue_);

  list_insert_after(node, retransmit_queue_.prev);

  // set up the timer.
  if (should_setup) {
    retransmit_timer_ = new TimerHandler(/*is_persist = */ true);
    std::function<void()> retransmit_wrapper = [this]() {
      // TODO : add into constant.h
      this->RetransmitCallback();
      setTimerAfter(1000, this->retransmit_timer_);
    };
    retransmit_timer_->RegisterCallback(retransmit_wrapper);
    setTimerAfter(1000, retransmit_timer_);
  }
}

// WARNING: hold socket lock before calling this function.
void Socket::RtxDequeue() {
  if (!list_empty(&retransmit_queue_)) {
    struct list_head* head = retransmit_queue_.next;
    list_remove(head);

    if (list_empty(&retransmit_queue_)) {
      cancellTimer(retransmit_timer_);
      retransmit_timer_ = nullptr;
    }
  }
}

void Socket::RetransmitCallback() {
  std::scoped_lock lock(socket_mutex_);

  struct list_head* node = &retransmit_queue_;
  struct list_head* last = &retransmit_queue_;

  for (node = node->next; node != last; node = node->next) {
    struct SocketBuffer* buffer = container_of(node, struct SocketBuffer, link);
    // (1) record retransmit time
    buffer->send_time = std::chrono::high_resolution_clock::now();
    // (2) resend.
    MINITCP_LOG(INFO) << "retransmit callback : send out a packet."
                      << "seq = " << buffer->seq << std::endl
                      << "ack = " << buffer->ack << std::endl;
    sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_, buffer->seq,
                  buffer->ack, buffer->flags, buffer->recv_window,
                  buffer->buffer, buffer->length);
  }
}

// WARNING : hold socket lock before calling this function.
int Socket::SendTCPPacketImpl(std::uint8_t flags, const void* buffer,
                              int length) {
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
    socket_buffer->length = length;
  }

  send_seq_num_ += length;

  // add into send queue.
  TxEnqueue(&socket_buffer->link);

  return 0;
}

// Send data without retransmission.
// WARNING : hold socket lock before calling this function.
int Socket::SendTCPPacketPush(std::uint32_t sequence, std::uint32_t ack,
                              std::uint8_t flags, const void* buffer,
                              int length) {
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

  std::uint32_t local_seqnum = rand();
  std::uint32_t remote_seqnum = ntohl(tcp_header->th_seq) + 1;

  class Socket* request_socket =
      new Socket(remote_ip, local_ip, remote_port, local_port, local_seqnum,
                 remote_seqnum);

  request_socket->state_ = ConnectionState::kSynReceived;

  // (2) insert this new request socket into my listen_queue_.
  request_socket->listen_socket_ = this;

  // (3) insert this new request socket into establish map.
  insertEstablish(remote_ip, local_ip, remote_port, local_port, request_socket);

  // (4) send SYNACK and setup timer for this request socket.
  request_socket->SendTCPPacketImpl(TH_SYN | TH_ACK, nullptr, 1);

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
    socket_cv_.notify_one();
  }

  return status;
}

int Socket::ReceiveData(struct tcphdr* tcp_header, const void* buffer,
                        int length) {
  std::uint32_t remote_seqnum = ntohl(tcp_header->th_seq);
  if (length > 0) {
    if (remote_seqnum >= recv_seq_num_) {
      MINITCP_LOG(INFO) << "Receive data : received " << length
                        << " bytes from remote, which reads "
                        << reinterpret_cast<const char*>(buffer) << std::endl;
      if (remote_seqnum == recv_seq_num_) {
        recv_seq_num_ += length;
      }
      SendTCPPacketPush(send_seq_num_, recv_seq_num_, TH_ACK, NULL, 0);
      received_ = true;
    }
  }
}

int Socket::ReceiveAck(struct tcphdr* tcp_header) {
  int received_ack = ntohl(tcp_header->th_ack);
  int status = RetransmitShrink(received_ack);
  if (!status) {
    RetransmitExtend();
  }
  return status;
}

int Socket::ReceiveStateProcess(ip_t remote_ip, ip_t local_ip,
                                struct tcphdr* tcp_header, const void* buffer,
                                int length) {
  std::scoped_lock lock(socket_mutex_);

  int status = 0;
  switch (SocketBase::state_) {
    case ConnectionState::kSynSent:
      if (tcp_header->th_flags == TH_SYN | TH_ACK) {
        std::uint32_t received_ack = ntohl(tcp_header->th_ack);
        status = ReceiveAck(tcp_header);
        if (!status) {
          status = ReceiveConnection(tcp_header);
        } else {
          SendTCPPacketPush(received_ack, recv_seq_num_ + 1, TH_RST, NULL, 0);
        }
      }
      break;
    case ConnectionState::kSynReceived:
      if (tcp_header->th_flags == TH_SYN) {
        status = 1;
      } else if (tcp_header->th_flags == TH_ACK) {
        status = ReceiveAck(tcp_header);
        if (!status) {
          if (listen_socket_) {
            listen_socket_->AddToAcceptQueue(this);
            state_ = ConnectionState::kEstablished;
            status = 0;
          } else {
            status = 1;
          }
        }
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
    case ConnectionState::kEstablished:
      if (tcp_header->th_ack) {
        status = ReceiveAck(tcp_header);
      }
      // if (!status) {
      //   ReceiveData(tcp_header, buffer, length);
      // }
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
  max_backlog_ = 1;
  SocketBase::SetState(ConnectionState::kListen);
  return 0;
}

class Socket* Socket::Accept() {
  std::unique_lock<std::mutex> lock(socket_mutex_);

  // block until accept_queue_ is not empty
  socket_cv_.wait(lock, [this]() { return !this->accept_queue_.empty(); });

  class Socket* request = PopAcceptQueue();

  return request;
}

// block or timeout
int Socket::Connect(struct sockaddr* address, socklen_t address_len) {
  std::unique_lock<std::mutex> lock(socket_mutex_);

  auto address_in = reinterpret_cast<struct sockaddr_in*>(address);

  dest_ip_ = address_in->sin_addr;
  dest_port_ = address_in->sin_port;

  int status;
  status = SendTCPPacketImpl(TH_SYN, NULL, 1);

  if (status) {
    return status;
  }

  state_ = ConnectionState::kSynSent;

  socket_cv_.wait(lock, [this]() {
    return this->state_ == ConnectionState::kEstablished ||
           this->state_ == ConnectionState::kClosed;
  });

  return state_ == ConnectionState::kClosed;
}

int Socket::Read(void* buffer, int length) {
  std::unique_lock lock(socket_mutex_);

  socket_cv_.wait(lock, [this]() { return this->received_; });

  received_ = false;

  return 0;
}

int Socket::Write(const void* buffer, int length) {
  std::scoped_lock lock(socket_mutex_);

  int status = SendTCPPacketImpl(TH_ACK, buffer, length);

  return status;
}

// ShutDown
int Socket::Close() {
  std::scoped_lock lock(socket_mutex_);
  return 0;
}

}  // namespace transport
}  // namespace minitcp