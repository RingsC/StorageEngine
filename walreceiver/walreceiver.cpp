#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/thread/thread.hpp>
#include "utils/attr_setter.h"
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include "utils/util.h"
#include <iostream>
#include <fstream>
#include "access/xact.h"
#include "catalog/xdb_catalog.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/utils.h"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "Configs.h"
#include "walreceiver.h"
#include "common/XdbCommon.h"
#include "postmaster/xdb_main.h"
#include "replication_utils.h"
#include "replication/syncrep.h"
#include "replication/walsender.h"

using std::cout;
using std::endl;
using namespace::FounderXDB::StorageEngineNS;
std::map<std::string,std::string> ProgramOpts;

namespace bf = boost::filesystem;

extern StorageEngine *pStorageEngine ;
extern Transaction *pTransaction ;

static
void test_result(bool &);

static
void thread_test_result_handle(bool *);

XLogRecPtr
RequestXLogSwitch(void);

static 
void init_argv_int(char **argv, int index, int &pram)
{
	std::string tmp_ = argv[index];
	pram = atoi(tmp_.c_str());
}

int t_argc = 0;
char **t_argv = NULL;

void save_arg(const int argc, char **argv)
{
	t_argc = argc;
	t_argv = argv;
	std::string t1_argv;
	for(int i = 0; i < t_argc; ++i)
	{
		t1_argv.append(argv[i]);
		t1_argv.append(" ");
	}
	t1_argv.erase(t1_argv.rfind(" "));

	//save program options
	using namespace std;
	using namespace boost::xpressive;
	sregex opReg = sregex::compile("(-\\w+)\\s+(\\S+)");
	smatch mach;
	std::string::const_iterator it = t1_argv.begin(); 
	while (regex_search(it,t1_argv.end(),mach,opReg))
	{
		ProgramOpts[mach[1]] = mach[2];
		it = mach[0].second;
	}
}

bool isInWarmStandby = false;

bool amIInServerAtNext = false;

std::string g_strDataDir;

std::string server_mark;
int can_server_stop = 0;
int receivers = 0;

#define printf_info(text, ...) \
	printf("\n\n%d ------\n", pthread_self()); \
	printf(text, __VA_ARGS__); \
	printf("\n\n");

#define printf_info_noargs(text) \
	printf("\n\n%d ------\n", pthread_self()); \
	printf(text); \
	printf("\n\n");

static
void init_argv_string(char **argv, int index, std::string &pram)
{
	if(t_argc <= index)
		return ;

	pram = argv[index];
}

static 
void mark_result(bool success)
{
	unsigned int pid = pthread_self();
	char result_file_name[512];
	memset(result_file_name, 0, 512);

	sprintf(result_file_name, "%u", pid);

	std::string result_dir = receiver_place;
	result_dir.append("/");
	result_dir.append(result_file_name);

	FILE *f = fopen(result_dir.c_str(), "w");

	if(success)
	{
		fprintf(f, "1");
	} else 
	{
		fprintf(f, "0");
	}

	fclose(f);

	remove(server_up_file.c_str());
}

static 
void init_info_dir()
{
	std::string::size_type pos = place.rfind("StorageEngine");
	pos +=strlen("StorageEngine");
	place.erase(pos);
	place.append("/info");

	sender_lock_file = place;
	sender_lock_file.append("/sender.l");

	receiver_place = place;
	receiver_place.append("/receiver_result");

	receiver_stop = place;
	receiver_stop.append("/receiver_stop");

	sender_result_file = place;
	sender_result_file.append("/sender_result");

	test_fail_dir = place;
	test_fail_dir.append("/test_result");

	server_down_file = place;
	server_down_file.append("/server_down");

	server_up_file = place;
	server_up_file.append("/server_up");
}

static
bool wait_for_sender_ok()
{
	if(!bf::exists(sender_lock_file.c_str()))
		return false;

	return true;
}

#define CHECK_DATA_FAIL_NAP_TIME 3
#define CHECK_DATA_OK_NAP_TIME 1
#define MAX_WAIT_TIME 20

#define UNKNOW_SERVER_MARK "none"

static
void read_result()
{
	assert(bf::exists(sender_result_file));

	FILE *f = fopen(sender_result_file.c_str(), "r");

	while(!feof(f))
	{
		Oid relid = 0;
		std::pair<unsigned int, unsigned int> t_pair;
		fscanf(f, "%u%u%u", &relid, &t_pair.first, &t_pair.second);
		data_map[relid] = t_pair;
	}

	fclose(f);
}

static
void mark_receiver_stop()
{
	std::string stop_state_file = receiver_stop;

	char file_name[512];
	int tid = pthread_self();
	memset(file_name, 0, sizeof(file_name));

	sprintf(file_name, "/%d", tid);

	stop_state_file.append(file_name);

	FILE *f = fopen(stop_state_file.c_str(), "w");
	fclose(f);
}

static
void shut_down_xlog()
{
	extern fxdb_thread_t *xlog_handle;

	pg_wakeup(xlog_handle->thid, SIGUSR2);
}

static
void warm_standy_wait()
{
	
	while(true)
	{
		if(wait_for_sender_ok())
		{
			break;
		}

		my_sleep(1);
	}

	/* �ȴ�5�룬��xlog������� */
	for(int i = 0; i < 5; ++i)
		my_sleep(1);

	mark_result(true);

	printf_info_noargs("stop receiver : ok!");

	wait_for_server_down();

	if(!amIInServerAtNext)
		exit(0);
	else if(amIInServerAtNext && can_server_stop)
		exit(0);
	else
	{
		shut_down_xlog();
	}
}

static
void prepare_warm_standby(storage_params *param)
{
	if(!param->slaveEnableHotStandby)
	{
		isInWarmStandby = true;
		boost::thread_group tg;
		tg.create_thread(boost::bind(&warm_standy_wait));
	}
}
static
void read_user_standy_config_file()
{
	std::ifstream in(user_standy_config_file.c_str());

	if(!in)
		return;

	in >> server_mark;
	in >> can_server_stop;
	in >> rels;
	in >> receivers;
	in >> over_time;
	in >> mode;

	in.close();

	if(server_mark.compare(UNKNOW_SERVER_MARK) == 0)
		return;

	if(server_mark.compare(g_strDataDir) == 0)
		amIInServerAtNext = true;
}

static
void prepare_wal_insert_data()
{
	read_result();
	reset_info_dir();
}

static
bool wal_insert_data()
{
	lock_sender();

	bool result = Wal_in_server_and_insert_data();

	unlock_sender();

	return result;
}

static
bool end_insert_data()
{
	wait_for_all_receiver(receivers);

	bool result = check_test_result();

	/* �ȴ�standy����stop_engine */
	my_sleep(3 * receivers);

	return result;
}

static
void wait_for_recovery_end()
{
	while(true)
	{
		if(!RecoveryInProgress())
		{
			break;
		}
		my_sleep(3);
	}
}

extern void startWalreceiverListener(bool);
extern bool StandbyMode;
extern char	   *SyncRepStandbyNames;

#define SyncStandbysDefined() \
	(SyncRepStandbyNames != NULL && SyncRepStandbyNames[0] != '\0')

BOOST_AUTO_TEST_SUITE(TestReplication)

BOOST_AUTO_TEST_CASE(case1)
{
	int DATA_TIMES;
	std::string replication_server = "-replication";
	std::string check_data_001 = "-check_data_001";
	//
	//save_arg(argc,argv);

	/* ��ʼ��standy���ò��� */
	init_argv_string(t_argv, 5, user_standy_config_file);
	read_user_standy_config_file();

	place = t_argv[0];
	init_info_dir();

	std::string server_type = t_argv[3];
	std::string count_site = t_argv[4];
	if (t_argv[4]!= NULL)
	{
		FILE *fp;
		const char *p=count_site.c_str();
		fp = fopen(p,"r");
		fscanf(fp,"%d",&DATA_TIMES);//read insert data times from file
		fclose(fp);

		std::string filename = "DATA_TIMES.txt";
		std::string newfile = "CHECK";
		std::string::size_type pos;
		//cout << "before replace:" << count_site <<endl;
		if ((pos=count_site.find(filename))!=string::npos)
		{
			count_site.replace(pos,filename.length(),newfile);//create new file for svn check
		}
		//cout << "after replace:" << count_site <<endl;
	}

	//getchar();

	storage_params* params = GetStorageParam(ProgramOpts["-conf"]);

	prepare_warm_standby(params);

	FDPG_StorageEngine::fd_start_engine(g_strDataDir.c_str(),80,false,params);
	printf("replication receiver working...\n");

	if (server_type == replication_server)
	{
		Walreceiver();
	}

	if (server_type == check_data_001)
	{
		init_tmprel_colinfo();

		if(params->slaveEnableHotStandby)
		{
			Wal_check_data_001(DATA_TIMES , count_site);
			BOOST_CHECK(true);
		}

		if(amIInServerAtNext && !can_server_stop)
		{
			if(params->slaveEnableHotStandby)
			{
				wait_for_server_down();

				shut_down_xlog();

				wait_for_recovery_end();

				startWalreceiverListener(false);
			}

			prepare_wal_insert_data();

			printf_info_noargs("hot server works..");

			bool result = true;

			/* �����������ҹر�ͬ���ύ */
			/* ԭ����������ɨ����Ҫд��־��ͬʱ���ǵı�����û���� */
			char *tmp_sync_name = NULL;
			{
				if (SyncRepRequested() && SyncStandbysDefined())
				{
					tmp_sync_name = SyncRepStandbyNames;
					SyncRepStandbyNames = "";
				}
			}

			boost::thread_group tg;
			tg.create_thread(boost::bind(&thread_test_result_handle, &result));
			tg.join_all();

			/* ��ԭͬ���ύ */
			{
				if (SyncRepRequested() && SyncStandbysDefined())
				{
					SyncRepStandbyNames = tmp_sync_name;
				}
			}

			BOOST_CHECK(result);

			printf_info("%schange to server", g_strDataDir.c_str());

			mark_server_up();

			wal_insert_data();
			result = end_insert_data();

			BOOST_CHECK(result);

			mark_server_down();
		}
	}

	//if(!amIInServerAtNext)
	//	FDPG_StorageEngine::fd_end_engine();

	printf_info_noargs("stop receiver : ok!111111111");

	exit(0);
}

BOOST_AUTO_TEST_SUITE_END()

bool Walreceiver()
{ 
	return true;
}

static
void test_temp_rel(std::pair<unsigned int, unsigned int> &t_pair)
{
	unsigned int count = 0;

	begin_transaction();

	/* ������ʱ�� */
	Oid ttid = FDPG_Heap::fd_temp_heap_create(MyDatabaseTableSpace, MyDatabaseId, HEAP_COLINFO_ID);

	FDPG_Transaction::fd_CommandCounterIncrement();

	Relation tmpR = FDPG_Heap::fd_heap_open(ttid, RowExclusiveLock);
	/* ������ʱ������ */
	FDPG_Index::fd_index_create(tmpR, BTREE_TYPE, 0, INDEX_COLINFO_ID);

	FDPG_Transaction::fd_CommandCounterIncrement();

	/* ������ҳ��������� */
	HeapTuple tuple = NULL;
	for(unsigned int i = 0; i < t_pair.first / 4; ++i)
	{
		tuple = fdxdb_heap_formtuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));
		FDPG_Heap::fd_simple_heap_insert(tmpR, tuple);
		pfree(tuple);
	}

	FDPG_Heap::fd_heap_close(tmpR, NoLock);

	FDPG_Transaction::fd_CommandCounterIncrement();

	tmpR = FDPG_Heap::fd_heap_open(ttid, RowExclusiveLock);

	/* ����ɨ�裬���������ݸ��� */
	char *tdata = NULL;
	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(tmpR, SnapshotNow, 0, NULL);
	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		tdata = FDPG_Common::fd_tuple_to_chars(tuple);
		assert(!memcmp(tdata, test_insert_data_one_row, sizeof(test_insert_data_one_row)));
		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_update_data_one_row, sizeof(test_update_data_one_row));
		FDPG_Heap::fd_simple_heap_update(tmpR, &tuple->t_self, tup);
		pfree(tup);
		pfree(tdata);
		++count;
	}
	FDPG_Heap::fd_heap_endscan(scan);

	assert(count == t_pair.first / 4);

	/* �ٴδ���һ������ */
	FDPG_Index::fd_index_create(tmpR, BTREE_TYPE, 0, INDEX_COLINFO_ID);

	FDPG_Heap::fd_heap_close(tmpR, NoLock);

	FDPG_Transaction::fd_CommandCounterIncrement();

	tmpR = FDPG_Heap::fd_heap_open(ttid, RowExclusiveLock);

	count = 0;

	/* ɾ�����е����� */
	scan = FDPG_Heap::fd_heap_beginscan(tmpR, SnapshotNow, 0, NULL);
	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		tdata = FDPG_Common::fd_tuple_to_chars(tuple);
		assert(!memcmp(tdata, test_update_data_one_row, sizeof(test_update_data_one_row)));
		FDPG_Heap::fd_simple_heap_delete(tmpR, &tuple->t_self);
		pfree(tdata);
		++count;
	}
	FDPG_Heap::fd_heap_endscan(scan);

	FDPG_Heap::fd_heap_close(tmpR, NoLock);

	assert(count == t_pair.first / 4);

	FDPG_Transaction::fd_CommandCounterIncrement();

	tmpR = FDPG_Heap::fd_heap_open(ttid, RowExclusiveLock);

	count = 0;

	/* ��ʱ����û������ */
	scan = FDPG_Heap::fd_heap_beginscan(tmpR, SnapshotNow, 0, NULL);
	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		++count;
	}
	FDPG_Heap::fd_heap_endscan(scan);

	assert(count == 0);

	FDPG_Heap::fd_heap_close(tmpR, RowExclusiveLock);

	commit_transaction();
}

static 
std::pair<unsigned int, unsigned int>
collect_result(Oid relid)
{
	std::pair<unsigned int, unsigned int> t_pair;
	t_pair.first = 0;
	t_pair.second = 0;

	begin_transaction();//����һ������ 
	Oid relspace = MyDatabaseTableSpace;
	Relation testRelation;
	Relation indexRelation;

	//open index scan
	testRelation = FDPG_Heap::fd_heap_open(relid,AccessShareLock, MyDatabaseId);
	indexRelation = FDPG_Index::fd_index_open(relid + 10000, AccessShareLock);

	IndexScanDesc scan;
	scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, 0, NULL);
	FDPG_Index::fd_index_rescan(scan, NULL, 0, NULL, 0);
	HeapTuple tuple;
	char * temp;
	//int counter = 0;
	int flag = 1;
	while ((tuple = FDPG_Index::fd_index_getnext(scan, ForwardScanDirection)) != NULL)
	{        
		//++counter;
		temp=fxdb_tuple_to_chars(tuple);
		flag = memcmp(temp,test_insert_data_one_row,sizeof(test_insert_data_one_row));
		if(flag != 0)
		{
			t_pair.second += 1;
		} else {
			t_pair.first += 1;
		}

		FDPG_Memory::fd_pfree(temp);

	}

	FDPG_Index::fd_index_endscan(scan);

	FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
	FDPG_Heap::fd_heap_close(testRelation, AccessShareLock);
	//FDPG_Heap::fd_heap_drop(RELID, MyDatabaseId);//������ֻ���Ĳ����Խ�������ɾ���ġ�
	commit_transaction();

	return t_pair;
}

static
bool test_time_out(TimestampTz tz)
{
	TimestampTz current = GetCurrentTimestamp();

	long sec = 0;
	int msec = 0;

	TimestampDifference(tz, current, &sec, &msec);

	if(sec > MAX_WAIT_TIME)
	{
		return true;
	}

	return false;
}

static
void mark_test_fail()
{
	char data[512];
	memset(data, 0, sizeof(data));

	unsigned int pid = pthread_self();
	sprintf(data, "%u", pid);

	std::string fail_file = test_fail_dir;
	fail_file.append("/");
	fail_file.append(data);

	FILE *f = fopen(fail_file.c_str(), "w");
	fclose(f);
}

static
bool test_can_not_do(Oid relid)
{
	bool result = true;

	/* ���ܴ����� */
	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, 1000000);//�쳣(error)
		result = false;
	} catch(StorageEngineExceptionUniversal &se)
	{
		printf("\nstandy cannot create table : %s\n", se.getErrorMsg());
		user_abort_transaction();
	}

	/* ���ܲ������� */
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(relid, RowExclusiveLock);
		HeapTuple tuple = FDPG_Heap::fd_heap_form_tuple("aa", 2);
		FDPG_Heap::fd_simple_heap_insert(rel, tuple);//�쳣(error)
		printf_info_noargs("������ִ�� insert ����.error!");
		result = false;
	} catch(StorageEngineExceptionUniversal &se)
	{
		printf("\nstandy cannot insert data : %s\n", se.getErrorMsg());
		user_abort_transaction();
	}

	/* ���ܸ������� */
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(relid, RowExclusiveLock);
		HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		HeapTuple tup = NULL;
		while((tup = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			HeapTuple tuple = FDPG_Heap::fd_heap_form_tuple("aa", 2);
			FDPG_Heap::fd_simple_heap_update(rel, &tup->t_self, tuple);//�쳣(error)
		}
		printf_info_noargs("������ִ�� update ����.error!");
		result = false;
	} catch(StorageEngineExceptionUniversal &se)
	{
		printf("\nstandy cannot update data : %s\n", se.getErrorMsg());
		user_abort_transaction();
	}

	/* ����ɾ������ */
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(relid, RowExclusiveLock);
		HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		HeapTuple tup = NULL;
		while((tup = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			FDPG_Heap::fd_simple_heap_delete(rel, &tup->t_self);//�쳣(error)
		}
		printf_info_noargs("������ִ�� delete ����.error!");
		result = false;
	} catch(StorageEngineExceptionUniversal &se)
	{
		printf("\nstandy cannot delete data : %s\n", se.getErrorMsg());
		user_abort_transaction();
	}

	/* ����ɾ���� */
	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_drop(relid);
		printf_info_noargs("������ִ�� drop ����.error!");//�쳣(error)
		result = false;
	} catch(StorageEngineExceptionUniversal &se)
	{
		printf("\nstandy cannot delete table : %s\n", se.getErrorMsg());
		user_abort_transaction();
	}

	/* ���ܴ������� */
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(relid, RowExclusiveLock);
		FDPG_Index::fd_index_create(rel, BTREE_TYPE, 2000000, INDEX_COLINFO_ID);//�쳣(error)
		printf_info_noargs("������ִ�� drop ����.error!");
		result = false;
	} catch(StorageEngineExceptionUniversal &se)
	{
		printf("\nstandy cannot create index : %s\n", se.getErrorMsg());
		user_abort_transaction();
	}

	return result;
}

static
void test_result(bool &all_test_ok)
{
	std::map<Oid, std::pair<unsigned int, unsigned int> >::iterator it = data_map.begin();

	try
	{
		while(it != data_map.end())
		{
			if(!all_test_ok)
			{
				break;
			}

			TimestampTz tz = GetCurrentTimestamp();

			while(1)
			{
				std::pair<unsigned int, unsigned int> t_pair;

				int relid = it->first;

				t_pair = collect_result(relid);

				if (t_pair.first != 0 || t_pair.second != 0)
				{	
					if(t_pair.first == data_map[relid].first &&
						t_pair.second == data_map[relid].second)
					{
						printf_info("receiver check rel %u data : success...", relid);

						if(!amIInServerAtNext)
							test_temp_rel(t_pair);

						my_sleep(CHECK_DATA_OK_NAP_TIME);
						break;
					} else {
						if(test_time_out(tz))
						{
							all_test_ok = false;
							printf_info("receiver check rel %u data : fail!!!!", relid);
							break;
						}
						printf_info("receiver check rel %u data : fail!!!! just wait for a while...", relid);

						my_sleep(CHECK_DATA_FAIL_NAP_TIME);
						continue;
					}
				} else {
					if(test_time_out(tz))
					{
						all_test_ok = false;
						printf_info_noargs("receiver check data : fail!!!!");
						break;
					}
					printf_info_noargs("receiver check data : fail!!!! just wait for a while...");

					my_sleep(CHECK_DATA_FAIL_NAP_TIME);
					continue;
				}

			}
			++it;
		}
	} catch (StorageEngineExceptionUniversal &se)
	{
		std::cout << se.getErrorMsg() << std::endl;
		user_abort_transaction();
		all_test_ok = false;
	}
}

static
void thread_wal_in_server_handle(bool *result_)
{
	extern void *fxdb_SubPostmaster_Main(void *);

	BackendParameters *params = (BackendParameters*)malloc(sizeof(BackendParameters));
	params->MyThreadType = backend;

	fxdb_SubPostmaster_Main(params);

	bool result = true;

	printf("replication server working...\n");

	try
	{
		TimestampTz start_time = GetCurrentTimestamp();

		/* ����ѭ���������� */
		while(true)
		{

			rd_do();

			if(check_over_time(start_time))
			{
				break;
			}

			my_sleep(1);
		}

		/* �����������ݽ����������receiver������ */
		server_mark_result();

		result = true;

	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		result = false;
	}	

	*result_ = result;

	proc_exit(0);
}

bool Wal_in_server_and_insert_data()
{
	bool result;

	boost::thread_group tg;
	tg.create_thread(boost::bind(&thread_wal_in_server_handle, &result));
	tg.join_all();

	return result;
}

static
void thread_test_result_handle(bool *all_test_ok)
{
	extern void *fxdb_SubPostmaster_Main(void *);

	BackendParameters *params = (BackendParameters*)malloc(sizeof(BackendParameters));
	params->MyThreadType = backend;

	fxdb_SubPostmaster_Main(params);

	test_result(*all_test_ok);

	proc_exit(0);
}

bool Wal_check_data_001(int DATA_TIMES , std::string file_site)
{
	bool isFirstTime = true;

	printf("check_data_001 working...\n");

	bool result = true;

	try
	{
		/* ����ѭ������xlog */
		while(true)
		{

			if(isFirstTime)
			{
				/* ������һ���������ȴ��������´����ı���Ϣ���͹��� */
				my_sleep(5);
				isFirstTime = false;
			}

			/* �ȴ�������������,�˳� */
			if(wait_for_sender_ok())
				break;

			my_sleep(1);
		}
	} catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		result = false;
	}

	/* ����ʱ������������ */

	//�ȴ�xlog�������
	for(int i = 0; i < 5; ++i)
		my_sleep(1);

	read_result();

	bool all_test_ok = true;

	boost::thread_group tg;
	tg.create_thread(boost::bind(&thread_test_result_handle, &all_test_ok));
	tg.join_all();

	if(!isInWarmStandby)
	{
		if(!all_test_ok || !test_can_not_do((data_map.begin())->first))
			result = false;
	} else 
	{
		if(!all_test_ok)
			result = false;
	}

	if(!result)
	{
		mark_test_fail();
	}

	for(int i = 0; i < 3; ++i)
		my_sleep(1);

	mark_result(true);

	return true;
}