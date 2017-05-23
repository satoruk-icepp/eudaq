#include <iostream>
#include "CAEN_V1290.hpp"
#include "Unpacker.hpp"


int main() {
	std::vector<WORD> data;
	
	CAEN_V1290* device = new CAEN_V1290();
	Unpacker* unpacker = new Unpacker(1);

	std::cout<<std::endl<<"++++++++ Test of some functions ++++++++"<<std::endl<<std::endl; 
	std::cout<<"Configuring the device: "<<device->Config()<<std::endl;
	std::cout<<"Initialising the device: "<<device->Init()<<std::endl;

	std::cout<<std::endl<<"...Done"<<std::endl;



	std::cout<<std::endl<<"+++++++++++++++++++"<<std::endl;
	std::cout<<"Enabling test mode"<<std::endl;
	std::cout<<"+++++++++++++++++++"<<std::endl<<std::endl;
	
	device->EnableTDCTestMode(0x1111);
	std::cout<<"Testing mode enabled"<<std::endl;
	device->SoftwareTrigger();
	std::cout<<"Software trigger fired"<<std::endl;
	device->Read(data);
	
	std::cout<<"Read events after software trigger: "<<std::endl;
	for (size_t i=0; i<data.size(); i++) {
		std::cout<<"data "<<i<<" : "<<data[i]<<std::endl;
	}
	unpacker->ConvertTDCData(data);
	std::cout<<"Disabling test mode"<<std::endl;
	device->DisableTDCTestMode();
}