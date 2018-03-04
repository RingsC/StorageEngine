# top-level files
set (bootstrap
	main.cpp
)

# the source groups
source_group("" FILES ${bootstrap})

# append all files to the bootstrap
set (bootstrap
	${bootstrap}
)

add_executable(bootstrap_storage ${bootstrap})
include_directories(../testStorageEngineDll/include ../../../include/StorageEngine)
add_dependencies(bootstrap_storage StorageEngineDll)
target_link_libraries(bootstrap_storage StorageEngineDll)
install(TARGETS bootstrap_storage
	RUNTIME DESTINATION server/bin
	COMPONENT test)