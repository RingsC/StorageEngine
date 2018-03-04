#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/common
#
# IDENTIFICATION
#    src/backend/access/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/transaction
top_builddir = ../..
include $(top_builddir)/linux/Makefile.global

OBJS = \
	test_transaction_index_dll.o \
	test_transaction_toast_dll.o \
	test_trans_toast_index_dll.o \
	test_transaction_index_create_dll.o \
	test_transaction_insert_dll.o \
	test_transaction_update_dll.o \
	test_trans_multi_index_update.o \
	test_transaction_delete_dll.o \
	test_subtrans_dll.o \
	test_transaction_isolation_dll.o

#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
