#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for access/common
#
# IDENTIFICATION
#    src/backend/access/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/test_main
top_builddir = ../..
include $(top_builddir)/linux/Makefile.global

#OBJS = \
#	UnitTest.o \
#	test_fram.o \
#	test_main_dll.o
#`OBJS = /home/storage/wangfh/test/src/postgresql-9.1beta2/teststorageEngine/src/test_fram/UnitTest.o \
#`	/home/storage/wangfh/test/src/postgresql-9.1beta2/teststorageEngine/src/test_fram/test_fram.o \
#`	test_main_dll.o

#OBJS = ~/test/src/postgresql-9.1beta2/teststorageEngine/src/test_fram/UnitTest.o \
#	~/test/src/postgresql-9.1beta2/teststorageEngine/src/test_fram/test_fram.o \
#	test_main_dll.o
#include $(top_srcdir)/src/backend/common.mk
OBJS = ../../../teststorageEngine/src/test_fram/UnitTest.o \
	../../../teststorageEngine/src/test_fram/test_fram.o \
	test_main_dll.o
include ../common.mk
all:
	cp ../../../teststorageEngine/src/test_fram/UnitTest.o $(top_builddir)/bin
	cp ../../../teststorageEngine/src/test_fram/test_fram.o $(top_builddir)/bin
	cp ./*.o $(top_builddir)/bin
