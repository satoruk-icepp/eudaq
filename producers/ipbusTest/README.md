# Ipbus and Eudaq : 
Started from RpiTestProducer (in sandbox) and miniTLUProducer (and Controller).

To build and compile the code:

 * Install ipbus : [link](https://svnweb.cern.ch/trac/cactus/wiki/uhalQuickTutorial#HowtoInstalltheIPbusSuite)

 * Then do:  
  `export LD_LIBRARY_PATH=/opt/cactus/lib:$HOME/root/lib:$LD_LIBRARY_PATH`  
  `export PATH=/opt/cactus/bin:$HOME/root/bin:$PATH`
  
 * From eudaq root dir:
```
cd build
cmake -DBUILD_ipbusTest=ON ../
make install -j4
```

* Run this ipbus example:
  * Start an ipbus skiroc emulation, (see [myIpbusTest](https://github.com/asteencern/ipbus-test)). It will produce random data. 
  * Then start the euDAQ with command `./STARTRUN ipbustest` from the root directory
  * Modify the config file in *eudaq/producers/ipbusTest/conf/IpbusTestConfig.conf* to add the location of the **ConnectionFile**      
    * Now you can start the run. It will readout the data and store them in file.
