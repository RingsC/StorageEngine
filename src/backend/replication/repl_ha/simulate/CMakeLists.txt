#################################################################################
# simulate
#################################################################################
set (simulate
)

# include -> utils files
set (simulate_include
	include/ha_in_pg.h
	../ha_debug.h
	../ha_env.h
	../ha_net.h
	../ha_head.h
)

# src files
set (simulate_src
	src/ha_in_pg.cpp
	src/ha_simulate_env.cpp
	../ha_election.cpp
	../ha_env.cpp
	../ha_net.cpp
	../ha_select.cpp
)

# the source groups
source_group(""                       FILES ${simulate})
source_group("include"                FILES ${simulate_include})
source_group("src"                    FILES ${simulate_src})


set (simulate
	${simulate_include}
	${simulate_src}
)

if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
endif(MSVC)

add_definitions(-DHA_SIMULATE_ENV)

add_executable(simulate_env ${simulate})

INCLUDE_DIRECTORIES(..)

IF(DEFINED WIN32)
	target_link_libraries(simulate_env Ws2_32)
	INCLUDE_DIRECTORIES(../../../../../pthread)
ENDIF()
target_link_libraries(simulate_env pthread)

INSTALL(TARGETS simulate_env
	RUNTIME DESTINATION server/bin
	COMPONENT test)
