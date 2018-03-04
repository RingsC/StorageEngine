#include "interface/PGLargeObject.h"

#include "interface/FDPGAdapter.h"
#include "interface/ErrNo.h"
#include "interface/StorageEngineExceptionUniversal.h"

namespace FounderXDB{
	namespace StorageEngineNS {

		PGLargeObject::PGLargeObject(LargeObjectDesc *lo_desc,
			LargeObjectStore::LOOpenFlags flags,
			std::map<std::pair<DatabaseID, std::string>, LargeObject *> *recentOpenLO) :
			m_loDesc(lo_desc), m_flags(flags), m_recentOpenLO(recentOpenLO) {

				Assert(lo_desc != NULL);

				m_formClass = NULL;
				FDPG_LargeObject::fd_inv_getmeta(m_loDesc, &m_formClass);
			}

		PGLargeObject::~PGLargeObject()
		{
			if(m_loDesc)
				FDPG_LargeObject::fd_inv_close(m_loDesc);

			m_loDesc = NULL;

			if(m_formClass)
				FDPG_Memory::fd_pfree(m_formClass);

			m_formClass = NULL;
		}

		LOSizeType PGLargeObject::seek(Transaction *, LOSizeType offset, LOWhence whence)
		{
			if(m_loDesc == NULL)
			{
				throw StorageEngineExceptionUniversal(LOGIC_ERR, "LargeObject not open.");
			}

			uint64 roffset = 0;
			FDPG_LargeObject::fd_inv_seek(m_loDesc, offset, whence, roffset);
			return roffset;
		}

		size_t PGLargeObject::read(Transaction *, char *buf, size_t buflen)
		{
			if(m_loDesc == NULL)
			{
				throw StorageEngineExceptionUniversal(LOGIC_ERR, "LargeObject not open.");
			}

			if(NULL == buf || buflen <= 0)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid buf.");
			}

			if(!(m_flags & LargeObjectStore::READ))
			{
				char msg[256];
				memset(msg, 0, sizeof(msg));
				sprintf(msg, "Large object %s was not opened for reading", getName());
				throw StorageEngineExceptionUniversal(LARGE_OBJECT_ERROR, msg);
			}

			int nread = 0;
			FDPG_LargeObject::fd_inv_read(m_loDesc, (const int)buflen, buf, nread);
			return nread;
		}

		size_t PGLargeObject::write(Transaction *, const char *buf, size_t nbytes)
		{
			if(m_loDesc == NULL)
			{
				throw StorageEngineExceptionUniversal(LOGIC_ERR, "LargeObject not open.");
			}

			if(NULL == buf || nbytes <= 0)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid buf.");
			}

			if(!(m_flags & LargeObjectStore::WRITE))
			{
				char msg[256];
				memset(msg, 0, sizeof(msg));
				sprintf(msg, "Large object %s was not opened for writing", getName());
				throw StorageEngineExceptionUniversal(LARGE_OBJECT_ERROR, msg);
			}

			int nwritten = 0;
			FDPG_LargeObject::fd_inv_write(m_loDesc, buf, (const int)nbytes, nwritten);
			return nwritten;
		}

		LOSizeType PGLargeObject::tell() const
		{
			if(m_loDesc == NULL)
			{
				throw StorageEngineExceptionUniversal(LOGIC_ERR, "LargeObject not open.");
			}

			uint64 offset = 0;
			FDPG_LargeObject::fd_inv_tell(m_loDesc, offset);
			return offset;
		}

		void PGLargeObject::truncate(Transaction *, LOSizeType nbytes)
		{
			if(m_loDesc == NULL)
			{
				throw StorageEngineExceptionUniversal(LOGIC_ERR, "LargeObject not open.");
			}

			if(nbytes < 0)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid size.");
			}

			if(!(m_flags & LargeObjectStore::WRITE))
			{
				char msg[256];
				memset(msg, 0, sizeof(msg));
				sprintf(msg, "Large object %s was not opened for writing", getName());
				throw StorageEngineExceptionUniversal(LARGE_OBJECT_ERROR, msg);
			}

			FDPG_LargeObject::fd_inv_truncate(m_loDesc, nbytes);
		}
        
		void PGLargeObject::getExtraData(DataItem &data) const
		{
			if(!m_formClass)
			{
				throw StorageEngineExceptionUniversal(LOGIC_ERR, "LargeObject not exists.");
			}

			void *retData = NULL;
            size_t len;
			FDPG_LargeObject::fd_inv_metaget_size(m_formClass, len);
			FDPG_LargeObject::fd_inv_metaget_extradata(m_formClass, &retData);
            data.setSize(len);
            data.setData(retData);
		}
		
        bool PGLargeObject::updateExtraData(const void *data, size_t len, DataItem &oldExtraData)
		{
			if(!m_formClass)
			{
				throw StorageEngineExceptionUniversal(LOGIC_ERR, "LargeObject not exists.");
			}

			/* 
			 * 注意这里在事务中会存在内存泄露，原因是要返回原先的extraData，所以 
			 * 之前的m_formClass需要等到事务提交后才能free。
			 */
			//size_t oldlen = 0;
			this->getExtraData(oldExtraData);

			return (FDPG_LargeObject::fd_inv_metaupdate_extradata(m_loDesc, data, len, &m_formClass));
		}

		const char *PGLargeObject::getName() const
		{
			if(!m_formClass)
			{
				throw StorageEngineExceptionUniversal(LOGIC_ERR, "LargeObject not exists.");
			}

			char *retData = NULL;

			FDPG_LargeObject::fd_inv_metaget_name(m_formClass, &retData);

			return retData;
		}

		LargeObjectID PGLargeObject::getId() const
		{
			if(!m_formClass)
			{
				throw StorageEngineExceptionUniversal(LOGIC_ERR, "LargeObject not exists.");
			}

			LargeObjectID lo_id = InvalidOid;

			FDPG_LargeObject::fd_inv_metaget_id(m_formClass, lo_id);

			return lo_id;
		}

		void PGLargeObject::cacheRecent()
		{
			m_recentOpenLO->operator [](std::pair<DatabaseID, std::string>(MyDatabaseId, getName())) = this;
		}

		void PGLargeObject::unCacheRecent()
		{
			std::map<std::pair<DatabaseID, std::string>, LargeObject *>::iterator it = 
				m_recentOpenLO->find(std::pair<DatabaseID, std::string>(MyDatabaseId, getName()));

			if(it != m_recentOpenLO->end())
			{
				m_recentOpenLO->erase(it);
			}
		}

	} //StorageEngineNS
} //FounderXDB