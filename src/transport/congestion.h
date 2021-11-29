#ifndef MINITCP_SRC_TRANSPORT_CONGESTION_H_
#define MINITCP_SRC_TRANSPORT_CONGESTION_H_

#include "../common/types.h"

namespace minitcp {
namespace transport {

// congestion state for loss-based congestion control algorithm.
enum class ManagerState {
  kSlowStart = 0,
  kCongestionAvoidance,
  kFastRecovery,
};

class CCManagerBase {
 public:
  CCManagerBase() = default;
  ~CCManagerBase() = default;

  void SetState(enum ManagerState new_state) { state_ = new_state; }

  virtual void ReceiveAck(std::uint32_t ack_num) = 0;
  virtual void DoSlowStart() = 0;
  virtual void DoFastRecovery() = 0;

 protected:
  enum ManagerState state_;
};

class TCPReno : public CCManagerBase {
 public:
  TCPReno() = default;
  ~TCPReno() = default;

  void ReceiveAck(std::uint32_t ack_num) override;
  void DoSlowStart() override;
  void DoFastRecovery() override;

 private:
  std::uint32_t acked_num_;  // last ack number
  std::uint32_t
      duplicated_count_;  // dup ack count, dup ack == 3 implies a loss.

  std::uint32_t congestion_window_;  // congestion window.
  std::uint32_t ssthresh_;           // init : 64 kb.
};

}  // namespace transport
}  // namespace minitcp

#endif  // ! MINITCP_SRC_TRANSPORT_CONGESTION_H_