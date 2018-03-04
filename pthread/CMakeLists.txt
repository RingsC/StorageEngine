# top-level files
set (pthread
)

# include files
set (pthread_include
	../../../include/pthread/pthread.h
	../../../include/pthread/pthread_init.h
)

# source files
set (pthread_source
	dllmain.cpp
	pthread.cpp
	pthread_init.cpp
)

# resource files
set (pthread_resource
)

# the source groups
source_group(""     FILES ${pthread})
source_group("include"  FILES ${pthread_include})
source_group("source"  FILES ${pthread_source})
source_group("resource" FILES ${pthread_resource})

# append all files to the pthread
set (pthread
	${pthread}
	${pthread_include}
	${pthread_source}
	${pthread_resource}
)

add_library(pthread SHARED ${pthread})
include_directories(../src/include 
    ../src/include/port/win32 
    ../src/include/port/win32_msvc 
    ../src/backend)
add_definitions(-DPTHREAD_IMPL -DPTHREAD_EXPORTS)
install(TARGETS pthread
	RUNTIME DESTINATION server/bin
	COMPONENT core)