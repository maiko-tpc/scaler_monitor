#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


int main(int argc, char** argv){
  int sd;
  struct sockaddr_in addr;
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(10001);
  addr.sin_addr.s_addr = inet_addr("172.16.211.102");

  connect(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

  if(send(sd, "S2E", 3, 0) < 0) {
    perror("send");
    return -1;
  }
  
  close(sd);
  
  return 0;
}
