#include "CAEN_v1290.h"

#define DEBUG_UNPACKER 1


//implementation adapted from H4DQM: https://github.com/cmsromadaq/H4DQM/blob/master/src/CAEN_V1290.cpp
//CAEN v1290 manual found in: http://www.tunl.duke.edu/documents/public/electronics/CAEN/caen_v1290.pdf

int CAEN_V1290::Unpack (std::ifstream &stream) {
  while (true) {
    unsigned int tdcRawData;
    std::cout<<tdcRawData<<std::endl;
    stream.read((char*)&tdcRawData, sizeof (tdcRawData));
    if (DEBUG_UNPACKER) 
      EUDAQ_INFO("TDC WORD: " + std::to_string(tdcRawData));
    
    if (tdcRawData>>28 == 10 ) { //TDC BOE
      unsigned int tdcEvent=tdcRawData & 0xFFFFFFF;   //looks at the first 28 bits
      if (DEBUG_UNPACKER) 
        EUDAQ_INFO("[CAEN_V12490][Unpack]       | TDC 1190 BOE: event " + std::to_string(tdcEvent+1));
    }

    else if (tdcRawData>>28 == 8) //TDC EOE
      break;
    

    else if (tdcRawData>>28 == 0) {//TDC DATUM
      tdcData thisData;
      thisData.channel = (tdcRawData>>21) & 0x1f;   //looks at the bits 22 - 27
      thisData.tdcReadout = tdcRawData & 0x1fffff;  //looks at bits 1 - 21
      if (DEBUG_UNPACKER) 
        EUDAQ_INFO("[CAEN_V12490][Unpack]       | tdc 1190 board " + std::to_string(thisData.board) + " channel " + std::to_string(thisData.channel) + " tdcReadout " + std::to_string(thisData.tdcReadout));
      
    //Todo: pass the data to the event
    }
    break;
  }
  return 0 ;
}
