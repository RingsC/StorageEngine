#ifndef HS_UTILS_HPP
#define HS_UTILS_HPP

typedef int64 TimestampTz;

#define test_insert_data_one_row "test_insert_data_one_row_"
#define test_update_data_one_row "test_update_data_one_row_"
#define base_backup_ok_file "/bk_ok"

#define HEAP_COLINFO_ID 12
#define INDEX_COLINFO_ID 13

extern int rels;
extern int over_time;
extern int mode;

extern 
std::map<unsigned int, std::pair<unsigned int/*for insert data*/, unsigned int/*for update data*/> > data_map;

extern std::string place;
extern std::string receiver_place;
extern std::string receiver_stop;
extern std::string sender_lock_file;
extern std::string sender_result_file;
extern std::string test_fail_dir;
extern std::string user_standy_config_file;
extern std::string user_server_config_file;
extern std::string server_down_file;
extern std::string server_up_file;

const int BASE_RELID = 10000;
const int BASE_INDEXID = 20000;

namespace FounderXDB
{
namespace StorageEngineNS
{
class ColumnInfo;
}
}

void form_heap_colinfo(FounderXDB::StorageEngineNS::ColumnInfo &colinfo);
void commit_transaction();
void user_abort_transaction();
void begin_transaction();
void begin_first_transaction();
void begin_subtransaction();
void commit_subtransaction();
void abort_subtransaction();

void rd_do();
void init_data_map();
bool check_over_time(TimestampTz &start_time);
void server_mark_result();
void reset_info_dir();
void lock_sender();
void unlock_sender();
bool wait_for_all_receiver(int receiver_count);
bool check_test_result();
void my_sleep(unsigned int sec);
void mark_server_down();
void wait_for_server_down();
void mark_server_up();
void init_tmprel_colinfo();

#endif