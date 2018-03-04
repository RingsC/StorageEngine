# top-level files
set (PgPerformanceCompBdb
)

# include files
set (PgPerformanceCompBdb_include
)

# include -> utils files
set (PgPerformanceCompBdb_include_utils
	include/db-linux.h
	include/db-windows.h
	include/db_cxx.h
	include/utils/pgbdbutils.h
	include/utils/performance_timer.h
)

# include -> pg files
set (PgPerformanceCompBdb_include_pg
	include/pg/pg_insert.h
)

# include -> bdb files
set (PgPerformanceCompBdb_include_bdb
	include/bdb/bdb_insert.h
)



# src files
set (PgPerformanceCompBdb_src
)

# src -> utils files
set (PgPerformanceCompBdb_src_utils
	src/utils/pgbdbutils.cpp
	src/utils/performance_timer.cpp
)

# src -> pg files
set (PgPerformanceCompBdb_src_pg
	src/pg/pg_insert.cpp
)

# src -> bdb files
set (PgPerformanceCompBdb_src_bdb
	src/bdb/bdb_insert.cpp
)

#src -> main file
set (PgPerformanceCompBdb_src_permance_main
	src/performance_main/performance_main.cpp
)

# the source groups
source_group(""                         FILES ${PgPerformanceCompBdb})
source_group("include"                  FILES ${PgPerformanceCompBdb_include})
source_group("include\\utils" FILES ${PgPerformanceCompBdb_include_utils})
source_group("include\\pg"          FILES ${PgPerformanceCompBdb_include_pg})
source_group("include\\bdb"        FILES ${PgPerformanceCompBdb_include_bdb})
source_group("src"                      FILES ${PgPerformanceCompBdb_src})
source_group("src\\utils"     FILES ${PgPerformanceCompBdb_src_utils})
source_group("src\\pg"              FILES ${PgPerformanceCompBdb_src_pg})
source_group("src\\bdb"            FILES ${PgPerformanceCompBdb_src_bdb})
source_group("src\\permance_main"            FILES ${PgPerformanceCompBdb_src_permance_main})

# append all files to the PgPerformanceCompBdb
set (PgPerformanceCompBdb
	${PgPerformanceCompBdb}
	${PgPerformanceCompBdb_include}
	${PgPerformanceCompBdb_include_utils}
	${PgPerformanceCompBdb_include_pg}
	${PgPerformanceCompBdb_include_bdb}
	${PgPerformanceCompBdb_src}
	${PgPerformanceCompBdb_src_utils}
	${PgPerformanceCompBdb_src_pg}
	${PgPerformanceCompBdb_src_bdb}
	${PgPerformanceCompBdb_src_permance_main}
)

if (MSVC)
switch_compiler_flag("/W3" "/W0")
else(MSVC)
switch_compiler_flag("-Wall" "-w")
switch_compiler_flag("-Werror" "")
endif(MSVC)

link_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(PgPerformanceCompBdb_DEP StorageEngineDll)
add_executable(PgPerformanceCompBdb ${PgPerformanceCompBdb})
include_directories(include ../../../include/StorageEngine)

if(MSVC)
set(dbx86 libdb53-32)
set(dbx64 libdb53)
set(nameext lib)
else(MSVC)
set(dbx86 db_cxx-5.3-32)
set(dbx64 db_cxx-5.3)
set(nameext so)
endif(MSVC)


if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	target_link_libraries(PgPerformanceCompBdb debug ${dbx64}d.${nameext})
	target_link_libraries(PgPerformanceCompBdb  optimized ${dbx64}.${nameext})
else(CMAKE_SIZEOF_VOID_P EQUAL 8)
	target_link_libraries(PgPerformanceCompBdb debug ${dbx86}d.${nameext})
	target_link_libraries(PgPerformanceCompBdb  optimized ${dbx86}.${nameext})
endif ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
target_link_libraries(PgPerformanceCompBdb StorageEngineDll)



install(TARGETS PgPerformanceCompBdb 
	RUNTIME DESTINATION server/bin
	COMPONENT test
	)
if (MSVC)
    install(FILES 
	       libdb53d.dll
           CONFIGURATIONS Debug
           DESTINATION server/bin
		   COMPONENT test
           )
    install(FILES 
		   libdb53.dll
           CONFIGURATIONS Release|RelWithDebInfo|MinSizeRel
           DESTINATION server/bin
		   COMPONENT test
           )
else (MSVC)
    install(FILES 
			libdb_cxx-5.3.so
            DESTINATION server/bin
			COMPONENT test
            )
endif (MSVC)
