#ifndef CAEN_V1290_H
#define CAEN_V1290_H

#include <fstream>
#include <iostream>
#include <string>

#include "eudaq/Logger.hh"

#define WORDSIZE 4

struct tdcData {
  unsigned int board ;
  unsigned int channel ;
  unsigned int tdcReadout ;
} ;

#define DEBUG_BOARD 0

class CAEN_V1290 {
  public: 
    CAEN_V1290 (){};
    int Unpack (std::ifstream &stream) ;
};

#endif