/*-------------------------------------------------------------------------
 *
 * tablespace.h
 *		Tablespace management commands (create/drop tablespace).
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/tablespace.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TABLESPACE_H
#define TABLESPACE_H

#include "access/xlog.h"
#include "nodes/parsenodes.h"
#include "storage/itemptr.h"
/* XLOG stuff */
#define XLOG_TBLSPC_CREATE		0x00
#define XLOG_TBLSPC_DROP		0x10
#define DEFAULT_TABLESPACE_NAME "defaulttablespace"
typedef struct xl_tblspc_create_rec
{
	Oid			ts_id;
	char		ts_path[1];		/* VARIABLE LENGTH STRING */
} xl_tblspc_create_rec;

typedef struct xl_tblspc_drop_rec
{
	Oid			ts_id;
} xl_tblspc_drop_rec;

typedef struct TableSpaceOpts
{
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	float8		random_page_cost;
	float8		seq_page_cost;
} TableSpaceOpts;

#ifdef FOUNDER_XDB_SE

#pragma pack(4)
struct Form_tablespace_data
{
	NameData        spcname;        /* tablespace name */
	Oid             id;
	bool            tempOk;
	text            spclocation;    /* physical location (VAR LENGTH) */
};
#pragma pack()

typedef Form_tablespace_data* FormTablespaceData;

extern Oid  CreateTableSpace(CreateTableSpaceStmt *stmt,bool isTempOk);
#else
extern void CreateTableSpace(CreateTableSpaceStmt *stmt);
#endif
extern void DropTableSpace(DropTableSpaceStmt *stmt);
#ifndef FOUNDER_XDB_SE
extern void RenameTableSpace(const char *oldname, const char *newname);
extern void AlterTableSpaceOwner(const char *name, Oid newOwnerId);
//extern void AlterTableSpaceOptions(AlterTableSpaceOptionsStmt *stmt);
#endif
extern void TablespaceCreateDbspace(Oid spcNode, Oid dbNode, bool isRedo);

extern Oid	GetDefaultTablespace(char relpersistence);

extern void PrepareTempTablespaces(void);

extern Oid	get_tablespace_oid(const char *tablespacename, bool missing_ok,ItemPointer it = NULL);
extern char *get_tablespace_name(Oid spc_oid);
extern char *get_tablespace_location(const char* tablespacename);
extern char *get_tablespace_location(Oid tablespacename);
extern bool tablespace_is_empty(Oid tablespaceId);
extern bool directory_is_empty(const char *path);

extern void tblspc_redo(XLogRecPtr lsn, XLogRecord *rptr);
extern void tblspc_desc(StringInfo buf, uint8 xl_info, char *rec);
extern void create_tablespace_directories(char *location,
										  const Oid tablespaceoid);
#ifdef FOUNDER_XDB_SE
extern bool destroy_tablespace_directories(Oid tablespaceoid, bool redo);
#endif //FOUNDER_XDB_SE

void InitTablespaceColiId( void );

int GetTablespaceMetaColiId( void );
int GetTablespaceNameIdxColiId( void );
int GetTablespaceOidIdxColiId( void );

#ifdef FOUNDER_XDB_SE
List *ListTableSpace();
List *ListTableSpaceDefault();
void InitTableSpaceLocationList(char *str_list);
void GetMaxTablespaceId( void );
#endif //FOUNDER_XDB_SE
#endif   /* TABLESPACE_H */
