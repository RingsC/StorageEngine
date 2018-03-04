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

_parentDir = 'Recovery2Target'

def CheckAndRmFile(toRm):
    if os.path.exists(toRm):
	print 'will rm ' + toRm
        os.remove(toRm)

def WritConf(timeFile = None):
    CheckAndRmFile(conf.homeDir + '/' + _parentDir + '/conf')

    configs = _backup_conf
    if timeFile:
	tFile = open(timeFile,'r')
	configs = _recovery_conf + tFile.readline()
	configs = configs.replace('<tli>',tFile.readline())
	tFile.close()

    confFile = open(conf.homeDir + '/' + _parentDir + '/conf' ,'w')
    confFile.write(configs)
    confFile.close()

def RmPharseFile():
    CheckAndRmFile(conf.homeDir + '/' + _parentDir + '/pharse')
    CheckAndRmFile(conf.homeDir + '/' + _parentDir + '/conf')
    CheckAndRmFile(conf.homeDir + '/' + _parentDir + '/time1')
    CheckAndRmFile(conf.homeDir + '/' + _parentDir + '/time2')
    CheckAndRmFile(conf.homeDir + '/' + _parentDir + '/time3')

def main():
    #clean old directorys
    conf.SetDataDir(_parentDir)
    conf.Init()

    #rm pharse file
    exec_str = conf.db_execpath + ' -c recovery2target -d ' + conf.db_baseDir + ' --run_test=t_init_func_/SHGLI_TEST/TestRecovery2Target_' + ' -conf '
    exec_str += conf.homeDir + '/' + _parentDir + '/conf'
    run_list = [
	    RmPharseFile
	    ,exec_str # Create Table        time1
	    ,conf.WriteCopyFile
	    ,partial(WritConf)
	    ,exec_str # Do base backup      time2
	    ,exec_str # make more changes   time3

	    #these command used to recovery to time2
            ,'python ' + conf.script_dir() + '/PrepareRecovery.py Recovery2Target ' + conf.db_distri + ' ' + conf.db_bits         #prepare recovery
	    ,partial(WritConf,timeFile=conf.homeDir + '/' + _parentDir + '/time2')
	    ,conf.WriteCopyFile
	    ,exec_str # recovery to time2
            ,partial(WritConf)
	    ,exec_str # make some changes

       	    #these command used to recovery to time2
            ,'python ' + conf.script_dir() + '/PrepareRecovery.py Recovery2Target ' + conf.db_distri + ' ' + conf.db_bits         #prepare recovery
	    ,partial(WritConf,timeFile=conf.homeDir + '/' + _parentDir + '/time3')
	    ,conf.WriteCopyFile
            ,exec_str # recovery to time3
	    #,RmPharseFile
	    ]
    conf.CallAll(run_list)

if __name__ == '__main__':
    main()
