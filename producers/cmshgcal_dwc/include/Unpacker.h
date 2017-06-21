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

#include "eudaq/Logger.hh"
#include "CAEN_v1290.h"

enum CHANNEL_INDEX {
 UNDEFINED = -1,

 X1_UP = 0,
 X1_DOWN,
 Y1_UP,
 Y1_DOWN,

 X2_UP = 4,
 X2_DOWN,
 Y2_UP,
 Y2_DOWN,

 X3_UP = 8,
 X3_DOWN,
 Y3_UP,
 Y3_DOWN,

 X4_UP = 12,
 X4_DOWN,
 Y4_UP,
 Y4_DOWN
};

#define WORDSIZE 4

struct tdcData {
  int event;
  int ID;
  std::map<unsigned int, unsigned int> timeOfArrivals;
} ;

#define DEBUG_BOARD 0

class Unpacker {
  public: 
    
    tdcData ConvertTDCData(std::vector<WORD>);
    
  private:
    int Unpack (std::vector<WORD>) ;
    std::map<unsigned int, std::vector<unsigned int> > timeStamps;   //like in September 2016 test-beam: minimum of all read timestamps
    tdcData currentData;
};

#endif