#ifndef PG_LARGE_OBJ_HPP
#define PG_LARGE_OBJ_HPP

#include <map>
#include <string>
#include "LargeObject.h"

struct LargeObjectDesc;
struct Form_meta_large_data;

namespace FounderXDB{
	namespace StorageEngineNS {

		class PGLargeObject : public LargeObject
		{
			
			friend class PGLargeObjectStore;

		public:
			virtual ~PGLargeObject();

			virtual LOSizeType seek(Transaction *, LOSizeType offset, LOWhence whence);
			virtual size_t read(Transaction *, char *buf, size_t buflen);
			virtual size_t write(Transaction *, const char *buf, size_t nbytes);
			virtual LOSizeType tell() const;
			virtual void truncate(Transaction *, LOSizeType nbytes);
			virtual void getExtraData(DataItem &data) const;
			virtual bool updateExtraData(const void *data, size_t len, DataItem &oldExtraData);
			
			virtual const char *getName() const;
			virtual LargeObjectID getId() const;

		private:
			PGLargeObject(LargeObjectDesc *lo_desc, 
				LargeObjectStore::LOOpenFlags flags,
				std::map<std::pair<DatabaseID, std::string>, LargeObject *> *recentOpenLO);

			void cacheRecent();
			void unCacheRecent();

		private:
			LargeObjectDesc *m_loDesc;
			Form_meta_large_data *m_formClass;
			LargeObjectStore::LOOpenFlags m_flags;
			std::map<std::pair<DatabaseID, std::string>, LargeObject *> *m_recentOpenLO;
		};

	} //StorageEngineNS
} //FounderXDB

#endif //PG_LARGE_OBJ_HPP