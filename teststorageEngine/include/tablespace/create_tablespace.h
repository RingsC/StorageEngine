//create tablespace tests
bool test_create_tablespace_valid();
bool test_create_tablespace_with_system_reserve_name();
bool test_create_tblspc_with_occupied_name();
bool test_create_tblspc_with_occupied_path();
bool test_create_tblspc_with_nonexist_path();
bool test_create_tblspc_with_relative_path();
bool test_create_tblspc_with_too_long_path();
bool test_create_tblspc_with_pg_Prefix_name();
bool test_create_tblspc_transaction_abort();
//drop tablespace tests
bool test_drop_default_tablespace();
bool test_drop_tablesapce_exist_miss_err();
bool test_drop_tablespace_nonexist_miss_err();
bool test_drop_tablespace_nonexist_miss_ok();
bool test_drop_tablespace_with_empty_db();
bool test_drop_tablespace_with_unempty_db();
bool test_drop_tablespace_empty();
//get table space oid or name
bool test_get_tablespace_oid_exist();
bool test_get_tablespace_oid_nonexist();
bool test_get_tablespace_name();
bool test_get_default_tablespace_id();

//create database tests
bool test_create_db_with_normal_dbname_len();
bool test_create_db_with_dbname_len_63();
bool test_create_db_with_dbname_len_64();
bool test_create_db_with_larger_than_max_dbname_len();
bool test_create_db_under_tablespace();
bool test_create_database_under_tablespace_no_dir();
bool test_create_db_with_invalid_tblspc();
bool test_create_db_with_occupied_name();
bool test_create_db_under_diff_tblspc();
bool test_concurrently_createdb();
bool test_concurrently_create_tablespace();
//test drop database tests
bool test_drop_heap_under_tablespace();
bool test_clear_and_drop_tablespace();
bool test_drop_database_empty();


//////////////////utils
Oid my_create_database(char* dbname, const char* tblspcname);
Oid my_create_heap(const char* tblspcname, const char* dbname, const Oid& relid, const Oid& colid);
Oid my_create_tablespace(char* tblspcName, const char* tblspcDirName);
bool test_pause();
std::string my_create_absolute_path(std::string &strDirPath, const char* pchDirName, std::string &strDesPath);
void my_drop_tablespace(char* tblspcname, bool miss_ok);
bool my_create_db_name_len_test(const int &nLen);
void my_drop_database(const char* chDbName);