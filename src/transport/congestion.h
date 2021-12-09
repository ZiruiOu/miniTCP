#ifndef MINITCP_SRC_TRANSPORT_CONGESTION_H_
#define MINITCP_SRC_TRANSPORT_CONGESTION_H_

#include "../common/constant.h"
#include "../common/types.h"

namespace minitcp {
namespace transport {

class Socket;

// congestion state for loss-based congestion control algorithm.
enum class ManagerState {
  kSlowStart = 0,
  kCongestionAvoidance,
  kFastRecovery,
};

class CCManagerBase {
 public:
  CCManagerBase() : state_(ManagerState::kSlowStart){};
  ~CCManagerBase() = default;

  // disallow copy & move
  CCManagerBase(const CCManagerBase&) = delete;
  CCManagerBase& operator=(const CCManagerBase&) = delete;

  CCManagerBase(CCManagerBase&&) = delete;
  CCManagerBase& operator=(CCManagerBase&&) = delete;

  void SetState(enum ManagerState new_state) { state_ = new_state; }

  // callback function for socket
  virtual bool OnReceiveAck(std::uint32_t ack_num) = 0;
  virtual void OnTimeout() = 0;

 protected:
  enum ManagerState state_;
};

class TCPReno : public CCManagerBase {
 public:
  TCPReno(class Socket* socket)
      : socket_(socket), ssthresh_(65536), acked_num_(0), duplicated_count_(0) {
    SetCongestionWindow(kTCPMss);
  };
  ~TCPReno() = default;

  bool OnReceiveAck(std::uint32_t ack_num) override;
  void OnTimeout() override;

  bool OnSlowStart(std::uint32_t ack_num);
  bool OnCongestionAvoidance(std::uint32_t ack_num);
  bool OnFastRecovery(std::uint32_t ack_num);

  void SetCongestionWindow(std::size_t cwnd);

 private:
  class Socket* socket_;
  std::uint32_t acked_num_;  // last ack number
  std::uint32_t
      duplicated_count_;  // dup ack count, dup ack == 3 implies a packet loss.

  std::size_t congestion_window_;  // congestion window.
  std::size_t ssthresh_;           // initialize : 64 kB.
};

}  // namespace transport
}  // namespace minitcp

#endif  // ! MINITCP_SRC_TRANSPORT_CONGESTION_H_