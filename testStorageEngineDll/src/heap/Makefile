#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/common
#
# IDENTIFICATION
#    src/backend/access/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/heap
top_builddir = ../..
include $(top_builddir)/linux/Makefile.global

OBJS = test_heap_close_dll.o \
	test_heap_create_dll.o \
	test_heap_delete_dll.o \
	test_heap_drop_dll.o \
	test_heap_open_dll.o \
	test_heap_update_dll.o \
	test_toast_dll.o \
	test_heap_select_dll.o \
	test_heap_insert_dll.o \
	test_dump_load.o

#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
