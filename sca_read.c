#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int SLEEP_TIME = 100000;
const int N_CH = 8;
const int DIGIT = 8;
const int DATA_LEN = 76;

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

  int con = connect(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  printf("connect=%d\n", con);
  
  char data[DATA_LEN];
  data[0] = '\0';
  int data_length=0;
  printf("length=%d\n", data_length);

  int res=send(sd, "SEE", 3, 0);
  usleep(SLEEP_TIME);
  data_length=recv(sd, &data, DATA_LEN, 0);

  close(sd);
  
  if(data_length != DATA_LEN){
    printf("Output data length error: %d\n", data_length);
    printf("data: %s\n", data);
    exit(-1);
  }

  if(data[0] != 'S'){
    printf("format error! The usleep(%d) time should be longer.\n", SLEEP_TIME);
    printf("data: %s\n", data);
    exit(-1);
  }

  char sca_str[N_CH][DIGIT];
  unsigned int  sca_val[N_CH];
  
  int i;

  for(i=0; i<N_CH; i++){
    sprintf(sca_str[i], "%c%c%c%c%c%c%c%c",
	    data[i*8+1], data[i*8+2], data[i*8+3], data[i*8+4],
	    data[i*8+5], data[i*8+6], data[i*8+7], data[i*8+8]);
    sca_val[i] = atoi(sca_str[i]);
    printf("SCA %d: %08d\n", i, sca_val[i]);
  }

  
  return 0;
}
