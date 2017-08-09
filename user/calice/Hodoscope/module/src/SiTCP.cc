#include "SiTCP.h"
//#include "SiTCP_utl.h"
#include <chrono> //for sleep 
#include <thread> //for sleep

SiTCP::SiTCP(){
  std::cout<<"#SiTCP control --> Start."<<std::endl;
  noDumpFile = 1;
  noLastFrameFile = 1;
  strcpy(dump_filename ,"dump_file");
  iframe = 0;
  nframe_break = 0;
  last_frame = 0;
  EOFc = 0x82;
};

SiTCP::~SiTCP(){};

bool SiTCP::SetIPPort(const char* IpAddr, unsigned int tcp, unsigned int udp){
  sitcpIpAddr = IpAddr;
  if(tcp<=0 || 65535<=tcp || udp<=0 || 65535<=udp){
    if(tcp<=0 || 65535<=tcp){
      std::cout<<"error : invalid port : tcp = "<<tcp<<std::endl;
    }
    if(udp<=0 || 65535<=udp){
      std::cout<<"error : invalid port : udp = "<<udp<<std::endl;
    }
    exit(1);
  }
  tcpPort = tcp;
  udpPort = udp;
  puts("");
  std::cout<<"#SiTCP:IP Address = "<<sitcpIpAddr<<std::endl;
  std::cout<<"#      TCP Port   = "<<tcpPort<<std::endl;
  std::cout<<"#      UDP Port   = "<<udpPort<<std::endl;
  puts("");

  if(noDumpFile==0){
    if((dump_fp = fopen(dump_filename,"wb")) == NULL) {
      std::cout<<"dump_file open error !!"<<std::endl;
      return false;
    }
    std::cout<<"#SiTCP dump file opened"<<std::endl;
    puts("");
  }

  if(noLastFrameFile==0){
    if((last_frame_fp = fopen(last_frame_filename,"wb")) == NULL) {
      std::cout<<"last_frame_file open error !!"<<std::endl;
      return false;
    }
    std::cout<<"#SiTCP last frame file opened"<<std::endl;
    puts("");
  }

  return true;
}

bool SiTCP::CreateTCPSock(){
  std::cout<<"#Create socket for TCP...";

#ifdef _WIN32
  WSADATA wsaData;
  int wsaRet = WSAStartup(MAKEWORD(2, 2), &wsaData); //initialize winsocks 2.2
  if (wsaRet) { std::cout << "ERROR: WSA init failed with code " << wsaRet << std::endl; return false; }
  std::cout << "DEBUG: WSAinit OK" << std::endl;

  tcpsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (tcpsock < 0) {
	  perror("TCP socket");
	  std::cout << "errno = " << errno << std::endl;
	  WSACleanup;
	  exit(1);
  }
  memset(&tcpAddr, 0, sizeof(tcpAddr));
  tcpAddr.sin_family = AF_INET;
  tcpAddr.sin_port = htons(tcpPort);
  tcpAddr.sin_addr.s_addr = inet_addr(sitcpIpAddr);
  std::cout << "  Done" << std::endl;

  std::cout << "#  ->Trying to connect to " << sitcpIpAddr << " ..." << std::endl;
  if (connect(tcpsock, (struct sockaddr *)&tcpAddr, sizeof(tcpAddr)) < 0) {
	  if (errno != EINPROGRESS) perror("TCP connection");
	  FD_ZERO(&rmask);
	  FD_SET(tcpsock, &rmask);
	  wmask = rmask;
	  timeout.tv_sec = 3;
	  timeout.tv_usec = 0;

	  int rc = select(tcpsock + 1, &rmask, NULL, NULL, &timeout);
	  if (rc<0) perror("connect-select error");
	  if (rc == 0) {
		  puts("\n     =====time out !=====\n ");
		  exit(1);
	  }
	  else {
		  puts("\n     ===connection error===\n ");
		  exit(1);
	  }
  }
  FD_ZERO(&readfds);
  FD_SET(tcpsock, &readfds);
  FD_SET(0, &readfds);

  puts("#  ->Connect success!\n");

  return true;
#else
  tcpsock = socket(AF_INET, SOCK_STREAM, 0);
  if(tcpsock < 0){
    perror("TCP socket");
    std::cout<<"errno = "<<errno<<std::endl;
    exit(1);
  }
  memset(&tcpAddr, 0, sizeof(tcpAddr));
  tcpAddr.sin_family      = AF_INET;
  tcpAddr.sin_port        = htons(tcpPort);
  tcpAddr.sin_addr.s_addr = inet_addr(sitcpIpAddr);
  std::cout<<"  Done"<<std::endl;

  std::cout<<"#  ->Trying to connect to "<<sitcpIpAddr<<" ..."<<std::endl;
  if(connect(tcpsock, (struct sockaddr *)&tcpAddr, sizeof(tcpAddr)) < 0){
    if(errno != EINPROGRESS) perror("TCP connection");
    FD_ZERO(&rmask);
    FD_SET(tcpsock, &rmask);
    wmask = rmask;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    int rc = select(tcpsock+1, &rmask, NULL, NULL, &timeout);
    if(rc<0) perror("connect-select error");
    if(rc==0){
      puts("\n     =====time out !=====\n ");
      exit(1);
    }
    else{
      puts("\n     ===connection error===\n ");
      exit(1);
    }
  }
  FD_ZERO(&readfds);
  FD_SET(tcpsock, &readfds);
  FD_SET(0, &readfds);

  puts("#  ->Connect success!\n");

  return true;
#endif // _WIN32

}

bool SiTCP::CreateUDPSock(){
  std::cout<<"#Create socket for RBCP...";
  udpsock = socket(AF_INET, SOCK_DGRAM, 0);
  if(udpsock < 0){
    perror("UDP socket");
    std::cout<<"errno = "<<errno<<std::endl;
    exit(1);
  }
  memset(&udpAddr, 0, sizeof(udpAddr));
  udpAddr.sin_family      = AF_INET;
  udpAddr.sin_port        = htons(udpPort);
  udpAddr.sin_addr.s_addr = inet_addr(sitcpIpAddr);
  std::cout<<"  Done"<<std::endl;
  return true;
}

bool SiTCP::CloseDumpFile(){
  std::cout<<"#Close dump file...";
  fclose(dump_fp);
  std::cout<<"  Done"<<std::endl;
  return true;
}

bool SiTCP::CloseLastFrameFile(){
  std::cout<<"#Close last frame file...";
  fclose(last_frame_fp);
  std::cout<<"  Done"<<std::endl;
  return true;
}

bool SiTCP::CloseUDPSock(){
  std::cout<<"#Close UDP Socket...";
  close(udpsock);
  std::cout<<"  Done"<<std::endl;
  return true;
}

bool SiTCP::CloseTCPSock(){
  std::cout<<"#Close TCP Socket...";
#ifdef _WIN32
  closesocket(tcpsock);
  WSACleanup;
#else
  close(tcpsock);
#endif // _WIN32

  std::cout<<"  Done"<<std::endl;
  return true;
}

void SiTCP::RBCPskeleton(unsigned char type, unsigned char command, unsigned char id, unsigned char length, unsigned int address){
  sndHeader.type=type;
  sndHeader.command=command;
  sndHeader.id=id;
  sndHeader.length=length;
  sndHeader.address=htonl(address);
  memcpy(sndBuf, &sndHeader, sizeof(sndHeader));
}

int SiTCP::rcvRBCP_ACK(int output){
  fd_set setSelect;
  int rcvdBytes;
  unsigned char rcvdBuf[1024];

  int rd_data;

  //  puts("\nWait to receive the ACK packet...");

  int recvVal = 1;

  while(recvVal){

    FD_ZERO(&setSelect);
    FD_SET(udpsock, &setSelect);

    timeout.tv_sec  = 3;
    timeout.tv_usec = 0;

    if(select(udpsock+1, &setSelect, NULL, NULL,&timeout)==0){
      /* time out */
      recvVal = 0;
      puts("\n***** Timeout ! *****");
      return -1;
    }
    else {
      /* receive packet */
      if(FD_ISSET(udpsock,&setSelect)){
        rcvdBytes=recvfrom(udpsock, (char *)rcvdBuf, 2048, 0, NULL, NULL);
        rcvdBuf[rcvdBytes]=0;
        if(output == 1){
          //puts("\n***** A pacekt is received ! *****.");
          //puts("Received data:");

          int j=0;
          for(int i=0; i<rcvdBytes; i++){
            if(j==0) {
              //printf("\t[%.3x]:%.2x ",i,rcvdBuf[i]);
              j++;
            }else if(j==3){
              //printf("%.2x \n",rcvdBuf[i]);
              j=0;
            }else{
              //printf("%.2x ",rcvdBuf[i]);
              j++;
            }
          }
          //puts("\n");
          rd_data = rcvdBuf[8];

        }
        recvVal = 0;
      }
    }
  }
  // printf("%.2x",rd_data);
  return rd_data;
  //usleep(50);
}

void SiTCP::SetReg8(int address,int num){
  if(time<0 || 255<num){
    std::cout<<"Test Number should be 0-255*40ns"<<std::endl;
    exit(1);
  }
  RBCPskeleton(0xff, 0x80, 0x1, 0x1, (char)address);
  sndData[0] = num;
  memcpy(sndBuf+sizeof(GetsndHeader()), sndData, 1);
  //dump_sndBuf(sndBuf,16);

  int len = 0;
  len = sendto(GetUDPSock(), (char *)sndBuf, 1 + sizeof(GetsndHeader()), 0, (struct sockaddr *)&udpAddr, sizeof(udpAddr));
  if(len < 0){
    perror("Test CLK");
    exit(1);
  }
  rcvRBCP_ACK(0);
}

void SiTCP::SetReg32(int address,unsigned int data){
  int num;
  num = data >> 24 ;
  SetReg8(address,num);
  address++;
  num = (data & 0x00ff0000) >> 16 ;
  SetReg8(address,num);
  address++;
  num = (data & 0x00ff00) >> 8 ;
  SetReg8(address,num);
  address++;
  num = (data & 0x000000ff) ;
  SetReg8(address,num);
}

/*
void SiTCP::ReadReg32(int address){
if(address<0 || 0x70<address){
std::cout<<"address should be 0x0~0x6c"<<std::endl;
exit(1);
}
RBCPskeleton(0xff, 0x80, 0x1, 0x1, (char)address);
sndData[0] = num;
memcpy(sndBuf+sizeof(GetsndHeader()), sndData, 1);
//dump_sndBuf(sndBuf,16);

int len = 0;
len = sendto(GetUDPSock(), sndBuf, 1 + sizeof(GetsndHeader()), 0, (struct sockaddr *)&udpAddr, sizeof(udpAddr));
if(len < 0){
perror("Test CLK");
exit(1);
}
rcvRBCP_ACK(4);
}
*/

int SiTCP::RecvClear(void){
  int sitcp_fd = -1;
  fd_set fds;
  struct timeval tv;
  //unsigned char data[16];
  //unsigned char data[256];
  //unsigned char data[1024];
  unsigned char data[4096];
  //unsigned char data[16384];
  int rlen,ret,ntimeout;

  sitcp_fd = SiTCP::GetTCPSock();
  ntimeout = 0;

  while(1) {
    FD_ZERO(&fds);
    FD_SET(sitcp_fd, &fds);
    tv.tv_sec  = 1;
    tv.tv_usec = 0;

    ret = select(sitcp_fd + 1,&fds,NULL,NULL,&tv);
    if(ret < 0){
      if(errno == EINTR) continue ;
      else goto error;
    } else if(ret == 0) {
      //timeout
      ntimeout++;
      std::cout<<"#timeout "<<ntimeout<<" clear buffer"<<std::endl;
      ////break;
      if(ntimeout>1) break;

    } else if(FD_ISSET(sitcp_fd,&fds)) {
      ntimeout = 0;
      //rlen = recv(sitcp_fd, data, sizeof(data), MSG_WAITALL);
#ifdef _WIN32
	  rlen = recv(sitcp_fd, (char*)data, sizeof(data), NULL);
#else
	  rlen = recv(sitcp_fd, (char*)data, sizeof(data), MSG_DONTWAIT);
#endif // _WIN32

      if(rlen < 0){
        perror("Receive data.");
        return 1;
      }
      //dump_data(data,rlen);
      //dump_intdata(data,rlen);
      ////if(rlen < 16) break;
      //if(rlen < 16) ;

    } else ;
  }
  return 0;
  error:
  std::cout<<"select error: ret="<<ret<<std::endl;
  return 1;
}

int SiTCP::RecvFrame(void){
  int sitcp_fd = -1;
  fd_set fds;
  struct timeval tv;
  struct f_rec   recx;
  //unsigned char data[16];
  //unsigned char data[256];
  //unsigned char data[1024];
  unsigned char data[4096];
  //unsigned char data[16384];
  int rlen,ret,ntimeout;

  std::cout<<"#RecvFrame() called "<<std::endl;

  sitcp_fd = SiTCP::GetTCPSock();
  ntimeout = 0;

  while(1) {
    FD_ZERO(&fds);
    FD_SET(sitcp_fd, &fds);
    //tv.tv_sec  = 1;
    tv.tv_sec  = 4;
    tv.tv_usec = 0;

    ret = select(sitcp_fd + 1,&fds,NULL,NULL,&tv);
    if(ret < 0){
      if(errno == EINTR) continue ;
      else goto error;
    } else if(ret == 0) {
      //timeout
      ntimeout++;
      std::cout<<"#timeout "<<ntimeout<<"times"<<std::endl;
      ////break;
      if(ntimeout>8) break;

    } else if(FD_ISSET(sitcp_fd,&fds)) {
      ntimeout = 0;
      //rlen = recv(sitcp_fd, data, sizeof(data), MSG_WAITALL);
      //rlen = recv(sitcp_fd, data, sizeof(data), MSG_DONTWAIT);
#ifdef _WIN32
	  recx.rlen = recv(sitcp_fd, (char *) recx.data, sizeof(recx.data), NULL);
#else
	  recx.rlen = recv(sitcp_fd, recx.data, sizeof(recx.data), MSG_DONTWAIT);
#endif // _WIN32


      //if(rlen < 0){
      if(recx.rlen < 0){
        perror("Receive data.");
        return 1;
      }

      if(noDumpFile){
        //dump_data(data,rlen);
        SiTCP::dump_data(recx.data,recx.rlen);
      } else {
        //if(fwrite(&recx,sizeof(struct f_rec),1,dump_fp) != 1) {//20150329 error corrected.
        if(fwrite(recx.data,(size_t)1,recx.rlen,dump_fp) != recx.rlen) {
          std::cout<<"dump file fwrite error !!"<<std::endl;
          return -1;
        }
      }

      if(noLastFrameFile==0 && last_frame==1){
        if(fwrite(recx.data,(size_t)1,recx.rlen,last_frame_fp) != recx.rlen) {
          std::cout<<"last frame file fwrite error !!"<<std::endl;
          return -1;
        }
      }


      SiTCP::countEOF(recx.data,recx.rlen);

      if(SiTCP::iframe==(SiTCP::nframe_break - 1) && SiTCP::nframe_break >0){
        last_frame = 1;
        //fwrite rest of data
      }

      if(SiTCP::iframe>=SiTCP::nframe_break && SiTCP::nframe_break >0) return 0;

      //if(rlen < 16) ;

    } else ;
  }
  return 0;
  error:
  std::cout<<"select error: ret="<<ret<<std::endl;
  return 1;
}

void SiTCP::SetDCTP(unsigned int dctp_val){
  unsigned int dac_param;
  dac_param = 0x1000 | (dctp_val&0x0fff); // 0001_xxxx_xxxx_xxxx
  SiTCP::SetReg32(ADDR_SET_DAC,dac_param);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //usleep(100000);                          //wait 100msec

  ain_value = dctp_val; //for debug purpose
  printf("#dctp_val=%04x\n",dctp_val);

}

void SiTCP::SetVREF1(unsigned int vref1_val){
  unsigned int dac_param;
  dac_param = 0x5000 | (vref1_val&0x0fff); // 0101_xxxx_xxxx_xxxx
  SiTCP::SetReg32(ADDR_SET_DAC,dac_param);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //usleep(100000);                          //wait 100msec

  printf("#vref1_val=%04x\n",vref1_val);

}

void SiTCP::SetVREF2(unsigned int vref2_val){
  unsigned int dac_param;
  dac_param = 0x9000 | (vref2_val&0x0fff); // 1001_xxxx_xxxx_xxxx
  SiTCP::SetReg32(ADDR_SET_DAC,dac_param);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //usleep(100000);                          //wait 100msec

  printf("#vref2_val=%04x\n",vref2_val);

}


void SiTCP::dump_intdata(unsigned char *sndBuf, int len){
  int ii;
  for(ii=0;ii<len;ii++){
    printf("%03d ",sndBuf[ii]);
  }
  printf("\n");
}

void SiTCP::dump_data(unsigned char *sndBuf, int len){
  int ii;
  int ihex;
  for(ii=0;ii<len;ii++){
    ihex = sndBuf[ii]/16 ;
    putchar(fourbittoa(ihex)) ;
    ihex = sndBuf[ii]%16 ;
    putchar(fourbittoa(ihex)) ;
    putchar(' ');
    //putchar('\n');
    sndBuf[ii] = 0 ;
  }
  putchar('\n');
}

void SiTCP::dump_sndBuf(unsigned char *sndBuf, int len){
  int ii;
  int ihex;
  puts("sndBuf=");
  for(ii=0;ii<len;ii++){
    ihex = sndBuf[ii]/16 ;
    putchar(fourbittoa(ihex)) ;
    ihex = sndBuf[ii]%16 ;
    putchar(fourbittoa(ihex)) ;
    putchar(' ');
  }
  putchar('\n');
  putchar('\n');
}

char SiTCP::fourbittoa(char ihex){
  char ahex ;
  switch(ihex){
    case  0: ahex = '0';break;
    case  1: ahex = '1';break;
    case  2: ahex = '2';break;
    case  3: ahex = '3';break;
    case  4: ahex = '4';break;
    case  5: ahex = '5';break;
    case  6: ahex = '6';break;
    case  7: ahex = '7';break;
    case  8: ahex = '8';break;
    case  9: ahex = '9';break;
    case  10: ahex = 'a';break;
    case  11: ahex = 'b';break;
    case  12: ahex = 'c';break;
    case  13: ahex = 'd';break;
    case  14: ahex = 'e';break;
    case  15: ahex = 'f';break;
    default : ahex = 'x';break;
  }
  return ahex ;
}

void SiTCP::SetNoDumpFile(void){

  noDumpFile = 1;
  return;
}

void SiTCP::SetNoLastFrameFile(void){

  noLastFrameFile = 1;
  return;
}

void SiTCP::SetDumpFileName(char *arg){

  strcpy(dump_filename,arg+3);
  printf("#dump_filename=%s\n",dump_filename);
  noDumpFile = 0;

  return;
}

void SiTCP::SetLastFrameFileName(char *arg){

  strcpy(last_frame_filename,arg+3);
  printf("#last_frame_filename=%s\n",last_frame_filename);
  noLastFrameFile = 0;

  return;
}

void SiTCP::SetNframeBreak(int nn,unsigned char EOFcode){
  printf("#SetNframeBreak() called. ");
  SiTCP::nframe_break = nn;
  SiTCP::iframe = 0;
  SiTCP::EOFc = EOFcode;
  printf(" nframe_break=%d, EOCc=%2x\n",SiTCP::nframe_break,SiTCP::EOFc);
}

void SiTCP::countEOF(unsigned char *sndBuf, int len){
  int ii;
  for(ii=0;ii<len;ii++){
    //printf("%2x\n",sndBuf[ii]);
    if(sndBuf[ii]==SiTCP::EOFc){
      SiTCP::iframe++;
      printf("\n#detect EOF, iframe=%d\n",SiTCP::iframe);
    }
    sndBuf[ii] = 0 ;
  }
}

int SiTCP::SetTCPTimeOut(int optval1, int optval2){
  struct timeval tv;
  tv.tv_sec  = optval1;
  tv.tv_usec = optval2;
  setsockopt(tcpsock,SOL_SOCKET,SO_RCVTIMEO,(char*)&tv, sizeof(tv));
  int flag = 1;
  setsockopt(tcpsock, IPPROTO_TCP,TCP_NODELAY, (char*)&flag, sizeof(flag));
  return 0;
}
