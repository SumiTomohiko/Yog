/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     AMPER = 258,
     AS = 259,
     BAR = 260,
     BREAK = 261,
     CLASS = 262,
     COMMA = 263,
     DEF = 264,
     DO = 265,
     DOT = 266,
     DOUBLE_STAR = 267,
     ELIF = 268,
     ELSE = 269,
     END = 270,
     EQUAL = 271,
     EXCEPT = 272,
     FINALLY = 273,
     GREATER = 274,
     IF = 275,
     LBRACE = 276,
     LBRACKET = 277,
     LESS = 278,
     LPAR = 279,
     NAME = 280,
     NEWLINE = 281,
     NEXT = 282,
     NUMBER = 283,
     PLUS = 284,
     RBRACE = 285,
     RBRACKET = 286,
     RPAR = 287,
     STAR = 288,
     TRY = 289,
     WHILE = 290
   };
#endif
/* Tokens.  */
#define AMPER 258
#define AS 259
#define BAR 260
#define BREAK 261
#define CLASS 262
#define COMMA 263
#define DEF 264
#define DO 265
#define DOT 266
#define DOUBLE_STAR 267
#define ELIF 268
#define ELSE 269
#define END 270
#define EQUAL 271
#define EXCEPT 272
#define FINALLY 273
#define GREATER 274
#define IF 275
#define LBRACE 276
#define LBRACKET 277
#define LESS 278
#define LPAR 279
#define NAME 280
#define NEWLINE 281
#define NEXT 282
#define NUMBER 283
#define PLUS 284
#define RBRACE 285
#define RBRACKET 286
#define RPAR 287
#define STAR 288
#define TRY 289
#define WHILE 290




/* Copy the first part of user declarations.  */
#line 1 "parser.y"

#include <stdio.h>
#include "yog/yog.h"

#define YYPARSE_PARAM   parser
#define PARSER          ((YogParser*)YYPARSE_PARAM)
#define YYLEX_PARAM     (PARSER)->lexer
#define ENV             (PARSER)->env
#define VM              (PARSER)->vm

static void 
yyerror(char* s)
{
    fprintf(stderr, "%s\n", s);
}

static void 
gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
#define GC(f)   DO_GC(env, do_gc, f(node))
    YogNode* node = ptr;
    switch (node->type) {
        case NODE_ASSIGN:
            NODE_RIGHT(node) = do_gc(env, NODE_RIGHT(node));
            break;
        case NODE_VARIABLE:
            break;
        case NODE_LITERAL:
            break;
        case NODE_METHOD_CALL:
            NODE_RECEIVER(node) = do_gc(env, NODE_RECEIVER(node));
            NODE_ARGS(node) = do_gc(env, NODE_ARGS(node));
            break;
        case NODE_COMMAND_CALL:
            NODE_ARGS(node) = do_gc(env, NODE_ARGS(node));
            break;
        case NODE_FUNC_CALL:
            NODE_CALLEE(node) = do_gc(env, NODE_CALLEE(node));
            NODE_ARGS(node) = do_gc(env, NODE_ARGS(node));
            break;
        case NODE_FUNC_DEF:
            NODE_PARAMS(node) = do_gc(env, NODE_PARAMS(node));
            NODE_STMTS(node) = do_gc(env, NODE_STMTS(node));
            break;
        case NODE_FINALLY:
            GC(NODE_HEAD);
            GC(NODE_BODY);
            break;
        case NODE_EXCEPT:
            GC(NODE_HEAD);
            GC(NODE_EXCEPTS);
            GC(NODE_ELSE);
            break;
        case NODE_EXCEPT_BODY:
            GC(NODE_EXC_TYPE);
            GC(NODE_BODY);
            break;
        case NODE_WHILE:
            NODE_TEST(node) = do_gc(env, NODE_TEST(node));
            NODE_STMTS(node) = do_gc(env, NODE_STMTS(node));
            break;
        case NODE_BREAK:
        case NODE_NEXT:
            NODE_EXPR(node) = do_gc(env, NODE_EXPR(node));
            break;
        default:
            Yog_assert(env, FALSE, "Unknown node type.");
            break;
    }
#undef GC
}

static YogNode* 
YogNode_new(YogEnv* env, YogNodeType type) 
{
    YogNode* node = ALLOC_OBJ(env, gc_children, YogNode);
#if 0
    node->lineno = Yog_get_lineno();
#endif
    node->lineno = 0;
    node->type = type;

    return node;
}

#define NODE_NEW(type)  YogNode_new(ENV, type)

#define BLOCK_ARG_NEW(node, params, stmts) do { \
    node = NODE_NEW(NODE_BLOCK_ARG); \
    NODE_PARAMS(node) = params; \
    NODE_STMTS(node) = stmts; \
} while (0)

#define PARAMS_NEW(array, params_without_default, params_with_default, block_param, var_param, kw_param) do { \
    array = YogArray_new(ENV); \
    if (params_without_default != NULL) { \
        YogArray_extend(ENV, array, params_without_default); \
    } \
    if (params_with_default != NULL) { \
        YogArray_extend(ENV, array, params_with_default); \
    } \
    if (block_param != NULL) { \
        YogVal val = YogVal_ptr(block_param); \
        YogArray_push(ENV, array, val); \
    } \
    if (var_param != NULL) { \
        YogVal val = YogVal_ptr(var_param); \
        YogArray_push(ENV, array, val); \
    } \
    if (kw_param != NULL) { \
        YogVal val = YogVal_ptr(kw_param); \
        YogArray_push(ENV, array, val); \
    } \
} while (0)

#define COMMAND_CALL_NEW(node, name, args, blockarg) do { \
    node = NODE_NEW(NODE_COMMAND_CALL); \
    NODE_COMMAND(node) = name; \
    NODE_ARGS(node) = args; \
    NODE_BLOCK(node) = blockarg; \
} while (0)

#define OBJ_ARRAY_NEW(array, elem) do { \
    if (elem != NULL) { \
        array = YogArray_new(ENV); \
        YogArray_push(ENV, array, YogVal_ptr(elem)); \
    } \
    else { \
        array = NULL; \
    } \
} while (0)

#define OBJ_ARRAY_PUSH(result, array, elem) do { \
    if (elem != NULL) { \
        if (array == NULL) { \
            array = YogArray_new(ENV); \
        } \
        YogArray_push(ENV, array, YogVal_ptr(elem)); \
    } \
    result = array; \
} while (0)

#define PARAM_NEW(node, type, id, default_) do { \
    node = NODE_NEW(type); \
    NODE_NAME(node) = id; \
    NODE_DEFAULT(node) = default_; \
} while (0)

#define PARAM_ARRAY_PUSH(array, id, default_) do { \
    YogNode* node = NULL; \
    PARAM_NEW(node, NODE_PARAM, id, default_); \
    YogVal val = YogVal_ptr(node); \
    YogArray_push(ENV, array, val); \
} while (0)

#define FUNC_DEF_NEW(node, name, params, stmts) do { \
    node = NODE_NEW(NODE_FUNC_DEF); \
    NODE_NAME(node) = name; \
    NODE_PARAMS(node) = params; \
    NODE_STMTS(node) = stmts; \
} while (0)

#define FUNC_CALL_NEW(node, callee, args, blockarg) do { \
    node = NODE_NEW(NODE_FUNC_CALL); \
    NODE_CALLEE(node) = callee; \
    NODE_ARGS(node) = args; \
    NODE_BLOCK(node) = blockarg; \
} while (0)

#define VARIABLE_NEW(node, id) do { \
    node = NODE_NEW(NODE_VARIABLE); \
    NODE_ID(node) = id; \
} while (0)

#define EXCEPT_BODY_NEW(node, type, var, stmts) do { \
    node = NODE_NEW(NODE_EXCEPT_BODY); \
    NODE_EXC_TYPE(node) = type; \
    NODE_EXC_VAR(node) = var; \
    NODE_BODY(node) = stmts; \
} while (0)

#define EXCEPT_NEW(node, head, excepts, else_) do { \
    node = NODE_NEW(NODE_EXCEPT); \
    NODE_HEAD(node) = head; \
    NODE_EXCEPTS(node) = excepts; \
    NODE_ELSE(node) = else_; \
} while (0)

#define FINALLY_NEW(node, head, body) do { \
    node = NODE_NEW(NODE_FINALLY); \
    NODE_HEAD(node) = head; \
    NODE_BODY(node) = body; \
} while (0)

#define EXCEPT_FINALLY_NEW(node, stmts, excepts, else_, finally) do { \
    EXCEPT_NEW(node, stmts, excepts, else_); \
    if (finally != NULL) { \
        YogArray* array = NULL; \
        OBJ_ARRAY_NEW(array, node); \
        FINALLY_NEW(node, array, finally); \
    } \
} while (0)

#define BREAK_NEW(node, expr) do { \
    node = NODE_NEW(NODE_BREAK); \
    NODE_EXPR(node) = expr; \
} while (0)

#define NEXT_NEW(node, expr) do { \
    node = NODE_NEW(NODE_NEXT); \
    NODE_EXPR(node) = expr; \
} while (0)

#define METHOD_CALL_NEW(node, recv, name, args, blockarg) do { \
    node = NODE_NEW(NODE_METHOD_CALL); \
    NODE_RECEIVER(node) = recv; \
    NODE_METHOD(node) = name; \
    NODE_ARGS(node) = args; \
    NODE_BLOCK(node) = blockarg; \
} while (0)

#define METHOD_CALL_NEW1(node, recv, name, arg) do { \
    YogArray* args = YogArray_new(ENV); \
    YogArray_push(ENV, args, YogVal_ptr(arg)); \
    METHOD_CALL_NEW(node, recv, name, args, NULL); \
} while (0)

#define IF_NEW(node, expr, stmts, tail) do { \
    node = NODE_NEW(NODE_IF); \
    NODE_IF_TEST(node) = expr; \
    NODE_IF_STMTS(node) = stmts; \
    NODE_IF_TAIL(node) = tail; \
} while (0)


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

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 236 "parser.y"
typedef union YYSTYPE {
    struct YogArray* array;
    struct YogNode* node;
    struct YogVal val;
    ID name;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 397 "parser.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 409 "parser.c"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
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
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  40
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   169

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  39
/* YYNRULES -- Number of rules. */
#define YYNRULES  103
/* YYNRULES -- Number of states. */
#define YYNSTATES  173

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
      35
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     7,    11,    12,    14,    16,    19,
      27,    33,    39,    44,    46,    49,    51,    54,    60,    66,
      67,    70,    72,    77,    78,    81,    89,    99,   107,   115,
     121,   129,   135,   141,   145,   153,   159,   165,   169,   175,
     179,   183,   185,   193,   199,   205,   209,   215,   219,   223,
     225,   231,   235,   239,   241,   245,   247,   249,   250,   253,
     256,   260,   261,   263,   266,   268,   272,   274,   278,   281,
     283,   287,   289,   293,   295,   297,   299,   301,   303,   307,
     309,   311,   313,   315,   317,   319,   323,   325,   327,   329,
     331,   333,   339,   343,   344,   346,   347,   354,   356,   359,
     366,   371,   375,   376
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      37,     0,    -1,    38,    -1,    39,    -1,    38,    26,    39,
      -1,    -1,    43,    -1,    54,    -1,    25,    53,    -1,    34,
      38,    72,    14,    38,    74,    15,    -1,    34,    38,    72,
      74,    15,    -1,    34,    38,    18,    38,    15,    -1,    35,
      54,    38,    15,    -1,     6,    -1,     6,    54,    -1,    27,
      -1,    27,    54,    -1,    20,    54,    38,    41,    15,    -1,
       7,    25,    40,    38,    15,    -1,    -1,    19,    54,    -1,
      42,    -1,    13,    54,    38,    41,    -1,    -1,    14,    38,
      -1,     9,    25,    24,    44,    32,    38,    15,    -1,    50,
       8,    51,     8,    47,     8,    46,     8,    45,    -1,    50,
       8,    51,     8,    47,     8,    46,    -1,    50,     8,    51,
       8,    47,     8,    45,    -1,    50,     8,    51,     8,    47,
      -1,    50,     8,    51,     8,    46,     8,    45,    -1,    50,
       8,    51,     8,    46,    -1,    50,     8,    51,     8,    45,
      -1,    50,     8,    51,    -1,    50,     8,    47,     8,    46,
       8,    45,    -1,    50,     8,    47,     8,    46,    -1,    50,
       8,    47,     8,    45,    -1,    50,     8,    47,    -1,    50,
       8,    46,     8,    45,    -1,    50,     8,    46,    -1,    50,
       8,    45,    -1,    50,    -1,    51,     8,    47,     8,    46,
       8,    45,    -1,    51,     8,    47,     8,    46,    -1,    51,
       8,    47,     8,    45,    -1,    51,     8,    47,    -1,    51,
       8,    46,     8,    45,    -1,    51,     8,    46,    -1,    51,
       8,    45,    -1,    51,    -1,    47,     8,    46,     8,    45,
      -1,    47,     8,    46,    -1,    47,     8,    45,    -1,    47,
      -1,    46,     8,    45,    -1,    46,    -1,    45,    -1,    -1,
      12,    25,    -1,    33,    25,    -1,     3,    25,    48,    -1,
      -1,    49,    -1,    16,    54,    -1,    25,    -1,    50,     8,
      25,    -1,    52,    -1,    51,     8,    52,    -1,    25,    49,
      -1,    54,    -1,    53,     8,    54,    -1,    55,    -1,    25,
      16,    56,    -1,    56,    -1,    57,    -1,    58,    -1,    59,
      -1,    61,    -1,    61,    60,    61,    -1,    23,    -1,    62,
      -1,    63,    -1,    64,    -1,    65,    -1,    66,    -1,    65,
      29,    66,    -1,    67,    -1,    68,    -1,    69,    -1,    25,
      -1,    28,    -1,    69,    24,    70,    32,    71,    -1,    69,
      11,    25,    -1,    -1,    53,    -1,    -1,    10,    22,    44,
      31,    38,    15,    -1,    73,    -1,    72,    73,    -1,    17,
      54,     4,    25,    26,    38,    -1,    17,    54,    26,    38,
      -1,    17,    26,    38,    -1,    -1,    18,    38,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   320,   320,   324,   327,   331,   334,   335,   336,   346,
     349,   352,   355,   361,   364,   367,   370,   373,   376,   384,
     387,   391,   392,   398,   401,   405,   409,   412,   415,   418,
     421,   424,   427,   430,   433,   436,   439,   442,   445,   448,
     451,   454,   457,   460,   463,   466,   469,   472,   475,   478,
     481,   484,   487,   490,   493,   496,   499,   502,   506,   510,
     514,   518,   521,   523,   527,   531,   536,   539,   543,   547,
     550,   554,   556,   562,   564,   566,   568,   570,   571,   575,
     577,   579,   581,   583,   585,   586,   590,   592,   594,   596,
     599,   604,   612,   619,   622,   624,   627,   631,   634,   638,
     642,   645,   649,   652
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "AMPER", "AS", "BAR", "BREAK", "CLASS",
  "COMMA", "DEF", "DO", "DOT", "DOUBLE_STAR", "ELIF", "ELSE", "END",
  "EQUAL", "EXCEPT", "FINALLY", "GREATER", "IF", "LBRACE", "LBRACKET",
  "LESS", "LPAR", "NAME", "NEWLINE", "NEXT", "NUMBER", "PLUS", "RBRACE",
  "RBRACKET", "RPAR", "STAR", "TRY", "WHILE", "$accept", "module", "stmts",
  "stmt", "super_opt", "if_tail", "else_opt", "func_def", "params",
  "kw_param", "var_param", "block_param", "param_default_opt",
  "param_default", "params_without_default", "params_with_default",
  "param_with_default", "args", "expr", "assign_expr", "logical_or_expr",
  "logical_and_expr", "not_expr", "comparison", "comp_op", "xor_expr",
  "or_expr", "and_expr", "shift_expr", "arith_expr", "term", "factor",
  "power", "atom", "args_opt", "blockarg_opt", "excepts", "except",
  "finally_opt", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    36,    37,    38,    38,    39,    39,    39,    39,    39,
      39,    39,    39,    39,    39,    39,    39,    39,    39,    40,
      40,    41,    41,    42,    42,    43,    44,    44,    44,    44,
      44,    44,    44,    44,    44,    44,    44,    44,    44,    44,
      44,    44,    44,    44,    44,    44,    44,    44,    44,    44,
      44,    44,    44,    44,    44,    44,    44,    44,    45,    46,
      47,    48,    48,    49,    50,    50,    51,    51,    52,    53,
      53,    54,    55,    55,    56,    57,    58,    59,    59,    60,
      61,    62,    63,    64,    65,    65,    66,    67,    68,    69,
      69,    69,    69,    70,    70,    71,    71,    72,    72,    73,
      73,    73,    74,    74
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     3,     0,     1,     1,     2,     7,
       5,     5,     4,     1,     2,     1,     2,     5,     5,     0,
       2,     1,     4,     0,     2,     7,     9,     7,     7,     5,
       7,     5,     5,     3,     7,     5,     5,     3,     5,     3,
       3,     1,     7,     5,     5,     3,     5,     3,     3,     1,
       5,     3,     3,     1,     3,     1,     1,     0,     2,     2,
       3,     0,     1,     2,     1,     3,     1,     3,     2,     1,
       3,     1,     3,     1,     1,     1,     1,     1,     3,     1,
       1,     1,     1,     1,     1,     3,     1,     1,     1,     1,
       1,     5,     3,     0,     1,     0,     6,     1,     2,     6,
       4,     3,     0,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       5,    13,     0,     0,     0,    89,    15,    90,     5,     0,
       0,     2,     3,     6,     7,    71,    73,    74,    75,    76,
      77,    80,    81,    82,    83,    84,    86,    87,    88,    89,
      14,    19,     0,     5,     0,     8,    69,    16,     0,     5,
       1,     5,    79,     0,     0,     0,    93,     0,     5,    57,
      23,    89,    72,     0,     0,     5,   102,    97,     0,     4,
      78,    85,    92,    94,     0,    20,     0,     0,     0,    64,
       0,     0,    56,    55,    53,    41,    49,    66,     0,     5,
       0,    21,    70,     5,     0,     0,     5,     5,    98,     0,
      12,    95,    18,    61,    58,     0,    68,    59,     5,     0,
       0,     0,     0,     5,    24,    17,   101,     0,     5,    11,
     102,   103,    10,     0,    91,    60,    62,    63,     0,    54,
      52,    51,    65,    40,    39,    37,    33,     0,    48,    47,
      45,    67,    23,     0,   100,     0,    57,    25,     0,     0,
       0,     0,     0,     0,    22,     5,     9,     0,    50,    38,
      36,    35,    32,    31,    29,    46,    44,    43,    99,     5,
       0,     0,     0,     0,     0,    34,    30,    28,    27,    42,
      96,     0,    26
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    10,    11,    12,    48,    80,    81,    13,    71,    72,
      73,    74,   115,    96,    75,    76,    77,    35,    14,    15,
      16,    17,    18,    19,    43,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    64,   114,    56,    57,    89
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -101
static const short int yypact[] =
{
      87,    61,   -13,   -12,    61,    26,    61,  -101,    87,    61,
      14,    -7,  -101,  -101,  -101,  -101,  -101,  -101,  -101,  -101,
      29,  -101,  -101,  -101,    39,  -101,  -101,  -101,    -6,    37,
    -101,    53,    34,    87,    95,    69,  -101,  -101,   100,    87,
    -101,    87,  -101,    95,    95,    62,    61,    61,    87,    12,
       3,  -101,  -101,    61,     8,    87,    56,  -101,    24,  -101,
    -101,  -101,  -101,    69,    71,  -101,    76,    79,    86,    85,
      88,    77,  -101,   116,   117,   119,   120,  -101,    61,    87,
     114,  -101,  -101,    87,     6,    84,    87,    87,  -101,   115,
    -101,   121,  -101,    85,  -101,    61,  -101,  -101,    87,   122,
      -3,    32,    73,    87,    -7,  -101,    -7,   107,    87,  -101,
      20,    -7,  -101,   111,  -101,  -101,  -101,  -101,    90,  -101,
    -101,   127,    85,  -101,   128,   130,   131,    85,  -101,   132,
     133,  -101,     3,   118,    -7,   134,    12,  -101,   122,   122,
      -3,    73,   122,    -3,  -101,    87,  -101,   112,  -101,  -101,
    -101,   137,  -101,   138,   139,  -101,  -101,   140,    -7,    87,
     122,   122,    -3,   122,    93,  -101,  -101,  -101,   142,  -101,
    -101,   122,  -101
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
    -101,  -101,    -8,   101,  -101,    21,  -101,  -101,    16,   -79,
     -74,  -100,  -101,    63,  -101,    54,   -98,   108,     2,  -101,
     123,  -101,  -101,  -101,  -101,   124,  -101,  -101,  -101,  -101,
     125,  -101,  -101,  -101,  -101,  -101,  -101,   102,    49
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      38,   125,   130,    30,   131,    45,    33,    36,    37,    68,
     107,    39,    31,    32,    40,    67,    78,    79,    46,    41,
     119,   120,   123,   128,    68,    50,   121,   124,   129,    41,
      70,    58,   108,    29,    83,    67,     7,    69,    87,    90,
      66,   154,    34,   131,    68,    70,    41,    85,    36,    65,
      41,    29,    42,    34,     7,    82,    84,   122,    49,   148,
     149,   150,   152,   155,   156,    70,   151,   153,    44,   157,
      86,   104,    47,    54,    87,   106,    67,    53,   110,   111,
     103,   165,   166,   167,   169,    68,    29,    62,   168,     7,
     118,    92,   172,     1,     2,   132,     3,   117,   127,   109,
     134,    95,    41,    91,    93,   137,    70,     4,   170,    98,
      41,    94,     5,    97,     6,     7,    41,    54,    55,    41,
      51,     8,     9,     7,    99,   100,    41,   101,   102,   105,
     112,   113,   133,   136,    68,   138,   139,   158,   140,   141,
     142,   143,    59,   159,   145,   160,   161,   162,   163,   146,
     171,   164,   147,   144,    63,   126,   116,    52,    88,   135,
       0,     0,     0,     0,     0,     0,     0,    60,     0,    61
};

static const short int yycheck[] =
{
       8,   101,   102,     1,   102,    11,     4,     5,     6,    12,
       4,     9,    25,    25,     0,     3,    13,    14,    24,    26,
      99,   100,   101,   102,    12,    33,   100,   101,   102,    26,
      33,    39,    26,    25,    26,     3,    28,    25,    18,    15,
      48,   141,    16,   141,    12,    33,    26,    55,    46,    47,
      26,    25,    23,    16,    28,    53,    54,    25,    24,   138,
     139,   140,   141,   142,   143,    33,   140,   141,    29,   143,
      14,    79,    19,    17,    18,    83,     3,     8,    86,    87,
      78,   160,   161,   162,   163,    12,    25,    25,   162,    28,
      98,    15,   171,     6,     7,   103,     9,    95,    25,    15,
     108,    16,    26,    32,    25,    15,    33,    20,    15,    32,
      26,    25,    25,    25,    27,    28,    26,    17,    18,    26,
      25,    34,    35,    28,     8,     8,    26,     8,     8,    15,
      15,    10,    25,    22,    12,     8,     8,   145,     8,     8,
       8,     8,    41,    31,    26,     8,     8,     8,     8,    15,
       8,   159,   136,   132,    46,   101,    93,    34,    56,   110,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    43,    -1,    44
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     6,     7,     9,    20,    25,    27,    28,    34,    35,
      37,    38,    39,    43,    54,    55,    56,    57,    58,    59,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    25,
      54,    25,    25,    54,    16,    53,    54,    54,    38,    54,
       0,    26,    23,    60,    29,    11,    24,    19,    40,    24,
      38,    25,    56,     8,    17,    18,    72,    73,    38,    39,
      61,    66,    25,    53,    70,    54,    38,     3,    12,    25,
      33,    44,    45,    46,    47,    50,    51,    52,    13,    14,
      41,    42,    54,    26,    54,    38,    14,    18,    73,    74,
      15,    32,    15,    25,    25,    16,    49,    25,    32,     8,
       8,     8,     8,    54,    38,    15,    38,     4,    26,    15,
      38,    38,    15,    10,    71,    48,    49,    54,    38,    45,
      45,    46,    25,    45,    46,    47,    51,    25,    45,    46,
      47,    52,    38,    25,    38,    74,    22,    15,     8,     8,
       8,     8,     8,     8,    41,    26,    15,    44,    45,    45,
      45,    46,    45,    46,    47,    45,    45,    46,    38,    31,
       8,     8,     8,     8,    38,    45,    45,    45,    46,    45,
      15,     8,    45
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
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
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
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
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
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

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
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
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
      size_t yyn = 0;
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

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
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
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()
    ;
#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

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
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


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
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

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

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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
        case 2:
#line 320 "parser.y"
    {
            PARSER->stmts = (yyvsp[0].array);
        }
    break;

  case 3:
#line 324 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 4:
#line 327 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
        }
    break;

  case 5:
#line 331 "parser.y"
    {
            (yyval.node) = NULL;
        }
    break;

  case 8:
#line 336 "parser.y"
    {
            COMMAND_CALL_NEW((yyval.node), (yyvsp[-1].name), (yyvsp[0].array), NULL);
        }
    break;

  case 9:
#line 346 "parser.y"
    {
            EXCEPT_FINALLY_NEW((yyval.node), (yyvsp[-5].array), (yyvsp[-4].array), (yyvsp[-2].array), (yyvsp[-1].array));
        }
    break;

  case 10:
#line 349 "parser.y"
    {
            EXCEPT_FINALLY_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-2].array), NULL, (yyvsp[-1].array));
        }
    break;

  case 11:
#line 352 "parser.y"
    {
            FINALLY_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-1].array));
        }
    break;

  case 12:
#line 355 "parser.y"
    {
            YogNode* node = NODE_NEW(NODE_WHILE);
            NODE_TEST(node) = (yyvsp[-2].node);
            NODE_STMTS(node) = (yyvsp[-1].array);
            (yyval.node) = node;
        }
    break;

  case 13:
#line 361 "parser.y"
    {
            BREAK_NEW((yyval.node), NULL);
        }
    break;

  case 14:
#line 364 "parser.y"
    {
            BREAK_NEW((yyval.node), (yyvsp[0].node));
        }
    break;

  case 15:
#line 367 "parser.y"
    {
            NEXT_NEW((yyval.node), NULL);
        }
    break;

  case 16:
#line 370 "parser.y"
    {
            NEXT_NEW((yyval.node), (yyvsp[0].node));
        }
    break;

  case 17:
#line 373 "parser.y"
    {
            IF_NEW((yyval.node), (yyvsp[-3].node), (yyvsp[-2].array), (yyvsp[-1].array));
        }
    break;

  case 18:
#line 376 "parser.y"
    {
            YogNode* node = NODE_NEW(NODE_KLASS);
            NODE_NAME(node) = (yyvsp[-3].name);
            NODE_SUPER(node) = (yyvsp[-2].node);
            NODE_STMTS(node) = (yyvsp[-1].array);
            (yyval.node) = node;
        }
    break;

  case 19:
#line 384 "parser.y"
    {
                (yyval.node) = NULL;
            }
    break;

  case 20:
#line 387 "parser.y"
    {
                (yyval.node) = (yyvsp[0].node);
            }
    break;

  case 22:
#line 392 "parser.y"
    {
            YogNode* node = NULL;
            IF_NEW(node, (yyvsp[-2].node), (yyvsp[-1].array), (yyvsp[0].array));
            OBJ_ARRAY_NEW((yyval.array), node);
        }
    break;

  case 23:
#line 398 "parser.y"
    {
                (yyval.array) = NULL;
            }
    break;

  case 24:
#line 401 "parser.y"
    {
                (yyval.array) = (yyvsp[0].array);
            }
    break;

  case 25:
#line 405 "parser.y"
    {
                FUNC_DEF_NEW((yyval.node), (yyvsp[-5].name), (yyvsp[-3].array), (yyvsp[-1].array));
            }
    break;

  case 26:
#line 409 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-8].array), (yyvsp[-6].array), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 27:
#line 412 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 28:
#line 415 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 29:
#line 418 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 30:
#line 421 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 31:
#line 424 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 32:
#line 427 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 33:
#line 430 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), (yyvsp[0].array), NULL, NULL, NULL);
        }
    break;

  case 34:
#line 433 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), NULL, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 35:
#line 436 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NULL, (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 36:
#line 439 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NULL, (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 37:
#line 442 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NULL, (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 38:
#line 445 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 39:
#line 448 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NULL, NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 40:
#line 451 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NULL, NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 41:
#line 454 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[0].array), NULL, NULL, NULL, NULL);
        }
    break;

  case 42:
#line 457 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-6].array), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 43:
#line 460 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-4].array), (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 44:
#line 463 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-4].array), (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 45:
#line 466 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-2].array), (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 46:
#line 469 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-4].array), NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 47:
#line 472 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-2].array), NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 48:
#line 475 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-2].array), NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 49:
#line 478 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[0].array), NULL, NULL, NULL);
        }
    break;

  case 50:
#line 481 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 51:
#line 484 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 52:
#line 487 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 53:
#line 490 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 54:
#line 493 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 55:
#line 496 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 56:
#line 499 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 57:
#line 502 "parser.y"
    {
            (yyval.array) = NULL;
        }
    break;

  case 58:
#line 506 "parser.y"
    {
                PARAM_NEW((yyval.node), NODE_KW_PARAM, (yyvsp[0].name), NULL);
            }
    break;

  case 59:
#line 510 "parser.y"
    {
                PARAM_NEW((yyval.node), NODE_VAR_PARAM, (yyvsp[0].name), NULL);
            }
    break;

  case 60:
#line 514 "parser.y"
    {
                    PARAM_NEW((yyval.node), NODE_BLOCK_PARAM, (yyvsp[-1].name), (yyvsp[0].node));
                }
    break;

  case 61:
#line 518 "parser.y"
    {
                        (yyval.node) = NULL;
                    }
    break;

  case 63:
#line 523 "parser.y"
    {
                    (yyval.node) = (yyvsp[0].node);
                }
    break;

  case 64:
#line 527 "parser.y"
    {
                            (yyval.array) = YogArray_new(ENV);
                            PARAM_ARRAY_PUSH((yyval.array), (yyvsp[0].name), NULL);
                        }
    break;

  case 65:
#line 531 "parser.y"
    {
                            PARAM_ARRAY_PUSH((yyvsp[-2].array), (yyvsp[0].name), NULL);
                            (yyval.array) = (yyvsp[-2].array);
                        }
    break;

  case 66:
#line 536 "parser.y"
    {
                            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
                        }
    break;

  case 67:
#line 539 "parser.y"
    {
                            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
                        }
    break;

  case 68:
#line 543 "parser.y"
    {
                        PARAM_NEW((yyval.node), NODE_PARAM, (yyvsp[-1].name), (yyvsp[0].node));
                    }
    break;

  case 69:
#line 547 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 70:
#line 550 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
        }
    break;

  case 72:
#line 556 "parser.y"
    {
                YogNode* node = NODE_NEW(NODE_ASSIGN);
                NODE_LEFT(node) = (yyvsp[-2].name);
                NODE_RIGHT(node) = (yyvsp[0].node);
                (yyval.node) = node;
            }
    break;

  case 78:
#line 571 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 85:
#line 586 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 89:
#line 596 "parser.y"
    {
            VARIABLE_NEW((yyval.node), (yyvsp[0].name));
        }
    break;

  case 90:
#line 599 "parser.y"
    {
            YogNode* node = NODE_NEW(NODE_LITERAL);
            NODE_VAL(node) = (yyvsp[0].val);
            (yyval.node) = node;
        }
    break;

  case 91:
#line 604 "parser.y"
    {
            if ((yyvsp[-4].node)->type == NODE_ATTR) {
                METHOD_CALL_NEW((yyval.node), NODE_OBJ((yyvsp[-4].node)), NODE_NAME((yyvsp[-4].node)), (yyvsp[-2].array), (yyvsp[0].node));
            }
            else {
                FUNC_CALL_NEW((yyval.node), (yyvsp[-4].node), (yyvsp[-2].array), (yyvsp[0].node));
            }
        }
    break;

  case 92:
#line 612 "parser.y"
    {
            YogNode* node = NODE_NEW(NODE_ATTR);
            NODE_OBJ(node) = (yyvsp[-2].node);
            NODE_NAME(node) = (yyvsp[0].name);
            (yyval.node) = node;
        }
    break;

  case 93:
#line 619 "parser.y"
    {
                (yyval.array) = NULL;
            }
    break;

  case 95:
#line 624 "parser.y"
    {
                    (yyval.node) = NULL;
                }
    break;

  case 96:
#line 627 "parser.y"
    {
                    BLOCK_ARG_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-1].array));
                }
    break;

  case 97:
#line 631 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 98:
#line 634 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-1].array), (yyvsp[0].node));
        }
    break;

  case 99:
#line 638 "parser.y"
    {
            Yog_assert(ENV, (yyvsp[-2].name) != NO_EXC_VAR, "Too many variables.");
            EXCEPT_BODY_NEW((yyval.node), (yyvsp[-4].node), (yyvsp[-2].name), (yyvsp[0].array));
        }
    break;

  case 100:
#line 642 "parser.y"
    {
            EXCEPT_BODY_NEW((yyval.node), (yyvsp[-2].node), NO_EXC_VAR, (yyvsp[0].array));
        }
    break;

  case 101:
#line 645 "parser.y"
    {
            EXCEPT_BODY_NEW((yyval.node), NULL, NO_EXC_VAR, (yyvsp[0].array));
        }
    break;

  case 102:
#line 649 "parser.y"
    {
                (yyval.array) = NULL;
            }
    break;

  case 103:
#line 652 "parser.y"
    {
                (yyval.array) = (yyvsp[0].array);
            }
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2165 "parser.c"

  yyvsp -= yylen;
  yyssp -= yylen;


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
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
	  char *yyfmt;
	  char const *yyf;
	  static char const yyunexpected[] = "syntax error, unexpected %s";
	  static char const yyexpecting[] = ", expecting %s";
	  static char const yyor[] = " or %s";
	  char yyformat[sizeof yyunexpected
			+ sizeof yyexpecting - 1
			+ ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
			   * (sizeof yyor - 1))];
	  char const *yyprefix = yyexpecting;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 1;

	  yyarg[0] = yytname[yytype];
	  yyfmt = yystpcpy (yyformat, yyunexpected);

	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
		  {
		    yycount = 1;
		    yysize = yysize0;
		    yyformat[sizeof yyunexpected - 1] = '\0';
		    break;
		  }
		yyarg[yycount++] = yytname[yyx];
		yysize1 = yysize + yytnamerr (0, yytname[yyx]);
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
		{
		  if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		    {
		      yyp += yytnamerr (yyp, yyarg[yyi++]);
		      yyf += 2;
		    }
		  else
		    {
		      yyp++;
		      yyf++;
		    }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
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
      if (yyn != YYPACT_NINF)
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


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
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

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 656 "parser.y"

/*
single_input: NEWLINE | simple_stmt | compound_stmt NEWLINE
file_input: (NEWLINE | stmt)* ENDMARKER
eval_input: testlist NEWLINE* ENDMARKER

decorator: '@' dotted_name [ '(' [arglist] ')' ] NEWLINE
decorators: decorator+
decorated: decorators (classdef | funcdef)
funcdef: 'def' NAME parameters ['->' test] ':' suite
parameters: '(' [typedargslist] ')'
typedargslist: ((tfpdef ['=' test] ',')*
                ('*' [tfpdef] (',' tfpdef ['=' test])* [',' '**' tfpdef] | '**' tfpdef)
                | tfpdef ['=' test] (',' tfpdef ['=' test])* [','])
tfpdef: NAME [':' test]
varargslist: ((vfpdef ['=' test] ',')*
              ('*' [vfpdef] (',' vfpdef ['=' test])*  [',' '**' vfpdef] | '**' vfpdef)
              | vfpdef ['=' test] (',' vfpdef ['=' test])* [','])
vfpdef: NAME

stmt: simple_stmt | compound_stmt
simple_stmt: small_stmt (';' small_stmt)* [';'] NEWLINE
small_stmt: (expr_stmt | del_stmt | pass_stmt | flow_stmt |
             import_stmt | global_stmt | nonlocal_stmt | assert_stmt)
expr_stmt: testlist (augassign (yield_expr|testlist) |
                     ('=' (yield_expr|testlist))*)
augassign: ('+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^=' |
            '<<=' | '>>=' | '**=' | '//=')
# For normal assignments, additional restrictions enforced by the interpreter
del_stmt: 'del' exprlist
pass_stmt: 'pass'
flow_stmt: break_stmt | continue_stmt | return_stmt | raise_stmt | yield_stmt
break_stmt: 'break'
continue_stmt: 'continue'
return_stmt: 'return' [testlist]
yield_stmt: yield_expr
raise_stmt: 'raise' [test ['from' test]]
import_stmt: import_name | import_from
import_name: 'import' dotted_as_names
# note below: the ('.' | '...') is necessary because '...' is tokenized as ELLIPSIS
import_from: ('from' (('.' | '...')* dotted_name | ('.' | '...')+)
              'import' ('*' | '(' import_as_names ')' | import_as_names))
import_as_name: NAME ['as' NAME]
dotted_as_name: dotted_name ['as' NAME]
import_as_names: import_as_name (',' import_as_name)* [',']
dotted_as_names: dotted_as_name (',' dotted_as_name)*
dotted_name: NAME ('.' NAME)*
global_stmt: 'global' NAME (',' NAME)*
nonlocal_stmt: 'nonlocal' NAME (',' NAME)*
assert_stmt: 'assert' test [',' test]

compound_stmt: if_stmt | while_stmt | for_stmt | try_stmt | with_stmt | funcdef | classdef | decorated
if_stmt: 'if' test ':' suite ('elif' test ':' suite)* ['else' ':' suite]
while_stmt: 'while' test ':' suite ['else' ':' suite]
for_stmt: 'for' exprlist 'in' testlist ':' suite ['else' ':' suite]
try_stmt: ('try' ':' suite
           ((except_clause ':' suite)+
	    ['else' ':' suite]
	    ['finally' ':' suite] |
	   'finally' ':' suite))
with_stmt: 'with' test [ with_var ] ':' suite
with_var: 'as' expr
# NB compile.c makes sure that the default except clause is last
except_clause: 'except' [test ['as' NAME]]
suite: simple_stmt | NEWLINE INDENT stmt+ DEDENT

test: or_test ['if' or_test 'else' test] | lambdef
test_nocond: or_test | lambdef_nocond
lambdef: 'lambda' [varargslist] ':' test
lambdef_nocond: 'lambda' [varargslist] ':' test_nocond
or_test: and_test ('or' and_test)*
and_test: not_test ('and' not_test)*
not_test: 'not' not_test | comparison
comparison: star_expr (comp_op star_expr)*
comp_op: '<'|'>'|'=='|'>='|'<='|'!='|'in'|'not' 'in'|'is'|'is' 'not'
star_expr: ['*'] expr
expr: xor_expr ('|' xor_expr)*
xor_expr: and_expr ('^' and_expr)*
and_expr: shift_expr ('&' shift_expr)*
shift_expr: arith_expr (('<<'|'>>') arith_expr)*
arith_expr: term (('+'|'-') term)*
term: factor (('*'|'/'|'%'|'//') factor)*
factor: ('+'|'-'|'~') factor | power
power: atom trailer* ['**' factor]
atom: ('(' [yield_expr|testlist_comp] ')' |
       '[' [testlist_comp] ']' |
       '{' [dictorsetmaker] '}' |
       NAME | NUMBER | STRING+ | '...' | 'None' | 'True' | 'False')
testlist_comp: test ( comp_for | (',' test)* [','] )
trailer: '(' [arglist] ')' | '[' subscriptlist ']' | '.' NAME
subscriptlist: subscript (',' subscript)* [',']
subscript: test | [test] ':' [test] [sliceop]
sliceop: ':' [test]
exprlist: star_expr (',' star_expr)* [',']
testlist: test (',' test)* [',']
dictorsetmaker: ( (test ':' test (comp_for | (',' test ':' test)* [','])) |
                  (test (comp_for | (',' test)* [','])) )

classdef: 'class' NAME ['(' [arglist] ')'] ':' suite

arglist: (argument ',')* (argument [',']
                         |'*' test (',' argument)* [',' '**' test] 
                         |'**' test)
argument: test [comp_for] | test '=' test  # Really [keyword '='] test

comp_iter: comp_for | comp_if
comp_for: 'for' exprlist 'in' or_test [comp_iter]
comp_if: 'if' test_nocond [comp_iter]

testlist1: test (',' test)*

# not used in grammar, but may appear in "node" passed from Parser to Compiler
encoding_decl: NAME

yield_expr: 'yield' [testlist]
*/

YogParser* 
YogParser_new(YogEnv* env) 
{
    YogParser* parser = ALLOC_OBJ(env, NULL, YogParser);
    parser->env = env;
    parser->lexer = YogLexer_new(env);

    return parser;
}

YogArray* 
YogParser_parse_file(YogEnv* env, YogParser* parser, const char* filename)
{
    YogLexer* lexer = parser->lexer;
    if (filename != NULL) {
        lexer->fp = fopen(filename, "r");
        YogLexer_read_encoding(env, lexer);
    }
    else {
        lexer->fp = stdin;
    }

    yyparse(parser);

    if (filename != NULL) {
        fclose(lexer->fp);
    }

    return parser->stmts;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */

