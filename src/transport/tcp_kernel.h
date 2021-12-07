#ifndef MINITCP_SRC_TRANSPORT_TCP_KERNEL_H_
#define MINITCP_SRC_TRANSPORT_TCP_KERNEL_H_

#include "../common/types.h"
#include "socket_impl.h"

namespace minitcp {
namespace transport {

class Socket;

/**@brief : get a new fd from Linux kernel.
 *
 * @return : return -1 on failure, else return the new fd.
 */
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

/**@brief send a TCP packet by calling sendIPPacket.
 *
 * @param src_ip  local ip of the socket.
 * @param dest_ip remote ip of the socket.
 * @param src_port local port of the socket.
 * @param dest_port remote port of the socket.
 * @param sequence  sequence number of the TCP packet.
 * @param ack       ack number of the TCP packet.
 * @param flags     TCP control flags.
 * @param window    the receiver window (for flow control).
 * @param buffer    TCP payload.
 * @param len       length of the TCP payload.
 *
 * @return 0 on success, 1 on failure.
 **/
int sendTCPPacket(ip_t src_ip, ip_t dest_ip, port_t src_port, port_t dest_port,
                  std::uint32_t sequence, std::uint32_t ack, std::uint8_t flags,
                  std::uint16_t window, const void* buffer, int len);

int calculatePacketBytes(std::uint8_t flags, int length);

connection_key_t makeKey(ip_t remote_ip, ip_t local_ip, port_t remote_port,
                         port_t local_port);

[[nodiscard]] int insertListen(ip_t local_ip, port_t local_port,
                               class Socket* request);

class Socket* findListen(ip_t local_ip, port_t local_port);

int removeListen(ip_t local_ip, port_t local_port);

[[nodiscard]] int insertEstablish(ip_t remote_ip, ip_t local_ip,
                                  port_t remote_port, port_t local_port,
                                  class Socket* socket);

class Socket* findEstablish(ip_t remote_ip, ip_t local_ip, port_t remote_port,
                            port_t local_port);

int removeEstablish(ip_t remote_ip, ip_t local_ip, port_t remote_port,
                    port_t local_port);

class Socket* findSocketByFd(int fd);
[[nodiscard]] int insertSocketByFd(int fd, class Socket* socket);
[[nodiscard]] int removeSocketByFd(int fd, class Socket* socket);

int cleanupSocket(int fd);

port_t getFreePort();

/**@brief TCP Packet receive callback.
 *
 * @param [in] ip_header : ip header of the received packet.
 * @param [in] buffer : ip payload.
 * @param length : length of the ip payload.
 *
 * @return return 0 on succcessfully parse the packet, -1 on failure.
 **/
int tcpReceiveCallback(const struct ip* ip_header, const void* buffer,
                       int length);

}  // namespace transport
}  // namespace minitcp

#endif  //! MINITCP_SRC_TRANSPORT_TCP_KERNEL_H_