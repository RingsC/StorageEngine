#ifndef XDB_INDEX_H
#define XDB_INDEX_H
//pg_index
/*
* Index AMs that support ordered scans must support these two indoption
* bits.  Otherwise, the content of the per-column indoption fields is
* open for future definition.
*/
#define INDOPTION_DESC			0x0001	/* values are in reverse order */
#define INDOPTION_NULLS_FIRST	0x0002	/* NULLs are first instead of last */
#endif   /* XDB_INDEX_H */