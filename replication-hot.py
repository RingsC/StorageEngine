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
    	os.remove(toRm)

def SigServerStop(fileDir) :
    f = open(fileDir, 'w');
    f.close()

def main():
    #print "Hello World!"

    #set directory and exe
    curDir = script_dir().replace('\\','/') + '/'
    if 1 == len(curDir):
        curDir = ''

    _top_dir = curDir + '../../../'

    if sys.platform == 'win32' :
      _bin_dir= _top_dir + 'build/code/src/StorageEngine/'
    else :
      _bin_dir= _top_dir + 'dist/server/bin/'
    
   # _bin_dir = os.path.abspath(_bin_dir) + '/'

    if sys.platform == 'win32':
       server_site = _bin_dir + 'walsender/Debug/walsender.exe '
       baseBackup_site = _bin_dir + 'baseBackup/Debug/baseBackup.exe '
       receiver_site = _bin_dir + 'walreceiver/Debug/walreceiver.exe '
    else:
       server_site = _bin_dir + 'walsender '
       baseBackup_site = _bin_dir + 'basebackup '
       receiver_site = _bin_dir + 'walreceiver '

    server_data_site = curDir + 'replication/data'
    receiver_data_site = curDir + 'replication/basebackup'

    unknow_server_mark = 'none'
     
    baseBackup_conf = '-h localhost -p 65432 -D ' + receiver_data_site + ' -U a'
    baseBacukup_server_conf = server_data_site + ' -conf ' + curDir + 'serverconfig/' + 'basebackup_server_hotstandby_config.txt -basebackup '
    replication_server_conf = server_data_site + ' -conf ' + curDir + 'serverconfig/' + 'replication_server_hotstandby_config.txt -replication '
    receiver_server_conf = receiver_data_site + ' -conf ' + curDir + 'serverconfig/' + 'replication_receiver_hotstandby_config.txt -replication '
    user_server_conf_file = curDir + 'serverconfig/user_server_conf.conf'
    user_standy_conf_file = curDir + 'serverconfig/user_standy_conf.conf'
    
    SERVER_CONF_FILE = curDir + 'serverconfig/replication_server_hotstandby_config_check.conf'
    STANDY_CONF_FILE = curDir + 'serverconfig/replication_receiver_hotstandby_config_check.conf'
    replication_server_conf_check = server_data_site + ' -conf ' + SERVER_CONF_FILE + ' -replication '
    check_data_conf = receiver_data_site + ' -conf ' + curDir +  'serverconfig/' + 'replication_receiver_hotstandby_config_check.txt -check_data_001 '   
    DATA_TIMES_conf = curDir + 'serverconfig/' + 'DATA_TIMES.txt'
    CHECK_WALSENDER = curDir + 'serverconfig/' + 'CHECK_WALSENDER'
    CHECK_BASEBACKUP_OK_FILE = curDir + 'serverconfig/' + 'bk_ok'
    ARCHIVE_COMMAND = ''
    RESTORE_COMMAND = ''
    SERVER_UP_MARK_FILE = _bin_dir + 'info/server_up'

    CREATE_REL_NUMS = '5'
    RECEIVER_NUMS = 3
    OVER_TIME = 10 #min
    USE_EXISTS_DATADIR = 0

    MODE = '1'

    SERVER_MARK = unknow_server_mark
    CAN_SERVER_STOP = 0

    USER_SERVER_CONF_PARAM_NUM = 5
    USER_STANDY_CONF_PARAM_NUM = 6

    #1 :: change standy to server
    #0 :: shutdown server and start new one
    HOT_CHANGE_SERVER = 1
    
    RECEIVER_DATA_SITE = [[0]] * RECEIVER_NUMS
    BASEBACKUP_CONF = [[0]] * RECEIVER_NUMS
    RECEIVER_SERVER_CONF = [[0]] * RECEIVER_NUMS
    RECEIVER_SERVER_CONF_FILE = [[0]] * RECEIVER_NUMS
    
    # init all receiver data dir
    for i in range(0, RECEIVER_NUMS) :
    	RECEIVER_DATA_SITE[i] = receiver_data_site + str(i)
    	
    # init base backup conf
    for i in range(0, RECEIVER_NUMS) :
    	BASEBACKUP_CONF[i] = '-h localhost -p 65432 -D ' + RECEIVER_DATA_SITE[i]
    	
    # init receiver conf
    for i in range(0, RECEIVER_NUMS) :
    	RECEIVER_SERVER_CONF[i] = RECEIVER_DATA_SITE[i] + ' -conf ' + curDir +  'serverconfig/' + \
    	'replication_receiver_hotstandby_config_check.txt -check_data_001 '

    archive_dir = curDir + 'replication/archive'

    ARCHIVE_COMMAND = archive_dir
    RESTORE_COMMAND = archive_dir

    print "Deleteing previous data site and creating new data site..."
    CheckAndRemoveThenCreate(curDir + 'replication');
    CheckAndRemoveThenCreate(receiver_data_site);
    CheckAndRemoveThenCreate(curDir + 'replication/data');
    CheckAndRemoveThenCreate(archive_dir)

    if sys.platform == 'win32':
       bootstrap=_bin_dir + 'bootstrap/Debug/bootstrap_storage.exe '
    else:
       bootstrap=_bin_dir + 'bootstrap_storage '
       
    bootstrap += curDir + 'replication/data'

    print "bootstrap...\n"
    
    p0 = subprocess.Popen(bootstrap ).wait();
        
    p1 = subprocess.Popen(server_site + baseBacukup_server_conf + CHECK_WALSENDER);
    while 1:
     if os.path.exists(CHECK_WALSENDER):
      fp = open(CHECK_WALSENDER, "r")
      check_wal = fp.read()
      if check_wal == "1":
       print 'walsender is ready for basebackup...'
       fp.close()                     
       os.remove(CHECK_WALSENDER)            
       break
                       
    #print baseBackup_site + baseBackup_conf
    p2 = subprocess.Popen(baseBackup_site + BASEBACKUP_CONF[0] );
    while 1:
     ret1 = subprocess.Popen.poll(p2);
     if ret1 == 0:
      print 'terminate backup_server process... '
      fp = open(CHECK_BASEBACKUP_OK_FILE, 'w')
      fp.close()
      p1.wait()
      break
       			
       			
    #copy backup data dir
    data_dir = curDir + '/replication'
    src_data_dir = data_dir + '/' + 'basebackup0'
    for i in range(1, RECEIVER_NUMS) :
     det_data_dir = data_dir + '/' + 'basebackup' + str(i)
     shutil.copytree(src_data_dir, det_data_dir)
        

    os.remove('replcation_error.txt')
    os.remove('replcation_out.txt')
    errFile = open('replcation_error.txt','a+')
    outFile = open('replcation_out.txt','a+')

    #for hot_change_server in range(0, 2) :

    #hot standby and warm standby
    for standy_type in range(0, 2) :

      if HOT_CHANGE_SERVER == 0 :

        if os.path.exists(STANDY_CONF_FILE) :
          os.remove(STANDY_CONF_FILE)

        if standy_type == 0 : #warm standby
          print '\n\nin warm standby..\n\n'
          InitStandyConfFileUseWarm(STANDY_CONF_FILE, RESTORE_COMMAND, 0)
        if standy_type == 1 : #hot standby
          print '\n\nin hot standby..\n\n'
          InitStandyConfFile(STANDY_CONF_FILE, RESTORE_COMMAND, 0)

      #sync server and async server
      for server_type in range(0, 2) :

        if HOT_CHANGE_SERVER == 0 :

          if os.path.exists(SERVER_CONF_FILE):
            os.remove(SERVER_CONF_FILE)

          if server_type == 0 : #async server
            print '\n\n\nasync server running\n\n\n'
            InitServerConfFile(SERVER_CONF_FILE, ARCHIVE_COMMAND)
          if server_type == 1 : #sync server
            print '\n\n\nsync server running\n\n\n'
            InitServerConfFileUseSyncCommit(SERVER_CONF_FILE, ARCHIVE_COMMAND)
        else :
          if os.path.exists(SERVER_CONF_FILE) :
            os.remove(SERVER_CONF_FILE)
          if os.path.exists(STANDY_CONF_FILE) :
            os.remove(STANDY_CONF_FILE)
          InitStandyConfFile_HotChangeServer(SERVER_CONF_FILE, RESTORE_COMMAND, ARCHIVE_COMMAND, server_type == 1, standy_type == 0, 1)
          InitStandyConfFile_HotChangeServer(STANDY_CONF_FILE, RESTORE_COMMAND, ARCHIVE_COMMAND, server_type == 1, standy_type == 0, 0)

        DATA_SITE = [[0]] * (RECEIVER_NUMS + 1)

        DATA_SITE[0] = server_data_site

        #init data dir
        for i in range(1, RECEIVER_NUMS + 1) :
          DATA_SITE[i] = RECEIVER_DATA_SITE[i - 1]

        #start server and standy
        for num in range(0, RECEIVER_NUMS + 1) :

          SERVER_MARK = unknow_server_mark
          CAN_SERVER_STOP = 0

          if (num == RECEIVER_NUMS) and HOT_CHANGE_SERVER :
            time.sleep(5)
            break

          if (num == RECEIVER_NUMS - 1) and HOT_CHANGE_SERVER :
            CAN_SERVER_STOP = 1

          if num > 0 :
            tmp_data_site = DATA_SITE[RECEIVER_NUMS]
            DATA_SITE[RECEIVER_NUMS] = DATA_SITE[num - 1]
            DATA_SITE[num - 1] = tmp_data_site

          if HOT_CHANGE_SERVER:
            SERVER_MARK = DATA_SITE[num + 1]

          #init server config
          replication_server_conf_check = DATA_SITE[num] + ' -conf ' + SERVER_CONF_FILE + ' -replication '

          #init data site
          tmp_data_site = DATA_SITE[RECEIVER_NUMS]
          DATA_SITE[RECEIVER_NUMS] = DATA_SITE[num]
          DATA_SITE[num] = tmp_data_site

          if HOT_CHANGE_SERVER == 1 :
            MODE = 2

          #init user server conf file
          paramValue = [[0]] * USER_SERVER_CONF_PARAM_NUM
          paramValue[0] = CREATE_REL_NUMS
          paramValue[1] = RECEIVER_NUMS
          paramValue[2] = USE_EXISTS_DATADIR
          paramValue[3] = OVER_TIME
          paramValue[4] = MODE
          InitUserConfFile(user_server_conf_file, paramValue, USER_SERVER_CONF_PARAM_NUM)

          paramValue = [[0]] * USER_STANDY_CONF_PARAM_NUM
          paramValue[0] = SERVER_MARK
          paramValue[1] = str(CAN_SERVER_STOP)
          paramValue[2] = CREATE_REL_NUMS
          paramValue[3] = RECEIVER_NUMS
          paramValue[4] = OVER_TIME
          paramValue[5] = MODE
          InitUserConfFile(user_standy_conf_file, paramValue, USER_STANDY_CONF_PARAM_NUM)

          if USE_EXISTS_DATADIR == 0 :
            USE_EXISTS_DATADIR = 1

          #start server
          if not (HOT_CHANGE_SERVER and num > 0) :
            time.sleep(3)
            print 'starting new server...\n'
            #print server_site + replication_server_conf_check + DATA_TIMES_conf
            p3 = subprocess.Popen(server_site + replication_server_conf_check + DATA_TIMES_conf +\
             ' ' + user_server_conf_file, stderr = errFile, stdout = outFile)

          # init receiver conf
          for i in range(0, RECEIVER_NUMS) :
            RECEIVER_SERVER_CONF[i] = DATA_SITE[i] + ' -conf ' + STANDY_CONF_FILE + ' -check_data_001 '

          print 'starting new receiver to check the data...'

          p_handle = [[0]] * (RECEIVER_NUMS)

          if not WaitForServerUp(SERVER_UP_MARK_FILE, RECEIVER_NUMS) :
            print "\n\nserver can not start up\n\n"
            return 1

          #start standy
          for i in range(0, RECEIVER_NUMS) :
            time.sleep(2)
            p_handle[i] = subprocess.Popen(receiver_site + RECEIVER_SERVER_CONF[i] + DATA_TIMES_conf + ' ' + user_standy_conf_file, stderr = errFile)

          for i in range(0, RECEIVER_NUMS) :
            if not (DATA_SITE[i] == SERVER_MARK) :
              p_handle[i].wait()
            else :
              p_server = p_handle[i]

          if CAN_SERVER_STOP == 1 :
            p_server.wait()

          p3.wait()
    
    '''else:
        print "linux bootstrap..."

        p0 = subprocess.Popen(bootstrap , shell=True).wait();
        print bootstrap
        p1 = subprocess.Popen(server_site + baseBacukup_server_conf + CHECK_WALSENDER, shell=True );
        print server_site + baseBacukup_server_conf + CHECK_WALSENDER
        print CHECK_WALSENDER
        while 1:
             if os.path.exists(CHECK_WALSENDER):
                   fp = open(CHECK_WALSENDER, "r")
                   check_wal = fp.read()
                   if check_wal == "1":
                       print 'walsender is ready for basebackup...'
                       fp.close()                     
                       os.remove(CHECK_WALSENDER)            
                       
                       break

        p2 = subprocess.Popen(baseBackup_site + baseBackup_conf, shell=True );
        print baseBackup_site + baseBackup_conf
        while 1:
            ret1 = subprocess.Popen.poll(p2);
            if ret1 == 0:
               print 'terminate backup_server process... '
	       killsg('walsender');
               break
            
        #lsof -i:65432
        errFile = open('replcation_error.txt','w')
        outFile = open('replcation_out.txt','w')
        print 'starting new server...' 
        p3 = subprocess.Popen(server_site + replication_server_conf_check + DATA_TIMES_conf + CREATE_REL_NUMS , shell = True, stdout=outFile , stderr=errFile);
        print server_site + replication_server_conf_check + DATA_TIMES_conf
        print 'starting new receiver to check the data...'
        time.sleep(5);
        print receiver_site + check_data_conf + DATA_TIMES_conf
        p4 = subprocess.Popen(receiver_site + check_data_conf + DATA_TIMES_conf , shell = True, stdout=outFile , stderr=errFile); 

        while 1:
            ret2 = subprocess.Popen.poll(p3);
            if ret2 == 0:
                fc = open('replcation_out.txt')
                for line in fc.readlines():
                     print line

                break
    '''
      
def killsg(process):
    'kill process function for linux'
    cmd = 'ps -A | grep ' +  process + '| awk \'{print $1}\' | xargs kill'
    print cmd
    os.system('ps -A | grep ' +  process + '| awk \'{print $1}\' | xargs kill')
    os.system('ps -A | grep ' + process + '| wc -l')

    #pidF = open('hspid','r')
    #strPid = pidF.readline().strip()
    #if 0 == len(strPid):
        #ll = strPid.split(' ');
        #kis = 'kill -9 ' + ll[0]
        #print kis
        #os.system(kis);
    #else:
        #print 'there is no process named ' + process

def CheckAndRemove(directory):
    'check if directory is exists or not, if it exists and then delete it'
    if os.path.exists(directory):
       shutil.rmtree(directory)

def CheckAndRemoveThenCreate(directory):
    if os.path.exists(directory):
      shutil.rmtree(directory)

    #create the directory
    os.mkdir(directory)

def InitServerConfFile(fileDir, archive_command) :
    fp = open(fileDir, 'a')
    fp.write('masterEnableStandby = 2\n')
    fp.write('PostPortNumber = 62345\n')
    fp.write('max_wal_senders = 5\n')
    #fp.write('XLogArchiveMode = 1\n')
    #fp.write('XLogArchiveCommand = ' + archive_command + '\n')
    fp.close()
  
def InitServerConfFileUseSyncCommit(fileDir, archive_command) :
    fp = open(fileDir, 'a')
    fp.write('SyncRepStandbyNames = *\n')
    fp.write('masterEnableStandby = 2\n')
    fp.write('PostPortNumber = 62345\n')
    fp.write('max_wal_senders = 5\n')
    #fp.write('XLogArchiveMode = 1\n')
    #fp.write('XLogArchiveCommand = ' + archive_command + '\n')
    fp.close()

def InitStandyConfFile(fileDir, restore_command, isServer) :
    fp = open(fileDir, 'a')
    fp.write('slaveEnableHotStandby = 1\n')
    if not isServer :
      fp.write('PrimaryConnInfo= host=localhost port=62345\n')
      fp.write('StandbyMode = 1\n')
    #fp.write('recoveryRestoreCommand = ' + restore_command + '\n')
    #fp.write('latestTimeLine = 1\n')
    fp.close()

def InitStandyConfFileUseWarm(fileDir, restore_command, isServer) :
    fp = open(fileDir, 'a')
    fp.write('slaveEnableHotStandby = 0\n')
    if not isServer :
      fp.write('PrimaryConnInfo= host=localhost port=62345\n')
      fp.write('StandbyMode = 1\n')
    #fp.write('recoveryRestoreCommand = ' + restore_command + '\n')
    #fp.write('latestTimeLine = 1\n')
    fp.close()

def InitStandyConfFile_HotChangeServer(fileDir, restore_command, archive_command, isSyncCommit, isWarm, isServer) :
    if isWarm :
      print '\n\nin warm standby..\n\n'
    else :
      print '\n\nin hot standby..\n\n'

    if isSyncCommit :
      print '\n\nsync server...\n\n'
    else :
      print '\n\nasync server...\n\n'

    if isSyncCommit :
      InitServerConfFileUseSyncCommit(fileDir, archive_command)
    else :
      InitServerConfFile(fileDir, archive_command)

    if isWarm :
      InitStandyConfFileUseWarm(fileDir, restore_command, isServer)
    else :
      InitStandyConfFile(fileDir, restore_command, isServer)


def InitUserConfFile(fileDir, paramValue, arrLen) :
    fp = open(fileDir, 'w')
    for i in range(0, arrLen) :
      fp.write("%s\n"%(paramValue[i]))
    fp.close()

def WaitForServerUp(file_dir, receiver_num) :
    for i in range(0, 60 + 10 * receiver_num) :
      time.sleep(1)
      if os.path.exists(file_dir) :
        return 1
    return 0
if __name__ == '__main__':
    main()

#try:
#except OsError,ex:
#print ex.msg()
