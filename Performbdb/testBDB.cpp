#include <sys/types.h>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "bdb_utils.h"
#include "testBDB.h"

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

using namespace std;
int  main()
{
	try 
	{
		//test_performbdb_heap_create();
		//test_performbdb_heap_insert();
		//test_performbdb_heap_insert_special_random();
		//test_performbdb_heap_insert_large_data();
		//test_performbdb_heap_insert_special();

		//test_performbdb_heap_delete();
		//test_performbdb_heap_delete_special();
		//test_performbdb_heap_delete_special_random();
		//test_performbdb_heap_delete_large_data();

		//test_performbdb_heap_update();
		//test_performbdb_heap_update_special_random();
		//test_performbdb_heap_update_large_data();

		//test_performbdb_heap_scan();
		//test_performbdb_heap_scan_special();
		//test_performbdb_heap_scan_special_random();
		//test_performbdb_heap_scan_large_data();

		test_performbdb_thread_insert();
		//test_performbdb_thread_delete();
		//test_performbdb_thread_update();
	} 
	catch  (DbException  & dbe)
	{
		//cout << " 创建数据库失败: " ;
		cout << dbe.what() << endl;
	} 

	return   0 ;
}

//int
//main(int argc, char *argv[])
//{
//	int ch, rflag;
//	const char *database;
//
//	rflag = 0;
//	while ((ch = getopt(argc, argv, "r")) != EOF)
//		switch (ch) {
//		case 'r':
//			rflag = 1;
//			break;
//		case '?':
//		default:
//			return (usage());
//		}
//	argc -= optind;
//	argv += optind;
//
//	/* Accept optional database name. */
//	database = *argv == NULL ? DATABASE : argv[0];
//
//	// Use a try block just to report any errors.
//	// An alternate approach to using exceptions is to
//	// use error models (see DbEnv::set_error_model()) so
//	// that error codes are returned for all Berkeley DB methods.
//	//
//	try {
//		AccessExample app;
//		app.run((bool)(rflag == 1 ? true : false), database);
//		return (EXIT_SUCCESS);
//	}
//	catch (DbException &dbe) {
//		cerr << "AccessExample: " << dbe.what() << "\n";
//		return (EXIT_FAILURE);
//	}
//}
//
//AccessExample::AccessExample()
//{
//}
//
//void AccessExample::run(bool removeExistingDatabase, const char *fileName)
//{
//	// Remove the previous database.
//	if (removeExistingDatabase)
//		(void)remove(fileName);
//
//	// Create the database object.
//	// There is no environment for this simple example.
//	Db db(0, 0);
//
//	db.set_error_stream(&cerr);
//	db.set_errpfx("AccessExample");
//	db.set_pagesize(1024);		/* Page size: 1K. */
//	db.set_cachesize(0, 32 * 1024, 0);
//	db.open(NULL, fileName, NULL, DB_BTREE, DB_CREATE, 0664);
//
//	//
//	// Insert records into the database, where the key is the user
//	// input and the data is the user input in reverse order.
//	//
//	char buf[1024], rbuf[1024];
//	char *p, *t;
//	int ret;
//	u_int32_t len;
//
//	for (;;) {
//		cout << "input> ";
//		cout.flush();
//
//		cin.getline(buf, sizeof(buf));
//		if (cin.eof())
//			break;
//
//		if ((len = (u_int32_t)strlen(buf)) <= 0)
//			continue;
//		for (t = rbuf, p = buf + (len - 1); p >= buf;)
//			*t++ = *p--;
//		*t++ = '\0';
//
//		Dbt key(buf, len + 1);
//		Dbt data(rbuf, len + 1);
//
//		ret = db.put(0, &key, &data, DB_NOOVERWRITE);
//		if (ret == DB_KEYEXIST) {
//			cout << "Key " << buf << " already exists.\n";
//		}
//	}
//	cout << "\n";
//
//	// We put a try block around this section of code
//	// to ensure that our database is properly closed
//	// in the event of an error.
//	//
//	try {
//		// Acquire a cursor for the table.
//		Dbc *dbcp;
//		db.cursor(NULL, &dbcp, 0);
//
//		// Walk through the table, printing the key/data pairs.
//		Dbt key;
//		Dbt data;
//		while (dbcp->get(&key, &data, DB_NEXT) == 0) {
//			char *key_string = (char *)key.get_data();
//			char *data_string = (char *)data.get_data();
//			cout << key_string << " : " << data_string << "\n";
//		}
//		dbcp->close();
//	}
//	catch (DbException &dbe) {
//		cerr << "AccessExample: " << dbe.what() << "\n";
//	}
//
//	db.close(0);
//}
