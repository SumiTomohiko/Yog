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
     COMP_OP = 264,
     DEF = 265,
     DO = 266,
     DOT = 267,
     DOUBLE_STAR = 268,
     ELIF = 269,
     ELSE = 270,
     END = 271,
     EQUAL = 272,
     EXCEPT = 273,
     FINALLY = 274,
     GREATER = 275,
     IF = 276,
     LBRACE = 277,
     LBRACKET = 278,
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
#define COMP_OP 264
#define DEF 265
#define DO 266
#define DOT 267
#define DOUBLE_STAR 268
#define ELIF 269
#define ELSE 270
#define END 271
#define EQUAL 272
#define EXCEPT 273
#define FINALLY 274
#define GREATER 275
#define IF 276
#define LBRACE 277
#define LBRACKET 278
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

static YogEnv* parsing_env = NULL;
static YogVm* parsing_vm = NULL;
static YogArray* parsed_tree = NULL;

void 
Yog_set_parsing_env(YogEnv* env) 
{
    parsing_env = env;
    parsing_vm = ENV_VM(env);
}

YogEnv*
Yog_get_parsing_env()
{
    return parsing_env;
}

YogArray* 
Yog_get_parsed_tree() 
{
    return parsed_tree;
}

#define ENV parsing_env
#define VM  parsing_vm

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
    node->lineno = Yog_get_lineno();
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

/* XXX: To avoid warning. Better way? */
int yylex(void);


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
#line 256 "parser.y"
typedef union YYSTYPE {
    YogArray* array;
    YogNode* node;
    YogVal val;
    ID name;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 417 "parser.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 429 "parser.c"

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
#define YYLAST   167

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  38
/* YYNRULES -- Number of rules. */
#define YYNRULES  102
/* YYNRULES -- Number of states. */
#define YYNSTATES  172

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
     309,   311,   313,   315,   317,   321,   323,   325,   327,   329,
     331,   337,   341,   342,   344,   345,   352,   354,   357,   364,
     369,   373,   374
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      37,     0,    -1,    38,    -1,    39,    -1,    38,    26,    39,
      -1,    -1,    43,    -1,    54,    -1,    25,    53,    -1,    34,
      38,    71,    15,    38,    73,    16,    -1,    34,    38,    71,
      73,    16,    -1,    34,    38,    19,    38,    16,    -1,    35,
      54,    38,    16,    -1,     6,    -1,     6,    54,    -1,    27,
      -1,    27,    54,    -1,    21,    54,    38,    41,    16,    -1,
       7,    25,    40,    38,    16,    -1,    -1,    20,    54,    -1,
      42,    -1,    14,    54,    38,    41,    -1,    -1,    15,    38,
      -1,    10,    25,    24,    44,    32,    38,    16,    -1,    50,
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
      13,    25,    -1,    33,    25,    -1,     3,    25,    48,    -1,
      -1,    49,    -1,    17,    54,    -1,    25,    -1,    50,     8,
      25,    -1,    52,    -1,    51,     8,    52,    -1,    25,    49,
      -1,    54,    -1,    53,     8,    54,    -1,    55,    -1,    25,
      17,    56,    -1,    56,    -1,    57,    -1,    58,    -1,    59,
      -1,    60,    -1,    60,     9,    60,    -1,    61,    -1,    62,
      -1,    63,    -1,    64,    -1,    65,    -1,    64,    29,    65,
      -1,    66,    -1,    67,    -1,    68,    -1,    25,    -1,    28,
      -1,    68,    24,    69,    32,    70,    -1,    68,    12,    25,
      -1,    -1,    53,    -1,    -1,    11,    23,    44,    31,    38,
      16,    -1,    72,    -1,    71,    72,    -1,    18,    54,     4,
      25,    26,    38,    -1,    18,    54,    26,    38,    -1,    18,
      26,    38,    -1,    -1,    19,    38,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   339,   339,   343,   346,   350,   353,   354,   355,   365,
     368,   371,   374,   380,   383,   386,   389,   392,   395,   403,
     406,   410,   411,   417,   420,   424,   428,   431,   434,   437,
     440,   443,   446,   449,   452,   455,   458,   461,   464,   467,
     470,   473,   476,   479,   482,   485,   488,   491,   494,   497,
     500,   503,   506,   509,   512,   515,   518,   521,   525,   529,
     533,   537,   540,   542,   546,   550,   555,   558,   562,   566,
     569,   573,   575,   581,   583,   585,   587,   589,   590,   594,
     596,   598,   600,   602,   603,   607,   609,   611,   613,   616,
     621,   629,   636,   639,   641,   644,   648,   651,   655,   659,
     662,   666,   669
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "AMPER", "AS", "BAR", "BREAK", "CLASS",
  "COMMA", "COMP_OP", "DEF", "DO", "DOT", "DOUBLE_STAR", "ELIF", "ELSE",
  "END", "EQUAL", "EXCEPT", "FINALLY", "GREATER", "IF", "LBRACE",
  "LBRACKET", "LPAR", "NAME", "NEWLINE", "NEXT", "NUMBER", "PLUS",
  "RBRACE", "RBRACKET", "RPAR", "STAR", "TRY", "WHILE", "$accept",
  "module", "stmts", "stmt", "super_opt", "if_tail", "else_opt",
  "func_def", "params", "kw_param", "var_param", "block_param",
  "param_default_opt", "param_default", "params_without_default",
  "params_with_default", "param_with_default", "args", "expr",
  "assign_expr", "logical_or_expr", "logical_and_expr", "not_expr",
  "comparison", "xor_expr", "or_expr", "and_expr", "shift_expr",
  "arith_expr", "term", "factor", "power", "atom", "args_opt",
  "blockarg_opt", "excepts", "except", "finally_opt", 0
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
      61,    62,    63,    64,    64,    65,    66,    67,    68,    68,
      68,    68,    69,    69,    70,    70,    71,    71,    72,    72,
      72,    73,    73
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
       1,     1,     1,     1,     3,     1,     1,     1,     1,     1,
       5,     3,     0,     1,     0,     6,     1,     2,     6,     4,
       3,     0,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       5,    13,     0,     0,     0,    88,    15,    89,     5,     0,
       0,     2,     3,     6,     7,    71,    73,    74,    75,    76,
      77,    79,    80,    81,    82,    83,    85,    86,    87,    88,
      14,    19,     0,     5,     0,     8,    69,    16,     0,     5,
       1,     5,     0,     0,     0,    92,     0,     5,    57,    23,
      88,    72,     0,     0,     5,   101,    96,     0,     4,    78,
      84,    91,    93,     0,    20,     0,     0,     0,    64,     0,
       0,    56,    55,    53,    41,    49,    66,     0,     5,     0,
      21,    70,     5,     0,     0,     5,     5,    97,     0,    12,
      94,    18,    61,    58,     0,    68,    59,     5,     0,     0,
       0,     0,     5,    24,    17,   100,     0,     5,    11,   101,
     102,    10,     0,    90,    60,    62,    63,     0,    54,    52,
      51,    65,    40,    39,    37,    33,     0,    48,    47,    45,
      67,    23,     0,    99,     0,    57,    25,     0,     0,     0,
       0,     0,     0,    22,     5,     9,     0,    50,    38,    36,
      35,    32,    31,    29,    46,    44,    43,    98,     5,     0,
       0,     0,     0,     0,    34,    30,    28,    27,    42,    95,
       0,    26
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    10,    11,    12,    47,    79,    80,    13,    70,    71,
      72,    73,   114,    95,    74,    75,    76,    35,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    63,   113,    55,    56,    88
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -100
static const short int yypact[] =
{
      80,    48,   -12,     9,    48,    40,    48,  -100,    80,    48,
      18,    24,  -100,  -100,  -100,  -100,  -100,  -100,  -100,  -100,
      26,  -100,  -100,  -100,    22,  -100,  -100,  -100,    -7,    54,
    -100,    36,    60,    80,    70,    98,  -100,  -100,    93,    80,
    -100,    80,    70,    70,    88,    48,    48,    80,    11,    23,
    -100,  -100,    48,    92,    80,    -3,  -100,    14,  -100,  -100,
    -100,  -100,    98,    89,  -100,    59,    91,    97,   106,    99,
      94,  -100,   101,   117,   119,   120,  -100,    48,    80,   113,
    -100,  -100,    80,     6,    76,    80,    80,  -100,   114,  -100,
     121,  -100,   106,  -100,    48,  -100,  -100,    80,   118,    -4,
      20,    39,    80,    24,  -100,    24,   108,    80,  -100,    78,
      24,  -100,   111,  -100,  -100,  -100,  -100,    77,  -100,  -100,
     127,   106,  -100,   129,   130,   131,   106,  -100,   132,   133,
    -100,    23,   116,    24,   128,    11,  -100,   118,   118,    -4,
      39,   118,    -4,  -100,    80,  -100,   112,  -100,  -100,  -100,
     137,  -100,   138,   139,  -100,  -100,   140,    24,    80,   118,
     118,    -4,   118,    84,  -100,  -100,  -100,   141,  -100,  -100,
     118,  -100
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
    -100,  -100,    -8,   110,  -100,    21,  -100,  -100,    19,   -79,
     -73,   -99,  -100,    61,  -100,    55,   -97,   115,     2,  -100,
     122,  -100,  -100,  -100,   123,  -100,  -100,  -100,  -100,   124,
    -100,  -100,  -100,  -100,  -100,  -100,   102,    49
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      38,   124,   129,    30,   130,    44,    33,    36,    37,    67,
     106,    39,    85,    31,    66,    53,    86,    45,    40,   118,
     119,   122,   127,    66,    67,    49,   120,   123,   128,    69,
      89,    57,   107,    67,    32,    42,    68,    77,    78,    65,
      41,   153,    66,   130,    69,   121,    84,    36,    64,    41,
      41,    43,    67,    69,    81,    83,    46,    34,   147,   148,
     149,   151,   154,   155,   126,    29,   150,   152,     7,   156,
     103,    34,    69,    29,   105,    91,     7,   109,   110,   102,
     164,   165,   166,   168,    48,    41,     1,     2,   167,   117,
       3,   171,   108,   136,   131,    50,   116,    86,     7,   133,
     169,     4,    41,    41,    41,     5,    52,     6,     7,    98,
      41,    53,    54,    61,     8,     9,    92,    29,    82,    41,
       7,    90,    93,    94,    96,    99,    97,   100,   101,   104,
     111,    67,   112,   132,   135,   137,   157,   138,   139,   140,
     141,   142,   144,   158,   145,   159,   160,   161,   162,   170,
     163,    58,   143,   115,   146,   125,    51,    87,   134,     0,
      62,     0,     0,     0,     0,    59,     0,    60
};

static const short int yycheck[] =
{
       8,   100,   101,     1,   101,    12,     4,     5,     6,    13,
       4,     9,    15,    25,     3,    18,    19,    24,     0,    98,
      99,   100,   101,     3,    13,    33,    99,   100,   101,    33,
      16,    39,    26,    13,    25,     9,    25,    14,    15,    47,
      26,   140,     3,   140,    33,    25,    54,    45,    46,    26,
      26,    29,    13,    33,    52,    53,    20,    17,   137,   138,
     139,   140,   141,   142,    25,    25,   139,   140,    28,   142,
      78,    17,    33,    25,    82,    16,    28,    85,    86,    77,
     159,   160,   161,   162,    24,    26,     6,     7,   161,    97,
      10,   170,    16,    16,   102,    25,    94,    19,    28,   107,
      16,    21,    26,    26,    26,    25,     8,    27,    28,     8,
      26,    18,    19,    25,    34,    35,    25,    25,    26,    26,
      28,    32,    25,    17,    25,     8,    32,     8,     8,    16,
      16,    13,    11,    25,    23,     8,   144,     8,     8,     8,
       8,     8,    26,    31,    16,     8,     8,     8,     8,     8,
     158,    41,   131,    92,   135,   100,    34,    55,   109,    -1,
      45,    -1,    -1,    -1,    -1,    42,    -1,    43
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     6,     7,    10,    21,    25,    27,    28,    34,    35,
      37,    38,    39,    43,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    25,
      54,    25,    25,    54,    17,    53,    54,    54,    38,    54,
       0,    26,     9,    29,    12,    24,    20,    40,    24,    38,
      25,    56,     8,    18,    19,    71,    72,    38,    39,    60,
      65,    25,    53,    69,    54,    38,     3,    13,    25,    33,
      44,    45,    46,    47,    50,    51,    52,    14,    15,    41,
      42,    54,    26,    54,    38,    15,    19,    72,    73,    16,
      32,    16,    25,    25,    17,    49,    25,    32,     8,     8,
       8,     8,    54,    38,    16,    38,     4,    26,    16,    38,
      38,    16,    11,    70,    48,    49,    54,    38,    45,    45,
      46,    25,    45,    46,    47,    51,    25,    45,    46,    47,
      52,    38,    25,    38,    73,    23,    16,     8,     8,     8,
       8,     8,     8,    41,    26,    16,    44,    45,    45,    45,
      46,    45,    46,    47,    45,    45,    46,    38,    31,     8,
       8,     8,     8,    38,    45,    45,    45,    46,    45,    16,
       8,    45
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
#line 339 "parser.y"
    {
            parsed_tree = (yyvsp[0].array);
        }
    break;

  case 3:
#line 343 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 4:
#line 346 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
        }
    break;

  case 5:
#line 350 "parser.y"
    {
            (yyval.node) = NULL;
        }
    break;

  case 8:
#line 355 "parser.y"
    {
            COMMAND_CALL_NEW((yyval.node), (yyvsp[-1].name), (yyvsp[0].array), NULL);
        }
    break;

  case 9:
#line 365 "parser.y"
    {
            EXCEPT_FINALLY_NEW((yyval.node), (yyvsp[-5].array), (yyvsp[-4].array), (yyvsp[-2].array), (yyvsp[-1].array));
        }
    break;

  case 10:
#line 368 "parser.y"
    {
            EXCEPT_FINALLY_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-2].array), NULL, (yyvsp[-1].array));
        }
    break;

  case 11:
#line 371 "parser.y"
    {
            FINALLY_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-1].array));
        }
    break;

  case 12:
#line 374 "parser.y"
    {
            YogNode* node = NODE_NEW(NODE_WHILE);
            NODE_TEST(node) = (yyvsp[-2].node);
            NODE_STMTS(node) = (yyvsp[-1].array);
            (yyval.node) = node;
        }
    break;

  case 13:
#line 380 "parser.y"
    {
            BREAK_NEW((yyval.node), NULL);
        }
    break;

  case 14:
#line 383 "parser.y"
    {
            BREAK_NEW((yyval.node), (yyvsp[0].node));
        }
    break;

  case 15:
#line 386 "parser.y"
    {
            NEXT_NEW((yyval.node), NULL);
        }
    break;

  case 16:
#line 389 "parser.y"
    {
            NEXT_NEW((yyval.node), (yyvsp[0].node));
        }
    break;

  case 17:
#line 392 "parser.y"
    {
            IF_NEW((yyval.node), (yyvsp[-3].node), (yyvsp[-2].array), (yyvsp[-1].array));
        }
    break;

  case 18:
#line 395 "parser.y"
    {
            YogNode* node = NODE_NEW(NODE_KLASS);
            NODE_NAME(node) = (yyvsp[-3].name);
            NODE_SUPER(node) = (yyvsp[-2].node);
            NODE_STMTS(node) = (yyvsp[-1].array);
            (yyval.node) = node;
        }
    break;

  case 19:
#line 403 "parser.y"
    {
                (yyval.node) = NULL;
            }
    break;

  case 20:
#line 406 "parser.y"
    {
                (yyval.node) = (yyvsp[0].node);
            }
    break;

  case 22:
#line 411 "parser.y"
    {
            YogNode* node = NULL;
            IF_NEW(node, (yyvsp[-2].node), (yyvsp[-1].array), (yyvsp[0].array));
            OBJ_ARRAY_NEW((yyval.array), node);
        }
    break;

  case 23:
#line 417 "parser.y"
    {
                (yyval.array) = NULL;
            }
    break;

  case 24:
#line 420 "parser.y"
    {
                (yyval.array) = (yyvsp[0].array);
            }
    break;

  case 25:
#line 424 "parser.y"
    {
                FUNC_DEF_NEW((yyval.node), (yyvsp[-5].name), (yyvsp[-3].array), (yyvsp[-1].array));
            }
    break;

  case 26:
#line 428 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-8].array), (yyvsp[-6].array), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 27:
#line 431 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 28:
#line 434 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 29:
#line 437 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 30:
#line 440 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 31:
#line 443 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 32:
#line 446 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 33:
#line 449 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), (yyvsp[0].array), NULL, NULL, NULL);
        }
    break;

  case 34:
#line 452 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), NULL, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 35:
#line 455 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NULL, (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 36:
#line 458 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NULL, (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 37:
#line 461 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NULL, (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 38:
#line 464 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 39:
#line 467 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NULL, NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 40:
#line 470 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NULL, NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 41:
#line 473 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[0].array), NULL, NULL, NULL, NULL);
        }
    break;

  case 42:
#line 476 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-6].array), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 43:
#line 479 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-4].array), (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 44:
#line 482 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-4].array), (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 45:
#line 485 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-2].array), (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 46:
#line 488 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-4].array), NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 47:
#line 491 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-2].array), NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 48:
#line 494 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-2].array), NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 49:
#line 497 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[0].array), NULL, NULL, NULL);
        }
    break;

  case 50:
#line 500 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 51:
#line 503 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 52:
#line 506 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 53:
#line 509 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 54:
#line 512 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 55:
#line 515 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 56:
#line 518 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 57:
#line 521 "parser.y"
    {
            (yyval.array) = NULL;
        }
    break;

  case 58:
#line 525 "parser.y"
    {
                PARAM_NEW((yyval.node), NODE_KW_PARAM, (yyvsp[0].name), NULL);
            }
    break;

  case 59:
#line 529 "parser.y"
    {
                PARAM_NEW((yyval.node), NODE_VAR_PARAM, (yyvsp[0].name), NULL);
            }
    break;

  case 60:
#line 533 "parser.y"
    {
                    PARAM_NEW((yyval.node), NODE_BLOCK_PARAM, (yyvsp[-1].name), (yyvsp[0].node));
                }
    break;

  case 61:
#line 537 "parser.y"
    {
                        (yyval.node) = NULL;
                    }
    break;

  case 63:
#line 542 "parser.y"
    {
                    (yyval.node) = (yyvsp[0].node);
                }
    break;

  case 64:
#line 546 "parser.y"
    {
                            (yyval.array) = YogArray_new(ENV);
                            PARAM_ARRAY_PUSH((yyval.array), (yyvsp[0].name), NULL);
                        }
    break;

  case 65:
#line 550 "parser.y"
    {
                            PARAM_ARRAY_PUSH((yyvsp[-2].array), (yyvsp[0].name), NULL);
                            (yyval.array) = (yyvsp[-2].array);
                        }
    break;

  case 66:
#line 555 "parser.y"
    {
                            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
                        }
    break;

  case 67:
#line 558 "parser.y"
    {
                            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
                        }
    break;

  case 68:
#line 562 "parser.y"
    {
                        PARAM_NEW((yyval.node), NODE_PARAM, (yyvsp[-1].name), (yyvsp[0].node));
                    }
    break;

  case 69:
#line 566 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 70:
#line 569 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
        }
    break;

  case 72:
#line 575 "parser.y"
    {
                YogNode* node = NODE_NEW(NODE_ASSIGN);
                NODE_LEFT(node) = (yyvsp[-2].name);
                NODE_RIGHT(node) = (yyvsp[0].node);
                (yyval.node) = node;
            }
    break;

  case 78:
#line 590 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 84:
#line 603 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 88:
#line 613 "parser.y"
    {
            VARIABLE_NEW((yyval.node), (yyvsp[0].name));
        }
    break;

  case 89:
#line 616 "parser.y"
    {
            YogNode* node = NODE_NEW(NODE_LITERAL);
            NODE_VAL(node) = (yyvsp[0].val);
            (yyval.node) = node;
        }
    break;

  case 90:
#line 621 "parser.y"
    {
            if ((yyvsp[-4].node)->type == NODE_ATTR) {
                METHOD_CALL_NEW((yyval.node), NODE_OBJ((yyvsp[-4].node)), NODE_NAME((yyvsp[-4].node)), (yyvsp[-2].array), (yyvsp[0].node));
            }
            else {
                FUNC_CALL_NEW((yyval.node), (yyvsp[-4].node), (yyvsp[-2].array), (yyvsp[0].node));
            }
        }
    break;

  case 91:
#line 629 "parser.y"
    {
            YogNode* node = NODE_NEW(NODE_ATTR);
            NODE_OBJ(node) = (yyvsp[-2].node);
            NODE_NAME(node) = (yyvsp[0].name);
            (yyval.node) = node;
        }
    break;

  case 92:
#line 636 "parser.y"
    {
                (yyval.array) = NULL;
            }
    break;

  case 94:
#line 641 "parser.y"
    {
                    (yyval.node) = NULL;
                }
    break;

  case 95:
#line 644 "parser.y"
    {
                    BLOCK_ARG_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-1].array));
                }
    break;

  case 96:
#line 648 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 97:
#line 651 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-1].array), (yyvsp[0].node));
        }
    break;

  case 98:
#line 655 "parser.y"
    {
            Yog_assert(ENV, (yyvsp[-2].name) != NO_EXC_VAR, "Too many variables.");
            EXCEPT_BODY_NEW((yyval.node), (yyvsp[-4].node), (yyvsp[-2].name), (yyvsp[0].array));
        }
    break;

  case 99:
#line 659 "parser.y"
    {
            EXCEPT_BODY_NEW((yyval.node), (yyvsp[-2].node), NO_EXC_VAR, (yyvsp[0].array));
        }
    break;

  case 100:
#line 662 "parser.y"
    {
            EXCEPT_BODY_NEW((yyval.node), NULL, NO_EXC_VAR, (yyvsp[0].array));
        }
    break;

  case 101:
#line 666 "parser.y"
    {
                (yyval.array) = NULL;
            }
    break;

  case 102:
#line 669 "parser.y"
    {
                (yyval.array) = (yyvsp[0].array);
            }
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2185 "parser.c"

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


#line 673 "parser.y"

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
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */

