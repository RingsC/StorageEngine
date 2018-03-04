#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/common
#
# IDENTIFICATION
#    src/backend/access/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backup_recovery
top_builddir = ../..
include $(top_builddir)/linux/Makefile.global

OBJS = BRTest.o Recovery2Target.o timelines.o Recovery2Recently.o

#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
