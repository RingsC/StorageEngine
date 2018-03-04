import os;
import sys;
import subprocess
import shutil
pathi = os.path.abspath(sys.path[0] + '/../backup_recovery' )
print pathi
sys.path.append(pathi)
import backupconf as conf
from functools import partial

conf.Init()

#for base backup
_backup_conf = ''' 
doRecovery = 0
XLogArchiveMode = 1 
XLogArchiveCommand = ../archive
'''
#for recovery
_recovery_conf = '''
XLogArchiveMode = 0
doRecovery = 1
recoveryRestoreCommand = ../archive
'''
_target_time_file = "/tblspc_recovery_time_file"


#write config file
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
	tFile.close()

    confFile = open(conf.homeDir + '/conf' ,'w')
    confFile.write(configs)
    confFile.close()
#copy base backup to recovery
_temp_datadir_name = "data_temp"
_data_log_dir = "/fxdb_xlog"
def CopyBasebackup():
    if os.path.exists(conf.db_baseDir + _data_log_dir):
        shutil.rmtree(conf.db_baseDir + _data_log_dir)
    if os.path.exists(conf.db_dataDir + _data_log_dir):
        shutil.copytree(conf.db_dataDir + _data_log_dir,conf.db_baseDir + _data_log_dir)
        shutil.rmtree(conf.db_dataDir)
        shutil.copytree(conf.db_baseDir,conf.db_dataDir)        

#execute all command
run_list = [
    WritConf
    ,conf.WriteCopyFile
    ,conf.db_execpath + ' -d ' + conf.db_baseDir + ' -hh needrun --run_test=t_init_func_/GUO_CT_TBLSPC/test_create_tblspc_prepare_ -conf ' + conf.homeDir + '/conf'
    ,conf.db_execpath + ' -d ' + conf.db_baseDir + ' -hh needrun --run_test=t_init_func_/GUO_CT_TBLSPC/make_base_backup_ -conf ' + conf.homeDir + '/conf'
    ,conf.db_execpath + ' -d ' + conf.db_baseDir + ' -hh needrun --run_test=t_init_func_/GUO_CT_TBLSPC/test_create_tblspc_ -conf ' + conf.homeDir + '/conf'
    ,conf.db_execpath + ' -d ' + conf.db_baseDir + ' -hh needrun --run_test=t_init_func_/GUO_CT_TBLSPC/test_drop_tblspc_ -conf ' + conf.homeDir + '/conf'
    ,CopyBasebackup
    ,partial(WritConf,timeFile = conf.homeDir + _target_time_file)
    ,conf.db_execpath + ' -hh needrun --run_test=t_init_func_/GUO_CT_TBLSPC/test_tblspc_recovery_ -conf ' + conf.homeDir + '/conf'
    ]

conf.CallAll(run_list)