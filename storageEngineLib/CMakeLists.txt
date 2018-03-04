# top-level files
set (storageEngineLib
)

# src files
set (storageEngineLib_src
)

# src -> backend files
set (storageEngineLib_src_backend
	../src/backend/win32ver.rc
)

# src -> backend -> access files
set (storageEngineLib_src_backend_access
)

# src -> backend -> access -> common files
set (storageEngineLib_src_backend_access_common
	../src/backend/access/common/xdb_common.c
	../src/backend/access/common/heaptuple.c
	../src/backend/access/common/indextuple.c
	../src/backend/access/common/reloptions.c
	../src/backend/access/common/scankey.c
	../src/backend/access/common/tupdesc.c
)

# src -> backend -> access -> hash files
set (storageEngineLib_src_backend_access_hash
	../src/backend/access/hash/hashfunc.c
	../src/backend/access/hash/hash.c
	../src/backend/access/hash/hashovfl.c
	../src/backend/access/hash/hashinsert.c
	../src/backend/access/hash/hashpage.c
	../src/backend/access/hash/hashscan.c
	../src/backend/access/hash/hashsearch.c
	../src/backend/access/hash/hashsort.c
	../src/backend/access/hash/hashutil.c
)

# src -> backend -> access -> heap files
set (storageEngineLib_src_backend_access_heap
	../src/backend/access/heap/xdb_heap.c
	../src/backend/access/heap/heapam.c
	../src/backend/access/heap/hio.c
	../src/backend/access/heap/pruneheap.c
	../src/backend/access/heap/rewriteheap.c
	../src/backend/access/heap/syncscan.c
	../src/backend/access/heap/tuptoaster.c
	../src/backend/access/heap/visibilitymap.c
)

# src -> backend -> access -> index files
set (storageEngineLib_src_backend_access_index
	../src/backend/access/index/genam.c
	../src/backend/access/index/indexam.c
)

# src -> backend -> access -> nbtree files
set (storageEngineLib_src_backend_access_nbtree
	../src/backend/access/nbtree/nbtcompare.c
	../src/backend/access/nbtree/nbtinsert.c
	../src/backend/access/nbtree/nbtpage.c
	../src/backend/access/nbtree/nbtree.c
	../src/backend/access/nbtree/nbtsearch.c
	../src/backend/access/nbtree/nbtsort.c
	../src/backend/access/nbtree/nbtutils.c
	../src/backend/access/nbtree/nbtxlog.c
)

# src -> backend -> access -> transam files
set (storageEngineLib_src_backend_access_transam
	../src/backend/access/transam/clog.c
	../src/backend/access/transam/multixact.c
	../src/backend/access/transam/rmgr.c
	../src/backend/access/transam/slru.c
	../src/backend/access/transam/subtrans.c
	../src/backend/access/transam/transam.c
	../src/backend/access/transam/twophase.c
	../src/backend/access/transam/twophase_rmgr.c
	../src/backend/access/transam/varsup.c
	../src/backend/access/transam/xact.c
	../src/backend/access/transam/xlog.c
	../src/backend/access/transam/xlogutils.c
	../src/backend/access/transam/timeline.c
)

# src -> backend -> catalog files
set (storageEngineLib_src_backend_catalog
	../src/backend/catalog/database.c
	../src/backend/catalog/mataData.cpp
	../src/backend/catalog/xdb_catalog.c
	../src/backend/catalog/catalog.c
	../src/backend/catalog/heap.c
	../src/backend/catalog/index.c
	../src/backend/catalog/storage.c
	../src/backend/catalog/toasting.c
	../src/backend/catalog/rdf_storage_handler.c
)

# src -> backend -> commands files
set (storageEngineLib_src_backend_commands
	../src/backend/commands/tablecmds.c
	../src/backend/commands/tablespace.c
	../src/backend/commands/vacuum.c
	../src/backend/commands/vacuumlazy.c
	../src/backend/commands/cluster.c
)

# src -> backend -> lib files
set (storageEngineLib_src_backend_lib
	../src/backend/lib/dllist.c
	../src/backend/lib/stringinfo.c
)

# src -> backend -> libpq files
set (storageEngineLib_src_backend_libpq
	../src/backend/libpq/auth.c
	../src/backend/libpq/be-secure.c
	../src/backend/libpq/fe-connect.c
	../src/backend/libpq/fe-exec.c
	../src/backend/libpq/fe-misc.c
	../src/backend/libpq/fe-protocol2.c
	../src/backend/libpq/fe-protocol3.c
	../src/backend/libpq/fe-secure.c
	../src/backend/libpq/ip.c
	../src/backend/libpq/pqcomm.c
	../src/backend/libpq/pqexpbuffer.c
	../src/backend/libpq/pqformat.c
)
if (MSVC)
set (storageEngineLib_src_backend_libpq
    ${storageEngineLib_src_backend_libpq}
    ../src/backend/libpq/win32.c
    )
endif (MSVC)

# src -> backend -> nodes files
set (storageEngineLib_src_backend_nodes
	../src/backend/nodes/equalfuncs.c
	../src/backend/nodes/makefuncs.c
	../src/backend/nodes/outfuncs.c
	../src/backend/nodes/bitmapset.c
	../src/backend/nodes/list.c
	../src/backend/nodes/nodes.c
	../src/backend/nodes/tidbitmap.c
	../src/backend/nodes/value.c
)

# src -> backend -> port files
set (storageEngineLib_src_backend_port
	../src/backend/port/copyfile.c
	../src/backend/port/fxdb_shmem.c
	../src/backend/port/sysv_shmem.c
	../src/backend/port/thread_commu.c
	../src/backend/port/timer.c
	../src/backend/port/pipe.c
)
if (MSVC)
set (storageEngineLib_src_backend_port
	${storageEngineLib_src_backend_port}
	../src/backend/port/win32_latch.c
	../src/backend/port/win32_sema.c
)
else (MSVC)
set (storageEngineLib_src_backend_port
	${storageEngineLib_src_backend_port}
	#../src/backend/port/dynloader.c
	../src/backend/port/dynloader/linux.c
	../src/backend/port/pg_sema.c
	../src/backend/port/pg_shmem.c
	../src/backend/port/pg_latch.c
)
endif (MSVC)

# src -> backend -> port -> dynloader files
set (storageEngineLib_src_backend_port_dynloader
	../src/backend/port/dynloader/win32.c
)

# src -> backend -> port -> win32 files
set (storageEngineLib_src_backend_port_win32
	../src/backend/port/win32/socket.c
	../src/backend/port/win32/crashdump.c
	../src/backend/port/win32/security.c
	../src/backend/port/win32/timer.c
)

if (NOT MSVC)
set (storageEngineLib_src_backend_port_win32 ${storageEngineLib_src_backend_port_win32}
	../src/backend/port/win32/mingwcompat.c
)
endif (NOT MSVC)

# src -> backend -> postmaster files
set (storageEngineLib_src_backend_postmaster
	../src/backend/postmaster/xdb_main.c
	../src/backend/postmaster/autovacuum.c
	../src/backend/postmaster/bgwriter.c
	../src/backend/postmaster/pgarch.c
	../src/backend/postmaster/pgstat.c
	../src/backend/postmaster/postmaster.c
	../src/backend/postmaster/syslogger.c
	../src/backend/postmaster/walwriter.c
	../src/backend/postmaster/lsm.c
)
if (NOT MSVC)
set(storageEngineLib_src_backend_postmaster ${storageEngineLib_src_backend_postmaster}
	../src/backend/postmaster/fork_process.c)
endif(NOT MSVC)

# src -> backend -> replication files
set (storageEngineLib_src_backend_replication_repl_ha
	../src/backend/replication/repl_ha/ha_election.cpp
	../src/backend/replication/repl_ha/ha_env.cpp
	../src/backend/replication/repl_ha/ha_net.cpp
	../src/backend/replication/repl_ha/ha_select.cpp)

set (storageEngineLib_src_backend_replication
	../src/backend/replication/basebackup.c
	../src/backend/replication/libpqwalreceiver.c
	../src/backend/replication/repl_gram.c
	../src/backend/replication/syncrep.c
	../src/backend/replication/walreceiver.c
	../src/backend/replication/walreceiverfuncs.c
	../src/backend/replication/walsender.c
)

# src -> backend -> storage files
set (storageEngineLib_src_backend_storage
)

# src -> backend -> storage -> buffer files
set (storageEngineLib_src_backend_storage_buffer
	../src/backend/storage/buffer/xdb_buffer.c
	../src/backend/storage/buffer/buf_init.c
	../src/backend/storage/buffer/buf_table.c
	../src/backend/storage/buffer/bufmgr.c
	../src/backend/storage/buffer/freelist.c
	../src/backend/storage/buffer/localbuf.c
)

# src -> backend -> storage -> file files
set (storageEngineLib_src_backend_storage_file
	../src/backend/storage/file/fileinfo.c
	../src/backend/storage/file/buffile.c
	../src/backend/storage/file/copydir.c
	../src/backend/storage/file/fd.c  
	../src/backend/storage/file/reinit.c
)

# src -> backend -> storage -> freespace files
set (storageEngineLib_src_backend_storage_freespace
	../src/backend/storage/freespace/freespace.c
	../src/backend/storage/freespace/fsmpage.c
	../src/backend/storage/freespace/indexfsm.c
)

# src -> backend -> storage -> ipc files
set (storageEngineLib_src_backend_storage_ipc
	../src/backend/storage/ipc/ipc.c
	../src/backend/storage/ipc/ipci.c
	../src/backend/storage/ipc/pmsignal.c
	../src/backend/storage/ipc/procarray.c
	../src/backend/storage/ipc/procsignal.c
	../src/backend/storage/ipc/shmem.c
	../src/backend/storage/ipc/shmqueue.c
	../src/backend/storage/ipc/sinval.c
	../src/backend/storage/ipc/sinvaladt.c
	../src/backend/storage/ipc/standby.c
)

# src -> backend -> storage -> large_object files
set (storageEngineLib_src_backend_storage_large_object
	../src/backend/storage/large_object/inv_api.c
)

# src -> backend -> storage -> lmgr files
set (storageEngineLib_src_backend_storage_lmgr
	../src/backend/storage/lmgr/deadlock.c
	../src/backend/storage/lmgr/lmgr.c
	../src/backend/storage/lmgr/lock.c
	../src/backend/storage/lmgr/lwlock.c
	../src/backend/storage/lmgr/predicate.c
	../src/backend/storage/lmgr/proc.c
	../src/backend/storage/lmgr/s_lock.c
	../src/backend/storage/lmgr/spin.c
)

# src -> backend -> storage -> page files
set (storageEngineLib_src_backend_storage_page
	../src/backend/storage/page/bufpage.c
	../src/backend/storage/page/itemptr.c
)

# src -> backend -> storage -> smgr files
set (storageEngineLib_src_backend_storage_smgr
	../src/backend/storage/smgr/md.c
	../src/backend/storage/smgr/smgr.c
	../src/backend/storage/smgr/smgrtype.c
)

# src -> backend -> tcop files
set (storageEngineLib_src_backend_tcop
	../src/backend/tcop/dest.c
	../src/backend/tcop/postgres.c
)

# src -> backend -> utils files
set (storageEngineLib_src_backend_utils
	../src/backend/utils/fmgrtab.c
	../src/backend/utils/linkerror.c
)

# src -> backend -> utils -> adt files
set (storageEngineLib_src_backend_utils_adt
	../src/backend/utils/adt/bool.c
	../src/backend/utils/adt/datum.c
	../src/backend/utils/adt/name.c
	../src/backend/utils/adt/pg_lzcompress.c
	../src/backend/utils/adt/timestamp.c
	../src/backend/utils/adt/varlena.c
)

# src -> backend -> utils -> cache files
set (storageEngineLib_src_backend_utils_cache
	../src/backend/utils/cache/inval.c
	../src/backend/utils/cache/relcache.c
)

# src -> backend -> utils -> error files
set (storageEngineLib_src_backend_utils_error
	../src/backend/utils/error/assert.c
	../src/backend/utils/error/elog.c
)

# src -> backend -> utils -> fmgr files
set (storageEngineLib_src_backend_utils_fmgr
	../src/backend/utils/fmgr/fmgr.c
)

# src -> backend -> utils -> hash files
set (storageEngineLib_src_backend_utils_hash
	../src/backend/utils/hash/dynahash.c
	../src/backend/utils/hash/hashfn.c
	../src/backend/utils/hash/pg_crc.c
)

# src -> backend -> utils -> init files
set (storageEngineLib_src_backend_utils_init
	../src/backend/utils/init/globals.c
	../src/backend/utils/init/miscinit.c
)

# src -> backend -> utils -> misc files
set (storageEngineLib_src_backend_utils_misc
	#../src/backend/utils/misc/guc-file.l
)

# src -> backend -> utils -> mmgr files
set (storageEngineLib_src_backend_utils_mmgr
	../src/backend/utils/mmgr/aset.c
	../src/backend/utils/mmgr/mcxt.c
	../src/backend/utils/mmgr/MemTrace.cpp
)

# src -> backend -> utils -> resowner files
set (storageEngineLib_src_backend_utils_resowner
	../src/backend/utils/resowner/resowner.c
)

# src -> backend -> utils -> sort files
set (storageEngineLib_src_backend_utils_sort
	../src/backend/utils/sort/logtape.c
	../src/backend/utils/sort/tuplesort.c
	../src/backend/utils/sort/tuplestore.c
)

# src -> backend -> utils -> time files
set (storageEngineLib_src_backend_utils_time
	../src/backend/utils/time/combocid.c
	../src/backend/utils/time/snapmgr.c
	../src/backend/utils/time/tqual.c
)

# src -> include files
set (storageEngineLib_src_include
	../src/include/fmgr.h
)

# src -> include -> access files
set (storageEngineLib_src_include_access
	../src/include/access/attnum.h
	../src/include/access/clog.h
	../src/include/access/genam.h
	../src/include/access/gin.h
	../src/include/access/gin_private.h
	../src/include/access/gist.h
	../src/include/access/gist_private.h
	../src/include/access/gistscan.h
	../src/include/access/hash.h
	../src/include/access/heapam.h
	../src/include/access/hio.h
	../src/include/access/htup.h
	../src/include/access/itup.h
	../src/include/access/multixact.h
	../src/include/access/nbtree.h
	../src/include/access/reloptions.h
	../src/include/access/relscan.h
	../src/include/access/rewriteheap.h
	../src/include/access/rmgr.h
	../src/include/access/sdir.h
	../src/include/access/skey.h
	../src/include/access/slru.h
	../src/include/access/subtrans.h
	../src/include/access/sysattr.h
	../src/include/access/transam.h
	../src/include/access/tupdesc.h
	../src/include/access/tupmacs.h
	../src/include/access/tuptoaster.h
	../src/include/access/twophase.h
	../src/include/access/twophase_rmgr.h
	../src/include/access/valid.h
	../src/include/access/visibilitymap.h
	../src/include/access/xact.h
	../src/include/access/xdb_common.h
	../src/include/access/xdb_heap.h
	../src/include/access/xdb_transam.h
	../src/include/access/xlog.h
	../src/include/access/xlog_internal.h
	../src/include/access/xlogdefs.h
	../src/include/access/xlogutils.h
	../src/include/access/timeline.h
)

# src -> include -> catalog files
set (storageEngineLib_src_include_catalog
	../src/include/catalog/catalog.h
	../src/include/catalog/genbki.h
	../src/include/catalog/heap.h
	../src/include/catalog/index.h
	../src/include/catalog/indexing.h
	../src/include/catalog/metaData.h
	../src/include/catalog/pg_control.h
	../src/include/catalog/storage.h
	../src/include/catalog/toasting.h
	../src/include/catalog/xdb_catalog.h
	../src/include/catalog/rdf_storage_handler.h
)

# src -> include -> commands files
set (storageEngineLib_src_include_commands
	#../src/include/commands/async.h
	../src/include/commands/tablecmds.h
	../src/include/commands/tablespace.h
	../src/include/commands/vacuum.h
	../src/include/commands/cluster.h
)

# src -> include -> interface files
set (storageEngineLib_src_include_interface
	../src/include/interface/ErrNo.h
	../src/include/interface/FDPGAdapter.h
	#../src/include/interface/metaData.h
	../src/include/interface/StorageEngineExceptionUniversal.h
)

# src -> include -> lib files
set (storageEngineLib_src_include_lib
	../src/include/lib/dllist.h
	../src/include/lib/stringinfo.h
)

# src -> include -> libpq files
set (storageEngineLib_src_include_libpq
	../src/include/libpq/auth.h
	../src/include/libpq/be-fsstubs.h
	../src/include/libpq/crypt.h
	../src/include/libpq/fe-auth.h
	../src/include/libpq/hba.h
	../src/include/libpq/ip.h
	../src/include/libpq/libpq-be.h
	../src/include/libpq/libpq-events.h
	../src/include/libpq/libpq-fe.h
	../src/include/libpq/libpq-fs.h
	../src/include/libpq/libpq-int.h
	../src/include/libpq/libpq.h
	../src/include/libpq/md5.h
	../src/include/libpq/pqcomm.h
	../src/include/libpq/pqexpbuffer.h
	../src/include/libpq/pqformat.h
	../src/include/libpq/pqsignal.h
	../src/include/libpq/win32.h
)

# src -> include -> nodes files
set (storageEngineLib_src_include_nodes
	../src/include/nodes/bitmapset.h
	../src/include/nodes/execnodes.h
	../src/include/nodes/makefuncs.h
	../src/include/nodes/memnodes.h
	../src/include/nodes/nodes.h
	../src/include/nodes/parsenodes.h
	../src/include/nodes/pg_list.h
	../src/include/nodes/tidbitmap.h
	../src/include/nodes/value.h
)

# src -> include -> port files
set (storageEngineLib_src_include_port
	../src/include/port/aix.h
	../src/include/port/bsdi.h
	../src/include/port/cygwin.h
	../src/include/port/darwin.h
	../src/include/port/dgux.h
	../src/include/port/freebsd.h
	../src/include/port/fxdb_shmem.h
	../src/include/port/hpux.h
	../src/include/port/irix.h
	../src/include/port/linux.h
	../src/include/port/netbsd.h
	../src/include/port/nextstep.h
	../src/include/port/openbsd.h
	../src/include/port/osf.h
	#../src/include/port/pg_pthread.h
	../src/include/port/sco.h
	../src/include/port/solaris.h
	../src/include/port/sunos4.h
	../src/include/port/svr4.h
	../src/include/port/thread_commu.h
	../src/include/port/ultrix4.h
	../src/include/port/univel.h
	../src/include/port/unixware.h
	../src/include/port/win32.h
)

# src -> include -> port -> win32 files
set (storageEngineLib_src_include_port_win32
	../src/include/port/win32/dlfcn.h
	../src/include/port/win32/grp.h
	../src/include/port/win32/netdb.h
	../src/include/port/win32/pwd.h
)

# src -> include -> port -> win32 -> arpa files
set (storageEngineLib_src_include_port_win32_arpa
	../src/include/port/win32/arpa/inet.h
)

# src -> include -> port -> win32_msvc files
set (storageEngineLib_src_include_port_win32_msvc
	../src/include/port/win32_msvc/dirent.h
	../src/include/port/win32_msvc/unistd.h
	../src/include/port/win32_msvc/utime.h
)

# src -> include -> port -> win32_msvc -> sys files
set (storageEngineLib_src_include_port_win32_msvc_sys
	../src/include/port/win32_msvc/sys/file.h
	../src/include/port/win32_msvc/sys/param.h
	../src/include/port/win32_msvc/sys/time.h
)

# src -> include -> port -> win32 -> netinet files
set (storageEngineLib_src_include_port_win32_netinet
	../src/include/port/win32/netinet/in.h
)

# src -> include -> port -> win32 -> sys files
set (storageEngineLib_src_include_port_win32_sys
	../src/include/port/win32/sys/socket.h
	../src/include/port/win32/sys/wait.h
)

# src -> include -> postmaster files
set (storageEngineLib_src_include_postmaster
	../src/include/postmaster/autovacuum.h
	../src/include/postmaster/bgwriter.h
	../src/include/postmaster/fork_process.h
	../src/include/postmaster/pgarch.h
	../src/include/postmaster/postmaster.h
	../src/include/postmaster/syslogger.h
	../src/include/postmaster/walwriter.h
	../src/include/postmaster/xdb_main.h
)

# src -> include -> replication files
set (storageEngineLib_src_include_replication_repl_ha
	../src/backend/replication/repl_ha/ha_debug.h
	../src/backend/replication/repl_ha/ha_env.h
	../src/backend/replication/repl_ha/ha_net.h
	../src/backend/replication/repl_ha/ha_head.h)

set (storageEngineLib_src_include_replication
	../src/include/replication/basebackup.h
	#../src/include/replication/replnodes.h
	../src/include/replication/syncrep.h
	../src/include/replication/walprotocol.h
	../src/include/replication/walreceiver.h
	../src/include/replication/walsender.h
)

# src -> include -> storage files
set (storageEngineLib_src_include_storage
	../src/include/storage/backendid.h
	../src/include/storage/block.h
	../src/include/storage/buf.h
	../src/include/storage/buf_internals.h
	../src/include/storage/buffile.h
	../src/include/storage/bufmgr.h
	../src/include/storage/bufpage.h
	../src/include/storage/copydir.h
	../src/include/storage/fd.h
	../src/include/storage/fileinfo.h
	../src/include/storage/freespace.h
	../src/include/storage/fsm_internals.h
	../src/include/storage/indexfsm.h
	../src/include/storage/ipc.h
	../src/include/storage/item.h
	../src/include/storage/itemid.h
	../src/include/storage/itemptr.h
	../src/include/storage/large_object.h
	../src/include/storage/latch.h
	../src/include/storage/lmgr.h
	../src/include/storage/lock.h
	../src/include/storage/lwlock.h
	../src/include/storage/off.h
	../src/include/storage/pg_sema.h
	../src/include/storage/pg_shmem.h
	../src/include/storage/pmsignal.h
	../src/include/storage/pos.h
	../src/include/storage/predicate.h
	../src/include/storage/predicate_internals.h
	../src/include/storage/proc.h
	../src/include/storage/procarray.h
	../src/include/storage/procsignal.h
	../src/include/storage/reinit.h
	../src/include/storage/relfilenode.h
	../src/include/storage/s_lock.h
	../src/include/storage/shmem.h
	../src/include/storage/sinval.h
	../src/include/storage/sinvaladt.h
	../src/include/storage/smgr.h
	../src/include/storage/spin.h
	../src/include/storage/standby.h
	../src/include/storage/xdb_buffer.h
	../src/include/storage/xdb_storage_ipc.h
)

# src -> include -> tcop files
set (storageEngineLib_src_include_tcop
	../src/include/tcop/tcopprot.h
)

# src -> include -> utils files
set (storageEngineLib_src_include_utils
	../src/include/utils/array.h
	../src/include/utils/builtins.h
	../src/include/utils/combocid.h
	../src/include/utils/datetime.h
	../src/include/utils/datum.h
	../src/include/utils/dynahash.h
	../src/include/utils/dynamic_loader.h
	../src/include/utils/elog.h
	../src/include/utils/errcodes.h
	../src/include/utils/fmgroids.h
	../src/include/utils/fmgrtab.h
	../src/include/utils/geo_decls.h
	../src/include/utils/guc.h
	../src/include/utils/hsearch.h
	../src/include/utils/int8.h
	../src/include/utils/inval.h
	../src/include/utils/logtape.h
	../src/include/utils/lsyscache.h
	../src/include/utils/memutils.h
	../src/include/utils/palloc.h
	../src/include/utils/pg_crc.h
	../src/include/utils/pg_lzcompress.h
	#../src/include/utils/pg_rusage.h
	../src/include/utils/probes.h
	../src/include/utils/rbtree.h
	../src/include/utils/rel.h
	../src/include/utils/relcache.h
	../src/include/utils/relmapper.h
	../src/include/utils/resowner.h
	../src/include/utils/selfuncs.h
	../src/include/utils/snapmgr.h
	../src/include/utils/snapshot.h
	../src/include/utils/spccache.h
	../src/include/utils/syscache.h
	../src/include/utils/timestamp.h
	../src/include/utils/tqual.h
	../src/include/utils/tuplesort.h
	../src/include/utils/tuplestore.h
	../src/include/utils/typcache.h
	../src/include/utils/tzparser.h
	../src/include/utils/xdb_utils.h
	../src/include/utils/MemTrace.h
)

# src -> interface files
set (storageEngineLib_src_interface
	../src/interface/FDPGAdapter.cpp
	../src/interface/StorageEngineExceptionUniversal.cpp
)

# src -> port files
set (storageEngineLib_src_port
	../src/port/dirmod.c
	../src/port/exec.c
    ../src/port/inet_net_ntop.c
	../src/port/noblock.c
	../src/port/path.c
	../src/port/pgcheckdir.c
	../src/port/pgmkdirp.c
	../src/port/pgstrcasecmp.c
	../src/port/qsort.c
	../src/port/qsort_arg.c
	../src/port/sprompt.c
	../src/port/thread.c
	../src/port/strlcpy.c
)
if (MSVC)
set (storageEngineLib_src_port
	${storageEngineLib_src_port}
	../src/port/crypt.c
	../src/port/dirent.c
	../src/port/erand48.c
	../src/port/fseeko.c
	../src/port/getaddrinfo.c
	../src/port/getopt.c
	../src/port/getopt_long.c
	../src/port/getrusage.c
	../src/port/gettimeofday.c
	../src/port/inet_aton.c
	../src/port/kill.c
	../src/port/open.c
	../src/port/random.c
	../src/port/rint.c
	../src/port/snprintf.c
	../src/port/srandom.c
	../src/port/strlcat.c
	../src/port/win32env.c
	../src/port/win32error.c
)
else (MSVC)
set (storageEngineLib_src_port
	${storageEngineLib_src_port}
	../src/port/getpeereid.c
	../src/port/pgsleep.c
)
endif(MSVC)
# src -> timezone files
set (storageEngineLib_src_timezone
	../src/timezone/localtime.c
	../src/timezone/pgtz.c
	../src/timezone/strftime.c
)

# the source groups
source_group(""                                    FILES ${storageEngineLib})
source_group("src"                                 FILES ${storageEngineLib_src})
source_group("src\\backend"                        FILES ${storageEngineLib_src_backend})
source_group("src\\backend\\access"                FILES ${storageEngineLib_src_backend_access})
source_group("src\\backend\\access\\common"        FILES ${storageEngineLib_src_backend_access_common})
source_group("src\\backend\\access\\hash"          FILES ${storageEngineLib_src_backend_access_hash})
source_group("src\\backend\\access\\heap"          FILES ${storageEngineLib_src_backend_access_heap})
source_group("src\\backend\\access\\index"         FILES ${storageEngineLib_src_backend_access_index})
source_group("src\\backend\\access\\nbtree"        FILES ${storageEngineLib_src_backend_access_nbtree})
source_group("src\\backend\\access\\transam"       FILES ${storageEngineLib_src_backend_access_transam})
source_group("src\\backend\\catalog"               FILES ${storageEngineLib_src_backend_catalog})
source_group("src\\backend\\commands"              FILES ${storageEngineLib_src_backend_commands})
source_group("src\\backend\\lib"                   FILES ${storageEngineLib_src_backend_lib})
source_group("src\\backend\\libpq"                 FILES ${storageEngineLib_src_backend_libpq})
source_group("src\\backend\\nodes"                 FILES ${storageEngineLib_src_backend_nodes})
source_group("src\\backend\\port"                  FILES ${storageEngineLib_src_backend_port})
source_group("src\\backend\\port\\dynloader"       FILES ${storageEngineLib_src_backend_port_dynloader})
source_group("src\\backend\\port\\win32"           FILES ${storageEngineLib_src_backend_port_win32})
source_group("src\\backend\\postmaster"            FILES ${storageEngineLib_src_backend_postmaster})
source_group("src\\backend\\replication"           FILES ${storageEngineLib_src_backend_replication})
source_group("src\\backend\\replication\\repl_ha"  FILES ${storageEngineLib_src_backend_replication_repl_ha})
source_group("src\\backend\\storage"               FILES ${storageEngineLib_src_backend_storage})
source_group("src\\backend\\storage\\buffer"       FILES ${storageEngineLib_src_backend_storage_buffer})
source_group("src\\backend\\storage\\file"         FILES ${storageEngineLib_src_backend_storage_file})
source_group("src\\backend\\storage\\freespace"    FILES ${storageEngineLib_src_backend_storage_freespace})
source_group("src\\backend\\storage\\ipc"          FILES ${storageEngineLib_src_backend_storage_ipc})
source_group("src\\backend\\storage\\large_object" FILES ${storageEngineLib_src_backend_storage_large_object})
source_group("src\\backend\\storage\\lmgr"         FILES ${storageEngineLib_src_backend_storage_lmgr})
source_group("src\\backend\\storage\\page"         FILES ${storageEngineLib_src_backend_storage_page})
source_group("src\\backend\\storage\\smgr"         FILES ${storageEngineLib_src_backend_storage_smgr})
source_group("src\\backend\\tcop"                  FILES ${storageEngineLib_src_backend_tcop})
source_group("src\\backend\\utils"                 FILES ${storageEngineLib_src_backend_utils})
source_group("src\\backend\\utils\\adt"            FILES ${storageEngineLib_src_backend_utils_adt})
source_group("src\\backend\\utils\\cache"          FILES ${storageEngineLib_src_backend_utils_cache})
source_group("src\\backend\\utils\\error"          FILES ${storageEngineLib_src_backend_utils_error})
source_group("src\\backend\\utils\\fmgr"           FILES ${storageEngineLib_src_backend_utils_fmgr})
source_group("src\\backend\\utils\\hash"           FILES ${storageEngineLib_src_backend_utils_hash})
source_group("src\\backend\\utils\\init"           FILES ${storageEngineLib_src_backend_utils_init})
source_group("src\\backend\\utils\\misc"           FILES ${storageEngineLib_src_backend_utils_misc})
source_group("src\\backend\\utils\\mmgr"           FILES ${storageEngineLib_src_backend_utils_mmgr})
source_group("src\\backend\\utils\\resowner"       FILES ${storageEngineLib_src_backend_utils_resowner})
source_group("src\\backend\\utils\\sort"           FILES ${storageEngineLib_src_backend_utils_sort})
source_group("src\\backend\\utils\\time"           FILES ${storageEngineLib_src_backend_utils_time})
source_group("src\\include"                        FILES ${storageEngineLib_src_include})
source_group("src\\include\\access"                FILES ${storageEngineLib_src_include_access})
source_group("src\\include\\catalog"               FILES ${storageEngineLib_src_include_catalog})
source_group("src\\include\\commands"              FILES ${storageEngineLib_src_include_commands})
source_group("src\\include\\interface"             FILES ${storageEngineLib_src_include_interface})
source_group("src\\include\\lib"                   FILES ${storageEngineLib_src_include_lib})
source_group("src\\include\\libpq"                 FILES ${storageEngineLib_src_include_libpq})
source_group("src\\include\\nodes"                 FILES ${storageEngineLib_src_include_nodes})
source_group("src\\include\\port"                  FILES ${storageEngineLib_src_include_port})
source_group("src\\include\\port\\win32"           FILES ${storageEngineLib_src_include_port_win32})
source_group("src\\include\\port\\win32\\arpa"     FILES ${storageEngineLib_src_include_port_win32_arpa})
source_group("src\\include\\port\\win32_msvc"      FILES ${storageEngineLib_src_include_port_win32_msvc})
source_group("src\\include\\port\\win32_msvc\\sys" FILES ${storageEngineLib_src_include_port_win32_msvc_sys})
source_group("src\\include\\port\\win32\\netinet"  FILES ${storageEngineLib_src_include_port_win32_netinet})
source_group("src\\include\\port\\win32\\sys"      FILES ${storageEngineLib_src_include_port_win32_sys})
source_group("src\\include\\postmaster"            FILES ${storageEngineLib_src_include_postmaster})
source_group("src\\include\\replication"           FILES ${storageEngineLib_src_include_replication})
source_group("src\\include\\replication\\repl_ha"  FILES ${storageEngineLib_src_include_replication_repl_ha})
source_group("src\\include\\storage"               FILES ${storageEngineLib_src_include_storage})
source_group("src\\include\\tcop"                  FILES ${storageEngineLib_src_include_tcop})
source_group("src\\include\\utils"                 FILES ${storageEngineLib_src_include_utils})
source_group("src\\interface"                      FILES ${storageEngineLib_src_interface})
source_group("src\\port"                           FILES ${storageEngineLib_src_port})
source_group("src\\timezone"                       FILES ${storageEngineLib_src_timezone})

set (storageEngineLib_SRC
    ${storageEngineLib_src}
	${storageEngineLib_src_backend_access}
	${storageEngineLib_src_backend_access_common}
	${storageEngineLib_src_backend_access_hash}
	${storageEngineLib_src_backend_access_heap}
	${storageEngineLib_src_backend_access_index}
	${storageEngineLib_src_backend_access_nbtree}
	${storageEngineLib_src_backend_access_transam}
	${storageEngineLib_src_backend_catalog}
	${storageEngineLib_src_backend_commands}
	${storageEngineLib_src_backend_lib}
	${storageEngineLib_src_backend_libpq}
	${storageEngineLib_src_backend_nodes}
	${storageEngineLib_src_backend_port}
	${storageEngineLib_src_backend_postmaster}
	${storageEngineLib_src_backend_replication}
	${storageEngineLib_src_backend_replication_repl_ha}
	${storageEngineLib_src_backend_storage}
	${storageEngineLib_src_backend_storage_buffer}
	${storageEngineLib_src_backend_storage_file}
	${storageEngineLib_src_backend_storage_freespace}
	${storageEngineLib_src_backend_storage_ipc}
	${storageEngineLib_src_backend_storage_large_object}
	${storageEngineLib_src_backend_storage_lmgr}
	${storageEngineLib_src_backend_storage_page}
	${storageEngineLib_src_backend_storage_smgr}
	${storageEngineLib_src_backend_tcop}
	${storageEngineLib_src_backend_utils}
	${storageEngineLib_src_backend_utils_adt}
	${storageEngineLib_src_backend_utils_cache}
	${storageEngineLib_src_backend_utils_error}
	${storageEngineLib_src_backend_utils_fmgr}
	${storageEngineLib_src_backend_utils_hash}
	${storageEngineLib_src_backend_utils_init}
	${storageEngineLib_src_backend_utils_misc}
	${storageEngineLib_src_backend_utils_mmgr}
	${storageEngineLib_src_backend_utils_resowner}
	${storageEngineLib_src_backend_utils_sort}
	${storageEngineLib_src_backend_utils_time}
    ${storageEngineLib_src_interface}
	${storageEngineLib_src_port}
	${storageEngineLib_src_timezone}
)
if (MSVC)
set (storageEngineLib_SRC
	${storageEngineLib_SRC}
	${storageEngineLib_src_backend_port_dynloader}
	${storageEngineLib_src_backend_port_win32}
	${storageEngineLib_src_backend}
)
endif (MSVC)

set (storageEngineLib_INCLUDE
	${storageEngineLib_src_include}
	${storageEngineLib_src_include_access}
	${storageEngineLib_src_include_catalog}
	${storageEngineLib_src_include_commands}
	${storageEngineLib_src_include_interface}
	${storageEngineLib_src_include_lib}
	${storageEngineLib_src_include_libpq}
	${storageEngineLib_src_include_nodes}
	${storageEngineLib_src_include_port}
	${storageEngineLib_src_include_port_win32}
	${storageEngineLib_src_include_port_win32_arpa}
	${storageEngineLib_src_include_port_win32_msvc}
	${storageEngineLib_src_include_port_win32_msvc_sys}
	${storageEngineLib_src_include_port_win32_netinet}
	${storageEngineLib_src_include_port_win32_sys}
	${storageEngineLib_src_include_postmaster}
	${storageEngineLib_src_include_replication}
	${storageEngineLib_src_include_replication_repl_ha}
	${storageEngineLib_src_include_storage}
	${storageEngineLib_src_include_tcop}
	${storageEngineLib_src_include_utils}
)
# append all files to the storageEngineLib
set (storageEngineLib
	${storageEngineLib}
	${storageEngineLib_SRC}
	${storageEngineLib_INCLUDE}
)
set_source_files_properties(${storageEngineLib_SRC} PROPERTIES LANGUAGE CXX)

add_library(storageEngineLib STATIC ${storageEngineLib})

include_directories(../src/include 
    ../src/backend 
    )
add_definitions(
    -DEXEC_BACKEND 
    -DBUILDING_DLL
    -DFOUNDER_XDB_SE)
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DUSE_ASSERT_CHECKING")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DUSE_ASSERT_CHECKING")
set (storageEngineLib_DEP CommonUtils)
if (MSVC)
    set_target_properties( storageEngineLib PROPERTIES COMPILE_FLAGS "/wd4127 /wd4611" )
    include_directories(
      ../src/include/port/win32 
      ../src/include/port/win32_msvc 
      ../pthread
    )
    add_definitions(-D__WIN32__ 
    -D__WINDOWS__ 
    -DWIN32_STACK_RLIMIT=4194304 
   )

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
   		SET_TARGET_PROPERTIES(storageEngineLib PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
   	endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
   set (storageEngineLib_DEP pthread ${storageEngineLib_DEP})
else (MSVC)
   add_definitions(-D_GNU_SOURCE)
endif (MSVC)
add_dependencies(storageEngineLib ${storageEngineLib_DEP})
target_link_libraries(storageEngineLib ${storageEngineLib_DEP})
#install(TARGETS storageEngineLib
#	RUNTIME DESTINATION server/bin
#	LIBRARY DESTINATION server/bin
#	COMPONENT core)
#install(TARGETS storageEngineLib
#	ARCHIVE DESTINATION api/c++/lib
#	COMPONENT develop)
