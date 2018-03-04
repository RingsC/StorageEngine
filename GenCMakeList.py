#!/usr/bin/env python
import sys
import os
import shutil
import subprocess
import collections
import re
import time


class CMakeList(object):
    def __init__(self,root,target):
	self.root = os.path.abspath(root)
	self.root = self.root.replace('\\','/')
	self.target = target
        self._cmakelist = '#Top-level files\n'

    def addset(self,name,value):
	if isinstance(value,str):
	    self._cmakelist += 'set(' + name + ' ' + value + ')\n\n'
	elif isinstance(value,list):
	    self._cmakelist += 'set(' + name + '\n';
	    for v in value:
		self._cmakelist += v + '\n'
	    self._cmakelist += ')\n\n'
    def addcomment(self,comment):
	self._cmakelist += '#' + comment + '\n'

    def addgroup(self,parent,fileVar):
        parent = parent.replace('/',r'\\')
	self._cmakelist += 'source_group("' + parent +'"   FILES ${' + fileVar + '})\n\n'

    def addexe(self,exeName,srcs):
	self._cmakelist += 'add_executable(' + exeName + ' ${' + srcs + '})\n\n'

    def addincludedir(self,incs):
	self._cmakelist += 'include_directories(\n'

	for inc in incs:
	    self._cmakelist += inc + '\n'
	self._cmakelist += ')\n\n'

    def adddep(self,target,deps):
	self._cmakelist += 'add_dependencies(' + target + '\n'
	for dep in deps:
	    self._cmakelist += dep + '\n'
	self._cmakelist += ')\n\n'

    def addlink(self,target,links):
	self._cmakelist += 'target_link_libraries(' + target + '\n'
	for link in links:
	    self._cmakelist += link + '\n'
	self._cmakelist += ')\n\n'

    def addmacro(self,macros):
        if isinstance(macros,str):
            self._cmakelist += 'add_definitions(-D'+macros+')\n'
        elif isinstance(macros,list):
            self._cmakelist += 'add_definitions(\n'
            for macro in macros:
                self._cmakelist += '-D' + macro + '\n'

            self._cmakelist += ')\n'
                
        self._cmakelist += ''
    def addinstall(self,target):
	self._cmakelist += 'install(TARGETS ' + target + '\n'
	self._cmakelist += '''RUNTIME DESTINATION bin
	    LIBRARY DESTINATION bin
	    ARCHIVE DESTINATION lib)
	    '''
    def rel2root(self,path):
	path = path.replace(self.root,'')
	if len(path) > 0 and path[0] == '/':
	    path = path[1:]
	return path
	
    def visitdir(self,directory):
	directory = os.path.abspath(directory);
	directory = directory.replace('\\','/')
	fs = os.listdir(directory)
	name = os.path.basename(self.root) + '_' + self.rel2root(directory);
	name = name.replace('/','_')
	self.addcomment('test')
	filelists = []
	for ff in fs:
            if os.path.isfile(directory + '/' + ff):
	        filelists.extend([self.rel2root(directory + '/' + ff)])

	self.addset(name,filelists)
	self.addgroup(self.rel2root(directory),name)

	retfiles = []
	dirs = []
	filelists = fs
	for ff in filelists:
	    f = directory + '/' + ff
	    if not os.path.isfile(f):
	        print f
		dirs.extend([self.rel2root(f)])
		ds,fs = self.visitdir(f)
		dirs.extend(ds)
		retfiles.extend(fs)
	    else:
		relf = self.rel2root(f);
		#print 'relf ', relf
		#print 'before extend: ' , retfiles
		retfiles.extend([relf])
		#print 'after extend: ', retfiles

	return (dirs,retfiles)
    def Write2(self,rootdir):
	cmakefile = open(rootdir + '/CMakeLists.txt','w')
	cmakefile.write(self._cmakelist)
	cmakefile.close()    

class CustomerConf(object):
    def __init__(self,fileName):
        self.fileName = fileName
        self.curMetho = None
        self.incdirs = []
        self.linklibs = []
        self.deps = []
        self.macros = []
        
    def parse_incdirs(self,line):
        self.incdirs.extend([line])

    def parse_linklibs(self,line):
        self.linklibs.extend([line])

    def parse_deps(self,line):
        self.deps.extend([line])

    def parse_macros(self,line):
        self.macros.extend([line])

    def get_incdirs(self): return self.incdirs
    def get_linklibs(self): return self.linklibs
    def get_deps(self): return self.deps
    def get_macros(self): return self.macros
    
    def parse(self):
        try:
            self.file = open(self.fileName)
            commentReg = re.compile(r'\s*#.*')
            segReg = re.compile(r'\s*(\w+)\s*:')
            for line in self.file:
                if commentReg.match(line) is not None:
                    pass
                else:
                    match = segReg.match(line)
                    if match is not None:
                        try:
                            methodName = 'parse_' + match.groups()[0]
                            method = getattr(self,methodName)
                            self.curMethod = method
                        except(TypeError ,ex):
                            print ex
                    else:
                        #general
                        if self.curMethod is None:
                            print 'Syntax error:'
                        else:
                            self.curMethod(line)
                        
                
        except(IOError ,ex):
            print ex
        self.curMethod = None
            
        
        
def main(rootdir,target,fileName = None):
    #check layout
    incdirs = []
    incfiles = []
    linklibs = []
    deps = []
    macros = []

    cmake = CMakeList(rootdir,target)
    if not os.path.exists(rootdir + '/include'):
        print 'there is no include directory'
        exit(-1)
    else:
        incdirs,incfiles = cmake.visitdir(rootdir + '/include')
        incdirs.extend(['include'])

    srcdirs = []
    srcfiles = []
    if not os.path.exists(rootdir + '/src'):
        print 'There is no src directory'
        exit(-1)
    else:
        srcdirs,srcfiles = cmake.visitdir(rootdir + '/src')

    if fileName is not None:
        confParse = CustomerConf(fileName)
        confParse.parse()
        incdirs.extend(confParse.get_incdirs())
        linklibs = confParse.get_linklibs()
        macros = confParse.get_macros()
        deps = confParse.get_deps()

        
    srcfiles.extend(incfiles)
    cmake.addset(target + '_SRCLISTS',srcfiles)
    cmake.addexe(target,target + '_SRCLISTS')
    cmake.addlink(target,linklibs)
    cmake.adddep(target,deps)
    cmake.addmacro(macros)
    cmake.addincludedir(incdirs)
    #addlink()
    cmake.addinstall(target)
    cmake.Write2(rootdir)
    #print incfiles
    #print srcfiles



if __name__ == '__main__':
    arglen = len(sys.argv)
    if 3 == arglen:
        main(sys.argv[1],sys.argv[2])
    elif 4 == arglen:
        main(sys.argv[1],sys.argv[2],sys.argv[3])
    else:
        print 'GenCMakeList.py rootdir target [conf]'
        
