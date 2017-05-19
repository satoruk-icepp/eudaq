#include <iostream>
#include "CAEN_V1290.hpp"
#include "Unpacker.hpp"


int main() {
	std::vector<WORD> data;
	
	CAEN_V1290* device = new CAEN_V1290();
	Unpacker* unpacker = new Unpacker(1);

	std::cout<<std::endl<<"++++++++ Test of some functions ++++++++"<<std::endl<<std::endl; 

	std::cout<<"Setting the handle: "<<device->SetHandle(1)<<std::endl;
	std::cout<<"Configuring the device: "<<device->Config()<<std::endl;
	std::cout<<"Initialising the device: "<<device->Init()<<std::endl;
	//std::cout<<"Writing some word to the device: "<<device->OpWriteTDC(CAEN_V1290_TRMATCH_OPCODE)<<std::endl;	//private member functions
	std::cout<<"Reading some TDC data: "<<device->Read(data)<<std::endl;
	//std::cout<<"Checking the status of the device posterior to the read: "<<device->CheckStatusAfterRead()<<std::endl;	//private member functions
	std::cout<<"Clearing the buffer: "<<device->BufferClear()<<std::endl;
	std::cout<<"Clearing the device: "<<device->Clear()<<std::endl;

	std::cout<<std::endl<<"...Done"<<std::endl;

	data.clear();
	device->generatePseudoData(data);

	for (size_t i=0; i<data.size(); i++) {
		std::cout<<"data "<<i<<" : "<<data[i]<<std::endl;
	}

	unpacker->ConvertTDCData(data);
}