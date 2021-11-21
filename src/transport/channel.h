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
    explicit Channel(std::size_t buffer_size) : buffer_size_(buffer_size) {
        buffer_.reserve(buffer_size_ + 1);
    }
    ~Channel() = default;

    void reset(std::size_t new_size) {
        // FIXME : only function listen can call this function.
        buffer_size_ = new_size;
        if (buffer_size_ > 0) {
            buffer_.reset(buffer_size_ + 1);
        } else {
            buffer_.reset(0);
        }
    }

    bool empty() { return head_ == tail_; }

    bool push(T item) {
        std::size_t next_tail = (tail_ + 1);
        if (next_tail >= buffer_size_) {
            next_tail -= buffer_size_;
        }
        if (next_tail == head_) {
            return false;
        }
        // TODO : move ?
        buffer_[next_tail] = item;
        tail_ = next_tail;
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
    std::vector<T> buffer_;
};
}  // namespace transport
}  // namespace minitcp

#endif  // ! MINITCP_SRC_TRANSPORT_CHANNEL_H_