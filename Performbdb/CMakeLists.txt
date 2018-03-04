# top-level files
set (Performbdb
)

# include files
set (Performbdb_include
	testBDB-heapcreate.h
	testBDB-heapdelete.h
	testBDB-heapinsert.h
	testBDB-heapscan.h
	testBDB-heapupdate.h
	testBDB.h
)

# include -> utils files
set (Performbdb_include_utils
	bdb_utils.h
	include/db.h
	include/db_cxx.h
)

# src files
set (Performbdb_src
	testBDB-heapcreate.cpp
	testBDB-heapdelete.cpp
	testBDB-heapinsert.cpp
	testBDB-heapscan.cpp
	testBDB-heapupdate.cpp
	testBDB.cpp
)

# src -> utils files
set (Performbdb_src_utils
	bdb_utils.cpp
)

# the source groups
source_group(""               FILES ${Performbdb})
source_group("include"        FILES ${Performbdb_include})
source_group("include\\utils" FILES ${Performbdb_include_utils})
source_group("src"            FILES ${Performbdb_src})
source_group("src\\utils"     FILES ${Performbdb_src_utils})

# append all files to the Performbdb
set (Performbdb
	${Performbdb}
	${Performbdb_include}
	${Performbdb_include_utils}
	${Performbdb_src}
	${Performbdb_src_utils}
)

if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
endif(MSVC)

link_directories(.)
add_executable(Performbdb ${Performbdb})
include_directories(Performbdb ../testStorageEngineDll/include include)

if(CMAKE_SIZEOF_VOID_P EQUAL 8 )
    target_link_libraries(Performbdb libdb48_x64)
else(CMAKE_SIZEOF_VOID_P EQUAL 8 )
    target_link_libraries(Performbdb libdb48)
endif (CMAKE_SIZEOF_VOID_P EQUAL 8 )
install(TARGETS Performbdb
	RUNTIME DESTINATION server/bin
	COMPONENT test)