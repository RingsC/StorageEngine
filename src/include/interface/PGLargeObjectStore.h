#ifndef PG_LARGE_OBJ_STORE_HPP
#define PG_LARGE_OBJ_STORE_HPP

#include <string>
#include <map>
#include "LargeObject.h"

struct LargeObjectDesc;

namespace FounderXDB {
	namespace StorageEngineNS {

		class PGLargeObjectStore : public LargeObjectStore {

			friend class LargeObjectStore;

		public:
			virtual ~PGLargeObjectStore();

			virtual LargeObject *getRecentOpenLO(Transaction *txn, std::string name) const ;

			virtual LargeObjectID create(Transaction *, const char *name, 
				const void *extraData /* = 0 */, size_t extraDataLen /* = 0 */);

			virtual LargeObject *open(Transaction *, LargeObjectID loid, 
				LOOpenFlags flags);
			virtual LargeObject *open(Transaction *, const char *name, 
				LOOpenFlags flags);

			virtual void close(LargeObject *lo);

			virtual bool drop(Transaction *, const char *name);
			virtual bool drop(Transaction *, LargeObjectID loid);

			virtual std::string loIDGetName(LargeObjectID lo_id);
			virtual LargeObjectID nameGetLoID(std::string name);

		private:
			PGLargeObjectStore();

		private:

			/* 
			 * �ڳ�Աm_loStore�����ʱ��LargeObjectStore����״̬���࣬���� 
			 * �����������̹߳���ͬһ��m_loStore��������Ժ�Ĵ����м�����״
			 * ̬��Ϣ����ô��m_loStoreӦ����һ��__declspec( thread )���εĳ�Ա
			 */
			static PGLargeObjectStore *m_loStore;
		};
	
} //StorageEngineNS
} //FounderXDB

#endif //PG_LARGE_OBJ_STORE_HPP