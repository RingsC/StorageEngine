#include <iostream>
#include <string>
#include <cassert>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <cstdarg>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/timer.hpp>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "replication_utils.h"
#include "interface/FDPGAdapter.h"
#include "catalog/xdb_catalog.h"
#include "catalog/index.h"
#include "access/xact.h"
#include "interface/FDPGAdapter.h"
#include "PGSETypes.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;
using namespace FounderXDB::StorageEngineNS;
StorageEngine *pStorageEngine = NULL;
Transaction *pTransaction = NULL;

namespace bf = boost::filesystem;

int rels = 0;
int over_time = 0;
int mode = 1;

std::map<Oid, std::pair<unsigned int/*for insert data*/, unsigned int/*for update data*/> > data_map;

std::string place;
std::string receiver_place;
std::string receiver_stop;
std::string sender_lock_file;
std::string sender_result_file;
std::string test_fail_dir;
std::string user_standy_config_file;
std::string user_server_config_file;
std::string server_down_file;
std::string server_up_file;

void commit_transaction()
{
        FDPG_Transaction::fd_StartTransactionCommand();
        FDPG_Transaction::fd_EndTransactionBlock();
        FDPG_Transaction::fd_CommitTransactionCommand();

}

void user_abort_transaction()
{
    FDPG_Transaction::fd_StartTransactionCommand();
    FDPG_Transaction::fd_UserAbortTransactionBlock();
    FDPG_Transaction::fd_CommitTransactionCommand();
} 

void begin_transaction()
{
    FDPG_Transaction::fd_StartTransactionCommand();
    FDPG_Transaction::fd_BeginTransactionBlock();
    FDPG_Transaction::fd_CommitTransactionCommand();
}

void begin_first_transaction()
{	
	FDPG_Transaction::fd_AbortOutOfAnyTransaction();
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_BeginTransactionBlock();
	FDPG_Transaction::fd_CommitTransactionCommand();
}

void begin_subtransaction( void )
{
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_BeginSubTransaction("");
	FDPG_Transaction::fd_CommandCounterIncrement();
	//FDPG_Transaction::fd_CommitTransactionCommand();
}

void commit_subtransaction( void )
{
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_CommitSubTransaction();
	FDPG_Transaction::fd_CommandCounterIncrement();
	//FDPG_Transaction::fd_CommitTransactionCommand();
}

void abort_subtransaction()
{
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_AbortSubTransaction();
	FDPG_Transaction::fd_CommandCounterIncrement();
}

void heap_split(RangeDatai& rang, const char*, int i, size_t len)
{
    switch(i) 
    { 
    case 1:{
                rang.start = 0;
                rang.len = 4;
                break;
           }
    case 2: {
                rang.start = 4;
                rang.len = 8;
                break;
            }
    case 3: {
                rang.start = 12;
                rang.len = 4;
                break;
            }
    case 4: {
                rang.start = 16;
                rang.len = 8;
                break;
            }
    default :{
                return ;
             }

    }
}

int str_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
    size_t i = 0;
    while(i < len1 && i < len2)
    {
        if(str1[i] < str2[i])
            return -1;
        else if(str1[i] > str2[i])
            return 1;
        else i++;

    }
    if(len1 == len2)
        return 0;
    if(len1 > len2)
        return 1;
    return -1;
}

void form_heap_colinfo(ColumnInfo &colinfo)
{
    colinfo.keys = 0;
    colinfo.col_number = NULL;
    colinfo.split_function = heap_split;
    colinfo.rd_comfunction = (CompareCallbacki*)palloc(sizeof(CompareCallbacki));
    colinfo.rd_comfunction[0] = str_compare;
}


ItemPointerData findTuple(const char *data, Relation rel, int &sta, int &count, int cmp_len)
{
    if(cmp_len == 0)
    {
        cmp_len = (int)strlen(data) + 1;
    }
    ItemPointerData ipd;
    memset(&ipd, 0, sizeof(ItemPointerData));
    //assert(NULL != rel);
    sta = -1;
    count = 0;
    char *tmp_data;
    HeapTuple tuple;
    HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
    while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
    {
        tmp_data = fxdb_tuple_to_chars(tuple);
        if(memcmp(data, tmp_data, cmp_len) == 0)
        {
            ipd = tuple->t_self;
            if(ipd.ip_posid==0)
            {
                int i=0;
            }
            ++count;
            sta = 1;
        }
        pfree(tmp_data);
    }
    heap_endscan(scan);
    return ipd;
}

static 
void do_insert(Oid rid = 0)
{
	int rows = (rand() % 1000) + 500;

	Oid relid = 0;

	if(rid == 0)
		relid = (rand() % rels) + BASE_RELID;
	else
		relid = rid;

	begin_transaction();

	//open heap and insert a tuple
	Relation testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock);

	HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));		

	int count = rows;

	while ((count--) > 0)
		FDPG_Heap::fd_simple_heap_insert(testRelation, tup);

	pfree(tup);

	FDPG_Heap::fd_heap_close(testRelation, NoLock);

	commit_transaction();

	data_map[relid].first += rows;
}

static 
void do_delete()
{
	int mod_me = (rand() % 6) + 2;

	Oid relid = (rand() % rels) + BASE_RELID;

	begin_transaction();

	//open heap and scan
	Relation testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);

	HeapTuple tuple = NULL;

	unsigned int index = 0;
	char *data = NULL;

	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		if((++index) % mod_me == 0)
		{
			data = FDPG_Common::fd_tuple_to_chars(tuple);
			FDPG_Heap::fd_simple_heap_delete(testRelation, &tuple->t_self);
			if(memcmp(data, test_insert_data_one_row, sizeof(test_insert_data_one_row)) == 0)
			{
				data_map[relid].first -= 1;
			} else {
				data_map[relid].second -= 1;
			}
			FDPG_Memory::fd_pfree(data);
		}
	}

	FDPG_Heap::fd_heap_endscan(scan);
	FDPG_Heap::fd_heap_close(testRelation, NoLock);

	commit_transaction();
}

static 
void do_update()
{
	int mod_me = (rand() % 6) + 2;

	Oid relid = (rand() % rels) + BASE_RELID;

	begin_transaction();

	//open heap and scan
	Relation testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);

	HeapTuple tuple = NULL;

	unsigned int index = 0;
	char *data = NULL;

	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		if((++index) % mod_me == 0)
		{
			data = FDPG_Common::fd_tuple_to_chars(tuple);
			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_update_data_one_row, sizeof(test_update_data_one_row));
			FDPG_Heap::fd_simple_heap_update(testRelation, &tuple->t_self, tup);
			if(memcmp(data, test_insert_data_one_row, sizeof(test_insert_data_one_row)) == 0)
			{
				data_map[relid].first -= 1;	
			} else {
				data_map[relid].second -= 1;
			}
			data_map[relid].second += 1;
			FDPG_Memory::fd_pfree(data);
			FDPG_Memory::fd_pfree(tup);
		}
	}

	FDPG_Heap::fd_heap_endscan(scan);
	FDPG_Heap::fd_heap_close(testRelation, NoLock);

	commit_transaction();
}

void rd_do()
{
	static bool is_first_time = true;

	if(is_first_time)
	{
		is_first_time = false;
		for(int i = 0; i < rels; ++i)
		{
			do_insert(BASE_RELID + i);
		}
		return;
	}

	int num = rand();
	num = (num % 3) + 1;

	switch(num)
	{
	case 1:
		do_insert();
		break;
	case 2:
		do_delete();
		break;
	case 3:
		do_update();
		break;
	default:
		assert(false);
	}
}

void init_data_map()
{
	begin_transaction();
	for(Oid relid = (Oid)BASE_RELID; relid < (Oid)(BASE_RELID + rels); ++relid)
	{
		data_map[relid].first = 0;
		data_map[relid].second = 0;
		Relation rel = FDPG_Heap::fd_heap_open(relid, AccessShareLock);
		HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		HeapTuple tup = NULL;
		while((tup = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			char *data = FDPG_Common::fd_tuple_to_chars(tup);
			if(memcmp(data, test_insert_data_one_row, sizeof(test_insert_data_one_row)) == 0)
			{
				data_map[relid].first += 1;
			} else {
				data_map[relid].second += 1;
			}
			FDPG_Memory::fd_pfree(data);
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);
	}
	commit_transaction();
}

bool check_over_time(TimestampTz &start_time)
{
	TimestampTz t_time = GetCurrentTimestamp();

	long secs = 0;
	int msecs = 0;

	double cmp1, cmp2;

	TimestampDifference(start_time, t_time, &secs, &msecs);

	cmp1 = over_time;
	cmp2 = secs / 60.0;

	if(cmp2 > cmp1)
	{
		return true;
	} else {
		return false;
	}
}

void server_mark_result()
{
	FILE *f = fopen(sender_result_file.c_str(), "w");

	std::map<Oid, std::pair<unsigned int, unsigned int> >::iterator it = data_map.begin();

	while(it != data_map.end())
	{
		fprintf(f, " %u %u %u", it->first, it->second.first, it->second.second);
		++it;
	}

	fclose(f);
}
void reset_info_dir()
{
	bf::remove_all(place.c_str());

	bf::create_directory(place);

	bf::remove_all(test_fail_dir);
	bf::remove_all(receiver_place);

	bf::create_directories(receiver_place);
	bf::create_directories(receiver_stop);
	bf::create_directories(test_fail_dir);
}

void lock_sender()
{
	remove(sender_lock_file.c_str());
}

void unlock_sender()
{
	assert(!bf::exists(sender_lock_file.c_str()));

	FILE *f = fopen(sender_lock_file.c_str(), "w");
	fclose(f);
}

bool wait_for_all_receiver(int receiver_count)
{
	bool result = true;

	while(true)
	{
		if(!bf::is_empty(receiver_place.c_str()))
		{
			int count_result_file = 0;

			bf::path fullPath(receiver_place);
			bf::directory_iterator end;
			bf::directory_iterator it(fullPath);
			for(; it != end; ++it)
			{
				++count_result_file;
			}
			if(count_result_file == receiver_count)
			{
				break;
			} else {
				my_sleep(1);
			}
		} else {
			my_sleep(1);
			continue;
		}
	}

	return result;
}


bool check_test_result()
{
	if(bf::is_empty(test_fail_dir))
	{
		return true;
	}

	return false;
}

void my_sleep(unsigned int sec)
{
	boost::xtime t;
	boost::xtime_get(&t, boost::TIME_UTC_);

	t.sec += sec;
	boost::thread::sleep(t);
}

void mark_server_down()
{
	if(mode == 2)
	{
		FILE *f = fopen(server_down_file.c_str(), "w");
		fclose(f);
	}
}

void wait_for_server_down()
{
	if(mode == 2)
	{
		while(true)
		{
			if(bf::exists(server_down_file))
			{
				break;
			}

			my_sleep(1);
		}

		my_sleep(3);
	}
}

void mark_server_up()
{
	bf::remove(server_down_file);

	FILE *f = fopen(server_up_file.c_str(), "w");
	if(!f)
		assert(false);
	fclose(f);
}

static
int simple_cmp(const char *data1, size_t len1, const char *data2, size_t len2)
{
	return strcmp(data1, data2);
}

static
void simple_splite(RangeData &rd, const char *data, int col, size_t len)
{	if(col == 1)
	{
		rd.start = 0;
		rd.len = 2;
		return;
	}

	assert(false);
}

extern void setColInfo(Oid colid, Colinfo pcol_info);

void init_tmprel_colinfo()
{
	Colinfo col = (Colinfo)malloc(sizeof(ColinfoData));
	memset(col, 0, sizeof(ColinfoData));
	col->keys = 0;
	col->split_function = simple_splite;
	setColInfo(HEAP_COLINFO_ID, col);
	col = (Colinfo)malloc(sizeof(ColinfoData));
	memset(col, 0, sizeof(ColinfoData));
	col->keys = 1;
	col->split_function = simple_splite;
	col->col_number = (size_t*)malloc(sizeof(size_t));
	col->col_number[0] = 1;
	col->rd_comfunction = (CompareCallback*)malloc(sizeof(CompareCallback));
	col->rd_comfunction[0] = simple_cmp;
	setColInfo(INDEX_COLINFO_ID, col);
}





