
set(TEST_PERFORM_INCLUDE
	include/perform_heap.h
	include/perform_main.h
	include/perform_utils.h
	include/perform_unit_test.h
	include/perform_frame.h
	include/attr_setter.h
	include/perform_bulk_insert.h
	include/perform_sort_index.h
	include/perform_bitmap_scan.h
	)

set(TEST_PERFORM_SRC
	src/perform_heap.cpp
	src/perform_main.cpp
	src/perform_utils.cpp
	src/perform_unit_test.cpp
	src/perform_frame.cpp
	src/ConfigParse.cpp
	src/perform_bulk_insert.cpp
	src/perform_sort_index.cpp
	src/perform_bitmap_scan.cpp
	)

set(TEST_PERFORM 
    ${TEST_PERFORM_INCLUDE}
    ${TEST_PERFORM_SRC}
	)

source_group("include" FILES ${TEST_PERFORM_INCLUDE})
source_group("src" FILES ${TEST_PERFORM_SRC})


if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
endif(MSVC)

add_executable(testPerform ${TEST_PERFORM})

add_definitions(-DTESTFRAME)

include_directories(
	include
	../../../include/StorageEngine
	)

add_dependencies(testPerform StorageEngineDll)

target_link_libraries(testPerform StorageEngineDll ${BOOST_UNIT_TEST})

install(TARGETS testPerform
	RUNTIME DESTINATION server/bin
	COMPONENT test
	)