Standalone CAEN V1290 readout code
=========================================

Minimal code to interface the targeted DWC producer (EUDAQ) with the CAEN V1290 TDC(s).
Initial commit corresponds to the minimal implementation within the H4DAQ implementation (see References).

## Prerequisites
Code is tested on SLC6 with the following additional dependencies:

* ```gcc version 4.4.7```
* ```cmake version 2.8.12.2```
* ```CAENVMELib```
* ```CAENComm Library```
* ```CAENDigitizer Library```

## Installation on pcminn03
The test program is installed and can be run on pcminn03 in the test-lab (b.27). Its location is in ```/home/cmsdaq/DAQ/eudaq/sandbox/CAEN_v1290_Standalone```.
It is currently connected to the VME crate. When running ```./test``` the according LEDs signal that read and write access via the VME crate is occuring.

Next (23 May 2017):

* Feed oscilloscope signals into the TDC and stress-test the interface. 

## How to install and run
After downloading this directory:

1. ```cd thisdir```
2. ```. install_CAEN_libraries.sh```
3. ```cd build```
4. ```cmake ..```
5. ```make```
6. ```. test```


## Directory structure
* ```/build```  -  build directory
* ```/include/CAEN_V1290.hpp```  -  Header file in which device specific access codes are defined.
* ```/include/Unpacker.hpp```  -  Header file for the unpacker class that converts the bitstreams into physical information.
* ```/libraries/```  -  Contains the compiled and necessary libraries for the interfacing to the CAEN TDC.
* ```/src/CAEN_V1290.cpp```  -  Source file that implements the interface.
* ```/src/Unpacker.cpp```  -  Source file for the unpacker class that converts the bitstreams into physical information.
* ```/CMakeLists.txt```  -  cmake steering file to automatically produce make-files.
* ```/install_CAEN_libraries.sh``` -  Installation script that places the compiled libraries into the appropriate directories.
* ```/main.cc```  -   Main program that instantiates a TDC device and calls some functions for testing.


## References
* [Link to the CAEN_V1290 implementation within the H4DAQ on GitHub](https://github.com/cmsromadaq/H4DAQ/blob/master/src/CAEN_V1290.cpp)
* [Link to CAENVME library manual](https://www.slac.stanford.edu/grp/ssrl/spear/epics/site/gtr/V1724_REV19.pdf)
* [Download CAENVMELib](http://www.caen.it/csite/CaenProd.jsp?idmod=689&parent=38)  (available in ```/libraries```)
* [Download CAENComm Library](http://www.caen.it/csite/CaenProd.jsp?idmod=684&parent=43)  (available in ```/libraries```)
* [Download CAENDigitizer Library](http://www.caen.it/csite/CaenProd.jsp?parent=38&idmod=717)  (available in ```/libraries```)