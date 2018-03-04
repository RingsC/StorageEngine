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

override LIBS := -lboost_regex -lboost_unit_test_framework -lstorageEngine -lboost_system -lboost_filesystem -lboost_thread -lutils \
    -lboost_date_time -llog4cxx -lcrypto -lgnuintl -lboost_locale

OBJS = bin/UnitTest.o bin/test_fram.o bin/test_heap_close_dll.o bin/test_heap_create_dll.o bin/test_heap_delete_dll.o \
	bin/test_heap_drop_dll.o \
	bin/test_heap_insert_dll.o bin/test_heap_open_dll.o bin/test_heap_select_dll.o \
	bin/test_heap_update_dll.o bin/test_index_dll.o \
	bin/test_index_insert_dll.o bin/test_index_transation_dll.o bin/test_index_update_dll.o \
	bin/test_main_dll.o bin/test_toast_dll.o \
	bin/test_toast_index_dll.o bin/test_transaction_delete_dll.o bin/test_transaction_index_create_dll.o \
	bin/test_transaction_index_dll.o \
	bin/test_transaction_insert_dll.o bin/test_transaction_toast_dll.o bin/test_transaction_update_dll.o \
	bin/test_trans_multi_index_update.o \
	bin/test_trans_toast_index_dll.o bin/utils_dll.o \
        bin/test_sequence.o bin/utils.o bin/test_subtrans_dll.o \
	bin/test_memorycontext.o bin/test_index_cmp_dll.o bin/heap_store_dll.o\
	bin/test_dump_load.o bin/BRTest.o bin/ConfigParse.o bin/Recovery2Target.o bin/timelines.o \
        bin/test_tablespace.o	bin/large_object_test.o  bin/test_transaction_isolation_dll.o \
	bin/deadlocktest.o  bin/test_bitmap_dll.o  bin/test_lock_dll.o bin/Recovery2Recently.o \
	bin/TestFrame.o bin/TestTask.o bin/TestThreadPool.o


all: ../bin/testStorageEngineDLL
../bin/testStorageEngineDLL: $(OBJS)
	g++ $(CPPFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
clean:
	rm -f ../bin/testStorageEngineDLL
