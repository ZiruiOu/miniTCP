#ifndef MINITCP_SRC_TRANSPORT_RINGBUFFER_H_
#define MINITCP_SRC_TRANSPORT_RINGBUFFER_H_
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace minitcp {
namespace transport {

class RingBuffer {
 public:
  explicit RingBuffer(std::size_t size) : size_(size) {
    buffer_ = new char[size]();
  }
  ~RingBuffer() { delete[] buffer_; }

  RingBuffer(const RingBuffer &) = delete;
  RingBuffer &operator=(const RingBuffer &) = delete;

  RingBuffer(RingBuffer &&) = delete;
  RingBuffer &operator=(RingBuffer &&) = delete;

  std::size_t GetFreeSpace() {
    std::size_t available_length = 0;
    if (read_tail_ >= read_head_) {
      available_length = read_head_ + size_ - read_tail_;
    } else {
      available_length = read_head_ - read_tail_;
    }
    return available_length;
  }

  std::size_t GetAvailableBytes() {
    if (read_head_ <= read_tail_) {
      return read_tail_ - read_head_;
    } else {
      return read_tail_ - read_head_ + size_;
    }
  }

  std::size_t Read(char *buffer, std::size_t length) {
    std::size_t bytes = GetAvailableBytes();
    bytes = std::min(bytes, length);

    std::size_t temp = std::min(bytes, size_ - read_head_);
    std::memcpy(buffer, buffer_ + read_head_, temp);
    if (bytes > temp) {
      std::memcpy(buffer + temp, buffer_, bytes - temp);
    }
    read_head_ = (read_head_ + bytes) % size_;
    // MINITCP_LOG(DEBUG) << "ringbuffer : read " << bytes
    //                   << " from this ringbuffer "
    //                   << " the read_start = " << read_head_ << std::endl
    //                   << " the read_tail =  " << read_tail_ << std::endl;
    return bytes;
  }

  bool Write(const char *buffer, std::size_t length) {
    std::size_t rest_length = GetFreeSpace();
    if (length < rest_length) {
      std::size_t temp = std::min(length, size_ - read_tail_);
      std::memcpy(buffer_ + read_tail_, buffer, temp);
      if (length > temp) {
        std::memcpy(buffer_, buffer + temp, length - temp);
      }
      read_tail_ = (read_tail_ + length) % size_;
      return true;
    }
    return false;
  }

 private:
  std::size_t read_head_{0};
  std::size_t read_tail_{0};
  std::size_t size_;
  char *buffer_;
};

}  // namespace transport
}  // namespace minitcp

#endif  // ! MINITCP_SRC_TRANSPORT_RINGBUFFER_H_