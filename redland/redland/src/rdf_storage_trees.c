/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_trees.c - RDF Storage in memory using balanced trees
 *
 * Copyright (C) 2000-2008, David Beckett http://www.dajobe.org/
 * Copyright (C) 2000-2004, University of Bristol, UK http://www.bristol.ac.uk/
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


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif



#include <stdio.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
/* for ptrdiff_t */
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
#include <sys/types.h>

#include <redland.h>

/* Not yet fully implemented (namely iteration) */
/*#define RDF_STORAGE_TREES_WITH_CONTEXTS 1*/

typedef struct
{
#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
  librdf_node* context;
#endif
  raptor_avltree* spo_tree; /* Always present */
  raptor_avltree* sop_tree; /* Optional */
  raptor_avltree* ops_tree; /* Optional */
  raptor_avltree* pso_tree; /* Optional */
} librdf_storage_trees_graph;

typedef struct
{
  librdf_storage_trees_graph* graph; /* Statements without a context */
#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
  raptor_avltree* contexts; /* Tree of librdf_storage_trees_graph */
#endif
  int index_sop;
  int index_ops;
  int index_pso;
} librdf_storage_trees_instance;

/* prototypes for local functions */
static int librdf_storage_trees_init(librdf_storage* storage, const char *name, librdf_hash* options);
static int librdf_storage_trees_open(librdf_storage* storage, librdf_model* model);
static int librdf_storage_trees_close(librdf_storage* storage);
static int librdf_storage_trees_size(librdf_storage* storage);
static int librdf_storage_trees_add_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_trees_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
static int librdf_storage_trees_remove_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_trees_remove_statement_internal(librdf_storage_trees_graph* graph, librdf_statement* statement);
static int librdf_storage_trees_contains_statement(librdf_storage* storage, librdf_statement* statement);
static librdf_stream* librdf_storage_trees_serialise(librdf_storage* storage);
static librdf_stream* librdf_storage_trees_find_statements(librdf_storage* storage, librdf_statement* statement);

/* graph functions */
static librdf_storage_trees_graph* librdf_storage_trees_graph_new(librdf_storage* storage, librdf_node* context);
static void librdf_storage_trees_graph_free(void* data);
#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
static int librdf_storage_trees_graph_compare(const void* data1, const void* data2);
#endif

/* serialising implementing functions */
static int librdf_storage_trees_serialise_end_of_stream(void* context);
static int librdf_storage_trees_serialise_next_statement(void* context);
static void* librdf_storage_trees_serialise_get_statement(void* context, int flags);
static void librdf_storage_trees_serialise_finished(void* context);

/* context functions */
#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
static int librdf_storage_trees_context_add_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_trees_context_remove_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static librdf_stream* librdf_storage_trees_context_serialise(librdf_storage* storage, librdf_node* context_node);
#endif

/* statement tree functions */
static int librdf_statement_compare_spo(const void* data1, const void* data2);
static int librdf_statement_compare_sop(const void* data1, const void* data2);
static int librdf_statement_compare_ops(const void* data1, const void* data2);
static int librdf_statement_compare_pso(const void* data1, const void* data2);
static void librdf_storage_trees_avl_free(void* data);


static void librdf_storage_trees_register_factory(librdf_storage_factory *factory);


/* functions implementing storage api */
static int
librdf_storage_trees_init(librdf_storage* storage, const char *name,
                         librdf_hash* options)
{
  const int index_spo_option = librdf_hash_get_as_boolean(options, "index-spo") > 0;
  const int index_sop_option = librdf_hash_get_as_boolean(options, "index-sop") > 0;
  const int index_ops_option = librdf_hash_get_as_boolean(options, "index-ops") > 0;
  const int index_pso_option = librdf_hash_get_as_boolean(options, "index-pso") > 0;

  librdf_storage_trees_instance* context;

  context = LIBRDF_CALLOC(librdf_storage_trees_instance*, 1, sizeof(*context));
  if(!context) {
    if(options)
      librdf_free_hash(options);
    return 1;
  }

  librdf_storage_set_instance(storage, context);

#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
  /* Support contexts if option given */
  if (librdf_hash_get_as_boolean(options, "contexts") > 0) {
    context->contexts=librdf_new_avltree(librdf_storage_trees_graph_compare,
      librdf_storage_trees_graph_free);
  } else {
    context->contexts=NULL;
  }
#endif

  /* No indexing options given, index all by default */
  if (!index_spo_option && !index_sop_option && !index_ops_option && !index_pso_option) {
    context->index_sop=1;
    context->index_ops=1;
    context->index_pso=1;
  } else {
    /* spo is always indexed, option just exists so user can
     * specifically /only/ index spo */
    context->index_sop=index_sop_option;
    context->index_ops=index_ops_option;
    context->index_pso=index_pso_option;
  }
  
  context->graph = librdf_storage_trees_graph_new(storage, NULL);
  
  /* no more options, might as well free them now */
  if(options)
    librdf_free_hash(options);

  return 0;
}


static void
librdf_storage_trees_terminate(librdf_storage* storage)
{
  if (storage->instance != NULL)
    LIBRDF_FREE(librdf_storage_trees_instance, storage->instance);
}


static int
librdf_storage_trees_open(librdf_storage* storage, librdf_model* model)
{
  /* nop */
  return 0;
}


/**
 * librdf_storage_trees_close:
 * @storage: the storage
 *
 * .
 * 
 * Close the storage, and free all content since there is no persistance.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_trees_close(librdf_storage* storage)
{
  librdf_storage_trees_instance* context=(librdf_storage_trees_instance*)storage->instance;
  
  librdf_storage_trees_graph_free(context->graph);
  context->graph=NULL;
  
#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
  librdf_free_avltree(context->contexts);
  context->contexts=NULL;
#endif
  
  return 0;
}


static int
librdf_storage_trees_size(librdf_storage* storage)
{
  librdf_storage_trees_instance* context=(librdf_storage_trees_instance*)storage->instance;

  return raptor_avltree_size(context->graph->spo_tree);
}


static int
librdf_storage_trees_add_statement_internal(librdf_storage* storage,
                                            librdf_storage_trees_graph* graph,
                                            librdf_statement* statement) 
{
  librdf_storage_trees_instance* context=(librdf_storage_trees_instance*)storage->instance;
  int status = 0;
  
  /* copy statement (store single copy in all trees) */
  statement = librdf_new_statement_from_statement(statement);
    
  /* spo_tree owns statement */
  status = raptor_avltree_add(graph->spo_tree, statement);
  if (status > 0) /* item already exists; old item remains in tree */
    return 0;
  else if (status < 0) /* failure */
    return status;
    
  /* others have null deleters */
  /* (XXX: corrupt model if insertions fail) */

  if (context->index_sop)
    raptor_avltree_add(graph->sop_tree, statement);
    
  if (context->index_ops)
    raptor_avltree_add(graph->ops_tree, statement);
    
  if (context->index_pso)
    raptor_avltree_add(graph->pso_tree, statement);
    
  return status;
}


/**
 * librdf_storage_trees_add_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to add
 *
 * Add a statement (with no context) to the storage.
 * 
 * Return value: non 0 on failure (negative if error, positive if statement
 * already exists).
 **/
static int
librdf_storage_trees_add_statement(librdf_storage* storage,
                                   librdf_statement* statement) 
{
  librdf_storage_trees_instance* context=(librdf_storage_trees_instance*)storage->instance;
  return librdf_storage_trees_add_statement_internal(storage, context->graph, statement);
}


static int
librdf_storage_trees_add_statements(librdf_storage* storage,
                                    librdf_stream* statement_stream)
{
  int status=0;

  for(; !librdf_stream_end(statement_stream); librdf_stream_next(statement_stream)) {
    librdf_statement* statement=librdf_stream_get_object(statement_stream);

    if (statement) {
      status=librdf_storage_trees_add_statement(storage, statement);
      if (status)
        break;
    } else {
      status=1;
      break;
    }
  }
  
  return status;
}

static int
librdf_storage_trees_remove_statement_internal(librdf_storage_trees_graph* graph,
                                               librdf_statement* statement) 
{
  if (graph->sop_tree)
    raptor_avltree_delete(graph->sop_tree, statement);

  if (graph->ops_tree)
    raptor_avltree_delete(graph->ops_tree, statement);

  if (graph->pso_tree)
    raptor_avltree_delete(graph->pso_tree, statement);
  
  raptor_avltree_delete(graph->spo_tree, statement);
  
  return 0;
}


/**
 * librdf_storage_trees_remove_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to remove
 *
 * Remove a statement (without context) from the storage.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_trees_remove_statement(librdf_storage* storage, 
                                      librdf_statement* statement) 
{
  librdf_storage_trees_instance* context=(librdf_storage_trees_instance*)storage->instance;

  return librdf_storage_trees_remove_statement_internal(context->graph, statement);
}

static int
librdf_storage_trees_contains_statement(librdf_storage* storage, librdf_statement* statement)
{
  librdf_storage_trees_instance* context=(librdf_storage_trees_instance*)storage->instance;

  return (raptor_avltree_search(context->graph->spo_tree, statement) != NULL);
}


typedef struct {
  librdf_storage *storage;
  raptor_avltree_iterator *avltree_iterator;
#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
  librdf_node *context_node;
#endif
} librdf_storage_trees_serialise_stream_context;


static librdf_stream*
librdf_storage_trees_serialise_range(librdf_storage* storage, librdf_statement* range)
{
  librdf_storage_trees_instance* context=(librdf_storage_trees_instance*)storage->instance;
  librdf_storage_trees_serialise_stream_context* scontext;
  librdf_stream* stream;
  int filter = 0;
  
  scontext = LIBRDF_CALLOC(librdf_storage_trees_serialise_stream_context*, 1,
                           sizeof(*scontext));
  if(!scontext)
    return NULL;
    
  scontext->avltree_iterator = NULL;

  /* ?s ?p ?o */
  if (!range || (!range->subject && !range->predicate && !range->object)) {
    scontext->avltree_iterator = raptor_new_avltree_iterator(context->graph->spo_tree,
                                                             /* range */ NULL,
                                                             /* range free */ NULL,
                                                             1);
    if (range) {
      librdf_free_statement(range);
      range=NULL;
    }
  /* s ?p o */
  } else if (range->subject && !range->predicate && range->object) {
    if (context->index_sop)
      scontext->avltree_iterator = raptor_new_avltree_iterator(context->graph->sop_tree,
                                                               range, 
                                                               librdf_storage_trees_avl_free,
                                                               1);
	else
		filter=1;
  /* s _ _ */
  } else if (range->subject) {
    scontext->avltree_iterator = raptor_new_avltree_iterator(context->graph->spo_tree,
                                                             range,
                                                             librdf_storage_trees_avl_free,
                                                             1);
  /* ?s _ o */
  } else if (range->object) {
    if (context->index_ops)
      scontext->avltree_iterator = raptor_new_avltree_iterator(context->graph->ops_tree,
                                                               range,
                                                               librdf_storage_trees_avl_free,
                                                               1);
	else
		filter=1;
  /* ?s p ?o */
  } else { /* range->predicate != NULL */
    if (context->index_pso)
      scontext->avltree_iterator = raptor_new_avltree_iterator(context->graph->pso_tree,
                                                               range,
                                                               librdf_storage_trees_avl_free,
                                                               1);
	else
		filter=1;
  }
    
  /* If filter is set, we're missing the required index.
   * Iterate over the entire model and filter the stream.
   * (With a fully indexed store, this will never happen) */
  if (filter) {
    scontext->avltree_iterator = raptor_new_avltree_iterator(context->graph->spo_tree,
                                                     range,
                                                     librdf_storage_trees_avl_free,
                                                     1);
  }

#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
  scontext->context_node=NULL;
#endif

  if(!scontext->avltree_iterator) {
    LIBRDF_FREE(librdf_storage_trees_serialise_stream_context, scontext);
    return librdf_new_empty_stream(storage->world);
  }
  
  scontext->storage=storage;
  librdf_storage_add_reference(scontext->storage);

  stream=librdf_new_stream(storage->world,
                           (void*)scontext,
                           &librdf_storage_trees_serialise_end_of_stream,
                           &librdf_storage_trees_serialise_next_statement,
                           &librdf_storage_trees_serialise_get_statement,
                           &librdf_storage_trees_serialise_finished);
  
  if(!stream) {
    librdf_storage_trees_serialise_finished((void*)scontext);
    return NULL;
  }

  if(filter) {
    if(librdf_stream_add_map(stream, &librdf_stream_statement_find_map, NULL, (void*)range)) {
      /* error - stream_add_map failed */
      librdf_free_stream(stream);
      stream=NULL;
    }
  }
  
  return stream;  
}


static librdf_stream*
librdf_storage_trees_serialise(librdf_storage* storage)
{
  return librdf_storage_trees_serialise_range(storage, NULL);
}


static int
librdf_storage_trees_serialise_end_of_stream(void* context)
{
  librdf_storage_trees_serialise_stream_context* scontext=(librdf_storage_trees_serialise_stream_context*)context;

  return raptor_avltree_iterator_is_end(scontext->avltree_iterator);
}

static int
librdf_storage_trees_serialise_next_statement(void* context)
{
  librdf_storage_trees_serialise_stream_context* scontext=(librdf_storage_trees_serialise_stream_context*)context;

  return raptor_avltree_iterator_next(scontext->avltree_iterator);
}


static void*
librdf_storage_trees_serialise_get_statement(void* context, int flags)
{
  librdf_storage_trees_serialise_stream_context* scontext=(librdf_storage_trees_serialise_stream_context*)context;

  if(!scontext->avltree_iterator)
    return NULL;

  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
      return (librdf_statement*)raptor_avltree_iterator_get(scontext->avltree_iterator);

#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return scontext->context_node;
#endif

    default:
      return NULL;
  }
}


static void
librdf_storage_trees_serialise_finished(void* context)
{
  librdf_storage_trees_serialise_stream_context* scontext=(librdf_storage_trees_serialise_stream_context*)context;

  if(scontext->avltree_iterator)
    raptor_free_avltree_iterator(scontext->avltree_iterator);

  if(scontext->storage)
    librdf_storage_remove_reference(scontext->storage);
  
  LIBRDF_FREE(librdf_storage_trees_serialise_stream_context, scontext);
}


#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
/**
 * librdf_storage_trees_context_add_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to add
 *
 * Add a statement to a storage context.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_trees_context_add_statement(librdf_storage* storage,
                                           librdf_node* context_node,
                                           librdf_statement* statement) 
{
  librdf_storage_trees_instance* context=(librdf_storage_trees_instance*)storage->instance;
  
  librdf_storage_trees_graph* key=librdf_storage_trees_graph_new(storage, context_node);
  librdf_storage_trees_graph* graph=(librdf_storage_trees_graph*)
    raptor_avltree_search(context->contexts, key);
  
  if(graph) {
    librdf_storage_trees_graph_free(key);
  } else {
    raptor_avltree_add(context->contexts, key);
    graph=key;
  }
    
  return librdf_storage_trees_add_statement_internal(storage, graph, statement);
}


/**
 * librdf_storage_trees_context_remove_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to remove
 *
 * Remove a statement from a storage context.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_trees_context_remove_statement(librdf_storage* storage, 
                                              librdf_node* context_node,
                                              librdf_statement* statement) 
{
  librdf_storage_trees_instance* context=(librdf_storage_trees_instance*)storage->instance;
  librdf_storage_trees_graph* key=librdf_storage_trees_graph_new(storage, context_node);
  librdf_storage_trees_graph* graph=(librdf_storage_trees_graph*)
    raptor_avltree_search(context->contexts, &key);
  librdf_storage_trees_graph_free(key);
  if (graph) {
    return librdf_storage_trees_remove_statement_internal(graph, statement);
  } else {
    return -1;
  }
}


/**
 * librdf_storage_trees_context_serialise:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 *
 * List all statements in a storage context.
 * 
 * Return value: #librdf_stream of statements or NULL on failure or context is empty
 **/
static librdf_stream*
librdf_storage_trees_context_serialise(librdf_storage* storage,
                                        librdf_node* context_node) 
{
  return NULL;
}


/**
 * librdf_storage_trees_context_get_contexts:
 * @storage: #librdf_storage object
 *
 * List all context nodes in a storage.
 * 
 * Return value: #librdf_iterator of context_nodes or NULL on failure or no contexts
 **/
static librdf_iterator*
librdf_storage_trees_get_contexts(librdf_storage* storage) 
{
  return NULL;
}
#endif


/**
 * librdf_storage_trees_find_statements:
 * @storage: the storage
 * @statement: the statement to match
 *
 * .
 * 
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 * Uses #librdf_statement_match to do the matching.
 * 
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_trees_find_statements(librdf_storage* storage, librdf_statement* statement)
{
  librdf_stream* stream;

  librdf_statement* range=librdf_new_statement_from_statement(statement);
  if(!range)
    return NULL;

  stream=librdf_storage_trees_serialise_range(storage, range);

  return stream;
}

/* statement tree functions */

static int
librdf_storage_trees_node_compare(librdf_node* n1, librdf_node* n2)
{
  if (n1 == n2) {
    return 0;
  } else if (n1->type != n2->type) {
    return LIBRDF_GOOD_CAST(int, n2->type) - LIBRDF_GOOD_CAST(int, n1->type);
  } else {
    switch (n1->type) {
      case RAPTOR_TERM_TYPE_URI:
        return librdf_uri_compare(n1->value.uri, n2->value.uri);

      case RAPTOR_TERM_TYPE_LITERAL:
        if(1) {
          const unsigned char l1 = n1->value.literal.language_len;
          const unsigned char l2 = n2->value.literal.language_len;
          const unsigned char l  = (l1 < l2) ? l1 : l2;

          /* compare first by data type */
          int r = raptor_uri_compare(n1->value.literal.datatype,
                                     n2->value.literal.datatype);
          if(r)
            return r;

          /* if data type is equal, compare by value */
          r = strcmp((const char*)n1->value.literal.string,
                     (const char*)n2->value.literal.string);
          if(r)
            return r;

          /* if both data type and value are equal, compare by language */
          if(l) {
            return strncmp((const char*)n1->value.literal.language,
                           (const char*)n2->value.literal.language, (size_t)l);
          } else {
            /* if l == 0 strncmp will always return 0; in that case
             * consider the node with no language to be lesser. */
            return l1 - l2;
          }
        }
      case RAPTOR_TERM_TYPE_BLANK:
        return strcmp((char*)n1->value.blank.string,
                      (char*)n2->value.blank.string);

      case RAPTOR_TERM_TYPE_UNKNOWN:
      default:
        if(1) {
          ptrdiff_t d;
          d = LIBRDF_GOOD_CAST(char*, n2) - LIBRDF_GOOD_CAST(char*, n1);
          return (d > 0) - (d < 0);
        }
    }
  }
}



/* Compare two statements in (s, p, o) order.
 * NULL fields act as wildcards. */
static int
librdf_statement_compare_spo(const void* data1, const void* data2)
{
  librdf_statement* a = (librdf_statement*)data1;
  librdf_statement* b = (librdf_statement*)data2;
  int cmp = 0;

  /* Subject */
  if (a->subject == NULL || b->subject == NULL)
    return 0; /* wildcard subject match */
  else
    cmp = librdf_storage_trees_node_compare(a->subject, b->subject);

  if (cmp != 0)
    return cmp;

  /* Predicate */
  if (a->predicate == NULL || b->predicate == NULL)
    return 0; /* wildcard predicate match */
  else
    cmp = librdf_storage_trees_node_compare(a->predicate, b->predicate);

  if (cmp != 0)
    return cmp;

  /* Object */
  if (a->object == NULL || b->object == NULL)
    return 0; /* wildcard object match */
  else
    cmp = librdf_storage_trees_node_compare(a->object, b->object);

  return cmp;
}


/* Compare two statements in (o, s, p) order.
 * NULL fields act as wildcards. */
static int
librdf_statement_compare_sop(const void* data1, const void* data2)
{
  librdf_statement* a = (librdf_statement*)data1;
  librdf_statement* b = (librdf_statement*)data2;
  int cmp = 0;

  /* Subject */
  if (a->subject == NULL || b->subject == NULL)
    return 0; /* wildcard subject match */
  else
    cmp = librdf_storage_trees_node_compare(a->subject, b->subject);

  if (cmp != 0)
    return cmp;

  /* Object */
  if (a->object == NULL || b->object == NULL)
    return 0; /* wildcard object match */
  else
    cmp = librdf_storage_trees_node_compare(a->object, b->object);

  if (cmp != 0)
    return cmp;

  /* Predicate */
  if (a->predicate == NULL || b->predicate == NULL)
    return 0; /* wildcard predicate match */
  else
    cmp = librdf_storage_trees_node_compare(a->predicate, b->predicate);

  return cmp;
}


/* Compare two statements in (o, p, s) order.
 * NULL fields act as wildcards. */
static int
librdf_statement_compare_ops(const void* data1, const void* data2)
{
  librdf_statement* a = (librdf_statement*)data1;
  librdf_statement* b = (librdf_statement*)data2;
  int cmp = 0;

  /* Object */
  if (a->object == NULL || b->object == NULL)
    return 0; /* wildcard object match */
  else
    cmp = librdf_storage_trees_node_compare(a->object, b->object);
  
  if (cmp != 0)
    return cmp;

  /* Predicate */
  if (a->predicate == NULL || b->predicate == NULL)
    return 0; /* wildcard predicate match */
  else
    cmp = librdf_storage_trees_node_compare(a->predicate, b->predicate);

  if (cmp != 0)
    return cmp;
  
  /* Subject */
  if (a->subject == NULL || b->subject == NULL)
    return 0; /* wildcard subject match */
  else
    cmp = librdf_storage_trees_node_compare(a->subject, b->subject);

  return cmp;
}


/* Compare two statements in (p, s, o) order.
 * NULL fields act as wildcards. */
static int
librdf_statement_compare_pso(const void* data1, const void* data2)
{
  librdf_statement* a = (librdf_statement*)data1;
  librdf_statement* b = (librdf_statement*)data2;
  int cmp = 0;

  /* Predicate */
  if (a->predicate == NULL || b->predicate == NULL)
    return 0; /* wildcard predicate match */
  else
    cmp = librdf_storage_trees_node_compare(a->predicate, b->predicate);

  if (cmp != 0)
    return cmp;
  
  /* Subject */
  if (a->subject == NULL || b->subject == NULL)
    return 0; /* wildcard subject match */
  else
    cmp = librdf_storage_trees_node_compare(a->subject, b->subject);

  if (cmp != 0)
    return cmp;
  
  /* Object */
  if (a->object == NULL || b->object == NULL)
    return 0; /* wildcard object match */
  else
    cmp = librdf_storage_trees_node_compare(a->object, b->object);
  
  return cmp;
}


static void
librdf_storage_trees_avl_free(void* data)
{
  librdf_statement* stmnt=(librdf_statement*)data;
  librdf_free_statement(stmnt);
}


/* graph functions */

static librdf_storage_trees_graph*
librdf_storage_trees_graph_new(librdf_storage* storage, librdf_node* context_node)
{
  librdf_storage_trees_instance* context=(librdf_storage_trees_instance*)storage->instance;
  librdf_storage_trees_graph* graph;

  graph = LIBRDF_MALLOC(librdf_storage_trees_graph*, sizeof(*graph));
  
#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
  graph->context=(context_node ? librdf_new_node_from_node(context_node) : NULL);
#endif

  /* Always create SPO index */
  graph->spo_tree = raptor_new_avltree(librdf_statement_compare_spo,
                                       librdf_storage_trees_avl_free,
                                       /* flags */ 0);
  if(!graph->spo_tree) {
    LIBRDF_FREE(librdf_storage_trees_graph, graph);
    return NULL;
  }
  
  if(context->index_sop)
    graph->sop_tree = raptor_new_avltree(librdf_statement_compare_sop, NULL,
                                         /* flags */ 0);
  else
    graph->sop_tree=NULL;

  if(context->index_ops)
    graph->ops_tree = raptor_new_avltree(librdf_statement_compare_ops, NULL,
                                         /* flags */ 0);
  else
    graph->ops_tree=NULL;
  
  if(context->index_pso)
    graph->pso_tree = raptor_new_avltree(librdf_statement_compare_pso, NULL,
                                         /* flags */ 0);
  else
    graph->pso_tree=NULL;

  return graph;
}


#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
static int
librdf_storage_trees_graph_compare(const void* data1, const void* data2)
{
  librdf_storage_trees_graph* a = (librdf_storage_trees_graph*)data1;
  librdf_storage_trees_graph* b = (librdf_storage_trees_graph*)data2;
  return librdf_storage_trees_node_compare(a->context, b->context);
}
#endif


static void
librdf_storage_trees_graph_free(void* data)
{
  librdf_storage_trees_graph* graph = (librdf_storage_trees_graph*)data;
  
#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
  librdf_free_node(graph->context);
#endif
  
  /* Extra index trees have null deleters (statements are shared) */
  if (graph->sop_tree)
    raptor_free_avltree(graph->sop_tree);
  if (graph->ops_tree)
    raptor_free_avltree(graph->ops_tree);
  if (graph->pso_tree)
    raptor_free_avltree(graph->pso_tree);

  /* Free spo tree and statements */
  raptor_free_avltree(graph->spo_tree);

  graph->spo_tree = NULL;
  graph->sop_tree = NULL;
  graph->ops_tree = NULL;
  graph->pso_tree = NULL;

  LIBRDF_FREE(librdf_storage_trees_graph, graph);
}


/**
 * librdf_storage_trees_get_feature:
 * @storage: #librdf_storage object
 * @feature: #librdf_uri feature property
 *
 * Get the value of a storage feature.
 * 
 * Return value: #librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
static librdf_node*
librdf_storage_trees_get_feature(librdf_storage* storage, librdf_uri* feature)
{
#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
  librdf_storage_trees_instance* scontext=(librdf_storage_trees_instance*)storage->instance;
  unsigned char *uri_string;

  if(!feature)
    return NULL;

  uri_string=librdf_uri_as_string(feature);
  if(!uri_string)
    return NULL;
  
  if(!strcmp((const char*)uri_string, LIBRDF_MODEL_FEATURE_CONTEXTS)) {
    unsigned char value[2];

    sprintf((char*)value, "%d", (scontext->contexts != NULL));
    return librdf_new_node_from_typed_literal(storage->world, 
                                              value, NULL, NULL);
  }
#endif

  return NULL;
}


/** Local entry point for dynamically loaded storage module */
static void
librdf_storage_trees_register_factory(librdf_storage_factory *factory) 
{
  LIBRDF_ASSERT_CONDITION(!strncmp(factory->name, "trees", 5));

  factory->version                  = LIBRDF_STORAGE_INTERFACE_VERSION;
  factory->init                     = librdf_storage_trees_init;
  factory->clone                    = NULL;
  factory->terminate                = librdf_storage_trees_terminate;
  factory->open                     = librdf_storage_trees_open;
  factory->close                    = librdf_storage_trees_close;
  factory->size                     = librdf_storage_trees_size;
  factory->add_statement            = librdf_storage_trees_add_statement;
  factory->add_statements           = librdf_storage_trees_add_statements;
  factory->remove_statement         = librdf_storage_trees_remove_statement;
  factory->contains_statement       = librdf_storage_trees_contains_statement;
  factory->serialise                = librdf_storage_trees_serialise;

  factory->find_statements          = librdf_storage_trees_find_statements;
  /* These could be implemented, but only if all indexes are available.
   * If they returned NULL if the indexes weren't available,
   * librdf_storage_find_statements would break, unfortunately.
   * Since these are exposed by model methods, the storage interface
   * needs to be fixed so these can be exposed but find_statements
   * still work */
  factory->find_sources             = NULL;
  factory->find_arcs                = NULL;
  factory->find_targets             = NULL;

#ifdef RDF_STORAGE_TREES_WITH_CONTEXTS
  factory->context_add_statement    = librdf_storage_trees_context_add_statement;
  factory->context_remove_statement = librdf_storage_trees_context_remove_statement;
  factory->context_serialise        = librdf_storage_trees_context_serialise;
  factory->get_contexts             = librdf_storage_trees_get_contexts;
#else
  factory->context_add_statement    = NULL;
  factory->context_remove_statement = NULL;
  factory->context_serialise        = NULL;
  factory->get_contexts             = NULL;
#endif

  factory->sync                     = NULL;
  factory->get_feature              = librdf_storage_trees_get_feature;
}


/*
 * librdf_init_storage_trees:
 * @world: world object
 *
 * INTERNAL - Initialise the built-in storage_trees module.
 */
void
librdf_init_storage_trees(librdf_world *world)
{
  librdf_storage_register_factory(world, "trees", "Balanced trees",
                                  &librdf_storage_trees_register_factory);
}
