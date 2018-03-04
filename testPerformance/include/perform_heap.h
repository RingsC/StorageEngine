#ifndef TEST_PERFORM_HEAP_H
#define TEST_PERFORM_HEAP_H

#include <stdlib.h>
#include <iostream>
#include <vector>
#include "PGSETypes.h"
#include "Transaction.h"
#include "EntrySet.h"
#include "perform_utils.h"

using namespace FounderXDB::StorageEngineNS;

class CPerformInsert : public CPerformBase
{
public:
	void Execute(Transaction* pTrans, EntrySet* pEntrySet);

	void ClearTask();

	virtual ~CPerformInsert();

	static EntrySetID GetEntrySetId();

private:
	static std::vector<EntrySetID> m_EntrySet;
};
class CPerformInsertConcurrent : public CPerformBase
{
public:
	void Execute(Transaction* pTrans, EntrySet* pEntrySet);

	virtual ~CPerformInsertConcurrent();
};
class CPerformDelete : public CPerformBase
{
public:
	virtual void Execute(Transaction* pTrans, EntrySet* pEntrySet);

	virtual ~CPerformDelete();
};
class CPerformUpdate : public CPerformBase
{
public:
	virtual void Execute(Transaction* pTrans, EntrySet* pEntrySet);

	virtual ~CPerformUpdate();
};
class CPerformSearch : public CPerformBase
{
public:
	virtual void Execute(Transaction* pTrans, EntrySet* pEntrySet);

	virtual ~CPerformSearch();
};
class CPerformIndexSearch : public CPerformBase
{
public:
	virtual void Execute(Transaction* pTrans, EntrySet* pEntrySet);

	virtual ~CPerformIndexSearch();
};

class CPerformFactory
{
public:
	enum PerformType
	{
		PERFORM_INSERT,
		PERFORM_DELETE,
		PERFORM_UPDATE,
		PERFORM_SEARCH,
		PERFORM_INDEX_SEARCH,
		PERFORM_INSERT_CONCURRENT
	};

	CPerformBase* GetInstance(PerformType type);
};

//////////////////////////////////////////////////////////////////////////
bool test_perform_heap_insert();
bool test_perform_heap_insert_index();
bool test_perform_heap_insert_concurrent();
bool test_perform_heap_insert_concurrent_index();

bool test_perform_heap_traverse();
bool test_perform_heap_traverse_index();
bool test_perform_heap_traverse_concurrent();
bool test_perform_heap_traverse_concurrent_index();

bool test_perform_heap_delete();
bool test_perform_heap_delete_index();
bool test_perform_heap_delete_concurrent();
bool test_perform_heap_delete_concurrent_index();

bool test_perform_heap_update();
bool test_perform_heap_update_index();
bool test_perform_heap_update_concurrent();
bool test_perform_heap_update_concurrent_index();

bool test_perform_heap_prepare_data();
bool test_perform_heap_query();
bool test_perform_heap_prepare_data_index();
bool test_perform_heap_query_index();
bool test_perform_heap_insert_trans();
bool test_perform_heap_insert_trans_index();

bool test_perform_heap_query_trans_index();
#endif//TEST_PERFORM_COMPARE_H
