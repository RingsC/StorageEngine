add_subdirectory(storageEngineLib)
add_subdirectory(StorageEngineDll)
add_subdirectory(baseBackup)
add_subdirectory(fxdb_controldata)
add_subdirectory(redland)

if (MSVC)
	add_subdirectory(pthread)
endif (MSVC)

if(FD_ENABLE_STORAGE_TEST)
    add_subdirectory(bootstrap)
    add_subdirectory(PgPerformanceCompBdb)
    if (MSVC)
        add_subdirectory(Performbdb)
        add_subdirectory(Performpg)
    endif (MSVC)
    add_subdirectory(teststorageEngine)
    add_subdirectory(testStorageEngineDll)
    add_subdirectory(walreceiver)
    add_subdirectory(walsender)
    add_subdirectory(testThreadTransaction)
    add_subdirectory(src/backend/replication/repl_ha/simulate)
    add_subdirectory(testPerformance)
    add_subdirectory(testTwoPhaseCommit)
endif(FD_ENABLE_STORAGE_TEST)
