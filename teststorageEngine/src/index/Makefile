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

OBJS = test_index_cmp.o \
	test_index.o \
	test_index_delete.o \
	test_toast_index.o

#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
