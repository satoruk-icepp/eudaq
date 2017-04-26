# Ipbus and Eudaq : 
Started from RpiTestProducer (in sandbox) and miniTLUProducer (and Controller).

To build and compile the code:

 -Install ipbus : [link](https://svnweb.cern.ch/trac/cactus/wiki/uhalQuickTutorial#HowtoInstalltheIPbusSuite)

 -Then 

  `$ export LD_LIBRARY_PATH=/opt/cactus/lib:$LD_LIBRARY_PATH`

  `$ export PATH=/opt/cactus/bin:$PATH`

 -From eudaq root dir:

  `$ cd build`

  `$ cmake -DBUILD_cmshgcal=ON ../`

  `$ make install -j4`

Run This producer:
 `$ ./STARTRUN cmshgcal`
  
  - Load config file in eudaq/producers/cmshgcal/conf/hgcal.conf, and start the run. It will readout the data and store them in file.
  - Start orm emulation: see [myIpbusTest](https://github.com/asteencern/ipbus-test/tree/hgcal-test)

