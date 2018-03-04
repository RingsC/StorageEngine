#include "postgres.h"
#include "catalog/rdf_storage_handler.h"
#include "interface/PGGraphStorage.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "Macros.h"

void RDFCreateStatementsTable(uint64 id, const char *name, RDFMeta_Models storage);
void RDFStatementAdd(RDF_Statements statement, RDFMeta_Models storage);
unsigned int RDFStatementRemove(RDF_Statements statement, RDFMeta_Models storage);
void RDFStatementContextRemove(uint64 context,RDFMeta_Models storage);
void RDFCounterStorageSize(Oid relid, long *count);
bool RDFStatementExists(RDF_Statements statement, RDFMeta_Models storage);
void RDFGetStorageOid(uint64 id, const char *name, RDFMeta_Models storage);
SysScanDesc RDFStatementSearch(RDF_Statements statement, RDFMeta_Models storage);
SysScanDesc RDFStatementContextSearch(uint64 ctxt, RDFMeta_Models storage);
RDF_Statements RDFStatementGetNext(SysScanDesc sd);
void RDFStetementFree(RDF_Statements statement_store);
void RDFResultsClear(SysScanDesc sd);
void RDFStatementTableDrop(RDFMeta_Models storage);
RDFMeta_Models RDFGetAllGraph(unsigned int *count);
HeapTuple RDFStatmentFormtuple(RDF_Statements statement, MemoryContext cxt);
HeapTuple RDFRBLFormtuple(nodes *node, uint64 node_id, MemoryContext cxt);
bool RDFRBLExists(uint64 id, char **data);
void RDFRBLCacheInsert(uint64 id, nodes *node);
bool RDFStatementCacheExists(uint64 sid, uint64 pid, uint64 oid);
void RDFStatementCacheInsert(uint64 sid, uint64 pid, uint64 oid);
void RDFStatementCacheDestroy();
void RDFStatementMultiAdd(HeapTuple *arr_statements, int statement_arr_lens,
													HeapTuple *arr_rbls, int rbl_arr_lens,
													RDFMeta_Models storage);
MemoryContext RDFGetTempContext(char *tname);
void RDFDeleteTempContext(MemoryContext cxt);
void RDFNewXactStart();
void RDFNewXactCommit();
void RDFNewXactAbort();

namespace FounderXDB{
	namespace GraphEngineNS {

		using namespace FounderXDB::StorageEngineNS;
		static const unsigned int StatementInsertThresh = 20 * 10000;
	
		PGGraphStorage::PGGraphStorage()
		{
			insert_statu = MULTI_INSERT_DEFAULT;
			storage_info.stid = 0;
			storage_info.id = 0;
			storage_info.index_os_id = 0;
			storage_info.index_po_id = 0;
			storage_info.index_spo_id = 0;
			storage_info.index_inf_id = 0;
			storage_info.db_id = 0;
			tmp_tup_cxt = NULL;
			memset(storage_info.name.data, 0, NAMEDATALEN);
			statement_arr = NULL;
			rbl_arr = NULL;
			statement_arr_lens = 0;
			rbl_arr_lens = 0;
			arr_memcxt = NULL;
			rev_nums = 0;
		}

		bool PGGraphStorage::CreateStatementsTable(uint64 id, const char *name)
		{
			THROW_CALL(RDFCreateStatementsTable, id, name, &storage_info);
			return true;
		}

		bool PGGraphStorage::GetStorageEntrySetID(uint64 id, const char *name)
		{
			THROW_CALL(RDFGetStorageOid, id, name, &storage_info);
			return true;
		}

		bool PGGraphStorage::StatementAdd(RDF_Statements statement)
		{
			namespace FXDBSE = FounderXDB::StorageEngineNS;
			bool result = false;
			if (insert_statu == MULTI_INSERT_DEFAULT)
			{
				/* 单个三元组插入例程 */
				THROW_CALL(RDFStatementAdd, statement, &storage_info);
				result = true;
			}
			else if (insert_statu == MULTI_INSERT_PREPARE)
			{
				bool ex = false;

				PG_TRY();
				{
					/* 批量三元组插入例程 */
					if (statement_arr_lens == (int)StatementInsertThresh)
					{
						THROW_CALL(RDFStatementMultiAdd, statement_arr, statement_arr_lens, rbl_arr, rbl_arr_lens, &storage_info);
						result = true;

						/* 释放tmpcxt上分配的所有内存并重新分配一个新的内存块 */
						RDFDeleteTempContext(tmp_tup_cxt);
						tmp_tup_cxt = NULL;

						/* 初始化成员变量的状态 */
						statement_arr_lens = 0;
						rbl_arr_lens = 0;

						/* 销毁事务缓存 */
						RDFStatementCacheDestroy();
					}

					if (tmp_tup_cxt == NULL)
					{
						tmp_tup_cxt = RDFGetTempContext((char*)"Nodes tmp context");
					}

					if (arr_memcxt == NULL)
					{
						arr_memcxt = FXDBSE::MemoryContext::getMemoryContext(FXDBSE::MemoryContext::AlwaysCreate);
						arr_memcxt->create("Temp arr context",MemoryContext::getMemoryContext(MemoryContext::Top));
					}

					if (statement_arr == NULL)
					{
						/* 这里假定如果statement_arr为空，那么rbl_arr必然为空 */
						assert(rbl_arr == NULL);

						statement_arr = (HeapTuple *)arr_memcxt->alloc(sizeof(HeapTuple) * StatementInsertThresh);

						/* 
						 *由于一个statement有四个内容，包括s、p、o和context 
						 *所以rbl_arr的容量最坏估计要乘以4
						 */
						rbl_arr = (HeapTuple *)arr_memcxt->alloc(sizeof(HeapTuple) * (StatementInsertThresh * 4));
					}

					/* 
					 *这里务必使用exists函数先判断statement的唯一性，
					 *因为底层的批量插入函数不会去判断唯一性
					 */
					if (!RDFStatementExists(statement, &storage_info) &&
							!RDFStatementCacheExists(statement->subject_id, statement->predicate_id, statement->object_id))
					{
						statement_arr[statement_arr_lens++] = RDFStatmentFormtuple(statement, tmp_tup_cxt);
						RDFStatementCacheInsert(statement->subject_id, statement->predicate_id, statement->object_id);
					}

					if (!RDFRBLExists(statement->subject_id, NULL))
					{
						rbl_arr[rbl_arr_lens++] = RDFRBLFormtuple(statement->subject, statement->subject_id, tmp_tup_cxt);
						RDFRBLCacheInsert(statement->subject_id, statement->subject);
					}

					if (!RDFRBLExists(statement->predicate_id, NULL))
					{
						rbl_arr[rbl_arr_lens++] = RDFRBLFormtuple(statement->predicate, statement->predicate_id, tmp_tup_cxt);
						RDFRBLCacheInsert(statement->predicate_id, statement->predicate);
					}

					if (!RDFRBLExists(statement->object_id, NULL))
					{
						rbl_arr[rbl_arr_lens++] = RDFRBLFormtuple(statement->object, statement->object_id, tmp_tup_cxt);
						RDFRBLCacheInsert(statement->object_id, statement->object);
					}

					if (statement->context != 0 && !RDFRBLExists(statement->context, NULL))
					{
						rbl_arr[rbl_arr_lens++] =  RDFRBLFormtuple(statement->ctxt, statement->context, tmp_tup_cxt);
						RDFRBLCacheInsert(statement->context, statement->ctxt);
					}	
					result = true;
				} PG_CATCH();
				{
					ex = true;
				} PG_END_TRY();

				if (ex)
					ThrowException();
			} else if (insert_statu == MULTI_INSERT_EXECUTE)
			{
				/* 刷出余下数组中的statement */
				result = this->flushStatementsArr();
				insert_statu = MULTI_INSERT_DEFAULT;
			}

			return result;
		}

		bool PGGraphStorage::StatementRemove(RDF_Statements statement)
		{
			bool result = false;
			THROW_CALL(this->rev_nums = RDFStatementRemove, statement, &storage_info);
			result = true;

			return result;
		}

		bool PGGraphStorage::StatementContextRemove(uint64 context)
		{
			bool result = false;
			THROW_CALL(RDFStatementContextRemove, context, &storage_info);
			result = true;

			return result;
		}

		unsigned int PGGraphStorage::CounterRemoveNum()
		{
			unsigned int ret = this->rev_nums;
			this->rev_nums = 0;
			return ret;
		}

		bool PGGraphStorage::CounterStorageSize(long *count)
		{
			bool result = false;
			THROW_CALL(RDFCounterStorageSize, storage_info.stid, count);
			result = true;

			return result;
		}

		bool PGGraphStorage::StatementExists(RDF_Statements statement)
		{
			bool result = false;
			THROW_CALL(result = RDFStatementExists, statement, &storage_info);
			return result;
		}

		SysScanDesc PGGraphStorage::StatementSearch(RDF_Statements statement)
		{
			SysScanDesc sd = NULL;
			THROW_CALL(sd = RDFStatementSearch, statement, &storage_info);

			return sd;
		}

		SysScanDesc PGGraphStorage::StatementContextSearch(uint64 context)
		{
			SysScanDesc sd = NULL;
			THROW_CALL(sd = RDFStatementContextSearch, context, &storage_info);

			return sd;
		}

		RDF_Statements PGGraphStorage::StatementGetNext(SysScanDesc sd) const
		{
			return RDFStatementGetNext(sd);
		}

		void PGGraphStorage::StetementFree(RDF_Statements statement)
		{
			RDFStetementFree(statement);
		}

		void PGGraphStorage::ResultsClear(SysScanDesc sd)
		{
			RDFResultsClear(sd);
		}

		bool PGGraphStorage::DropStorage()
		{
			THROW_CALL(RDFStatementTableDrop, &storage_info);
			return true;
		}

		/// 获取所有图信息
		RDFMeta_Models RDFStorage::GetAllGraph(unsigned int *count)
		{
			RDFMeta_Models result = NULL;
			THROW_CALL(result = RDFGetAllGraph, count);
			return result;
		}

		bool PGGraphStorage::NewXactStart()
		{
			return true;
			//return RDFNewXactStart();
		}

		bool PGGraphStorage::NewXactCommit()
		{
			return true;
			//return RDFNewXactCommit();
		}

		bool PGGraphStorage::NewXactAbort()
		{
			return true;
			//return RDFNewXactAbort();
		}

		void PGGraphStorage::setExecMultiInsert()
		{
			insert_statu = MULTI_INSERT_EXECUTE;
			
			this->flushStatementsArr();

			insert_statu = MULTI_INSERT_DEFAULT;
		}

		void PGGraphStorage::prepareExecMultiInsert()
		{
			insert_statu = MULTI_INSERT_PREPARE;
		}

		bool PGGraphStorage::flushStatementsArr()
		{
			bool result = false;
			/* 插入最后的一批statement */
			if (statement_arr_lens > 0)
			{
				THROW_CALL(RDFStatementMultiAdd, statement_arr, statement_arr_lens, rbl_arr, rbl_arr_lens, &storage_info);
			}

			RDFStatementCacheDestroy();

			result = true;

			/* 重置变量 */
			if (tmp_tup_cxt)
				RDFDeleteTempContext(tmp_tup_cxt);
			if (arr_memcxt)
				arr_memcxt->destroy();
			arr_memcxt = NULL;
			tmp_tup_cxt = NULL;
			statement_arr_lens = 0;
			rbl_arr_lens = 0;
			statement_arr = NULL;
			rbl_arr = NULL;

			return result;
		}

	} //GraphEngineNS
} //FounderXDB
