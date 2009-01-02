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
     NUMBER = 286,
     PLUS = 287,
     RBRACE = 288,
     RBRACKET = 289,
     REGEXP = 290,
     RETURN = 291,
     RPAR = 292,
     STAR = 293,
     STRING = 294,
     TRY = 295,
     WHILE = 296,
     tFALSE = 297,
     tTRUE = 298,
     t__LINE__ = 299
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
#define NUMBER 286
#define PLUS 287
#define RBRACE 288
#define RBRACKET 289
#define REGEXP 290
#define RETURN 291
#define RPAR 292
#define STAR 293
#define STRING 294
#define TRY 295
#define WHILE 296
#define tFALSE 297
#define tTRUE 298
#define t__LINE__ 299




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
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
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
        break;
    case NODE_METHOD_CALL:
        KEEP(method_call.recv);
        KEEP(method_call.args);
        KEEP(method_call.blockarg);
        break;
    case NODE_NEXT:
        KEEP(next.expr);
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
    YogNode* node = ALLOC_OBJ(env, keep_children, NULL, YogNode);
    node->lineno = parser->lineno;
    node->type = type;

    return node;
}

#define NODE_NEW(type)  YogNode_new(ENV, PARSER, type)

#define ASSIGN(var, index, ptr)         var = index
#define ASSIGN_PTR(var, param)          do { \
    if (param != NODE_NONE) { \
        FRAME_LOCAL_PTR(ENV, var, param); \
    } \
    else { \
        var = NULL; \
    } \
} while (0)
#define ASSIGN_OBJ(var, type, param)    do { \
    if (param != NODE_NONE) { \
        FRAME_LOCAL_OBJ(ENV, var, type, param); \
    } \
    else { \
        var = NULL; \
    } \
} while (0)
#define ASSIGN_ARRAY(var, param)        ASSIGN_OBJ(var, YogArray, param)

#define LITERAL_NEW(node, val_)  do { \
    YogNode* nd = NODE_NEW(NODE_LITERAL); \
    nd->u.literal.val = val_; \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define BLOCK_ARG_NEW(node, params_, stmts_) do { \
    YogNode* nd = NODE_NEW(NODE_BLOCK_ARG); \
    \
    YogArray* params = NULL; \
    ASSIGN_ARRAY(params, params_); \
    nd->u.blockarg.params = params; \
    \
    YogArray* stmts = NULL; \
    ASSIGN_ARRAY(stmts, stmts_); \
    nd->u.blockarg.stmts = stmts; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define PARAMS_NEW(array, params_without_default_, params_with_default_, block_param_, var_param_, kw_param_) do { \
    YogArray* ary = YogArray_new(ENV); \
    FRAME_DECL_LOCAL(ENV, ary_idx, OBJ2VAL(ary)); \
    \
    YogArray* params_without_default = NULL; \
    ASSIGN_ARRAY(params_without_default, params_without_default_); \
    if (params_without_default != NULL) { \
        FRAME_LOCAL_OBJ(ENV, ary, YogArray, ary_idx); \
        YogArray_extend(ENV, ary, params_without_default); \
    } \
    \
    YogArray* params_with_default = NULL; \
    ASSIGN_ARRAY(params_with_default, params_with_default_); \
    if (params_with_default != NULL) { \
        FRAME_LOCAL_OBJ(ENV, ary, YogArray, ary_idx); \
        YogArray_extend(ENV, ary, params_with_default); \
    } \
    \
    YogNode* block_param = NULL; \
    ASSIGN_PTR(block_param, block_param_); \
    if (block_param != NULL) { \
        FRAME_LOCAL_OBJ(ENV, ary, YogArray, ary_idx); \
        YogVal val = PTR2VAL(block_param); \
        YogArray_push(ENV, ary, val); \
    } \
    \
    YogNode* var_param = NULL; \
    ASSIGN_PTR(var_param, var_param_); \
    if (var_param != NULL) { \
        FRAME_LOCAL_OBJ(ENV, ary, YogArray, ary_idx); \
        YogVal val = PTR2VAL(var_param); \
        YogArray_push(ENV, ary, val); \
    } \
    \
    YogNode* kw_param = NULL; \
    ASSIGN_PTR(kw_param, kw_param_); \
    if (kw_param != NULL) { \
        FRAME_LOCAL_OBJ(ENV, ary, YogArray, ary_idx); \
        YogVal val = PTR2VAL(kw_param); \
        YogArray_push(ENV, ary, val); \
    } \
    \
    ASSIGN(array, ary_idx, ary); \
} while (0)

#define COMMAND_CALL_NEW(node, name_, args_, blockarg_) do { \
    YogNode* nd = NODE_NEW(NODE_COMMAND_CALL); \
    nd->u.command_call.name = name_; \
    \
    YogArray* args = NULL; \
    ASSIGN_ARRAY(args, args_); \
    nd->u.command_call.args = args; \
    \
    YogNode* blockarg = NULL; \
    ASSIGN_PTR(blockarg, blockarg_); \
    nd->u.command_call.blockarg = blockarg; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define OBJ_ARRAY_NEW(array, elem_) do { \
    YogNode* elem = NULL; \
    ASSIGN_PTR(elem, elem_); \
    \
    if (elem != NULL) { \
        YogArray* ary = YogArray_new(ENV); \
        FRAME_DECL_LOCAL(ENV, ary_idx, OBJ2VAL(ary)); \
        \
        FRAME_LOCAL_OBJ(ENV, ary, YogArray, ary_idx); \
        ASSIGN_PTR(elem, elem_); \
        YogArray_push(ENV, ary, PTR2VAL(elem)); \
        \
        ASSIGN(array, ary_idx, ary); \
    } \
    else { \
        YogArray* ary = NULL; \
        FRAME_DECL_LOCAL(ENV, ary_idx, OBJ2VAL(ary)); \
        \
        ASSIGN(array, ary_idx, ary); \
    } \
} while (0)

#define OBJ_ARRAY_PUSH(result, array_, elem_) do { \
    YogNode* elem = NULL; \
    ASSIGN_PTR(elem, elem_); \
    if (elem != NULL) { \
        YogArray* array = NULL; \
        ASSIGN_ARRAY(array, array_); \
        if (array != NULL) { \
            YogArray_push(ENV, array, PTR2VAL(elem)); \
            \
            result = array_; \
        } \
        else { \
            array = YogArray_new(ENV); \
            FRAME_DECL_LOCAL(ENV, array_idx, OBJ2VAL(array)); \
            \
            FRAME_LOCAL_OBJ(ENV, array, YogArray, array_idx); \
            ASSIGN_PTR(elem, elem_); \
            YogArray_push(ENV, array, PTR2VAL(elem)); \
            \
            result = array_idx; \
        } \
    } \
    else { \
        result = array_; \
    } \
} while (0)

#define PARAM_NEW(node, type, id, default__) do { \
    YogNode* nd = NODE_NEW(type); \
    nd->u.param.name = id; \
    \
    YogNode* default_ = NULL; \
    ASSIGN_PTR(default_, default__); \
    nd->u.param.default_ = default_; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define PARAM_ARRAY_PUSH(array_, id, default_) do { \
    YogNode* node = NULL; \
    FRAME_DECL_LOCAL(ENV, node_idx, PTR2VAL(node)); \
    PARAM_NEW(node_idx, NODE_PARAM, id, default_); \
    \
    YogArray* array = NULL; \
    ASSIGN_ARRAY(array, array_); \
    YogArray_push(ENV, array, PTR2VAL(node)); \
} while (0)

#define FUNC_DEF_NEW(node, name_, params_, stmts_) do { \
    YogNode* nd = NODE_NEW(NODE_FUNC_DEF); \
    nd->u.funcdef.name = name_; \
    \
    YogArray* params = NULL; \
    ASSIGN_ARRAY(params, params_); \
    nd->u.funcdef.params = params; \
    \
    YogArray* stmts = NULL; \
    ASSIGN_ARRAY(stmts, stmts_); \
    nd->u.funcdef.stmts = stmts; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define FUNC_CALL_NEW(node, callee_, args_, blockarg_) do { \
    YogNode* nd = NODE_NEW(NODE_FUNC_CALL); \
    \
    YogNode* callee = NULL; \
    ASSIGN_PTR(callee, callee_); \
    nd->u.func_call.callee = callee; \
    \
    YogArray* args = NULL; \
    ASSIGN_ARRAY(args, args_); \
    nd->u.func_call.args = args; \
    \
    YogNode* blockarg = NULL; \
    ASSIGN_PTR(blockarg, blockarg_); \
    nd->u.func_call.blockarg = blockarg; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define VARIABLE_NEW(node, id_) do { \
    YogNode* nd = NODE_NEW(NODE_VARIABLE); \
    nd->u.variable.id = id_; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define EXCEPT_BODY_NEW(node, type_, var_, stmts_) do { \
    YogNode* nd = NODE_NEW(NODE_EXCEPT_BODY); \
    \
    YogNode* type = NULL; \
    ASSIGN_PTR(type, type_); \
    nd->u.except_body.type = type; \
    \
    nd->u.except_body.var = var_; \
    \
    YogArray* stmts = NULL; \
    ASSIGN_ARRAY(stmts, stmts_); \
    nd->u.except_body.stmts = stmts; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define EXCEPT_NEW(node, head_, excepts_, else__) do { \
    YogNode* EXCEPT_NEW_nd = NODE_NEW(NODE_EXCEPT); \
    \
    YogArray* head = NULL; \
    ASSIGN_ARRAY(head, head_); \
    EXCEPT_NEW_nd->u.except.head = head; \
    \
    YogArray* excepts = NULL; \
    ASSIGN_ARRAY(excepts, excepts_); \
    EXCEPT_NEW_nd->u.except.excepts = excepts; \
    \
    YogArray* else_ = NULL; \
    ASSIGN_ARRAY(else_, else__); \
    EXCEPT_NEW_nd->u.except.else_ = else_; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(EXCEPT_NEW_nd)); \
    ASSIGN(node, nd_idx, EXCEPT_NEW_nd); \
} while (0)

#define FINALLY_NEW(node, head_, body_) do { \
    YogNode* FINALLY_NEW_nd = NODE_NEW(NODE_FINALLY); \
    \
    YogArray* head = NULL; \
    ASSIGN_ARRAY(head, head_); \
    FINALLY_NEW_nd->u.finally.head = head; \
    \
    YogArray* body = NULL; \
    ASSIGN_ARRAY(body, body_); \
    FINALLY_NEW_nd->u.finally.body = body; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(FINALLY_NEW_nd)); \
    ASSIGN(node, nd_idx, FINALLY_NEW_nd); \
} while (0)

#define EXCEPT_FINALLY_NEW(node, stmts, excepts, else_, finally_) do { \
    NODE_TYPE nd = NODE_NONE; \
    EXCEPT_NEW(nd, stmts, excepts, else_); \
    \
    YogArray* finally = NULL; \
    ASSIGN_ARRAY(finally, finally_); \
    if (finally != NULL) { \
        ARRAY_TYPE array = NODE_NONE; \
        OBJ_ARRAY_NEW(array, nd); \
        FINALLY_NEW(nd, array, finally_); \
    } \
    \
    YogNode* pnode = NULL; \
    FRAME_LOCAL_PTR(ENV, pnode, nd); \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(pnode)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define BREAK_NEW(node, expr_) do { \
    YogNode* nd = NODE_NEW(NODE_BREAK); \
    \
    YogNode* expr = NULL; \
    ASSIGN_PTR(expr, expr_); \
    nd->u.break_.expr = expr; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define NEXT_NEW(node, expr_) do { \
    YogNode* nd = NODE_NEW(NODE_NEXT); \
    \
    YogNode* expr = NULL; \
    ASSIGN_PTR(expr, expr_); \
    nd->u.next.expr = expr; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define RETURN_NEW(node, expr_) do { \
    YogNode* nd = NODE_NEW(NODE_RETURN); \
    \
    YogNode* expr = NULL; \
    ASSIGN_PTR(expr, expr_); \
    nd->u.return_.expr = expr; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define METHOD_CALL_NEW(node, recv_, name_, args_, blockarg_) do { \
    YogNode* nd = NODE_NEW(NODE_METHOD_CALL); \
    \
    YogNode* recv = NULL; \
    ASSIGN_PTR(recv, recv_); \
    nd->u.method_call.recv = recv; \
    \
    nd->u.method_call.name = name_; \
    \
    YogArray* args = NULL; \
    ASSIGN_ARRAY(args, args_); \
    nd->u.method_call.args = args; \
    \
    YogNode* blockarg = NULL; \
    ASSIGN_PTR(blockarg, blockarg_); \
    nd->u.method_call.blockarg = blockarg; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define METHOD_CALL_NEW1(node, recv, name, arg_) do { \
    YogArray* args = YogArray_new(ENV); \
    FRAME_DECL_LOCAL(ENV, args_idx, OBJ2VAL(args)); \
    \
    ASSIGN_ARRAY(args, args_idx); \
    YogNode* arg = NULL; \
    ASSIGN_PTR(arg, arg_); \
    YogArray_push(ENV, args, PTR2VAL(arg)); \
    \
    METHOD_CALL_NEW(node, recv, name, args_idx, NODE_NONE); \
} while (0)

#define IF_NEW(node, test_, stmts_, tail_) do { \
    YogNode* nd = NODE_NEW(NODE_IF); \
    \
    YogNode* test = NULL; \
    ASSIGN_PTR(test, test_); \
    nd->u.if_.test = test; \
    \
    YogArray* stmts = NULL; \
    ASSIGN_ARRAY(stmts, stmts_); \
    nd->u.if_.stmts = stmts; \
    \
    YogArray* tail = NULL; \
    ASSIGN_ARRAY(tail, tail_); \
    nd->u.if_.tail = tail; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define WHILE_NEW(node, test_, stmts_) do { \
    YogNode* nd = NODE_NEW(NODE_WHILE); \
    \
    YogNode* test = NULL; \
    ASSIGN_PTR(test, test_); \
    nd->u.while_.test = test; \
    \
    YogArray* stmts = NULL; \
    ASSIGN_ARRAY(stmts, stmts_); \
    nd->u.while_.stmts = stmts; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define KLASS_NEW(node, name_, super_, stmts_) do { \
    YogNode* nd = NODE_NEW(NODE_KLASS); \
    nd->u.klass.name = name_; \
    \
    YogNode* super = NULL; \
    ASSIGN_PTR(super, super_); \
    nd->u.klass.super = super; \
    \
    YogArray* stmts = NULL; \
    ASSIGN_ARRAY(stmts, stmts_); \
    nd->u.klass.stmts = stmts; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0);

#define ASSIGN_NEW(node, left_, right_) do { \
    YogNode* nd = NODE_NEW(NODE_ASSIGN); \
    \
    YogNode* left = NULL; \
    ASSIGN_PTR(left, left_); \
    nd->u.assign.left = left; \
    \
    YogNode* right = NULL; \
    ASSIGN_PTR(right, right_); \
    nd->u.assign.right = right; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define SUBSCRIPT_NEW(node, prefix_, index_) do { \
    YogNode* nd = NODE_NEW(NODE_SUBSCRIPT); \
    \
    YogNode* prefix = NULL; \
    ASSIGN_PTR(prefix, prefix_); \
    nd->u.subscript.prefix = prefix; \
    \
    YogNode* index = NULL; \
    ASSIGN_PTR(index, index_); \
    nd->u.subscript.index = index; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
} while (0)

#define ATTR_NEW(node, obj_, name_) do { \
    YogNode* nd = NODE_NEW(NODE_ATTR); \
    \
    YogNode* obj = NULL; \
    ASSIGN_PTR(obj, obj_); \
    nd->u.attr.obj = obj; \
    \
    nd->u.attr.name = name_; \
    \
    FRAME_DECL_LOCAL(ENV, nd_idx, PTR2VAL(nd)); \
    ASSIGN(node, nd_idx, nd); \
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
#line 567 "parser.y"
typedef union YYSTYPE {
    ARRAY_TYPE array;
    NODE_TYPE node;
    struct YogVal val;
    ID name;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 746 "parser.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 758 "parser.c"

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
#define YYFINAL  48
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   193

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  45
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  42
/* YYNRULES -- Number of rules. */
#define YYNRULES  116
/* YYNRULES -- Number of states. */
#define YYNSTATES  190

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   299

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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     7,    11,    12,    14,    16,    19,
      27,    33,    39,    44,    46,    49,    51,    54,    56,    59,
      65,    71,    72,    75,    77,    82,    83,    86,    94,   104,
     112,   120,   126,   134,   140,   146,   150,   158,   164,   170,
     174,   180,   184,   188,   190,   198,   204,   210,   214,   220,
     224,   228,   230,   236,   240,   244,   246,   250,   252,   254,
     255,   258,   261,   265,   266,   268,   271,   273,   277,   279,
     283,   286,   288,   292,   294,   298,   300,   302,   304,   306,
     308,   312,   314,   316,   318,   320,   322,   326,   328,   332,
     334,   338,   340,   342,   344,   346,   352,   357,   361,   363,
     365,   367,   369,   371,   373,   375,   376,   378,   379,   386,
     388,   391,   398,   403,   407,   409,   410
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      46,     0,    -1,    47,    -1,    48,    -1,    47,    85,    48,
      -1,    -1,    52,    -1,    63,    -1,    28,    62,    -1,    40,
      47,    83,    15,    47,    86,    16,    -1,    40,    47,    83,
      86,    16,    -1,    40,    47,    20,    47,    16,    -1,    41,
      63,    47,    16,    -1,     6,    -1,     6,    63,    -1,    30,
      -1,    30,    63,    -1,    36,    -1,    36,    63,    -1,    22,
      63,    47,    50,    16,    -1,     7,    28,    49,    47,    16,
      -1,    -1,    21,    63,    -1,    51,    -1,    14,    63,    47,
      50,    -1,    -1,    15,    47,    -1,     9,    28,    26,    53,
      37,    47,    16,    -1,    59,     8,    60,     8,    56,     8,
      55,     8,    54,    -1,    59,     8,    60,     8,    56,     8,
      55,    -1,    59,     8,    60,     8,    56,     8,    54,    -1,
      59,     8,    60,     8,    56,    -1,    59,     8,    60,     8,
      55,     8,    54,    -1,    59,     8,    60,     8,    55,    -1,
      59,     8,    60,     8,    54,    -1,    59,     8,    60,    -1,
      59,     8,    56,     8,    55,     8,    54,    -1,    59,     8,
      56,     8,    55,    -1,    59,     8,    56,     8,    54,    -1,
      59,     8,    56,    -1,    59,     8,    55,     8,    54,    -1,
      59,     8,    55,    -1,    59,     8,    54,    -1,    59,    -1,
      60,     8,    56,     8,    55,     8,    54,    -1,    60,     8,
      56,     8,    55,    -1,    60,     8,    56,     8,    54,    -1,
      60,     8,    56,    -1,    60,     8,    55,     8,    54,    -1,
      60,     8,    55,    -1,    60,     8,    54,    -1,    60,    -1,
      56,     8,    55,     8,    54,    -1,    56,     8,    55,    -1,
      56,     8,    54,    -1,    56,    -1,    55,     8,    54,    -1,
      55,    -1,    54,    -1,    -1,    13,    28,    -1,    38,    28,
      -1,     3,    28,    57,    -1,    -1,    58,    -1,    17,    63,
      -1,    28,    -1,    59,     8,    28,    -1,    61,    -1,    60,
       8,    61,    -1,    28,    58,    -1,    63,    -1,    62,     8,
      63,    -1,    64,    -1,    79,    17,    65,    -1,    65,    -1,
      66,    -1,    67,    -1,    68,    -1,    70,    -1,    70,    69,
      70,    -1,    25,    -1,    71,    -1,    72,    -1,    73,    -1,
      74,    -1,    73,    27,    75,    -1,    75,    -1,    74,    18,
      75,    -1,    76,    -1,    75,    32,    76,    -1,    77,    -1,
      78,    -1,    79,    -1,    80,    -1,    79,    26,    81,    37,
      82,    -1,    79,    24,    63,    34,    -1,    79,    12,    28,
      -1,    28,    -1,    31,    -1,    35,    -1,    39,    -1,    43,
      -1,    42,    -1,    44,    -1,    -1,    62,    -1,    -1,    11,
      24,    53,    34,    47,    16,    -1,    84,    -1,    83,    84,
      -1,    19,    63,     4,    28,    85,    47,    -1,    19,    63,
      85,    47,    -1,    19,    85,    47,    -1,    29,    -1,    -1,
      20,    47,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   666,   666,   672,   675,   679,   682,   683,   684,   694,
     697,   700,   703,   706,   709,   712,   715,   718,   721,   724,
     727,   731,   734,   738,   739,   745,   748,   752,   756,   759,
     762,   765,   768,   771,   774,   777,   780,   783,   786,   789,
     792,   795,   798,   801,   804,   807,   810,   813,   816,   819,
     822,   825,   828,   831,   834,   837,   840,   843,   846,   849,
     853,   857,   861,   865,   868,   870,   874,   880,   885,   888,
     892,   896,   899,   903,   905,   908,   910,   912,   914,   916,
     917,   921,   923,   925,   927,   929,   930,   934,   935,   939,
     940,   944,   946,   948,   950,   951,   962,   965,   969,   972,
     975,   978,   981,   984,   987,   993,   996,   998,  1001,  1005,
    1008,  1012,  1016,  1019,  1023,  1027,  1030
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
  "NUMBER", "PLUS", "RBRACE", "RBRACKET", "REGEXP", "RETURN", "RPAR",
  "STAR", "STRING", "TRY", "WHILE", "tFALSE", "tTRUE", "t__LINE__",
  "$accept", "module", "stmts", "stmt", "super_opt", "if_tail", "else_opt",
  "func_def", "params", "kw_param", "var_param", "block_param",
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
     295,   296,   297,   298,   299
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    45,    46,    47,    47,    48,    48,    48,    48,    48,
      48,    48,    48,    48,    48,    48,    48,    48,    48,    48,
      48,    49,    49,    50,    50,    51,    51,    52,    53,    53,
      53,    53,    53,    53,    53,    53,    53,    53,    53,    53,
      53,    53,    53,    53,    53,    53,    53,    53,    53,    53,
      53,    53,    53,    53,    53,    53,    53,    53,    53,    53,
      54,    55,    56,    57,    57,    58,    59,    59,    60,    60,
      61,    62,    62,    63,    64,    64,    65,    66,    67,    68,
      68,    69,    70,    71,    72,    73,    73,    74,    74,    75,
      75,    76,    77,    78,    79,    79,    79,    79,    80,    80,
      80,    80,    80,    80,    80,    81,    81,    82,    82,    83,
      83,    84,    84,    84,    85,    86,    86
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     3,     0,     1,     1,     2,     7,
       5,     5,     4,     1,     2,     1,     2,     1,     2,     5,
       5,     0,     2,     1,     4,     0,     2,     7,     9,     7,
       7,     5,     7,     5,     5,     3,     7,     5,     5,     3,
       5,     3,     3,     1,     7,     5,     5,     3,     5,     3,
       3,     1,     5,     3,     3,     1,     3,     1,     1,     0,
       2,     2,     3,     0,     1,     2,     1,     3,     1,     3,
       2,     1,     3,     1,     3,     1,     1,     1,     1,     1,
       3,     1,     1,     1,     1,     1,     3,     1,     3,     1,
       3,     1,     1,     1,     1,     5,     4,     3,     1,     1,
       1,     1,     1,     1,     1,     0,     1,     0,     6,     1,
       2,     6,     4,     3,     1,     0,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       5,    13,     0,     0,     0,    98,    15,    99,   100,    17,
     101,     5,     0,   103,   102,   104,     0,     2,     3,     6,
       7,    73,    75,    76,    77,    78,    79,    82,    83,    84,
      85,    87,    89,    91,    92,    93,    94,    98,    14,    21,
       0,     5,     8,    71,    16,    18,     0,     5,     1,   114,
       5,    81,     0,     0,     0,     0,     0,     0,     0,   105,
       0,     5,    59,    25,     0,     0,     5,   115,   109,     0,
       4,    80,    93,    86,    88,    90,    97,    74,     0,   106,
       0,    22,     0,     0,     0,    66,     0,     0,    58,    57,
      55,    43,    51,    68,     0,     5,     0,    23,    72,     0,
       5,     0,     5,     5,   110,     0,    12,    96,   107,    20,
      63,    60,     0,    70,    61,     5,     0,     0,     0,     0,
       5,    26,    19,     0,     5,   113,    11,   115,   116,    10,
       0,    95,    62,    64,    65,     0,    56,    54,    53,    67,
      42,    41,    39,    35,     0,    50,    49,    47,    69,    25,
       0,   112,     0,    59,    27,     0,     0,     0,     0,     0,
       0,    24,     5,     9,     0,    52,    40,    38,    37,    34,
      33,    31,    48,    46,    45,   111,     5,     0,     0,     0,
       0,     0,    36,    32,    30,    29,    44,   108,     0,    28
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,    16,    17,    18,    61,    96,    97,    19,    87,    88,
      89,    90,   132,   113,    91,    92,    93,    42,    20,    21,
      22,    23,    24,    25,    52,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    80,   131,    67,    68,
      50,   105
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -112
static const short int yypact[] =
{
      89,   124,   -26,    -2,   124,   124,   124,  -112,  -112,   124,
    -112,    89,   124,  -112,  -112,  -112,    33,    34,  -112,  -112,
    -112,  -112,  -112,  -112,  -112,  -112,    43,  -112,  -112,    42,
      53,    73,  -112,  -112,  -112,    -1,  -112,  -112,  -112,    64,
      81,    89,   107,  -112,  -112,  -112,    54,    89,  -112,  -112,
      89,  -112,   124,   124,   124,   124,    86,   124,   124,   124,
     124,    89,    14,     6,   124,   106,    89,     9,  -112,    16,
    -112,  -112,    22,    73,    73,  -112,  -112,  -112,    82,   107,
      85,  -112,    41,    90,    95,   109,    99,   101,  -112,   128,
     131,   132,   134,  -112,   124,    89,   127,  -112,  -112,    15,
      89,    74,    89,    89,  -112,   130,  -112,  -112,   133,  -112,
     109,  -112,   124,  -112,  -112,    89,   140,     5,    48,    59,
      89,    34,  -112,   119,    89,    34,  -112,     2,    34,  -112,
     136,  -112,  -112,  -112,  -112,    77,  -112,  -112,   146,   109,
    -112,   148,   149,   150,   109,  -112,   153,   154,  -112,     6,
      34,    34,   157,    14,  -112,   140,   140,     5,    59,   140,
       5,  -112,    89,  -112,   141,  -112,  -112,  -112,   156,  -112,
     168,   169,  -112,  -112,   170,    34,    89,   140,   140,     5,
     140,    92,  -112,  -112,  -112,   171,  -112,  -112,   140,  -112
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -112,  -112,   -11,   135,  -112,    31,  -112,  -112,    28,   -78,
    -104,  -111,  -112,    72,  -112,    65,  -109,   125,     0,  -112,
     129,  -112,  -112,  -112,  -112,   137,  -112,  -112,  -112,  -112,
      13,   138,  -112,  -112,   117,  -112,  -112,  -112,  -112,   120,
     -62,    61
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      46,    38,    39,   100,    41,    43,    44,   142,   147,    45,
     148,    56,    47,   138,   141,   146,    57,    83,    84,   123,
      94,    95,   103,    58,   102,    59,    40,    84,    65,   103,
      63,    49,   106,    48,    56,    49,    69,   124,   136,   137,
     140,   145,    85,    86,    49,    49,    58,   171,    59,   148,
      82,    83,    86,   168,   170,   101,   174,   109,    78,    43,
      81,    84,    83,    49,    98,    99,    73,    74,    51,    53,
      49,    54,    84,    65,    66,   185,   139,   165,   166,   167,
     169,   172,   173,    49,   121,    60,    86,   144,   162,   125,
     126,   127,   128,   154,   120,     1,     2,    86,     3,   182,
     183,   184,   186,    49,   135,    55,    49,    62,   187,   149,
     189,     4,   134,   151,    76,    64,   107,     5,   110,     6,
       7,    49,   108,   111,     8,     9,   112,   114,    10,    11,
      12,    13,    14,    15,    37,    49,   116,     7,   115,   117,
     118,     8,   119,   122,   130,    10,   129,   150,    13,    14,
      15,   175,    37,    84,   155,     7,   156,   157,   158,     8,
     153,   159,   160,    10,   177,   181,    13,    14,    15,    72,
      72,    72,    72,   163,    72,   176,   178,   179,   180,   188,
     161,   164,   133,   143,    79,    70,    77,   104,   152,    71,
       0,     0,     0,    75
};

static const short int yycheck[] =
{
      11,     1,    28,    65,     4,     5,     6,   118,   119,     9,
     119,    12,    12,   117,   118,   119,    17,     3,    13,     4,
      14,    15,    20,    24,    15,    26,    28,    13,    19,    20,
      41,    29,    16,     0,    12,    29,    47,    99,   116,   117,
     118,   119,    28,    38,    29,    29,    24,   158,    26,   158,
      61,     3,    38,   157,   158,    66,   160,    16,    58,    59,
      60,    13,     3,    29,    64,    65,    53,    54,    25,    27,
      29,    18,    13,    19,    20,   179,    28,   155,   156,   157,
     158,   159,   160,    29,    95,    21,    38,    28,   150,   100,
      16,   102,   103,    16,    94,     6,     7,    38,     9,   177,
     178,   179,   180,    29,   115,    32,    29,    26,    16,   120,
     188,    22,   112,   124,    28,     8,    34,    28,    28,    30,
      31,    29,    37,    28,    35,    36,    17,    28,    39,    40,
      41,    42,    43,    44,    28,    29,     8,    31,    37,     8,
       8,    35,     8,    16,    11,    39,    16,    28,    42,    43,
      44,   162,    28,    13,     8,    31,     8,     8,     8,    35,
      24,     8,     8,    39,     8,   176,    42,    43,    44,    52,
      53,    54,    55,    16,    57,    34,     8,     8,     8,     8,
     149,   153,   110,   118,    59,    50,    57,    67,   127,    52,
      -1,    -1,    -1,    55
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     6,     7,     9,    22,    28,    30,    31,    35,    36,
      39,    40,    41,    42,    43,    44,    46,    47,    48,    52,
      63,    64,    65,    66,    67,    68,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    28,    63,    28,
      28,    63,    62,    63,    63,    63,    47,    63,     0,    29,
      85,    25,    69,    27,    18,    32,    12,    17,    24,    26,
      21,    49,    26,    47,     8,    19,    20,    83,    84,    47,
      48,    70,    79,    75,    75,    76,    28,    65,    63,    62,
      81,    63,    47,     3,    13,    28,    38,    53,    54,    55,
      56,    59,    60,    61,    14,    15,    50,    51,    63,    63,
      85,    47,    15,    20,    84,    86,    16,    34,    37,    16,
      28,    28,    17,    58,    28,    37,     8,     8,     8,     8,
      63,    47,    16,     4,    85,    47,    16,    47,    47,    16,
      11,    82,    57,    58,    63,    47,    54,    54,    55,    28,
      54,    55,    56,    60,    28,    54,    55,    56,    61,    47,
      28,    47,    86,    24,    16,     8,     8,     8,     8,     8,
       8,    50,    85,    16,    53,    54,    54,    54,    55,    54,
      55,    56,    54,    54,    55,    47,    34,     8,     8,     8,
       8,    47,    54,    54,    54,    55,    54,    16,     8,    54
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
#line 666 "parser.y"
    {
            YogArray* stmts = NULL;
            ASSIGN_ARRAY(stmts, (yyvsp[0].array));
            PARSER->stmts = stmts;
        }
    break;

  case 3:
#line 672 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 4:
#line 675 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
        }
    break;

  case 5:
#line 679 "parser.y"
    {
            (yyval.node) = NODE_NONE;
        }
    break;

  case 8:
#line 684 "parser.y"
    {
            COMMAND_CALL_NEW((yyval.node), (yyvsp[-1].name), (yyvsp[0].array), NODE_NONE);
        }
    break;

  case 9:
#line 694 "parser.y"
    {
            EXCEPT_FINALLY_NEW((yyval.node), (yyvsp[-5].array), (yyvsp[-4].array), (yyvsp[-2].array), (yyvsp[-1].array));
        }
    break;

  case 10:
#line 697 "parser.y"
    {
            EXCEPT_FINALLY_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-2].array), NODE_NONE, (yyvsp[-1].array));
        }
    break;

  case 11:
#line 700 "parser.y"
    {
            FINALLY_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-1].array));
        }
    break;

  case 12:
#line 703 "parser.y"
    {
            WHILE_NEW((yyval.node), (yyvsp[-2].node), (yyvsp[-1].array));
        }
    break;

  case 13:
#line 706 "parser.y"
    {
            BREAK_NEW((yyval.node), NODE_NONE);
        }
    break;

  case 14:
#line 709 "parser.y"
    {
            BREAK_NEW((yyval.node), (yyvsp[0].node));
        }
    break;

  case 15:
#line 712 "parser.y"
    {
            NEXT_NEW((yyval.node), NODE_NONE);
        }
    break;

  case 16:
#line 715 "parser.y"
    {
            NEXT_NEW((yyval.node), (yyvsp[0].node));
        }
    break;

  case 17:
#line 718 "parser.y"
    {
            RETURN_NEW((yyval.node), NODE_NONE);
        }
    break;

  case 18:
#line 721 "parser.y"
    {
            RETURN_NEW((yyval.node), (yyvsp[0].node));
        }
    break;

  case 19:
#line 724 "parser.y"
    {
            IF_NEW((yyval.node), (yyvsp[-3].node), (yyvsp[-2].array), (yyvsp[-1].array));
        }
    break;

  case 20:
#line 727 "parser.y"
    {
            KLASS_NEW((yyval.node), (yyvsp[-3].name), (yyvsp[-2].node), (yyvsp[-1].array));
        }
    break;

  case 21:
#line 731 "parser.y"
    {
                (yyval.node) = NODE_NONE;
            }
    break;

  case 22:
#line 734 "parser.y"
    {
                (yyval.node) = (yyvsp[0].node);
            }
    break;

  case 24:
#line 739 "parser.y"
    {
            NODE_TYPE node;
            IF_NEW(node, (yyvsp[-2].node), (yyvsp[-1].array), (yyvsp[0].array));
            OBJ_ARRAY_NEW((yyval.array), node);
        }
    break;

  case 25:
#line 745 "parser.y"
    {
                (yyval.array) = NODE_NONE;
            }
    break;

  case 26:
#line 748 "parser.y"
    {
                (yyval.array) = (yyvsp[0].array);
            }
    break;

  case 27:
#line 752 "parser.y"
    {
                FUNC_DEF_NEW((yyval.node), (yyvsp[-5].name), (yyvsp[-3].array), (yyvsp[-1].array));
            }
    break;

  case 28:
#line 756 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-8].array), (yyvsp[-6].array), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 29:
#line 759 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), (yyvsp[-2].node), (yyvsp[0].node), NODE_NONE);
        }
    break;

  case 30:
#line 762 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), (yyvsp[-2].node), NODE_NONE, (yyvsp[0].node));
        }
    break;

  case 31:
#line 765 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), (yyvsp[0].node), NODE_NONE, NODE_NONE);
        }
    break;

  case 32:
#line 768 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), (yyvsp[-4].array), NODE_NONE, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 33:
#line 771 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), NODE_NONE, (yyvsp[0].node), NODE_NONE);
        }
    break;

  case 34:
#line 774 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), (yyvsp[-2].array), NODE_NONE, NODE_NONE, (yyvsp[0].node));
        }
    break;

  case 35:
#line 777 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), (yyvsp[0].array), NODE_NONE, NODE_NONE, NODE_NONE);
        }
    break;

  case 36:
#line 780 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-6].array), NODE_NONE, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 37:
#line 783 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NODE_NONE, (yyvsp[-2].node), (yyvsp[0].node), NODE_NONE);
        }
    break;

  case 38:
#line 786 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NODE_NONE, (yyvsp[-2].node), NODE_NONE, (yyvsp[0].node));
        }
    break;

  case 39:
#line 789 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NODE_NONE, (yyvsp[0].node), NODE_NONE, NODE_NONE);
        }
    break;

  case 40:
#line 792 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-4].array), NODE_NONE, NODE_NONE, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 41:
#line 795 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NODE_NONE, NODE_NONE, (yyvsp[0].node), NODE_NONE);
        }
    break;

  case 42:
#line 798 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[-2].array), NODE_NONE, NODE_NONE, NODE_NONE, (yyvsp[0].node));
        }
    break;

  case 43:
#line 801 "parser.y"
    {
            PARAMS_NEW((yyval.array), (yyvsp[0].array), NODE_NONE, NODE_NONE, NODE_NONE, NODE_NONE);
        }
    break;

  case 44:
#line 804 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, (yyvsp[-6].array), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 45:
#line 807 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, (yyvsp[-4].array), (yyvsp[-2].node), (yyvsp[0].node), NODE_NONE);
        }
    break;

  case 46:
#line 810 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, (yyvsp[-4].array), (yyvsp[-2].node), NODE_NONE, (yyvsp[0].node));
        }
    break;

  case 47:
#line 813 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, (yyvsp[-2].array), (yyvsp[0].node), NODE_NONE, NODE_NONE);
        }
    break;

  case 48:
#line 816 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, (yyvsp[-4].array), NODE_NONE, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 49:
#line 819 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, (yyvsp[-2].array), NODE_NONE, (yyvsp[0].node), NODE_NONE);
        }
    break;

  case 50:
#line 822 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, (yyvsp[-2].array), NODE_NONE, NODE_NONE, (yyvsp[0].node));
        }
    break;

  case 51:
#line 825 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, (yyvsp[0].array), NODE_NONE, NODE_NONE, NODE_NONE);
        }
    break;

  case 52:
#line 828 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, NODE_NONE, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 53:
#line 831 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, NODE_NONE, (yyvsp[-2].node), (yyvsp[0].node), NODE_NONE);
        }
    break;

  case 54:
#line 834 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, NODE_NONE, (yyvsp[-2].node), NODE_NONE, (yyvsp[0].node));
        }
    break;

  case 55:
#line 837 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, NODE_NONE, (yyvsp[0].node), NODE_NONE, NODE_NONE);
        }
    break;

  case 56:
#line 840 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, NODE_NONE, NODE_NONE, (yyvsp[-2].node), (yyvsp[0].node));
        }
    break;

  case 57:
#line 843 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, NODE_NONE, NODE_NONE, (yyvsp[0].node), NODE_NONE);
        }
    break;

  case 58:
#line 846 "parser.y"
    {
            PARAMS_NEW((yyval.array), NODE_NONE, NODE_NONE, NODE_NONE, NODE_NONE, (yyvsp[0].node));
        }
    break;

  case 59:
#line 849 "parser.y"
    {
            (yyval.array) = NODE_NONE;
        }
    break;

  case 60:
#line 853 "parser.y"
    {
                PARAM_NEW((yyval.node), NODE_KW_PARAM, (yyvsp[0].name), NODE_NONE);
            }
    break;

  case 61:
#line 857 "parser.y"
    {
                PARAM_NEW((yyval.node), NODE_VAR_PARAM, (yyvsp[0].name), NODE_NONE);
            }
    break;

  case 62:
#line 861 "parser.y"
    {
                    PARAM_NEW((yyval.node), NODE_BLOCK_PARAM, (yyvsp[-1].name), (yyvsp[0].node));
                }
    break;

  case 63:
#line 865 "parser.y"
    {
                        (yyval.node) = NODE_NONE;
                    }
    break;

  case 65:
#line 870 "parser.y"
    {
                    (yyval.node) = (yyvsp[0].node);
                }
    break;

  case 66:
#line 874 "parser.y"
    {
                            YogArray* array = YogArray_new(ENV);
                            FRAME_DECL_LOCAL(ENV, array_idx, OBJ2VAL(array));
                            (yyval.array) = array_idx;
                            PARAM_ARRAY_PUSH((yyval.array), (yyvsp[0].name), NODE_NONE);
                        }
    break;

  case 67:
#line 880 "parser.y"
    {
                            PARAM_ARRAY_PUSH((yyvsp[-2].array), (yyvsp[0].name), NODE_NONE);
                            (yyval.array) = (yyvsp[-2].array);
                        }
    break;

  case 68:
#line 885 "parser.y"
    {
                            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
                        }
    break;

  case 69:
#line 888 "parser.y"
    {
                            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
                        }
    break;

  case 70:
#line 892 "parser.y"
    {
                        PARAM_NEW((yyval.node), NODE_PARAM, (yyvsp[-1].name), (yyvsp[0].node));
                    }
    break;

  case 71:
#line 896 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 72:
#line 899 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-2].array), (yyvsp[0].node));
        }
    break;

  case 74:
#line 905 "parser.y"
    {
                ASSIGN_NEW((yyval.node), (yyvsp[-2].node), (yyvsp[0].node));
            }
    break;

  case 80:
#line 917 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 86:
#line 930 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 88:
#line 935 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 90:
#line 940 "parser.y"
    {
                METHOD_CALL_NEW1((yyval.node), (yyvsp[-2].node), (yyvsp[-1].name), (yyvsp[0].node));
            }
    break;

  case 95:
#line 951 "parser.y"
    {
                    YogNode* postfix_expr_node = NULL;
                    ASSIGN_PTR(postfix_expr_node, (yyvsp[-4].node));
                    if (postfix_expr_node->type == NODE_ATTR) {
                        FRAME_DECL_LOCAL(ENV, obj_idx, OBJ2VAL(postfix_expr_node->u.attr.obj));
                        METHOD_CALL_NEW((yyval.node), obj_idx, postfix_expr_node->u.attr.name, (yyvsp[-2].array), (yyvsp[0].node));
                    }
                    else {
                        FUNC_CALL_NEW((yyval.node), (yyvsp[-4].node), (yyvsp[-2].array), (yyvsp[0].node));
                    }
                }
    break;

  case 96:
#line 962 "parser.y"
    {
                    SUBSCRIPT_NEW((yyval.node), (yyvsp[-3].node), (yyvsp[-1].node));
                }
    break;

  case 97:
#line 965 "parser.y"
    {
                    ATTR_NEW((yyval.node), (yyvsp[-2].node), (yyvsp[0].name));
                }
    break;

  case 98:
#line 969 "parser.y"
    {
            VARIABLE_NEW((yyval.node), (yyvsp[0].name));
        }
    break;

  case 99:
#line 972 "parser.y"
    {
            LITERAL_NEW((yyval.node), (yyvsp[0].val));
        }
    break;

  case 100:
#line 975 "parser.y"
    {
            LITERAL_NEW((yyval.node), (yyvsp[0].val));
        }
    break;

  case 101:
#line 978 "parser.y"
    {
            LITERAL_NEW((yyval.node), (yyvsp[0].val));
        }
    break;

  case 102:
#line 981 "parser.y"
    {
            LITERAL_NEW((yyval.node), YTRUE);
        }
    break;

  case 103:
#line 984 "parser.y"
    {
            LITERAL_NEW((yyval.node), YFALSE);
        }
    break;

  case 104:
#line 987 "parser.y"
    {
            int lineno = PARSER->lineno;
            YogVal val = INT2VAL(lineno);
            LITERAL_NEW((yyval.node), val);
        }
    break;

  case 105:
#line 993 "parser.y"
    {
                (yyval.array) = NODE_NONE;
            }
    break;

  case 107:
#line 998 "parser.y"
    {
                    (yyval.node) = NODE_NONE;
                }
    break;

  case 108:
#line 1001 "parser.y"
    {
                    BLOCK_ARG_NEW((yyval.node), (yyvsp[-3].array), (yyvsp[-1].array));
                }
    break;

  case 109:
#line 1005 "parser.y"
    {
            OBJ_ARRAY_NEW((yyval.array), (yyvsp[0].node));
        }
    break;

  case 110:
#line 1008 "parser.y"
    {
            OBJ_ARRAY_PUSH((yyval.array), (yyvsp[-1].array), (yyvsp[0].node));
        }
    break;

  case 111:
#line 1012 "parser.y"
    {
            YOG_ASSERT(ENV, (yyvsp[-2].name) != NO_EXC_VAR, "Too many variables.");
            EXCEPT_BODY_NEW((yyval.node), (yyvsp[-4].node), (yyvsp[-2].name), (yyvsp[0].array));
        }
    break;

  case 112:
#line 1016 "parser.y"
    {
            EXCEPT_BODY_NEW((yyval.node), (yyvsp[-2].node), NO_EXC_VAR, (yyvsp[0].array));
        }
    break;

  case 113:
#line 1019 "parser.y"
    {
            EXCEPT_BODY_NEW((yyval.node), NODE_NONE, NO_EXC_VAR, (yyvsp[0].array));
        }
    break;

  case 114:
#line 1023 "parser.y"
    {
                PARSER->lineno++;
            }
    break;

  case 115:
#line 1027 "parser.y"
    {
                (yyval.array) = NODE_NONE;
            }
    break;

  case 116:
#line 1030 "parser.y"
    {
                (yyval.array) = (yyvsp[0].array);
            }
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2607 "parser.c"

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


#line 1034 "parser.y"

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
        fclose(parser->lexer->fp);
    }

    return parser->stmts;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */

