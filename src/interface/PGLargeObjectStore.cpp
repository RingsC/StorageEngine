#include "interface/PGLargeObjectStore.h"

#include "interface/FDPGAdapter.h"
#include "interface/PGLargeObject.h"
#include "PgMemcontext.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/ErrNo.h"
#include "interface/PGTransaction.h"

#include "storage/large_object.h"

//conflict with define open pgwin32_open
#undef  open
#define NAME_MAX 255

namespace FounderXDB {
	namespace StorageEngineNS {

		PGLargeObjectStore *PGLargeObjectStore::m_loStore = NULL;

		LargeObjectStore *LargeObjectStore::getLargeObjectStore(DatabaseID /*dbid  = 0 */)
		{
			if(NULL == PGLargeObjectStore::m_loStore)
			{
				PGLargeObjectStore::m_loStore = new PGLargeObjectStore();
			}
			return PGLargeObjectStore::m_loStore;
			
		}

		bool LargeObjectStore::getListLargeObject(TableSpaceID tblspace, uint32 scan_num, 
			LargeObjectID *idx_loid, std::vector<LOInfo> &v_loinfo)
		{
			//v_loinfo.clear();

			uint32 id_idx = *idx_loid;
			std::vector<FDPG_LargeObject::lo_info> v_tmp_loinfo;
			bool ret_flag = FDPG_LargeObject::fd_inv_get_lo_list(tblspace, scan_num, &id_idx, v_tmp_loinfo);

			*idx_loid = id_idx;

			for(unsigned int i = 0; i < v_tmp_loinfo.size(); ++i)
			{
				LOInfo lo;
				lo.lo_id = v_tmp_loinfo[i].lo_id;
				lo.lo_name = v_tmp_loinfo[i].lo_name;
				lo.tblspc = v_tmp_loinfo[i].tblspc;
				v_loinfo.push_back(lo);
			}

			return ret_flag;
		}

		PGLargeObjectStore::PGLargeObjectStore()
		{
			
		}

		PGLargeObjectStore::~PGLargeObjectStore()
		{
			
		}

		LargeObject *PGLargeObjectStore::getRecentOpenLO(Transaction *txn, std::string name) const 
		{
			Assert(txn != NULL);

			PGTransaction *ptxn = static_cast<PGTransaction *>(txn);

			return ptxn->getRecentOpenLO(name);
		}

		LargeObjectID PGLargeObjectStore::create(
			FounderXDB::StorageEngineNS::Transaction *txn, 
			const char *name, 
			const void *extraData, 
			size_t extraDataLen)
		{
			if(NULL == name)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid name!");
			} else if (FDPG_LargeObject::fd_inv_name_get_id(const_cast<char*>(name)))
			{
				char msg[1024];
				memset(msg, 0, 1024);
				sprintf(msg, "LargeObject %s already exists.", name);
				throw StorageEngineExceptionUniversal(LARGE_OBJECT_ERROR, msg);
			}

			if(strlen(name) > NAME_MAX)
			{
				char msg[256];
				memset(msg, 0, sizeof(msg));
				int name_len = NAME_MAX;
				sprintf(msg, "\"name\" is too long(less than %d bytes)!", name_len);
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, msg);
			}

			uint32 lo_id = 0;
			FDPG_LargeObject::fd_inv_create(name, (const uint32)extraDataLen, extraData, lo_id);
			return lo_id;
		}

		LargeObject *PGLargeObjectStore::open(Transaction *txn, LargeObjectID loid, 
			LOOpenFlags flags)
		{
			if(loid <= 0)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid LargeObjectID!");
			}

			bool isWrite;

			if(flags & LargeObjectStore::WRITE)
			{
				isWrite = true;
			} else if(flags & LargeObjectStore::READ)
			{
				isWrite = false;
			} else 
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid flags!");
			}

			PGTransaction *tmpTxn = static_cast<PGTransaction*>(txn);

			MemoryContext *cxt = txn->getAssociatedMemoryContext();
			PgMemcontext *p_mem = static_cast<PgMemcontext*>(cxt);

			LargeObjectDesc *lod = NULL;

			LargeObject *retLo = tmpTxn->getLargeObject(loid, isWrite);

			if(!retLo)
			{
				FDPG_Transaction::fd_GetTransactionSnapshot();

				FDPG_LargeObject::fd_inv_open(loid, flags, p_mem->m_pContext, &lod);

				retLo = new (*cxt)PGLargeObject(lod, flags, tmpTxn->getRecentOpenLOHash());
				tmpTxn->cacheLargeObject(retLo, isWrite);
			}

			PGLargeObject *tmpLo = static_cast<PGLargeObject *>(retLo);
			tmpLo->cacheRecent();

			return retLo;
		}

		LargeObject *PGLargeObjectStore::open(Transaction *txn, const char *name, 
			LOOpenFlags flags)
		{
			if(NULL == name)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid name!");
			}

			bool isWrite;

			if(flags & LargeObjectStore::WRITE)
			{
				isWrite = true;
			} else if(flags & LargeObjectStore::READ)
			{
				isWrite = false;
			} else 
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid flags!");
			}

			PGTransaction *tmpTxn = static_cast<PGTransaction*>(txn);

			MemoryContext *cxt = txn->getAssociatedMemoryContext();
			PgMemcontext *p_mem = static_cast<PgMemcontext*>(cxt);

			LargeObjectDesc *lod = NULL;

			LargeObject *retLo = tmpTxn->getLargeObject(name, isWrite);

			if(!retLo)
			{
				FDPG_Transaction::fd_GetTransactionSnapshot();

				FDPG_LargeObject::fd_inv_open(name, flags, p_mem->m_pContext, &lod);

				retLo = new (*cxt)PGLargeObject(lod, flags, tmpTxn->getRecentOpenLOHash());
				tmpTxn->cacheLargeObject(retLo, isWrite);
			} 

			PGLargeObject *tmpLo = static_cast<PGLargeObject *>(retLo);
			tmpLo->cacheRecent();

			return retLo;
		}

		void PGLargeObjectStore::close(LargeObject *lo)
		{
			if(NULL == lo)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid LargeObject!");
			}

			PGLargeObject *tmpLo = static_cast<PGLargeObject *>(lo);
			tmpLo->unCacheRecent();
		}

		bool PGLargeObjectStore::drop(Transaction *txn, LargeObjectID loid)
		{
			if(loid <= 0)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid LargeObjectID!");
			}

			bool retval = false;
			FDPG_LargeObject::fd_inv_drop(loid, retval);

			PGTransaction *tmpTxn = static_cast<PGTransaction*>(txn);

			if(retval)
			{
				LargeObject *tmpLo = tmpTxn->getLargeObject(loid, true);
				if(tmpLo != NULL)
				{
					PGLargeObject *pgTmpLo = static_cast<PGLargeObject *>(tmpLo);
					pgTmpLo->unCacheRecent();
				} else {
					tmpLo = tmpTxn->getLargeObject(loid, false);
					if(tmpLo != NULL)
					{
						PGLargeObject *pgTmpLo = static_cast<PGLargeObject *>(tmpLo);
						pgTmpLo->unCacheRecent();
					}
				}

				tmpTxn->unCacheLargeObject(loid);
			}
			else
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "LargeObject not found");
			
			return retval;
		}

		std::string PGLargeObjectStore::loIDGetName(LargeObjectID lo_id)
		{
			if(lo_id <= 0)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid id!");
			}

			std::string name = FDPG_LargeObject::fd_inv_id_get_name(lo_id);

			return name;
		}

		LargeObjectID PGLargeObjectStore::nameGetLoID(std::string name)
		{
			if(name.length() <= 0)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid name!");
			}

			return FDPG_LargeObject::fd_inv_name_get_id(const_cast<char*>(name.c_str()));
		}

		bool PGLargeObjectStore::drop(Transaction *txn, const char *name)
		{
			if(NULL == name)
			{
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid name!");
			}

			bool retval = false;
			FDPG_LargeObject::fd_inv_drop(name, retval);

			PGTransaction *tmpTxn = static_cast<PGTransaction*>(txn);

			if(retval)
			{
				LargeObject *tmpLo = tmpTxn->getLargeObject(name, true);
				if(tmpLo != NULL)
				{
					PGLargeObject *pgTmpLo = static_cast<PGLargeObject *>(tmpLo);
					pgTmpLo->unCacheRecent();
				} else {
					tmpLo = tmpTxn->getLargeObject(name, false);
					if(tmpLo != NULL)
					{
						PGLargeObject *pgTmpLo = static_cast<PGLargeObject *>(tmpLo);
						pgTmpLo->unCacheRecent();
					}
				}

				tmpTxn->unCacheLargeObject(name);
			}
			else
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "LargeObject not found");

			return retval;
		}

	} //FounderXDB
} //StorageEngineNS

#undef open