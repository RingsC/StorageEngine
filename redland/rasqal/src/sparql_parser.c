/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         sparql_parser_parse
#define yylex           sparql_parser_lex
#define yyerror         sparql_parser_error
#define yylval          sparql_parser_lval
#define yychar          sparql_parser_char
#define yydebug         sparql_parser_debug
#define yynerrs         sparql_parser_nerrs


/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 28 "./sparql_parser.y"

#ifdef HAVE_CONFIG_H
#include <rasqal_config.h>
#endif



#include <stdio.h>
#include <stdarg.h>

#include <rasqal.h>
#include <rasqal_internal.h>

/* Pure parser argument (a void*) */
#define YYPARSE_PARAM rq
#include <sparql_parser.h>

#define YY_DECL int sparql_lexer_lex (YYSTYPE *sparql_parser_lval, yyscan_t yyscanner)
#define YY_NO_UNISTD_H 1
#include <sparql_lexer.h>

#include <sparql_common.h>


/* Set RASQAL_DEBUG to 3 for super verbose parsing - watching the shift/reduces */
#if 0
#undef RASQAL_DEBUG
#define RASQAL_DEBUG 3
#endif


#define DEBUG_FH stderr

/* Make verbose error messages for syntax errors */
#define YYERROR_VERBOSE 1

/* Fail with an debug error message if RASQAL_DEBUG > 1 */
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1
#define YYERROR_MSG(msg) do { fputs("** YYERROR ", DEBUG_FH); fputs(msg, DEBUG_FH); fputc('\n', DEBUG_FH); YYERROR; } while(0)
#else
#define YYERROR_MSG(ignore) YYERROR
#endif

/* Slow down the grammar operation and watch it work */
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 2
#define YYDEBUG 1
#endif

/* the lexer does not seem to track this */
#undef RASQAL_SPARQL_USE_ERROR_COLUMNS

/* Missing sparql_lexer.c/h prototypes on Bison < 2.6 */
#if defined(BISON_VERSION) && BISON_VERSION < 206
int sparql_lexer_get_column(yyscan_t yyscanner);
#endif

/* What the lexer wants */
extern int sparql_lexer_lex (YYSTYPE *sparql_parser_lval, yyscan_t scanner);
#define YYLEX_PARAM ((rasqal_sparql_query_language*)(((rasqal_query*)rq)->context))->scanner

/* Pure parser argument (a void*) */
#define YYPARSE_PARAM rq

/* Make the yyerror below use the rdf_parser */
#undef yyerror
#define yyerror(message) sparql_query_error((rasqal_query*)rq, message)

/* Make lex/yacc interface as small as possible */
#undef yylex
#define yylex sparql_lexer_lex


static int sparql_parse(rasqal_query* rq);
static void sparql_query_error(rasqal_query* rq, const char *message);
static void sparql_query_error_full(rasqal_query *rq, const char *message, ...) RASQAL_PRINTF_FORMAT(2, 3);

static sparql_uri_applies*
new_uri_applies(raptor_uri* uri, rasqal_update_graph_applies applies) 
{
  sparql_uri_applies* ua;

  ua = RASQAL_MALLOC(sparql_uri_applies*, sizeof(*ua));
  if(!ua)
    return NULL;
  
  ua->uri = uri;
  ua->applies = applies;

  return ua;
}


static void
free_uri_applies(sparql_uri_applies* ua)
{
  if(ua->uri)
    raptor_free_uri(ua->uri);
  RASQAL_FREE(sparql_uri_applies*, ua);
}





/* Line 268 of yacc.c  */
#line 186 "sparql_parser.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
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

/* Line 293 of yacc.c  */
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



/* Line 293 of yacc.c  */
#line 533 "sparql_parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 545 "sparql_parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  8
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2170

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  160
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  150
/* YYNRULES -- Number of rules.  */
#define YYNRULES  382
/* YYNRULES -- Number of states.  */
#define YYNSTATES  796

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   398

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   159,     2,     2,   124,     2,     2,     2,
     117,   118,   136,   134,   116,   135,   158,   137,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   157,
       2,     2,     2,   123,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   119,     2,   120,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   121,     2,   122,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,    12,    14,    15,    17,    19,
      21,    23,    27,    30,    32,    33,    35,    37,    39,    41,
      43,    45,    47,    49,    51,    53,    56,    59,    60,    65,
      66,    71,    75,    79,    83,    86,    88,    90,    93,    97,
      99,   101,   107,   109,   111,   113,   115,   117,   119,   121,
     123,   124,   126,   128,   134,   140,   146,   152,   158,   163,
     164,   168,   170,   177,   183,   189,   197,   203,   209,   212,
     216,   218,   222,   225,   228,   231,   235,   241,   247,   251,
     253,   259,   265,   267,   269,   272,   274,   278,   284,   290,
     302,   310,   318,   326,   328,   330,   332,   334,   337,   341,
     343,   345,   346,   350,   354,   358,   361,   363,   365,   367,
     369,   371,   373,   377,   383,   389,   395,   401,   404,   406,
     408,   409,   411,   414,   416,   419,   421,   423,   424,   429,
     432,   434,   437,   438,   440,   442,   447,   449,   453,   454,
     456,   459,   461,   464,   465,   468,   471,   473,   475,   476,
     480,   481,   484,   486,   489,   492,   494,   496,   498,   500,
     503,   506,   512,   513,   516,   518,   520,   521,   524,   526,
     530,   533,   536,   538,   540,   543,   547,   551,   553,   555,
     557,   559,   561,   565,   569,   572,   574,   575,   578,   580,
     581,   585,   587,   589,   591,   592,   596,   598,   600,   602,
     604,   606,   608,   610,   612,   615,   619,   624,   627,   631,
     633,   637,   639,   646,   653,   656,   658,   660,   662,   664,
     665,   672,   676,   679,   683,   687,   689,   690,   694,   696,
     697,   701,   703,   706,   709,   713,   716,   717,   719,   720,
     723,   726,   727,   729,   731,   733,   735,   737,   741,   745,
     748,   750,   752,   754,   756,   758,   760,   762,   765,   768,
     770,   773,   776,   778,   780,   782,   784,   786,   788,   791,
     793,   797,   799,   803,   805,   809,   813,   817,   821,   825,
     829,   833,   838,   840,   844,   848,   851,   854,   856,   860,
     864,   866,   869,   872,   875,   877,   879,   881,   883,   885,
     887,   889,   893,   898,   903,   910,   915,   920,   925,   930,
     935,   939,   943,   948,   953,   958,   963,   968,   973,   978,
     983,   988,   993,   997,  1001,  1003,  1005,  1014,  1021,  1028,
    1035,  1040,  1045,  1050,  1055,  1057,  1059,  1061,  1066,  1073,
    1082,  1087,  1092,  1099,  1106,  1113,  1118,  1123,  1130,  1137,
    1146,  1157,  1164,  1173,  1178,  1183,  1188,  1193,  1198,  1203,
    1208,  1213,  1217,  1221,  1226,  1231,  1233,  1235,  1237,  1239,
    1241,  1243,  1245,  1247,  1249,  1251,  1253,  1255,  1257,  1259,
    1261,  1263,  1265
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     161,     0,    -1,   162,    -1,   165,    -1,   168,   163,   164,
     235,    -1,   110,    -1,    -1,   171,    -1,   189,    -1,   190,
      -1,   192,    -1,   168,   167,   166,    -1,   157,   165,    -1,
     157,    -1,    -1,   195,    -1,   200,    -1,   201,    -1,   203,
      -1,   205,    -1,   206,    -1,   210,    -1,   211,    -1,   212,
      -1,   213,    -1,   169,   170,    -1,    15,   151,    -1,    -1,
     170,    14,   156,   151,    -1,    -1,   173,   215,   219,   221,
      -1,   173,   219,   221,    -1,     3,    10,   174,    -1,     3,
      11,   174,    -1,     3,   174,    -1,   175,    -1,   136,    -1,
     175,   176,    -1,   175,   116,   176,    -1,   176,    -1,   285,
      -1,   117,   289,    68,   287,   118,    -1,   180,    -1,   181,
      -1,   182,    -1,   183,    -1,   184,    -1,   187,    -1,   188,
      -1,    10,    -1,    -1,   289,    -1,   136,    -1,    43,   117,
     178,   179,   118,    -1,    44,   117,   178,   289,   118,    -1,
      45,   117,   178,   289,   118,    -1,    46,   117,   178,   289,
     118,    -1,    47,   117,   178,   289,   118,    -1,   157,    50,
     128,   139,    -1,    -1,   186,   116,   289,    -1,   289,    -1,
      48,   117,   178,   186,   185,   118,    -1,    49,   117,   178,
     289,   118,    -1,     8,   267,   215,   219,   221,    -1,     8,
     215,     5,   121,   269,   122,   221,    -1,     7,   191,   215,
     220,   221,    -1,     7,   136,   215,   220,   221,    -1,   191,
     284,    -1,   191,   116,   284,    -1,   284,    -1,     9,   215,
     219,    -1,     4,   216,    -1,     4,   217,    -1,    17,   151,
      -1,    51,   214,   220,    -1,    51,   121,   199,   122,   219,
      -1,    51,    57,   121,   196,   122,    -1,    51,     5,   243,
      -1,   250,    -1,    17,   151,   121,   250,   122,    -1,    17,
     284,   121,   269,   122,    -1,   269,    -1,   197,    -1,   199,
     198,    -1,   198,    -1,    52,   214,   220,    -1,    52,   121,
     199,   122,   220,    -1,    52,    57,   121,   196,   122,    -1,
      53,   151,    51,   121,   199,   122,    52,   121,   199,   122,
     220,    -1,    53,   151,    51,   121,   199,   122,   220,    -1,
      53,   151,    52,   121,   199,   122,   220,    -1,    53,   151,
      52,    57,   121,   196,   122,    -1,   194,    -1,    61,    -1,
      18,    -1,    66,    -1,    17,    61,    -1,    54,   204,   202,
      -1,    54,    -1,    56,    -1,    -1,    55,   204,   151,    -1,
      55,   204,   194,    -1,    58,   204,   202,    -1,   207,   151,
      -1,   151,    -1,    61,    -1,   151,    -1,   194,    -1,   151,
      -1,    61,    -1,    59,   204,   151,    -1,    59,   204,   207,
      60,   209,    -1,    63,   204,   208,    62,   208,    -1,    64,
     204,   208,    62,   208,    -1,    65,   204,   208,    62,   208,
      -1,   214,   193,    -1,   193,    -1,   214,    -1,    -1,   218,
      -1,    18,   218,    -1,   308,    -1,     5,   243,    -1,   243,
      -1,   219,    -1,    -1,   225,   228,   230,   229,    -1,   222,
     224,    -1,   224,    -1,    68,   285,    -1,    -1,   298,    -1,
     263,    -1,   117,   289,   223,   118,    -1,   285,    -1,    41,
      22,   222,    -1,    -1,   261,    -1,   227,   226,    -1,   226,
      -1,    42,   227,    -1,    -1,   233,   234,    -1,   234,   233,
      -1,   233,    -1,   234,    -1,    -1,    21,    22,   231,    -1,
      -1,   231,   232,    -1,   232,    -1,    24,   297,    -1,    25,
     297,    -1,   263,    -1,   285,    -1,   297,    -1,   298,    -1,
      12,   144,    -1,    20,   144,    -1,    72,   236,   121,   237,
     122,    -1,    -1,   236,   285,    -1,   285,    -1,   238,    -1,
      -1,   238,   239,    -1,   239,    -1,   117,   240,   118,    -1,
     117,   118,    -1,   240,   242,    -1,   242,    -1,   139,    -1,
     139,   140,    -1,   139,   125,   308,    -1,   304,   125,   308,
      -1,   308,    -1,   241,    -1,   304,    -1,   150,    -1,    73,
      -1,   121,   172,   122,    -1,   121,   244,   122,    -1,   245,
     246,    -1,   250,    -1,    -1,   246,   247,    -1,   247,    -1,
      -1,   248,   249,   245,    -1,   251,    -1,   260,    -1,   158,
      -1,    -1,   270,   158,   245,    -1,   270,    -1,   256,    -1,
     252,    -1,   255,    -1,   253,    -1,   254,    -1,   258,    -1,
     259,    -1,     6,   243,    -1,    17,   284,   243,    -1,    74,
     204,   284,   243,    -1,    75,   243,    -1,   243,    13,   257,
      -1,   243,    -1,   257,    13,   243,    -1,   243,    -1,   111,
     117,   285,   138,   289,   118,    -1,    96,   117,   289,    68,
     285,   118,    -1,    19,   261,    -1,   297,    -1,   298,    -1,
     263,    -1,   157,    -1,    -1,   308,   117,   178,   266,   262,
     118,    -1,   303,   266,   118,    -1,    67,   265,    -1,   117,
     266,   118,    -1,   266,   116,   289,    -1,   289,    -1,    -1,
     121,   268,   122,    -1,   269,    -1,    -1,   270,   158,   268,
      -1,   270,    -1,   283,   271,    -1,   278,   273,    -1,   277,
     274,   272,    -1,   157,   273,    -1,    -1,   271,    -1,    -1,
     276,   275,    -1,   116,   274,    -1,    -1,   282,    -1,   284,
      -1,    27,    -1,   280,    -1,   279,    -1,   119,   271,   120,
      -1,   117,   281,   118,    -1,   281,   282,    -1,   282,    -1,
     283,    -1,   278,    -1,   285,    -1,   288,    -1,   285,    -1,
     308,    -1,   123,   286,    -1,   124,   286,    -1,   156,    -1,
     123,   286,    -1,   124,   286,    -1,   286,    -1,   308,    -1,
     241,    -1,   304,    -1,   150,    -1,   309,    -1,   117,   118,
      -1,   290,    -1,   290,   126,   291,    -1,   291,    -1,   291,
     127,   292,    -1,   292,    -1,   293,   128,   293,    -1,   293,
     129,   293,    -1,   293,   130,   293,    -1,   293,   131,   293,
      -1,   293,   132,   293,    -1,   293,   133,   293,    -1,   293,
      71,   265,    -1,   293,    70,    71,   265,    -1,   293,    -1,
     294,   134,   293,    -1,   294,   135,   293,    -1,   294,   306,
      -1,   294,   307,    -1,   294,    -1,   295,   136,   294,    -1,
     295,   137,   294,    -1,   295,    -1,   159,   296,    -1,   134,
     296,    -1,   135,   296,    -1,   296,    -1,   297,    -1,   298,
      -1,   263,    -1,   288,    -1,   285,    -1,   177,    -1,   117,
     289,   118,    -1,    30,   117,   289,   118,    -1,    34,   117,
     289,   118,    -1,    26,   117,   289,   116,   289,   118,    -1,
      35,   117,   289,   118,    -1,    16,   117,   285,   118,    -1,
      31,   117,   289,   118,    -1,    32,   117,   289,   118,    -1,
      33,   117,   289,   118,    -1,    33,   117,   118,    -1,   101,
     117,   118,    -1,    97,   117,   289,   118,    -1,    99,   117,
     289,   118,    -1,   100,   117,   289,   118,    -1,    98,   117,
     289,   118,    -1,   102,   117,   289,   118,    -1,   103,   117,
     289,   118,    -1,   104,   117,   289,   118,    -1,   105,   117,
     289,   118,    -1,   106,   117,   289,   118,    -1,   107,   117,
     289,   118,    -1,   108,   117,   118,    -1,   109,   117,   118,
      -1,   299,    -1,   264,    -1,    69,   117,   289,   116,   289,
     116,   289,   118,    -1,    28,   117,   289,   116,   289,   118,
      -1,    29,   117,   289,   116,   289,   118,    -1,    40,   117,
     289,   116,   289,   118,    -1,    36,   117,   289,   118,    -1,
      37,   117,   289,   118,    -1,    38,   117,   289,   118,    -1,
      39,   117,   289,   118,    -1,   300,    -1,   301,    -1,   302,
      -1,    84,   117,   289,   118,    -1,    85,   117,   289,   116,
     289,   118,    -1,    85,   117,   289,   116,   289,   116,   289,
     118,    -1,    86,   117,   289,   118,    -1,    87,   117,   289,
     118,    -1,    88,   117,   289,   116,   289,   118,    -1,    89,
     117,   289,   116,   289,   118,    -1,    90,   117,   289,   116,
     289,   118,    -1,    91,   117,   289,   118,    -1,    92,   117,
     186,   118,    -1,    93,   117,   289,   116,   289,   118,    -1,
      94,   117,   289,   116,   289,   118,    -1,    95,   117,   289,
     116,   289,   116,   289,   118,    -1,    95,   117,   289,   116,
     289,   116,   289,   116,   289,   118,    -1,    23,   117,   289,
     116,   289,   118,    -1,    23,   117,   289,   116,   289,   116,
     289,   118,    -1,    76,   117,   289,   118,    -1,    77,   117,
     289,   118,    -1,    78,   117,   289,   118,    -1,    79,   117,
     289,   118,    -1,    80,   117,   289,   118,    -1,    81,   117,
     289,   118,    -1,    82,   117,   289,   118,    -1,    83,   117,
     289,   118,    -1,   112,   117,   118,    -1,   113,   117,   118,
      -1,   114,   117,   289,   118,    -1,   115,   117,   289,   118,
      -1,   152,    -1,   154,    -1,   305,    -1,   306,    -1,   307,
      -1,   144,    -1,   147,    -1,   141,    -1,   145,    -1,   148,
      -1,   142,    -1,   146,    -1,   149,    -1,   143,    -1,   151,
      -1,   153,    -1,   155,    -1,   119,   120,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   495,   495,   496,   501,   510,   524,   531,   551,   556,
     561,   569,   573,   574,   575,   579,   583,   587,   591,   595,
     599,   603,   607,   611,   615,   623,   631,   636,   643,   673,
     680,   698,   712,   717,   722,   732,   736,   746,   755,   764,
     782,   786,   828,   832,   836,   840,   844,   848,   852,   859,
     873,   879,   883,   891,   912,   933,   954,   975,   996,  1003,
    1012,  1021,  1037,  1065,  1087,  1110,  1155,  1179,  1196,  1205,
    1214,  1230,  1251,  1255,  1263,  1271,  1291,  1323,  1343,  1387,
    1404,  1437,  1452,  1456,  1464,  1479,  1488,  1508,  1538,  1558,
    1602,  1644,  1686,  1710,  1714,  1718,  1722,  1726,  1739,  1771,
    1806,  1811,  1818,  1845,  1880,  1916,  1925,  1944,  1948,  1955,
    1959,  1967,  1979,  2006,  2052,  2083,  2114,  2145,  2151,  2160,
    2165,  2172,  2196,  2219,  2227,  2231,  2239,  2244,  2251,  2265,
    2275,  2295,  2300,  2307,  2311,  2315,  2342,  2356,  2370,  2377,
    2384,  2394,  2414,  2428,  2435,  2440,  2445,  2450,  2456,  2464,
    2469,  2476,  2486,  2506,  2513,  2520,  2528,  2545,  2553,  2565,
    2579,  2592,  2606,  2613,  2622,  2639,  2644,  2651,  2663,  2683,
    2707,  2715,  2724,  2742,  2748,  2754,  2762,  2780,  2784,  2788,
    2792,  2796,  2808,  2812,  2823,  2881,  2895,  2907,  2944,  2949,
    2961,  3009,  3022,  3050,  3051,  3056,  3087,  3095,  3099,  3103,
    3107,  3111,  3115,  3119,  3127,  3166,  3216,  3233,  3243,  3258,
    3265,  3275,  3300,  3321,  3342,  3350,  3354,  3358,  3365,  3370,
    3377,  3416,  3450,  3478,  3485,  3495,  3512,  3520,  3528,  3533,
    3544,  3574,  3589,  3633,  3681,  3779,  3784,  3791,  3796,  3803,
    3881,  3886,  3893,  3901,  3911,  3939,  3943,  3951,  4018,  4145,
    4198,  4235,  4239,  4247,  4259,  4272,  4278,  4286,  4290,  4297,
    4308,  4312,  4316,  4326,  4330,  4334,  4338,  4342,  4346,  4356,
    4364,  4371,  4379,  4387,  4396,  4403,  4410,  4417,  4424,  4431,
    4438,  4443,  4448,  4457,  4464,  4471,  4484,  4497,  4504,  4511,
    4518,  4526,  4533,  4537,  4544,  4558,  4562,  4566,  4575,  4581,
    4591,  4599,  4607,  4614,  4621,  4628,  4635,  4650,  4657,  4664,
    4671,  4678,  4685,  4692,  4699,  4706,  4713,  4720,  4727,  4734,
    4741,  4748,  4755,  4762,  4769,  4773,  4777,  4784,  4791,  4798,
    4805,  4812,  4819,  4826,  4833,  4837,  4841,  4848,  4855,  4862,
    4869,  4876,  4883,  4890,  4897,  4904,  4911,  4918,  4925,  4932,
    4939,  4950,  4957,  4968,  4975,  4982,  4989,  4996,  5003,  5010,
    5017,  5028,  5045,  5063,  5081,  5106,  5112,  5132,  5136,  5140,
    5147,  5151,  5155,  5163,  5167,  5171,  5179,  5183,  5187,  5199,
    5205,  5225,  5231
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "SELECT", "FROM", "WHERE", "OPTIONAL",
  "DESCRIBE", "CONSTRUCT", "ASK", "DISTINCT", "REDUCED", "LIMIT", "UNION",
  "PREFIX", "BASE", "BOUND", "GRAPH", "NAMED", "FILTER", "OFFSET", "ORDER",
  "BY", "REGEX", "ASC", "DESC", "LANGMATCHES", "\"a\"", "\"strlang\"",
  "\"strdt\"", "\"str\"", "\"iri\"", "\"uri\"", "\"bnode\"", "\"lang\"",
  "\"datatype\"", "\"isUri\"", "\"isBlank\"", "\"isLiteral\"",
  "\"isNumeric\"", "\"sameTerm\"", "GROUP", "HAVING", "COUNT", "SUM",
  "AVG", "MIN", "MAX", "GROUP_CONCAT", "SAMPLE", "SEPARATOR", "DELETE",
  "INSERT", "WITH", "CLEAR", "CREATE", "SILENT", "DATA", "DROP", "LOAD",
  "INTO", "DEFAULT", "TO", "ADD", "MOVE", "COPY", "ALL", "COALESCE", "AS",
  "IF", "NOT", "IN", "BINDINGS", "UNDEF", "SERVICE", "MINUS", "YEAR",
  "MONTH", "DAY", "HOURS", "MINUTES", "SECONDS", "TIMEZONE", "TZ",
  "STRLEN", "SUBSTR", "UCASE", "LCASE", "STRSTARTS", "STRENDS", "CONTAINS",
  "ENCODE_FOR_URI", "CONCAT", "STRBEFORE", "STRAFTER", "REPLACE", "BIND",
  "ABS", "ROUND", "CEIL", "FLOOR", "RAND", "MD5", "SHA1", "SHA224",
  "SHA256", "SHA384", "SHA512", "UUID", "STRUUID", "EXPLAIN", "LET",
  "CURRENT_DATETIME", "NOW", "FROM_UNIXTIME", "TO_UNIXTIME", "','", "'('",
  "')'", "'['", "']'", "'{'", "'}'", "'?'", "'$'", "\"^^\"", "SC_OR",
  "SC_AND", "EQ", "NEQ", "LT", "GT", "LE", "GE", "'+'", "'-'", "'*'",
  "'/'", "\":=\"", "\"string\"", "\"language tag\"", "\"double literal\"",
  "\"double positive literal\"", "\"double negative literal\"",
  "\"integer literal\"", "\"integer positive literal\"",
  "\"integer negative literal\"", "\"decimal literal\"",
  "\"decimal positive literal\"", "\"decimal negative literal\"",
  "\"boolean literal\"", "\"URI literal\"", "\"URI literal (\"",
  "\"QName literal\"", "\"QName literal (\"", "\"blank node literal\"",
  "\"identifier\"", "';'", "'.'", "'!'", "$accept", "Sparql", "Query",
  "ExplainOpt", "ReportFormat", "Update", "UpdateTailOpt",
  "UpdateOperation", "Prologue", "BaseDeclOpt", "PrefixDeclListOpt",
  "SelectQuery", "SubSelect", "SelectClause", "SelectExpressionList",
  "SelectExpressionListTail", "SelectTerm", "AggregateExpression",
  "DistinctOpt", "ExpressionOrStar", "CountAggregateExpression",
  "SumAggregateExpression", "AvgAggregateExpression",
  "MinAggregateExpression", "MaxAggregateExpression", "SeparatorOpt",
  "ExpressionList", "GroupConcatAggregateExpression",
  "SampleAggregateExpression", "ConstructQuery", "DescribeQuery",
  "VarOrIRIrefList", "AskQuery", "DatasetClause", "GraphRef",
  "DeleteQuery", "GraphTriples", "GraphTemplate", "ModifyTemplate",
  "ModifyTemplateList", "InsertQuery", "UpdateQuery", "GraphRefAll",
  "ClearQuery", "SilentOpt", "CreateQuery", "DropQuery", "IriRefList",
  "GraphOrDefault", "OldGraphRef", "LoadQuery", "AddQuery", "MoveQuery",
  "CopyQuery", "DatasetClauseList", "DatasetClauseListOpt",
  "DefaultGraphClause", "NamedGraphClause", "SourceSelector",
  "WhereClause", "WhereClauseOpt", "SolutionModifier",
  "GroupConditionList", "AsVarOpt", "GroupCondition", "GroupClauseOpt",
  "HavingCondition", "HavingConditionList", "HavingClauseOpt",
  "LimitOffsetClausesOpt", "OrderClauseOpt", "OrderConditionList",
  "OrderCondition", "LimitClause", "OffsetClause", "BindingsClauseOpt",
  "VarList", "BindingsRowListOpt", "BindingsRowList", "BindingsRow",
  "BindingValueList", "RDFLiteral", "BindingValue", "GroupGraphPattern",
  "GroupGraphPatternSub", "TriplesBlockOpt", "GraphPatternListOpt",
  "GraphPatternList", "GraphPatternListFilter", "DotOptional",
  "TriplesBlock", "GraphPatternNotTriples", "OptionalGraphPattern",
  "GraphGraphPattern", "ServiceGraphPattern", "MinusGraphPattern",
  "GroupOrUnionGraphPattern", "GroupOrUnionGraphPatternList",
  "LetGraphPattern", "BindGraphPattern", "Filter", "Constraint",
  "ParamsOpt", "FunctionCall", "CoalesceExpression", "ArgList",
  "ArgListNoBraces", "ConstructTemplate", "ConstructTriplesOpt",
  "ConstructTriples", "TriplesSameSubject", "PropertyListNotEmpty",
  "PropertyListTailOpt", "PropertyList", "ObjectList", "ObjectTail",
  "Object", "Verb", "TriplesNode", "BlankNodePropertyList", "Collection",
  "GraphNodeListNotEmpty", "GraphNode", "VarOrTerm", "VarOrIRIref", "Var",
  "VarName", "VarOrBadVarName", "GraphTerm", "Expression",
  "ConditionalOrExpression", "ConditionalAndExpression",
  "RelationalExpression", "AdditiveExpression", "MultiplicativeExpression",
  "UnaryExpression", "PrimaryExpression", "BrackettedExpression",
  "BuiltInCall", "StringExpression", "RegexExpression",
  "DatetimeBuiltinAccessors", "DatetimeExtensions", "IRIrefBrace",
  "NumericLiteral", "NumericLiteralUnsigned", "NumericLiteralPositive",
  "NumericLiteralNegative", "IRIref", "BlankNode", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,    44,    40,    41,    91,
      93,   123,   125,    63,    36,   371,   372,   373,   374,   375,
     376,   377,   378,   379,    43,    45,    42,    47,   380,   381,
     382,   383,   384,   385,   386,   387,   388,   389,   390,   391,
     392,   393,   394,   395,   396,   397,   398,    59,    46,    33
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   160,   161,   161,   162,   163,   163,   164,   164,   164,
     164,   165,   166,   166,   166,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   168,   169,   169,   170,   170,
     171,   172,   173,   173,   173,   174,   174,   175,   175,   175,
     176,   176,   177,   177,   177,   177,   177,   177,   177,   178,
     178,   179,   179,   180,   181,   182,   183,   184,   185,   185,
     186,   186,   187,   188,   189,   189,   190,   190,   191,   191,
     191,   192,   193,   193,   194,   195,   195,   195,   195,   196,
     196,   197,   198,   198,   199,   199,   200,   200,   200,   201,
     201,   201,   201,   202,   202,   202,   202,   202,   203,   203,
     204,   204,   205,   205,   206,   207,   207,   208,   208,   209,
     209,   209,   210,   210,   211,   212,   213,   214,   214,   215,
     215,   216,   217,   218,   219,   219,   220,   220,   221,   222,
     222,   223,   223,   224,   224,   224,   224,   225,   225,   226,
     227,   227,   228,   228,   229,   229,   229,   229,   229,   230,
     230,   231,   231,   232,   232,   232,   232,   232,   232,   233,
     234,   235,   235,   236,   236,   237,   237,   238,   238,   239,
     239,   240,   240,   241,   241,   241,   241,   242,   242,   242,
     242,   242,   243,   243,   244,   245,   245,   246,   246,   246,
     247,   248,   248,   249,   249,   250,   250,   251,   251,   251,
     251,   251,   251,   251,   252,   253,   254,   255,   256,   256,
     257,   257,   258,   259,   260,   261,   261,   261,   262,   262,
     263,   263,   264,   265,   266,   266,   266,   267,   268,   268,
     269,   269,   270,   270,   271,   272,   272,   273,   273,   274,
     275,   275,   276,   277,   277,   278,   278,   279,   280,   281,
     281,   282,   282,   283,   283,   284,   284,   285,   285,   286,
     287,   287,   287,   288,   288,   288,   288,   288,   288,   289,
     290,   290,   291,   291,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   293,   293,   293,   293,   293,   294,   294,
     294,   295,   295,   295,   295,   296,   296,   296,   296,   296,
     296,   297,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   299,   299,   299,
     299,   299,   299,   299,   299,   299,   299,   299,   299,   299,
     299,   300,   300,   301,   301,   301,   301,   301,   301,   301,
     301,   302,   302,   302,   302,   303,   303,   304,   304,   304,
     305,   305,   305,   306,   306,   306,   307,   307,   307,   308,
     308,   309,   309
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     4,     1,     0,     1,     1,     1,
       1,     3,     2,     1,     0,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     0,     4,     0,
       4,     3,     3,     3,     2,     1,     1,     2,     3,     1,
       1,     5,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     1,     1,     5,     5,     5,     5,     5,     4,     0,
       3,     1,     6,     5,     5,     7,     5,     5,     2,     3,
       1,     3,     2,     2,     2,     3,     5,     5,     3,     1,
       5,     5,     1,     1,     2,     1,     3,     5,     5,    11,
       7,     7,     7,     1,     1,     1,     1,     2,     3,     1,
       1,     0,     3,     3,     3,     2,     1,     1,     1,     1,
       1,     1,     3,     5,     5,     5,     5,     2,     1,     1,
       0,     1,     2,     1,     2,     1,     1,     0,     4,     2,
       1,     2,     0,     1,     1,     4,     1,     3,     0,     1,
       2,     1,     2,     0,     2,     2,     1,     1,     0,     3,
       0,     2,     1,     2,     2,     1,     1,     1,     1,     2,
       2,     5,     0,     2,     1,     1,     0,     2,     1,     3,
       2,     2,     1,     1,     2,     3,     3,     1,     1,     1,
       1,     1,     3,     3,     2,     1,     0,     2,     1,     0,
       3,     1,     1,     1,     0,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     3,     4,     2,     3,     1,
       3,     1,     6,     6,     2,     1,     1,     1,     1,     0,
       6,     3,     2,     3,     3,     1,     0,     3,     1,     0,
       3,     1,     2,     2,     3,     2,     0,     1,     0,     2,
       2,     0,     1,     1,     1,     1,     1,     3,     3,     2,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     1,
       2,     2,     1,     1,     1,     1,     1,     1,     2,     1,
       3,     1,     3,     1,     3,     3,     3,     3,     3,     3,
       3,     4,     1,     3,     3,     2,     2,     1,     3,     3,
       1,     2,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     3,     4,     4,     6,     4,     4,     4,     4,     4,
       3,     3,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     3,     3,     1,     1,     8,     6,     6,     6,
       4,     4,     4,     4,     1,     1,     1,     4,     6,     8,
       4,     4,     6,     6,     6,     4,     4,     6,     6,     8,
      10,     6,     8,     4,     4,     4,     4,     4,     4,     4,
       4,     3,     3,     4,     4,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
      27,     0,     0,     2,     3,     6,    29,    26,     1,     0,
       0,     0,   101,   101,   101,   101,   101,   101,   101,     5,
       0,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,     0,     0,     0,     0,   118,   127,     0,
       0,   127,     0,   100,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   120,   120,   162,     7,   120,     8,     9,
      10,    27,    11,     0,     0,   379,   380,    72,    73,   121,
     123,   186,    78,     0,     0,     0,     0,     0,     0,   173,
     372,   375,   378,   370,   373,   376,   371,   374,   377,   266,
     381,    83,    85,     0,   264,    82,   231,   238,   246,   245,
       0,   253,   254,   265,   367,   368,   369,   263,   267,     0,
     117,   126,    75,   125,     0,     0,    86,     0,     0,     0,
      95,    94,    96,    93,    98,     0,   102,   103,   104,   106,
       0,   107,   108,     0,     0,     0,     0,     0,     0,    36,
      34,    35,    39,    40,   120,   120,    70,   255,   256,   229,
     119,     0,   120,     0,     0,     4,     0,    12,     0,     0,
     122,     0,     0,     0,   189,   185,   196,     0,     0,    79,
       0,   268,   252,     0,   250,   251,   244,   382,     0,     0,
     243,   259,   257,   258,     0,   174,     0,    84,   229,   237,
     233,   232,     0,   124,     0,   127,     0,     0,     0,    97,
      74,     0,   105,     0,     0,     0,    32,    33,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   365,   366,     0,   300,    42,    43,
      44,    45,    46,    47,    48,   297,   325,   299,   298,     0,
     269,   271,   273,   282,   287,   290,   294,   295,   296,   324,
     334,   335,   336,   226,   263,     0,    37,   127,     0,   127,
      68,     0,   228,     0,     0,    71,     0,   164,   138,    28,
     182,   138,   183,     0,     0,     0,   101,     0,     0,     0,
     209,   184,   188,   194,   191,   198,   200,   201,   199,   197,
     202,   203,   192,   186,     0,    77,     0,   248,   249,   247,
     236,   241,   242,   175,    76,   230,   176,    88,    87,     0,
       0,     0,   111,   110,   109,   113,   114,   115,   116,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    50,    50,    50,    50,
      50,    50,   226,   222,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   292,   293,   291,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   285,
     286,     0,     0,     0,   225,    50,    38,   138,    69,   138,
     227,     0,   138,   166,   163,     0,    30,   143,    31,   204,
       0,     0,   214,   217,   215,   216,     0,     0,   207,     0,
       0,     0,   187,   193,   186,   195,     0,     0,   238,   234,
       0,   239,   127,     0,   127,     0,     0,     0,     0,     0,
       0,     0,     0,   310,     0,     0,     0,     0,     0,     0,
       0,     0,    49,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    61,
       0,     0,     0,     0,     0,     0,     0,   311,     0,     0,
       0,     0,     0,     0,   322,   323,   361,   362,     0,     0,
     301,     0,     0,   262,     0,   270,   272,     0,   280,   274,
     275,   276,   277,   278,   279,   283,   284,   288,   289,     0,
     221,   226,    67,    66,     0,    64,     0,     0,   165,   168,
       0,     0,   150,   205,     0,     0,     0,   211,   208,   190,
       0,    81,   235,   240,     0,    90,    92,    91,   306,     0,
       0,     0,     0,   302,   307,   308,   309,   303,   305,   330,
     331,   332,   333,     0,    52,     0,    51,     0,     0,     0,
       0,    59,     0,   223,     0,   353,   354,   355,   356,   357,
     358,   359,   360,   337,     0,   340,   341,     0,     0,     0,
     345,     0,   346,     0,     0,     0,   312,   315,   313,   314,
     316,   317,   318,   319,   320,   321,   363,   364,   260,   261,
      41,   281,   224,   219,   138,   181,   170,   180,     0,   178,
     172,   179,   177,   161,   167,     0,   137,   130,   134,   136,
     133,   141,   142,   139,     0,   148,   206,     0,     0,     0,
      80,     0,     0,     0,     0,     0,     0,    53,    54,    55,
      56,    57,     0,     0,    63,     0,     0,     0,     0,     0,
      60,     0,     0,     0,   218,     0,    65,   169,   171,   132,
     129,   140,     0,     0,     0,   128,   146,   147,     0,     0,
     210,     0,     0,   351,   304,   327,   328,   329,     0,    62,
       0,     0,   338,   342,   343,   344,   347,   348,     0,   220,
       0,     0,     0,     0,   149,   152,   155,   156,   157,   158,
     159,   160,   144,   145,   213,   212,   127,     0,     0,     0,
       0,     0,   131,   135,   153,   154,   151,    89,   352,    58,
     326,   339,     0,   349,     0,   350
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,    20,    55,     4,    62,    21,     5,     6,
      32,    56,   161,    57,   140,   141,   142,   277,   513,   625,
     278,   279,   280,   281,   282,   713,   538,   283,   284,    58,
      59,   145,    60,    37,   123,    22,   168,    91,    92,    93,
      23,    24,   124,    25,    44,    26,    27,   130,   133,   365,
      28,    29,    30,    31,   150,   151,    67,    68,    69,   111,
     112,   466,   686,   761,   687,   467,   691,   692,   592,   735,
     695,   764,   765,   736,   737,   155,   316,   587,   588,   589,
     678,    94,   680,   113,   163,   164,   331,   332,   333,   484,
     165,   334,   335,   336,   337,   338,   339,   598,   340,   341,
     342,   693,   725,   285,   286,   393,   453,   152,   311,    95,
      96,   189,   489,   190,   350,   491,   351,   179,    97,    98,
      99,   173,   352,   100,   180,   287,   182,   564,   288,   454,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   103,   104,   105,   106,   304,   108
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -534
static const yytype_int16 yypact[] =
{
      60,   -65,    79,  -534,  -534,   355,  -534,  -534,  -534,    66,
      89,   -35,    25,    52,    52,    52,    52,    52,    52,  -534,
     354,   -25,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,
    -534,  -534,    78,    17,    18,    21,   740,  -534,    55,    28,
     740,    55,   274,  -534,   273,    11,   273,    37,   -23,   -23,
     -23,   133,   200,    63,   190,   132,  -534,   190,  -534,  -534,
    -534,    69,  -534,    56,   -73,  -534,  -534,  -534,  -534,  -534,
    -534,    94,  -534,   950,   182,  1002,    75,    58,    58,   -36,
    -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,
    -534,  -534,  -534,   357,  -534,  -534,    50,   162,  -534,  -534,
     162,  -534,  -534,   102,  -534,  -534,  -534,  -534,  -534,    18,
    -534,  -534,  -534,  -534,   950,   454,  -534,   109,     6,    35,
    -534,  -534,  -534,  -534,  -534,    95,  -534,  -534,  -534,    26,
     -30,  -534,  -534,   189,   192,   205,   136,   136,  1582,  -534,
    -534,   -18,  -534,  -534,   190,    14,  -534,  -534,  -534,  2015,
     190,   257,   190,    57,   248,  -534,    57,  -534,   707,   126,
    -534,   174,    57,   187,   291,  -534,   154,   171,   233,  -534,
     163,  -534,  -534,  1980,  -534,  -534,  -534,  -534,   239,  2015,
    -534,  -534,  -534,  -534,   -73,  -534,    57,  -534,  2015,  -534,
    -534,  -534,   -73,  -534,   246,    57,   740,   252,   740,  -534,
    -534,    40,  -534,   -23,   -23,   -23,  -534,  -534,   258,   261,
     267,   275,   281,   282,   283,   284,   286,   287,   294,   298,
     300,   304,   305,   314,   316,   317,   318,   321,   322,   323,
     327,   330,   332,   333,   342,   346,   349,   350,   351,   352,
     353,   356,   358,   360,   361,   365,   366,   367,   368,   369,
     370,   371,   374,   375,   376,   377,   378,   397,   398,   400,
     401,   402,   404,   406,   408,   410,   411,   412,   414,   436,
    1162,   271,  1722,  1722,  -534,  -534,  1722,  -534,  -534,  -534,
    -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,   487,
     434,   345,  -534,   217,   440,   201,  -534,  -534,  -534,  -534,
    -534,  -534,  -534,  1582,   444,   141,  -534,    57,   182,    57,
    -534,   435,  -534,   441,    57,  -534,   219,  -534,   522,  -534,
    -534,   522,  -534,    18,   182,  1964,    52,    18,   447,   448,
     554,   291,  -534,   421,  -534,  -534,  -534,  -534,  -534,  -534,
    -534,  -534,  -534,  2015,   459,  -534,  2015,  -534,  -534,  -534,
     424,   468,  -534,  -534,  -534,  -534,  -534,  -534,  -534,   494,
     950,   539,  -534,  -534,  -534,  -534,  -534,  -534,  -534,   248,
    1582,  1582,  1582,  1582,  1582,  1582,  1582,  1302,  1582,  1582,
    1582,  1582,  1582,  1582,  1582,   577,   577,   577,   577,   577,
     577,   577,  1582,  -534,  1582,  1582,  1582,  1582,  1582,  1582,
    1582,  1582,  1582,  1582,  1582,  1582,  1582,  1582,  1582,  1582,
    1582,  1582,  1582,  1582,  1582,  1582,  1582,  1582,  1582,   472,
    1582,  1582,  1582,  1582,  1582,  1582,   473,   474,   476,   488,
    1582,  1582,   490,  -534,  -534,  -534,    96,  1582,  1582,   541,
     330,  1582,  1582,  1582,  1582,  1582,  1582,  1582,  1582,  -534,
    -534,  1582,  1582,    84,  -534,   577,  -534,   522,  -534,   522,
    -534,  2015,   522,   493,  -534,   593,  -534,   578,  -534,  -534,
      18,  1582,  -534,  -534,  -534,  -534,   444,   182,  -534,  1582,
     248,    18,  -534,  -534,  2015,  -534,  2015,   497,   162,  -534,
    2015,  -534,    72,   499,    57,   504,   507,   508,   509,   511,
     510,   512,   513,  -534,   514,   516,   530,   532,   533,   534,
     537,   543,  -534,  1442,  1582,  1582,  1582,  1582,  1582,  1582,
     107,   549,   548,   550,   552,   553,   555,   556,   558,   559,
     561,   551,   573,   575,   579,   580,   581,   584,   186,  -534,
     582,   583,   587,   586,   588,   589,   590,  -534,   592,   594,
     595,   599,   600,   605,  -534,  -534,  -534,  -534,   606,   607,
    -534,    58,    58,  -534,   608,   345,  -534,   330,  -534,  -534,
    -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  1582,
    -534,  1582,  -534,  -534,   535,  -534,   391,   609,   493,  -534,
    1862,  1964,   651,  -534,    18,   637,   491,  -534,   698,  -534,
     612,  -534,  -534,  -534,   628,  -534,  -534,  -534,  -534,  1582,
    1582,  1582,  1582,  -534,  -534,  -534,  -534,  -534,  -534,  -534,
    -534,  -534,  -534,  1582,  -534,   610,  -534,   611,   633,   635,
     636,   -43,   638,  -534,  1582,  -534,  -534,  -534,  -534,  -534,
    -534,  -534,  -534,  -534,  1582,  -534,  -534,  1582,  1582,  1582,
    -534,  1582,  -534,  1582,  1582,  1582,  -534,  -534,  -534,  -534,
    -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,
    -534,  -534,  -534,   -42,   522,  -534,  -534,  -534,   989,  -534,
    -534,   102,  -534,  -534,  -534,  1582,  1862,  -534,  -534,  -534,
    -534,  -534,  1964,  -534,   693,    83,  -534,   248,  1582,    18,
    -534,   740,   214,   645,   646,   649,   655,  -534,  -534,  -534,
    -534,  -534,   677,   656,  -534,   614,   238,   657,   658,   660,
    -534,   661,   662,   639,  -534,   663,  -534,  -534,  -534,   701,
    -534,  -534,   899,   640,   641,  -534,   762,   774,   669,   670,
    -534,   597,  1582,  -534,  -534,  -534,  -534,  -534,   664,  -534,
    1582,  1582,  -534,  -534,  -534,  -534,  -534,  -534,  1582,  -534,
     248,   671,   673,   673,   899,  -534,  -534,  -534,  -534,  -534,
    -534,  -534,  -534,  -534,  -534,  -534,    57,   675,   652,   676,
     679,   251,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,
    -534,  -534,  1582,  -534,   680,  -534
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -534,  -534,  -534,  -534,  -534,   734,  -534,  -534,   738,  -534,
    -534,  -534,  -534,   745,   244,  -534,  -126,  -534,   -70,  -534,
    -534,  -534,  -534,  -534,  -534,  -534,   299,  -534,  -534,  -534,
    -534,  -534,  -534,    53,   -21,  -534,  -112,  -534,   -88,   -37,
    -534,  -534,   772,  -534,    32,  -534,  -534,  -534,     2,  -534,
    -534,  -534,  -534,  -534,   379,    77,  -534,  -534,   755,  -122,
     -41,  -288,  -534,  -534,   134,  -534,   129,  -534,  -534,  -534,
    -534,  -534,    86,    85,    88,  -534,  -534,  -534,  -534,   270,
    -534,  -533,   188,   -24,  -534,  -326,  -534,   529,  -534,  -534,
     -59,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,  -534,
    -534,   540,  -534,  -316,  -534,  -404,  -380,  -534,   681,  -130,
     -60,   -10,  -534,   380,   381,  -534,  -534,  -534,   -54,  -534,
    -534,  -534,   -38,   -53,   -45,    36,   -72,  -534,    82,   431,
    -534,   430,   432,   104,   -58,  -534,    22,  -302,  -309,  -534,
    -534,  -534,  -534,  -534,  -530,  -534,   598,   602,   -32,  -534
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -113
static const yytype_int16 yytable[] =
{
     116,    70,   194,   115,   107,   187,   183,   146,   107,   473,
      72,   166,   520,   166,   169,   306,   475,   485,    33,   312,
     148,   172,   175,   474,   127,   -99,  -112,   187,   125,   170,
     201,   315,    70,   468,   318,    64,   568,   174,   131,   107,
     321,   107,   148,   107,   148,    45,    46,    47,    48,    49,
      50,   134,   135,   679,   166,   169,   681,   125,   312,    33,
     109,   107,   109,   197,   354,   148,   178,    33,   148,   -13,
      33,    34,   101,   651,   579,     1,   101,   109,    65,     8,
      66,    43,   107,   107,     1,   193,     7,   143,   147,   184,
     191,   110,    63,    33,   110,   733,   199,    51,   305,   138,
     310,   362,   176,   734,   185,    77,    78,   101,    43,   101,
     147,   101,   147,   148,   712,   724,    42,   107,   102,   172,
     175,   202,   102,    35,   604,   172,   175,   198,   132,   101,
     308,   153,    61,   147,   156,   348,   147,    77,    78,    71,
     330,   107,    73,   136,   137,   679,    39,   107,   681,   114,
     101,   101,   353,   102,   358,   102,   107,   102,   599,   359,
     356,   361,   126,   671,   107,    65,   107,    66,    65,   582,
      66,   583,   143,   143,   585,   102,    71,   143,    71,   456,
     364,   147,   -99,  -112,   149,   101,   200,    36,   129,   176,
     317,   363,   462,    71,    33,   177,   102,   102,    77,    78,
     579,   673,   580,   110,   154,   366,   367,   368,   188,   101,
      40,    75,   159,    76,   181,   101,   487,    77,    78,   561,
     562,   307,   309,   579,   101,   633,    65,   192,    66,   314,
     196,   102,   101,    79,   101,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    65,   200,    66,   493,    90,
     138,   203,   181,   138,   204,   102,    77,    78,   138,    77,
      78,   102,   313,   458,    77,    78,   457,   205,   459,   139,
     102,   187,   139,   187,   688,   473,   148,   319,   102,   470,
     102,   690,   475,   166,   346,    77,    78,   439,   440,   474,
     119,   120,   148,   476,   433,   434,   320,   323,   435,   469,
     166,   169,   651,   478,   652,    77,    78,   330,   324,   322,
     325,   107,   343,    65,   107,    66,   514,   515,   516,   517,
     518,   519,   344,    77,    78,   117,   118,   107,   107,   107,
     742,   584,   743,    65,   121,    66,   144,   451,   452,   122,
     463,   143,    77,    78,   147,   441,   442,   443,   444,   445,
     446,    65,   464,    66,   751,   345,   752,    51,   477,   349,
     147,    52,    53,    54,   563,   326,   327,   792,   357,   793,
     688,    77,    78,   360,    74,   369,   473,   690,   370,   101,
     206,   207,   101,   475,   371,   581,   726,   328,    38,    41,
     474,   177,   372,   577,   578,   101,   101,   101,   373,   374,
     375,   376,   329,   377,   378,   495,     9,    10,    11,    12,
      13,   379,    71,    14,    15,   380,   766,   381,    16,    17,
      18,   382,   383,   769,   166,   102,   166,   600,   102,   107,
     768,   384,   594,   385,   386,   387,   172,   175,   388,   389,
     390,   102,   102,   102,   391,   148,   593,   392,   766,   394,
     395,   605,   107,   607,   107,   769,   148,   597,   107,   396,
     784,   785,   768,   397,   675,    19,   398,   399,   400,   401,
     402,    74,   438,   403,    75,   404,    76,   405,   406,   186,
      77,    78,   407,   408,   409,   410,   411,   412,   413,   668,
     669,   414,   415,   416,   417,   418,    79,   101,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    65,   676,
      66,    74,    90,   147,   419,   420,   596,   421,   422,   423,
     101,   424,   101,   425,   147,   426,   101,   427,   428,   429,
      79,   430,    80,    81,    82,    83,    84,    85,    86,    87,
      88,   677,    65,   102,    66,   569,   570,   571,   572,   573,
     574,   575,   576,   431,   682,   436,    74,   460,   476,   476,
     437,   455,   461,   465,   479,   480,   102,   481,   102,   289,
     696,    75,   102,    76,   447,   448,   195,    77,    78,   483,
     486,   488,    81,    82,   490,    84,    85,   512,    87,    88,
     547,   554,   555,    79,   556,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    65,   557,    66,   560,    90,
     586,    75,   567,    76,    74,   590,   492,    77,    78,   601,
     591,   606,   608,   609,   610,   611,   689,   612,   613,   698,
     614,   615,   616,    79,   617,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    65,   682,    66,   618,    90,
     619,   620,   621,   187,   476,   622,    75,   674,    76,   623,
     476,   494,    77,    78,   741,   634,   635,   644,   636,   107,
     637,   638,   694,   639,   640,   740,   641,   642,    79,   643,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      65,   645,    66,   646,    90,   647,   648,   649,   653,   654,
     476,   432,   650,   655,   656,   697,   657,   658,   659,   107,
     660,   699,   661,   662,    75,   732,    76,   663,   664,   776,
      77,    78,   689,   665,   666,   667,   670,   748,   707,   708,
     750,   683,   476,   738,   700,   787,    79,   101,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    65,   701,
      66,   709,    90,   710,   711,   758,   714,    74,     9,    10,
      11,    12,    13,   744,   745,    14,    15,   746,   767,   760,
      16,    17,    18,   747,   749,   753,   754,   101,   755,   756,
     757,   759,   734,   102,   770,   771,   733,   774,   775,   783,
     471,   789,   778,   788,   790,   157,   782,   791,   795,   158,
     767,   496,   497,   498,   499,   500,   501,   502,   504,   505,
     506,   507,   508,   509,   510,   511,   162,   631,   128,   160,
     730,   731,   773,   102,   772,   521,   522,   523,   524,   525,
     526,   527,   528,   529,   530,   531,   532,   533,   534,   535,
     536,   537,   539,   540,   541,   542,   543,   544,   545,   546,
     786,   548,   549,   550,   551,   552,   553,    75,   684,    76,
     482,   558,   559,    77,    78,   472,   728,   565,   602,   355,
     566,   603,     0,     0,     0,     0,     0,     0,     0,    79,
       0,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    65,   449,    66,     0,    90,   450,     0,     0,     0,
       0,     0,   432,     0,     0,     0,     0,     0,     0,     0,
     595,     0,     0,     0,     0,   208,     0,     0,     0,     0,
       0,     0,   209,   762,   763,   210,     0,   211,   212,   213,
     214,   215,   216,   217,   218,   219,   220,   221,   222,   223,
       0,     0,     0,     0,   626,   627,   628,   629,   630,   539,
     632,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   231,   167,   232,     0,
       0,     0,     0,     0,     0,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   251,   252,     0,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,     0,
     672,   266,   267,   268,   269,     0,   471,     0,     0,     0,
       0,     0,    77,    78,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     702,   703,   704,   705,     0,     0,     0,     0,     0,     0,
      65,   274,    66,   275,   706,     0,     0,     0,     0,     0,
       0,     0,   675,     0,     0,   715,     0,    75,     0,    76,
       0,     0,     0,    77,    78,   716,     0,     0,   717,   718,
     719,     0,   720,     0,   721,   722,   723,     0,     0,    79,
       0,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    65,     0,    66,     0,    90,     0,   727,     0,     0,
       0,     0,     0,     0,     0,     0,   729,     0,     0,    75,
     171,    76,     0,     0,     0,    77,    78,     0,    79,   739,
      80,    81,    82,    83,    84,    85,    86,    87,    88,   677,
      65,    79,    66,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    65,     0,    66,     0,    90,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   777,     0,     0,     0,     0,   208,     0,
       0,   779,   780,     0,     0,   209,     0,     0,   210,   781,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,     0,     0,   224,   225,   226,   227,   228,
     229,   230,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   794,     0,     0,     0,     0,     0,   231,
       0,   232,     0,     0,     0,     0,     0,     0,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,     0,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,     0,     0,   266,   267,   268,   269,     0,   270,
     171,   271,     0,     0,     0,    77,    78,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   272,   273,     0,     0,
       0,    79,     0,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    65,   274,    66,   275,    90,   208,     0,
       0,   276,     0,     0,     0,   209,     0,     0,   210,     0,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,     0,     0,   224,   225,   226,   227,   228,
     229,   230,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   231,
       0,   232,     0,     0,     0,     0,     0,     0,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,     0,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,     0,     0,   266,   267,   268,   269,     0,   270,
     503,   271,     0,     0,     0,    77,    78,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   272,   273,     0,     0,
       0,    79,     0,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    65,   274,    66,   275,    90,   208,     0,
       0,   276,     0,     0,     0,   209,     0,     0,   210,     0,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,     0,     0,   224,   225,   226,   227,   228,
     229,   230,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   231,
       0,   232,     0,     0,     0,     0,     0,     0,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,     0,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,     0,     0,   266,   267,   268,   269,     0,   270,
       0,   271,     0,     0,     0,    77,    78,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   272,   273,   624,     0,
       0,    79,     0,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    65,   274,    66,   275,    90,   208,     0,
       0,   276,     0,     0,     0,   209,     0,     0,   210,     0,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,     0,     0,   224,   225,   226,   227,   228,
     229,   230,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   231,
       0,   232,     0,     0,     0,     0,     0,     0,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,     0,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,     0,     0,   266,   267,   268,   269,     0,   270,
       0,   271,     0,     0,     0,    77,    78,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   272,   273,     0,     0,
       0,    79,     0,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    65,   274,    66,   275,    90,   208,     0,
       0,   276,     0,     0,     0,   209,     0,     0,   210,     0,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,     0,     0,   224,   225,   226,   227,   228,
     229,   230,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   231,
       0,   232,     0,     0,     0,     0,     0,     0,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,     0,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,     0,     0,   266,   267,   268,   269,     0,   270,
       0,   271,     0,     0,     0,    77,    78,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    79,     0,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    65,   274,    66,   275,    90,   208,     0,
       0,     0,     0,     0,     0,   209,     0,     0,   210,     0,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   231,
       0,   232,     0,     0,     0,     0,     0,     0,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,     0,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,     0,     0,   266,   267,   268,   269,     0,   685,
     208,     0,     0,     0,     0,    77,    78,   209,     0,     0,
     210,     0,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,     0,     0,     0,     0,     0,
       0,     0,     0,    65,   274,    66,   275,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   231,     0,   232,     0,     0,     0,     0,     0,     0,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
       0,   253,   254,   255,   256,   257,   258,   259,   260,   261,
     262,   263,   264,   265,     0,     0,   266,   267,   268,   269,
       0,   471,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    75,   347,    76,
       0,     0,     0,    77,    78,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    65,   274,    66,   275,    79,
       0,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    65,    75,    66,    76,    90,     0,     0,    77,    78,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    79,     0,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    65,     0,    66,     0,
      90
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-534))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      41,    33,   114,    40,    36,    93,    78,    52,    40,   325,
      34,    71,   392,    73,    73,   141,   325,   343,     4,   149,
      52,    75,    75,   325,    45,     0,     0,   115,    17,    74,
      60,   153,    64,   321,   156,    18,   440,    75,    61,    71,
     162,    73,    74,    75,    76,    13,    14,    15,    16,    17,
      18,    49,    50,   586,   114,   114,   586,    17,   188,     4,
       5,    93,     5,    57,   186,    97,    76,     4,   100,     0,
       4,     5,    36,   116,   116,    15,    40,     5,   151,     0,
     153,    56,   114,   115,    15,   109,   151,    51,    52,   125,
     100,    38,    14,     4,    41,    12,    61,     3,   116,   117,
     145,    61,    27,    20,   140,   123,   124,    71,    56,    73,
      74,    75,    76,   145,   157,   157,   151,   149,    36,   173,
     173,   151,    40,    57,    52,   179,   179,   121,   151,    93,
     116,    54,   157,    97,    57,   173,   100,   123,   124,   121,
     164,   173,   121,    10,    11,   678,    57,   179,   678,   121,
     114,   115,   184,    71,   195,    73,   188,    75,   484,   196,
     192,   198,   151,   567,   196,   151,   198,   153,   151,   457,
     153,   459,   136,   137,   462,    93,   121,   141,   121,   305,
     201,   145,   157,   157,   121,   149,   151,   121,   151,    27,
     154,   151,   314,   121,     4,   120,   114,   115,   123,   124,
     116,   581,   118,   150,    72,   203,   204,   205,   158,   173,
     121,   117,   156,   119,   156,   179,   346,   123,   124,   123,
     124,   144,   145,   116,   188,   118,   151,   125,   153,   152,
     121,   149,   196,   139,   198,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   151,   153,   360,   155,
     117,    62,   156,   117,    62,   173,   123,   124,   117,   123,
     124,   179,     5,   308,   123,   124,   307,    62,   309,   136,
     188,   359,   136,   361,   590,   591,   308,   151,   196,   324,
     198,   590,   591,   343,   121,   123,   124,    70,    71,   591,
      17,    18,   324,   325,   272,   273,   122,     6,   276,   323,
     360,   360,   116,   327,   118,   123,   124,   331,    17,   122,
      19,   343,   158,   151,   346,   153,   386,   387,   388,   389,
     390,   391,   151,   123,   124,    51,    52,   359,   360,   361,
     116,   461,   118,   151,    61,   153,   136,   136,   137,    66,
     121,   305,   123,   124,   308,   128,   129,   130,   131,   132,
     133,   151,   316,   153,   116,   122,   118,     3,   326,   120,
     324,     7,     8,     9,   436,    74,    75,   116,   122,   118,
     686,   123,   124,   121,    17,   117,   692,   686,   117,   343,
     136,   137,   346,   692,   117,   455,   674,    96,     9,    10,
     692,   120,   117,   451,   452,   359,   360,   361,   117,   117,
     117,   117,   111,   117,   117,   369,    51,    52,    53,    54,
      55,   117,   121,    58,    59,   117,   732,   117,    63,    64,
      65,   117,   117,   732,   484,   343,   486,   486,   346,   461,
     732,   117,   477,   117,   117,   117,   490,   490,   117,   117,
     117,   359,   360,   361,   117,   477,   470,   117,   764,   117,
     117,   492,   484,   494,   486,   764,   488,   481,   490,   117,
     762,   763,   764,   117,    73,   110,   117,   117,   117,   117,
     117,    17,   127,   117,   117,   117,   119,   117,   117,   122,
     123,   124,   117,   117,   117,   117,   117,   117,   117,   561,
     562,   117,   117,   117,   117,   117,   139,   461,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   118,
     153,    17,   155,   477,   117,   117,   480,   117,   117,   117,
     484,   117,   486,   117,   488,   117,   490,   117,   117,   117,
     139,   117,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   461,   153,   441,   442,   443,   444,   445,
     446,   447,   448,   117,   586,    68,    17,   122,   590,   591,
     126,   117,   121,    41,   117,   117,   484,    13,   486,   138,
     594,   117,   490,   119,   134,   135,   122,   123,   124,   158,
     121,   157,   142,   143,   116,   145,   146,    10,   148,   149,
     118,   118,   118,   139,   118,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   118,   153,   118,   155,
     117,   117,    71,   119,    17,    22,   122,   123,   124,   122,
      42,   122,   118,   116,   116,   116,   590,   116,   118,   138,
     118,   118,   118,   139,   118,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   678,   153,   118,   155,
     118,   118,   118,   741,   686,   118,   117,   122,   119,   116,
     692,   122,   123,   124,   701,   116,   118,   116,   118,   701,
     118,   118,    21,   118,   118,   699,   118,   118,   139,   118,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   118,   153,   118,   155,   116,   116,   116,   116,   116,
     732,   270,   118,   116,   118,    68,   118,   118,   118,   741,
     118,    13,   118,   118,   117,    22,   119,   118,   118,   122,
     123,   124,   686,   118,   118,   118,   118,    50,   118,   118,
     116,   122,   764,   697,   122,   776,   139,   701,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   121,
     153,   118,   155,   118,   118,   116,   118,    17,    51,    52,
      53,    54,    55,   118,   118,    58,    59,   118,   732,    68,
      63,    64,    65,   118,   118,   118,   118,   741,   118,   118,
     118,   118,    20,   701,   144,   144,    12,   118,   118,   118,
     117,   139,   128,   118,   118,    61,   760,   118,   118,    61,
     764,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   383,   384,    71,   518,    46,    64,
     686,   692,   737,   741,   736,   394,   395,   396,   397,   398,
     399,   400,   401,   402,   403,   404,   405,   406,   407,   408,
     409,   410,   411,   412,   413,   414,   415,   416,   417,   418,
     764,   420,   421,   422,   423,   424,   425,   117,   588,   119,
     331,   430,   431,   123,   124,   325,   678,   437,   488,   188,
     438,   490,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,
      -1,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   294,   153,    -1,   155,   294,    -1,    -1,    -1,
      -1,    -1,   471,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     479,    -1,    -1,    -1,    -1,    16,    -1,    -1,    -1,    -1,
      -1,    -1,    23,    24,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      -1,    -1,    -1,    -1,   513,   514,   515,   516,   517,   518,
     519,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    67,    17,    69,    -1,
      -1,    -1,    -1,    -1,    -1,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    -1,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,    -1,
     579,   112,   113,   114,   115,    -1,   117,    -1,    -1,    -1,
      -1,    -1,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     609,   610,   611,   612,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   623,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    73,    -1,    -1,   634,    -1,   117,    -1,   119,
      -1,    -1,    -1,   123,   124,   644,    -1,    -1,   647,   648,
     649,    -1,   651,    -1,   653,   654,   655,    -1,    -1,   139,
      -1,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,    -1,   153,    -1,   155,    -1,   118,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   685,    -1,    -1,   117,
     118,   119,    -1,    -1,    -1,   123,   124,    -1,   139,   698,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   139,   153,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,    -1,   153,    -1,   155,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   742,    -1,    -1,    -1,    -1,    16,    -1,
      -1,   750,   751,    -1,    -1,    23,    -1,    -1,    26,   758,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    44,    45,    46,    47,
      48,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   792,    -1,    -1,    -1,    -1,    -1,    67,
      -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,   112,   113,   114,   115,    -1,   117,
     118,   119,    -1,    -1,    -1,   123,   124,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   134,   135,    -1,    -1,
      -1,   139,    -1,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,    16,    -1,
      -1,   159,    -1,    -1,    -1,    23,    -1,    -1,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    44,    45,    46,    47,
      48,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    67,
      -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,   112,   113,   114,   115,    -1,   117,
     118,   119,    -1,    -1,    -1,   123,   124,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   134,   135,    -1,    -1,
      -1,   139,    -1,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,    16,    -1,
      -1,   159,    -1,    -1,    -1,    23,    -1,    -1,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    44,    45,    46,    47,
      48,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    67,
      -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,   112,   113,   114,   115,    -1,   117,
      -1,   119,    -1,    -1,    -1,   123,   124,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   134,   135,   136,    -1,
      -1,   139,    -1,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,    16,    -1,
      -1,   159,    -1,    -1,    -1,    23,    -1,    -1,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    44,    45,    46,    47,
      48,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    67,
      -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,   112,   113,   114,   115,    -1,   117,
      -1,   119,    -1,    -1,    -1,   123,   124,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   134,   135,    -1,    -1,
      -1,   139,    -1,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,    16,    -1,
      -1,   159,    -1,    -1,    -1,    23,    -1,    -1,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    43,    44,    45,    46,    47,
      48,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    67,
      -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,   112,   113,   114,   115,    -1,   117,
      -1,   119,    -1,    -1,    -1,   123,   124,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   139,    -1,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,    16,    -1,
      -1,    -1,    -1,    -1,    -1,    23,    -1,    -1,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    67,
      -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    -1,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,   112,   113,   114,   115,    -1,   117,
      16,    -1,    -1,    -1,    -1,   123,   124,    23,    -1,    -1,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   151,   152,   153,   154,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    67,    -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      -1,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,    -1,    -1,   112,   113,   114,   115,
      -1,   117,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,   119,
      -1,    -1,    -1,   123,   124,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   139,
      -1,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   117,   153,   119,   155,    -1,    -1,   123,   124,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   139,    -1,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,    -1,   153,    -1,
     155
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,    15,   161,   162,   165,   168,   169,   151,     0,    51,
      52,    53,    54,    55,    58,    59,    63,    64,    65,   110,
     163,   167,   195,   200,   201,   203,   205,   206,   210,   211,
     212,   213,   170,     4,     5,    57,   121,   193,   214,    57,
     121,   214,   151,    56,   204,   204,   204,   204,   204,   204,
     204,     3,     7,     8,     9,   164,   171,   173,   189,   190,
     192,   157,   166,    14,    18,   151,   153,   216,   217,   218,
     308,   121,   243,   121,    17,   117,   119,   123,   124,   139,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     155,   197,   198,   199,   241,   269,   270,   278,   279,   280,
     283,   285,   288,   304,   305,   306,   307,   308,   309,     5,
     193,   219,   220,   243,   121,   199,   220,    51,    52,    17,
      18,    61,    66,   194,   202,    17,   151,   194,   202,   151,
     207,    61,   151,   208,   208,   208,    10,    11,   117,   136,
     174,   175,   176,   285,   136,   191,   284,   285,   308,   121,
     214,   215,   267,   215,    72,   235,   215,   165,   168,   156,
     218,   172,   173,   244,   245,   250,   270,    17,   196,   250,
     284,   118,   278,   281,   282,   283,    27,   120,   271,   277,
     284,   156,   286,   286,   125,   140,   122,   198,   158,   271,
     273,   271,   125,   243,   196,   122,   121,    57,   121,    61,
     151,    60,   151,    62,    62,    62,   174,   174,    16,    23,
      26,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    43,    44,    45,    46,    47,    48,
      49,    67,    69,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   112,   113,   114,   115,
     117,   119,   134,   135,   152,   154,   159,   177,   180,   181,
     182,   183,   184,   187,   188,   263,   264,   285,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   308,   116,   176,   215,   116,   215,
     284,   268,   269,     5,   215,   219,   236,   285,   219,   151,
     122,   219,   122,     6,    17,    19,    74,    75,    96,   111,
     243,   246,   247,   248,   251,   252,   253,   254,   255,   256,
     258,   259,   260,   158,   151,   122,   121,   118,   282,   120,
     274,   276,   282,   308,   219,   268,   308,   122,   220,   199,
     121,   199,    61,   151,   194,   209,   208,   208,   208,   117,
     117,   117,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   265,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   289,   296,   296,   296,    68,   126,   127,    70,
      71,   128,   129,   130,   131,   132,   133,   134,   135,   306,
     307,   136,   137,   266,   289,   117,   176,   220,   284,   220,
     122,   121,   219,   121,   285,    41,   221,   225,   221,   243,
     284,   117,   261,   263,   297,   298,   308,   204,   243,   117,
     117,    13,   247,   158,   249,   245,   121,   269,   157,   272,
     116,   275,   122,   196,   122,   285,   289,   289,   289,   289,
     289,   289,   289,   118,   289,   289,   289,   289,   289,   289,
     289,   289,    10,   178,   178,   178,   178,   178,   178,   178,
     266,   289,   289,   289,   289,   289,   289,   289,   289,   289,
     289,   289,   289,   289,   289,   289,   289,   289,   186,   289,
     289,   289,   289,   289,   289,   289,   289,   118,   289,   289,
     289,   289,   289,   289,   118,   118,   118,   118,   289,   289,
     118,   123,   124,   286,   287,   291,   292,    71,   265,   293,
     293,   293,   293,   293,   293,   293,   293,   294,   294,   116,
     118,   178,   221,   221,   269,   221,   117,   237,   238,   239,
      22,    42,   228,   243,   284,   289,   285,   243,   257,   245,
     250,   122,   273,   274,    52,   220,   122,   220,   118,   116,
     116,   116,   116,   118,   118,   118,   118,   118,   118,   118,
     118,   118,   118,   116,   136,   179,   289,   289,   289,   289,
     289,   186,   289,   118,   116,   118,   118,   118,   118,   118,
     118,   118,   118,   118,   116,   118,   118,   116,   116,   116,
     118,   116,   118,   116,   116,   116,   118,   118,   118,   118,
     118,   118,   118,   118,   118,   118,   118,   118,   286,   286,
     118,   265,   289,   266,   122,    73,   118,   150,   240,   241,
     242,   304,   308,   122,   239,   117,   222,   224,   263,   285,
     298,   226,   227,   261,    21,   230,   243,    68,   138,    13,
     122,   121,   289,   289,   289,   289,   289,   118,   118,   118,
     118,   118,   157,   185,   118,   289,   289,   289,   289,   289,
     289,   289,   289,   289,   157,   262,   221,   118,   242,   289,
     224,   226,    22,    12,    20,   229,   233,   234,   285,   289,
     243,   199,   116,   118,   118,   118,   118,   118,    50,   118,
     116,   116,   118,   118,   118,   118,   118,   118,   116,   118,
      68,   223,    24,    25,   231,   232,   263,   285,   297,   298,
     144,   144,   234,   233,   118,   118,   122,   289,   128,   289,
     289,   289,   285,   118,   297,   297,   232,   220,   118,   139,
     118,   118,   116,   118,   289,   118
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
      case 139: /* "\"string\"" */

/* Line 1391 of yacc.c  */
#line 369 "./sparql_parser.y"
	{
  if((yyvaluep->name))
    RASQAL_FREE(char*, (yyvaluep->name));
};

/* Line 1391 of yacc.c  */
#line 2535 "sparql_parser.c"
	break;
      case 141: /* "\"double literal\"" */

/* Line 1391 of yacc.c  */
#line 348 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2547 "sparql_parser.c"
	break;
      case 142: /* "\"double positive literal\"" */

/* Line 1391 of yacc.c  */
#line 348 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2559 "sparql_parser.c"
	break;
      case 143: /* "\"double negative literal\"" */

/* Line 1391 of yacc.c  */
#line 348 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2571 "sparql_parser.c"
	break;
      case 144: /* "\"integer literal\"" */

/* Line 1391 of yacc.c  */
#line 348 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2583 "sparql_parser.c"
	break;
      case 145: /* "\"integer positive literal\"" */

/* Line 1391 of yacc.c  */
#line 348 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2595 "sparql_parser.c"
	break;
      case 146: /* "\"integer negative literal\"" */

/* Line 1391 of yacc.c  */
#line 348 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2607 "sparql_parser.c"
	break;
      case 147: /* "\"decimal literal\"" */

/* Line 1391 of yacc.c  */
#line 348 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2619 "sparql_parser.c"
	break;
      case 148: /* "\"decimal positive literal\"" */

/* Line 1391 of yacc.c  */
#line 348 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2631 "sparql_parser.c"
	break;
      case 149: /* "\"decimal negative literal\"" */

/* Line 1391 of yacc.c  */
#line 348 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2643 "sparql_parser.c"
	break;
      case 150: /* "\"boolean literal\"" */

/* Line 1391 of yacc.c  */
#line 348 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2655 "sparql_parser.c"
	break;
      case 151: /* "\"URI literal\"" */

/* Line 1391 of yacc.c  */
#line 358 "./sparql_parser.y"
	{
  if((yyvaluep->uri))
    raptor_free_uri((yyvaluep->uri));
};

/* Line 1391 of yacc.c  */
#line 2667 "sparql_parser.c"
	break;
      case 152: /* "\"URI literal (\"" */

/* Line 1391 of yacc.c  */
#line 358 "./sparql_parser.y"
	{
  if((yyvaluep->uri))
    raptor_free_uri((yyvaluep->uri));
};

/* Line 1391 of yacc.c  */
#line 2679 "sparql_parser.c"
	break;
      case 153: /* "\"QName literal\"" */

/* Line 1391 of yacc.c  */
#line 369 "./sparql_parser.y"
	{
  if((yyvaluep->name))
    RASQAL_FREE(char*, (yyvaluep->name));
};

/* Line 1391 of yacc.c  */
#line 2691 "sparql_parser.c"
	break;
      case 154: /* "\"QName literal (\"" */

/* Line 1391 of yacc.c  */
#line 369 "./sparql_parser.y"
	{
  if((yyvaluep->name))
    RASQAL_FREE(char*, (yyvaluep->name));
};

/* Line 1391 of yacc.c  */
#line 2703 "sparql_parser.c"
	break;
      case 155: /* "\"blank node literal\"" */

/* Line 1391 of yacc.c  */
#line 369 "./sparql_parser.y"
	{
  if((yyvaluep->name))
    RASQAL_FREE(char*, (yyvaluep->name));
};

/* Line 1391 of yacc.c  */
#line 2715 "sparql_parser.c"
	break;
      case 156: /* "\"identifier\"" */

/* Line 1391 of yacc.c  */
#line 369 "./sparql_parser.y"
	{
  if((yyvaluep->name))
    RASQAL_FREE(char*, (yyvaluep->name));
};

/* Line 1391 of yacc.c  */
#line 2727 "sparql_parser.c"
	break;
      case 171: /* "SelectQuery" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 2739 "sparql_parser.c"
	break;
      case 172: /* "SubSelect" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 2751 "sparql_parser.c"
	break;
      case 173: /* "SelectClause" */

/* Line 1391 of yacc.c  */
#line 473 "./sparql_parser.y"
	{
  if((yyvaluep->projection))
    rasqal_free_projection((yyvaluep->projection));
};

/* Line 1391 of yacc.c  */
#line 2763 "sparql_parser.c"
	break;
      case 174: /* "SelectExpressionList" */

/* Line 1391 of yacc.c  */
#line 473 "./sparql_parser.y"
	{
  if((yyvaluep->projection))
    rasqal_free_projection((yyvaluep->projection));
};

/* Line 1391 of yacc.c  */
#line 2775 "sparql_parser.c"
	break;
      case 175: /* "SelectExpressionListTail" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 2787 "sparql_parser.c"
	break;
      case 176: /* "SelectTerm" */

/* Line 1391 of yacc.c  */
#line 449 "./sparql_parser.y"
	{
  if((yyvaluep->variable))
    rasqal_free_variable((yyvaluep->variable));
};

/* Line 1391 of yacc.c  */
#line 2799 "sparql_parser.c"
	break;
      case 177: /* "AggregateExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 2811 "sparql_parser.c"
	break;
      case 179: /* "ExpressionOrStar" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 2823 "sparql_parser.c"
	break;
      case 180: /* "CountAggregateExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 2835 "sparql_parser.c"
	break;
      case 181: /* "SumAggregateExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 2847 "sparql_parser.c"
	break;
      case 182: /* "AvgAggregateExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 2859 "sparql_parser.c"
	break;
      case 183: /* "MinAggregateExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 2871 "sparql_parser.c"
	break;
      case 184: /* "MaxAggregateExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 2883 "sparql_parser.c"
	break;
      case 185: /* "SeparatorOpt" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 2895 "sparql_parser.c"
	break;
      case 187: /* "GroupConcatAggregateExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 2907 "sparql_parser.c"
	break;
      case 188: /* "SampleAggregateExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 2919 "sparql_parser.c"
	break;
      case 189: /* "ConstructQuery" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 2931 "sparql_parser.c"
	break;
      case 190: /* "DescribeQuery" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 2943 "sparql_parser.c"
	break;
      case 191: /* "VarOrIRIrefList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 2955 "sparql_parser.c"
	break;
      case 193: /* "DatasetClause" */

/* Line 1391 of yacc.c  */
#line 455 "./sparql_parser.y"
	{
  if((yyvaluep->data_graph))
    rasqal_free_data_graph((yyvaluep->data_graph));
};

/* Line 1391 of yacc.c  */
#line 2967 "sparql_parser.c"
	break;
      case 196: /* "GraphTriples" */

/* Line 1391 of yacc.c  */
#line 392 "./sparql_parser.y"
	{
  if((yyvaluep->update))
    rasqal_free_update_operation((yyvaluep->update));
};

/* Line 1391 of yacc.c  */
#line 2979 "sparql_parser.c"
	break;
      case 197: /* "GraphTemplate" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 2991 "sparql_parser.c"
	break;
      case 198: /* "ModifyTemplate" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3003 "sparql_parser.c"
	break;
      case 199: /* "ModifyTemplateList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3015 "sparql_parser.c"
	break;
      case 202: /* "GraphRefAll" */

/* Line 1391 of yacc.c  */
#line 364 "./sparql_parser.y"
	{
  if((yyvaluep->uri_applies))
    free_uri_applies((yyvaluep->uri_applies));
};

/* Line 1391 of yacc.c  */
#line 3027 "sparql_parser.c"
	break;
      case 207: /* "IriRefList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3039 "sparql_parser.c"
	break;
      case 214: /* "DatasetClauseList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3051 "sparql_parser.c"
	break;
      case 215: /* "DatasetClauseListOpt" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3063 "sparql_parser.c"
	break;
      case 216: /* "DefaultGraphClause" */

/* Line 1391 of yacc.c  */
#line 455 "./sparql_parser.y"
	{
  if((yyvaluep->data_graph))
    rasqal_free_data_graph((yyvaluep->data_graph));
};

/* Line 1391 of yacc.c  */
#line 3075 "sparql_parser.c"
	break;
      case 217: /* "NamedGraphClause" */

/* Line 1391 of yacc.c  */
#line 455 "./sparql_parser.y"
	{
  if((yyvaluep->data_graph))
    rasqal_free_data_graph((yyvaluep->data_graph));
};

/* Line 1391 of yacc.c  */
#line 3087 "sparql_parser.c"
	break;
      case 218: /* "SourceSelector" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 3099 "sparql_parser.c"
	break;
      case 221: /* "SolutionModifier" */

/* Line 1391 of yacc.c  */
#line 467 "./sparql_parser.y"
	{
  if((yyvaluep->modifier))
    rasqal_free_solution_modifier((yyvaluep->modifier));
};

/* Line 1391 of yacc.c  */
#line 3111 "sparql_parser.c"
	break;
      case 222: /* "GroupConditionList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3123 "sparql_parser.c"
	break;
      case 223: /* "AsVarOpt" */

/* Line 1391 of yacc.c  */
#line 449 "./sparql_parser.y"
	{
  if((yyvaluep->variable))
    rasqal_free_variable((yyvaluep->variable));
};

/* Line 1391 of yacc.c  */
#line 3135 "sparql_parser.c"
	break;
      case 224: /* "GroupCondition" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3147 "sparql_parser.c"
	break;
      case 225: /* "GroupClauseOpt" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3159 "sparql_parser.c"
	break;
      case 226: /* "HavingCondition" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3171 "sparql_parser.c"
	break;
      case 227: /* "HavingConditionList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3183 "sparql_parser.c"
	break;
      case 228: /* "HavingClauseOpt" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3195 "sparql_parser.c"
	break;
      case 230: /* "OrderClauseOpt" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3207 "sparql_parser.c"
	break;
      case 231: /* "OrderConditionList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3219 "sparql_parser.c"
	break;
      case 232: /* "OrderCondition" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3231 "sparql_parser.c"
	break;
      case 235: /* "BindingsClauseOpt" */

/* Line 1391 of yacc.c  */
#line 480 "./sparql_parser.y"
	{
  if((yyvaluep->bindings))
    rasqal_free_bindings((yyvaluep->bindings));
};

/* Line 1391 of yacc.c  */
#line 3243 "sparql_parser.c"
	break;
      case 236: /* "VarList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3255 "sparql_parser.c"
	break;
      case 237: /* "BindingsRowListOpt" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3267 "sparql_parser.c"
	break;
      case 238: /* "BindingsRowList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3279 "sparql_parser.c"
	break;
      case 239: /* "BindingsRow" */

/* Line 1391 of yacc.c  */
#line 461 "./sparql_parser.y"
	{
  if((yyvaluep->row))
    rasqal_free_row((yyvaluep->row));
};

/* Line 1391 of yacc.c  */
#line 3291 "sparql_parser.c"
	break;
      case 240: /* "BindingValueList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3303 "sparql_parser.c"
	break;
      case 241: /* "RDFLiteral" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 3315 "sparql_parser.c"
	break;
      case 242: /* "BindingValue" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 3327 "sparql_parser.c"
	break;
      case 243: /* "GroupGraphPattern" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3339 "sparql_parser.c"
	break;
      case 244: /* "GroupGraphPatternSub" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3351 "sparql_parser.c"
	break;
      case 245: /* "TriplesBlockOpt" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3363 "sparql_parser.c"
	break;
      case 246: /* "GraphPatternListOpt" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3375 "sparql_parser.c"
	break;
      case 247: /* "GraphPatternList" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3387 "sparql_parser.c"
	break;
      case 248: /* "GraphPatternListFilter" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3399 "sparql_parser.c"
	break;
      case 250: /* "TriplesBlock" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3411 "sparql_parser.c"
	break;
      case 251: /* "GraphPatternNotTriples" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3423 "sparql_parser.c"
	break;
      case 252: /* "OptionalGraphPattern" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3435 "sparql_parser.c"
	break;
      case 253: /* "GraphGraphPattern" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3447 "sparql_parser.c"
	break;
      case 254: /* "ServiceGraphPattern" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3459 "sparql_parser.c"
	break;
      case 255: /* "MinusGraphPattern" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3471 "sparql_parser.c"
	break;
      case 256: /* "GroupOrUnionGraphPattern" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3483 "sparql_parser.c"
	break;
      case 257: /* "GroupOrUnionGraphPatternList" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3495 "sparql_parser.c"
	break;
      case 258: /* "LetGraphPattern" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3507 "sparql_parser.c"
	break;
      case 259: /* "BindGraphPattern" */

/* Line 1391 of yacc.c  */
#line 409 "./sparql_parser.y"
	{
  if((yyvaluep->graph_pattern))
    rasqal_free_graph_pattern((yyvaluep->graph_pattern));
};

/* Line 1391 of yacc.c  */
#line 3519 "sparql_parser.c"
	break;
      case 260: /* "Filter" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3531 "sparql_parser.c"
	break;
      case 261: /* "Constraint" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3543 "sparql_parser.c"
	break;
      case 262: /* "ParamsOpt" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3555 "sparql_parser.c"
	break;
      case 263: /* "FunctionCall" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3567 "sparql_parser.c"
	break;
      case 264: /* "CoalesceExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3579 "sparql_parser.c"
	break;
      case 265: /* "ArgList" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3591 "sparql_parser.c"
	break;
      case 266: /* "ArgListNoBraces" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3603 "sparql_parser.c"
	break;
      case 267: /* "ConstructTemplate" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3615 "sparql_parser.c"
	break;
      case 268: /* "ConstructTriplesOpt" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3627 "sparql_parser.c"
	break;
      case 269: /* "ConstructTriples" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3639 "sparql_parser.c"
	break;
      case 270: /* "TriplesSameSubject" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3651 "sparql_parser.c"
	break;
      case 271: /* "PropertyListNotEmpty" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3663 "sparql_parser.c"
	break;
      case 272: /* "PropertyListTailOpt" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3675 "sparql_parser.c"
	break;
      case 273: /* "PropertyList" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3687 "sparql_parser.c"
	break;
      case 274: /* "ObjectList" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3699 "sparql_parser.c"
	break;
      case 275: /* "ObjectTail" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3711 "sparql_parser.c"
	break;
      case 276: /* "Object" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3723 "sparql_parser.c"
	break;
      case 277: /* "Verb" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3735 "sparql_parser.c"
	break;
      case 278: /* "TriplesNode" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3747 "sparql_parser.c"
	break;
      case 279: /* "BlankNodePropertyList" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3759 "sparql_parser.c"
	break;
      case 280: /* "Collection" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3771 "sparql_parser.c"
	break;
      case 281: /* "GraphNodeListNotEmpty" */

/* Line 1391 of yacc.c  */
#line 375 "./sparql_parser.y"
	{
  if((yyvaluep->seq))
    raptor_free_sequence((yyvaluep->seq));
};

/* Line 1391 of yacc.c  */
#line 3783 "sparql_parser.c"
	break;
      case 282: /* "GraphNode" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3795 "sparql_parser.c"
	break;
      case 283: /* "VarOrTerm" */

/* Line 1391 of yacc.c  */
#line 398 "./sparql_parser.y"
	{
  if((yyvaluep->formula))
    rasqal_free_formula((yyvaluep->formula));
};

/* Line 1391 of yacc.c  */
#line 3807 "sparql_parser.c"
	break;
      case 284: /* "VarOrIRIref" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 3819 "sparql_parser.c"
	break;
      case 285: /* "Var" */

/* Line 1391 of yacc.c  */
#line 449 "./sparql_parser.y"
	{
  if((yyvaluep->variable))
    rasqal_free_variable((yyvaluep->variable));
};

/* Line 1391 of yacc.c  */
#line 3831 "sparql_parser.c"
	break;
      case 286: /* "VarName" */

/* Line 1391 of yacc.c  */
#line 449 "./sparql_parser.y"
	{
  if((yyvaluep->variable))
    rasqal_free_variable((yyvaluep->variable));
};

/* Line 1391 of yacc.c  */
#line 3843 "sparql_parser.c"
	break;
      case 288: /* "GraphTerm" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 3855 "sparql_parser.c"
	break;
      case 289: /* "Expression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3867 "sparql_parser.c"
	break;
      case 290: /* "ConditionalOrExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3879 "sparql_parser.c"
	break;
      case 291: /* "ConditionalAndExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3891 "sparql_parser.c"
	break;
      case 292: /* "RelationalExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3903 "sparql_parser.c"
	break;
      case 293: /* "AdditiveExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3915 "sparql_parser.c"
	break;
      case 294: /* "MultiplicativeExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3927 "sparql_parser.c"
	break;
      case 295: /* "UnaryExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3939 "sparql_parser.c"
	break;
      case 296: /* "PrimaryExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3951 "sparql_parser.c"
	break;
      case 297: /* "BrackettedExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3963 "sparql_parser.c"
	break;
      case 298: /* "BuiltInCall" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3975 "sparql_parser.c"
	break;
      case 300: /* "RegexExpression" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3987 "sparql_parser.c"
	break;
      case 301: /* "DatetimeBuiltinAccessors" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 3999 "sparql_parser.c"
	break;
      case 302: /* "DatetimeExtensions" */

/* Line 1391 of yacc.c  */
#line 421 "./sparql_parser.y"
	{
  if((yyvaluep->expr))
    rasqal_free_expression((yyvaluep->expr));
};

/* Line 1391 of yacc.c  */
#line 4011 "sparql_parser.c"
	break;
      case 303: /* "IRIrefBrace" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 4023 "sparql_parser.c"
	break;
      case 304: /* "NumericLiteral" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 4035 "sparql_parser.c"
	break;
      case 305: /* "NumericLiteralUnsigned" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 4047 "sparql_parser.c"
	break;
      case 306: /* "NumericLiteralPositive" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 4059 "sparql_parser.c"
	break;
      case 307: /* "NumericLiteralNegative" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 4071 "sparql_parser.c"
	break;
      case 308: /* "IRIref" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 4083 "sparql_parser.c"
	break;
      case 309: /* "BlankNode" */

/* Line 1391 of yacc.c  */
#line 438 "./sparql_parser.y"
	{
  if((yyvaluep->literal))
    rasqal_free_literal((yyvaluep->literal));
};

/* Line 1391 of yacc.c  */
#line 4095 "sparql_parser.c"
	break;

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:

/* Line 1806 of yacc.c  */
#line 502 "./sparql_parser.y"
    {
  if((yyvsp[(4) - (4)].bindings))
    ((rasqal_query*)rq)->bindings = (yyvsp[(4) - (4)].bindings);
}
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 511 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(sparql->experimental)
    ((rasqal_query*)rq)->explain = 1;
  else {
    sparql_syntax_error((rasqal_query*)rq, 
                        "EXPLAIN can only used with LAQRS");
    YYERROR;
  }
}
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 524 "./sparql_parser.y"
    {
  /* nothing to do */
}
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 532 "./sparql_parser.y"
    {
  raptor_sequence* seq;
  rasqal_graph_pattern* where_gp;

  /* Query graph pattern is first GP inside sequence of sub-GPs */
  seq = rasqal_graph_pattern_get_sub_graph_pattern_sequence((yyvsp[(1) - (1)].graph_pattern));
  where_gp = (rasqal_graph_pattern*)raptor_sequence_delete_at(seq, 0);

  rasqal_query_store_select_query(((rasqal_query*)rq),
                                  (yyvsp[(1) - (1)].graph_pattern)->projection,
                                  (yyvsp[(1) - (1)].graph_pattern)->data_graphs,
                                  where_gp,
                                  (yyvsp[(1) - (1)].graph_pattern)->modifier);
  (yyvsp[(1) - (1)].graph_pattern)->projection = NULL;
  (yyvsp[(1) - (1)].graph_pattern)->data_graphs = NULL;
  (yyvsp[(1) - (1)].graph_pattern)->modifier = NULL;

  rasqal_free_graph_pattern((yyvsp[(1) - (1)].graph_pattern));
}
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 552 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->constructs = (yyvsp[(1) - (1)].seq);
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_CONSTRUCT;
}
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 557 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->describes = (yyvsp[(1) - (1)].seq);
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_DESCRIBE;
}
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 562 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_ASK;
}
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 580 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_DELETE;
}
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 584 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_INSERT;
}
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 588 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_UPDATE;
}
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 592 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_UPDATE;
}
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 596 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_UPDATE;
}
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 600 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_UPDATE;
}
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 604 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_UPDATE;
}
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 608 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_UPDATE;
}
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 612 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_UPDATE;
}
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 616 "./sparql_parser.y"
    {
  ((rasqal_query*)rq)->verb = RASQAL_QUERY_VERB_UPDATE;
}
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 624 "./sparql_parser.y"
    {
  /* nothing to do */
}
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 632 "./sparql_parser.y"
    {
  rasqal_query_set_base_uri((rasqal_query*)rq, (yyvsp[(2) - (2)].uri));
}
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 636 "./sparql_parser.y"
    {
  /* nothing to do */
}
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 644 "./sparql_parser.y"
    {
  raptor_sequence *seq = ((rasqal_query*)rq)->prefixes;
  unsigned const char* prefix_string = (yyvsp[(3) - (4)].name);
  size_t prefix_length = 0;

  if(prefix_string)
    prefix_length = strlen(RASQAL_GOOD_CAST(const char*, prefix_string));
  
  if(raptor_namespaces_find_namespace(((rasqal_query*)rq)->namespaces,
                                      prefix_string, RASQAL_BAD_CAST(int, prefix_length))) {
    /* A prefix may be defined only once */
    sparql_syntax_warning(((rasqal_query*)rq), 
                          "PREFIX %s can be defined only once.",
                          prefix_string ? RASQAL_GOOD_CAST(const char*, prefix_string) : ":");
    RASQAL_FREE(char*, prefix_string);
    raptor_free_uri((yyvsp[(4) - (4)].uri));
  } else {
    rasqal_prefix *p;
    p = rasqal_new_prefix(((rasqal_query*)rq)->world, prefix_string, (yyvsp[(4) - (4)].uri));
    if(!p)
      YYERROR_MSG("PrefixDeclOpt: failed to create new prefix");
    if(raptor_sequence_push(seq, p))
      YYERROR_MSG("PrefixDeclOpt: cannot push prefix to seq");
    if(rasqal_query_declare_prefix(((rasqal_query*)rq), p)) {
      YYERROR_MSG("PrefixDeclOpt: cannot declare prefix");
    }
  }
}
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 673 "./sparql_parser.y"
    {
  /* nothing to do, rq->prefixes already initialised */
}
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 681 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.graph_pattern) = NULL;
  if(!sparql->sparql_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "SELECT can only be used with a SPARQL query");
    YYERROR;
  } else {
    (yyval.graph_pattern) = rasqal_new_select_graph_pattern((rasqal_query*)rq,
                                         (yyvsp[(1) - (4)].projection), (yyvsp[(2) - (4)].seq), (yyvsp[(3) - (4)].graph_pattern), (yyvsp[(4) - (4)].modifier));
  }
}
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 699 "./sparql_parser.y"
    {
  if((yyvsp[(1) - (3)].projection) && (yyvsp[(2) - (3)].graph_pattern) && (yyvsp[(3) - (3)].modifier))
    (yyval.graph_pattern) = rasqal_new_select_graph_pattern((rasqal_query*)rq,
                                         (yyvsp[(1) - (3)].projection),
                                         /* data graphs */ NULL,
                                         (yyvsp[(2) - (3)].graph_pattern),
                                         (yyvsp[(3) - (3)].modifier));
  else
    (yyval.graph_pattern) = NULL;
}
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 713 "./sparql_parser.y"
    {
  (yyval.projection) = (yyvsp[(3) - (3)].projection);
  (yyval.projection)->distinct = 1;
}
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 718 "./sparql_parser.y"
    {
  (yyval.projection) = (yyvsp[(3) - (3)].projection);
  (yyval.projection)->distinct = 2;
}
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 723 "./sparql_parser.y"
    {
  (yyval.projection) = (yyvsp[(2) - (2)].projection);
}
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 733 "./sparql_parser.y"
    {
  (yyval.projection) = rasqal_new_projection((rasqal_query*)rq, (yyvsp[(1) - (1)].seq), 0, 0);
}
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 737 "./sparql_parser.y"
    {
  (yyval.projection) = rasqal_new_projection((rasqal_query*)rq, NULL, /* wildcard */ 1, 0);
}
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 747 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if(raptor_sequence_push((yyval.seq), (yyvsp[(2) - (2)].variable))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("SelectExpressionListTail 1: sequence push failed");
  }
}
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 756 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (3)].seq);
  if(raptor_sequence_push((yyval.seq), (yyvsp[(3) - (3)].variable))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("SelectExpressionListTail 2: sequence push failed");
  }
}
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 765 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_variable,
                           (raptor_data_print_handler)rasqal_variable_print);
  if(!(yyval.seq))
    YYERROR_MSG("SelectExpressionListTail 3: failed to create sequence");
  if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].variable))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("SelectExpressionListTail 3: sequence push failed");
  }
}
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 783 "./sparql_parser.y"
    {
  (yyval.variable) = (yyvsp[(1) - (1)].variable);
}
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 787 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.variable) = NULL;
  if(!sparql->sparql11_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "SELECT ( expression ) AS Variable cannot be used with SPARQL 1.0");
    YYERROR;
  } else if((yyvsp[(2) - (5)].expr) && (yyvsp[(4) - (5)].variable)) {
    if(rasqal_expression_mentions_variable((yyvsp[(2) - (5)].expr), (yyvsp[(4) - (5)].variable))) {
      sparql_query_error_full((rasqal_query*)rq, 
                              "Expression in SELECT ( expression ) AS %s contains the variable name '%s'",
                              (yyvsp[(4) - (5)].variable)->name, (yyvsp[(4) - (5)].variable)->name);
      YYERROR;
    } else {
      (yyval.variable) = (yyvsp[(4) - (5)].variable);
      (yyval.variable)->expression = (yyvsp[(2) - (5)].expr);
    }

  }
}
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 829 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 833 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 837 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 841 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 845 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 849 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 853 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 860 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "functions with DISTINCT cannot be used with SPARQL 1.0");
    YYERROR;
  }
  
  (yyval.uinteger) = RASQAL_EXPR_FLAG_DISTINCT;
}
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 873 "./sparql_parser.y"
    {
  (yyval.uinteger) = 0;
}
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 880 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 884 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_0op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_VARSTAR);
}
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 892 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(!sparql->sparql11_aggregates) {
    sparql_syntax_error((rasqal_query*)rq,
                        "COUNT() cannot be used with SPARQL 1.0");
    YYERROR;
  } else {
    (yyval.expr) = rasqal_new_aggregate_function_expression(((rasqal_query*)rq)->world,
                                                  RASQAL_EXPR_COUNT, (yyvsp[(4) - (5)].expr),
                                                  NULL /* params */, (yyvsp[(3) - (5)].uinteger));
    if(!(yyval.expr))
      YYERROR_MSG("CountAggregateExpression: cannot create expr");
  }
}
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 913 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(!sparql->sparql11_aggregates) {
    sparql_syntax_error((rasqal_query*)rq,
                        "SUM() cannot be used with SPARQL 1.0");
    YYERROR;
  } else {
    (yyval.expr) = rasqal_new_aggregate_function_expression(((rasqal_query*)rq)->world,
                                                  RASQAL_EXPR_SUM, (yyvsp[(4) - (5)].expr),
                                                  NULL /* params */, (yyvsp[(3) - (5)].uinteger));
    if(!(yyval.expr))
      YYERROR_MSG("SumAggregateExpression: cannot create expr");
  }
}
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 934 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(!sparql->sparql11_aggregates) {
    sparql_syntax_error((rasqal_query*)rq,
                        "AVG() cannot be used with SPARQL 1.0");
    YYERROR;
  } else {
    (yyval.expr) = rasqal_new_aggregate_function_expression(((rasqal_query*)rq)->world,
                                                  RASQAL_EXPR_AVG, (yyvsp[(4) - (5)].expr),
                                                  NULL /* params */, (yyvsp[(3) - (5)].uinteger));
    if(!(yyval.expr))
      YYERROR_MSG("AvgAggregateExpression: cannot create expr");
  }
}
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 955 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(!sparql->sparql11_aggregates) {
    sparql_syntax_error((rasqal_query*)rq,
                        "MIN() cannot be used with SPARQL 1.0");
    YYERROR;
  } else {
    (yyval.expr) = rasqal_new_aggregate_function_expression(((rasqal_query*)rq)->world,
                                                  RASQAL_EXPR_MIN, (yyvsp[(4) - (5)].expr),
                                                  NULL /* params */, (yyvsp[(3) - (5)].uinteger));
    if(!(yyval.expr))
      YYERROR_MSG("MinAggregateExpression: cannot create expr");
  }
}
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 976 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(!sparql->sparql11_aggregates) {
    sparql_syntax_error((rasqal_query*)rq,
                        "MAX() cannot be used with SPARQL 1.0");
    YYERROR;
  } else {
    (yyval.expr) = rasqal_new_aggregate_function_expression(((rasqal_query*)rq)->world,
                                                  RASQAL_EXPR_MAX, (yyvsp[(4) - (5)].expr),
                                                  NULL /* params */, (yyvsp[(3) - (5)].uinteger));
    if(!(yyval.expr))
      YYERROR_MSG("MaxAggregateExpression: cannot create expr");
  }
}
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 997 "./sparql_parser.y"
    {
  (yyval.literal) = rasqal_new_string_literal(((rasqal_query*)rq)->world, (yyvsp[(4) - (4)].name), 
	                         NULL /* language */,
                                 NULL /* dt uri */, NULL /* dt_qname */);
}
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 1003 "./sparql_parser.y"
    {
  (yyval.literal) = NULL;
}
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 1013 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (3)].seq);
  if(raptor_sequence_push((yyval.seq), (yyvsp[(3) - (3)].expr))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("ExpressionList 1: sequence push failed");
  }
}
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 1022 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_expression,
                           (raptor_data_print_handler)rasqal_expression_print);
  if(!(yyval.seq))
    YYERROR_MSG("ExpressionList 2: failed to create sequence");

  if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].expr))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("ExpressionList 2: sequence push failed");
  }
}
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 1038 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(!sparql->sparql11_aggregates) {
    sparql_syntax_error((rasqal_query*)rq,
                        "GROUP_CONCAT() cannot be used with SPARQL 1.0");
    YYERROR;
  } else {
    int flags = 0;
    
    if((yyvsp[(3) - (6)].uinteger))
      flags |= RASQAL_EXPR_FLAG_DISTINCT;

    (yyval.expr) = rasqal_new_group_concat_expression(((rasqal_query*)rq)->world,
                                            flags /* flags */,
                                            (yyvsp[(4) - (6)].seq) /* args */,
                                            (yyvsp[(5) - (6)].literal) /* separator */);
    if(!(yyval.expr))
      YYERROR_MSG("GroupConcatAggregateExpression: cannot create expr");
  }
}
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 1066 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(!sparql->sparql11_aggregates) {
    sparql_syntax_error((rasqal_query*)rq,
                        "SAMPLE() cannot be used with SPARQL 1.0");
    YYERROR;
  } else {
    (yyval.expr) = rasqal_new_aggregate_function_expression(((rasqal_query*)rq)->world,
                                                  RASQAL_EXPR_SAMPLE, (yyvsp[(4) - (5)].expr),
                                                  NULL /* params */, (yyvsp[(3) - (5)].uinteger));
    if(!(yyval.expr))
      YYERROR_MSG("SampleAggregateExpression: cannot create expr");
  }
}
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 1089 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.seq) = NULL;
  if(!sparql->sparql_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "CONSTRUCT can only be used with a SPARQL query");
    YYERROR;
  }
  
  (yyval.seq) = (yyvsp[(2) - (5)].seq);

  if((yyvsp[(3) - (5)].seq))
    rasqal_query_add_data_graphs((rasqal_query*)rq, (yyvsp[(3) - (5)].seq));
  ((rasqal_query*)rq)->query_graph_pattern = (yyvsp[(4) - (5)].graph_pattern);

  if((yyvsp[(5) - (5)].modifier))
    ((rasqal_query*)rq)->modifier = (yyvsp[(5) - (5)].modifier);
}
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 1111 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_graph_pattern* where_gp;
  raptor_sequence* seq = NULL;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "CONSTRUCT can only be used with a SPARQL query");
    YYERROR;
  }

  if((yyvsp[(5) - (7)].seq)) {
    int i;
    int size = raptor_sequence_size((yyvsp[(5) - (7)].seq));
    
    seq = raptor_new_sequence((raptor_data_free_handler)rasqal_free_triple,
                              (raptor_data_print_handler)rasqal_triple_print);
    for(i = 0; i < size; i++) {
      rasqal_triple* t = (rasqal_triple*)raptor_sequence_get_at((yyvsp[(5) - (7)].seq), i);
      t = rasqal_new_triple_from_triple(t);
      raptor_sequence_push(seq, t);
    }
  }
  
  where_gp = rasqal_new_basic_graph_pattern_from_triples((rasqal_query*)rq, seq);
  seq = NULL;
  if(!where_gp)
    YYERROR_MSG("ConstructQuery: cannot create graph pattern");

  (yyval.seq) = (yyvsp[(5) - (7)].seq);

  if((yyvsp[(2) - (7)].seq))
    rasqal_query_add_data_graphs((rasqal_query*)rq, (yyvsp[(2) - (7)].seq));
  ((rasqal_query*)rq)->query_graph_pattern = where_gp;

  if((yyvsp[(7) - (7)].modifier))
    ((rasqal_query*)rq)->modifier = (yyvsp[(7) - (7)].modifier);
}
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 1157 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.seq) = NULL;
  if(!sparql->sparql_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "DESCRIBE can only be used with a SPARQL query");
    YYERROR;
  }
  
  (yyval.seq) = (yyvsp[(2) - (5)].seq);

  if((yyvsp[(3) - (5)].seq))
    rasqal_query_add_data_graphs((rasqal_query*)rq, (yyvsp[(3) - (5)].seq));

  ((rasqal_query*)rq)->query_graph_pattern = (yyvsp[(4) - (5)].graph_pattern);

  if((yyvsp[(5) - (5)].modifier))
    ((rasqal_query*)rq)->modifier = (yyvsp[(5) - (5)].modifier);
}
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 1181 "./sparql_parser.y"
    {
  (yyval.seq) = NULL;

  if((yyvsp[(3) - (5)].seq))
    rasqal_query_add_data_graphs((rasqal_query*)rq, (yyvsp[(3) - (5)].seq));

  ((rasqal_query*)rq)->query_graph_pattern = (yyvsp[(4) - (5)].graph_pattern);

  if((yyvsp[(5) - (5)].modifier))
    ((rasqal_query*)rq)->modifier = (yyvsp[(5) - (5)].modifier);
}
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 1197 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if(raptor_sequence_push((yyval.seq), (yyvsp[(2) - (2)].literal))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("VarOrIRIrefList 1: sequence push failed");
  }
}
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 1206 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (3)].seq);
  if(raptor_sequence_push((yyval.seq), (yyvsp[(3) - (3)].literal))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("VarOrIRIrefList 2: sequence push failed");
  }
}
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 1215 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_literal,
                           (raptor_data_print_handler)rasqal_literal_print);
  if(!(yyval.seq))
    YYERROR_MSG("VarOrIRIrefList 3: cannot create seq");
  if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].literal))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("VarOrIRIrefList 3: sequence push failed");
  }
}
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 1232 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "ASK can only be used with a SPARQL query");
    YYERROR;
  }
  
  if((yyvsp[(2) - (3)].seq))
    rasqal_query_add_data_graphs((rasqal_query*)rq, (yyvsp[(2) - (3)].seq));

  ((rasqal_query*)rq)->query_graph_pattern = (yyvsp[(3) - (3)].graph_pattern);
}
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 1252 "./sparql_parser.y"
    {
  (yyval.data_graph) = (yyvsp[(2) - (2)].data_graph);
}
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 1256 "./sparql_parser.y"
    {
  (yyval.data_graph) = (yyvsp[(2) - (2)].data_graph);
}
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 1264 "./sparql_parser.y"
    {
  (yyval.uri) = (yyvsp[(2) - (2)].uri);
}
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 1272 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "DELETE can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }
  
  /* LAQRS: experimental syntax */
  sparql_syntax_warning(((rasqal_query*)rq), 
                        "DELETE FROM <uri> ... WHERE ... is deprecated LAQRS syntax.");

  if((yyvsp[(2) - (3)].seq))
    rasqal_query_add_data_graphs((rasqal_query*)rq, (yyvsp[(2) - (3)].seq));

  ((rasqal_query*)rq)->query_graph_pattern = (yyvsp[(3) - (3)].graph_pattern);
}
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 1292 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "DELETE can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  /* SPARQL 1.1 (Draft) update:
   * deleting via template + query - not inline atomic triples 
   */

  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_UPDATE,
                                       NULL /* graph_uri */,
                                       NULL /* document_uri */,
                                       NULL /* insert templates */,
                                       (yyvsp[(3) - (5)].seq) /* delete templates */,
                                       (yyvsp[(5) - (5)].graph_pattern) /* where */,
                                       0 /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("DeleteQuery: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("DeleteQuery: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 1324 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "DELETE can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }
  
  /* SPARQL 1.1 (Draft) update:
   * deleting inline triples - not inserting from graph URIs 
   */
  (yyvsp[(4) - (5)].update)->type = RASQAL_UPDATE_TYPE_UPDATE;
  (yyvsp[(4) - (5)].update)->delete_templates = (yyvsp[(4) - (5)].update)->insert_templates; (yyvsp[(4) - (5)].update)->insert_templates = NULL;
  (yyvsp[(4) - (5)].update)->flags |= RASQAL_UPDATE_FLAGS_DATA;
  
  rasqal_query_add_update_operation((rasqal_query*)rq, (yyvsp[(4) - (5)].update));
}
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 1344 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;
  raptor_sequence* delete_templates = NULL;
  
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "DELETE WHERE { } can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  /* SPARQL 1.1 (Draft) update:
   * deleting via template - not inline atomic triples 
   */

  /* Turn GP into flattened triples */
  if((yyvsp[(3) - (3)].graph_pattern)) {
    delete_templates = rasqal_graph_pattern_get_flattened_triples((rasqal_query*)rq, (yyvsp[(3) - (3)].graph_pattern));
    rasqal_free_graph_pattern((yyvsp[(3) - (3)].graph_pattern));
    (yyvsp[(3) - (3)].graph_pattern) = NULL;
  }

  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_UPDATE,
                                       NULL /* graph_uri */,
                                       NULL /* document_uri */,
                                       NULL /* insert templates */,
                                       delete_templates /* delete templates */,
                                       NULL /* where */,
                                       0 /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("DeleteQuery: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("DeleteQuery: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 1388 "./sparql_parser.y"
    {
  (yyval.update) = NULL;
 
  if((yyvsp[(1) - (1)].formula)) {
    (yyval.update) = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_UNKNOWN,
                                     NULL /* graph_uri */,
                                     NULL /* document_uri */,
                                     (yyvsp[(1) - (1)].formula)->triples /* insert templates */, 
                                     NULL /* delete templates */,
                                     NULL /* where */,
                                     0 /* flags */,
                                     RASQAL_UPDATE_GRAPH_ONE /* applies */);
    (yyvsp[(1) - (1)].formula)->triples = NULL;
    rasqal_free_formula((yyvsp[(1) - (1)].formula));
  }
}
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 1405 "./sparql_parser.y"
    {
  (yyval.update) = NULL;

  if((yyvsp[(4) - (5)].formula)) {
    raptor_sequence* seq;
    seq = (yyvsp[(4) - (5)].formula)->triples;

    if((yyvsp[(2) - (5)].uri)) {
      rasqal_literal* origin_literal;
      
      origin_literal = rasqal_new_uri_literal(((rasqal_query*)rq)->world, (yyvsp[(2) - (5)].uri));
      (yyvsp[(2) - (5)].uri) = NULL;

      rasqal_triples_sequence_set_origin(/* dest */ NULL, seq, origin_literal);
      rasqal_free_literal(origin_literal);
    }
    (yyval.update) = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_UNKNOWN,
                                     NULL /* graph uri */,
                                     NULL /* document uri */,
                                     seq /* insert templates */,
                                     NULL /* delete templates */,
                                     NULL /* where */,
                                     0 /* flags */,
                                     RASQAL_UPDATE_GRAPH_ONE /* applies */);
    (yyvsp[(4) - (5)].formula)->triples = NULL;
    rasqal_free_formula((yyvsp[(4) - (5)].formula));
  }
}
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 1438 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(4) - (5)].seq);

  if((yyvsp[(2) - (5)].literal)) {
    rasqal_triples_sequence_set_origin(NULL, (yyval.seq), (yyvsp[(2) - (5)].literal));
    rasqal_free_literal((yyvsp[(2) - (5)].literal));
    (yyvsp[(2) - (5)].literal) = NULL;
  }
}
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 1453 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (1)].seq);
}
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 1457 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (1)].seq);
}
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 1465 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);

  if((yyvsp[(2) - (2)].seq)) {
    if(raptor_sequence_join((yyval.seq), (yyvsp[(2) - (2)].seq))) {
      raptor_free_sequence((yyvsp[(2) - (2)].seq));
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("ModifyTemplateList: sequence join failed");
    }
    raptor_free_sequence((yyvsp[(2) - (2)].seq));
  }

}
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 1480 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (1)].seq);
}
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 1489 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql  = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "INSERT can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  /* LAQRS: experimental syntax */
  sparql_syntax_warning(((rasqal_query*)rq), 
                        "INSERT FROM <uri> ... WHERE ... is deprecated LAQRS syntax.");

  if((yyvsp[(2) - (3)].seq))
    rasqal_query_add_data_graphs((rasqal_query*)rq, (yyvsp[(2) - (3)].seq));

  ((rasqal_query*)rq)->query_graph_pattern = (yyvsp[(3) - (3)].graph_pattern);
}
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 1509 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "INSERT can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }
  
  /* inserting via template + query - not inline atomic triples */

  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_UPDATE,
                                       NULL /* graph_uri */,
                                       NULL /* document_uri */,
                                       (yyvsp[(3) - (5)].seq) /* insert templates */,
                                       NULL /* delete templates */,
                                       (yyvsp[(5) - (5)].graph_pattern) /* where */,
                                       0 /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("InsertQuery: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("InsertQuery: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 1539 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "INSERT DATA can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }
  
  /* inserting inline atomic triples (no variables) - not via template */
  (yyvsp[(4) - (5)].update)->type = RASQAL_UPDATE_TYPE_UPDATE;
  (yyvsp[(4) - (5)].update)->flags |= RASQAL_UPDATE_FLAGS_DATA;

  rasqal_query_add_update_operation((rasqal_query*)rq, (yyvsp[(4) - (5)].update));
}
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 1562 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "WITH can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }
  
  if((yyvsp[(2) - (11)].uri)) {
    rasqal_literal* origin_literal;

    origin_literal = rasqal_new_uri_literal(((rasqal_query*)rq)->world, (yyvsp[(2) - (11)].uri));
    (yyvsp[(2) - (11)].uri) = NULL;

    rasqal_triples_sequence_set_origin(/* dest */ NULL, (yyvsp[(9) - (11)].seq), origin_literal);
    rasqal_triples_sequence_set_origin(/* dest */ NULL, (yyvsp[(5) - (11)].seq), origin_literal);

    rasqal_free_literal(origin_literal);
  }

  /* after this $5, $9 and $12 are owned by update */
  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_UPDATE,
                                       NULL /* graph uri */, 
                                       NULL /* document uri */,
                                       (yyvsp[(9) - (11)].seq) /* insert templates */,
                                       (yyvsp[(5) - (11)].seq) /* delete templates */,
                                       (yyvsp[(11) - (11)].graph_pattern) /* where */,
                                       0 /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("UpdateQuery 1: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("UpdateQuery 1: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 1605 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "WITH can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }
  
  if((yyvsp[(2) - (7)].uri)) {
    rasqal_literal* origin_literal;
    
    origin_literal = rasqal_new_uri_literal(((rasqal_query*)rq)->world, (yyvsp[(2) - (7)].uri));
    (yyvsp[(2) - (7)].uri) = NULL;

    rasqal_triples_sequence_set_origin(/* dest */ NULL, (yyvsp[(5) - (7)].seq), origin_literal);

    rasqal_free_literal(origin_literal);
  }
  
  /* after this $5 and $7 are owned by update */
  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_UPDATE,
                                       NULL /* graph uri */, 
                                       NULL /* document uri */,
                                       NULL /* insert templates */,
                                       (yyvsp[(5) - (7)].seq) /* delete templates */,
                                       (yyvsp[(7) - (7)].graph_pattern) /* where */,
                                       0 /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("UpdateQuery 2: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("UpdateQuery 2: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 1647 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "WITH can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  if((yyvsp[(2) - (7)].uri)) {
    rasqal_literal* origin_literal;
    
    origin_literal = rasqal_new_uri_literal(((rasqal_query*)rq)->world, (yyvsp[(2) - (7)].uri));
    (yyvsp[(2) - (7)].uri) = NULL;

    rasqal_triples_sequence_set_origin(/* dest */ NULL, (yyvsp[(5) - (7)].seq), origin_literal);

    rasqal_free_literal(origin_literal);
  }

  /* after this $5 and $7 are owned by update */
  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_UPDATE,
                                       NULL /* graph uri */, 
                                       NULL /* document uri */,
                                       (yyvsp[(5) - (7)].seq) /* insert templates */,
                                       NULL /* delete templates */,
                                       (yyvsp[(7) - (7)].graph_pattern) /* where */,
                                       0 /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("UpdateQuery 3: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("UpdateQuery 3: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 1688 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "WITH can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  /* inserting inline atomic triples (no variables) - not via template */
  (yyvsp[(6) - (7)].update)->graph_uri = (yyvsp[(2) - (7)].uri); /* graph uri */
  (yyvsp[(6) - (7)].update)->type = RASQAL_UPDATE_TYPE_UPDATE;
  (yyvsp[(6) - (7)].update)->flags |= RASQAL_UPDATE_FLAGS_DATA;

  rasqal_query_add_update_operation((rasqal_query*)rq, (yyvsp[(6) - (7)].update));
}
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 1711 "./sparql_parser.y"
    {
  (yyval.uri_applies) = new_uri_applies((yyvsp[(1) - (1)].uri), RASQAL_UPDATE_GRAPH_ONE);
}
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 1715 "./sparql_parser.y"
    {
  (yyval.uri_applies) = new_uri_applies(NULL, RASQAL_UPDATE_GRAPH_DEFAULT);
}
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 1719 "./sparql_parser.y"
    {
  (yyval.uri_applies) = new_uri_applies(NULL, RASQAL_UPDATE_GRAPH_NAMED);
}
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 1723 "./sparql_parser.y"
    {
  (yyval.uri_applies) = new_uri_applies(NULL, RASQAL_UPDATE_GRAPH_ALL);
}
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 1727 "./sparql_parser.y"
    {
  /* Early draft syntax - deprecated */
  sparql_syntax_warning((rasqal_query*)rq,
                        "CLEAR GRAPH DEFAULT is replaced by CLEAR DEFAULT in later SPARQL 1.1 drafts");


  (yyval.uri_applies) = new_uri_applies(NULL, RASQAL_UPDATE_GRAPH_DEFAULT);
}
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 1740 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "CLEAR (SILENT) DEFAULT | NAMED | ALL can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  if((yyvsp[(3) - (3)].uri_applies)) {
    update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_CLEAR,
                                         (yyvsp[(3) - (3)].uri_applies)->uri ? raptor_uri_copy((yyvsp[(3) - (3)].uri_applies)->uri) : NULL /* graph uri or NULL */,
                                         NULL /* document uri */,
                                         NULL, NULL,
                                         NULL /*where */,
                                         (yyvsp[(2) - (3)].integer) /* flags */,
                                         (yyvsp[(3) - (3)].uri_applies)->applies /* applies */);
    free_uri_applies((yyvsp[(3) - (3)].uri_applies));
    (yyvsp[(3) - (3)].uri_applies) = NULL;

    if(!update) {
      YYERROR_MSG("ClearQuery: rasqal_new_update_operation failed");
    } else {
      if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
        YYERROR_MSG("ClearQuery: rasqal_query_add_update_operation failed");
    }
  }
}
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 1772 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "CLEAR can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  /* Early draft syntax - deprecated */
  sparql_syntax_warning((rasqal_query*)rq,
                        "CLEAR is replaced by CLEAR DEFAULT in later SPARQL 1.1 drafts");

  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_CLEAR,
                                       NULL /* graph uri */, 
                                       NULL /* document uri */,
                                       NULL, NULL,
                                       NULL /* where */,
                                       0 /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("ClearQuery: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("ClearQuery: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 1807 "./sparql_parser.y"
    {
  (yyval.integer) = RASQAL_UPDATE_FLAGS_SILENT;
}
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 1811 "./sparql_parser.y"
    {
  (yyval.integer) = 0;
}
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 1819 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq, 
                        "CREATE (SILENT) <uri> can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_CREATE,
                                       (yyvsp[(3) - (3)].uri) /* graph uri */, 
                                       NULL /* document uri */,
                                       NULL, NULL,
                                       NULL /*where */,
                                       (yyvsp[(2) - (3)].integer) /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("CreateQuery: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("CreateQuery: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 1846 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq, 
                        "CREATE (SILENT) GRAPH <uri> can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  /* Early draft syntax - deprecated */
  sparql_syntax_warning((rasqal_query*)rq,
                        "CREATE (SILENT) GRAPH <uri> is replaced by CREATE (SILENT) <uri> in later SPARQL 1.1 drafts");

  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_CREATE,
                                       (yyvsp[(3) - (3)].uri) /* graph uri */, 
                                       NULL /* document uri */,
                                       NULL, NULL,
                                       NULL /*where */,
                                       RASQAL_UPDATE_FLAGS_SILENT /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("CreateQuery: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("CreateQuery: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 1881 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;
  
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq, 
                        "DROP (SILENT) DEFAULT | NAMED | ALL can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  if((yyvsp[(3) - (3)].uri_applies)) {
    update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_DROP,
                                         (yyvsp[(3) - (3)].uri_applies)->uri ? raptor_uri_copy((yyvsp[(3) - (3)].uri_applies)->uri) : NULL /* graph uri or NULL */,
                                         NULL /* document uri */,
                                         NULL, NULL,
                                         NULL /*where */,
                                         (yyvsp[(2) - (3)].integer) /* flags */,
                                         (yyvsp[(3) - (3)].uri_applies)->applies /* applies */);
    free_uri_applies((yyvsp[(3) - (3)].uri_applies));
    (yyvsp[(3) - (3)].uri_applies) = NULL;

    if(!update) {
      YYERROR_MSG("DropQuery: rasqal_new_update_operation failed");
    } else {
      if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
        YYERROR_MSG("DropQuery: rasqal_query_add_update_operation failed");
    }
  }
}
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 1917 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if(raptor_sequence_push((yyval.seq), (yyvsp[(2) - (2)].uri))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("IriRefList 1: sequence push failed");
  }
}
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 1926 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)raptor_free_uri,
                           (raptor_data_print_handler)raptor_uri_print);
  if(!(yyval.seq)) {
    if((yyvsp[(1) - (1)].uri))
      raptor_free_uri((yyvsp[(1) - (1)].uri));
    YYERROR_MSG("IriRefList 2: cannot create sequence");
  }
  if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].uri))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("IriRefList 2: sequence push failed");
  }
}
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 1945 "./sparql_parser.y"
    {
  (yyval.uri) = NULL;
}
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 1949 "./sparql_parser.y"
    {
  (yyval.uri) = (yyvsp[(1) - (1)].uri);
}
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 1956 "./sparql_parser.y"
    {
  (yyval.uri) = (yyvsp[(1) - (1)].uri);
}
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 1960 "./sparql_parser.y"
    {
  /* Early draft syntax allowed a list of URIs - deprecated */
  sparql_syntax_warning((rasqal_query*)rq,
                        "LOAD <document uri list> INTO <graph uri> is replaced by LOAD <document uri> INTO GRAPH <graph uri> in later SPARQL 1.1 drafts");

  (yyval.uri) = (yyvsp[(1) - (1)].uri);
}
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 1968 "./sparql_parser.y"
    {
  /* Early draft syntax allowed a list of URIs - deprecated */
  sparql_syntax_warning((rasqal_query*)rq,
                        "LOAD <document uri list> INTO DEFAULT is replaced by LOAD <document uri> in later SPARQL 1.1 drafts");

  (yyval.uri) = NULL;
}
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 1980 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;
  
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq, 
                        "LOAD <uri> can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_LOAD,
                                       NULL /* graph uri */, 
                                       (yyvsp[(3) - (3)].uri) /* document uri */,
                                       NULL, NULL,
                                       NULL /* where */,
                                       (yyvsp[(2) - (3)].integer) /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("LoadQuery: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("LoadQuery: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 2007 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  int i;
  raptor_uri* doc_uri;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq, 
                        "LOAD <document uri> INTO GRAPH <graph URI> / DEFAULT can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  for(i = 0; (doc_uri = (raptor_uri*)raptor_sequence_get_at((yyvsp[(3) - (5)].seq), i)); i++) {
    rasqal_update_operation* update;
    update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_LOAD,
                                         (yyvsp[(5) - (5)].uri) ? raptor_uri_copy((yyvsp[(5) - (5)].uri)) : NULL /* graph uri */,
                                         raptor_uri_copy(doc_uri) /* document uri */,
                                         NULL, NULL,
                                         NULL /*where */,
                                         (yyvsp[(2) - (5)].integer) /* flags */,
                                         RASQAL_UPDATE_GRAPH_ONE /* applies */);
    if(!update) {
      YYERROR_MSG("LoadQuery: rasqal_new_update_operation failed");
    } else {
      if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
        YYERROR_MSG("LoadQuery: rasqal_query_add_update_operation failed");
    }

    if(i == 1)
      /* Early draft syntax allowed a list of URIs - deprecated */
      sparql_syntax_warning((rasqal_query*)rq,
                            "LOAD <document uri list> INTO <graph uri> / DEFAULT is replaced by LOAD <document uri> INTO GRAPH <graph uri> or LOAD <document uri> in later SPARQL 1.1 drafts");
    

  }

  raptor_free_sequence((yyvsp[(3) - (5)].seq));
  if((yyvsp[(5) - (5)].uri))
    raptor_free_uri((yyvsp[(5) - (5)].uri));
}
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 2053 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "ADD (SILENT) <uri> TO <uri> can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_ADD,
                                       (yyvsp[(3) - (5)].uri) /* graph uri or NULL */, 
                                       (yyvsp[(5) - (5)].uri) /* document uri */,
                                       NULL, NULL,
                                       NULL /*where */,
                                       (yyvsp[(2) - (5)].integer) /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("AddQuery: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("AddQuery: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 2084 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "MOVE (SILENT) <uri> TO <uri> can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_MOVE,
                                       (yyvsp[(3) - (5)].uri) /* graph uri or NULL */, 
                                       (yyvsp[(5) - (5)].uri) /* document uri */,
                                       NULL, NULL,
                                       NULL /*where */,
                                       (yyvsp[(2) - (5)].integer) /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("MoveQuery: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("MoveQuery: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 2115 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  rasqal_update_operation* update;

  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  if(!sparql->sparql11_update) {
    sparql_syntax_error((rasqal_query*)rq,
                        "COPY (SILENT) <uri> TO <uri> can only be used with a SPARQL 1.1 Update");
    YYERROR;
  }

  update = rasqal_new_update_operation(RASQAL_UPDATE_TYPE_COPY,
                                       (yyvsp[(3) - (5)].uri) /* graph uri or NULL */, 
                                       (yyvsp[(5) - (5)].uri) /* document uri */,
                                       NULL, NULL,
                                       NULL /*where */,
                                       (yyvsp[(2) - (5)].integer) /* flags */,
                                       RASQAL_UPDATE_GRAPH_ONE /* applies */);
  if(!update) {
    YYERROR_MSG("CopyQuery: rasqal_new_update_operation failed");
  } else {
    if(rasqal_query_add_update_operation(((rasqal_query*)rq), update))
      YYERROR_MSG("CopyQuery: rasqal_query_add_update_operation failed");
  }
}
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 2146 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if((yyvsp[(1) - (2)].seq) && (yyvsp[(2) - (2)].data_graph))
    raptor_sequence_push((yyvsp[(1) - (2)].seq), (yyvsp[(2) - (2)].data_graph));
}
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 2152 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_data_graph, (raptor_data_print_handler)rasqal_data_graph_print);
  if((yyval.seq) && (yyvsp[(1) - (1)].data_graph))
    raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].data_graph));
}
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 2161 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (1)].seq);
}
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 2165 "./sparql_parser.y"
    {
  (yyval.seq) = NULL;
}
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 2173 "./sparql_parser.y"
    {
  if((yyvsp[(1) - (1)].literal)) {
    raptor_uri* uri = rasqal_literal_as_uri((yyvsp[(1) - (1)].literal));
    rasqal_data_graph* dg;

    dg = rasqal_new_data_graph_from_uri(((rasqal_query*)rq)->world, uri,
                                        NULL, RASQAL_DATA_GRAPH_BACKGROUND,
                                        NULL, NULL, NULL);

    if(!dg) {
      rasqal_free_literal((yyvsp[(1) - (1)].literal));
      YYERROR_MSG("DefaultGraphClause: rasqal_query_new_data_graph_from_uri() failed");
    }
    rasqal_free_literal((yyvsp[(1) - (1)].literal));

    (yyval.data_graph) = dg;
  } else
    (yyval.data_graph) = NULL;
}
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 2197 "./sparql_parser.y"
    {
  if((yyvsp[(2) - (2)].literal)) {
    raptor_uri* uri = rasqal_literal_as_uri((yyvsp[(2) - (2)].literal));
    rasqal_data_graph* dg;

    dg = rasqal_new_data_graph_from_uri(((rasqal_query*)rq)->world, uri,
                                        uri, RASQAL_DATA_GRAPH_NAMED,
                                        NULL, NULL, NULL);
    
    if(!dg) {
      rasqal_free_literal((yyvsp[(2) - (2)].literal));
      YYERROR_MSG("NamedGraphClause: rasqal_query_new_data_graph_from_uri() failed");
    }
    rasqal_free_literal((yyvsp[(2) - (2)].literal));
    (yyval.data_graph) = dg;
  } else
    (yyval.data_graph) = NULL;
}
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 2220 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 2228 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(2) - (2)].graph_pattern);
}
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 2232 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 2240 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 2244 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = NULL;
}
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 2252 "./sparql_parser.y"
    {
  (yyval.modifier) = rasqal_new_solution_modifier((rasqal_query*)rq,
                                    /* order_conditions */ (yyvsp[(3) - (4)].seq),
                                    /* group_conditions */ (yyvsp[(1) - (4)].seq),
                                    /* having_conditions */ (yyvsp[(2) - (4)].seq),
                                    /* limit */ (yyvsp[(4) - (4)].limit_offset)[0],
                                    /* offset */ (yyvsp[(4) - (4)].limit_offset)[1]);
  
}
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 2266 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if((yyvsp[(2) - (2)].expr))
    if(raptor_sequence_push((yyval.seq), (yyvsp[(2) - (2)].expr))) {
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("GroupConditionList 1: sequence push failed");
    }
}
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 2276 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_expression,
                           (raptor_data_print_handler)rasqal_expression_print);
  if(!(yyval.seq)) {
    if((yyvsp[(1) - (1)].expr))
      rasqal_free_expression((yyvsp[(1) - (1)].expr));
    YYERROR_MSG("GroupConditionList 2: cannot create sequence");
  }
  if((yyvsp[(1) - (1)].expr))
    if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].expr))) {
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("GroupConditionList 2: sequence push failed");
    }
}
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 2296 "./sparql_parser.y"
    {
  (yyval.variable) = (yyvsp[(2) - (2)].variable);
}
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 2300 "./sparql_parser.y"
    {
  (yyval.variable) = NULL;
}
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 2308 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 2312 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 2316 "./sparql_parser.y"
    {
  rasqal_literal* l;

  (yyval.expr) = (yyvsp[(2) - (4)].expr);
  if((yyvsp[(3) - (4)].variable)) {
    if(rasqal_expression_mentions_variable((yyval.expr), (yyvsp[(3) - (4)].variable))) {
      sparql_query_error_full((rasqal_query*)rq, 
                              "Expression in GROUP BY ( expression ) AS %s contains the variable name '%s'",
                              (yyvsp[(3) - (4)].variable)->name, (yyvsp[(3) - (4)].variable)->name);
    } else {
      /* Expression AS Variable */
      (yyvsp[(3) - (4)].variable)->expression = (yyval.expr);
      (yyval.expr) = NULL;
      
      l = rasqal_new_variable_literal(((rasqal_query*)rq)->world, (yyvsp[(3) - (4)].variable));
      if(!l)
        YYERROR_MSG("GroupCondition 4: cannot create variable literal");
      (yyvsp[(3) - (4)].variable) = NULL;

      (yyval.expr) = rasqal_new_literal_expression(((rasqal_query*)rq)->world, l);
      if(!(yyval.expr))
        YYERROR_MSG("GroupCondition 4: cannot create variable literal expression");
    }
  }
  
}
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 2343 "./sparql_parser.y"
    {
  rasqal_literal* l;
  l = rasqal_new_variable_literal(((rasqal_query*)rq)->world, (yyvsp[(1) - (1)].variable));
  if(!l)
    YYERROR_MSG("GroupCondition 5: cannot create lit");
  (yyval.expr) = rasqal_new_literal_expression(((rasqal_query*)rq)->world, l);
  if(!(yyval.expr))
    YYERROR_MSG("GroupCondition 5: cannot create lit expr");
}
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 2357 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.seq) = NULL;
  if(!sparql->sparql11_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "GROUP BY cannot be used with SPARQL 1.0");
    YYERROR;
  } else
    (yyval.seq) = (yyvsp[(3) - (3)].seq);
}
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 2370 "./sparql_parser.y"
    {
  (yyval.seq) = NULL;
}
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 2378 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 2385 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if((yyvsp[(2) - (2)].expr))
    if(raptor_sequence_push((yyval.seq), (yyvsp[(2) - (2)].expr))) {
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("HavingConditionList 1: sequence push failed");
    }
}
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 2395 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_expression,
                           (raptor_data_print_handler)rasqal_expression_print);
  if(!(yyval.seq)) {
    if((yyvsp[(1) - (1)].expr))
      rasqal_free_expression((yyvsp[(1) - (1)].expr));
    YYERROR_MSG("HavingConditionList 2: cannot create sequence");
  }
  if((yyvsp[(1) - (1)].expr))
    if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].expr))) {
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("HavingConditionList 2: sequence push failed");
    }
}
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 2415 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.seq) = NULL;
  if(!sparql->sparql11_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "HAVING cannot be used with SPARQL 1.0");
    YYERROR;
  } else 
    (yyval.seq) = (yyvsp[(2) - (2)].seq);
}
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 2428 "./sparql_parser.y"
    {
  (yyval.seq) = NULL;
}
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 2436 "./sparql_parser.y"
    {
  (yyval.limit_offset)[0] = (yyvsp[(1) - (2)].integer);
  (yyval.limit_offset)[1] = (yyvsp[(2) - (2)].integer);
}
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 2441 "./sparql_parser.y"
    {
  (yyval.limit_offset)[0] = (yyvsp[(2) - (2)].integer);
  (yyval.limit_offset)[1] = (yyvsp[(1) - (2)].integer);
}
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 2446 "./sparql_parser.y"
    {
  (yyval.limit_offset)[0] = (yyvsp[(1) - (1)].integer);
  (yyval.limit_offset)[1] = -1;
}
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 2451 "./sparql_parser.y"
    {
  (yyval.limit_offset)[0] = -1;
  (yyval.limit_offset)[1] = (yyvsp[(1) - (1)].integer);
}
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 2456 "./sparql_parser.y"
    {
  (yyval.limit_offset)[0] = -1;
  (yyval.limit_offset)[1] = -1;
}
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 2465 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(3) - (3)].seq);
}
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 2469 "./sparql_parser.y"
    {
  (yyval.seq) = NULL;
}
    break;

  case 151:

/* Line 1806 of yacc.c  */
#line 2477 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if((yyvsp[(2) - (2)].expr))
    if(raptor_sequence_push((yyval.seq), (yyvsp[(2) - (2)].expr))) {
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("OrderConditionList 1: sequence push failed");
    }
}
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 2487 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_expression,
                           (raptor_data_print_handler)rasqal_expression_print);
  if(!(yyval.seq)) {
    if((yyvsp[(1) - (1)].expr))
      rasqal_free_expression((yyvsp[(1) - (1)].expr));
    YYERROR_MSG("OrderConditionList 2: cannot create sequence");
  }
  if((yyvsp[(1) - (1)].expr))
    if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].expr))) {
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("OrderConditionList 2: sequence push failed");
    }
}
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 2507 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ORDER_COND_ASC, (yyvsp[(2) - (2)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("OrderCondition 1: cannot create expr");
}
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 2514 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ORDER_COND_DESC, (yyvsp[(2) - (2)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("OrderCondition 2: cannot create expr");
}
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 2521 "./sparql_parser.y"
    {
  /* The direction of ordering is ascending by default */
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ORDER_COND_ASC, (yyvsp[(1) - (1)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("OrderCondition 3: cannot create expr");
}
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 2529 "./sparql_parser.y"
    {
  rasqal_literal* l;
  rasqal_expression *e;
  l = rasqal_new_variable_literal(((rasqal_query*)rq)->world, (yyvsp[(1) - (1)].variable));
  if(!l)
    YYERROR_MSG("OrderCondition 4: cannot create lit");
  e = rasqal_new_literal_expression(((rasqal_query*)rq)->world, l);
  if(!e)
    YYERROR_MSG("OrderCondition 4: cannot create lit expr");

  /* The direction of ordering is ascending by default */
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ORDER_COND_ASC, e);
  if(!(yyval.expr))
    YYERROR_MSG("OrderCondition 1: cannot create expr");
}
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 2546 "./sparql_parser.y"
    {
  /* The direction of ordering is ascending by default */
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ORDER_COND_ASC, (yyvsp[(1) - (1)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("OrderCondition 5: cannot create expr");
}
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 2554 "./sparql_parser.y"
    {
  /* The direction of ordering is ascending by default */
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ORDER_COND_ASC, (yyvsp[(1) - (1)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("OrderCondition 6: cannot create expr");
}
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 2566 "./sparql_parser.y"
    {
  (yyval.integer) = -1;

  if((yyvsp[(2) - (2)].literal) != NULL) {
    (yyval.integer) = (yyvsp[(2) - (2)].literal)->value.integer;
    rasqal_free_literal((yyvsp[(2) - (2)].literal));
  }
  
}
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 2580 "./sparql_parser.y"
    {
  (yyval.integer) = -1;

  if((yyvsp[(2) - (2)].literal) != NULL) {
    (yyval.integer) = (yyvsp[(2) - (2)].literal)->value.integer;
    rasqal_free_literal((yyvsp[(2) - (2)].literal));
  }
}
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 2593 "./sparql_parser.y"
    {
  if((yyvsp[(2) - (5)].seq)) {
    (yyval.bindings) = rasqal_new_bindings((rasqal_query*)rq, (yyvsp[(2) - (5)].seq), (yyvsp[(4) - (5)].seq));
    if(!(yyval.bindings))
      YYERROR_MSG("BindingsClauseOpt: cannot create bindings");
  } else {
    if((yyvsp[(4) - (5)].seq))
      raptor_free_sequence((yyvsp[(4) - (5)].seq));

    (yyval.bindings) = NULL;
  }
}
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 2606 "./sparql_parser.y"
    {
  (yyval.bindings) = NULL;
}
    break;

  case 163:

/* Line 1806 of yacc.c  */
#line 2614 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if(raptor_sequence_push((yyval.seq), (yyvsp[(2) - (2)].variable))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("VarList 1: sequence push failed");
  }
}
    break;

  case 164:

/* Line 1806 of yacc.c  */
#line 2623 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_variable,
                           (raptor_data_print_handler)rasqal_variable_print);
  if(!(yyval.seq))
    YYERROR_MSG("VarList 2: cannot create seq");

  if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].variable))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("VarList 3: sequence push failed");
  }
}
    break;

  case 165:

/* Line 1806 of yacc.c  */
#line 2640 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (1)].seq);
}
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 2644 "./sparql_parser.y"
    {
  (yyval.seq) = NULL;
}
    break;

  case 167:

/* Line 1806 of yacc.c  */
#line 2652 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if(raptor_sequence_push((yyval.seq), (yyvsp[(2) - (2)].row))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("BindingsRowList 1: sequence push failed");
  } else {
    int size = raptor_sequence_size((yyval.seq));
    (yyvsp[(2) - (2)].row)->offset = size-1;
  }
}
    break;

  case 168:

/* Line 1806 of yacc.c  */
#line 2664 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_row,
                           (raptor_data_print_handler)rasqal_row_print);
  if(!(yyval.seq)) {
    if((yyvsp[(1) - (1)].row))
      rasqal_free_row((yyvsp[(1) - (1)].row));

    YYERROR_MSG("BindingsRowList 2: cannot create sequence");
  }
  if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].row))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("BindingsRowList 2: sequence push failed");
  }
}
    break;

  case 169:

/* Line 1806 of yacc.c  */
#line 2684 "./sparql_parser.y"
    {
  (yyval.row) = NULL;
  if((yyvsp[(2) - (3)].seq)) {
    int size;
    rasqal_row* row;
    int i;
    
    size = raptor_sequence_size((yyvsp[(2) - (3)].seq));

    row = rasqal_new_row_for_size(((rasqal_query*)rq)->world, size);
    if(!row) {
      YYERROR_MSG("BindingsRow: cannot create row");
    } else {
      for(i = 0; i < size; i++) {
        rasqal_literal* value = (rasqal_literal*)raptor_sequence_get_at((yyvsp[(2) - (3)].seq), i);
        rasqal_row_set_value_at(row, i, value);
      }
    }
    raptor_free_sequence((yyvsp[(2) - (3)].seq));
    
    (yyval.row) = row;
  }
}
    break;

  case 170:

/* Line 1806 of yacc.c  */
#line 2708 "./sparql_parser.y"
    {
  (yyval.row) = NULL;
}
    break;

  case 171:

/* Line 1806 of yacc.c  */
#line 2716 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if(raptor_sequence_push((yyval.seq), (yyvsp[(2) - (2)].literal))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("IriRefList 1: sequence push failed");
  }
}
    break;

  case 172:

/* Line 1806 of yacc.c  */
#line 2725 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_literal,
                           (raptor_data_print_handler)rasqal_literal_print);
  if(!(yyval.seq)) {
    if((yyvsp[(1) - (1)].literal))
      rasqal_free_literal((yyvsp[(1) - (1)].literal));
    YYERROR_MSG("IriRefList 2: cannot create sequence");
  }
  if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].literal))) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
    YYERROR_MSG("IriRefList 2: sequence push failed");
  }
}
    break;

  case 173:

/* Line 1806 of yacc.c  */
#line 2743 "./sparql_parser.y"
    {
  (yyval.literal) = rasqal_new_string_literal(((rasqal_query*)rq)->world, (yyvsp[(1) - (1)].name), 
	                         NULL /* language */,
                                 NULL /* dt uri */, NULL /* dt_qname */);
}
    break;

  case 174:

/* Line 1806 of yacc.c  */
#line 2749 "./sparql_parser.y"
    {
  (yyval.literal) = rasqal_new_string_literal(((rasqal_query*)rq)->world, (yyvsp[(1) - (2)].name), 
	                         RASQAL_GOOD_CAST(const char*, (yyvsp[(2) - (2)].name)),
                                 NULL /* dt uri */, NULL /* dt_qname */);
}
    break;

  case 175:

/* Line 1806 of yacc.c  */
#line 2755 "./sparql_parser.y"
    {
  raptor_uri* dt_uri = raptor_uri_copy(rasqal_literal_as_uri((yyvsp[(3) - (3)].literal)));
  (yyval.literal) = rasqal_new_string_literal(((rasqal_query*)rq)->world, (yyvsp[(1) - (3)].name), 
	                         NULL /* language */,
                                 dt_uri, NULL /* dt_qname */);
  rasqal_free_literal((yyvsp[(3) - (3)].literal));
}
    break;

  case 176:

/* Line 1806 of yacc.c  */
#line 2763 "./sparql_parser.y"
    {
  if((yyvsp[(1) - (3)].literal)) {
    raptor_uri* dt_uri = raptor_uri_copy(rasqal_literal_as_uri((yyvsp[(3) - (3)].literal)));
    const unsigned char *str = (yyvsp[(1) - (3)].literal)->string;
    (yyvsp[(1) - (3)].literal)->string = NULL;

    (yyval.literal) = rasqal_new_string_literal(((rasqal_query*)rq)->world, str,
                                   NULL /* language */,
                                   dt_uri, NULL /* dt_qname */);
  }
  rasqal_free_literal((yyvsp[(3) - (3)].literal));
  rasqal_free_literal((yyvsp[(1) - (3)].literal));
}
    break;

  case 177:

/* Line 1806 of yacc.c  */
#line 2781 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 178:

/* Line 1806 of yacc.c  */
#line 2785 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 179:

/* Line 1806 of yacc.c  */
#line 2789 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 180:

/* Line 1806 of yacc.c  */
#line 2793 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 181:

/* Line 1806 of yacc.c  */
#line 2797 "./sparql_parser.y"
    {
  (yyval.literal) = NULL;
}
    break;

  case 182:

/* Line 1806 of yacc.c  */
#line 2809 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(2) - (3)].graph_pattern);
}
    break;

  case 183:

/* Line 1806 of yacc.c  */
#line 2813 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(2) - (3)].graph_pattern);
}
    break;

  case 184:

/* Line 1806 of yacc.c  */
#line 2824 "./sparql_parser.y"
    {
  rasqal_graph_pattern *formula_gp = NULL;

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "GroupGraphPattern\n  TriplesBlockOpt=");
  if((yyvsp[(2) - (2)].graph_pattern))
    rasqal_formula_print((yyvsp[(1) - (2)].formula), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fprintf(DEBUG_FH, ", GraphpatternListOpt=");
  if((yyvsp[(2) - (2)].graph_pattern))
    rasqal_graph_pattern_print((yyvsp[(2) - (2)].graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fputs("\n", DEBUG_FH);
#endif


  if(!(yyvsp[(1) - (2)].formula) && !(yyvsp[(2) - (2)].graph_pattern)) {
    (yyval.graph_pattern) = rasqal_new_2_group_graph_pattern((rasqal_query*)rq, NULL, NULL);
    if(!(yyval.graph_pattern))
      YYERROR_MSG("GroupGraphPattern: cannot create group gp");
  } else {
    if((yyvsp[(1) - (2)].formula)) {
      formula_gp = rasqal_new_basic_graph_pattern_from_formula((rasqal_query*)rq,
                                                               (yyvsp[(1) - (2)].formula));
      if(!formula_gp) {
        if((yyvsp[(2) - (2)].graph_pattern))
          rasqal_free_graph_pattern((yyvsp[(2) - (2)].graph_pattern));
        YYERROR_MSG("GroupGraphPattern: cannot create formula_gp");
      }
    }

    if((yyvsp[(2) - (2)].graph_pattern)) {
      (yyval.graph_pattern) = (yyvsp[(2) - (2)].graph_pattern);
      if(formula_gp && raptor_sequence_shift((yyval.graph_pattern)->graph_patterns, formula_gp)) {
        rasqal_free_graph_pattern((yyval.graph_pattern));
        (yyval.graph_pattern) = NULL;
        YYERROR_MSG("GroupGraphPattern: sequence push failed");
      }
    } else
      (yyval.graph_pattern) = formula_gp;
  }
  
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "  after graph pattern=");
  if((yyval.graph_pattern))
    rasqal_graph_pattern_print((yyval.graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fprintf(DEBUG_FH, "\n\n");
#endif
}
    break;

  case 185:

/* Line 1806 of yacc.c  */
#line 2882 "./sparql_parser.y"
    {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "TriplesBlockOpt 1\n  TriplesBlock=");
  if((yyvsp[(1) - (1)].formula))
    rasqal_formula_print((yyvsp[(1) - (1)].formula), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fputs("\n\n", DEBUG_FH);
#endif

  (yyval.formula) = (yyvsp[(1) - (1)].formula);
}
    break;

  case 186:

/* Line 1806 of yacc.c  */
#line 2895 "./sparql_parser.y"
    {
  (yyval.formula) = NULL;
}
    break;

  case 187:

/* Line 1806 of yacc.c  */
#line 2908 "./sparql_parser.y"
    {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "GraphPatternListOpt\n  GraphPatternListOpt=");
  if((yyvsp[(1) - (2)].graph_pattern))
    rasqal_graph_pattern_print((yyvsp[(1) - (2)].graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fprintf(DEBUG_FH, ", GraphPatternList=");
  if((yyvsp[(2) - (2)].graph_pattern))
    rasqal_graph_pattern_print((yyvsp[(2) - (2)].graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fputs("\n", DEBUG_FH);
#endif

  (yyval.graph_pattern) =  ((yyvsp[(1) - (2)].graph_pattern) ? (yyvsp[(1) - (2)].graph_pattern) : (yyvsp[(2) - (2)].graph_pattern));
  if((yyvsp[(1) - (2)].graph_pattern) && (yyvsp[(2) - (2)].graph_pattern)) {
    (yyval.graph_pattern) = (yyvsp[(1) - (2)].graph_pattern);
    if(rasqal_graph_patterns_join((yyval.graph_pattern), (yyvsp[(2) - (2)].graph_pattern))) {
      rasqal_free_graph_pattern((yyval.graph_pattern));
      rasqal_free_graph_pattern((yyvsp[(2) - (2)].graph_pattern));
      (yyval.graph_pattern) = NULL;
      YYERROR_MSG("GraphPatternListOpt: sequence join failed");
    }
    rasqal_free_graph_pattern((yyvsp[(2) - (2)].graph_pattern));
  }
  
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "  after grouping graph pattern=");
  if((yyval.graph_pattern))
    rasqal_graph_pattern_print((yyval.graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fprintf(DEBUG_FH, "\n\n");
#endif
}
    break;

  case 188:

/* Line 1806 of yacc.c  */
#line 2945 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 189:

/* Line 1806 of yacc.c  */
#line 2949 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = NULL;
}
    break;

  case 190:

/* Line 1806 of yacc.c  */
#line 2962 "./sparql_parser.y"
    {
  rasqal_graph_pattern *formula_gp = NULL;

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "GraphPatternList\n  GraphPatternListFilter=");
  if((yyvsp[(1) - (3)].graph_pattern))
    rasqal_graph_pattern_print((yyvsp[(1) - (3)].graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fprintf(DEBUG_FH, ", TriplesBlockOpt=");
  if((yyvsp[(3) - (3)].formula))
    rasqal_formula_print((yyvsp[(3) - (3)].formula), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fputs("\n", DEBUG_FH);
#endif

  if((yyvsp[(3) - (3)].formula)) {
    formula_gp = rasqal_new_basic_graph_pattern_from_formula((rasqal_query*)rq, 
                                                             (yyvsp[(3) - (3)].formula));
    if(!formula_gp) {
      if((yyvsp[(1) - (3)].graph_pattern))
        rasqal_free_graph_pattern((yyvsp[(1) - (3)].graph_pattern));
      YYERROR_MSG("GraphPatternList: cannot create formula_gp");
    }
  }
  (yyval.graph_pattern) = rasqal_new_2_group_graph_pattern((rasqal_query*)rq, (yyvsp[(1) - (3)].graph_pattern), formula_gp);
  if(!(yyval.graph_pattern))
    YYERROR_MSG("GraphPatternList: cannot create sequence");

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "  after graph pattern=");
  if((yyval.graph_pattern))
    rasqal_graph_pattern_print((yyval.graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fprintf(DEBUG_FH, "\n\n");
#endif
}
    break;

  case 191:

/* Line 1806 of yacc.c  */
#line 3010 "./sparql_parser.y"
    {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "GraphPatternListFilter 1\n  GraphPatternNotTriples=");
  if((yyvsp[(1) - (1)].graph_pattern))
    rasqal_graph_pattern_print((yyvsp[(1) - (1)].graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fputs("\n\n", DEBUG_FH);
#endif

  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 192:

/* Line 1806 of yacc.c  */
#line 3023 "./sparql_parser.y"
    {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "GraphPatternListFilter 2\n  Filter=");
  if((yyvsp[(1) - (1)].expr))
    rasqal_expression_print((yyvsp[(1) - (1)].expr), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fputs("\n", DEBUG_FH);
#endif

  (yyval.graph_pattern) = rasqal_new_filter_graph_pattern((rasqal_query*)rq, (yyvsp[(1) - (1)].expr));
  if(!(yyval.graph_pattern))
    YYERROR_MSG("GraphPatternListFilter 2: cannot create graph pattern");

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "  after graph pattern=");
  if((yyval.graph_pattern))
    rasqal_graph_pattern_print((yyval.graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fprintf(DEBUG_FH, "\n\n");
#endif
}
    break;

  case 195:

/* Line 1806 of yacc.c  */
#line 3057 "./sparql_parser.y"
    {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "TriplesBlock\n  TriplesSameSubject=");
  if((yyvsp[(1) - (3)].formula))
    rasqal_formula_print((yyvsp[(1) - (3)].formula), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fprintf(DEBUG_FH, ", TriplesBlockOpt=");
  if((yyvsp[(3) - (3)].formula))
    rasqal_formula_print((yyvsp[(3) - (3)].formula), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fputs("\n", DEBUG_FH);
#endif


  (yyval.formula) =  ((yyvsp[(1) - (3)].formula) ? (yyvsp[(1) - (3)].formula) : (yyvsp[(3) - (3)].formula));
  if((yyvsp[(1) - (3)].formula) && (yyvsp[(3) - (3)].formula)) {
    /* $1 and $3 are freed as necessary */
    (yyval.formula) = rasqal_formula_join((yyvsp[(1) - (3)].formula), (yyvsp[(3) - (3)].formula));
    if(!(yyvsp[(1) - (3)].formula))
      YYERROR_MSG("TriplesBlock: formula join failed");
  }

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "  after joining formula=");
  rasqal_formula_print((yyval.formula), DEBUG_FH);
  fprintf(DEBUG_FH, "\n\n");
#endif
}
    break;

  case 196:

/* Line 1806 of yacc.c  */
#line 3088 "./sparql_parser.y"
    {
  (yyval.formula) = (yyvsp[(1) - (1)].formula);
}
    break;

  case 197:

/* Line 1806 of yacc.c  */
#line 3096 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 198:

/* Line 1806 of yacc.c  */
#line 3100 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 199:

/* Line 1806 of yacc.c  */
#line 3104 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 200:

/* Line 1806 of yacc.c  */
#line 3108 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 201:

/* Line 1806 of yacc.c  */
#line 3112 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 202:

/* Line 1806 of yacc.c  */
#line 3116 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 203:

/* Line 1806 of yacc.c  */
#line 3120 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 204:

/* Line 1806 of yacc.c  */
#line 3128 "./sparql_parser.y"
    {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "PatternElementForms 4\n  graphpattern=");
  if((yyvsp[(2) - (2)].graph_pattern))
    rasqal_graph_pattern_print((yyvsp[(2) - (2)].graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fputs("\n\n", DEBUG_FH);
#endif

  (yyval.graph_pattern) = NULL;

  if((yyvsp[(2) - (2)].graph_pattern)) {
    raptor_sequence *seq;

    seq = raptor_new_sequence((raptor_data_free_handler)rasqal_free_graph_pattern,
                              (raptor_data_print_handler)rasqal_graph_pattern_print);
    if(!seq) {
      rasqal_free_graph_pattern((yyvsp[(2) - (2)].graph_pattern));
      YYERROR_MSG("OptionalGraphPattern 1: cannot create sequence");
    } else {
      if(raptor_sequence_push(seq, (yyvsp[(2) - (2)].graph_pattern))) {
        raptor_free_sequence(seq);
        YYERROR_MSG("OptionalGraphPattern 2: sequence push failed");
      } else {
        (yyval.graph_pattern) = rasqal_new_graph_pattern_from_sequence((rasqal_query*)rq,
                                                    seq,
                                                    RASQAL_GRAPH_PATTERN_OPERATOR_OPTIONAL);
        if(!(yyval.graph_pattern))
          YYERROR_MSG("OptionalGraphPattern: cannot create graph pattern");
      }
    }
  }
}
    break;

  case 205:

/* Line 1806 of yacc.c  */
#line 3167 "./sparql_parser.y"
    {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "GraphGraphPattern 2\n  varoruri=");
  rasqal_literal_print((yyvsp[(2) - (3)].literal), DEBUG_FH);
  fprintf(DEBUG_FH, ", graphpattern=");
  if((yyvsp[(3) - (3)].graph_pattern))
    rasqal_graph_pattern_print((yyvsp[(3) - (3)].graph_pattern), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fputs("\n\n", DEBUG_FH);
#endif

  if((yyvsp[(3) - (3)].graph_pattern)) {
    raptor_sequence *seq;

    seq = raptor_new_sequence((raptor_data_free_handler)rasqal_free_graph_pattern,
                              (raptor_data_print_handler)rasqal_graph_pattern_print);
    if(!seq) {
      rasqal_free_graph_pattern((yyvsp[(3) - (3)].graph_pattern));
      YYERROR_MSG("GraphGraphPattern 1: cannot create sequence");
    } else {
      if(raptor_sequence_push(seq, (yyvsp[(3) - (3)].graph_pattern))) {
        raptor_free_sequence(seq);
        YYERROR_MSG("GraphGraphPattern 2: sequence push failed");
      } else {
        (yyval.graph_pattern) = rasqal_new_graph_pattern_from_sequence((rasqal_query*)rq,
                                                    seq,
                                                    RASQAL_GRAPH_PATTERN_OPERATOR_GRAPH);
        if(!(yyval.graph_pattern))
          YYERROR_MSG("GraphGraphPattern: cannot create graph pattern");
        else
          rasqal_graph_pattern_set_origin((yyval.graph_pattern), (yyvsp[(2) - (3)].literal));
      }
    }
  }


#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "GraphGraphPattern\n  graphpattern=");
  rasqal_graph_pattern_print((yyval.graph_pattern), DEBUG_FH);
  fputs("\n\n", DEBUG_FH);
#endif

  rasqal_free_literal((yyvsp[(2) - (3)].literal));
}
    break;

  case 206:

/* Line 1806 of yacc.c  */
#line 3217 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = rasqal_new_single_graph_pattern((rasqal_query*)rq,
                                       RASQAL_GRAPH_PATTERN_OPERATOR_SERVICE,
                                       (yyvsp[(4) - (4)].graph_pattern));
  if((yyval.graph_pattern)) {
    (yyval.graph_pattern)->silent = ((yyvsp[(2) - (4)].integer) & RASQAL_UPDATE_FLAGS_SILENT) ? 1 : 0;

    (yyval.graph_pattern)->origin = (yyvsp[(3) - (4)].literal);
    (yyvsp[(3) - (4)].literal) = NULL;
  } else if((yyvsp[(3) - (4)].literal))
    rasqal_free_literal((yyvsp[(3) - (4)].literal));
}
    break;

  case 207:

/* Line 1806 of yacc.c  */
#line 3234 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = rasqal_new_single_graph_pattern((rasqal_query*)rq,
                                       RASQAL_GRAPH_PATTERN_OPERATOR_MINUS,
                                       (yyvsp[(2) - (2)].graph_pattern));
}
    break;

  case 208:

/* Line 1806 of yacc.c  */
#line 3244 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(3) - (3)].graph_pattern);
  if(raptor_sequence_shift((yyval.graph_pattern)->graph_patterns, (yyvsp[(1) - (3)].graph_pattern))) {
    rasqal_free_graph_pattern((yyval.graph_pattern));
    (yyval.graph_pattern) = NULL;
    YYERROR_MSG("GroupOrUnionGraphPattern: sequence push failed");
  }

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "UnionGraphPattern\n  graphpattern=");
  rasqal_graph_pattern_print((yyval.graph_pattern), DEBUG_FH);
  fputs("\n\n", DEBUG_FH);
#endif
}
    break;

  case 209:

/* Line 1806 of yacc.c  */
#line 3259 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (1)].graph_pattern);
}
    break;

  case 210:

/* Line 1806 of yacc.c  */
#line 3266 "./sparql_parser.y"
    {
  (yyval.graph_pattern) = (yyvsp[(1) - (3)].graph_pattern);
  if((yyvsp[(3) - (3)].graph_pattern))
    if(raptor_sequence_push((yyval.graph_pattern)->graph_patterns, (yyvsp[(3) - (3)].graph_pattern))) {
      rasqal_free_graph_pattern((yyval.graph_pattern));
      (yyval.graph_pattern) = NULL;
      YYERROR_MSG("GroupOrUnionGraphPatternList 1: sequence push failed");
    }
}
    break;

  case 211:

/* Line 1806 of yacc.c  */
#line 3276 "./sparql_parser.y"
    {
  raptor_sequence *seq;
  seq = raptor_new_sequence((raptor_data_free_handler)rasqal_free_graph_pattern,
                            (raptor_data_print_handler)rasqal_graph_pattern_print);
  if(!seq) {
    if((yyvsp[(1) - (1)].graph_pattern))
      rasqal_free_graph_pattern((yyvsp[(1) - (1)].graph_pattern));
    YYERROR_MSG("GroupOrUnionGraphPatternList 2: cannot create sequence");
  }
  if((yyvsp[(1) - (1)].graph_pattern))
    if(raptor_sequence_push(seq, (yyvsp[(1) - (1)].graph_pattern))) {
      raptor_free_sequence(seq);
      YYERROR_MSG("GroupOrUnionGraphPatternList 2: sequence push failed");
    }
  (yyval.graph_pattern) = rasqal_new_graph_pattern_from_sequence((rasqal_query*)rq,
                                              seq,
                                              RASQAL_GRAPH_PATTERN_OPERATOR_UNION);
  if(!(yyval.graph_pattern))
    YYERROR_MSG("GroupOrUnionGraphPatternList 1: cannot create gp");
}
    break;

  case 212:

/* Line 1806 of yacc.c  */
#line 3301 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.graph_pattern) = NULL;
  if((yyvsp[(3) - (6)].variable) && (yyvsp[(5) - (6)].expr)) {
    if(sparql->experimental)
      (yyval.graph_pattern) = rasqal_new_let_graph_pattern((rasqal_query*)rq, (yyvsp[(3) - (6)].variable), (yyvsp[(5) - (6)].expr));
    else {
      sparql_syntax_error((rasqal_query*)rq,
                          "LET can only be used with LAQRS");
      YYERROR;
    }
  } else
    (yyval.graph_pattern) = NULL;
}
    break;

  case 213:

/* Line 1806 of yacc.c  */
#line 3322 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.graph_pattern) = NULL;
  if((yyvsp[(3) - (6)].expr) && (yyvsp[(5) - (6)].variable)) {
    if(!sparql->sparql11_query) {
      sparql_syntax_error((rasqal_query*)rq,
                          "BIND cannot be used with SPARQL 1.0");
      YYERROR;
    } else {
      (yyval.graph_pattern) = rasqal_new_let_graph_pattern((rasqal_query*)rq, (yyvsp[(5) - (6)].variable), (yyvsp[(3) - (6)].expr));
    }
  } else
    (yyval.graph_pattern) = NULL;
}
    break;

  case 214:

/* Line 1806 of yacc.c  */
#line 3343 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(2) - (2)].expr);
}
    break;

  case 215:

/* Line 1806 of yacc.c  */
#line 3351 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 216:

/* Line 1806 of yacc.c  */
#line 3355 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 217:

/* Line 1806 of yacc.c  */
#line 3359 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 218:

/* Line 1806 of yacc.c  */
#line 3366 "./sparql_parser.y"
    {
  (yyval.seq) = NULL;
}
    break;

  case 219:

/* Line 1806 of yacc.c  */
#line 3370 "./sparql_parser.y"
    {
  (yyval.seq) = NULL;
}
    break;

  case 220:

/* Line 1806 of yacc.c  */
#line 3378 "./sparql_parser.y"
    {
  raptor_uri* uri = rasqal_literal_as_uri((yyvsp[(1) - (6)].literal));
  
  if(!(yyvsp[(4) - (6)].seq)) {
    (yyvsp[(4) - (6)].seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_expression,
                             (raptor_data_print_handler)rasqal_expression_print);
    if(!(yyvsp[(4) - (6)].seq)) {
      rasqal_free_literal((yyvsp[(1) - (6)].literal));
      YYERROR_MSG("FunctionCall: cannot create sequence");
    }
  }

  uri = raptor_uri_copy(uri);

  if(raptor_sequence_size((yyvsp[(4) - (6)].seq)) == 1 &&
     rasqal_xsd_is_datatype_uri(((rasqal_query*)rq)->world, uri)) {
    rasqal_expression* e = (rasqal_expression*)raptor_sequence_pop((yyvsp[(4) - (6)].seq));
    (yyval.expr) = rasqal_new_cast_expression(((rasqal_query*)rq)->world, uri, e);
    if((yyval.expr))
      (yyval.expr)->flags |= (yyvsp[(3) - (6)].uinteger);
    raptor_free_sequence((yyvsp[(4) - (6)].seq));
  } else {
    unsigned int flags = 0;
    if((yyvsp[(3) - (6)].uinteger))
      flags |= 1;
    
    (yyval.expr) = rasqal_new_function_expression(((rasqal_query*)rq)->world, 
                                        uri, (yyvsp[(4) - (6)].seq), (yyvsp[(5) - (6)].seq) /* params */,
                                        flags);
    if((yyval.expr))
      (yyval.expr)->flags |= (yyvsp[(3) - (6)].uinteger);
  }
  rasqal_free_literal((yyvsp[(1) - (6)].literal));

  if(!(yyval.expr))
    YYERROR_MSG("FunctionCall: cannot create expr");
}
    break;

  case 221:

/* Line 1806 of yacc.c  */
#line 3417 "./sparql_parser.y"
    {
  raptor_uri* uri = rasqal_literal_as_uri((yyvsp[(1) - (3)].literal));
  
  if(!(yyvsp[(2) - (3)].seq)) {
    (yyvsp[(2) - (3)].seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_expression,
                             (raptor_data_print_handler)rasqal_expression_print);
    if(!(yyvsp[(2) - (3)].seq)) {
      rasqal_free_literal((yyvsp[(1) - (3)].literal));
      YYERROR_MSG("FunctionCall: cannot create sequence");
    }
  }

  uri = raptor_uri_copy(uri);

  if(raptor_sequence_size((yyvsp[(2) - (3)].seq)) == 1 &&
     rasqal_xsd_is_datatype_uri(((rasqal_query*)rq)->world, uri)) {
    rasqal_expression* e = (rasqal_expression*)raptor_sequence_pop((yyvsp[(2) - (3)].seq));
    (yyval.expr) = rasqal_new_cast_expression(((rasqal_query*)rq)->world, uri, e);
    raptor_free_sequence((yyvsp[(2) - (3)].seq));
  } else {
    (yyval.expr) = rasqal_new_function_expression(((rasqal_query*)rq)->world,
                                        uri, (yyvsp[(2) - (3)].seq), NULL /* params */,
                                        0 /* flags */);
  }
  rasqal_free_literal((yyvsp[(1) - (3)].literal));

  if(!(yyval.expr))
    YYERROR_MSG("FunctionCall: cannot create expr");
}
    break;

  case 222:

/* Line 1806 of yacc.c  */
#line 3451 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(!sparql->sparql11_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "COALESCE cannot be used with SPARQL 1.0");
    YYERROR;
  }
  
  if(!(yyvsp[(2) - (2)].seq)) {
    (yyvsp[(2) - (2)].seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_expression,
                             (raptor_data_print_handler)rasqal_expression_print);
    if(!(yyvsp[(2) - (2)].seq))
      YYERROR_MSG("FunctionCall: cannot create sequence");
  }

  (yyval.expr) = rasqal_new_expr_seq_expression(((rasqal_query*)rq)->world, 
                                      RASQAL_EXPR_COALESCE, (yyvsp[(2) - (2)].seq));
  if(!(yyval.expr))
    YYERROR_MSG("Coalesce: cannot create expr");
}
    break;

  case 223:

/* Line 1806 of yacc.c  */
#line 3479 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(2) - (3)].seq);
}
    break;

  case 224:

/* Line 1806 of yacc.c  */
#line 3486 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (3)].seq);
  if((yyvsp[(3) - (3)].expr))
    if(raptor_sequence_push((yyval.seq), (yyvsp[(3) - (3)].expr))) {
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("ArgListNoBraces 1: sequence push failed");
    }
}
    break;

  case 225:

/* Line 1806 of yacc.c  */
#line 3496 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_expression,
                           (raptor_data_print_handler)rasqal_expression_print);
  if(!(yyval.seq)) {
    if((yyvsp[(1) - (1)].expr))
      rasqal_free_expression((yyvsp[(1) - (1)].expr));
    YYERROR_MSG("ArgListNoBraces 2: cannot create sequence");
  }
  if((yyvsp[(1) - (1)].expr))
    if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].expr))) {
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("ArgListNoBraces 2: sequence push failed");
    }
}
    break;

  case 226:

/* Line 1806 of yacc.c  */
#line 3512 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_expression,
                           (raptor_data_print_handler)rasqal_expression_print);
}
    break;

  case 227:

/* Line 1806 of yacc.c  */
#line 3521 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(2) - (3)].seq);
}
    break;

  case 228:

/* Line 1806 of yacc.c  */
#line 3529 "./sparql_parser.y"
    {
  (yyval.seq) = (yyvsp[(1) - (1)].seq);
}
    break;

  case 229:

/* Line 1806 of yacc.c  */
#line 3533 "./sparql_parser.y"
    {
  (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_triple,
                           (raptor_data_print_handler)rasqal_triple_print);
  if(!(yyval.seq)) {
    YYERROR_MSG("ConstructTriplesOpt: cannot create sequence");
  }
}
    break;

  case 230:

/* Line 1806 of yacc.c  */
#line 3545 "./sparql_parser.y"
    {
  (yyval.seq) = NULL;
 
  if((yyvsp[(1) - (3)].formula)) {
    (yyval.seq) = (yyvsp[(1) - (3)].formula)->triples;
    (yyvsp[(1) - (3)].formula)->triples = NULL;
    rasqal_free_formula((yyvsp[(1) - (3)].formula));
  }
  
  if((yyvsp[(3) - (3)].seq)) {
    if(!(yyval.seq)) {
      (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_triple,
                               (raptor_data_print_handler)rasqal_triple_print);
      if(!(yyval.seq)) {
        raptor_free_sequence((yyvsp[(3) - (3)].seq));
        YYERROR_MSG("ConstructTriples: cannot create sequence");
      }
    }

    if(raptor_sequence_join((yyval.seq), (yyvsp[(3) - (3)].seq))) {
      raptor_free_sequence((yyvsp[(3) - (3)].seq));
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("ConstructTriples: sequence join failed");
    }
    raptor_free_sequence((yyvsp[(3) - (3)].seq));
  }

 }
    break;

  case 231:

/* Line 1806 of yacc.c  */
#line 3575 "./sparql_parser.y"
    {
  (yyval.seq) = NULL;
  
  if((yyvsp[(1) - (1)].formula)) {
    (yyval.seq) = (yyvsp[(1) - (1)].formula)->triples;
    (yyvsp[(1) - (1)].formula)->triples = NULL;
    rasqal_free_formula((yyvsp[(1) - (1)].formula));
  }
  
}
    break;

  case 232:

/* Line 1806 of yacc.c  */
#line 3590 "./sparql_parser.y"
    {
  int i;

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "TriplesSameSubject 1\n  subject=");
  rasqal_formula_print((yyvsp[(1) - (2)].formula), DEBUG_FH);
  if((yyvsp[(2) - (2)].formula)) {
    fprintf(DEBUG_FH, "\n  propertyList=");
    rasqal_formula_print((yyvsp[(2) - (2)].formula), DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
  } else     
    fprintf(DEBUG_FH, "\n  and empty propertyList\n");
#endif

  if((yyvsp[(2) - (2)].formula)) {
    raptor_sequence *seq = (yyvsp[(2) - (2)].formula)->triples;
    rasqal_literal *subject = (yyvsp[(1) - (2)].formula)->value;
    int size = raptor_sequence_size(seq);
    
    /* non-empty property list, handle it  */
    for(i = 0; i < size; i++) {
      rasqal_triple* t2 = (rasqal_triple*)raptor_sequence_get_at(seq, i);
      if(t2->subject)
        continue;
      t2->subject = rasqal_new_literal_from_literal(subject);
    }
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
    fprintf(DEBUG_FH, "  after substitution propertyList=");
    rasqal_formula_print((yyvsp[(2) - (2)].formula), DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
#endif
  }

  (yyval.formula) = rasqal_formula_join((yyvsp[(1) - (2)].formula), (yyvsp[(2) - (2)].formula));
  if(!(yyval.formula))
    YYERROR_MSG("TriplesSameSubject 1: formula join failed");

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "  after joining formula=");
  rasqal_formula_print((yyval.formula), DEBUG_FH);
  fprintf(DEBUG_FH, "\n\n");
#endif
}
    break;

  case 233:

/* Line 1806 of yacc.c  */
#line 3634 "./sparql_parser.y"
    {
  int i;

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "TriplesSameSubject 2\n  TriplesNode=");
  rasqal_formula_print((yyvsp[(1) - (2)].formula), DEBUG_FH);
  if((yyvsp[(2) - (2)].formula)) {
    fprintf(DEBUG_FH, "\n  propertyList=");
    rasqal_formula_print((yyvsp[(2) - (2)].formula), DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
  } else     
    fprintf(DEBUG_FH, "\n  and empty propertyList\n");
#endif

  if((yyvsp[(2) - (2)].formula)) {
    raptor_sequence *seq = (yyvsp[(2) - (2)].formula)->triples;
    rasqal_literal *subject = (yyvsp[(1) - (2)].formula)->value;
    int size = raptor_sequence_size(seq);
    
    /* non-empty property list, handle it  */
    for(i = 0; i < size; i++) {
      rasqal_triple* t2 = (rasqal_triple*)raptor_sequence_get_at(seq, i);
      if(t2->subject)
        continue;
      t2->subject = rasqal_new_literal_from_literal(subject);
    }
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
    fprintf(DEBUG_FH, "  after substitution propertyList=");
    rasqal_formula_print((yyvsp[(2) - (2)].formula), DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
#endif
  }

  (yyval.formula) = rasqal_formula_join((yyvsp[(1) - (2)].formula), (yyvsp[(2) - (2)].formula));
  if(!(yyval.formula))
    YYERROR_MSG("TriplesSameSubject 2: formula join failed");

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "  after joining formula=");
  rasqal_formula_print((yyval.formula), DEBUG_FH);
  fprintf(DEBUG_FH, "\n\n");
#endif
}
    break;

  case 234:

/* Line 1806 of yacc.c  */
#line 3682 "./sparql_parser.y"
    {
  int i;
  
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "PropertyList 1\n  Verb=");
  rasqal_formula_print((yyvsp[(1) - (3)].formula), DEBUG_FH);
  fprintf(DEBUG_FH, "\n  ObjectList=");
  rasqal_formula_print((yyvsp[(2) - (3)].formula), DEBUG_FH);
  fprintf(DEBUG_FH, "\n  PropertyListTail=");
  if((yyvsp[(3) - (3)].formula) != NULL)
    rasqal_formula_print((yyvsp[(3) - (3)].formula), DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fprintf(DEBUG_FH, "\n");
#endif
  
  if((yyvsp[(2) - (3)].formula) == NULL) {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
    fprintf(DEBUG_FH, " empty ObjectList not processed\n");
#endif
  } else if((yyvsp[(1) - (3)].formula) && (yyvsp[(2) - (3)].formula)) {
    raptor_sequence *seq = (yyvsp[(2) - (3)].formula)->triples;
    rasqal_literal *predicate = (yyvsp[(1) - (3)].formula)->value;
    rasqal_formula *formula;
    rasqal_triple *t2;
    int size;
    
    formula = rasqal_new_formula(((rasqal_query*)rq)->world);
    if(!formula) {
      rasqal_free_formula((yyvsp[(1) - (3)].formula));
      rasqal_free_formula((yyvsp[(2) - (3)].formula));
      if((yyvsp[(3) - (3)].formula))
        rasqal_free_formula((yyvsp[(3) - (3)].formula));
      YYERROR_MSG("PropertyList 1: cannot create formula");
    }
    formula->triples = raptor_new_sequence((raptor_data_free_handler)rasqal_free_triple,
                                           (raptor_data_print_handler)rasqal_triple_print);
    if(!formula->triples) {
      rasqal_free_formula(formula);
      rasqal_free_formula((yyvsp[(1) - (3)].formula));
      rasqal_free_formula((yyvsp[(2) - (3)].formula));
      if((yyvsp[(3) - (3)].formula))
        rasqal_free_formula((yyvsp[(3) - (3)].formula));
      YYERROR_MSG("PropertyList 1: cannot create sequence");
    }

    /* non-empty property list, handle it  */
    size = raptor_sequence_size(seq);
    for(i = 0; i < size; i++) {
      t2 = (rasqal_triple*)raptor_sequence_get_at(seq, i);
      if(!t2->predicate)
        t2->predicate = (rasqal_literal*)rasqal_new_literal_from_literal(predicate);
    }
  
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
    fprintf(DEBUG_FH, "  after substitution ObjectList=");
    raptor_sequence_print(seq, DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
#endif

    while(raptor_sequence_size(seq)) {
      t2 = (rasqal_triple*)raptor_sequence_unshift(seq);
      if(raptor_sequence_push(formula->triples, t2)) {
        rasqal_free_formula(formula);
        rasqal_free_formula((yyvsp[(1) - (3)].formula));
        rasqal_free_formula((yyvsp[(2) - (3)].formula));
        if((yyvsp[(3) - (3)].formula))
          rasqal_free_formula((yyvsp[(3) - (3)].formula));
        YYERROR_MSG("PropertyList 1: sequence push failed");
      }
    }

    (yyvsp[(3) - (3)].formula) = rasqal_formula_join(formula, (yyvsp[(3) - (3)].formula));
    if(!(yyvsp[(3) - (3)].formula)) {
      rasqal_free_formula((yyvsp[(1) - (3)].formula));
      rasqal_free_formula((yyvsp[(2) - (3)].formula));
      YYERROR_MSG("PropertyList 1: formula join failed");
    }

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
    fprintf(DEBUG_FH, "  after appending ObjectList=");
    rasqal_formula_print((yyvsp[(3) - (3)].formula), DEBUG_FH);
    fprintf(DEBUG_FH, "\n\n");
#endif

    rasqal_free_formula((yyvsp[(2) - (3)].formula));
  }

  if((yyvsp[(1) - (3)].formula))
    rasqal_free_formula((yyvsp[(1) - (3)].formula));

  (yyval.formula) = (yyvsp[(3) - (3)].formula);
}
    break;

  case 235:

/* Line 1806 of yacc.c  */
#line 3780 "./sparql_parser.y"
    {
  (yyval.formula) = (yyvsp[(2) - (2)].formula);
}
    break;

  case 236:

/* Line 1806 of yacc.c  */
#line 3784 "./sparql_parser.y"
    {
  (yyval.formula) = NULL;
}
    break;

  case 237:

/* Line 1806 of yacc.c  */
#line 3792 "./sparql_parser.y"
    {
  (yyval.formula) = (yyvsp[(1) - (1)].formula);
}
    break;

  case 238:

/* Line 1806 of yacc.c  */
#line 3796 "./sparql_parser.y"
    {
  (yyval.formula) = NULL;
}
    break;

  case 239:

/* Line 1806 of yacc.c  */
#line 3804 "./sparql_parser.y"
    {
  rasqal_formula *formula;
  rasqal_triple *triple;

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "ObjectList 1\n");
  fprintf(DEBUG_FH, "  Object=\n");
  rasqal_formula_print((yyvsp[(1) - (2)].formula), DEBUG_FH);
  fprintf(DEBUG_FH, "\n");
  if((yyvsp[(2) - (2)].formula)) {
    fprintf(DEBUG_FH, "  ObjectTail=");
    rasqal_formula_print((yyvsp[(2) - (2)].formula), DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
  } else
    fprintf(DEBUG_FH, "  and empty ObjectTail\n");
#endif

  formula = rasqal_new_formula(((rasqal_query*)rq)->world);
  if(!formula) {
    rasqal_free_formula((yyvsp[(1) - (2)].formula));
    if((yyvsp[(2) - (2)].formula))
      rasqal_free_formula((yyvsp[(2) - (2)].formula));
    YYERROR_MSG("ObjectList: cannot create formula");
  }
  
  formula->triples = raptor_new_sequence((raptor_data_free_handler)rasqal_free_triple,
                                         (raptor_data_print_handler)rasqal_triple_print);
  if(!formula->triples) {
    rasqal_free_formula(formula);
    rasqal_free_formula((yyvsp[(1) - (2)].formula));
    if((yyvsp[(2) - (2)].formula))
      rasqal_free_formula((yyvsp[(2) - (2)].formula));
    YYERROR_MSG("ObjectList: cannot create sequence");
  }

  triple = rasqal_new_triple(NULL, NULL, (yyvsp[(1) - (2)].formula)->value);
  (yyvsp[(1) - (2)].formula)->value = NULL; /* value now owned by triple */
  if(!triple) {
    rasqal_free_formula(formula);
    rasqal_free_formula((yyvsp[(1) - (2)].formula));
    if((yyvsp[(2) - (2)].formula))
      rasqal_free_formula((yyvsp[(2) - (2)].formula));
    YYERROR_MSG("ObjectList: cannot create triple");
  }

  if(raptor_sequence_push(formula->triples, triple)) {
    rasqal_free_formula(formula);
    rasqal_free_formula((yyvsp[(1) - (2)].formula));
    if((yyvsp[(2) - (2)].formula))
      rasqal_free_formula((yyvsp[(2) - (2)].formula));
    YYERROR_MSG("ObjectList: sequence push failed");
  }

  (yyval.formula) = rasqal_formula_join(formula, (yyvsp[(1) - (2)].formula));
  if(!(yyval.formula)) {
    if((yyvsp[(2) - (2)].formula))
      rasqal_free_formula((yyvsp[(2) - (2)].formula));
    YYERROR_MSG("ObjectList: formula join $1 failed");
  }

  (yyval.formula) = rasqal_formula_join((yyval.formula), (yyvsp[(2) - (2)].formula));
  if(!(yyval.formula))
    YYERROR_MSG("ObjectList: formula join $2 failed");

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "  objectList is now ");
  if((yyval.formula))
    raptor_sequence_print((yyval.formula)->triples, DEBUG_FH);
  else
    fputs("NULL", DEBUG_FH);
  fprintf(DEBUG_FH, "\n\n");
#endif
}
    break;

  case 240:

/* Line 1806 of yacc.c  */
#line 3882 "./sparql_parser.y"
    {
  (yyval.formula) = (yyvsp[(2) - (2)].formula);
}
    break;

  case 241:

/* Line 1806 of yacc.c  */
#line 3886 "./sparql_parser.y"
    {
  (yyval.formula) = NULL;
}
    break;

  case 242:

/* Line 1806 of yacc.c  */
#line 3894 "./sparql_parser.y"
    {
  (yyval.formula) = (yyvsp[(1) - (1)].formula);
}
    break;

  case 243:

/* Line 1806 of yacc.c  */
#line 3902 "./sparql_parser.y"
    {
  (yyval.formula) = rasqal_new_formula(((rasqal_query*)rq)->world);
  if(!(yyval.formula)) {
    if((yyvsp[(1) - (1)].literal))
      rasqal_free_literal((yyvsp[(1) - (1)].literal));
    YYERROR_MSG("Verb 1: cannot create formula");
  }
  (yyval.formula)->value = (yyvsp[(1) - (1)].literal);
}
    break;

  case 244:

/* Line 1806 of yacc.c  */
#line 3912 "./sparql_parser.y"
    {
  raptor_uri *uri;

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "verb Verb=rdf:type (a)\n");
#endif

  uri = raptor_new_uri_for_rdf_concept(((rasqal_query*)rq)->world->raptor_world_ptr,
                                       RASQAL_GOOD_CAST(const unsigned char*, "type"));
  if(!uri)
    YYERROR_MSG("Verb 2: uri for rdf concept type failed");
  (yyval.formula) = rasqal_new_formula(((rasqal_query*)rq)->world);
  if(!(yyval.formula)) {
    raptor_free_uri(uri);
    YYERROR_MSG("Verb 2: cannot create formula");
  }
  (yyval.formula)->value = rasqal_new_uri_literal(((rasqal_query*)rq)->world, uri);
  if(!(yyval.formula)->value) {
    rasqal_free_formula((yyval.formula));
    (yyval.formula) = NULL;
    YYERROR_MSG("Verb 2: cannot create uri literal");
  }
}
    break;

  case 245:

/* Line 1806 of yacc.c  */
#line 3940 "./sparql_parser.y"
    {
  (yyval.formula) = (yyvsp[(1) - (1)].formula);
}
    break;

  case 246:

/* Line 1806 of yacc.c  */
#line 3944 "./sparql_parser.y"
    {
  (yyval.formula) = (yyvsp[(1) - (1)].formula);
}
    break;

  case 247:

/* Line 1806 of yacc.c  */
#line 3952 "./sparql_parser.y"
    {
  int i;
  const unsigned char *id;

  if((yyvsp[(2) - (3)].formula) == NULL) {
    (yyval.formula) = rasqal_new_formula(((rasqal_query*)rq)->world);
    if(!(yyval.formula))
      YYERROR_MSG("BlankNodePropertyList: cannot create formula");
  } else {
    (yyval.formula) = (yyvsp[(2) - (3)].formula);
    if((yyval.formula)->value) {
      rasqal_free_literal((yyval.formula)->value);
      (yyval.formula)->value = NULL;
    }
  }
  
  id = rasqal_query_generate_bnodeid((rasqal_query*)rq, NULL);
  if(!id) {
    rasqal_free_formula((yyval.formula));
    (yyval.formula) = NULL;
    YYERROR_MSG("BlankNodeProperyList: cannot create bnodeid");
  }

  (yyval.formula)->value = rasqal_new_simple_literal(((rasqal_query*)rq)->world,
                                        RASQAL_LITERAL_BLANK, id);
  if(!(yyval.formula)->value) {
    rasqal_free_formula((yyval.formula));
    (yyval.formula) = NULL;
    YYERROR_MSG("BlankNodePropertyList: cannot create literal");
  }

  if((yyvsp[(2) - (3)].formula) == NULL) {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
    fprintf(DEBUG_FH, "TriplesNode\n  PropertyList=");
    rasqal_formula_print((yyval.formula), DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
#endif
  } else {
    raptor_sequence *seq = (yyvsp[(2) - (3)].formula)->triples;

    /* non-empty property list, handle it  */
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
    fprintf(DEBUG_FH, "TriplesNode\n  PropertyList=");
    raptor_sequence_print(seq, DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
#endif

    for(i = 0; i<raptor_sequence_size(seq); i++) {
      rasqal_triple* t2 = (rasqal_triple*)raptor_sequence_get_at(seq, i);
      if(t2->subject)
        continue;
      
      t2->subject = (rasqal_literal*)rasqal_new_literal_from_literal((yyval.formula)->value);
    }

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1
    fprintf(DEBUG_FH, "  after substitution formula=");
    rasqal_formula_print((yyval.formula), DEBUG_FH);
    fprintf(DEBUG_FH, "\n\n");
#endif
  }
}
    break;

  case 248:

/* Line 1806 of yacc.c  */
#line 4019 "./sparql_parser.y"
    {
  int i;
  rasqal_query* rdf_query = (rasqal_query*)rq;
  rasqal_literal* first_identifier = NULL;
  rasqal_literal* rest_identifier = NULL;
  rasqal_literal* object = NULL;
  rasqal_literal* blank = NULL;

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1
  char const *errmsg;
  #define YYERR_MSG_GOTO(label,msg) do { errmsg = msg; goto label; } while(0)
#else
  #define YYERR_MSG_GOTO(label,ignore) goto label
#endif

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "Collection\n  GraphNodeListNotEmpty=");
  raptor_sequence_print((yyvsp[(2) - (3)].seq), DEBUG_FH);
  fprintf(DEBUG_FH, "\n");
#endif

  (yyval.formula) = rasqal_new_formula(((rasqal_query*)rq)->world);
  if(!(yyval.formula))
    YYERR_MSG_GOTO(err_Collection, "Collection: cannot create formula");

  (yyval.formula)->triples = raptor_new_sequence((raptor_data_free_handler)rasqal_free_triple,
                                    (raptor_data_print_handler)rasqal_triple_print);
  if(!(yyval.formula)->triples)
    YYERR_MSG_GOTO(err_Collection, "Collection: cannot create sequence");

  first_identifier = rasqal_new_uri_literal(rdf_query->world,
                                            raptor_uri_copy(rdf_query->world->rdf_first_uri));
  if(!first_identifier)
    YYERR_MSG_GOTO(err_Collection, "Collection: cannot first_identifier");
  
  rest_identifier = rasqal_new_uri_literal(rdf_query->world,
                                           raptor_uri_copy(rdf_query->world->rdf_rest_uri));
  if(!rest_identifier)
    YYERR_MSG_GOTO(err_Collection, "Collection: cannot create rest_identifier");
  
  object = rasqal_new_uri_literal(rdf_query->world,
                                  raptor_uri_copy(rdf_query->world->rdf_nil_uri));
  if(!object)
    YYERR_MSG_GOTO(err_Collection, "Collection: cannot create nil object");

  for(i = raptor_sequence_size((yyvsp[(2) - (3)].seq))-1; i >= 0; i--) {
    rasqal_formula* f = (rasqal_formula*)raptor_sequence_get_at((yyvsp[(2) - (3)].seq), i);
    rasqal_triple *t2;
    const unsigned char *blank_id = NULL;

    blank_id = rasqal_query_generate_bnodeid(rdf_query, NULL);
    if(!blank_id)
      YYERR_MSG_GOTO(err_Collection, "Collection: cannot create bnodeid");

    blank = rasqal_new_simple_literal(((rasqal_query*)rq)->world, RASQAL_LITERAL_BLANK, blank_id);
    if(!blank)
      YYERR_MSG_GOTO(err_Collection, "Collection: cannot create bnode");

    /* Move existing formula triples */
    if(f->triples)
      if(raptor_sequence_join((yyval.formula)->triples, f->triples))
        YYERR_MSG_GOTO(err_Collection, "Collection: sequence join failed");

    /* add new triples we needed */
    t2 = rasqal_new_triple(rasqal_new_literal_from_literal(blank),
                           rasqal_new_literal_from_literal(first_identifier),
                           rasqal_new_literal_from_literal(f->value));
    if(!t2)
      YYERR_MSG_GOTO(err_Collection, "Collection: cannot create triple");

    if(raptor_sequence_push((yyval.formula)->triples, t2))
      YYERR_MSG_GOTO(err_Collection, "Collection: cannot create triple");

    t2 = rasqal_new_triple(rasqal_new_literal_from_literal(blank),
                           rasqal_new_literal_from_literal(rest_identifier),
                           rasqal_new_literal_from_literal(object));
    if(!t2)
      YYERR_MSG_GOTO(err_Collection, "Collection: cannot create triple 2");

    if(raptor_sequence_push((yyval.formula)->triples, t2))
      YYERR_MSG_GOTO(err_Collection, "Collection: sequence push 2 failed");

    rasqal_free_literal(object);
    object=blank;
    blank = NULL;
  }

  /* free sequence of formulas just processed */
  raptor_free_sequence((yyvsp[(2) - (3)].seq));
  
  (yyval.formula)->value=object;
  
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1
  fprintf(DEBUG_FH, "  after substitution collection=");
  rasqal_formula_print((yyval.formula), DEBUG_FH);
  fprintf(DEBUG_FH, "\n\n");
#endif

  rasqal_free_literal(first_identifier);
  rasqal_free_literal(rest_identifier);

  break; /* success */

  err_Collection:
  
  if(blank)
    rasqal_free_literal(blank);
  if(object)
    rasqal_free_literal(object);
  if(rest_identifier)
    rasqal_free_literal(rest_identifier);
  if(first_identifier)
    rasqal_free_literal(first_identifier);
  if((yyvsp[(2) - (3)].seq))
    raptor_free_sequence((yyvsp[(2) - (3)].seq));
  if((yyval.formula)) {
    rasqal_free_formula((yyval.formula));
    (yyval.formula) = NULL;
  }
  YYERROR_MSG(errmsg);
}
    break;

  case 249:

/* Line 1806 of yacc.c  */
#line 4146 "./sparql_parser.y"
    {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  char const *errmsg;

  fprintf(DEBUG_FH, "GraphNodeListNotEmpty 1\n");
  if((yyvsp[(2) - (2)].formula)) {
    fprintf(DEBUG_FH, "  GraphNode=");
    rasqal_formula_print((yyvsp[(2) - (2)].formula), DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
  } else  
    fprintf(DEBUG_FH, "  and empty GraphNode\n");
  if((yyvsp[(1) - (2)].seq)) {
    fprintf(DEBUG_FH, "  GraphNodeListNotEmpty=");
    raptor_sequence_print((yyvsp[(1) - (2)].seq), DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
  } else
    fprintf(DEBUG_FH, "  and empty GraphNodeListNotEmpty\n");
#endif

  (yyval.seq) = (yyvsp[(1) - (2)].seq);
  if(!(yyval.seq)) {
    (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_formula,
                             (raptor_data_print_handler)rasqal_formula_print);
    if(!(yyval.seq))
      YYERR_MSG_GOTO(err_GraphNodeListNotEmpty,
                     "GraphNodeListNotEmpty: cannot create formula");
  }
  
  if((yyvsp[(2) - (2)].formula)) {
    if(raptor_sequence_push((yyval.seq), (yyvsp[(2) - (2)].formula))) {
      YYERR_MSG_GOTO(err_GraphNodeListNotEmpty,
                     "GraphNodeListNotEmpty 1: sequence push failed");
    }
    (yyvsp[(2) - (2)].formula) = NULL;
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
    fprintf(DEBUG_FH, "  itemList is now ");
    raptor_sequence_print((yyval.seq), DEBUG_FH);
    fprintf(DEBUG_FH, "\n\n");
#endif
  }

  break; /* success */

  err_GraphNodeListNotEmpty:
  if((yyvsp[(2) - (2)].formula))
    rasqal_free_formula((yyvsp[(2) - (2)].formula));
  if((yyval.seq)) {
    raptor_free_sequence((yyval.seq));
    (yyval.seq) = NULL;
  }
  YYERROR_MSG(errmsg);
}
    break;

  case 250:

/* Line 1806 of yacc.c  */
#line 4199 "./sparql_parser.y"
    {
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "GraphNodeListNotEmpty 2\n");
  if((yyvsp[(1) - (1)].formula)) {
    fprintf(DEBUG_FH, "  GraphNode=");
    rasqal_formula_print((yyvsp[(1) - (1)].formula), DEBUG_FH);
    fprintf(DEBUG_FH, "\n");
  } else  
    fprintf(DEBUG_FH, "  and empty GraphNode\n");
#endif

  if(!(yyvsp[(1) - (1)].formula))
    (yyval.seq) = NULL;
  else {
    (yyval.seq) = raptor_new_sequence((raptor_data_free_handler)rasqal_free_formula,
                             (raptor_data_print_handler)rasqal_formula_print);
    if(!(yyval.seq)) {
      rasqal_free_formula((yyvsp[(1) - (1)].formula));
      YYERROR_MSG("GraphNodeListNotEmpty 2: cannot create sequence");
    }
    if(raptor_sequence_push((yyval.seq), (yyvsp[(1) - (1)].formula))) {
      raptor_free_sequence((yyval.seq));
      (yyval.seq) = NULL;
      YYERROR_MSG("GraphNodeListNotEmpty 2: sequence push failed");
    }
  }
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 1  
  fprintf(DEBUG_FH, "  GraphNodeListNotEmpty is now ");
  raptor_sequence_print((yyval.seq), DEBUG_FH);
  fprintf(DEBUG_FH, "\n\n");
#endif
}
    break;

  case 251:

/* Line 1806 of yacc.c  */
#line 4236 "./sparql_parser.y"
    {
  (yyval.formula) = (yyvsp[(1) - (1)].formula);
}
    break;

  case 252:

/* Line 1806 of yacc.c  */
#line 4240 "./sparql_parser.y"
    {
  (yyval.formula) = (yyvsp[(1) - (1)].formula);
}
    break;

  case 253:

/* Line 1806 of yacc.c  */
#line 4248 "./sparql_parser.y"
    {
  (yyval.formula) = rasqal_new_formula(((rasqal_query*)rq)->world);
  if(!(yyval.formula))
    YYERROR_MSG("VarOrTerm 1: cannot create formula");
  (yyval.formula)->value = rasqal_new_variable_literal(((rasqal_query*)rq)->world, (yyvsp[(1) - (1)].variable));
  if(!(yyval.formula)->value) {
    rasqal_free_formula((yyval.formula));
    (yyval.formula) = NULL;
    YYERROR_MSG("VarOrTerm 1: cannot create literal");
  }
}
    break;

  case 254:

/* Line 1806 of yacc.c  */
#line 4260 "./sparql_parser.y"
    {
  (yyval.formula) = rasqal_new_formula(((rasqal_query*)rq)->world);
  if(!(yyval.formula)) {
    if((yyvsp[(1) - (1)].literal))
      rasqal_free_literal((yyvsp[(1) - (1)].literal));
    YYERROR_MSG("VarOrTerm 2: cannot create formula");
  }
  (yyval.formula)->value = (yyvsp[(1) - (1)].literal);
}
    break;

  case 255:

/* Line 1806 of yacc.c  */
#line 4273 "./sparql_parser.y"
    {
  (yyval.literal) = rasqal_new_variable_literal(((rasqal_query*)rq)->world, (yyvsp[(1) - (1)].variable));
  if(!(yyval.literal))
    YYERROR_MSG("VarOrIRIref: cannot create literal");
}
    break;

  case 256:

/* Line 1806 of yacc.c  */
#line 4279 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 257:

/* Line 1806 of yacc.c  */
#line 4287 "./sparql_parser.y"
    {
  (yyval.variable) = (yyvsp[(2) - (2)].variable);
}
    break;

  case 258:

/* Line 1806 of yacc.c  */
#line 4291 "./sparql_parser.y"
    {
  (yyval.variable) = (yyvsp[(2) - (2)].variable);
}
    break;

  case 259:

/* Line 1806 of yacc.c  */
#line 4298 "./sparql_parser.y"
    {
  (yyval.variable) = rasqal_variables_table_add(((rasqal_query*)rq)->vars_table,
                                  RASQAL_VARIABLE_TYPE_NORMAL, (yyvsp[(1) - (1)].name), NULL);
  if(!(yyval.variable))
    YYERROR_MSG("VarName: cannot create var");
}
    break;

  case 260:

/* Line 1806 of yacc.c  */
#line 4309 "./sparql_parser.y"
    {
  (yyval.variable) = (yyvsp[(2) - (2)].variable);
}
    break;

  case 261:

/* Line 1806 of yacc.c  */
#line 4313 "./sparql_parser.y"
    {
  (yyval.variable) = (yyvsp[(2) - (2)].variable);
}
    break;

  case 262:

/* Line 1806 of yacc.c  */
#line 4317 "./sparql_parser.y"
    {
  (yyval.variable) = (yyvsp[(1) - (1)].variable);
  sparql_syntax_warning(((rasqal_query*)rq), 
                        "... AS varname is deprecated LAQRS syntax, use ... AS ?varname");
}
    break;

  case 263:

/* Line 1806 of yacc.c  */
#line 4327 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 264:

/* Line 1806 of yacc.c  */
#line 4331 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 265:

/* Line 1806 of yacc.c  */
#line 4335 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 266:

/* Line 1806 of yacc.c  */
#line 4339 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 267:

/* Line 1806 of yacc.c  */
#line 4343 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 268:

/* Line 1806 of yacc.c  */
#line 4347 "./sparql_parser.y"
    {
  (yyval.literal) = rasqal_new_uri_literal(((rasqal_query*)rq)->world, 
                              raptor_uri_copy(((rasqal_query*)rq)->world->rdf_nil_uri));
  if(!(yyval.literal))
    YYERROR_MSG("GraphTerm: cannot create literal");
}
    break;

  case 269:

/* Line 1806 of yacc.c  */
#line 4357 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 270:

/* Line 1806 of yacc.c  */
#line 4365 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_OR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("ConditionalOrExpression: cannot create expr");
}
    break;

  case 271:

/* Line 1806 of yacc.c  */
#line 4372 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 272:

/* Line 1806 of yacc.c  */
#line 4380 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("ConditionalAndExpression: cannot create expr");
;
}
    break;

  case 273:

/* Line 1806 of yacc.c  */
#line 4388 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 274:

/* Line 1806 of yacc.c  */
#line 4397 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_EQ, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("RelationalExpression 1: cannot create expr");
}
    break;

  case 275:

/* Line 1806 of yacc.c  */
#line 4404 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_NEQ, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("RelationalExpression 2: cannot create expr");
}
    break;

  case 276:

/* Line 1806 of yacc.c  */
#line 4411 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_LT, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("RelationalExpression 3: cannot create expr");
}
    break;

  case 277:

/* Line 1806 of yacc.c  */
#line 4418 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_GT, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("RelationalExpression 4: cannot create expr");
}
    break;

  case 278:

/* Line 1806 of yacc.c  */
#line 4425 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_LE, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("RelationalExpression 5: cannot create expr");
}
    break;

  case 279:

/* Line 1806 of yacc.c  */
#line 4432 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_GE, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("RelationalExpression 6: cannot create expr");
}
    break;

  case 280:

/* Line 1806 of yacc.c  */
#line 4439 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_set_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_IN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].seq));
}
    break;

  case 281:

/* Line 1806 of yacc.c  */
#line 4444 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_set_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_NOT_IN, (yyvsp[(1) - (4)].expr), (yyvsp[(4) - (4)].seq));
}
    break;

  case 282:

/* Line 1806 of yacc.c  */
#line 4449 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 283:

/* Line 1806 of yacc.c  */
#line 4458 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_PLUS, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("AdditiveExpression 1: cannot create expr");
}
    break;

  case 284:

/* Line 1806 of yacc.c  */
#line 4465 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_MINUS, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("AdditiveExpression 2: cannot create expr");
}
    break;

  case 285:

/* Line 1806 of yacc.c  */
#line 4472 "./sparql_parser.y"
    {
  rasqal_expression *e;
  e = rasqal_new_literal_expression(((rasqal_query*)rq)->world, (yyvsp[(2) - (2)].literal));
  if(!e) {
    rasqal_free_expression((yyvsp[(1) - (2)].expr));
    YYERROR_MSG("AdditiveExpression 3: cannot create expr");
  }
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_PLUS, (yyvsp[(1) - (2)].expr), e);
  if(!(yyval.expr))
    YYERROR_MSG("AdditiveExpression 4: cannot create expr");
}
    break;

  case 286:

/* Line 1806 of yacc.c  */
#line 4485 "./sparql_parser.y"
    {
  rasqal_expression *e;
  e = rasqal_new_literal_expression(((rasqal_query*)rq)->world, (yyvsp[(2) - (2)].literal));
  if(!e) {
    rasqal_free_expression((yyvsp[(1) - (2)].expr));
    YYERROR_MSG("AdditiveExpression 5: cannot create expr");
  }
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_PLUS, (yyvsp[(1) - (2)].expr), e);
  if(!(yyval.expr))
    YYERROR_MSG("AdditiveExpression 6: cannot create expr");
}
    break;

  case 287:

/* Line 1806 of yacc.c  */
#line 4498 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 288:

/* Line 1806 of yacc.c  */
#line 4505 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_STAR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("MultiplicativeExpression 1: cannot create expr");
}
    break;

  case 289:

/* Line 1806 of yacc.c  */
#line 4512 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_SLASH, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("MultiplicativeExpression 2: cannot create expr");
}
    break;

  case 290:

/* Line 1806 of yacc.c  */
#line 4519 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 291:

/* Line 1806 of yacc.c  */
#line 4527 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_BANG, (yyvsp[(2) - (2)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("UnaryExpression 1: cannot create expr");
}
    break;

  case 292:

/* Line 1806 of yacc.c  */
#line 4534 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(2) - (2)].expr);
}
    break;

  case 293:

/* Line 1806 of yacc.c  */
#line 4538 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_UMINUS, (yyvsp[(2) - (2)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("UnaryExpression 3: cannot create expr");
}
    break;

  case 294:

/* Line 1806 of yacc.c  */
#line 4545 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 295:

/* Line 1806 of yacc.c  */
#line 4559 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 296:

/* Line 1806 of yacc.c  */
#line 4563 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 297:

/* Line 1806 of yacc.c  */
#line 4567 "./sparql_parser.y"
    {
  /* Grammar has IRIrefOrFunction here which is "IRIref ArgList?"
   * and essentially shorthand for FunctionCall | IRIref.  The Rasqal
   * SPARQL lexer distinguishes these for us with IRIrefBrace.
   * IRIref is covered below by GraphTerm.
   */
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 298:

/* Line 1806 of yacc.c  */
#line 4576 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_literal_expression(((rasqal_query*)rq)->world, (yyvsp[(1) - (1)].literal));
  if(!(yyval.expr))
    YYERROR_MSG("PrimaryExpression 4: cannot create expr");
}
    break;

  case 299:

/* Line 1806 of yacc.c  */
#line 4582 "./sparql_parser.y"
    {
  rasqal_literal *l;
  l = rasqal_new_variable_literal(((rasqal_query*)rq)->world, (yyvsp[(1) - (1)].variable));
  if(!l)
    YYERROR_MSG("PrimaryExpression 5: cannot create literal");
  (yyval.expr) = rasqal_new_literal_expression(((rasqal_query*)rq)->world, l);
  if(!(yyval.expr))
    YYERROR_MSG("PrimaryExpression 5: cannot create expr");
}
    break;

  case 300:

/* Line 1806 of yacc.c  */
#line 4592 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 301:

/* Line 1806 of yacc.c  */
#line 4600 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(2) - (3)].expr);
}
    break;

  case 302:

/* Line 1806 of yacc.c  */
#line 4608 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_STR, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 1: cannot create expr");
}
    break;

  case 303:

/* Line 1806 of yacc.c  */
#line 4615 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_LANG, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 2: cannot create expr");
}
    break;

  case 304:

/* Line 1806 of yacc.c  */
#line 4622 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_LANGMATCHES, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 3: cannot create expr");
}
    break;

  case 305:

/* Line 1806 of yacc.c  */
#line 4629 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_DATATYPE, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 4: cannot create expr");
}
    break;

  case 306:

/* Line 1806 of yacc.c  */
#line 4636 "./sparql_parser.y"
    {
  rasqal_literal *l;
  rasqal_expression *e;
  l = rasqal_new_variable_literal(((rasqal_query*)rq)->world, (yyvsp[(3) - (4)].variable));
  if(!l)
    YYERROR_MSG("BuiltInCall 5: cannot create literal");
  e = rasqal_new_literal_expression(((rasqal_query*)rq)->world, l);
  if(!e)
    YYERROR_MSG("BuiltInCall 6: cannot create literal expr");
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_BOUND, e);
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7: cannot create expr");
}
    break;

  case 307:

/* Line 1806 of yacc.c  */
#line 4651 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_IRI, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7a: cannot create expr");
}
    break;

  case 308:

/* Line 1806 of yacc.c  */
#line 4658 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_IRI, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7b: cannot create expr");
}
    break;

  case 309:

/* Line 1806 of yacc.c  */
#line 4665 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_BNODE, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7c: cannot create expr");
}
    break;

  case 310:

/* Line 1806 of yacc.c  */
#line 4672 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_BNODE, NULL);
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7d: cannot create expr");
}
    break;

  case 311:

/* Line 1806 of yacc.c  */
#line 4679 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_0op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_RAND);
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7e: cannot create expr");
}
    break;

  case 312:

/* Line 1806 of yacc.c  */
#line 4686 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_ABS, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7f: cannot create expr");
}
    break;

  case 313:

/* Line 1806 of yacc.c  */
#line 4693 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_CEIL, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7g: cannot create expr");
}
    break;

  case 314:

/* Line 1806 of yacc.c  */
#line 4700 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_FLOOR, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7h: cannot create expr");
}
    break;

  case 315:

/* Line 1806 of yacc.c  */
#line 4707 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_ROUND, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7i: cannot create expr");
}
    break;

  case 316:

/* Line 1806 of yacc.c  */
#line 4714 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_MD5, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7j: cannot create expr");
}
    break;

  case 317:

/* Line 1806 of yacc.c  */
#line 4721 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_SHA1, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7k: cannot create expr");
}
    break;

  case 318:

/* Line 1806 of yacc.c  */
#line 4728 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_SHA224, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7l: cannot create expr");
}
    break;

  case 319:

/* Line 1806 of yacc.c  */
#line 4735 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_SHA256, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7m: cannot create expr");
}
    break;

  case 320:

/* Line 1806 of yacc.c  */
#line 4742 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_SHA384, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7n: cannot create expr");
}
    break;

  case 321:

/* Line 1806 of yacc.c  */
#line 4749 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_SHA512, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7o: cannot create expr");
}
    break;

  case 322:

/* Line 1806 of yacc.c  */
#line 4756 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_0op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_UUID);
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7p: cannot create expr");
}
    break;

  case 323:

/* Line 1806 of yacc.c  */
#line 4763 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_0op_expression(((rasqal_query*)rq)->world, 
                                 RASQAL_EXPR_STRUUID);
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7q: cannot create expr");
}
    break;

  case 324:

/* Line 1806 of yacc.c  */
#line 4770 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 325:

/* Line 1806 of yacc.c  */
#line 4774 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 326:

/* Line 1806 of yacc.c  */
#line 4778 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_3op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_IF, (yyvsp[(3) - (8)].expr), (yyvsp[(5) - (8)].expr), (yyvsp[(7) - (8)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7e: cannot create expr");
}
    break;

  case 327:

/* Line 1806 of yacc.c  */
#line 4785 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_STRLANG, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7f: cannot create expr");
}
    break;

  case 328:

/* Line 1806 of yacc.c  */
#line 4792 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_STRDT, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 7g: cannot create expr");
}
    break;

  case 329:

/* Line 1806 of yacc.c  */
#line 4799 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_SAMETERM, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 8: cannot create expr");
}
    break;

  case 330:

/* Line 1806 of yacc.c  */
#line 4806 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ISURI, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 9: cannot create expr");
}
    break;

  case 331:

/* Line 1806 of yacc.c  */
#line 4813 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ISBLANK, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 10: cannot create expr");
}
    break;

  case 332:

/* Line 1806 of yacc.c  */
#line 4820 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ISLITERAL, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 11: cannot create expr");
}
    break;

  case 333:

/* Line 1806 of yacc.c  */
#line 4827 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ISNUMERIC, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("BuiltInCall 12: cannot create expr");
}
    break;

  case 334:

/* Line 1806 of yacc.c  */
#line 4834 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 335:

/* Line 1806 of yacc.c  */
#line 4838 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 336:

/* Line 1806 of yacc.c  */
#line 4842 "./sparql_parser.y"
    {
  (yyval.expr) = (yyvsp[(1) - (1)].expr);
}
    break;

  case 337:

/* Line 1806 of yacc.c  */
#line 4849 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_STRLEN, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create STRLEN() expr");
}
    break;

  case 338:

/* Line 1806 of yacc.c  */
#line 4856 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_3op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_SUBSTR, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr), NULL);
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create SUBSTR() expr");
}
    break;

  case 339:

/* Line 1806 of yacc.c  */
#line 4863 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_3op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_SUBSTR, (yyvsp[(3) - (8)].expr), (yyvsp[(5) - (8)].expr), (yyvsp[(7) - (8)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create SUBSTR() expr");
}
    break;

  case 340:

/* Line 1806 of yacc.c  */
#line 4870 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_UCASE, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create UCASE() expr");
}
    break;

  case 341:

/* Line 1806 of yacc.c  */
#line 4877 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_LCASE, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create LCASE() expr");
}
    break;

  case 342:

/* Line 1806 of yacc.c  */
#line 4884 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_STRSTARTS, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create STRSTARTS() expr");
}
    break;

  case 343:

/* Line 1806 of yacc.c  */
#line 4891 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_STRENDS, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create STRENDS() expr");
}
    break;

  case 344:

/* Line 1806 of yacc.c  */
#line 4898 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_CONTAINS, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create YEAR expr");
}
    break;

  case 345:

/* Line 1806 of yacc.c  */
#line 4905 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_ENCODE_FOR_URI, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create ENCODE_FOR_URI() expr");
}
    break;

  case 346:

/* Line 1806 of yacc.c  */
#line 4912 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_expr_seq_expression(((rasqal_query*)rq)->world, 
                                      RASQAL_EXPR_CONCAT, (yyvsp[(3) - (4)].seq));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create CONCAT() expr");
}
    break;

  case 347:

/* Line 1806 of yacc.c  */
#line 4919 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_STRBEFORE, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create STRBEFORE() expr");
}
    break;

  case 348:

/* Line 1806 of yacc.c  */
#line 4926 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_2op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_STRAFTER, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create STRAFTER() expr");
}
    break;

  case 349:

/* Line 1806 of yacc.c  */
#line 4933 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_3op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_REPLACE, (yyvsp[(3) - (8)].expr), (yyvsp[(5) - (8)].expr), (yyvsp[(7) - (8)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create REPLACE() expr");
}
    break;

  case 350:

/* Line 1806 of yacc.c  */
#line 4940 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_4op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_REPLACE, (yyvsp[(3) - (10)].expr), (yyvsp[(5) - (10)].expr), (yyvsp[(7) - (10)].expr), (yyvsp[(9) - (10)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("StringExpression: cannot create REPLACE() expr");
}
    break;

  case 351:

/* Line 1806 of yacc.c  */
#line 4951 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_3op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_REGEX, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr), NULL);
  if(!(yyval.expr))
    YYERROR_MSG("RegexExpression 1: cannot create expr");
}
    break;

  case 352:

/* Line 1806 of yacc.c  */
#line 4958 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_3op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_REGEX, (yyvsp[(3) - (8)].expr), (yyvsp[(5) - (8)].expr), (yyvsp[(7) - (8)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("RegexExpression 2: cannot create expr");
}
    break;

  case 353:

/* Line 1806 of yacc.c  */
#line 4969 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_YEAR, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("DatetimeBuiltinAccessors: cannot create YEAR expr");
}
    break;

  case 354:

/* Line 1806 of yacc.c  */
#line 4976 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_MONTH, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("DatetimeBuiltinAccessors: cannot create MONTH expr");
}
    break;

  case 355:

/* Line 1806 of yacc.c  */
#line 4983 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_DAY, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("DatetimeBuiltinAccessors: cannot create DAY expr");
}
    break;

  case 356:

/* Line 1806 of yacc.c  */
#line 4990 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_HOURS, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("DatetimeBuiltinAccessors: cannot create HOURS expr");
}
    break;

  case 357:

/* Line 1806 of yacc.c  */
#line 4997 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_MINUTES, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("DatetimeBuiltinAccessors: cannot create MINUTES expr");
}
    break;

  case 358:

/* Line 1806 of yacc.c  */
#line 5004 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_SECONDS, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("DatetimeBuiltinAccessors: cannot create SECONDS expr");
}
    break;

  case 359:

/* Line 1806 of yacc.c  */
#line 5011 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_TIMEZONE, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("DatetimeBuiltinAccessors: cannot create TIMEZONE expr");
}
    break;

  case 360:

/* Line 1806 of yacc.c  */
#line 5018 "./sparql_parser.y"
    {
  (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                 RASQAL_EXPR_TZ, (yyvsp[(3) - (4)].expr));
  if(!(yyval.expr))
    YYERROR_MSG("DatetimeBuiltinAccessors: cannot create TZ expr");
}
    break;

  case 361:

/* Line 1806 of yacc.c  */
#line 5029 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(sparql->experimental) {
    (yyval.expr) = rasqal_new_0op_expression(((rasqal_query*)rq)->world,
                                   RASQAL_EXPR_CURRENT_DATETIME);
    if(!(yyval.expr))
      YYERROR_MSG("DatetimeExtensions: cannot create CURRENT_DATETIME() expr");
  } else {
    sparql_syntax_error((rasqal_query*)rq, 
                        "CURRENT_DATETIME() can only used with LAQRS");
    YYERROR;
  }
}
    break;

  case 362:

/* Line 1806 of yacc.c  */
#line 5046 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);
  
  (yyval.expr) = NULL;
  if(!sparql->sparql11_query) {
    sparql_syntax_error((rasqal_query*)rq,
                        "NOW() cannot be used with SPARQL 1.0");
    YYERROR;
  }
  
  (yyval.expr) = rasqal_new_0op_expression(((rasqal_query*)rq)->world,
                                   RASQAL_EXPR_NOW);
  if(!(yyval.expr))
    YYERROR_MSG("DatetimeExtensions: cannot create NOW()");

}
    break;

  case 363:

/* Line 1806 of yacc.c  */
#line 5064 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(sparql->experimental) {
    (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                   RASQAL_EXPR_FROM_UNIXTIME, (yyvsp[(3) - (4)].expr));
    if(!(yyval.expr))
      YYERROR_MSG("DatetimeExtensions: cannot create FROM_UNIXTIME() expr");
  } else {
    sparql_syntax_error((rasqal_query*)rq, 
                        "FROM_UNIXTIME() can only used with LAQRS");
    YYERROR;
  }
  
}
    break;

  case 364:

/* Line 1806 of yacc.c  */
#line 5082 "./sparql_parser.y"
    {
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)(((rasqal_query*)rq)->context);

  (yyval.expr) = NULL;
  if(sparql->experimental) {
    (yyval.expr) = rasqal_new_1op_expression(((rasqal_query*)rq)->world,
                                   RASQAL_EXPR_TO_UNIXTIME, (yyvsp[(3) - (4)].expr));
    if(!(yyval.expr))
      YYERROR_MSG("DatetimeExtensions: cannot create TO_UNIXTIME() expr");
  } else {
    sparql_syntax_error((rasqal_query*)rq, 
                        "TO_UNIXTIME() can only used with LAQRS");
    YYERROR;
  }
  
}
    break;

  case 365:

/* Line 1806 of yacc.c  */
#line 5107 "./sparql_parser.y"
    {
  (yyval.literal) = rasqal_new_uri_literal(((rasqal_query*)rq)->world, (yyvsp[(1) - (1)].uri));
  if(!(yyval.literal))
    YYERROR_MSG("IRIrefBrace 1: cannot create literal");
}
    break;

  case 366:

/* Line 1806 of yacc.c  */
#line 5113 "./sparql_parser.y"
    {
  (yyval.literal) = rasqal_new_simple_literal(((rasqal_query*)rq)->world,
                                 RASQAL_LITERAL_QNAME, (yyvsp[(1) - (1)].name));
  if(!(yyval.literal))
    YYERROR_MSG("IRIrefBrace 2: cannot create literal");
  if(rasqal_literal_expand_qname((rasqal_query*)rq, (yyval.literal))) {
    sparql_query_error_full((rasqal_query*)rq,
                            "QName %s cannot be expanded", (yyvsp[(1) - (1)].name));
    rasqal_free_literal((yyval.literal));
    (yyval.literal) = NULL;
    YYERROR_MSG("IRIrefBrace 2: cannot expand qname");
  }
}
    break;

  case 367:

/* Line 1806 of yacc.c  */
#line 5133 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 368:

/* Line 1806 of yacc.c  */
#line 5137 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 369:

/* Line 1806 of yacc.c  */
#line 5141 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 370:

/* Line 1806 of yacc.c  */
#line 5148 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 371:

/* Line 1806 of yacc.c  */
#line 5152 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 372:

/* Line 1806 of yacc.c  */
#line 5156 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 373:

/* Line 1806 of yacc.c  */
#line 5164 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 374:

/* Line 1806 of yacc.c  */
#line 5168 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 375:

/* Line 1806 of yacc.c  */
#line 5172 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 376:

/* Line 1806 of yacc.c  */
#line 5180 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 377:

/* Line 1806 of yacc.c  */
#line 5184 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 378:

/* Line 1806 of yacc.c  */
#line 5188 "./sparql_parser.y"
    {
  (yyval.literal) = (yyvsp[(1) - (1)].literal);
}
    break;

  case 379:

/* Line 1806 of yacc.c  */
#line 5200 "./sparql_parser.y"
    {
  (yyval.literal) = rasqal_new_uri_literal(((rasqal_query*)rq)->world, (yyvsp[(1) - (1)].uri));
  if(!(yyval.literal))
    YYERROR_MSG("IRIref 1: cannot create literal");
}
    break;

  case 380:

/* Line 1806 of yacc.c  */
#line 5206 "./sparql_parser.y"
    {
  (yyval.literal) = rasqal_new_simple_literal(((rasqal_query*)rq)->world,
                                 RASQAL_LITERAL_QNAME, (yyvsp[(1) - (1)].name));
  if(!(yyval.literal))
    YYERROR_MSG("IRIref 2: cannot create literal");
  if(rasqal_literal_expand_qname((rasqal_query*)rq, (yyval.literal))) {
    sparql_query_error_full((rasqal_query*)rq,
                            "QName %s cannot be expanded", (yyvsp[(1) - (1)].name));
    rasqal_free_literal((yyval.literal));
    (yyval.literal) = NULL;
    YYERROR_MSG("IRIrefBrace 2: cannot expand qname");
  }
}
    break;

  case 381:

/* Line 1806 of yacc.c  */
#line 5226 "./sparql_parser.y"
    {
  (yyval.literal) = rasqal_new_simple_literal(((rasqal_query*)rq)->world,
                                 RASQAL_LITERAL_BLANK, (yyvsp[(1) - (1)].name));
  if(!(yyval.literal))
    YYERROR_MSG("BlankNode 1: cannot create literal");
}
    break;

  case 382:

/* Line 1806 of yacc.c  */
#line 5232 "./sparql_parser.y"
    {
  const unsigned char *id;
  id = rasqal_query_generate_bnodeid((rasqal_query*)rq, NULL);
  if(!id)
    YYERROR_MSG("BlankNode 2: cannot create bnodeid");
  (yyval.literal) = rasqal_new_simple_literal(((rasqal_query*)rq)->world,
                                 RASQAL_LITERAL_BLANK, id);
  if(!(yyval.literal))
    YYERROR_MSG("BlankNode 2: cannot create literal");
}
    break;



/* Line 1806 of yacc.c  */
#line 10359 "sparql_parser.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 2067 of yacc.c  */
#line 5251 "./sparql_parser.y"



/* Support functions */


/* This is declared in sparql_lexer.h but never used, so we always get
 * a warning unless this dummy code is here.  Used once below in an error case.
 */
static int yy_init_globals (yyscan_t yyscanner ) { return 0; }


/**
 * rasqal_sparql_query_language_init - Initialise the SPARQL query language parser
 *
 * Return value: non 0 on failure
 **/
static int
rasqal_sparql_query_language_init(rasqal_query* rdf_query, const char *name)
{
  rasqal_sparql_query_language* rqe;

  rqe = (rasqal_sparql_query_language*)rdf_query->context;

  rdf_query->compare_flags = RASQAL_COMPARE_XQUERY;

  /* All the sparql query families support this */
  rqe->sparql10 = 1;
  rqe->sparql_query = 1;
  /* SPARQL 1.1 is the default */
  rqe->sparql11_query = 1;
  rqe->sparql11_aggregates = 1;
  rqe->sparql11_property_paths = 1;
  rqe->sparql11_update = 1;

  /* SPARQL 1.0 disables SPARQL 1.1 features */
  if(!strncmp(name, "sparql10", 8)) {
    rqe->sparql11_query = 0;
    rqe->sparql11_aggregates = 0;
    rqe->sparql11_property_paths = 0;
    rqe->sparql11_update = 0;
  }

  if(!strcmp(name, "sparql11-query")) {
    /* No update if SPARQL 1.1 query */
    rqe->sparql11_update = 0;
  }
  
  if(!strcmp(name, "sparql11-update")) {
    /* No query if SPARQL 1.1 update */
    rqe->sparql_query = 0;
    rqe->sparql11_query = 0;
  }
  
  /* LAQRS for experiments */
  if(!strcmp(name, "laqrs"))
    rqe->experimental = 1;

  return 0;
}


/**
 * rasqal_sparql_query_language_terminate - Free the SPARQL query language parser
 *
 * Return value: non 0 on failure
 **/
static void
rasqal_sparql_query_language_terminate(rasqal_query* rdf_query)
{
  rasqal_sparql_query_language* sparql;
  sparql = (rasqal_sparql_query_language*)rdf_query->context;

  if(sparql && sparql->scanner_set) {
    sparql_lexer_lex_destroy(sparql->scanner);
    sparql->scanner_set = 0;
  }

}


static int
rasqal_sparql_query_language_prepare(rasqal_query* rdf_query)
{
  /* rasqal_sparql_query_language* sparql = (rasqal_sparql_query_language*)rdf_query->context; */
  int rc;
  
  if(!rdf_query->query_string)
    return 1;

  rc = rasqal_query_reset_select_query(rdf_query);
  if(rc)
    return 1;

  rc = sparql_parse(rdf_query);
  if(rc)
    return rc;

  /* FIXME - should check remaining query parts  */
  if(rasqal_sequence_has_qname(rdf_query->triples) ||
     rasqal_sequence_has_qname(rdf_query->constructs) ||
     rasqal_query_constraints_has_qname(rdf_query)) {
    sparql_query_error(rdf_query, "SPARQL query has unexpanded QNames");
    return 1;
  }

  /* SPARQL: Turn [] into anonymous variables */
  if(rasqal_query_build_anonymous_variables(rdf_query))
    return 1;
  
  /* SPARQL: Expand 'SELECT *' */
  if(rasqal_query_expand_wildcards(rdf_query,
                                   rasqal_query_get_projection(rdf_query)))
    return 1;
  
  return 0;
}


static int
sparql_parse(rasqal_query* rq)
{
  rasqal_sparql_query_language* rqe;
  raptor_locator *locator=&rq->locator;

  rqe = (rasqal_sparql_query_language*)rq->context;

  if(!rq->query_string)
    return yy_init_globals(NULL); /* 0 but a way to use yy_init_globals */

  locator->line = 1;
  locator->column = -1; /* No column info */
  locator->byte = -1; /* No bytes info */

#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 2
  sparql_parser_debug = 1;
#endif

  rqe->lineno = 1;

  if(sparql_lexer_lex_init(&rqe->scanner))
    return 1;
  rqe->scanner_set = 1;

  sparql_lexer_set_extra(((rasqal_query*)rq), rqe->scanner);

  (void)sparql_lexer__scan_buffer(RASQAL_GOOD_CAST(char*, rq->query_string),
                                  rq->query_string_length, rqe->scanner);

  rqe->error_count = 0;

  sparql_parser_parse(rq);

  sparql_lexer_lex_destroy(rqe->scanner);
  rqe->scanner_set = 0;

  /* Parsing failed */
  if(rq->failed)
    return 1;
  
  return 0;
}


static void
sparql_query_error(rasqal_query *rq, const char *msg)
{
  rasqal_sparql_query_language* rqe;

  rqe = (rasqal_sparql_query_language*)rq->context;

  if(rqe->error_count++)
    return;

  rq->locator.line = rqe->lineno;
#ifdef RASQAL_SPARQL_USE_ERROR_COLUMNS
  /*  rq->locator.column = sparql_lexer_get_column(yyscanner);*/
#endif

  rq->failed = 1;
  rasqal_log_error_simple(((rasqal_query*)rq)->world, RAPTOR_LOG_LEVEL_ERROR,
                          &rq->locator, "%s", msg);
}


static void
sparql_query_error_full(rasqal_query *rq, const char *message, ...)
{
  va_list arguments;
  rasqal_sparql_query_language* rqe;

  rqe = (rasqal_sparql_query_language*)rq->context;

  if(rqe->error_count++)
    return;

  rq->locator.line = rqe->lineno;
#ifdef RASQAL_SPARQL_USE_ERROR_COLUMNS
  /*  rq->locator.column = sparql_lexer_get_column(yyscanner);*/
#endif

  va_start(arguments, message);

  rq->failed = 1;
  rasqal_log_error_varargs(((rasqal_query*)rq)->world, RAPTOR_LOG_LEVEL_ERROR,
                           &rq->locator, message, arguments);

  va_end(arguments);
}


int
sparql_syntax_error(rasqal_query *rq, const char *message, ...)
{
  rasqal_sparql_query_language *rqe;
  va_list arguments;

  rqe = (rasqal_sparql_query_language*)rq->context;

  if(rqe->error_count++)
    return 0;

  rq->locator.line=rqe->lineno;
#ifdef RASQAL_SPARQL_USE_ERROR_COLUMNS
  /*  rp->locator.column=sparql_lexer_get_column(yyscanner);*/
#endif

  va_start(arguments, message);
  rq->failed = 1;
  rasqal_log_error_varargs(((rasqal_query*)rq)->world, RAPTOR_LOG_LEVEL_ERROR,
                           &rq->locator, message, arguments);
  va_end(arguments);

  return 0;
}


int
sparql_syntax_warning(rasqal_query *rq, const char *message, ...)
{
  rasqal_sparql_query_language *rqe;
  va_list arguments;

  if(RASQAL_WARNING_LEVEL_QUERY_SYNTAX < rq->world->warning_level)
    return 0;
  
  rqe = (rasqal_sparql_query_language*)rq->context;

  rq->locator.line = rqe->lineno;
#ifdef RASQAL_SPARQL_USE_ERROR_COLUMNS
  /*  rq->locator.column=sparql_lexer_get_column(yyscanner);*/
#endif

  va_start(arguments, message);
  rasqal_log_error_varargs(((rasqal_query*)rq)->world, RAPTOR_LOG_LEVEL_WARN,
                           &rq->locator, message, arguments);
  va_end(arguments);

  return 0;
}


static int
rasqal_sparql_query_language_iostream_write_escaped_counted_string(rasqal_query* query,
                                                                   raptor_iostream* iostr,
                                                                   const unsigned char* string,
                                                                   size_t len)
{
  const char delim = '"';
  
  raptor_iostream_write_byte(delim, iostr);
  if(raptor_string_ntriples_write(string, len, delim, iostr))
    return 1;
  
  raptor_iostream_write_byte(delim, iostr);

  return 0;
}


static const char* const sparql_names[] = { "sparql10", NULL};

static const raptor_type_q sparql_types[] = {
  { NULL, 0, 0}
};


static int
rasqal_sparql_query_language_register_factory(rasqal_query_language_factory *factory)
{
  int rc = 0;

  factory->desc.names = sparql_names;

  factory->desc.mime_types = sparql_types;

  factory->desc.label = "SPARQL 1.0 W3C RDF Query Language";

  factory->desc.uri_strings = NULL;

  factory->context_length = sizeof(rasqal_sparql_query_language);

  factory->init      = rasqal_sparql_query_language_init;
  factory->terminate = rasqal_sparql_query_language_terminate;
  factory->prepare   = rasqal_sparql_query_language_prepare;
  factory->iostream_write_escaped_counted_string = rasqal_sparql_query_language_iostream_write_escaped_counted_string;

  return rc;
}


int
rasqal_init_query_language_sparql(rasqal_world* world)
{
  return !rasqal_query_language_register_factory(world,
                                                 &rasqal_sparql_query_language_register_factory);
}


static const char* const sparql11_names[] = { "sparql", "sparql11", NULL };


static const char* const sparql11_uri_strings[] = {
  "http://www.w3.org/TR/rdf-sparql-query/",
  NULL
};

static const raptor_type_q sparql11_types[] = {
  { "application/sparql", 18, 10}, 
  { NULL, 0, 0}
};


static int
rasqal_sparql11_language_register_factory(rasqal_query_language_factory *factory)
{
  int rc = 0;

  factory->desc.names = sparql11_names;

  factory->desc.mime_types = sparql11_types;

  factory->desc.label = "SPARQL 1.1 (DRAFT) Query and Update Languages";

  /* What URI describes Query and Update languages? */
  factory->desc.uri_strings = sparql11_uri_strings;

  factory->context_length = sizeof(rasqal_sparql_query_language);

  factory->init      = rasqal_sparql_query_language_init;
  factory->terminate = rasqal_sparql_query_language_terminate;
  factory->prepare   = rasqal_sparql_query_language_prepare;
  factory->iostream_write_escaped_counted_string = rasqal_sparql_query_language_iostream_write_escaped_counted_string;

  return rc;
}


static const char* const sparql11_query_names[] = { "sparql11-query", NULL };

static const char* const sparql11_query_uri_strings[] = {
  "http://www.w3.org/TR/2010/WD-sparql11-query-20101014/",
  NULL
};

static const raptor_type_q sparql11_query_types[] = {
  { NULL, 0, 0}
};


static int
rasqal_sparql11_query_language_register_factory(rasqal_query_language_factory *factory)
{
  int rc = 0;

  factory->desc.names = sparql11_query_names;

  factory->desc.mime_types = sparql11_query_types;

  factory->desc.label = "SPARQL 1.1 (DRAFT) Query Language";

  factory->desc.uri_strings = sparql11_query_uri_strings;

  factory->context_length = sizeof(rasqal_sparql_query_language);

  factory->init      = rasqal_sparql_query_language_init;
  factory->terminate = rasqal_sparql_query_language_terminate;
  factory->prepare   = rasqal_sparql_query_language_prepare;
  factory->iostream_write_escaped_counted_string = rasqal_sparql_query_language_iostream_write_escaped_counted_string;

  return rc;
}


static const char* const sparql11_update_names[] = { "sparql11-update", NULL };

static const char* const sparql11_update_uri_strings[] = {
  "http://www.w3.org/TR/2010/WD-sparql11-update-20101014/",
  NULL
};

static const raptor_type_q sparql11_update_types[] = {
  { NULL, 0, 0}
};


static int
rasqal_sparql11_update_language_register_factory(rasqal_query_language_factory *factory)
{
  int rc = 0;

  factory->desc.names = sparql11_update_names;

  factory->desc.mime_types = sparql11_update_types;

  factory->desc.label = "SPARQL 1.1 (DRAFT) Update Language";

  factory->desc.uri_strings = sparql11_update_uri_strings;

  factory->context_length = sizeof(rasqal_sparql_query_language);

  factory->init      = rasqal_sparql_query_language_init;
  factory->terminate = rasqal_sparql_query_language_terminate;
  factory->prepare   = rasqal_sparql_query_language_prepare;
  factory->iostream_write_escaped_counted_string = rasqal_sparql_query_language_iostream_write_escaped_counted_string;

  return rc;
}


int
rasqal_init_query_language_sparql11(rasqal_world* world)
{
  if(!rasqal_query_language_register_factory(world,
                                             &rasqal_sparql11_language_register_factory))
    return 1;
  
  if(!rasqal_query_language_register_factory(world,
                                             &rasqal_sparql11_query_language_register_factory))
    return 1;
  
  if(!rasqal_query_language_register_factory(world,
                                             &rasqal_sparql11_update_language_register_factory))
    return 1;
  
  return 0;
}


static const char* const laqrs_names[] = { "laqrs", NULL};

static const raptor_type_q laqrs_types[] = {
  { NULL, 0, 0}
};


static int
rasqal_laqrs_query_language_register_factory(rasqal_query_language_factory *factory)
{
  int rc = 0;

  factory->desc.names = laqrs_names;

  factory->desc.mime_types = laqrs_types;

  factory->desc.label = "LAQRS adds to Querying RDF in SPARQL";

  factory->desc.uri_strings = NULL;

  factory->context_length = sizeof(rasqal_sparql_query_language);

  factory->init      = rasqal_sparql_query_language_init;
  factory->terminate = rasqal_sparql_query_language_terminate;
  factory->prepare   = rasqal_sparql_query_language_prepare;
  factory->iostream_write_escaped_counted_string = rasqal_sparql_query_language_iostream_write_escaped_counted_string;

  return rc;
}


int
rasqal_init_query_language_laqrs(rasqal_world* world)
{
  return !rasqal_query_language_register_factory(world,
                                                 &rasqal_laqrs_query_language_register_factory);
}


#ifdef STANDALONE
#include <stdio.h>
#include <locale.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifndef HAVE_GETOPT
#include <rasqal_getopt.h>
#endif

#ifdef NEED_OPTIND_DECLARATION
extern int optind;
extern char *optarg;
#endif

#define GETOPT_STRING "di:"

#define SPARQL_FILE_BUF_SIZE 4096

static char query_string[SPARQL_FILE_BUF_SIZE];

int
main(int argc, char *argv[]) 
{
  const char *program = rasqal_basename(argv[0]);
  rasqal_query *query;
  FILE *fh;
  int rc;
  const char *filename = NULL;
  raptor_uri* base_uri = NULL;
  unsigned char *uri_string = NULL;
  const char* query_language = "sparql";
  int usage = 0;
  rasqal_world *world;
  size_t read_len;

  world = rasqal_new_world();
  if(!world || rasqal_world_open(world))
    exit(1);

  filename = getenv("SPARQL_QUERY_FILE");
    
  while(!usage) {
    int c = getopt (argc, argv, GETOPT_STRING);

    if (c == -1)
      break;

    switch (c) {
      case 0:
      case '?': /* getopt() - unknown option */
        usage = 1;
        break;
        
      case 'd':
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 2
        sparql_parser_debug = 1;
#endif
        break;
  
      case 'i':
        if(optarg) {
          if(rasqal_language_name_check(world, optarg)) {
            query_language = optarg;
          } else {
            fprintf(stderr, "%s: Unknown query language '%s'\n",
                    program, optarg);
            usage = 1;
          }
        }
        break;
    }
  }

  if(!filename) {
    if((argc-optind) != 1) {
      fprintf(stderr, "%s: Too many arguments.\n", program);
      usage = 1;
    } else
      filename = argv[optind];
  }
  
  if(usage) {
    fprintf(stderr, "SPARQL/LAQRS parser test for Rasqal %s\n", 
            rasqal_version_string);
    fprintf(stderr, "USAGE: %s [OPTIONS] SPARQL-QUERY-FILE\n", program);
    fprintf(stderr, "OPTIONS:\n");
#if defined(RASQAL_DEBUG) && RASQAL_DEBUG > 2
    fprintf(stderr, " -d           Bison parser debugging\n");
#endif
    fprintf(stderr, " -i LANGUAGE  Set query language\n");
    rc = 1;
    goto tidy;
  }


 fh = fopen(filename, "r");
 if(!fh) {
   fprintf(stderr, "%s: Cannot open file %s - %s\n", program, filename,
           strerror(errno));
   rc = 1;
   goto tidy;
 }
 
  memset(query_string, 0, SPARQL_FILE_BUF_SIZE);
  read_len = fread(query_string, SPARQL_FILE_BUF_SIZE, 1, fh);
  if(read_len < SPARQL_FILE_BUF_SIZE) {
    if(ferror(fh)) {
      fprintf(stderr, "%s: file '%s' read failed - %s\n",
              program, filename, strerror(errno));
      fclose(fh);
      rc = 1;
      goto tidy;
    }
  }
  
  fclose(fh);

  query = rasqal_new_query(world, query_language, NULL);
  rc = 1;
  if(query) {
    uri_string = raptor_uri_filename_to_uri_string(filename);

    if(uri_string) {
      base_uri = raptor_new_uri(world->raptor_world_ptr, uri_string);

      if(base_uri) {
        rc = rasqal_query_prepare(query,
                                  RASQAL_GOOD_CAST(const unsigned char*, query_string),
                                  base_uri);

        if(!rc)
          rasqal_query_print(query, stdout);
      }
    }
  }

  tidy:
  if(query)
    rasqal_free_query(query);

  if(base_uri)
    raptor_free_uri(base_uri);

  if(uri_string)
    raptor_free_memory(uri_string);

  if(world)
    rasqal_free_world(world);

  return rc;
}
#endif

