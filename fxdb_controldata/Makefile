
override CPPFLAGS += -I../src/include/port -I../../include -I../../../include/StorageEngine -I../teststorageEngine/include -I../../StorageEngine/testStorageEngineDll/include -I../../StorageEngine/src/include -I../../../../3rd_party/include -I ../../StorageEngine/testStorageEngineDll/src/utils -D FOUNDER_XDB_SE -D BOOST_ALL_DYN_LINK  -O2 -Wall -Wpointer-arith -Wendif-labels -Wformat-security -fno-strict-aliasing -fwrapv -D_GNU_SOURCE

#SERVER_LIBS := -lutils -lgnuintl -llog4cxx -lNetLib -lExecution -ldbxml-2.5 -ldb-4.8 -lboost_thread -lboost_system -lXdbServerSDK -lDBProtocol $(LIBS) 
LIBS := -L../bin  -lpostgres -lpthread


all: ../bin/fxdb_controldata

../bin/fxdb_controldata: pg_controldata.o
	g++  $(CPPFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

pg_basebackup.o : pg_controldata.c	
	g++ $(CPPFLAGS) -c $^ -o $@


clean distclean:
	rm -f ../bin/fxdb_controldata pg_controldata.o

