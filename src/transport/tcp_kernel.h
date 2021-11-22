#ifndef MINITCP_SRC_TRANSPORT_TCP_KERNEL_H_
#define MINITCP_SRC_TRANSPORT_TCP_KERNEL_H_

#include "../common/types.h"

namespace minitcp {
namespace transport {

class Socket;
class RequestSocket;

int getNextFd();

// struct tcphdr {
//	u_short	th_sport;		/* source port */
//	u_short	th_dport;		/* destination port */
//	tcp_seq	th_seq;			/* sequence number */
//	tcp_seq	th_ack;			/* acknowledgement number */
//#if BYTE_ORDER == LITTLE_ENDIAN
//	u_char	th_x2:4,		/* (unused) */
//		th_off:4;		/* data offset */
//#endif
//#if BYTE_ORDER == BIG_ENDIAN
//	u_char	th_off:4,		/* data offset */
//		th_x2:4;		/* (unused) */
//#endif
//	u_char	th_flags;
//#define	TH_FIN	0x01
//#define	TH_SYN	0x02
//#define	TH_RST	0x04
//#define	TH_PUSH	0x08
//#define	TH_ACK	0x10
//#define	TH_URG	0x20
//	u_short	th_win;			/* window */
//	u_short	th_sum;			/* checksum */
//	u_short	th_urp;			/* urgent pointer */
// };

//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |          Source Port          |       Destination Port        |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                        Sequence Number                        |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                    Acknowledgment Number                      |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |  Data |           |U|A|P|R|S|F|                               |
//   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
//   |       |           |G|K|H|T|N|N|                               |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |           Checksum            |         Urgent Pointer        |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                    Options                    |    Padding    |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                             data                              |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

std::uint8_t* createTCPPacket(port_t src_port, port_t dest_port,
                              std::uint32_t sequence, std::uint32_t ack,
                              std::uint8_t flags, std::uint16_t window,
                              const void* buffer, int len);

int sendTCPPacket(ip_t src_ip, ip_t dest_ip, port_t src_port, port_t dest_port,
                  std::uint32_t sequence, std::uint32_t ack, std::uint8_t flags,
                  std::uint16_t window, const void* buffer, int len);

}  // namespace transport
}  // namespace minitcp

#endif  //! MINITCP_SRC_TRANSPORT_TCP_KERNEL_H_