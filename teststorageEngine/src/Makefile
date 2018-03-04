#
# Makefile for Founder XMLDB src
#
# src/Makefile
#

subdir = src
top_builddir = ..
#include $(top_builddir)/linux/Makefile.global
include linux/Makefile.global

SUBDIRS	    = src/heap src/index src/meta src/transaction src/test_fram src/utils src/thread_commu src/heap_sort_store src/large_object\
src/tablespace src/deadlock src/fatal_exception src/TestFrame src/TestFrameCase src/cluster


$(recurse)

#OBJS = $(SUBDIROBJS)
#testStorageEngineDLL: $(OBJS)
#	$(CC) -D BOOST_TEST_CALL_DECL $(CPPFLAGS) $(call expand_subsys,$^) $(LIBS) -o $@

