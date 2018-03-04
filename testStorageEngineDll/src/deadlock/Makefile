#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/common
#
# IDENTIFICATION
#    src/backend/access/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/deadlock
top_builddir = ../..
include $(top_builddir)/linux/Makefile.global

OBJS = deadlocktest.o

#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
