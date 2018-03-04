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
			/// ����statementԪ���ݱ�
			virtual bool CreateStatementsTable(uint64 id, const char *name);

			/// ����id����name��ȡstorage���id
			virtual bool GetStorageEntrySetID(uint64 id, const char *name);

			/// ��statementԪ���ݱ��в���һ�����߶��statement
			virtual bool StatementAdd(RDF_Statements statement);

			/// ��statementԪ���ݱ���ɾ��һ��statement
			virtual bool StatementRemove(RDF_Statements statement);

			/// ����contextɾ��һ����Ԫ��
			virtual bool StatementContextRemove(uint64 context);

			/// ͳ��ɾ����Ԫ�������
			virtual unsigned int CounterRemoveNum();

			/// ����һ��storage���ж��ٸ���Ԫ��
			virtual bool CounterStorageSize(long *count);

			// ����һ����Ԫ���Ƿ����
			virtual bool StatementExists(RDF_Statements statement);

			/// ���ݸ�����statement������Ԫ�飬���ؽ����
			virtual SysScanDesc StatementSearch(RDF_Statements statement);

			///	���ݸ�����context������Ԫ�飬���ؽ����
			virtual SysScanDesc StatementContextSearch(uint64 context);

			/// ���ؽ��������һ����Ԫ��
			virtual RDF_Statements StatementGetNext(SysScanDesc sd) const;

			/// �ͷ�һ����Ԫ��
			virtual void StetementFree(RDF_Statements statement);

			/// ��ս����
			virtual void ResultsClear(SysScanDesc sd);

			/// ɾ��һ��storage
			virtual bool DropStorage();

			/// ����һ��������
			virtual bool NewXactStart();

			/// �ύһ������
			virtual bool NewXactCommit();

			/// �ع�һ������
			virtual bool NewXactAbort();

			/// ���ñ�����ִ����������
			virtual void setExecMultiInsert();

			/// ���ñ�����׼����������
			virtual void prepareExecMultiInsert();
		private:
			bool flushStatementsArr();
		};	//PGGraphStorage
	} // GraphEngineNS
} //FounderXDB
#endif // PG_GRAPG_STORAGE_H 
