# Ipbus and Eudaq : 
Started from RpiTestProducer (in sandbox) and miniTLUProducer (and Controller).

To build and compile the code:

* Install ipbus : [link](https://svnweb.cern.ch/trac/cactus/wiki/uhalQuickTutorial#HowtoInstalltheIPbusSuite)

* Import libraries paths if needed:
```
export LD_LIBRARY_PATH=/opt/cactus/lib:$LD_LIBRARY_PATH
export PATH=/opt/cactus/bin:$PATH
```

* Compile. From eudaq root dir:
```
cd build
cmake -DBUILD_cmshgcal=ON ../
make install -j4
```

* Start ORM data emulation: see [ipbus-test](https://github.com/asteencern/ipbus-test/tree/hgcal-test) repository. This can be launched from any computer with ipbus installed. Then, IP address of that computer should be put in *producers/cmshgcal/etc/connection.xml* instead of *localhost*.
* Run this producer (from root of eudaq): `./STARTRUN cmshgcal`
  * Load config file from *eudaq/producers/cmshgcal/conf/hgcal.conf*, and start the run. It will readout the data and store them in file.


