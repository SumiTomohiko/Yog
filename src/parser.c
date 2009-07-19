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
    case NODE_ARRAY:
        KEEP(array.elems);
        break;
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
make_array_with(YogEnv* env, YogVal elem) 
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
Array_new(YogEnv* env, unsigned int lineno, YogVal elems)
{
    SAVE_ARG(env, elems);

    YogVal node = YogNode_new(env, NODE_ARRAY, lineno);
    NODE(node)->u.array.elems = elems;

    RETURN(env, node);
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
        YogVal array = make_array_with(env, except);
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
Attr_new(YogEnv* env, unsigned int lineno, YogVal obj, ID name) 
{
    SAVE_ARG(env, obj);

    YogVal node = YogNode_new(env, NODE_ATTR, lineno);
    MODIFY(env, NODE(node)->u.attr.obj, obj);
    NODE(node)->u.attr.name = name;

    RETURN(env, node);
}

static YogVal 
FuncCall_new2(YogEnv* env, unsigned int lineno, YogVal recv, ID name, YogVal arg) 
{
    SAVE_ARGS2(env, recv, arg);
    YogVal postfix = YUNDEF;
    YogVal args = YUNDEF;
    PUSH_LOCALS2(env, postfix, args);

    postfix = Attr_new(env, lineno, recv, name);

    args = YogArray_new(env);
    YogArray_push(env, args, arg);

    YogVal node = FuncCall_new(env, lineno, postfix, args, YNIL);

    RETURN(env, node);
}

static YogVal 
FuncCall_new3(YogEnv* env, unsigned int lineno, YogVal recv, ID name)
{
    SAVE_ARG(env, recv);
    YogVal postfix = YUNDEF;
    PUSH_LOCAL(env, postfix);

    postfix = Attr_new(env, lineno, recv, name);

    YogVal node = FuncCall_new(env, lineno, postfix, YNIL, YNIL);

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
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    node = NODE_NEW(NODE_SUBSCRIPT, lineno);
    NODE(node)->u.subscript.prefix = prefix;
    NODE(node)->u.subscript.index = index;

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

static FILE*
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
    return make_array_with(env, ID2VAL(id));
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
#line 616 "parser.c"
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
#define YYNOCODE 95
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy135;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 222
#define YYNRULE 133
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
 /*     0 */   356,   77,  141,  139,  140,   76,  146,  147,  148,  149,
 /*    10 */    58,   43,  151,  152,   88,   91,   56,   53,  114,  115,
 /*    20 */   164,  155,  165,  145,   75,  158,  147,  148,  149,   58,
 /*    30 */    27,  151,  152,   88,   91,   56,   53,   76,    1,  164,
 /*    40 */   155,  165,   20,   21,   23,   24,   25,   99,  166,   61,
 /*    50 */    50,  221,  153,  155,  165,  107,    2,   14,    3,  109,
 /*    60 */   110,  121,  125,  127,  211,   96,   39,  203,  166,   18,
 /*    70 */   180,  167,  168,  169,  170,  171,  172,  173,   42,  141,
 /*    80 */   139,  140,  112,  116,  193,  181,   39,  197,  132,   18,
 /*    90 */   180,  167,  168,  169,  170,  171,  172,  173,   19,  186,
 /*   100 */   145,   75,  158,  147,  148,  149,   58,   38,  151,  152,
 /*   110 */    88,   91,   56,   53,   73,   85,  164,  155,  165,   54,
 /*   120 */   141,  139,  140,   76,  114,  115,  117,  219,  150,   36,
 /*   130 */   151,  152,   88,   91,   56,   53,  113,  190,  164,  155,
 /*   140 */   165,  145,   75,  158,  147,  148,  149,   58,  166,  151,
 /*   150 */   152,   88,   91,   56,   53,  119,  200,  164,  155,  165,
 /*   160 */    86,  141,  139,  140,   76,  175,   39,  103,  106,   18,
 /*   170 */    34,  167,  168,  169,  170,  171,  172,  173,   31,  154,
 /*   180 */   155,  165,  145,   75,  158,  147,  148,  149,   58,  217,
 /*   190 */   151,  152,   88,   91,   56,   53,   76,   94,  164,  155,
 /*   200 */   165,   78,  141,  139,  140,   28,   89,   56,   53,  123,
 /*   210 */   205,  164,  155,  165,  128,  110,  121,  125,  127,  211,
 /*   220 */   126,  209,  203,  145,   75,  158,  147,  148,  149,   58,
 /*   230 */   184,  151,  152,   88,   91,   56,   53,  194,  195,  164,
 /*   240 */   155,  165,   79,  141,  139,  140,   76,  111,  118,  120,
 /*   250 */   202,   16,   16,  203,  143,   16,    3,   55,   53,   15,
 /*   260 */   100,  164,  155,  165,  145,   75,  158,  147,  148,  149,
 /*   270 */    58,  101,  151,  152,   88,   91,   56,   53,   68,   76,
 /*   280 */   164,  155,  165,   44,  141,  139,  140,  104,  114,  115,
 /*   290 */   117,   52,   65,   16,  164,  155,  165,  159,  160,   19,
 /*   300 */    59,   74,  114,  115,  117,  145,   75,  158,  147,  148,
 /*   310 */   149,   58,  114,  151,  152,   88,   91,   56,   53,  222,
 /*   320 */    16,  164,  155,  165,   45,  141,  139,  140,  122,  124,
 /*   330 */   207,  161,   16,  197,   16,  176,  214,  182,   13,  162,
 /*   340 */   163,   16,   16,  188,  187,  220,  145,   75,  158,  147,
 /*   350 */   148,  149,   58,  192,  151,  152,   88,   91,   56,   53,
 /*   360 */   199,  198,  164,  155,  165,   98,  141,  139,  140,  131,
 /*   370 */   201,  204,  206,   17,  129,  208,  210,  142,   30,   16,
 /*   380 */     4,   32,   22,  133,   35,  174,   29,  145,   75,  158,
 /*   390 */   147,  148,  149,   58,    5,  151,  152,   88,   91,   56,
 /*   400 */    53,   57,    6,  164,  155,  165,   80,  141,  139,  140,
 /*   410 */    16,  179,    8,    7,   60,    9,  183,   33,  102,  213,
 /*   420 */    62,   10,  105,  185,  215,  108,  218,   26,  145,   75,
 /*   430 */   158,  147,  148,  149,   58,  189,  151,  152,   88,   91,
 /*   440 */    56,   53,  191,   37,  164,  155,  165,   81,  141,  139,
 /*   450 */   140,  131,   40,   46,   11,   17,   63,   64,   51,   47,
 /*   460 */    66,   12,   67,   41,   48,   69,   70,   49,   29,  145,
 /*   470 */    75,  158,  147,  148,  149,   58,   71,  151,  152,   88,
 /*   480 */    91,   56,   53,   72,  216,  164,  155,  165,   82,  141,
 /*   490 */   139,  140,  357,  134,  357,  357,  357,  357,  357,  357,
 /*   500 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   510 */   145,   75,  158,  147,  148,  149,   58,  357,  151,  152,
 /*   520 */    88,   91,   56,   53,  357,  357,  164,  155,  165,  135,
 /*   530 */   141,  139,  140,  357,  357,  357,  357,  357,  357,  357,
 /*   540 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   550 */   357,  145,   75,  158,  147,  148,  149,   58,  357,  151,
 /*   560 */   152,   88,   91,   56,   53,  357,  357,  164,  155,  165,
 /*   570 */   136,  141,  139,  140,  357,  357,  357,  357,  357,  357,
 /*   580 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   590 */   357,  357,  145,   75,  158,  147,  148,  149,   58,  357,
 /*   600 */   151,  152,   88,   91,   56,   53,  357,  357,  164,  155,
 /*   610 */   165,  137,  141,  139,  140,  357,  357,  357,  357,  357,
 /*   620 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   630 */   357,  357,  357,  145,   75,  158,  147,  148,  149,   58,
 /*   640 */   357,  151,  152,   88,   91,   56,   53,  357,  357,  164,
 /*   650 */   155,  165,   84,  141,  139,  140,  357,  357,  357,  357,
 /*   660 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   670 */   357,  357,  357,  357,  145,   75,  158,  147,  148,  149,
 /*   680 */    58,  357,  151,  152,   88,   91,   56,   53,  357,  357,
 /*   690 */   164,  155,  165,  138,  139,  140,  357,  357,  357,  357,
 /*   700 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   710 */   357,  357,  357,  357,  145,   75,  158,  147,  148,  149,
 /*   720 */    58,  357,  151,  152,   88,   91,   56,   53,  156,  357,
 /*   730 */   164,  155,  165,  357,  357,  357,  357,  357,  357,  357,
 /*   740 */   357,  357,  357,  357,  357,  357,   90,  145,   75,  158,
 /*   750 */   147,  148,  149,   58,  357,  151,  152,   88,   91,   56,
 /*   760 */    53,  357,  357,  164,  155,  165,   93,  156,  357,  357,
 /*   770 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   780 */   357,  357,  357,  357,  357,   90,  145,   75,  158,  147,
 /*   790 */   148,  149,   58,  357,  151,  152,   88,   91,   56,   53,
 /*   800 */    83,  357,  164,  155,  165,   92,  357,  357,  357,  357,
 /*   810 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  145,
 /*   820 */    75,  158,  147,  148,  149,   58,  357,  151,  152,   88,
 /*   830 */    91,   56,   53,  357,  357,  164,  155,  165,   87,  357,
 /*   840 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   850 */   357,  357,  357,  357,  357,  357,  357,  145,   75,  158,
 /*   860 */   147,  148,  149,   58,  357,  151,  152,   88,   91,   56,
 /*   870 */    53,  357,  144,  164,  155,  165,  357,  357,  357,  357,
 /*   880 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   890 */   357,  145,   75,  158,  147,  148,  149,   58,  357,  151,
 /*   900 */   152,   88,   91,   56,   53,  357,  357,  164,  155,  165,
 /*   910 */   157,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   920 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  145,
 /*   930 */    75,  158,  147,  148,  149,   58,  357,  151,  152,   88,
 /*   940 */    91,   56,   53,  357,  177,  164,  155,  165,  357,  357,
 /*   950 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*   960 */   357,  357,  357,  145,   75,  158,  147,  148,  149,   58,
 /*   970 */   357,  151,  152,   88,   91,   56,   53,  357,  357,  164,
 /*   980 */   155,  165,  178,  357,  357,  357,  357,  357,  357,  357,
 /*   990 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*  1000 */   357,  145,   75,  158,  147,  148,  149,   58,  357,  151,
 /*  1010 */   152,   88,   91,   56,   53,  357,   95,  164,  155,  165,
 /*  1020 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*  1030 */   357,  357,  357,  357,  357,  145,   75,  158,  147,  148,
 /*  1040 */   149,   58,  357,  151,  152,   88,   91,   56,   53,  357,
 /*  1050 */   357,  164,  155,  165,   97,  357,  357,  357,  357,  357,
 /*  1060 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*  1070 */   357,  357,  357,  145,   75,  158,  147,  148,  149,   58,
 /*  1080 */   357,  151,  152,   88,   91,   56,   53,  357,  196,  164,
 /*  1090 */   155,  165,  357,  357,  357,  357,  357,  357,  357,  357,
 /*  1100 */   357,  357,  357,  357,  357,  357,  357,  145,   75,  158,
 /*  1110 */   147,  148,  149,   58,  357,  151,  152,   88,   91,   56,
 /*  1120 */    53,  357,  357,  164,  155,  165,  212,  357,  357,  357,
 /*  1130 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  357,
 /*  1140 */   357,  357,  357,  357,  357,  145,   75,  158,  147,  148,
 /*  1150 */   149,   58,  357,  151,  152,   88,   91,   56,   53,  357,
 /*  1160 */   130,  164,  155,  165,  357,  357,  357,  357,  357,  357,
 /*  1170 */   357,  357,  357,  357,  357,  357,  357,  357,  357,  145,
 /*  1180 */    75,  158,  147,  148,  149,   58,  357,  151,  152,   88,
 /*  1190 */    91,   56,   53,  357,  357,  164,  155,  165,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    48,   49,   50,   51,   52,   72,   73,   74,   75,   76,
 /*    10 */    77,   53,   79,   80,   81,   82,   83,   84,   22,   23,
 /*    20 */    87,   88,   89,   71,   72,   73,   74,   75,   76,   77,
 /*    30 */    25,   79,   80,   81,   82,   83,   84,   72,    2,   87,
 /*    40 */    88,   89,    6,    7,    8,    9,   10,   11,   12,   13,
 /*    50 */    14,   93,   87,   88,   89,   19,    3,    1,    5,   61,
 /*    60 */    62,   63,   64,   65,   66,   55,   30,   69,   12,   33,
 /*    70 */    60,   35,   36,   37,   38,   39,   40,   41,   49,   50,
 /*    80 */    51,   52,   64,   65,   66,   55,   30,   69,   54,   33,
 /*    90 */    60,   35,   36,   37,   38,   39,   40,   41,   45,   12,
 /*   100 */    71,   72,   73,   74,   75,   76,   77,   86,   79,   80,
 /*   110 */    81,   82,   83,   84,   12,   54,   87,   88,   89,   49,
 /*   120 */    50,   51,   52,   72,   22,   23,   24,   93,   77,   85,
 /*   130 */    79,   80,   81,   82,   83,   84,   65,   66,   87,   88,
 /*   140 */    89,   71,   72,   73,   74,   75,   76,   77,   12,   79,
 /*   150 */    80,   81,   82,   83,   84,   65,   66,   87,   88,   89,
 /*   160 */    49,   50,   51,   52,   72,   91,   30,   58,   59,   33,
 /*   170 */    33,   35,   36,   37,   38,   39,   40,   41,   78,   87,
 /*   180 */    88,   89,   71,   72,   73,   74,   75,   76,   77,   26,
 /*   190 */    79,   80,   81,   82,   83,   84,   72,   92,   87,   88,
 /*   200 */    89,   49,   50,   51,   52,   17,   82,   83,   84,   65,
 /*   210 */    66,   87,   88,   89,   61,   62,   63,   64,   65,   66,
 /*   220 */    65,   66,   69,   71,   72,   73,   74,   75,   76,   77,
 /*   230 */    12,   79,   80,   81,   82,   83,   84,   67,   68,   87,
 /*   240 */    88,   89,   49,   50,   51,   52,   72,   63,   64,   65,
 /*   250 */    66,    1,    1,   69,    4,    1,    5,   83,   84,    5,
 /*   260 */    56,   87,   88,   89,   71,   72,   73,   74,   75,   76,
 /*   270 */    77,   57,   79,   80,   81,   82,   83,   84,   12,   72,
 /*   280 */    87,   88,   89,   49,   50,   51,   52,   59,   22,   23,
 /*   290 */    24,   84,   12,    1,   87,   88,   89,   29,   30,   45,
 /*   300 */    42,   43,   22,   23,   24,   71,   72,   73,   74,   75,
 /*   310 */    76,   77,   22,   79,   80,   81,   82,   83,   84,    0,
 /*   320 */     1,   87,   88,   89,   49,   50,   51,   52,   64,   65,
 /*   330 */    66,   23,    1,   69,    1,    4,   44,    4,    1,   31,
 /*   340 */    32,    1,    1,   66,    4,    4,   71,   72,   73,   74,
 /*   350 */    75,   76,   77,   66,   79,   80,   81,   82,   83,   84,
 /*   360 */    66,   68,   87,   88,   89,   49,   50,   51,   52,   16,
 /*   370 */    66,   66,   66,   20,   92,   66,   66,    4,   25,    1,
 /*   380 */     1,   27,   15,   46,   28,   34,   33,   71,   72,   73,
 /*   390 */    74,   75,   76,   77,    1,   79,   80,   81,   82,   83,
 /*   400 */    84,   21,    1,   87,   88,   89,   49,   50,   51,   52,
 /*   410 */     1,    4,    3,    1,   12,    1,   12,   20,   15,   34,
 /*   420 */    15,   21,   16,   12,   34,   12,    4,   18,   71,   72,
 /*   430 */    73,   74,   75,   76,   77,   12,   79,   80,   81,   82,
 /*   440 */    83,   84,   12,   15,   87,   88,   89,   49,   50,   51,
 /*   450 */    52,   16,   15,   15,    1,   20,   15,   15,   12,   15,
 /*   460 */    15,    1,   15,   15,   15,   15,   15,   15,   33,   71,
 /*   470 */    72,   73,   74,   75,   76,   77,   15,   79,   80,   81,
 /*   480 */    82,   83,   84,   15,   12,   87,   88,   89,   49,   50,
 /*   490 */    51,   52,   94,   12,   94,   94,   94,   94,   94,   94,
 /*   500 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   510 */    71,   72,   73,   74,   75,   76,   77,   94,   79,   80,
 /*   520 */    81,   82,   83,   84,   94,   94,   87,   88,   89,   49,
 /*   530 */    50,   51,   52,   94,   94,   94,   94,   94,   94,   94,
 /*   540 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   550 */    94,   71,   72,   73,   74,   75,   76,   77,   94,   79,
 /*   560 */    80,   81,   82,   83,   84,   94,   94,   87,   88,   89,
 /*   570 */    49,   50,   51,   52,   94,   94,   94,   94,   94,   94,
 /*   580 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   590 */    94,   94,   71,   72,   73,   74,   75,   76,   77,   94,
 /*   600 */    79,   80,   81,   82,   83,   84,   94,   94,   87,   88,
 /*   610 */    89,   49,   50,   51,   52,   94,   94,   94,   94,   94,
 /*   620 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   630 */    94,   94,   94,   71,   72,   73,   74,   75,   76,   77,
 /*   640 */    94,   79,   80,   81,   82,   83,   84,   94,   94,   87,
 /*   650 */    88,   89,   49,   50,   51,   52,   94,   94,   94,   94,
 /*   660 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   670 */    94,   94,   94,   94,   71,   72,   73,   74,   75,   76,
 /*   680 */    77,   94,   79,   80,   81,   82,   83,   84,   94,   94,
 /*   690 */    87,   88,   89,   50,   51,   52,   94,   94,   94,   94,
 /*   700 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   710 */    94,   94,   94,   94,   71,   72,   73,   74,   75,   76,
 /*   720 */    77,   94,   79,   80,   81,   82,   83,   84,   52,   94,
 /*   730 */    87,   88,   89,   94,   94,   94,   94,   94,   94,   94,
 /*   740 */    94,   94,   94,   94,   94,   94,   70,   71,   72,   73,
 /*   750 */    74,   75,   76,   77,   94,   79,   80,   81,   82,   83,
 /*   760 */    84,   94,   94,   87,   88,   89,   90,   52,   94,   94,
 /*   770 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   780 */    94,   94,   94,   94,   94,   70,   71,   72,   73,   74,
 /*   790 */    75,   76,   77,   94,   79,   80,   81,   82,   83,   84,
 /*   800 */    52,   94,   87,   88,   89,   90,   94,   94,   94,   94,
 /*   810 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   71,
 /*   820 */    72,   73,   74,   75,   76,   77,   94,   79,   80,   81,
 /*   830 */    82,   83,   84,   94,   94,   87,   88,   89,   52,   94,
 /*   840 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   850 */    94,   94,   94,   94,   94,   94,   94,   71,   72,   73,
 /*   860 */    74,   75,   76,   77,   94,   79,   80,   81,   82,   83,
 /*   870 */    84,   94,   52,   87,   88,   89,   94,   94,   94,   94,
 /*   880 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   890 */    94,   71,   72,   73,   74,   75,   76,   77,   94,   79,
 /*   900 */    80,   81,   82,   83,   84,   94,   94,   87,   88,   89,
 /*   910 */    52,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   920 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   71,
 /*   930 */    72,   73,   74,   75,   76,   77,   94,   79,   80,   81,
 /*   940 */    82,   83,   84,   94,   52,   87,   88,   89,   94,   94,
 /*   950 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*   960 */    94,   94,   94,   71,   72,   73,   74,   75,   76,   77,
 /*   970 */    94,   79,   80,   81,   82,   83,   84,   94,   94,   87,
 /*   980 */    88,   89,   52,   94,   94,   94,   94,   94,   94,   94,
 /*   990 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*  1000 */    94,   71,   72,   73,   74,   75,   76,   77,   94,   79,
 /*  1010 */    80,   81,   82,   83,   84,   94,   52,   87,   88,   89,
 /*  1020 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*  1030 */    94,   94,   94,   94,   94,   71,   72,   73,   74,   75,
 /*  1040 */    76,   77,   94,   79,   80,   81,   82,   83,   84,   94,
 /*  1050 */    94,   87,   88,   89,   52,   94,   94,   94,   94,   94,
 /*  1060 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*  1070 */    94,   94,   94,   71,   72,   73,   74,   75,   76,   77,
 /*  1080 */    94,   79,   80,   81,   82,   83,   84,   94,   52,   87,
 /*  1090 */    88,   89,   94,   94,   94,   94,   94,   94,   94,   94,
 /*  1100 */    94,   94,   94,   94,   94,   94,   94,   71,   72,   73,
 /*  1110 */    74,   75,   76,   77,   94,   79,   80,   81,   82,   83,
 /*  1120 */    84,   94,   94,   87,   88,   89,   52,   94,   94,   94,
 /*  1130 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
 /*  1140 */    94,   94,   94,   94,   94,   71,   72,   73,   74,   75,
 /*  1150 */    76,   77,   94,   79,   80,   81,   82,   83,   84,   94,
 /*  1160 */    52,   87,   88,   89,   94,   94,   94,   94,   94,   94,
 /*  1170 */    94,   94,   94,   94,   94,   94,   94,   94,   94,   71,
 /*  1180 */    72,   73,   74,   75,   76,   77,   94,   79,   80,   81,
 /*  1190 */    82,   83,   84,   94,   94,   87,   88,   89,
};
#define YY_SHIFT_USE_DFLT (-5)
#define YY_SHIFT_MAX 137
static const short yy_shift_ofst[] = {
 /*     0 */    36,   36,   36,   36,   36,   36,   36,   36,   36,   36,
 /*    10 */    36,   36,   36,   36,   36,   36,   36,  136,  136,   56,
 /*    20 */   136,  136,  136,  136,  136,  136,  136,  136,  136,  136,
 /*    30 */   136,  136,  136,  102,  102,  136,  136,  266,  136,  136,
 /*    40 */   280,  280,  254,   53,  409,  409,   -4,   -4,   -4,   -4,
 /*    50 */    87,    5,  308,  308,  251,  268,  268,  258,  163,  137,
 /*    60 */   188,  218,   87,  290,  290,    5,  290,  290,    5,  290,
 /*    70 */   290,  290,  290,    5,  137,  353,  435,  319,  250,  331,
 /*    80 */   333,  340,  292,  337,  341,  373,  378,  379,  354,  356,
 /*    90 */   367,  356,  351,  380,  393,  401,  407,  412,  378,  402,
 /*   100 */   414,  403,  404,  405,  406,  411,  406,  413,  397,  400,
 /*   110 */   428,  437,  438,  441,  423,  430,  442,  446,  444,  445,
 /*   120 */   447,  448,  449,  450,  451,  452,  461,  468,  385,  453,
 /*   130 */   390,  472,  422,  481,  460,  378,  378,  378,
};
#define YY_REDUCE_USE_DFLT (-68)
#define YY_REDUCE_MAX 74
static const short yy_reduce_ofst[] = {
 /*     0 */   -48,   29,   70,  111,  152,  193,  234,  275,  316,  357,
 /*    10 */   398,  439,  480,  521,  562,  603,  643,  676,  715,  748,
 /*    20 */   786,  820,  858,  892,  930,  964, 1002, 1036, 1074, 1108,
 /*    30 */   -67,   51,  124,   -2,  153,  174,  207,  184,  -35,   92,
 /*    40 */    18,  264,  -42,   34,   10,   30,   71,   90,  144,  155,
 /*    50 */   109,  170,   21,   21,   61,   44,   44,   74,  100,  105,
 /*    60 */   204,  214,  228,  277,  287,  293,  294,  304,  293,  305,
 /*    70 */   306,  309,  310,  293,  282,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   225,  225,  225,  225,  225,  225,  225,  225,  225,  225,
 /*    10 */   225,  225,  225,  225,  225,  225,  225,  341,  341,  355,
 /*    20 */   355,  232,  355,  234,  236,  355,  355,  355,  355,  355,
 /*    30 */   355,  355,  355,  286,  286,  355,  355,  355,  355,  355,
 /*    40 */   355,  355,  355,  353,  252,  252,  355,  355,  355,  355,
 /*    50 */   355,  290,  317,  316,  353,  315,  314,  343,  306,  346,
 /*    60 */   248,  355,  355,  355,  355,  355,  355,  355,  294,  355,
 /*    70 */   355,  355,  355,  293,  346,  327,  327,  355,  355,  355,
 /*    80 */   355,  355,  355,  355,  355,  355,  354,  355,  311,  313,
 /*    90 */   342,  312,  355,  355,  355,  355,  355,  355,  253,  355,
 /*   100 */   355,  240,  355,  241,  243,  355,  242,  355,  355,  355,
 /*   110 */   270,  262,  258,  256,  355,  355,  260,  355,  266,  264,
 /*   120 */   268,  278,  274,  272,  276,  282,  280,  284,  355,  355,
 /*   130 */   355,  355,  355,  355,  355,  350,  351,  352,  224,  226,
 /*   140 */   227,  223,  228,  231,  233,  300,  301,  303,  304,  305,
 /*   150 */   307,  309,  310,  320,  325,  326,  298,  299,  302,  318,
 /*   160 */   319,  322,  323,  324,  321,  328,  332,  333,  334,  335,
 /*   170 */   336,  337,  338,  339,  340,  329,  344,  235,  237,  238,
 /*   180 */   250,  251,  239,  247,  246,  245,  244,  254,  255,  287,
 /*   190 */   257,  288,  259,  261,  289,  291,  292,  296,  297,  263,
 /*   200 */   265,  267,  269,  295,  271,  273,  275,  277,  279,  281,
 /*   210 */   283,  285,  249,  347,  345,  330,  331,  308,  229,  349,
 /*   220 */   230,  348,
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
  "$",             "NEWLINE",       "TRY",           "ELSE",        
  "END",           "FINALLY",       "WHILE",         "BREAK",       
  "NEXT",          "RETURN",        "IF",            "CLASS",       
  "NAME",          "NONLOCAL",      "IMPORT",        "COMMA",       
  "DOT",           "GREATER",       "ELIF",          "DEF",         
  "LPAR",          "RPAR",          "DOUBLE_STAR",   "STAR",        
  "AMPER",         "EQUAL",         "LESS",          "LSHIFT",      
  "EQUAL_TILDA",   "PLUS",          "MINUS",         "DIV",         
  "DIV_DIV",       "LBRACKET",      "RBRACKET",      "NUMBER",      
  "REGEXP",        "STRING",        "NIL",           "TRUE",        
  "FALSE",         "LINE",          "DO",            "LBRACE",      
  "RBRACE",        "EXCEPT",        "AS",            "error",       
  "module",        "stmts",         "stmt",          "func_def",    
  "expr",          "excepts",       "finally_opt",   "if_tail",     
  "super_opt",     "names",         "dotted_names",  "dotted_name", 
  "else_opt",      "params",        "params_without_default",  "params_with_default",
  "block_param",   "var_param",     "kw_param",      "param_default_opt",
  "param_default",  "param_with_default",  "args",          "assign_expr", 
  "postfix_expr",  "logical_or_expr",  "logical_and_expr",  "not_expr",    
  "comparison",    "xor_expr",      "comp_op",       "or_expr",     
  "and_expr",      "shift_expr",    "match_expr",    "arith_expr",  
  "term",          "arith_op",      "term_op",       "factor",      
  "power",         "atom",          "args_opt",      "blockarg_opt",
  "blockarg_params_opt",  "except",      
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
 /*   6 */ "stmt ::= TRY stmts excepts ELSE stmts finally_opt END",
 /*   7 */ "stmt ::= TRY stmts excepts finally_opt END",
 /*   8 */ "stmt ::= TRY stmts FINALLY stmts END",
 /*   9 */ "stmt ::= WHILE expr NEWLINE stmts END",
 /*  10 */ "stmt ::= BREAK",
 /*  11 */ "stmt ::= BREAK expr",
 /*  12 */ "stmt ::= NEXT",
 /*  13 */ "stmt ::= NEXT expr",
 /*  14 */ "stmt ::= RETURN",
 /*  15 */ "stmt ::= RETURN expr",
 /*  16 */ "stmt ::= IF expr NEWLINE stmts if_tail END",
 /*  17 */ "stmt ::= CLASS NAME super_opt NEWLINE stmts END",
 /*  18 */ "stmt ::= NONLOCAL names",
 /*  19 */ "stmt ::= IMPORT dotted_names",
 /*  20 */ "dotted_names ::= dotted_name",
 /*  21 */ "dotted_names ::= dotted_names COMMA dotted_name",
 /*  22 */ "dotted_name ::= NAME",
 /*  23 */ "dotted_name ::= dotted_name DOT NAME",
 /*  24 */ "names ::= NAME",
 /*  25 */ "names ::= names COMMA NAME",
 /*  26 */ "super_opt ::=",
 /*  27 */ "super_opt ::= GREATER expr",
 /*  28 */ "if_tail ::= else_opt",
 /*  29 */ "if_tail ::= ELIF expr NEWLINE stmts if_tail",
 /*  30 */ "else_opt ::=",
 /*  31 */ "else_opt ::= ELSE stmts",
 /*  32 */ "func_def ::= DEF NAME LPAR params RPAR stmts END",
 /*  33 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  34 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param",
 /*  35 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param",
 /*  36 */ "params ::= params_without_default COMMA params_with_default COMMA block_param",
 /*  37 */ "params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param",
 /*  38 */ "params ::= params_without_default COMMA params_with_default COMMA var_param",
 /*  39 */ "params ::= params_without_default COMMA params_with_default COMMA kw_param",
 /*  40 */ "params ::= params_without_default COMMA params_with_default",
 /*  41 */ "params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  42 */ "params ::= params_without_default COMMA block_param COMMA var_param",
 /*  43 */ "params ::= params_without_default COMMA block_param COMMA kw_param",
 /*  44 */ "params ::= params_without_default COMMA block_param",
 /*  45 */ "params ::= params_without_default COMMA var_param COMMA kw_param",
 /*  46 */ "params ::= params_without_default COMMA var_param",
 /*  47 */ "params ::= params_without_default COMMA kw_param",
 /*  48 */ "params ::= params_without_default",
 /*  49 */ "params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  50 */ "params ::= params_with_default COMMA block_param COMMA var_param",
 /*  51 */ "params ::= params_with_default COMMA block_param COMMA kw_param",
 /*  52 */ "params ::= params_with_default COMMA block_param",
 /*  53 */ "params ::= params_with_default COMMA var_param COMMA kw_param",
 /*  54 */ "params ::= params_with_default COMMA var_param",
 /*  55 */ "params ::= params_with_default COMMA kw_param",
 /*  56 */ "params ::= params_with_default",
 /*  57 */ "params ::= block_param COMMA var_param COMMA kw_param",
 /*  58 */ "params ::= block_param COMMA var_param",
 /*  59 */ "params ::= block_param COMMA kw_param",
 /*  60 */ "params ::= block_param",
 /*  61 */ "params ::= var_param COMMA kw_param",
 /*  62 */ "params ::= var_param",
 /*  63 */ "params ::= kw_param",
 /*  64 */ "params ::=",
 /*  65 */ "kw_param ::= DOUBLE_STAR NAME",
 /*  66 */ "var_param ::= STAR NAME",
 /*  67 */ "block_param ::= AMPER NAME param_default_opt",
 /*  68 */ "param_default_opt ::=",
 /*  69 */ "param_default_opt ::= param_default",
 /*  70 */ "param_default ::= EQUAL expr",
 /*  71 */ "params_without_default ::= NAME",
 /*  72 */ "params_without_default ::= params_without_default COMMA NAME",
 /*  73 */ "params_with_default ::= param_with_default",
 /*  74 */ "params_with_default ::= params_with_default COMMA param_with_default",
 /*  75 */ "param_with_default ::= NAME param_default",
 /*  76 */ "args ::= expr",
 /*  77 */ "args ::= args COMMA expr",
 /*  78 */ "expr ::= assign_expr",
 /*  79 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /*  80 */ "assign_expr ::= logical_or_expr",
 /*  81 */ "logical_or_expr ::= logical_and_expr",
 /*  82 */ "logical_and_expr ::= not_expr",
 /*  83 */ "not_expr ::= comparison",
 /*  84 */ "comparison ::= xor_expr",
 /*  85 */ "comparison ::= xor_expr comp_op xor_expr",
 /*  86 */ "comp_op ::= LESS",
 /*  87 */ "xor_expr ::= or_expr",
 /*  88 */ "or_expr ::= and_expr",
 /*  89 */ "and_expr ::= shift_expr",
 /*  90 */ "shift_expr ::= match_expr",
 /*  91 */ "shift_expr ::= shift_expr LSHIFT match_expr",
 /*  92 */ "match_expr ::= arith_expr",
 /*  93 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /*  94 */ "arith_expr ::= term",
 /*  95 */ "arith_expr ::= arith_expr arith_op term",
 /*  96 */ "arith_op ::= PLUS",
 /*  97 */ "arith_op ::= MINUS",
 /*  98 */ "term ::= term term_op factor",
 /*  99 */ "term ::= factor",
 /* 100 */ "term_op ::= STAR",
 /* 101 */ "term_op ::= DIV",
 /* 102 */ "term_op ::= DIV_DIV",
 /* 103 */ "factor ::= MINUS factor",
 /* 104 */ "factor ::= power",
 /* 105 */ "power ::= postfix_expr",
 /* 106 */ "postfix_expr ::= atom",
 /* 107 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 108 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 109 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 110 */ "atom ::= NAME",
 /* 111 */ "atom ::= NUMBER",
 /* 112 */ "atom ::= REGEXP",
 /* 113 */ "atom ::= STRING",
 /* 114 */ "atom ::= NIL",
 /* 115 */ "atom ::= TRUE",
 /* 116 */ "atom ::= FALSE",
 /* 117 */ "atom ::= LINE",
 /* 118 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 119 */ "args_opt ::=",
 /* 120 */ "args_opt ::= args",
 /* 121 */ "blockarg_opt ::=",
 /* 122 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 123 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 124 */ "blockarg_params_opt ::=",
 /* 125 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 126 */ "excepts ::= except",
 /* 127 */ "excepts ::= excepts except",
 /* 128 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 129 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 130 */ "except ::= EXCEPT NEWLINE stmts",
 /* 131 */ "finally_opt ::=",
 /* 132 */ "finally_opt ::= FINALLY stmts",
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
  { 48, 1 },
  { 49, 1 },
  { 49, 3 },
  { 50, 0 },
  { 50, 1 },
  { 50, 1 },
  { 50, 7 },
  { 50, 5 },
  { 50, 5 },
  { 50, 5 },
  { 50, 1 },
  { 50, 2 },
  { 50, 1 },
  { 50, 2 },
  { 50, 1 },
  { 50, 2 },
  { 50, 6 },
  { 50, 6 },
  { 50, 2 },
  { 50, 2 },
  { 58, 1 },
  { 58, 3 },
  { 59, 1 },
  { 59, 3 },
  { 57, 1 },
  { 57, 3 },
  { 56, 0 },
  { 56, 2 },
  { 55, 1 },
  { 55, 5 },
  { 60, 0 },
  { 60, 2 },
  { 51, 7 },
  { 61, 9 },
  { 61, 7 },
  { 61, 7 },
  { 61, 5 },
  { 61, 7 },
  { 61, 5 },
  { 61, 5 },
  { 61, 3 },
  { 61, 7 },
  { 61, 5 },
  { 61, 5 },
  { 61, 3 },
  { 61, 5 },
  { 61, 3 },
  { 61, 3 },
  { 61, 1 },
  { 61, 7 },
  { 61, 5 },
  { 61, 5 },
  { 61, 3 },
  { 61, 5 },
  { 61, 3 },
  { 61, 3 },
  { 61, 1 },
  { 61, 5 },
  { 61, 3 },
  { 61, 3 },
  { 61, 1 },
  { 61, 3 },
  { 61, 1 },
  { 61, 1 },
  { 61, 0 },
  { 66, 2 },
  { 65, 2 },
  { 64, 3 },
  { 67, 0 },
  { 67, 1 },
  { 68, 2 },
  { 62, 1 },
  { 62, 3 },
  { 63, 1 },
  { 63, 3 },
  { 69, 2 },
  { 70, 1 },
  { 70, 3 },
  { 52, 1 },
  { 71, 3 },
  { 71, 1 },
  { 73, 1 },
  { 74, 1 },
  { 75, 1 },
  { 76, 1 },
  { 76, 3 },
  { 78, 1 },
  { 77, 1 },
  { 79, 1 },
  { 80, 1 },
  { 81, 1 },
  { 81, 3 },
  { 82, 1 },
  { 82, 3 },
  { 83, 1 },
  { 83, 3 },
  { 85, 1 },
  { 85, 1 },
  { 84, 3 },
  { 84, 1 },
  { 86, 1 },
  { 86, 1 },
  { 86, 1 },
  { 87, 2 },
  { 87, 1 },
  { 88, 1 },
  { 72, 1 },
  { 72, 5 },
  { 72, 4 },
  { 72, 3 },
  { 89, 1 },
  { 89, 1 },
  { 89, 1 },
  { 89, 1 },
  { 89, 1 },
  { 89, 1 },
  { 89, 1 },
  { 89, 1 },
  { 89, 3 },
  { 90, 0 },
  { 90, 1 },
  { 91, 0 },
  { 91, 5 },
  { 91, 5 },
  { 92, 0 },
  { 92, 3 },
  { 53, 1 },
  { 53, 2 },
  { 93, 6 },
  { 93, 4 },
  { 93, 3 },
  { 54, 0 },
  { 54, 2 },
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
#line 612 "parser.y"
{
    *pval = yymsp[0].minor.yy135;
}
#line 1846 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 126: /* excepts ::= except */
#line 616 "parser.y"
{
    yygotominor.yy135 = make_array_with(env, yymsp[0].minor.yy135);
}
#line 1857 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 619 "parser.y"
{
    yygotominor.yy135 = Array_push(env, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 1867 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 119: /* args_opt ::= */
      case 121: /* blockarg_opt ::= */
      case 124: /* blockarg_params_opt ::= */
      case 131: /* finally_opt ::= */
#line 623 "parser.y"
{
    yygotominor.yy135 = YNIL;
}
#line 1881 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 27: /* super_opt ::= GREATER expr */
      case 28: /* if_tail ::= else_opt */
      case 31: /* else_opt ::= ELSE stmts */
      case 69: /* param_default_opt ::= param_default */
      case 70: /* param_default ::= EQUAL expr */
      case 78: /* expr ::= assign_expr */
      case 80: /* assign_expr ::= logical_or_expr */
      case 81: /* logical_or_expr ::= logical_and_expr */
      case 82: /* logical_and_expr ::= not_expr */
      case 83: /* not_expr ::= comparison */
      case 84: /* comparison ::= xor_expr */
      case 87: /* xor_expr ::= or_expr */
      case 88: /* or_expr ::= and_expr */
      case 89: /* and_expr ::= shift_expr */
      case 90: /* shift_expr ::= match_expr */
      case 92: /* match_expr ::= arith_expr */
      case 94: /* arith_expr ::= term */
      case 99: /* term ::= factor */
      case 104: /* factor ::= power */
      case 105: /* power ::= postfix_expr */
      case 106: /* postfix_expr ::= atom */
      case 120: /* args_opt ::= args */
      case 132: /* finally_opt ::= FINALLY stmts */
#line 626 "parser.y"
{
    yygotominor.yy135 = yymsp[0].minor.yy135;
}
#line 1911 "parser.c"
        break;
      case 5: /* stmt ::= expr */
#line 629 "parser.y"
{
    if (PTR_AS(YogNode, yymsp[0].minor.yy135)->type == NODE_VARIABLE) {
        unsigned int lineno = NODE_LINENO(yymsp[0].minor.yy135);
        ID id = PTR_AS(YogNode, yymsp[0].minor.yy135)->u.variable.id;
        yygotominor.yy135 = CommandCall_new(env, lineno, id, YNIL, YNIL);
    }
    else {
        yygotominor.yy135 = yymsp[0].minor.yy135;
    }
}
#line 1925 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 639 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy135 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy135, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[-1].minor.yy135);
}
#line 1933 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 643 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy135 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-2].minor.yy135, YNIL, yymsp[-1].minor.yy135);
}
#line 1941 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 647 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy135 = Finally_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 1949 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 651 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy135 = While_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 1957 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 655 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Break_new(env, lineno, YNIL);
}
#line 1965 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 659 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy135 = Break_new(env, lineno, yymsp[0].minor.yy135);
}
#line 1973 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 663 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Next_new(env, lineno, YNIL);
}
#line 1981 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 667 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy135 = Next_new(env, lineno, yymsp[0].minor.yy135);
}
#line 1989 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 671 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Return_new(env, lineno, YNIL);
}
#line 1997 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 675 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy135 = Return_new(env, lineno, yymsp[0].minor.yy135);
}
#line 2005 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 679 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy135 = If_new(env, lineno, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[-1].minor.yy135);
}
#line 2013 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 683 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy135 = Klass_new(env, lineno, id, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 2022 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 688 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy135 = Nonlocal_new(env, lineno, yymsp[0].minor.yy135);
}
#line 2030 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 692 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy135 = Import_new(env, lineno, yymsp[0].minor.yy135);
}
#line 2038 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 704 "parser.y"
{
    yygotominor.yy135 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2046 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 707 "parser.y"
{
    yygotominor.yy135 = Array_push_token_id(env, yymsp[-2].minor.yy135, yymsp[0].minor.yy0);
}
#line 2054 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 728 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135, yymsp[0].minor.yy135);
    yygotominor.yy135 = make_array_with(env, node);
}
#line 2063 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 741 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy135 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 2072 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 747 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-8].minor.yy135, yymsp[-6].minor.yy135, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2079 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 750 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-6].minor.yy135, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL);
}
#line 2086 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 753 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-6].minor.yy135, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135);
}
#line 2093 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 756 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL, YNIL);
}
#line 2100 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 759 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-6].minor.yy135, yymsp[-4].minor.yy135, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2107 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 762 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135, YNIL);
}
#line 2114 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 765 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, YNIL, YNIL, yymsp[0].minor.yy135);
}
#line 2121 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 768 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL, YNIL, YNIL);
}
#line 2128 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 771 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-6].minor.yy135, YNIL, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2135 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 774 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL);
}
#line 2142 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 777 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, YNIL, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135);
}
#line 2149 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 780 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135, YNIL, YNIL);
}
#line 2156 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 783 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, YNIL, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2163 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 786 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-2].minor.yy135, YNIL, YNIL, yymsp[0].minor.yy135, YNIL);
}
#line 2170 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 789 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-2].minor.yy135, YNIL, YNIL, YNIL, yymsp[0].minor.yy135);
}
#line 2177 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 792 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[0].minor.yy135, YNIL, YNIL, YNIL, YNIL);
}
#line 2184 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 795 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-6].minor.yy135, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2191 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 798 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL);
}
#line 2198 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 801 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135);
}
#line 2205 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 804 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL, YNIL);
}
#line 2212 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 807 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-4].minor.yy135, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2219 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 810 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135, YNIL);
}
#line 2226 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 813 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-2].minor.yy135, YNIL, YNIL, yymsp[0].minor.yy135);
}
#line 2233 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 816 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[0].minor.yy135, YNIL, YNIL, YNIL);
}
#line 2240 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 819 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2247 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 822 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL);
}
#line 2254 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 825 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135);
}
#line 2261 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 828 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy135, YNIL, YNIL);
}
#line 2268 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 831 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2275 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 834 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy135, YNIL);
}
#line 2282 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 837 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy135);
}
#line 2289 "parser.c"
        break;
      case 64: /* params ::= */
#line 840 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2296 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 844 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy135 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2305 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 850 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy135 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2314 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 856 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy135 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy135);
}
#line 2323 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 873 "parser.y"
{
    yygotominor.yy135 = YogArray_new(env);
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy135, lineno, id, YNIL);
}
#line 2333 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 879 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy135, lineno, id, YNIL);
    yygotominor.yy135 = yymsp[-2].minor.yy135;
}
#line 2343 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 893 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy135 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy135);
}
#line 2352 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 910 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy135);
    yygotominor.yy135 = Assign_new(env, lineno, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2360 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 933 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy135);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy135)->u.id;
    yygotominor.yy135 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy135, id, yymsp[0].minor.yy135);
}
#line 2369 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
#line 939 "parser.y"
{
    yygotominor.yy135 = yymsp[0].minor.yy0;
}
#line 2376 "parser.c"
        break;
      case 91: /* shift_expr ::= shift_expr LSHIFT match_expr */
      case 93: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 958 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy135);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy135 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy135, id, yymsp[0].minor.yy135);
}
#line 2386 "parser.c"
        break;
      case 95: /* arith_expr ::= arith_expr arith_op term */
      case 98: /* term ::= term term_op factor */
#line 976 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy135);
    yygotominor.yy135 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy135, VAL2ID(yymsp[-1].minor.yy135), yymsp[0].minor.yy135);
}
#line 2395 "parser.c"
        break;
      case 96: /* arith_op ::= PLUS */
      case 97: /* arith_op ::= MINUS */
      case 100: /* term_op ::= STAR */
      case 101: /* term_op ::= DIV */
      case 102: /* term_op ::= DIV_DIV */
#line 981 "parser.y"
{
    yygotominor.yy135 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2406 "parser.c"
        break;
      case 103: /* factor ::= MINUS factor */
#line 1006 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy135 = FuncCall_new3(env, lineno, yymsp[0].minor.yy135, id);
}
#line 2415 "parser.c"
        break;
      case 107: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1022 "parser.y"
{
    yygotominor.yy135 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy135), yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2422 "parser.c"
        break;
      case 108: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1025 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-3].minor.yy135);
    yygotominor.yy135 = Subscript_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 2430 "parser.c"
        break;
      case 109: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1029 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy135);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy135 = Attr_new(env, lineno, yymsp[-2].minor.yy135, id);
}
#line 2439 "parser.c"
        break;
      case 110: /* atom ::= NAME */
#line 1035 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy135 = Variable_new(env, lineno, id);
}
#line 2448 "parser.c"
        break;
      case 111: /* atom ::= NUMBER */
      case 112: /* atom ::= REGEXP */
      case 113: /* atom ::= STRING */
#line 1040 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy135 = Literal_new(env, lineno, val);
}
#line 2459 "parser.c"
        break;
      case 114: /* atom ::= NIL */
#line 1055 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Literal_new(env, lineno, YNIL);
}
#line 2467 "parser.c"
        break;
      case 115: /* atom ::= TRUE */
#line 1059 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Literal_new(env, lineno, YTRUE);
}
#line 2475 "parser.c"
        break;
      case 116: /* atom ::= FALSE */
#line 1063 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Literal_new(env, lineno, YFALSE);
}
#line 2483 "parser.c"
        break;
      case 117: /* atom ::= LINE */
#line 1067 "parser.y"
{
    unsigned int lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy135 = Literal_new(env, lineno, val);
}
#line 2492 "parser.c"
        break;
      case 118: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1072 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy135 = Array_new(env, lineno, yymsp[-1].minor.yy135);
}
#line 2500 "parser.c"
        break;
      case 122: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 123: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1087 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy135 = BlockArg_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 2509 "parser.c"
        break;
      case 125: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1099 "parser.y"
{
    yygotominor.yy135 = yymsp[-1].minor.yy135;
}
#line 2516 "parser.c"
        break;
      case 127: /* excepts ::= excepts except */
#line 1106 "parser.y"
{
    yygotominor.yy135 = Array_push(env, yymsp[-1].minor.yy135, yymsp[0].minor.yy135);
}
#line 2523 "parser.c"
        break;
      case 128: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1110 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy135 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy135, id, yymsp[0].minor.yy135);
}
#line 2533 "parser.c"
        break;
      case 129: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1116 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy135 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy135, NO_EXC_VAR, yymsp[0].minor.yy135);
}
#line 2541 "parser.c"
        break;
      case 130: /* except ::= EXCEPT NEWLINE stmts */
#line 1120 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy135 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy135);
}
#line 2549 "parser.c"
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
