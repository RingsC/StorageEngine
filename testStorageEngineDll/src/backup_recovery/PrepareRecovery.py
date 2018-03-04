#!/usr/bin/env python
import sys
import os
import shutil
import subprocess
import backupconf as reconf

class RecoveryDB:
    "class for recovery"
    def __init__(self,dataDir,dataDirBack,baseDir,archDir):
        self.dataDir = dataDir
        self.dataDirBack = dataDirBack
        self.baseDir = baseDir
        self.archiveDir = archDir
    
    def prepareFiles(self):
        #1. backup data directory
        if os.path.exists(self.dataDirBack):
            shutil.rmtree(self.dataDirBack)

	print 'copy file %s to %s' %(self.dataDir,self.dataDirBack)
        shutil.copytree(self.dataDir,self.dataDirBack)

        #2. remove the data directory
	print 'remove directory %s' % self.dataDir
        shutil.rmtree(self.dataDir)
        #os.mkdir(self.dataDir);

        #3. copy baseup to the data directory
	print 'copy directory %s to %s' % (self.baseDir,self.dataDir)
        shutil.copytree(self.baseDir,self.dataDir)

        #4. remove all files in the directory xlog
	print 'rm all files in directory fxdb_xlog'
        shutil.rmtree(self.dataDir + '/fxdb_xlog')
        #os.mkdir(self.dataDir + '/fxdb_xlog')

        #5. copy the xlog backuped in the 1 step to xlog directory
	print ''
        shutil.copytree(self.dataDirBack + '/fxdb_xlog',self.dataDir + '/fxdb_xlog')

    def createRecoveryConf(self):
        data = 'restore_command = ' + '\'cp ' + self.archiveDir + '/%f %p\''
        recoveryFile = open(self.dataDir + '/recovery.conf','w')
        recoveryFile.write(data)

def main():
    "used to prepare a recovery"
    #print "Hello"
    #print reconf.db_dataDir,reconf.db_backDir,reconf.db_archive
    print sys.argv
    if len(sys.argv) > 1:
        reconf.SetDataDir(sys.argv[1])
    MyRecovery = RecoveryDB(reconf.db_dataDir,reconf.db_backDir,reconf.db_baseDir,reconf.db_archive)
    MyRecovery.prepareFiles()
    MyRecovery.createRecoveryConf()
    
if __name__ == '__main__':
    main()

