#!/usr/bin/env python
import backupconf as conf
import os
import sys
import shutil
from functools import partial

#for base backup
_backup_conf = ''' 
doRecovery = 0
XLogArchiveMode = 1 
XLogArchiveCommand = ../archive
'''

#for recovery 1
_recovery_conf = '''
doRecovery = 1;
recoveryRestoreCommand = ../archive
recoveryTargetTLI=<tli>
recoveryTargetTime = '''

def CheckAndRmFile(toRm):
    if os.path.exists(toRm):
	print 'will rm ' + toRm
        os.remove(toRm)

def WritConf(timeFile = None):
    CheckAndRmFile(conf.homeDir + '/conf')

    configs = _backup_conf
    if timeFile:
	tFile = open(timeFile,'r')
	configs = _recovery_conf + tFile.readline()
	configs = configs.replace('<tli>',tFile.readline())
	tFile.close()

    confFile = open(conf.homeDir + '/conf' ,'w')
    confFile.write(configs)
    confFile.close()

def RmPharseFile():
    CheckAndRmFile(conf.homeDir + '/pharse1')
    CheckAndRmFile(conf.homeDir + '/conf')
    CheckAndRmFile(conf.homeDir + '/timeR0')
    CheckAndRmFile(conf.homeDir + '/timeR1')
    CheckAndRmFile(conf.homeDir + '/timeR2')
    CheckAndRmFile(conf.homeDir + '/timeR3')

def main():
    #clean old directorys
    conf.Init()

    #rm pharse file
    exec_str = conf.db_execpath + ' -c recovery2recently -d ' + conf.db_baseDir + ' --run_test=t_init_func_/SHGLI_TEST/<test_case>_' + ' -conf '
    exec_str += conf.homeDir + '/conf'
    run_list = [
	    RmPharseFile
	    ,partial(WritConf)
	    ,exec_str.replace('<test_case>','PrepareBasebackup') # Create Table        timeR1

	    #these command used to recovery to timeR1
            ,'python ' + conf.script_dir() + '/PrepareRecovery.py'         #prepare recovery
	    ,partial(WritConf,timeFile=conf.homeDir + '/timeR1')
	    ,exec_str.replace('<test_case>','RecoveryFirst') # Recovery 2 timeR1

	    #make more changes
	    ,partial(WritConf)
	    ,exec_str.replace('<test_case>','MakeMoreChanges1')  #timeR3

       	    #these command used to recovery to 
            ,'python ' + conf.script_dir() + '/PrepareRecovery.py'         #prepare recovery
	    ,partial(WritConf,timeFile=conf.homeDir + '/timeR3')
            ,exec_str.replace('<test_case>','Recovery2Recently') # recovery 2 recently
	    ,RmPharseFile
	    ]
    #conf.CallAll(run_list,len(run_list) - 2)
    conf.CallAll(run_list)
if __name__ == '__main__':
    main()
