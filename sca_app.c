#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <ncurses.h> // sudo apt install libncurses-dev

/* for kbhit func */
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

const int SLEEP_TIME = 100000; // in us
const int REFRESH_TIME = 1000000; // in us
//const int N_CH = 8;
#define N_CH  8
const int DIGIT = 8;
const int DATA_LEN = 76;
const int PORT = 10001;

char ch_name[N_CH][64];

int sca_start(int sock_num);
int sca_stop(int sock_num);
int sca_reset(int sock_num);
int sca_cnt(int sock_num, int cmd);

int sca_read(int sock_num, int *sca_val);
int show_val(int *sca_val, double *rate);
int kbhit();

volatile sig_atomic_t eflag = 0;
void abrt_handler(int sig, siginfo_t *info, void *ctx);

int main(int argc, char** argv){
  int i;
  
  /* define action when Cntl-c is given */
  struct sigaction sa_sigabrt;
  memset(&sa_sigabrt, 0, sizeof(sa_sigabrt));
  sa_sigabrt.sa_sigaction = abrt_handler;
  sa_sigabrt.sa_flags = SA_SIGINFO;

  if (sigaction(SIGINT, &sa_sigabrt, NULL) < 0 ) {
    exit(1);
  }

  /* default channel name */
  for(int i=0; i<N_CH; i++){
    sprintf(ch_name[i], "SCA %d", i);
  }

  /* read channel name def file */
  FILE* fp;
  fp = fopen("ch_name.def", "r");
  char tmp[64];
  int cnt_name=0;
  if(fp != NULL){
    while(fgets(tmp, 64, fp) != NULL){
      if(cnt_name<N_CH){
	// delete "\n" in the last character
	if(*tmp && tmp[strlen(tmp)-1]=='\n') tmp[strlen(tmp)-1] =0;
	sprintf(ch_name[cnt_name], "%s", tmp);
	//	printf("%s\n", ch_name[i]);
	cnt_name++;
      }
    }
  }

  /* count maximum ch_name length */
  int max_len = 0;
  int tmp_len;
  for(int i=0; i<N_CH; i++){
    tmp_len = strlen(ch_name[i]);
    if(tmp_len > max_len) max_len = tmp_len;
  }

  //  printf("max len=%d\n", max_len);
  
  /* set IP address */
  char ip_address[256];
  int host_flag=0;

  if(argc==2){
    if(argv[1][0]=='1'){
      sprintf(ip_address, "%s", argv[1]);
    }
    else{
      /* get IP address by hostname */
      struct sockaddr_in ip;
      struct hostent *host;
      host = gethostbyname(argv[1]);
      if( host != NULL ){
	printf("hostname: %s\n", argv[1]);
	ip.sin_addr =  *(struct in_addr *)(host->h_addr_list[0]);
	sprintf(ip_address, "%s", inet_ntoa(ip.sin_addr));
	host_flag=1;
      }
      if( host ==NULL){
	printf("Unknown hostname: %s\n", argv[1]);
	exit(1);
      }
    }
  }

  if(argc!=2){
    //    sprintf(ip_address, "192.168.253.144");  // RARiS 2025 for scinti
    sprintf(ip_address, "192.168.253.145");  // RARiS 2025 for MAIKo
  }

  printf("IP address: %s\n", ip_address);
  
  /* Prepare socket */
  struct sockaddr_in addr;
  int sd = socket(AF_INET, SOCK_STREAM, 0);
  printf("sd=%d\n", sd);
  if(sd<0){
    perror("socket");
    return -1;
  }
  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = inet_addr(ip_address);

  /* Connect to the scaler */
  int connect_res = connect(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if(connect_res <0){
    printf("TCP connection error: %d\n", connect_res);
    return -1;
  }
  
  int sca_val[N_CH];
  int old_sca_val[N_CH];  

  for(i=0; i<N_CH; i++){
    sca_val[i] = 0;
    old_sca_val[i] = 0;
  }

  double rate[N_CH];
  
  /* Init ncurses */
  initscr();

  /* show ip address */
  mvprintw(0, 2, "IP: %s", ip_address);
  if(host_flag==1){
    mvprintw(0, 22, "host: %s", argv[1]);
  }

  
  int cnt=0;
  int kbres=0;
  while(!eflag){

    /* start, stop, or reset */
    kbres=kbhit();
    if(kbres>0){
      sca_cnt(sd, kbres);
    }
    
    /* read scaler values */
    sca_read(sd, sca_val);

    /* calcurate rate */
    for(i=0; i<N_CH; i++){
      rate[i] = (sca_val[i] - old_sca_val[i])/(REFRESH_TIME/1000000.0);
      if(rate[i]<0) rate[i]=0;
      old_sca_val[i] = sca_val[i];
    }
    
    /* show scaler values */
    show_val(sca_val, rate);

    usleep(REFRESH_TIME - SLEEP_TIME);
  }

  /* Close the soclet */
  close(sd);  

  /* End ncurses */
  endwin();

  return 0;
}

/* scaler control */
int sca_cnt(int sock_num, int cmd){
  int res;

  if(cmd==1){
    send(sock_num, "S1E", 3, 0);
    usleep(SLEEP_TIME);  
  }

  if(cmd==2){
    send(sock_num, "S2E", 3, 0);
    usleep(SLEEP_TIME);  
  }

  if(cmd==3){
    send(sock_num, "S3E", 3, 0);
    usleep(SLEEP_TIME);  
  }
  
  return 0;
}

int sca_start(int sock_num){
  int send_res = send(sock_num, "S1E", 3, 0);
  usleep(SLEEP_TIME);  
  return 0;
}

int sca_stop(int sock_num){
  int send_res = send(sock_num, "S2E", 3, 0);
  usleep(SLEEP_TIME);  
  return 0;
}

int sca_reset(int sock_num){
  int send_res = send(sock_num, "S3E", 3, 0);
  usleep(SLEEP_TIME);  
  return 0;
}


int sca_read(int sock_num, int *sca_val){
  char data[DATA_LEN];

  int send_res = send(sock_num, "SEE", 3, 0);
  usleep(SLEEP_TIME);
  int data_length=recv(sock_num, &data, DATA_LEN, 0);

  //  printf("res=%d\n", send_res);
  //  printf("length=%d\n", data_length);
  //  printf("%s\n", data);

  int good_flag=1;
  
  if(data_length != DATA_LEN){
    printf("Output data length error: %d\n", data_length);
    printf("data: %s\n", data);
    good_flag=0;
  }

  if(data[0] != 'S'){
    printf("format error! The usleep(%d) time should be longer.\n", SLEEP_TIME);
    printf("data: %s\n", data);
    good_flag=0;
  }

  char sca_str[N_CH][DIGIT];
  int i;

  if(good_flag==0){
    for(i=0; i<N_CH; i++){
      sca_val[i]=77777777;
    }
    return -1;
  }
  
  if(good_flag==1){
    for(i=0; i<N_CH; i++){
      sprintf(sca_str[i], "%c%c%c%c%c%c%c%c",
	      data[i*8+1], data[i*8+2], data[i*8+3], data[i*8+4],
	      data[i*8+5], data[i*8+6], data[i*8+7], data[i*8+8]);
      sca_val[i] = atoi(sca_str[i]);
      //      printf("SCA %d: %08d\n", i, sca_val[i]);
    }    
  }

  return 1;
}


int show_val(int *sca_val, double *rate){
  int i;
  char date[64];
  time_t t = time(NULL);;

  clear(); // clear the ncurses

  strftime(date, sizeof(date), "%Y/%m/%d %a %H:%M:%S", localtime(&t));
  mvprintw(1, 2, "1:Start,  2:Stop,  3:Reset,  Ctrl-c:Exit \n");
  mvprintw(2, 2, "%s \n", date);  
  mvprintw(3, 2, "---------------------------------------------------\n");  
  for(i=0; i<N_CH; i++){
    //    mvprintw(4+i,  2, "SCA %d: %08d", i, sca_val[i]);
    //    mvprintw(4+i, 20, "%10.1f Hz\n", rate[i]);    
    mvprintw(4+i,  2,  "SCA %d (%s)", i, ch_name[i]);
    mvprintw(4+i, 30, "%08d", sca_val[i]);        
    mvprintw(4+i, 40, "%10.1f Hz\n", rate[i]);    
  }
  refresh();
  return 0;
}


void abrt_handler(int sig, siginfo_t *info, void *ctx) {
  printf("\n");
  printf("Interrupt is detected!\n");
  printf("Closing the socket...\n");
  printf("\n");
  eflag=1;
}


/* get whether keyboard input or not */
int kbhit(){
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
      /* get only '1' or '2' or '3' */
      if(ch==49 || ch==50 || ch==51) return (ch-48);

      else return 0;
    }

    return 0;
}
