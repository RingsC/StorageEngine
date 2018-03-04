#
# Makefile for Founder XMLDB src
#
# src/Makefile
#

subdir = src
top_builddir = ..
#include $(top_builddir)/linux/Makefile.global
include linux/Makefile.global

SUBDIRS	    = src/test_main src/heap src/index src/transaction src/utils src/sequence src/memcontext src/heap_sort_store \
	      src/backup_recovery src/large_object src/tablespace src/deadlock src/lock src/bitmap src/TestFrame src/cluster

$(recurse)

#OBJS = $(SUBDIROBJS)
#testStorageEngineDLL: $(OBJS)
#	$(CC) -D BOOST_TEST_CALL_DECL $(CPPFLAGS) $(call expand_subsys,$^) $(LIBS) -o $@
