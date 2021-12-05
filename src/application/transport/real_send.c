#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  int fd2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  return 0;
}