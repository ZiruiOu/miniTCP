#ifndef MINITCP_SRC_COMMON_BLOCK_H_
#define MINITCP_SRC_COMMON_BLOCK_H_

#include <cstdint>
#include <vector>

template <typename T>
class BlockQueue {
   public:
    BlockQueue(std::size_t size) : size_(size) {}
    ~BlockQueue() = default;

   private:
    std::size_t size_;
};

#endif  // ! MINITCP_SRC_COMMON_BLOCK_H_