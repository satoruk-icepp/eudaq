#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#ifdef _WIN32
//TODO remove the _WIN32 if not necessary in linux
#include <array>
#endif

class AHCalRawEvent2StdEventConverter: public eudaq::StdEventConverter {
   public:
      bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
      static const uint32_t m_id_factory = eudaq::cstr2hash("CaliceObject");

   private:
      int getPlaneNumberFromCHIPID(int chipid) const;
      int getXcoordFromCHIPID(int chipid, int channelNr) const;
      int getYcoordFromCHIPID(int chipid, int channelNr) const;
};

namespace {
auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
      Register<AHCalRawEvent2StdEventConverter>(AHCalRawEvent2StdEventConverter::m_id_factory);
}

bool AHCalRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const {
   std::string sensortype = "Calice"; //TODO ?? "HBU"
   const int planeCount = 1;
   auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
   size_t nblocks = ev->NumBlocks();
   std::vector<std::unique_ptr<eudaq::StandardPlane>> planes;
   std::vector<int> HBUHits;
   std::vector<std::array<int, 144>> HBUs;         //HBU(aka plane) index, x*12+y
   for (int i = 0; i < planeCount; ++i) {
      std::array<int, 144> HBU;
      HBU.fill(-1); //fill all channels to -1
      HBUs.push_back(HBU); //add the HBU to the HBU
      HBUHits.push_back(0);
   }
   unsigned int nblock = 7; // the first 7 blocks contain other information
   std::cout << ev->GetEventNumber() << "<" << std::flush;

   while (nblock < ev->NumBlocks()) {         //iterate over all asic packets from (hopefully) same BXID
      std::vector<int> data;
      const auto & bl = ev->GetBlock(nblock++);
      data.resize(bl.size() / sizeof(int));
      memcpy(&data[0], &bl[0], bl.size());
      if (data.size() != 77) std::cout << "vector has size : " << bl.size() << "\tdata : " << data.size() << std::endl;
      //data structure of packet: data[i]=
      //i=0 --> cycleNr
      //i=1 --> bunch crossing id
      //i=2 --> memcell or EvtNr (same thing, different naming)
      //i=3 --> ChipId
      //i=4 --> Nchannels per chip (normally 36)
      //i=5 to NC+4 -->  14 bits that contains TDC and hit/gainbit
      //i=NC+5 to NC+NC+4  -->  14 bits that contains ADC and again a copy of the hit/gainbit
      //debug prints:
      //std:cout << "Data_" << data[0] << "_" << data[1] << "_" << data[2] << "_" << data[3] << "_" << data[4] << "_" << data[5] << std::endl;
      if (data[1] == 0) continue; //don't store dummy trigger
      int chipid = data[3];
      int planeNumber = getPlaneNumberFromCHIPID(chipid);

      for (int ichan = 0; ichan < data[4]; ichan++) {
         int adc = data[5 + data[4] + ichan] & 0x0FFF; // extract adc
         int gainbit = (data[5 + data[4] + ichan] & 0x2000) ? 1 : 0; //extract gainbit
         int hitbit = (data[5 + data[4] + ichan] & 0x1000) ? 1 : 0;  //extract hitbit
         if (planeNumber >= 0) {  //plane, which is not found, has index -1
            if (hitbit) {
               int coordIndex = getXcoordFromCHIPID(chipid, ichan) * 12 + getYcoordFromCHIPID(chipid, ichan);  //get the index from the HBU array
               if (HBUs[planeNumber][coordIndex] >= 0) std::cout << "ERROR: channel already has a value" << std::endl;
               HBUs[planeNumber][coordIndex] = gainbit ? adc : 10 * adc;
               //HBUs[planeNumber][coordIndex] = 1;
               if (HBUs[planeNumber][coordIndex] < 0) HBUs[planeNumber][coordIndex] = 0;
               HBUHits[planeNumber]++;
            }
         }
      }
      std::cout << "." << std::flush;
   }
   for (int i = 0; i < HBUs.size(); ++i) {
      std::unique_ptr<eudaq::StandardPlane> plane(new eudaq::StandardPlane(i, "CaliceObject", sensortype));
      //plane->SetSizeRaw(13, 13, 1, 0);
      int pixindex = 0;
      plane->SetSizeZS(12, 12, HBUHits[i], 1, 0);
      for (int x = 0; x < 12; x++) {
         for (int y = 0; y < 12; y++) {
            if (HBUs[i][x * 12 + y] >= 0) {
               plane->SetPixel(pixindex++, x, y, HBUs[i][x * 12 + y]);
            }
         }
      }

      //if (ev->GetEventNumber() > 0) plane->SetTLUEvent(ev.GetEventNumber());
      //save planes with hits hits to the onlinedisplay

      d2->AddPlane(*plane);
      std::cout << ":" << std::flush;
   }
   std::cout << ">" << std::endl;
   return true;
}

int AHCalRawEvent2StdEventConverter::getPlaneNumberFromCHIPID(int chipid) const {
   int planeNumber = -1;
   switch ((chipid - 1) >> 2) {
      case 48: //193
         planeNumber = 0;
         break;
         // case 59: //237
         //    planeNumber = 0;
         //    break;
         // case 60: //241
         //    planeNumber = 1;
         //    break;
         // case 61: //245
         //    planeNumber = 2;
         //    break;
         // case 30: //121
         //    planeNumber = 3;
         //    break;
         // case 29: //117
         //    planeNumber = 4;
         //    break;
         // case 62: //249
         //    planeNumber = 5;
         //    break;
         // case 56: //225
         //    planeNumber = 6;
         //    break;
         // case 54: //217
         //    planeNumber = 7;
         //    break;
         // case 53: //213
         //    planeNumber = 8;
         //    break;
         // case 51: //205
         //    planeNumber = 9;
         //    break;
         // case 55: //221
         //    planeNumber = 10;
         //    break;
         // case 50: //201
         //    planeNumber = 11;
         //    break;
      default:
         break;
   }
   return planeNumber;
}

int AHCalRawEvent2StdEventConverter::getXcoordFromCHIPID(int chipid, int channelNr) const {
   int subx = channelNr / 6;
   if (((chipid & 0x03) == 0x01) || ((chipid & 0x03) == 0x02)) {
      //1st and 2nd spiroc are in the righ half of HBU
      subx += 6;
   }
   return subx;
}

int AHCalRawEvent2StdEventConverter::getYcoordFromCHIPID(int chipid, int channelNr) const {
   int suby = channelNr % 6;
   if (((chipid & 0x03) == 0x00) || ((chipid & 0x03) == 0x03)) {
      //3rd and 4th spiroc have different channel order
      suby = 5 - suby;
   }
   if (((chipid & 0x03) == 0x01) || ((chipid & 0x03) == 0x03)) {
      suby += 6;
   }
   return suby;
}
