# top-level files
set (baseBackup
	pg_basebackup.c
)

# the source groups
source_group("" FILES ${baseBackup})

# append all files to the baseBackup
set (baseBackup
	${baseBackup}
)

add_executable(baseBackup ${baseBackup})
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
set_source_files_properties(${baseBackup} PROPERTIES LANGUAGE CXX )
add_dependencies(baseBackup StorageEngineDll)
target_link_libraries(baseBackup StorageEngineDll)
install(TARGETS baseBackup
	RUNTIME DESTINATION server/bin
	COMPONENT core)