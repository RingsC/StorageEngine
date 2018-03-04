# top-level files
set (walreceiver
)

# include files
set (walreceiver_include
	../serverconfig/attr_setter.h
	../serverconfig/replication_utils.h
	walreceiver.h
)

# source files
set (walreceiver_source
	../serverconfig/ConfigParse.cpp
	../serverconfig/replication_utils.cpp
	UnitTest.cpp
	walreceiver.cpp
)

# the source groups
source_group(""    FILES ${walreceiver})
source_group("include" FILES ${walreceiver_include})
source_group("source" FILES ${walreceiver_source})

# append all files to the walreceiver
set (walreceiver
	${walreceiver}
	${walreceiver_include}
	${walreceiver_source}
)

if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
endif(MSVC)

add_executable(walreceiver ${walreceiver})
include_directories(include
    ../src/backend 
    ../serverconfig
    ../testStorageEngineDll/include
    ../teststorageEngine/include
    ../src/include)
add_definitions(-DFOUNDER_XDB_SE -D_GNU_SOURCE)
if (MSVC)
include_directories(../src/include/port/win32 
    ../src/include/port/win32_msvc
	../pthread)
else (MSVC)
add_definitions(-D_GNU_SOURCE)
endif (MSVC)
add_dependencies(walreceiver StorageEngineDll)
target_link_libraries(walreceiver StorageEngineDll ${BOOST_UNIT_TEST})
install(TARGETS walreceiver
	RUNTIME DESTINATION server/bin
	COMPONENT test)