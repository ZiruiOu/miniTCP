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

// TODO : optimizations.
void Socket::RetransmitExtend() {
  int status = 0;
  std::uint32_t bytes_in_flight = send_unack_nextseq_ - send_unack_base_;

  struct list_head* node = send_queue_.next;
  while (node != &send_queue_) {
    struct SocketBuffer* buffer = container_of(node, struct SocketBuffer, link);

    int length = calculatePacketBytes(buffer->flags, buffer->length);

    if (length + bytes_in_flight < send_window_) {
      // (1) record sending time.
      buffer->send_time = std::chrono::high_resolution_clock::now();
      // (2) send.
      // MINITCP_LOG(INFO) << " retransmit extend : send out a packet."
      //                  << std::endl;
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

    int length = calculatePacketBytes(buffer->flags, buffer->length);

    if (buffer->seq + length <= received_ack) {
      status = 0;
      send_unack_base_ += length;
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
  // if (should_setup) {
  //}
}

// WARNING: hold socket lock before calling this function.
void Socket::RtxDequeue() {
  if (!list_empty(&retransmit_queue_)) {
    struct list_head* head = retransmit_queue_.next;
    list_remove(head);

    // if (list_empty(&retransmit_queue_) && retransmit_timer_) {
    //   cancellTimer(retransmit_timer_);
    //   retransmit_timer_ = nullptr;
    // }
  }
}

void Socket::SetUpTimer() {
  retransmit_timer_ = new TimerHandler(/*is_persist = */ true);
  std::function<void()> retransmit_wrapper = [this]() {
    // TODO : add into constant.h
    this->RetransmitCallback();
    setTimerAfter(1000, this->retransmit_timer_);
  };
  retransmit_timer_->RegisterCallback(retransmit_wrapper);
  setTimerAfter(1000, retransmit_timer_);
}

void Socket::CancellTimer() {
  cancellTimer(retransmit_timer_);
  retransmit_timer_ = nullptr;
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
    // MINITCP_LOG(INFO) << "retransmit callback : send out a packet."
    //                  << "seq = " << buffer->seq << std::endl
    //                  << "ack = " << buffer->ack << std::endl;
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

  send_seq_num_ += calculatePacketBytes(flags, length);

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
    socket_cv_.notify_one();
  }

  return status;
}

int Socket::ReceiveData(struct tcphdr* tcp_header, const void* buffer,
                        int length) {
  std::uint32_t remote_seqnum = ntohl(tcp_header->th_seq);

  length = calculatePacketBytes(tcp_header->th_flags, length);

  bool is_finish = (tcp_header->th_flags == (TH_FIN | TH_ACK));

  if (length > 0) {
    if (remote_seqnum >= recv_seq_num_) {
      MINITCP_LOG(INFO) << "Receive data : received " << length
                        << " bytes from remote, which reads "
                        << reinterpret_cast<const char*>(buffer) << std::endl;
      if (remote_seqnum == recv_seq_num_) {
        recv_seq_num_ += length;
      }
      SendTCPPacketPush(send_seq_num_, recv_seq_num_, TH_ACK, NULL, 0);
      if (is_finish) {
        ReceiveShutDown();
      }
      received_ = true;
    }
  }
}

int Socket::ReceiveAck(struct tcphdr* tcp_header) {
  int received_ack = ntohl(tcp_header->th_ack);
  RetransmitShrink(received_ack);
  RetransmitExtend();
}

int Socket::ReceiveShutDown() {
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
  SendTCPPacketImpl(TH_FIN | TH_ACK, nullptr, 0);
}

int Socket::ReceiveStateProcess(ip_t remote_ip, ip_t local_ip,
                                struct tcphdr* tcp_header, const void* buffer,
                                int length) {
  std::scoped_lock lock(socket_mutex_);

  MINITCP_LOG(INFO) << "entering packet handler with state = "
                    << static_cast<std::uint32_t>(state_) << std::endl;

  int status = 0;

  std::uint32_t remote_seqnum = ntohl(tcp_header->th_seq);
  std::uint32_t received_ack = ntohl(tcp_header->th_ack);
  std::uint8_t received_flags = tcp_header->th_flags;

  switch (SocketBase::state_) {
    case ConnectionState::kSynSent:
      if (received_flags & TH_SYN) {
        if (received_flags & TH_ACK) {
          ReceiveAck(tcp_header);
          if (send_seq_num_ == received_ack) {
            status = ReceiveConnection(tcp_header);
          } else {
            SendTCPPacketPush(received_ack, recv_seq_num_ + 1, TH_RST, NULL, 0);
          }
        } else {
          RtxDequeue();
          send_seq_num_ -= 1;
          recv_seq_num_ = remote_seqnum + 1;
          SendTCPPacketImpl(TH_SYN | TH_ACK, nullptr, 0);
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
        MINITCP_LOG(INFO) << "received synack packet " << std::endl;
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
      // TODO : wait for 2 MSS.
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
  status = SendTCPPacketImpl(TH_SYN, NULL, 0);

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
  std::unique_lock<std::mutex> lock(socket_mutex_);

  // (1) send FIN + ACK, convert to FinWait1
  // (2) wait until ACK, then back to FinWait2

  // (3) receive FIN + ACK, convert to Timewait
  // (4) send ACK

  ShutDown();

  socket_cv_.wait(
      lock, [this]() { return this->state_ == ConnectionState::kClosed; });
  return 0;
}

}  // namespace transport
}  // namespace minitcp