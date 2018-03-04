#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <stdio.h>
#include <malloc.h>
#include <vector>
#include <algorithm>
#include <exception>
#include <fstream>

#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"

#include "utils/util.h"
#include "meta/meta_test.h"
#include "test_fram.h"
#include "catalog/metaData.h"
#include "catalog/catalog.h"
#include "catalog/xdb_catalog.h"
#include "commands/tablespace.h"
#include "Macros.h"

#include "storage/large_object.h"

#define MYBLOCKSIZE 8192

void simple_create_tblspc(//BackendParameters *param,
													const string spcname,
													const string dirname,
													bool *retval);
static
void simple_drop_tblspc(const string spcname,
												const string dirname);

static 
int write_lo(File fd, Oid *lo_id, char *lo_name)
{
	begin_transaction();

	char *extraData = "myname";
	*lo_id = inv_create(lo_name, strlen("myname")+1, (void*)extraData);
	Oid tmp_lo_id = name_get_loid(lo_name);

	Assert((*lo_id) == tmp_lo_id);

	/* 0x00020000Ϊд�����ı��λ */
	LargeObjectDesc *lod = inv_open(*lo_id, 0x00020000, TopMemoryContext);
	char buf[MYBLOCKSIZE];
	int nbytes = 0;
	int tmp = 0;
	uint64 countSize = 0; /* ��¼һ��д������ֽڵ����� */

	time_t t1 = time(NULL);

	while ((nbytes = FileRead(fd, buf, MYBLOCKSIZE)) > 0)
	{
		tmp = inv_write(lod, buf, nbytes);
		countSize += tmp;
		Assert(tmp == nbytes);
	}

	time_t t2 = time(NULL);

	inv_close(lod);
	commit_transaction();

	std::cout << "д�� " << countSize	<< " �ֽں�ʱ " << difftime(t2, t1) << " ��.\n";

	return countSize;
}

static
int read_lo(File fd, Oid lo_id)
{
	begin_transaction();

	extern void PushActiveSnapshot(Snapshot snap);
	extern void PopActiveSnapshot();
	extern Snapshot GetTransactionSnapshot();

	PushActiveSnapshot(SnapshotNow);
	/* 0x00040000Ϊ�������ı��λ */
	LargeObjectDesc *lod = inv_open(lo_id, 0x00040000, TopMemoryContext);
	char buf[MYBLOCKSIZE];
	int nbytes = 0;
	int tmp = 0;
	uint64 countSize = 0; /* ��¼һ�����������ֽڵ����� */

	time_t t1 = time(NULL);

	while((nbytes = inv_read(lod, buf, sizeof(buf))) > 0)
	{
		tmp = FileWrite(fd, buf, nbytes);
		countSize += tmp;
		Assert(tmp == nbytes);
	}

	time_t t2 = time(NULL);

	FileClose(fd);
	inv_close(lod);
	PopActiveSnapshot();
	commit_transaction();

	std::cout << "���� " << countSize	<< " �ֽں�ʱ " << difftime(t2, t1) << " ��.\n";

	return countSize;
}

static
void truncate_lo(const Oid lo_id, uint64 len)
{
	begin_transaction();
	extern void PushActiveSnapshot(Snapshot snap);
	extern void PopActiveSnapshot();
	extern Snapshot GetTransactionSnapshot();
	LargeObjectDesc *lod = inv_open(lo_id, 0x00020000, TopMemoryContext);

	time_t t1 = time(NULL);

	/* 
	*truncate�ô�����size��һ����С����ֵȻ����ȥ��ȡ
	*�ô�������ֵ���Ƚ�ǰ������ʱ����ȷ��
	*/
	inv_truncate(lod, (int64)len);//�Ƴ�60%������

	time_t t2 = time(NULL);

	inv_close(lod);
	commit_transaction();

	std::cout << "truncate������ " << len	<< " ��С��ʱ " << difftime(t2, t1) << " ��.\n";
}

static
void drop_lo(const Oid lo_id, bool &return_sta)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void PushActiveSnapshot(SnapshotData*);
	extern void PopActiveSnapshot();
	/* ɾ���ô���� */
	begin_transaction();
	inv_drop(lo_id);
	commit_transaction();

	/* ����Ƿ���ȷɾ�� */
	begin_transaction();
	PushActiveSnapshot(SnapshotNow);

	if(lo_exit(lo_id)) {
		return_sta = false;
	}

	LargeObjectDesc *lod = (LargeObjectDesc *) MemoryContextAlloc(TopMemoryContext,
		sizeof(LargeObjectDesc));

	extern Snapshot RegisterSnapshotOnOwner(Snapshot snapshot, ResourceOwner owner);
	extern Snapshot GetActiveSnapshot();
	lod->id = lo_id;
	lod->subid = GetCurrentSubTransactionId();
	lod->offset = 0;
	lod->snapshot = RegisterSnapshotOnOwner(GetActiveSnapshot(),
		TopTransactionResourceOwner);
	lod->flags = IFS_RDLOCK;

	int nbytes = 0;
	char buf[MYBLOCKSIZE];
	PG_TRY();
	{
		/* �˴��ڶ�ȡ��ʱ����Ҳ��������id�Ӷ��׳�һ���쳣 */
		if((nbytes = inv_read(lod, buf, sizeof(buf))) > 0)
		{
			return_sta = false;
		}
		return_sta = false;
	}
	PG_CATCH();
	{
		return_sta = true;
	}
	PG_END_TRY();
	inv_close(lod);
	PopActiveSnapshot();
	commit_transaction();
}

bool test_simple_large_object()
{
	SHUTDOWN_TEST_STEP_1(LTTest)
	{
		INTENT("���Լ򵥼���һ��������ļ�Ȼ���ȡ������"
			   "���غͶ�ȡ�Ƿ���ȷ����ȷ������ǰ��һ�¡�");

		using namespace std;

		bool return_sta = true;
		Oid lo_id = 0;
		Oid countWSize = 0, countRSize = 0;

		PG_TRY(); {
			string arg = get_my_arg("test_simple_large_object");
			if(arg.length() <= 0)
			{
				cout << "�����������ȷ!\n";
				return true;
			}
			File fd;
			/* ���ش������ļ�����·���������в��������� */
			fd = PathNameOpenFile(const_cast<char*>(arg.c_str()), O_RDONLY | PG_BINARY, S_IRWXU);
			if(fd < 0)
			{
				cout << "���ļ�ʧ�ܣ�(" << arg << ")\n";
				return false;
			}
			countWSize = write_lo(fd, &lo_id, "my_large_object");
			FileClose(fd);
			/*
			* ���������������һ����ȡ�ô��������
			*/

			string arg2 = arg;
			arg2.append(".1");

			fd = PathNameOpenFile(const_cast<char*>(arg2.c_str()), O_CREAT | O_WRONLY | O_TRUNC | PG_BINARY,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			
			if(fd < 0)
			{
				cout << "�����ļ�ʧ�ܣ�(" << arg << ")\n";
				return false;
			}
			
			countRSize = read_lo(fd, lo_id);

			if(countRSize != countWSize)
			{
				return_sta = false;
			}

			truncate_lo(lo_id, countWSize * 0.4);

			string arg3 = arg;
			arg3.append(".2");
			fd = PathNameOpenFile(const_cast<char*>(arg3.c_str()), O_CREAT | O_WRONLY | O_TRUNC | PG_BINARY,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if(fd < 0)
			{
				cout << "�����ļ�ʧ�ܣ�(" << arg << ")\n";
				remove(arg2.c_str());
				return false;
			}
			uint64 countTSize = 0;
			countTSize = read_lo(fd, lo_id);
			if(countTSize != (uint64)(countWSize * 0.4))
			{
				return_sta = false;
			}

			/* �Ƚ������ļ������������Ƿ�һ�� */
			File cmp1 = PathNameOpenFile(const_cast<char*>(arg.c_str()),  O_RDONLY | PG_BINARY, S_IRWXU);
			File cmp2 = PathNameOpenFile(const_cast<char*>(arg2.c_str()),  O_RDONLY | PG_BINARY, S_IRWXU);
			File cmp3 = PathNameOpenFile(const_cast<char*>(arg3.c_str()),  O_RDONLY | PG_BINARY, S_IRWXU);
			if(cmp1 < 0 || cmp2 < 0 || cmp3 < 0)
			{
				cout << "���ļ�ʧ�ܣ�\n";
				remove(arg2.c_str());
				remove(arg3.c_str());
				return false;
			}

			char buf1[MYBLOCKSIZE], buf2[MYBLOCKSIZE], buf3[MYBLOCKSIZE];
			int nbytes1 = 0, nbytes2 = 0, nbytes3 = 0;
			uint64 readSize = 0;
			while (((nbytes1 = FileRead(cmp1, buf1, MYBLOCKSIZE)) > 0) && 
				((nbytes2 = FileRead(cmp2, buf2, MYBLOCKSIZE)) > 0) &&
				((nbytes3 = FileRead(cmp3, buf3, MYBLOCKSIZE)) > 0))
			{
				/* �Ƚ�����д��Ͷ�ȡ�Ĵ���� */
				if(nbytes1 != nbytes2)
				{
					return_sta = false;
					break;
				}
				if(memcmp(buf1, buf2, nbytes1))
				{
					return_sta = false;
					break;
				}

				/* �Ƚ�������ȡ��truncate��Ĵ���� */
				if(readSize <= countTSize)
				{
					readSize += nbytes3;
					if(memcmp(buf1, buf3, nbytes3))
					{
						return_sta = false;
						break;
					}
				}
			}
			if(readSize != countTSize)
			{
				return_sta = false;
			}
			FileClose(cmp1);
			FileClose(cmp2);
			FileClose(cmp3);

			/* ����ɾ������� */
			drop_lo(lo_id, return_sta);

			remove(arg2.c_str());
			remove(arg3.c_str());
		} PG_CATCH(); {
			return_sta = false;
		} PG_END_TRY();
		return return_sta;
	}
	SHUTDOWN_TEST_STEP_2()
		CLEAN_SHUTDOWN_TEST();
	/* Do nothing... */
	return true;
	END_SHUTDOWN_TEST()
}

static
bool cmp_file(std::string path1, std::string path2, uint64 len)
{
	using namespace std;

	bool return_sta = true;

	ifstream in1(path1.c_str(), ios_base::binary|ios_base::app), 
		in2(path2.c_str(), ios_base::binary|ios_base::app);
	const uint64 BUFSIZE = 15000; 
	char *buf1 = new char[BUFSIZE];
	char *buf2 = new char[BUFSIZE];
	uint64 countSize = 0;
	uint64 readSize = 0;

	while(true)
	{
		if(len == countSize)
		{
			break;
		}
		else if((len - countSize) < BUFSIZE)
		{
			readSize = len - countSize;
		}else
		{
			readSize = BUFSIZE;
		}
		in1.read(buf1, readSize);
		in2.read(buf2, readSize);
		if(memcmp(buf1, buf2, readSize))
		{
			return_sta = false;
			break;
		}
		countSize += readSize;
	}
	in1.close();
	in2.close();
	delete[] buf1;
	delete[] buf2;

	return return_sta;
}

static 
void thread_write_read_truncate_drop_lo(void *param, std::vector<std::string> *v_path, bool *sta)
{
	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(param);

	using namespace std;
	using namespace FounderXDB::StorageEngineNS;

	*sta = true;
	Oid lo_id = 0;
	srand(time(NULL));
	int pos = (rand() % v_path->size());
	string my_path = v_path->at(pos);
	File fd = PathNameOpenFile(const_cast<char*>(my_path.c_str()), O_RDONLY | PG_BINARY, S_IRWXU);;
	if(fd < 0)
	{
		cout << "���ļ�" << my_path << "ʧ�ܣ�\n";
		*sta = false;
		proc_exit(0);
		return;
	}

	string path1, path2;
	uint64 countWSize = 0, countRSize = 0;

	try
	{
		path1 = my_path;
		char app[10];
		memset(app, 0, sizeof(app));
		sprintf(app, "%d", MyProcPid);
		path1.append(app);
		path1.append(".1");

		countWSize = write_lo(fd, &lo_id, const_cast<char*>(path1.c_str()));

		FileClose(fd);

		fd = PathNameOpenFile(const_cast<char*>(path1.c_str()), O_CREAT | O_WRONLY | O_TRUNC | PG_BINARY,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		Assert(fd > 0);

		countRSize = read_lo(fd, lo_id);


		path2 = path1;
		path2.append(".2");
		fd = PathNameOpenFile(const_cast<char*>(path2.c_str()), O_CREAT | O_WRONLY | O_TRUNC | PG_BINARY,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		Assert(fd > 0);

		//ɾ��60%������
		truncate_lo(lo_id, countWSize * 0.4);
		read_lo(fd, lo_id);

		/* �ȽϽ�� */
		ifstream in1(path1.c_str()), in2(path2.c_str());
		in1.seekg(0, ios_base::end);
		uint64 len1 = in1.tellg();
		in1.close();
		in2.seekg(0, ios_base::end);
		uint64 len2 = in2.tellg();
		in2.close();
		*sta = cmp_file(my_path, path1, len1);
		if(*sta)
		{
			*sta = cmp_file(my_path, path2, len2);
		}

		if(*sta)
		{
			drop_lo(lo_id, *sta);
		}

	} catch (StorageEngineExceptionUniversal &se)
	{
		cout << se.getErrorMsg();
		*sta = false;
	}

	remove(path2.c_str());
	remove(path1.c_str());
	proc_exit(0);
}

static 
void thread_create_tablespcN(std::vector<std::string> *v_tblspc, bool *sta, BackendParameters *GET_PARAM())
{
	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	for(int i = 0; i < v_tblspc->size(); ++i)
	{
		simple_create_tblspc(v_tblspc->operator[](i), v_tblspc->operator[](i), &sta[i]);
	}

	proc_exit(0);
}

void thread_drop_tablespcN(std::vector<std::string> *v_tblspc, bool *sta, BackendParameters *GET_PARAM())
{
	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	int tmp_sta = *sta;

	using namespace FounderXDB::StorageEngineNS;


	/* ɾ�����б�ռ� */
	for(int i = 0; i < v_tblspc->size(); ++i)
	{
		int try_num = 3;
		do {
			/* ��ೢ��3��ȥɾ����ռ� */
			try
			{
				simple_drop_tblspc(v_tblspc->operator[](i), v_tblspc->operator[](i));
				try_num = 0;
				*sta = tmp_sta;
			} catch(StorageEngineExceptionUniversal &se)
			{
				*sta = false;
				std::cout << se.getErrorMsg() << std::endl;
				user_abort_transaction();
				--try_num;
				/* ˯��5�룬�ȴ������̶߳��ڸ�tblspace�������ͷ� */
				pg_sleep(1000 * 10000);
			}
		} while(try_num > 0);

	}
	

	proc_exit(0);
}

bool test_thread_large_object()
{
	INTENT("���̲߳��Դ����Ĵ�����д�롢��ȡ��truncate�Ƿ�������������");

	using namespace std;
	using namespace boost;
	using namespace FounderXDB::StorageEngineNS;

	string arg = get_my_arg("test_thread_large_object");
	if(arg.length() <= 0)
	{
		cout << "��Ч����!\n";
		return true;
	}

	PREPARE_TEST();

	bool return_sta = true;

	vector<string> v_path;
	string::size_type pos = 0;
	while((pos = arg.find("@", 0)) != string::npos)
	{
		v_path.push_back(arg.substr(0, pos));
		arg = arg.substr(pos + 1);
	}
	v_path.push_back(arg);

	const int THREADS = 5;
	bool *sta = new bool[THREADS];

	vector<string> v_tblspc;
	for(int i = 0; i < THREADS; ++i)
	{
		char dat[100];
		memset(dat, 0, sizeof(dat));
		sprintf(dat, "tablespace%d", i + 1);
		v_tblspc.push_back(dat);
	}

	/* ������ռ� */
	GET_PARAM() = get_param();
	SAVE_PARAM(GET_PARAM());
	GET_THREAD_GROUP().create_thread(bind(thread_create_tablespcN, &v_tblspc, sta, GET_PARAM()));
	GET_THREAD_GROUP().join_all();

#undef setlocale /* ����pg��setlocale������ʹ��CĬ�� */
	setlocale(LC_ALL, "Chinese-simplified");

	srand(time(NULL));
	for(int i = 0; i < THREADS; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(thread_write_read_truncate_drop_lo, 
			GET_PARAM(), &v_path, &sta[i]));
	}
	GET_THREAD_GROUP().join_all();

	setlocale(LC_ALL, "");

	/* ɾ�����б�ռ� */
	GET_PARAM() = get_param();
	SAVE_PARAM(GET_PARAM());
	GET_THREAD_GROUP().create_thread(bind(thread_drop_tablespcN, &v_tblspc, &return_sta, GET_PARAM()));
	GET_THREAD_GROUP().join_all();

	FREE_PARAM(BackendParameters *);

	if(return_sta)
	{
		/* �����, ����������Ӧ��Ϊtrue */
		for(int i = 0; i < THREADS; ++i)
		{
			if(!sta[i])
			{
				return_sta = false;
				break;
			}
		}
	}

	delete []sta;

	return return_sta;
}

static
void simple_drop_tblspc(const string spcname,
												const string dirname)
{
	using namespace FounderXDB::StorageEngineNS;
	using namespace std;

	extern std::string g_strDataDir;

	string path = g_strDataDir;
	path.append("\\");
	path.append(dirname);

	begin_transaction();
	DropTableSpaceStmt stmt;
	stmt.missing_ok = true;
	stmt.tablespacename = const_cast<char*>(spcname.c_str());
	stmt.type = T_Invalid;

	THROW_CALL(DropTableSpace,&stmt);
	commit_transaction();

	rmdir(path.c_str());
}

void simple_create_tblspc(//BackendParameters *param,
													const string spcname,
													const string dirname,
													bool *retval)
{
	//extern void* fxdb_SubPostmaster_Main(void *params);
	//fxdb_SubPostmaster_Main(param);

	using namespace FounderXDB::StorageEngineNS;
	using namespace std;

	extern std::string g_strDataDir;

	Oid spcid = InvalidOid;
	try
	{
		string path = g_strDataDir;
		path.append("\\");
		path.append(dirname);
		rmdir(path.c_str());
#ifdef WIN32
		mkdir(path.c_str());
#else
		mkdir(path.c_str(), S_IWRITE|S_IREAD|S_IRWXU);
#endif //WIN32

		CreateTableSpaceStmt stmt;
		stmt.location = const_cast<char*>(path.c_str());
		stmt.owner = "";
		stmt.tablespacename = const_cast<char*>(spcname.c_str());
		stmt.type = T_Invalid;
		begin_transaction();
		THROW_CALL(spcid = CreateTableSpace, &stmt, true);
		commit_transaction();
	} catch(StorageEngineExceptionUniversal &se) {
		cout << se.getErrorMsg() << endl;
		*retval = false;
	}

	if(spcid != InvalidOid)
	{
		*retval = true;
	} else {
		*retval = false;
	}

	//proc_exit(0);
}