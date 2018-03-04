#include "PGSETypes.h"

struct HeapInfo
{
	uint32 unHeapName;
	uint32 unColumnId;
};

bool test_base_backup();
bool make_base_backup();
bool test_heap_recovery();
bool test_recovery_modify();
bool test_heap_recovery_again();
bool test_heap_level_recovery();

//utils
bool is_heap_exists(HeapInfo heap);
void my_heap_setcolumnInfo(uint32 unColumnId);
void write_heapname_columnId(uint32 unHeapName, uint32 unColumnId);
bool create_heap_by_id(uint32 unColumnId);
void write_recovery_time_file(uint32 unColumnId);
void test_clear_files();
bool is_need_run();