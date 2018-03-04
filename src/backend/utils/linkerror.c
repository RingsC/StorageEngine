#include "postgres.h"
#include "c.h"
#include "fmgr.h"
#include "funcapi.h"
#include "access/hash.h"
#include "access/htup.h"
#include "access/xlog.h"
#include "access/xlogdefs.h"
//#include "catalog/objectaccess.h"
#include "commands/tablespace.h"
#include "commands/vacuum.h"
#include "commands/xdb_command.h"
#include "lib/stringinfo.h"
//#include "libpq/pqformat.h"
#include "miscadmin.h"
#include "utils/array.h"
#ifndef FOUNDER_XDB_SE
#include "utils/catcache.h"
#endif
#include "utils/datetime.h"
#include "utils/guc.h"
#ifndef FOUNDER_XDB_SE
#include "utils/plancache.h"
#endif
#include "utils/syscache.h"
#include "utils/typcache.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"

/*
*include\utils\lsyscache.h get_opfamily_proc
*/
Oid get_opfamily_proc(Oid opfamily, Oid lefttype, Oid righttype,
											int16 procnum)
{
    Assert(false);
	return 0;
}

/*
*include\utils\lsyscache.hļµget_opcode
*/
RegProcedure get_opcode(Oid opno)
{
    Assert(false);
	return 0;
}

/*
*include\utils\lsyscache.hļµget_opfamily_member
*/
Oid get_opfamily_member(Oid opfamily, Oid lefttype, Oid righttype,
												int16 strategy)
{
    Assert(false);
	return 0;
}

/*
*include\utils\lsyscache.hļµget_array_type
*/
Oid	get_array_type(Oid typid)
{
    Assert(false);
	return 0;
}

/*
*superuser
*/
bool superuser()
{
	return true; //TODO
}

/*
*include\utils\guc.h SetConfigOption
*/
void SetConfigOption(const char *name, const char *value,
										 GucContext context, GucSource source)
{
    Assert(false);
}

/*
*include\utils\guc.hļµGetConfigOptionResetString
*/
const char *GetConfigOptionResetString(const char *name)
{
    Assert(false);
	return "\0";
}

/*
*include\utils\guc.h write_nondefault_variables
*/
void write_nondefault_variables(GucContext context)
{
    Assert(false);
}

/*
*include\utils\guc.h pg_timezone_abbrev_initialize
*/
void pg_timezone_abbrev_initialize()
{
    Assert(false);
}

/*
*include\utils\datetime.h CheckDateTokenTables
*/
bool CheckDateTokenTables()
{
    Assert(false);
	return false;
}

/*
*include\utils\guc.h SelectConfigFiles
*/
bool SelectConfigFiles(const char *userDoption, const char *progname)
{
    Assert(false);
	return false;
}

/*
*include\utils\guc.h ParseLongOption
*/
void ParseLongOption(const char *string, char **name, char **value)
{
    Assert(false);
}

/*
*include\utils\guc.h InitializeGUCOptions
*/

/*
*include\utils\builtins.h xidComparator
*/
int	xidComparator(const void *arg1, const void *arg2)
{
    Assert(false);
	return 0;
}

/*
*include\utils\syscache.h SearchSysCacheCopyHeapTupleDataָ
*/
HeapTuple SearchSysCacheCopy(int cacheId,
														 Datum key1, Datum key2, Datum key3, Datum key4)
{
    Assert(false);
	return NULL;
}

/*
*include\utils\relmapper.h RelationMapOidToFilenode,޷0
*/
Oid	RelationMapOidToFilenode(Oid relationId, bool shared)
{
    Assert(false);
	return 0;
}

/*
*include\utils\relmapper.h RelationMapInvalidateAll
*/
void RelationMapInvalidateAll()
{
    Assert(false);
}

/*
*include\utils\relmapper.h RelationMapUpdateMap
*/
void RelationMapUpdateMap(Oid relationId, Oid fileNode, bool shared,
													bool immediate)
{
    Assert(false);
}


/*
*include\utils\relmapper.h RelationMapInitialize
*/
void RelationMapInitialize()
{
    Assert(false);
}

/*
*include\utils\relmapper.h RelationMapInitializePhase2
*/
void RelationMapInitializePhase2()
{
    Assert(false);
}

/*
*include\utils\syscache.h InitCatalogCachePhase2
*/
void InitCatalogCachePhase2()
{
    Assert(false);
}

/*
*include\utils\relmapper.h RelationMapInitializePhase3
*/
void RelationMapInitializePhase3()
{
    Assert(false);
}

/*
*include\nodesw\relmapper.h copyObject
*/
void *copyObject(void *obj)
{
    Assert(false);
	return NULL;
}

/*
*include\utils\guc.h parse_real
*/
bool parse_real(const char *value, double *result)
{
    Assert(false);
	return false;
}

/*
*include\utils\guc.h parse_int
*/
bool parse_int(const char *value, int *result, int flags,
							 const char **hintmsg)
{
    Assert(false);
	return false;
}

#ifndef FOUNDER_XDB_SE
/*
*include\access\hash.h ReleaseResources_hash
*/
void ReleaseResources_hash()
{
    Assert(false);
}
#endif

/*
*include\utils\catcache.hļµReleaseCatCache
*/
void ReleaseCatCache(HeapTuple tuple)
{
    Assert(false);
}

/*
*include\utils\catcache.h PrintCatCacheLeakWarning
*/
void PrintCatCacheLeakWarning(HeapTuple tuple)
{
    Assert(false);
}

/*
*include\access\gist_private.h gist_xlog_cleanup
*/
void gist_xlog_cleanup()
{
    Assert(false);
}

/*
*include\access\gist_private.h gist_xlog_startup
*/
void gist_xlog_startup()
{
    Assert(false);
}

/*
*include\access\gist_private.h gist_desc
*/
void gist_desc(StringInfo buf, uint8 xl_info, char *rec)
{
    Assert(false);
}

/*
*include\access\gist_private.hļµgist_redo
*/
void gist_redo(XLogRecPtr lsn, XLogRecord *record)
{
    Assert(false);
}

/*
*include\access\gin.h gin_safe_restartpoint
*/
bool gin_safe_restartpoint()
{
    Assert(false);	
	return false;
}

/*
*include\access\gin.h gin_xlog_cleanup
*/
void gin_xlog_cleanup()
{
    Assert(false);
}

/*
*include\access\gin.hļµgin_xlog_startup
*/
void gin_xlog_startup()
{
    Assert(false);
}

/*
*include\access\gin.h gin_desc
*/
void gin_desc(StringInfo buf, uint8 xl_info, char *rec)
{
    Assert(false);
}

/*
*include\access\gin.h gin_redo
*/
void gin_redo(XLogRecPtr lsn, XLogRecord *record)
{
    Assert(false);
}

/*
*include\utils\relmapper.h relmap_desc
*/
void relmap_desc(StringInfo buf, uint8 xl_info, char *rec)
{
    Assert(false);
}

/*
*include\utils\relmapper.h relmap_redo
*/
void relmap_redo(XLogRecPtr lsn, XLogRecord *record)
{
    Assert(false);
}

/*
*backend\utils\error\elog.c application_name 
*backend\postmaster\pgstat.c application_name 
*include\utils\guc.h externapplication_name 
*/
//THREAD_LOCAL char *application_name = "\0";

/*
*backend\postmaster\postmaster.c external_pid_file 
*include\utils\guc.h externexternal_pid_file
*/
THREAD_LOCAL char *external_pid_file = (char*)"\0";

/*
*port\path.c progname
*backend\tcop\postgres.c progname
*backend\postmaster\postmaster.c  progname
*include\postmaster\postmaster.hexternprogname
*/
const char *progname = "\0";


//datetime.h
int	DetermineTimeZoneOffset(struct pg_tm * tm, pg_tz *tzp)
{
    Assert(false);
    return 0;
}

int DecodeDateTime(char **field, int *ftype,
			   int nf, int *dtype,
			   struct pg_tm * tm, fsec_t *fsec, int *tzp)
{
    Assert(false);
    return 0;
}

int ParseDateTime(const char *timestr, char *workbuf, size_t buflen,
			  char **field, int *ftype,
			  int maxfields, int *numfields)
{
    Assert(false);
    return 0;
}			  

void EncodeDateTime(struct pg_tm * tm, fsec_t fsec, int *tzp, char **tzn, int style, char *str)
{
    Assert(false);
}
int DecodeISO8601Interval(char *str,
					  int *dtype, struct pg_tm * tm, fsec_t *fsec)
{
    Assert(false);
    return 0;
}
int DecodeInterval(char **field, int *ftype, int nf, int range,
			   int *dtype, struct pg_tm * tm, fsec_t *fsec)
{
    Assert(false);
    return  0;
}
int	date2j(int year, int month, int day)
{
    Assert(false);
    return 0;
}
void DateTimeParseError(int dterr, const char *str,
				   const char *datatype)
{
    Assert(false);
}				   
void EncodeInterval(struct pg_tm * tm, fsec_t fsec, int style, char *str)
{
    Assert(false);
}

int	DecodeUnits(int field, char *lowtoken, int *val)
{
    Assert(false);
    return 0;
}
int	DecodeSpecial(int field, char *lowtoken, int *val)
{
    Assert(false);
    return 0;
}
int	j2day(int jd)
{
    Assert(false);
    return 0;
}						
		   				  

//utils/array.h
int32 *ArrayGetIntegerTypmods(ArrayType *arr, int *n)
{
    Assert(false);
    return NULL;
}
ArrayType *construct_array(Datum *elems, int nelems,
				Oid elmtype,
				int elmlen, bool elmbyval, char elmalign)
{
    Assert(false);
    return NULL;
}
void deconstruct_array(ArrayType *array,
				  Oid elmtype,
				  int elmlen, bool elmbyval, char elmalign,
				  Datum **elemsp, bool **nullsp, int *nelemsp)
{
    Assert(false);
}				  			




//utils/builtins.h
void text_to_cstring_buffer(const text *src, char *dst, size_t dst_len)
{
    Assert(false);  
}	
int32 type_maximum_size(Oid type_oid, int32 typemod)
{
    Assert(false);
    return 0;
}
char *format_type_with_typemod(Oid type_oid, int32 typemod)
{
    Assert(false);
    return NULL;        
}
char *format_type_be(Oid type_oid)
{
    Assert(false);
    return NULL;  
}

//utils/syscache.h
void ReleaseSysCache(HeapTuple tuple)
{
    Assert(false);
}
HeapTuple SearchSysCache(int cacheId,
			   Datum key1, Datum key2, Datum key3, Datum key4)
{
   Assert(false);
   HeapTuple tm = {0};
   return tm; 
}
bool SearchSysCacheExists(int cacheId,
					 Datum key1, Datum key2, Datum key3, Datum key4)
{
    Assert(false);
    return false;
}					 	
//misadmin.h

bool superuser_arg(Oid roleid)
{
    Assert(false);
    return false;
}
void GetUserIdAndSecContext(Oid *userid, int *sec_context)
{
    Assert(false);
}
void SetUserIdAndSecContext(Oid userid, int sec_context)
{
    Assert(false);
}	
//utils/lsyscache.h
void get_typlenbyval(Oid typid, int16 *typlen, bool *typbyval)
{
    Assert(false);
} 
bool get_compare_function_for_ordering_op(Oid opno,
									 Oid *cmpfunc, bool *reverse)
{
    Assert(false);
    return false;
}
//commands/tablespace.h
void PrepareTempTablespaces(void)
{
    Assert(false);
}	

//utils/typcache.h
TupleDesc lookup_rowtype_tupdesc_noerror(Oid type_id, int32 typmod,
							   bool noError)
{
    Assert(false);
    TupleDesc tm = {0};
    return tm;
}
//funcapi.h
void end_MultiFuncCall(PG_FUNCTION_ARGS, FuncCallContext *funcctx)
{
    Assert(false);
}
FuncCallContext *per_MultiFuncCall(PG_FUNCTION_ARGS)
{
    Assert(false);
    return NULL;
}
TupleDesc BlessTupleDesc(TupleDesc tupdesc)
{
    Assert(false);
    TupleDesc tm = {0};
    return tm;
}
FuncCallContext *init_MultiFuncCall(PG_FUNCTION_ARGS)
{
    Assert(false);
    return NULL;
}	
//utils/guc.h
void AtEOXact_GUC(bool isCommit, int nestLevel)
{
    Assert(false);
}
int	NewGUCNestLevel(void)
{
    Assert(false);
    return 0;
}

THREAD_LOCAL int client_min_messages = 0;

void FreeConfigVariables(ConfigVariable *list)
{
    Assert(false);
}

void analyze_rel(Oid relid, VacuumStmt *vacstmt,
			BufferAccessStrategy bstrategy)
{
    Assert(false);
}

//miscadmin.h
void PreventCommandDuringRecovery(char const *)
{
    Assert(false);
}
//commands/xdb_command.h
#if 0 /* this function in database.c */
char *get_database_name(unsigned int)
{
    Assert(false);
    return NULL;
}
#endif
//utils/guc.h

bool ParseConfigFp(FILE *fp, const char *config_file,
			  int depth, int elevel,
			  ConfigVariable **head_p, ConfigVariable **tail_p)
{
    Assert(false);
    return false;
}
//lsyscache.h
char *get_namespace_name(unsigned int)
{
    Assert(false);
     return NULL;
}


//miscadmin.h
void InitPostgres(char const *,unsigned int,char const *,char *)
{
    Assert(false);
}
//miscadmin.h
void BaseInit(void)
{
    Assert(false);
}
//lsyscache.h
unsigned int get_rel_namespace(unsigned int)
{
    Assert(false);
    return 0;
}
//lsyscache.h
char *get_rel_name(unsigned int)
{
    Assert(false);
    return NULL;
}
////pgstat.h
//void pgstat_send_bgwriter(void)
//{
//    Assert(false);
//}

bool IsToastNamespace(unsigned int)
{
    Assert(false);
    return true;
}

bool IsSystemNamespace(unsigned int)
{
    Assert(false);
    return true;
}

THREAD_LOCAL int trace_recovery_messages = 0 ;



int log_NULL_files = 0;

THREAD_LOCAL bool IgnoreSystemIndexes = false;

THREAD_LOCAL int log_temp_files = 0;
//fmgr.h
char *OidOutputFunctionCall(unsigned int,unsigned int)
{
    Assert(false);
    return NULL;
}
						
//lsyscache.h
void getTypeOutputInfo(unsigned int,unsigned int *,bool *)
{
    Assert(false);
}
//builtins.h
char *pg_get_indexdef_columns(unsigned int,bool)
{
    Assert(false);
    return NULL;
}

//relcache.h
struct Bitmapset *RelationGetIndexAttrBitmap(struct RelationData *)
{
    Assert(false);
    return NULL;
}
//lsyscache.h
unsigned int get_relname_relid(char const *,unsigned int)
{
    Assert(false);
    return 0;
}

//buildins.h
int2vector *buildint2vector(short const *,int)
{
    Assert(false);
    return NULL;
}
//relcache.h
void RelationGetExclusionInfo(struct RelationData *,unsigned int * *,unsigned int * *,unsigned short * *)
{
    Assert(false);
}

//pglargeobject.h
unsigned int LargeObjectCreate(unsigned int)
{
    Assert(false);
    return 0;
}
//pglargeobject.h
bool LargeObjectExists(unsigned int)
{
    Assert(false);
    return true;
}
//relmapper.h
void RelationMapInvalidate(bool)
{
    Assert(false);
}
void CheckPointRelationMap(void)
{
    Assert(false);
}
//catcache.h
void CatalogCacheFlushCatalog(unsigned int)
{
    Assert(false);
}
//catcache.h
void CatalogCacheIdInvalidate(int cacheId, uint32 hashValue,
						 ItemPointer pointer)
{
    Assert(false);
}
//catcache.h
void ResetCatalogCaches(void)
{
    Assert(false);
}

//catcache.h
void PrepareToInvalidateCacheTuple(Relation relation,
							  HeapTuple tuple,
						   void (*function) (int, uint32, ItemPointer, Oid))
{
    Assert(false);
}

//void PrepareToInvalidateCacheTuple(struct RelationData *,struct HeapTupleData *,void (__cdecl*)(int,unsigned int,struct INULLointerData *,unsigned int))
//{
//
//}

//lsyscache.h
bool type_is_rowtype(unsigned int)
{
    Assert(false);
    return true;
}
//lsyscache.h
unsigned int get_rel_type_id(unsigned int)
{
    Assert(false);
    return 0;
}

//miscadmin.h
char *GetUserNameFromId(unsigned int)
{
    Assert(false);
    return NULL;
}
bool is_authenticated_user_replication_role(void)
{
    Assert(false);
    return false;
}

//PGDLLIMPORT object_access_hook_type object_access_hook = NULL;

void AtCCI_RelationMap(void)
{
    Assert(false);
}

void AtEOXact_RelationMap(bool isCommit)
{
    Assert(false);    
}
void AtPrepare_RelationMap(void)
{
    Assert(false);
}
const int day_tab[2][13] = {{0},{0}};

//this function should be restored with original largeobj code.
void AtEOXact_LargeObject(bool)
{
    Assert(false);
}
