#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/heap
#
# IDENTIFICATION
#    src/backend/access/heap/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/access/heap
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

OBJS = heapam.o hio.o pruneheap.o rewriteheap.o syncscan.o tuptoaster.o visibilitymap.o xdb_heap.o

include $(top_srcdir)/src/backend/common.mk

all:
	cp ./*.o $(top_builddir)/bin
