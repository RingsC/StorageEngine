#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for storage/file
#
# IDENTIFICATION
#    src/backend/storage/file/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/storage/file
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

OBJS = fd.o buffile.o copydir.o reinit.o fileinfo.o

include $(top_srcdir)/src/backend/common.mk

all:
	cp ./*.o $(top_srcdir)/bin
