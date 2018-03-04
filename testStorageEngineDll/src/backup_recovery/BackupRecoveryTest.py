#!/usr/bin/env python
import os;
import sys;
import subprocess
import shutil
import backupconf as conf

conf.Init()

run_list = [
    conf.db_execpath + ' -c preparebk -d ' + conf.db_baseDir + ' --run_test=t_init_func_/SHGLI_TEST/prepareBackup_' #prepare backup databases
    ,conf.WriteCopyFile                                                                                              #prepare copy.py
    ,conf.db_execpath + ' -c archive -d ' + conf.db_baseDir + ' --run_test=t_init_func_/SHGLI_TEST/test_backup_'     #create base backup
    ,'python ' + conf.script_dir() + '/PrepareRecovery.py'                      #prepare recovery
    ,conf.WriteCopyFile                                                                                              #prepare copy.py
    ,conf.db_execpath + ' -c recovery --run_test=t_init_func_/SHGLI_TEST/test_Recovery_'                             #recovery and check result
]
conf.CallAll(run_list)


