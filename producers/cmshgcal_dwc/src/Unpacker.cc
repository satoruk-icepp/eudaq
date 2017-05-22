#include "Unpacker.h"

#define DEBUG_UNPACKER 0

#include <string>

//implementation adapted from H4DQM: https://github.com/cmsromadaq/H4DQM/blob/master/src/CAEN_V1290.cpp
//CAEN v1290 manual found in: http://www.tunl.duke.edu/documents/public/electronics/CAEN/caen_v1290.pdf
int Unpacker::Unpack (std::vector<WORD> Words) {
  for (size_t i = 0; i<Words.size(); i++) {
    uint32_t currentWord = Words[i];
    
    if (DEBUG_UNPACKER) 
      std::cout << "TDC WORD: " << currentWord <<std::endl;
    
    if (currentWord>>28 == 10 ) { //TDC BOE
      unsigned int tdcEvent= (currentWord>>5) & 0x3FFFFF; 
      
      if (DEBUG_UNPACKER) 
        std::cout << "[CAEN_V12490][Unpack] | TDC 1290 BOE: event " << tdcEvent+1 << std::endl;
      continue;
    }
    
    else if (currentWord>>28 == 0) {//TDC DATUM
      unsigned int channel = (currentWord>>21) & 0x1f;   //looks at the bits 22 - 27
      unsigned int tdcReadout = currentWord & 0x1fffff;  //looks at bits 1 - 21

      if (DEBUG_UNPACKER) 
        std::cout<<"CHANNEL = "<<channel<<std::endl;

      if (DEBUG_UNPACKER) 
        std::cout << "[CAEN_V12490][Unpack] | tdc 1290 board channel " << channel +" tdcReadout " << tdcReadout <<std::endl;
      
      //check if channel exists, if not create it
      if (timeStamps.find(channel) == timeStamps.end()) {
        timeStamps[channel] = std::vector<unsigned int>();
      }
  
      timeStamps[channel].push_back(tdcReadout);
      continue;
    }
    
    else if (currentWord>>28 == 8) { //TDC EOE 
      if (DEBUG_UNPACKER) 
        std::cout << "[CAEN_V12490][Unpack] | TDC 1290 BOE: end of event " << std::endl;
      break;
    }
   
    else{
      if (DEBUG_UNPACKER) 
        std::cout << "[CAEN_V12490][Unpack] | TDC 1290 BOE: Invalid format " << std::endl;
      break;
    }
  }

  return 0 ;
}

std::map<unsigned int, unsigned int> Unpacker::ConvertTDCData(std::vector<WORD> Words) {

  std::map<unsigned int, unsigned int> timeOfArrivals;

  for(std::map<unsigned int, std::vector<unsigned int> >::iterator channelTimeStamps = timeStamps.begin(); channelTimeStamps != timeStamps.end(); ++channelTimeStamps) {
    channelTimeStamps->second.clear();
  }
  timeStamps.clear();

  //unpack the stream
  Unpack(Words);
  
  for(std::map<unsigned int, std::vector<unsigned int> >::iterator channelTimeStamps = timeStamps.begin(); channelTimeStamps != timeStamps.end(); ++channelTimeStamps) {
    unsigned int this_channel = channelTimeStamps->first; 
    timeOfArrivals[this_channel] = -1;    //no arrival
    
    if (channelTimeStamps->second.size() > 0) {
      //std::cout<<"this_channel: "<<this_channel<<"   time of arrival: "<<*min_element(channelTimeStamps->second.begin(), channelTimeStamps->second.end())<<std::endl;
      timeOfArrivals[this_channel] = *min_element(channelTimeStamps->second.begin(), channelTimeStamps->second.end());
    }
  }

  return timeOfArrivals;
}
