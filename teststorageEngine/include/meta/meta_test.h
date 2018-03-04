#include "utils/rel.h"
#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "postmaster/xdb_main.h"

//get max oid
Oid get_max_id1();


//delete metadata
bool fxdb_delete_meta(unsigned int object_id);

//search all index info on the table_id
bool fxdb_get_index_info(unsigned int table_id, IndinfoData& colinfo);

void fxdb_create_catalog();

int test_insert_metadata();

int test_get_max_id();

int test_delete_metadata();

int test_get_index_info();

int  test_get_meta_info();

int test_get_tblspace();

int test_set_get_indinfo();

int test_get_type();

int test_get_max_id2();

int test_check_meta_info();

int test_thread_meta_test();