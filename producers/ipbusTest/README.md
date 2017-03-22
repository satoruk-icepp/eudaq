# Ipbus and Eudaq : 
Started from RpiTestProducer (in sandbox) and miniTLUProducer (and Controller).

To build and compile the code:

 -Install ipbus : [link](https://svnweb.cern.ch/trac/cactus/wiki/uhalQuickTutorial#HowtoInstalltheIPbusSuite)

 -Then 

  `$ export LD_LIBRARY_PATH=/opt/cactus/lib:$HOME/root/lib:$LD_LIBRARY_PATH`
  `$ export PATH=/opt/cactus/bin:$HOME/root/bin:$PATH`

 -From eudaq root dir:

  `$ cd build`
  `$ cmake -DBUILD_ipbusTest=ON ../`
  `$ make instal -j4`

Run this ipbus example:

 -Start an ipbus hardware emulation, fill it with crap data : see [myIpbusTest](https://github.com/asteencern/ipbus-test) 

 `$ ./STARTRUN ipbustest`

 - Load config file in eudaq/producers/ipbusTest/conf/IpbusTestConfig.conf, and start the run. It will readout the data and store them in file.
