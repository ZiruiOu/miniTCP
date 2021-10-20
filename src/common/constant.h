#ifndef MINITCP_SRC_COMMON_CONSTANT_H_
#define MINITCP_SRC_COMMON_CONSTANT_H_

#include <cstdint>

namespace minitcp {

// reserve some handler id for some special service
const std::size_t kReservedHandlerId = 6;

// for ethernet device epolling
const int kMaxEpollEvent = 40;

// for ethernet transmission
const int kFramePayloadSize = 1500;

// for ip transmission

// for tcp transmission
const int kMaxSegmentSize = 600;
}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_CONSTANT_H_