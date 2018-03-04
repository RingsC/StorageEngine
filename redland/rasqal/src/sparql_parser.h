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

/* These are SPARQL token definitions */
#ifdef OPTIONAL
#undef OPTIONAL
#endif

#ifdef DELETE
#undef DELETE
#endif

#ifdef IN
#undef IN
#endif

#ifdef GROUP
#undef GROUP
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     SELECT = 258,
     FROM = 259,
     WHERE = 260,
     OPTIONAL = 261,
     DESCRIBE = 262,
     CONSTRUCT = 263,
     ASK = 264,
     DISTINCT = 265,
     REDUCED = 266,
     LIMIT = 267,
     UNION = 268,
     PREFIX = 269,
     BASE = 270,
     BOUND = 271,
     GRAPH = 272,
     NAMED = 273,
     FILTER = 274,
     OFFSET = 275,
     ORDER = 276,
     BY = 277,
     REGEX = 278,
     ASC = 279,
     DESC = 280,
     LANGMATCHES = 281,
     A = 282,
     STRLANG = 283,
     STRDT = 284,
     STR = 285,
     IRI = 286,
     URI = 287,
     BNODE = 288,
     LANG = 289,
     DATATYPE = 290,
     ISURI = 291,
     ISBLANK = 292,
     ISLITERAL = 293,
     ISNUMERIC = 294,
     SAMETERM = 295,
     GROUP = 296,
     HAVING = 297,
     COUNT = 298,
     SUM = 299,
     AVG = 300,
     MIN = 301,
     MAX = 302,
     GROUP_CONCAT = 303,
     SAMPLE = 304,
     SEPARATOR = 305,
     DELETE = 306,
     INSERT = 307,
     WITH = 308,
     CLEAR = 309,
     CREATE = 310,
     SILENT = 311,
     DATA = 312,
     DROP = 313,
     LOAD = 314,
     INTO = 315,
     DEFAULT = 316,
     TO = 317,
     ADD = 318,
     MOVE = 319,
     COPY = 320,
     ALL = 321,
     COALESCE = 322,
     AS = 323,
     IF = 324,
     NOT = 325,
     IN = 326,
     BINDINGS = 327,
     UNDEF = 328,
     SERVICE = 329,
     MINUS = 330,
     YEAR = 331,
     MONTH = 332,
     DAY = 333,
     HOURS = 334,
     MINUTES = 335,
     SECONDS = 336,
     TIMEZONE = 337,
     TZ = 338,
     STRLEN = 339,
     SUBSTR = 340,
     UCASE = 341,
     LCASE = 342,
     STRSTARTS = 343,
     STRENDS = 344,
     CONTAINS = 345,
     ENCODE_FOR_URI = 346,
     CONCAT = 347,
     STRBEFORE = 348,
     STRAFTER = 349,
     REPLACE = 350,
     BIND = 351,
     ABS = 352,
     ROUND = 353,
     CEIL = 354,
     FLOOR = 355,
     RAND = 356,
     MD5 = 357,
     SHA1 = 358,
     SHA224 = 359,
     SHA256 = 360,
     SHA384 = 361,
     SHA512 = 362,
     UUID = 363,
     STRUUID = 364,
     EXPLAIN = 365,
     LET = 366,
     CURRENT_DATETIME = 367,
     NOW = 368,
     FROM_UNIXTIME = 369,
     TO_UNIXTIME = 370,
     HATHAT = 371,
     SC_OR = 372,
     SC_AND = 373,
     EQ = 374,
     NEQ = 375,
     LT = 376,
     GT = 377,
     LE = 378,
     GE = 379,
     ASSIGN = 380,
     STRING = 381,
     LANG_TAG = 382,
     DOUBLE_LITERAL = 383,
     DOUBLE_POSITIVE_LITERAL = 384,
     DOUBLE_NEGATIVE_LITERAL = 385,
     INTEGER_LITERAL = 386,
     INTEGER_POSITIVE_LITERAL = 387,
     INTEGER_NEGATIVE_LITERAL = 388,
     DECIMAL_LITERAL = 389,
     DECIMAL_POSITIVE_LITERAL = 390,
     DECIMAL_NEGATIVE_LITERAL = 391,
     BOOLEAN_LITERAL = 392,
     URI_LITERAL = 393,
     URI_LITERAL_BRACE = 394,
     QNAME_LITERAL = 395,
     QNAME_LITERAL_BRACE = 396,
     BLANK_LITERAL = 397,
     IDENTIFIER = 398
   };
#endif
/* Tokens.  */
#define SELECT 258
#define FROM 259
#define WHERE 260
#define OPTIONAL 261
#define DESCRIBE 262
#define CONSTRUCT 263
#define ASK 264
#define DISTINCT 265
#define REDUCED 266
#define LIMIT 267
#define UNION 268
#define PREFIX 269
#define BASE 270
#define BOUND 271
#define GRAPH 272
#define NAMED 273
#define FILTER 274
#define OFFSET 275
#define ORDER 276
#define BY 277
#define REGEX 278
#define ASC 279
#define DESC 280
#define LANGMATCHES 281
#define A 282
#define STRLANG 283
#define STRDT 284
#define STR 285
#define IRI 286
#define URI 287
#define BNODE 288
#define LANG 289
#define DATATYPE 290
#define ISURI 291
#define ISBLANK 292
#define ISLITERAL 293
#define ISNUMERIC 294
#define SAMETERM 295
#define GROUP 296
#define HAVING 297
#define COUNT 298
#define SUM 299
#define AVG 300
#define MIN 301
#define MAX 302
#define GROUP_CONCAT 303
#define SAMPLE 304
#define SEPARATOR 305
#define DELETE 306
#define INSERT 307
#define WITH 308
#define CLEAR 309
#define CREATE 310
#define SILENT 311
#define DATA 312
#define DROP 313
#define LOAD 314
#define INTO 315
#define DEFAULT 316
#define TO 317
#define ADD 318
#define MOVE 319
#define COPY 320
#define ALL 321
#define COALESCE 322
#define AS 323
#define IF 324
#define NOT 325
#define IN 326
#define BINDINGS 327
#define UNDEF 328
#define SERVICE 329
#define MINUS 330
#define YEAR 331
#define MONTH 332
#define DAY 333
#define HOURS 334
#define MINUTES 335
#define SECONDS 336
#define TIMEZONE 337
#define TZ 338
#define STRLEN 339
#define SUBSTR 340
#define UCASE 341
#define LCASE 342
#define STRSTARTS 343
#define STRENDS 344
#define CONTAINS 345
#define ENCODE_FOR_URI 346
#define CONCAT 347
#define STRBEFORE 348
#define STRAFTER 349
#define REPLACE 350
#define BIND 351
#define ABS 352
#define ROUND 353
#define CEIL 354
#define FLOOR 355
#define RAND 356
#define MD5 357
#define SHA1 358
#define SHA224 359
#define SHA256 360
#define SHA384 361
#define SHA512 362
#define UUID 363
#define STRUUID 364
#define EXPLAIN 365
#define LET 366
#define CURRENT_DATETIME 367
#define NOW 368
#define FROM_UNIXTIME 369
#define TO_UNIXTIME 370
#define HATHAT 371
#define SC_OR 372
#define SC_AND 373
#define EQ 374
#define NEQ 375
#define LT 376
#define GT 377
#define LE 378
#define GE 379
#define ASSIGN 380
#if defined(STRING)
#undef STRING
#endif
#define STRING 381
#define LANG_TAG 382
#define DOUBLE_LITERAL 383
#define DOUBLE_POSITIVE_LITERAL 384
#define DOUBLE_NEGATIVE_LITERAL 385
#define INTEGER_LITERAL 386
#define INTEGER_POSITIVE_LITERAL 387
#define INTEGER_NEGATIVE_LITERAL 388
#define DECIMAL_LITERAL 389
#define DECIMAL_POSITIVE_LITERAL 390
#define DECIMAL_NEGATIVE_LITERAL 391
#define BOOLEAN_LITERAL 392
#define URI_LITERAL 393
#define URI_LITERAL_BRACE 394
#define QNAME_LITERAL 395
#define QNAME_LITERAL_BRACE 396
#define BLANK_LITERAL 397
#define IDENTIFIER 398




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 142 "./sparql_parser.y"

  raptor_sequence *seq;
  rasqal_variable *variable;
  rasqal_literal *literal;
  rasqal_triple *triple;
  rasqal_expression *expr;
  rasqal_graph_pattern *graph_pattern;
  double floating;
  raptor_uri *uri;
  unsigned char *name;
  rasqal_formula *formula;
  rasqal_update_operation *update;
  unsigned int uinteger;
  rasqal_data_graph* data_graph;
  rasqal_row* row;
  rasqal_solution_modifier* modifier;
  int limit_offset[2];
  int integer;
  rasqal_projection* projection;
  rasqal_bindings* bindings;
  sparql_uri_applies* uri_applies;



/* Line 2068 of yacc.c  */
#line 361 "sparql_parser.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




