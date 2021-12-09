#include "congestion.h"

#include "../common/logging.h"
#include "socket_impl.h"

namespace minitcp {
namespace transport {

void TCPReno::SetCongestionWindow(std::size_t cwnd) {
  congestion_window_ = cwnd;
  socket_->SetCongestionWindow(congestion_window_);
  // MINITCP_LOG(INFO) << "TCPReno : the current congestion window is "
  //                   << congestion_window_ << std::endl;
}

bool TCPReno::OnReceiveAck(std::uint32_t ack_num) {
  bool retransmit = false;
  switch (state_) {
    case ManagerState::kSlowStart:
      retransmit = OnSlowStart(ack_num);
      break;
    case ManagerState::kCongestionAvoidance:
      retransmit = OnCongestionAvoidance(ack_num);
      break;
    case ManagerState::kFastRecovery:
      retransmit = OnFastRecovery(ack_num);
      break;
    default:
      MINITCP_LOG(ERROR) << "TCPReno : invalid congestion control state. "
                         << std::endl;
      break;
  }
  return retransmit;
}

void TCPReno::OnTimeout() {
  state_ = ManagerState::kSlowStart;
  ssthresh_ = std::max(1ul * kTCPMss, congestion_window_ / 2);
  SetCongestionWindow(kTCPMss);
  acked_num_ = 0;
  duplicated_count_ = 0;
}

bool TCPReno::OnSlowStart(std::uint32_t ack_num) {
  // new ack or dup ack ?
  std::size_t new_cwnd = 0;
  if (!acked_num_ || ack_num > acked_num_) {
    acked_num_ = ack_num;
    duplicated_count_ = 0;
    if (congestion_window_ < ssthresh_) {
      new_cwnd = congestion_window_ + kTCPMss;
      SetCongestionWindow(new_cwnd);
    } else {
      // congestion avoidance
      state_ = ManagerState::kCongestionAvoidance;
      new_cwnd =
          congestion_window_ + kTCPMss * (1.0 * kTCPMss) / congestion_window_;
      SetCongestionWindow(new_cwnd);
    }
    return false;
  } else if (ack_num == acked_num_) {
    duplicated_count_++;
    if (duplicated_count_ == 3) {
      // fast recovery
      state_ = ManagerState::kFastRecovery;
      ssthresh_ = std::max(1ul * kTCPMss, congestion_window_ / 2);
      new_cwnd = ssthresh_ + 3 * kTCPMss;
      SetCongestionWindow(new_cwnd);
      return true;
    } else {
      return false;
    }
  }
}

bool TCPReno::OnCongestionAvoidance(std::uint32_t ack_num) {
  // new ack or dup ack ?
  std::size_t new_cwnd;
  if (!acked_num_ || (ack_num > acked_num_)) {
    acked_num_ = ack_num;
    duplicated_count_ = 0;
    new_cwnd =
        congestion_window_ + kTCPMss * (1.0 * kTCPMss) / congestion_window_;
    SetCongestionWindow(new_cwnd);
  } else if (ack_num == acked_num_) {
    duplicated_count_++;
    if (duplicated_count_ == 3) {
      state_ = ManagerState::kFastRecovery;
      ssthresh_ = std::max(1ul * kTCPMss, congestion_window_ / 2);
      new_cwnd = ssthresh_ + 3 * kTCPMss;
      SetCongestionWindow(new_cwnd);
      return true;
    } else {
      return false;
    }
  }
}

bool TCPReno::OnFastRecovery(std::uint32_t ack_num) {
  // new ack or dup ack ?

  if (!acked_num_ || (ack_num > acked_num_)) {
    state_ = ManagerState::kCongestionAvoidance;

    SetCongestionWindow(ssthresh_);

    acked_num_ = ack_num;
    duplicated_count_ = 0;

    return false;
  } else if (ack_num == acked_num_) {
    SetCongestionWindow(congestion_window_ + kTCPMss);
    return true;
  }
}

}  // namespace transport
}  // namespace minitcp