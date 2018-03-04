# top-level files
set (testStorageEngineDll
)

# include files
set (testStorageEngineDll_include
)

# include -> backup_recovery files
set (testStorageEngineDll_include_backup_recovery
	include/backup_recovery/BRTest.h
	include/backup_recovery/Recovery2Recently.h
	include/backup_recovery/timelines.h
)

# include -> bitmap files
set (testStorageEngineDll_include_bitmap
	include/bitmap/test_bitmap_dll.h
)

# include -> DeadLock files
set (testStorageEngineDll_include_deadlock
	include/DeadLock/deadlocktest.h
)

# include -> heap files
set (testStorageEngineDll_include_heap
	include/heap/test_dump_load.h
	include/heap/test_heap_close_dll.h
	include/heap/test_heap_create_dll.h
	include/heap/test_heap_delete_dll.h
	include/heap/test_heap_drop_dll.h
	include/heap/test_heap_insert_dll.h
	include/heap/test_heap_open_dll.h
	include/heap/test_heap_select_dll.h
	include/heap/test_heap_update_dll.h
	include/heap/test_toast_dll.h
	include/heap/test_heap_multi_insert.h
)

# include -> heap_sort_store files
set (testStorageEngineDll_include_heap_sort_store
	include/heap_sort_store/heap_store_dll.h
)

# include -> index files
set (testStorageEngineDll_include_index
	include/index/test_index_cmp_dll.h
	include/index/test_index_dll.h
	include/index/test_index_insert_dll.h
	include/index/test_index_transation_dll.h
	include/index/test_index_update_dll.h
	include/index/test_toast_index_dll.h
	include/index/test_split_funcs_dll.h
	include/index/test_lsm_index_dll.h
)

# include -> large_object files
set (testStorageEngineDll_include_large_object
	include/large_object/large_object_test.h
)

# include -> lock files
set (testStorageEngineDll_include_lock
	include/lock/test_lock_dll.h
)

# include -> memcontext files
set (testStorageEngineDll_include_memcontext
	include/memcontext/test_memcontext.h
)

# include -> sequence files
set (testStorageEngineDll_include_sequence
	include/sequence/utils.h
)

# include -> tablespace files
set (testStorageEngineDll_include_tablespace
	include/tablespace/test_tablespace.h
)

# include -> transaction files
set (testStorageEngineDll_include_transaction
	include/transaction/test_subtrans_dll.h
	include/transaction/test_trans_multi_index_update.h
	include/transaction/test_trans_toast_index_dll.h
	include/transaction/test_transaction_delete_dll.h
	include/transaction/test_transaction_index_create_dll.h
	include/transaction/test_transaction_index_dll.h
	include/transaction/test_transaction_insert_dll.h
	include/transaction/test_transaction_isolation_dll.h
	include/transaction/test_transaction_toast_dll.h
	include/transaction/test_transaction_update_dll.h
	include/transaction/test_transactionid_min_and_max.h
)

# include -> utils files
set (testStorageEngineDll_include_utils
	include/utils/attr_setter.h
	include/utils/utils_dll.h
)

#include -> cluster files
set (testStorageEngineDll_include_cluster
	include/cluster/test_cluster_simple.hpp
)


# src files
set (testStorageEngineDll_src
)

# src -> backup_recovery files
set (testStorageEngineDll_src_backup_recovery
	src/backup_recovery/BRTest.cpp
	src/backup_recovery/Recovery2Recently.cpp
	src/backup_recovery/Recovery2Target.cpp
	src/backup_recovery/timelines.cpp
)

# src -> bitmap files
set (testStorageEngineDll_src_bitmap
	src/bitmap/test_bitmap_dll.cpp
)

# src -> Deadlock files
set (testStorageEngineDll_src_deadlock
	src/deadlock/deadlocktest.cpp
)

# src -> heap files
set (testStorageEngineDll_src_heap
	src/heap/test_dump_load.cpp
	src/heap/test_heap_close_dll.cpp
	src/heap/test_heap_create_dll.cpp
	src/heap/test_heap_delete_dll.cpp
	src/heap/test_heap_drop_dll.cpp
	src/heap/test_heap_insert_dll.cpp
	src/heap/test_heap_open_dll.cpp
	src/heap/test_heap_select_dll.cpp
	src/heap/test_heap_update_dll.cpp
	src/heap/test_toast_dll.cpp
	src/heap/test_heap_multi_insert.cpp
)

# src -> heap_sort_store files
set (testStorageEngineDll_src_heap_sort_store
	src/heap_sort_store/heap_store_dll.cpp
)

# src -> index files
set (testStorageEngineDll_src_index
	src/index/test_index_cmp_dll.cpp
	src/index/test_index_dll.cpp
	src/index/test_index_insert_dll.cpp
	src/index/test_index_transation_dll.cpp
	src/index/test_index_update_dll.cpp
	src/index/test_toast_index_dll.cpp
	src/index/test_split_funcs_dll.cpp
	src/index/test_lsm_index_dll.cpp
)

# src -> large_object files
set (testStorageEngineDll_src_large_object
	src/large_object/large_object_test.cpp
)

# src -> lock files
set (testStorageEngineDll_src_lock
	src/lock/test_lock_dll.cpp
)

# src -> memcontext files
set (testStorageEngineDll_src_memcontext
	src/memcontext/test_memorycontext.cpp
)

# src -> sequence files
set (testStorageEngineDll_src_sequence
	src/sequence/test_sequence.cpp
	src/sequence/utils.cpp
)

# src -> tablespace files
set (testStorageEngineDll_src_tablespace
	src/tablespace/test_tablespace.cpp
)

# src -> test_main files
set (testStorageEngineDll_src_test_main
	../teststorageEngine/src/test_fram/test_fram.cpp
	../teststorageEngine/src/test_fram/UnitTest.cpp
	src/test_main/test_main_dll.cpp
)

# src -> transaction files
set (testStorageEngineDll_src_transaction
	src/transaction/test_subtrans_dll.cpp
	src/transaction/test_trans_multi_index_update.cpp
	src/transaction/test_trans_toast_index_dll.cpp
	src/transaction/test_transaction_delete_dll.cpp
	src/transaction/test_transaction_index_create_dll.cpp
	src/transaction/test_transaction_index_dll.cpp
	src/transaction/test_transaction_insert_dll.cpp
	src/transaction/test_transaction_isolation_dll.cpp
	src/transaction/test_transaction_toast_dll.cpp
	src/transaction/test_transaction_update_dll.cpp
	src/transaction/test_transactionid_min_and_max.cpp
)

# src -> utils files
set (testStorageEngineDll_src_utils
	src/utils/ConfigParse.cpp
	src/utils/utils_dll.cpp
)

# src -> cluster files
set (testStorageEngineDll_src_cluster
	src/cluster/test_cluster_simple.cpp
)

# the source groups
source_group(""                         FILES ${testStorageEngineDll})
source_group("include"                  FILES ${testStorageEngineDll_include})
source_group("include\\backup_recovery" FILES ${testStorageEngineDll_include_backup_recovery})
source_group("include\\bitmap"          FILES ${testStorageEngineDll_include_bitmap})
source_group("include\\DeadLock"        FILES ${testStorageEngineDll_include_deadlock})
source_group("include\\heap"            FILES ${testStorageEngineDll_include_heap})
source_group("include\\heap_sort_store" FILES ${testStorageEngineDll_include_heap_sort_store})
source_group("include\\index"           FILES ${testStorageEngineDll_include_index})
source_group("include\\large_object"    FILES ${testStorageEngineDll_include_large_object})
source_group("include\\lock"            FILES ${testStorageEngineDll_include_lock})
source_group("include\\memcontext"      FILES ${testStorageEngineDll_include_memcontext})
source_group("include\\sequence"        FILES ${testStorageEngineDll_include_sequence})
source_group("include\\tablespace"      FILES ${testStorageEngineDll_include_tablespace})
source_group("include\\transaction"     FILES ${testStorageEngineDll_include_transaction})
source_group("include\\utils"           FILES ${testStorageEngineDll_include_utils})
source_group("include\\cluster"         FILES ${testStorageEngineDll_include_cluster})
source_group("src"                      FILES ${testStorageEngineDll_src})
source_group("src\\backup_recovery"     FILES ${testStorageEngineDll_src_backup_recovery})
source_group("src\\bitmap"              FILES ${testStorageEngineDll_src_bitmap})
source_group("src\\Deadlock"            FILES ${testStorageEngineDll_src_deadlock})
source_group("src\\heap"                FILES ${testStorageEngineDll_src_heap})
source_group("src\\heap_sort_store"     FILES ${testStorageEngineDll_src_heap_sort_store})
source_group("src\\index"               FILES ${testStorageEngineDll_src_index})
source_group("src\\large_object"        FILES ${testStorageEngineDll_src_large_object})
source_group("src\\lock"                FILES ${testStorageEngineDll_src_lock})
source_group("src\\memcontext"          FILES ${testStorageEngineDll_src_memcontext})
source_group("src\\sequence"            FILES ${testStorageEngineDll_src_sequence})
source_group("src\\tablespace"          FILES ${testStorageEngineDll_src_tablespace})
source_group("src\\test_main"           FILES ${testStorageEngineDll_src_test_main})
source_group("src\\transaction"         FILES ${testStorageEngineDll_src_transaction})
source_group("src\\utils"               FILES ${testStorageEngineDll_src_utils})
source_group("src\\cluster"             FILES ${testStorageEngineDll_src_cluster})

# append all files to the testStorageEngineDll
set (testStorageEngineDll
	${testStorageEngineDll}
	${testStorageEngineDll_include}
	${testStorageEngineDll_include_backup_recovery}
	${testStorageEngineDll_include_bitmap}
	${testStorageEngineDll_include_deadlock}
	${testStorageEngineDll_include_heap}
	${testStorageEngineDll_include_heap_sort_store}
	${testStorageEngineDll_include_index}
	${testStorageEngineDll_include_large_object}
	${testStorageEngineDll_include_lock}
	${testStorageEngineDll_include_memcontext}
	${testStorageEngineDll_include_sequence}
	${testStorageEngineDll_include_tablespace}
	${testStorageEngineDll_include_transaction}
	${testStorageEngineDll_include_utils}
	${testStorageEngineDll_include_cluster}
    ${testStorageEngineDll_include_TestFrame}
	${testStorageEngineDll_include_TestFrameCase}
	${testStorageEngineDll_include_TestFrameCommon}
	${testStorageEngineDll_src}
	${testStorageEngineDll_src_backup_recovery}
	${testStorageEngineDll_src_bitmap}
	${testStorageEngineDll_src_deadlock}
	${testStorageEngineDll_src_heap}
	${testStorageEngineDll_src_heap_sort_store}
	${testStorageEngineDll_src_index}
	${testStorageEngineDll_src_large_object}
	${testStorageEngineDll_src_lock}
	${testStorageEngineDll_src_memcontext}
	${testStorageEngineDll_src_sequence}
	${testStorageEngineDll_src_tablespace}
	${testStorageEngineDll_src_test_main}
	${testStorageEngineDll_src_transaction}
	${testStorageEngineDll_src_utils}
	${testStorageEngineDll_src_cluster}
)

if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
endif(MSVC)

add_executable(testStorageEngineDLL ${testStorageEngineDll})
add_definitions(-DTESTFRAME)
include_directories(include ../../../include/StorageEngine ../teststorageEngine/include ../teststorageEngine/src)
add_dependencies(testStorageEngineDLL StorageEngineDll)
target_link_libraries(testStorageEngineDLL StorageEngineDll ${BOOST_UNIT_TEST})
install(TARGETS testStorageEngineDLL
	RUNTIME DESTINATION server/bin
	COMPONENT test)
