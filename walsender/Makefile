#
# src/unittest
#

#subdir = src/unittest

#override CPPFLAGS := $(CPPFLAGS) -DLIBINTL_STATIC -DLOG4CXX_STATIC -DBOOST_ALL_DYN_LINK
override CPPFLAGS +=-I../../../include -I../serverconfig -I../../include -I../../../include/StorageEngine -I../teststorageEngine/include -I../../StorageEngine/testStorageEngineDll/include -I../../StorageEngine/src/include -I../../../../3rd_party/include -I ../../StorageEngine/testStorageEngineDll/src/utils -D FOUNDER_XDB_SE -D BOOST_ALL_DYN_LINK  -O2 -Wall -Wpointer-arith -Wendif-labels -Wformat-security -fno-strict-aliasing -fwrapv -D_GNU_SOURCE 

#SERVER_LIBS := -lutils -lgnuintl -llog4cxx -lNetLib -lExecution -ldbxml-2.5 -ldb-4.8 -lboost_thread -lboost_system -lXdbServerSDK -lDBProtocol $(LIBS) 
LIBS := -L../bin  -lstorageEngine -lpthread -lutils -lboost_regex -lboost_unit_test_framework -lstorageEngine -lboost_system -lboost_filesystem -lboost_thread \
    -lboost_date_time -llog4cxx -lcrypto -lgnuintl -lboost_locale -lreplication



all: ../bin/walsender

../bin/walsender: walsender.o ../serverconfig/replication_utils.o ../serverconfig/ConfigParse.o 
	g++ $(CPPFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

walsender.o : walsender.cpp	
	g++ $(CPPFLAGS) -c $^ -o $@

clean distclean:
	rm -f ../bin/walsender

