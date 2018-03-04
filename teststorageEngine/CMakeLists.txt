# top-level files
set (teststorageEngine
)

# include files
set (teststorageEngine_include
	include/test_fram.h
	include/UnitTest.h
)

# include -> bgwriter files
set (teststorageEngine_include_bgwriter
	include/bgwriter/test_bgwriter.h
)

# include -> deadlock files
set (teststorageEngine_include_deadlock
	include/deadlock/deadlocktest.h
)

# include -> fatal_exception files
set (teststorageEngine_include_fatal_exception
	include/fatal_exception/fatal_exception.h
)

# include -> heap files
set (teststorageEngine_include_heap
	include/heap/compress.h
	include/heap/test_delete.h
	include/heap/test_heap_close.h
	include/heap/test_heap_create.h
	include/heap/test_heap_drop.h
	include/heap/test_heap_hot_update.h
	include/heap/test_heap_open.h
	include/heap/test_insert.h
	include/heap/test_select.h
	include/heap/test_toast.h
	include/heap/test_truncate.h
)

# include -> heap_sort_store files
set (teststorageEngine_include_heap_sort_store
	include/heap_sort_store/heap_store.h
)

# include -> index files
set (teststorageEngine_include_index
	include/index/test_index.h
	include/index/test_index_cmp.h
	include/index/test_index_delete.h
	include/index/test_index_insert.h
	include/index/test_index_update.h
	include/index/test_toast_index.h
	include/index/test_lsm_index.h
)

# include -> large_object files
set (teststorageEngine_include_large_object
	include/large_object/large_object_test.h
)

# include -> meta files
set (teststorageEngine_include_meta
	include/meta/meta_test.h
)

# include -> tablespace files
set (teststorageEngine_include_tablespace
	include/tablespace/create_tablespace.h
)

# include -> thread_commu files
set (teststorageEngine_include_thread_commu
	include/thread_commu/test_multthread_communicate.h
	include/thread_commu/thread_commu.h
	include/thread_commu/thread_commu_hs.h
	src/thread_commu/thread_atexit_text.h
)

# include -> transaction files
set (teststorageEngine_include_transaction
	include/transaction/test_index_transaction_search.h
	include/transaction/test_subtrans.h
	include/transaction/test_trans_toast_index.h
	include/transaction/test_transaction_delete.h
	include/transaction/test_transaction_insert.h
	include/transaction/test_transaction_update.h
	include/transaction/test_transactionWithIndex.h
	include/transaction/test_transactionWithToast.h
	include/transaction/test_isolation_level.h
	include/transaction/test_trans_prepare.h
)

# include -> database
set (teststorageEngine_include_database
	include/database/test_db.h
)


#include -> cluster_simple files
set (teststorageEngine_include_cluster_simple
	include/cluster/cluster_simple.h
	include/cluster/cluster_thread.h
)

#include -> vacuum files
set (teststorageEngine_include_vacuum
	include/vacuum/test_vacuum.h
)

# include -> utils files
set (teststorageEngine_include_utils
	include/utils/toast_test_utils.h
	include/utils/util.h
)

# src files
set (teststorageEngine_src
)

# src -> bgwriter files
set (teststorageEngine_src_bgwriter
	src/bgwriter/test_bgwriter.cpp
)

# src -> deadlock files
set (teststorageEngine_src_deadlock
	src/deadlock/deadlocktest.cpp
)

# src -> fatal_exception files
set (teststorageEngine_src_fatal_exception
	src/fatal_exception/fatal_exception.cpp
)

# src -> heap files
set (teststorageEngine_src_heap
	src/heap/compress.cpp
	src/heap/test_delete.cpp
	src/heap/test_heap_close.cpp
	src/heap/test_heap_create.cpp
	src/heap/test_heap_drop.cpp
	src/heap/test_heap_hot_update.cpp
	src/heap/test_heap_open.cpp
	src/heap/test_insert.cpp
	src/heap/test_select.cpp
	src/heap/test_truncate.cpp
	src/transaction/test_toast.cpp
)

# src -> heap_sort_store files
set (teststorageEngine_src_heap_sort_store
	src/heap_sort_store/heap_store.cpp
)

# src -> index files
set (teststorageEngine_src_index
	src/heap/test_index_insert.cpp
	src/heap/test_index_update.cpp
	src/index/test_index.cpp
	src/index/test_index_cmp.cpp
	src/index/test_index_delete.cpp
	src/index/test_toast_index.cpp
	src/index/test_lsm_index.cpp
)

# src -> large_object files
set (teststorageEngine_src_large_object
	src/large_object/large_object_test.cpp
)

# src -> meta files
set (teststorageEngine_src_meta
	src/meta/meta_test.cpp
)

# src -> tablespace files
set (teststorageEngine_src_tablespace
	src/tablespace/create_tablespace.cpp
)

# src -> test_fram files
set (teststorageEngine_src_test_fram
	src/test_fram/test_fram.cpp
	src/test_fram/test_main.cpp
	src/test_fram/UnitTest.cpp
)

# src -> thread_commu files
set (teststorageEngine_src_thread_commu
	src/thread_commu/test_multthread_communicate.cpp
	src/thread_commu/TestThreadCommon.cpp
	src/thread_commu/TestThreadCommon.h
	src/thread_commu/thread_atexitTest.cpp
	src/thread_commu/thread_commu.cpp
	src/thread_commu/thread_commu_hs.cpp
)

# src -> transaction files
set (teststorageEngine_src_transaction
	src/transaction/test_index_transaction_search.cpp
	src/transaction/test_subtrans.cpp
	src/transaction/test_trans_toast_index.cpp
	src/transaction/test_transaction_delete.cpp
	src/transaction/test_transaction_insert.cpp
	src/transaction/test_transaction_update.cpp
	src/transaction/test_transactionWithIndex.cpp
	src/transaction/test_transWithToast.cpp
	src/transaction/test_isolation_level.cpp
	src/transaction/test_trans_prepare.cpp
)

set (teststorageEngine_src_database
	src/database/test_db.cpp
)

# src -> cluster_simple files
set (teststorageEngine_src_cluster_simple
	src/cluster/cluster_simple.cpp
	src/cluster/cluster_thread.cpp
)

# src -> vacuum files
set (teststorageEngine_src_vacuum
	src/vacuum/test_vacuum.cpp
)

# src -> transaction -> teamcity files
set (teststorageEngine_src_transaction_teamcity
)

# src -> utils files
set (teststorageEngine_src_utils
	src/utils/toast_test_utils.cpp
	src/utils/utils.cpp
)

# the source groups
source_group(""                           FILES ${teststorageEngine})
source_group("include"                    FILES ${teststorageEngine_include})
source_group("include\\bgwriter"          FILES ${teststorageEngine_include_bgwriter})
source_group("include\\deadlock"          FILES ${teststorageEngine_include_deadlock})
source_group("include\\fatal_exception"   FILES ${teststorageEngine_include_fatal_exception})
source_group("include\\heap"              FILES ${teststorageEngine_include_heap})
source_group("include\\heap_sort_store"   FILES ${teststorageEngine_include_heap_sort_store})
source_group("include\\index"             FILES ${teststorageEngine_include_index})
source_group("include\\large_object"      FILES ${teststorageEngine_include_large_object})
source_group("include\\meta"              FILES ${teststorageEngine_include_meta})
source_group("include\\tablespace"        FILES ${teststorageEngine_include_tablespace})
source_group("include\\thread_commu"      FILES ${teststorageEngine_include_thread_commu})
source_group("include\\transaction"       FILES ${teststorageEngine_include_transaction})
source_group("include\\database"	      FILES ${teststorageEngine_include_database})
source_group("include\\cluster"           FILES ${teststorageEngine_include_cluster_simple})
source_group("include\\vacuum"            FILES ${teststorageEngine_include_vacuum})
source_group("include\\utils"             FILES ${teststorageEngine_include_utils})
source_group("src"                        FILES ${teststorageEngine_src})
source_group("src\\bgwriter"              FILES ${teststorageEngine_src_bgwriter})
source_group("src\\deadlock"              FILES ${teststorageEngine_src_deadlock})
source_group("src\\fatal_exception"       FILES ${teststorageEngine_src_fatal_exception})
source_group("src\\heap"                  FILES ${teststorageEngine_src_heap})
source_group("src\\heap_sort_store"       FILES ${teststorageEngine_src_heap_sort_store})
source_group("src\\index"                 FILES ${teststorageEngine_src_index})
source_group("src\\large_object"          FILES ${teststorageEngine_src_large_object})
source_group("src\\meta"                  FILES ${teststorageEngine_src_meta})
source_group("src\\tablespace"            FILES ${teststorageEngine_src_tablespace})
source_group("src\\test_fram"             FILES ${teststorageEngine_src_test_fram})
source_group("src\\thread_commu"          FILES ${teststorageEngine_src_thread_commu})
source_group("src\\transaction"           FILES ${teststorageEngine_src_transaction})
source_group("src\\database"              FILES ${teststorageEngine_src_database})
source_group("src\\cluster"		  		  FILES ${teststorageEngine_src_cluster_simple})
source_group("src\\vacuum"		  		  FILES ${teststorageEngine_src_vacuum})
source_group("src\\transaction\\teamcity" FILES ${teststorageEngine_src_transaction_teamcity})
source_group("src\\utils"                 FILES ${teststorageEngine_src_utils})

# append all files to the teststorageEngine
set (teststorageEngine
	${teststorageEngine}
	${teststorageEngine_include}
	${teststorageEngine_include_bgwriter}
	${teststorageEngine_include_deadlock}
	${teststorageEngine_include_fatal_exception}
	${teststorageEngine_include_heap}
	${teststorageEngine_include_heap_sort_store}
	${teststorageEngine_include_index}
	${teststorageEngine_include_large_object}
	${teststorageEngine_include_meta}
	${teststorageEngine_include_tablespace}
	${teststorageEngine_include_thread_commu}
	${teststorageEngine_include_transaction}
	${teststorageEngine_include_database}
	${teststorageEngine_include_cluster_simple}
	${teststorageEngine_include_vacuum}
	${teststorageEngine_include_utils}
	${teststorageEngine_src}
	${teststorageEngine_src_bgwriter}
	${teststorageEngine_src_deadlock}
	${teststorageEngine_src_fatal_exception}
	${teststorageEngine_src_heap}
	${teststorageEngine_src_heap_sort_store}
	${teststorageEngine_src_index}
	${teststorageEngine_src_large_object}
	${teststorageEngine_src_meta}
	${teststorageEngine_src_tablespace}
	${teststorageEngine_src_test_fram}
	${teststorageEngine_src_thread_commu}
	${teststorageEngine_src_transaction}
	${teststorageEngine_src_database}
	${teststorageEngine_src_cluster_simple}
	${teststorageEngine_src_vacuum}
	${teststorageEngine_src_transaction_teamcity}
	${teststorageEngine_src_utils}
)

if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
endif(MSVC)

add_executable(testStorageEngineLIB ${teststorageEngine})
include_directories(../src/include 
    ../src/backend 
    ../../../include/StorageEngine
    include
    src)
add_definitions(
    -DEXEC_BACKEND
    -DFOUNDER_XDB_SE
    -DUSE_ASSERT_CHECKING)
if (MSVC)
    include_directories(
      ../src/include/port/win32 
      ../src/include/port/win32_msvc 
      ../pthread
    )
    add_definitions(-D__WIN32__ 
    -D__WINDOWS__ 
    -DWIN32_STACK_RLIMIT=4194304 
   ) 
else (MSVC)
   add_definitions(-DEMORY_CONTEXT_CHECKIN
     -D_GNU_SOURCE)
endif (MSVC)
add_dependencies(testStorageEngineLIB StorageEngineDll)
target_link_libraries(testStorageEngineLIB StorageEngineDll ${BOOST_UNIT_TEST})
install(TARGETS testStorageEngineLIB
	RUNTIME DESTINATION server/bin
	COMPONENT test)
