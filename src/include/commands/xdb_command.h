#ifndef XDB_COMMAND_H
#define XDB_COMMAND_H

/*
 * get_database_name - given a database OID, look up the name
 *
 * Returns a palloc'd string, or NULL if no such database.
 */
char *
get_database_name(Oid dbid);

#endif //XDB_COMMAND_H