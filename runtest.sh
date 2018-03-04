#!/bin/sh

ulimit -n 8192
now_dir=`pwd`
init_dir="$now_dir/bin/"
libdata_dir="$now_dir/datalib/"
dlldata_dir="$now_dir/datadll/"
test_dir="$now_dir/bin/"

cd $now_dir
if [ -d "$libdata_dir" ]
then
 rm -rf  "$libdata_dir"
fi

if [ -d "$dlldata_dir" ]
then
 rm -rf  "$dlldata_dir"
fi

mkdir -p "$libdata_dir"  
mkdir -p "$dlldata_dir"   

cd $test_dir
export LD_LIBRARY_PATH="$now_dir"/../../../3rd_party/lib/Linux/x86/boost:"$now_dir"/bin

./initDataDir "$libdata_dir"
cp -r "$libdata_dir"* "$dlldata_dir"
#cd $test_dir

#run lib testcase and transaction persistence testcase

./testEngineStorageLIB "$libdata_dir" -c TransPersistence
./testEngineStorageLIB "$libdata_dir" -c TransPersistence --run_test=t_init_func_/TransPersistence/*
./testEngineStorageLIB "$libdata_dir" -c TransExit --run_test=t_init_func_/TransExitCase1/*
./testEngineStorageLIB "$libdata_dir" -c TransExit --run_test=t_init_func_/TransExitCase1/*
./testEngineStorageLIB "$libdata_dir" -c TransExit --run_test=t_init_func_/TransExitCase2/*
./testEngineStorageLIB "$libdata_dir" -c TransExit --run_test=t_init_func_/TransExitCase2/*
./testEngineStorageLIB "$libdata_dir" -c TransExit --run_test=t_init_func_/TransExitCase3/*
./testEngineStorageLIB "$libdata_dir" -c TransExit --run_test=t_init_func_/TransExitCase3/*

#run dll testcase and transaction persistence testcase
./testEngineStorageDLL "$dlldata_dir" -c TransPersistence
./testEngineStorageDLL "$dlldata_dir" -c TransPersistence --run_test=t_init_func_/TransPersistence/*
