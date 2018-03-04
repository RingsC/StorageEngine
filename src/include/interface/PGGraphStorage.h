#ifndef PG_GRAPG_STORAGE_H 
#define PG_GRAPG_STORAGE_H  

/// @file PGGraphStorage.h
/// @brief Define graph storage interface.
///
#include "PGSETypes.h"
#include "GraphStorage.h"

namespace FounderXDB{
	namespace GraphEngineNS {

		enum MULTI_INSERT_STATU
		{
			MULTI_INSERT_DEFAULT,
			MULTI_INSERT_PREPARE,
			MULTI_INSERT_EXECUTE
		};

		/// @ingroup PGGraphStorage
		class PGSE_DLLEXTERN PGGraphStorage : public RDFStorage
		{
		private:
			RDFMeta_Models_Data storage_info;
			MULTI_INSERT_STATU insert_statu;
			HeapTuple *statement_arr;
			HeapTuple *rbl_arr;
			int statement_arr_lens;
			int rbl_arr_lens;
			FounderXDB::StorageEngineNS::MemoryContext *arr_memcxt;
			MemoryContext tmp_tup_cxt;
			unsigned int rev_nums;
		public:
			DEFINE_OPERATOR_NEW_DELETE;
			virtual ~PGGraphStorage(){}
			PGGraphStorage();
			/// 创建statement元数据表
			virtual bool CreateStatementsTable(uint64 id, const char *name);

			/// 根据id或者name获取storage表的id
			virtual bool GetStorageEntrySetID(uint64 id, const char *name);

			/// 向statement元数据表中插入一个或者多个statement
			virtual bool StatementAdd(RDF_Statements statement);

			/// 从statement元数据表中删除一个statement
			virtual bool StatementRemove(RDF_Statements statement);

			/// 根据context删除一个三元组
			virtual bool StatementContextRemove(uint64 context);

			/// 统计删除三元组的数量
			virtual unsigned int CounterRemoveNum();

			/// 计算一个storage中有多少个三元组
			virtual bool CounterStorageSize(long *count);

			// 查找一个三元组是否存在
			virtual bool StatementExists(RDF_Statements statement);

			/// 根据给定的statement查找三元组，返回结果集
			virtual SysScanDesc StatementSearch(RDF_Statements statement);

			///	根据给定的context查找三元组，返回结果集
			virtual SysScanDesc StatementContextSearch(uint64 context);

			/// 返回结果集中下一个三元组
			virtual RDF_Statements StatementGetNext(SysScanDesc sd) const;

			/// 释放一个三元组
			virtual void StetementFree(RDF_Statements statement);

			/// 清空结果集
			virtual void ResultsClear(SysScanDesc sd);

			/// 删除一个storage
			virtual bool DropStorage();

			/// 启动一个新事务
			virtual bool NewXactStart();

			/// 提交一个事务
			virtual bool NewXactCommit();

			/// 回滚一个事务
			virtual bool NewXactAbort();

			/// 设置变量，执行批量插入
			virtual void setExecMultiInsert();

			/// 设置变量，准备批量插入
			virtual void prepareExecMultiInsert();
		private:
			bool flushStatementsArr();
		};	//PGGraphStorage
	} // GraphEngineNS
} //FounderXDB
#endif // PG_GRAPG_STORAGE_H 
