#Top-level files
#test
set(testThreadTransaction_include
)

source_group("include"   FILES ${testThreadTransaction_include})

#test
set(testThreadTransaction_include_deadlock
include/DeadLock/deadlock.h)
source_group("include\\DeadLock" FILES ${testThreadTransaction_include_deadlock})

#test
set(testThreadTransaction_include_large_object
	include/large_object/large_object.h)
source_group("include\\large_object" FILES ${testThreadTransaction_include_large_object})

#test
set(testThreadTransaction_include_heap
include/heap/createheap.h
include/heap/dropheap.h
include/heap/test_dump_load.h
include/heap/test_heap_close.h
include/heap/test_heap_create.h
include/heap/test_heap_delete.h
include/heap/test_heap_drop.h
include/heap/test_heap_insert.h
include/heap/test_heap_open.h
include/heap/test_heap_select.h
include/heap/test_heap_toast.h
include/heap/test_heap_update.h
include/heap/test_heap_store.h
include/DeadLock/deadlock.h
)

source_group("include\\heap"   FILES ${testThreadTransaction_include_heap})

source_group("include\\heap\\kk"   FILES ${testThreadTransaction_include_heap_kk})

#test
set(testThreadTransaction_include_index
include/index/createindex.h
include/index/dropindex.h
include/index/testbitmapscan.h
)

source_group("include\\index"   FILES ${testThreadTransaction_include_index})

set(testThreadTransaction_include_utils
include/test_utils/test_utils.h
)

source_group("include\\test_utils" FILES ${testThreadTransaction_include_utils})
#test
set(testThreadTransaction_include_TestFrame
include/TestFrame/TestFrame.h
include/TestFrame/TestTask.h
include/TestFrame/TestThreadPool.h
include/TestFrame/test_fram.h
)

source_group("include\\TestFrame"   FILES ${testThreadTransaction_include_TestFrame})

#test
set(testThreadTransaction_include_TestFrameCase
include/TestFrameCase/TestFrameCaseDll.h
)

source_group("include\\TestFrameCase"   FILES ${testThreadTransaction_include_TestFrameCase})

#test
set(testThreadTransaction_include_TestFrameCommon
include/TestFrameCommon/TestFrameCommon.h
)

source_group("include\\TestFrameCommon"   FILES ${testThreadTransaction_include_TestFrameCommon})

#test
set(testThreadTransaction_src
)

source_group("src"   FILES ${testThreadTransaction_src})

#test
set(testThreadTransaction_src_DeadLock
  src/DeadLock/deadlock.cpp)

source_group("src\\DeadLock" FILES ${testThreadTransaction_src_DeadLock})

#test
set(testThreadTransaction_src_large_object
	src/large_object/large_object.cpp)

source_group("src\\large_object" FILES ${testThreadTransaction_src_large_object})

#test
set(testThreadTransaction_src_heap
src/heap/createheap.cpp
src/heap/dropheap.cpp
src/heap/test_dump_load.cpp
src/heap/test_heap_close.cpp
src/heap/test_heap_create.cpp
src/heap/test_heap_delete.cpp
src/heap/test_heap_drop.cpp
src/heap/test_heap_insert.cpp
src/heap/test_heap_open.cpp
src/heap/test_heap_select.cpp
src/heap/test_heap_toast.cpp
src/heap/test_heap_update.cpp
src/heap/test_heap_store.cpp
)

source_group("src\\heap"   FILES ${testThreadTransaction_src_heap})

#test
set(testThreadTransaction_src_index
src/index/createindex.cpp
src/index/dropindex.cpp
src/index/testbitmapscan.cpp
)

source_group("src\\index"   FILES ${testThreadTransaction_src_index})

#test
set(testThreadTransaction_src_TestFrame
src/TestFrame/TestFrame.cpp
src/TestFrame/TestTask.cpp
src/TestFrame/TestThreadPool.cpp
)

source_group("src\\TestFrame"   FILES ${testThreadTransaction_src_TestFrame})

#test
set(testThreadTransaction_src_TestFrameCase
src/TestFrameCase/TestFrameCaseDll.cpp
../testStorageEngineDll/src/utils/ConfigParse.cpp
)

source_group("src\\TestFrameCase"   FILES ${testThreadTransaction_src_TestFrameCase})

#test
set(testThreadTransaction_src_TestFrameCommon
src/TestFrameCommon/TestFrameCommon.cpp
)

source_group("src\\TestFrameCommon"   FILES ${testThreadTransaction_src_TestFrameCommon})

#test
set(testThreadTransaction_src_TestFrameMain
src/TestFrameMain/TestFrameMainDll.cpp
../teststorageEngine/src/test_fram/test_fram.cpp
src/TestFrameMain/UnitTest.cpp
)

source_group("src\\TestFrameMain"   FILES ${testThreadTransaction_src_TestFrameMain})

set(testThreadTransaction_src_utils
src/test_utils/test_utils.cpp
)

source_group("src\\test_utils" FILES ${testThreadTransaction_src_utils})

set(testThreadTransaction_SRCLISTS
src/heap/createheap.cpp
src/heap/dropheap.cpp
src/index/createindex.cpp
src/index/dropindex.cpp
src/index/testbitmapscan.cpp
src/TestFrame/TestFrame.cpp
src/TestFrame/TestTask.cpp
src/TestFrame/TestThreadPool.cpp
src/TestFrameCase/TestFrameCaseDll.cpp
src/TestFrameCommon/TestFrameCommon.cpp
src/TestFrameMain/TestFrameMainDll.cpp
../teststorageEngine/src/test_fram/test_fram.cpp
src/TestFrameMain/UnitTest.cpp
../testStorageEngineDll/src/utils/ConfigParse.cpp
include/test_utils/test_utils.h
src/test_utils/test_utils.cpp
include/heap/createheap.h
include/heap/dropheap.h
include/index/createindex.h
include/index/dropindex.h
include/index/testbitmapscan.h
include/TestFrame/TestFrame.h
include/TestFrame/TestTask.h
include/TestFrame/TestThreadPool.h
include/TestFrame/test_fram.h
include/TestFrameCase/TestFrameCaseDll.h
include/TestFrameCommon/TestFrameCommon.h
include/heap/test_dump_load.h
include/heap/test_heap_close.h
include/heap/test_heap_create.h
include/heap/test_heap_delete.h
include/heap/test_heap_drop.h
include/heap/test_heap_insert.h
include/heap/test_heap_open.h
include/heap/test_heap_select.h
include/heap/test_heap_toast.h
include/heap/test_heap_update.h
include/heap/test_heap_store.h
src/heap/test_dump_load.cpp
src/heap/test_heap_close.cpp
src/heap/test_heap_create.cpp
src/heap/test_heap_delete.cpp
src/heap/test_heap_drop.cpp
src/heap/test_heap_insert.cpp
src/heap/test_heap_open.cpp
src/heap/test_heap_select.cpp
src/heap/test_heap_toast.cpp
src/heap/test_heap_update.cpp
src/heap/test_heap_store.cpp
include/large_object/large_object.h
src/large_object/large_object.cpp
include/DeadLock/deadlock.h
src/DeadLock/deadlock.cpp
)


if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
endif(MSVC)

add_executable(testThreadTransaction ${testThreadTransaction_SRCLISTS})

add_dependencies(testThreadTransaction
StorageEngineDll

)

target_link_libraries(testThreadTransaction
StorageEngineDll ${BOOST_UNIT_TEST}
)

include_directories(
include/heap
include/index
include/TestFrame
include/TestFrameCase
include/TestFrameCommon
include
../testStorageEngineDll/include
../src/include
)
add_definitions(
-DTESTFRAME
-DFOUNDER_XDB_SE
)
if (MSVC)
    include_directories(
      ../src/include/port/win32 
      ../src/include/port/win32_msvc 
    )
    add_definitions(-D__WIN32__ 
    -D__WINDOWS__ 
    -DWIN32_STACK_RLIMIT=4194304 
   ) 
else (MSVC)
   add_definitions(-DEMORY_CONTEXT_CHECKIN
     -D_GNU_SOURCE)
endif (MSVC)
install(TARGETS testThreadTransaction
RUNTIME DESTINATION server/bin
		COMPONENT test)
