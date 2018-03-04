#ifndef GRAPG_STORAGE_H
#define GRAPG_STORAGE_H

/// @file GraphStorage.h
/// @brief Defines the RDFStorage public interface.

#include "Transaction.h"
#include "PGSETypes.h"
#include "StorageEngineException.h"

struct Tuplestorestate;

namespace FounderXDB{
	namespace GraphEngineNS {
		/// 定义图数据的读写操作
		class PGSE_DLLEXTERN RDFStorage {
		public:

			DEFINE_OPERATOR_NEW_DELETE;
			virtual ~RDFStorage(){}
			/// 创建statement元数据表
			virtual bool CreateStatementsTable(uint64 id, const char *name) = 0;

			/// 根据id或者name获取storage表的id
			virtual bool GetStorageEntrySetID(uint64 id, const char *name) = 0;

			/* 
			 * 向statement元数据表中插入一个或者多个statement。
			 * 注意该函数的批量插入功能在一个插入过程中是完全依赖于底层
			 * 的cache机制，原因是无论是statement表还是rbl表，当前
			 * 的批量插入过程中并没有真正的插入到表中，而是全部缓存
			 * 到cache中，通过该cache来检测当前插入例程的唯一性问题。
			 * 当然之前的事务插入到表中的数据还是可以检测到的。
			 */
			virtual bool StatementAdd(RDF_Statements statement) = 0;

			/// 从statement元数据表中删除一个statement
			virtual bool StatementRemove(RDF_Statements statement) = 0;

			/// 根据context删除一个三元组
			virtual bool StatementContextRemove(uint64 context) = 0;

			/// 统计删除三元组的数量
			virtual unsigned int CounterRemoveNum() = 0;

			/// 计算一个storage中有多少个三元组
			virtual bool CounterStorageSize(long *count) = 0;

			// 查找一个三元组是否存在
			virtual bool StatementExists(RDF_Statements statement) = 0;

			/// 根据给定的statement查找三元组，返回结果集
			virtual SysScanDesc StatementSearch(RDF_Statements statement) = 0;

			///	根据给定的context查找三元组，返回结果集
			virtual SysScanDesc StatementContextSearch(uint64 context) = 0;

			/// 返回结果集中下一个三元组
			virtual RDF_Statements StatementGetNext(SysScanDesc sd) const = 0;

			/// 释放一个三元组
			virtual void StetementFree(RDF_Statements statement) = 0;

			/// 清空结果集
			virtual void ResultsClear(SysScanDesc sd)  = 0;

			/// 删除一个storage
			virtual bool DropStorage() = 0;

			/// 获取所有图信息
			static RDFMeta_Models GetAllGraph(unsigned int *count);

			/// 启动一个新事务
			virtual bool NewXactStart() = 0;

			/// 提交一个事务
			virtual bool NewXactCommit() = 0;

			/// 回滚一个事务
			virtual bool NewXactAbort() = 0;

			/// 设置变量，执行批量插入
			virtual void setExecMultiInsert() = 0;

			/// 设置变量，准备批量插入
			virtual void prepareExecMultiInsert() = 0;
		};//RDFStorage

	} // GraphEngineNS
} // FounderXDB
#endif // GRAPG_STORAGE_H
