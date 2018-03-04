#!/usr/bin/env python
import sys
import os
import subprocess
import shutil
import time
def script_dir():
    path = sys.path[0]
    if os.path.isdir(path):
        return path
    else :
        return os.path.dirname(path)

def run_test(cases,out=None,err=None):
    if not isinstance(cases,list):
        raise TypeError

    for case in cases:
        print case
        subprocess.Popen(case,stdout=out,stderr=err,shell=True).wait()

def onerror(fun,path,excinfo):
    pass

def WaitAndRemove(path):
    while os.path.exists(path):
        try:
            shutil.rmtree(path,onerror = onerror)
        except:
            time.sleep(1)
    
def KillProcessByName(name):
    if sys.platform == 'win32':
        pass
    else:
        killFile = open('kill.txt','w')
        subprocess.Popen('ps -A | grep -i ' + name,stdout=killFile,shell=True).wait()
        killFile.close()
        killFile = open('kill.txt','r')
        for line in killFile:
            splitLine = line.split()
            killPid = splitLine[0]
            subprocess.Popen('kill -s 9 ' + killPid,shell=True).wait()
        os.remove('kill.txt')

def main(param):
    #set directory and exe
    KillProcessByName('testStorage')
    curDir = script_dir().replace('\\','/') + '/'
    if 1 == len(curDir):
        curDir = ''

    _top_dir = curDir + '../../../'
    _test_dir = _top_dir + "testdir/"
    if not os.path.exists(_test_dir):
        os.mkdir(_test_dir)
    _top_dir = os.path.abspath(_top_dir) + '/'
    _bin_dir=_top_dir + 'dist/server/bin/'
    _lib_dir=_top_dir + 'dist/server/bin/';
    libData = os.path.abspath(_test_dir + 'libData')
    dllData = os.path.abspath(_test_dir + 'dllData')
    ThreadData = os.path.abspath(_test_dir + 'ThreadData')

    bootstrap=_bin_dir + 'bootstrap_storage '
    libTest= _bin_dir + 'testStorageEngineLIB '
    dllTest= _bin_dir + 'testStorageEngineDLL '
    ThreadTest= _bin_dir + 'testThreadTransaction '

    #create data dir
    WaitAndRemove(libData)
    WaitAndRemove(dllData)
    WaitAndRemove(ThreadData)

    os.mkdir(dllData)
    os.mkdir(libData)
    os.mkdir(ThreadData)

    print curDir
    #run lib testcase and transaction persistence testcase
    libCases = [
     bootstrap + libData
    ,libTest  + libData +  ' -c TransPersistence'
    #,libTest + libData +  ' -c TransPersistence --run_test=t_init_func_/TransPersistence/*'
    #,'python ' + curDir + 'replication-hot.py'
    ,libTest + libData +  ' -c TransExit --run_test=t_init_func_/TransExitCase1/*'
    ,libTest + libData +  ' -c TransExit --run_test=t_init_func_/TransExitCase1/*'
    ,libTest + libData +  ' -c TransExit --run_test=t_init_func_/TransExitCase2/*'
    ,libTest + libData +  ' -c TransExit --run_test=t_init_func_/TransExitCase2/*'
    ,libTest + libData +  ' -c TransExit --run_test=t_init_func_/TransExitCase3/*'
    ,libTest + libData +  ' -c TransExit --run_test=t_init_func_/TransExitCase3/*'
    ,libTest + libData +  ' -c TransExit --run_test=t_init_func_/TransExitCase7/*'
    ,libTest + libData +  ' -c TransExit --run_test=t_init_func_/TransExitCase7/*'
    ]

    #run dll testcase and transaction persistence testcase
    dllCases = [
     bootstrap  + dllData
    ,dllTest + dllData + ' -c TransPersistence' + ' test_replication_check=' + curDir + 'serverconfig/CHECK;'
    #,dllTest + dllData + ' -c TransPersistence --run_test=t_init_func_/TransPersistence/*'
    #,'python ' + curDir + 'testStorageEngineDll/src/backup_recovery/BackupRecoveryTest.py'
    #,'python ' + curDir + 'testStorageEngineDll/src/backup_recovery/Recovery2Target.py'
    #,'python ' + curDir + 'testStorageEngineDll/src/backup_recovery/timeline.py'
    #,'python ' + curDir + 'testStorageEngineDll/src/tablespace/tablespace_backup_recovery.py'
    ,'python ' + curDir + 'testStorageEngineDll/src/heap/multi_insert.py'
    ]

    ThreadCases = [
     bootstrap + ThreadData
     ,ThreadTest + ThreadData
    ]

    #allCases = libCases + dllCases + ThreadCases
    allCases = libCases + dllCases
    run_test(allCases)
def Usage():
    print '''Usage:
    runtest.py debug/release 32/64
    or
    runtest.py 32/64 debug/release
    '''
if __name__ == '__main__':
    if len(sys.argv) >= 2 and sys.argv[1] == '-h':
        Usage()
        sys.exit(0)

    main(sys.argv)


