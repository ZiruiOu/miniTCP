#include <arpa/inet.h>
#include <fcntl.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <thread>

#include "../common/constant.h"
#include "../common/logging.h"
#include "../transport/callbacks.h"
#include "../transport/socket.h"
#include "../transport/tcp_kernel.h"

using namespace minitcp;

int main(int argc, char *argv[]) {
  transport::startTCPKernel();

  char src_ip_str[] = "10.100.1.1";
  char dest_ip_str[] = "10.100.2.1";

  ip_t src_ip, dest_ip;
  inet_aton(src_ip_str, &src_ip);
  inet_aton(dest_ip_str, &dest_ip);

  struct sockaddr_in address;

  class transport::Socket socket;

  insertEstablish(dest_ip, src_ip, 80, 80, &socket);

  address.sin_family = AF_INET;
  address.sin_addr = src_ip;
  address.sin_port = 80;
  socket.Bind((struct sockaddr *)&address, sizeof(address));

  address.sin_addr = dest_ip;

  char buffer[1410] = {};

  int fd = open("../hello.txt", O_RDONLY);

  // MINITCP_LOG(ERROR) << " open file with fd = " << fd << " errno = " << errno
  //                    << std::endl;

  socket.Connect((struct sockaddr *)&address, sizeof(address));

  for (;;) {
    int result = read(fd, buffer, 1400);
    if (result == 0) {
      break;
    }
    buffer[result] = 0;
    socket.Write(buffer, result);
  }
  char msg_buffer[20] = "hello how are you?";
  socket.Write(msg_buffer, 20);
  socket.Close();

  while (true)
    ;
  return 0;
}