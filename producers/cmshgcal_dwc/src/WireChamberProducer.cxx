#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <cstdlib>
#include <string>
#include <vector>

#include <boost/crc.hpp>
#include <boost/cstdint.hpp>
#include <boost/timer/timer.hpp>
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>

#include <TFile.h>
#include <TTree.h>

#include "IpbusHwController.h"
#include "WireChamberTriggerController.h"
#include "CAEN_v1290.h"
#include "Unpacker.h"



void startTriggerThread( WireChamberTriggerController* trg_ctrl, uint32_t *run, ACQ_MODE* mode) {
  trg_ctrl->startrunning( *run, *mode );
}

void readTDCThread( CAEN_V1290* tdc, std::vector<WORD>& data) {
  //tdc->Read(data);      //needs to be tested with proper trigger
  tdc->generatePseudoData(data);
}


static const std::string EVENT_TYPE = "DelayWireChambers";

class WireChamberProducer : public eudaq::Producer {
  public:

  WireChamberProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), stopping(false), done(false), started(0) {
      std::cout<<"Initialisation of the DWC Producer..."<<std::endl;
      tdc = new CAEN_V1290();
      tdc_unpacker = new Unpacker();
      outTree=NULL;
      m_sync_orm=NULL;
      m_WireChamberTriggerController=NULL;
    
      for (size_t i=0; i<4; i++) {   //four DWCs
        dwc_XUP_timestamp.push_back(-999);
        dwc_XDOWN_timestamp.push_back(-999);
        dwc_YUP_timestamp.push_back(-999);
        dwc_YDOWN_timestamp.push_back(-999);
        recoX.push_back(-999);
        recoY.push_back(-999);
      }

    }

  virtual void OnConfigure(const eudaq::Configuration & config) {
    std::cout << "Configuring: " << config.Name() << std::endl;

    //Setup the the wire chambers objects for filling
    N_DWCs = config.Get("NumberOfWireChambers", 2);

    CAEN_V1290::CAEN_V1290_Config_t _config;
    _config.baseAddress = config.Get("baseAddress", 0x00AA0000);
    _config.model = config.Get("model", 1);
    _config.triggerTimeSubtraction = config.Get("triggerTimeSubtraction", 1);
    _config.triggerMatchMode = config.Get("triggerMatchMode", 1);
    _config.emptyEventEnable = config.Get("emptyEventEnable", 1);
    _config.edgeDetectionMode = config.Get("edgeDetectionMode", 2);
    _config.timeResolution = config.Get("timeResolution", 3);
    _config.maxHitsPerEvent = config.Get("maxHitsPerEvent", 9);
    _config.enabledChannels = config.Get("enabledChannels", 0x00FF);
    _config.windowWidth = config.Get("windowWidth", 0x40);
    _config.windowOffset = config.Get("windowOffset", 0x24);

    tdc->Config(_config);
    

    //read the channel map
    for (unsigned int channel=0; channel<16; channel++){
      channel_map[static_cast<CHANNEL_INDEX>(channel)] = config.Get(("channel_"+std::to_string(channel)).c_str(), -1);
    }

    //conversion slope:
    conversionSlope = config.Get("conversionSlope", 0.2);

    //setup the synchronisation board
    int mode = config.Get("AcquisitionMode", 0);
    switch( mode ){
      case 0 : 
        m_acqmode = DEBUG; break;
      default : 
      m_acqmode = DEBUG; break;
    }
    
    //instances of the trigger controller and the sync orm
    if (m_sync_orm!=NULL) delete m_sync_orm;
    if (m_WireChamberTriggerController!=NULL) delete m_WireChamberTriggerController; 
    m_sync_orm = new ipbus::IpbusHwController(config.Get("ConnectionFile","file://./etc/connection.xml"),
                config.Get("SYNC_ORM_NAME","SYNC_ORM"));
    m_WireChamberTriggerController = new WireChamberTriggerController(tdc, m_sync_orm);

    //Read the data output file prefix
    dataFilePrefix = config.Get("dataFilePrefix", "../data/dwc_run_");

    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");

  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_ev = 0;
    EUDAQ_INFO("Start Run: "+param);
    // It must send a BORE to the Data Collector
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
    //bore.SetTag("EXAMPLE", eudaq::to_string(m_exampleparam));
    SendEvent(bore);

    if (outTree != NULL) delete outTree;
    outTree = new TTree("DelayWireChambers", "DelayWireChambers");
    outTree->Branch("run", &m_run);
    outTree->Branch("event", &m_ev);
    outTree->Branch("channelXUP_timestamps", &dwc_XUP_timestamp);
    outTree->Branch("channelXDOWN_timestamps", &dwc_XDOWN_timestamp);
    outTree->Branch("channelYUP_timestamps", &dwc_YUP_timestamp);
    outTree->Branch("channelYDOWN_timestamps", &dwc_YDOWN_timestamp);
    outTree->Branch("recoX", &recoX);
    outTree->Branch("recoY", &recoY);
    
    m_triggerThread=boost::thread(startTriggerThread,m_WireChamberTriggerController,&m_run,&m_acqmode);

    SetStatus(eudaq::Status::LVL_OK, "Running");
    started=true;
  }

  // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    EUDAQ_INFO("Stopping Run");
    started=false;
    // Set a flag tao signal to the polling loop that the run is over
    stopping = true;

    m_WireChamberTriggerController->stopRun();

    // wait until all events have been read out from the hardware
    //while (stopping) {
    //  eudaq::mSleep(20);
    //}

    // Send an EORE after all the real events have been sent
    // You can also set tags on it (as with the BORE) if necessary
    SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));


    std::cout<<"Saving the data into the outputfile"<<std::endl;
    //save the tree into a file
    TFile* outfile = new TFile((dataFilePrefix+std::to_string(m_run)+".root").c_str(), "RECREATE");
    outTree->Write();
    outfile->Close();

    stopping = false;
  }

  // This gets called when the Run Control is terminating,
  // we should also exit.
  virtual void OnTerminate() {
    EUDAQ_INFO("Terminating...");
    done = true;
    eudaq::mSleep(200);
    
    delete tdc;
    delete tdc_unpacker;
    delete m_sync_orm;
    delete m_WireChamberTriggerController;
  }


  void ReadoutLoop() {
    while(!done) {

      if (!started || m_WireChamberTriggerController==NULL) {
        std::cout<<"Run has not been started. Sleeping for 2 seconds... "<<std::endl;
        EUDAQ_DEBUG("Run has not been started. Sleeping for 2 seconds... ");
        eudaq::mSleep(2000);
        continue;
      }

      if(!m_WireChamberTriggerController->checkState( (STATES)RDOUT_RDY ) || m_WireChamberTriggerController->eventNumber()!=m_ev+1 ) {
        eudaq::mSleep(20);
        continue;
      }
      
      //we get here, there is data to record      
      m_ev++;
      
      //First: Make instance of threads to read out the DWC TDCs in parallel
      std::vector<WORD> data;
      boost::thread TDC_thread = boost::thread(readTDCThread, tdc, std::ref(data));
      TDC_thread.join();
      
      if (stopping) continue;

      m_WireChamberTriggerController->readoutCompleted();

      std::map<unsigned int, unsigned int> timeOfArrivals = tdc_unpacker->ConvertTDCData(data);

      //making an EUDAQ event
      eudaq::RawDataEvent ev(EVENT_TYPE,m_run,m_ev);
      ev.AddBlock(1, data);

      dwc_XUP_timestamp[0] = (channel_map[X1_UP] >= 0) ? timeOfArrivals[channel_map[X1_UP]] : -999;
      dwc_XDOWN_timestamp[0] = (channel_map[X1_DOWN] >= 0) ? timeOfArrivals[channel_map[X1_DOWN]] : -999;
      dwc_YUP_timestamp[0] = (channel_map[Y1_UP] >= 0) ? timeOfArrivals[channel_map[Y1_UP]] : -999;
      dwc_YDOWN_timestamp[0] = (channel_map[Y1_DOWN] >= 0) ? timeOfArrivals[channel_map[Y1_DOWN]] : -999;
      recoX[0] = (dwc_XUP_timestamp[0]!=-999 && dwc_XDOWN_timestamp[0]!=-999) ? conversionSlope*(dwc_XDOWN_timestamp[0]-dwc_XUP_timestamp[0])/4 : -999.;    //4 time stamps are 1ns, slope is 0.2mm/ns
      recoY[0] = (dwc_YUP_timestamp[0]!=-999 && dwc_YDOWN_timestamp[0]!=-999) ? conversionSlope*(dwc_YDOWN_timestamp[0]-dwc_YUP_timestamp[0])/4 : -999.;    //4 time stamps are 1ns, slope is 0.2mm/ns


      dwc_XUP_timestamp[1] = (channel_map[X2_UP] >= 0) ? timeOfArrivals[channel_map[X2_UP]] : -999;
      dwc_XDOWN_timestamp[1] = (channel_map[X2_DOWN] >= 0) ? timeOfArrivals[channel_map[X2_DOWN]] : -999;
      dwc_YUP_timestamp[1] = (channel_map[Y2_UP] >= 0) ? timeOfArrivals[channel_map[Y2_UP]] : -999;
      dwc_YDOWN_timestamp[1] = (channel_map[Y2_DOWN] >= 0) ? timeOfArrivals[channel_map[Y2_DOWN]] : -999;
      recoX[1] = (dwc_XUP_timestamp[1]!=-999 && dwc_XDOWN_timestamp[1]!=-999) ? conversionSlope*(dwc_XDOWN_timestamp[1]-dwc_XUP_timestamp[1])/4 : -999.;    
      recoY[1] = (dwc_YUP_timestamp[1]!=-999 && dwc_YDOWN_timestamp[1]!=-999) ? conversionSlope*(dwc_YDOWN_timestamp[1]-dwc_YUP_timestamp[1])/4 : -999.;    


      dwc_XUP_timestamp[2] = (channel_map[X3_UP] >= 0) ? timeOfArrivals[channel_map[X3_UP]] : -999;
      dwc_XDOWN_timestamp[2] = (channel_map[X3_DOWN] >= 0) ? timeOfArrivals[channel_map[X3_DOWN]] : -999;
      dwc_YUP_timestamp[2] = (channel_map[Y3_UP] >= 0) ? timeOfArrivals[channel_map[Y3_UP]] : -999;
      dwc_YDOWN_timestamp[2] = (channel_map[Y3_DOWN] >= 0) ? timeOfArrivals[channel_map[Y3_DOWN]] : -999;
      recoX[2] = (dwc_XUP_timestamp[2]!=-999 && dwc_XDOWN_timestamp[2]!=-999) ? conversionSlope*(dwc_XDOWN_timestamp[2]-dwc_XUP_timestamp[2])/4 : -999.;    
      recoY[2] = (dwc_YUP_timestamp[2]!=-999 && dwc_YDOWN_timestamp[2]!=-999) ? conversionSlope*(dwc_YDOWN_timestamp[2]-dwc_YUP_timestamp[2])/4 : -999.;    


      dwc_XUP_timestamp[3] = (channel_map[X4_UP] >= 0) ? timeOfArrivals[channel_map[X4_UP]] : -999;
      dwc_XDOWN_timestamp[3] = (channel_map[X4_DOWN] >= 0) ? timeOfArrivals[channel_map[X4_DOWN]] : -999;
      dwc_YUP_timestamp[3] = (channel_map[Y4_UP] >= 0) ? timeOfArrivals[channel_map[Y4_UP]] : -999;
      dwc_YDOWN_timestamp[3] = (channel_map[Y4_DOWN] >= 0) ? timeOfArrivals[channel_map[Y4_DOWN]] : -999;      
      recoX[3] = (dwc_XUP_timestamp[3]!=-999 && dwc_XDOWN_timestamp[3]!=-999) ? conversionSlope*(dwc_XDOWN_timestamp[3]-dwc_XUP_timestamp[3])/4 : -999.;    
      recoY[3] = (dwc_YUP_timestamp[3]!=-999 && dwc_YDOWN_timestamp[3]!=-999) ? conversionSlope*(dwc_YDOWN_timestamp[3]-dwc_YUP_timestamp[3])/4 : -999.;    


      outTree->Fill();

      //Adding the event to the EUDAQ format
      
      SendEvent(ev);
    }
  }

  private:
    unsigned m_run, m_ev;
    bool stopping, done, started;

    unsigned int N_DWCs;
    std::string dataFilePrefix;
    boost::thread m_triggerThread;

    //set on configuration
    CAEN_V1290* tdc;
    Unpacker* tdc_unpacker;
    ACQ_MODE m_acqmode;
    ipbus::IpbusHwController*  m_sync_orm;
    WireChamberTriggerController* m_WireChamberTriggerController;

    std::map<CHANNEL_INDEX, unsigned int> channel_map;

    //generated for each run
    TTree* outTree;

    std::vector<int> dwc_XUP_timestamp;  //e.g. x min
    std::vector<int> dwc_XDOWN_timestamp;  //e.g. x max
    std::vector<int> dwc_YUP_timestamp;  //e.g. y min
    std::vector<int> dwc_YDOWN_timestamp;  //e.g. y max
    
    double conversionSlope;
    std::vector<double> recoX;
    std::vector<double> recoY;
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("Delay Wire Chamber Producer", "0.1",
  "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
  "tcp://localhost:44000", "address",
  "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
  "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "DWCs", "string",
  "The name of this Producer.");
  
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    WireChamberProducer producer(name.Value(), rctrl.Value());
    producer.ReadoutLoop();
    
    EUDAQ_INFO("Quitting");
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
