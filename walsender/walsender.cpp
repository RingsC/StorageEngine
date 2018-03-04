#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include "utils/attr_setter.h"
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/thread/thread.hpp>
#include "utils/util.h"
#include <iostream>
#include <fstream>
#include "interface/FDPGAdapter.h"
#include "catalog/xdb_catalog.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/utils.h"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h" 
#include "Configs.h"
#include "walsender.h"
#include "common/XdbCommon.h"
#include "postmaster/xdb_main.h"
#include "access/xact.h"
#include "replication_utils.h"


using std::cout;
using std::endl;
using std::string;
using namespace::FounderXDB::StorageEngineNS;
std::map<std::string,std::string> ProgramOpts;

namespace bf = boost::filesystem;

extern StorageEngine *pStorageEngine ;
extern Transaction *pTransaction ;
void MySleep(long millsec);

int t_argc;
char **t_argv;
std::string g_strDataDir;

void save_arg(const int argc, char **argv)
{
	t_argc = argc;
	t_argv = argv;
	int s_argc = argc;
	std::string s_argv;
	for(int i = 0; i < s_argc; ++i)
	{
		s_argv.append(argv[i]);
		s_argv.append(" ");
	}
	s_argv.erase(s_argv.rfind(" "));

	//save program options
	using namespace std;
	using namespace boost::xpressive;
	sregex opReg = sregex::compile("(-\\w+)\\s+(\\S+)");
	smatch mach;
	std::string::const_iterator it = s_argv.begin(); 
	while (regex_search(it,s_argv.end(),mach,opReg))
	{
		ProgramOpts[mach[1]] = mach[2];
		it = mach[0].second;
	}
}

int receivers = 1;

int useExistsDataDir;

#define printf_info(text, ...) \
	printf("\n\n%d ------\n", pthread_self()); \
	printf(text, __VA_ARGS__); \
	printf("\n\n");

#define printf_info_noargs(text) \
	printf("\n\n%d ------\n", pthread_self()); \
	printf(text); \
	printf("\n\n");

static 
void init_info_dir()
{
	std::string::size_type pos = place.rfind("StorageEngine");
	pos +=strlen("StorageEngine");
	place.erase(pos);
	place.append("/info");

	bf::remove_all(place.c_str());

	bf::create_directory(place);

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

	bf::remove_all(test_fail_dir);
	bf::remove_all(receiver_place);

	bf::create_directories(receiver_place);
	bf::create_directories(receiver_stop);
	bf::create_directories(test_fail_dir);
}

#define NAP_TIME 2

static 
void init_argv_int(char **argv, int index, int &pram)
{
	if(t_argc <= index)
		return ;

	std::string tmp_ = argv[index];
	pram = atoi(tmp_.c_str());
}

static
void init_argv_string(char **argv, int index, std::string &pram)
{
	if(t_argc <= index)
		return ;

		pram = argv[index];
}

static
void read_user_server_config_file()
{
	std::ifstream in(user_server_config_file.c_str());

	if(!in)
		return;

	in >> rels;
	in >> receivers;
	in >> useExistsDataDir;
	in >> over_time;
	in >> mode;

	in.close();
}

BOOST_AUTO_TEST_SUITE(TestReplication)

BOOST_AUTO_TEST_CASE(case1)
{
	int DATA_TIMES;

	/* 初始化进程通信的文件路径 */
	place = t_argv[0];

	/* 初始化服务器压力配置文件  */
	init_argv_string(t_argv, 5, user_server_config_file);
	read_user_server_config_file();

	init_info_dir();

	std::string server_type = t_argv[3];
	std::string count_site = t_argv[4];
	std::string filename = "DATA_TIMES.txt";
	int flag = (int)count_site.find(filename);
	if (flag >= 0)
	{
		FILE *fp;
		const char *p=count_site.c_str();
		fp = fopen(p,"r");
		fscanf(fp,"%d",&DATA_TIMES);//read insert data times from file
	}

	std::string newfile = "CHECK";
	std::string::size_type pos;
	//cout << "before replace:" << count_site <<endl;
	if ((pos=count_site.find(filename))!=string::npos)
	{
		count_site.replace(pos,filename.length(),newfile);//file for check walreceiver finished
	}
	//cout << "after replace:" << count_site <<endl;
	std::string basebackup_server = "-basebackup";
	std::string replication_server = "-replication";

	storage_params* params = GetStorageParam(ProgramOpts["-conf"]);

	FDPG_StorageEngine::fd_start_engine(g_strDataDir.c_str(),80,false,params);

	init_tmprel_colinfo();

	if (server_type == basebackup_server)
	{
		Walsender_insert_data_basebackup(count_site);
	}
	if (server_type == replication_server)
	{
		mark_server_up();

		srand((unsigned int)time(NULL));

		lock_sender();

		Walsender_insert_data_replication(DATA_TIMES,count_site);

		unlock_sender();
	}

	if(server_type == basebackup_server)
	{
		BOOST_CHECK(true);
	} else 
	{
		wait_for_all_receiver(receivers);

		bool result = check_test_result();

		BOOST_CHECK(result);

		/* 等待standy正常stop_engine */
		my_sleep(3 * receivers);

		mark_server_down();
	}

	exit(0);	
}

BOOST_AUTO_TEST_SUITE_END()

static
bool check_all_receiver_stop()
{
	int count = 0;

	bf::path fullPath(receiver_stop);
	bf::directory_iterator end;
	bf::directory_iterator it(fullPath);

	while(it != end)
	{
		++count;
		++it;
	}

	if(count == receivers)
	{
		return true;
	}

	return false;
}

static
bool check_all_result_ok()
{
	assert(!bf::is_empty(receiver_place.c_str()));

	bool result = true;

	bf::path fullPath(receiver_place);
	bf::directory_iterator end;
	bf::directory_iterator it(fullPath);

	for(; it != end; ++it)
	{
		if(result)
		{
			int test = 0;
			FILE *f = fopen(it->path().string().c_str(), "r");
			fscanf(f, "%d", &test);
			fclose(f);

			if(!test)
			{
				result = false;
			}
		}
		remove(it->path().string().c_str());
	}

	return result;
}

static
void thread_handle()
{
	extern void *fxdb_SubPostmaster_Main(void *);

	BackendParameters * params = (BackendParameters*)malloc(sizeof(BackendParameters));
	params->MyThreadType = backend;

	fxdb_SubPostmaster_Main(params);

	int relid = BASE_RELID;
	int index_id = BASE_INDEXID;

	if(!useExistsDataDir)
	{
		begin_first_transaction();

		/* 创建多个表执行测试 */
		for(int i = 0; i < rels; ++i)
		{
			FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, relid + i, MyDatabaseId, HEAP_COLINFO_ID);
			Relation rel = FDPG_Heap::fd_heap_open(relid + i, ShareLock);
			FDPG_Index::fd_index_create(rel, BTREE_TYPE, index_id + i, INDEX_COLINFO_ID);
			FDPG_Heap::fd_heap_close(rel, ShareLock);
		}

		commit_transaction();
	} else 
	{
		init_data_map();
	}

	TimestampTz start_time = GetCurrentTimestamp();

	/* 主机循环操作数据 */
	while(true)
	{

		rd_do();

		if(check_over_time(start_time))
		{
			break;
		}

		my_sleep(1);
	}

	proc_exit(0);
}

bool Walsender_insert_data_replication(int DATA_TIMES,std::string file_site)
{
	printf("replication server working...\n");

	try
	{
		ColumnInfo heap_colinfo;
		form_heap_colinfo(heap_colinfo);
		setColumnInfo(1, &heap_colinfo);

		boost::thread_group tg;
		tg.create_thread(boost::bind(&thread_handle));
		tg.join_all();

		/* 主机操作数据结束，向各个receiver报告结果 */
		server_mark_result();

		return true;

	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}	
	return true;
}

bool Walsender_update_data_replication()
{
	printf("replication server working...\n");
	try
	{
		int RELID = 10000;
		ColumnInfo heap_colinfo;
		form_heap_colinfo(heap_colinfo);
		setColumnInfo(1, &heap_colinfo);

		begin_first_transaction();

		Oid relspace = MyDatabaseTableSpace;
		FDPG_Heap::fd_heap_create(relspace,RELID);

		Oid reltablespace = MyDatabaseTableSpace;
		Relation testRelation;

		//open heap and insert a tuple
		testRelation = FDPG_Heap::fd_heap_open(RELID,RowExclusiveLock, MyDatabaseId);

		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));		

		FDPG_Heap::fd_simple_heap_insert(testRelation, tup);

		pStorageEngine->endStatement();

		ItemPointerData ipd;

		int sta = 0;
		int count = 0;
		ipd = findTuple(test_insert_data_one_row, testRelation, sta, count);
		HeapTuple tuple_new = fdxdb_heap_formtuple("new_data", sizeof("new_data"));
		FDPG_Heap::fd_simple_heap_update(testRelation, &ipd, tuple_new);

		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);

		FDPG_Heap::fd_heap_drop(RELID, MyDatabaseId);
		commit_transaction();

	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}	
	return true;
}

bool Walsender_delete_data_replication()
{
	printf("replication server working...\n");
	try 
	{
		int RELID = 10000;
		ColumnInfo heap_colinfo;
		form_heap_colinfo(heap_colinfo);
		setColumnInfo(1, &heap_colinfo);

		begin_first_transaction();

		Oid relspace = MyDatabaseTableSpace;
		FDPG_Heap::fd_heap_create(relspace,RELID);

		Oid reltablespace = MyDatabaseTableSpace;
		Relation testRelation;

		//open heap and insert a tuple
		testRelation = FDPG_Heap::fd_heap_open(RELID,RowExclusiveLock, MyDatabaseId);

		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));		

		while (1)
		{
			FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
		}

		pStorageEngine->endStatement();

		ItemPointerData ipd;
		int sta = 0;
		int count = 0;
		ipd = findTuple(test_insert_data_one_row, testRelation, sta, count);
		FDPG_Heap::fd_simple_heap_delete(testRelation, &ipd);

		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);

		FDPG_Heap::fd_heap_drop(RELID, MyDatabaseId);
		commit_transaction();

	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}	
	return true;
}

bool  Walsender_insert_data_basebackup(std::string file_site)
{
	printf("basebackup server working...\n");

	FILE *fp;
	const char *p=file_site.c_str();
	fp = fopen(p,"w+");
	fprintf(fp,"%d",1);//print flag for walsender ready check
	fclose(fp);

	std::string bk_ok_file = file_site;
	std::string::size_type pos = bk_ok_file.rfind("/");
	bk_ok_file.erase(pos);
	bk_ok_file.append(base_backup_ok_file);

	while(1)
	{
		if(bf::exists(bk_ok_file))
		{
			remove(bk_ok_file.c_str());
			return true;
		}
		my_sleep(1);
	}

	return true;
}

//void MySleep(long millsec)
//{
//    boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(millsec));
//}
