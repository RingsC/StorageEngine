#ifndef _ICQ_TYPES_H_
#define _ICQ_TYPES_H_
//#include <sys/types.h>
#include "boost/thread/thread.hpp"
#include "boost/thread.hpp" 
#include  "boost/thread/xtime.hpp" 
#include  "boost/thread/tss.hpp"
#include <iostream>
//#include <iomanip>
#include <errno.h>
#include <db.h>

//#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "bdb_utils.h"
#include "testBDB-heapdelete.h"

#ifdef _WIN32
extern "C" {
	extern int getopt(int, char * const *, const char *);
	extern int optind;
}
#else
#include <unistd.h>
#endif

#include <db_cxx.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using namespace boost;
//using namespace std;

#define MAX_RAM_STR_SIZE 2000

bool test_performbdb_heap_delete()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;


		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);

		init_DBT(&key, &data);

		int count;

		cout<<"BDB删除(数据/索引)...行数:";
		cin>>count;
		for (int i=0;i<count;i++)
		{
			//Dbt key(&i,sizeof(i));
			key.data = &i;
			key.size = sizeof(int);
			ret = db->put(db, NULL, &key, &data, NULL);//DB_NOOVERWRITE检测是否data/key pair已经存在
			//print_error(ret);

		}

		if(ret == 0)
			printf("INSERT SUCCESS!\n");
		else
			printf("INSERT FAILED!\n");


		{
			TimerFixture e;
			for (int i=0;i<count;i++)
			{
				key.data = &i;
				key.size = sizeof(int);
				ret = db->del(db, NULL, &key, 0);
				//print_error(ret);
			}
				
			print_error(ret);
			if(ret == 0)
				printf("DELETE SUCCESS!\n");
			else
				printf("DELETE FAILED!\n");
	
				db->close(db, 0);
				dbenv->close(dbenv, 0);
		}
	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

bool test_performbdb_heap_delete_special_random()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;
		char name[10] = {'\0'};
		memcpy(name,"test",sizeof("test"));

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);

		init_DBT(&key, &data);
		data.data = name;
		data.size = sizeof("test");

		int count;

		cout<<"BDB删除随机(数据/索引)，数据的最大长度为...行数:";
		cin>>count;
		srand((unsigned)time( NULL ));
		for (int i=0;i<count;i++)
		{
			std::string temp_data;
			RandomGenString(temp_data,(rand()%MAX_RAM_STR_SIZE+1));
			data.data = &temp_data;
			data.size = temp_data.length();
			//sizeof(temp_data);

			key.data = &i;
			key.size = sizeof(int);
			ret = db->put(db, NULL, &key, &data, NULL);//DB_NOOVERWRITE检测是否data/key pair已经存在
			print_error(ret);
		}

		if(ret == 0)
			printf("INSERT SUCCESS!\n");
		else
			printf("INSERT FAILED!\n");


		{
			TimerFixture e;
			for (int i=0;i<count;i++)
			{
				key.data = &i;
				key.size = sizeof(int);
				ret = db->del(db, NULL, &key, 0);
				//print_error(ret);
			}

			print_error(ret);
			if(ret == 0)
				printf("DELETE SUCCESS!\n");
			else
				printf("DELETE FAILED!\n");

			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}
	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

bool test_performbdb_heap_delete_large_data()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;
		char name[10] = {'\0'};
		memcpy(name,"test",sizeof("test"));

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);

		init_DBT(&key, &data);
		data.data = &name;
		data.size = sizeof("test");

		int count,data_size;

		cout<<"BDB删除随机(数据/索引)...行数:";
		cin>>count;

		cout<<endl<<"插入数据的大小(生成数据为2的N次方字节)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
		cin>>data_size;

		//srand((unsigned)time( NULL ));
		for (int i=0;i<count;i++)
		{
			std::string temp_data;
			RandomGenString(temp_data,1<<data_size);
			data.data = (void *)temp_data.c_str();
			data.size = temp_data.length();
			//sizeof(temp_data);

			key.data = &i;
			key.size = sizeof(int);
			ret = db->put(db, NULL, &key, &data, NULL);//DB_NOOVERWRITE检测是否data/key pair已经存在
			print_error(ret);
		}

		if(ret == 0)
			printf("INSERT SUCCESS!\n");
		else
			printf("INSERT FAILED!\n");


		{
			TimerFixture e;
			for (int i=0;i<count;i++)
			{
				key.data = &i;
				key.size = sizeof(int);
				ret = db->del(db, NULL, &key, 0);
				//print_error(ret);
			}

			print_error(ret);
			if(ret == 0)
				printf("DELETE SUCCESS!\n");
			else
				printf("DELETE FAILED!\n");

			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}
	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

bool test_performbdb_thread_delete()
{
	int thread_num,count,data_size,status = 0;


	cout<<"BDB向一个表中插入随机(数据/索引)...数据量:";
	cin>>count;

	cout<<endl<<"插入随机数据的大小(生成数据为2的N次方字节)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
	cin>>data_size;

	cout<<endl<<"多个线程删除表中数据:";
	cin>>thread_num;

	//vector<EntryID>data_eid;
	thread_group tg;
	tg.create_thread(bind(&BDB_thread_heap_create, &status));
	tg.join_all();

	tg.create_thread(bind(&BDB_thread_heap_insert,  count , data_size , &status ));
	tg.join_all();

	cout<<endl<<"INSERT DATA FINISH!"<<endl;

	for (int i=0 ; i<thread_num ;i++)
	{
		tg.create_thread(bind(&BDB_thread_heap_delete,  i , (count/thread_num) , &status ));
	}
	
	tg.join_all();

	cout<<endl<<"DELETE DATA FINISH!"<<endl;


	//tg.create_thread(bind(&BDB_thread_heap_remove, &status));
	//tg.join_all();

	check_status(status);

	return true;
}

bool test_performbdb_heap_delete_special()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;
		char name[10] = {'\0'};
		memcpy(name,"test",sizeof("test"));

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		ret = db->set_pagesize(db , 8*1024);
		ret = db->set_flags(db , DB_REVSPLITOFF);
		print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);

		init_DBT(&key, &data);
		data.data = &name;
		data.size = sizeof("test");

		int count,data_size;

		cout<<"BDB删除随机(数据/索引)...行数:";
		cin>>count;

		cout<<endl<<"插入数据的大小(生成数据为2的N次方字节)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
		cin>>data_size;

		//srand((unsigned)time( NULL ));
		for (int i=0;i<count;i++)
		{
			std::string temp_data;
			RandomGenString(temp_data,1<<data_size);
			data.data = (void *)temp_data.c_str();
			data.size = temp_data.length();
			//sizeof(temp_data);

			key.data = &i;
			key.size = sizeof(int);
			ret = db->put(db, NULL, &key, &data, NULL);//DB_NOOVERWRITE检测是否data/key pair已经存在
			print_error(ret);
		}

		if(ret == 0)
			printf("INSERT SUCCESS!\n");
		else
			printf("INSERT FAILED!\n");


		{
			TimerFixture e;
			for (int i=0;i<count;i++)
			{
				key.data = &i;
				key.size = sizeof(int);
				ret = db->del(db, NULL, &key, 0);
				//print_error(ret);
			}

			print_error(ret);
			if(ret == 0)
				printf("DELETE SUCCESS!\n");
			else
				printf("DELETE FAILED!\n");

			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}
	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

#endif