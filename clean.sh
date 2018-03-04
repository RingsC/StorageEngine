#! /bin/bash
cd bin
rm -f *
cd ../src/backend
make clean
cd ../interface
make clean
cd ../../teststorageEngine
make clean
cd ../testStorageEngineDll
make clean
cd ../bootstrap
make clean
cd ../walreceiver
make clean
cd ../walsender
make clean
