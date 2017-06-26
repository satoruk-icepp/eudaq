
echo ">> Installation of CAEN libraries"

TOP=$PWD;

cd $PWD;

echo ">> ...CAENVMELib-2.50"
cd $PWD/libraries/CAENVMELib-2.50/lib;
./install_x64;

echo ">> ...CAENComm-1.2"
cd $PWD/../../CAENComm-1.2/lib;
./install_x64;

echo ">> ...CAENDigitizer_2.7.9"
cd $PWD/../../CAENDigitizer_2.7.9;
./install_64;

echo ">> ...Done¨"
cd $TOP;