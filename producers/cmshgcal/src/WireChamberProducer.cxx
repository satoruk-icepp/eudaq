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

#include "CAEN_v1290.h"

//Start from ExampleProducer in main/exe/src

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "DelayWireChambers";

class WireChamberProducer : public eudaq::Producer {
  public:

  WireChamberProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), stopping(false), done(false), started(0) {
      std::cout<<"I am initialising..."<<std::endl;
    }


  virtual void OnConfigure(const eudaq::Configuration & config) {
    N_DWCs = config.Get("NumberOfWireChambers", 2);
    std::cout<<"Number of DWCs: "<<N_DWCs<<std::endl;

    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_ev = 0;
    EUDAQ_INFO("Start Run: ");
    // It must send a BORE to the Data Collector
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
    //bore.SetTag("EXAMPLE", eudaq::to_string(m_exampleparam));
    SendEvent(bore);

    SetStatus(eudaq::Status::LVL_OK, "Running");
    started=true;
  }

  // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    EUDAQ_INFO("Stopping Run");
    started=false;
    // Set a flag to signal to the polling loop that the run is over
    stopping = true;

    // wait until all events have been read out from the hardware
    while (stopping) {
      eudaq::mSleep(20);
    }

    // Send an EORE after all the real events have been sent
    // You can also set tags on it (as with the BORE) if necessary
    SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));
  }

  // This gets called when the Run Control is terminating,
  // we should also exit.
  virtual void OnTerminate() {
    EUDAQ_INFO("Terminating...");
    done = true;
  }

  void ReadoutLoop() {
    while (!done) {
      //come up with some logic to read out the wire chambers
      if (true) {
        if (stopping) {
          stopping = false;
        }
        eudaq::mSleep(20);
        continue;
      }

      if (!started) {
        eudaq::mSleep(20);
        continue;
      }
      // If we get here, there must be data to read out
      // Create a RawDataEvent to contain the event data to be sent
      eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);

      /*    Replace wire chambers here
      for (unsigned plane = 0; plane < hardware.NumSensors(); ++plane) {
      // Read out a block of raw data from the hardware
      std::vector<unsigned char> buffer = hardware.ReadSensor(plane);
      // Each data block has an ID that is used for ordering the planes later
      // If there are multiple sensors, they should be numbered incrementally

      // Add the block of raw data to the event
      ev.AddBlock(plane, buffer);
      }
      */  


      SendEvent(ev);
      m_ev++;
    }
  }

  private:
    unsigned m_run, m_ev;
    bool stopping, done,started;

    unsigned int N_DWCs;
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
