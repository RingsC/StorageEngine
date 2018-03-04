#include <sys/types.h>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "bdb_utils.h"
#include "testBDB-heapcreate.h"

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


int
usage()
{
	(void)fprintf(stderr, "usage: AccessExample [-r] [database]\n");
	return (EXIT_FAILURE);
}

using namespace std;
bool test_performbdb_heap_create()
{
	try 
	{
		int count;

		cout<<"请输入建表的数量:";
		cin>>count;

		char **names;
		names = (char **) malloc (sizeof(char *)*count);
		for (int i=0;i<count;i++)
		{
			names[i]=(char *)malloc(sizeof(char)*10);
			memset(names[i],0,sizeof(char)*10);
		}

		int namesize=sizeof("heap");
		for (int i=0;i<count;i++)
		{
			std::stringstream s;
			s<<"heap"<<i;  
			memcpy(names[i],s.str().c_str(),s.str().length());
		}

		Db db(NULL,0);
		int ret=0;
		TimerFixture timecount;
		{
			for (int i=0;i<count;i++)
			{
				db.open(NULL,names[i],NULL,DB_BTREE,DB_CREATE,0);
			}

		}

		db.close(0);// 关闭数据库
		for (int i=0;i<count;i++)
		{
			db.remove(names[i],NULL,0);
		}

	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}
