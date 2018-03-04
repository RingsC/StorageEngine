#
# Makefile for Founder XMLDB src
#
# src/Makefile
#

#g++ -DBOOST_ALL_DYN_LINK $(CPPFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
subdir = src
top_builddir = ..
#include $(top_builddir)/linux/Makefile.global
include linux/Makefile.global
#override LDFLAGS := -Wl,--as-needed -L../../../../3rd_party/lib/Linux/x86/boost -L../bin
LIBS += -lboost_regex -lboost_unit_test_framework -lpostgres -lboost_system -lboost_filesystem -lboost_thread -lutils \
    -lboost_date_time -llog4cxx -lcrypto -lgnuintl -lboost_locale

OBJS = bin/test_delete.o bin/test_heap_close.o bin/test_heap_create.o bin/test_heap_drop.o bin/test_heap_open.o bin/test_truncate.o \
	bin/test_index_insert.o bin/test_index_update.o bin/test_insert.o bin/test_select.o \
	bin/test_index_cmp.o bin/test_index.o bin/test_index_delete.o bin/test_toast_index.o \
	bin/meta_test.o bin/test_fram.o bin/test_main.o bin/UnitTest.o \
	bin/test_index_transaction_search.o bin/test_toast.o bin/test_transaction_delete.o \
	bin/test_transaction_insert.o bin/test_transaction_update.o bin/test_transactionWithIndex.o \
	bin/test_trans_toast_index.o bin/test_transWithToast.o \
	bin/toast_test_utils.o  bin/utils.o bin/thread_commu.o bin/TestThreadCommon.o bin/test_multthread_communicate.o \
	bin/thread_commu_hs.o bin/test_subtrans.o bin/compress.o bin/heap_store.o \
	bin/large_object_test.o\
	bin/create_tablespace.o bin/deadlocktest.o bin/thread_atexitTest.o bin/test_heap_hot_update.o\
	bin/fatal_exception.o bin/TestFrame.o bin/TestTask.o bin/TestThreadPool.o bin/TestFrameCase.o bin/TestFrameMain.o

#OBJS:=$(wildcard bin/*.o)

all: ../bin/testStorageEngineLIB
../bin/testStorageEngineLIB: $(OBJS)
	#echo $(OBJS)
	g++ $(CPPFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
	#g++ $(CPPFLAGS) $(LDFLAGS) $(OBJS) -lpostgres -lutils  -llog4cxx -lboost_system -lboost_filesystem -lboost_thread -lboost_date_time -o $@
clean:
	rm -f ../bin/testStorageEngineLIB
