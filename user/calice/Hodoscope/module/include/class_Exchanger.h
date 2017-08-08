//#pragma once
#ifndef _CLASSEXCHANGER_H_
#define _CLASSEXCHANGER_H_

#include "SiTCP.h"

class Exchanger:public SiTCP{

public:
  Exchanger(const char* IpAddr, unsigned int tcp, unsigned int udp);
  ~Exchanger();

  void udp_send(unsigned int address, int data);//send only 1 data length
  int udp_recv(unsigned int address);//recv only 1 data length
  void tcp_send(unsigned int data);//send only 1 data length
  int tcp_recv();//recv only 1 data length

  int WriteData(unsigned int data);
  int ReadData(unsigned int signal, unsigned int *data);
  int tcp_multi_recv(char* data_buf, unsigned int *ReadLength);
  void RBCPpacket_send(unsigned int address, unsigned char length, int* data);
  void RBCPpacket_recv(unsigned int address, unsigned char length);
  void RBCP_multi_packet_send(unsigned int address,unsigned int total_length, int* data);
  int read_madc(int data);
  void clear_all();
};

#endif
