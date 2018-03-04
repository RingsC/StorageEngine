# top-level files
set (Performpg
)

# include files
set (Performpg_include
	testPG-heapcreate.h
	testPG-heapdelete.h
	testPG-heapinsert.h
	testPG-heapupdate.h
	testPG.h
)

# include -> utils files
set (Performpg_include_utils
	pg_utils_dll.h
)

# src files
set (Performpg_src
	testPG-heapcreate.cpp
	testPG-heapdelete.cpp
	testPG-heapinsert.cpp
	testPG-heapupdate.cpp
	testPG.cpp
)

# src -> utils files
set (Performpg_src_utils
	pg_utils_dll.cpp
)

# the source groups
source_group(""               FILES ${Performpg})
source_group("include"        FILES ${Performpg_include})
source_group("include\\utils" FILES ${Performpg_include_utils})
source_group("src"            FILES ${Performpg_src})
source_group("src\\utils"     FILES ${Performpg_src_utils})

# append all files to the Performpg
set (Performpg
	${Performpg}
	${Performpg_include}
	${Performpg_include_utils}
	${Performpg_src}
	${Performpg_src_utils}
)

if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
endif(MSVC)

add_executable(Performpg ${Performpg})
include_directories(../testStorageEngineDll/include)
add_dependencies(Performpg StorageEngineDll)
target_link_libraries(Performpg StorageEngineDll)
install(TARGETS Performpg
	RUNTIME DESTINATION server/bin
	COMPONENT test)