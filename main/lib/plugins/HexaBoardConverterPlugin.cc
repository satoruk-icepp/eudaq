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
          return getlittleendian<unsigned short>(
              &rev->GetBlock(0)[TRIGGER_OFFSET]);
        }
      }
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }

    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {
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

	std::vector<unsigned short> data;
	data.resize(bl.size() / sizeof(short));
	std::memcpy(&data[0], &bl[0], bl.size());


	StandardPlane plane(2*pl+1, EVENT_TYPE, sensortype);

	if (data.size()%17!=0){
	  EUDAQ_WARN("There is something wrong with the data. Not factorized by 17 entries per channel: "+eudaq::to_string(data.size()));
	}


       	const unsigned nHits  = data.size()/17;
	plane.SetSizeZS(4, 64, nHits, 16);

	for (int ind=0; ind<nHits; ind++){

	  // APZ DBG. Test the monitoring:
	  //for (int ts=0; ts<16; ts++)
	  //plane.SetPixel(ind, 2, 32+ind, 500-10*ind, false, ts);
	  
	
	  const unsigned ski = data[17*ind]/100;
	  if (ski > 3){
	    std::cout<<" EROOR in ski. It is "<<ski<<std::endl;
	    EUDAQ_WARN("There is another error with encoding. ski="+eudaq::to_string(ski));
	  }
	  else {
	    const unsigned pix = data[17*ind]-ski*100;

	    //if (ski==0 && pix==0)
	    //std::cout<<" Zero-Zero problem. ind = "<<ind<<"  ID:"<<data[17*ind]<<std::endl;
	    
	    for (int ts=0; ts<16; ts++)
	      plane.SetPixel(ind, ski, pix, data[17*ind+1 + ts], false, ts);
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
