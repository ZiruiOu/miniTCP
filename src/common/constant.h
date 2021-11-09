#ifndef MINITCP_SRC_COMMON_CONSTANT_H_
#define MINITCP_SRC_COMMON_CONSTANT_H_

#include <cstdint>

namespace minitcp {

// pcap parameters
// snap len
const int kPcapSnapLen = 3000;
// timeout
const int kPcapTimeout = -1;

// epoll parameters
// epoll concurrent events
const int kMaxConcurrentEvents = 200;

// epoll stdin fd
const int kEpollStdinFd = -1;

// ethernet type
const std::uint16_t kEtherIPv4Type = 0x0800;
const std::uint16_t kEtherArpType = 0x0806;
// FIXME : not a formal one
const std::uint16_t kEtherRouteType = 0x0900;

// reserve some handler id for some special service
const std::size_t kReservedHandlerId = 6;

// for ethernet device epolling
const int kMaxEpollEvent = 40;

// for ethernet transmission
const int kFramePayloadSize = 1500;

// for arp aging
const int kMaxArpAging = 200;
// for arp type
const std::uint16_t kArpTypeRequest = 1;
const std::uint16_t kArpTypeReply = 2;

// for my rip protocol
const int kRipMaxAging = 6;
const int kRipMaxGarbage = 4;
// poison reversing threshold
const int kRipPoisonThresh = 16;

// for ip transmission

// for tcp transmission
const int kMaxSegmentSize = 600;
}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_CONSTANT_H_