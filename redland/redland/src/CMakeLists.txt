SET(FILES_SRC 
#memcmp.c
rdf_files.c
rdf_heuristics.c
rdf_model.c
rdf_parser_raptor.c
rdf_raptor.c
rdf_storage.c
rdf_storage_fxdb.c
#rdf_storage_postgresql.c
#rdf_storage_tstore.c
rdf_concepts.c
#rdf_hash_bdb.c
rdf_init.c
rdf_model_storage.c
rdf_query.c
rdf_serializer.c
rdf_storage_file.c
#rdf_storage_sql.c
#rdf_storage_virtuoso.c
rdf_digest.c
rdf_hash.c
rdf_iterator.c
rdf_node.c
rdf_query_rasqal.c
rdf_serializer_raptor.c
rdf_storage_hashes.c
#rdf_storage_sqlite.c
rdf_stream.c
rdf_digest_md5.c
rdf_hash_cursor.c
rdf_list.c
rdf_node_common.c
rdf_query_results.c
rdf_statement.c
rdf_storage_list.c
#rdf_storage_sql_test.c
rdf_uri.c
rdf_digest_sha1.c
rdf_hash_memory.c
rdf_log.c
rdf_parser.c 
#rdf_query_virtuoso.c
rdf_statement_common.c
#rdf_storage_mysql.c
rdf_storage_trees.c
rdf_utf8.c
)

SET_SOURCE_FILES_PROPERTIES(${FILES_SRC} PROPERTIES LANGUAGE CXX)

SET(FILES_H
 librdf.h
 rdf_concepts.h
 rdf_concepts_internal.h
 rdf_digest.h
 rdf_digest_internal.h
 rdf_files.h
 rdf_hash.h
 rdf_hash_internal.h
 rdf_heuristics.h
 rdf_init.h
 rdf_init_internal.h
 rdf_internal.h
 rdf_iterator.h
 rdf_iterator_internal.h
 rdf_list.h
 rdf_list_internal.h
 rdf_log.h
 rdf_log_internal.h
 rdf_model.h
 rdf_model_internal.h
 rdf_node.h
 rdf_node_internal.h
 rdf_parser.h
 rdf_parser_internal.h
 rdf_query.h
 rdf_query_internal.h
 rdf_raptor.h
 rdf_raptor_internal.h
 rdf_serializer.h
 rdf_serializer_internal.h
 rdf_statement.h
 rdf_statement_internal.h
 rdf_storage.h
 rdf_storage_internal.h
 rdf_storage_module.h
 rdf_storage_virtuoso_internal.h
 rdf_stream.h
 rdf_stream_internal.h
 rdf_types.h
 rdf_uri.h
 rdf_uri_internal.h
 rdf_utf8.h
 redland.h
 rdf_config.h
 win32_rdf_config.h
)

SET_SOURCE_FILES_PROPERTIES(${FILES_SRC} PROPERTIES LANGUAGE CXX)
IF(NOT MSVC)
SET_SOURCE_FILES_PROPERTIES(rdf_digest_md5.c PROPERTIES COMPILE_FLAGS "-Wno-sizeof-pointer-memaccess")
ENDIF(NOT MSVC)
ADD_LIBRARY(redland ${FILES_SRC} ${FILES_H})
ADD_DEPENDenCIES(redland ${RASQAL_NAME})
TARGET_LINK_LIBRARIES(redland ${LIBXML2_NAME} ${RAPTOR2_NAME} ${RASQAL_NAME})

#INSTALL(FILES
#	${CMAKE_CURRENT_BINARY_DIR}/librdf.h
#	rdf_concepts.h
#	rdf_digest.h
#	rdf_files.h
#	rdf_hash.h
#	rdf_heuristics.h
#	rdf_init.h
#	rdf_internal.h
#	rdf_iterator.h
#	rdf_list.h
#	rdf_log.h
#	rdf_model.h
#	rdf_node.h
#	rdf_parser.h
#	rdf_query.h
#	rdf_raptor.h
#	rdf_serializer.h
#	rdf_statement.h
#	rdf_storage.h
#	rdf_storage_module.h
#	rdf_stream.h
#	rdf_types.h
#	rdf_uri.h
#	rdf_utf8.h
#	redland.h
#	DESTINATION server/include
#)

INSTALL(
	TARGETS redland
	EXPORT  Redland2Config
	RUNTIME DESTINATION server/bin
	LIBRARY DESTINATION server/bin
	COMPONENT core
)
