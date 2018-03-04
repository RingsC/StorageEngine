/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     A = 258,
     HAT = 259,
     DOT = 260,
     COMMA = 261,
     SEMICOLON = 262,
     LEFT_SQUARE = 263,
     RIGHT_SQUARE = 264,
     LEFT_ROUND = 265,
     RIGHT_ROUND = 266,
     LEFT_CURLY = 267,
     RIGHT_CURLY = 268,
     TRUE_TOKEN = 269,
     FALSE_TOKEN = 270,
     PREFIX = 271,
     BASE = 272,
     SPARQL_PREFIX = 273,
     SPARQL_BASE = 274,
     STRING_LITERAL = 275,
     URI_LITERAL = 276,
     GRAPH_NAME_LEFT_CURLY = 277,
     BLANK_LITERAL = 278,
     QNAME_LITERAL = 279,
     IDENTIFIER = 280,
     LANGTAG = 281,
     INTEGER_LITERAL = 282,
     FLOATING_LITERAL = 283,
     DECIMAL_LITERAL = 284,
     ERROR_TOKEN = 285
   };
#endif
/* Tokens.  */
#define A 258
#define HAT 259
#define DOT 260
#define COMMA 261
#define SEMICOLON 262
#define LEFT_SQUARE 263
#define RIGHT_SQUARE 264
#define LEFT_ROUND 265
#define RIGHT_ROUND 266
#define LEFT_CURLY 267
#define RIGHT_CURLY 268
#define TRUE_TOKEN 269
#define FALSE_TOKEN 270
#define PREFIX 271
#define BASE 272
#define SPARQL_PREFIX 273
#define SPARQL_BASE 274
#define STRING_LITERAL 275
#define URI_LITERAL 276
#define GRAPH_NAME_LEFT_CURLY 277
#define BLANK_LITERAL 278
#define QNAME_LITERAL 279
#define IDENTIFIER 280
#define LANGTAG 281
#define INTEGER_LITERAL 282
#define FLOATING_LITERAL 283
#define DECIMAL_LITERAL 284
#define ERROR_TOKEN 285




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 119 "./turtle_parser.y"

  unsigned char *string;
  raptor_term *identifier;
  raptor_sequence *sequence;
  raptor_uri *uri;
  int integer; /* 0+ for a xsd:integer datatyped RDF literal */



/* Line 2068 of yacc.c  */
#line 120 "turtle_parser.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




