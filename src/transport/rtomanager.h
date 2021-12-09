#ifndef MINITCP_SRC_TRANSPORT_RTOMANAGER_H_
#define MINITCP_SRC_TRANSPORT_RTOMANAGER_H_

#include <cmath>
#include <cstdint>

#include "../common/constant.h"
#include "../common/types.h"

namespace minitcp {
namespace transport {

class RttEstimator {
 public:
  RttEstimator() = default;
  ~RttEstimator() = default;

  void AddSample(timestamp_t send_time, timestamp_t ack_time) {
    std::size_t delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                            ack_time - send_time)
                            .count();
    if (!estimated_rtt_) {
      estimated_rtt_ = delta;
    } else {
      estimated_rtt_ = 0.875 * estimated_rtt_ + 0.125 * delta;
      deviation_rtt_ =
          0.75 * deviation_rtt_ + 0.25 * std::fabs(delta - estimated_rtt_);
      estimated_rto_ = estimated_rtt_ + 4 * deviation_rtt_;
    }
  }

  void TimeoutCallback() {
    estimated_rto_ =
        std::max(kTCPMinRto, std::min(estimated_rto_ * 2, kTCPMaxRto));
  }
  std::size_t GetEstimatedRtt() const { return estimated_rtt_; }
  std::size_t GetTimeoutInterval() const { return estimated_rto_; }

 private:
  std::size_t estimated_rto_{1000};
  std::size_t estimated_rtt_{0};  // in milliseconds.
  std::size_t deviation_rtt_{0};  // in milliseconds.
};

}  // namespace transport
}  // namespace minitcp

#endif  // ! MINITCP_SRC_TRANSPORT_RTOMANAGER_H_