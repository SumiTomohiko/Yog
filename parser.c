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
     tAMPER = 258,
     tAS = 259,
     tBAR = 260,
     tBREAK = 261,
     tCLASS = 262,
     tCOMMA = 263,
     tDEF = 264,
     tDIV = 265,
     tDO = 266,
     tDOT = 267,
     tDOUBLE_STAR = 268,
     tELIF = 269,
     tELSE = 270,
     tEND = 271,
     tEQUAL = 272,
     tEQUAL_TILDA = 273,
     tEXCEPT = 274,
     tFINALLY = 275,
     tGREATER = 276,
     tIF = 277,
     tLBRACE = 278,
     tLBRACKET = 279,
     tLESS = 280,
     tLPAR = 281,
     tLSHIFT = 282,
     tNAME = 283,
     tNEWLINE = 284,
     tNEXT = 285,
     tNONLOCAL = 286,
     tNUMBER = 287,
     tPLUS = 288,
     tRBRACE = 289,
     tRBRACKET = 290,
     tREGEXP = 291,
     tRETURN = 292,
     tRPAR = 293,
     tSTAR = 294,
     tSTRING = 295,
     tTRY = 296,
     tWHILE = 297,
     tFALSE = 298,
     tTRUE = 299,
     t__LINE__ = 300
   };
#endif
/* Tokens.  */
#define tAMPER 258
#define tAS 259
#define tBAR 260
#define tBREAK 261
#define tCLASS 262
#define tCOMMA 263
#define tDEF 264
#define tDIV 265
#define tDO 266
#define tDOT 267
#define tDOUBLE_STAR 268
#define tELIF 269
#define tELSE 270
#define tEND 271
#define tEQUAL 272
#define tEQUAL_TILDA 273
#define tEXCEPT 274
#define tFINALLY 275
#define tGREATER 276
#define tIF 277
#define tLBRACE 278
#define tLBRACKET 279
#define tLESS 280
#define tLPAR 281
#define tLSHIFT 282
#define tNAME 283
#define tNEWLINE 284
#define tNEXT 285
#define tNONLOCAL 286
#define tNUMBER 287
#define tPLUS 288
#define tRBRACE 289
#define tRBRACKET 290
#define tREGEXP 291
#define tRETURN 292
#define tRPAR 293
#define tSTAR 294
#define tSTRING 295
#define tTRY 296
#define tWHILE 297
#define tFALSE 298
#define tTRUE 299
#define t__LINE__ 300




/* Copy the first part of user declarations.  */
#line 1 "parser.y"

/* TODO: replace yacc to lemon.c */
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

#define KEEP(member)    do { \
    node->u.member = YogVal_keep(env, node->u.member, keeper); \
} while (0)
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

static YogVal 
YogNode_new(YogEnv* env, YogParser* parser, YogNodeType type) 
{
    YogNode* node = ALLOC_OBJ(env, YogNode_keep_children, NULL, YogNode);
    node->lineno = parser->lineno;
    node->type = type;

    return PTR2VAL(node);
}

#define NODE_NEW(type)  YogNode_new(ENV, PARSER, type)
#define NODE(v)         PTR_AS(YogNode, (v))

#define LITERAL_NEW(node, val_) do { \
    node = NODE_NEW(NODE_LITERAL); \
    NODE(node)->u.literal.val = val_; \
} while (0)

#define BLOCK_ARG_NEW(node, params_, stmts_) do { \
    node = NODE_NEW(NODE_BLOCK_ARG); \
    NODE(node)->u.blockarg.params = params_; \
    NODE(node)->u.blockarg.stmts = stmts_; \
} while (0)

#define PARAMS_NEW(array, params_without_default, params_with_default, block_param, var_param, kw_param) do { \
    array = YogArray_new(ENV); \
    \
    if (IS_OBJ(params_without_default)) { \
        YogArray_extend(ENV, array, params_without_default); \
    } \
    \
    if (IS_OBJ(params_with_default)) { \
        YogArray_extend(ENV, array, params_with_default); \
    } \
    \
    if (IS_PTR(block_param)) { \
        YogArray_push(ENV, array, block_param); \
    } \
    \
    if (IS_PTR(var_param)) { \
        YogArray_push(ENV, array, var_param); \
    } \
    \
    if (IS_PTR(kw_param)) { \
        YogArray_push(ENV, array, kw_param); \
    } \
} while (0)

#define COMMAND_CALL_NEW(node, name_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_COMMAND_CALL); \
    NODE(node)->u.command_call.name = name_; \
    NODE(node)->u.command_call.args = args_; \
    NODE(node)->u.command_call.blockarg = blockarg_; \
} while (0)

#define OBJ_ARRAY_NEW(array, elem) do { \
    if (IS_PTR(elem)) { \
        array = YogArray_new(ENV); \
        YogArray_push(ENV, array, elem); \
    } \
    else { \
        array = YNIL; \
    } \
} while (0)

#define OBJ_ARRAY_PUSH(result, array, elem) do { \
    if (IS_PTR(elem)) { \
        if (!IS_OBJ(array)) { \
            array = YogArray_new(ENV); \
        } \
        YogArray_push(ENV, array, elem); \
    } \
    result = array; \
} while (0)

#define PARAM_NEW(node, type, id, default__) do { \
    node = NODE_NEW(type); \
    NODE(node)->u.param.name = id; \
    NODE(node)->u.param.default_ = default__; \
} while (0)

#define PARAM_ARRAY_PUSH(array, id, default_) do { \
    YogVal node = YUNDEF; \
    PARAM_NEW(node, NODE_PARAM, id, default_); \
    YogArray_push(ENV, array, node); \
} while (0)

#define FUNC_DEF_NEW(node, name_, params_, stmts_) do { \
    node = NODE_NEW(NODE_FUNC_DEF); \
    NODE(node)->u.funcdef.name = name_; \
    NODE(node)->u.funcdef.params = params_; \
    NODE(node)->u.funcdef.stmts = stmts_; \
} while (0)

#define FUNC_CALL_NEW(node, callee_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_FUNC_CALL); \
    NODE(node)->u.func_call.callee = callee_; \
    NODE(node)->u.func_call.args = args_; \
    NODE(node)->u.func_call.blockarg = blockarg_; \
} while (0)

#define VARIABLE_NEW(node, id_) do { \
    node = NODE_NEW(NODE_VARIABLE); \
    NODE(node)->u.variable.id = id_; \
} while (0)

#define EXCEPT_BODY_NEW(node, type_, var_, stmts_) do { \
    node = NODE_NEW(NODE_EXCEPT_BODY); \
    NODE(node)->u.except_body.type = type_; \
    NODE(node)->u.except_body.var = var_; \
    NODE(node)->u.except_body.stmts = stmts_; \
} while (0)

#define EXCEPT_NEW(node, head_, excepts_, else__) do { \
    node = NODE_NEW(NODE_EXCEPT); \
    NODE(node)->u.except.head = head_; \
    NODE(node)->u.except.excepts = excepts_; \
    NODE(node)->u.except.else_ = else__; \
} while (0)

#define FINALLY_NEW(node, head_, body_) do { \
    node = NODE_NEW(NODE_FINALLY); \
    NODE(node)->u.finally.head = head_; \
    NODE(node)->u.finally.body = body_; \
} while (0)

#define EXCEPT_FINALLY_NEW(node, stmts, excepts, else_, finally) do { \
    EXCEPT_NEW(node, stmts, excepts, else_); \
    \
    if (IS_PTR(finally)) { \
        YogVal array = YUNDEF; \
        OBJ_ARRAY_NEW(array, node); \
        FINALLY_NEW(node, array, finally); \
    } \
} while (0)

#define BREAK_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_BREAK); \
    NODE(node)->u.break_.expr = expr_; \
} while (0)

#define NEXT_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_NEXT); \
    NODE(node)->u.next.expr = expr_; \
} while (0)

#define RETURN_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_RETURN); \
    NODE(node)->u.return_.expr = expr_; \
} while (0)

#define METHOD_CALL_NEW(node, recv_, name_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_METHOD_CALL); \
    NODE(node)->u.method_call.recv = recv_; \
    NODE(node)->u.method_call.name = name_; \
    NODE(node)->u.method_call.args = args_; \
    NODE(node)->u.method_call.blockarg = blockarg_; \
} while (0)

#define METHOD_CALL_NEW1(node, recv, name, arg) do { \
    YogVal args = YogArray_new(ENV); \
    YogArray_push(ENV, args, arg); \
    METHOD_CALL_NEW(node, recv, name, args, YNIL); \
} while (0)

#define IF_NEW(node, test_, stmts_, tail_) do { \
    node = NODE_NEW(NODE_IF); \
    NODE(node)->u.if_.test = test_; \
    NODE(node)->u.if_.stmts = stmts_; \
    NODE(node)->u.if_.tail = tail_; \
} while (0)

#define WHILE_NEW(node, test_, stmts_) do { \
    node = NODE_NEW(NODE_WHILE); \
    NODE(node)->u.while_.test = test_; \
    NODE(node)->u.while_.stmts = stmts_; \
} while (0)

#define KLASS_NEW(node, name_, super_, stmts_) do { \
    node = NODE_NEW(NODE_KLASS); \
    NODE(node)->u.klass.name = name_; \
    NODE(node)->u.klass.super = super_; \
    NODE(node)->u.klass.stmts = stmts_; \
} while (0);

#define ASSIGN_NEW(node, left_, right_) do { \
    node = NODE_NEW(NODE_ASSIGN); \
    NODE(node)->u.assign.left = left_; \
    NODE(node)->u.assign.right = right_; \
} while (0)

#define SUBSCRIPT_NEW(node, prefix_, index_) do { \
    node = NODE_NEW(NODE_SUBSCRIPT); \
    NODE(node)->u.subscript.prefix = prefix_; \
    NODE(node)->u.subscript.index = index_; \
} while (0)

#define ATTR_NEW(node, obj_, name_) do { \
    node = NODE_NEW(NODE_ATTR); \
    NODE(node)->u.attr.obj = obj_; \
    NODE(node)->u.attr.name = name_; \
} while (0)

#define NONLOCAL_NEW(node, names_) do { \
    node = NODE_NEW(NODE_NONLOCAL); \
    NODE(node)->u.nonlocal.names = names_; \
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
#line 326 "parser.y"
typedef union YYSTYPE {
    struct YogVal val;
    ID name;
    unsigned int lineno;
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
#define YYLAST   201

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  46
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  46
/* YYNRULES -- Number of rules. */
#define YYNRULES  124
/* YYNRULES -- Number of states. */
#define YYNSTATES  202

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
      65,    66,    73,    76,    78,    82,    83,    86,    88,    93,
      94,    97,   105,   115,   123,   131,   137,   145,   151,   157,
     161,   169,   175,   181,   185,   191,   195,   199,   201,   209,
     215,   221,   225,   231,   235,   239,   241,   247,   251,   255,
     257,   261,   263,   265,   266,   269,   272,   276,   277,   279,
     282,   284,   288,   290,   294,   297,   299,   303,   305,   309,
     311,   313,   315,   317,   319,   323,   325,   327,   329,   331,
     333,   337,   339,   343,   345,   349,   351,   353,   355,   357,
     358,   365,   370,   374,   376,   378,   380,   382,   384,   386,
     388,   389,   391,   392,   397,   402,   403,   407,   409,   412,
     419,   424,   428,   430,   431
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      47,     0,    -1,    48,    -1,    49,    -1,    48,    90,    49,
      -1,    -1,    55,    -1,    66,    -1,    28,    65,    -1,    41,
      48,    88,    15,    48,    91,    16,    -1,    41,    48,    88,
      91,    16,    -1,    41,    48,    20,    48,    16,    -1,    42,
      66,    48,    16,    -1,     6,    -1,     6,    66,    -1,    30,
      -1,    30,    66,    -1,    37,    -1,    37,    66,    -1,    22,
      66,    48,    53,    16,    -1,    -1,     7,    50,    28,    52,
      48,    16,    -1,    31,    51,    -1,    28,    -1,    51,     8,
      28,    -1,    -1,    21,    66,    -1,    54,    -1,    14,    66,
      48,    53,    -1,    -1,    15,    48,    -1,     9,    28,    26,
      56,    38,    48,    16,    -1,    62,     8,    63,     8,    59,
       8,    58,     8,    57,    -1,    62,     8,    63,     8,    59,
       8,    58,    -1,    62,     8,    63,     8,    59,     8,    57,
      -1,    62,     8,    63,     8,    59,    -1,    62,     8,    63,
       8,    58,     8,    57,    -1,    62,     8,    63,     8,    58,
      -1,    62,     8,    63,     8,    57,    -1,    62,     8,    63,
      -1,    62,     8,    59,     8,    58,     8,    57,    -1,    62,
       8,    59,     8,    58,    -1,    62,     8,    59,     8,    57,
      -1,    62,     8,    59,    -1,    62,     8,    58,     8,    57,
      -1,    62,     8,    58,    -1,    62,     8,    57,    -1,    62,
      -1,    63,     8,    59,     8,    58,     8,    57,    -1,    63,
       8,    59,     8,    58,    -1,    63,     8,    59,     8,    57,
      -1,    63,     8,    59,    -1,    63,     8,    58,     8,    57,
      -1,    63,     8,    58,    -1,    63,     8,    57,    -1,    63,
      -1,    59,     8,    58,     8,    57,    -1,    59,     8,    58,
      -1,    59,     8,    57,    -1,    59,    -1,    58,     8,    57,
      -1,    58,    -1,    57,    -1,    -1,    13,    28,    -1,    39,
      28,    -1,     3,    28,    60,    -1,    -1,    61,    -1,    17,
      66,    -1,    28,    -1,    62,     8,    28,    -1,    64,    -1,
      63,     8,    64,    -1,    28,    61,    -1,    66,    -1,    65,
       8,    66,    -1,    67,    -1,    82,    17,    68,    -1,    68,
      -1,    69,    -1,    70,    -1,    71,    -1,    73,    -1,    73,
      72,    73,    -1,    25,    -1,    74,    -1,    75,    -1,    76,
      -1,    77,    -1,    76,    27,    78,    -1,    78,    -1,    77,
      18,    78,    -1,    79,    -1,    78,    33,    79,    -1,    80,
      -1,    81,    -1,    82,    -1,    84,    -1,    -1,    82,    83,
      26,    85,    38,    86,    -1,    82,    24,    66,    35,    -1,
      82,    12,    28,    -1,    28,    -1,    32,    -1,    36,    -1,
      40,    -1,    44,    -1,    43,    -1,    45,    -1,    -1,    65,
      -1,    -1,    11,    87,    48,    16,    -1,    23,    87,    48,
      34,    -1,    -1,    24,    56,    35,    -1,    89,    -1,    88,
      89,    -1,    19,    66,     4,    28,    90,    48,    -1,    19,
      66,    90,    48,    -1,    19,    90,    48,    -1,    29,    -1,
      -1,    20,    48,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   427,   427,   431,   434,   438,   441,   442,   450,   460,
     463,   466,   469,   472,   475,   478,   481,   484,   487,   490,
     493,   493,   497,   501,   505,   510,   513,   517,   518,   524,
     527,   531,   535,   538,   541,   544,   547,   550,   553,   556,
     559,   562,   565,   568,   571,   574,   577,   580,   583,   586,
     589,   592,   595,   598,   601,   604,   607,   610,   613,   616,
     619,   622,   625,   628,   632,   636,   640,   644,   647,   649,
     653,   657,   662,   665,   669,   673,   676,   680,   682,   685,
     687,   689,   691,   693,   694,   698,   700,   702,   704,   706,
     707,   711,   712,   716,   717,   721,   723,   725,   727,   728,
     728,   737,   740,   744,   747,   750,   753,   756,   759,   762,
     768,   771,   773,   776,   779,   783,   786,   790,   793,   797,
     801,   804,   808,   812,   815
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tAMPER", "tAS", "tBAR", "tBREAK",
  "tCLASS", "tCOMMA", "tDEF", "tDIV", "tDO", "tDOT", "tDOUBLE_STAR",
  "tELIF", "tELSE", "tEND", "tEQUAL", "tEQUAL_TILDA", "tEXCEPT",
  "tFINALLY", "tGREATER", "tIF", "tLBRACE", "tLBRACKET", "tLESS", "tLPAR",
  "tLSHIFT", "tNAME", "tNEWLINE", "tNEXT", "tNONLOCAL", "tNUMBER", "tPLUS",
  "tRBRACE", "tRBRACKET", "tREGEXP", "tRETURN", "tRPAR", "tSTAR",
  "tSTRING", "tTRY", "tWHILE", "tFALSE", "tTRUE", "t__LINE__", "$accept",
  "module", "stmts", "stmt", "@1", "names", "super_opt", "if_tail",
  "else_opt", "func_def", "params", "kw_param", "var_param", "block_param",
  "param_default_opt", "param_default", "params_without_default",
  "params_with_default", "param_with_default", "args", "expr",
  "assign_expr", "logical_or_expr", "logical_and_expr", "not_expr",
  "comparison", "comp_op", "xor_expr", "or_expr", "and_expr", "shift_expr",
  "match_expr", "arith_expr", "term", "factor", "power", "postfix_expr",
  "@2", "atom", "args_opt", "blockarg_opt", "blockarg_params_opt",
  "excepts", "except", "newline", "finally_opt", 0
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
      50,    49,    49,    51,    51,    52,    52,    53,    53,    54,
      54,    55,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
      56,    56,    56,    56,    57,    58,    59,    60,    60,    61,
      62,    62,    63,    63,    64,    65,    65,    66,    67,    67,
      68,    69,    70,    71,    71,    72,    73,    74,    75,    76,
      76,    77,    77,    78,    78,    79,    80,    81,    82,    83,
      82,    82,    82,    84,    84,    84,    84,    84,    84,    84,
      85,    85,    86,    86,    86,    87,    87,    88,    88,    89,
      89,    89,    90,    91,    91
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     3,     0,     1,     1,     2,     7,
       5,     5,     4,     1,     2,     1,     2,     1,     2,     5,
       0,     6,     2,     1,     3,     0,     2,     1,     4,     0,
       2,     7,     9,     7,     7,     5,     7,     5,     5,     3,
       7,     5,     5,     3,     5,     3,     3,     1,     7,     5,
       5,     3,     5,     3,     3,     1,     5,     3,     3,     1,
       3,     1,     1,     0,     2,     2,     3,     0,     1,     2,
       1,     3,     1,     3,     2,     1,     3,     1,     3,     1,
       1,     1,     1,     1,     3,     1,     1,     1,     1,     1,
       3,     1,     3,     1,     3,     1,     1,     1,     1,     0,
       6,     4,     3,     1,     1,     1,     1,     1,     1,     1,
       0,     1,     0,     4,     4,     0,     3,     1,     2,     6,
       4,     3,     1,     0,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       5,    13,    20,     0,     0,   103,    15,     0,   104,   105,
      17,   106,     5,     0,   108,   107,   109,     0,     2,     3,
       6,     7,    77,    79,    80,    81,    82,    83,    86,    87,
      88,    89,    91,    93,    95,    96,    97,    98,   103,    14,
       0,     0,     5,     8,    75,    16,    23,    22,    18,     0,
       5,     1,   122,     5,    85,     0,     0,     0,     0,     0,
       0,     0,     0,    25,    63,    29,     0,     0,     0,     5,
     123,   117,     0,     4,    84,    97,    90,    92,    94,   102,
      78,     0,   110,     0,     5,     0,     0,    70,     0,     0,
      62,    61,    59,    47,    55,    72,     0,     5,     0,    27,
      76,    24,     0,     5,     0,     5,     5,   118,     0,    12,
     101,   111,     0,    26,     0,    67,    64,     0,    74,    65,
       5,     0,     0,     0,     0,     5,    30,    19,     0,     5,
     121,    11,   123,   124,    10,   112,    21,    66,    68,    69,
       0,    60,    58,    57,    71,    46,    45,    43,    39,     0,
      54,    53,    51,    73,    29,     0,   120,     0,   115,   115,
     100,    31,     0,     0,     0,     0,     0,     0,    28,     5,
       9,    63,     5,     5,    56,    44,    42,    41,    38,    37,
      35,    52,    50,    49,   119,     0,     0,     0,     0,     0,
       0,     0,   116,   113,   114,    40,    36,    34,    33,    48,
       0,    32
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,    17,    18,    19,    40,    47,    84,    98,    99,    20,
      89,    90,    91,    92,   137,   118,    93,    94,    95,    43,
      21,    22,    23,    24,    25,    26,    55,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    62,    37,   112,
     160,   172,    70,    71,    53,   108
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -121
static const short int yypact[] =
{
     134,    79,  -121,    -7,    79,    79,    79,    14,  -121,  -121,
      79,  -121,   134,    79,  -121,  -121,  -121,    64,    20,  -121,
    -121,  -121,  -121,  -121,  -121,  -121,  -121,    46,  -121,  -121,
      47,    55,    53,  -121,  -121,  -121,    51,  -121,  -121,  -121,
      67,    63,   134,    88,  -121,  -121,  -121,    98,  -121,    61,
     134,  -121,  -121,   134,  -121,    79,    79,    79,    79,    70,
      79,    79,    86,    95,     9,    10,    79,    92,   102,   134,
      90,  -121,     2,  -121,  -121,     8,    53,    53,  -121,  -121,
    -121,    94,    79,    79,   134,   104,   105,    97,   107,    83,
    -121,   128,   131,   136,   144,  -121,    79,   134,   138,  -121,
    -121,  -121,    11,   134,     7,   134,   134,  -121,   139,  -121,
    -121,    88,   120,  -121,    17,    97,  -121,    79,  -121,  -121,
     134,   146,     4,    13,    16,   134,    20,  -121,   135,   134,
      20,  -121,    27,    20,  -121,    59,  -121,  -121,  -121,  -121,
      49,  -121,  -121,   159,    97,  -121,   160,   161,   164,    97,
    -121,   165,   172,  -121,    10,    20,    20,   166,   157,   157,
    -121,  -121,   146,   146,     4,    16,   146,     4,  -121,   134,
    -121,     9,   134,   134,  -121,  -121,  -121,   175,  -121,   176,
     177,  -121,  -121,   178,    20,   152,    50,    58,   146,   146,
       4,   146,  -121,  -121,  -121,  -121,  -121,  -121,   180,  -121,
     146,  -121
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -121,  -121,   -12,   137,  -121,  -121,  -121,    35,  -121,  -121,
      21,   -63,  -114,  -120,  -121,    76,  -121,    71,  -111,   111,
       1,  -121,   140,  -121,  -121,  -121,  -121,   141,  -121,  -121,
    -121,  -121,   -29,   143,  -121,  -121,    93,  -121,  -121,  -121,
    -121,    36,  -121,   127,   -67,    66
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -100
static const short int yytable[] =
{
      49,   103,    39,   147,   152,    42,    44,    45,   143,   146,
     151,    48,    85,   153,    50,   128,    85,    86,   109,    85,
      59,    41,    86,   131,    96,    97,    86,    76,    77,    86,
      65,    52,    61,   136,   -99,   129,    52,    87,    72,    52,
      52,   144,    46,    88,   149,   180,    52,   106,    88,    52,
     177,   179,    88,   183,   153,    88,    52,   104,   141,   142,
     145,   150,    81,    59,    51,   161,   193,   100,    60,   102,
     158,    54,   114,    57,    56,    61,   198,   -99,    52,    52,
      68,    69,   159,    44,   113,   126,    58,    52,   169,    64,
      52,   130,   194,   132,   133,    63,    66,   125,    79,   174,
     175,   176,   178,   181,   182,   105,    67,    38,   140,    68,
     106,     8,    82,   154,   117,     9,    83,   156,   139,    11,
     101,   120,    14,    15,    16,   195,   196,   197,   199,   110,
      38,    52,   115,   116,     8,   119,   121,   201,     9,   122,
       1,     2,    11,     3,   123,    14,    15,    16,    75,    75,
      75,    75,   124,    75,   127,   134,     4,   184,   135,    86,
     186,   187,     5,   155,     6,     7,     8,   162,   163,   164,
       9,    10,   165,   166,    11,    12,    13,    14,    15,    16,
     167,   171,   170,   188,   189,   190,   191,   192,   200,   168,
      73,   138,   185,   111,   148,   173,    74,   107,   157,     0,
      80,    78
};

static const short int yycheck[] =
{
      12,    68,     1,   123,   124,     4,     5,     6,   122,   123,
     124,    10,     3,   124,    13,     4,     3,    13,    16,     3,
      12,    28,    13,    16,    14,    15,    13,    56,    57,    13,
      42,    29,    24,    16,    26,   102,    29,    28,    50,    29,
      29,    28,    28,    39,    28,   165,    29,    20,    39,    29,
     164,   165,    39,   167,   165,    39,    29,    69,   121,   122,
     123,   124,    61,    12,     0,    16,    16,    66,    17,    68,
      11,    25,    84,    18,    27,    24,   190,    26,    29,    29,
      19,    20,    23,    82,    83,    97,    33,    29,   155,    26,
      29,   103,    34,   105,   106,    28,     8,    96,    28,   162,
     163,   164,   165,   166,   167,    15,     8,    28,   120,    19,
      20,    32,    26,   125,    17,    36,    21,   129,   117,    40,
      28,    38,    43,    44,    45,   188,   189,   190,   191,    35,
      28,    29,    28,    28,    32,    28,     8,   200,    36,     8,
       6,     7,    40,     9,     8,    43,    44,    45,    55,    56,
      57,    58,     8,    60,    16,    16,    22,   169,    38,    13,
     172,   173,    28,    28,    30,    31,    32,     8,     8,     8,
      36,    37,     8,     8,    40,    41,    42,    43,    44,    45,
       8,    24,    16,     8,     8,     8,     8,    35,     8,   154,
      53,   115,   171,    82,   123,   159,    55,    70,   132,    -1,
      60,    58
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     6,     7,     9,    22,    28,    30,    31,    32,    36,
      37,    40,    41,    42,    43,    44,    45,    47,    48,    49,
      55,    66,    67,    68,    69,    70,    71,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    84,    28,    66,
      50,    28,    66,    65,    66,    66,    28,    51,    66,    48,
      66,     0,    29,    90,    25,    72,    27,    18,    33,    12,
      17,    24,    83,    28,    26,    48,     8,     8,    19,    20,
      88,    89,    48,    49,    73,    82,    78,    78,    79,    28,
      68,    66,    26,    21,    52,     3,    13,    28,    39,    56,
      57,    58,    59,    62,    63,    64,    14,    15,    53,    54,
      66,    28,    66,    90,    48,    15,    20,    89,    91,    16,
      35,    65,    85,    66,    48,    28,    28,    17,    61,    28,
      38,     8,     8,     8,     8,    66,    48,    16,     4,    90,
      48,    16,    48,    48,    16,    38,    16,    60,    61,    66,
      48,    57,    57,    58,    28,    57,    58,    59,    63,    28,
      57,    58,    59,    64,    48,    28,    48,    91,    11,    23,
      86,    16,     8,     8,     8,     8,     8,     8,    53,    90,
      16,    24,    87,    87,    57,    57,    57,    58,    57,    58,
      59,    57,    57,    58,    48,    56,    48,    48,     8,     8,
       8,     8,    35,    16,    34,    57,    57,    57,    58,    57,
       8,    57
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
#line 427 "parser.y"
    {
            PARSER->stmts = (yyvsp[0].val);
        }
    break;

  case 3:
#line 431 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.val), (yyvsp[0].val));
        }
    break;

  case 4:
#line 434 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.val), (yyvsp[-2].val), (yyvsp[0].val));
        }
    break;

  case 5:
#line 438 "parser.y"
    {
            (yyval.val) = YNIL;
        }
    break;

  case 7:
#line 442 "parser.y"
    {
            if (PTR_AS(YogNode, (yyvsp[0].val))->type == NODE_VARIABLE) {
                COMMAND_CALL_NEW((yyval.val), PTR_AS(YogNode, (yyvsp[0].val))->u.variable.id, YNIL, YNIL);
            }
            else {
                (yyval.val) = (yyvsp[0].val);
            }
        }
    break;

  case 8:
#line 450 "parser.y"
    {
            COMMAND_CALL_NEW((yyval.val), (yyvsp[-1].name), (yyvsp[0].val), YNIL);
        }
    break;

  case 9:
#line 460 "parser.y"
    {
            EXCEPT_FINALLY_NEW((yyval.val), (yyvsp[-5].val), (yyvsp[-4].val), (yyvsp[-2].val), (yyvsp[-1].val));
        }
    break;

  case 10:
#line 463 "parser.y"
    {
            EXCEPT_FINALLY_NEW((yyval.val), (yyvsp[-3].val), (yyvsp[-2].val), YNIL, (yyvsp[-1].val));
        }
    break;

  case 11:
#line 466 "parser.y"
    {
            FINALLY_NEW((yyval.val), (yyvsp[-3].val), (yyvsp[-1].val));
        }
    break;

  case 12:
#line 469 "parser.y"
    {
            WHILE_NEW((yyval.val), (yyvsp[-2].val), (yyvsp[-1].val));
        }
    break;

  case 13:
#line 472 "parser.y"
    {
            BREAK_NEW((yyval.val), YNIL);
        }
    break;

  case 14:
#line 475 "parser.y"
    {
            BREAK_NEW((yyval.val), (yyvsp[0].val));
        }
    break;

  case 15:
#line 478 "parser.y"
    {
            NEXT_NEW((yyval.val), YNIL);
        }
    break;

  case 16:
#line 481 "parser.y"
    {
            NEXT_NEW((yyval.val), (yyvsp[0].val));
        }
    break;

  case 17:
#line 484 "parser.y"
    {
            RETURN_NEW((yyval.val), YNIL);
        }
    break;

  case 18:
#line 487 "parser.y"
    {
            RETURN_NEW((yyval.val), (yyvsp[0].val));
        }
    break;

  case 19:
#line 490 "parser.y"
    {
            IF_NEW((yyval.val), (yyvsp[-3].val), (yyvsp[-2].val), (yyvsp[-1].val));
        }
    break;

  case 20:
#line 493 "parser.y"
    { (yyval.lineno) = PARSER->lineno; }
    break;

  case 21:
#line 493 "parser.y"
    {
            KLASS_NEW((yyval.val), (yyvsp[-3].name), (yyvsp[-2].val), (yyvsp[-1].val));
            NODE((yyval.val))->lineno = (yyvsp[-4].lineno);
        }
    break;

  case 22:
#line 497 "parser.y"
    {
            NONLOCAL_NEW((yyval.val), (yyvsp[0].val));
        }
    break;

  case 23:
#line 501 "parser.y"
    {
            (yyval.val) = YogArray_new(ENV);
            YogArray_push(ENV, (yyval.val), ID2VAL((yyvsp[0].name)));
        }
    break;

  case 24:
#line 505 "parser.y"
    {
            YogArray_push(ENV, (yyvsp[-2].val), ID2VAL((yyvsp[0].name)));
            (yyval.val) = (yyvsp[-2].val);
        }
    break;

  case 25:
#line 510 "parser.y"
    {
                (yyval.val) = YNIL;
            }
    break;

  case 26:
#line 513 "parser.y"
    {
                (yyval.val) = (yyvsp[0].val);
            }
    break;

  case 28:
#line 518 "parser.y"
    {
            YogVal node = YUNDEF;
            IF_NEW(node, (yyvsp[-2].val), (yyvsp[-1].val), (yyvsp[0].val));
            OBJ_ARRAY_NEW((yyval.val), node);
        }
    break;

  case 29:
#line 524 "parser.y"
    {
                (yyval.val) = YNIL;
            }
    break;

  case 30:
#line 527 "parser.y"
    {
                (yyval.val) = (yyvsp[0].val);
            }
    break;

  case 31:
#line 531 "parser.y"
    {
                FUNC_DEF_NEW((yyval.val), (yyvsp[-5].name), (yyvsp[-3].val), (yyvsp[-1].val));
            }
    break;

  case 32:
#line 535 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-8].val), (yyvsp[-6].val), (yyvsp[-4].val), (yyvsp[-2].val), (yyvsp[0].val));
        }
    break;

  case 33:
#line 538 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-6].val), (yyvsp[-4].val), (yyvsp[-2].val), (yyvsp[0].val), YNIL);
        }
    break;

  case 34:
#line 541 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-6].val), (yyvsp[-4].val), (yyvsp[-2].val), YNIL, (yyvsp[0].val));
        }
    break;

  case 35:
#line 544 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-4].val), (yyvsp[-2].val), (yyvsp[0].val), YNIL, YNIL);
        }
    break;

  case 36:
#line 547 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-6].val), (yyvsp[-4].val), YNIL, (yyvsp[-2].val), (yyvsp[0].val));
        }
    break;

  case 37:
#line 550 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-4].val), (yyvsp[-2].val), YNIL, (yyvsp[0].val), YNIL);
        }
    break;

  case 38:
#line 553 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-4].val), (yyvsp[-2].val), YNIL, YNIL, (yyvsp[0].val));
        }
    break;

  case 39:
#line 556 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-2].val), (yyvsp[0].val), YNIL, YNIL, YNIL);
        }
    break;

  case 40:
#line 559 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-6].val), YNIL, (yyvsp[-4].val), (yyvsp[-2].val), (yyvsp[0].val));
        }
    break;

  case 41:
#line 562 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-4].val), YNIL, (yyvsp[-2].val), (yyvsp[0].val), YNIL);
        }
    break;

  case 42:
#line 565 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-4].val), YNIL, (yyvsp[-2].val), YNIL, (yyvsp[0].val));
        }
    break;

  case 43:
#line 568 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-2].val), YNIL, (yyvsp[0].val), YNIL, YNIL);
        }
    break;

  case 44:
#line 571 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-4].val), YNIL, YNIL, (yyvsp[-2].val), (yyvsp[0].val));
        }
    break;

  case 45:
#line 574 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-2].val), YNIL, YNIL, (yyvsp[0].val), YNIL);
        }
    break;

  case 46:
#line 577 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[-2].val), YNIL, YNIL, YNIL, (yyvsp[0].val));
        }
    break;

  case 47:
#line 580 "parser.y"
    {
            PARAMS_NEW((yyval.val), (yyvsp[0].val), YNIL, YNIL, YNIL, YNIL);
        }
    break;

  case 48:
#line 583 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, (yyvsp[-6].val), (yyvsp[-4].val), (yyvsp[-2].val), (yyvsp[0].val));
        }
    break;

  case 49:
#line 586 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, (yyvsp[-4].val), (yyvsp[-2].val), (yyvsp[0].val), YNIL);
        }
    break;

  case 50:
#line 589 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, (yyvsp[-4].val), (yyvsp[-2].val), YNIL, (yyvsp[0].val));
        }
    break;

  case 51:
#line 592 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, (yyvsp[-2].val), (yyvsp[0].val), YNIL, YNIL);
        }
    break;

  case 52:
#line 595 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, (yyvsp[-4].val), YNIL, (yyvsp[-2].val), (yyvsp[0].val));
        }
    break;

  case 53:
#line 598 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, (yyvsp[-2].val), YNIL, (yyvsp[0].val), YNIL);
        }
    break;

  case 54:
#line 601 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, (yyvsp[-2].val), YNIL, YNIL, (yyvsp[0].val));
        }
    break;

  case 55:
#line 604 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, (yyvsp[0].val), YNIL, YNIL, YNIL);
        }
    break;

  case 56:
#line 607 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, YNIL, (yyvsp[-4].val), (yyvsp[-2].val), (yyvsp[0].val));
        }
    break;

  case 57:
#line 610 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, YNIL, (yyvsp[-2].val), (yyvsp[0].val), YNIL);
        }
    break;

  case 58:
#line 613 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, YNIL, (yyvsp[-2].val), YNIL, (yyvsp[0].val));
        }
    break;

  case 59:
#line 616 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, YNIL, (yyvsp[0].val), YNIL, YNIL);
        }
    break;

  case 60:
#line 619 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, YNIL, YNIL, (yyvsp[-2].val), (yyvsp[0].val));
        }
    break;

  case 61:
#line 622 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, YNIL, YNIL, (yyvsp[0].val), YNIL);
        }
    break;

  case 62:
#line 625 "parser.y"
    {
            PARAMS_NEW((yyval.val), YNIL, YNIL, YNIL, YNIL, (yyvsp[0].val));
        }
    break;

  case 63:
#line 628 "parser.y"
    {
            (yyval.val) = YNIL;
        }
    break;

  case 64:
#line 632 "parser.y"
    {
                PARAM_NEW((yyval.val), NODE_KW_PARAM, (yyvsp[0].name), YNIL);
            }
    break;

  case 65:
#line 636 "parser.y"
    {
                PARAM_NEW((yyval.val), NODE_VAR_PARAM, (yyvsp[0].name), YNIL);
            }
    break;

  case 66:
#line 640 "parser.y"
    {
                    PARAM_NEW((yyval.val), NODE_BLOCK_PARAM, (yyvsp[-1].name), (yyvsp[0].val));
                }
    break;

  case 67:
#line 644 "parser.y"
    {
                        (yyval.val) = YNIL;
                    }
    break;

  case 69:
#line 649 "parser.y"
    {
                    (yyval.val) = (yyvsp[0].val);
                }
    break;

  case 70:
#line 653 "parser.y"
    {
                            (yyval.val) = YogArray_new(ENV);
                            PARAM_ARRAY_PUSH((yyval.val), (yyvsp[0].name), YNIL);
                        }
    break;

  case 71:
#line 657 "parser.y"
    {
                            PARAM_ARRAY_PUSH((yyvsp[-2].val), (yyvsp[0].name), YNIL);
                            (yyval.val) = (yyvsp[-2].val);
                        }
    break;

  case 72:
#line 662 "parser.y"
    {
                            OBJ_ARRAY_NEW((yyval.val), (yyvsp[0].val));
                        }
    break;

  case 73:
#line 665 "parser.y"
    {
                            OBJ_ARRAY_PUSH((yyval.val), (yyvsp[-2].val), (yyvsp[0].val));
                        }
    break;

  case 74:
#line 669 "parser.y"
    {
                        PARAM_NEW((yyval.val), NODE_PARAM, (yyvsp[-1].name), (yyvsp[0].val));
                    }
    break;

  case 75:
#line 673 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.val), (yyvsp[0].val));
        }
    break;

  case 76:
#line 676 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.val), (yyvsp[-2].val), (yyvsp[0].val));
        }
    break;

  case 78:
#line 682 "parser.y"
    {
                ASSIGN_NEW((yyval.val), (yyvsp[-2].val), (yyvsp[0].val));
            }
    break;

  case 84:
#line 694 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.val), (yyvsp[-2].val), (yyvsp[-1].name), (yyvsp[0].val));
            }
    break;

  case 90:
#line 707 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.val), (yyvsp[-2].val), (yyvsp[-1].name), (yyvsp[0].val));
            }
    break;

  case 92:
#line 712 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.val), (yyvsp[-2].val), (yyvsp[-1].name), (yyvsp[0].val));
            }
    break;

  case 94:
#line 717 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.val), (yyvsp[-2].val), (yyvsp[-1].name), (yyvsp[0].val));
            }
    break;

  case 99:
#line 728 "parser.y"
    { (yyval.lineno) = PARSER->lineno; }
    break;

  case 100:
#line 728 "parser.y"
    {
                    if (NODE((yyvsp[-5].val))->type == NODE_ATTR) {
                        METHOD_CALL_NEW((yyval.val), NODE((yyvsp[-5].val))->u.attr.obj, NODE((yyvsp[-5].val))->u.attr.name, (yyvsp[-2].val), (yyvsp[0].val));
                    }
                    else {
                        FUNC_CALL_NEW((yyval.val), (yyvsp[-5].val), (yyvsp[-2].val), (yyvsp[0].val));
                    }
                    NODE((yyval.val))->lineno = (yyvsp[-4].lineno);
                }
    break;

  case 101:
#line 737 "parser.y"
    {
                    SUBSCRIPT_NEW((yyval.val), (yyvsp[-3].val), (yyvsp[-1].val));
                }
    break;

  case 102:
#line 740 "parser.y"
    {
                    ATTR_NEW((yyval.val), (yyvsp[-2].val), (yyvsp[0].name));
                }
    break;

  case 103:
#line 744 "parser.y"
    {
            VARIABLE_NEW((yyval.val), (yyvsp[0].name));
        }
    break;

  case 104:
#line 747 "parser.y"
    {
            LITERAL_NEW((yyval.val), (yyvsp[0].val));
        }
    break;

  case 105:
#line 750 "parser.y"
    {
            LITERAL_NEW((yyval.val), (yyvsp[0].val));
        }
    break;

  case 106:
#line 753 "parser.y"
    {
            LITERAL_NEW((yyval.val), (yyvsp[0].val));
        }
    break;

  case 107:
#line 756 "parser.y"
    {
            LITERAL_NEW((yyval.val), YTRUE);
        }
    break;

  case 108:
#line 759 "parser.y"
    {
            LITERAL_NEW((yyval.val), YFALSE);
        }
    break;

  case 109:
#line 762 "parser.y"
    {
            int lineno = PARSER->lineno;
            YogVal val = INT2VAL(lineno);
            LITERAL_NEW((yyval.val), val);
        }
    break;

  case 110:
#line 768 "parser.y"
    {
                (yyval.val) = YNIL;
            }
    break;

  case 112:
#line 773 "parser.y"
    {
                    (yyval.val) = YNIL;
                }
    break;

  case 113:
#line 776 "parser.y"
    {
                    BLOCK_ARG_NEW((yyval.val), (yyvsp[-2].val), (yyvsp[-1].val));
                }
    break;

  case 114:
#line 779 "parser.y"
    {
                    BLOCK_ARG_NEW((yyval.val), (yyvsp[-2].val), (yyvsp[-1].val));
                }
    break;

  case 115:
#line 783 "parser.y"
    {
                            (yyval.val) = YNIL;
                        }
    break;

  case 116:
#line 786 "parser.y"
    {
                            (yyval.val) = (yyvsp[-1].val);
                        }
    break;

  case 117:
#line 790 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.val), (yyvsp[0].val));
        }
    break;

  case 118:
#line 793 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.val), (yyvsp[-1].val), (yyvsp[0].val));
        }
    break;

  case 119:
#line 797 "parser.y"
    {
            YOG_ASSERT(ENV, (yyvsp[-2].name) != NO_EXC_VAR, "Too many variables.");
            EXCEPT_BODY_NEW((yyval.val), (yyvsp[-4].val), (yyvsp[-2].name), (yyvsp[0].val));
        }
    break;

  case 120:
#line 801 "parser.y"
    {
            EXCEPT_BODY_NEW((yyval.val), (yyvsp[-2].val), NO_EXC_VAR, (yyvsp[0].val));
        }
    break;

  case 121:
#line 804 "parser.y"
    {
            EXCEPT_BODY_NEW((yyval.val), YNIL, NO_EXC_VAR, (yyvsp[0].val));
        }
    break;

  case 122:
#line 808 "parser.y"
    {
                PARSER->lineno++;
            }
    break;

  case 123:
#line 812 "parser.y"
    {
                (yyval.val) = YNIL;
            }
    break;

  case 124:
#line 815 "parser.y"
    {
                (yyval.val) = (yyvsp[0].val);
            }
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2444 "parser.c"

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


#line 819 "parser.y"

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

static void 
YogParser_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogParser* parser = ptr;
#define KEEP(member)    do { \
    parser->member = YogVal_keep(env, parser->member, keeper); \
} while (0)
    KEEP(lexer);
    KEEP(stmts);
#undef KEEP
}

static YogVal 
YogParser_new(YogEnv* env) 
{
    YogParser* parser = ALLOC_OBJ(env, YogParser_keep_children, NULL, YogParser);
    parser->env = env;
    parser->lexer = YUNDEF;
    parser->stmts = YUNDEF;
    parser->lineno = 1;

    return PTR2VAL(parser);
}

YogVal 
YogParser_parse_file(YogEnv* env, const char* filename)
{
    SAVE_LOCALS(env);

    YogVal parser = YUNDEF;
    YogVal lexer = YUNDEF;
    PUSH_LOCALS2(env, parser, lexer);

    parser = YogParser_new(env);
    lexer = YogLexer_new(env);
    PTR_AS(YogParser, parser)->lexer = lexer;
    if (filename != NULL) {
        PTR_AS(YogLexer, lexer)->fp = fopen(filename, "r");
        YogLexer_read_encoding(env, lexer);
    }
    else {
        PTR_AS(YogLexer, lexer)->fp = stdin;
    }

    BOOL old_disable_gc = ENV_VM(env)->disable_gc;
    ENV_VM(env)->disable_gc = TRUE;
    yyparse(PTR_AS(YogParser, parser));
    ENV_VM(env)->disable_gc = old_disable_gc;

    if (filename != NULL) {
        fclose(PTR_AS(YogLexer, lexer)->fp);
    }

    RETURN(env, PTR_AS(YogParser, parser)->stmts);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */

