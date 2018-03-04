#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/common
#
# IDENTIFICATION
#    src/backend/access/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/thread_commu
top_builddir = ../..
include $(top_builddir)/linux/Makefile.global

OBJS = thread_commu.o TestThreadCommon.o test_multthread_communicate.o thread_commu_hs.o  thread_atexitTest.o
#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
