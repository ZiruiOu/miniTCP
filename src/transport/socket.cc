#include "socket.h"

#include "../common/logging.h"
#include "../common/timer.h"
#include "tcp_kernel.h"

namespace minitcp {
namespace transport {

// WARNING : hold socket lock before calling this function.
void Socket::RtxEnqueue(struct SocketBuffer* buffer) {
  int should_setup = !list_empty(&retransmit_queue_);
  list_insert_after(&buffer->link, retransmit_queue_.prev);

  // if the retransmit queue was empty before, set up the retransmit timer.
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
struct SocketBuffer* Socket::RtxDeque() {
  if (list_empty(&retransmit_queue_)) {
    return nullptr;
  } else {
    struct list_head* head = retransmit_queue_.next;
    struct SocketBuffer* buffer = container_of(head, struct SocketBuffer, link);
    list_remove(head);

    // TODO : maintain unack_base and unack_seq_num

    // if the retransmit queue become empty, cancell the timer.
    if (list_empty(&retransmit_queue_)) {
      cancellTimer(retransmit_timer_);
      retransmit_timer_ = nullptr;
    }

    return buffer;
  }
}

void Socket::RetransmitCallback() {
  std::scoped_lock lock(socket_mutex_);

  struct list_head* node = &retransmit_queue_;
  struct list_head* last = &retransmit_queue_;

  for (node = node->next; node != last; node = node->next) {
    struct SocketBuffer* buffer = container_of(node, struct SocketBuffer, link);
    sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_, buffer->seq,
                  buffer->ack, buffer->flags, buffer->recv_window,
                  buffer->buffer, buffer->length);
  }
}

// Send data with retransmission.
// WARNING : hold socket lock before calling this function.
int Socket::SendTCPPacketImpl(std::uint8_t flags, const void* buffer,
                              int length) {
  int status =
      sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_, send_seq_num_,
                    recv_seq_num_, flags, recv_window_, buffer, length);
  if (!status) {
    struct SocketBuffer* socket_buffer = new struct SocketBuffer;

    socket_buffer->flags = flags;
    socket_buffer->seq = send_seq_num_;
    socket_buffer->ack = recv_seq_num_;
    socket_buffer->recv_window = recv_window_;

    if (buffer) {
      socket_buffer->buffer = new char[length]();
      std::memcpy(socket_buffer->buffer, buffer, length);
    } else {
      socket_buffer->buffer = 0;
    }
    send_unack_nextseq_ += length;

    RtxEnqueue(socket_buffer);
  }

  return status;
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
  class Socket* request_socket =
      new Socket(remote_ip, local_ip, remote_port, local_port, rand(),
                 ntohl(tcp_header->th_seq));
  request_socket->state_ = ConnectionState::kSynSent;

  // (2) insert this new request socket into my listen_queue_.
  request_socket->listen_socket_ = this;

  // auto key = std::make_pair(remote_ip.s_addr, remote_port);
  // auto iterator = listen_queue_.insert(std::make_pair(key, request_socket));
  // request_socket->listen_link_ = iterator;

  // (3) insert this new request socket into establish map.
  insertEstablish(remote_ip, local_ip, remote_port, local_port, request_socket);

  // (4) send SYNACK and setup timer for this request socket.
  SendTCPPacketImpl(TH_SYN | TH_ACK, nullptr, 1);

  return 0;
}

// SynAck handler
// WARNING : hold lock before calling this function.
int Socket::ReceiveConnection(struct tcphdr* tcp_header) {
  recv_seq_num_ = ntohl(tcp_header->th_seq);

  // TODO : SendTCPPacketImpl(TH_ACK, NULL, 0);
  // (1) send TCP packet.
  // (2) maintain seq_num_ and next_seq_num_.
  // (3) setup new timer.
  struct SocketBuffer* buffer = RtxDeque();

  sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_, next_seq_num_,
                recv_seq_num_, TH_ACK, 0, NULL, 0);
  send_seq_num_ = next_seq_num_;
  recv_seq_num_ += 1;
  next_seq_num_ = send_seq_num_ + 1;
  SendTCPPacketPush(send_seq_num_, recv_seq_num_, TH_ACK, nullptr, 0);

  state_ = ConnectionState::kEstablished;
  socket_cv_.notify_one();
}
// unack_seq_num - unack_next_seq_num - send_seq_num - send_next_seq_num =
// send_seq_num + length

int Socket::ReceiveStateProcess(ip_t remote_ip, ip_t local_ip,
                                struct tcphdr* tcp_header, const void* buffer,
                                int length) {
  std::scoped_lock lock(socket_mutex_);

  int status = 0;
  switch (SocketBase::state_) {
    case ConnectionState::kSynSent:
      if (tcp_header->th_flags == TH_SYN | TH_ACK) {
        if (ntohl(tcp_header->th_ack) == next_seq_num_) {
          status = ReceiveConnection(tcp_header);
        } else {
          // reset remote socket
          sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_,
                        ntohl(tcp_header->th_ack), recv_seq_num_ + 1, TH_RST, 0,
                        NULL, 0);
        }
      }
      break;
    case ConnectionState::kSynReceived:
      if (tcp_header->th_flags == TH_SYN) {
        // duplicated SYN: drop.
        status = 1;
      } else if (tcp_header->th_flags == TH_ACK) {
        // accept when the ack number match.
        if (recv_seq_num_ + 1 == ntohl(tcp_header->th_ack)) {
          // move to accept queue of the listen queue.
          if (listen_socket_) {
            listen_socket_->MoveToAcceptQueue(this);
            state_ = ConnectionState::kEstablished;
            status = 0;
            // TODO : piggyback some data. call
            // ReceiveProcessData()
          } else {
            // TODO : clean up.
            status = 1;
          }
        }
      } else if (tcp_header->th_flags == TH_RST) {
        // FIXME : clean up this request socket.
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
      // may piggyback some data.
      // so you need to process some data.
      if (tcp_header->th_flags == TH_ACK) {
        // simple go back-n scheme.
      }
      // TODO : call ReceiveProcessData() to process the received data.
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

  // send SYN datagram.
  // TODO :
  int status = sendTCPPacket(src_ip_, dest_ip_, src_port_, dest_port_,
                             send_seq_num_, 0, TH_SYN, 4096, NULL, 0);

  // TODO : setup retransmission timer.

  state_ = ConnectionState::kSynSent;

  if (status) {
    return status;
  }

  socket_cv_.wait(lock, [this]() {
    return this->state_ == ConnectionState::kEstablished ||
           this->state_ == ConnectionState::kClosed;
  });

  return state_ == ConnectionState::kClosed;
}

// ShutDown
int Socket::Close() {
  std::scoped_lock lock(socket_mutex_);
  return 0;
}

}  // namespace transport
}  // namespace minitcp