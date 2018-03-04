#ifndef _ICQ_TYPES_H_
#define _ICQ_TYPES_H_
//#include <sys/types.h>
#include <iostream>
//#include <iomanip>
#include <errno.h>
#include <db.h>
//#include <pthread.h>
#include "boost/thread/thread.hpp"
#include "boost/thread.hpp" 
#include "boost/thread/xtime.hpp" 
#include "boost/thread/tss.hpp"
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "bdb_utils.h"
#include "testBDB-heapinsert.h"

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

using namespace boost;

using namespace std;

#define MAX_RAM_STR_SIZE 2000

bool test_performbdb_heap_insert()
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

		/* �ڻ�����֮ǰ���ɵ�����ʽΪdbenv->set_XXX()�����ɺ������û���*/
		/* ֪ͨDBʹ��Rijndael�����㷨���ο�����4�������ݽ��д���*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* ����DB�Ļ���Ϊ5M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		/* ����DB�������ݿ��ļ���Ŀ¼*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* �����ݿ⻷����ע����ĸ���־�ֱ�ָʾDB ������־�����������桢��������ϵ
		ͳ*/
	
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

		cout<<"BDB����(����/����)...����:";
		cin>>count;
		{
			TimerFixture e;
			for (int i=0;i<count;i++)
			{
				//Dbt key(&i,sizeof(i));
				key.data = &i;
				key.size = sizeof(int);
				ret = db->put(db, NULL, &key, &data, NULL);//DB_NOOVERWRITE����Ƿ�data/key pair�Ѿ�����
				//print_error(ret);
			}

				if(ret == 0)
					printf("INSERT SUCCESS!\n");
				else
					printf("INSERT FAILED!\n");
				db->close(db, 0);
				dbenv->close(dbenv, 0);
		}
	} 
	catch  (DbException  & e)
	{
		//cout << " �������ݿ�ʧ��: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

bool test_performbdb_heap_insert_special_random()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* �ڻ�����֮ǰ���ɵ�����ʽΪdbenv->set_XXX()�����ɺ������û���*/
		/* ֪ͨDBʹ��Rijndael�����㷨���ο�����4�������ݽ��д���*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* ����DB�Ļ���Ϊ5M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		/* ����DB�������ݿ��ļ���Ŀ¼*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* �����ݿ⻷����ע����ĸ���־�ֱ�ָʾDB ������־�����������桢��������ϵ
		ͳ*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);

		init_DBT(&key, &data);
		int count;
		srand((unsigned)time( NULL ));
		cout<<"BDB�������(����/����)��������󳤶�ΪMAX_RAM_STR_SIZE...����:";
		cin>>count;
		{
			TimerFixture e;
			for (int i=0;i<count;i++)
			{
				std::string temp_data;
				RandomGenString(temp_data,(rand()%MAX_RAM_STR_SIZE+1));
				data.data = (void *)temp_data.c_str();
				data.size = temp_data.length();
					//sizeof(temp_data);

				key.data = &i;
				key.size = sizeof(int);
				ret = db->put(db, NULL, &key, &data, DB_NOOVERWRITE);//DB_NOOVERWRITE����Ƿ�data/key pair�Ѿ�����
				print_error(ret);
			}

			if(ret == 0)
				printf("INSERT SUCCESS!\n");
			else
				printf("INSERT FAILED!\n");
			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}
	} 
	catch  (DbException  & e)
	{
		//cout << " �������ݿ�ʧ��: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

bool test_performbdb_heap_insert_large_data()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* �ڻ�����֮ǰ���ɵ�����ʽΪdbenv->set_XXX()�����ɺ������û���*/
		/* ֪ͨDBʹ��Rijndael�����㷨���ο�����4�������ݽ��д���*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* ����DB�Ļ���Ϊ5M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		/* ����DB�������ݿ��ļ���Ŀ¼*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* �����ݿ⻷����ע����ĸ���־�ֱ�ָʾDB ������־�����������桢��������ϵ
		ͳ*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);

		init_DBT(&key, &data);
		int count,data_size;
		//srand((unsigned)time( NULL ));
		cout<<"BDB�������(����/����)...����:";
		cin>>count;

		cout<<endl<<"�������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
		cin>>data_size;
		{
			TimerFixture e;
			for (int i=0;i<count;i++)
			{
				std::string temp_data;
				RandomGenString(temp_data,1<<data_size);
				data.data = (void*)temp_data.c_str();
				data.size = temp_data.length();
				//sizeof(temp_data);

				key.data = &i;
				key.size = sizeof(int);
				ret = db->put(db, NULL, &key, &data, NULL);//DB_NOOVERWRITE����Ƿ�data/key pair�Ѿ�����
				print_error(ret);
			}

			if(ret == 0)
				printf("INSERT SUCCESS!\n");
			else
				printf("INSERT FAILED!\n");
			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}
	} 
	catch  (DbException  & e)
	{
		//cout << " �������ݿ�ʧ��: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}
#endif

bool test_performbdb_thread_insert()
{
	int thread_num,count,data_size,status = 0;


	cout<<"BDB���߳���һ�����в������(����/����)...�߳�����:";
	cin>>thread_num;

	cout<<endl<<"ÿ���̲߳���������:";
	cin>>count;

	cout<<endl<<"����������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
	cin>>data_size;

	//vector<EntryID>data_eid;
	thread_group tg;
	tg.create_thread(bind(&BDB_thread_heap_create, &status));
	tg.join_all();

	{
		TimerFixture e;
		for(int i = 0; i < thread_num; ++i)
		{
			tg.create_thread(bind(&BDB_thread_heap_insert,  count , data_size , &status ));
		}
		tg.join_all();
	}

	cout<<endl<<"INSERT DATA FINISH!"<<endl;

	//tg.create_thread(bind(&BDB_thread_heap_remove, &status));
	//tg.join_all();

	check_status(status);

	return true;
}

bool test_performbdb_heap_insert_special()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* �ڻ�����֮ǰ���ɵ�����ʽΪdbenv->set_XXX()�����ɺ������û���*/
		/* ֪ͨDBʹ��Rijndael�����㷨���ο�����4�������ݽ��д���*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* ����DB�Ļ���Ϊ5M */
		
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
	
		/* ����DB�������ݿ��ļ���Ŀ¼*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* �����ݿ⻷����ע����ĸ���־�ֱ�ָʾDB ������־�����������桢��������ϵ
		ͳ*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		ret = db->set_pagesize(db , 8*1024);
		ret = db->set_flags(db , DB_REVSPLITOFF);
		//ret = db->truncate(db,NULL,0,0);
		print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);

		init_DBT(&key, &data);
		int count,data_size;
		//srand((unsigned)time( NULL ));
		cout<<"BDB�������(����/����)...����:";
		cin>>count;

		cout<<endl<<"�������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
		cin>>data_size;
		{
			TimerFixture e;
			for (int i=0;i<count;i++)
			{
				std::string temp_data;
				RandomGenString(temp_data,1<<data_size);
				data.data = (void*)temp_data.c_str();
				data.size = temp_data.length();
				//sizeof(temp_data);

				key.data = &i;
				key.size = sizeof(int);
				ret = db->put(db, NULL, &key, &data, NULL);//DB_NOOVERWRITE����Ƿ�data/key pair�Ѿ�����
				print_error(ret);
			}

			if(ret == 0)
				printf("INSERT SUCCESS!\n");
			else
				printf("INSERT FAILED!\n");
			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}
	} 
	catch  (DbException  & e)
	{
		//cout << " �������ݿ�ʧ��: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

		//db.open(NULL,"MyTest.db",NULL,DB_BTREE,DB_CREATE,0);

		
				//Dbt data("test",sizeof("test"));
				////Dbt key("test",sizeof("test")); 
		
				////д�����ݿ�

				//int count;

				//cout<<"BDB�������ݣ���������...����:";
				//cin>>count;

				
				//{
				//	TimerFixture e;
				//	for (int i=0;i<count;i++)
				//	{
				//		Dbt key(&i,sizeof(i));
				//		db.put( NULL, &key, &data, 0);
				//	}
				//}

				//for (int i=0;i<count;i++)
				//{
				//		Dbt key(&i,sizeof(i));
				//		db.del(NULL,&key,NULL);
				//}

				
		


