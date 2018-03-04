#!/usr/bin/env python

import sys
import os
import collections 
import shutil
import subprocess

def script_dir():
    'return the path of this file'
    path = sys.path[0]
    if not os.path.isdir(path):
	path =os.path.dirname(path)
    if len(path) == 0:
	path += '.'

    return path
def onerror(fun,path,excinfo):
    pass

def CheckAndRemove(path):
    while os.path.exists(path):
	try:
	    shutil.rmtree(path,onerror = onerror)
	except:
	    time.sleep(1)

#def CheckAndRemove(directory):
#    'check if directory is exists or not, if it exists and then delete it'
#    if os.path.exists(directory):
#	shutil.rmtree(directory)

def CallAll(calls,count=-1):
    if not isinstance(calls,list):
	raise TypeError('you should pass a list to CallAll')

    count = len(calls) if count < 0 or count > len(calls) else count;
    for i in range(0,count):
	item = calls[i]
	if isinstance(item, collections.Callable):
	    item()
	elif isinstance(item,str):
	    print(item)
	    subprocess.Popen(item,shell=True).wait()
	elif isinstance(item,list):
	    subprocess.call(item)
	else:
	    raise TypeError('not supported')

def Init():
    #clean old directorys
    CheckAndRemove(db_dataDir)
    CheckAndRemove(db_backDir)
    CheckAndRemove(db_baseDir)
    CheckAndRemove(db_archive)

    if not os.path.exists(db_dataDir):
	os.mkdir(db_dataDir)

    if not os.path.exists(db_baseDir):
	os.mkdir(db_baseDir)

    if not os.path.exists(db_archive):
	os.mkdir(db_archive)

    #create data dir
    subprocess.Popen(db_dataCreator + db_dataDir,shell=True).wait(); 

def AdjustParams(argv):
    params = argv
    params.sort();
    if 0 == len(params):
	params.extend(['32','Debug'])
    elif 1 == len(params):
	if params[0] == '32' or params[0] == '64':
	    params[1] = 'Debug'
	else:
	    params.insert(0,'32')

    
    if params[0] == '32':
	bits = 'x86'
    else:
	bits = 'x64'

    params[1] = params[1].capitalize()
    if sys.platform == 'win32':
	if '32' == params[0]:
	    distri = 'Win32/'
	else:
	    distri = 'x64/'

    else:
	if '32' == params[0]:
	    distri = 'linux/x86/'
	else:
	    distri = 'linux/x64/'
    
    distri += params[1] + '/'

    return (distri,bits)

def WriteCopyFile(parentDir = None):
    _data = '''#!/usr/bin/env python
import shutil
import os
import sys
def main(argv):
    try:
        shutil.copyfile(argv[0],argv[1])
        print 'copy file ' + argv[0] + ' to ' + argv[1]
        #print 'Has copied one fle'
    except IOError,ex:
        print ex

if __name__ == '__main__':
    main(sys.argv[1:])
'''
#    if parentDir:
#        db_dataDir += '/' + parentDir
    copy = open(db_dataDir + '/copy.py','w')
    copy.write(_data)
    copy.close()

#private variable scoped in this script
_top_dir = script_dir() 
if len(_top_dir) > 0:
    _top_dir += '/../../../../../..'
else:
    _top_dir =  '../../../../../..'

_top_dir.replace('\\','/')
_top_dir = os.path.abspath(_top_dir) + '/'
_bin_dir = _top_dir + 'dist/server/bin/'

#set enviroment
_lib_dir =_top_dir + 'dist/server/bin/'
_execpath =  _bin_dir + '/'

#variables exported
homeDir = _top_dir + 'code/src/StorageEngine'
db_dataDir = homeDir + '/data'
db_backDir = homeDir + '/back'
db_baseDir = homeDir + '/base'
db_archive = homeDir + '/archive'
db_execpath = _execpath + '/testStorageEngineDLL ' + db_dataDir
db_dataCreator = _execpath + '/bootstrap_storage '
db_isShell = True

def SetDataDir(parentDir):
    global db_dataDir
    global db_backDir
    global db_baseDir
    global db_archive
    global db_execpath
    
    db_parentDir = homeDir + '/' + parentDir
    if not os.path.exists(db_parentDir):
        print db_parentDir
	os.mkdir(db_parentDir)
    db_dataDir = db_parentDir + '/data'
    db_backDir = db_parentDir + '/back'
    db_baseDir = db_parentDir + '/base'
    db_archive = db_parentDir + '/archive'
    db_execpath = _execpath + '/testStorageEngineDLL ' + db_dataDir
    db_dataCreator = _execpath + '/bootstrap_storage '
    


def test():
    SetDataDir("Recovery2Target")
    print homeDir
    print db_execpath
    print db_dataDir 
    print db_backDir 
    print db_baseDir 
    print db_archive 

if __name__  == '__main__':
    test()

