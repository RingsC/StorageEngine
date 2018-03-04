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
			 * 在成员m_loStore定义的时候，LargeObjectStore是无状态的类，所以 
			 * 可以让所有线程共享同一个m_loStore。如果在以后的代码中加入了状
			 * 态信息，那么该m_loStore应该是一个__declspec( thread )修饰的成员
			 */
			static PGLargeObjectStore *m_loStore;
		};
	
} //StorageEngineNS
} //FounderXDB

#endif //PG_LARGE_OBJ_STORE_HPP