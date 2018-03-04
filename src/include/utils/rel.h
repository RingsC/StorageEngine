/*-------------------------------------------------------------------------
*
* rel.h
*	  POSTGRES relation descriptor (a/k/a relcache entry) definitions.
*
*
* Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
* Portions Copyright (c) 1994, Regents of the University of California
*
* src/include/utils/rel.h
*
*-------------------------------------------------------------------------
*/
#ifndef REL_H
#define REL_H

#include "access/tupdesc.h"
#include "catalog/pg_am.h"
#include "catalog/pg_class.h"
#include "catalog/pg_index.h"
#include "fmgr.h"
#include "nodes/bitmapset.h"
#ifndef FOUNDER_XDB_SE
#include "rewrite/prs2lock.h"
#endif //FOUNDER_XDB_SE
#include "storage/block.h"
#include "storage/relfilenode.h"
#include "utils/relcache.h"


#define Anum_pg_class_relkind			16

#ifdef FOUNDER_XDB_SE
typedef struct RangeData
{// the order of members must be identical to RangeDatai in EntrySet.h
    struct AddrLen{
    public:
    void *addr;
    size_t len;
    };
    void *itemSpace;
    size_t curpos;// next free byte position of itemSpace.
    size_t totalSpace; // total bytes of itemSpace.
    AddrLen *items;// we can't use stl containers because this class is also used in storageenginelib.
    size_t nItems; // NO. of valid AddrLen objects.
    size_t szItems;// NO. of AddrLen object slots in items array.
    bool ownMem;// whether release memory of itemSpace and items.
    
	size_t start;
	size_t len;

	void *userData;// set as Relation::userData
} RangeData;
void releaseRangeDataMemory(RangeData*);
const int MAX_LEN = 1024;
typedef int (*CompareCallback)(const char *str1, size_t len1, const char *str2, size_t len2);
typedef void (*Split)(RangeData&, const char*, int, size_t charlen);
typedef struct ColinfoData {
	size_t tuple_size;
	size_t keys;
	size_t *col_number;
	CompareCallback *rd_comfunction;
	Split split_function;
} ColinfoData;
typedef ColinfoData *Colinfo;
typedef struct IndinfoData{
	unsigned int  index_num;   //索引的个数
	unsigned int *type;
	unsigned int *index_array;
	Colinfo *index_info;
} IndinfoData;
typedef IndinfoData *Indinfo;

typedef struct MetaTableInfo{
	unsigned int tblspace;//current relation table space
	unsigned int type;		//current relation type
	/*
	*indinfo is not null when relation is data table.
	*indinfo is null when relation is index.
	*/
	IndinfoData *indinfo;	//all index info of current data table 
} MetaTableInfo;

typedef struct _MtInfoData{
	unsigned int spcid;//current relation table space
	unsigned int parentId;		//current relation type
	Oid database_id;
	unsigned int type;
	char relpersistence;
	Oid rel_filenode;			//Physical file name
	Colinfo tableColInfo;
	Oid colid;
	IndinfoData indinfo;	//all index info of current data table 
	uint32 userdataLength;
	void *userdata;
} MtInfoData;
typedef MetaTableInfo *MetaTable;
typedef MtInfoData *MtInfo;
#define MtInfo_GetType(mtinfo)  mtinfo.type
#define MtInfo_GetTablespace(mtinfo) mtinfo.spcid
#define MtInfo_GetColinfo(mtinfo) mtinfo.tableColInfo
#define MtInfo_GetColinfoCount(mtinfo) mtinfo.tableColInfo->col_number
#define MtInfo_GetColinfoComfunction(mtinfo) mtinfo.tableColInfo->rd_comfunction
#define MtInfo_GetIndexInfo(mtinfo) mtinfo.indinfo
#define MtInfo_GetIndexCount(mtinfo) mtinfo.indinfo.index_num
#define MtInfo_GetIndexType(mtinfo) mtinfo.indinfo.type
#define MtInfo_GetIndexOid(mtinfo) mtinfo.indinfo.index_array
#define MtInfo_GetIndexColinfo(mtinfo) mtinfo.indinfo.index_info
#define MtInfo_GetUserDataLength(mtinfo) mtinfo.userdataLength
#define MtInfo_GetUserData(mtinfo) mtinfo.userdata
#define MtInfo_Free(mtinfo)  \
do{\
	if(0 != MtInfo_GetIndexCount(mtinfo))\
	{\
		pfree(MtInfo_GetIndexType(mtinfo));\
		pfree(MtInfo_GetIndexOid(mtinfo));\
		pfree(MtInfo_GetIndexColinfo(mtinfo));\
		MtInfo_GetIndexCount(mtinfo) = 0;\
		MtInfo_GetIndexType(mtinfo) = NULL;\
		MtInfo_GetIndexOid(mtinfo) = NULL;\
		MtInfo_GetIndexColinfo(mtinfo) = NULL;\
		}\
	if(MtInfo_GetColinfo(mtinfo) != NULL){\
		if(MtInfo_GetColinfoCount(mtinfo) != NULL){\
			pfree(MtInfo_GetColinfoCount(mtinfo));\
			MtInfo_GetColinfoCount(mtinfo) = NULL; \
		}\
		if(MtInfo_GetColinfoComfunction(mtinfo) != NULL){\
			pfree(MtInfo_GetColinfoComfunction(mtinfo));\
			MtInfo_GetColinfoComfunction(mtinfo) = NULL;\
		}\
		pfree(MtInfo_GetColinfo(mtinfo));\
		MtInfo_GetColinfo(mtinfo) = NULL;\
	}\
	if(MtInfo_GetUserData(mtinfo) != NULL) \
	{\
		MtInfo_GetUserDataLength(mtinfo) = 0;\
		pfree(MtInfo_GetUserData(mtinfo));\
		MtInfo_GetUserData(mtinfo) = NULL;\
	}\
}while(0)

#if 0
extern uint32 g_lsm_tree_threshold_lsm;
extern uint32 g_lsm_subtree_threshold_lsm;
#define BlockNumberThresholdByRelType(type) \
(((type) == LSM_TYPE) ? g_lsm_tree_threshold_lsm \
: ((type) == LSM_SUBTYPE) ? g_lsm_subtree_threshold_lsm : InvalidBlockNumber)
#else
extern uint32 g_lsm_subtree_threshold_lsm;
#define BlockNumberThresholdByRelType(type) \
(((type) == LSM_SUBTYPE) ? g_lsm_subtree_threshold_lsm : InvalidBlockNumber)
#endif

#define BlockNumberThresholdByRel(rel) BlockNumberThresholdByRelType((rel)->mt_info.type)
#endif //FOUNDER_XDB_SE


/*
* LockRelId and LockInfo really belong to lmgr.h, but it's more convenient
* to declare them here so we can have a LockInfoData field in a Relation.
*/

typedef struct LockRelId
{
	Oid			relId;			/* a relation identifier */
	Oid			dbId;			/* a database identifier */
} LockRelId;

typedef struct LockInfoData
{
	LockRelId	lockRelId;
} LockInfoData;

typedef LockInfoData *LockInfo;

/*
* Likewise, this struct really belongs to trigger.h, but for convenience
* we put it here.
*/
typedef struct Trigger
{
	Oid			tgoid;			/* OID of trigger (pg_trigger row) */
	/* Remaining fields are copied from pg_trigger, see pg_trigger.h */
	char	   *tgname;
	Oid			tgfoid;
	int16		tgtype;
	char		tgenabled;
	bool		tgisinternal;
	Oid			tgconstrrelid;
	Oid			tgconstrindid;
	Oid			tgconstraint;
	bool		tgdeferrable;
	bool		tginitdeferred;
	int16		tgnargs;
	int16		tgnattr;
	int16	   *tgattr;
	char	  **tgargs;
	char	   *tgqual;
} Trigger;

typedef struct TriggerDesc
{
	Trigger    *triggers;		/* array of Trigger structs */
	int			numtriggers;	/* number of array entries */

	/*
	* These flags indicate whether the array contains at least one of each
	* type of trigger.  We use these to skip searching the array if not.
	*/
	bool		trig_insert_before_row;
	bool		trig_insert_after_row;
	bool		trig_insert_instead_row;
	bool		trig_insert_before_statement;
	bool		trig_insert_after_statement;
	bool		trig_update_before_row;
	bool		trig_update_after_row;
	bool		trig_update_instead_row;
	bool		trig_update_before_statement;
	bool		trig_update_after_statement;
	bool		trig_delete_before_row;
	bool		trig_delete_after_row;
	bool		trig_delete_instead_row;
	bool		trig_delete_before_statement;
	bool		trig_delete_after_statement;
	/* there are no row-level truncate triggers */
	bool		trig_truncate_before_statement;
	bool		trig_truncate_after_statement;
} TriggerDesc;


/*
* Cached lookup information for the index access method functions defined
* by the pg_am row associated with an index relation.
*/
typedef struct RelationAmInfo
{
	FmgrInfo	aminsert;
	FmgrInfo	ambeginscan;
	FmgrInfo	amgettuple;
	FmgrInfo	amgetbitmap;
	FmgrInfo	amrescan;
	FmgrInfo	amendscan;
	FmgrInfo	ammarkpos;
	FmgrInfo	amrestrpos;
	FmgrInfo	ambuild;
	FmgrInfo	ambuildcluster;
	FmgrInfo	ambuildempty;
	FmgrInfo	ambulkdelete;
	FmgrInfo	amvacuumcleanup;
	FmgrInfo	amcostestimate;
	FmgrInfo	amoptions;
} RelationAmInfo;


/*
* Here are the contents of a relation cache entry.
*/

typedef struct RelationData
{
	RelFileNode rd_node;		/* relation physical identifier */
	/* use "struct" here to avoid needing to include smgr.h: */
	struct SMgrRelationData *rd_smgr;	/* cached file handle, or NULL */
	int			rd_refcnt;		/* reference count */
	BackendId	rd_backend;		/* owning backend id, if temporary relation */
	bool		rd_isnailed;	/* rel is nailed in cache */
	bool		rd_isvalid;		/* relcache entry is valid */
	char		rd_indexvalid;	/* state of rd_indexlist: 0 = not valid, 1 =
													* valid, 2 = temporarily forced */
#ifdef FOUNDER_XDB_SE
	char rel_kind;
#endif
	/*
	* rd_createSubid is the ID of the highest subtransaction the rel has
	* survived into; or zero if the rel was not created in the current top
	* transaction.  This should be relied on only for optimization purposes;
	* it is possible for new-ness to be "forgotten" (eg, after CLUSTER).
	* Likewise, rd_newRelfilenodeSubid is the ID of the highest
	* subtransaction the relfilenode change has survived into, or zero if not
	* changed in the current transaction (or we have forgotten changing it).
	*/
	SubTransactionId rd_createSubid;	/* rel was created in current xact */
	SubTransactionId rd_newRelfilenodeSubid;	/* new relfilenode assigned in
																						* current xact */

	Form_pg_class rd_rel;		/* RELATION tuple */
	TupleDesc	rd_att;			/* tuple descriptor */
	Oid			rd_id;			/* relation's object id */

#ifndef FOUNDER_XDB_SE
	List	   *rd_indexlist;	/* list of OIDs of indexes on relation */
	Bitmapset  *rd_indexattr;	/* identifies columns used in indexes */
	Oid			rd_oidindex;	/* OID of unique index on OID, if any */
#endif //FOUNDER_XDB_SE
	LockInfoData rd_lockInfo;	/* lock mgr's info for locking relation */

#ifndef FOUNDER_XDB_SE
	RuleLock   *rd_rules;		/* rewrite rules */
	MemoryContext rd_rulescxt;	/* private memory cxt for rd_rules, if any */
	TriggerDesc *trigdesc;		/* Trigger info, or NULL if rel has none */
#endif //FOUNDER_XDB_SE
	/*
	* rd_options is set whenever rd_rel is loaded into the relcache entry.
	* Note that you can NOT look into rd_rel for this data.  NULL means "use
	* defaults".
	*/
	bytea	   *rd_options;		/* parsed pg_class.reloptions */


	/* These are non-NULL only for an index relation: */
	Form_pg_index rd_index;		/* pg_index tuple describing this index */

	/* use "struct" here to avoid needing to include htup.h: */
	struct HeapTupleData *rd_indextuple;		/* all of pg_index tuple */
	Form_pg_am	rd_am;			/* pg_am tuple for index's AM */

	/*
	* index access support info (used only for an index relation)
	*
	* Note: only default support procs for each opclass are cached, namely
	* those with lefttype and righttype equal to the opclass's opcintype. The
	* arrays are indexed by support function number, which is a sufficient
	* identifier given that restriction.
	*
	* Note: rd_amcache is available for index AMs to cache private data about
	* an index.  This must be just a cache since it may get reset at any time
	* (in particular, it will get reset by a relcache inval message for the
	* index).	If used, it must point to a single memory chunk palloc'd in
	* rd_indexcxt.  A relcache reset will include freeing that chunk and
	* setting rd_amcache = NULL.
	*/
	MemoryContext rd_indexcxt;	/* private memory cxt for this stuff */
	RelationAmInfo *rd_aminfo;	/* lookup info for funcs found in pg_am */
#ifndef FOUNDER_XDB_SE
	Oid		   *rd_opfamily;	/* OIDs of op families for each index col */
	Oid		   *rd_opcintype;	/* OIDs of opclass declared input data types */
	RegProcedure *rd_support;	/* OIDs of support procedures */
	FmgrInfo   *rd_supportinfo; /* lookup info for support procedures */
	int16	   *rd_indoption;	/* per-column AM-specific flags */
	List	   *rd_indexprs;	/* index expression trees, if any */
	List	   *rd_indpred;		/* index predicate tree, if any */
	Oid		   *rd_exclops;		/* OIDs of exclusion operators, if any */
	Oid		   *rd_exclprocs;	/* OIDs of exclusion ops' procs, if any */
	uint16	   *rd_exclstrats;	/* exclusion ops' strategy numbers, if any */
#endif
	void	   *rd_amcache;		/* available for use by index AM */
#ifndef FOUNDER_XDB_SE
	Oid		   *rd_indcollation;	/* OIDs of index collations */
#endif
#ifdef FOUNDER_XDB_SE
	Colinfo rd_colinfo;
	MtInfoData mt_info; /* meta data table info */
	Oid temp_toast_id;
	void *pLsmData;
	Oid lastLsmActiveIdx;
	Datum idx_max;
	Datum idx_min;
#endif //FOUNER_XDB_SE



	/*
	* Hack for CLUSTER, rewriting ALTER TABLE, etc: when writing a new
	* version of a table, we need to make any toast pointers inserted into it
	* have the existing toast table's OID, not the OID of the transient toast
	* table.  If rd_toastoid isn't InvalidOid, it is the OID to place in
	* toast pointers inserted into this rel.  (Note it's set on the new
	* version of the main heap, not the toast table itself.)
	*/
	Oid			rd_toastoid;	/* Real TOAST table's OID, or InvalidOid */

	/* use "struct" here to avoid needing to include pgstat.h: */
	struct PgStat_TableStatus *pgstat_info;		/* statistics collection area */

	void *user_data;// allow upper level or user to specify some struct in order to be used by various user callbacks, such as the user registered split function.
	char lockMode;
} RelationData;

/*
* StdRdOptions
*		Standard contents of rd_options for heaps and generic indexes.
*
* RelationGetFillFactor() and RelationGetTargetPageFreeSpace() can only
* be applied to relations that use this format or a superset for
* private options data.
*/
/* autovacuum-related reloptions. */
typedef struct AutoVacOpts
{
	bool		enabled;
	int			vacuum_threshold;
	int			analyze_threshold;
	int			vacuum_cost_delay;
	int			vacuum_cost_limit;
	int			freeze_min_age;
	int			freeze_max_age;
	int			freeze_table_age;
	float8		vacuum_scale_factor;
	float8		analyze_scale_factor;
} AutoVacOpts;

typedef struct StdRdOptions
{
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int			fillfactor;		/* page fill factor in percent (0..100) */
	AutoVacOpts autovacuum;		/* autovacuum-related options */
} StdRdOptions;

#define HEAP_MIN_FILLFACTOR			10
#define HEAP_DEFAULT_FILLFACTOR		100
/*
* RelationGetFillFactor
*		Returns the relation's fillfactor.  Note multiple eval of argument!
*/
#define RelationGetFillFactor(relation, defaultff) \
	((relation)->rd_options ? \
	((StdRdOptions *) (relation)->rd_options)->fillfactor : (defaultff))

/*
* RelationGetTargetPageUsage
*		Returns the relation's desired space usage per page in bytes.
*/
#define RelationGetTargetPageUsage(relation, defaultff) \
	(BLCKSZ * RelationGetFillFactor(relation, defaultff) / 100)

/*
* RelationGetTargetPageFreeSpace
*		Returns the relation's desired freespace per page in bytes.
*/
#define RelationGetTargetPageFreeSpace(relation, defaultff) \
	(BLCKSZ * (100 - RelationGetFillFactor(relation, defaultff)) / 100)

/*
* RelationIsValid
*		True iff relation descriptor is valid.
*/
#define RelationIsValid(relation) PointerIsValid(relation)

#define InvalidRelation ((Relation) NULL)

/*
* RelationHasReferenceCountZero
*		True iff relation reference count is zero.
*
* Note:
*		Assumes relation descriptor is valid.
*/
#define RelationHasReferenceCountZero(relation) \
		((bool)((relation)->rd_refcnt == 0))

/*
* RelationGetForm
*		Returns pg_class tuple for a relation.
*
* Note:
*		Assumes relation descriptor is valid.
*/

#define RelationGetForm(relation) ((relation)->rd_rel)


/*
* RelationGetRelid
*		Returns the OID of the relation
*/
#define RelationGetRelid(relation) ((relation)->rd_id)

/*
* RelationGetNumberOfAttributes
*		Returns the number of attributes in a relation.
*/

#define RelationGetNumberOfAttributes(relation) ((relation)->rd_rel->relnatts)



/*
* RelationGetDescr
*		Returns tuple descriptor for a relation.
*/
#define RelationGetDescr(relation) ((relation)->rd_att)

/*
* RelationGetRelationName
*		Returns the rel's name.
*
* Note that the name is only unique within the containing namespace.
*/
#define RelationGetRelationName(relation) \
	(NameStr((relation)->rd_rel->relname))

/*
* RelationGetNamespace
*		Returns the rel's namespace OID.
*/

#define RelationGetNamespace(relation) \
	((relation)->rd_rel->relnamespace)


/*
* RelationIsMapped
*		True if the relation uses the relfilenode map.
*
* NB: this is only meaningful for relkinds that have storage, else it
* will misleadingly say "true".
*/

#define RelationIsMapped(relation) \
	((relation)->rd_rel->relfilenode == InvalidOid)


/*
* RelationOpenSmgr
*		Open the relation at the smgr level, if not already done.
*/
#define RelationOpenSmgr(relation) \
	do { \
	if ((relation)->rd_smgr == NULL) \
	smgrsetowner(&((relation)->rd_smgr), smgropen((relation)->rd_node, (relation)->rd_backend)); \
	} while (0)

/*
* RelationCloseSmgr
*		Close the relation at the smgr level, if not already done.
*
* Note: smgrclose should unhook from owner pointer, hence the Assert.
*/
#define RelationCloseSmgr(relation) \
	do { \
	if ((relation)->rd_smgr != NULL) \
{ \
	smgrclose((relation)->rd_smgr); \
	Assert((relation)->rd_smgr == NULL); \
} \
	} while (0)

/*
* RelationGetTargetBlock
*		Fetch relation's current insertion target block.
*
* Returns InvalidBlockNumber if there is no current target block.	Note
* that the target block status is discarded on any smgr-level invalidation.
*/
#define RelationGetTargetBlock(relation) \
	( (relation)->rd_smgr != NULL ? (relation)->rd_smgr->smgr_targblock : InvalidBlockNumber )

/*
* RelationSetTargetBlock
*		Set relation's current insertion target block.
*/
#define RelationSetTargetBlock(relation, targblock) \
	do { \
	RelationOpenSmgr(relation); \
	(relation)->rd_smgr->smgr_targblock = (targblock); \
	} while (0)

/*
* RelationNeedsWAL
*		True if relation needs WAL.
*/


#define RelationNeedsWAL(relation) \
	((relation)->rd_rel->relpersistence == RELPERSISTENCE_PERMANENT)



/*
* RelationUsesLocalBuffers
*		True if relation's pages are stored in local buffers.
*/

#define RelationUsesLocalBuffers(relation) \
	((relation)->rd_rel->relpersistence == RELPERSISTENCE_TEMP)


/*
* RelationUsesTempNamespace
*		True if relation's catalog entries live in a private namespace.
*/

#define RelationUsesTempNamespace(relation) \
	((relation)->rd_rel->relpersistence == RELPERSISTENCE_TEMP)

#ifdef FOUNDER_XDB_SE

#define RelationIsTemp(relation) \
	((relation)->rd_rel->relpersistence == RELPERSISTENCE_TEMP)

#endif //FOUNDER_XDB_SE

/*
* RELATION_IS_LOCAL
*		If a rel is either temp or newly created in the current transaction,
*		it can be assumed to be visible only to the current backend.
*
* Beware of multiple eval of argument
*/
#ifndef FOUNDER_XDB_SE
#define RELATION_IS_LOCAL(relation) \
	((relation)->rd_backend == MyBackendId || \
	(relation)->rd_createSubid != InvalidSubTransactionId)
#else
#define RELATION_IS_LOCAL(relation) \
	((relation)->rd_backend == (int)CurrentTopLevelState->xid || \
	(relation)->rd_createSubid != InvalidSubTransactionId)
#endif //FOUNDER_XDB_SE

/*
* RELATION_IS_OTHER_TEMP
*		Test for a temporary relation that belongs to some other session.
*
* Beware of multiple eval of argument
*/
#ifndef FOUNDER_XDB_SE
#define RELATION_IS_OTHER_TEMP(relation) \
	((relation)->rd_rel->relpersistence == RELPERSISTENCE_TEMP \
	&& (relation)->rd_backend != MyBackendId)
#else
#define RELATION_IS_OTHER_TEMP(relation) \
	((relation)->rd_rel->relpersistence == (char)RELPERSISTENCE_TEMP \
	&& (relation)->rd_backend != (BackendId)CurrentTopLevelState->xid)
#endif //FOUNDER_XDB_SE




/* routines in utils/cache/relcache.c */
extern void RelationIncrementReferenceCount(Relation rel);
extern void RelationDecrementReferenceCount(Relation rel);

#endif   /* REL_H */
