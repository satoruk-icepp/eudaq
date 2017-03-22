#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#include <ostream>
#include <vector>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <boost/crc.hpp>
#include <boost/cstdint.hpp>

#include "IpbusTestController.h"

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "IPBUSTEST";


// Declare a new class that inherits from eudaq::Producer
class IpbusTestProducer : public eudaq::Producer {
public:
  // The constructor must call the eudaq::Producer constructor with the name
  // and the runcontrol connection string, and initialize any member variables.
  IpbusTestProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      m_run(0), m_ev(0), m_ipbus(nullptr), m_uhalLogLevel(5), m_blockSize(963), m_ipbusstate(STATE_UNCONF){}

private:
  unsigned m_run, m_ev, m_uhalLogLevel, m_blockSize;
  std::unique_ptr<ipbus::IpbusTestController> m_ipbus;
  enum HWState {
    STATE_ERROR,
    STATE_UNCONF,
    STATE_GOTOCONF,
    STATE_CONFED,
    STATE_GOTORUN,
    STATE_RUNNING,
    STATE_GOTOSTOP,
    STATE_GOTOTERM
  } m_ipbusstate;

public:
  bool checkCRC( const std::string & crcNodeName )
  {
    std::cout << "Start checkCRC" << std::endl;
    std::vector<uint32_t> tmp=m_ipbus->getData();
    uint32_t *data=&tmp[0];
    boost::crc_32_type checksum;
    checksum.process_bytes(data,(std::size_t)tmp.size());
    uint32_t crc=m_ipbus->ReadRegister(crcNodeName.c_str());
    std::ostringstream os( std::ostringstream::ate );
    if( crc==checksum.checksum() ){
      os.str(""); os << "checksum success (" << crc << ")";
      EUDAQ_DEBUG( os.str().c_str() );
      std::cout << os.str().c_str() << std::endl;
      return true;
    }
    os.str(""); os << "in IpbusTestProducer.cxx : checkCRC( const std::string & crcNodeName ) -> checksum fail : sent " << crc << "\t compute" << checksum.checksum() << "-> try again";
    EUDAQ_WARN( os.str().c_str() );
    std::cout << os.str().c_str() << std::endl;
    return false;
  }

  void MainLoop() 
  {
    while (m_ipbusstate != STATE_GOTOTERM){
      if( m_ipbusstate != STATE_RUNNING ) {
	std::cout << "je suis dans la main loop" << std::endl;
	std::cout << "m_ipbusstate = " << m_ipbusstate << std::endl;
	eudaq::mSleep(1000);
      }
      if (m_ipbusstate == STATE_UNCONF) {
	eudaq::mSleep(100);
	continue;
      }    

      if (m_ipbusstate == STATE_RUNNING) {
	if( m_ipbus->ReadRegister( "SKI0.READENABLE" )!=1 ) continue;
	std::cout << "wE READ data" << std::endl;
	m_ipbus->ReadDataBlock("SKI0.FIFO",m_blockSize);
	if( checkCRC("SKI0.CRC")!=1 ) continue;
	std::cout << "we send event" << std::endl;
	eudaq::RawDataEvent ev(EVENT_TYPE,m_run,m_ev);
	ev.AddBlock( 1,m_ipbus->getData() );
	SendEvent(ev);
	std::cout << "we change register" << std::endl;
	m_ipbus->SetRegister( "SKI0.READENABLE",0 );
	m_ipbus->SetRegister( "SKI0.WRITEENABLE",1 );
	m_ev++;
	m_ipbus->ResetTheData();
	continue;
      }
      if (m_ipbusstate == STATE_GOTOSTOP) {
	SendEvent( eudaq::RawDataEvent::EORE(EVENT_TYPE,m_run,++m_ev) );
	m_ipbusstate = STATE_CONFED;
	eudaq::mSleep(100);
	continue;
      }
    };
  }

private:
  // This gets called whenever the DAQ is configured
  virtual void OnConfigure(const eudaq::Configuration & config) 
  {
    std::cout << "Configuring: " << config.Name() << std::endl;
    
    // Do any configuration of the hardware here
    // Configuration file values are accessible as config.Get(name, default)
    m_uhalLogLevel = config.Get("UhalLogLevel", 5);
    m_blockSize = config.Get("DataBlockSize",962);

    std::cout << "Uhal log level = " << m_uhalLogLevel << std::endl;

    if (m_ipbus)
      m_ipbus = nullptr;
    m_ipbus = std::unique_ptr<ipbus::IpbusTestController>( new ipbus::IpbusTestController(config.Get("ConnectionFile","file://connection.xml"),
											  config.Get("DeviceName","controlhub2")) );
    // At this point, connection between daq and ipbus hw should be ok

    m_ipbusstate=STATE_CONFED;
    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) 
  {
    m_ipbusstate = STATE_GOTORUN;
    SetStatus(eudaq::Status::LVL_OK, "Starting");
    
    m_run = param;
    m_ev = 0;
    
    // It must send a BORE to the Data Collector
    // change this as soon as we have a data format
    std::cout << "on envois un evt tout pourri" << std::endl;
    SendEvent( eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run) );
    std::cout << "ca a du marcher" << std::endl;

    m_ipbusstate = STATE_RUNNING;
    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Started");
  }

  // // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    try {
      SetStatus(eudaq::Status::LVL_OK, "Stopping");
      //m_tlu->SetTriggerVeto(1);
      m_ipbusstate = STATE_GOTOSTOP;
      while (m_ipbusstate == STATE_GOTOSTOP) {
	eudaq::mSleep(1000); //waiting for EORE being send
      }
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
      
    } catch (const std::exception & e) {
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error: " + eudaq::to_string(e.what()));
    } catch (...) {
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    }
  }

  // // This gets called when the Run Control is terminating,
  // // we should also exit.
  virtual void OnTerminate() {
    SetStatus(eudaq::Status::LVL_OK, "Terminating...");
    m_ipbusstate = STATE_GOTOTERM;
    eudaq::mSleep(1000);
  }
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("IPbusTest Producer", "1.0", "The Producer task for the IpbusTest");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "IPBUSTEST", "string", "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    IpbusTestProducer producer(name.Value(),rctrl.Value());
    std::cout << "on demarre la main loop" << std::endl;
    producer.MainLoop();
    std::cout << "Quitting" << std::endl;
    eudaq::mSleep(300);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
