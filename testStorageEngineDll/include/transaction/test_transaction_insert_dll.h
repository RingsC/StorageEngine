//extern int EID;
bool test_transaction_insert_dll_000();
bool test_transaction_insert_dll_001();
bool test_transaction_insert_dll_002_step1();
bool test_transaction_insert_dll_002_step2();
bool test_transaction_insert_dll_003();

int test_trans_persistence_insert();

extern EntrySet *open_entry_set();
extern EntrySetScan *heap_begin_scan(EntrySet *);

#define TRANSACTION_ROWS 10 
extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;

using std::cout;
using std::endl;
using std::vector;
using std::string;