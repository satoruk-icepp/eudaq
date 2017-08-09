#ifndef SiTCP_H
#define SiTCP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <errno.h>

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#include <winsock.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif


#define ADDR_CLR            0x00
#define ADDR_SET_CCRLCR     0x04
#define ADDR_SET_FRAME      0x08
#define ADDR_FRAME_MODE     0x0c
#define ADDR_TCP            0x10
#define ADDR_TESTMODE       0x44        
#define ADDR_SET_DAC        0x4c        

struct bcp_header{
  unsigned char type;
  unsigned char command;
  unsigned char id;
  unsigned char length;
  unsigned int address;
};

struct f_rec{
  int rlen;
  //unsigned char data[4096];
  unsigned char data[16384];
};

class SiTCP{
private:
  const char*  sitcpIpAddr;
  unsigned int tcpPort;
  unsigned int udpPort;
  int          udpsock;
#ifdef _WIN32
  SOCKET       tcpsock;
#else
  int          tcpsock;
#endif // _WIN32

  struct bcp_header  sndHeader;
  unsigned int ain_value;
  FILE         *dump_fp;
  FILE         *last_frame_fp;
  int          noDumpFile;
  int          last_frame;
  int          noLastFrameFile;
  char         dump_filename[100];
  char         last_frame_filename[100];
  int          nframe_break;
  int          iframe;
  unsigned char EOFc;

public:
  struct sockaddr_in tcpAddr;
  struct sockaddr_in udpAddr;
  fd_set             rmask, wmask, readfds;
#ifdef _WIN32
  char      sndBuf[2048]; 
#else
  unsigned char      sndBuf[2048];
#endif // _WIN32

  unsigned char      sndData[256];
  struct timeval     timeout;

  SiTCP();
  ~SiTCP();
  bool SetIPPort(const char* IpAddr, unsigned int tcp, unsigned int udp);
  bool CloseDumpFile();
  bool CloseLastFrameFile();
  bool CreateUDPSock();
  bool CloseUDPSock();
  bool CreateTCPSock();
  bool CloseTCPSock();
  void RBCPskeleton(unsigned char type, unsigned char command, unsigned char id, 
                      unsigned char length, unsigned int address);
  int  rcvRBCP_ACK(int output);
  
  int  GetTCPSock(){return tcpsock;}
  int  GetUDPSock(){return udpsock;}
  bcp_header GetsndHeader() const {return sndHeader;}
  void SetReg8(int address,int num);
  void SetReg32(int address,unsigned int data);
  //int  ReadReg32(int address);
  int  RecvClear(void);
  int  RecvFrame(void);
  void SetDCTP(unsigned int);
  void SetVREF1(unsigned int);
  void SetVREF2(unsigned int);

  void dump_intdata(unsigned char*,int) ;
  void dump_data(unsigned char*,int) ;
  void dump_sndBuf(unsigned char*,int) ;
  char fourbittoa(char) ;
  void SetNoDumpFile(void);
  void SetNoLastFrameFile(void);
  void SetDumpFileName(char *);
  void SetLastFrameFileName(char *);
  void SetNframeBreak(int,unsigned char);
  //void SetNframeBreak(int);
  //void SetNframeBreak(void);
  void countEOF(unsigned char*,int);
  int SetTCPTimeOut(int optval1, int optval2);
};
#endif
