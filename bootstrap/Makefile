#
# src/unittest
#

#subdir = src/unittest

#override CPPFLAGS := $(CPPFLAGS) -DLIBINTL_STATIC -DLOG4CXX_STATIC -DBOOST_ALL_DYN_LINK
override CPPFLAGS += -I../../include -I../testStorageEngineDll/include -I../teststorageEngine/include -D FOUNDER_XDB_SE  -O2 -Wall -Wpointer-arith -Wendif-labels -Wformat-security -fno-strict-aliasing -fwrapv  -D_GNU_SOURCE

#SERVER_LIBS := -lutils -lgnuintl -llog4cxx -lNetLib -lExecution -ldbxml-2.5 -ldb-4.8 -lboost_thread -lboost_system -lXdbServerSDK -lDBProtocol $(LIBS) 
LIBS := -L../bin  -lstorageEngine  -lutils -lexpat -lboost_regex -lboost_unit_test_framework -lstorageEngine -lboost_system -lboost_filesystem -lboost_thread \
    -lboost_date_time -llog4cxx -lcrypto -lgnuintl -lboost_locale


all: ../bin/bootstrap_storage

../bin/bootstrap_storage: main.o
	g++ main.o $(CPPFLAGS) $(LDFLAGS) $(LIBS) -o $@

main.o: main.cpp	
	g++ $(CPPFLAGS) -c main.cpp -o $@

clean distclean:
	rm -f ../bin/bootstrap_storage main.o

