/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>

#if 0
#   define DEBUG(x)     x
#else
#   define DEBUG(x)
#endif
#line 9 "parser.y"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "yog/array.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/parser.h"
#include "yog/thread.h"
#include "yog/yog.h"

typedef struct ParserState ParserState;

static void Parse(struct YogEnv*, YogVal, int, YogVal, YogVal*);
static YogVal LemonParser_new(YogEnv*);
static void ParseTrace(FILE*, char*);

static void 
YogNode_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogNode* node = ptr;

#define KEEP(member)    YogGC_keep(env, &node->u.member, keeper, heap)
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
    case NODE_IMPORT:
        KEEP(import.names);
        break;
    case NODE_KLASS:
        KEEP(klass.super);
        KEEP(klass.stmts);
        break;
    case NODE_LITERAL:
        KEEP(literal.val);
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
YogNode_new(YogEnv* env, YogNodeType type, unsigned int lineno) 
{
    YogVal node = ALLOC_OBJ(env, YogNode_keep_children, NULL, YogNode);
    PTR_AS(YogNode, node)->lineno = lineno;
    PTR_AS(YogNode, node)->type = type;

    return node;
}

#define NODE_NEW(type, lineno)  YogNode_new(env, (type), (lineno))
#define NODE(v)                 PTR_AS(YogNode, (v))

static YogVal 
Literal_new(YogEnv* env, unsigned int lineno, YogVal val) 
{
    SAVE_ARG(env, val);

    YogVal node = YogNode_new(env, NODE_LITERAL, lineno);
    MODIFY(env, NODE(node)->u.literal.val, val);

    RETURN(env, node);
}

static YogVal 
BlockArg_new(YogEnv* env, unsigned int lineno, YogVal params, YogVal stmts) 
{
    SAVE_ARGS2(env, params, stmts);

    YogVal node = YogNode_new(env, NODE_BLOCK_ARG, lineno);
    MODIFY(env, NODE(node)->u.blockarg.params, params);
    MODIFY(env, NODE(node)->u.blockarg.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Params_new(YogEnv* env, YogVal params_without_default, YogVal params_with_default, YogVal block_param, YogVal var_param, YogVal kw_param) 
{
    SAVE_ARGS5(env, params_without_default, params_with_default, block_param, var_param, kw_param);

    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

    array = YogArray_new(env);
    if (IS_PTR(params_without_default)) {
        YogArray_extend(env, array, params_without_default);
    }
    if (IS_PTR(params_with_default)) {
        YogArray_extend(env, array, params_with_default);
    }
    if (IS_PTR(block_param)) {
        YogArray_push(env, array, block_param);
    }
    if (IS_PTR(var_param)) {
        YogArray_push(env, array, var_param);
    }
    if (IS_PTR(kw_param)) {
        YogArray_push(env, array, kw_param);
    }

    RETURN(env, array);
}

static YogVal 
CommandCall_new(YogEnv* env, unsigned int lineno, ID name, YogVal args, YogVal blockarg) 
{
    SAVE_ARGS2(env, args, blockarg);

    YogVal node = YogNode_new(env, NODE_COMMAND_CALL, lineno);
    NODE(node)->u.command_call.name = name;
    MODIFY(env, NODE(node)->u.command_call.args, args);
    MODIFY(env, NODE(node)->u.command_call.blockarg, blockarg);

    RETURN(env, node);
}

static YogVal 
Array_new(YogEnv* env, YogVal elem) 
{
    SAVE_ARG(env, elem);

    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

    if (IS_PTR(elem) || IS_SYMBOL(elem)) {
        array = YogArray_new(env);
        YogArray_push(env, array, elem);
    }
    else {
        array = YNIL;
    }

    RETURN(env, array);
}

static YogVal 
Array_push(YogEnv* env, YogVal array, YogVal elem) 
{
    SAVE_ARGS2(env, array, elem);

    if (IS_PTR(elem) || IS_SYMBOL(elem)) {
        if (!IS_PTR(array)) {
            array = YogArray_new(env);
        }
        YogArray_push(env, array, elem);
    }

    RETURN(env, array);
}

static YogVal 
Param_new(YogEnv* env, YogNodeType type, unsigned int lineno, ID id, YogVal default_) 
{
    SAVE_ARG(env, default_);

    YogVal node = YogNode_new(env, type, lineno);
    NODE(node)->u.param.name = id;
    MODIFY(env, NODE(node)->u.param.default_, default_);

    RETURN(env, node);
}

static void 
ParamArray_push(YogEnv* env, YogVal array, unsigned int lineno, ID id, YogVal default_) 
{
    SAVE_ARGS2(env, array, default_);

    YogVal node = Param_new(env, NODE_PARAM, lineno, id, default_);
    YogArray_push(env, array, node);

    RETURN_VOID(env);
}

static YogVal 
FuncDef_new(YogEnv* env, unsigned int lineno, ID name, YogVal params, YogVal stmts) 
{
    SAVE_ARGS2(env, params, stmts);

    YogVal node = YogNode_new(env, NODE_FUNC_DEF, lineno);
    NODE(node)->u.funcdef.name = name;
    MODIFY(env, NODE(node)->u.funcdef.params, params);
    MODIFY(env, NODE(node)->u.funcdef.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
FuncCall_new(YogEnv* env, unsigned int lineno, YogVal callee, YogVal args, YogVal blockarg) 
{
    SAVE_ARGS3(env, callee, args, blockarg);

    YogVal node = NODE_NEW(NODE_FUNC_CALL, lineno);
    MODIFY(env, NODE(node)->u.func_call.callee, callee);
    MODIFY(env, NODE(node)->u.func_call.args, args);
    MODIFY(env, NODE(node)->u.func_call.blockarg, blockarg);

    RETURN(env, node);
}

static YogVal 
Variable_new(YogEnv* env, unsigned int lineno, ID id) 
{
    YogVal node = NODE_NEW(NODE_VARIABLE, lineno);
    NODE(node)->u.variable.id = id;

    return node;
}

static YogVal 
ExceptBody_new(YogEnv* env, unsigned int lineno, YogVal type, ID var, YogVal stmts) 
{
    SAVE_ARGS2(env, type, stmts);

    YogVal node = NODE_NEW(NODE_EXCEPT_BODY, lineno);
    MODIFY(env, NODE(node)->u.except_body.type, type);
    NODE(node)->u.except_body.var = var;
    MODIFY(env, NODE(node)->u.except_body.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Except_new(YogEnv* env, unsigned int lineno, YogVal head, YogVal excepts, YogVal else_) 
{
    SAVE_ARGS3(env, head, excepts, else_);

    YogVal node = NODE_NEW(NODE_EXCEPT, lineno);
    MODIFY(env, NODE(node)->u.except.head, head);
    MODIFY(env, NODE(node)->u.except.excepts, excepts);
    MODIFY(env, NODE(node)->u.except.else_, else_);

    RETURN(env, node);
}

static YogVal 
Finally_new(YogEnv* env, unsigned int lineno, YogVal head, YogVal body) 
{
    SAVE_ARGS2(env, head, body);

    YogVal node = NODE_NEW(NODE_FINALLY, lineno);
    MODIFY(env, NODE(node)->u.finally.head, head);
    MODIFY(env, NODE(node)->u.finally.body, body);

    RETURN(env, node);
}

static YogVal 
ExceptFinally_new(YogEnv* env, unsigned int lineno, YogVal stmts, YogVal excepts, YogVal else_, YogVal finally) 
{
    SAVE_ARGS4(env, stmts, excepts, else_, finally);

    YogVal except = YUNDEF;
    PUSH_LOCAL(env, except);

    except = Except_new(env, lineno, stmts, excepts, else_);

    YogVal node;
    if (IS_PTR(finally)) {
        YogVal array = Array_new(env, except);
        node = Finally_new(env, lineno, array, finally);
    }
    else {
        node = except;
    }

    RETURN(env, node);
}

static YogVal 
Break_new(YogEnv* env, unsigned int lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = YogNode_new(env, NODE_BREAK, lineno);
    MODIFY(env, NODE(node)->u.break_.expr, expr);

    RETURN(env, node);
}

static YogVal 
Next_new(YogEnv* env, unsigned int lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = NODE_NEW(NODE_NEXT, lineno);
    MODIFY(env, NODE(node)->u.next.expr, expr);

    RETURN(env, node);
}

static YogVal 
Return_new(YogEnv* env, unsigned int lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = NODE_NEW(NODE_RETURN, lineno);
    MODIFY(env, NODE(node)->u.return_.expr, expr);

    RETURN(env, node);
}

static YogVal 
MethodCall_new(YogEnv* env, unsigned int lineno, YogVal recv, ID name, YogVal args, YogVal blockarg) 
{
    SAVE_ARGS3(env, recv, args, blockarg);

    YogVal node = NODE_NEW(NODE_METHOD_CALL, lineno);
    MODIFY(env, NODE(node)->u.method_call.recv, recv);
    NODE(node)->u.method_call.name = name;
    MODIFY(env, NODE(node)->u.method_call.args, args);
    MODIFY(env, NODE(node)->u.method_call.blockarg, blockarg);

    RETURN(env, node);
}

#if 0
#define METHOD_CALL_NEW(node, lineno, recv_, name_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_METHOD_CALL, lineno); \
    NODE(node)->u.method_call.recv = recv_; \
    NODE(node)->u.method_call.name = name_; \
    NODE(node)->u.method_call.args = args_; \
    NODE(node)->u.method_call.blockarg = blockarg_; \
} while (0)
#endif

static YogVal 
MethodCall1_new(YogEnv* env, unsigned int lineno, YogVal recv, ID name, YogVal arg) 
{
    SAVE_ARGS2(env, recv, arg);

    YogVal args = YUNDEF;
    PUSH_LOCAL(env, args);
   
    args = YogArray_new(env);
    YogArray_push(env, args, arg);
   
    YogVal node = MethodCall_new(env, lineno, recv, name, args, YNIL);

    RETURN(env, node);
}

static YogVal 
If_new(YogEnv* env, unsigned int lineno, YogVal test, YogVal stmts, YogVal tail)
{
    SAVE_ARGS3(env, test, stmts, tail);

    YogVal node = YogNode_new(env, NODE_IF, lineno);
    MODIFY(env, NODE(node)->u.if_.test, test);
    MODIFY(env, NODE(node)->u.if_.stmts, stmts);
    MODIFY(env, NODE(node)->u.if_.tail, tail);

    RETURN(env, node);
}

static YogVal 
While_new(YogEnv* env, unsigned int lineno, YogVal test, YogVal stmts) 
{
    SAVE_ARGS2(env, test, stmts);

    YogVal node = YogNode_new(env, NODE_WHILE, lineno);
    MODIFY(env, NODE(node)->u.while_.test, test);
    MODIFY(env, NODE(node)->u.while_.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Klass_new(YogEnv* env, unsigned int lineno, ID name, YogVal super, YogVal stmts)
{
    SAVE_ARGS2(env, super, stmts);

    YogVal node = YogNode_new(env, NODE_KLASS, lineno);
    NODE(node)->u.klass.name = name;
    MODIFY(env, NODE(node)->u.klass.super, super);
    MODIFY(env, NODE(node)->u.klass.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Assign_new(YogEnv* env, unsigned int lineno, YogVal left, YogVal right) 
{
    SAVE_ARGS2(env, left, right);

    YogVal node = NODE_NEW(NODE_ASSIGN, lineno);
    MODIFY(env, NODE(node)->u.assign.left, left);
    MODIFY(env, NODE(node)->u.assign.right, right);

    RETURN(env, node);
}

static YogVal 
Subscript_new(YogEnv* env, unsigned int lineno, YogVal prefix, YogVal index) 
{
    SAVE_ARGS2(env, prefix, index);

    YogVal node = YogNode_new(env, NODE_SUBSCRIPT, lineno);
    MODIFY(env, NODE(node)->u.subscript.prefix, prefix);
    MODIFY(env, NODE(node)->u.subscript.index, index);

    RETURN(env, node);
}

static YogVal 
Attr_new(YogEnv* env, unsigned int lineno, YogVal obj, ID name) 
{
    SAVE_ARG(env, obj);

    YogVal node = YogNode_new(env, NODE_ATTR, lineno);
    MODIFY(env, NODE(node)->u.attr.obj, obj);
    NODE(node)->u.attr.name = name;

    RETURN(env, node);
}

static YogVal 
Nonlocal_new(YogEnv* env, unsigned int lineno, YogVal names) 
{
    SAVE_ARG(env, names);

    YogVal node = YogNode_new(env, NODE_NONLOCAL, lineno);
    MODIFY(env, NODE(node)->u.nonlocal.names, names);

    RETURN(env, node);
}

static YogVal
Import_new(YogEnv* env, unsigned int lineno, YogVal names)
{
    SAVE_ARG(env, names);

    YogVal node = YogNode_new(env, NODE_IMPORT, lineno);
    MODIFY(env, NODE(node)->u.import.names, names);

    RETURN(env, node);
}

FILE*
open(const char* filename)
{
    if (filename == NULL) {
        return stdin;
    }

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }

    return fp;
}

YogVal 
YogParser_parse_file(YogEnv* env, const char* filename, BOOL debug)
{
    SAVE_LOCALS(env);

    YogVal lexer = YUNDEF;
    YogVal ast = YUNDEF;
    YogVal lemon_parser = YUNDEF;
    YogVal token = YUNDEF;
    PUSH_LOCALS4(env, lexer, ast, lemon_parser, token);

    FILE* fp = open(filename);
    if (fp == NULL) {
        RETURN(env, YNIL);
    }

    lexer = YogLexer_new(env);
    PTR_AS(YogLexer, lexer)->fp = fp;
    if (filename != NULL) {
        YogLexer_read_encoding(env, lexer);
    }

    lemon_parser = LemonParser_new(env);
    if (debug) {
        ParseTrace(stdout, "parser> ");
    }
    while (YogLexer_next_token(env, lexer, &token)) {
        unsigned int type = PTR_AS(YogToken, token)->type;
        Parse(env, lemon_parser, type, token, &ast);
    }
    Parse(env, lemon_parser, 0, YNIL, &ast);

    if (filename != NULL) {
        fclose(fp);
    }

    RETURN(env, ast);
}

static YogVal
id2array(YogEnv* env, ID id)
{
    return Array_new(env, ID2VAL(id));
}

static YogVal
id_token2array(YogEnv* env, YogVal token)
{
    return id2array(env, PTR_AS(YogToken, token)->u.id);
}

static YogVal
Array_push_token_id(YogEnv* env, YogVal array, YogVal token)
{
    SAVE_ARGS2(env, array, token);
    ID id = PTR_AS(YogToken, token)->u.id;
    YogVal retval = Array_push(env, array, ID2VAL(id));

    RETURN(env, retval);
}

#define TOKEN_LINENO(token)     PTR_AS(YogToken, (token))->lineno
#define NODE_LINENO(node)       PTR_AS(YogNode, (node))->lineno
#line 608 "parser.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#if !defined(INTERFACE)
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    ParseTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 89
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy111;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 205
#define YYNRULE 125
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   331,   71,  155,  129,  130,  100,  101,  112,  116,  118,
 /*    10 */   194,   89,  164,  186,  105,  106,  163,  163,   72,  155,
 /*    20 */   129,  130,  169,  132,   69,  144,  134,  135,  136,   52,
 /*    30 */    16,  138,  139,   80,   83,   85,  145,  141,  142,  146,
 /*    40 */   132,   69,  144,  134,  135,  136,   52,   70,  138,  139,
 /*    50 */    80,   83,   85,  145,  141,  142,  146,  119,   81,  145,
 /*    60 */   141,  142,  146,   70,   40,  155,  129,  130,  137,  201,
 /*    70 */   138,  139,   80,   83,   85,  145,  141,  142,  146,  104,
 /*    80 */   173,   51,  155,  129,  130,   12,  132,   69,  144,  134,
 /*    90 */   135,  136,   52,  197,  138,  139,   80,   83,   85,  145,
 /*   100 */   141,  142,  146,  132,   69,  144,  134,  135,  136,   52,
 /*   110 */    70,  138,  139,   80,   83,   85,  145,  141,  142,  146,
 /*   120 */    67,   84,  145,  141,  142,  146,  120,   88,  155,  129,
 /*   130 */   130,  125,  101,  112,  116,  118,  194,  110,  183,  186,
 /*   140 */   105,  106,  108,   16,   73,  155,  129,  130,    4,  132,
 /*   150 */    69,  144,  134,  135,  136,   52,   27,  138,  139,   80,
 /*   160 */    83,   85,  145,  141,  142,  146,  132,   69,  144,  134,
 /*   170 */   135,  136,   52,   70,  138,  139,   80,   83,   85,  145,
 /*   180 */   141,  142,  146,   62,   87,  140,  141,  142,  146,  154,
 /*   190 */    42,  155,  129,  130,  102,  109,  111,  185,   16,    3,
 /*   200 */   186,    4,  156,  105,  106,  108,  204,   43,  155,  129,
 /*   210 */   130,   31,  132,   69,  144,  134,  135,  136,   52,   33,
 /*   220 */   138,  139,   80,   83,   85,  145,  141,  142,  146,  132,
 /*   230 */    69,  144,  134,  135,  136,   52,   19,  138,  139,   80,
 /*   240 */    83,   85,  145,  141,  142,  146,   59,  103,  107,  176,
 /*   250 */   114,  188,  180,   90,  155,  129,  130,  113,  115,  190,
 /*   260 */   117,  192,  180,    1,   94,   97,  105,  106,  108,   16,
 /*   270 */    74,  155,  129,  130,   14,  132,   69,  144,  134,  135,
 /*   280 */   136,   52,   28,  138,  139,   80,   83,   85,  145,  141,
 /*   290 */   142,  146,  132,   69,  144,  134,  135,  136,   52,    9,
 /*   300 */   138,  139,   80,   83,   85,  145,  141,  142,  146,   19,
 /*   310 */    16,  127,  167,    8,   92,   17,   75,  155,  129,  130,
 /*   320 */    30,  177,  178,   53,   68,   29,   16,   26,  205,   16,
 /*   330 */   158,   95,  171,  122,  155,  129,  130,   41,  132,   69,
 /*   340 */   144,  134,  135,  136,   52,  105,  138,  139,   80,   83,
 /*   350 */    85,  145,  141,  142,  146,  132,   69,  144,  134,  135,
 /*   360 */   136,   52,  175,  138,  139,   80,   83,   85,  145,  141,
 /*   370 */   142,  146,  127,   16,  199,  182,   17,  165,  181,  123,
 /*   380 */   155,  129,  130,   16,   16,  184,   29,  170,  198,  187,
 /*   390 */   189,   15,  191,  193,   20,   34,  124,  155,  129,  130,
 /*   400 */    36,  132,   69,  144,  134,  135,  136,   52,   35,  138,
 /*   410 */   139,   80,   83,   85,  145,  141,  142,  146,  132,   69,
 /*   420 */   144,  134,  135,  136,   52,   50,  138,  139,   80,   83,
 /*   430 */    85,  145,  141,  142,  146,  157,   16,  162,   54,   93,
 /*   440 */   166,   56,   77,  155,  129,  130,  168,   96,   99,   32,
 /*   450 */   172,  174,   37,   49,   10,   38,   44,   57,   58,   78,
 /*   460 */   155,  129,  130,  196,  132,   69,  144,  134,  135,  136,
 /*   470 */    52,   45,  138,  139,   80,   83,   85,  145,  141,  142,
 /*   480 */   146,  132,   69,  144,  134,  135,  136,   52,  147,  138,
 /*   490 */   139,   80,   83,   85,  145,  141,  142,  146,   60,   61,
 /*   500 */    39,  121,   46,   63,   64,   47,  128,  129,  130,   65,
 /*   510 */    66,   11,  200,  202,  203,  332,  332,  332,  148,  149,
 /*   520 */   150,  151,  152,  153,  332,  143,   82,  132,   69,  144,
 /*   530 */   134,  135,  136,   52,  332,  138,  139,   80,   83,   85,
 /*   540 */   145,  141,  142,  146,  132,   69,  144,  134,  135,  136,
 /*   550 */    52,  332,  138,  139,   80,   83,   85,  145,  141,  142,
 /*   560 */   146,   86,  332,  332,  332,  332,  332,  332,  332,  332,
 /*   570 */   332,  332,  332,  332,  143,   79,   70,  133,  134,  135,
 /*   580 */   136,   52,  332,  138,  139,   80,   83,   85,  145,  141,
 /*   590 */   142,  146,  332,  132,   69,  144,  134,  135,  136,   52,
 /*   600 */   332,  138,  139,   80,   83,   85,  145,  141,  142,  146,
 /*   610 */    18,    2,  332,  332,  332,   21,   22,   23,   24,   25,
 /*   620 */    91,   55,   48,   13,  147,  332,  332,   98,  332,  332,
 /*   630 */   332,  332,  332,  332,  332,  332,  332,  332,  332,  332,
 /*   640 */   148,  149,  150,  151,  152,  153,   76,  332,  332,  332,
 /*   650 */   332,  332,  332,  332,  148,  149,  150,  151,  152,  153,
 /*   660 */   332,  332,  332,  332,  131,  132,   69,  144,  134,  135,
 /*   670 */   136,   52,  332,  138,  139,   80,   83,   85,  145,  141,
 /*   680 */   142,  146,  332,  132,   69,  144,  134,  135,  136,   52,
 /*   690 */     5,  138,  139,   80,   83,   85,  145,  141,  142,  146,
 /*   700 */   332,  332,  332,  332,  332,  332,  332,  332,  332,  132,
 /*   710 */    69,  144,  134,  135,  136,   52,  159,  138,  139,   80,
 /*   720 */    83,   85,  145,  141,  142,  146,  332,  332,  332,  332,
 /*   730 */   332,  332,  332,  160,  332,  132,   69,  144,  134,  135,
 /*   740 */   136,   52,  332,  138,  139,   80,   83,   85,  145,  141,
 /*   750 */   142,  146,  132,   69,  144,  134,  135,  136,   52,  161,
 /*   760 */   138,  139,   80,   83,   85,  145,  141,  142,  146,  332,
 /*   770 */   332,  332,  332,  332,  332,  332,  332,  332,  132,   69,
 /*   780 */   144,  134,  135,  136,   52,    6,  138,  139,   80,   83,
 /*   790 */    85,  145,  141,  142,  146,  332,  332,  332,  332,  332,
 /*   800 */   332,  332,    7,  332,  132,   69,  144,  134,  135,  136,
 /*   810 */    52,  332,  138,  139,   80,   83,   85,  145,  141,  142,
 /*   820 */   146,  132,   69,  144,  134,  135,  136,   52,  179,  138,
 /*   830 */   139,   80,   83,   85,  145,  141,  142,  146,  332,  332,
 /*   840 */   332,  332,  332,  332,  332,  332,  332,  132,   69,  144,
 /*   850 */   134,  135,  136,   52,  195,  138,  139,   80,   83,   85,
 /*   860 */   145,  141,  142,  146,  332,  332,  332,  332,  332,  332,
 /*   870 */   332,  126,  332,  132,   69,  144,  134,  135,  136,   52,
 /*   880 */   332,  138,  139,   80,   83,   85,  145,  141,  142,  146,
 /*   890 */   132,   69,  144,  134,  135,  136,   52,  332,  138,  139,
 /*   900 */    80,   83,   85,  145,  141,  142,  146,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    44,   45,   46,   47,   48,   58,   59,   60,   61,   62,
 /*    10 */    63,   52,   52,   66,   22,   23,   57,   57,   45,   46,
 /*    20 */    47,   48,    2,   67,   68,   69,   70,   71,   72,   73,
 /*    30 */     1,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*    40 */    67,   68,   69,   70,   71,   72,   73,   68,   75,   76,
 /*    50 */    77,   78,   79,   80,   81,   82,   83,   51,   79,   80,
 /*    60 */    81,   82,   83,   68,   45,   46,   47,   48,   73,   40,
 /*    70 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   62,
 /*    80 */    63,   45,   46,   47,   48,    1,   67,   68,   69,   70,
 /*    90 */    71,   72,   73,   87,   75,   76,   77,   78,   79,   80,
 /*   100 */    81,   82,   83,   67,   68,   69,   70,   71,   72,   73,
 /*   110 */    68,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   120 */     2,   79,   80,   81,   82,   83,   42,   45,   46,   47,
 /*   130 */    48,   58,   59,   60,   61,   62,   63,   62,   63,   66,
 /*   140 */    22,   23,   24,    1,   45,   46,   47,   48,    6,   67,
 /*   150 */    68,   69,   70,   71,   72,   73,   25,   75,   76,   77,
 /*   160 */    78,   79,   80,   81,   82,   83,   67,   68,   69,   70,
 /*   170 */    71,   72,   73,   68,   75,   76,   77,   78,   79,   80,
 /*   180 */    81,   82,   83,    2,   51,   80,   81,   82,   83,   85,
 /*   190 */    45,   46,   47,   48,   60,   61,   62,   63,    1,    4,
 /*   200 */    66,    6,    5,   22,   23,   24,   26,   45,   46,   47,
 /*   210 */    48,   74,   67,   68,   69,   70,   71,   72,   73,   30,
 /*   220 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   67,
 /*   230 */    68,   69,   70,   71,   72,   73,   41,   75,   76,   77,
 /*   240 */    78,   79,   80,   81,   82,   83,    2,   61,   62,   63,
 /*   250 */    62,   63,   66,   45,   46,   47,   48,   61,   62,   63,
 /*   260 */    62,   63,   66,   86,   55,   56,   22,   23,   24,    1,
 /*   270 */    45,   46,   47,   48,    6,   67,   68,   69,   70,   71,
 /*   280 */    72,   73,   17,   75,   76,   77,   78,   79,   80,   81,
 /*   290 */    82,   83,   67,   68,   69,   70,   71,   72,   73,   53,
 /*   300 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   41,
 /*   310 */     1,   16,    2,    4,   54,   20,   45,   46,   47,   48,
 /*   320 */    25,   64,   65,   38,   39,   30,    1,   18,    0,    1,
 /*   330 */     5,   56,   63,   45,   46,   47,   48,   50,   67,   68,
 /*   340 */    69,   70,   71,   72,   73,   22,   75,   76,   77,   78,
 /*   350 */    79,   80,   81,   82,   83,   67,   68,   69,   70,   71,
 /*   360 */    72,   73,   63,   75,   76,   77,   78,   79,   80,   81,
 /*   370 */    82,   83,   16,    1,   87,   63,   20,    5,   65,   45,
 /*   380 */    46,   47,   48,    1,    1,   63,   30,    5,    5,   63,
 /*   390 */    63,   86,   63,   63,   15,   27,   45,   46,   47,   48,
 /*   400 */    29,   67,   68,   69,   70,   71,   72,   73,   28,   75,
 /*   410 */    76,   77,   78,   79,   80,   81,   82,   83,   67,   68,
 /*   420 */    69,   70,   71,   72,   73,   21,   75,   76,   77,   78,
 /*   430 */    79,   80,   81,   82,   83,    5,    1,    5,    2,   15,
 /*   440 */     2,   15,   45,   46,   47,   48,    2,   16,    2,   20,
 /*   450 */     2,    2,   15,    2,   21,   15,   15,   15,   15,   45,
 /*   460 */    46,   47,   48,    5,   67,   68,   69,   70,   71,   72,
 /*   470 */    73,   15,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   480 */    83,   67,   68,   69,   70,   71,   72,   73,    2,   75,
 /*   490 */    76,   77,   78,   79,   80,   81,   82,   83,   15,   15,
 /*   500 */    15,    2,   15,   15,   15,   15,   46,   47,   48,   15,
 /*   510 */    15,    1,   31,   31,    2,   88,   88,   88,   32,   33,
 /*   520 */    34,   35,   36,   37,   88,   48,   49,   67,   68,   69,
 /*   530 */    70,   71,   72,   73,   88,   75,   76,   77,   78,   79,
 /*   540 */    80,   81,   82,   83,   67,   68,   69,   70,   71,   72,
 /*   550 */    73,   88,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   560 */    83,   84,   88,   88,   88,   88,   88,   88,   88,   88,
 /*   570 */    88,   88,   88,   88,   48,   49,   68,   69,   70,   71,
 /*   580 */    72,   73,   88,   75,   76,   77,   78,   79,   80,   81,
 /*   590 */    82,   83,   88,   67,   68,   69,   70,   71,   72,   73,
 /*   600 */    88,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   610 */     2,    3,   88,   88,   88,    7,    8,    9,   10,   11,
 /*   620 */    12,   13,   14,    1,    2,   88,   88,   19,   88,   88,
 /*   630 */    88,   88,   88,   88,   88,   88,   88,   88,   88,   88,
 /*   640 */    32,   33,   34,   35,   36,   37,   48,   88,   88,   88,
 /*   650 */    88,   88,   88,   88,   32,   33,   34,   35,   36,   37,
 /*   660 */    88,   88,   88,   88,   48,   67,   68,   69,   70,   71,
 /*   670 */    72,   73,   88,   75,   76,   77,   78,   79,   80,   81,
 /*   680 */    82,   83,   88,   67,   68,   69,   70,   71,   72,   73,
 /*   690 */    48,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   700 */    88,   88,   88,   88,   88,   88,   88,   88,   88,   67,
 /*   710 */    68,   69,   70,   71,   72,   73,   48,   75,   76,   77,
 /*   720 */    78,   79,   80,   81,   82,   83,   88,   88,   88,   88,
 /*   730 */    88,   88,   88,   48,   88,   67,   68,   69,   70,   71,
 /*   740 */    72,   73,   88,   75,   76,   77,   78,   79,   80,   81,
 /*   750 */    82,   83,   67,   68,   69,   70,   71,   72,   73,   48,
 /*   760 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   88,
 /*   770 */    88,   88,   88,   88,   88,   88,   88,   88,   67,   68,
 /*   780 */    69,   70,   71,   72,   73,   48,   75,   76,   77,   78,
 /*   790 */    79,   80,   81,   82,   83,   88,   88,   88,   88,   88,
 /*   800 */    88,   88,   48,   88,   67,   68,   69,   70,   71,   72,
 /*   810 */    73,   88,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   820 */    83,   67,   68,   69,   70,   71,   72,   73,   48,   75,
 /*   830 */    76,   77,   78,   79,   80,   81,   82,   83,   88,   88,
 /*   840 */    88,   88,   88,   88,   88,   88,   88,   67,   68,   69,
 /*   850 */    70,   71,   72,   73,   48,   75,   76,   77,   78,   79,
 /*   860 */    80,   81,   82,   83,   88,   88,   88,   88,   88,   88,
 /*   870 */    88,   48,   88,   67,   68,   69,   70,   71,   72,   73,
 /*   880 */    88,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   890 */    67,   68,   69,   70,   71,   72,   73,   88,   75,   76,
 /*   900 */    77,   78,   79,   80,   81,   82,   83,
};
#define YY_SHIFT_USE_DFLT (-9)
#define YY_SHIFT_MAX 127
static const short yy_shift_ofst[] = {
 /*     0 */   608,  608,  608,  608,  608,  608,  608,  608,  608,  608,
 /*    10 */   608,  608,  608,  608,  608,  608,  608,  486,  486,  622,
 /*    20 */   486,  486,  486,  486,  486,  486,  486,  486,  486,  486,
 /*    30 */   486,  486,  118,  118,  486,  486,  486,  181,  244,  244,
 /*    40 */   268,  195,  309,  309,   -8,   -8,   -8,   -8,   20,  131,
 /*    50 */   285,  142,  180,  189,  265,  310,   20,  323,  323,  131,
 /*    60 */   323,  323,  131,  323,  323,  323,  323,  131,  189,  295,
 /*    70 */   356,  328,  197,  325,  372,  382,   84,  383,   29,  379,
 /*    80 */   368,  371,  379,  380,  371,  371,  404,  430,  435,  432,
 /*    90 */   435,  436,  424,  438,  426,  431,  444,  431,  446,  429,
 /*   100 */   433,  437,  440,  441,  442,  448,  449,  443,  451,  456,
 /*   110 */   483,  484,  485,  487,  488,  489,  490,  494,  495,  458,
 /*   120 */   499,  510,  435,  435,  435,  481,  482,  512,
};
#define YY_REDUCE_USE_DFLT (-54)
#define YY_REDUCE_MAX 68
static const short yy_reduce_ofst[] = {
 /*     0 */   -44,  -27,   19,   36,   82,   99,  145,  162,  208,  225,
 /*    10 */   271,  288,  334,  351,  397,  414,  460,  477,  526,  598,
 /*    20 */   616,  642,  668,  685,  711,  737,  754,  780,  806,  823,
 /*    30 */   508,   -5,  -53,   73,  -21,   42,  105,  134,  186,  196,
 /*    40 */   287,    6,  -41,  -40,   17,   75,  188,  198,  209,  257,
 /*    50 */   104,  133,  137,  177,  246,  260,  275,  269,  299,  313,
 /*    60 */   312,  322,  313,  326,  327,  329,  330,  313,  305,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   208,  208,  208,  208,  208,  208,  208,  208,  208,  208,
 /*    10 */   208,  208,  208,  208,  208,  208,  208,  316,  309,  330,
 /*    20 */   330,  330,  216,  218,  220,  330,  330,  330,  330,  330,
 /*    30 */   330,  330,  270,  270,  330,  330,  330,  330,  330,  330,
 /*    40 */   330,  328,  236,  236,  330,  330,  330,  330,  330,  274,
 /*    50 */   318,  328,  290,  321,  232,  330,  330,  330,  330,  330,
 /*    60 */   330,  330,  278,  330,  330,  330,  330,  277,  321,  304,
 /*    70 */   304,  330,  330,  330,  330,  330,  330,  330,  330,  211,
 /*    80 */   295,  297,  317,  296,  299,  298,  330,  330,  329,  330,
 /*    90 */   237,  330,  224,  330,  225,  227,  330,  226,  330,  330,
 /*   100 */   330,  254,  246,  242,  240,  330,  330,  244,  330,  250,
 /*   110 */   248,  252,  262,  258,  256,  260,  266,  264,  268,  330,
 /*   120 */   330,  330,  325,  326,  327,  330,  330,  330,  207,  209,
 /*   130 */   210,  283,  284,  285,  287,  288,  289,  291,  293,  294,
 /*   140 */   301,  302,  303,  282,  286,  300,  305,  309,  310,  311,
 /*   150 */   312,  313,  314,  315,  306,  206,  319,  212,  215,  217,
 /*   160 */   219,  221,  222,  234,  235,  223,  231,  230,  229,  228,
 /*   170 */   238,  239,  271,  241,  272,  243,  245,  273,  275,  276,
 /*   180 */   280,  281,  247,  249,  251,  253,  279,  255,  257,  259,
 /*   190 */   261,  263,  265,  267,  269,  233,  213,  324,  214,  323,
 /*   200 */   322,  320,  307,  308,  292,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#if defined(YYFALLBACK)
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#if defined(YYTRACKMAXSTACKDEPTH)
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  ParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#if 0
static void 
dump_parser_stack(YogEnv* env, YogVal parser) 
{
    DPRINTF("-------------------- dump of stack --------------------");
    int i;
    for (i = 0; i < PTR_AS(yyParser, parser)->yyidx; i++) {
        YogVal val = PTR_AS(yyParser, parser)->yystack[i + 1].minor.yy0;
        DPRINTF("PTR_AS(yyParser, parser)->yyidx=%d, i=%d, val=%p", PTR_AS(yyParser, parser)->yyidx, i, VAL2PTR(val));
#if 0
        YogVal_print(env, val);
#endif
    }
    DPRINTF("-------------------- end of stack --------------------");
}
#endif

#if !defined(NDEBUG)
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#if !defined(NDEBUG)
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
static void ParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#if !defined(NDEBUG)
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "NEWLINE",       "NAME",          "TRY",         
  "ELSE",          "END",           "FINALLY",       "WHILE",       
  "BREAK",         "NEXT",          "RETURN",        "IF",          
  "CLASS",         "NONLOCAL",      "IMPORT",        "COMMA",       
  "DOT",           "GREATER",       "ELIF",          "DEF",         
  "LPAR",          "RPAR",          "DOUBLE_STAR",   "STAR",        
  "AMPER",         "EQUAL",         "LESS",          "LSHIFT",      
  "EQUAL_TILDA",   "PLUS",          "LBRACKET",      "RBRACKET",    
  "NUMBER",        "REGEXP",        "STRING",        "TRUE",        
  "FALSE",         "LINE",          "DO",            "LBRACE",      
  "RBRACE",        "EXCEPT",        "AS",            "error",       
  "module",        "stmts",         "stmt",          "func_def",    
  "expr",          "args",          "excepts",       "finally_opt", 
  "if_tail",       "super_opt",     "names",         "dotted_names",
  "dotted_name",   "else_opt",      "params",        "params_without_default",
  "params_with_default",  "block_param",   "var_param",     "kw_param",    
  "param_default_opt",  "param_default",  "param_with_default",  "assign_expr", 
  "postfix_expr",  "logical_or_expr",  "logical_and_expr",  "not_expr",    
  "comparison",    "xor_expr",      "comp_op",       "or_expr",     
  "and_expr",      "shift_expr",    "match_expr",    "arith_expr",  
  "term",          "factor",        "power",         "atom",        
  "args_opt",      "blockarg_opt",  "blockarg_params_opt",  "except",      
};
#endif /* NDEBUG */

#if !defined(NDEBUG)
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "module ::= stmts",
 /*   1 */ "stmts ::= stmt",
 /*   2 */ "stmts ::= stmts NEWLINE stmt",
 /*   3 */ "stmt ::=",
 /*   4 */ "stmt ::= func_def",
 /*   5 */ "stmt ::= expr",
 /*   6 */ "stmt ::= NAME args",
 /*   7 */ "stmt ::= TRY stmts excepts ELSE stmts finally_opt END",
 /*   8 */ "stmt ::= TRY stmts excepts finally_opt END",
 /*   9 */ "stmt ::= TRY stmts FINALLY stmts END",
 /*  10 */ "stmt ::= WHILE expr stmts END",
 /*  11 */ "stmt ::= BREAK",
 /*  12 */ "stmt ::= BREAK expr",
 /*  13 */ "stmt ::= NEXT",
 /*  14 */ "stmt ::= NEXT expr",
 /*  15 */ "stmt ::= RETURN",
 /*  16 */ "stmt ::= RETURN expr",
 /*  17 */ "stmt ::= IF expr stmts if_tail END",
 /*  18 */ "stmt ::= CLASS NAME super_opt stmts END",
 /*  19 */ "stmt ::= NONLOCAL names",
 /*  20 */ "stmt ::= IMPORT dotted_names",
 /*  21 */ "dotted_names ::= dotted_name",
 /*  22 */ "dotted_names ::= dotted_names COMMA dotted_name",
 /*  23 */ "dotted_name ::= NAME",
 /*  24 */ "dotted_name ::= dotted_name DOT NAME",
 /*  25 */ "names ::= NAME",
 /*  26 */ "names ::= names COMMA NAME",
 /*  27 */ "super_opt ::=",
 /*  28 */ "super_opt ::= GREATER expr",
 /*  29 */ "if_tail ::= else_opt",
 /*  30 */ "if_tail ::= ELIF expr stmts if_tail",
 /*  31 */ "else_opt ::=",
 /*  32 */ "else_opt ::= ELSE stmts",
 /*  33 */ "func_def ::= DEF NAME LPAR params RPAR stmts END",
 /*  34 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  35 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param",
 /*  36 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param",
 /*  37 */ "params ::= params_without_default COMMA params_with_default COMMA block_param",
 /*  38 */ "params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param",
 /*  39 */ "params ::= params_without_default COMMA params_with_default COMMA var_param",
 /*  40 */ "params ::= params_without_default COMMA params_with_default COMMA kw_param",
 /*  41 */ "params ::= params_without_default COMMA params_with_default",
 /*  42 */ "params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  43 */ "params ::= params_without_default COMMA block_param COMMA var_param",
 /*  44 */ "params ::= params_without_default COMMA block_param COMMA kw_param",
 /*  45 */ "params ::= params_without_default COMMA block_param",
 /*  46 */ "params ::= params_without_default COMMA var_param COMMA kw_param",
 /*  47 */ "params ::= params_without_default COMMA var_param",
 /*  48 */ "params ::= params_without_default COMMA kw_param",
 /*  49 */ "params ::= params_without_default",
 /*  50 */ "params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  51 */ "params ::= params_with_default COMMA block_param COMMA var_param",
 /*  52 */ "params ::= params_with_default COMMA block_param COMMA kw_param",
 /*  53 */ "params ::= params_with_default COMMA block_param",
 /*  54 */ "params ::= params_with_default COMMA var_param COMMA kw_param",
 /*  55 */ "params ::= params_with_default COMMA var_param",
 /*  56 */ "params ::= params_with_default COMMA kw_param",
 /*  57 */ "params ::= params_with_default",
 /*  58 */ "params ::= block_param COMMA var_param COMMA kw_param",
 /*  59 */ "params ::= block_param COMMA var_param",
 /*  60 */ "params ::= block_param COMMA kw_param",
 /*  61 */ "params ::= block_param",
 /*  62 */ "params ::= var_param COMMA kw_param",
 /*  63 */ "params ::= var_param",
 /*  64 */ "params ::= kw_param",
 /*  65 */ "params ::=",
 /*  66 */ "kw_param ::= DOUBLE_STAR NAME",
 /*  67 */ "var_param ::= STAR NAME",
 /*  68 */ "block_param ::= AMPER NAME param_default_opt",
 /*  69 */ "param_default_opt ::=",
 /*  70 */ "param_default_opt ::= param_default",
 /*  71 */ "param_default ::= EQUAL expr",
 /*  72 */ "params_without_default ::= NAME",
 /*  73 */ "params_without_default ::= params_without_default COMMA NAME",
 /*  74 */ "params_with_default ::= param_with_default",
 /*  75 */ "params_with_default ::= params_with_default COMMA param_with_default",
 /*  76 */ "param_with_default ::= NAME param_default",
 /*  77 */ "args ::= expr",
 /*  78 */ "args ::= args COMMA expr",
 /*  79 */ "expr ::= assign_expr",
 /*  80 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /*  81 */ "assign_expr ::= logical_or_expr",
 /*  82 */ "logical_or_expr ::= logical_and_expr",
 /*  83 */ "logical_and_expr ::= not_expr",
 /*  84 */ "not_expr ::= comparison",
 /*  85 */ "comparison ::= xor_expr",
 /*  86 */ "comparison ::= xor_expr comp_op xor_expr",
 /*  87 */ "comp_op ::= LESS",
 /*  88 */ "xor_expr ::= or_expr",
 /*  89 */ "or_expr ::= and_expr",
 /*  90 */ "and_expr ::= shift_expr",
 /*  91 */ "shift_expr ::= match_expr",
 /*  92 */ "shift_expr ::= shift_expr LSHIFT arith_expr",
 /*  93 */ "match_expr ::= arith_expr",
 /*  94 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /*  95 */ "arith_expr ::= term",
 /*  96 */ "arith_expr ::= arith_expr PLUS term",
 /*  97 */ "term ::= factor",
 /*  98 */ "factor ::= power",
 /*  99 */ "power ::= postfix_expr",
 /* 100 */ "postfix_expr ::= atom",
 /* 101 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 102 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 103 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 104 */ "atom ::= NAME",
 /* 105 */ "atom ::= NUMBER",
 /* 106 */ "atom ::= REGEXP",
 /* 107 */ "atom ::= STRING",
 /* 108 */ "atom ::= TRUE",
 /* 109 */ "atom ::= FALSE",
 /* 110 */ "atom ::= LINE",
 /* 111 */ "args_opt ::=",
 /* 112 */ "args_opt ::= args",
 /* 113 */ "blockarg_opt ::=",
 /* 114 */ "blockarg_opt ::= DO blockarg_params_opt stmts END",
 /* 115 */ "blockarg_opt ::= LBRACE blockarg_params_opt stmts RBRACE",
 /* 116 */ "blockarg_params_opt ::=",
 /* 117 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 118 */ "excepts ::= except",
 /* 119 */ "excepts ::= excepts except",
 /* 120 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 121 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 122 */ "except ::= EXCEPT NEWLINE stmts",
 /* 123 */ "finally_opt ::=",
 /* 124 */ "finally_opt ::= FINALLY stmts",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(YogVal p){
  int newSize;
  yyStackEntry *pNew;

  newSize = PTR_AS(yyParser, p)->yystksz*2 + 100;
  pNew = realloc(PTR_AS(yyParser, p)->yystack, newSize * sizeof(pNew[0]));
  if( pNew ){
    PTR_AS(yyParser, p)->yystack = pNew;
    PTR_AS(yyParser, p)->yystksz = newSize;
#if !defined(NDEBUG)
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, PTR_AS(yyParser, p)->yystksz);
    }
#endif
  }
}
#endif

static void 
LemonParser_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    yyParser* pParser = ptr;
    int i;
    for (i = 0; i < pParser->yyidx; i++) {
        YogGC_keep(env, &pParser->yystack[i + 1].minor.yy0, keeper, heap);
    }
}

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to Parse and ParseFree.
*/
static YogVal 
LemonParser_new(YogEnv* env) 
{
    YogVal pParser = ALLOC_OBJ(env, LemonParser_keep_children, NULL, yyParser);
  if (IS_PTR(pParser)) {
    PTR_AS(yyParser, pParser)->yyidx = -1;
#if defined(YYTRACKMAXSTACKDEPTH)
    PTR_AS(yyParser, pParser)->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    PTR_AS(yyParser, pParser)->yystack = NULL;
    PTR_AS(yyParser, pParser)->yystksz = 0;
    yyGrowStack(pParser);
#else
    int i;
    for (i = 0; i < YYSTACKDEPTH; i++) {
        PTR_AS(yyParser, pParser)->yystack[i].minor.yy0 = YUNDEF;
    }
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(
  YogVal parser, 
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  ParseARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(YogVal parser) {
  YYCODETYPE yymajor;
  unsigned int yyidx = PTR_AS(yyParser, parser)->yyidx;
  yyStackEntry *yytos = &PTR_AS(yyParser, parser)->yystack[yyidx];

  if (PTR_AS(yyParser, parser)->yyidx < 0) return 0;
#if !defined(NDEBUG)
  if (yyTraceFILE && (0 <= PTR_AS(yyParser, parser)->yyidx)) {
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor(parser, yymajor, &yytos->minor);
  PTR_AS(yyParser, parser)->yyidx--;
  return yymajor;
}

#if 0
/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from ParseAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void ParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}
#endif

/*
** Return the peak depth of the stack for a parser.
*/
#if defined(YYTRACKMAXSTACKDEPTH)
static int ParseStackPeak(YogVal parser) {
  return PTR_AS(yyParser, parser)->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  YogVal parser, 
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int yyidx = PTR_AS(yyParser, parser)->yyidx;
  int stateno = PTR_AS(yyParser, parser)->yystack[yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#if defined(YYFALLBACK)
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#if !defined(NDEBUG)
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(parser, iFallback);
      }
#endif
#if defined(YYWILDCARD)
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#if !defined(NDEBUG)
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#if defined(YYERRORSYMBOL)
  if( stateno>YY_REDUCE_MAX ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_MAX );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#if defined(YYERRORSYMBOL)
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_SZ_ACTTAB );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(YogVal parser, YYMINORTYPE *yypMinor){
   ParseARG_FETCH;
   PTR_AS(yyParser, parser)->yyidx--;
#if !defined(NDEBUG)
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while (0 <= PTR_AS(yyParser, parser)->yyidx) yy_pop_parser_stack(parser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
   ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  YogEnv* env, 
  YogVal parser, 
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  PTR_AS(yyParser, parser)->yyidx++;
#if defined(YYTRACKMAXSTACKDEPTH)
  if( PTR_AS(yyParser, parser)->yyidx>PTR_AS(yyParser, parser)->yyidxMax ){
    PTR_AS(yyParser, parser)->yyidxMax = PTR_AS(yyParser, parser)->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( PTR_AS(yyParser, parser)->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(parser, yypMinor);
    return;
  }
#else
  if( PTR_AS(yyParser, parser)->yyidx>=PTR_AS(yyParser, parser)->yystksz ){
    yyGrowStack(parser);
    if( PTR_AS(yyParser, parser)->yyidx>=PTR_AS(yyParser, parser)->yystksz ){
      yyStackOverflow(parser, yypMinor);
      return;
    }
  }
#endif
  yytos = &PTR_AS(yyParser, parser)->yystack[PTR_AS(yyParser, parser)->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor = *yypMinor;
  ADD_REF(env, yytos->minor.yy0);
#if !defined(NDEBUG)
  if( yyTraceFILE && PTR_AS(yyParser, parser)->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=PTR_AS(yyParser, parser)->yyidx; i++) {
      YYCODETYPE major = PTR_AS(yyParser, parser)->yystack[i].major;
      fprintf(yyTraceFILE," %s",yyTokenName[major]);
    }
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 44, 1 },
  { 45, 1 },
  { 45, 3 },
  { 46, 0 },
  { 46, 1 },
  { 46, 1 },
  { 46, 2 },
  { 46, 7 },
  { 46, 5 },
  { 46, 5 },
  { 46, 4 },
  { 46, 1 },
  { 46, 2 },
  { 46, 1 },
  { 46, 2 },
  { 46, 1 },
  { 46, 2 },
  { 46, 5 },
  { 46, 5 },
  { 46, 2 },
  { 46, 2 },
  { 55, 1 },
  { 55, 3 },
  { 56, 1 },
  { 56, 3 },
  { 54, 1 },
  { 54, 3 },
  { 53, 0 },
  { 53, 2 },
  { 52, 1 },
  { 52, 4 },
  { 57, 0 },
  { 57, 2 },
  { 47, 7 },
  { 58, 9 },
  { 58, 7 },
  { 58, 7 },
  { 58, 5 },
  { 58, 7 },
  { 58, 5 },
  { 58, 5 },
  { 58, 3 },
  { 58, 7 },
  { 58, 5 },
  { 58, 5 },
  { 58, 3 },
  { 58, 5 },
  { 58, 3 },
  { 58, 3 },
  { 58, 1 },
  { 58, 7 },
  { 58, 5 },
  { 58, 5 },
  { 58, 3 },
  { 58, 5 },
  { 58, 3 },
  { 58, 3 },
  { 58, 1 },
  { 58, 5 },
  { 58, 3 },
  { 58, 3 },
  { 58, 1 },
  { 58, 3 },
  { 58, 1 },
  { 58, 1 },
  { 58, 0 },
  { 63, 2 },
  { 62, 2 },
  { 61, 3 },
  { 64, 0 },
  { 64, 1 },
  { 65, 2 },
  { 59, 1 },
  { 59, 3 },
  { 60, 1 },
  { 60, 3 },
  { 66, 2 },
  { 49, 1 },
  { 49, 3 },
  { 48, 1 },
  { 67, 3 },
  { 67, 1 },
  { 69, 1 },
  { 70, 1 },
  { 71, 1 },
  { 72, 1 },
  { 72, 3 },
  { 74, 1 },
  { 73, 1 },
  { 75, 1 },
  { 76, 1 },
  { 77, 1 },
  { 77, 3 },
  { 78, 1 },
  { 78, 3 },
  { 79, 1 },
  { 79, 3 },
  { 80, 1 },
  { 81, 1 },
  { 82, 1 },
  { 68, 1 },
  { 68, 5 },
  { 68, 4 },
  { 68, 3 },
  { 83, 1 },
  { 83, 1 },
  { 83, 1 },
  { 83, 1 },
  { 83, 1 },
  { 83, 1 },
  { 83, 1 },
  { 84, 0 },
  { 84, 1 },
  { 85, 0 },
  { 85, 4 },
  { 85, 4 },
  { 86, 0 },
  { 86, 3 },
  { 50, 1 },
  { 50, 2 },
  { 87, 6 },
  { 87, 4 },
  { 87, 3 },
  { 51, 0 },
  { 51, 2 },
};

static void yy_accept(YogVal);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  YogEnv* env, 
  YogVal parser, 
  int yyruleno                 /* Number of the rule by which to reduce */
){
  SAVE_LOCALS(env);

  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  int yyidx = PTR_AS(yyParser, parser)->yyidx;
#define yymsp   (&PTR_AS(yyParser, parser)->yystack[yyidx])
  int yysize;                     /* Amount to pop the stack */

  yygotominor.yy0 = YUNDEF;

  YogLocals locals;
  locals.num_vals = 2;
  locals.size = 1;
  locals.vals[0] = &parser;
  locals.vals[1] = &yygotominor.yy0;
  locals.vals[2] = NULL;
  locals.vals[3] = NULL;
  locals.vals[4] = NULL;
  PUSH_LOCAL_TABLE(env, locals);

  ParseARG_FETCH;
#if !defined(NDEBUG)
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  /*memset(&yygotominor, 0, sizeof(yygotominor));*/


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 0: /* module ::= stmts */
#line 604 "parser.y"
{
    *pval = yymsp[0].minor.yy111;
}
#line 1758 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 21: /* dotted_names ::= dotted_name */
      case 74: /* params_with_default ::= param_with_default */
      case 77: /* args ::= expr */
      case 118: /* excepts ::= except */
#line 608 "parser.y"
{
    yygotominor.yy111 = Array_new(env, yymsp[0].minor.yy111);
}
#line 1769 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 22: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 75: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 78: /* args ::= args COMMA expr */
#line 611 "parser.y"
{
    yygotominor.yy111 = Array_push(env, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 1779 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 27: /* super_opt ::= */
      case 31: /* else_opt ::= */
      case 69: /* param_default_opt ::= */
      case 111: /* args_opt ::= */
      case 113: /* blockarg_opt ::= */
      case 116: /* blockarg_params_opt ::= */
      case 123: /* finally_opt ::= */
#line 615 "parser.y"
{
    yygotominor.yy111 = YNIL;
}
#line 1793 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 28: /* super_opt ::= GREATER expr */
      case 29: /* if_tail ::= else_opt */
      case 32: /* else_opt ::= ELSE stmts */
      case 70: /* param_default_opt ::= param_default */
      case 71: /* param_default ::= EQUAL expr */
      case 79: /* expr ::= assign_expr */
      case 81: /* assign_expr ::= logical_or_expr */
      case 82: /* logical_or_expr ::= logical_and_expr */
      case 83: /* logical_and_expr ::= not_expr */
      case 84: /* not_expr ::= comparison */
      case 85: /* comparison ::= xor_expr */
      case 88: /* xor_expr ::= or_expr */
      case 89: /* or_expr ::= and_expr */
      case 90: /* and_expr ::= shift_expr */
      case 91: /* shift_expr ::= match_expr */
      case 93: /* match_expr ::= arith_expr */
      case 95: /* arith_expr ::= term */
      case 97: /* term ::= factor */
      case 98: /* factor ::= power */
      case 99: /* power ::= postfix_expr */
      case 100: /* postfix_expr ::= atom */
      case 112: /* args_opt ::= args */
      case 124: /* finally_opt ::= FINALLY stmts */
#line 618 "parser.y"
{
    yygotominor.yy111 = yymsp[0].minor.yy111;
}
#line 1823 "parser.c"
        break;
      case 5: /* stmt ::= expr */
#line 621 "parser.y"
{
    if (PTR_AS(YogNode, yymsp[0].minor.yy111)->type == NODE_VARIABLE) {
        unsigned int lineno = NODE_LINENO(yymsp[0].minor.yy111);
        ID id = PTR_AS(YogNode, yymsp[0].minor.yy111)->u.variable.id;
        yygotominor.yy111 = CommandCall_new(env, lineno, id, YNIL, YNIL);
    }
    else {
        yygotominor.yy111 = yymsp[0].minor.yy111;
    }
}
#line 1837 "parser.c"
        break;
      case 6: /* stmt ::= NAME args */
#line 631 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy111 = CommandCall_new(env, lineno, id, yymsp[0].minor.yy111, YNIL);
}
#line 1846 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 643 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy111 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1854 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts excepts finally_opt END */
#line 647 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[-1].minor.yy111);
}
#line 1862 "parser.c"
        break;
      case 9: /* stmt ::= TRY stmts FINALLY stmts END */
#line 651 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = Finally_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1870 "parser.c"
        break;
      case 10: /* stmt ::= WHILE expr stmts END */
#line 655 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy111 = While_new(env, lineno, yymsp[-2].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1878 "parser.c"
        break;
      case 11: /* stmt ::= BREAK */
#line 659 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Break_new(env, lineno, YNIL);
}
#line 1886 "parser.c"
        break;
      case 12: /* stmt ::= BREAK expr */
#line 663 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Break_new(env, lineno, yymsp[0].minor.yy111);
}
#line 1894 "parser.c"
        break;
      case 13: /* stmt ::= NEXT */
#line 667 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Next_new(env, lineno, YNIL);
}
#line 1902 "parser.c"
        break;
      case 14: /* stmt ::= NEXT expr */
#line 671 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Next_new(env, lineno, yymsp[0].minor.yy111);
}
#line 1910 "parser.c"
        break;
      case 15: /* stmt ::= RETURN */
#line 675 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Return_new(env, lineno, YNIL);
}
#line 1918 "parser.c"
        break;
      case 16: /* stmt ::= RETURN expr */
#line 679 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Return_new(env, lineno, yymsp[0].minor.yy111);
}
#line 1926 "parser.c"
        break;
      case 17: /* stmt ::= IF expr stmts if_tail END */
#line 683 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = If_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-2].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1934 "parser.c"
        break;
      case 18: /* stmt ::= CLASS NAME super_opt stmts END */
#line 687 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-3].minor.yy0)->u.id;
    yygotominor.yy111 = Klass_new(env, lineno, id, yymsp[-2].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1943 "parser.c"
        break;
      case 19: /* stmt ::= NONLOCAL names */
#line 692 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Nonlocal_new(env, lineno, yymsp[0].minor.yy111);
}
#line 1951 "parser.c"
        break;
      case 20: /* stmt ::= IMPORT dotted_names */
#line 696 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Import_new(env, lineno, yymsp[0].minor.yy111);
}
#line 1959 "parser.c"
        break;
      case 23: /* dotted_name ::= NAME */
      case 25: /* names ::= NAME */
#line 708 "parser.y"
{
    yygotominor.yy111 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 1967 "parser.c"
        break;
      case 24: /* dotted_name ::= dotted_name DOT NAME */
      case 26: /* names ::= names COMMA NAME */
#line 711 "parser.y"
{
    yygotominor.yy111 = Array_push_token_id(env, yymsp[-2].minor.yy111, yymsp[0].minor.yy0);
}
#line 1975 "parser.c"
        break;
      case 30: /* if_tail ::= ELIF expr stmts if_tail */
#line 732 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-2].minor.yy111, yymsp[-1].minor.yy111, yymsp[0].minor.yy111);
    yygotominor.yy111 = Array_new(env, node);
}
#line 1984 "parser.c"
        break;
      case 33: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 745 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy111 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1993 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 751 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-8].minor.yy111, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2000 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 754 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2007 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 757 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2014 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 760 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2021 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 763 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2028 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 766 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2035 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 769 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2042 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA params_with_default */
#line 772 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL, YNIL, YNIL);
}
#line 2049 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 775 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2056 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 778 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2063 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 781 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2070 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA block_param */
#line 784 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2077 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 787 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, YNIL, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2084 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA var_param */
#line 790 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, YNIL, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2091 "parser.c"
        break;
      case 48: /* params ::= params_without_default COMMA kw_param */
#line 793 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, YNIL, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2098 "parser.c"
        break;
      case 49: /* params ::= params_without_default */
#line 796 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[0].minor.yy111, YNIL, YNIL, YNIL, YNIL);
}
#line 2105 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 799 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2112 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 802 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2119 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 805 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2126 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA block_param */
#line 808 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2133 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 811 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2140 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA var_param */
#line 814 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2147 "parser.c"
        break;
      case 56: /* params ::= params_with_default COMMA kw_param */
#line 817 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-2].minor.yy111, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2154 "parser.c"
        break;
      case 57: /* params ::= params_with_default */
#line 820 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[0].minor.yy111, YNIL, YNIL, YNIL);
}
#line 2161 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 823 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2168 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA var_param */
#line 826 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2175 "parser.c"
        break;
      case 60: /* params ::= block_param COMMA kw_param */
#line 829 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2182 "parser.c"
        break;
      case 61: /* params ::= block_param */
#line 832 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2189 "parser.c"
        break;
      case 62: /* params ::= var_param COMMA kw_param */
#line 835 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2196 "parser.c"
        break;
      case 63: /* params ::= var_param */
#line 838 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2203 "parser.c"
        break;
      case 64: /* params ::= kw_param */
#line 841 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2210 "parser.c"
        break;
      case 65: /* params ::= */
#line 844 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2217 "parser.c"
        break;
      case 66: /* kw_param ::= DOUBLE_STAR NAME */
#line 848 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2226 "parser.c"
        break;
      case 67: /* var_param ::= STAR NAME */
#line 854 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2235 "parser.c"
        break;
      case 68: /* block_param ::= AMPER NAME param_default_opt */
#line 860 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy111);
}
#line 2244 "parser.c"
        break;
      case 72: /* params_without_default ::= NAME */
#line 877 "parser.y"
{
    yygotominor.yy111 = YogArray_new(env);
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy111, lineno, id, YNIL);
}
#line 2254 "parser.c"
        break;
      case 73: /* params_without_default ::= params_without_default COMMA NAME */
#line 883 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy111, lineno, id, YNIL);
    yygotominor.yy111 = yymsp[-2].minor.yy111;
}
#line 2264 "parser.c"
        break;
      case 76: /* param_with_default ::= NAME param_default */
#line 897 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy111);
}
#line 2273 "parser.c"
        break;
      case 80: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 914 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    yygotominor.yy111 = Assign_new(env, lineno, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2281 "parser.c"
        break;
      case 86: /* comparison ::= xor_expr comp_op xor_expr */
#line 937 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy111)->u.id;
    yygotominor.yy111 = MethodCall1_new(env, lineno, yymsp[-2].minor.yy111, id, yymsp[0].minor.yy111);
}
#line 2290 "parser.c"
        break;
      case 87: /* comp_op ::= LESS */
#line 943 "parser.y"
{
    yygotominor.yy111 = yymsp[0].minor.yy0;
}
#line 2297 "parser.c"
        break;
      case 92: /* shift_expr ::= shift_expr LSHIFT arith_expr */
      case 94: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
      case 96: /* arith_expr ::= arith_expr PLUS term */
#line 962 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy111 = MethodCall1_new(env, lineno, yymsp[-2].minor.yy111, id, yymsp[0].minor.yy111);
}
#line 2308 "parser.c"
        break;
      case 101: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1001 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-4].minor.yy111);
    if (NODE(yymsp[-4].minor.yy111)->type == NODE_ATTR) {
        YogVal recv = NODE(yymsp[-4].minor.yy111)->u.attr.obj;
        ID name = NODE(yymsp[-4].minor.yy111)->u.attr.name;
        yygotominor.yy111 = MethodCall_new(env, lineno, recv, name, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
    }
    else {
        yygotominor.yy111 = FuncCall_new(env, lineno, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
    }
}
#line 2323 "parser.c"
        break;
      case 102: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1012 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-3].minor.yy111);
    yygotominor.yy111 = Subscript_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 2331 "parser.c"
        break;
      case 103: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1016 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Attr_new(env, lineno, yymsp[-2].minor.yy111, id);
}
#line 2340 "parser.c"
        break;
      case 104: /* atom ::= NAME */
#line 1022 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Variable_new(env, lineno, id);
}
#line 2349 "parser.c"
        break;
      case 105: /* atom ::= NUMBER */
      case 106: /* atom ::= REGEXP */
      case 107: /* atom ::= STRING */
#line 1027 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy111 = Literal_new(env, lineno, val);
}
#line 2360 "parser.c"
        break;
      case 108: /* atom ::= TRUE */
#line 1042 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Literal_new(env, lineno, YTRUE);
}
#line 2368 "parser.c"
        break;
      case 109: /* atom ::= FALSE */
#line 1046 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Literal_new(env, lineno, YFALSE);
}
#line 2376 "parser.c"
        break;
      case 110: /* atom ::= LINE */
#line 1050 "parser.y"
{
    unsigned int lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy111 = Literal_new(env, lineno, val);
}
#line 2385 "parser.c"
        break;
      case 114: /* blockarg_opt ::= DO blockarg_params_opt stmts END */
      case 115: /* blockarg_opt ::= LBRACE blockarg_params_opt stmts RBRACE */
#line 1066 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy111 = BlockArg_new(env, lineno, yymsp[-2].minor.yy111, yymsp[-1].minor.yy111);
}
#line 2394 "parser.c"
        break;
      case 117: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1078 "parser.y"
{
    yygotominor.yy111 = yymsp[-1].minor.yy111;
}
#line 2401 "parser.c"
        break;
      case 119: /* excepts ::= excepts except */
#line 1085 "parser.y"
{
    yygotominor.yy111 = Array_push(env, yymsp[-1].minor.yy111, yymsp[0].minor.yy111);
}
#line 2408 "parser.c"
        break;
      case 120: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1089 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy111 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy111, id, yymsp[0].minor.yy111);
}
#line 2418 "parser.c"
        break;
      case 121: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1095 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy111 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy111, NO_EXC_VAR, yymsp[0].minor.yy111);
}
#line 2426 "parser.c"
        break;
      case 122: /* except ::= EXCEPT NEWLINE stmts */
#line 1099 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy111 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy111);
}
#line 2434 "parser.c"
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  PTR_AS(yyParser, parser)->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact < YYNSTATE ){
#if defined(NDEBUG)
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      PTR_AS(yyParser, parser)->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = yyact;
      yymsp->major = yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(env, parser, yyact, yygoto, &yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(parser);
  }

  RETURN_VOID(env);
}

/*
** The following code executes when the parse fails
*/
static void yy_parse_failed(
    YogVal parser
){
  ParseARG_FETCH;
#if !defined(NDEBUG)
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while (0 <= PTR_AS(yyParser, parser)->yyidx) yy_pop_parser_stack(parser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  YogVal parser, 
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  YogVal parser
){
  ParseARG_FETCH;
#if !defined(NDEBUG)
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while (0 <= PTR_AS(yyParser, parser)->yyidx) yy_pop_parser_stack(parser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
static void Parse(
  YogEnv* env, 
  YogVal parser, 
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  yyminorunion.yy0 = YUNDEF;
  SAVE_LOCALS(env);
  YogLocals locals;
  locals.num_vals = 3;
  locals.size = 1;
  locals.vals[0] = &parser;
  locals.vals[1] = &yyminor;
  locals.vals[2] = &yyminorunion.yy0;
  locals.vals[3] = NULL;
  locals.vals[4] = NULL;
  PUSH_LOCAL_TABLE(env, locals);

  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#if defined(YYERRORSYMBOL)
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif

  /* (re)initialize the parser, if necessary */
  if (PTR_AS(yyParser, parser)->yyidx < 0) {
#if YYSTACKDEPTH<=0
    if (PTR_AS(yyParser, parser)->yystksz <= 0) {
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(parser, &yyminorunion);
      RETURN_VOID(env);
    }
#endif
    PTR_AS(yyParser, parser)->yyidx = 0;
    PTR_AS(yyParser, parser)->yyerrcnt = -1;
    PTR_AS(yyParser, parser)->yystack[0].stateno = 0;
    PTR_AS(yyParser, parser)->yystack[0].major = 0;
    PTR_AS(yyParser, parser)->yystack[0].minor.yy0 = YUNDEF;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  ParseARG_STORE;

#if !defined(NDEBUG)
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(parser,(YYCODETYPE)yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(env, parser, yyact, yymajor, &yyminorunion);
      PTR_AS(yyParser, parser)->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(env, parser, yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#if defined(YYERRORSYMBOL)
      int yymx;
#endif
#if !defined(NDEBUG)
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#if defined(YYERRORSYMBOL)
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if (PTR_AS(yyParser, parser)->yyerrcnt < 0) {
        yy_syntax_error(parser, yymajor, yyminorunion);
      }
      int yyidx = PTR_AS(yyParser, parser)->yyidx;
      yymx = PTR_AS(yyParser, parser)->yystack[yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#if !defined(NDEBUG)
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(parser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          (0 <= PTR_AS(yyParser, parser)->yyidx) &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        PTR_AS(yyParser, parser)->yystack[PTR_AS(yyParser, parser)->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(parser);
        }
        if ((PTR_AS(yyParser, parser)->yyidx < 0) || (yymajor == 0)) {
          yy_destructor(parser, (YYCODETYPE)yymajor, &yyminorunion);
          yy_parse_failed(parser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(env, parser, yyact, YYERRORSYMBOL, &u2);
        }
      }
      PTR_AS(yyParser, parser)->yyerrcnt = 3;
      yyerrorhit = 1;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if (PTR_AS(yyParser, parser)->yyerrcnt <= 0) {
        yy_syntax_error(parser, yymajor, yyminorunion);
      }
      PTR_AS(yyParser, parser)->yyerrcnt = 3;
      yy_destructor(parser, (YYCODETYPE)yymajor, &yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(parser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && PTR_AS(yyParser, parser)->yyidx>=0 );

  RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
