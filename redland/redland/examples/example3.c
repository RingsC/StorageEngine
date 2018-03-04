/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example3.c - Redland example code creating model, statement and storing it in 10 lines
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


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include <redland.h>

/* one prototype needed */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_world* world;
  librdf_storage *storage;
  librdf_model* model;
  librdf_statement* statement;
  raptor_world *raptor_world_ptr;
  raptor_iostream* iostr;
  
  world=librdf_new_world();
  librdf_world_open(world);
  raptor_world_ptr = librdf_world_get_raptor(world);

  model=librdf_new_model(world, storage=librdf_new_storage(world, "hashes", "test", "hash-type='bdb',dir='.'"), NULL);

  librdf_model_add_statement(model, 
                             statement=librdf_new_statement_from_nodes(world, librdf_new_node_from_uri_string(world, (const unsigned char*)"http://www.dajobe.org/"),
                                                             librdf_new_node_from_uri_string(world, (const unsigned char*)"http://purl.org/dc/elements/1.1/creator"),
                                                             librdf_new_node_from_literal(world, (const unsigned char*)"Dave Beckett", NULL, 0)
                                                             )
                             );

  librdf_free_statement(statement);

  iostr = raptor_new_iostream_to_file_handle(raptor_world_ptr, stdout);
  librdf_model_write(model, iostr);
  raptor_free_iostream(iostr);
  
  librdf_free_model(model);
  librdf_free_storage(storage);

  librdf_free_world(world);

#ifdef LIBRDF_MEMORY_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}
