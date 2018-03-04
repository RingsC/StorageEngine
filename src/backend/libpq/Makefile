#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for libpq subsystem (backend half of libpq interface)
#
# IDENTIFICATION
#    src/backend/libpq/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/libpq
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

# be-fsstubs is here for historical reasons, probably belongs elsewhere

OBJS =auth.o pqformat.o  be-secure.o ip.o  pqcomm.o fe-connect.o fe-exec.o  fe-misc.o   fe-protocol2.o fe-protocol3.o fe-secure.o  pqexpbuffer.o

include $(top_srcdir)/src/backend/common.mk

all:
	cp ./*.o $(top_builddir)/bin
