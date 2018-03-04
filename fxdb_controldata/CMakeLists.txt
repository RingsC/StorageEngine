# top-level files
set (fxdb_controldata
	pg_controldata.c
)

# the source groups
source_group("" FILES ${fxdb_controldata})

# append all files to the fxdb_controldata
set (fxdb_controldata
	${fxdb_controldata}
)

add_executable(fxdb_controldata ${fxdb_controldata})
include_directories(../src/include 
    ../src/backend)
add_definitions(
    -DEXEC_BACKEND 
    -DBUILDING_DLL 
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
     -DCLOBBER_FREED_MEMORY
     -D_GNU_SOURCE)
endif (MSVC)
set_source_files_properties(${fxdb_controldata} PROPERTIES LANGUAGE CXX )
add_dependencies(fxdb_controldata StorageEngineDll)
target_link_libraries(fxdb_controldata StorageEngineDll)
install(TARGETS fxdb_controldata
	RUNTIME DESTINATION server/bin
	COMPONENT core)