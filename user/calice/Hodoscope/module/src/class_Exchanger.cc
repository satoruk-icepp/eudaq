#include "class_Exchanger.h"
//#define binary(bit) strtol(bit,NULL,2)
#include <chrono> //for sleep 
#include <thread> //for sleep

Exchanger::Exchanger(const char* IpAddr, unsigned int tcp, unsigned int udp){
  SetIPPort(IpAddr,tcp,udp);
};
Exchanger::~Exchanger(){};

// Send 1 data & address by UDP
void Exchanger::udp_send(unsigned int address, int data){// data: send only 1 data length
  int iaddress = address;
  RBCPskeleton(0xff, 0x80, address, 0x1, iaddress);
  sndData[0] = data;
  memcpy(sndBuf+sizeof(GetsndHeader()), sndData, sizeof sndData);
  int len = 0;
  len = sendto(GetUDPSock(), sndBuf, 1 + sizeof(GetsndHeader()), 0,
              (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!
  if(len < 0){
    perror("UDP send error");
    exit(1);
  }
  rcvRBCP_ACK(1);
}


// Recv 1 data & address by UDP
int Exchanger::udp_recv(unsigned int address){//recv only 1 data length
  
  RBCPskeleton(0xff, 0xc0, address, 0x1, address);
  int len = 0;
  len = sendto(GetUDPSock(), sndBuf, 1 + sizeof(GetsndHeader()), 0,
              (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!
  if(len < 0){
    perror("UDP recv error");
    exit(1);
  }
  int rd_data = 0;
  rd_data = rcvRBCP_ACK(1);
  // std::cout << rd_data << std::endl;

  return rd_data;
}

void Exchanger::tcp_send(unsigned int data){// data: send only 1 data length
  //std::cout << std::hex << data << std::endl;
  int len = 0;
  len = send(GetTCPSock(),(char*)&data, sizeof(int), 0);
  if(len < 0){
    perror("TCP send error");
    exit(1);
  }
  return;
}

int Exchanger::tcp_recv(){// recv only 1 data length
  int len = 0;
  unsigned int buf = 0;
  //unsigned int length = 4;
  //recv(GetTCPSock(), (char*)&buf, &length,0);
  recv(GetTCPSock(), (char*)&buf, sizeof(int),0);
  if(len < 0){
    perror("TCP recv error");
    exit(1);
  }
  return buf;
}

// Send 1 packet data & address by UDP
void Exchanger::RBCPpacket_send(unsigned int address,unsigned char length, int* data){
  
  std::cout << "enter packet function" << std::endl;
  
  RBCPskeleton(0xff, 0x80, address, length, address);
  for(int i=0; i<256; ++i){
    sndData[i] = data[i];// <- sndData: 0~255
  }
  memcpy(sndBuf+sizeof(GetsndHeader()), sndData, sizeof sndData);
  
  int len = 0;
  len = sendto(GetUDPSock(), sndBuf, sizeof sndData + sizeof(GetsndHeader()), 0,
              (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!

  if(len < 0){
    perror("UDP send error");
    exit(1);
  }
  rcvRBCP_ACK(0);
}

// Recv 1 packet data & address by UDP
void Exchanger::RBCPpacket_recv(unsigned int address,unsigned char length){
  
  std::cout << "enter packet function" << std::endl;
  
  RBCPskeleton(0xff, 0xc0, address, length, address);
  int len = 0;
  len = sendto(GetUDPSock(), sndBuf, sizeof sndData + sizeof(GetsndHeader()), 0,
              (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!

  if(len < 0){
    perror("UDP recv error");
    exit(1);
  }
  rcvRBCP_ACK(1);
}


int Exchanger::WriteData(unsigned int data){
  data += 128 << 24;
  /* 
  for(int i = 31; i>-1; --i){
    unsigned int hoge = (data >> i) & 1;
    if(hoge){
       fprintf(stdout, "!");
    }else{
       fprintf(stdout, ".");
    }
    if(i%8 == 0){
       fprintf(stdout, "\n");
    }
  }
  fprintf(stdout, "\n");
  */
  //std::cout << std::hex << data << std::endl;//haru
  tcp_send(data);
  unsigned int buf = 0;
  buf = tcp_recv();
  if(data - buf){
    std::cerr << "### Data Transmit Error ###" << std::endl;
    std::cerr << "Transmited data is " << std::hex << data << std::endl;
    std::cout << "Returned data is   " << std::hex << buf << std::endl;
    return -1;
  }
  return 0;
}

int Exchanger::ReadData(unsigned int signal, unsigned int *data){
   signal += 64 << 24;
   tcp_send(signal);
   //unsigned int length = 4;
   *data = tcp_recv();
   /*    
   for(int i = 31; i>-1; --i){
      unsigned int hoge = (*data >> i) & 1;
       if(hoge){
         fprintf(stdout, "!");
       }else{
         fprintf(stdout, ".");
       }
       if(i%8 == 0){
         fprintf(stdout, "\n");
       }
   }
   fprintf(stdout, "\n");
   */
   return 0;
}


//=============== multi packet sending method ===============
int Exchanger::tcp_multi_recv(char* data_buf, unsigned int *ReadLength){
  unsigned int revd_size = 0;
  int tmp_returnVal = 0;

  while(revd_size < *ReadLength){
    tmp_returnVal = recv(GetTCPSock(), data_buf +revd_size, *ReadLength -revd_size, 0);
    if(tmp_returnVal < 0){
      int errbuf = errno;
      perror("TCP receive");
      if(errbuf == EAGAIN){
         /*
         if(EndADC == 1){
          std::cout << "No data" << std::endl;
            break;
          }
         continue;
         */
        }else if(errbuf == EINTR){
          continue;
        }else{
        return -1;
      }
    }
    revd_size += tmp_returnVal;
  }
  //std::cout << "ret is " << std::dec << revd_size << std::endl;
  return revd_size;
}

void Exchanger::RBCP_multi_packet_send(unsigned int address,unsigned int total_length, int* data){

  unsigned int total_packet = ((total_length/255)+1)*2;
  unsigned int packetNo(0);

  std::cout << "------------------ RBCP : register access ------------------" << std::endl;    
  std::cout << "" << std::endl;    
  std::cout << "Start sending packet data";    

  for(packetNo = 0; packetNo<total_packet; ++packetNo){

    //    rcvRBCP_ACK(0);
    RBCPskeleton(0xff, 0x80, address+(packetNo*255), 0xff, address+(packetNo*255));
      
    for(int i = 0; i < 255; ++i){
      sndData[i] = data[i+(packetNo*255)];// <- sndData: 0~255
    }
    
    memcpy(sndBuf+sizeof(GetsndHeader()), sndData, sizeof sndData);

    int len = 0;
    len = sendto(GetUDPSock(), sndBuf, sizeof sndData + sizeof(GetsndHeader()), 0,
                (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!

    if(len < 0){
      perror("Send DAQ Start Signal");
      exit(1);
    }

    rcvRBCP_ACK(0);
    //std::cout << rcvRBCP_ACK(1) << std::endl;

    std::cout << ".";
  }
  std::cout << "Done!!!" << std::endl;
  std::cout << "" << std::endl;

  std::cout << "Number of sent packets = " << packetNo << std::endl;
  std::cout << "" << std::endl;

}

//--------------------EASIROC MADC sender---------------------------
int Exchanger::read_madc(int data){// data: send only 1 data length
  
  //Set ADC ch to FPGA reg
  RBCPskeleton(0xff, 0x80, 0x00000012, 0x1, 0x00000012);
  sndData[0] = data;

  memcpy(sndBuf+sizeof(GetsndHeader()), sndData, sizeof sndData);
  int len = 0;
  len = sendto(GetUDPSock(), sndBuf, 1 + sizeof(GetsndHeader()), 0,
              (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!
  if(len < 0){
    perror("UDP Send error");
    exit(1);
  }
  rcvRBCP_ACK(1);
  
  RBCPskeleton(0xff, 0x80, 0x0000001f, 0x1, 0x0000001f);
  sndData[0] = 1;
  
  memcpy(sndBuf+sizeof(GetsndHeader()), sndData, sizeof sndData);
  len = 0;
  len = sendto(GetUDPSock(), sndBuf, 1 + sizeof(GetsndHeader()), 0,
              (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!
  if(len < 0){
    perror("UDP Send error");
    exit(1);
  }
  rcvRBCP_ACK(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  //usleep(20000);//wait ADC ch change
  
  //start Read ADC
  RBCPskeleton(0xff, 0x80, 0x00000012, 0x1, 0x00000012);
  sndData[0] = 240;
  
  memcpy(sndBuf+sizeof(GetsndHeader()), sndData, sizeof sndData);
  len = 0;
  len = sendto(GetUDPSock(), sndBuf, 1 + sizeof(GetsndHeader()), 0,
              (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!
  if(len < 0){
    perror("UDP Send error");
    exit(1);
  }
  rcvRBCP_ACK(1);

  RBCPskeleton(0xff, 0x80, 0x0000001f, 0x1, 0x0000001f);
  sndData[0] = 0;
  
  memcpy(sndBuf+sizeof(GetsndHeader()), sndData, sizeof sndData);
  len = 0;
  len = sendto(GetUDPSock(), sndBuf, 1 + sizeof(GetsndHeader()), 0,
              (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!
  if(len < 0){
    perror("UDP Send error");
    exit(1);
  }
  rcvRBCP_ACK(1);
  
  //read data
  int rd_data = 0;
  int rd_data1 = 0;
  int rd_data2 = 0;
  RBCPskeleton(0xff, 0xc0, 0x00000004, 0x1, 0x00000004);
  len = 0;
  len = sendto(GetUDPSock(), sndBuf, 1 + sizeof(GetsndHeader()), 0,
              (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!
  if(len < 0){
    perror("UDP recv error");
    exit(1);
  }
  rd_data1 = rcvRBCP_ACK(1);
  //std::cout << rd_data << std::endl;
  RBCPskeleton(0xff, 0xc0, 0x00000005, 0x1, 0x00000005);
  len = 0;
  len = sendto(GetUDPSock(), sndBuf, 1 + sizeof(GetsndHeader()), 0,
              (struct sockaddr *)&udpAddr, sizeof(udpAddr));//important!
  if(len < 0){
    perror("UDP recv error");
    exit(1);
  }
  rd_data2 = rcvRBCP_ACK(1);
  //std::cout << rd_data << std::endl;
  rd_data = rd_data1*256 + rd_data2;
  
  return rd_data;
}


void Exchanger::clear_all(){
  unsigned char address;
  int data;
  //int nbyte;

  // Send register clear
  address = 0x00;
  data = 0x0;
  udp_send(address, data);

  std::cout << "Clear all registers in FPGA." << std::endl;
}

