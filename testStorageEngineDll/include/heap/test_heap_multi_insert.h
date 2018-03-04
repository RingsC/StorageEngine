#ifndef TEST_HEAP_MULTI_INSERT_H
#define TEST_HEAP_MULTI_INSERT_H

struct TestData 
{
	TestData()
	{
		tuple_count = 0;
		tuple_len = 0;
		pData = NULL;
	}

	void Clear_Mem()
	{
		if (pData)
		{
			free(pData);
			pData = NULL;
		}
	}
	uint32 tuple_count;//tuple count in mem pData
	uint32 tuple_len;//single tuple len in mem pData
	char* pData;
	~TestData()
	{
		Clear_Mem();		
	}
};

bool test_heap_multi_insert_frm_items();
bool test_heap_multi_insert_frm_entrySet();
bool test_heap_multi_insert_frm_items_index();
bool test_heap_multi_insert_frm_entrySet_index();
bool test_heap_multi_insert_identify();
bool test_heap_multi_insert_identify_index();
bool test_heap_multi_insert_frm_entrySet_ShutDown();
bool test_heap_multi_insert_frm_items_ShutDown();
bool test_heap_multi_insert_frm_items_ShutDown_index();
bool test_heap_multi_insert_frm_entrySet_ShutDown_index();
bool test_heap_multi_insert_multi();
bool test_heap_multi_insert_multi_index();
bool test_heap_multi_insert_multi_frmEntrySet();
bool test_heap_multi_insert_multi_frmEntrySet_index();
bool test_heap_multi_insert_toast();
bool test_heap_multi_insert_thread_transaction();

#endif//TEST_HEAP_MULTI_INSERT_H