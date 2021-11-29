#ifndef MINITCP_SRC_TRANSPORT_CHANNEL_H_
#define MINITCP_SRC_TRANSPORT_CHANNEL_H_

#include <optional>
#include <vector>

#include "../common/logging.h"
#include "../common/types.h"

// A single reader single writer channel.

namespace minitcp {
namespace transport {
template <class T>
class Channel {
 public:
  Channel() = default;
  ~Channel() = default;

  bool empty() { return head_ == tail_; }

  bool push(T item) {
    buffer_.push_back(item);
    return true;
  }

  std::optional<T> pop() {
    if (head_ == tail_) {
      return {};
    }
    // barrier();
    T item = buffer_[head_];
    head_ = head_ + 1;
    if (head_ >= buffer_size_) {
      head_ -= buffer_size_;
    }
    return item;
  }

 private:
  std::size_t head_{0};
  std::size_t tail_{0};
  std::size_t buffer_size_;
  std::queue<T> buffer_;
};
}  // namespace transport
}  // namespace minitcp

#endif  // ! MINITCP_SRC_TRANSPORT_CHANNEL_H_