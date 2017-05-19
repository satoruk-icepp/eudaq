#ifndef H_WIRECHAMBERTRIGGERCONTROLLER
#define H_WIRECHAMBERTRIGGERCONTROLLER

#include <iostream>
#include <vector>

#include "IpbusHwController.h"
#include "CAEN_v1290.h"

#include "boost/thread/thread.hpp"

#include <random>
#include <algorithm>


enum STATES{ 
  WAIT,
  BUSY,
  RDOUT_RDY,
  RDOUT_FIN,
  END_RUN
};

enum ACQ_MODE{
  BEAMTEST,
  PEDESTAL,
  DEBUG
};

class WireChamberTriggerController {
 public:
  WireChamberTriggerController(){;}
  WireChamberTriggerController(std::vector< CAEN_v1290* > rdout, ipbus::IpbusHwController* sync);
  void startrunning(uint32_t, const ACQ_MODE);

  bool checkState( STATES st ) const { return m_state==st; }
  void stopRun() { m_gotostop=true; std::cout << "receive stop command : gotostop = " << m_gotostop << std::endl; }
  void readoutCompleted() {m_rdoutcompleted=true;};

  void setAcqMode( const ACQ_MODE mode ){m_acqmode=mode;}
  ACQ_MODE getAcqMode(  ) const {return m_acqmode;}
  
  uint32_t eventNumber() const {return m_evt;}
  uint32_t runNumber() const {return m_run;}

  STATES getState() const {return m_state;}
  void setState(STATES state) {m_state = state;}


 private:
  void runBeamTest();
  void runPedestal();
  void runDebug();

  std::vector< CAEN_v1290* > m_rdout_orms;
  ipbus::IpbusHwController*  m_sync_orm;
  STATES m_state;
  ACQ_MODE m_acqmode;
  uint32_t m_run;
  uint32_t m_evt;
  bool m_gotostop;
  bool m_rdoutcompleted;
};

#endif
