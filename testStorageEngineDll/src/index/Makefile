#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/common
#
# IDENTIFICATION
#    src/backend/access/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/index
top_builddir = ../..
include $(top_builddir)/linux/Makefile.global

OBJS = test_index_dll.o \
	test_index_insert_dll.o \
	test_index_transation_dll.o \
	test_index_update_dll.o \
	test_index_cmp_dll.o\
	test_toast_index_dll.o

#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
