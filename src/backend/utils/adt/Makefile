#
# Makefile for utils/adt
#
# src/backend/utils/adt/Makefile
#

subdir = src/backend/utils/adt
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

# seems to be required for some date/time stuff 1999/07/22 bjm
ifeq "$(findstring alpha,$(host_cpu))" "alpha"
ifeq "$(GCC)" "yes"
override CFLAGS+= -mieee
endif
endif

OBJS = bool.o datum.o name.o timestamp.o varlena.o pg_lzcompress.o

like.o: like.c like_match.c

include $(top_srcdir)/src/backend/common.mk

all:
	cp ./*.o $(top_builddir)/bin
