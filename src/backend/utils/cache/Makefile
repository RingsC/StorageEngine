#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for utils/cache
#
# IDENTIFICATION
#    src/backend/utils/cache/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/utils/cache
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

OBJS = inval.o relcache.o

include $(top_srcdir)/src/backend/common.mk

all:
	cp ./*.o  $(top_builddir)/bin
