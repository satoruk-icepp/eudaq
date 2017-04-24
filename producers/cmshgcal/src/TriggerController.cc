#include "TriggerController.h"
#include "boost/thread/thread.hpp"

TriggerController::TriggerController(std::vector< ipbus::IpbusHwController* > rdout, 
				     ipbus::IpbusHwController* sync) : m_state(WAIT),
								       m_acqmode(BEAMTEST),
								       m_run(0),
								       m_evt(0),
								       m_rdout_orms(rdout),
								       m_sync_orm(sync),
								       m_gotostop(false),
								       m_rdoutcompleted(false)
{;}

void TriggerController::readoutCompleted() 
{ 
    m_rdoutcompleted=true; 
    //    std::cout << "receive rdoutcompleted command : rdoutcompleted = " << m_rdoutcompleted << std::endl; 
}

void TriggerController::startrunning( uint32_t runNumber, const ACQ_MODE mode )
{
  m_run=runNumber;
  m_evt=0;
  m_state=WAIT;
  m_acqmode=mode;
  m_gotostop=false;
  
  m_sync_orm->SetRegister("SYNC.BUSY",0);
  m_sync_orm->SetRegister("SYNC.RUN_NUMBER",runNumber);
  m_sync_orm->SetRegister("SYNC.EVT_NUMBER",0);
  for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
    (*it)->SetRegister("RDOUT.RDOUT_RDY",0); 
    (*it)->SetRegister("RDOUT.RUN_NUMBER",runNumber);
    (*it)->SetRegister("RDOUT.EVT_NUMBER",0);
  }

  while(1){
    boost::this_thread::sleep( boost::posix_time::milliseconds(2000) );
    break;
  }
  switch( m_acqmode ){
  case BEAMTEST : runBeamTest(); break;
  case PEDESTAL : runPedestal(); break;
  case DEBUG : runDebug(); break;
  }
}

void TriggerController::runBeamTest()
{
  while(1){
    if( m_state==END_RUN ) break;
    if( m_sync_orm->ReadRegister("SYNC.BUSY")!=1 ) continue;
    m_state=BUSY;
    m_evt++;
    m_sync_orm->SetRegister("SYNC.EVT_NUMBER",m_evt);
    while(1){
      bool fin=true;
      for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
	if( (*it)->ReadRegister("RDOUT.RDOUT_RDY")!=1 ) 
	  fin=false;
      if( fin ) break;
    }
    m_rdoutcompleted=false;
    m_state=RDOUT_RDY;
    while(1){
      if( m_rdoutcompleted==true ){
	m_state=RDOUT_FIN;
	break;
      }
      boost::this_thread::sleep( boost::posix_time::microseconds(1) );
    }
    m_sync_orm->SetRegister("SYNC.BUSY",0);
    for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
      (*it)->SetRegister("RDOUT.RDOUT_RDY",0); 
      (*it)->SetRegister("RDOUT.EVT_NUMBER",m_evt);
    }
    if( m_gotostop ) m_state=END_RUN;
    else m_state=WAIT;
  }
  return;
}

void TriggerController::runPedestal()
{
  while(1){
    if( m_state==END_RUN ) break;
    m_state=BUSY;
    m_evt++;
    m_sync_orm->SetRegister("SYNC.BUSY",1);
    m_sync_orm->SetRegister("SYNC.EVT_NUMBER",m_evt);
    while(1){
      bool fin=true;
      for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
	if( (*it)->ReadRegister("RDOUT.RDOUT_RDY")!=1 ) 
	  fin=false;
      if( fin ) break;
    }
    m_rdoutcompleted=false;
    m_state=RDOUT_RDY;
    while(1){
      if( m_rdoutcompleted ){
	m_state=RDOUT_FIN;
	break;
      }
      boost::this_thread::sleep( boost::posix_time::microseconds(1) );
      //mandatory at least on my VM otherwise the CPU never gets time to set m_rdoutcompleted
    }
    m_sync_orm->SetRegister("SYNC.BUSY",0);
    for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
      (*it)->SetRegister("RDOUT.RDOUT_RDY",0); 
      (*it)->SetRegister("RDOUT.EVT_NUMBER",m_evt);
    }
    if( m_gotostop ) m_state=END_RUN;
    else m_state=WAIT;
  }
  return;
}

void TriggerController::runDebug()
{
  while(1){
    if( m_state==END_RUN ) break;
    m_state=BUSY;
    boost::this_thread::sleep( boost::posix_time::milliseconds(200) );
    std::cout << "Debug mode : start evt acquisition : " << std::endl;
    m_evt++;
    m_sync_orm->SetRegister("SYNC.BUSY",1);
    m_sync_orm->SetRegister("SYNC.EVT_NUMBER",m_evt);
    //int i=0;
    //while(1){
      // bool fin=true;
      // for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it )
      // 	if( (*it)->ReadRegister("RDOUT.RDOUT_RDY")!=1 ) 
      // 	  fin=false;
      // if( fin ) break;
      // else{
      //std::cout << "wait all orms are ready for read out" << std::endl;
      std::cout << "in debug mode we wait 500 ms instead of waiting all orms are ready for readout" << std::endl;
      boost::this_thread::sleep( boost::posix_time::milliseconds(5) );
      //break;
	//if(i>10) break;
      //}
      //i++;
      //}
    m_rdoutcompleted=false;
    m_state=RDOUT_RDY;
    while(1){
      if( m_rdoutcompleted==false ){
	std::cout << "wait daq finished the read out" << std::endl;
	boost::this_thread::sleep( boost::posix_time::milliseconds(100) );
      }
      else {
	m_state=RDOUT_FIN;
	std::cout << "daq finished the read out and the state is " << m_state << std::endl;
	break;
      }
      if( m_gotostop ) break;
    }
    m_sync_orm->SetRegister("SYNC.BUSY",0);
    for( std::vector<ipbus::IpbusHwController*>::iterator it=m_rdout_orms.begin(); it!=m_rdout_orms.end(); ++it ){
      (*it)->SetRegister("RDOUT.RDOUT_RDY",0); 
      (*it)->SetRegister("RDOUT.EVT_NUMBER",m_evt);
    }
    if( m_gotostop ) m_state=END_RUN;
    else m_state=WAIT;
  }
  std::cout << "on quit la trigger running thread" << std::endl;
  return;
}
