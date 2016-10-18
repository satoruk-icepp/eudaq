#ifndef PROCESSOR_HH_
#define PROCESSOR_HH_

#include<set>
#include<vector>
#include<string>
#include<queue>
#include<memory>
#include<thread>
#include<mutex>
#include<atomic>
#include<condition_variable>

#include"Event.hh"
#include"Configuration.hh"

#include"Factory.hh"

namespace eudaq {
  class Processor;

#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT
  Factory<Processor>;
  extern template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&)>&
  Factory<Processor>::Instance<std::string&>();
  extern template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)(std::string&&)>&
  Factory<Processor>::Instance<std::string&&>();
  extern template DLLEXPORT std::map<uint32_t, typename Factory<Processor>::UP_BASE (*)()>&
  Factory<Processor>::Instance<>();
#endif
  

  using ProcessorUP = Factory<Processor>::UP_BASE;
  using ProcessorSP = Factory<Processor>::SP_BASE;
  using ProcessorWP = Factory<Processor>::WP_BASE;

  
  using PSSP = ProcessorSP;
  using PSWP = ProcessorWP;
  using EVUP = EventUP;
  
  class DLLEXPORT Processor: public std::enable_shared_from_this<Processor>{
  public:
    enum STATE:uint32_t{
      STATE_UNCONF,
      STATE_READY,
      STATE_ERROR,
      STATE_THREAD_UNCONF,
      STATE_THREAD_READY,
      STATE_THREAD_BUSY,
      STATE_THREAD_ERROR
    };

    enum FLAG{
      FLAG_HUB_RUN=1,
      FLAG_HUB_BUSY=2,
      FLAG_PDC_RUN=4,
      FLAG_PDC_BUSY=8,
      FLAG_CSM_RUN=16,
      FLAG_CSM_BUSY=32
    };

    static ProcessorSP MakeShared(std::string pstype, std::string cmd);
    
    Processor(std::string pstype, std::string cmd);
    
    virtual ~Processor();
    virtual void ProcessUserEvent(EventUP ev);
    virtual void ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par);
    virtual void ProduceEvent();
    void ConsumeEvent();
    
    void HubProcessing();
    void Processing(EventUP ev);
    void AsyncProcessing(EventUP ev);
    void SyncProcessing(EventUP ev);
    void ForwardEvent(EventUP ev);
    void RegisterProcessing(ProcessorSP ps, EventUP ev);

    void ProcessSysCmd(std::string cmd_name, std::string cmd_par);
    void ProcessSysEvent(EventUP ev);
    
    void RunProducerThread();
    void RunConsumerThread();
    void RunHubThread(); //delilver
    
    void AddNextProcessor(ProcessorSP ps);
    void AddUpstream(ProcessorWP ps);
    void UpdatePSHub(ProcessorWP ps);

    void ProcessCmd(const std::string& cmd_list);

    
    bool IsHub(){return m_flag&FLAG_HUB_RUN ;};
    bool IsAsync(){return m_flag&FLAG_CSM_RUN ;};
    std::string GetType(){return m_pstype;};
    uint32_t GetID(){return m_psid;};
    STATE GetState(){return m_state;};
    void Print(std::ostream &os, uint32_t offset=0) const;
    
    ProcessorSP operator>>(ProcessorSP psr);
    ProcessorSP operator>>(const std::string& stream_str);
    ProcessorSP operator<<(const std::string& cmd_str);

    ProcessorSP operator+(const std::string& evtype);
    ProcessorSP operator-(const std::string& evtype);
    ProcessorSP operator<<=(EventUP ev);
    
  private:
    std::string m_pstype;
    uint32_t m_psid;
    ProcessorWP m_ps_hub;
    std::vector<ProcessorWP> m_ps_upstr;
    std::vector<std::pair<ProcessorSP, std::set<uint32_t>>> m_pslist_next;
    
    std::set<uint32_t> m_evlist_white; //TODO: for processor input
    std::set<uint32_t> m_evlist_black; //TODO: for processor output
    std::vector<std::pair<std::string, std::string>> m_cmdlist_init;    
    
    std::queue<EventUP> m_fifo_events; //fifo async
    std::queue<std::pair<ProcessorSP, EventUP> > m_fifo_pcs; //fifo hub
    std::mutex m_mtx_fifo;
    std::mutex m_mtx_pcs;
    std::mutex m_mtx_state;
    std::mutex m_mtx_list;
    std::condition_variable m_cv;
    std::condition_variable m_cv_pcs;
    std::atomic<STATE> m_state;
    std::atomic<int> m_flag;
  };

  template <typename T>
  ProcessorSP operator>>(ProcessorSP psl, T&& t){
    return *psl>>std::forward<T>(t);
  }

  template <typename T>
  ProcessorSP operator<<(ProcessorSP psl, T&& t){
    return *psl<<std::forward<T>(t);
  }

  template <typename T>
  ProcessorSP operator+(ProcessorSP psl, T&& t){
    return (*psl)+std::forward<T>(t);
  }

  template <typename T>
  ProcessorSP operator-(ProcessorSP psl, T&& t){
    return (*psl)-std::forward<T>(t);
  }
  
  template <typename T>
  ProcessorSP operator<<=(ProcessorSP psl, T&& t){
    return (*psl)<<=std::forward<T>(t);
  }
  
}


#endif
