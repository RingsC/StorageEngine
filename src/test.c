#include "postgres.h"

#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "utils/tqual.h"
#include "catalog/pg_class.h"
#include "utils/builtins.h"
#include "access/xdb_common.h"
#include "catalog/xdb_catalog.h"
#include "access/xlog.h"
#include "catalog/index.h"

#include<string.h>
#include "postmaster/xdb_main.h"
#include "utils/fmgroids.h"



extern const char * fdxdb_heap_to_chars(HeapTuple tuple, size_t *len);
extern Datum fdxdb_string_formdatum(const char *p, size_t len);
extern char * text_to_cstring(const text *t);
extern bool test_update();
extern bool test_heap_drop();
extern bool test_delete();
extern bool test_heap_open();
extern bool test_sqscan();
extern bool test_indexscan();
extern bool test_indexscan_null();
extern bool test_multi_col_sqscan();
extern   bool test_index_insert();


extern TupleDesc single_attibute_tupDesc;

static void initdatabase();

static void runtest();


int main(int argc, char *argv[])
{
   // initdatabase();
    runtest();
}
static void initdatabase()
{
    const char* pg_data = "C:\\storageEngine\\data";
    initdb(pg_data);
}
static void runtest()
{
   
}

void test_insert()
{

}


