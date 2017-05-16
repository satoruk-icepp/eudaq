#include "CAEN_v1290.h"

#define DEBUG_UNPACKER 0

std::default_random_engine generator;
std::normal_distribution<double> delay(20.0,1.0);
std::normal_distribution<double> mean_ch0(600., 10.);
std::normal_distribution<double> mean_ch1(500., 10.);
std::normal_distribution<double> mean_ch2(1200., 30.);
std::normal_distribution<double> mean_ch3(800., 20.);


//implementation adapted from H4DQM: https://github.com/cmsromadaq/H4DQM/blob/master/src/CAEN_V1290.cpp
//CAEN v1290 manual found in: http://www.tunl.duke.edu/documents/public/electronics/CAEN/caen_v1290.pdf
int CAEN_v1290::Unpack (std::ifstream &stream) {
  while (true) {
    unsigned int tdcRawData;
    stream.read((char*)&tdcRawData, sizeof (tdcRawData));
    if (DEBUG_UNPACKER) 
      EUDAQ_INFO("TDC WORD: " + std::to_string(tdcRawData));
    
    if (tdcRawData>>28 == 10 ) { //TDC BOE
      unsigned int tdcEvent=tdcRawData & 0xFFFFFFF;   //looks at the first 28 bits
      currentData.event = tdcEvent;
      if (DEBUG_UNPACKER) 
        EUDAQ_INFO("[CAEN_V12490][Unpack] ID:"+std::to_string(currentData.ID)+"      | TDC 1190 BOE: event " + std::to_string(tdcEvent+1));
      continue;
    }
    
    else if (tdcRawData>>28 == 0) {//TDC DATUM
      unsigned int channel = (tdcRawData>>21) & 0x1f;   //looks at the bits 22 - 27
      unsigned int tdcReadout = tdcRawData & 0x1fffff;  //looks at bits 1 - 21
      if (DEBUG_UNPACKER) 
        EUDAQ_INFO("[CAEN_V12490][Unpack] ID:"+std::to_string(currentData.ID)+"      | tdc 1190 board channel " + std::to_string(channel) + " tdcReadout " + std::to_string(tdcReadout));
      channelTimeStamps[channel].push_back(tdcReadout);
      continue;
    }
    
    else if (tdcRawData>>28 == 8) { //TDC EOE 
      EUDAQ_INFO("[CAEN_V12490][Unpack] ID:"+std::to_string(currentData.ID)+"      | TDC 1190 BOE: end of event ");
      break;
    }
    
    else
      break;
  }

  stream.close();
  return 0 ;
}

void writeToStream(std::fstream &ofs, unsigned int bitStream) {
  char charData[4];
  charData[0] = bitStream & 0xFF; 
  charData[1] = (bitStream >> 8) & 0xFF; 
  charData[2] = (bitStream >> 16) & 0xFF; 
  charData[3] = (bitStream >> 24) & 0xFF; 
  ofs.write((char*)charData, 4);
}

void CAEN_v1290::generatePseudoData() {
  std::fstream ofs;
  ofs.open("/tmp/eudaq_scratch/dwc_"+std::to_string(currentData.ID)+".txt", std::ios::out | std::ios::binary);

  unsigned int bitStream = 0;

  //first the BOE
  unsigned int eventNr = currentData.event++;
  bitStream = bitStream | 10<<28;
  writeToStream(ofs, bitStream);
  
  //generate the channel information
  for (unsigned int channel=0; channel<=3; channel++) {
    unsigned int readout;
    if (channel==0) readout=mean_ch0(generator);
    else if (channel==1) readout=mean_ch1(generator);
    else if (channel==2) readout=mean_ch2(generator);
    else if (channel==3) readout=mean_ch3(generator);

    for (int N=0; N<20; N++) {
      bitStream = 0;
      bitStream = bitStream | 0<<28;
      bitStream = bitStream | channel<<21;
      bitStream = bitStream | (readout+N);
      writeToStream(ofs, bitStream);
    }
  }
  
  //last the BOE
  bitStream = 0;
  bitStream = bitStream | 8<<28;
  writeToStream(ofs, bitStream);

  ofs.close();
}



tdcData CAEN_v1290::GetCurrentData() {
  return currentData;
}

void CAEN_v1290::ReadoutTDC() {
  //double dT_delay = std::max(0.0, delay(generator));
  //eudaq::mSleep(dT_delay);
  generatePseudoData();
  
  //clear the buffer:
  for (unsigned int channel=0; channel<=3; channel++)
    channelTimeStamps[channel].clear();

  std::ifstream ifs;
  ifs.open("/tmp/eudaq_scratch/dwc_"+std::to_string(currentData.ID)+".txt", std::ios::in | std::ios::binary);
  Unpack(ifs);

  currentData.timeStamp_ch0 = *min_element(channelTimeStamps[0].begin(), channelTimeStamps[0].end());
  currentData.timeStamp_ch1 = *min_element(channelTimeStamps[1].begin(), channelTimeStamps[1].end());
  currentData.timeStamp_ch2 = *min_element(channelTimeStamps[2].begin(), channelTimeStamps[2].end());
  currentData.timeStamp_ch3 = *min_element(channelTimeStamps[3].begin(), channelTimeStamps[3].end());
}
