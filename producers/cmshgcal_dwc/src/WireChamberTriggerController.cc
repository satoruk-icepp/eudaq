#include "WireChamberTriggerController.h"


WireChamberTriggerController::WireChamberTriggerController(CAEN_V1290* rdout, 
ipbus::IpbusHwController* sync) : m_state(WAIT), m_acqmode(BEAMTEST), m_run(0), m_evt(0), m_rdout_orm(rdout), m_sync_orm(sync), m_gotostop(false), m_rdoutcompleted(false){}


void WireChamberTriggerController::startrunning( uint32_t runNumber, const ACQ_MODE mode ) {
  m_run=runNumber;
  m_evt=0;
  m_state=WAIT;
  m_acqmode=mode;
  m_gotostop=false;

  m_sync_orm->SetRegister("SYNC.BUSY",0);
  m_sync_orm->SetRegister("SYNC.RUN_NUMBER",runNumber);
  m_sync_orm->SetRegister("SYNC.EVT_NUMBER",0);

  while(true){
    boost::this_thread::sleep( boost::posix_time::milliseconds(2000) );
    break;
  }

  switch( m_acqmode ){
    case BEAMTEST : runBeamTest(); break;
    case PEDESTAL : runPedestal(); break;
    case DEBUG : runDebug(); break;
  }
}

//Deprecated for now
void WireChamberTriggerController::runBeamTest() {
  while(1){
    if( m_state==END_RUN ) break;
    if( m_sync_orm->ReadRegister("SYNC.BUSY")!=1 ) continue;
    m_state=BUSY;
    m_evt++;
    m_sync_orm->SetRegister("SYNC.EVT_NUMBER",m_evt);
    while(true){
      bool fin=true;
      if( fin ) break;
    }
    m_rdoutcompleted=false;
    m_state=RDOUT_RDY;
    while(true){
      if( m_rdoutcompleted==true ){
        m_state=RDOUT_FIN;
        break;
      }
      boost::this_thread::sleep( boost::posix_time::microseconds(1) );
    }
    m_sync_orm->SetRegister("SYNC.BUSY",0);
    if( m_gotostop ) m_state=END_RUN;
    else m_state=WAIT;
  }
  return;
}

//Deprecated for now
void WireChamberTriggerController::runPedestal() {
  while(true){
    if( m_state==END_RUN ) break;
    m_state=BUSY;
    m_evt++;
    m_sync_orm->SetRegister("SYNC.BUSY",1);
    m_sync_orm->SetRegister("SYNC.EVT_NUMBER",m_evt);
    while(true) {
      bool fin=true;
      if( fin ) break;
    }
    m_rdoutcompleted=false;
    m_state=RDOUT_RDY;
    while(true){
      if( m_rdoutcompleted ){
        m_state=RDOUT_FIN;
        break;
      }
      boost::this_thread::sleep( boost::posix_time::microseconds(1) );
      //mandatory at least on my VM otherwise the CPU never gets time to set m_rdoutcompleted
    }
    m_sync_orm->SetRegister("SYNC.BUSY",0);

    if( m_gotostop ) m_state=END_RUN;
    else m_state=WAIT;
  }
  return;
}


void startEventGeneratorThread( WireChamberTriggerController* trg_ctrl) {
  //this loop generates fake events and sets the trigger to the busy state
  std::default_random_engine generator;
  std::normal_distribution<double> gaussianInSpill(50.0,5.0);
  std::normal_distribution<double> gaussianInRun(1000.0,100.0);
  int N_events_per_spill=10;
  int N_event_counter = 0;

  while(true){
    if( trg_ctrl->getState()==END_RUN ) break;    

    double deltaT = std::max(0.0, gaussianInSpill(generator));
    if (! (N_event_counter % N_events_per_spill)) deltaT = std::max(0.0, gaussianInRun(generator));

    boost::this_thread::sleep( boost::posix_time::milliseconds(deltaT) );

    N_event_counter++;

    if( trg_ctrl->getState()==END_RUN ) break;    //check if the state has been changed to end run in between the waiting period
    trg_ctrl->setState(BUSY);
  }

}


void WireChamberTriggerController::runDebug() {
  boost::thread eventGenThread = boost::thread(startEventGeneratorThread, this);
  
  while(true){
    if( m_state==END_RUN ) break;
    boost::this_thread::sleep( boost::posix_time::milliseconds(1) );    //one event every 10seconds
    if (m_state!=BUSY) continue;
    
    m_evt++;

    //m_sync_orm->SetRegister("SYNC.BUSY",1);
    //m_sync_orm->SetRegister("SYNC.EVT_NUMBER",m_evt);

    boost::this_thread::sleep( boost::posix_time::milliseconds(5) );

    m_rdoutcompleted=false;
    m_state=RDOUT_RDY;
    
    while(true){
    
      if( m_rdoutcompleted==false ){    
        boost::this_thread::sleep( boost::posix_time::milliseconds(10) );
      }
      else {
        m_state=RDOUT_FIN;
        break;
      }
      if( m_gotostop ) break;
    }

    if( m_gotostop ) {
      this->setState(END_RUN);
    }
    else m_state=WAIT;
  }
  std::cout << "on quit la trigger running thread" << std::endl;
  return;

}
