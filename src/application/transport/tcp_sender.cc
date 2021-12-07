#include <arpa/inet.h>
#include <fcntl.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char dest_ip_str[] = "10.102.1.2";

  struct sockaddr_in server_addr, local_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(4096);
  inet_pton(AF_INET, dest_ip_str, &server_addr.sin_addr);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  char my_message[100] =
      "Across the Great Fie Wall we can reach any where in the world!";

  write(sockfd, my_message, strlen(my_message));

  close(sockfd);

  return 0;
}