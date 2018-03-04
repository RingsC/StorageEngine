#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for backend/nodes
#
# IDENTIFICATION
#    src/backend/nodes/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/nodes
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

OBJS = nodes.o list.o bitmapset.o tidbitmap.o \
       equalfuncs.o makefuncs.o  \
       outfuncs.o  value.o

include $(top_srcdir)/src/backend/common.mk

all:
	cp ./*.o $(top_srcdir)/bin
