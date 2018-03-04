set(TEST_TWOPHASE_INCLUDE
    include/attr_setter.h
    include/test_twophase_heap_operations.h
    include/test_twophase_frame.h
    include/test_twophase_utils.h
    include/test_twophase_trans_iso_level.h
	)

set(TEST_TWOPHASE_SRC
	src/test_twophase_heap_operations.cpp
    src/test_twophase_main.cpp
    src/test_twophase_frame.cpp
    src/test_twophase_utils.cpp
    src/ConfigParse.cpp
    src/twophase_unit_test.cpp
    src/test_twophase_trans_iso_level.cpp
	)

set(TEST_TWOPHASE
    ${TEST_TWOPHASE_INCLUDE}
    ${TEST_TWOPHASE_SRC}
	)
source_group("include" FILES ${TEST_TWOPHASE_INCLUDE})
source_group("src" FILES ${TEST_TWOPHASE_SRC})

if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
endif(MSVC)

add_executable(testTwoPhaseCommit ${TEST_TWOPHASE})

add_definitions(-DTESTFRAME)

include_directories(
	../../../include/StorageEngine
    ../src/include
    ../src/backend
    include
    src
	)

add_dependencies(testTwoPhaseCommit StorageEngineDll)


if (MSVC)
else(MSVC)
    message(STATUS "system is linux````")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 )
        set(ARCH_NAME x64)
    else(CMAKE_SIZEOF_VOID_P EQUAL 8 )
        set(ARCH_NAME x86)
    endif (CMAKE_SIZEOF_VOID_P EQUAL 8 )

    INCLUDE_DIRECTORIES(../../../../3rd_party/lib/Linux/${ARCH_NAME}/boost/)

    target_link_libraries(testTwoPhaseCommit libboost_serialization.so)

endif (MSVC)


target_link_libraries(testTwoPhaseCommit StorageEngineDll ${BOOST_UNIT_TEST})

install(TARGETS testTwoPhaseCommit
	RUNTIME DESTINATION server/bin
	LIBRARY DESTINATION server/bin
	ARCHIVE DESTINATION api/c++/lib
	)
