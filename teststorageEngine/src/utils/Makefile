#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/common
#
# IDENTIFICATION
#    src/backend/access/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/utils
top_builddir = ../..
include $(top_builddir)/linux/Makefile.global

OBJS = toast_test_utils.o utils.o

#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
