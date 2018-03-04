/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_fxdb.c - RDF Storage in FXDB interface definition.
 *
 * This package is Free Software and part of Redland http://librdf.org/
 *
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 *
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 *
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 *
 *
 */

#include "postgres.h"
#undef fprintf
#undef HAVE_VSNPRINTF
#undef HAVE_STDLIB_H
#undef HAVE_STRING_H
#undef HAVE_SYS_STAT_H
#undef HAVE_SYS_TYPES_H
#undef WIN32_LEAN_AND_MEAN
#undef snprintf
#undef HAVE_GETOPT
#undef HAVE_GETOPT_H
#undef HAVE_GETOPT_LONG
#undef HAVE_SNPRINTF
#undef HAVE_STDINT_H
#undef HAVE_SYS_TIME_H
#undef HAVE_UNISTD_H

#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

//#ifdef WIN32
//#include <win32_rdf_config.h>
//#endif

//#include <stdio.h>
//#include <string.h>
//#ifdef HAVE_STDLIB_H
//#include <stdlib.h>
//#endif
#include <sys/types.h>

#include <rdf_types.h>

#include "StorageEngine.h"
#include "catalog/rdf_storage_handler.h"
#include "interface/PGGraphStorage.h"

#include <redland.h>

using namespace FounderXDB::GraphEngineNS;

typedef struct {

  /* hash of model name in the database (table Models, column ID) */
  u64 model;

  /* if inserts should be optimized by locking and index optimizations */
  int bulk;

  /* if a table with merged models should be maintained */
  int merge;

  /* digest object for node hashes */
  librdf_digest *digest;

	/* 事务句柄，标识当前是否启动了事务 */
	int *transaction_handle;

	/* 元数据表操作句柄类 */
	RDFStorage *storage;

} librdf_storage_fxdb_instance;

/* prototypes for local functions */
static int librdf_storage_fxdb_init(librdf_storage* storage, const char *name,
                                          librdf_hash* options);
static int librdf_storage_fxdb_merge(librdf_storage* storage);
static void librdf_storage_fxdb_terminate(librdf_storage* storage);
static int librdf_storage_fxdb_open(librdf_storage* storage,
                                          librdf_model* model);
static int librdf_storage_fxdb_close(librdf_storage* storage);
static int librdf_storage_fxdb_sync(librdf_storage* storage);
static int librdf_storage_fxdb_size(librdf_storage* storage);
static int librdf_storage_fxdb_add_statement(librdf_storage* storage,
                                              librdf_statement* statement);
static int librdf_storage_fxdb_add_statements(librdf_storage* storage,
                                               librdf_stream* statement_stream);
static int librdf_storage_fxdb_remove_statement(librdf_storage* storage,
                                                 librdf_statement* statement);
static int librdf_storage_fxdb_contains_statement(librdf_storage* storage,
                                                   librdf_statement* statement);


librdf_stream* librdf_storage_fxdb_serialise(librdf_storage* storage);

static librdf_stream* librdf_storage_fxdb_find_statements(librdf_storage* storage,
                                                                librdf_statement* statement);
static librdf_stream* librdf_storage_fxdb_find_statements_with_options(librdf_storage* storage,
                                                                             librdf_statement* statement,
                                                                             librdf_node* context_node,
                                                                             librdf_hash* options);

/* context functions */
static int librdf_storage_fxdb_context_add_statement(librdf_storage* storage,
                                                           librdf_node* context_node,
                                                           librdf_statement* statement);
static int librdf_storage_fxdb_context_add_statements(librdf_storage* storage,
                                                            librdf_node* context_node,
                                                            librdf_stream* statement_stream);
static int librdf_storage_fxdb_context_remove_statement(librdf_storage* storage,
                                                              librdf_node* context_node,
                                                              librdf_statement* statement);
static int librdf_storage_fxdb_context_remove_statements(librdf_storage* storage,
                                                               librdf_node* context_node);
static librdf_stream*
       librdf_storage_fxdb_context_serialise(librdf_storage* storage,
                                                   librdf_node* context_node);
static librdf_stream* librdf_storage_fxdb_find_statements_in_context(librdf_storage* storage,
                                                                           librdf_statement* statement,
                                                                           librdf_node* context_node);
static librdf_iterator* librdf_storage_fxdb_get_contexts(librdf_storage* storage);

static void librdf_storage_fxdb_register_factory(librdf_storage_factory *factory);

static RDF_Statements statement_get_store(librdf_statement *statements, librdf_node* context_node);

static void statement_store_from_id(RDF_Statements statement_store, u64 subject, 
																		u64 predicate, u64 object, u64 context);

#ifdef MODULAR_LIBRDF
void librdf_storage_module_register_factory(librdf_world *world);
#endif

/* "private" helper definitions */
typedef struct {
  librdf_storage *storage;
  librdf_statement *current_statement;
  librdf_node *current_context;
  librdf_statement *query_statement;
  librdf_node *query_context;
	SysScanDesc sd;
	RDFStorage *rdf_storage;

	/* 是否使用内容匹配查找(不使用hash id) */
  int is_literal_match;
} librdf_storage_fxdb_sos_context;

typedef struct {
  librdf_storage *storage;
  librdf_node *current_context;
	SysScanDesc sd;
	RDFStorage *rdf_storage;
} librdf_storage_fxdb_get_contexts_context;

static u64 librdf_storage_fxdb_hash(librdf_storage* storage,
                                          const char *type,
                                          const char *string, size_t length);
static u64 librdf_storage_fxdb_node_hash(librdf_storage* storage,
                                               librdf_node* node);
static int librdf_storage_fxdb_start_bulk(librdf_storage* storage);
static int librdf_storage_fxdb_stop_bulk(librdf_storage* storage);
static int librdf_storage_fxdb_context_add_statement_helper(librdf_storage* storage,
                                                                  librdf_node* context_node,
                                                                  librdf_statement* statement);

/* methods for stream of statements */
static int librdf_storage_fxdb_find_statements_in_context_end_of_stream(void* context);
static int librdf_storage_fxdb_find_statements_in_context_next_statement(void* context);
static void* librdf_storage_fxdb_find_statements_in_context_get_statement(void* context, int flags);
static void librdf_storage_fxdb_find_statements_in_context_finished(void* context);

/* methods for iterator for contexts */
static int librdf_storage_fxdb_get_contexts_end_of_iterator(void* context);
static int librdf_storage_fxdb_get_contexts_next_context(void* context);
static void* librdf_storage_fxdb_get_contexts_get_context(void* context, int flags);
static void librdf_storage_fxdb_get_contexts_finished(void* context);



static int librdf_storage_fxdb_transaction_rollback(librdf_storage* storage);



/* functions implementing storage api */


/*
 * librdf_storage_fxdb_hash - Find hash value of string.
 * @storage: the storage
 * @type: character type of node to hash ("R", "L" or "B")
 * @string: a string to get hash for
 * @length: length of string
 *
 * Find hash value of string.
 *
 * Return value: Non-zero on succes.
 **/
static u64
librdf_storage_fxdb_hash(librdf_storage* storage, const char *type,
                               const char *string, size_t length)
{
  librdf_storage_fxdb_instance* context;
  u64 hash;
  byte* digest;
  int i;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(string, char*, 0);

  context = (librdf_storage_fxdb_instance*)storage->instance;

  /* (Re)initialize digest object */
  librdf_digest_init(context->digest);

  /* Update digest with data */
  if(type)
    librdf_digest_update(context->digest, (unsigned char*)type, 1);
  librdf_digest_update(context->digest, (unsigned char*)string, length);
  librdf_digest_final(context->digest);

  /* Copy first 8 bytes of digest into unsigned 64bit hash
   * using a method portable across big/little endianness
   *
   * Fixes Issue#0000023 - http://bugs.librdf.org/mantis/view.php?id=23
   */
  digest = (byte*) librdf_digest_get_digest(context->digest);
  hash = 0;
  for(i=0; i<8; i++)
    hash += ((u64) digest[i]) << (i*8);

  return hash;
}


/*
 * librdf_storage_fxdb_init:
 * @storage: the storage
 * @name: model name
 * @options: [new] [, bulk] [, merge].
 *
 * INTERNAL - 创建statements元数据表
 *
 * The boolean bulk option can be set to true if optimized inserts (table
 * locks and temporary key disabling) is wanted. Note that this will block
 * all other access, and requires table locking and alter table privileges.
 *
 * The boolean merge option can be set to true if a merged "view" of all
 * models should be maintained. This "view" will be a table with TYPE=MERGE.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_fxdb_init(librdf_storage* storage, const char *name,
                               librdf_hash* options)
{
	using namespace FounderXDB::StorageEngineNS;
  librdf_storage_fxdb_instance *context=(librdf_storage_fxdb_instance*)storage->instance;
  int status=0;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(name, char*, 1);

  context = LIBRDF_CALLOC(librdf_storage_fxdb_instance*, 1,
                          sizeof(*context));
  if(!context) {
    librdf_free_hash(options);
    return 1;
  }

  librdf_storage_set_instance(storage, context);


  /* Create digest */
  if(!(context->digest=librdf_new_digest(storage->world,"MD5"))) {
    librdf_free_hash(options);
    return 1;
  }

  /* Save hash of model name */
  context->model = librdf_storage_fxdb_hash(storage, NULL, name,
                                                  strlen(name));

  /* Maintain merge table? */
  context->merge=(librdf_hash_get_as_boolean(options, "merge")>0);

	FounderXDB::StorageEngineNS::MemoryContext *topMem = 
		FounderXDB::StorageEngineNS::MemoryContext::getMemoryContext(FounderXDB::StorageEngineNS::MemoryContext::Top);
	context->storage = new(*topMem)PGGraphStorage();

	/* Start new transaction. */
	if (storage->factory->transaction_start(storage))
	{
		librdf_free_hash(options);
		return 1;
	}

  /* Create tables, if new and not existing */
  if(!status && (librdf_hash_get_as_boolean(options, "new")>0))
  {
		if (!context->storage->CreateStatementsTable(context->model, name))
			status = 1;
  } else 
	{
		if (!context->storage->GetStorageEntrySetID(context->model, name))
			status = 1;
	}

  /* Optimize loads? */
	context->bulk=(librdf_hash_get_as_boolean(options, "bulk")>0);

	/* Truncate model? */
	if(!status && (librdf_hash_get_as_boolean(options, "new")>0))
		status=librdf_storage_fxdb_context_remove_statements(storage, NULL);

	if (storage->factory->transaction_commit(storage))
	{
		status = 1;
	}

	/* Unused options: write (always...) */
	librdf_free_hash(options);

  return status;
}


/*
 * librdf_storage_fxdb_merge:
 * @storage: the storage
 *
 * INTERNAL - (re)create merged "view" of all models
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_fxdb_merge(librdf_storage* storage)
{
	LIBRDF_ASSERT_RETURN(0, "Unknow function!", NULL);

  return 0;
}


/*
 * librdf_storage_fxdb_terminate:
 * @storage: the storage
 *
 * INTERNAL - Close the storage.
 *
 * Return value: None.
 **/
static void
librdf_storage_fxdb_terminate(librdf_storage* storage)
{
  librdf_storage_fxdb_instance *context=(librdf_storage_fxdb_instance*)storage->instance;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(storage, librdf_storage);

  if(context->digest)
    librdf_free_digest(context->digest);

  if(context->transaction_handle)
    librdf_storage_fxdb_transaction_rollback(storage);

	if(context->storage)
		delete ((librdf_storage_fxdb_instance *)storage->instance)->storage;

  LIBRDF_FREE(librdf_storage_fxdb_instance, storage->instance);
}


/*
 * librdf_storage_fxdb_open:
 * @storage: the storage
 * @model: the model
 *
 * INTERNAL - Create or open model in database (nop).
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_fxdb_open(librdf_storage* storage, librdf_model* model)
{
  return 0;
}


/*
 * librdf_storage_fxdb_close:
 * @storage: the storage
 *
 * INTERNAL - Close model (nop).
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_fxdb_close(librdf_storage* storage)
{
  librdf_storage_fxdb_transaction_rollback(storage);

  return librdf_storage_fxdb_sync(storage);
}


/*
 * librdf_storage_fxdb_sync
 * @storage: the storage
 *
 * INTERNAL - Flush all tables, making sure they are saved on disk.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_fxdb_sync(librdf_storage* storage)
{
  librdf_storage_fxdb_instance *context=(librdf_storage_fxdb_instance*)storage->instance;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  /* Make sure optimizing for bulk operations is stopped? */
  if(context->bulk)
    librdf_storage_fxdb_stop_bulk(storage);

  return 0;
}


/*
 * librdf_storage_fxdb_size:
 * @storage: the storage
 *
 * INTERNAL - Close model (nop).
 *
 * Return value: Negative on failure.
 **/
static int
librdf_storage_fxdb_size(librdf_storage* storage)
{
  librdf_storage_fxdb_instance *context=(librdf_storage_fxdb_instance*)storage->instance;
  long count;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, -1);

  if(!context->storage->CounterStorageSize(&count)) {
		librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
			"query for model size failed.");
  }

  return LIBRDF_BAD_CAST(int, count);
}


static int
librdf_storage_fxdb_add_statement(librdf_storage* storage,
                                   librdf_statement* statement)
{
  return librdf_storage_fxdb_context_add_statement_helper(storage, NULL,
                                                           statement);
}


/*
 * librdf_storage_fxdb_add_statements:
 * @storage: the storage
 * @statement_stream: the stream of statements
 *
 * INTERNAL - Add statements in stream to storage, without context.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_fxdb_add_statements(librdf_storage* storage,
                                    librdf_stream* statement_stream)
{
  return librdf_storage_fxdb_context_add_statements(storage, NULL,
                                                     statement_stream);
}

/*
 * librdf_storage_fxdb_node_hash - Create hash value for node
 * @storage: the storage
 * @node: a node to get hash for (and possibly create in database)
 * @add: whether to add the node to the database
 *
 * Return value: Non-zero on succes.
 **/
static u64
librdf_storage_fxdb_node_hash(librdf_storage* storage,
                               librdf_node* node)
{
  if(!node) return 0;
  librdf_node_type type=librdf_node_get_type(node);
  u64 hash;
  size_t nodelen;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(node, librdf_node, 0);

  if(type==LIBRDF_NODE_TYPE_RESOURCE) {
    /* Get hash */
    unsigned char *uri=librdf_uri_as_counted_string(librdf_node_get_uri(node), &nodelen);
    hash = librdf_storage_fxdb_hash(storage, "R", (char*)uri, nodelen);
  } else if(type==LIBRDF_NODE_TYPE_LITERAL) {
    /* Get hash */
    unsigned char *value, *datatype=0;
    char *lang, *nodestring;
    librdf_uri *dt;
    size_t valuelen, langlen=0, datatypelen=0;

    value=librdf_node_get_literal_value_as_counted_string(node,&valuelen);
    lang=librdf_node_get_literal_value_language(node);
    if(lang)
      langlen=strlen(lang);
    dt=librdf_node_get_literal_value_datatype_uri(node);
    if(dt)
      datatype=librdf_uri_as_counted_string(dt,&datatypelen);
    if(datatype)
      datatypelen=strlen((const char*)datatype);

    /* Create composite node string for hash generation */
    nodestring = LIBRDF_MALLOC(char*, valuelen + langlen + datatypelen + 3);
    if(!nodestring) {
      return 0;
    }
    strcpy(nodestring, (const char*)value);
    strcat(nodestring, "<");
    if(lang)
      strcat(nodestring, lang);
    strcat(nodestring, ">");
    if(datatype)
      strcat(nodestring, (const char*)datatype);
    nodelen=valuelen+langlen+datatypelen+2;
    hash = librdf_storage_fxdb_hash(storage, "L", nodestring, nodelen);
    LIBRDF_FREE(char*, nodestring);
  } else if(type==LIBRDF_NODE_TYPE_BLANK) {
    /* Get hash */
    unsigned char *name = librdf_node_get_blank_identifier(node);
    nodelen = strlen((const char*)name);
    hash = librdf_storage_fxdb_hash(storage, "B", (char*)name, nodelen);
  } else {
    /* Some node type we don't know about? */
    return 0;
  }

  return hash;
}


/*
 * librdf_storage_fxdb_start_bulk:
 * @storage: the storage
 *
 * INTERNAL - Prepare for bulk insert operation
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_fxdb_start_bulk(librdf_storage* storage)
{
  return 1;
}


/*
 * librdf_storage_fxdb_stop_bulk:
 * @storage: the storage
 *
 * INTERNAL - End bulk insert operation
 *
 * Return value: Non-zero on failure.
 */
static int
librdf_storage_fxdb_stop_bulk(librdf_storage* storage)
{
  return 1;
}


/*
 * librdf_storage_fxdb_context_add_statements:
 * @storage: the storage
 * @context_node: #librdf_node object
 * @statement_stream: the stream of statements
 *
 * INTERNAL - Add statements in stream to storage, with context.
 *
 * Return value: Non-zero on failure.
 **/
static int
librdf_storage_fxdb_context_add_statements(librdf_storage* storage,
                                            librdf_node* context_node,
                                            librdf_stream* statement_stream)
{
  librdf_storage_fxdb_instance* context=(librdf_storage_fxdb_instance*)storage->instance;
  u64 ctxt=0;
  int helper=0;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement_stream, librdf_stream, 1);

  /* Optimize for bulk loads? */
  if(context->bulk) {
    if(librdf_storage_fxdb_start_bulk(storage))
      return 1;
  }

  /* Find hash for context, creating if necessary */
  if(context_node) {
    ctxt=librdf_storage_fxdb_node_hash(storage,context_node);
    if(!ctxt)
      return 1;
  }

  while(!helper && !librdf_stream_end(statement_stream)) {
    librdf_statement* statement=librdf_stream_get_object(statement_stream);
    if(!context->bulk) {
      /* Do not add duplicate statements
       * but do not check for this when in bulk mode.
       */
      if(librdf_storage_fxdb_contains_statement(storage, statement)) {
        librdf_stream_next(statement_stream);
        continue;
      }
    }

    helper=librdf_storage_fxdb_context_add_statement_helper(storage, context_node,
                                                             statement);
    librdf_stream_next(statement_stream);
  }

  return helper;
}


/*
 * librdf_storage_fxdb_context_add_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to add
 *
 * INTERNAL - Add a statement to a storage context
 *
 * Return value: non 0 on failure
 **/
static int
librdf_storage_fxdb_context_add_statement(librdf_storage* storage,
                                          librdf_node* context_node,
                                          librdf_statement* statement)
{
  u64 ctxt=0;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

  /* Find hash for context, creating if necessary */
  if(context_node) {
    ctxt=librdf_storage_fxdb_node_hash(storage,context_node);
    if(!ctxt)
      return 1;
  }

  return librdf_storage_fxdb_context_add_statement_helper(storage, context_node,
                                                           statement);
}

static
void statement_resource_free(nodes *node)
{
	if (node->data.resource.uri)
		LIBRDF_FREE(char *, node->data.resource.uri);
}

static
void statement_bnode_free(nodes *node)
{
	if (node->data.bnode.name)
		LIBRDF_FREE(char *, node->data.bnode.name);
}

static
void statement_literal_free(nodes *node)
{
	if (node->data.literal.values)
		LIBRDF_FREE(char *, node->data.literal.values);
	if (node->data.literal.language)
		LIBRDF_FREE(char *, node->data.literal.language);
	if (node->data.literal.data_type)
		LIBRDF_FREE(char *, node->data.literal.data_type);
}

static
void statement_node_free(nodes *node)
{
	switch(node->type)
	{
	case RDF_TYPE_RESOURCE:
		statement_resource_free(node);
		break;
	case RDF_TYPE_LITERAL:
		statement_literal_free(node);
		break;
	case RDF_TYPE_BNODES:
		statement_bnode_free(node);
		break;
	}
	LIBRDF_FREE(nodes *, node);
}

static
void statement_store_free(RDF_Statements statement_store)
{
	if (statement_store->subject)
		statement_node_free(statement_store->subject);

	if (statement_store->predicate)
		statement_node_free(statement_store->predicate);

	if (statement_store->object)
		statement_node_free(statement_store->object);

	if (statement_store->ctxt)
		statement_node_free(statement_store->ctxt);

	LIBRDF_FREE(RDF_Statements, statement_store);
}

static
void term_type_uri_copy(librdf_node *term, nodes *node)
{
	node->type = RDF_TYPE_RESOURCE;
	size_t size;
	char *uri_str = (char *)librdf_uri_as_counted_string(librdf_node_get_uri(term), &size);

	if (!uri_str)
		return ;

	node->data.resource.uri = LIBRDF_CALLOC(char *, 1, size + 1);
	memcpy(node->data.resource.uri, uri_str, size);
	node->data.resource.uri_lens = size;
}

static
void term_type_blank_copy(raptor_term *term, nodes *node)
{
	node->type = RDF_TYPE_BNODES;

	if (!term->value.blank.string)
		return ;

	node->data.bnode.name = LIBRDF_CALLOC(char *, 1, term->value.blank.string_len + 1);
	strcpy(node->data.bnode.name, (char *)term->value.blank.string);
	node->data.bnode.name_lens = term->value.blank.string_len;
}

static
void term_type_literal_copy(librdf_node *term, nodes *node)
{
	size_t dt_size;
	char *dt_str = NULL;
	raptor_uri *dt_uri = librdf_node_get_literal_value_datatype_uri(term);

	if (dt_uri)
		dt_str = (char *)librdf_uri_as_counted_string(dt_uri, &dt_size);
		

	if (term->value.literal.string)
	{
		node->data.literal.values = LIBRDF_CALLOC(char *, 1, term->value.literal.string_len + 1);
		strcpy(node->data.literal.values, (char *)term->value.literal.string);
		node->data.literal.value_lens = term->value.literal.string_len;
	}

	if (term->value.literal.language)
	{
		node->data.literal.language = LIBRDF_CALLOC(char *, 1, term->value.literal.language_len + 1);
		strcpy(node->data.literal.language, (char *)term->value.literal.language);
		node->data.literal.language_lens = term->value.literal.language_len;
	}

	if (dt_str)
	{
		node->data.literal.data_type = LIBRDF_CALLOC(char *, 1, dt_size + 1);
		strcpy(node->data.literal.data_type, dt_str);
		node->data.literal.data_type_lens = dt_size;
	}

	node->type = RDF_TYPE_LITERAL;
}

static
nodes *statement_get_nodes(raptor_term *term)
{
	LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(term, raptor_term, NULL);

	nodes *node = LIBRDF_CALLOC(nodes *, 1, sizeof(nodes));

	switch(term->type)
	{
		case RAPTOR_TERM_TYPE_URI :
			term_type_uri_copy(term, node);
			break;
		case RAPTOR_TERM_TYPE_LITERAL :
			term_type_literal_copy(term, node);
			break;
		case RAPTOR_TERM_TYPE_BLANK :
			term_type_blank_copy(term, node);
			break;
		default :
			LIBRDF_ASSERT_RETURN(0, "Unknow type!", NULL);
	}

	return node;
}

/*
 * statement_get_store
 * @statements: #librdf_statement statement to copy
 *
 * INTERNAL - 从一个#librdf_statement构造一个#RDF_Statements返回
 *
 * Return value: non-zero on failure
 **/
static
RDF_Statements statement_get_store(librdf_statement *statements, librdf_node* context_node)
{
	RDF_Statements statement_store = LIBRDF_CALLOC(RDF_Statements, 1,
		sizeof(RDF_Statements_Store));
	
	if (statements->subject)
		statement_store->subject = statement_get_nodes(statements->subject);
	if (statements->predicate)
		statement_store->predicate = statement_get_nodes(statements->predicate);
	if (statements->object)
		statement_store->object = statement_get_nodes(statements->object);
	if (context_node)
		statement_store->ctxt = statement_get_nodes(context_node);

	statement_store->inf_id = statements->inference_id;

	return statement_store;
}

/*
 * statement_store_from_id
 *
 * INTERNAL - 从给定的hash id值构造一个#RDF_Statements
 **/
static void statement_store_from_id(RDF_Statements statement_store, u64 subject, 
																		u64 predicate, u64 object, u64 context)

{
	statement_store->subject_id = subject;
	statement_store->predicate_id = predicate;
	statement_store->object_id = object;
	statement_store->context = context;
}

/*
 * librdf_storage_fxdb_context_add_statement_helper
 * @storage: #librdf_storage object
 * @ctxt: u64 context hash
 * @statement: #librdf_statement statement to add
 *
 * INTERNAL - Perform actual addition of a statement to a storage context
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_fxdb_context_add_statement_helper(librdf_storage* storage,
                                          librdf_node* context_node, librdf_statement* statement)
{
  librdf_storage_fxdb_instance* context=(librdf_storage_fxdb_instance*)storage->instance;
 
  u64 subject, predicate, object, ctxt = 0;
  int status = 1;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);
	
	/* Find hashes for nodes, creating if necessary */
	subject=librdf_storage_fxdb_node_hash(storage,
		librdf_statement_get_subject(statement));
	predicate=librdf_storage_fxdb_node_hash(storage,
		librdf_statement_get_predicate(statement));
	object=librdf_storage_fxdb_node_hash(storage,
		librdf_statement_get_object(statement));
	if (context_node)
		ctxt = librdf_storage_fxdb_node_hash(storage, context_node);
	if(subject && predicate && object) {
		RDF_Statements statement_store;
		statement_store = statement_get_store(statement, context_node);

		statement_store_from_id(statement_store, subject, predicate, object, ctxt);

		if(!context->storage->StatementAdd(statement_store)) {
			librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
				"Add Statements failed.");
		} else
		{
			status = 0;
		}

		statement_store_free(statement_store);
	}

  return status;
}


/*
 * librdf_storage_fxdb_contains_statement:
 * @storage: the storage
 * @statement: a complete statement
 *
 * INTERNAL - Test if a given complete statement is present in the model
 *
 * Return value: Non-zero if the model contains the statement.
 **/
static int
librdf_storage_fxdb_contains_statement(librdf_storage* storage,
                                             librdf_statement* statement)
{
  librdf_storage_fxdb_instance* context = (librdf_storage_fxdb_instance*)storage->instance;
  u64 subject, predicate, object;
  int status = 0;
	RDF_Statements_Store statement_store;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 0);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 0);

	/* Find hashes for nodes */
	subject=librdf_storage_fxdb_node_hash(storage,
		librdf_statement_get_subject(statement));
	predicate=librdf_storage_fxdb_node_hash(storage,
		librdf_statement_get_predicate(statement));
	object=librdf_storage_fxdb_node_hash(storage,
		librdf_statement_get_object(statement));

	statement_store_from_id(&statement_store, subject, predicate, object, 0);

	statement_store.inf_id = librdf_statement_get_inference_id(statement);

	if(subject && predicate && object) {
		if(context->storage->StatementExists(&statement_store))
			status = 1;
	}

  return status;
}


/*
 * librdf_storage_fxdb_remove_statement:
 * @storage: #librdf_storage object
 * @statement: #librdf_statement statement to remove
 *
 * INTERNAL - Remove a statement from storage
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_fxdb_remove_statement(librdf_storage* storage, librdf_statement* statement)
{
  return librdf_storage_fxdb_context_remove_statement(storage,NULL,statement);
}


/*
 * librdf_storage_fxdb_context_remove_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to remove
 *
 * INTERNAL - Remove a statement from a storage context
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_fxdb_context_remove_statement(librdf_storage* storage,
                                             librdf_node* context_node,
                                             librdf_statement* statement)
{
  librdf_storage_fxdb_instance* context=(librdf_storage_fxdb_instance*)storage->instance;
  u64 subject = 0, predicate = 0, object = 0, ctxt = 0;
  int status = 1;
	RDF_Statements_Store statement_store;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(statement, librdf_statement, 1);

	/* Find hashes for nodes */
	if (librdf_statement_get_subject(statement) != NULL)
		subject=librdf_storage_fxdb_node_hash(storage,
		librdf_statement_get_subject(statement));
	if (librdf_statement_get_predicate(statement) != NULL)
		predicate=librdf_storage_fxdb_node_hash(storage,
		librdf_statement_get_predicate(statement));
	if (librdf_statement_get_object(statement) != NULL)
		object=librdf_storage_fxdb_node_hash(storage,
		librdf_statement_get_object(statement));
	statement_store.inf_id = librdf_statement_get_inference_id(statement);
		

	if (!subject && !predicate && !object && !context_node && statement_store.inf_id < 0) 
	{
		if(context->storage->StatementRemove(NULL)) {
			status = 0;
		} else {
			librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
				"Remove statements failed.");
		}
	} else 
	{
		if(context_node) {
			ctxt=librdf_storage_fxdb_node_hash(storage,context_node);
		}
		statement_store.subject_id = subject;
		statement_store.predicate_id = predicate;
		statement_store.object_id = object;
		statement_store.context = 0;

		if(ctxt != 0) 
			statement_store.context = ctxt;

		if(context->storage->StatementRemove(&statement_store)) {
			status = 0;
		} else {
			librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
				"Remove statements failed.");
		}
	}  //end if (!subject && !predicate && !object)

  return status;
}


/*
 * librdf_storage_fxdb_context_remove_statements:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 *
 * INTERNAL - Remove all statement from a storage context
 *
 * Return value: non-zero on failure
 **/
static int
librdf_storage_fxdb_context_remove_statements(librdf_storage* storage,
                                               librdf_node* context_node)
{
  librdf_storage_fxdb_instance* context=(librdf_storage_fxdb_instance*)storage->instance;
  u64 ctxt=0;
  int status = 1;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

	if(context_node) {
		ctxt=librdf_storage_fxdb_node_hash(storage,context_node);
	}
	if(context->storage->StatementContextRemove(ctxt)) {
		status = 0;
	} else {
		librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
			"delete Statements failed.");
	}

  return status;
}


/*
 * librdf_storage_fxdb_serialise:
 * @storage: the storage
 *
 * INTERNAL - Return a stream of all statements in a storage.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
librdf_stream* librdf_storage_fxdb_serialise(librdf_storage* storage)
{
  return librdf_storage_fxdb_find_statements_in_context(storage,NULL,NULL);
}


/*
 * librdf_storage_fxdb_find_statements:
 * @storage: the storage
 * @statement: the statement to match
 *
 * INTERNAL - Find a graph of statements in storage.
 *
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_fxdb_find_statements(librdf_storage* storage,
                                     librdf_statement* statement)
{
  return librdf_storage_fxdb_find_statements_in_context(storage,statement,NULL);
}


/*
 * librdf_storage_fxdb_context_serialise:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 *
 * INTERNAL - List all statements in a storage context
 *
 * Return value: #librdf_stream of statements or NULL on failure or context is empty
 **/
static librdf_stream*
librdf_storage_fxdb_context_serialise(librdf_storage* storage,
                                       librdf_node* context_node)
{
  return librdf_storage_fxdb_find_statements_in_context(storage,NULL,context_node);
}


/*
 * librdf_storage_fxdb_find_statements_in_context:
 * @storage: the storage
 * @statement: the statement to match
 * @context_node: the context to search
 *
 * INTERNAL - Find a graph of statements in a storage context.
 *
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_fxdb_find_statements_in_context(librdf_storage* storage, librdf_statement* statement,librdf_node* context_node)
{
  return librdf_storage_fxdb_find_statements_with_options(storage, statement, context_node, NULL);
}


/*
 * librdf_storage_fxdb_find_statements_with_options:
 * @storage: the storage
 * @statement: the statement to match
 * @context_node: the context to search
 * @options: #librdf_hash of match options or NULL
 *
 * INTERNAL - Find a graph of statements in a storage context with options.
 *
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 *
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_fxdb_find_statements_with_options(librdf_storage* storage,
                                                  librdf_statement* statement,
                                                  librdf_node* context_node,
                                                  librdf_hash* options)
{
  librdf_storage_fxdb_instance* context=(librdf_storage_fxdb_instance*)storage->instance;
  librdf_storage_fxdb_sos_context* sos;
  librdf_node *subject=NULL, *predicate=NULL, *object=NULL;
	u64 subject_id = 0, predicate_id = 0, object_id = 0, context_id = 0;
	RDF_Statements_Store statement_store;
	SysScanDesc sd = NULL;
  librdf_stream *stream;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  /* Initialize sos context */
  sos = LIBRDF_CALLOC(librdf_storage_fxdb_sos_context*, 1, sizeof(*sos));
  if(!sos)
    return NULL;

  sos->storage=storage;
  librdf_storage_add_reference(sos->storage);

  if(statement)
    sos->query_statement=librdf_new_statement_from_statement(statement);
  if(context_node)
    sos->query_context=librdf_new_node_from_node(context_node);
  sos->current_statement=NULL;
  sos->current_context=NULL;

  if(options) {
    sos->is_literal_match=librdf_hash_get_as_boolean(options, "match-substring");
  }

  if(statement) {
    subject=librdf_statement_get_subject(statement);
    predicate=librdf_statement_get_predicate(statement);
    object=librdf_statement_get_object(statement);
  }

  /* Subject */
  if(statement && subject) {
		subject_id = librdf_storage_fxdb_node_hash(storage,subject);
  }

  /* Predicate */
  if(statement && predicate) {
		predicate_id = librdf_storage_fxdb_node_hash(storage, predicate);
  }

  /* Object */
  if(statement && object) {
		object_id = librdf_storage_fxdb_node_hash(storage, object);
  }

  /* Context */
  if(context_node) {
		context_id = librdf_storage_fxdb_node_hash(storage,context_node);
  }
	statement_store.inf_id = librdf_statement_get_inference_id(statement);

	statement_store_from_id(&statement_store, subject_id, predicate_id, object_id, context_id);

	sd = context->storage->StatementSearch(&statement_store);
	sos->sd = sd;
	sos->rdf_storage = context->storage;

	if (!sd) {
		librdf_log(sos->storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
			"find statements failed.");
		librdf_storage_fxdb_find_statements_in_context_finished((void*)sos);
		return NULL;
	}

  /* Get first statement, if any, and initialize stream */
  if(librdf_storage_fxdb_find_statements_in_context_next_statement(sos) ) {
    librdf_storage_fxdb_find_statements_in_context_finished((void*)sos);
    return librdf_new_empty_stream(storage->world);
  }

  stream=librdf_new_stream(storage->world,(void*)sos,
                           &librdf_storage_fxdb_find_statements_in_context_end_of_stream,
                           &librdf_storage_fxdb_find_statements_in_context_next_statement,
                           &librdf_storage_fxdb_find_statements_in_context_get_statement,
                           &librdf_storage_fxdb_find_statements_in_context_finished);
  if(!stream) {
    librdf_storage_fxdb_find_statements_in_context_finished((void*)sos);
    return NULL;
  }

  return stream;
}

static int
librdf_storage_fxdb_find_statements_in_context_end_of_stream(void* context)
{
  librdf_storage_fxdb_sos_context* sos=(librdf_storage_fxdb_sos_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, 1);

  return sos->current_statement==NULL;
}


static int
librdf_storage_fxdb_find_statements_in_context_next_statement(void* context)
{
	librdf_storage_fxdb_sos_context* sos=(librdf_storage_fxdb_sos_context*)context;
	librdf_node *subject=NULL, *predicate=NULL, *object=NULL;
	librdf_node *node=NULL;
	RDF_Statements statement_store = NULL;
	int type;

	LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, 1);

	if( (statement_store = sos->rdf_storage->StatementGetNext(sos->sd)) != NULL ) {

		/* Get ready for context */
		if(sos->current_context)
			librdf_free_node(sos->current_context);
		sos->current_context=NULL;
		/* Is this a query with statement parts? */
		if(sos->query_statement) {
			subject=librdf_statement_get_subject(sos->query_statement);
			predicate=librdf_statement_get_predicate(sos->query_statement);
			if(sos->is_literal_match)
				object=NULL;
			else
				object=librdf_statement_get_object(sos->query_statement);
		}


		/* Make sure we have a statement object to return */
		if(!sos->current_statement) {
			if(!(sos->current_statement=librdf_new_statement(sos->storage->world)))
			{
				sos->rdf_storage->StetementFree(statement_store);
				return 1;
			}
		}

		librdf_statement_clear(sos->current_statement);
		/* Query without variables? */
		if(subject && predicate && object && sos->query_context) {
			librdf_statement_set_subject(sos->current_statement,librdf_new_node_from_node(subject));
			librdf_statement_set_predicate(sos->current_statement,librdf_new_node_from_node(predicate));
			librdf_statement_set_object(sos->current_statement,librdf_new_node_from_node(object));
			sos->current_context=librdf_new_node_from_node(sos->query_context);
		} else {
			///* Turn row parts into statement and context */
			/* Subject - constant or from row? */
			if(subject) {
				librdf_statement_set_subject(sos->current_statement,librdf_new_node_from_node(subject));
			} else {
				/* Resource or Bnode? */
				type = statement_store->subject->type;
				if(type == RDF_TYPE_RESOURCE) {
					if(!(node=librdf_new_node_from_uri_string(sos->storage->world,
						(const unsigned char*)statement_store->subject->data.resource.uri)))
					{
						sos->rdf_storage->StetementFree(statement_store);
						return 1;
					}
				} else if(type == RDF_TYPE_BNODES) {
					if(!(node=librdf_new_node_from_blank_identifier(sos->storage->world,
						(const unsigned char*)statement_store->subject->data.bnode.name)))
					{
						sos->rdf_storage->StetementFree(statement_store);
						return 1;
					}
				} else
				{
					sos->rdf_storage->StetementFree(statement_store);
					return 1;
				}

				librdf_statement_set_subject(sos->current_statement,node);
			} /* end else subject */

			/* Predicate - constant or from row? */
			if(predicate) {
				librdf_statement_set_predicate(sos->current_statement,librdf_new_node_from_node(predicate));
			} else {
				/* Resource? */
				type = statement_store->predicate->type;
				if(type == RDF_TYPE_RESOURCE) {
					if(!(node=librdf_new_node_from_uri_string(sos->storage->world,
						(const unsigned char*)statement_store->predicate->data.resource.uri)))
					{
						sos->rdf_storage->StetementFree(statement_store);
						return 1;
					}
				} else
				{
					sos->rdf_storage->StetementFree(statement_store);
					return 1;
				}

				librdf_statement_set_predicate(sos->current_statement,node);
			} /* end else predicate */
			/* Object - constant or from row? */
			if(object) {
				librdf_statement_set_object(sos->current_statement,librdf_new_node_from_node(object));
			} else {
				/* Resource, Bnode or Literal? */
				type = statement_store->object->type;
				if(type == RDF_TYPE_RESOURCE) {
					if(!(node=librdf_new_node_from_uri_string(sos->storage->world,
						(const unsigned char*)statement_store->object->data.resource.uri)))
					{
						sos->rdf_storage->StetementFree(statement_store);
						return 1;
					}
				} else if(type == RDF_TYPE_BNODES) {
					if(!(node=librdf_new_node_from_blank_identifier(sos->storage->world,
						(const unsigned char*)statement_store->object->data.bnode.name)))
					{
						sos->rdf_storage->StetementFree(statement_store);
						return 1;
					}
				} else if(type == RDF_TYPE_LITERAL) {
					/* Typed literal? */
					librdf_uri *datatype=NULL;
					if(statement_store->object->data.literal.data_type_lens > 0)
						datatype=librdf_new_uri(sos->storage->world,
						(const unsigned char*)statement_store->object->data.literal.data_type);
					if(!(node=librdf_new_node_from_typed_literal(sos->storage->world,
						(unsigned char *)statement_store->object->data.literal.values,
						statement_store->object->data.literal.language,
						datatype)))
					{
						sos->rdf_storage->StetementFree(statement_store);
						return 1;
					}
				} else
				{
					sos->rdf_storage->StetementFree(statement_store);
					return 1;
				}

				librdf_statement_set_object(sos->current_statement,node);
			} /* end else object */

			/* Context - constant or from row? */
			if(sos->query_context) {
				sos->current_context=librdf_new_node_from_node(sos->query_context);
			} else {
				/* Resource, Bnode or Literal? */
				if (statement_store->ctxt)
				{
					type = statement_store->ctxt->type;
					if(type == RDF_TYPE_RESOURCE) {
						if(!(node=librdf_new_node_from_uri_string(sos->storage->world,
							(const unsigned char*)statement_store->ctxt->data.resource.uri)))
						{
							sos->rdf_storage->StetementFree(statement_store);
							return 1;
						}
					} else if(type == RDF_TYPE_BNODES) {
						if(!(node=librdf_new_node_from_blank_identifier(sos->storage->world,
							(const unsigned char*)statement_store->ctxt->data.bnode.name)))
						{
							sos->rdf_storage->StetementFree(statement_store);
							return 1;
						}
					} else if(type == RDF_TYPE_LITERAL) {
						/* Typed literal? */
						librdf_uri *datatype=NULL;
						if(statement_store->ctxt->data.literal.data_type_lens > 0)
						{
							datatype=librdf_new_uri(sos->storage->world,
							(const unsigned char*)statement_store->ctxt->data.literal.data_type);
						}
						if(!(node=librdf_new_node_from_typed_literal(sos->storage->world,
							(unsigned char *)statement_store->ctxt->data.literal.values,
							statement_store->ctxt->data.literal.language,
							datatype)))
						{
							sos->rdf_storage->StetementFree(statement_store);
							return 1;
						}
					}
				}else
					/* no context */
					node=NULL;
				sos->current_context=node;
			} /* end else sos->query_context */
		} /* end else subject && predicate && object && sos->query_context */
		if (statement_store)
			sos->rdf_storage->StetementFree(statement_store);
	} else {  /* end if (statement_store = sos->rdf_storage->StatementGetNext(sos->results)) != NULL */
		if(sos->current_statement)
			librdf_free_statement(sos->current_statement);
		sos->current_statement=NULL;
		if(sos->current_context)
			librdf_free_node(sos->current_context);
		sos->current_context=NULL;
	}

	return 0;
}


static void*
librdf_storage_fxdb_find_statements_in_context_get_statement(void* context, int flags)
{
  librdf_storage_fxdb_sos_context* sos=(librdf_storage_fxdb_sos_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, NULL);

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return sos->current_statement;
    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return sos->current_context;
    default:
      LIBRDF_DEBUG2("Unknown flags %d\n", flags);
      return NULL;
  }
}


static void
librdf_storage_fxdb_find_statements_in_context_finished(void* context)
{
  librdf_storage_fxdb_sos_context* sos=(librdf_storage_fxdb_sos_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(context, void);

	if (sos->sd)
		sos->rdf_storage->ResultsClear(sos->sd);

  if(sos->current_statement)
    librdf_free_statement(sos->current_statement);

  if(sos->current_context)
    librdf_free_node(sos->current_context);

  if(sos->query_statement)
    librdf_free_statement(sos->query_statement);

  if(sos->query_context)
    librdf_free_node(sos->query_context);

  if(sos->storage)
    librdf_storage_remove_reference(sos->storage);

  LIBRDF_FREE(librdf_storage_fxdb_sos_context, sos);

}


/*
 * librdf_storage_fxdb_get_contexts:
 * @storage: the storage
 *
 * INTERNAL - Return an iterator with the context nodes present in storage.
 *
 * Return value: a #librdf_iterator or NULL on failure
 **/
static librdf_iterator*
librdf_storage_fxdb_get_contexts(librdf_storage* storage)
{
  librdf_storage_fxdb_instance* context=(librdf_storage_fxdb_instance*)storage->instance;
  librdf_storage_fxdb_get_contexts_context* gccontext;
  librdf_iterator *iterator;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  /* Initialize get_contexts context */
  gccontext = LIBRDF_CALLOC(librdf_storage_fxdb_get_contexts_context*, 1,
                            sizeof(*gccontext));
  if(!gccontext)
    return NULL;
  gccontext->storage=storage;
	gccontext->rdf_storage = context->storage;
  librdf_storage_add_reference(gccontext->storage);

  gccontext->current_context=NULL;
	gccontext->sd = context->storage->StatementContextSearch(0);

  if (!gccontext->sd) {
		librdf_log(gccontext->storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
			"contexts search failed.");
		librdf_storage_fxdb_get_contexts_finished((void*)gccontext);
		return NULL;
  }

  /* Get first context, if any, and initialize iterator */
  if(librdf_storage_fxdb_get_contexts_next_context(gccontext) ||
      !gccontext->current_context) {
    librdf_storage_fxdb_get_contexts_finished((void*)gccontext);
    return librdf_new_empty_iterator(storage->world);
  }

  iterator=librdf_new_iterator(storage->world,(void*)gccontext,
                               &librdf_storage_fxdb_get_contexts_end_of_iterator,
                               &librdf_storage_fxdb_get_contexts_next_context,
                               &librdf_storage_fxdb_get_contexts_get_context,
                               &librdf_storage_fxdb_get_contexts_finished);
  if(!iterator)
    librdf_storage_fxdb_get_contexts_finished(gccontext);
  return iterator;
}


static int
librdf_storage_fxdb_get_contexts_end_of_iterator(void* context)
{
  librdf_storage_fxdb_get_contexts_context* gccontext=(librdf_storage_fxdb_get_contexts_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, 1);

  return gccontext->current_context==NULL;
}


static int
librdf_storage_fxdb_get_contexts_next_context(void* context)
{
  librdf_storage_fxdb_get_contexts_context* gccontext=(librdf_storage_fxdb_get_contexts_context*)context;
  librdf_node *node;
	RDF_Statements statement_store = NULL;
	int type;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, 1);

  if( (statement_store = gccontext->rdf_storage->StatementGetNext(gccontext->sd)) != NULL ) {
    /* Free old context node, if allocated */
    if(gccontext->current_context)
      librdf_free_node(gccontext->current_context);
    /* Resource, Bnode or Literal? */
		type = statement_store->ctxt->type;
    if(type == RDF_TYPE_RESOURCE) {
      if(!(node=librdf_new_node_from_uri_string(gccontext->storage->world,
                                                 (const unsigned char*)statement_store->ctxt->data.resource.uri)))
			{
				gccontext->rdf_storage->StetementFree(statement_store);
				return 1;
			}
     } else if(type == RDF_TYPE_BNODES) {
      if(!(node=librdf_new_node_from_blank_identifier(gccontext->storage->world,
                                                       (const unsigned char*)statement_store->ctxt->data.bnode.name)))
			{
				gccontext->rdf_storage->StetementFree(statement_store);
        return 1;
			}
    } else if(type == RDF_TYPE_LITERAL) {
      /* Typed literal? */
      librdf_uri *datatype=NULL;
      if(statement_store->ctxt->data.literal.data_type_lens > 0)
        datatype=librdf_new_uri(gccontext->storage->world,
                                (const unsigned char*)statement_store->ctxt->data.literal.data_type);
      if(!(node=librdf_new_node_from_typed_literal(gccontext->storage->world,
                                                    (unsigned char *)statement_store->ctxt->data.literal.values,
                                                    statement_store->ctxt->data.literal.language,
                                                    datatype)))
			{
				gccontext->rdf_storage->StetementFree(statement_store);
        return 1;
			}
    } else
		{
			gccontext->rdf_storage->StetementFree(statement_store);
			return 1;
		}

		gccontext->rdf_storage->StetementFree(statement_store);
    gccontext->current_context=node;
  } else {
    if(gccontext->current_context)
      librdf_free_node(gccontext->current_context);
    gccontext->current_context=NULL;
  }

  return 0;
}


static void*
librdf_storage_fxdb_get_contexts_get_context(void* context, int flags)
{
  librdf_storage_fxdb_get_contexts_context* gccontext=(librdf_storage_fxdb_get_contexts_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(context, void, NULL);

  return gccontext->current_context;
}


#if 0
/* FIXME: why is this not used ? */
static void
librdf_storage_postgresql_free_gccontext_row(void* context)
{
  librdf_storage_fxdb_get_contexts_context* gccontext=(librdf_storage_fxdb_get_contexts_context*)context;
}
#endif


static void
librdf_storage_fxdb_get_contexts_finished(void* context)
{
  librdf_storage_fxdb_get_contexts_context* gccontext=(librdf_storage_fxdb_get_contexts_context*)context;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(context, void);

  if(gccontext->sd)
    gccontext->rdf_storage->ResultsClear(gccontext->sd);

  if(gccontext->current_context)
    librdf_free_node(gccontext->current_context);

  if(gccontext->storage)
    librdf_storage_remove_reference(gccontext->storage);

  LIBRDF_FREE(librdf_storage_fxdb_get_contexts_context, gccontext);

}


/*
 * librdf_storage_fxdb_get_feature:
 * @storage: #librdf_storage object
 * @feature: #librdf_uri feature property
 *
 * INTERNAL - get the value of a storage feature
 *
 * Return value: #librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
static librdf_node*
librdf_storage_fxdb_get_feature(librdf_storage* storage, librdf_uri* feature)
{
  unsigned char *uri_string;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(feature, librdf_uri, NULL);

  if(!feature)
    return NULL;

  uri_string=librdf_uri_as_string(feature);
  if(!uri_string)
    return NULL;

  if(!strcmp((const char*)uri_string, (const char*)LIBRDF_MODEL_FEATURE_CONTEXTS)) {
    /* Always have contexts */
    static const unsigned char value[2]="0";

    return librdf_new_node_from_typed_literal(storage->world,
                                              value,
                                              NULL, NULL);
  }

  return NULL;
}




/*
 * librdf_storage_fxdb_transaction_start:
 * @storage: the storage object
 *
 * INTERNAL - Start a transaction
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_fxdb_transaction_start(librdf_storage* storage)
{
  librdf_storage_fxdb_instance *context=(librdf_storage_fxdb_instance*)storage->instance;
  int status = 1;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  if(context->transaction_handle) {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "transaction already started");
    return status;
  }

  if (context->storage->NewXactStart()) {
		status = 0;
		context->transaction_handle = (int *)LIBRDF_CALLOC(int *, 1, sizeof(int));
  } else {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "Transaction start failed");
  }

  if (0 != status) {
		if (context->transaction_handle)
			LIBRDF_FREE(int *, context->transaction_handle);
    context->transaction_handle = NULL;
  }

  return status;
}


/*
 * librdf_storage_fxdb_transaction_start_with_handle:
 * @storage: the storage object
 * @handle: the transaction object
 *
 * INTERNAL - Start a transaction using an existing external transaction object.
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_fxdb_transaction_start_with_handle(librdf_storage* storage,
                                                   void* handle)
{
  return librdf_storage_fxdb_transaction_start(storage);
}


/*
 * librdf_storage_fxdb_transaction_commit:
 * @storage: the storage object
 *
 * INTERNAL - Commit a transaction.
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_fxdb_transaction_commit(librdf_storage* storage)
{
  librdf_storage_fxdb_instance *context=(librdf_storage_fxdb_instance*)storage->instance;
  int status = 1;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  if(!context->transaction_handle)
    return status;

  if (context->storage->NewXactCommit()) {
		status = 0;
  } else {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "Transaction commit failed.");
  }

	if (context->transaction_handle)
		LIBRDF_FREE(int *, context->transaction_handle);
  context->transaction_handle = NULL;

  return status;
}


/*
 * librdf_storage_fxdb_transaction_rollback:
 * @storage: the storage object
 *
 * INTERNAL - Rollback a transaction.
 *
 * Return value: non-0 on failure
 **/
static int
librdf_storage_fxdb_transaction_rollback(librdf_storage* storage)
{
  librdf_storage_fxdb_instance *context=(librdf_storage_fxdb_instance*)storage->instance;
  int status = 1;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, 1);

  if(!context->transaction_handle)
    return status;

  if (context->storage->NewXactAbort()) {
		status = 0;
  } else {
    librdf_log(storage->world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
               "Transaction abort failed.");
  }

	if (context->transaction_handle)
		LIBRDF_FREE(int *, context->transaction_handle);
  context->transaction_handle = NULL;

  return status;
}


/*
 * librdf_storage_fxdb_transaction_get_handle:
 * @storage: the storage object
 *
 * INTERNAL - Get the current transaction handle.
 *
 * Return value: non-0 on success
 **/
static void*
librdf_storage_fxdb_transaction_get_handle(librdf_storage* storage)
{
  librdf_storage_fxdb_instance *context=(librdf_storage_fxdb_instance*)storage->instance;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(storage, librdf_storage, NULL);

  return context->transaction_handle;
}

void*
librdf_storage_fxdb_get_all_graph(unsigned int *count)
{
	return (void *)RDFStorage::GetAllGraph(count);
}

/*
 * librdf_storage_fxdb_drop:
 * @storage: the storage object
 *
 * 删除一个storage
 *
 * 删除失败返回非0值
 **/
static int
librdf_storage_fxdb_drop(librdf_storage *storage)
{
	int stuta = 0;
	librdf_storage_fxdb_instance* context=(librdf_storage_fxdb_instance*)storage->instance;
	if (!context->storage->DropStorage())
		stuta = -1;

	return stuta;
}

/*
 * librdf_storage_set_multi_insert_prepare:
 * @storage: the storage object
 *
 * 设置变量为批量插入做准备
 **/
static void
librdf_storage_set_multi_insert_prepare(librdf_storage *storage)
{
	librdf_storage_fxdb_instance* context=(librdf_storage_fxdb_instance*)storage->instance;
	context->storage->prepareExecMultiInsert();
}

/*
 * librdf_storage_set_multi_insert_exec:
 * @storage: the storage object
 *
 * 设置变量执行批量插入
 **/
static void
librdf_storage_set_multi_insert_exec(librdf_storage *storage)
{
	librdf_storage_fxdb_instance* context=(librdf_storage_fxdb_instance*)storage->instance;
	context->storage->setExecMultiInsert();
}

/*
 * librdf_storage_counter_remove_rows:
 * @storage: the storage object
 *
 * 计算删除的statement条数
 **/
static unsigned int
librdf_storage_counter_remove_rows(librdf_storage *storage)
{
	librdf_storage_fxdb_instance* context=(librdf_storage_fxdb_instance*)storage->instance;
	return context->storage->CounterRemoveNum();
}

/* local function to register fxdb storage functions */
static void
librdf_storage_fxdb_register_factory(librdf_storage_factory *factory)
{
  LIBRDF_ASSERT_CONDITION(!strcmp(factory->name, "fxdb"));

  factory->version            = LIBRDF_STORAGE_INTERFACE_VERSION;
  factory->init               = librdf_storage_fxdb_init;
  factory->terminate          = librdf_storage_fxdb_terminate;
  factory->open               = librdf_storage_fxdb_open;
  factory->close              = librdf_storage_fxdb_close;
  factory->sync               = librdf_storage_fxdb_sync;
  factory->size               = librdf_storage_fxdb_size;
  factory->add_statement      = librdf_storage_fxdb_add_statement;
  factory->add_statements     = librdf_storage_fxdb_add_statements;
  factory->remove_statement   = librdf_storage_fxdb_remove_statement;
  factory->contains_statement = librdf_storage_fxdb_contains_statement;
  factory->serialise          = librdf_storage_fxdb_serialise;
  factory->find_statements    = librdf_storage_fxdb_find_statements;
  factory->find_statements_with_options    = librdf_storage_fxdb_find_statements_with_options;
  factory->context_add_statement      = librdf_storage_fxdb_context_add_statement;
  factory->context_add_statements     = librdf_storage_fxdb_context_add_statements;
  factory->context_remove_statement   = librdf_storage_fxdb_context_remove_statement;
  factory->context_remove_statements  = librdf_storage_fxdb_context_remove_statements;
  factory->context_serialise          = librdf_storage_fxdb_context_serialise;
  factory->find_statements_in_context = librdf_storage_fxdb_find_statements_in_context;
  factory->get_contexts               = librdf_storage_fxdb_get_contexts;
  factory->get_feature                = librdf_storage_fxdb_get_feature;

  factory->transaction_start             = librdf_storage_fxdb_transaction_start;
  factory->transaction_start_with_handle = librdf_storage_fxdb_transaction_start_with_handle;
  factory->transaction_commit            = librdf_storage_fxdb_transaction_commit;
  factory->transaction_rollback          = librdf_storage_fxdb_transaction_rollback;
  factory->transaction_get_handle        = librdf_storage_fxdb_transaction_get_handle;
	factory->drop_storage									 = librdf_storage_fxdb_drop;
	factory->set_multi_insert_prepare			 = librdf_storage_set_multi_insert_prepare;
	factory->set_multi_insert_execute			 = librdf_storage_set_multi_insert_exec;
	factory->counter_remove_rows					 = librdf_storage_counter_remove_rows;
}

#ifdef MODULAR_LIBRDF

/* Entry point for dynamically loaded storage module */
void
librdf_storage_module_register_factory(librdf_world *world)
{
  librdf_storage_register_factory(world, "postgresql",
                                  "PostgreSQL database store",
                                  &librdf_storage_fxdb_register_factory);
}

#else

/*
 * librdf_init_storage_fxdb:
 * @world: world object
 *
 * INTERNAL - Initialise the built-in storage_fxdb module.
 */
void
librdf_init_storage_fxdb(librdf_world *world)
{
  librdf_storage_register_factory(world, "fxdb", 
                                  "fxdb database store",
                                  &librdf_storage_fxdb_register_factory);
}

#endif
