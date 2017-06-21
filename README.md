* This branch contains the progress of the DWC inclusion into the common DAQ. 
* A standalone, simple DWC producer for the upcoming July testbeam has been implemented in ```producers/cmshgcal_dwc``` that copes with the readout of one CAEN_V1290N TDC (16-channels) and interprets the signals as timestamps from up to four delay wire chambers.
* Standalone read-out functionality for testing is available in ```/sandbox/CAEN_v1290_Standalone```.

####Instructions for installation of EUDAQ on pcminn03:

1. ```cd build;```
2. ```cmake3 -DBUILD_cmshgcal_dwc=ON ..```
3. ```make -j4 install``` 

####Run:

* Need access to the computer with keyboard, mouse and screen. 
* Execution via ssh-connection requires the xserver packages to be installed which are not so far.
* ```./STARTRUN cmshgcal_dwc```

####Configuration:

* Choose the file in /producers/cmshgcal_dwc/conf/hgcaldwc.conf
It containes all relevant CAENV1290 TDC settings and assigns channels 0-3 to the according channels of a hypothetical delay-wire-chamber.


#####Please refer to the README.md in the ```tb2017``` branch for further references.