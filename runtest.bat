@echo off      
set now_dir=%cd%
set init_dir=%now_dir%\..\..\..\bin\Win32\Debug\
set lib_dir=%now_dir%\..\..\..\3rd_party\lib\Windows\x86\boost
set path=%path%;%lib_dir%
set data_dir=%now_dir%
set libdata_dir=%data_dir%\data_lib
set dlldata_dir=%data_dir%\data_dll
set lib_test_dir=%init_dir%
set dll_test_dir=%init_dir%

cd %now_dir%
if exist %libdata_dir% rd %libdata_dir% /s /q
if exist %dlldata_dir% rd %dlldata_dir% /s /q
mkdir %libdata_dir%
mkdir %dlldata_dir%

cd %init_dir%
call bootstrap_storage.exe %libdata_dir% 
xcopy %libdata_dir% %dlldata_dir% /s/e

cd %lib_test_dir%
REM run lib testcase and transaction persistence testcase step1 

 call testStorageEngineLib.exe %libdata_dir% -c TransPersistence

REM run transaction persistence testcase step2 ONLY

 call testStorageEngineLib.exe %libdata_dir% -c TransPersistence --run_test=t_init_func_/TransPersistence/*

REM run transaction exit testcase step1
call testStorageEngineLib.exe %libdata_dir% -c TransExit --run_test=t_init_func_/TransExitCase1/*
REM run transaction exit testcase step2
call testStorageEngineLib.exe %libdata_dir% -c TransExit --run_test=t_init_func_/TransExitCase1/*

call testStorageEngineLib.exe %libdata_dir% -c TransExit --run_test=t_init_func_/TransExitCase2/*
call testStorageEngineLib.exe %libdata_dir% -c TransExit --run_test=t_init_func_/TransExitCase2/*

call testStorageEngineLib.exe %libdata_dir% -c TransExit --run_test=t_init_func_/TransExitCase3/*
call testStorageEngineLib.exe %libdata_dir% -c TransExit --run_test=t_init_func_/TransExitCase3/*

cd %dll_test_dir%
REM run dll testcase and transaction persistence testcase step1 

call testStorageEngineDll.exe %dlldata_dir% -c TransPersistence

REM run transaction persistence testcase step2 ONLY

 call testStorageEngineDll.exe %dlldata_dir% -c TransPersistence --run_test=t_init_func_/TransPersistence/*
cd %now_dir%
