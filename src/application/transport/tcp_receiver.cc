#include <arpa/inet.h>
#include <fcntl.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char server_ip[] = "10.102.1.2";

  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addrlen;

  int listenfd = socket(AF_INET, SOCK_STREAM, 0);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(4096);
  inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
  bind(listenfd, (const sockaddr *)&server_addr, sizeof(server_addr));

  listen(listenfd, 5);

  int acceptfd =
      accept(listenfd, (struct sockaddr *)&client_addr, &client_addrlen);

  char message[200] = {};
  int result = read(acceptfd, message, sizeof(message));

  printf("read %d bytes from client, which reads %s\n", result, message);

  sleep(5);
  close(listenfd);
  close(acceptfd);
}