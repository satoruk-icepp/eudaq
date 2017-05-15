#ifndef CAEN_V1290_H
#define CAEN_V1290_H

#include <fstream>
#include <iostream>
#include <string>

#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <random>
#include <algorithm> 


#define WORDSIZE 4

struct tdcData {
  int ID;
  unsigned int timeStamp_ch0;
  unsigned int timeStamp_ch1;
  unsigned int timeStamp_ch2;
  unsigned int timeStamp_ch3;
} ;

#define DEBUG_BOARD 0

class CAEN_v1290 {
  public: 
    CAEN_v1290 (int _ID){
      ID = _ID;
    };
    
    void ReadoutTDC();
    tdcData GetCurrentData();
    
  private:
    int Unpack (std::ifstream &stream) ;
    int ID;
    tdcData currentData;
};

#endif