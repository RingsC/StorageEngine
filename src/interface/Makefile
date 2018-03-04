#------------------------------------------------------------------------- # # Makefile-- #    Makefile for backend/nodes
#
# IDENTIFICATION
#    src/backend/nodes/Makefile
#
#-------------------------------------------------------------------------

subdir = src/interface
top_builddir = ../..
src_dir = ../../../../interface
interface_dir = ../../src/include
include $(top_builddir)/src/Makefile.global

override CPPFLAGS += -shared $(CFLAGS) $(CPPFLAGS)
#override CFLAGS := -I$(src_dir) -I$(interface_dir) -shared $(CFLAGS)
#CPPFLAGS += -I$(src_dir)
#CPPFLAGS += -I$(interface_dir) 

OBJS = utils.o PGEntrySet.o PGIndexEntrySet.o PGStorageEngine.o pgtransaction.o \
       StorageEngine.o StorageEngineExceptionUniversal.o PgXdbSequence.o PgMemcontext.o \
       PGTempEntrySet.o PGSortEntrySet.o PGDump.o MemoryPool.o PGSEException.o PGLargeObject.o \
       PGLargeObjectStore.o PGEntryIDBitmap.o  PGFDBitmapSet.o  PGXdbLock.o  XdbLock.o

LIBS = -L../../bin -lpostgres -lutils -lpthread -lboost_filesystem -lboost_system -lboost_thread -lboost_date_time -llog4cxx

all: libstorageEngine.so

libstorageEngine.so: $(OBJS)
	$(CC) -shared $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
	mv libstorageEngine.so ../../bin/
clean:
	rm -f ../../bin/libstorageEngine.so $(OBJS)

#include $(top_srcdir)/src/backend/common.mk
