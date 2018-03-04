# top-level files
set (walsender
)

# include files
set (walsender_include
	../serverconfig/attr_setter.h
	../serverconfig/replication_utils.h
	walsender.h
)

# source files
set (walsender_source
	../serverconfig/ConfigParse.cpp
	../serverconfig/replication_utils.cpp
	UnitTest.cpp
	walsender.cpp
)

# the source groups
source_group(""    FILES ${walsender})
source_group("include" FILES ${walsender_include})
source_group("source" FILES ${walsender_source})

# append all files to the walsender
set (walsender
	${walsender}
	${walsender_include}
	${walsender_source}
)

if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
endif(MSVC)

add_executable(walsender ${walsender})
include_directories(include 
    ../src/backend 
    ../serverconfig
    ../testStorageEngineDll/include
    ../teststorageEngine/include
    ../src/include)
add_definitions(-DFOUNDER_XDB_SE)
if (MSVC)
include_directories(../src/include/port/win32 
    ../src/include/port/win32_msvc
	../pthread)
else (MSVC)
add_definitions(-D_GNU_SOURCE)
endif (MSVC)
add_dependencies(walsender StorageEngineDll)
target_link_libraries(walsender StorageEngineDll ${BOOST_UNIT_TEST})
install(TARGETS walsender
	RUNTIME DESTINATION server/bin
	COMPONENT test)