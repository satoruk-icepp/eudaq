#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>
#ifndef _WIN32
#include <sys/file.h>
#endif

class CaliceHodoscopeProducer : public eudaq::Producer {
  public:
  CaliceHodoscopeProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void Mainloop();
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("CaliceHodoscopeProducer");
private:
  bool m_flag_ts;
  bool m_flag_tg;
  uint32_t m_plane_id;
  FILE* m_file_lock;
  std::chrono::milliseconds m_ms_busy;
  std::thread m_thd_run;
  bool m_exit_of_run;
};
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<CaliceHodoscopeProducer, const std::string&, const std::string&>(CaliceHodoscopeProducer::m_id_factory);
}
CaliceHodoscopeProducer::CaliceHodoscopeProducer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_exit_of_run(false){  
}
void CaliceHodoscopeProducer::DoInitialise(){
  auto ini = GetInitConfiguration();
  std::string lock_path = ini->Get("EX0_DEV_LOCK_PATH", "ex0lockfile.txt");
  m_file_lock = fopen(lock_path.c_str(), "a");
#ifndef _WIN32
  if(flock(fileno(m_file_lock), LOCK_EX|LOCK_NB)){ //fail
    EUDAQ_THROW("unable to lock the lockfile: "+lock_path );
  }
#endif
}

void CaliceHodoscopeProducer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_plane_id = conf->Get("EX0_PLANE_ID", 0);
  m_ms_busy = std::chrono::milliseconds(conf->Get("EX0_DURATION_BUSY_MS", 1000));
  m_flag_ts = conf->Get("EX0_ENABLE_TIEMSTAMP", 0);
  m_flag_tg = conf->Get("EX0_ENABLE_TRIGERNUMBER", 0);
  if(!m_flag_ts && !m_flag_tg){
    EUDAQ_WARN("Both Timestamp and TriggerNumber are disabled. Now, Timestamp is enabled by default");
    m_flag_ts = false;
    m_flag_tg = true;
  }
}
void CaliceHodoscopeProducer::DoStartRun(){
  m_exit_of_run = false;
  m_thd_run = std::thread(&CaliceHodoscopeProducer::Mainloop, this);
}
void CaliceHodoscopeProducer::DoStopRun(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}
void CaliceHodoscopeProducer::DoReset(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
#ifndef _WIN32
  flock(fileno(m_file_lock), LOCK_UN);
#endif
  fclose(m_file_lock);
  m_thd_run = std::thread();
  m_ms_busy = std::chrono::milliseconds();
  m_exit_of_run = false;
}
void CaliceHodoscopeProducer::DoTerminate(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
  fclose(m_file_lock);
}
void CaliceHodoscopeProducer::Mainloop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  uint32_t trigger_n = 0;
  uint8_t x_pixel = 16;
  uint8_t y_pixel = 16;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> position(0, x_pixel*y_pixel-1);
  std::uniform_int_distribution<uint32_t> signal(0, 255);
  while(!m_exit_of_run){
    auto ev = eudaq::Event::MakeUnique("Ex0Raw");  
    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;
    if(m_flag_ts){
      std::chrono::nanoseconds du_ts_beg_ns(tp_trigger - tp_start_run);
      std::chrono::nanoseconds du_ts_end_ns(tp_end_of_busy - tp_start_run);
      ev->SetTimestamp(du_ts_beg_ns.count(), du_ts_end_ns.count());
    }
    if(m_flag_tg)
      ev->SetTriggerN(trigger_n);

    std::vector<uint8_t> hit(x_pixel*y_pixel, 0);
    hit[position(gen)] = signal(gen);
    std::vector<uint8_t> data;
    data.push_back(x_pixel);
    data.push_back(y_pixel);
    data.insert(data.end(), hit.begin(), hit.end());
    
    uint32_t block_id = m_plane_id;
    ev->AddBlock(block_id, data);
    SendEvent(std::move(ev));
    trigger_n++;
    std::this_thread::sleep_until(tp_end_of_busy);
  }
}
