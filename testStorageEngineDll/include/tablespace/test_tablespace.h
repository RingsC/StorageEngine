#ifndef _TEST_TABLESPACE_H
#define _TEST_TABLESPACE_H

bool test_getalldatabaseInfo_tbspc();

bool testCreateTablespace( void );

bool testDropTablespace( void );

bool test_create_tblspc();
bool test_drop_tblspc();
bool test_tblspc_recovery();
bool test_create_tblspc_prepare();
bool database_heap_index_create(const char* tblpsc);
bool test_create_heap_under_diff_tblspc();
bool test_get_create_db_oid();

void write_recovery_time_to_file();
void write_heap_id_to_file(const char* filename, int heapid);
void remove_my_dir(const char* dirname);

bool TestDBExtraData( void );

bool TestGetAllDBInfos( void );
bool TestGetDBInfoById( void );
#endif
