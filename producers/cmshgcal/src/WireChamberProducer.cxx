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




void startTriggerThread( WireChamberTriggerController* trg_ctrl, uint32_t *run, ACQ_MODE* mode) {
  trg_ctrl->startrunning( *run, *mode );
}

void readTDCThread( CAEN_v1290* tdc) {
  tdc->ReadoutTDC();
}


static const std::string EVENT_TYPE = "DelayWireChambers";

class WireChamberProducer : public eudaq::Producer {
  public:

  WireChamberProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), stopping(false), done(false), started(0) {
      std::cout<<"Initialisation of the DWC Producer..."<<std::endl;
      outTree=NULL;
      m_sync_orm=NULL;
      m_WireChamberTriggerController=NULL;
    }

  virtual void OnConfigure(const eudaq::Configuration & config) {
    std::cout << "Configuring: " << config.Name() << std::endl;

    //Setup the the wire chambers objects for filling
    N_DWCs = config.Get("NumberOfWireChambers", 2);
    for (std::vector<CAEN_v1290*>::iterator dwc_tdc = DWC_TDCs.begin(); dwc_tdc != DWC_TDCs.end(); dwc_tdc++) delete (*dwc_tdc);
      
    DWC_TDCs.clear();
    for (size_t dwc_tdc_index=1; dwc_tdc_index<=N_DWCs; dwc_tdc_index++) DWC_TDCs.push_back(new CAEN_v1290(dwc_tdc_index));     //possibly have to add some hardware address here

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
    m_WireChamberTriggerController = new WireChamberTriggerController(DWC_TDCs, m_sync_orm);


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
    outTree->Branch("N_DWC", &N_DWCs);
    outTree->Branch("ids", &dwc_ids_for_filling);
    outTree->Branch("channel0_timestamps", &dwc_ch0_min_timestamp_for_filling);
    outTree->Branch("channel1_timestamps", &dwc_ch1_min_timestamp_for_filling);
    outTree->Branch("channel2_timestamps", &dwc_ch2_min_timestamp_for_filling);
    outTree->Branch("channel3_timestamps", &dwc_ch3_min_timestamp_for_filling);

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
      boost::thread threadVec[DWC_TDCs.size()];
      for( int dwc_index=0; dwc_index<(int)DWC_TDCs.size(); dwc_index++) {
        threadVec[dwc_index]=boost::thread(readTDCThread, DWC_TDCs[dwc_index]);
        threadVec[dwc_index].join();
      }

      if (stopping) continue;


      m_WireChamberTriggerController->readoutCompleted();

      //making an EUDAQ event
      eudaq::RawDataEvent ev(EVENT_TYPE,m_run,m_ev);
      
      //clear the old entries
      dwc_event_for_filling.clear();
      dwc_ids_for_filling.clear();
      dwc_ch0_min_timestamp_for_filling.clear();
      dwc_ch1_min_timestamp_for_filling.clear();
      dwc_ch2_min_timestamp_for_filling.clear();
      dwc_ch3_min_timestamp_for_filling.clear();


      for (size_t dwc_tdc_index=0; dwc_tdc_index<N_DWCs; dwc_tdc_index++) {
        tdcData thisData = DWC_TDCs[dwc_tdc_index]->GetCurrentData(); 
        std::vector<unsigned int> dataForEUDAQ;
        dataForEUDAQ.push_back(thisData.event);        
        dwc_event_for_filling.push_back(thisData.event);
        dataForEUDAQ.push_back(thisData.ID);
        dwc_ids_for_filling.push_back(thisData.ID);
        dataForEUDAQ.push_back(thisData.timeStamp_ch0);
        dwc_ch0_min_timestamp_for_filling.push_back(thisData.timeStamp_ch0);
        dataForEUDAQ.push_back(thisData.timeStamp_ch1);
        dwc_ch1_min_timestamp_for_filling.push_back(thisData.timeStamp_ch1);
        dataForEUDAQ.push_back(thisData.timeStamp_ch2);
        dwc_ch2_min_timestamp_for_filling.push_back(thisData.timeStamp_ch2);
        dataForEUDAQ.push_back(thisData.timeStamp_ch3);
        dwc_ch3_min_timestamp_for_filling.push_back(thisData.timeStamp_ch3);
        ev.AddBlock(dwc_tdc_index, dataForEUDAQ);
      }
      std::cout<<"Writing event nr. : "<<m_ev<<" to the tree"<<std::endl;
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
    std::vector<CAEN_v1290*> DWC_TDCs;
    ACQ_MODE m_acqmode;
    ipbus::IpbusHwController*  m_sync_orm;
    WireChamberTriggerController *m_WireChamberTriggerController;


    //generated for each run
    TTree* outTree;

    std::vector<int> dwc_event_for_filling;
    std::vector<int> dwc_ids_for_filling;
    std::vector<unsigned int> dwc_ch0_min_timestamp_for_filling;  //e.g. x min
    std::vector<unsigned int> dwc_ch1_min_timestamp_for_filling;  //e.g. x max
    std::vector<unsigned int> dwc_ch2_min_timestamp_for_filling;  //e.g. y min
    std::vector<unsigned int> dwc_ch3_min_timestamp_for_filling;  //e.g. y max
    

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
