/************************************************************************


测试点列表：
编号		编写者      修改类型      修改时间		修改内容		
000			许彦      创建		  2011.8.3		 测试插入一行数据是否正确
001			许彦	  创建		  2011.8.4		 测试插入多行数据是否正确
002			许彦	  创建		  2011.8.4		 测试插入数据长度较大时是否正确

003			许彦	  创建		  2011.8.4		 测试当heap没有打开时，插入数据是否正确(暂时不测)
004			许彦	  创建		  2011.8.4		 测试当heap不存在时，插入数据是否正确(暂时不测)

005			许彦	  创建		  2011.8.8		 测试插入数据中包含空串的情况
006			许彦	  创建		  2011.8.8		 formtuple时设置数据长度参数为小于或大于数据实际长度，测试插入数据是否正确
007			许彦	  创建		  2011.8.16		 测试插入大量数据行是否正确

008			许彦	  创建		  2011.9.14		 测试heap_getnext方法中所用的宏:VARSIZE_ANY(value),VARDATA_ANY(value)返回结果是否正确

010			许彦	  创建		  2011.8.8		 测试插入数据长度为255字节,256字节的不同分支情况(未完成)

011			许彦	  创建		  2011.11.3		 先建2个表，在起2个线程分别去不同的表中插入一条数据
012			许彦	  创建		  2011.11.3		 先建1个表，在起2个线程分别去相同的表中插入一条相同数据，最后在主线程中检查是否正确
013			许彦	  创建		  2011.11.3		 先建10个表，在起10个线程分别去不同的表中插入一条数据
014			许彦	  创建		  2011.11.3		 先建5个表，在起10个线程，每2个线程去相同的表中插入一条数据，最后在主线程中检查是否正确
015			许彦	  创建		  2011.11.3		 先建3个表，在起4个线程，其中2个线程去相同的表中插入一条数据，另外2个线程分别去不同的表中插入数据，最后在主线程中检查是否正确
016			许彦	  创建		  2011.11.3		 测试用线程创建表是否正确，启动一个线程创建3张表，然后在线程中删除这3张表，在主线程中检查表是否删除
017			许彦	  创建		  2011.11.3		 先建1个表，在起2个线程分别去相同的表中插入一些不同数据，最后在主线程中检查是否正确
018			许彦	  创建		  2011.11.3		 先建1个表，在起2个线程分别去相同的表中插入一条较长的数据，最后在主线程中检查是否正确
019			许彦	  创建		  2011.11.7		 测试用2个线程创建同一个ID的表是否正确
020			许彦	  创建		  2011.11.8		 先建1个表，在起50个线程分别去相同的表中插入10条不同数据，最后在主线程中检查是否正确
021			许彦	  创建		  2011.11.8		 先建1个表，在起50个线程分别去相同的表中插入10条大数据，最后在主线程中检查是否正确
022			许彦      创建		  2011.11.18	 先建1个表，在起50个线程分别去相同的表中插入10条不同数据，最后在启多线程并发查询检查结果
023			许彦      创建		  2011.11.30	 一个进程建表并插入一些数据不删表，另一个进程打开表检查结果再删表
024			许彦      创建		  2011.12.1		 一个进程建表并插入一些数据不删表，插入过程可能随机中断掉,另一个进程打开表检查结果再删表
025			许彦      创建		  2011.12.1		 一个进程建表并插入一些数据再删除掉，不删表，另一个进程打开表检查结果再删表
026			许彦      创建		  2011.12.1		 一个进程建表并插入一些数据再删除掉，不删表，删除过程可能随机中断掉,另一个进程打开表检查结果再删表(测例需要修改)
027			许彦      创建		  2011.12.1		 一个进程建表并插入一些数据再更新数据，不删表，另一个进程打开表检查结果再删表
028			许彦      创建		  2011.12.1		 一个进程建表并插入一些数据再更新数据，不删表，更新过程可能随机中断掉,另一个进程打开表检查结果再删表（测例需要修改）

029			许彦      创建		  2012.2.10		 一个进程建表并插入一些数据，插入事务在提交后立即Exit,另一个进程打开表检查结果再删表
030			许彦      创建		  2012.2.10		 一个进程建表并插入一些数据再删除掉，删除事务提交后立即Exit,另一个进程打开表检查结果再删表
031			许彦      创建		  2012.2.10		 一个进程建表并插入一些数据再更新数据，更新事务提交后立即Exit,另一个进程打开表检查结果再删表

************************************************************************/
#include "utils/SafeMap.h"
#include "utils/util.h"
#include <bitset>

#include <boost/thread/mutex.hpp> 
#include <boost/thread/thread.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <vector>
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "utils/fmgroids.h"
#include "access/tuptoaster.h"


#include "test_fram.h"
#include "heap/test_insert.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/utils.h"
#include "DataItem.h"
void on_exit_reset(void);
using namespace std;
using namespace FounderXDB::StorageEngineNS;
extern TupleDesc single_attibute_tupDesc;

int RID = 20000;
int THREAD_RID = 30000;
int BACKEND_ID = 10;

char *my_itoa(int value, char *string, int radix)
{
	char tmp[33];
	char *tp = tmp;
	int i;
	unsigned v;
	int sign;
	char *sp;

	if (radix > 36 || radix <= 1)
	{
		//		__set_errno(EDOM);
		return 0;
	}

	sign = (radix == 10 && value < 0);
	if (sign)
		v = -value;
	else
		v = (unsigned)value;
	while (v || tp == tmp)
	{
		i = v % radix;
		v = v / radix;
		if (i < 10)
			*tp++ = i+'0';
		else
			*tp++ = i + 'a' - 10;
	}

	if (string == 0)
		string = (char *)malloc((tp-tmp)+sign+1);
	sp = string;

	if (sign)
		*sp++ = '-';
	while (tp > tmp)
		*sp++ = *--tp;
	*sp = 0;
	return string;
}

//创建一张表
extern void setColInfo(Oid colid, Colinfo pcol_info);
void createTable(const int rel_id, const Colinfo heap_info)
{
	try
	{
		begin_first_transaction();
//		Oid relid = RID;
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		setColInfo(rel_id,heap_info);
		FDPG_Heap::fd_heap_create(relspace, rel_id, 0, rel_id);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}	
}

void MyCreateTable(int nTables,const int rel_id, const Colinfo heap_info)
{
	try
	{
		begin_first_transaction();
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		for(int rid = rel_id;rid < THREAD_RID+nTables;rid++)
		{
			setColInfo(rid,heap_info);
			FDPG_Heap::fd_heap_create(relspace, rid,MyDatabaseId,rid);
		}		
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}	
}

void threadMyCreateTable(void *GET_PARAM(),int nTables,const int rel_id, const Colinfo heap_info)
{
	fxdb_SubPostmaster_Main(GET_PARAM());
	MyCreateTable(nTables,rel_id,heap_info);
	proc_exit(0);
}


void MyDropTable(int nTables)
{
	try
	{
		begin_transaction();
		for(int rid = THREAD_RID;rid < THREAD_RID+nTables;rid++)
		{
			FDPG_Heap::fd_heap_drop(rid, MyDatabaseId);
		}		
		commit_transaction();
	}	
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}
	THREAD_RID+=nTables;
}

void threadMyDropTable(void *GET_PARAM(),int nTables)
{
	fxdb_SubPostmaster_Main(GET_PARAM());
	MyDropTable(nTables);
	proc_exit(0);
}
extern void prepare_param(BackendParameters *paramArr[],const int nThread);
extern void free_param(BackendParameters *paramArr[],const int nThread);
void thread_create_heap_xy(int nTables,const int rel_id, const Colinfo heap_info)
{
	boost::thread_group tg;
	BackendParameters *GET_PARAM()[1];
	prepare_param(GET_PARAM(),1);
	tg.create_thread(boost::bind(&threadMyCreateTable,GET_PARAM()[0], nTables,rel_id,heap_info));
	tg.join_all();
	free_param(GET_PARAM(),1);
}

void thread_drop_heap(int nTables)
{
	boost::thread_group tg;
	BackendParameters *GET_PARAM()[1];
	prepare_param(GET_PARAM(),1);
	tg.create_thread(boost::bind(&threadMyDropTable,GET_PARAM()[0], nTables));
	tg.join_all();
	free_param(GET_PARAM(),1);
	
}

//删除测试用例创建的表
void dropTable()
{
	try
	{
		begin_transaction();
		Oid relid = RID;
		FDPG_Heap::fd_heap_drop(relid, MyDatabaseId);
		commit_transaction();
		++RID;	
	}	
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}	
}

bool testWalsender()
{
	INTENT("测试插入一行数据是否正确");
	int relid = RID;
//	while(true){
//#ifdef WIN32
//		Sleep(100);
//#else
//		sleep(100);
//#endif
//	}
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);//这里可以随便设置,因为只是简单插入数据，没用到colinfo
		//createTable(relid,heap_info);
		int i=10000;
		while(1)
		{
			begin_transaction();
			Oid reltablespace = MyDatabaseTableSpace;
			Relation testRelation;

			//open heap and insert a tuple
			SAVE_INFO_FOR_DEBUG();
			testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));		
		
			int i=100;
			while(i-->0)
			 FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
		
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			commit_transaction();
		}
			while(true){
#ifdef WIN32
		Sleep(100);
#else
		sleep(100);
#endif
	}
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}	
	dropTable();
	return true;
}
bool testWalreceiver()
{
		while(true){
#ifdef WIN32
		Sleep(100);
#else
		sleep(100);
#endif
	}
	return false;

}
bool test_simple_heap_insert_000()
{
	INTENT("测试插入一行数据是否正确");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);//这里可以随便设置,因为只是简单插入数据，没用到colinfo
		createTable(relid,heap_info);
		setColInfo(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Relation testRelation;

		//open heap and insert a tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));
		if (NULL == tup) 
		{
			printf("heap form tuple is falied\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}
		
		//插入一行
		Oid tupid = FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
		CHECK_BOOL(tupid == 0);
		if(tupid != 0)
		{
			printf("插入一行数据失败！\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select no scankey
		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		HeapTuple tuple;
		char * temp;
		int counter = 0;
		int flag = 1;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
			++counter;
			temp=fxdb_tuple_to_chars(tuple);
			flag = memcmp(temp,test_insert_data_one_row,sizeof(test_insert_data_one_row));
			CHECK_BOOL(flag == 0);
			if(flag != 0)
			{
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				dropTable();
				return false;
			}
			else
			{
				//printf("插入数据为：%s 长度为%d 行数：%d\n",temp,strlen(temp),counter);	
			}			
		}

		CHECK_BOOL(counter == 1);
		if(counter != 1)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}	
	dropTable();
	return true;
}

bool test_simple_heap_insert_001()
{
	INTENT("测试插入多行数据是否正确");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = RID;
		Relation testRelation;

		//open heap and insert a tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));
		if (NULL == tup) 
		{
			printf("heap form tuple is falied\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}
		
		//插入多行
		Oid tupid = InvalidOid;
		for(int i=0;i<rows;i++)
		{
			tupid =	FDPG_Heap::fd_simple_heap_insert(testRelation, tup);		
			//CHECK_BOOL(tupid == 0);
			if(tupid != 0)
			{
				printf("插入第%d行数据出错！\n",i+1);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				dropTable();
				return false;
			}
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select without scankey
		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		HeapTuple tuple;
		char * temp;
		int counter = 0;
		int flag = 1;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
			++counter;
			temp=fxdb_tuple_to_chars(tuple);
			flag = memcmp(temp,test_insert_data_one_row,sizeof(test_insert_data_one_row));
			//CHECK_BOOL( flag == 0);
			if(flag != 0)
			{
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				dropTable();
				return false;
			}
			else
			{
				//printf("插入数据为：%s 长度为%d 行数：%d\n",temp,strlen(temp),counter);	
			}				
		}

		CHECK_BOOL(counter == rows);
		if(counter != rows)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}	
	dropTable();
	return true;
}

bool test_simple_heap_insert_002()
{
	INTENT("测试插入数据长度较大时是否正确");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace; 
		Oid relid = RID;
		Relation testRelation;

		//open heap and insert a tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_long_data_len_1020, sizeof(test_insert_long_data_len_1020));
		if (NULL == tup) 
		{
			printf("heap form tuple is falied\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}

		//插入一行
		Oid tupid = FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
		CHECK_BOOL(tupid == 0);
		if(tupid != 0)
		{
			printf("插入一行数据失败！\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select no scankey
		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		HeapTuple tuple;
		char * temp;
		int counter = 0;
		int flag = 1;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
			++counter;
			temp=fxdb_tuple_to_chars(tuple);
			flag = memcmp(temp,test_insert_long_data_len_1020,sizeof(test_insert_long_data_len_1020));
			CHECK_BOOL( flag == 0);
			if(flag != 0)
			{
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				dropTable();
				return false;
			}
			else
			{
				//printf("插入数据为：%s 长度为%d 行数：%d\n",temp,strlen(temp),counter);	
			}					
		}

		CHECK_BOOL(counter == 1);
		if(counter != 1)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}	
	dropTable();
	return true;
}

bool test_simple_heap_insert_003()
{
	INTENT("测试当heap没有打开时，插入数据是否正确");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = RID;
		Relation testRelation;

		//open heap and insert a tuple
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		
		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));
		if (NULL == tup) 
		{
			printf("heap form tuple is falied\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			return false;
		}

		//关闭heap，插入一行
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		Oid tupid = FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
		
		//如果插入失败，会返回非0
		CHECK_BOOL(tupid != 0);
		if(tupid == 0)
		{
			printf("向关闭的heap中插入数据成功，测试失败！\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}

		FDPG_Transaction::fd_CommandCounterIncrement();

		//select no scankey
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		HeapTuple tuple;
		char * temp;
		int counter = 0;
		int flag = 1;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
			++counter;
			temp=fxdb_tuple_to_chars(tuple);
			flag = memcmp(temp,test_insert_data_one_row,sizeof(test_insert_data_one_row));
			CHECK_BOOL( flag == 0);
			if(flag != 0)
			{
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				dropTable();
				return false;
			}
			else
			{
				//printf("插入数据为：%s 长度为%d 行数：%d\n",temp,strlen(temp),counter);	
			}			
		}
		CHECK_BOOL(counter == rows);
		if(counter != rows)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			dropTable();
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}	
	dropTable();
	return true;
}

bool test_simple_heap_insert_004()
{
	INTENT("测试当heap不存在时，插入数据是否正确");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = RID;
		Relation testRelation;

		//open heap and insert a tuple
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));
		if (NULL == tup) 
		{
			printf("heap form tuple is falied\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}

		//drop heap以后再插入数据，注意drop之前要先close heap
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Heap::fd_heap_drop(relid, MyDatabaseId);
		Oid tupid = FDPG_Heap::fd_simple_heap_insert(testRelation, tup);

		//如果插入失败，会返回非0
		CHECK_BOOL(tupid != 0);
		if(tupid == 0)
		{
			printf("向不存在的heap中插入数据成功，测试失败！\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}

		FDPG_Transaction::fd_CommandCounterIncrement();

		//select no scankey
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		HeapTuple tuple;
		char * temp;
		int counter = 0;
		int flag = 1;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
			++counter;
			temp=fxdb_tuple_to_chars(tuple);
			flag = memcmp(temp,test_insert_data_one_row,sizeof(test_insert_data_one_row));
			CHECK_BOOL( flag == 0);
			if(flag != 0)
			{
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				dropTable();
				return false;
			}
			else
			{
				//printf("插入数据为：%s 长度为%d 行数：%d\n",temp,strlen(temp),counter);	
			}			
		}
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}	
	dropTable();
	return true;
}

bool test_simple_heap_insert_005()
{
	INTENT("测试插入数据中包含空串的情况");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = RID;
		Relation testRelation;
		HeapTuple tuple;

		//插入一些空串和包含空串的字符串
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		char data[20][20] = {"", "\0\0\0\0\0", "\0\0abcde\0\0", "abc\0\0de\0\0"};
		for(int i=0;i<4;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = FDPG_Heap::fd_heap_form_tuple(data[i], sizeof(data[i]));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		char * temp;
		int flag = 1;

		for(int i=0;i<4;i++)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			CHECK_BOOL(tuple != NULL);
			temp=fxdb_tuple_to_chars(tuple);
			int kk = sizeof(data[i]);
			flag = memcmp(temp,data[i],sizeof(data[i]));
			CHECK_BOOL(flag == 0);
			flag=1;
		}

		//比较数据行数是否一致
		FDPG_Heap::fd_heap_endscan(scan);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		int counter = 0;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
			++counter;		
		}
		
		CHECK_BOOL(counter == 4);
		if(counter != 4)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}	
	dropTable();
	return true;
}

bool test_simple_heap_insert_006()
{
	INTENT("formtuple时设置数据长度参数为小于或大与数据实际长度，测试插入数据是否正确");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = RID;
		Relation testRelation; 

		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		HeapTuple tup;

		//设置len长度为实际长度-10
		
		CHECK_BOOL((tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, strlen(test_insert_data_one_row)-10)) != NULL);
		CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(testRelation, tup) == 0);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//设置len长度为实际长度+10
		const char *mem_before_insert=test_insert_data_one_row;
		char *copymem = (char *)malloc(sizeof(test_insert_data_one_row)+10);
		memcpy(copymem,mem_before_insert,sizeof(test_insert_data_one_row)+10);
		CHECK_BOOL((tup = FDPG_Heap::fd_heap_form_tuple(mem_before_insert, sizeof(test_insert_data_one_row)+10)) != NULL);
		
		CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(testRelation, tup) == 0);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select without scankey
		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		HeapTuple tuple;
		char * temp;
		int flag = 1;

		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		CHECK_BOOL(tuple != NULL);
		temp=fxdb_tuple_to_chars(tuple);
		//插入数据与实际数据的 前len-10个字符进行比较，比较应该相等
		flag = memcmp(temp,test_insert_data_one_row,strlen(test_insert_data_one_row)-10);
		CHECK_BOOL(flag == 0);
		if(flag != 0)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}
		//插入数据与实际数据的 前len-9个字符进行比较，实际数据比插入数据多1个字符，比较应该不相等
		flag = memcmp(temp,test_insert_data_one_row,strlen(test_insert_data_one_row)-9);
		CHECK_BOOL(flag != 0);
		if(flag == 0)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}

		flag = 1;
		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		CHECK_BOOL(tuple != NULL);
		temp=fxdb_tuple_to_chars(tuple);
		//插入数据与实际数据的 前len+10个字符进行比较，比较应该相等
		flag = memcmp(temp,copymem,sizeof(test_insert_data_one_row)+10);
		CHECK_BOOL(flag == 0);
		if(flag != 0)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}
		//插入数据与实际数据的 前len+11个字符进行比较，实际数据比插入数据多1个字符，比较应该不相等
// 		flag = memcmp(temp,copymem,sizeof(test_insert_data_one_row)+11);
// 		CHECK_BOOL(flag != 0);
// 		if(flag == 0)
// 		{
// 			FDPG_Heap::fd_heap_endscan(scan);
// 			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
// 			user_abort_transaction();
// 			return false;
// 		}

//		flag = strcmp(temp,test_insert_data_one_row);
//		CHECK_BOOL(flag == 0);

		//比较数据行数是否一致
		FDPG_Heap::fd_heap_endscan(scan);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		int counter = 0;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
			++counter;		
		}

		CHECK_BOOL(counter == 2);
		if(counter != 2)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}	
	dropTable();
	return true;
}

bool test_simple_heap_insert_007()
{
	INTENT("测试插入大量数据行是否正确");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = RID;
		Relation testRelation;
		HeapTuple tuple;
		//open heap and insert some tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		char insertData[50] = test_insert_data_one_row;
		char copyData[MANYROWS][50]; //存放插入数据，后边查询的时候进行比较
		char string[20];
		int i = 0;
		for(;i < MANYROWS ;i++)
		{
			my_itoa(i,string,10);
			strcat(insertData,string);
			//memcpy(copyData[i],insertData,sizeof(insertData));
			memcpy(copyData[i],insertData,strlen(insertData)+1);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, strlen(insertData)+1);
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);			
			//printf("插入数据为：	%s\n",insertData);
			insertData[strlen(test_insert_data_one_row)] = '\0'; //为了连接初始的字符串，需要在该位置设置一个'\0'
		}

		FDPG_Transaction::fd_CommandCounterIncrement();

		//select without scankey
		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		char * temp;
		int counter = 0;
		int flag = 1;
		i = 0;
		SAVE_INFO_FOR_DEBUG();
		//printf("\n*********下边为查询结果*********\n\n");
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )
		{        
			temp=fxdb_tuple_to_chars(tuple);
			flag = memcmp(temp,copyData[i],strlen(temp)+1);
			if(flag != 0)
			{
				printf("插入数据为：temp=%s	copyData=%s   i=%d\n",temp,copyData[i],i);
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				dropTable();
				return false;
			}
			else
			{
				//printf("查询数据为：	%s 长度为%d 行数：%d\n",temp,strlen(temp),counter);	
			}		
			++i;
			++counter;
		}

		CHECK_BOOL(counter == MANYROWS);
		if(counter != MANYROWS)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}	
	dropTable();
	return true;
}

bool test_simple_heap_insert_008()
{
	INTENT("测试heap_getnext方法中所用的宏:VARSIZE_ANY(value),VARDATA_ANY(value)返回结果是否正确");
	try
	{
		SAVE_INFO_FOR_DEBUG();
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = RID;
		Relation testRelation;

		//open heap and insert a tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		
		int len = sizeof(test_insert_data_one_row);
		char *str = (char *)malloc(len);
		memcpy(str,test_insert_data_one_row,len);
		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row,len);
		if (NULL == tup) 
		{
			printf("heap form tuple is falied\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			return false;
		}

		//插入一行
		Oid tupid = FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
		CHECK_BOOL(tupid == 0);
		if(tupid != 0)
		{
			printf("插入一行数据失败！\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			return false;
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select no scankey
		HeapScanDesc scan;
		HeapTuple tuple;
//		char * temp;
		int counter = 0;
		int flag = 1;
//		EntryID eid;
		Datum value;
		bool isNull;
		int scan_len;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
 			++counter;
			value = heap_getattr(tuple, 1, single_attibute_tupDesc, &isNull);
			flag = memcmp(str,VARDATA_ANY(value),len);
			scan_len = VARSIZE_ANY(value);
			CHECK_BOOL(flag == 0);
			if(flag != 0)
			{
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
 				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
 				user_abort_transaction();
				return false;
			}
			CHECK_BOOL(len == scan_len);
			if(len != scan_len)
			{
				printf("插入数据与查询出来的数据长度不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}		
		}
		CHECK_BOOL(counter == 1);
		if(counter != 1)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
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

void thread_simple_heap_insert_011(void *GET_PARAM(),Oid rid)
{	
	fxdb_SubPostmaster_Main(GET_PARAM());
	int flag = 1;
	INTENT("测试插入一行数据是否正确");
	try
	{
		SAVE_INFO_FOR_DEBUG();
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = rid;
		Relation testRelation;

		//open heap and insert a tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));
		if (NULL == tup) 
		{
			printf("heap form tuple is falied\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
		}

		//插入一行
		Oid tupid = FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
//		CHECK_BOOL(tupid == 0);
		if(tupid != 0)
		{
			printf("插入一行数据失败！\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select no scankey
		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		HeapTuple tuple;
		char * temp;
		int counter = 0;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
			++counter;
			temp=fxdb_tuple_to_chars(tuple);
			flag = memcmp(temp,test_insert_data_one_row,sizeof(test_insert_data_one_row));
//			CHECK_BOOL(flag == 0);
			if(flag != 0)
			{
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
			}
			else
			{
				//printf("插入数据为：%s 长度为%d 行数：%d\n",temp,strlen(temp),counter);	
			}			
		}
		CHECK_BOOL(counter == 1);
		if(counter != 1)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;

	}	
	cout << "flag=" << flag << endl;

	
}

void thread_simple_heap_insert_one_row(void *GET_PARAM(),Oid rid)
{	
	fxdb_SubPostmaster_Main(GET_PARAM());
	INTENT("测试插入一行数据是否正确");
	try
	{
		SAVE_INFO_FOR_DEBUG();
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = rid;
		Relation testRelation;

		//open heap and insert a tuple
		SAVE_INFO_FOR_DEBUG();

		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(test_insert_data_one_row, sizeof(test_insert_data_one_row));
		if (NULL == tup) 
		{
			printf("heap form tuple is falied\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
		}

		//插入一行
		Oid tupid = FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
//		CHECK_BOOL(tupid == 0);
		if(tupid != 0)
		{
			printf("插入一行数据失败！\n");
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
		}

		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}	
	proc_exit(0);
// 	cout << "flag=" << flag << endl;
// 	cout << "main_flag=" << *main_flag << endl;

}



bool check_result(Oid rid,char cmpData[][30],const int dataRows)
{
	try
	{
		begin_transaction();
		HeapScanDesc scan;
		Relation testRelation;
		Oid reltablespace = MyDatabaseTableSpace;
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		HeapTuple tuple;
		char * temp;
		int counter = 0;
		int flag = -1;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
			++counter;
			temp=fxdb_tuple_to_chars(tuple);
			flag = strcmp(temp,cmpData[counter-1]);
//			CHECK_BOOL(flag == 0);
			if(flag != 0)
			{
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			else
			{
				//printf("插入数据为：%s 长度为%d 行数：%d\n",temp,strlen(temp),counter);	
			}			
		}
//		CHECK_BOOL(counter == dataRows);
		if(counter != dataRows)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
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

bool check_long_result(Oid rid,char cmpData[][1024],const int dataRows)
{
	try
	{
		begin_transaction();
		HeapScanDesc scan;
		Relation testRelation;
		Oid reltablespace = MyDatabaseTableSpace;
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		HeapTuple tuple;
		char * temp;
		int counter = 0;
		int flag = -1;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{        
			++counter;
			temp=fxdb_tuple_to_chars(tuple);
			flag = strcmp(temp,cmpData[counter-1]);
//			CHECK_BOOL(flag == 0);
			if(flag != 0)
			{
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			else
			{
				//printf("插入数据为：%s 长度为%d 行数：%d\n",temp,strlen(temp),counter);	
			}			
		}
//		CHECK_BOOL(counter == dataRows);
		if(counter != dataRows)
		{
			printf("插入数据行数与返回行数不相等！\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
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

// bool loop_check_result()
// {
// 
// }

void my_insert_data(Oid rid,char data[][30],const int dataRows)
{
	try
	{
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = rid;
		Relation testRelation;
		HeapTuple tup;

		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		for(int i = 0;i < dataRows; i++)
		{
			tup = FDPG_Heap::fd_heap_form_tuple(data[i], strlen(data[i])+1);
			if (NULL == tup) 
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
			}

			//插入一行
			Oid tupid = FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
//			CHECK_BOOL(tupid == 0);
			if(tupid != 0)
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
			}
		}	

		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}	
}

void my_insert_long_data(Oid rid,char data[][1024],const int dataRows)
{
	try
	{
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = rid;
		Relation testRelation;
		HeapTuple tup;

		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		for(int i = 0;i < dataRows; i++)
		{
			tup = FDPG_Heap::fd_heap_form_tuple(data[i], strlen(data[i])+1);
			if (NULL == tup) 
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
			}

			//插入一行
			Oid tupid = FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
//			CHECK_BOOL(tupid == 0);
			if(tupid != 0)
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
			}
		}	

		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}	
}

void my_insert_large_data(Oid rid,char *pData[],unsigned int dataSize,const int dataRows)
{
	try
	{
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Oid relid = rid;
		Relation testRelation;
		HeapTuple tup;

		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		for(int i = 0;i < dataRows; i++)
		{
			tup = FDPG_Heap::fd_heap_form_tuple(pData[i], dataSize);
			if (NULL == tup) 
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
			}

			//插入一行
			Oid tupid = FDPG_Heap::fd_simple_heap_insert(testRelation, tup);
//			CHECK_BOOL(tupid == 0);
			if(tupid != 0)
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
			}
		}	

		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}	
}



void prepare_param(BackendParameters *paramArr[],const int nThread)
{
	for(int i = 0;i < nThread;i++)
	{
		paramArr[i] = (BackendParameters *) malloc(sizeof(BackendParameters));
		fxdb_save_thread_variables(paramArr[i],BACKEND_ID++,backend);
	}
}

void free_param(BackendParameters *paramArr[],const int nThread)
{
	for(int i = 0;i < nThread;i++)
	{
		free(paramArr[i]); 
	}
}

#define THREAD_NUMS_2 2
//先建2个表，在起2个线程分别去不同的表中插入一条数据
bool test_thread_simple_heap_insert_011()
{
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(THREAD_NUMS_2,rid,heap_info);
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_2];	
	prepare_param(paramArr,THREAD_NUMS_2);

	//启动线程
	for(int i = 0;i < THREAD_NUMS_2;i++)
	{
		tg.create_thread(boost::bind(&thread_simple_heap_insert_one_row,(void*)paramArr[i],rid+i));
	}

	tg.join_all();
	free_param(paramArr,THREAD_NUMS_2);

	char cmpData[][30] = {test_insert_data_one_row};
	const int dataRows = 1;
	for(int i = 0;i < THREAD_NUMS_2;i++)
	{
		if(check_result(rid,cmpData,dataRows) == false)
		{
			thread_drop_heap(THREAD_NUMS_2);
			return false;
		}
		++rid;
	}
	thread_drop_heap(THREAD_NUMS_2);
    return true;
}



//先建1个表，在起2个线程分别去相同的表中插入一条相同数据，最后在主线程中检查是否正确
bool test_thread_simple_heap_insert_012()
{
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_2];
	prepare_param(paramArr,THREAD_NUMS_2);

	for(int i = 0;i < THREAD_NUMS_2;i++)
	{
		tg.create_thread(boost::bind(&thread_simple_heap_insert_one_row,(void*)paramArr[i],rid));
	}

	tg.join_all();
	free_param(paramArr,THREAD_NUMS_2);

	char cmpData[][30] = {test_insert_data_one_row,test_insert_data_one_row};
	const int dataRows = 2;
	main_flag = check_result(rid,cmpData,dataRows);

	thread_drop_heap(1);
	return main_flag;
}

#define THREAD_NUMS_10 10
//先建10个表，在起10个线程分别去不同的表中插入一条数据
bool test_thread_simple_heap_insert_013()
{
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(THREAD_NUMS_10,rid,heap_info);
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_10];	
	prepare_param(paramArr,THREAD_NUMS_10);

	for(int i = 0;i < THREAD_NUMS_10;i++)
	{
		tg.create_thread(boost::bind(&thread_simple_heap_insert_one_row,(void*)paramArr[i],rid+i));
	}

	tg.join_all();
	free_param(paramArr,THREAD_NUMS_10);

	char cmpData[][30] = {test_insert_data_one_row};
	const int dataRows = 1;
	for(int i = 0;i < THREAD_NUMS_10;i++)
	{
		if(check_result(rid,cmpData,dataRows) == false)
		{
			thread_drop_heap(THREAD_NUMS_10);
			return false;
		}	
		++rid;
	}
	thread_drop_heap(THREAD_NUMS_10);
	return true;
}

//先建5个表，在起10个线程，每2个线程去相同的表中插入一条数据，最后在主线程中检查是否正确
#define TABLE_NUMS_5 5
bool test_thread_simple_heap_insert_014()
{
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(TABLE_NUMS_5,rid,heap_info);
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_10];
	prepare_param(paramArr,THREAD_NUMS_10);

	for(int i = 0;i < THREAD_NUMS_10;i++)
	{
		tg.create_thread(boost::bind(&thread_simple_heap_insert_one_row,(void*)paramArr[i],rid));
		tg.create_thread(boost::bind(&thread_simple_heap_insert_one_row,(void*)paramArr[++i],rid++));
	}

	tg.join_all();
	free_param(paramArr,THREAD_NUMS_10);

	rid = THREAD_RID;
	char cmpData[][30] = {test_insert_data_one_row,test_insert_data_one_row};
	const int dataRows = 2;
	for(int i = 0;i < TABLE_NUMS_5;i++)
	{
		if(check_result(rid,cmpData,dataRows) == false)
		{
			thread_drop_heap(TABLE_NUMS_5);
			return false;
		}
		++rid;
	}
	
	thread_drop_heap(TABLE_NUMS_5);
	return true;;
}

//先建3个表，在起4个线程，其中2个线程去相同的表中插入一条数据，另外2个线程分别去不同的表中插入数据，最后在主线程中检查是否正确
#define TABLE_NUMS_3 3
#define THREAD_NUMS_4 4
bool test_thread_simple_heap_insert_015()
{
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(TABLE_NUMS_3,rid,heap_info);
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_4];
	prepare_param(paramArr,THREAD_NUMS_4);

	int i = 0;
	tg.create_thread(boost::bind(&thread_simple_heap_insert_one_row,(void*)paramArr[i++],rid));//相同heap中插入
	tg.create_thread(boost::bind(&thread_simple_heap_insert_one_row,(void*)paramArr[i++],rid));//相同heap中插入
	tg.create_thread(boost::bind(&thread_simple_heap_insert_one_row,(void*)paramArr[i++],++rid));//不同heap中插入
	tg.create_thread(boost::bind(&thread_simple_heap_insert_one_row,(void*)paramArr[i],++rid));//不同heap中插入
	tg.join_all();
	free_param(paramArr,THREAD_NUMS_4);

	rid = THREAD_RID;
	char cmpData[][30] = {test_insert_data_one_row,test_insert_data_one_row};
	const int dataRows = 2;	
	if(check_result(rid,cmpData,dataRows) == false)
	{
		thread_drop_heap(TABLE_NUMS_3);
		return false;
	}

	char cmpData2[][30] = {test_insert_data_one_row};
	const int dataRows2 = 1;
	for(int i = 0;i < 2; i++)
	{
		++rid;
		if(check_result(rid,cmpData2,dataRows2) == false)
		{
			thread_drop_heap(TABLE_NUMS_3);
			return false;
		}
	}
	
	thread_drop_heap(TABLE_NUMS_3);
	return true;;
}

void thread_create_table(void *GET_PARAM(),const int rel_id, const Colinfo heap_info)
{
	fxdb_SubPostmaster_Main(GET_PARAM());
	INTENT("测试用线程创建表是否正确");
	try
	{
		SAVE_INFO_FOR_DEBUG();
		MyCreateTable(TABLE_NUMS_3,rel_id,heap_info);
		MyDropTable(TABLE_NUMS_3);
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}	
	proc_exit(0);
}

//测试用线程创建表是否正确，启动一个线程创建3张表，然后在线程中删除这3张表，在主线程中检查表是否删除
#define THREAD_NUMS_1 1
bool test_thread_create_heap_016()
{
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_1];
	prepare_param(paramArr,THREAD_NUMS_1);

	for(int i = 0;i < THREAD_NUMS_1;i++)
	{
		tg.create_thread(boost::bind(&thread_create_table,(void*)paramArr[i],rid, heap_info));
	}
	
	tg.join_all();
	free_param(paramArr,THREAD_NUMS_1);

	try
	{
		Relation testRelation;
		for(int i = 0;i < TABLE_NUMS_3;i++)
		{	
			testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock, MyDatabaseId);
			//如果表已经被正确删除，open的时候会返回NULL
			if(testRelation != NULL)
			{
				MyDropTable(TABLE_NUMS_3);
				return false;
			}
			++rid;
		}
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}	
	return true;;
}

void thread_heap_insert_017(void *GET_PARAM(),Oid rid,char data[][30],const int dataRows)
{	
	fxdb_SubPostmaster_Main(GET_PARAM());
	my_insert_data(rid,data,dataRows);
	proc_exit(0);
}

//先建1个表，在起2个线程分别去相同的表中插入一些不同数据，最后在主线程中检查是否正确
bool test_thread_simple_heap_insert_017()
{
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_2];
	prepare_param(paramArr,THREAD_NUMS_2);

	int i = 0;
	char data1[][30] = {"test_data_1","test_data_2","test_data_3"};
	char data2[][30] = {"test_data_4","test_data_5","test_data_6"};
	const int dataRows = 3;

	tg.create_thread(boost::bind(&thread_heap_insert_017,(void*)paramArr[i++],rid,data1,dataRows));
	tg.create_thread(boost::bind(&thread_heap_insert_017,(void*)paramArr[i],rid,data2,dataRows));

	tg.join_all();
	free_param(paramArr,THREAD_NUMS_2);

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	char insert_data[][30] = {"test_data_1","test_data_2","test_data_3","test_data_4","test_data_5","test_data_6"};
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		if(_sta[i] != 1 || _count[i] != 1 )
		{
			main_flag = false;
		}
	}

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != ARRAY_LEN_CALC(insert_data))
	{
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

void thread_heap_insert_018(void *GET_PARAM(),Oid rid,char data[][1024],const int dataRows)
{	
	fxdb_SubPostmaster_Main(GET_PARAM());
	my_insert_long_data(rid,data,dataRows);
	proc_exit(0);
}

//先建1个表，在起2个线程分别去相同的表中插入一条较长的数据，最后在主线程中检查是否正确
bool test_thread_simple_heap_insert_018()
{
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_2];
	prepare_param(paramArr,THREAD_NUMS_2);

	int i = 0;
	char data1[][1024] = {test_insert_long_data_len_1020};
	char data2[][1024] = {test_insert_long_data_len_1020};
	const int dataRows = 1;

	tg.create_thread(boost::bind(&thread_heap_insert_018,(void*)paramArr[i++],rid,data1,dataRows));
	tg.create_thread(boost::bind(&thread_heap_insert_018,(void*)paramArr[i],rid,data2,dataRows));

	tg.join_all();
	free_param(paramArr,THREAD_NUMS_2);

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	char insert_data[][1024] = {test_insert_long_data_len_1020,test_insert_long_data_len_1020};
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		if(_sta[i] != 1 || _count[i] != THREAD_NUMS_2 )
		{
			main_flag = false;
		}
	}

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != ARRAY_LEN_CALC(insert_data))
	{
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

extern void thread_find(const char [][DATA_LEN],
												const Oid,
												const int,
												const BackendParameters *,
												int32 *);

void thread_create_same_table(void *GET_PARAM(),Oid rid,bool *main_flag,bool *ex_flag)
{
	fxdb_SubPostmaster_Main(GET_PARAM());
	INTENT("测试用多个线程创建同一个ID的表是否正确");
	try
	{
		begin_first_transaction();
		Oid relspace = MyDatabaseTableSpace;
		FDPG_Heap::fd_heap_create(relspace, rid);
		FDPG_Transaction::fd_CommandCounterIncrement();
		commit_transaction();
	}

	//如果多个线程创建的表的ID相同，出异常的那个线程会报异常号为33686021的异常
	catch (StorageEngineExceptionUniversal &ex) 
	{	
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		if(ex.getErrorNo() == 33686021)
			*ex_flag = true;
		proc_exit(0);
		return;
	}
	*main_flag = true;
	proc_exit(0);
}

void drop_table(Oid rid)
{
	try
	{
		begin_transaction();	
		FDPG_Heap::fd_heap_drop(rid, MyDatabaseId);			
		commit_transaction();
	}	
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}
}
//测试用多个线程创建同一个ID的表是否正确
bool test_thread_create_heap_019()
{
	Oid rid = 188888;
	bool main_flag = false;
	bool ex_flag = false;
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_2];
	prepare_param(paramArr,THREAD_NUMS_2);

	for(int i = 0;i < THREAD_NUMS_2;i++)
	{
		tg.create_thread(boost::bind(&thread_create_same_table,(void*)paramArr[i],rid,&main_flag,&ex_flag));
	}

	tg.join_all();
	free_param(paramArr,THREAD_NUMS_2);
	drop_table(rid);
	if((main_flag == true) && (ex_flag == true))
		return true;
	else
		return false;
}



//先建1个表，在起50个线程分别去相同的表中插入10条不同数据，最后在主线程中检查是否正确
#define THREAD_NUMS_50 50

extern void thread_insert(const char data[][DATA_LEN], 
						  const int array_len, 
						  const int data_len, 
						  const Oid rel_id, 
						  BackendParameters *GET_PARAM(), 
						  int *sta);
bool test_thread_simple_heap_insert_020()
{
	const int thread_num = 80;
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);

	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1",
		"asdfggg",
		"test_data11111111111111",
		test_insert_long_data_len_1020,
		"!!!aaaaaaaa",
		"$$$$$$$$bbbbbbbbbbbb",
		"!!!@#$#%#$^&*&*()_+",
		"string with space",
		"zxcvbnm,./asdfghjkl;'qwertyuiop[]`1234567890-=",
		"                     "
	};
	
	int sta[thread_num] = {0};
	for(int i = 0;i < thread_num;++i)
	{
		tg.create_thread(boost::bind(&thread_insert, insert_data, ARRAY_LEN_CALC(insert_data),DATA_LEN,rid,paramArr[i],&sta[i]));
	}

	tg.join_all();
	free_param(paramArr,thread_num);

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}
	
	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		if(_sta[i] != 1 || _count[i] != thread_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != ARRAY_LEN_CALC(insert_data)*thread_num)
	{
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

#define DATA_SIZE 40//1024*2
#define DATA_ROWS 1000

void thread_insert_large_data(char *pdataArr[], 
				   const int array_len, 
				   const int data_len, 
				   const Oid rel_id, 
				   BackendParameters *GET_PARAM(), 
				   int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	try
	{
		insert_large_data(pdataArr, array_len, data_len, rel_id);
		*sta = 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*sta = -1;
	}
	proc_exit(0);
}

bool haveSameData(char *pDataArr[],const int data_row,const int data_size)
{
	for(int i = 0; i < data_row; ++i)
	{
		for(int j = i+1; j < data_row; ++j)
		{
			if(memcmp(pDataArr[i],pDataArr[j],data_size) == 0)
				return false;
		}
	}
	return true;
}

bool test_thread_simple_heap_insert_021()
{
	const int thread_num = 80;//THREAD_NUMS_1;
	const int data_row = 20;//DATA_ROWS;
	const int data_size = 100;//1*1024*1024;//DATA_SIZE;
	int nTables = 1;
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(nTables,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);

	char *pDataArr[data_row] = {NULL};
	srand((unsigned) time(NULL)); /*播种子*/ 
	for(int i = 0; i < data_row; ++i)
	{
		pDataArr[i] = make_unique_data(data_size,i);
//		printf("%s\n",pDataArr[i]);
	}
//	while(!haveSameData(pDataArr,data_row,data_size));//随机生成数据有可能产生相同的数据，如果出现这种情况，则重新生成数据

	char string[20];
	my_itoa(data_row,string,10);
	int sta[thread_num] = {0};
	for(int i = 0;i < thread_num;++i)
	{
		tg.create_thread(boost::bind(&thread_insert_large_data, pDataArr,data_row,data_size+strlen(string),rid,paramArr[i],&sta[i] ));
	}

	tg.join_all();
	free_param(paramArr,thread_num);

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[data_row];
	int *_count = new int[data_row];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < data_row; ++j)
		{
			findTuple(pDataArr[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < data_row; ++i)
	{
		if(_sta[i] != 1 || _count[i] != thread_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != data_row*thread_num)
	{
		main_flag = false;
	}
	for(int i = 0; i < data_row; ++i)
	{
		free(pDataArr[i]);
	}
	
	thread_drop_heap(nTables);
	return main_flag;
}

static void threadheap_scan(void *param,int thread_ID,std::map<int,bool> &map_result,const Oid rid,const int data_row,char *pDataArr[], 
							const int thread_num)
{
	fxdb_SubPostmaster_Main(param);

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	bool main_flag = true;
	int *_sta = new int[data_row];
	int *_count = new int[data_row];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < data_row; ++j)
		{
			findTuple(pDataArr[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < data_row; ++i)
	{
		if(_sta[i] != 1 || _count[i] != thread_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != data_row*thread_num)
	{
		main_flag = false;
	}
	{
		//boost::lock_guard<boost::mutex> lock(MAP_MUTEX);
		//map_result.insert(make_pair(thread_ID,main_flag));
		map_insert_safe(thread_ID,map_result,main_flag);
	}
	
	proc_exit(0);
}

bool test_thread_simple_heap_insert_022()
{
	const int thread_num = 20;
	const int data_row = 10;
	const int data_size = 2100;
	int nTables = 1;
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(nTables,rid,heap_info);
//	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);

	char *pDataArr[data_row] = {NULL};
	srand((unsigned) time(NULL)); /*播种子*/ 

	for(int i = 0; i < data_row; ++i)
	{
		pDataArr[i] = make_unique_data(data_size,i);
//		printf("%s\n",pDataArr[i]);
	}

	char string[20];
	my_itoa(data_row,string,10);
	int sta[thread_num] = {0};
	for(int i = 0;i < thread_num;++i)
	{
		tg.create_thread(boost::bind(&thread_insert_large_data, pDataArr,data_row,data_size+strlen(string),rid,paramArr[i],&sta[i] ));
	}

	tg.join_all();
	free_param(paramArr,thread_num);
	
	//并发查询
	std::map<int,bool> map_result;
	prepare_param(paramArr,thread_num);
	for(int i = 0;i < thread_num;++i)
	{
		tg.create_thread(boost::bind(&threadheap_scan, paramArr[i],i+1,boost::ref(map_result),rid,data_row,pDataArr,thread_num ));
	}
	tg.join_all();
	free_param(paramArr,thread_num);
	for(int i = 0; i < data_row; ++i)
	{
		free(pDataArr[i]);
	}

	int found = 0;
	int notfound = 0;
	for(int i = 1; i<=thread_num; ++i)
	{
		if( map_result.count(i)==1 && map_result[i]==true )
			++found;
		else
			++notfound;
	}	

	thread_drop_heap(nTables);
	cout << "查询到正确结果的线程数：  found=" << found << endl;
	cout << "未查询到正确结果的线程数: notfound=" << notfound << endl;
	if(found != thread_num)
		return false;
	return true;
}



#define THREAD_NUM_50 50
#define THREAD_NUM_10 10
#define THREAD_NUM_5 5
#define THREAD_NUM_3 3

void thread_insert(const char data[][DATA_LEN], 
									 const int array_len, 
									 const int data_len, 
									 const Oid rel_id, 
									 BackendParameters *GET_PARAM(), 
									 int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	try
	{
		insert_data(data, array_len, data_len, rel_id);
		*sta = 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*sta = -1;
	}
	proc_exit(0);
}

void thread_transaction_insert(const char data[][DATA_LEN], 
				   const int array_len, 
				   const int data_len, 
				   const Oid rel_id, 
				   BackendParameters *GET_PARAM(), 
				   int *sta,
				   int transaction_num)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	try
	{
		for(int i = 0; i < transaction_num; ++i)
		{
			insert_data(data, array_len, data_len, rel_id);
		}
			
		*sta = 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*sta = -1;
	}
	proc_exit(0);
}

void thread_insert_with_index(const char data[][DATA_LEN], 
															const int data_len,
															const int array_len,  
															const HeapIndexRelation *hir,
															BackendParameters *GET_PARAM(), 
															IndexUniqueCheck iuc)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	using namespace FounderXDB::StorageEngineNS;
	begin_transaction();
	Oid relspace = MyDatabaseTableSpace;
	Relation rel = FDPG_Heap::fd_heap_open(hir->heap_id, RowExclusiveLock, MyDatabaseId);
	//	CHECK_BOOL(rel != NULL);
	HeapTuple tuple = NULL;
	int i = 0;
	try
	{
		for(; i < array_len; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(data[i], data_len);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			CommandCounterIncrement();
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		heap_close(rel, RowExclusiveLock);
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		proc_exit(0);
	}
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
	proc_exit(0);
}

void thread_insert_n(const char data[][DATA_LEN], 
									 const int array_len, 
									 const int data_len, 
									 const Oid rel_id, 
									 BackendParameters *GET_PARAM(), 
									 int *sta)
{
	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	using namespace FounderXDB::StorageEngineNS;

	*sta = 0;
	begin_transaction();
	Oid relspace = MyDatabaseTableSpace;
	Relation rel = NULL;
	try
	{
		rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
		if(rel == NULL)
		{
			user_abort_transaction();
			proc_exit(0);
			return;
		}
		HeapTuple tuple = NULL;
		for(int i = 0; i < array_len; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(data[i], data_len);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			++(*sta);
			CommandCounterIncrement();
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		return;
	}
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
	proc_exit(0);
}

static void wait_for_toast_vacuum_pgstat(Oid relid, Oid before, const int data_len)
{
	extern Oid get_toast_table_id(Oid);

	bool can_return = false;
	Oid toast_id = get_toast_table_id(relid);
	double blo = (double)data_len / TOAST_TUPLE_THRESHOLD;
	int iblo = blo;
	if((double)iblo < blo)
	{
		iblo += 1;
	}
	double tuples = before * iblo;
	while(true)
	{
		pg_sleep(1000 * 1000 * 10);
		begin_transaction();
		HeapTuple tup = fxdb_search_meta_table_copy(toast_id);
		if(!tup)
		{
			continue;
		}
		Form_meta_table classForm = fxdb_get_meta_table_struct(tup);
		if(classForm->reltuples == tuples)
		{
			can_return = true;
		}
		commit_transaction();
		if(can_return)
		{
			break;
		}
	}
}

static void wait_for_vacuum_pgstat(Oid relid, Oid before)
{
	bool can_return = false;
	while(true)
	{
		pg_sleep(1000 * 1000 * 10);
		begin_transaction();
		HeapTuple tup = fxdb_search_meta_table_copy(relid);
		if(!tup)
		{
			continue;
		}
		Form_meta_table classForm = fxdb_get_meta_table_struct(tup);
		if(classForm->reltuples == before)
		{
			can_return = true;
		}
		commit_transaction();
		if(can_return)
		{
			break;
		}
	}
}

void thread_pgstat_vacuum(int *sta, const int delete_row_num, 
													const int delete_thresh, const int data_len)
{
	extern void* fxdb_SubPostmaster_Main(void*);
	BackendParameters *param = (BackendParameters*)malloc(sizeof(BackendParameters));
	param->MyThreadType = backend;
	fxdb_SubPostmaster_Main(param);

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;

	uint32 toast_len = 0;

	clear_heap_id();
	*sta = true;
	Oid rel_id = get_heap_id();
	DataGenerater dg(delete_row_num, data_len);
	dg.dataGenerate();
	try
	{
		begin_transaction();
		/* 创建一张测试表 */
		FDPG_Heap::fd_heap_create(DEFAULTTABLESPACE_OID, rel_id, MyDatabaseId);
		commit_transaction();

		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rel_id, AccessExclusiveLock, NULL);
		/* 插入所有delete_row_num条数据，然后删除delete_row_num条数据。
		* 注意以上两次操作需要在一个事务中完成。
		*/

		/* 插入数据 */
		for(int i = 0; i < delete_row_num; ++i)
		{
			HeapTuple tuple = FDPG_Heap::fd_heap_form_tuple(dg[i], data_len);
			toast_len = tuple->t_len;
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			pfree(tuple);
		}
		CommandCounterIncrement();

		/* 删除数据, 余下若干条不删除*/
		srand(time(NULL));
		vector<string> v_cmp_src, v_cmp_det;
		HeapScanDesc hscan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		HeapTuple tuple = NULL;
		uint32 delete_num = 0;
		while(tuple = FDPG_Heap::fd_heap_getnext(hscan, ForwardScanDirection))
		{
			if((rand() + 1) % delete_thresh == 0)
			{
				char *data = fxdb_tuple_to_chars(tuple);
				v_cmp_src.push_back(data);
				pfree(data);
				continue;
			}
			FDPG_Heap::fd_simple_heap_delete(rel, &(tuple->t_self));
			++delete_num;
		}
		FDPG_Heap::fd_heap_endscan(hscan);
		FDPG_Heap::fd_heap_close(rel, AccessExclusiveLock);

		/* 提交事务，让pgstat和vacuum能看到该表的改动. */
		commit_transaction();

		wait_for_vacuum_pgstat(rel_id, delete_row_num - delete_num);
		if(toast_len > TOAST_TUPLE_THRESHOLD)
		{
			wait_for_toast_vacuum_pgstat(rel_id, delete_row_num - delete_num, toast_len);
		}

		/* vacuum结束，检测结果 */
		begin_transaction();
		rel = FDPG_Heap::fd_heap_open(rel_id, AccessExclusiveLock, NULL);
		hscan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		while(tuple = FDPG_Heap::fd_heap_getnext(hscan, ForwardScanDirection))
		{
			char *data = fxdb_tuple_to_chars(tuple);
			v_cmp_det.push_back(data);
			pfree(data);
		}
		FDPG_Heap::fd_heap_endscan(hscan);
		FDPG_Heap::fd_heap_close(rel, AccessExclusiveLock);
		commit_transaction();

		sort(v_cmp_src.begin(), v_cmp_src.end());
		sort(v_cmp_det.begin(), v_cmp_det.end());

		*sta = (v_cmp_src == v_cmp_det) ? 1 : 0;

		begin_transaction();
		FDPG_Heap::fd_heap_drop(rel_id);
		commit_transaction();

	} catch(StorageEngineExceptionUniversal &se)
	{
		*sta = false;
		user_abort_transaction();
		printf("%d : %s\n", se.getErrorNo(), se.getErrorMsg());
	}

	free(param);
	proc_exit(0);
}

int test_pgstat_vacuum()
{
	INTENT("测试pgstat和vacuum线程是否能正确工作。");

	using namespace boost;
	thread_group tg;

	int return_sta;
	tg.create_thread(bind(&thread_pgstat_vacuum, &return_sta, 50000, 3, DATA_LEN));
	tg.join_all();
	
	return return_sta;
}

void thread_index_pgstat_vacuum(int *sta, const int delete_row_num, 
																const int delete_thresh, const int data_len)
{
#undef INDEX_NUM
#define INDEX_NUM 2

	extern int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2);
	extern void* fxdb_SubPostmaster_Main(void*);
	BackendParameters *param = (BackendParameters*)malloc(sizeof(BackendParameters));
	param->MyThreadType = backend;
	fxdb_SubPostmaster_Main(param);

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;

	unsigned int toast_len = 0;

	clear_all();
	SpliterGenerater sg;
	int col_number[] = {1};
	Colinfo hci = sg.buildHeapColInfo(1, 100);
	CompareCallback ccb[] = {my_compare_str};
	Colinfo ici = sg.buildIndexColInfo(1, col_number, ccb, SpliterGenerater::index_split_to_any<1>);

	*sta = true;
	Oid rel_id = get_heap_id();
	setColInfo(rel_id, hci);
	Oid index_id[INDEX_NUM];
	for(int i = 0; i < INDEX_NUM; ++i)
	{
		index_id[i] = get_index_id();
		setColInfo(index_id[i], ici);
	}

	DataGenerater dg(delete_row_num, data_len);
	dg.dataGenerate();
	try
	{
		begin_transaction();
		/* 创建一张测试表 */
		FDPG_Heap::fd_heap_create(DEFAULTTABLESPACE_OID, rel_id, MyDatabaseId, rel_id);
		commit_transaction();

		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rel_id, AccessExclusiveLock, NULL);
		/* 创建索引表 */
		for(int i = 0; i < INDEX_NUM; ++i)
		{
			FDPG_Index::fd_index_create(rel, BTREE_TYPE, index_id[i], index_id[i]);
		}
		/* 插入所有delete_row_num条数据，然后删除delete_row_num条数据。
		* 注意以上两次操作需要在一个事务中完成。
		*/

		/* 插入数据 */
		for(int i = 0; i < delete_row_num; ++i)
		{
			HeapTuple tuple = FDPG_Heap::fd_heap_form_tuple(dg[i], data_len);
			toast_len = tuple->t_len;
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			pfree(tuple);
		}
		CommandCounterIncrement();

		/* 删除数据, 余下若干条不删除 */
		srand(time(NULL));
		vector<string> v_cmp_src;
		HeapScanDesc hscan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		HeapTuple tuple = NULL;
		unsigned int delete_num = 0;
		while(tuple = FDPG_Heap::fd_heap_getnext(hscan, ForwardScanDirection))
		{
			/* 随即偶数不删除 */
			if((rand() + 1) % delete_thresh == 0)
			{
				char *data = fxdb_tuple_to_chars(tuple);
				v_cmp_src.push_back(data);
				pfree(data);
				continue;
			}
			FDPG_Heap::fd_simple_heap_delete(rel, &(tuple->t_self));
			++delete_num;
		}
		FDPG_Heap::fd_heap_endscan(hscan);
		FDPG_Heap::fd_heap_close(rel, AccessExclusiveLock);

		/* 提交事务，让pgstat和vacuum能看到该表的改动. */
		commit_transaction();

		wait_for_vacuum_pgstat(rel_id, delete_row_num - delete_num);
		if(toast_len > TOAST_TUPLE_THRESHOLD)
		{
			wait_for_toast_vacuum_pgstat(rel_id, delete_row_num - delete_num, toast_len);
		}

		/* vacuum完毕，检测结果 */
		begin_transaction();
		rel = FDPG_Heap::fd_heap_open(rel_id, AccessShareLock);
		Relation index_rel[INDEX_NUM];
		for(int i = 0; i < INDEX_NUM; ++i)
		{
			index_rel[i] = FDPG_Index::fd_index_open(index_id[i], AccessShareLock);	
		}
		vector<string> cmp_arr[INDEX_NUM];
		/* 使用所有索引扫描结果 */
		for(int i = 0; i < INDEX_NUM; ++i)
		{
			IndexScanDesc iscan = FDPG_Index::fd_index_beginscan(rel, index_rel[i], SnapshotNow, 0, NULL);
			FDPG_Index::fd_index_rescan(iscan, NULL, 0, NULL, 0);
			HeapTuple tuple = NULL;
			while(tuple = FDPG_Index::fd_index_getnext(iscan, ForwardScanDirection))
			{
				char *data = fxdb_tuple_to_chars(tuple);
				cmp_arr[i].push_back(data);
				pfree(data);
			}
			FDPG_Index::fd_index_endscan(iscan);
			FDPG_Index::fd_index_close(index_rel[i], AccessShareLock);
		}
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);
		commit_transaction();

		/* 比较结果 */
		sort(v_cmp_src.begin(), v_cmp_src.end());
		for(int i = 0; i < INDEX_NUM; ++i)
		{
			sort(cmp_arr[i].begin(), cmp_arr[i].end());
			*sta = (v_cmp_src == cmp_arr[i]);
			if(*sta != 1)
			{
				break;
			}
		}

		begin_transaction();
		FDPG_Heap::fd_heap_drop(rel_id);
		commit_transaction();

	} catch(StorageEngineExceptionUniversal &se)
	{
		*sta = false;
		user_abort_transaction();
		printf("%d : %s\n", se.getErrorNo(), se.getErrorMsg());
	}

	free(param);
	proc_exit(0);
}

int test_index_pgstat_vacuum()
{
	INTENT("创建索引测试pgstat和vacuum线程是否能正确工作。");

	using namespace boost;
	thread_group tg;

	int return_sta;
	tg.create_thread(bind(&thread_index_pgstat_vacuum, &return_sta, 50000, 9, DATA_LEN));
	tg.join_all();

	return return_sta;
}

int test_toast_pgstat_vacuum()
{
	INTENT("创建toast测试pgstat和vacuum线程是否能正确vacuum toast表");

	using namespace boost;
	thread_group tg;

	int return_sta;
	tg.create_thread(bind(&thread_pgstat_vacuum, &return_sta, 50000, 3, DATA_LEN * 3));
	tg.join_all();

	return return_sta;
}

int test_index_toast_pgstat_vacuum()
{
	INTENT("创建toast并创建索引测试pgstat和vacuum线程是否能正确vacuum toast表");

	using namespace boost;
	thread_group tg;

	int return_sta;
	tg.create_thread(bind(&thread_index_pgstat_vacuum, &return_sta, 50000, 7, DATA_LEN * 3));
	tg.join_all();

	return return_sta;
}

void thread_vacuum_truncate(const int row_num, const int data_len, 
														const int delete_num, int *sta)
{
	extern void* fxdb_SubPostmaster_Main(void*);
	BackendParameters *param = (BackendParameters*)malloc(sizeof(BackendParameters));
	param->MyThreadType = backend;
	fxdb_SubPostmaster_Main(param);

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;

	uint32 toast_len = 0;

	clear_heap_id();
	*sta = true;
	Oid rel_id = get_heap_id();
	DataGenerater dg(row_num, data_len);
	dg.dataGenerate();
	try
	{
		begin_transaction();
		/* 创建一张测试表 */
		FDPG_Heap::fd_heap_create(DEFAULTTABLESPACE_OID, rel_id, MyDatabaseId);
		commit_transaction();

		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rel_id, AccessExclusiveLock, NULL);
		/* 插入所有row_num条数据，然后删除delete_num条数据。
		* 注意以上两次操作需要在一个事务中完成。
		*/

		/* 插入数据 */
		for(int i = 0; i < row_num; ++i)
		{
			HeapTuple tuple = FDPG_Heap::fd_heap_form_tuple(dg[i], data_len);
			toast_len = tuple->t_len;
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			pfree(tuple);
		}
		CommandCounterIncrement();

		/* 删除数据, 余下表中前面的若干块中的数据不删除*/
		srand(time(NULL));
		vector<string> v_cmp_src, v_cmp_det;
		HeapScanDesc hscan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		HeapTuple tuple = NULL;
		uint32 delete_thresh = 0;
		while(tuple = FDPG_Heap::fd_heap_getnext(hscan, ForwardScanDirection))
		{
			if(delete_thresh++ < (row_num - delete_num))
			{
				char *data = fxdb_tuple_to_chars(tuple);
				v_cmp_src.push_back(data);
				pfree(data);
				continue;
			}
			FDPG_Heap::fd_simple_heap_delete(rel, &(tuple->t_self));
		}
		FDPG_Heap::fd_heap_endscan(hscan);
		FDPG_Heap::fd_heap_close(rel, AccessExclusiveLock);

		/* 提交事务. */
		commit_transaction();

		wait_for_vacuum_pgstat(rel_id, row_num - delete_num);
		if(toast_len > TOAST_TUPLE_THRESHOLD)
		{
			wait_for_toast_vacuum_pgstat(rel_id, row_num - delete_num, toast_len);
		}

		/* vacuum结束，检测结果 */
		begin_transaction();
		rel = FDPG_Heap::fd_heap_open(rel_id, AccessExclusiveLock, NULL);
		hscan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		while(tuple = FDPG_Heap::fd_heap_getnext(hscan, ForwardScanDirection))
		{
			char *data = fxdb_tuple_to_chars(tuple);
			v_cmp_det.push_back(data);
			pfree(data);
		}
		FDPG_Heap::fd_heap_endscan(hscan);
		FDPG_Heap::fd_heap_close(rel, AccessExclusiveLock);
		commit_transaction();

		sort(v_cmp_src.begin(), v_cmp_src.end());
		sort(v_cmp_det.begin(), v_cmp_det.end());

		*sta = (v_cmp_src == v_cmp_det) ? 1 : 0;

		begin_transaction();
		FDPG_Heap::fd_heap_drop(rel_id);
		commit_transaction();

	} catch(StorageEngineExceptionUniversal &se)
	{
		*sta = false;
		user_abort_transaction();
		printf("%d : %s\n", se.getErrorNo(), se.getErrorMsg());
	}

	free(param);
	proc_exit(0);
}

int test_vacuum_truncate()
{
	INTENT("测试vacuum的truncate功能是否能正确执行。需要注意的"
				 "是一张表上的truncate触发的条件是vacuum后表的存在数"
				 "据的最后一个块与vacuum之前表的最后一个块之间存在一"
				 "定的距离。");

	using namespace boost;
	thread_group tg;

	int return_sta;
	tg.create_thread(bind(&thread_vacuum_truncate, 50000, DATA_LEN, 30000, &return_sta));
	tg.join_all();

	return return_sta;
}

EXTERN_SIMPLE_FUNCTION

int test_hq_thread_insert_000()
{
	INTENT("创建多个线程往一张表中插入相同的数据。"
				 "测试的正确结果是创建了多少个线程，该"
				 "表中就有多少条相同的数据。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	Colinfo heap_info = NULL;
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(RELID, create_sta);
	}

	char insert_data[][DATA_LEN] = 
	{
		"insert_data_11111111111111111"
	};

	int sta[THREAD_NUM_10] = {0};
	//启动10个线程插入insert_data_11111111111111111
	for(int i = 0; i < THREAD_NUM_10; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(
			bind(
			&thread_insert, 
			insert_data, 
			ARRAY_LEN_CALC(insert_data),
			DATA_LEN,
			RELID,
			GET_PARAM(),
			&sta[i]));
	}
	tg.join_all();

	/*
	* 检测测试结果，表中应该有THREAD_NUM_10条相同的数据
	*/
	int count = 0;
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(RELID,AccessShareLock, MyDatabaseId);

		int found = 0;
		findTuple("insert_data_11111111111111111", rel, found, count);
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		int drop_sta = 0;
		SIMPLE_DROP_HEAP(RELID, drop_sta);
		FREE_PARAM(BackendParameters *);
		return 0;
	}

	int drop_sta = 0;
	SIMPLE_DROP_HEAP(RELID, drop_sta);
	if(count != THREAD_NUM_10)
	{
		FREE_PARAM(BackendParameters *);
		return 0;
	}
	FREE_PARAM(BackendParameters *);
	return 1;
}

int test_hq_thread_insert_001()
{
	INTENT("启动多个线程往多张表中插入数据。"
				 "测试结果正确的依据是所有表中都存在"
				 "正确的、完整的数据。（本测例不存在"
				 "多个线程操作同一张表的情况）");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	/*
	* 创建50张表
	*/
	int create_sta = 0;
	Oid array_id[THREAD_NUM_50] = {0};
	clear_all();
	Colinfo heap_info = NULL;
	for(int i = 0; i < THREAD_NUM_50; ++i)
	{
		array_id[i] = get_heap_id();
		SIMPLE_CREATE_HEAP(array_id[i], create_sta);
	}

	/*
	* 构建每张表的数据（使用相同数据）
	*/
	char data[][DATA_LEN] = 
	{
		"1111111111111111111",
		"222222222",
		"test_data1",
		"1ddffsadd",
		"fdsasdfdsa",
		"cvcvasdfwer",
		"r55&&&&&&&&&&&&",
		"!!!!!!!!!!!!!",
		"****((((&^*",
		"0"
	};

	/*
	* 启动插入线程（50个）
	*/
	int sta[THREAD_NUM_50] = {0};
	for(int i = 0; i < THREAD_NUM_50; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_insert, data, ARRAY_LEN_CALC(data), DATA_LEN, array_id[i], GET_PARAM(), &sta[i]));
	}
	tg.join_all();

	/*
	* 检测结果
	*/
	bool test_success = true;
	//map_rel_sta_count存储的是每张表的每条数据在该表中是否存在和存在的数量
	std::map<int, std::pair<int*, int*> > map_rel_sta_count;
	try
	{
		begin_transaction();
		for(int i = 0; i < THREAD_NUM_50; ++i)
		{
			Relation rel = FDPG_Heap::fd_heap_open(array_id[i], AccessShareLock, MyDatabaseId);
			int *_sta = new int[ARRAY_LEN_CALC(data)];
			int *_count = new int[ARRAY_LEN_CALC(data)];
			for(int j = 0; j < ARRAY_LEN_CALC(data); ++j)
			{
				findTuple(data[j], rel, _sta[j], _count[j]);
			}
			map_rel_sta_count[array_id[i]] = std::pair<int*, int*>(_sta, _count);
			FDPG_Heap::fd_heap_close(rel, AccessShareLock);
		}
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		test_success = false;
	}

	//检测map_rel_sta_count内容
	std::map<int, std::pair<int*, int*> >::iterator begin;
	begin = map_rel_sta_count.begin();
	while(begin != map_rel_sta_count.end())
	{
		for(int i = 0; i < ARRAY_LEN_CALC(data); ++i)
		{
			//所有数据都找到且数量均为1则正确，否者失败
			if(begin->second.first[i] != 1 || begin->second.second[i] != 1)
			{
				test_success = false;
			}
		}
		delete []begin->second.first;
		delete []begin->second.second;
		++begin;
	}

	//统计所有表的元组数量
	int rel_count = 0;
	for(int i = 0; i < THREAD_NUM_50; ++i)
	{
		calc_tuple_num(array_id[i], rel_count);
		if(rel_count != ARRAY_LEN_CALC(data))
		{
			test_success = false;
			break;
		}
		rel_count = 0;
	}

	//删除所有表
	int drop_sta = 0;
	for(int i = 0; i < THREAD_NUM_50; ++i)
	{
		int rel_id =array_id[i];
		SIMPLE_DROP_HEAP(rel_id, drop_sta);
	}

	FREE_PARAM(BackendParameters *);
	return test_success;
}

int test_hq_thread_insert_002()
{
	INTENT("启动若干线程插入数据和若干线程查询数据。"
				 "测试的正确结果是所有插入操作都能完全进行，"
				 "查询线程可能查询到数据也可能没有查询到数据。"
				 "本测试中不涉及多个插入线程操作同一张表。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	clear_heap_id();

	int rel_id[THREAD_NUM_10];
	for(int i = 0; i < THREAD_NUM_10; ++i)
	{
		rel_id[i] = get_heap_id();
	}

	{
		int create_sta = 0;
		Colinfo heap_info = NULL;
		for(int i = 0; i < THREAD_NUM_10; ++i)
		{
			SIMPLE_CREATE_HEAP(rel_id[i], create_sta);
		}
	}

#define ROW 10
	const int LEN = 20;
	int sta = 0;
	std::bitset<THREAD_NUM_10> result(0x0000);
	std::vector<DataGenerater> v_dg;
	for(int i = 0; i < THREAD_NUM_10; ++i)
	{
		DataGenerater dg(ROW, LEN);
		char rel_data[ROW][DATA_LEN];
		dg.dataGenerate();
		dg.dataToDataArray2D(rel_data);
		v_dg.push_back(dg);
		int32 thread_find_[ARRAY_LEN_CALC(rel_data)];
		/*
		*启动多个插入线程
		*/
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_insert, rel_data, ARRAY_LEN_CALC(rel_data), DATA_LEN, rel_id[i], GET_PARAM(), &sta));
		/*
		*	启动多个查询线程
		*/
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_find, rel_data, rel_id[i], ARRAY_LEN_CALC(rel_data), GET_PARAM(), thread_find_));
		tg.join_all();
	}

	for(int i = 0; i < THREAD_NUM_10; ++i)
	{
		char rel_data[ROW][DATA_LEN];
		DataGenerater dg = v_dg[i];
		dg.dataToDataArray2D(rel_data);
		result[i] = check_equal_all(rel_id[i], rel_data, ARRAY_LEN_CALC(rel_data));
	}
#undef ROW

	bool _sta = false;
	CHECK_ALL_TRUE(result, THREAD_NUM_10, _sta);

	int _count[THREAD_NUM_10] = {0};
	for(int i = 0; i < THREAD_NUM_10; ++i)
	{
		calc_tuple_num(rel_id[i], _count[i]);
	}

	{
		int drop_sta = 0;
		for(int i = 0; i < THREAD_NUM_10; ++i)
		{
			SIMPLE_DROP_HEAP(rel_id[i], drop_sta);
		}
	}

	if(_sta == false)
	{
		return false;
	}

	if(sta == true)
	{
		for(int i = 0; i < ARRAY_LEN_CALC(_count); ++i)
		{
			if(_count[i] != 10)
			{
				FREE_PARAM(BackendParameters *);
				return false;
			}
		}
	}
	FREE_PARAM(BackendParameters *);
	return true;
}

//判断是否应该执行当前测试用例
bool isExecTest(std::string command)
{
	char *cl = NULL;
#ifdef WIN32
	cl = GetCommandLine();
#endif
	std::string str(cl);
	int pos = str.rfind(" ");
	std::string str2;
	str2 = str.substr(++pos, str.length() - pos);
// 	cout << str2.c_str() << endl;
// 	cout << command << endl;
	const char *c_str2 = str2.c_str();

	if(strcmp(c_str2,command.c_str()))
	{
		cout << strcmp(c_str2,command.c_str()) << endl;
		return false;
	}	
	else
	{
		return true;
	}	
}

#define LOG_PATH "c:\\storageEngine\\data\\base\\11967\\log.txt"
//#define FILE_NAME "c:\\storageEngine\\data\\base\\11967\\"
#define FILE_NAME g_strDataDir.c_str()
void log_result(int result)
{
	FILE *fp;
	fp = fopen(LOG_PATH,"w");
	char str2[20] = {'\0'};
	my_itoa(result,str2,10);
	fputs(str2,fp);
	fflush(fp);
	fclose(fp);
}

extern std::string g_strDataDir;
void save_result(Oid rid,int result,const char *funcName)
{
	char file_name[200] = {'\0'};
	strcpy(file_name,FILE_NAME);
	strcat(file_name,"\\");
	strcat(file_name,funcName);
	strcat(file_name,".txt");
	FILE *fp;
	fp = fopen(file_name,"w");
	char str2[20] = {'\0'};
	my_itoa(result,str2,10);
	fputs(str2,fp);
	fputs(" ",fp);
	memset(str2,'\0',sizeof(str2));
	my_itoa(rid,str2,10);
	fputs(str2,fp);
	fflush(fp);
	fclose(fp);
}

int read_result(const char *funcName,int &rid_out)
{
	char file_name[200] = {'\0'};
	strcpy(file_name,FILE_NAME);
	strcat(file_name,"\\");
	char func_name[80] = {'\0'};
	memcpy(func_name,funcName,strlen(funcName)-1);
	strcat(file_name,func_name);
	strcat(file_name,"1.txt");
	FILE *fp;
	char str[40] = {'\0'};
	fp = fopen(file_name,"r");
	//获取文件长度
	fseek(fp,0,SEEK_END);
	int len = ftell(fp);
	fseek(fp,0,SEEK_SET);
	fgets(str,len+1,fp);
	fclose(fp);
	remove(file_name);
	char num[20] = {'\0'};
	char rid[20] = {'\0'};
	bool flag = false;//标记空格是否出现过
	int j = 0;
	for(int i = 0; i < len; ++i)
	{
		if(str[i] == ' ')
		{
			flag = true;
			continue;
		}
		if(str[i] != ' ' && flag == false)
		{
			num[i] = str[i];
			continue;
		}
		
		if(str[i] != ' ' && flag == true)
		{
			rid[j] = str[i];
			++j;
		}
	}
	int check_num = atoi(num);
	rid_out = atoi(rid);
	return check_num;
}

#define COMMAND_DML "-c A"
#define COMMAND_CHECK "B"
#define COMMAND_DML_EXIT "C"

bool prepare_test_DML(char *str)
{
	if(isExecTest(str) == false && isExecTest(COMMAND_DML) == false)
	{
		cout << "本次进程不运行此测例" << endl;
		return false;
	}
}

bool prepare_test_CHECK(char *str)
{
	if(isExecTest(str) == false && isExecTest(COMMAND_CHECK) == false)
	{
		cout << "本次进程不运行此测例" << endl;
		return false;
	}
}

bool prepare_test_DML_exit(char *str)
{
	if(isExecTest(str) == false && isExecTest(COMMAND_DML_EXIT) == false)
	{
		cout << "本次进程不运行此测例" << endl;
		return false;
	}
}


bool test_thread_transaction_heap_insert_023_step1()
{
// 	if(isExecTest("1A") == false && isExecTest("A") == false)
// 	{
// 		cout << "本次进程不运行此测例" << endl;
// 		return true;
// 	}

// 	if(prepare_test_DML(__FUNCTION__) == false)
// 		return true;

	const int thread_num = 10;
	const int tranaction_num = 100;
	Oid rid = ++THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);

	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};
	
	int sta[thread_num] = {0};
	
	for(int i = 0;i < thread_num;++i)
	{		
		tg.create_thread(boost::bind(&thread_transaction_insert, insert_data,
			ARRAY_LEN_CALC(insert_data),DATA_LEN,rid,paramArr[i],&sta[i],tranaction_num));
	}

	tg.join_all();
	free_param(paramArr,thread_num);

	//检查插入事务是否成功
	for(int i = 0; i < thread_num; ++i)
	{
		if(sta[i] != 1)
		{
			main_flag = false;
			break;
		}
	}

	save_result(rid,thread_num*tranaction_num,__FUNCTION__);
	return main_flag;
}

bool test_thread_transaction_heap_insert_023_step2()
{
// 	if(isExecTest("1B") == false)
// 	{
// 		cout << "本次进程不运行此测例" << endl;
// 		return true;
// 	}
// 	if(prepare_test_CHECK(__FUNCTION__) == false)
// 		return true;
	int rid = 0;
	int check_num = read_result(__FUNCTION__,rid);
	THREAD_RID = rid;
 	bool main_flag = true;

	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		if(_sta[i] != 1 || _count[i] != check_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != ARRAY_LEN_CALC(insert_data)*check_num)
	{
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

bool test_thread_transaction_heap_insert_023()
{
	/*
	* 准备数据
	*/
	SHUTDOWN_TEST_STEP_1(TransPersistence)
	{
		return test_thread_transaction_heap_insert_023_step1();
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
	CLEAN_SHUTDOWN_TEST();
		return test_thread_transaction_heap_insert_023_step2();
	END_SHUTDOWN_TEST()
}

//插入过程可能随机中断掉
bool test_thread_transaction_heap_insert_024_step1()
{
// 	if(isExecTest("2A") == false)
// 	{
// 		cout << "本次进程不运行此测例" << endl;
// 		return true;
// 	}
//	if(prepare_test_DML_exit(__FUNCTION__) == false)
//		return true;

	const int thread_num = 10;
	const int tranaction_num = 1000;
	Oid rid = ++THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);

	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	int sta[thread_num] = {0};
	srand((unsigned) time(NULL)); /*播种子*/ 
	int random_num = rand() % thread_num + 1;
	cout << random_num << endl;
	int count = 0;
	for(int i = 0;i < thread_num;++i)
	{		
		if( i == random_num)
		{
// 			cout << count << endl;
// 			cout << random_num << endl;
			tg.join_all();
			save_result(rid,count*tranaction_num,__FUNCTION__);
			on_exit_reset();
			exit(0);
		}
		tg.create_thread(boost::bind(&thread_transaction_insert, insert_data,
			ARRAY_LEN_CALC(insert_data),DATA_LEN,rid,paramArr[i],&sta[i],tranaction_num));
		++count;
	}

	tg.join_all();
	free_param(paramArr,thread_num);

	//检查插入事务是否成功
	for(int i = 0; i < random_num; ++i)
	{
		if(sta[i] != 1)
		{
			main_flag = false;
			break;
		}
	}

	save_result(rid,thread_num*tranaction_num,__FUNCTION__);
	return main_flag;
}

bool test_thread_transaction_heap_insert_024_step2()
{
// 	if(isExecTest("2B") == false)
// 	{
// 		cout << "本次进程不运行此测例" << endl;
// 		return true;
// 	}
//	if(prepare_test_CHECK(__FUNCTION__) == false)
//		return true;
	int rid = 0;
	int check_num = read_result(__FUNCTION__,rid);
	THREAD_RID = rid;
	bool main_flag = true;
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		if(_sta[i] != 1 || _count[i] != check_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != ARRAY_LEN_CALC(insert_data)*check_num)
	{
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

bool test_thread_transaction_heap_insert_024()
{
	/*
	* 准备数据
	*/
	SHUTDOWN_TEST_STEP_1(TransExit)
	{
		return test_thread_transaction_heap_insert_024_step1();
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
		CLEAN_SHUTDOWN_TEST();
	return test_thread_transaction_heap_insert_024_step2();
	END_SHUTDOWN_TEST()
}

extern void thread_delete(int *found, const char* det_data, const int det_data_len, BackendParameters *params, const Oid rel_id);



//测试删除
bool test_thread_transaction_heap_insert_025_step1()
{
// 	if(isExecTest("3A") == false)
// 	{
// 		cout << "本次进程不运行此测例" << endl;
// 		return true;
// 	}
//	if(prepare_test_DML(__FUNCTION__) == false)
//		return true;

	const int thread_num = 2;
	const int tranaction_num = 10;
	const int data_row = 1;
	Oid rid = ++THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};
	
	//插入数据
	int sta[thread_num] = {0};
	for(int i = 0;i < thread_num;++i)
	{		
		tg.create_thread(boost::bind(&thread_transaction_insert, insert_data,
			ARRAY_LEN_CALC(insert_data),DATA_LEN,rid,paramArr[i],&sta[i],tranaction_num));
	}
	tg.join_all();
	free_param(paramArr,thread_num);
	
	//删除数据
	int rel_count = 0;
	calc_tuple_num(rid, rel_count);
	while(rel_count)
	{
		BackendParameters *paramArr2[thread_num*tranaction_num];
		prepare_param(paramArr2,thread_num*tranaction_num);
		int found[thread_num*tranaction_num] = {0};
		for(int i = 0; i < thread_num; ++i)
		{
			for(int j = 0; j < tranaction_num; ++j)
			{
				tg.create_thread(boost::bind(&thread_delete, &found[i*tranaction_num+j],
					"test_data_1", strlen("test_data_1"), paramArr2[i*tranaction_num+j], rid));
			}	
		}
		tg.join_all();
		free_param(paramArr2,thread_num*tranaction_num);
		//统计所有表的元组数量
		calc_tuple_num(rid, rel_count);
	}

	save_result(rid,rel_count,__FUNCTION__);
	return main_flag;
}

bool test_thread_transaction_heap_insert_025_step2()
{
// 	if(isExecTest("3B") == false)
// 	{
// 		cout << "本次进程不运行此测例" << endl;
// 		return true;
// 	}
//	if(prepare_test_CHECK(__FUNCTION__) == false)
//		return true;
	int rid = 0;
	int check_num = read_result(__FUNCTION__,rid);
	THREAD_RID = rid;
	bool main_flag = true;
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		cout << "count=" << _count[i] << endl;
		if( _count[i] != check_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != check_num)
	{
		cout << "rel_count" << rel_count << endl;
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

bool test_thread_transaction_heap_insert_025()
{
	/*
	* 准备数据
	*/
	SHUTDOWN_TEST_STEP_1(TransPersistence)
	{
		return test_thread_transaction_heap_insert_025_step1();
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
		CLEAN_SHUTDOWN_TEST();
	return test_thread_transaction_heap_insert_025_step2();
	END_SHUTDOWN_TEST()
}

//测试删除，过程可能随机中断
bool test_thread_transaction_heap_insert_026_step1()
{
	const int thread_num = 2;
	const int tranaction_num = 10;
	const int data_row = 1;
	Oid rid = ++THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//插入数据
	int sta[thread_num] = {0};
	for(int i = 0;i < thread_num;++i)
	{		
		tg.create_thread(boost::bind(&thread_transaction_insert, insert_data,
			ARRAY_LEN_CALC(insert_data),DATA_LEN,rid,paramArr[i],&sta[i],tranaction_num));
	}
	tg.join_all();
	free_param(paramArr,thread_num);

	//随机删除数据
	
	int rel_count = 0;
	calc_tuple_num(rid, rel_count);
	srand((unsigned) time(NULL)); /*播种子*/ 
	int random_num = rand() % rel_count;
	BackendParameters *paramArr2[thread_num*tranaction_num];
	prepare_param(paramArr2,thread_num*tranaction_num);
	int found[thread_num*tranaction_num] = {0};
	while(1)
	{
		if( rel_count <= random_num)
		{
			cout << rel_count << endl;
			cout << random_num << endl;
			tg.join_all();
			free_param(paramArr2,thread_num*tranaction_num);
			save_result(rid,rel_count,__FUNCTION__);
			on_exit_reset();
			exit(0);
		}
		for(int i = 0; i < thread_num; ++i)
		{
			for(int j = 0; j < tranaction_num; ++j)
			{
				tg.create_thread(boost::bind(&thread_delete, &found[i*tranaction_num+j],
					"test_data_1", strlen("test_data_1"), paramArr2[i*tranaction_num+j], rid));
			}	
		}
		tg.join_all();
//		free_param(paramArr2,thread_num*tranaction_num);
		//统计所有表的元组数量
		calc_tuple_num(rid, rel_count);
	}

	save_result(rid,rel_count,__FUNCTION__);
	return main_flag;
}

bool test_thread_transaction_heap_insert_026_step2()
{
	int rid = 0;
	int check_num = read_result(__FUNCTION__,rid);
	THREAD_RID = rid;
	bool main_flag = true;
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		cout << "count=" << _count[i] << endl;
		if( _count[i] != check_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != check_num)
	{
		cout << "rel_count" << rel_count << endl;
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

bool test_thread_transaction_heap_insert_026()
{
	/*
	* 准备数据
	*/
	SHUTDOWN_TEST_STEP_1(TransExit)
	{
		return test_thread_transaction_heap_insert_026_step1();
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
		CLEAN_SHUTDOWN_TEST();
	return test_thread_transaction_heap_insert_026_step2();
	END_SHUTDOWN_TEST()
}

extern void thread_update(const char *src_data,
						  const char *det_data,
						  const unsigned int det_data_len,
						  BackendParameters *params,
						  const Oid rel_id,
						  int *found);

void calc_given_tuple_num(Oid relid, int &count,char *data)
{
	PG_TRY();
	{
		count = 0;
		begin_transaction();
		Relation rel = heap_open(relid,  RowExclusiveLock);
		HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, 0, 0);
		HeapTuple tuple;
		char *tmp_data;
		while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			tmp_data = fxdb_tuple_to_chars(tuple);
			if(memcmp(data, tmp_data, strlen(tmp_data)) == 0)
			{
				++count;
			}
			pfree(tmp_data);	
		}
		heap_endscan(scan);
		heap_close(rel, RowExclusiveLock);
		commit_transaction();
	}
	PG_CATCH();
	{
		count = -1;
	}
	PG_END_TRY();
}

//测试更新
bool test_thread_transaction_heap_insert_027_step1()
{
// 	if(isExecTest("5A") == false)
// 	{
// 		cout << "本次进程不运行此测例" << endl;
// 		return true;
// 	}
//	if(prepare_test_DML(__FUNCTION__) == false)
//		return true;

	const int thread_num = 2;
	const int tranaction_num = 3;
	const int data_row = 1;
	Oid rid = ++THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//插入数据
	int sta[thread_num] = {0};
	for(int i = 0;i < thread_num;++i)
	{		
		tg.create_thread(boost::bind(&thread_transaction_insert, insert_data,
			ARRAY_LEN_CALC(insert_data),DATA_LEN,rid,paramArr[i],&sta[i],tranaction_num));
	}
	tg.join_all();
	cout << "sta[0]=" << sta[0] << endl;
	cout << "sta[1]=" << sta[1] << endl;
	free_param(paramArr,thread_num);

	//更新数据
	int rel_count = 0;
	calc_given_tuple_num(rid, rel_count,"update");
	int found[thread_num*tranaction_num] = {0};
	while(rel_count != thread_num*tranaction_num)
	{
		BackendParameters *paramArr2[thread_num*tranaction_num];
		prepare_param(paramArr2,thread_num*tranaction_num);
		int found[thread_num*tranaction_num] = {0};
		for(int i = 0; i < thread_num; ++i)
		{
			for(int j = 0; j < tranaction_num; ++j)
			{
				tg.create_thread(boost::bind(&thread_update, "test_data_1", "update",
					strlen("update"), paramArr2[i*tranaction_num+j], rid, &found[i*tranaction_num+j]));
			}	
		}
 		tg.join_all();
		free_param(paramArr2,thread_num*tranaction_num);
		//统计指定的元组数量
		calc_given_tuple_num(rid, rel_count,"update");
	}

	save_result(rid,rel_count,__FUNCTION__);
	return main_flag;
}

bool test_thread_transaction_heap_insert_027_step2()
{
// 	if(isExecTest("5B") == false)
// 	{
// 		cout << "本次进程不运行此测例" << endl;
// 		return true;
// 	}
//	if(prepare_test_CHECK(__FUNCTION__) == false)
//		return true;
	int rid = 0;
	int check_num = read_result(__FUNCTION__,rid);
	rid = THREAD_RID;
	bool main_flag = true;
	char insert_data[][DATA_LEN] = 
	{	
		"update"
	};

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		cout << "count=" << _count[i] << endl;
		if( _count[i] != check_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计指定的元组数量
	int rel_count = 0;	
	calc_given_tuple_num(rid, rel_count,"update");
	if(rel_count != check_num)
	{
		cout << "rel_count" << rel_count << endl;
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

bool test_thread_transaction_heap_insert_027()
{
	/*
	* 准备数据
	*/
	SHUTDOWN_TEST_STEP_1(TransPersistence)
	{
		return test_thread_transaction_heap_insert_027_step1();
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
		CLEAN_SHUTDOWN_TEST();
	return test_thread_transaction_heap_insert_027_step2();
	END_SHUTDOWN_TEST()
}

//更新过程可能随机中断
bool test_thread_transaction_heap_insert_028_step1()
{
	const int thread_num = 2;
	const int tranaction_num = 3;
	const int data_row = 1;
	Oid rid = ++THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//插入数据
	int sta[thread_num] = {0};
	for(int i = 0;i < thread_num;++i)
	{		
		tg.create_thread(boost::bind(&thread_transaction_insert, insert_data,
			ARRAY_LEN_CALC(insert_data),DATA_LEN,rid,paramArr[i],&sta[i],tranaction_num));
	}
	tg.join_all();
	free_param(paramArr,thread_num);

	//更新数据，过程可能随机中断
	int rel_count = 0;
	calc_given_tuple_num(rid, rel_count,"update");
	int rel_count2 = 0;
	calc_tuple_num(rid, rel_count2);
	int found[thread_num*tranaction_num] = {0};
	srand((unsigned) time(NULL)); /*播种子*/ 
	int random_num = rand() % rel_count2;
	while(true)
	{
		if( rel_count >= random_num)
		{
			cout << rel_count << endl;
			cout << random_num << endl;
			tg.join_all();
			save_result(rid,rel_count,__FUNCTION__);
			on_exit_reset();
			exit(0);
		}
		BackendParameters *paramArr2[thread_num*tranaction_num];
		prepare_param(paramArr2,thread_num*tranaction_num);
		int found[thread_num*tranaction_num] = {0};
		for(int i = 0; i < thread_num; ++i)
		{
			for(int j = 0; j < tranaction_num; ++j)
			{
				tg.create_thread(boost::bind(&thread_update, "test_data_1", "update",
					strlen("update"), paramArr2[i*tranaction_num+j], rid, &found[i*tranaction_num+j]));
			}	
		}
		tg.join_all();
		free_param(paramArr2,thread_num*tranaction_num);
		//统计指定的元组数量
		calc_given_tuple_num(rid, rel_count,"update");
	}

	save_result(rid,rel_count,__FUNCTION__);
	return main_flag;
}

bool test_thread_transaction_heap_insert_028_step2()
{
// 	if(isExecTest("6B") == false)
// 	{
// 		cout << "本次进程不运行此测例" << endl;
// 		return true;
// 	}
// 	if(prepare_test_CHECK(__FUNCTION__) == false)
// 		return true;
	int rid = 0;
	int check_num = read_result(__FUNCTION__,rid);
	THREAD_RID = rid;
	bool main_flag = true;
	char insert_data[][DATA_LEN] = 
	{	
		"update"
	};

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		cout << "count=" << _count[i] << endl;
		if( _count[i] != check_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计指定的元组数量
	int rel_count = 0;	
	calc_given_tuple_num(rid, rel_count,"update");
	if(rel_count != check_num)
	{
		cout << "rel_count" << rel_count << endl;
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

bool test_thread_transaction_heap_insert_028()
{
	/*
	* 准备数据
	*/
	SHUTDOWN_TEST_STEP_1(TransExit)
	{
		return test_thread_transaction_heap_insert_028_step1();
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
		CLEAN_SHUTDOWN_TEST();
	return test_thread_transaction_heap_insert_028_step2();
	END_SHUTDOWN_TEST()
}


bool testInsertLotsOfData( void )
{
	try
	{
		begin_transaction();
		{
			SimpleHeap heap(OIDGenerator::instance().GetTableSpaceID()
				,OIDGenerator::instance().GetHeapID(),FixSpliter::split,true);

			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);
			srand(time(NULL));
			char pp[1024] = {0};
			for (int i = 0; i < sizeof(pp) - 1;++i)
			{
				pp[i] = 'a' + random() % 26;
			}
			//insert some data in the db
			for (int i = 0 ; i < (1<<15);++i)
			{
				sprintf(pp + 1015,"%d",i);
				heap.Insert(pp);
			}

			CommandCounterIncrement();
		}
		commit_transaction();
	}
	CATCHEXCEPTION

	return true;
}

int TRANSACTION_COMMIT_NUM = 0;
int RANDOM_TRANSACTION_NUM = 0;

void insert_data_exit(const char insert_data[][DATA_LEN], 
				 const int array_len, 
				 const int data_len,
				 const int rel_id)
{
	using namespace FounderXDB::StorageEngineNS;
//	begin_transaction();
	Oid relspace = MyDatabaseTableSpace;
	Relation rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
	HeapTuple tuple = NULL;
	try
	{
		for(int i = 0; i < array_len; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(insert_data[i], data_len);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			CommandCounterIncrement();
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		heap_close(rel, RowExclusiveLock);
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		throw StorageEngineExceptionUniversal(se);
	}
	heap_close(rel, RowExclusiveLock);

	
}

void thread_transaction_insert_exit(const char data[][DATA_LEN], 
							   const int array_len, 
							   const int data_len, 
							   const Oid rel_id, 
							   BackendParameters *GET_PARAM(), 
							   int *sta,
							   int transaction_num)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	try
	{
		begin_transaction();
		for(int i = 0; i < transaction_num; ++i)
		{
			insert_data(data, array_len, data_len, rel_id);
		}
		{
			static boost::mutex mut;
			boost::lock_guard<boost::mutex> lock(mut);
			if(TRANSACTION_COMMIT_NUM < RANDOM_TRANSACTION_NUM)
			{
				commit_transaction();
				++TRANSACTION_COMMIT_NUM;
			}
			if(TRANSACTION_COMMIT_NUM == RANDOM_TRANSACTION_NUM)
			{
				while(1)
				{
#ifdef WIN32
					Sleep(10000);
#else
					//     sleep(10000);
#endif
				}
			}
			if(TRANSACTION_COMMIT_NUM > RANDOM_TRANSACTION_NUM)
				return;
		}
		*sta = 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*sta = -1;
	}
	proc_exit(0);
}

//插入过程可能随机中断掉
bool test_thread_transaction_heap_insert_029_step1()
{
	const int thread_num = 10;
	const int tranaction_num = 1000;
	Oid rid = ++THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);

	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	int sta[thread_num] = {0};
	srand((unsigned) time(NULL)); /*播种子*/ 
	int random_thread_num = rand() % thread_num + 1;

	TRANSACTION_COMMIT_NUM = 0;
	RANDOM_TRANSACTION_NUM = random_thread_num;

	cout << random_thread_num << endl;

	for(int i = 0;i < random_thread_num;++i)
	{		
		tg.create_thread(boost::bind(&thread_transaction_insert_exit, insert_data,
			ARRAY_LEN_CALC(insert_data),DATA_LEN,rid,paramArr[i],&sta[i],tranaction_num));
	}

	while(1)
	{
		if(TRANSACTION_COMMIT_NUM == RANDOM_TRANSACTION_NUM)
		{
			free_param(paramArr,thread_num);
			save_result(rid,TRANSACTION_COMMIT_NUM*tranaction_num,__FUNCTION__);
			on_exit_reset();
			exit(0);
		}
#ifdef WIN32
		Sleep(5000);
#else
		//     sleep(5000);
#endif
		
	}

	return main_flag;
}

bool test_thread_transaction_heap_insert_029_step2()
{
	int rid = 0;
	int check_num = read_result(__FUNCTION__,rid);
	THREAD_RID = rid;
	bool main_flag = true;
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		if(_sta[i] != 1 || _count[i] != check_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != ARRAY_LEN_CALC(insert_data)*check_num)
	{
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

bool test_thread_transaction_heap_insert_029()
{
	/*
	* 准备数据
	*/
	SHUTDOWN_TEST_STEP_1(TransExit)
	{
		return test_thread_transaction_heap_insert_029_step1();
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
		CLEAN_SHUTDOWN_TEST();
	return test_thread_transaction_heap_insert_029_step2();
	END_SHUTDOWN_TEST()
}

void thread_delete_exit(int *found, const char* det_data, const int det_data_len, BackendParameters *params, const Oid rel_id)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(params);
	*found = 0;
	int have=*found ;
	char cmp_data[DATA_LEN];
	memset(cmp_data, 0, sizeof(cmp_data));
	memcpy(cmp_data, det_data, det_data_len);
	try
	{
		begin_transaction();	
		Relation rel = heap_open(rel_id,RowExclusiveLock);
		int count = 0;
		ItemPointerData ipd = findTuple(cmp_data, rel, have, count, strlen(det_data));
		if(have == 1)
		{
			FDPG_Heap::fd_simple_heap_delete(rel, &ipd);

			CommandCounterIncrement();
		}
		heap_close(rel, RowExclusiveLock);	
		*found = 1;
		{
			static boost::mutex mut;
			boost::lock_guard<boost::mutex> lock(mut);
			if(TRANSACTION_COMMIT_NUM < RANDOM_TRANSACTION_NUM)
			{
				commit_transaction();
				++TRANSACTION_COMMIT_NUM;
			}
			if(TRANSACTION_COMMIT_NUM == RANDOM_TRANSACTION_NUM)
			{
				while(1)
				{
#ifdef WIN32
					Sleep(10000);
#else
					//     sleep(10000);
#endif
				}
			}
			if(TRANSACTION_COMMIT_NUM > RANDOM_TRANSACTION_NUM)
				return;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*found = -1;
		user_abort_transaction();
	}
	proc_exit(0);
}

//测试删除，过程可能随机中断
bool test_thread_transaction_heap_insert_030_step1()
{
	const int thread_num = 2;
	const int tranaction_num = 5;
	const int data_row = 1;
	Oid rid = ++THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//插入数据
	int sta[thread_num] = {0};
	for(int i = 0;i < thread_num;++i)
	{		
		tg.create_thread(boost::bind(&thread_transaction_insert, insert_data,
			ARRAY_LEN_CALC(insert_data),DATA_LEN,rid,paramArr[i],&sta[i],tranaction_num));
	}
	tg.join_all();
	free_param(paramArr,thread_num);

	//随机删除数据
	int rel_count = 0;
	calc_tuple_num(rid, rel_count);
	int total_rel = rel_count;
	srand((unsigned) time(NULL)); /*播种子*/ 
	int random_num = rand() % rel_count + 1;//要删除的数据条数

	TRANSACTION_COMMIT_NUM = 0;
	RANDOM_TRANSACTION_NUM = random_num;//一个事务删除一条

	BackendParameters *paramArr2[thread_num*tranaction_num];
	prepare_param(paramArr2,thread_num*tranaction_num);
	int found[thread_num*tranaction_num] = {0};

	while(1)
	{
		for(int i = 0; i < thread_num; ++i)
		{
			for(int j = 0; j < tranaction_num; ++j)
			{
				tg.create_thread(boost::bind(&thread_delete_exit, &found[i*tranaction_num+j],
					"test_data_1", strlen("test_data_1"), paramArr2[i*tranaction_num+j], rid));
			}	
		}
#ifdef WIN32
		Sleep(1000);
#else
		//     sleep(1000);
#endif
		if(TRANSACTION_COMMIT_NUM == RANDOM_TRANSACTION_NUM)
		{
			free_param(paramArr2,thread_num);
			save_result(rid,total_rel - TRANSACTION_COMMIT_NUM,__FUNCTION__);
			on_exit_reset();
			exit(0);
		}
	}
	
	return main_flag;
}

bool test_thread_transaction_heap_insert_030_step2()
{
	int rid = 0;
	int check_num = read_result(__FUNCTION__,rid);
	THREAD_RID = rid;
	bool main_flag = true;
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		cout << "count=" << _count[i] << endl;
		if( _count[i] != check_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计所有表的元组数量
	int rel_count = 0;	
	calc_tuple_num(rid, rel_count);
	if(rel_count != check_num)
	{
		cout << "rel_count" << rel_count << endl;
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

bool test_thread_transaction_heap_insert_030()
{
	/*
	* 准备数据
	*/
	SHUTDOWN_TEST_STEP_1(TransExit)
	{
		return test_thread_transaction_heap_insert_030_step1();
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
		CLEAN_SHUTDOWN_TEST();
	return test_thread_transaction_heap_insert_030_step2();
	END_SHUTDOWN_TEST()
}

void thread_update_exit(const char *src_data,
				   const char *det_data,
				   const unsigned int det_data_len,
				   BackendParameters *params,
				   const Oid rel_id,
				   int *found)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(params);
	*found = 0;
	Relation rel = NULL;
	try
	{
		begin_transaction();
		int count = 0;
		rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
		ItemPointerData ipd = findTuple(src_data, rel, *found, count);
		if(*found == 1)
		{
			HeapTuple tuple = fdxdb_heap_formtuple(det_data, det_data_len);
			FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple);
		}
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		{
			static boost::mutex mut;
			boost::lock_guard<boost::mutex> lock(mut);
			if(TRANSACTION_COMMIT_NUM < RANDOM_TRANSACTION_NUM)
			{
				commit_transaction();
				++TRANSACTION_COMMIT_NUM;
			}
			if(TRANSACTION_COMMIT_NUM == RANDOM_TRANSACTION_NUM)
			{
				while(1)
				{
#ifdef WIN32
					Sleep(10000);
#else
					//     sleep(10000);
#endif
				}
			}
			if(TRANSACTION_COMMIT_NUM > RANDOM_TRANSACTION_NUM)
				return;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*found = -1;
	}

	proc_exit(0);
}

//更新过程可能随机中断
bool test_thread_transaction_heap_insert_031_step1()
{
	const int thread_num = 2;
	const int tranaction_num = 3;
	const int data_row = 1;
	Oid rid = ++THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
	thread_create_heap_xy(1,rid,heap_info);
	bool main_flag = true;
	boost::thread_group tg;
	BackendParameters *paramArr[thread_num];
	prepare_param(paramArr,thread_num);
	char insert_data[][DATA_LEN] = 
	{	
		"test_data_1"
	};

	//插入数据
	int sta[thread_num] = {0};
	for(int i = 0;i < thread_num;++i)
	{		
		tg.create_thread(boost::bind(&thread_transaction_insert, insert_data,
			ARRAY_LEN_CALC(insert_data),DATA_LEN,rid,paramArr[i],&sta[i],tranaction_num));
	}
	tg.join_all();
	free_param(paramArr,thread_num);

	//更新数据，过程可能随机中断
	int rel_count = 0;
	calc_given_tuple_num(rid, rel_count,"update");
	int rel_count2 = 0;
	calc_tuple_num(rid, rel_count2);
	int found[thread_num*tranaction_num] = {0};
	srand((unsigned) time(NULL)); /*播种子*/ 
	int random_num = rand() % rel_count2;

	TRANSACTION_COMMIT_NUM = 0;
	RANDOM_TRANSACTION_NUM = random_num;//一个事务删除一条

	BackendParameters *paramArr2[thread_num*tranaction_num];
	prepare_param(paramArr2,thread_num*tranaction_num);

	while(1)
	{
		for(int i = 0; i < thread_num; ++i)
		{
			for(int j = 0; j < tranaction_num; ++j)
			{
				tg.create_thread(boost::bind(&thread_update_exit, "test_data_1", "update",
					strlen("update"), paramArr2[i*tranaction_num+j], rid, &found[i*tranaction_num+j]));
			}	
		}
#ifdef WIN32
		Sleep(1000);
#else
		//     sleep(1000);
#endif
		if(TRANSACTION_COMMIT_NUM == RANDOM_TRANSACTION_NUM)
		{
			free_param(paramArr2,thread_num);
			save_result(rid,TRANSACTION_COMMIT_NUM,__FUNCTION__);
			on_exit_reset();
			exit(0);
		}
	}

	return main_flag;
}

bool test_thread_transaction_heap_insert_031_step2()
{
	int rid = 0;
	int check_num = read_result(__FUNCTION__,rid);
	THREAD_RID = rid;
	bool main_flag = true;
	char insert_data[][DATA_LEN] = 
	{	
		"update"
	};

	//查找元组，如果全部找到，则count中的每个元素都应该为插入数据的条数,sta每个元素都为1
	int *_sta = new int[ARRAY_LEN_CALC(insert_data)];
	int *_count = new int[ARRAY_LEN_CALC(insert_data)];
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(rid,AccessShareLock, MyDatabaseId);		
		for(int j = 0; j < ARRAY_LEN_CALC(insert_data); ++j)
		{
			findTuple(insert_data[j], rel, _sta[j], _count[j]);
		}	
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);		
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		main_flag = false;
	}

	//根据count,和sta的值判断测试结果
	for(int i = 0;i < ARRAY_LEN_CALC(insert_data); ++i)
	{
		cout << "count=" << _count[i] << endl;
		if( _count[i] != check_num )
		{
			main_flag = false;
		}
	}
	delete []_sta;
	delete []_count;

	//统计指定的元组数量
	int rel_count = 0;	
	calc_given_tuple_num(rid, rel_count,"update");
	if(rel_count != check_num)
	{
		cout << "rel_count" << rel_count << endl;
		main_flag = false;
	}
	thread_drop_heap(1);
	return main_flag;
}

bool test_thread_transaction_heap_insert_031()
{
	/*
	* 准备数据
	*/
	SHUTDOWN_TEST_STEP_1(TransExit)
	{
		return test_thread_transaction_heap_insert_031_step1();
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
		CLEAN_SHUTDOWN_TEST();
	return test_thread_transaction_heap_insert_031_step2();
	END_SHUTDOWN_TEST()
}

EXTERN_SIMPLE_FUNCTION

enum Operation
{
	Insert = 1,
	Delete,
	Update,
	SizeOpt = 3
};

static
Oid chose_rel(const Oid *heap_arr, const int len)
{
	int base = rand() % len;

	return heap_arr[base];
}

boost::mutex map_lock;

#define INSERT_DATA "abcdefg"
#define UPDATE_DATA "hijklmn"

vector<string> *ran_do_insert(const Oid rel_id, bool &is_deadlock, int least_len)
{
	using namespace FounderXDB::StorageEngineNS;

	vector<string> *insert = new vector<string>();
	try
	{
		int row_num = rand() % 500 + 1;
		int data_len = rand() % 4096 + least_len;
		DataGenerater dg(row_num, data_len);
		dg.dataGenerate();

		for(int j = 0; j < row_num; ++j)
		{
			insert->push_back(dg[j]);
		}

		Relation rel = FDPG_Heap::fd_heap_open(rel_id, RowExclusiveLock);
		for(int j = 0; j < row_num; ++j)
		{
			HeapTuple tuple = fdxdb_heap_formtuple(dg[j], data_len);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			pfree(tuple);
		}
		FDPG_Heap::fd_heap_close(rel, NoLock);
	} catch	(DeadLockException &de) {
		is_deadlock = true;
		delete insert;
		return NULL;
	} catch (StorageEngineException &se) {
		printf("%s\n", se.getErrorMsg());
		is_deadlock = true;
		delete insert;
		return NULL;
	}

	return insert;
}

vector<pair<string, string> > *ran_do_update(const Oid rel_id, bool &is_deadlock, int least_len)
{
	using namespace FounderXDB::StorageEngineNS;

	const int UPDATE_THRESH = 10;

	Relation rel = FDPG_Heap::fd_heap_open(rel_id, RowExclusiveLock);
	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
	HeapTuple tuple = NULL;
	vector<pair<string, string> > *v_update = new vector<pair<string, string> >();
	bool has_update = false;

	while(tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection))
	{
		int data_len = rand() % 4096 + 100;
		DataGenerater dg(1, data_len);
		dg.dataGenerate();
		try
		{
			/* 一次更新70%的数据 */
			if(
				(float)(rand() % UPDATE_THRESH) >= 
				(float)(UPDATE_THRESH * 0.3)
				)
			{
				char *data = fxdb_tuple_to_chars(tuple);
				HeapTuple new_tuple = fdxdb_heap_formtuple(dg[0], data_len);
				FDPG_Heap::fd_simple_heap_update(rel, &tuple->t_self, new_tuple);
				v_update->push_back(pair<string, string>(data, dg[0]));
				has_update = true;
				pfree(data);
				pfree(new_tuple);
			}
		} catch	(DeadLockException &de) {
			is_deadlock = true;
			if(v_update)
				delete v_update;
			return NULL;
		} catch (StorageEngineException &se) {
			printf("%s\n", se.getErrorMsg());
			is_deadlock = true;
			if(v_update)
				delete v_update;
			return NULL;
		}
	}
	FDPG_Heap::fd_heap_endscan(scan);
	FDPG_Heap::fd_heap_close(rel, NoLock);
	
	if(!has_update)
	{
		delete v_update;
		return NULL;
	}

	return v_update;
}

vector<string> *ran_do_delete(const Oid rel_id, bool &is_deadlock)
{
	using namespace FounderXDB::StorageEngineNS;

	const int DELETE_THRESH = 10;

	vector<string> *v_delete = new vector<string>();

	Relation rel = FDPG_Heap::fd_heap_open(rel_id, RowExclusiveLock);
	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
	HeapTuple tuple = NULL;
	bool has_delete = false;
	while(tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection))
	{
		try
		{
			/* 一次删除约70%的数据 */
			if(
				(float)(rand() % DELETE_THRESH) >= 
				(float)(DELETE_THRESH * 0.3)
				)
			{
				char *data = fxdb_tuple_to_chars(tuple);
				FDPG_Heap::fd_simple_heap_delete(rel, &tuple->t_self);
				
				v_delete->push_back(data);
				has_delete = true;
				pfree(data);
			}
		} catch	(DeadLockException &de) {
			is_deadlock = true;
			delete v_delete;
			return NULL;
		} catch (StorageEngineException &se) {
			printf("%s\n", se.getErrorMsg());
			is_deadlock = true;
			delete v_delete;
			return NULL;
		}
	}
	FDPG_Heap::fd_heap_endscan(scan);
	FDPG_Heap::fd_heap_close(rel, NoLock);

	if(!has_delete)
	{
		delete v_delete;
		return NULL;
	}
	return v_delete;
}

static void update_insert(const int chose, 
													map<Oid, vector<string> > *rel_info, 
													vector<string> *v_insert)
{
	for(int i = 0; i < v_insert->size(); ++i)
	{
		(*rel_info)[chose].push_back((*v_insert)[i]);
	}
}

static void update_delete(const int chose, 
													map<Oid, vector<string> > *rel_info, 
													vector<string> *v_delete)
{
	vector<string> tmp((*rel_info)[chose].size() - (*v_delete).size());
	sort((*rel_info)[chose].begin(), (*rel_info)[chose].end());
	sort((*v_delete).begin(), (*v_delete).end());
	set_difference((*rel_info)[chose].begin(), (*rel_info)[chose].end(),
		(*v_delete).begin(), (*v_delete).end(), tmp.begin());
	(*rel_info)[chose].clear();
	(*rel_info)[chose] = tmp;
}

static void update_update(const int chose, 
													map<Oid, vector<string> > *rel_info, 
													vector<pair<string, string> > *v_update)
{
	vector<string>::iterator it = (*rel_info)[chose].begin();
	map<int, bool> bit_map;
	int map_pos = 0;

	for(int i = 0; i < v_update->size(); ++i)
	{
		while(true)
		{
			if(it == (*rel_info)[chose].end())
			{
				it = (*rel_info)[chose].begin();
				map_pos = 0;
			}
			
			if(v_update->operator[](i).first == (*it) && 
				 bit_map[map_pos] == false)
			{
				it->clear();
				it->assign(v_update->operator[](i).second);
				bit_map[map_pos] = true;
				break;
			}
			
			++it;
			++map_pos;
		}
	}
}

void thread_random_do(const int minutes, map<Oid, vector<string> > *rel_info, 
														 Oid *heap_arr, Oid *ind_arr, int heap_arr_len, int ind_arr_len, 
														 BackendParameters *params)
{
	extern void* fxdb_SubPostmaster_Main(void*);
	fxdb_SubPostmaster_Main(params);

	srand(time(NULL));
	TimestampTz start_time = GetCurrentTimestamp();
	Operation opt;

	Oid chose = InvalidOid;
	bool abort_or_commit;
	vector<string> *v_insert = NULL;
	vector<string> *v_delete = NULL;
	vector<pair<string, string> > *v_update = NULL;
	bool is_deadlock = false;
	
	while(true)
	{		
		abort_or_commit = rand() % 2;
		opt = (Operation)((rand() % SizeOpt) + 1);
		chose = chose_rel(heap_arr, heap_arr_len);

		begin_transaction();
		switch(opt)
		{
			case Insert:
				v_insert = ran_do_insert(chose, is_deadlock);
				break;
			case Delete:
				v_delete = ran_do_delete(chose, is_deadlock);
				break;
			case Update:
				v_update = ran_do_update(chose, is_deadlock);
				break;
			default:
				Assert(false);
		}

		if(is_deadlock)
		{
			user_abort_transaction();
			is_deadlock = false;
		} else if(abort_or_commit)
		{
			map_lock.lock();
			if(v_insert)
			{
				update_insert(chose, rel_info, v_insert);
			} else if (v_delete)
			{
				update_delete(chose, rel_info, v_delete);
			} else if (v_update)
			{
				update_update(chose, rel_info, v_update);
			}
			map_lock.unlock();
			commit_transaction();
		} else {
			user_abort_transaction();
		}

		if(v_insert)
			delete v_insert;
		if(v_delete)
			delete v_delete;
		if(v_update)
			delete v_update;

		v_insert = NULL;
		v_delete = NULL;
		v_update = NULL;

		TimestampTz end_time = GetCurrentTimestamp();
		long secs;
		int usecs;
		TimestampDifference(start_time, end_time, &secs, &usecs);
		if(secs >= minutes * 60)
		{
			break;
		}

		pg_sleep(3000000);
	}

	proc_exit(0);
}

bool test_thread_vacuum_lt()
{

	extern void printf_cxt(BackendParameters *GET_PARAM());

	SHUTDOWN_TEST_STEP_1(LTTest)
	{
		extern int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2);

		INTENT("模拟长时间数据库的运行，测试vacuum的稳定性。");

		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

#define ALL_HEAP_NUM 20 /* At least 5 */

		const int THREAD_NUM = 10;
		const int EACH_THREAD_TIME = 60; /* Minutes */

		PREPARE_TEST();

		clear_all();
		Oid heap_id[ALL_HEAP_NUM];
		for(int i = 0; i < ALL_HEAP_NUM; ++i)
		{
			heap_id[i] = get_heap_id();
		}
		Oid index_id[ALL_HEAP_NUM];
		for(int i = 0; i < ALL_HEAP_NUM; ++i)
		{
			index_id[i] = get_index_id();
		}

		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(1, 2);
		int col_number[1] = {1};
		CompareCallback com_func[1] = {my_compare_str};
		Colinfo index_info = sg.buildIndexColInfo(1, col_number, com_func, SpliterGenerater::index_split_to_any<1>);

		/* 创建测试用表和索引 */
		int create_sta = 0;
		int index_pos = 0;
		for(int i = 0; i < ALL_HEAP_NUM; ++i)
		{
			SIMPLE_CREATE_HEAP(heap_id[i], create_sta);
			SIMPLE_CREATE_INDEX(heap_id[i], index_id[index_pos], heap_info, index_info, create_sta);
			++index_pos;
		}

		map<Oid, vector<string> > rel_info;
		for(int i = 0; i < ALL_HEAP_NUM; ++i)
		{
			rel_info[heap_id[i]] = vector<string>();
		}

		for(int i = 0; i < THREAD_NUM; ++i)
		{
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(bind(&thread_random_do, EACH_THREAD_TIME, &rel_info, 
				heap_id, index_id, ALL_HEAP_NUM, ALL_HEAP_NUM, 
				GET_PARAM()));
		}

		GET_THREAD_GROUP().join_all();
		FREE_PARAM(BackendParameters *);

		bool return_sta = true;
		/* 检测结果 */
		map<Oid, vector<string> > result_info;
		map<Oid, vector<string> > i_result_info;
		try
		{
			for(int i = 0; i < ALL_HEAP_NUM; ++i)
			{
				{
					/* relation顺序扫描检查结果 */
					begin_transaction();
					Relation rel = FDPG_Heap::fd_heap_open(heap_id[i], AccessShareLock);
					HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
					HeapTuple tuple = NULL;
					while(tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection))
					{
						char *data = fxdb_tuple_to_chars(tuple);
						result_info[heap_id[i]].push_back(data);
						pfree(data);
					}
					FDPG_Heap::fd_heap_endscan(scan);
					FDPG_Heap::fd_heap_close(rel, AccessShareLock);
					commit_transaction();

					if(result_info[heap_id[i]].size() != 
						rel_info[heap_id[i]].size())
					{
						return_sta = false;
						break;
					}

					sort(result_info[heap_id[i]].begin(), result_info[heap_id[i]].end());
					sort(rel_info[heap_id[i]].begin(), rel_info[heap_id[i]].end());
					if(result_info[heap_id[i]] != rel_info[heap_id[i]])
					{
						return_sta = false;
						break;
					}
				}

				{
					/* 索引顺序扫描检测结果 */
					begin_transaction();
					Relation rel = FDPG_Heap::fd_heap_open(heap_id[i], AccessShareLock);
					Relation irel = FDPG_Index::fd_index_open(index_id[i], AccessShareLock);
					IndexScanDesc scan = FDPG_Index::fd_index_beginscan(rel, irel, SnapshotNow, 0, NULL);
					FDPG_Index::fd_index_rescan(scan, NULL ,0, NULL, 0);
					HeapTuple tuple = NULL;
					while(tuple = FDPG_Index::fd_index_getnext(scan, ForwardScanDirection))
					{
						char *data = fxdb_tuple_to_chars(tuple);
						i_result_info[heap_id[i]].push_back(data);
						pfree(data);
					}
					FDPG_Index::fd_index_endscan(scan);
					FDPG_Index::fd_index_close(irel, AccessShareLock);
					FDPG_Heap::fd_heap_close(rel, AccessShareLock);
					commit_transaction();

					if(i_result_info[heap_id[i]].size() != 
						rel_info[heap_id[i]].size())
					{
						return_sta = false;
						break;
					}

					sort(i_result_info[heap_id[i]].begin(), i_result_info[heap_id[i]].end());
					if(i_result_info[heap_id[i]] != rel_info[heap_id[i]])
					{
						return_sta = false;
						break;
					}
				}
			}
		} catch (StorageEngineExceptionUniversal &se)
		{
			printf("%s\n", se.getErrorMsg());
			return_sta = false;
		}

		for(int i = 0; i < THREAD_NUM; ++i)
		{
			int rid = (heap_id[i]);
			int drop_sta = 0;
			SIMPLE_DROP_HEAP(rid, drop_sta);
		}

		return return_sta;
	}
	SHUTDOWN_TEST_STEP_2()
		CLEAN_SHUTDOWN_TEST();
	/* Do nothing... */
		return true;
	END_SHUTDOWN_TEST()
}
bool myTest_simple_heap_insert_000()
{
	INTENT("测试插入一条数据是否正确");
	try
	{
		//initial test environment
		HeapFuncTest heap;
		heap.buildHeapColInfo();
		begin_transaction();
		
		//open heap and insert some tuples
		SAVE_INFO_FOR_DEBUG();
		heap.openHeap( RowExclusiveLock );

		//insert batch data
		char testData[] = "test insert one data";
		const int dataLen = sizeof(testData);
		const int Rows = 1;
		heap.insert( testData, dataLen );
		FDPG_Transaction::fd_CommandCounterIncrement();

		//scan tuples inserted
		heap.scanTupleInserted( Rows, testData, dataLen );

		//drop heap
		heap.dropHeap();

		if( heap.m_bSuccess == true )
			END_TRANSACTION

		return heap.m_bSuccess;
	}
	CATCH_EXCEPTION_AND_ABORT
}

bool myTest_simple_heap_insert_001()
{
	INTENT("测试插入一条长数据");
	try
	{
		//create heap
		HeapFuncTest heap;
		heap.buildHeapColInfo();
		begin_transaction();

		//open heap
		SAVE_INFO_FOR_DEBUG();
		heap.openHeap( RowExclusiveLock );
		
		//insert a long data
		char testData[] = test_insert_long_data_len_1000;
		const int dataLen = sizeof(testData);
		const int Rows = 1;
		heap.insert( testData, dataLen );
		FDPG_Transaction::fd_CommandCounterIncrement();

		//scan tuples inserted
		heap.scanTupleInserted( Rows, testData, dataLen );

		//drop heap
		heap.dropHeap();

		if( heap.m_bSuccess == true )
			END_TRANSACTION
		return heap.m_bSuccess;
	}
	CATCH_EXCEPTION_AND_ABORT
}

bool myTest_simple_heap_insert_002()
{
	INTENT("测试插入空数据,即数据内容长度为0")
	try
	{
		//create heap
		HeapFuncTest heap;
		heap.buildHeapColInfo();
		begin_transaction();

		//open heap
		SAVE_INFO_FOR_DEBUG();
		heap.openHeap( RowExclusiveLock );

		//insert a NULL data (length = 0)
		char testData[] = "abcdefg";
		const int dataLen = 0;
		const int Rows = 1;
		heap.insert( testData, dataLen );
		FDPG_Transaction::fd_CommandCounterIncrement();

		//scan tuples inserted
		heap.scanTupleInserted( Rows, testData, dataLen );

		//drop heap
		heap.dropHeap();

		if( heap.m_bSuccess == true )
			END_TRANSACTION
		return heap.m_bSuccess;
	}
	CATCH_EXCEPTION_AND_ABORT
}

bool myTest_simple_heap_insert_003()
{
	INTENT("测试插入含有空串的数据")
	try
	{
		//create heap
		HeapFuncTest heap;
		heap.buildHeapColInfo();
		begin_transaction();

		//open heap and insert some tuples
		SAVE_INFO_FOR_DEBUG();
		heap.openHeap( RowExclusiveLock );
		const int dataLen = 20;
		char testData[][dataLen] = { "abcd\0\0efg", "abcd\0", "\0abcd", "", " "};
		int amount = sizeof( testData ) / dataLen;

		//insert some tuples with null substring
		heap.insertTuples( testData, amount );
		FDPG_Transaction::fd_CommandCounterIncrement();

		//scan tuples inserted
		heap.scanTuplesInserted( testData, amount, dataLen );

		//drop heap
		heap.dropHeap();

		if( heap.m_bSuccess == true )
			END_TRANSACTION
		return heap.m_bSuccess;
	}
	CATCH_EXCEPTION_AND_ABORT
}