#ifndef XDB_COMMON_HPP
#define XDB_COMMON_HPP

#include "access/htup.h"
#include "access/tupdesc.h"
#include "storage/buf.h"

///excutor/tuptable.h
#ifndef FOUNDER_XDB_SE
typedef struct TupleTableSlot
{
	NodeTag		type;
	bool		tts_isempty;	/* true = slot is empty */
	bool		tts_shouldFree; /* should pfree tts_tuple? */
	bool		tts_shouldFreeMin;		/* should pfree tts_mintuple? */
	bool		tts_slow;		/* saved state for slot_deform_tuple */
	HeapTuple	tts_tuple;		/* physical tuple, or NULL if virtual */
	TupleDesc	tts_tupleDescriptor;	/* slot's tuple descriptor */
	MemoryContext tts_mcxt;		/* slot itself is in this context */
	Buffer		tts_buffer;		/* tuple's buffer, or InvalidBuffer */
	int			tts_nvalid;		/* # of valid values in tts_values */
	Datum	   *tts_values;		/* current per-attribute values */
	bool	   *tts_isnull;		/* current per-attribute isnull flags */
	MinimalTuple tts_mintuple;	/* minimal tuple, or NULL if none */
	HeapTupleData tts_minhdr;	/* workspace for minimal-tuple-only case */
	long		tts_off;		/* saved state for slot_deform_tuple */
} TupleTableSlot;

/* in access/common/heaptuple.c */
extern Datum slot_getattr(TupleTableSlot *slot, int attnum, bool *isnull);
extern void slot_getallattrs(TupleTableSlot *slot);
extern void slot_getsomeattrs(TupleTableSlot *slot, int attnum);
extern bool slot_attisnull(TupleTableSlot *slot, int attnum);
#endif
//utils/pg_class.h
#define Anum_pg_class_reloptions		26
#define		  RELKIND_RELATION		  'r'		/* ordinary table */
#define		  RELKIND_TOASTVALUE	  't'		/* for out-of-line values */
//utils/pg_collation.h
#define DEFAULT_COLLATION_OID	100
extern void fdxb_CreateToastAttTupleDesc();
extern void fdxdb_CreateSingeAttTupleDesc();
extern void fdxdb_CreateClusterIndexAttTupleDesc();
//extern HeapTuple fdxdb_heap_formtuple(const char *p, size_t len, uint32 nEids = 0, Oid *pEids = NULL);
extern char* fxdb_tuple_to_chars(HeapTuple tuple);
extern char* fdxdb_tuple_to_chars_with_len(HeapTuple tuple, int &len);
extern Datum fdxdb_string_formdatum(const char *p, size_t len);
extern Datum fdxdb_uint32_formdatum(const uint32& value);
extern Datum fdxdb_string_fortdatum_bulk(char *buf, const char *p, size_t len, int &cur);
extern void fxdb_tuple_print(HeapTuple tuple);


#endif




