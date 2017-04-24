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

#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

const size_t RAW_EV_SIZE=30787;

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "RPI";

// Declare a new class that inherits from eudaq::Producer
class RpiTestProducer : public eudaq::Producer {
  public:
    // The constructor must call the eudaq::Producer constructor with the name
    // and the runcontrol connection string, and initialize any member variables.
    RpiTestProducer(const std::string & name, const std::string & runcontrol)
      : eudaq::Producer(name, runcontrol),
      m_run(0), m_ev(0), m_stopping(false), m_done(false),
      m_sockfd1(0), m_sockfd2(0), m_running(false), m_configured(false){}
    
    // This gets called whenever the DAQ is configured
    virtual void OnConfigure(const eudaq::Configuration & config) {
      std::cout << "Configuring: " << config.Name() << std::endl;
      
      // Do any configuration of the hardware here
      // Configuration file values are accessible as config.Get(name, default)
      m_exampleparam = config.Get("Parameter", 0);
      m_ski = config.Get("Ski", 0);

      m_portTCP = config.Get("portTCP", 55511);
      m_portUDP = config.Get("portUDP", 55512);
      m_rpi_1_ip = config.Get("RPI_1_IP", "127.0.0.1");

      std::cout << "Example Parameter = " << m_exampleparam << std::endl;
      std::cout << "Example SKI Parameter = " << m_ski << std::endl;

      // At the end, set the status that will be displayed in the Run Control.
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");

      m_configured=true;
    }

    // This gets called whenever a new run is started
    // It receives the new run number as a parameter
    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_ev = 0;

      std::cout << "Start Run: " << m_run << std::endl;

      // openinig socket:

      bool con = OpenConnection();
      if (!con) {
	EUDAQ_WARN("Socket conection failed: no server. Can't start the run.");
	SetStatus(eudaq::Status::LVL_ERROR, "No Socket.");
	return;
      }

      SendCommand("START_RUN");

      char answer[20];
      bzero(answer, 20);
      int n = recv(m_sockfd1, answer, 20, 0);
      if (n <= 0) {
	std::cout<<n<<" Something is wrong with socket, we can't start the run..."<<std::endl;
	SetStatus(eudaq::Status::LVL_ERROR, "Can't Start Run on Hardware side.");
	return;
      }
      
      else {
	std::cout<<"Answer to START_RUN: "<<answer<<std::endl;
	if (strncmp(answer,"GOOD_START",10)!=0) {
	  std::cout<<n<<" Not expected answer from server, we can't start the run..."<<std::endl;
	  SetStatus(eudaq::Status::LVL_ERROR, "Can't Start Run on Hardware side.");
	  return;
	}
      }

      // Let's open a file for raw data:
      char rawFilename[256];
      sprintf(rawFilename, "../data/HexaData_Run%04d.raw", m_run); // The path is relative to eudaq/bin
      m_rawFile.open(rawFilename, std::ios::binary);
      
      //fout = fopen("myOUT.txt", "w");
      //fprintf(fout,"Total number of events: NN \n");
      
      // If we're here, then the Run was started on the Hardware side (TCP server will send data)
      
      // It must send a BORE to the Data Collector
      eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
      // You can set tags on the BORE that will be saved in the data file
      // and can be used later to help decoding
      bore.SetTag("MyTag", eudaq::to_string(m_exampleparam));
      // Send the event to the Data Collector
      SendEvent(bore);

      m_running=true;
      m_stopped=false;
      m_stopping=false;

      // At the end, set the status that will be displayed in the Run Control.
      SetStatus(eudaq::Status::LVL_OK, "Running");
      std::cout<<"Started it!"<<std::endl;
    }

    // This gets called whenever a run is stopped
    virtual void OnStopRun() {
      std::cout << "Stopping Run" << std::endl;

      // Send command to Hardware:
      SendCommand("STOP_RUN");


      // Set a flag to signal to the polling loop that the run is over
      SetStatus(eudaq::Status::LVL_OK, "Stopping...");

      eudaq::mSleep(1000);

      m_stopping = true;

      while (!m_stopped){
	char answer[20];
	bzero(answer, 20);
	int n = recv(m_sockfd1, answer, 20, 0);
	if (n <= 0) {
	  SetStatus(eudaq::Status::LVL_ERROR, "Can't Stop Run on Hardware side.");
	  return;
	}
	else {
	  std::cout<<"Answer to STOP_RUN: "<<answer<<std::endl;
	  if (strncmp(answer,"STOPPED_OK",10)!=0) {
	    std::cout<<"Something is wrong, we can't stop the run..."<<std::endl;
	    SetStatus(eudaq::Status::LVL_ERROR, "Can't Start Run on Hardware side.");
	    return;
	  }
	  
	  CloseConnection();
	  m_stopping = false;
	  m_running  = false;
	  m_stopped  = true;
	  
	}
      }

      m_rawFile.close();
      // If we were running, send signal to stop:
      //m_running=false;
      //fclose(fout);
      
      // wait until all events have been read out from the hardware
      while (m_stopping) {
        eudaq::mSleep(20);
      }

      m_stopped = true;
      // Send an EORE after all the real events have been sent
      // You can also set tags on it (as with the BORE) if necessary
      SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));

      SetStatus(eudaq::Status::LVL_OK, "Stopped.");
      std::cout << "Stopped it!" << std::endl;

    }

    // This gets called when the Run Control is terminating,
    // we should also exit.
    virtual void OnTerminate() {
      SetStatus(eudaq::Status::LVL_OK, "Terminating...");
      std::cout << "Terminating..." << std::endl;
      m_done = true;
    }

    // This is just an example, adapt it to your hardware
    void ReadoutLoop() {
      // Loop until Run Control tells us to terminate
      while (!m_done) {

	if (!m_running)
	  {
	    eudaq::mSleep(2000);
	    EUDAQ_DEBUG("Not Running; but sleeping");
	    SetStatus(eudaq::Status::LVL_USER, "Sleeping");
	    continue;
	  }

        //if (m_sockfd1 <= 0) {
	if (m_sockfd1 <= 0 || m_sockfd2 <= 0) {
	  EUDAQ_DEBUG("No sockets. Not Running; but sleeping");
	  SetStatus(eudaq::Status::LVL_USER, "No Sockets yet in Readout Loop.  sfd1="+
		    eudaq::to_string(m_sockfd1)+" sfd2="+eudaq::to_string(m_sockfd2));
	  eudaq::mSleep(100);
	  continue;
	}

	// **********
	// If we are below this point, we listen for data
	// ***********

	SetStatus(eudaq::Status::LVL_DEBUG, "Running");
	EUDAQ_DEBUG("Running again");

	const int bufsize = RAW_EV_SIZE;
	char buffer[bufsize];
	bzero(buffer, bufsize);

	// TCP version: int n = recv(m_sockfd1, buffer, bufsize, 0);
	std::cout<<"Before recvfrom. Are you blocking us?  sockLen="<<sockLen<<std::endl;
	int n = recvfrom(m_sockfd2, buffer, RAW_EV_SIZE, 0, (struct sockaddr *)&dst_addr, &sockLen);
	std::cout<<"After recvfrom. Un-blocked..."<<std::endl;
	if (n <= 0) {
	  if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
	    std::cout<<"n = "<<n<<" Socket timed out. Errno="<<errno<<std::endl;

	    std::time_t current_time = std::time(NULL);

	    if (current_time - m_last_readout_time < 30)
	      continue; // No need to stop the run, just wait
	    else {
	      // It's been too long without data, we shall give a warning
	      SetStatus(eudaq::Status::LVL_WARN, "No Data");
	      EUDAQ_WARN("Sockets: No data for too long..");
	    }

	  eudaq::mSleep(200);

	  }
	  else {
	    std::cout<<" n="<<n<<" errno="<<errno<<std::endl;
	    SetStatus(eudaq::Status::LVL_WARN, "Nothing to read from socket...");
	    EUDAQ_WARN("Sockets: ERROR reading from socket (it's probably disconnected)");
	    continue;
	  }
	}

	// We are here if there is data and it's size is > 0

	std::cout<<" After recv. Size of message recieved: n="<<n<<std::endl;
	//std::cout<<"In ReadoutLoop.  Here is the message from Server: \n"<<buffer<<std::endl;

	/*
	if (m_stopping){
	  // We have sent STOP_RUN command, let's see if we receive a confirmation:
	  if (strncmp("STOPPED_OK",(char*)buffer,10)==0){
	    
	    EUDAQ_EXTRA("Received Confirmation.. Stopping from readout loop");

	    CloseConnection();
	    m_stopping = false;
	    m_running  = false;
	    
	    eudaq::mSleep(100);
	    continue;
	  }
	  // If not, then it's probably the data continuing to come, let's save it.
	  
	}
	*/

	// If we get here, there must be data to read out
	m_last_readout_time = std::time(NULL);
	std::cout <<"size = "<<n<< "  m_last_readout_time:"<< m_last_readout_time<<std::endl;
	std::cout<<"First byte of the RAW event: "<<eudaq::to_hex(buffer[0])<<std::endl;
	std::cout<<"Last two bytes of the event: "<<eudaq::to_hex(buffer[RAW_EV_SIZE-2])
		 <<" "<<eudaq::to_hex(buffer[RAW_EV_SIZE-1])<<std::endl;

	if (n==(int)RAW_EV_SIZE && (unsigned char)buffer[0]==0xff){
	  // This is good data (at first sight)

	  // Write it into raw file: 
	  m_rawFile.write(buffer, RAW_EV_SIZE);

	  	
	  // Create a RawDataEvent to contain the event data to be sent
	  eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);
	  

	  std::array<std::array<unsigned int, 1924>,4> decoded = decode_raw((unsigned char*)buffer);
	  //unsigned int dati[4][128][13];
	  
	  std::vector<unsigned short> dataBlockZS;
	  
	  for (int ski = 0; ski < 4; ski++ ){
	    
	    //fprintf(fout, "Event %d Chip %d RollMask %x \n", m_ev, ski, decoded[ski][1920]);	    
	    
	    const int ped = 190;  // pedestal
	    const int noi = 20;   // noise

	    // ----------
	    // -- Based on the rollmask, lets determine which time-slices (frames) to add
	    //
	    unsigned int r = decoded[ski][1920];
	    //printf("Roll mask = %d \n", r);
	    int k1 = -1, k2 = -1;
	    for (int p=0; p<13; p++){
	      //printf("pos = %d, %d \n", p, r & (1<<12-p));
	      if (r & (1<<12-p)) {
		if (k1==-1)
		  k1 = p;
		else if (k2==-1)
		  k2 = p;
		else
		  printf("Error: more than two positions in roll mask! %x \n",r);
	      }
	    }

	    //printf("k1 = %d, k2 = %d \n", k1, k2);
	      
	    // Check that k1 and k2 are consecutive
	    char last = -1;
	    if (k1==0 && k2==12) { last = 0;}
	    else if (abs(k1-k2)>1)
	      EUDAQ_WARN("The k1 and k2 are not consecutive! abs(k1-k2) = "+ eudaq::to_string(abs(k1-k2)));
	    //printf("The k1 and k2 are not consecutive! abs(k1-k2) = %d\n", abs(k1-k2));
	    else
	      last = k2;

	    //printf("last = %d\n", last);
	    // k2+1 it the begin TS
	    
	    // Let's assume we can somehow determine the main frame
	    const char mainFrameOffset = 5; // offset of the pulse wrt trigger (k2 rollmask)
	    const char mainFrame = (last+mainFrameOffset)%13;


	    const int tsm2 = (((mainFrame - 2) % 13) + ((mainFrame >= 2) ? 0 : 13))%13;
	    const int tsm1 = (((mainFrame - 1) % 13) + ((mainFrame >= 1) ? 0 : 13))%13;
	    const int ts0  = mainFrame;
	    const int ts1  = (mainFrame+1)%13;
	    const int ts2  = (mainFrame+2)%13;

	    //printf("TS 0 to be saved: %d\n", tsm2);
	    //printf("TS 1 to be saved: %d\n", tsm1);
	    //printf("TS 2 to be saved: %d\n", ts0);
	    //printf("TS 3 to be saved: %d\n", ts1);
	    //printf("TS 4 to be saved: %d\n", ts2);

	    // -- End of main frame determination
	    

	    
	    for (int ch = 0; ch < 64; ch+=2){

	      const int chArrPos = 63-ch; // position of the hit in array
	      //int chargeLG = decoded[ski][mainFrame*128 + chArrPos] & 0x0FFF;
	      const int chargeHG = decoded[ski][mainFrame*128 + chArrPos] & 0x0FFF;
	      // ZeroSuppress:
	      if (chargeHG - (ped+noi) < 0) continue;
	      
	      dataBlockZS.push_back(ski*100+ch);

	      // Low gain (save 5 time-slices total):
	      dataBlockZS.push_back(decoded[ski][tsm2*128 + chArrPos] & 0x0FFF);
	      dataBlockZS.push_back(decoded[ski][tsm1*128 + chArrPos] & 0x0FFF);
	      dataBlockZS.push_back(decoded[ski][ts0*128 + chArrPos] & 0x0FFF);
	      dataBlockZS.push_back(decoded[ski][ts1*128 + chArrPos] & 0x0FFF);
	      dataBlockZS.push_back(decoded[ski][ts2*128 + chArrPos] & 0x0FFF);

	      // High gain:
	      dataBlockZS.push_back(decoded[ski][tsm2*128 + 64 + chArrPos] & 0x0FFF);
	      dataBlockZS.push_back(decoded[ski][tsm1*128 + 64 + chArrPos] & 0x0FFF);
	      dataBlockZS.push_back(decoded[ski][ts0*128 + 64 + chArrPos] & 0x0FFF);
	      dataBlockZS.push_back(decoded[ski][ts1*128 + 64 + chArrPos] & 0x0FFF);
	      dataBlockZS.push_back(decoded[ski][ts2*128 + 64 + chArrPos] & 0x0FFF);


	      // Filling TOA (stop falling clock)
	      dataBlockZS.push_back(decoded[ski][1664 + chArrPos] & 0x0FFF);

	      // Filling TOA (stop rising clock)
	      dataBlockZS.push_back(decoded[ski][1664 + 64 + chArrPos] & 0x0FFF);

	      // Filling TOT (slow)
	      dataBlockZS.push_back(decoded[ski][1664 + 2*64 + chArrPos] & 0x0FFF);

	      // Filling TOT (fast)
	      dataBlockZS.push_back(decoded[ski][1664 + 3*64 + chArrPos] & 0x0FFF);

	      // Global TS 14 MSB (it's gray encoded?). Not decoded here!
	      dataBlockZS.push_back(decoded[ski][1921]);
	      
	      // Global TS 12 LSB + 1 extra bit (binary encoded)
	      dataBlockZS.push_back(decoded[ski][1922]);

	      
	      //for (int ts = 0 ; ts < 13 ; ts++){
	      //dati[ski][ch][ts] = decoded[ski][ts*128+ch] & 0x0FFF;
	      //fprintf(fout, "%d  ", dati[ski][ch][ts]);
	      //} // end of ts

	      
	    } // end of ch
	  } //end of ski
	  
	  ev.AddBlock(0, dataBlockZS);
	  SendEvent(ev);

	}
	else {
	  if (n!=(int)RAW_EV_SIZE){
	    
	    EUDAQ_WARN("The event size is not right! n="+eudaq::to_string(n));
	    SetStatus(eudaq::Status::LVL_WARN, "Wrong event size.");
	  }
	  if (buffer[0]!=0xff){
	    EUDAQ_WARN("First byte is not FF. It is: "+eudaq::to_hex(buffer[0]));
	    SetStatus(eudaq::Status::LVL_WARN, "Corrupted Data");
	  }
	}
	
	m_ev++;
	continue;

      }// end of while(done) loop

    } // end of ReadLoop

    bool OpenConnection()
    {
      // This opens a TCP socket connection (available for both in-data and out-data)
      // We are going to set up this producer as CLIENT
      // Using this tutorial as guidence: http://www.linuxhowtos.org/C_C++/socket.htm
      // and code from:
      // https://github.com/EUDAQforLC/eudaq/blob/ahcal_telescope_december2016_v1.6/producers/calice/AHCAL/src/AHCALProducer.cc
      // TCP socket is to be used for messaging only (START, STOP, and confirmations)
      
      m_sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
      if (m_sockfd1<0) {
	EUDAQ_ERROR("Can't open TCP socket: m_sockfd="+eudaq::to_string(m_sockfd1));
	return 0;
      }
      
      //struct sockaddr_in dst_addr;
      bzero((char *) &dst_addr, sizeof(dst_addr));
      dst_addr.sin_family = AF_INET;
      dst_addr.sin_addr.s_addr = inet_addr(m_rpi_1_ip.c_str());
      dst_addr.sin_port = htons(m_portTCP);
 
      int ret = connect(m_sockfd1, (struct sockaddr *) &dst_addr, sizeof(dst_addr));
      if (ret != 0) {
	SetStatus(eudaq::Status::LVL_WARN, "No Socket.");
	EUDAQ_WARN("Can't connect() to socket: ret="+eudaq::to_string(ret)+"  sockfd="+eudaq::to_string(m_sockfd1));
	return 0;
      }

      // -----
      // And here is an UDP socket.
      // It is to be used for receiving data

      m_sockfd2 = socket(AF_INET, SOCK_DGRAM, 0);
      if (m_sockfd2<0) {
        EUDAQ_ERROR("Can't open UDP socket: m_sockfd="+eudaq::to_string(m_sockfd2));
        return 0;
      }

      // Re-using the same dst_addr
      bzero((char *) &dst_addr, sizeof(dst_addr));
      dst_addr.sin_family = AF_INET;
      //dst_addr.sin_addr.s_addr = inet_addr(m_rpi_1_ip.c_str());
      dst_addr.sin_port = htons(m_portUDP);

      struct hostent *server = gethostbyname("localhost");
      bcopy((char *)server->h_addr, 
	    (char *)&dst_addr.sin_addr.s_addr, server->h_length);

      std::cout<<"dst_addr: "<<dst_addr.sin_addr.s_addr<<std::endl;

      sockLen = sizeof(dst_addr);

      bind(m_sockfd2, (struct sockaddr *)&dst_addr, sizeof(dst_addr));
	
      //char buffer[]="HELLO";
      //int n = sendto(m_sockfd2, buffer, strlen(buffer), 0, (const struct sockaddr *)&dst_addr, sizeof(struct sockaddr_in));

      // ***********************
      // This makes the recv() command non-blocking.
      // After the timeout, it will get an error which we can catch and continue the loops

      struct timeval timeout;
      timeout.tv_sec = 20;
      timeout.tv_usec = 0;
      setsockopt(m_sockfd2, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
      //****************

      std::cout << "  Opened TCP socket: "<<m_sockfd1 << std::endl;
      return 1;
    }


    void CloseConnection()
    {
      //std::unique_lock<std::mutex> myLock(m_mufd);
      close(m_sockfd1);
      std::cout << "  Closed TCP socket: "<<m_sockfd1 << std::endl;
      m_sockfd1 = -1;
      close(m_sockfd2);
      std::cout << "  Closed UDP socket: "<<m_sockfd2 << std::endl;
      m_sockfd2 = -1;

    }

    void SendCommand(const char *command, int size=0) {
      
      if (size == 0) size = strlen(command);
      
      if (m_sockfd1 <= 0)
	std::cout << "SendCommand(): cannot send command because connection is not open." << std::endl;
      else {
	size_t bytesWritten = write(m_sockfd1, command, size);
	if (bytesWritten < 0)
	  std::cout << "There was an error writing to the TCP socket" << std::endl;
	else
	  std::cout << bytesWritten << " out of " << size << " bytes is  written to the TCP socket" << std::endl;
      }
    }

    // Methods for raw data conversion


    unsigned int gray_to_brady(unsigned int gray)
    {
      // Code from:
      //https://github.com/CMS-HGCAL/TestBeam/blob/826083b3bbc0d9d78b7d706198a9aee6b9711210/RawToDigi/plugins/HGCalTBRawToDigi.cc#L154-L170
      unsigned int result = gray & (1 << 11);
      result |= (gray ^ (result >> 1)) & (1 << 10);
      result |= (gray ^ (result >> 1)) & (1 << 9);
      result |= (gray ^ (result >> 1)) & (1 << 8);
      result |= (gray ^ (result >> 1)) & (1 << 7);
      result |= (gray ^ (result >> 1)) & (1 << 6);
      result |= (gray ^ (result >> 1)) & (1 << 5);
      result |= (gray ^ (result >> 1)) & (1 << 4);
      result |= (gray ^ (result >> 1)) & (1 << 3);
      result |= (gray ^ (result >> 1)) & (1 << 2);
      result |= (gray ^ (result >> 1)) & (1 << 1);
      result |= (gray ^ (result >> 1)) & (1 << 0);
      return result;
    }



    std::array<std::array<unsigned int,1924>,4> decode_raw(unsigned char * raw){

      // Code from Sandro with minor modifications
      //unsigned int ev[4][1924];
      std::array<std::array<unsigned int, 1924>,4> ev;
      
      unsigned char x;
      for(int i = 0; i < 1924; i = i+1){
	for (int k = 0; k < 4; k = k + 1){
	  ev[k][i] = 0;
	}
      }
      
      for(int  i = 0; i < 1924; i = i+1){
	for (int j=0; j < 16; j = j+1){
	  x = raw[1 + i*16 + j];
	  x = x & 15; // <-- APZ: Not sure why this is needed.
	  for (int k = 0; k < 4; k = k + 1){
	    ev[k][i] = ev[k][i] | (unsigned int) (((x >> (3 - k) ) & 1) << (15 - j));
	  }
	}
      }

      unsigned int t, bith;
      for(int k = 0; k < 4 ; k = k +1 ){
	for(int i = 0; i < 128*13; i = i + 1){
	  bith = ev[k][i] & 0x8000;

	  t = gray_to_brady(ev[k][i] & 0x7fff);
	  ev[k][i] =  bith | t;
	}
      }
    
      return ev;
    }


    
    

    
  private:
    unsigned m_run, m_ev, m_exampleparam;
    unsigned m_ski;
    bool m_stopping, m_stopped, m_done, m_started, m_running, m_configured;
    int m_sockfd1, m_sockfd2; //TCP and UDP socket connection file descriptors (fd)
    //std::mutex m_mufd;

    std::string m_rpi_1_ip;
    int m_portTCP, m_portUDP;

    std::time_t m_last_readout_time;

    std::ofstream m_rawFile;

    unsigned sockLen;
    //FILE *fout;

    struct sockaddr_in dst_addr;
    
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("RPI Test producer", "1.0",
      "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
      "tcp://localhost:44000", "address",
      "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "RPI", "string",
      "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    RpiTestProducer producer(name.Value(), rctrl.Value());
    // And set it running...
    producer.ReadoutLoop();
    // When the readout loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
