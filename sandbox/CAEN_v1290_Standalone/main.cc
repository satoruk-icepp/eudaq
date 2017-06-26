#include <iostream>
#include "CAEN_V1290.hpp"
#include "Unpacker.hpp"


int main() {
	std::vector<WORD> data;
	
	CAEN_V1290* device = new CAEN_V1290();
	Unpacker* unpacker = new Unpacker(1);

	std::cout<<"Configuring the device: "<<device->Config()<<std::endl;
	
	sleep(1);
	std::cout<<"Initialising the device: "<<std::endl;
	device->Init();

	std::cout<<std::endl<<"...Done"<<std::endl;



	//std::cout<<std::endl<<"+++++++++++++++++++"<<std::endl;
	//std::cout<<"Enabling test mode"<<std::endl;
	//std::cout<<"+++++++++++++++++++"<<std::endl<<std::endl;
	sleep(1);
	//device->EnableTDCTestMode(0x1111);
	//std::cout<<"Testing mode enabled"<<std::endl;
	
	
	int N_counter = 0;
	while(true) {
		//std::cout<<"Software trigger fired"<<std::endl;
		//device->SoftwareTrigger();
		
		device->Read(data);

		for (size_t i=0; i<data.size(); i++) {
			std::cout<<"data "<<i<<" : "<<data[i]<<std::endl;
		}
		//unpacker->ConvertTDCData(data);
		sleep(0.001);
		
		N_counter++;
		//if (N_counter==100) {
		//	std::cout<<"Quitting..."<<std::endl;
			//break;
		//} 
	}
	

	//sleep(1);
	//std::cout<<"Disabling test mode"<<std::endl;
	//device->DisableTDCTestMode();
}