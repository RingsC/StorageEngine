/*-------------------------------------------------------------------------
 *
 * index.h
 *	  prototypes for catalog/index.c.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/index.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef INDEX_H
#define INDEX_H

#include "nodes/execnodes.h"


#define DEFAULT_INDEX_TYPE	"btree"

/* Typedef for callback function for IndexBuildHeapScan */
typedef void (*IndexBuildCallback) (Relation index,
												HeapTuple htup,
												Datum *values,
												bool *isnull,
												bool tupleIsAlive,
												void *state);

#ifndef FOUNDER_XDB_SE
extern void index_check_primary_key(Relation heapRel,
						IndexInfo *indexInfo,
						bool is_alter_table);
#endif //FOUNDER_XDB_SE

extern Oid index_create(Relation heapRelation,
			 const char *indexRelationName,
			 Oid indexRelationId,
			 IndexInfo *indexInfo,
			 List *indexColNames,
			 Oid accessMethodObjectId,
			 Oid tableSpaceId,
			 Oid *collationObjectId,
			 Oid *classObjectId,
			 int16 *coloptions,
			 Datum reloptions,
			 bool isprimary,
			 bool isconstraint,
			 bool deferrable,
			 bool initdeferred,
			 bool allow_system_table_mods,
			 bool skip_build,
			 bool concurrent);

extern void index_constraint_create(Relation heapRelation,
						Oid indexRelationId,
						IndexInfo *indexInfo,
						const char *constraintName,
						char constraintType,
						bool deferrable,
						bool initdeferred,
						bool mark_as_primary,
						bool update_pgindex,
						bool allow_system_table_mods);

extern void index_drop(Oid indexId, Oid indrelId, Oid dbid);

extern IndexInfo *BuildIndexInfo(Relation index);

extern void FormIndexDatum(Relation heapRelation,
			   Relation indexRelation,
			   IndexInfo *indexInfo,
			   HeapTuple slot,
			   //EState *estate,
			   Datum *values,
			   bool *isnull);

extern void index_build(Relation heapRelation,
			Relation indexRelation,
			IndexInfo *indexInfo,
			bool isprimary,
			bool isreindex, void* userData = 0, filter_func_t filterfunc = 0);

extern double IndexBuildHeapScan(Relation heapRelation,
				   Relation indexRelation,
				   IndexInfo *indexInfo,
				   bool allow_sync,
				   IndexBuildCallback callback,
				   void *callback_state, void* userData = 0, filter_func_t filterfunc = 0);

extern void validate_index(Oid heapId, Oid indexId, Snapshot snapshot);
#ifndef FOUNDER_XDB_SE
extern void reindex_index(Oid indexId, Oid tablespace, bool skip_constraint_checks);
#else
extern void reindex_index(Oid heapId, Oid indexId, Oid dbid);
#endif
/* Flag bits for reindex_relation(): */
#define REINDEX_REL_PROCESS_TOAST		0x01
#define REINDEX_REL_SUPPRESS_INDEX_USE	0x02
#define REINDEX_REL_CHECK_CONSTRAINTS	0x04
#ifndef FOUNDER_XDB_SE
extern bool reindex_relation(Oid relid, Oid tablespace, int flags);
#else
extern bool reindex_relation(Oid relid, Oid dbid,int flags);
#endif
#ifdef FOUNDER_XDB_SE
extern void reindex_database(Oid dbid);
extern Datum index_get_max(Relation heapRelation, Relation indexRelation, Snapshot snapshot);
extern Datum index_get_min(Relation heapRelation, Relation indexRelation, Snapshot snapshot);
#endif
extern bool ReindexIsProcessingHeap(Oid heapOid);
extern bool ReindexIsProcessingIndex(Oid indexOid);

#endif   /* INDEX_H */
