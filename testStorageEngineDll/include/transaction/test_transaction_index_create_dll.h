using std::cout;
using std::endl;
using std::vector;
using std::string;
extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
extern EntrySetID EID;
extern EntrySet *open_entry_set();
extern EntrySetScan *heap_begin_scan(EntrySet *);
extern int numLenth(int);
extern char *formData(int num,char *str);
extern void insertData(EntrySet *pEntrySet,char *insert_str,vector<DataItem*> &dvec,char copy_insert_data[][20],uint32 nData,char *str);

bool test_index_transaction_create_dll_000();
bool test_index_transaction_create_dll_001();
bool test_index_transaction_create_dll_002();
bool test_index_transaction_create_dll_003();
bool test_index_transaction_create_dll_004();
bool test_index_transaction_create_dll_005();