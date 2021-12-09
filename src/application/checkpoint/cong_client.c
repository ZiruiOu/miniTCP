//
// Created by Chengke on 2019/10/22.
// Modified by Chengke on 2021/08/26.
//

#include <sys/time.h>

#include "unp.h"

const char* message =
    "hello\n"
    "world\n\n\n"
    "Looooooooooooooooooooooooooooooooooooooooooooong\n"
    "what\na\nt\ne\nr\nr\ni\nb\nl\ne\n"
    "\n\n\n";

double timeval_subtract(struct timeval* x, struct timeval* y) {
  double diff = x->tv_sec - y->tv_sec;
  diff += (x->tv_usec - y->tv_usec) / 1000000.0;
  return diff;
}

#define MSG_LEN 150000
char message_buf[MSG_LEN];

void populate_buf() {
  int i;
  int message_len = strlen(message);
  memcpy(message_buf, message, message_len);
  i = message_len;
  while (i + 1 < MSG_LEN) {
    message_buf[i] = 'a' + (i % 26);
    i += 1;
  }
  message_buf[i] = '\n';
}

void str_cli(FILE* fp, int sockfd, int sleep_) {
  char sendline[MAXLINE];
  char recvline[MAXLINE];
  while (fgets(sendline, MAXLINE, fp) != NULL) {
    writen(sockfd, sendline, strlen(sendline));

    if (readline(sockfd, recvline, MAXLINE) == 0) {
      printf("str_cli: server terminated prematurely\n");
      exit(1);
    }

    // fputs(recvline, stdout);
  }
}

void cli_client(const char* addr) {
  int sockfd;
  struct sockaddr_in servaddr;
  FILE* fp;
  struct timeval start_time, end_time;

  sockfd = Socket(AF_INET, SOCK_STREAM, 0);
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(10086);
  Inet_pton(AF_INET, addr, &servaddr.sin_addr);

  Connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

  for (int i = 0; i < 100000; i++) {
    populate_buf();
    fp = fmemopen(message_buf, MSG_LEN, "r");

    gettimeofday(&start_time, NULL);
    str_cli(fp, sockfd, i == 0);
    gettimeofday(&end_time, NULL);

    double time_elapse, rate;
    time_elapse = timeval_subtract(&end_time, &start_time);
    rate = MSG_LEN * 1.0 / time_elapse;

    printf("current rate = %.4lf KBps\n", rate / 1000.0);

    fclose(fp);
  }

  close(sockfd);
}

int main(int argc, char* argv[]) {
  int loop;

  if (argc != 2) {
    printf("usage: %s <IPaddress>\n", argv[0]);
    return -1;
  }

  cli_client(argv[1]);

  printf("client sending complete! client is exiting...");

  return 0;
}