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
		/// ����ͼ���ݵĶ�д����
		class PGSE_DLLEXTERN RDFStorage {
		public:

			DEFINE_OPERATOR_NEW_DELETE;
			virtual ~RDFStorage(){}
			/// ����statementԪ���ݱ�
			virtual bool CreateStatementsTable(uint64 id, const char *name) = 0;

			/// ����id����name��ȡstorage���id
			virtual bool GetStorageEntrySetID(uint64 id, const char *name) = 0;

			/* 
			 * ��statementԪ���ݱ��в���һ�����߶��statement��
			 * ע��ú������������빦����һ���������������ȫ�����ڵײ�
			 * ��cache���ƣ�ԭ����������statement����rbl����ǰ
			 * ��������������в�û�������Ĳ��뵽���У�����ȫ������
			 * ��cache�У�ͨ����cache����⵱ǰ�������̵�Ψһ�����⡣
			 * ��Ȼ֮ǰ��������뵽���е����ݻ��ǿ��Լ�⵽�ġ�
			 */
			virtual bool StatementAdd(RDF_Statements statement) = 0;

			/// ��statementԪ���ݱ���ɾ��һ��statement
			virtual bool StatementRemove(RDF_Statements statement) = 0;

			/// ����contextɾ��һ����Ԫ��
			virtual bool StatementContextRemove(uint64 context) = 0;

			/// ͳ��ɾ����Ԫ�������
			virtual unsigned int CounterRemoveNum() = 0;

			/// ����һ��storage���ж��ٸ���Ԫ��
			virtual bool CounterStorageSize(long *count) = 0;

			// ����һ����Ԫ���Ƿ����
			virtual bool StatementExists(RDF_Statements statement) = 0;

			/// ���ݸ�����statement������Ԫ�飬���ؽ����
			virtual SysScanDesc StatementSearch(RDF_Statements statement) = 0;

			///	���ݸ�����context������Ԫ�飬���ؽ����
			virtual SysScanDesc StatementContextSearch(uint64 context) = 0;

			/// ���ؽ��������һ����Ԫ��
			virtual RDF_Statements StatementGetNext(SysScanDesc sd) const = 0;

			/// �ͷ�һ����Ԫ��
			virtual void StetementFree(RDF_Statements statement) = 0;

			/// ��ս����
			virtual void ResultsClear(SysScanDesc sd)  = 0;

			/// ɾ��һ��storage
			virtual bool DropStorage() = 0;

			/// ��ȡ����ͼ��Ϣ
			static RDFMeta_Models GetAllGraph(unsigned int *count);

			/// ����һ��������
			virtual bool NewXactStart() = 0;

			/// �ύһ������
			virtual bool NewXactCommit() = 0;

			/// �ع�һ������
			virtual bool NewXactAbort() = 0;

			/// ���ñ�����ִ����������
			virtual void setExecMultiInsert() = 0;

			/// ���ñ�����׼����������
			virtual void prepareExecMultiInsert() = 0;
		};//RDFStorage

	} // GraphEngineNS
} // FounderXDB
#endif // GRAPG_STORAGE_H
