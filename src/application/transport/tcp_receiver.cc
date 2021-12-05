#include <arpa/inet.h>
#include <net/ethernet.h>
#include <stdio.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <thread>

#include "../common/constant.h"
#include "../common/logging.h"
#include "../common/types.h"
#include "../transport/callbacks.h"
#include "../transport/socket.h"
#include "../transport/tcp_kernel.h"

using namespace minitcp;

int main(int argc, char *argv[]) {
  transport::startTCPKernel();

  char src_ip_str[] = "10.100.2.1";
  char dest_ip_str[] = "10.100.1.1";

  ip_t src_ip, dest_ip;
  inet_aton(src_ip_str, &src_ip);
  inet_aton(dest_ip_str, &dest_ip);

  struct sockaddr_in address;

  class transport::Socket socket;

  insertListen(src_ip, 80, &socket);

  // if (!transport::findSocket(key)) {
  //     MINITCP_LOG(ERROR) << " socket is not inserted into the socket map."
  //<< std::endl;
  //}

  address.sin_family = AF_INET;
  address.sin_addr = src_ip;
  address.sin_port = 80;
  socket.Bind((struct sockaddr *)&address, sizeof(address));

  address.sin_addr = dest_ip;

  socket.Listen(10);

  struct sockaddr_in accept_address;
  socklen_t socket_length;

  class transport::Socket *new_socket =
      socket.Accept((struct sockaddr *)&accept_address, &socket_length);

  // std::this_thread::sleep_for(std::chrono::seconds(20));

  // auto new_socket = socket.Accept();

  // char text[100] = {};
  // new_socket->Read(text, 100);

  MINITCP_LOG(INFO) << "ok : set up " << std::endl;

  char text[1410] = {};
  for (;;) {
    MINITCP_LOG(DEBUG) << "start Reading from the established socket."
                       << std::endl;
    ssize_t bytes = new_socket->Read(text, 1400);
    MINITCP_LOG(DEBUG) << "ok : " << std::endl;
    text[bytes] = 0;
    std::cout << text << std::endl;
  }

  MINITCP_LOG(INFO) << "nice boat! " << std::endl;

  while (true)
    ;
  return 0;
}