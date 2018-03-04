#
# src/unittest
#

#subdir = src/unittest

#override CPPFLAGS := $(CPPFLAGS) -DLIBINTL_STATIC -DLOG4CXX_STATIC -DBOOST_ALL_DYN_LINK
override CPPFLAGS += -I../src/include/port -I../../include -I../../../include/StorageEngine -I../teststorageEngine/include -I../../StorageEngine/testStorageEngineDll/include -I../../StorageEngine/src/include -I../../../../3rd_party/include -I ../../StorageEngine/testStorageEngineDll/src/utils -D FOUNDER_XDB_SE -D BOOST_ALL_DYN_LINK  -O2 -Wall -Wpointer-arith -Wendif-labels -Wformat-security -fno-strict-aliasing -fwrapv -D_GNU_SOURCE

#SERVER_LIBS := -lutils -lgnuintl -llog4cxx -lNetLib -lExecution -ldbxml-2.5 -ldb-4.8 -lboost_thread -lboost_system -lXdbServerSDK -lDBProtocol $(LIBS) 
LIBS := -L../bin  -lpostgres -lpthread


all: ../bin/basebackup

../bin/basebackup: pg_basebackup.o
	g++  $(CPPFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

pg_basebackup.o : pg_basebackup.c	
	g++ $(CPPFLAGS) -c $^ -o $@


clean distclean:
	rm -f ../bin/basebackup pg_basebackup.o

