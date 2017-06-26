// CaliceGenericConverterPlugin.cc
#include <string>
#include <iostream>
#include <typeinfo>  // for the std::bad_cast
#include <memory> // for std::unique_ptr

#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/LCGenericObjectImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

using namespace eudaq;
using namespace std;
using namespace lcio;

namespace eudaq {

static const char* EVENT_TYPE = "CaliceObject";

#if USE_LCIO
// LCIO class
class CaliceLCGenericObject : public lcio::LCGenericObjectImpl {
   public:
   CaliceLCGenericObject() : lcio::LCGenericObjectImpl() {_typeName = EVENT_TYPE;}
   virtual ~CaliceLCGenericObject() {}

   void setTags(std::string &s) {_dataDescription = s;}
   std::string getTags()const {return _dataDescription;}

   void setDataInt(std::vector<int> &vec) {
      _intVec.resize(vec.size());
      std::copy(vec.begin(), vec.end(), _intVec.begin());
   }

   void setDataInt(std::vector<short> &vec) {
      _intVec.resize(vec.size());
      std::copy(vec.begin(), vec.end(), _intVec.begin());
   }

   const std::vector<int> & getDataInt()const {return _intVec;}
};

#endif

class CaliceGenericConverterPlugin: public DataConverterPlugin {

   public:
      virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const
            {
         std::string sensortype = "Calice"; //TODO ?? "HBU"

         // Unpack data
         const RawDataEvent * rev = dynamic_cast<const RawDataEvent *>(&ev);
         //std::cout << "[Number of blocks] " << rev->NumBlocks() << std::endl;

         // 0 .. "EUDAQDataScCAL";
         // 1 .. "i:CycleNr,i:BunchXID,i:EvtNr,i:ChipID,i:NChannels,i:TDC14bit[NC],i:ADC14bit[NC]";
         // 2 .. unix time
         // 3 .. slowcontrol
         // 4 .. LED info
         // 5 .. Temperature
         // 6 .. Cycle info - start, stop, trigger
         // 7 .. AHCAL data

         //prepare 6 AHCAL layers
         std::vector<std::unique_ptr<StandardPlane>> planes;
         for (int i = 0; i <6; ++i) {
            std::unique_ptr<StandardPlane> plane(new StandardPlane(i, EVENT_TYPE, sensortype));
            plane->SetSizeRaw(13, 13, 1, 0);
            if (ev.GetEventNumber() > 0) plane->SetTLUEvent(ev.GetEventNumber());
            planes.push_back(std::move(plane));         //unique_ptr<StandardPlane>(new StandardPlane(i, EVENT_TYPE, sensortype)));
         }
//         if (ev.GetEventNumber() > 0) {
//            for (int j = 0; j < 144; j = j + 3) {
//               planes[6]->PushPixel(j / 11, j % 12, 1, false, 0);
//            }
//         }

         unsigned int nblock = 7; // the first 7 blocks contain other information
         //if (rev->NumBlocks() == 7) return false;
         std::cout << ev.GetEventNumber() << "<" << std::flush;

         while (nblock < rev->NumBlocks()) {//iterate over all asic packets from (hopefully) same BXID
            vector<int> data;
            const RawDataEvent::data_t & bl = rev->GetBlock(nblock++);
            data.resize(bl.size() / sizeof(int));
            memcpy(&data[0], &bl[0], bl.size());
            if (data.size() != 77) cout << "vector has size : " << bl.size() << "\tdata : " << data.size() << endl;

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
               if (planeNumber >= 0) {//plane, which is not found, has index -1
                  if (hitbit) {
                     // std::cout << "x=" << getXcoordFromCHIPID(chipid, ichan) << "y=" << getYcoordFromCHIPID(chipid, ichan) << "adc="                           << (hitbit ? (gainbit ? adc : 10 * adc) : 0) << std::endl;
                     planes[planeNumber]->PushPixel(getXcoordFromCHIPID(chipid, ichan), getYcoordFromCHIPID(chipid, ichan),
                           hitbit ? (gainbit ? adc : 10 * adc) : 1, //storing the ADC value
                           //hitbit ? 1 : 0, //storing just the hit information
                           false, 0);
                  }
               }
            }
            std::cout << "." << std::flush;
         }
         std::cout << ">" << std::endl;

         //save hits to the onlinedisplay
         for (auto const & plane : planes) {
            sev.AddPlane(*plane);
         }
         return true;
      }

#if USE_LCIO
      virtual bool GetLCIOSubEvent(lcio::LCEvent &result, eudaq::Event const &source) const
      {
         //cout << "CaliceGenericConverterPlugin::GetLCIOSubEvent" << endl;
         // try to cast the Event
         eudaq::RawDataEvent const * rawev = 0;
         try {
            eudaq::RawDataEvent const & rawdataevent = dynamic_cast<eudaq::RawDataEvent const &>(source);
            rawev = &rawdataevent;
         } catch(std::bad_cast& e) {
            std::cout << e.what() << std::endl;
            return false;
         }
         // should check the type
         if(rawev->GetSubType() != EVENT_TYPE) {
            cout << "CaliceGenericConverter: type failed!" << endl;
            return false;
         }

         // no contents -ignore
         if(rawev->NumBlocks() == 3) return true;

         unsigned int nblock = 0;

         if(rawev->NumBlocks() > 3) {

            // first two blocks should be string, 3rd is time, time_usec
            const RawDataEvent::data_t & bl = rawev->GetBlock(nblock++);
            string colName((char *)&bl.front(), bl.size());

            const RawDataEvent::data_t & bl2 = rawev->GetBlock(nblock++);
            string dataDesc((char *)&bl2.front(), bl2.size());

            // EUDAQ TIMESTAMP, saved in ScReader.cc
            const RawDataEvent::data_t & bl3 = rawev->GetBlock(nblock++);
            if(bl3.size()<8) {cout << "CaliceGenericConverter: time data too short!" << endl; return false;}
            time_t timestamp = *(unsigned int *)(&bl3[0]);
            unsigned int timestamp_usec = *(int *)(&bl3[4]);

            //-------------------
            // READ/WRITE Temperature
            //fourth block is temperature (if colName ==  "EUDAQDataScCAL")
            const RawDataEvent::data_t & bl4 = rawev->GetBlock(nblock++);

            if(bl4.size() > 0) {
               // sensor specific data
               if(colName == "EUDAQDataScCAL") {
                  cout << "Looking for TempSensor collection..." << endl;

                  vector<int> vec;
                  vec.resize(bl4.size() / sizeof(int));
                  memcpy(&vec[0], &bl4[0],bl4.size());

                  LCCollectionVec *col2 = 0;
                  col2=createCollectionVec(result,"TempSensor", "i:LDA;i:port;i:T1;i:T2;i:T3;i:T4;i:T5;i:T6;i:TDIF;i:TPWR", timestamp);
                  getScCalTemperatureSubEvent(col2, vec);
               }
            }

            //-------------------
            // READ/WRITE SPIROC DATA
            LCCollectionVec *col = 0;
            col=createCollectionVec(result,colName,dataDesc, timestamp);
            getDataLCIOGenericObject(rawev,col,nblock);
            //-------------------

         }
         return true;
      }

      virtual LCCollectionVec* createCollectionVec(lcio::LCEvent &result, string colName, string dataDesc, time_t timestamp ) const {
         LCCollectionVec *col = 0;
         try {
            // looking for existing collection
            col = dynamic_cast<IMPL::LCCollectionVec *>(result.getCollection(colName));
            //cout << "collection found." << endl;
         } catch(DataNotAvailableException &e) {
            // create new collection
            //cout << "Creating TempSensor collection..." << endl;
            col = new IMPL::LCCollectionVec(LCIO::LCGENERICOBJECT);
            result.addCollection(col,colName);
            //  cout << "collection added." << endl;
         }
         col->parameters().setValue("DataDescription", dataDesc);
         //add timestamp (EUDAQ!!)
         struct tm *tms = localtime(&timestamp);
         char tmc[256];
         strftime(tmc, 256, "%a, %d %b %Y %T %z", tms);
         col->parameters().setValue("Timestamp", tmc);
         col->parameters().setValue("Timestamp_i", (int)timestamp);

         return col;
      }

      virtual void getScCalTemperatureSubEvent(LCCollectionVec *col2, vector<int> vec) const {

         vector<int> output;
         int lda = -1;
         int port = 0;
         //int dummylda = 0;
         for(unsigned int i=0;i<vec.size()-2;i+=3) {
            if((i/3)%2 ==0)continue; // just ignore the first data;
            //dummylda = vec[i]; // lda number: should be dummy
            if(output.size()!=0 && port != vec[i+1]) {
               cout << "Different port number comes before getting 8 data!." << endl;
               break;
            }
            port = vec[i+1]; // port number
            int data = vec[i+2];// data
            if(output.size() == 0) {
               if(port == 0)lda ++;
               output.push_back(lda);
               output.push_back(port);
            }
            output.push_back(data);

            if(output.size() == 10) {
               //            cout << "Storing TempSensor data..." << endl;
               CaliceLCGenericObject *obj = new CaliceLCGenericObject;
               obj->setDataInt(output);
               try {
                  col2->addElement(obj);
               } catch(ReadOnlyException &e) {
                  cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
                  delete obj;
               }
               output.clear();
            }
         }
      }

      virtual void getDataLCIOGenericObject(eudaq::RawDataEvent const * rawev, LCCollectionVec *col, int nblock) const {

         while(nblock < rawev->NumBlocks()) {
            // further blocks should be data (currently limited to integer)
            vector<short> v;
            const RawDataEvent::data_t & bl5 = rawev->GetBlock(nblock++);
            v.resize(bl5.size() / sizeof(short));
            memcpy(&v[0], &bl5[0],bl5.size());

            CaliceLCGenericObject *obj = new CaliceLCGenericObject;
            obj->setDataInt(v);
            try {
               col->addElement(obj);
            } catch(ReadOnlyException &e) {
               cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
               delete obj;
            }
         }
      }

#endif

   private:
      CaliceGenericConverterPlugin()
            : DataConverterPlugin(EVENT_TYPE)
      {
      }

      int getPlaneNumberFromCHIPID(int chipid) const {
         int planeNumber = -1;
         switch ((chipid - 1) >> 2) {
            case 59: //237
               planeNumber = 0;
               break;
            case 60: //241
               planeNumber = 1;
               break;
            case 61: //245
               planeNumber = 2;
               break;
            case 30: //121
               planeNumber = 3;
               break;
            case 29: //117
               planeNumber = 4;
               break;
            case 62: //249
               planeNumber = 5;
               break;
            default:
               break;
         }
         return planeNumber;
      }

      int getXcoordFromCHIPID(int chipid, int channelNr) const {
         int subx = channelNr / 6;
         if (((chipid & 0x03) == 0x01) || ((chipid & 0x03) == 0x02)) {
            //1st and 2nd spiroc are in the righ half of HBU
            subx += 6;
         }
         return subx + 1;
      }

      int getYcoordFromCHIPID(int chipid, int channelNr) const {
         int suby = channelNr % 6;
         if (((chipid & 0x03) == 0x00) || ((chipid & 0x03) == 0x03)) {
            //3rd and 4th spiroc have different channel order
            suby = 5 - suby;
         }
         if (((chipid & 0x03) == 0x01) || ((chipid & 0x03) == 0x03)) {
            suby += 6;
         }
         return suby + 1;
      }

      static CaliceGenericConverterPlugin m_instance;

};

// Instantiate the converter plugin instance
CaliceGenericConverterPlugin CaliceGenericConverterPlugin::m_instance;

}

