/*-------------------------------------------------------------------------
 *
 * catalog.h
 *	  prototypes for functions in backend/catalog/catalog.c
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/catalog.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef CATALOG_H
#define CATALOG_H
#ifndef FOUNDER_XDB_SE
#include "catalog/catversion.h"
#endif //FOUNDER_XDB_SE
#include "catalog/pg_class.h"
#include "storage/relfilenode.h"
#include "utils/relcache.h"

#define OIDCHARS		10		/* max chars printed by %u */
#define TABLESPACE_VERSION_DIRECTORY	"FXDB_" PG_MAJORVERSION "_" \
									CppAsString2(CATALOG_VERSION_NO)

extern const char *forkNames[];
extern ForkNumber forkname_to_number(char *forkName);
extern int	forkname_chars(const char *str, ForkNumber *);

extern char *relpathbackend(RelFileNode rnode, BackendId backend,
			   ForkNumber forknum);
extern char *GetDatabasePath(Oid dbNode, Oid spcNode);

/* First argument is a RelFileNodeBackend */
#define relpath(rnode, forknum) \
		relpathbackend((rnode).node, (rnode).backend, (forknum))

/* First argument is a RelFileNode */
#define relpathperm(rnode, forknum) \
		relpathbackend((rnode), InvalidBackendId, (forknum))

extern bool IsSystemRelation(Relation relation);
extern bool IsToastRelation(Relation relation);

#ifndef FOUNDER_XDB_SE
extern bool IsSystemClass(Form_pg_class reltuple);
extern bool IsToastClass(Form_pg_class reltuple);
#endif //FOUNDER_XDB_SE
#ifndef FOUNDER_XDB_SE
extern bool IsSystemNamespace(Oid namespaceId);
extern bool IsToastNamespace(Oid namespaceId);
#endif //FOUNDER_XDB_SE

extern bool IsReservedName(const char *name);

extern bool IsSharedRelation(Oid relationId);

extern Oid	GetNewOid(Relation relation);

#ifndef FOUNDER_XDB_SE
extern Oid GetNewOidWithIndex(Relation relation, Oid indexId,
				   AttrNumber oidcolumn);
#else
extern Oid GetNewOidWithIndex(Relation relation, Oid indexId,
				   AttrNumber oidcolumn);
#endif
#ifndef FOUNDER_XDB_SE
extern Oid GetNewRelFileNode(Oid reltablespace, Relation pg_class,
				  char relpersistence);
#else
extern Oid GetNewRelFileNode(Oid reltablespace, Oid dbid,
				  char relpersistence);

#endif

#endif   /* CATALOG_H */
