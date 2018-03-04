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

OBJS = test_delete.o \
	test_heap_close.o \
	test_heap_create.o \
	test_heap_drop.o \
	test_heap_open.o \
	test_index_insert.o \
	test_index_update.o \
	test_insert.o \
	test_select.o \
	test_truncate.o \
        compress.o \
	test_heap_hot_update.o
#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
