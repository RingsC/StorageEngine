/*-------------------------------------------------------------------------
 *
 * dest.h
 *	  support for communication destinations
 *
 * Whenever the backend executes a query that returns tuples, the results
 * have to go someplace.  For example:
 *
 *	  - stdout is the destination only when we are running a
 *		standalone backend (no postmaster) and are returning results
 *		back to an interactive user.
 *
 *	  - a remote process is the destination when we are
 *		running a backend with a frontend and the frontend executes
 *		PQexec() or PQfn().  In this case, the results are sent
 *		to the frontend via the functions in backend/libpq.
 *
 *	  - DestNone is the destination when the system executes
 *		a query internally.  The results are discarded.
 *
 * dest.c defines three functions that implement destination management:
 *
 * BeginCommand: initialize the destination at start of command.
 * CreateDestReceiver: return a pointer to a struct of destination-specific
 * receiver functions.
 * EndCommand: clean up the destination at end of command.
 *
 * BeginCommand/EndCommand are executed once per received SQL query.
 *
 * CreateDestReceiver returns a receiver object appropriate to the specified
 * destination.  The executor, as well as utility statements that can return
 * tuples, are passed the resulting DestReceiver* pointer.	Each executor run
 * or utility execution calls the receiver's rStartup method, then the
 * receiveSlot method (zero or more times), then the rShutdown method.
 * The same receiver object may be re-used multiple times; eventually it is
 * destroyed by calling its rDestroy method.
 *
 * In some cases, receiver objects require additional parameters that must
 * be passed to them after calling CreateDestReceiver.	Since the set of
 * parameters varies for different receiver types, this is not handled by
 * this module, but by direct calls from the calling code to receiver type
 * specific functions.
 *
 * The DestReceiver object returned by CreateDestReceiver may be a statically
 * allocated object (for destination types that require no local state),
 * in which case rDestroy is a no-op.  Alternatively it can be a palloc'd
 * object that has DestReceiver as its first field and contains additional
 * fields (see printtup.c for an example).	These additional fields are then
 * accessible to the DestReceiver functions by casting the DestReceiver*
 * pointer passed to them.	The palloc'd object is pfree'd by the rDestroy
 * method.	Note that the caller of CreateDestReceiver should take care to
 * do so in a memory context that is long-lived enough for the receiver
 * object not to disappear while still needed.
 *
 * Special provision: None_Receiver is a permanently available receiver
 * object for the DestNone destination.  This avoids useless creation/destroy
 * calls in portal and cursor manipulations.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/tcop/dest.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef DEST_H
#define DEST_H

//#include "executor/tuptable.h"
#include "access/htup.h"
#include "storage/buf.h"


/* buffer size to use for command completion tags */
#define COMPLETION_TAG_BUFSIZE	64

/*----------
* The executor stores tuples in a "tuple table" which is a List of
* independent TupleTableSlots.  There are several cases we need to handle:
*		1. physical tuple in a disk buffer page
*		2. physical tuple constructed in palloc'ed memory
*		3. "minimal" physical tuple constructed in palloc'ed memory
*		4. "virtual" tuple consisting of Datum/isnull arrays
*
* The first two cases are similar in that they both deal with "materialized"
* tuples, but resource management is different.  For a tuple in a disk page
* we need to hold a pin on the buffer until the TupleTableSlot's reference
* to the tuple is dropped; while for a palloc'd tuple we usually want the
* tuple pfree'd when the TupleTableSlot's reference is dropped.
*
* A "minimal" tuple is handled similarly to a palloc'd regular tuple.
* At present, minimal tuples never are stored in buffers, so there is no
* parallel to case 1.	Note that a minimal tuple has no "system columns".
* (Actually, it could have an OID, but we have no need to access the OID.)
*
* A "virtual" tuple is an optimization used to minimize physical data
* copying in a nest of plan nodes.  Any pass-by-reference Datums in the
* tuple point to storage that is not directly associated with the
* TupleTableSlot; generally they will point to part of a tuple stored in
* a lower plan node's output TupleTableSlot, or to a function result
* constructed in a plan node's per-tuple econtext.  It is the responsibility
* of the generating plan node to be sure these resources are not released
* for as long as the virtual tuple needs to be valid.	We only use virtual
* tuples in the result slots of plan nodes --- tuples to be copied anywhere
* else need to be "materialized" into physical tuples.  Note also that a
* virtual tuple does not have any "system columns".
*
* It is also possible for a TupleTableSlot to hold both physical and minimal
* copies of a tuple.  This is done when the slot is requested to provide
* the format other than the one it currently holds.  (Originally we attempted
* to handle such requests by replacing one format with the other, but that
* had the fatal defect of invalidating any pass-by-reference Datums pointing
* into the existing slot contents.)  Both copies must contain identical data
* payloads when this is the case.
*
* The Datum/isnull arrays of a TupleTableSlot serve double duty.  When the
* slot contains a virtual tuple, they are the authoritative data.	When the
* slot contains a physical tuple, the arrays contain data extracted from
* the tuple.  (In this state, any pass-by-reference Datums point into
* the physical tuple.)  The extracted information is built "lazily",
* ie, only as needed.	This serves to avoid repeated extraction of data
* from the physical tuple.
*
* A TupleTableSlot can also be "empty", holding no valid data.  This is
* the only valid state for a freshly-created slot that has not yet had a
* tuple descriptor assigned to it.  In this state, tts_isempty must be
* TRUE, tts_shouldFree FALSE, tts_tuple NULL, tts_buffer InvalidBuffer,
* and tts_nvalid zero.
*
* The tupleDescriptor is simply referenced, not copied, by the TupleTableSlot
* code.  The caller of ExecSetSlotDescriptor() is responsible for providing
* a descriptor that will live as long as the slot does.  (Typically, both
* slots and descriptors are in per-query memory and are freed by memory
* context deallocation at query end; so it's not worth providing any extra
* mechanism to do more.  However, the slot will increment the tupdesc
* reference count if a reference-counted tupdesc is supplied.)
*
* When tts_shouldFree is true, the physical tuple is "owned" by the slot
* and should be freed when the slot's reference to the tuple is dropped.
*
* If tts_buffer is not InvalidBuffer, then the slot is holding a pin
* on the indicated buffer page; drop the pin when we release the
* slot's reference to that buffer.  (tts_shouldFree should always be
* false in such a case, since presumably tts_tuple is pointing at the
* buffer page.)
*
* tts_nvalid indicates the number of valid columns in the tts_values/isnull
* arrays.	When the slot is holding a "virtual" tuple this must be equal
* to the descriptor's natts.  When the slot is holding a physical tuple
* this is equal to the number of columns we have extracted (we always
* extract columns from left to right, so there are no holes).
*
* tts_values/tts_isnull are allocated when a descriptor is assigned to the
* slot; they are of length equal to the descriptor's natts.
*
* tts_mintuple must always be NULL if the slot does not hold a "minimal"
* tuple.  When it does, tts_mintuple points to the actual MinimalTupleData
* object (the thing to be pfree'd if tts_shouldFreeMin is true).  If the slot
* has only a minimal and not also a regular physical tuple, then tts_tuple
* points at tts_minhdr and the fields of that struct are set correctly
* for access to the minimal tuple; in particular, tts_minhdr.t_data points
* MINIMAL_TUPLE_OFFSET bytes before tts_mintuple.	This allows column
* extraction to treat the case identically to regular physical tuples.
*
* tts_slow/tts_off are saved state for slot_deform_tuple, and should not
* be touched by any other code.
*----------
*/
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
/* ----------------
 *		CommandDest is a simplistic means of identifying the desired
 *		destination.  Someday this will probably need to be improved.
 *
 * Note: only the values DestNone, DestDebug, DestRemote are legal for the
 * global variable whereToSendOutput.	The other values may be used
 * as the destination for individual commands.
 * ----------------
 */
typedef enum
{
	DestNone,					/* results are discarded */
	DestDebug,					/* results go to debugging output */
	DestRemote,					/* results sent to frontend process */
	DestRemoteExecute,			/* sent to frontend, in Execute command */
	DestSPI,					/* results sent to SPI manager */
	DestTuplestore,				/* results sent to Tuplestore */
	DestIntoRel,				/* results sent to relation (SELECT INTO) */
	DestCopyOut,				/* results sent to COPY TO code */
	DestSQLFunction				/* results sent to SQL-language func mgr */
} CommandDest;

/* ----------------
 *		DestReceiver is a base type for destination-specific local state.
 *		In the simplest cases, there is no state info, just the function
 *		pointers that the executor must call.
 *
 * Note: the receiveSlot routine must be passed a slot containing a TupleDesc
 * identical to the one given to the rStartup routine.
 * ----------------
 */
typedef struct _DestReceiver DestReceiver;

struct _DestReceiver
{
	/* Called for each tuple to be output: */
	void		(*receiveSlot) (TupleTableSlot *slot,
											DestReceiver *self);
	/* Per-executor-run initialization and shutdown: */
	void		(*rStartup) (DestReceiver *self,
										 int operation,
										 TupleDesc typeinfo);
	void		(*rShutdown) (DestReceiver *self);
	/* Destroy the receiver object itself (if dynamically allocated) */
	void		(*rDestroy) (DestReceiver *self);
	/* CommandDest code for this receiver */
	CommandDest mydest;
	/* Private fields might appear beyond this point... */
};

extern DestReceiver *None_Receiver;		/* permanent receiver for DestNone */

/* The primary destination management functions */

extern void BeginCommand(const char *commandTag, CommandDest dest);
extern DestReceiver *CreateDestReceiver(CommandDest dest);
extern void EndCommand(const char *commandTag, CommandDest dest);

/* Additional functions that go with destination management, more or less. */

extern void NullCommand(CommandDest dest);
extern void ReadyForQuery(CommandDest dest);

#endif   /* DEST_H */
