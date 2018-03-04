

SET(RASQAL_SRC 
    gettimeofday.c
    rasqal_engine_algebra.c
    rasqal_format_rdf.c
    rasqal_projection.c 
    rasqal_row_compatible.c 
    rasqal_rowsource_project.c 
    rasqal_variable.c
    rasqal_algebra.c  
    rasqal_engine.c   
    rasqal_format_sparql_xml.c 
    rasqal_query.c     
    rasqal_rowsource_aggregation.c 
    rasqal_rowsource_rowsequence.c  
    rasqal_xsd_datatypes.c
    rasqal_bindings.c    
    rasqal_engine_sort.c   
    rasqal_format_sv.c       
    rasqal_query_results.c 
    rasqal_rowsource_assignment.c 
    rasqal_rowsource_slice.c    
    snprintf.c
    rasqal_data_graph.c 
    rasqal_expr.c        
    rasqal_format_table.c    
    rasqal_query_test.c    
    rasqal_rowsource.c   
    rasqal_rowsource_sort.c   
    sparql_lexer.c
    rasqal_dataset.c     
    rasqal_expr_datetimes.c 
    rasqal_formula.c   
    rasqal_query_transform.c 
    rasqal_rowsource_distinct.c  
    rasqal_rowsource_triples.c   
    sparql_parser.c
    rasqal_datetime.c   
    rasqal_expr_evaluate.c  
    rasqal_general.c        
    rasqal_query_write.c   
    rasqal_rowsource_empty.c    
    rasqal_rowsource_union.c  
    strcasecmp.c
    rasqal_decimal.c    
    rasqal_expr_numerics.c  
    rasqal_graph_pattern.c   
    rasqal_random.c   
    rasqal_rowsource_filter.c  
    rasqal_service.c   
    timegm.c
    rasqal_digest.c      
    rasqal_expr_strings.c   
    rasqal_iostream.c       
    rasqal_raptor.c     
    rasqal_rowsource_graph.c  
    rasqal_solution_modifier.c
    rasqal_digest_md5.c  
    rasqal_feature.c       
    rasqal_literal.c   
    rasqal_regex.c  
    rasqal_rowsource_groupby.c 
    rasqal_triple.c
    rasqal_digest_sha1.c 
    rasqal_format_html.c    
    rasqal_map.c         
    rasqal_result_formats.c  
    rasqal_rowsource_having.c   
    rasqal_triples_source.c
    rasqal_double.c    
    rasqal_format_json.c   
    rasqal_prefix.c       
    rasqal_row.c     
    rasqal_rowsource_join.c 
    rasqal_update.c
)
SET(RASQAL_H
mtwist_config.h
    rasqal.h 
    rasqal_internal.h
    sparql_common.h
    sparql_lexer.h
    sparql_parser.h
)

SET_SOURCE_FILES_PROPERTIES(${RASQAL_SRC} PROPERTIES LANGUAGE CXX)
IF(NOT MSVC)
SET_SOURCE_FILES_PROPERTIES(rasqal_digest_md5.c PROPERTIES COMPILE_FLAGS "-Wno-sizeof-pointer-memaccess")
ENDIF(NOT MSVC)
ADD_LIBRARY(rasqal ${RASQAL_SRC} ${RASQAL_H})
	
ADD_DEPENDENCIES(rasqal ${RAPTOR2_NAME})
TARGET_LINK_LIBRARIES(rasqal
	${LIBXML2_NAME}
	${RAPTOR2_NAME}
	${PCRE_NAME}
)


#INSTALL(FILES
#	${CMAKE_CURRENT_BINARY_DIR}/rasqal.h
#	DESTINATION include/rasqal
#	COMPONENT develop
#)

INSTALL(
	TARGETS rasqal
	RUNTIME DESTINATION server/bin
	LIBRARY DESTINATION server/bin
	COMPONENT core
)
