#ifndef UNPACKER_H
#define UNPACKER_H

#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <vector>
#include <algorithm> 

#include "CAEN_V1290.hpp"


#define WORDSIZE 4

struct tdcData {
  int event;
  int ID;
  std::map<unsigned int, unsigned int> timeOfArrivals;
} ;

#define DEBUG_BOARD 0

class Unpacker {
  public: 
    Unpacker (int _ID){
      currentData.ID = _ID;
      currentData.event = 0;    //is replaced by the true values as on the TDC
    };
    tdcData ConvertTDCData(std::vector<WORD>);
    
  private:
    int Unpack (std::vector<WORD>) ;
    std::map<unsigned int, std::vector<unsigned int> > timeStamps;   //like in September 2016 test-beam: minimum of all read timestamps
    tdcData currentData;
};

#endif