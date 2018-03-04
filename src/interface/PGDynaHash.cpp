#include "MemoryContext.h"
#include "interface/PGDynaHash.h"
#include "Macros.h"
#include "interface/StorageEngineExceptionUniversal.h"

namespace FounderXDB{
	namespace StorageEngineNS {

		DynaHash *DynaHash::createNewDynaHash(DynaHashCTL &ctl, MemoryContext* cxt)
		{
			if(cxt == NULL)
				cxt = MemoryContext::getMemoryContext(MemoryContext::Top);
			return new (*cxt)PGDynaHash(ctl);
		}

		uint32 DynaHash::uint64Hash(const void *key, size_t keysize)
		{
			return uint64_hash(key, keysize);
		}

		uint32 DynaHash::uint32Hash(const void *key, size_t keysize)
		{
			return oid_hash(key, keysize);
		}

		uint32 DynaHash::uint32HashValue(const void *key, size_t keysize)
		{
			return uint32_hash_notaddress(key, keysize);
		}

		uint32 DynaHash::stringHash(const void *key, size_t keysize)
		{
			return string_hash(key, keysize);
		}

		uint32 DynaHash::objectHash(const void *key, size_t keysize)
		{
			return tag_hash(key, keysize);
		}

		PGDynaHash::PGDynaHash(DynaHashCTL &ctl)
		{
			assert(ctl.hash_name != NULL && ctl.nelem > 0);

			HASHCTL hctl;
			int flags = 0;
			memset(&hctl, 0, sizeof(HASHCTL));
			flags = initCTL(hctl, ctl);

			THROW_CALL(hash = hash_create, ctl.hash_name, ctl.nelem, &hctl, flags);
		}

		PGDynaHash::~PGDynaHash()
		{
			THROW_CALL(hash_destroy, hash);
		}

		void *PGDynaHash::hashInsert(void *key, bool *found /* = NULL */)
		{
			return doHash(key, found, HASH_ENTER);
		}

		void *PGDynaHash::hashSearch(void *key)
		{
			return doHash(key, NULL, HASH_FIND);
		}

		void *PGDynaHash::hashDelete(void *key)
		{
			return doHash(key, NULL, HASH_REMOVE);
		}

		EntrySetScan *PGDynaHash::startHashSeqScan(MemoryContext* cxt)
		{
			if(cxt == NULL)
			   cxt = MemoryContext::getMemoryContext(MemoryContext::Top);
			return new (*cxt)PGDynaHashScan(hash);
		}

		void PGDynaHash::finishHashSeqScan(EntrySetScan *scan)
		{
			delete scan;
		}

		void *PGDynaHash::doHash(void *key, bool *found, int action)
		{
			void *result = NULL;
			THROW_CALL(result = hash_search, hash, key, (HASHACTION)action, found);

			return result;
		}

		int PGDynaHash::initCTL(HASHCTL &hctl, DynaHashCTL &ctl)
		{
			hctl.num_partitions = ctl.num_partitions;
			hctl.ssize = ctl.ssize;
			hctl.dsize = ctl.dsize;
			hctl.max_dsize = ctl.max_dsize;
			hctl.ffactor = ctl.ffactor;
			hctl.keysize = ctl.keysize;
			hctl.entrysize = ctl.entrysize;
			hctl.hash = ctl.hash;
			hctl.match = ctl.match;
			hctl.keycopy = ctl.keycopy;
			hctl.alloc = ctl.alloc;

			int flags = 0;
			if (hctl.hash)
				flags |= HASH_FUNCTION;
			if (hctl.keysize && hctl.entrysize)
				flags |= HASH_ELEM;
			if (hctl.match)
				flags |= HASH_COMPARE;
			if (hctl.alloc)
				flags |= HASH_ALLOC;
			if (hctl.keycopy)
				flags |= HASH_KEYCOPY;

			return flags;
		}

		PGDynaHashScan::PGDynaHashScan(HTAB *tab)
		{
			hash = tab;
			hash_seq_init(&status, hash);
		}

		PGDynaHashScan::~PGDynaHashScan() {}

		int PGDynaHashScan::getNext(CursorMovementFlags opt, EntryID &eid, DataItem& entry)
		{
			void *result = NULL;
			THROW_CALL(result = hash_seq_search, &status);

			if (!result)
				return NO_DATA_FOUND;

			entry.setData(result);

			return SUCCESS;
		}

	} //StorageEngineNS
} //FounderXDB