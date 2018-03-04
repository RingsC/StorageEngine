#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/common
#
# IDENTIFICATION
#    src/backend/access/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/cluster
top_builddir = ../..
include $(top_builddir)/linux/Makefile.global

OBJS = cluster_simple.o \
	cluster_thread.o

#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
