#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#endif

const size_t RAW_EV_SIZE=30787;

namespace eudaq {
  
  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  //static const char *EVENT_TYPE = "RPI";
  static const char *EVENT_TYPE = "HexaBoard";
  
  // Declare a new class that inherits from DataConverterPlugin
  class HexaBoardConverterPlugin : public DataConverterPlugin {
      
  public:
    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event &bore, const Configuration &cnf) {
      m_exampleparam = bore.GetTag("HeXa", 0);
#ifndef WIN32 // some linux Stuff //$$change
      (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
    }
    
    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
    virtual unsigned GetTriggerID(const Event &ev) const {
      static const unsigned TRIGGER_OFFSET = 8;
      // Make sure the event is of class RawDataEvent
      if (const RawDataEvent *rev = dynamic_cast<const RawDataEvent *>(&ev)) {
	// This is just an example, modified it to suit your raw data format
        // Make sure we have at least one block of data, and it is large enough
        if (rev->NumBlocks() > 0 &&
            rev->GetBlock(0).size() >= (TRIGGER_OFFSET + sizeof(short))) {
          // Read a little-endian unsigned short from offset TRIGGER_OFFSET
          return getlittleendian<unsigned short>( &rev->GetBlock(0)[TRIGGER_OFFSET]);
        }
      }
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }

    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent &sev, const Event &ev) const {

      // If the event type is used for different sensors
      // they can be differentiated here
      const std::string sensortype = "HexaBoard";


      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );

      //rev->Print(std::cout);

      const unsigned nPlanes = rev->NumBlocks();
      std::cout<<"Number of Raw Data Blocks (=Planes): "<<nPlanes<<std::endl;

      for (unsigned pl=0; pl<nPlanes; pl++){

	std::cout<<"Plane = "<<pl<<"  Raw GetID = "<<rev->GetID(pl)<<std::endl;

	const RawDataEvent::data_t & bl = rev->GetBlock(pl);

	std::cout<<"size of block: "<<bl.size()<<std::endl;


	std::vector<unsigned char> rawData;
	rawData.resize(bl.size() / sizeof(unsigned char));
	std::memcpy(&rawData[0], &bl[0], bl.size());


	StandardPlane plane(2*pl+1, EVENT_TYPE, sensortype);

	if (rawData.size()!=RAW_EV_SIZE){
	  EUDAQ_WARN("There is something wrong with the data. Size= "+eudaq::to_string(rawData.size()));
	}

	//if (data.size()%17!=0){
	//EUDAQ_WARN("There is something wrong with the data. Not factorized by 17 entries per channel: "+eudaq::to_string(data.size()));
	//}
	

	const std::array<std::array<unsigned int, 1924>,4> decoded = decode_raw(rawData);
		
	std::vector<unsigned short> dataBlockZS;
	
	for (int ski = 0; ski < 4; ski++ ){
	  
	  //fprintf(fout, "Event %d Chip %d RollMask %x \n", m_ev, ski, decoded[ski][1920]);
	  
	  const int ped = 190;  // pedestal
	  const int noi = 20;   // noise
	  
	  // ----------
	  // -- Based on the rollmask, lets determine which time-slices (frames) to add
	  //
	  unsigned int r = decoded[ski][1920];
	  //printf("Roll mask = %d \n", r);
	  int k1 = -1, k2 = -1;
	  for (int p=0; p<13; p++){
	    //printf("pos = %d, %d \n", p, r & (1<<12-p));
	    if (r & (1<<12-p)) {
	      if (k1==-1)
		k1 = p;
	      else if (k2==-1)
		k2 = p;
	      else
		printf("Error: more than two positions in roll mask! %x \n",r);
	    }
	  }

	  //printf("k1 = %d, k2 = %d \n", k1, k2);
	  
	  // Check that k1 and k2 are consecutive
	  char last = -1;
	  if (k1==0 && k2==12) { last = 0;}
	  else if (abs(k1-k2)>1)
	    EUDAQ_WARN("The k1 and k2 are not consecutive! abs(k1-k2) = "+ eudaq::to_string(abs(k1-k2)));
	  //printf("The k1 and k2 are not consecutive! abs(k1-k2) = %d\n", abs(k1-k2));
	  else
	    last = k2;
	  
	  //printf("last = %d\n", last);
	  // k2+1 it the begin TS
	  
	  // Let's assume we can somehow determine the main frame
	  const char mainFrameOffset = 5; // offset of the pulse wrt trigger (k2 rollmask)
	  const char mainFrame = (last+mainFrameOffset)%13;
	  
	  
	  const int tsm2 = (((mainFrame - 2) % 13) + ((mainFrame >= 2) ? 0 : 13))%13;
	  const int tsm1 = (((mainFrame - 1) % 13) + ((mainFrame >= 1) ? 0 : 13))%13;
	  const int ts0  = mainFrame;
	  const int ts1  = (mainFrame+1)%13;
	  const int ts2  = (mainFrame+2)%13;
	  
	  //printf("TS 0 to be saved: %d\n", tsm2);
	  //printf("TS 1 to be saved: %d\n", tsm1);
	  //printf("TS 2 to be saved: %d\n", ts0);
	  //printf("TS 3 to be saved: %d\n", ts1);
	  //printf("TS 4 to be saved: %d\n", ts2);
	  
	  // -- End of main frame determination

	    
	  for (int ch = 0; ch < 64; ch+=2){
	    
	    const int chArrPos = 63-ch; // position of the hit in array
	    //int chargeLG = decoded[ski][mainFrame*128 + chArrPos] & 0x0FFF;
	    const int chargeHG = decoded[ski][mainFrame*128 + chArrPos] & 0x0FFF;
	    // ZeroSuppress:
	    if (chargeHG - (ped+noi) < 0) continue;
	    
	    dataBlockZS.push_back(ski*100+ch);


	    // Low gain (save 5 time-slices total):
	    dataBlockZS.push_back(decoded[ski][tsm2*128 + chArrPos] & 0x0FFF);
	    dataBlockZS.push_back(decoded[ski][tsm1*128 + chArrPos] & 0x0FFF);
	    dataBlockZS.push_back(decoded[ski][ts0*128 + chArrPos] & 0x0FFF);
	    dataBlockZS.push_back(decoded[ski][ts1*128 + chArrPos] & 0x0FFF);
	    dataBlockZS.push_back(decoded[ski][ts2*128 + chArrPos] & 0x0FFF);
	    
	      // High gain:
	    dataBlockZS.push_back(decoded[ski][tsm2*128 + 64 + chArrPos] & 0x0FFF);
	    dataBlockZS.push_back(decoded[ski][tsm1*128 + 64 + chArrPos] & 0x0FFF);
	    dataBlockZS.push_back(decoded[ski][ts0*128 + 64 + chArrPos] & 0x0FFF);
	    dataBlockZS.push_back(decoded[ski][ts1*128 + 64 + chArrPos] & 0x0FFF);
	    dataBlockZS.push_back(decoded[ski][ts2*128 + 64 + chArrPos] & 0x0FFF);
	    
	    
	    // Filling TOA (stop falling clock)
	    dataBlockZS.push_back(decoded[ski][1664 + chArrPos] & 0x0FFF);
	    
	    // Filling TOA (stop rising clock)
	    dataBlockZS.push_back(decoded[ski][1664 + 64 + chArrPos] & 0x0FFF);
	    
	    // Filling TOT (slow)
	    dataBlockZS.push_back(decoded[ski][1664 + 2*64 + chArrPos] & 0x0FFF);
	    
	    // Filling TOT (fast)
	    dataBlockZS.push_back(decoded[ski][1664 + 3*64 + chArrPos] & 0x0FFF);
	    
	    // Global TS 14 MSB (it's gray encoded?). Not decoded here!
	    dataBlockZS.push_back(decoded[ski][1921]);
	    
	    // Global TS 12 LSB + 1 extra bit (binary encoded)
	    dataBlockZS.push_back(decoded[ski][1922]);
	    	    
	  }

	  
	  
	}
	
	const unsigned nHits  = dataBlockZS.size()/17;
	plane.SetSizeZS(4, 64, nHits, 16);

	for (int ind=0; ind<nHits; ind++){

	  // APZ DBG. Test the monitoring:
	  //for (int ts=0; ts<16; ts++)
	  //plane.SetPixel(ind, 2, 32+ind, 500-10*ind, false, ts);
	  
	
	  const unsigned ski = dataBlockZS[17*ind]/100;
	  if (ski > 3){
	    std::cout<<" EROOR in ski. It is "<<ski<<std::endl;
	    EUDAQ_WARN("There is another error with encoding. ski="+eudaq::to_string(ski));
	  }
	  else {
	    const unsigned pix = dataBlockZS[17*ind]-ski*100;

	    //if (ski==0 && pix==0)
	    //std::cout<<" Zero-Zero problem. ind = "<<ind<<"  ID:"<<data[17*ind]<<std::endl;
	    
	    for (int ts=0; ts<16; ts++)
	      plane.SetPixel(ind, ski, pix, dataBlockZS[17*ind+1 + ts], false, ts);
	  }
	
	}




	/* APZ DBG
	// These are just to test the planes in onlinemonitor:
	if (bl.size()>3000){
	  plane.SetSizeZS(4, 64, 5, 2);

	  plane.SetPixel(0, 2, 32, 500, false, 0);
	  plane.SetPixel(1, 2, 33, 400, false, 0);
	  plane.SetPixel(2, 2, 31, 400, false, 0);
	  plane.SetPixel(3, 1, 32, 400, false, 0);
	  plane.SetPixel(4, 3, 32, 400, false, 0);


	  plane.SetPixel(0, 2, 32, 300, false, 1);
	  plane.SetPixel(1, 2, 33, 100, false, 1);
	  plane.SetPixel(2, 2, 31, 100, false, 1);
	  plane.SetPixel(3, 1, 32, 100, false, 1);
	  plane.SetPixel(4, 3, 32, 100, false, 1);
	}
	else {
	  plane.SetSizeZS(4, 64, 3, 2);

	  plane.SetPixel(0, 1, 15, 500, false, 0);
	  plane.SetPixel(1, 1, 16, 400, false, 0);
	  plane.SetPixel(2, 1, 14, 400, false, 0);


	  plane.SetPixel(0, 1, 15, 300, false, 1);
	  plane.SetPixel(1, 1, 16, 100, false, 1);
	  plane.SetPixel(2, 1, 14, 100, false, 1);
	}
	*/

	// Set the trigger ID
	plane.SetTLUEvent(GetTriggerID(ev));
	// Add the plane to the StandardEvent

	sev.AddPlane(plane);


      }
      //std::cout<<"St Ev NumPlanes: "<<sev.NumPlanes()<<std::endl;

      // Indicate that data was successfully converted
      return true;

    }
    
    unsigned int gray_to_brady(unsigned int gray) const{
      // Code from:
      //https://github.com/CMS-HGCAL/TestBeam/blob/826083b3bbc0d9d78b7d706198a9aee6b9711210/RawToDigi/plugins/HGCalTBRawToDigi.cc#L154-L170
      unsigned int result = gray & (1 << 11);
      result |= (gray ^ (result >> 1)) & (1 << 10);
      result |= (gray ^ (result >> 1)) & (1 << 9);
      result |= (gray ^ (result >> 1)) & (1 << 8);
      result |= (gray ^ (result >> 1)) & (1 << 7);
      result |= (gray ^ (result >> 1)) & (1 << 6);
      result |= (gray ^ (result >> 1)) & (1 << 5);
      result |= (gray ^ (result >> 1)) & (1 << 4);
      result |= (gray ^ (result >> 1)) & (1 << 3);
      result |= (gray ^ (result >> 1)) & (1 << 2);
      result |= (gray ^ (result >> 1)) & (1 << 1);
      result |= (gray ^ (result >> 1)) & (1 << 0);
      return result;
    }
      
    std::array<std::array<unsigned int,1924>,4> decode_raw(std::vector<unsigned char>& raw) const{
      
      // Code from Sandro with minor modifications
      //unsigned int ev[4][1924];
      std::array<std::array<unsigned int, 1924>,4> ev;
      
      unsigned char x;
      for(int i = 0; i < 1924; i = i+1){
	for (int k = 0; k < 4; k = k + 1){
	  ev[k][i] = 0;
	}
      }
      
      for(int  i = 0; i < 1924; i = i+1){
	for (int j=0; j < 16; j = j+1){
	  x = raw[1 + i*16 + j];
	  x = x & 15; // <-- APZ: Not sure why this is needed.
	  for (int k = 0; k < 4; k = k + 1){
	    ev[k][i] = ev[k][i] | (unsigned int) (((x >> (3 - k) ) & 1) << (15 - j));
	  }
	}
      }

      unsigned int t, bith;
      for(int k = 0; k < 4 ; k = k +1 ){
	for(int i = 0; i < 128*13; i = i + 1){
	  bith = ev[k][i] & 0x8000;

	  t = gray_to_brady(ev[k][i] & 0x7fff);
	  ev[k][i] =  bith | t;
	}
      }
    
      return ev;
    }


      
#if USE_LCIO
    // This is where the conversion to LCIO is done
    virtual lcio::LCEvent *GetLCIOEvent(const Event * /*ev*/) const {
      return 0;
    }
#endif
    
  private:
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    HexaBoardConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE), m_exampleparam(0) {}

    // Information extracted in Initialize() can be stored here:
    unsigned m_exampleparam;

    // The single instance of this converter plugin
    static HexaBoardConverterPlugin m_instance;
  }; // class HexaBoardConverterPlugin

  // Instantiate the converter plugin instance
  HexaBoardConverterPlugin HexaBoardConverterPlugin::m_instance;

} // namespace eudaq
