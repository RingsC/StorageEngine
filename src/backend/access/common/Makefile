#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/common
#
# IDENTIFICATION
#    src/backend/access/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/access/common
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

OBJS = heaptuple.o indextuple.o reloptions.o scankey.o \
 tupdesc.o xdb_common.o

include $(top_srcdir)/src/backend/common.mk
all:
	cp ./*.o $(top_builddir)/bin
