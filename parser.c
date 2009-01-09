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
     DIV = 265,
     DO = 266,
     DOT = 267,
     DOUBLE_STAR = 268,
     ELIF = 269,
     ELSE = 270,
     END = 271,
     EQUAL = 272,
     EQUAL_TILDA = 273,
     EXCEPT = 274,
     FINALLY = 275,
     GREATER = 276,
     IF = 277,
     LBRACE = 278,
     LBRACKET = 279,
     LESS = 280,
     LPAR = 281,
     LSHIFT = 282,
     NAME = 283,
     NEWLINE = 284,
     NEXT = 285,
     NONLOCAL = 286,
     NUMBER = 287,
     PLUS = 288,
     RBRACE = 289,
     RBRACKET = 290,
     REGEXP = 291,
     RETURN = 292,
     RPAR = 293,
     STAR = 294,
     STRING = 295,
     TRY = 296,
     WHILE = 297,
     tFALSE = 298,
     tTRUE = 299,
     t__LINE__ = 300
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
#define DIV 265
#define DO 266
#define DOT 267
#define DOUBLE_STAR 268
#define ELIF 269
#define ELSE 270
#define END 271
#define EQUAL 272
#define EQUAL_TILDA 273
#define EXCEPT 274
#define FINALLY 275
#define GREATER 276
#define IF 277
#define LBRACE 278
#define LBRACKET 279
#define LESS 280
#define LPAR 281
#define LSHIFT 282
#define NAME 283
#define NEWLINE 284
#define NEXT 285
#define NONLOCAL 286
#define NUMBER 287
#define PLUS 288
#define RBRACE 289
#define RBRACKET 290
#define REGEXP 291
#define RETURN 292
#define RPAR 293
#define STAR 294
#define STRING 295
#define TRY 296
#define WHILE 297
#define tFALSE 298
#define tTRUE 299
#define t__LINE__ 300




/* Copy the first part of user declarations.  */
#line 1 "parser.y"

#include <stdio.h>
#include "yog/error.h"
#include "yog/parser.h"
#include "yog/yog.h"

#define YYPARSE_PARAM   parser
#define PARSER          ((YogParser*)YYPARSE_PARAM)
#define YYLEX_PARAM     (PARSER)->lexer
#define ENV             (PARSER)->env

static void 
yyerror(char* s)
{
    fprintf(stderr, "%s\n", s);
}

static void 
YogNode_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogNode* node = ptr;

#define KEEP(member)    node->u.member = (*keeper)(env, node->u.member)
    switch (node->type) {
    case NODE_ASSIGN:
        KEEP(assign.left);
        KEEP(assign.right);
        break;
    case NODE_ATTR:
        KEEP(attr.obj);
        break;
    case NODE_BLOCK_ARG:
        KEEP(blockarg.params);
        KEEP(blockarg.stmts);
        break;
    case NODE_BLOCK_PARAM:
    case NODE_KW_PARAM:
    case NODE_PARAM:
    case NODE_VAR_PARAM:
        KEEP(param.default_);
        break;
    case NODE_BREAK:
        KEEP(break_.expr);
        break;
    case NODE_COMMAND_CALL:
        KEEP(command_call.args);
        KEEP(command_call.blockarg);
        break;
    case NODE_EXCEPT:
        KEEP(except.head);
        KEEP(except.excepts);
        KEEP(except.else_);
        break;
    case NODE_EXCEPT_BODY:
        KEEP(except_body.type);
        KEEP(except_body.stmts);
        break;
    case NODE_FINALLY:
        KEEP(finally.head);
        KEEP(finally.body);
        break;
    case NODE_FUNC_CALL:
        KEEP(func_call.callee);
        KEEP(func_call.args);
        KEEP(func_call.blockarg);
        break;
    case NODE_FUNC_DEF:
        KEEP(funcdef.params);
        KEEP(funcdef.stmts);
        break;
    case NODE_IF:
        KEEP(if_.test);
        KEEP(if_.stmts);
        KEEP(if_.tail);
        break;
    case NODE_KLASS:
        KEEP(klass.super);
        KEEP(klass.stmts);
        break;
    case NODE_LITERAL:
        node->u.literal.val = YogVal_keep(env, node->u.literal.val, keeper);
        break;
    case NODE_METHOD_CALL:
        KEEP(method_call.recv);
        KEEP(method_call.args);
        KEEP(method_call.blockarg);
        break;
    case NODE_NEXT:
        KEEP(next.expr);
        break;
    case NODE_NONLOCAL:
        KEEP(nonlocal.names);
        break;
    case NODE_RETURN:
        KEEP(return_.expr);
        break;
    case NODE_SUBSCRIPT:
        KEEP(subscript.prefix);
        KEEP(subscript.index);
        break;
    case NODE_VARIABLE:
        break;
    case NODE_WHILE:
        KEEP(while_.test);
        KEEP(while_.stmts);
        break;
    default:
        YOG_ASSERT(env, FALSE, "Unknown node type.");
        break;
    }
#undef KEEP
}

static YogNode* 
YogNode_new(YogEnv* env, YogParser* parser, YogNodeType type) 
{
    YogNode* node = ALLOC_OBJ(env, YogNode_keep_children, NULL, YogNode);
    node->lineno = parser->lineno;
    node->type = type;

    return node;
}

#define NODE_NEW(type)  YogNode_new(ENV, PARSER, type)

#define LITERAL_NEW(node, val_) do { \
    node = NODE_NEW(NODE_LITERAL); \
    node->u.literal.val = val_; \
} while (0)

#define BLOCK_ARG_NEW(node, params_, stmts_) do { \
    node = NODE_NEW(NODE_BLOCK_ARG); \
    node->u.blockarg.params = params_; \
    node->u.blockarg.stmts = stmts_; \
} while (0)

#define PARAMS_NEW(array, params_without_default, params_with_default, block_param, var_param, kw_param) do { \
    array = YogArray_new(ENV); \
    \
    if (params_without_default != NULL) { \
        YogArray_extend(ENV, array, params_without_default); \
    } \
    \
    if (params_with_default != NULL) { \
        YogArray_extend(ENV, array, params_with_default); \
    } \
    \
    if (block_param != NULL) { \
        YogVal val = PTR2VAL(block_param); \
        YogArray_push(ENV, array, val); \
    } \
    \
    if (var_param != NULL) { \
        YogVal val = PTR2VAL(var_param); \
        YogArray_push(ENV, array, val); \
    } \
    \
    if (kw_param != NULL) { \
        YogVal val = PTR2VAL(kw_param); \
        YogArray_push(ENV, array, val); \
    } \
} while (0)

#define COMMAND_CALL_NEW(node, name_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_COMMAND_CALL); \
    node->u.command_call.name = name_; \
    node->u.command_call.args = args_; \
    node->u.command_call.blockarg = blockarg_; \
} while (0)

#define OBJ_ARRAY_NEW(array, elem) do { \
    if (elem != NULL) { \
        array = YogArray_new(ENV); \
        YogArray_push(ENV, array, PTR2VAL(elem)); \
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
        YogArray_push(ENV, array, PTR2VAL(elem)); \
    } \
    result = array; \
} while (0)

#define PARAM_NEW(node, type, id, default__) do { \
    node = NODE_NEW(type); \
    node->u.param.name = id; \
    node->u.param.default_ = default__; \
} while (0)

#define PARAM_ARRAY_PUSH(array, id, default_) do { \
    YogNode* node = NULL; \
    PARAM_NEW(node, NODE_PARAM, id, default_); \
    YogArray_push(ENV, array, PTR2VAL(node)); \
} while (0)

#define FUNC_DEF_NEW(node, name_, params_, stmts_) do { \
    node = NODE_NEW(NODE_FUNC_DEF); \
    node->u.funcdef.name = name_; \
    node->u.funcdef.params = params_; \
    node->u.funcdef.stmts = stmts_; \
} while (0)

#define FUNC_CALL_NEW(node, callee_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_FUNC_CALL); \
    node->u.func_call.callee = callee_; \
    node->u.func_call.args = args_; \
    node->u.func_call.blockarg = blockarg_; \
} while (0)

#define VARIABLE_NEW(node, id_) do { \
    node = NODE_NEW(NODE_VARIABLE); \
    node->u.variable.id = id_; \
} while (0)

#define EXCEPT_BODY_NEW(node, type_, var_, stmts_) do { \
    node = NODE_NEW(NODE_EXCEPT_BODY); \
    node->u.except_body.type = type_; \
    node->u.except_body.var = var_; \
    node->u.except_body.stmts = stmts_; \
} while (0)

#define EXCEPT_NEW(node, head_, excepts_, else__) do { \
    node = NODE_NEW(NODE_EXCEPT); \
    node->u.except.head = head_; \
    node->u.except.excepts = excepts_; \
    node->u.except.else_ = else__; \
} while (0)

#define FINALLY_NEW(node, head_, body_) do { \
    node = NODE_NEW(NODE_FINALLY); \
    node->u.finally.head = head_; \
    node->u.finally.body = body_; \
} while (0)

#define EXCEPT_FINALLY_NEW(node, stmts, excepts, else_, finally) do { \
    EXCEPT_NEW(node, stmts, excepts, else_); \
    \
    if (finally != NULL) { \
        YogArray* array = NULL; \
        OBJ_ARRAY_NEW(array, node); \
        FINALLY_NEW(node, array, finally); \
    } \
} while (0)

#define BREAK_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_BREAK); \
    node->u.break_.expr = expr_; \
} while (0)

#define NEXT_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_NEXT); \
    node->u.next.expr = expr_; \
} while (0)

#define RETURN_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_RETURN); \
    node->u.return_.expr = expr_; \
} while (0)

#define METHOD_CALL_NEW(node, recv_, name_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_METHOD_CALL); \
    node->u.method_call.recv = recv_; \
    node->u.method_call.name = name_; \
    node->u.method_call.args = args_; \
    node->u.method_call.blockarg = blockarg_; \
} while (0)

#define METHOD_CALL_NEW1(node, recv, name, arg) do { \
    YogArray* args = YogArray_new(ENV); \
    YogArray_push(ENV, args, PTR2VAL(arg)); \
    METHOD_CALL_NEW(node, recv, name, args, NULL); \
} while (0)

#define IF_NEW(node, test_, stmts_, tail_) do { \
    node = NODE_NEW(NODE_IF); \
    node->u.if_.test = test_; \
    node->u.if_.stmts = stmts_; \
    node->u.if_.tail = tail_; \
} while (0)

#define WHILE_NEW(node, test_, stmts_) do { \
    node = NODE_NEW(NODE_WHILE); \
    node->u.while_.test = test_; \
    node->u.while_.stmts = stmts_; \
} while (0)

#define KLASS_NEW(node, name_, super_, stmts_) do { \
    node = NODE_NEW(NODE_KLASS); \
    node->u.klass.name = name_; \
    node->u.klass.super = super_; \
    node->u.klass.stmts = stmts_; \
} while (0);

#define ASSIGN_NEW(node, left_, right_) do { \
    node = NODE_NEW(NODE_ASSIGN); \
    node->u.assign.left = left_; \
    node->u.assign.right = right_; \
} while (0)

#define SUBSCRIPT_NEW(node, prefix_, index_) do { \
    node = NODE_NEW(NODE_SUBSCRIPT); \
    node->u.subscript.prefix = prefix_; \
    node->u.subscript.index = index_; \
} while (0)

#define ATTR_NEW(node, obj_, name_) do { \
    node = NODE_NEW(NODE_ATTR); \
    node->u.attr.obj = obj_; \
    node->u.attr.name = name_; \
} while (0)

#define NONLOCAL_NEW(node, names_) do { \
    node = NODE_NEW(NODE_NONLOCAL); \
    node->u.nonlocal.names = names_; \
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
#line 325 "parser.y"
typedef union YYSTYPE {
    struct YogArray* array;
    struct YogNode* node;
    struct YogVal val;
    ID name;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 506 "parser.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 518 "parser.c"

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
#define YYFINAL  51
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   196

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  46
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  43
/* YYNRULES -- Number of rules. */
#define YYNRULES  119
/* YYNRULES -- Number of states. */
#define YYNSTATES  195

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   300

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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     7,    11,    12,    14,    16,    19,
      27,    33,    39,    44,    46,    49,    51,    54,    56,    59,
      65,    71,    74,    76,    80,    81,    84,    86,    91,    92,
      95,   103,   113,   121,   129,   135,   143,   149,   155,   159,
     167,   173,   179,   183,   189,   193,   197,   199,   207,   213,
     219,   223,   229,   233,   237,   239,   245,   249,   253,   255,
     259,   261,   263,   264,   267,   270,   274,   275,   277,   280,
     282,   286,   288,   292,   295,   297,   301,   303,   307,   309,
     311,   313,   315,   317,   321,   323,   325,   327,   329,   331,
     335,   337,   341,   343,   347,   349,   351,   353,   355,   361,
     366,   370,   372,   374,   376,   378,   380,   382,   384,   385,
     387,   388,   395,   397,   400,   407,   412,   416,   418,   419
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      47,     0,    -1,    48,    -1,    49,    -1,    48,    87,    49,
      -1,    -1,    54,    -1,    65,    -1,    28,    64,    -1,    41,
      48,    85,    15,    48,    88,    16,    -1,    41,    48,    85,
      88,    16,    -1,    41,    48,    20,    48,    16,    -1,    42,
      65,    48,    16,    -1,     6,    -1,     6,    65,    -1,    30,
      -1,    30,    65,    -1,    37,    -1,    37,    65,    -1,    22,
      65,    48,    52,    16,    -1,     7,    28,    51,    48,    16,
      -1,    31,    50,    -1,    28,    -1,    50,     8,    28,    -1,
      -1,    21,    65,    -1,    53,    -1,    14,    65,    48,    52,
      -1,    -1,    15,    48,    -1,     9,    28,    26,    55,    38,
      48,    16,    -1,    61,     8,    62,     8,    58,     8,    57,
       8,    56,    -1,    61,     8,    62,     8,    58,     8,    57,
      -1,    61,     8,    62,     8,    58,     8,    56,    -1,    61,
       8,    62,     8,    58,    -1,    61,     8,    62,     8,    57,
       8,    56,    -1,    61,     8,    62,     8,    57,    -1,    61,
       8,    62,     8,    56,    -1,    61,     8,    62,    -1,    61,
       8,    58,     8,    57,     8,    56,    -1,    61,     8,    58,
       8,    57,    -1,    61,     8,    58,     8,    56,    -1,    61,
       8,    58,    -1,    61,     8,    57,     8,    56,    -1,    61,
       8,    57,    -1,    61,     8,    56,    -1,    61,    -1,    62,
       8,    58,     8,    57,     8,    56,    -1,    62,     8,    58,
       8,    57,    -1,    62,     8,    58,     8,    56,    -1,    62,
       8,    58,    -1,    62,     8,    57,     8,    56,    -1,    62,
       8,    57,    -1,    62,     8,    56,    -1,    62,    -1,    58,
       8,    57,     8,    56,    -1,    58,     8,    57,    -1,    58,
       8,    56,    -1,    58,    -1,    57,     8,    56,    -1,    57,
      -1,    56,    -1,    -1,    13,    28,    -1,    39,    28,    -1,
       3,    28,    59,    -1,    -1,    60,    -1,    17,    65,    -1,
      28,    -1,    61,     8,    28,    -1,    63,    -1,    62,     8,
      63,    -1,    28,    60,    -1,    65,    -1,    64,     8,    65,
      -1,    66,    -1,    81,    17,    67,    -1,    67,    -1,    68,
      -1,    69,    -1,    70,    -1,    72,    -1,    72,    71,    72,
      -1,    25,    -1,    73,    -1,    74,    -1,    75,    -1,    76,
      -1,    75,    27,    77,    -1,    77,    -1,    76,    18,    77,
      -1,    78,    -1,    77,    33,    78,    -1,    79,    -1,    80,
      -1,    81,    -1,    82,    -1,    81,    26,    83,    38,    84,
      -1,    81,    24,    65,    35,    -1,    81,    12,    28,    -1,
      28,    -1,    32,    -1,    36,    -1,    40,    -1,    44,    -1,
      43,    -1,    45,    -1,    -1,    64,    -1,    -1,    11,    24,
      55,    35,    48,    16,    -1,    86,    -1,    85,    86,    -1,
      19,    65,     4,    28,    87,    48,    -1,    19,    65,    87,
      48,    -1,    19,    87,    48,    -1,    29,    -1,    -1,    20,
      48,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   426,   426,   430,   433,   437,   440,   441,   442,   452,
     455,   458,   461,   464,   467,   470,   473,   476,   479,   482,
     485,   488,   492,   496,   501,   504,   508,   509,   515,   518,
     522,   526,   529,   532,   535,   538,   541,   544,   547,   550,
     553,   556,   559,   562,   565,   568,   571,   574,   577,   580,
     583,   586,   589,   592,   595,   598,   601,   604,   607,   610,
     613,   616,   619,   623,   627,   631,   635,   638,   640,   644,
     648,   653,   656,   660,   664,   667,   671,   673,   676,   678,
     680,   682,   684,   685,   689,   691,   693,   695,   697,   698,
     702,   703,   707,   708,   712,   714,   716,   718,   719,   727,
     730,   734,   737,   740,   743,   746,   749,   752,   758,   761,
     763,   766,   770,   773,   777,   781,   784,   788,   792,   795
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "AMPER", "AS", "BAR", "BREAK", "CLASS",
  "COMMA", "DEF", "DIV", "DO", "DOT", "DOUBLE_STAR", "ELIF", "ELSE", "END",
  "EQUAL", "EQUAL_TILDA", "EXCEPT", "FINALLY", "GREATER", "IF", "LBRACE",
  "LBRACKET", "LESS", "LPAR", "LSHIFT", "NAME", "NEWLINE", "NEXT",
  "NONLOCAL", "NUMBER", "PLUS", "RBRACE", "RBRACKET", "REGEXP", "RETURN",
  "RPAR", "STAR", "STRING", "TRY", "WHILE", "tFALSE", "tTRUE", "t__LINE__",
  "$accept", "module", "stmts", "stmt", "names", "super_opt", "if_tail",
  "else_opt", "func_def", "params", "kw_param", "var_param", "block_param",
  "param_default_opt", "param_default", "params_without_default",
  "params_with_default", "param_with_default", "args", "expr",
  "assign_expr", "logical_or_expr", "logical_and_expr", "not_expr",
  "comparison", "comp_op", "xor_expr", "or_expr", "and_expr", "shift_expr",
  "match_expr", "arith_expr", "term", "factor", "power", "postfix_expr",
  "atom", "args_opt", "blockarg_opt", "excepts", "except", "newline",
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    46,    47,    48,    48,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    50,    50,    51,    51,    52,    52,    53,    53,
      54,    55,    55,    55,    55,    55,    55,    55,    55,    55,
      55,    55,    55,    55,    55,    55,    55,    55,    55,    55,
      55,    55,    55,    55,    55,    55,    55,    55,    55,    55,
      55,    55,    55,    56,    57,    58,    59,    59,    60,    61,
      61,    62,    62,    63,    64,    64,    65,    66,    66,    67,
      68,    69,    70,    70,    71,    72,    73,    74,    75,    75,
      76,    76,    77,    77,    78,    79,    80,    81,    81,    81,
      81,    82,    82,    82,    82,    82,    82,    82,    83,    83,
      84,    84,    85,    85,    86,    86,    86,    87,    88,    88
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     3,     0,     1,     1,     2,     7,
       5,     5,     4,     1,     2,     1,     2,     1,     2,     5,
       5,     2,     1,     3,     0,     2,     1,     4,     0,     2,
       7,     9,     7,     7,     5,     7,     5,     5,     3,     7,
       5,     5,     3,     5,     3,     3,     1,     7,     5,     5,
       3,     5,     3,     3,     1,     5,     3,     3,     1,     3,
       1,     1,     0,     2,     2,     3,     0,     1,     2,     1,
       3,     1,     3,     2,     1,     3,     1,     3,     1,     1,
       1,     1,     1,     3,     1,     1,     1,     1,     1,     3,
       1,     3,     1,     3,     1,     1,     1,     1,     5,     4,
       3,     1,     1,     1,     1,     1,     1,     1,     0,     1,
       0,     6,     1,     2,     6,     4,     3,     1,     0,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       5,    13,     0,     0,     0,   101,    15,     0,   102,   103,
      17,   104,     5,     0,   106,   105,   107,     0,     2,     3,
       6,     7,    76,    78,    79,    80,    81,    82,    85,    86,
      87,    88,    90,    92,    94,    95,    96,    97,   101,    14,
      24,     0,     5,     8,    74,    16,    22,    21,    18,     0,
       5,     1,   117,     5,    84,     0,     0,     0,     0,     0,
       0,     0,   108,     0,     5,    62,    28,     0,     0,     0,
       5,   118,   112,     0,     4,    83,    96,    89,    91,    93,
     100,    77,     0,   109,     0,    25,     0,     0,     0,    69,
       0,     0,    61,    60,    58,    46,    54,    71,     0,     5,
       0,    26,    75,    23,     0,     5,     0,     5,     5,   113,
       0,    12,    99,   110,    20,    66,    63,     0,    73,    64,
       5,     0,     0,     0,     0,     5,    29,    19,     0,     5,
     116,    11,   118,   119,    10,     0,    98,    65,    67,    68,
       0,    59,    57,    56,    70,    45,    44,    42,    38,     0,
      53,    52,    50,    72,    28,     0,   115,     0,    62,    30,
       0,     0,     0,     0,     0,     0,    27,     5,     9,     0,
      55,    43,    41,    40,    37,    36,    34,    51,    49,    48,
     114,     5,     0,     0,     0,     0,     0,    39,    35,    33,
      32,    47,   111,     0,    31
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,    17,    18,    19,    47,    64,   100,   101,    20,    91,
      92,    93,    94,   137,   118,    95,    96,    97,    43,    21,
      22,    23,    24,    25,    26,    55,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    84,   136,    71,
      72,    53,   110
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -116
static const short int yypact[] =
{
     100,   128,   -25,    -7,   128,   128,   128,     5,  -116,  -116,
     128,  -116,   100,   128,  -116,  -116,  -116,    31,    18,  -116,
    -116,  -116,  -116,  -116,  -116,  -116,  -116,    26,  -116,  -116,
      42,    27,    58,  -116,  -116,  -116,    48,  -116,  -116,  -116,
      50,    66,   100,    86,  -116,  -116,  -116,    89,  -116,    69,
     100,  -116,  -116,   100,  -116,   128,   128,   128,   128,    77,
     128,   128,   128,   128,   100,    16,    -2,   128,    88,   118,
     100,    95,  -116,    -1,  -116,  -116,    10,    58,    58,  -116,
    -116,  -116,    84,    86,    82,  -116,     8,    93,    96,   106,
      97,    91,  -116,   125,   126,   127,   130,  -116,   128,   100,
     110,  -116,  -116,  -116,    -3,   100,    37,   100,   100,  -116,
     111,  -116,  -116,   137,  -116,   106,  -116,   128,  -116,  -116,
     100,   136,     7,    22,    72,   100,    18,  -116,   123,   100,
      18,  -116,     3,    18,  -116,   115,  -116,  -116,  -116,  -116,
      38,  -116,  -116,   144,   106,  -116,   145,   149,   151,   106,
    -116,   157,   158,  -116,    -2,    18,    18,   154,    16,  -116,
     136,   136,     7,    72,   136,     7,  -116,   100,  -116,   132,
    -116,  -116,  -116,   170,  -116,   172,   173,  -116,  -116,   174,
      18,   100,   136,   136,     7,   136,    57,  -116,  -116,  -116,
     175,  -116,  -116,   136,  -116
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -116,  -116,   -12,   131,  -116,  -116,    32,  -116,  -116,    29,
     -81,  -106,  -115,  -116,    70,  -116,    65,  -114,   129,     1,
    -116,   133,  -116,  -116,  -116,  -116,   134,  -116,  -116,  -116,
    -116,    20,   138,  -116,  -116,   119,  -116,  -116,  -116,  -116,
     121,   -65,    62
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      49,   128,    39,    40,   105,    42,    44,    45,   147,   152,
     153,    48,    98,    99,    50,   111,   143,   146,   151,    87,
      88,    41,    59,   108,   114,    87,    52,    52,    52,    88,
      66,    51,    52,    46,    61,    88,    62,    52,    73,   129,
     141,   142,   145,   150,    89,    57,    90,    52,   176,   153,
     144,    54,    86,   131,   159,    90,   173,   175,   106,   179,
      59,    90,    82,    44,    85,    60,    52,    52,   102,    56,
     104,    63,    61,   192,    62,    87,    77,    78,   190,   170,
     171,   172,   174,   177,   178,    88,    52,   126,    69,    70,
     167,    58,    65,   130,    67,   132,   133,    68,    52,   125,
     149,   187,   188,   189,   191,    80,     1,     2,   140,     3,
     107,    90,   194,   154,    69,   108,   103,   156,   139,   112,
     113,   115,     4,   117,   116,   119,   127,   134,     5,   120,
       6,     7,     8,   121,   122,   123,     9,    10,   124,   158,
      11,    12,    13,    14,    15,    16,    38,    52,   135,    88,
       8,   155,   160,   161,     9,   180,    38,   162,    11,   163,
       8,    14,    15,    16,     9,   164,   165,   181,    11,   186,
     168,    14,    15,    16,    76,    76,    76,    76,   182,    76,
     183,   184,   185,   193,    74,   138,   166,   169,   148,    75,
       0,    83,   109,    81,   157,     0,    79
};

static const short int yycheck[] =
{
      12,     4,     1,    28,    69,     4,     5,     6,   123,   124,
     124,    10,    14,    15,    13,    16,   122,   123,   124,     3,
      13,    28,    12,    20,    16,     3,    29,    29,    29,    13,
      42,     0,    29,    28,    24,    13,    26,    29,    50,   104,
     121,   122,   123,   124,    28,    18,    39,    29,   163,   163,
      28,    25,    64,    16,    16,    39,   162,   163,    70,   165,
      12,    39,    61,    62,    63,    17,    29,    29,    67,    27,
      69,    21,    24,    16,    26,     3,    56,    57,   184,   160,
     161,   162,   163,   164,   165,    13,    29,    99,    19,    20,
     155,    33,    26,   105,     8,   107,   108,     8,    29,    98,
      28,   182,   183,   184,   185,    28,     6,     7,   120,     9,
      15,    39,   193,   125,    19,    20,    28,   129,   117,    35,
      38,    28,    22,    17,    28,    28,    16,    16,    28,    38,
      30,    31,    32,     8,     8,     8,    36,    37,     8,    24,
      40,    41,    42,    43,    44,    45,    28,    29,    11,    13,
      32,    28,     8,     8,    36,   167,    28,     8,    40,     8,
      32,    43,    44,    45,    36,     8,     8,    35,    40,   181,
      16,    43,    44,    45,    55,    56,    57,    58,     8,    60,
       8,     8,     8,     8,    53,   115,   154,   158,   123,    55,
      -1,    62,    71,    60,   132,    -1,    58
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     6,     7,     9,    22,    28,    30,    31,    32,    36,
      37,    40,    41,    42,    43,    44,    45,    47,    48,    49,
      54,    65,    66,    67,    68,    69,    70,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    28,    65,
      28,    28,    65,    64,    65,    65,    28,    50,    65,    48,
      65,     0,    29,    87,    25,    71,    27,    18,    33,    12,
      17,    24,    26,    21,    51,    26,    48,     8,     8,    19,
      20,    85,    86,    48,    49,    72,    81,    77,    77,    78,
      28,    67,    65,    64,    83,    65,    48,     3,    13,    28,
      39,    55,    56,    57,    58,    61,    62,    63,    14,    15,
      52,    53,    65,    28,    65,    87,    48,    15,    20,    86,
      88,    16,    35,    38,    16,    28,    28,    17,    60,    28,
      38,     8,     8,     8,     8,    65,    48,    16,     4,    87,
      48,    16,    48,    48,    16,    11,    84,    59,    60,    65,
      48,    56,    56,    57,    28,    56,    57,    58,    62,    28,
      56,    57,    58,    63,    48,    28,    48,    88,    24,    16,
       8,     8,     8,     8,     8,     8,    52,    87,    16,    55,
      56,    56,    56,    57,    56,    57,    58,    56,    56,    57,
      48,    35,     8,     8,     8,     8,    48,    56,    56,    56,
      57,    56,    16,     8,    56
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
#line 426 "parser.y"
    {
            PARSER->stmts = (yyvsp[0].array);
        }
    break;

  case 3:
#line 430 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 4:
#line 433 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
        }
    break;

  case 5:
#line 437 "parser.y"
    {
            (yyval.node) = NULL;
        }
    break;

  case 8:
#line 442 "parser.y"
    {
            COMMAND_CALL_NEW((yyval.node), (yyvsp[-1].name), (yyvsp[0].array), NULL);
        }
    break;

  case 9:
#line 452 "parser.y"
    {
            EXCEPT_FINALLY_NEW((yyval.node), (yyvsp[-5].array), (yyvsp[-4].array), (yyvsp[-2].array), (yyvsp[-1].array));
        }
    break;

  case 10:
#line 455 "parser.y"
    {
            EXCEPT_FINALLY_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-2].array), NULL, (yyvsp[-1].array));
        }
    break;

  case 11:
#line 458 "parser.y"
    {
            FINALLY_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-1].array));
        }
    break;

  case 12:
#line 461 "parser.y"
    {
            WHILE_NEW((yyval.node), (yyvsp[-2].node), (yyvsp[-1].array));
        }
    break;

  case 13:
#line 464 "parser.y"
    {
            BREAK_NEW((yyval.node), NULL);
        }
    break;

  case 14:
#line 467 "parser.y"
    {
            BREAK_NEW((yyval.node), (yyvsp[0].node));
        }
    break;

  case 15:
#line 470 "parser.y"
    {
            NEXT_NEW((yyval.node), NULL);
        }
    break;

  case 16:
#line 473 "parser.y"
    {
            NEXT_NEW((yyval.node), (yyvsp[0].node));
        }
    break;

  case 17:
#line 476 "parser.y"
    {
            RETURN_NEW((yyval.node), NULL);
        }
    break;

  case 18:
#line 479 "parser.y"
    {
            RETURN_NEW((yyval.node), (yyvsp[0].node));
        }
    break;

  case 19:
#line 482 "parser.y"
    {
            IF_NEW((yyval.node), (yyvsp[-3].node), (yyvsp[-2].array), (yyvsp[-1].array));
        }
    break;

  case 20:
#line 485 "parser.y"
    {
            KLASS_NEW((yyval.node), (yyvsp[-3].name), (yyvsp[-2].node), (yyvsp[-1].array));
        }
    break;

  case 21:
#line 488 "parser.y"
    {
            NONLOCAL_NEW((yyval.node), (yyvsp[0].array));
        }
    break;

  case 22:
#line 492 "parser.y"
    {
            (yyval.array) = YogArray_new(ENV);
            YogArray_push(ENV, (yyval.array), ID2VAL((yyvsp[0].name)));
        }
    break;

  case 23:
#line 496 "parser.y"
    {
            YogArray_push(ENV, (yyvsp[-2].array), ID2VAL((yyvsp[0].name)));
            (yyval.array) = (yyvsp[-2].array);
        }
    break;

  case 24:
#line 501 "parser.y"
    {
                (yyval.node) = NULL;
            }
    break;

  case 25:
#line 504 "parser.y"
    {
                (yyval.node) = (yyvsp[0].node);
            }
    break;

  case 27:
#line 509 "parser.y"
    {
            YogNode* node = NULL;
            IF_NEW(node, (yyvsp[-2].node), (yyvsp[-1].array), (yyvsp[0].array));
            OBJ_ARRAY_NEW((yyval.array), node);
        }
    break;

  case 28:
#line 515 "parser.y"
    {
                (yyval.array) = NULL;
            }
    break;

  case 29:
#line 518 "parser.y"
    {
                (yyval.array) = (yyvsp[0].array);
            }
    break;

  case 30:
#line 522 "parser.y"
    {
                FUNC_DEF_NEW((yyval.node), (yyvsp[-5].name), (yyvsp[-3].array), (yyvsp[-1].array));
            }
    break;

  case 31:
#line 526 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-8].array), (yyvsp[-6].array), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 32:
#line 529 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 33:
#line 532 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 34:
#line 535 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 35:
#line 538 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 36:
#line 541 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 37:
#line 544 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 38:
#line 547 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), (yyvsp[0].array), NULL, NULL, NULL);
        }
    break;

  case 39:
#line 550 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), NULL, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 40:
#line 553 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NULL, (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 41:
#line 556 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NULL, (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 42:
#line 559 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NULL, (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 43:
#line 562 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 44:
#line 565 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NULL, NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 45:
#line 568 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NULL, NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 46:
#line 571 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[0].array), NULL, NULL, NULL, NULL);
        }
    break;

  case 47:
#line 574 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-6].array), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 48:
#line 577 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-4].array), (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 49:
#line 580 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-4].array), (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 50:
#line 583 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-2].array), (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 51:
#line 586 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-4].array), NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 52:
#line 589 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-2].array), NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 53:
#line 592 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[-2].array), NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 54:
#line 595 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, (yyvsp[0].array), NULL, NULL, NULL);
        }
    break;

  case 55:
#line 598 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 56:
#line 601 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node), NULL);
        }
    break;

  case 57:
#line 604 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[-2].node), NULL, (yyvsp[0].node));
        }
    break;

  case 58:
#line 607 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, (yyvsp[0].node), NULL, NULL);
        }
    break;

  case 59:
#line 610 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 60:
#line 613 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, NULL, (yyvsp[0].node), NULL);
        }
    break;

  case 61:
#line 616 "parser.y"
    {
            PARAMS_NEW((yyval.array), NULL, NULL, NULL, NULL, (yyvsp[0].node));
        }
    break;

  case 62:
#line 619 "parser.y"
    {
            (yyval.array) = NULL;
        }
    break;

  case 63:
#line 623 "parser.y"
    {
                PARAM_NEW((yyval.node), NODE_KW_PARAM, (yyvsp[0].name), NULL);
            }
    break;

  case 64:
#line 627 "parser.y"
    {
                PARAM_NEW((yyval.node), NODE_VAR_PARAM, (yyvsp[0].name), NULL);
            }
    break;

  case 65:
#line 631 "parser.y"
    {
                    PARAM_NEW((yyval.node), NODE_BLOCK_PARAM, (yyvsp[-1].name), (yyvsp[0].node));
                }
    break;

  case 66:
#line 635 "parser.y"
    {
                        (yyval.node) = NULL;
                    }
    break;

  case 68:
#line 640 "parser.y"
    {
                    (yyval.node) = (yyvsp[0].node);
                }
    break;

  case 69:
#line 644 "parser.y"
    {
                            (yyval.array) = YogArray_new(ENV);
                            PARAM_ARRAY_PUSH((yyval.array), (yyvsp[0].name), NULL);
                        }
    break;

  case 70:
#line 648 "parser.y"
    {
                            PARAM_ARRAY_PUSH((yyvsp[-2].array), (yyvsp[0].name), NULL);
                            (yyval.array) = (yyvsp[-2].array);
                        }
    break;

  case 71:
#line 653 "parser.y"
    {
                            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
                        }
    break;

  case 72:
#line 656 "parser.y"
    {
                            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
                        }
    break;

  case 73:
#line 660 "parser.y"
    {
                        PARAM_NEW((yyval.node), NODE_PARAM, (yyvsp[-1].name), (yyvsp[0].node));
                    }
    break;

  case 74:
#line 664 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 75:
#line 667 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
        }
    break;

  case 77:
#line 673 "parser.y"
    {
                ASSIGN_NEW((yyval.node), (yyvsp[-2].node), (yyvsp[0].node));
            }
    break;

  case 83:
#line 685 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 89:
#line 698 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 91:
#line 703 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 93:
#line 708 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 98:
#line 719 "parser.y"
    {
                    if ((yyvsp[-4].node)->type == NODE_ATTR) {
                        METHOD_CALL_NEW((yyval.node), (yyvsp[-4].node)->u.attr.obj, (yyvsp[-4].node)->u.attr.name, (yyvsp[-2].array), (yyvsp[0].node));
                    }
                    else {
                        FUNC_CALL_NEW((yyval.node), (yyvsp[-4].node), (yyvsp[-2].array), (yyvsp[0].node));
                    }
                }
    break;

  case 99:
#line 727 "parser.y"
    {
                    SUBSCRIPT_NEW((yyval.node), (yyvsp[-3].node), (yyvsp[-1].node));
                }
    break;

  case 100:
#line 730 "parser.y"
    {
                    ATTR_NEW((yyval.node), (yyvsp[-2].node), (yyvsp[0].name));
                }
    break;

  case 101:
#line 734 "parser.y"
    {
            VARIABLE_NEW((yyval.node), (yyvsp[0].name));
        }
    break;

  case 102:
#line 737 "parser.y"
    {
            LITERAL_NEW((yyval.node), (yyvsp[0].val));
        }
    break;

  case 103:
#line 740 "parser.y"
    {
            LITERAL_NEW((yyval.node), (yyvsp[0].val));
        }
    break;

  case 104:
#line 743 "parser.y"
    {
            LITERAL_NEW((yyval.node), (yyvsp[0].val));
        }
    break;

  case 105:
#line 746 "parser.y"
    {
            LITERAL_NEW((yyval.node), YTRUE);
        }
    break;

  case 106:
#line 749 "parser.y"
    {
            LITERAL_NEW((yyval.node), YFALSE);
        }
    break;

  case 107:
#line 752 "parser.y"
    {
            int lineno = PARSER->lineno;
            YogVal val = INT2VAL(lineno);
            LITERAL_NEW((yyval.node), val);
        }
    break;

  case 108:
#line 758 "parser.y"
    {
                (yyval.array) = NULL;
            }
    break;

  case 110:
#line 763 "parser.y"
    {
                    (yyval.node) = NULL;
                }
    break;

  case 111:
#line 766 "parser.y"
    {
                    BLOCK_ARG_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-1].array));
                }
    break;

  case 112:
#line 770 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 113:
#line 773 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-1].array), (yyvsp[0].node));
        }
    break;

  case 114:
#line 777 "parser.y"
    {
            YOG_ASSERT(ENV, (yyvsp[-2].name) != NO_EXC_VAR, "Too many variables.");
            EXCEPT_BODY_NEW((yyval.node), (yyvsp[-4].node), (yyvsp[-2].name), (yyvsp[0].array));
        }
    break;

  case 115:
#line 781 "parser.y"
    {
            EXCEPT_BODY_NEW((yyval.node), (yyvsp[-2].node), NO_EXC_VAR, (yyvsp[0].array));
        }
    break;

  case 116:
#line 784 "parser.y"
    {
            EXCEPT_BODY_NEW((yyval.node), NULL, NO_EXC_VAR, (yyvsp[0].array));
        }
    break;

  case 117:
#line 788 "parser.y"
    {
                PARSER->lineno++;
            }
    break;

  case 118:
#line 792 "parser.y"
    {
                (yyval.array) = NULL;
            }
    break;

  case 119:
#line 795 "parser.y"
    {
                (yyval.array) = (yyvsp[0].array);
            }
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2388 "parser.c"

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


#line 799 "parser.y"

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

void 
YogParser_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogParser* parser = ptr;
#define KEEP(member)    parser->member = (*keeper)(env, parser->member)
    KEEP(lexer);
    KEEP(stmts);
#undef KEEP
}

void 
YogParser_initialize(YogEnv* env, YogParser* parser) 
{
    parser->env = env;
    parser->lexer = NULL;
    parser->stmts = NULL;
    parser->lineno = 1;
}

YogArray* 
YogParser_parse_file(YogEnv* env, YogParser* parser, const char* filename)
{
    YogLexer* lexer = YogLexer_new(env);
    parser->lexer = lexer;
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

