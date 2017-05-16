#ifndef CAEN_V1290_H
#define CAEN_V1290_H

#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>

#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <random>
#include <algorithm> 


#define WORDSIZE 4

struct tdcData {
  int event;
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
      currentData.ID = _ID;
      currentData.event = 0;    //will be replaced by the true values as on the TDC
      for (unsigned int ch=0; ch<=3; ch++) channelTimeStamps[ch] = std::vector<unsigned int>();
    };
    
    void ReadoutTDC();
    tdcData GetCurrentData();
    
  private:
    int Unpack (std::ifstream &stream) ;
    void generatePseudoData();
    std::map<unsigned int, std::vector<unsigned int> > channelTimeStamps;
    tdcData currentData;      //to be passed to the producer
};

#endif