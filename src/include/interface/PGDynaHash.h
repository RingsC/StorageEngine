#ifndef PG_DYNA_HASH_HPP
#define PG_DYNA_HASH_HPP

#include "DynaHash.h"

#include "postgres.h"
#include "utils/hsearch.h"

namespace FounderXDB{
	namespace StorageEngineNS {

		class PGDynaHash : public DynaHash
		{
			friend class DynaHash;

		public:
			virtual ~PGDynaHash();

			/// Do hash insert.
			virtual void *hashInsert(void *key, bool *found = NULL);

			/// Do hash search.
			virtual void *hashSearch(void *key);

			/// Do hash remove.
			virtual void *hashDelete(void *key);

			/// Start a hash seq scan.
			virtual EntrySetScan *startHashSeqScan(MemoryContext* cxt = NULL);

			/// Finish a scan.
			virtual void finishHashSeqScan(EntrySetScan *);

		private:
			explicit PGDynaHash(DynaHashCTL &ctl);

			int initCTL(HASHCTL &, DynaHashCTL &);
			void *doHash(void *key, bool *found, int action);

		private:
			HTAB *hash;
		};

		class PGDynaHashScan : public EntrySetScan
		{
			friend class PGDynaHash;
		private:
			/// Constructor
			explicit PGDynaHashScan(HTAB *);
			virtual ~PGDynaHashScan();

			/// Mark the current position.
			virtual int markPosition(){ return 0; }

			/// Restore to the latest marked position.
			virtual int restorePosition() { return 0; }

			/// Get next batch of entries.
			/// @param[in]  opt     access option
			/// @param[out] eid_entries     array of (entry-id, entry) pairs
			/// @return     value yet to be defined
			virtual int getNextBatch(CursorMovementFlags /*opt*/, 
				std::vector<std::pair<EntryID, DataItem> > &/*eid_entries*/){ return 0; }
		public:
			/// Get next entry.
			/// @param[in]  opt     useless
			/// @param[out] eid     useless 
			/// @param[out] entry   The entry to take out 
			/// @return     value yet to be defined
			virtual int getNext(CursorMovementFlags opt, EntryID &eid, DataItem& entry);
		private:
			HASH_SEQ_STATUS status;
			HTAB *hash;
		};

	} //StorageEngineNS
} //FounderXDB

#endif //PG_DYNA_HASH_HPP