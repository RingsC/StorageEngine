#!/usr/bin/env python
import sys
import os
import shutil
import subprocess
import time

def script_dir():
    path = sys.path[0]
    if os.path.isdir(path):
        return path
    else :
        return os.path.dirname(path)

def CheckAndRmFile(toRm):
    if os.path.exists(toRm):
	print 'will rm ' + toRm

        os.remove(toRm)

def main():
    print "Hello World!"

    #set directory and exe
    curDir = script_dir().replace('\\','/') + '/'
    if 1 == len(curDir):
        curDir = ''

    _top_dir = curDir + '../../../'
    #set enviroment
    if sys.platform == 'win32':
       _bin_dir= _top_dir + 'bin/Win32/Debug/'
    else:
       _bin_dir= curDir + 'bin/'

    if sys.platform == 'win32':
       server_site = _bin_dir + 'walsender.exe '
       baseBackup_site = _bin_dir + 'baseBackup.exe '
       receiver_site = _bin_dir + 'walreceiver.exe '
    else:
       server_site = _bin_dir + 'walsender '
       baseBackup_site = _bin_dir + 'basebackup '
       receiver_site = _bin_dir + 'walreceiver '

    server_data_site = curDir + 'replication/data'
    receiver_data_site = curDir + 'replication/basebackup'
     
    baseBackup_conf = '-h localhost -p 5432 -D ' + receiver_data_site + ' -U a'
    baseBacukup_server_conf = server_data_site + ' -conf ' + curDir + 'serverconfig/' + 'basebackup_server_config.txt -basebackup'
    replication_server_conf = server_data_site + ' -conf ' + curDir + 'serverconfig/' + 'replication_server_config.txt -replication'
    receiver_server_conf = receiver_data_site + ' -conf ' + curDir + 'serverconfig/' + 'replication_receiver_config.txt -replication'
    check_data_conf = receiver_data_site + ' -conf ' + 'serverconfig/' + 'default_server_config.txt -check_data_001'

    #print server_site + baseBacukup_server_conf

    archive_dir = curDir + 'replication/archive'

    CheckAndRemoveThenCreate(curDir + 'replication');
    CheckAndRemoveThenCreate(curDir + 'replication/data');
    if sys.platform == 'win32':
       bootstrap=_bin_dir + 'bootstrap_storage.exe ' + curDir + 'replication/data'
    else:
       bootstrap=_bin_dir + 'bootstrap_storage ' + curDir + 'replication/data'
    print "bootstrap..."

    p0 = subprocess.Popen(bootstrap , shell=True).wait();
    print "Deleteing previous data site and creating new data site..."
    CheckAndRemoveThenCreate(receiver_data_site);
    CheckAndRemoveThenCreate(archive_dir);

    p1 = subprocess.Popen(server_site + baseBacukup_server_conf , shell=True);
    #print p1
    time.sleep(5);

    p2 = subprocess.Popen(baseBackup_site + baseBackup_conf , shell=True);

    while 1:
        ret1 = subprocess.Popen.poll(p2);
        if ret1 == 0:
            print 'terminate backup_server process... '
            p1.terminate();
            p1.wait();
            
            break

    print 'starting new server...' 
    p3 = subprocess.Popen(server_site + replication_server_conf , shell=True);
    print 'starting new receiver...'
    p4 = subprocess.Popen(receiver_site + receiver_server_conf , shell=True);

    while 1:
        ret2 = subprocess.Popen.poll(p3);
        if ret2 == 0:
            print 'terminate replication_server process... '
            #p4.terminate();
            #p4.wait();
            
            break
    #time.sleep(15);
    print 'starting news receiver to check the data...'
    p5 = subprocess.Popen(receiver_site + check_data_conf , shell=True);   

def CheckAndRemove(directory):
    'check if directory is exists or not, if it exists and then delete it'
    if os.path.exists(directory):
       shutil.rmtree(directory)

def CheckAndRemoveThenCreate(directory):
    if os.path.exists(directory):
       shutil.rmtree(directory)

    #create the directory
    os.mkdir(directory)

if __name__ == '__main__':
    main()

      
