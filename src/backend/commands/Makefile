#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for backend/commands
#
# IDENTIFICATION
#    src/backend/commands/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/commands
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

OBJS = tablecmds.o tablespace.o \
	vacuum.o vacuumlazy.o \

include $(top_srcdir)/src/backend/common.mk

all:
	cp ./*.o  $(top_builddir)/bin
