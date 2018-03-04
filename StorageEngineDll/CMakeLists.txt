# top-level files
set (StorageEngineDll
)

# src files
set (StorageEngineDll_src
)

# src -> include files
set (StorageEngineDll_src_include
)

# src -> include -> interface files
set (StorageEngineDll_src_include_interface
	../src/include/interface/PGDump.h
	../src/include/interface/PGEntryIDBitmap.h
	../src/include/interface/PGEntrySet.h
	../src/include/interface/PGFDBitmapSet.h
	../src/include/interface/PGIndexEntrySet.h
	../src/include/interface/PGLargeObject.h
	../src/include/interface/PGLargeObjectStore.h
	../src/include/interface/PGSortEntrySet.h
	../src/include/interface/PGStorageEngine.h
	../src/include/interface/PGTempEntrySet.h
	../src/include/interface/PGTransaction.h
	../src/include/interface/PGXdbLock.h
	../src/include/interface/PgXdbSequence.h
	../src/include/interface/utils.h
	../src/interface/PgMemcontext.h
	../src/include/interface/PGGraphStorage.h
	../src/include/interface/GraphStorage.h
	../src/include/interface/PGDynaHash.h
)

# src -> interface files
set (StorageEngineDll_src_interface
	../src/interface/MemoryPool.cpp
	../src/interface/PGDump.cpp
	../src/interface/PGEntryIDBitmap.cpp
	../src/interface/PGEntrySet.cpp
	../src/interface/PGFDBitmapSet.cpp
	../src/interface/PGIndexEntrySet.cpp
	../src/interface/PGLargeObject.cpp
	../src/interface/PGLargeObjectStore.cpp
	../src/interface/PgMemcontext.cpp
	../src/interface/PGSEException.cpp
	../src/interface/PGSortEntrySet.cpp
	../src/interface/PGStorageEngine.cpp
	../src/interface/PGTempEntrySet.cpp
	../src/interface/pgtransaction.cpp
	../src/interface/PGXdbLock.cpp
	../src/interface/PgXdbSequence.cpp
	../src/interface/StorageEngine.cpp
	../src/interface/utils.cpp
	../src/interface/XdbLock.cpp
	../src/interface/PGGraphStorage.cpp
	../src/interface/PGDynaHash.cpp
)

# src -> public files
set (StorageEngineDll_src_public
	../../../include/StorageEngine/DataItem.h
	../../../include/StorageEngine/EntryIDBitmap.h
	../../../include/StorageEngine/EntrySet.h
	../../../include/StorageEngine/FDBitmapSet.h
	../../../include/StorageEngine/LargeObject.h
	../../../include/StorageEngine/Macros.h
	../../../include/StorageEngine/MemoryContext.h
	../../../include/StorageEngine/PGSETypes.h
	../../../include/StorageEngine/StorageEngine.h
	../../../include/StorageEngine/StorageEngineException.h
	../../../include/StorageEngine/Transaction.h
	../../../include/StorageEngine/WaitForGraph.h
	../../../include/StorageEngine/XdbLock.h
	../../../include/StorageEngine/XdbSequence.h
	../../../include/StorageEngine/DynaHash.h
)

# the source groups
source_group(""                        FILES ${StorageEngineDll})
source_group("src"                     FILES ${StorageEngineDll_src})
source_group("src\\include"            FILES ${StorageEngineDll_src_include})
source_group("src\\include\\interface" FILES ${StorageEngineDll_src_include_interface})
source_group("src\\interface"          FILES ${StorageEngineDll_src_interface})
source_group("src\\public"             FILES ${StorageEngineDll_src_public})

# append all files to the StorageEngineDll
set (StorageEngineDll
	${StorageEngineDll}
	${StorageEngineDll_src}
	${StorageEngineDll_src_include}
	${StorageEngineDll_src_include_interface}
	${StorageEngineDll_src_interface}
	${StorageEngineDll_src_public}
)

add_library(StorageEngineDll SHARED ${StorageEngineDll})
add_definitions(
    -DEXEC_BACKEND 
    -DFOUNDER_XDB_SE 
	)
include_directories(../src/include 
    ../src/backend 
    )
set (StorageEngineDll_DEP storageEngineLib)
if (MSVC)
    set_target_properties( StorageEngineDll PROPERTIES COMPILE_FLAGS "/wd4127 /wd4611" )
    include_directories(
      ../src/include/port/win32 
      ../src/include/port/win32_msvc 
      ../pthread
    )
    add_definitions(-D__WIN32__ 
    -D__WINDOWS__ 
    -DWIN32_STACK_RLIMIT=4194304 
    -DBUILDING_DLL 
    -DSTORAGEENGINE_DLL
    -DDLLTEST
   )
   set (StorageEngineDll_DEP ws2_32 ${StorageEngineDll_DEP})
else (MSVC)
   add_definitions(-D_GNU_SOURCE)
endif (MSVC)
add_dependencies(StorageEngineDll storageEngineLib)
target_link_libraries(StorageEngineDll ${StorageEngineDll_DEP})
install(TARGETS StorageEngineDll
	RUNTIME DESTINATION server/bin
	LIBRARY DESTINATION server/bin
	COMPONENT core)
