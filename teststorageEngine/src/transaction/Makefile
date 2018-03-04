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

OBJS = test_index_transaction_search.o \
	test_toast.o \
	test_transaction_delete.o \
	test_transaction_insert.o \
	test_transaction_update.o \
	test_transactionWithIndex.o \
	test_trans_toast_index.o \
	test_transWithToast.o \
	test_subtrans.o

#include $(top_srcdir)/src/backend/common.mk
include ../common.mk
all:
	cp ./*.o $(top_builddir)/bin
