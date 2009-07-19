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
#define YYNOCODE 94
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy175;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 221
#define YYNRULE 132
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
 /*     0 */   354,   77,  141,  139,  140,   76,  146,  147,  148,  149,
 /*    10 */    58,   43,  151,  152,   88,   91,   55,   56,  113,  189,
 /*    20 */   163,  155,  164,  145,   75,  158,  147,  148,  149,   58,
 /*    30 */   132,  151,  152,   88,   91,   55,   56,  185,    1,  163,
 /*    40 */   155,  164,   20,   21,   23,   24,   25,   99,  165,   61,
 /*    50 */    50,  220,   14,  114,  115,  107,  109,  110,  121,  125,
 /*    60 */   127,  210,   27,  165,  202,    2,   39,    3,   18,  218,
 /*    70 */   166,  167,  168,  169,  170,  171,  172,   42,  141,  139,
 /*    80 */   140,   39,   16,   18,   36,  166,  167,  168,  169,  170,
 /*    90 */   171,  172,  111,  118,  120,  201,  119,  199,  202,  145,
 /*   100 */    75,  158,  147,  148,  149,   58,   19,  151,  152,   88,
 /*   110 */    91,   55,   56,  123,  204,  163,  155,  164,   76,   52,
 /*   120 */   141,  139,  140,  150,  213,  151,  152,   88,   91,   55,
 /*   130 */    56,   96,  180,  163,  155,  164,  179,  179,  126,  208,
 /*   140 */    85,  145,   75,  158,  147,  148,  149,   58,  165,  151,
 /*   150 */   152,   88,   91,   55,   56,  103,  106,  163,  155,  164,
 /*   160 */    86,  141,  139,  140,   38,  131,   39,   73,   18,   17,
 /*   170 */   166,  167,  168,  169,  170,  171,  172,  114,  115,  117,
 /*   180 */   174,   29,  145,   75,  158,  147,  148,  149,   58,  216,
 /*   190 */   151,  152,   88,   91,   55,   56,   31,   76,  163,  155,
 /*   200 */   164,   78,  141,  139,  140,   76,   34,   89,   55,   56,
 /*   210 */   193,  194,  163,  155,  164,   16,   53,   56,   94,   15,
 /*   220 */   163,  155,  164,  145,   75,  158,  147,  148,  149,   58,
 /*   230 */    28,  151,  152,   88,   91,   55,   56,   59,   74,  163,
 /*   240 */   155,  164,   79,  141,  139,  140,  128,  110,  121,  125,
 /*   250 */   127,  210,  159,  160,  202,  112,  116,  192,   19,   16,
 /*   260 */   196,  221,   16,    3,  145,   75,  158,  147,  148,  149,
 /*   270 */    58,  100,  151,  152,   88,   91,   55,   56,   68,   76,
 /*   280 */   163,  155,  164,   44,  141,  139,  140,   65,  114,  115,
 /*   290 */   117,   54,  161,  183,  163,  155,  164,  114,  115,  117,
 /*   300 */   162,   16,  101,  104,  143,  145,   75,  158,  147,  148,
 /*   310 */   149,   58,  114,  151,  152,   88,   91,   55,   56,   76,
 /*   320 */   187,  163,  155,  164,   45,  141,  139,  140,  122,  124,
 /*   330 */   206,  197,  191,  196,  153,  155,  164,   16,   16,   16,
 /*   340 */   175,  181,  186,  198,  200,  203,  145,   75,  158,  147,
 /*   350 */   148,  149,   58,  205,  151,  152,   88,   91,   55,   56,
 /*   360 */    76,  207,  163,  155,  164,   98,  141,  139,  140,  131,
 /*   370 */    13,   16,  209,   17,  219,  154,  155,  164,   30,  142,
 /*   380 */    16,    4,  129,   32,  173,   29,   22,  145,   75,  158,
 /*   390 */   147,  148,  149,   58,   35,  151,  152,   88,   91,   55,
 /*   400 */    56,    5,   57,  163,  155,  164,   80,  141,  139,  140,
 /*   410 */    16,    6,    8,    7,  133,  178,   60,    9,  102,  182,
 /*   420 */    62,   33,  105,   10,  212,  184,  108,   26,  145,   75,
 /*   430 */   158,  147,  148,  149,   58,   37,  151,  152,   88,   91,
 /*   440 */    55,   56,   40,   46,  163,  155,  164,   81,  141,  139,
 /*   450 */   140,   63,   64,  188,  190,   51,   11,  214,  217,   47,
 /*   460 */    66,   67,   41,   48,   69,   70,   49,   71,   72,  145,
 /*   470 */    75,  158,  147,  148,  149,   58,  215,  151,  152,   88,
 /*   480 */    91,   55,   56,  134,   12,  163,  155,  164,   82,  141,
 /*   490 */   139,  140,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   500 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   510 */   145,   75,  158,  147,  148,  149,   58,  355,  151,  152,
 /*   520 */    88,   91,   55,   56,  355,  355,  163,  155,  164,  135,
 /*   530 */   141,  139,  140,  355,  355,  355,  355,  355,  355,  355,
 /*   540 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   550 */   355,  145,   75,  158,  147,  148,  149,   58,  355,  151,
 /*   560 */   152,   88,   91,   55,   56,  355,  355,  163,  155,  164,
 /*   570 */   136,  141,  139,  140,  355,  355,  355,  355,  355,  355,
 /*   580 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   590 */   355,  355,  145,   75,  158,  147,  148,  149,   58,  355,
 /*   600 */   151,  152,   88,   91,   55,   56,  355,  355,  163,  155,
 /*   610 */   164,  137,  141,  139,  140,  355,  355,  355,  355,  355,
 /*   620 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   630 */   355,  355,  355,  145,   75,  158,  147,  148,  149,   58,
 /*   640 */   355,  151,  152,   88,   91,   55,   56,  355,  355,  163,
 /*   650 */   155,  164,   84,  141,  139,  140,  355,  355,  355,  355,
 /*   660 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   670 */   355,  355,  355,  355,  145,   75,  158,  147,  148,  149,
 /*   680 */    58,  355,  151,  152,   88,   91,   55,   56,  355,  355,
 /*   690 */   163,  155,  164,  138,  139,  140,  355,  355,  355,  355,
 /*   700 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   710 */   355,  355,  355,  355,  145,   75,  158,  147,  148,  149,
 /*   720 */    58,  355,  151,  152,   88,   91,   55,   56,  156,  355,
 /*   730 */   163,  155,  164,  355,  355,  355,  355,  355,  355,  355,
 /*   740 */   355,  355,  355,  355,  355,  355,   90,  145,   75,  158,
 /*   750 */   147,  148,  149,   58,  355,  151,  152,   88,   91,   55,
 /*   760 */    56,  355,  355,  163,  155,  164,   93,  156,  355,  355,
 /*   770 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   780 */   355,  355,  355,  355,  355,   90,  145,   75,  158,  147,
 /*   790 */   148,  149,   58,  355,  151,  152,   88,   91,   55,   56,
 /*   800 */    83,  355,  163,  155,  164,   92,  355,  355,  355,  355,
 /*   810 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  145,
 /*   820 */    75,  158,  147,  148,  149,   58,  355,  151,  152,   88,
 /*   830 */    91,   55,   56,  355,  355,  163,  155,  164,   87,  355,
 /*   840 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   850 */   355,  355,  355,  355,  355,  355,  355,  145,   75,  158,
 /*   860 */   147,  148,  149,   58,  355,  151,  152,   88,   91,   55,
 /*   870 */    56,  144,  355,  163,  155,  164,  355,  355,  355,  355,
 /*   880 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   890 */   145,   75,  158,  147,  148,  149,   58,  355,  151,  152,
 /*   900 */    88,   91,   55,   56,  355,  355,  163,  155,  164,  157,
 /*   910 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   920 */   355,  355,  355,  355,  355,  355,  355,  355,  145,   75,
 /*   930 */   158,  147,  148,  149,   58,  355,  151,  152,   88,   91,
 /*   940 */    55,   56,  176,  355,  163,  155,  164,  355,  355,  355,
 /*   950 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   960 */   355,  145,   75,  158,  147,  148,  149,   58,  355,  151,
 /*   970 */   152,   88,   91,   55,   56,  355,  355,  163,  155,  164,
 /*   980 */   177,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*   990 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  145,
 /*  1000 */    75,  158,  147,  148,  149,   58,  355,  151,  152,   88,
 /*  1010 */    91,   55,   56,   95,  355,  163,  155,  164,  355,  355,
 /*  1020 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*  1030 */   355,  355,  145,   75,  158,  147,  148,  149,   58,  355,
 /*  1040 */   151,  152,   88,   91,   55,   56,  355,  355,  163,  155,
 /*  1050 */   164,   97,  355,  355,  355,  355,  355,  355,  355,  355,
 /*  1060 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*  1070 */   145,   75,  158,  147,  148,  149,   58,  355,  151,  152,
 /*  1080 */    88,   91,   55,   56,  195,  355,  163,  155,  164,  355,
 /*  1090 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*  1100 */   355,  355,  355,  145,   75,  158,  147,  148,  149,   58,
 /*  1110 */   355,  151,  152,   88,   91,   55,   56,  355,  355,  163,
 /*  1120 */   155,  164,  211,  355,  355,  355,  355,  355,  355,  355,
 /*  1130 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*  1140 */   355,  145,   75,  158,  147,  148,  149,   58,  355,  151,
 /*  1150 */   152,   88,   91,   55,   56,  130,  355,  163,  155,  164,
 /*  1160 */   355,  355,  355,  355,  355,  355,  355,  355,  355,  355,
 /*  1170 */   355,  355,  355,  355,  145,   75,  158,  147,  148,  149,
 /*  1180 */    58,  355,  151,  152,   88,   91,   55,   56,  355,  355,
 /*  1190 */   163,  155,  164,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    47,   48,   49,   50,   51,   71,   72,   73,   74,   75,
 /*    10 */    76,   52,   78,   79,   80,   81,   82,   83,   64,   65,
 /*    20 */    86,   87,   88,   70,   71,   72,   73,   74,   75,   76,
 /*    30 */    53,   78,   79,   80,   81,   82,   83,   12,    2,   86,
 /*    40 */    87,   88,    6,    7,    8,    9,   10,   11,   12,   13,
 /*    50 */    14,   92,    1,   22,   23,   19,   60,   61,   62,   63,
 /*    60 */    64,   65,   25,   12,   68,    3,   30,    5,   32,   92,
 /*    70 */    34,   35,   36,   37,   38,   39,   40,   48,   49,   50,
 /*    80 */    51,   30,    1,   32,   84,   34,   35,   36,   37,   38,
 /*    90 */    39,   40,   62,   63,   64,   65,   64,   65,   68,   70,
 /*   100 */    71,   72,   73,   74,   75,   76,   44,   78,   79,   80,
 /*   110 */    81,   82,   83,   64,   65,   86,   87,   88,   71,   48,
 /*   120 */    49,   50,   51,   76,   43,   78,   79,   80,   81,   82,
 /*   130 */    83,   54,   54,   86,   87,   88,   59,   59,   64,   65,
 /*   140 */    53,   70,   71,   72,   73,   74,   75,   76,   12,   78,
 /*   150 */    79,   80,   81,   82,   83,   57,   58,   86,   87,   88,
 /*   160 */    48,   49,   50,   51,   85,   16,   30,   12,   32,   20,
 /*   170 */    34,   35,   36,   37,   38,   39,   40,   22,   23,   24,
 /*   180 */    90,   32,   70,   71,   72,   73,   74,   75,   76,   26,
 /*   190 */    78,   79,   80,   81,   82,   83,   77,   71,   86,   87,
 /*   200 */    88,   48,   49,   50,   51,   71,   32,   81,   82,   83,
 /*   210 */    66,   67,   86,   87,   88,    1,   82,   83,   91,    5,
 /*   220 */    86,   87,   88,   70,   71,   72,   73,   74,   75,   76,
 /*   230 */    17,   78,   79,   80,   81,   82,   83,   41,   42,   86,
 /*   240 */    87,   88,   48,   49,   50,   51,   60,   61,   62,   63,
 /*   250 */    64,   65,   29,   30,   68,   63,   64,   65,   44,    1,
 /*   260 */    68,    0,    1,    5,   70,   71,   72,   73,   74,   75,
 /*   270 */    76,   55,   78,   79,   80,   81,   82,   83,   12,   71,
 /*   280 */    86,   87,   88,   48,   49,   50,   51,   12,   22,   23,
 /*   290 */    24,   83,   23,   12,   86,   87,   88,   22,   23,   24,
 /*   300 */    31,    1,   56,   58,    4,   70,   71,   72,   73,   74,
 /*   310 */    75,   76,   22,   78,   79,   80,   81,   82,   83,   71,
 /*   320 */    65,   86,   87,   88,   48,   49,   50,   51,   63,   64,
 /*   330 */    65,   67,   65,   68,   86,   87,   88,    1,    1,    1,
 /*   340 */     4,    4,    4,   65,   65,   65,   70,   71,   72,   73,
 /*   350 */    74,   75,   76,   65,   78,   79,   80,   81,   82,   83,
 /*   360 */    71,   65,   86,   87,   88,   48,   49,   50,   51,   16,
 /*   370 */     1,    1,   65,   20,    4,   86,   87,   88,   25,    4,
 /*   380 */     1,    1,   91,   27,   33,   32,   15,   70,   71,   72,
 /*   390 */    73,   74,   75,   76,   28,   78,   79,   80,   81,   82,
 /*   400 */    83,    1,   21,   86,   87,   88,   48,   49,   50,   51,
 /*   410 */     1,    1,    3,    1,   45,    4,   12,    1,   15,   12,
 /*   420 */    15,   20,   16,   21,   33,   12,   12,   18,   70,   71,
 /*   430 */    72,   73,   74,   75,   76,   15,   78,   79,   80,   81,
 /*   440 */    82,   83,   15,   15,   86,   87,   88,   48,   49,   50,
 /*   450 */    51,   15,   15,   12,   12,   12,    1,   33,    4,   15,
 /*   460 */    15,   15,   15,   15,   15,   15,   15,   15,   15,   70,
 /*   470 */    71,   72,   73,   74,   75,   76,   12,   78,   79,   80,
 /*   480 */    81,   82,   83,   12,    1,   86,   87,   88,   48,   49,
 /*   490 */    50,   51,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   500 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   510 */    70,   71,   72,   73,   74,   75,   76,   93,   78,   79,
 /*   520 */    80,   81,   82,   83,   93,   93,   86,   87,   88,   48,
 /*   530 */    49,   50,   51,   93,   93,   93,   93,   93,   93,   93,
 /*   540 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   550 */    93,   70,   71,   72,   73,   74,   75,   76,   93,   78,
 /*   560 */    79,   80,   81,   82,   83,   93,   93,   86,   87,   88,
 /*   570 */    48,   49,   50,   51,   93,   93,   93,   93,   93,   93,
 /*   580 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   590 */    93,   93,   70,   71,   72,   73,   74,   75,   76,   93,
 /*   600 */    78,   79,   80,   81,   82,   83,   93,   93,   86,   87,
 /*   610 */    88,   48,   49,   50,   51,   93,   93,   93,   93,   93,
 /*   620 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   630 */    93,   93,   93,   70,   71,   72,   73,   74,   75,   76,
 /*   640 */    93,   78,   79,   80,   81,   82,   83,   93,   93,   86,
 /*   650 */    87,   88,   48,   49,   50,   51,   93,   93,   93,   93,
 /*   660 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   670 */    93,   93,   93,   93,   70,   71,   72,   73,   74,   75,
 /*   680 */    76,   93,   78,   79,   80,   81,   82,   83,   93,   93,
 /*   690 */    86,   87,   88,   49,   50,   51,   93,   93,   93,   93,
 /*   700 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   710 */    93,   93,   93,   93,   70,   71,   72,   73,   74,   75,
 /*   720 */    76,   93,   78,   79,   80,   81,   82,   83,   51,   93,
 /*   730 */    86,   87,   88,   93,   93,   93,   93,   93,   93,   93,
 /*   740 */    93,   93,   93,   93,   93,   93,   69,   70,   71,   72,
 /*   750 */    73,   74,   75,   76,   93,   78,   79,   80,   81,   82,
 /*   760 */    83,   93,   93,   86,   87,   88,   89,   51,   93,   93,
 /*   770 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   780 */    93,   93,   93,   93,   93,   69,   70,   71,   72,   73,
 /*   790 */    74,   75,   76,   93,   78,   79,   80,   81,   82,   83,
 /*   800 */    51,   93,   86,   87,   88,   89,   93,   93,   93,   93,
 /*   810 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   70,
 /*   820 */    71,   72,   73,   74,   75,   76,   93,   78,   79,   80,
 /*   830 */    81,   82,   83,   93,   93,   86,   87,   88,   51,   93,
 /*   840 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   850 */    93,   93,   93,   93,   93,   93,   93,   70,   71,   72,
 /*   860 */    73,   74,   75,   76,   93,   78,   79,   80,   81,   82,
 /*   870 */    83,   51,   93,   86,   87,   88,   93,   93,   93,   93,
 /*   880 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   890 */    70,   71,   72,   73,   74,   75,   76,   93,   78,   79,
 /*   900 */    80,   81,   82,   83,   93,   93,   86,   87,   88,   51,
 /*   910 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   920 */    93,   93,   93,   93,   93,   93,   93,   93,   70,   71,
 /*   930 */    72,   73,   74,   75,   76,   93,   78,   79,   80,   81,
 /*   940 */    82,   83,   51,   93,   86,   87,   88,   93,   93,   93,
 /*   950 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   960 */    93,   70,   71,   72,   73,   74,   75,   76,   93,   78,
 /*   970 */    79,   80,   81,   82,   83,   93,   93,   86,   87,   88,
 /*   980 */    51,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*   990 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   70,
 /*  1000 */    71,   72,   73,   74,   75,   76,   93,   78,   79,   80,
 /*  1010 */    81,   82,   83,   51,   93,   86,   87,   88,   93,   93,
 /*  1020 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*  1030 */    93,   93,   70,   71,   72,   73,   74,   75,   76,   93,
 /*  1040 */    78,   79,   80,   81,   82,   83,   93,   93,   86,   87,
 /*  1050 */    88,   51,   93,   93,   93,   93,   93,   93,   93,   93,
 /*  1060 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*  1070 */    70,   71,   72,   73,   74,   75,   76,   93,   78,   79,
 /*  1080 */    80,   81,   82,   83,   51,   93,   86,   87,   88,   93,
 /*  1090 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*  1100 */    93,   93,   93,   70,   71,   72,   73,   74,   75,   76,
 /*  1110 */    93,   78,   79,   80,   81,   82,   83,   93,   93,   86,
 /*  1120 */    87,   88,   51,   93,   93,   93,   93,   93,   93,   93,
 /*  1130 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*  1140 */    93,   70,   71,   72,   73,   74,   75,   76,   93,   78,
 /*  1150 */    79,   80,   81,   82,   83,   51,   93,   86,   87,   88,
 /*  1160 */    93,   93,   93,   93,   93,   93,   93,   93,   93,   93,
 /*  1170 */    93,   93,   93,   93,   70,   71,   72,   73,   74,   75,
 /*  1180 */    76,   93,   78,   79,   80,   81,   82,   83,   93,   93,
 /*  1190 */    86,   87,   88,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 137
static const short yy_shift_ofst[] = {
 /*     0 */    36,   36,   36,   36,   36,   36,   36,   36,   36,   36,
 /*    10 */    36,   36,   36,   36,   36,   36,   36,  136,  136,   51,
 /*    20 */   136,  136,  136,  136,  136,  136,  136,  136,  136,  136,
 /*    30 */   136,  136,  136,  155,  155,  136,  136,  266,  136,  136,
 /*    40 */   275,  275,  214,   62,  409,  409,   31,   31,   31,   31,
 /*    50 */    25,   37,  258,  223,  269,  223,  269,  196,  163,  174,
 /*    60 */   213,  281,   25,  290,  290,   37,  290,  290,   37,  290,
 /*    70 */   290,  290,  290,   37,  174,  353,  149,  261,  300,  336,
 /*    80 */   337,  338,   81,  369,  370,  375,  379,  380,  356,  366,
 /*    90 */   371,  366,  351,  381,  400,  410,  411,  412,  379,  404,
 /*   100 */   416,  403,  407,  405,  406,  413,  406,  414,  401,  402,
 /*   110 */   420,  427,  428,  436,  441,  442,  437,  443,  444,  445,
 /*   120 */   446,  447,  448,  449,  450,  451,  452,  453,  391,  455,
 /*   130 */   424,  464,  454,  471,  483,  379,  379,  379,
};
#define YY_REDUCE_USE_DFLT (-67)
#define YY_REDUCE_MAX 74
static const short yy_reduce_ofst[] = {
 /*     0 */   -47,   29,   71,  112,  153,  194,  235,  276,  317,  358,
 /*    10 */   399,  440,  481,  522,  563,  604,  644,  677,  716,  749,
 /*    20 */   787,  820,  858,  891,  929,  962, 1000, 1033, 1071, 1104,
 /*    30 */   -66,   47,  126,   -4,  186,  134,  208,   30,  248,  289,
 /*    40 */   192,  265,  -41,  -23,   77,   78,  -46,   32,   49,   74,
 /*    50 */    98,  144,   87,    0,   79,    0,   79,   90,  119,  127,
 /*    60 */   216,  246,  245,  255,  267,  264,  278,  279,  264,  280,
 /*    70 */   288,  296,  307,  264,  291,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   224,  224,  224,  224,  224,  224,  224,  224,  224,  224,
 /*    10 */   224,  224,  224,  224,  224,  224,  224,  339,  339,  353,
 /*    20 */   353,  231,  353,  233,  235,  353,  353,  353,  353,  353,
 /*    30 */   353,  353,  353,  285,  285,  353,  353,  353,  353,  353,
 /*    40 */   353,  353,  353,  351,  251,  251,  353,  353,  353,  353,
 /*    50 */   353,  289,  351,  314,  316,  313,  315,  341,  305,  344,
 /*    60 */   247,  353,  353,  353,  353,  353,  353,  353,  293,  353,
 /*    70 */   353,  353,  353,  292,  344,  325,  325,  353,  353,  353,
 /*    80 */   353,  353,  353,  353,  353,  353,  352,  353,  310,  312,
 /*    90 */   340,  311,  353,  353,  353,  353,  353,  353,  252,  353,
 /*   100 */   353,  239,  353,  240,  242,  353,  241,  353,  353,  353,
 /*   110 */   269,  261,  257,  255,  353,  353,  259,  353,  265,  263,
 /*   120 */   267,  277,  273,  271,  275,  281,  279,  283,  353,  353,
 /*   130 */   353,  353,  353,  353,  353,  348,  349,  350,  223,  225,
 /*   140 */   226,  222,  227,  230,  232,  299,  300,  302,  303,  304,
 /*   150 */   306,  308,  309,  319,  323,  324,  297,  298,  301,  317,
 /*   160 */   318,  321,  322,  320,  326,  330,  331,  332,  333,  334,
 /*   170 */   335,  336,  337,  338,  327,  342,  234,  236,  237,  249,
 /*   180 */   250,  238,  246,  245,  244,  243,  253,  254,  286,  256,
 /*   190 */   287,  258,  260,  288,  290,  291,  295,  296,  262,  264,
 /*   200 */   266,  268,  294,  270,  272,  274,  276,  278,  280,  282,
 /*   210 */   284,  248,  345,  343,  328,  329,  307,  228,  347,  229,
 /*   220 */   346,
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
  "LBRACKET",      "RBRACKET",      "NUMBER",        "REGEXP",      
  "STRING",        "NIL",           "TRUE",          "FALSE",       
  "LINE",          "DO",            "LBRACE",        "RBRACE",      
  "EXCEPT",        "AS",            "error",         "module",      
  "stmts",         "stmt",          "func_def",      "expr",        
  "excepts",       "finally_opt",   "if_tail",       "super_opt",   
  "names",         "dotted_names",  "dotted_name",   "else_opt",    
  "params",        "params_without_default",  "params_with_default",  "block_param", 
  "var_param",     "kw_param",      "param_default_opt",  "param_default",
  "param_with_default",  "args",          "assign_expr",   "postfix_expr",
  "logical_or_expr",  "logical_and_expr",  "not_expr",      "comparison",  
  "xor_expr",      "comp_op",       "or_expr",       "and_expr",    
  "shift_expr",    "match_expr",    "arith_expr",    "term",        
  "arith_op",      "term_op",       "factor",        "power",       
  "atom",          "args_opt",      "blockarg_opt",  "blockarg_params_opt",
  "except",      
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
 /* 102 */ "factor ::= MINUS factor",
 /* 103 */ "factor ::= power",
 /* 104 */ "power ::= postfix_expr",
 /* 105 */ "postfix_expr ::= atom",
 /* 106 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 107 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 108 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 109 */ "atom ::= NAME",
 /* 110 */ "atom ::= NUMBER",
 /* 111 */ "atom ::= REGEXP",
 /* 112 */ "atom ::= STRING",
 /* 113 */ "atom ::= NIL",
 /* 114 */ "atom ::= TRUE",
 /* 115 */ "atom ::= FALSE",
 /* 116 */ "atom ::= LINE",
 /* 117 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 118 */ "args_opt ::=",
 /* 119 */ "args_opt ::= args",
 /* 120 */ "blockarg_opt ::=",
 /* 121 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 122 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 123 */ "blockarg_params_opt ::=",
 /* 124 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 125 */ "excepts ::= except",
 /* 126 */ "excepts ::= excepts except",
 /* 127 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 128 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 129 */ "except ::= EXCEPT NEWLINE stmts",
 /* 130 */ "finally_opt ::=",
 /* 131 */ "finally_opt ::= FINALLY stmts",
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
  { 47, 1 },
  { 48, 1 },
  { 48, 3 },
  { 49, 0 },
  { 49, 1 },
  { 49, 1 },
  { 49, 7 },
  { 49, 5 },
  { 49, 5 },
  { 49, 5 },
  { 49, 1 },
  { 49, 2 },
  { 49, 1 },
  { 49, 2 },
  { 49, 1 },
  { 49, 2 },
  { 49, 6 },
  { 49, 6 },
  { 49, 2 },
  { 49, 2 },
  { 57, 1 },
  { 57, 3 },
  { 58, 1 },
  { 58, 3 },
  { 56, 1 },
  { 56, 3 },
  { 55, 0 },
  { 55, 2 },
  { 54, 1 },
  { 54, 5 },
  { 59, 0 },
  { 59, 2 },
  { 50, 7 },
  { 60, 9 },
  { 60, 7 },
  { 60, 7 },
  { 60, 5 },
  { 60, 7 },
  { 60, 5 },
  { 60, 5 },
  { 60, 3 },
  { 60, 7 },
  { 60, 5 },
  { 60, 5 },
  { 60, 3 },
  { 60, 5 },
  { 60, 3 },
  { 60, 3 },
  { 60, 1 },
  { 60, 7 },
  { 60, 5 },
  { 60, 5 },
  { 60, 3 },
  { 60, 5 },
  { 60, 3 },
  { 60, 3 },
  { 60, 1 },
  { 60, 5 },
  { 60, 3 },
  { 60, 3 },
  { 60, 1 },
  { 60, 3 },
  { 60, 1 },
  { 60, 1 },
  { 60, 0 },
  { 65, 2 },
  { 64, 2 },
  { 63, 3 },
  { 66, 0 },
  { 66, 1 },
  { 67, 2 },
  { 61, 1 },
  { 61, 3 },
  { 62, 1 },
  { 62, 3 },
  { 68, 2 },
  { 69, 1 },
  { 69, 3 },
  { 51, 1 },
  { 70, 3 },
  { 70, 1 },
  { 72, 1 },
  { 73, 1 },
  { 74, 1 },
  { 75, 1 },
  { 75, 3 },
  { 77, 1 },
  { 76, 1 },
  { 78, 1 },
  { 79, 1 },
  { 80, 1 },
  { 80, 3 },
  { 81, 1 },
  { 81, 3 },
  { 82, 1 },
  { 82, 3 },
  { 84, 1 },
  { 84, 1 },
  { 83, 3 },
  { 83, 1 },
  { 85, 1 },
  { 85, 1 },
  { 86, 2 },
  { 86, 1 },
  { 87, 1 },
  { 71, 1 },
  { 71, 5 },
  { 71, 4 },
  { 71, 3 },
  { 88, 1 },
  { 88, 1 },
  { 88, 1 },
  { 88, 1 },
  { 88, 1 },
  { 88, 1 },
  { 88, 1 },
  { 88, 1 },
  { 88, 3 },
  { 89, 0 },
  { 89, 1 },
  { 90, 0 },
  { 90, 5 },
  { 90, 5 },
  { 91, 0 },
  { 91, 3 },
  { 52, 1 },
  { 52, 2 },
  { 92, 6 },
  { 92, 4 },
  { 92, 3 },
  { 53, 0 },
  { 53, 2 },
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
    *pval = yymsp[0].minor.yy175;
}
#line 1844 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 125: /* excepts ::= except */
#line 616 "parser.y"
{
    yygotominor.yy175 = make_array_with(env, yymsp[0].minor.yy175);
}
#line 1855 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 619 "parser.y"
{
    yygotominor.yy175 = Array_push(env, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 1865 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 118: /* args_opt ::= */
      case 120: /* blockarg_opt ::= */
      case 123: /* blockarg_params_opt ::= */
      case 130: /* finally_opt ::= */
#line 623 "parser.y"
{
    yygotominor.yy175 = YNIL;
}
#line 1879 "parser.c"
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
      case 103: /* factor ::= power */
      case 104: /* power ::= postfix_expr */
      case 105: /* postfix_expr ::= atom */
      case 119: /* args_opt ::= args */
      case 131: /* finally_opt ::= FINALLY stmts */
#line 626 "parser.y"
{
    yygotominor.yy175 = yymsp[0].minor.yy175;
}
#line 1909 "parser.c"
        break;
      case 5: /* stmt ::= expr */
#line 629 "parser.y"
{
    if (PTR_AS(YogNode, yymsp[0].minor.yy175)->type == NODE_VARIABLE) {
        unsigned int lineno = NODE_LINENO(yymsp[0].minor.yy175);
        ID id = PTR_AS(YogNode, yymsp[0].minor.yy175)->u.variable.id;
        yygotominor.yy175 = CommandCall_new(env, lineno, id, YNIL, YNIL);
    }
    else {
        yygotominor.yy175 = yymsp[0].minor.yy175;
    }
}
#line 1923 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 639 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy175 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy175, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[-1].minor.yy175);
}
#line 1931 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 643 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy175 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-2].minor.yy175, YNIL, yymsp[-1].minor.yy175);
}
#line 1939 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 647 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy175 = Finally_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 1947 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 651 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy175 = While_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 1955 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 655 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Break_new(env, lineno, YNIL);
}
#line 1963 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 659 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy175 = Break_new(env, lineno, yymsp[0].minor.yy175);
}
#line 1971 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 663 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Next_new(env, lineno, YNIL);
}
#line 1979 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 667 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy175 = Next_new(env, lineno, yymsp[0].minor.yy175);
}
#line 1987 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 671 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Return_new(env, lineno, YNIL);
}
#line 1995 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 675 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy175 = Return_new(env, lineno, yymsp[0].minor.yy175);
}
#line 2003 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 679 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy175 = If_new(env, lineno, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2011 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 683 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy175 = Klass_new(env, lineno, id, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2020 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 688 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy175 = Nonlocal_new(env, lineno, yymsp[0].minor.yy175);
}
#line 2028 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 692 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy175 = Import_new(env, lineno, yymsp[0].minor.yy175);
}
#line 2036 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 704 "parser.y"
{
    yygotominor.yy175 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2044 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 707 "parser.y"
{
    yygotominor.yy175 = Array_push_token_id(env, yymsp[-2].minor.yy175, yymsp[0].minor.yy0);
}
#line 2052 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 728 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175, yymsp[0].minor.yy175);
    yygotominor.yy175 = make_array_with(env, node);
}
#line 2061 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 741 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy175 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2070 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 747 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-8].minor.yy175, yymsp[-6].minor.yy175, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2077 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 750 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-6].minor.yy175, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL);
}
#line 2084 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 753 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-6].minor.yy175, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175);
}
#line 2091 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 756 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL, YNIL);
}
#line 2098 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 759 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-6].minor.yy175, yymsp[-4].minor.yy175, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2105 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 762 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175, YNIL);
}
#line 2112 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 765 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, YNIL, YNIL, yymsp[0].minor.yy175);
}
#line 2119 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 768 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL, YNIL, YNIL);
}
#line 2126 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 771 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-6].minor.yy175, YNIL, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2133 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 774 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL);
}
#line 2140 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 777 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, YNIL, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175);
}
#line 2147 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 780 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175, YNIL, YNIL);
}
#line 2154 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 783 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, YNIL, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2161 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 786 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-2].minor.yy175, YNIL, YNIL, yymsp[0].minor.yy175, YNIL);
}
#line 2168 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 789 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-2].minor.yy175, YNIL, YNIL, YNIL, yymsp[0].minor.yy175);
}
#line 2175 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 792 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[0].minor.yy175, YNIL, YNIL, YNIL, YNIL);
}
#line 2182 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 795 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-6].minor.yy175, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2189 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 798 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL);
}
#line 2196 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 801 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175);
}
#line 2203 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 804 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL, YNIL);
}
#line 2210 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 807 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-4].minor.yy175, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2217 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 810 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175, YNIL);
}
#line 2224 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 813 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-2].minor.yy175, YNIL, YNIL, yymsp[0].minor.yy175);
}
#line 2231 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 816 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[0].minor.yy175, YNIL, YNIL, YNIL);
}
#line 2238 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 819 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2245 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 822 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL);
}
#line 2252 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 825 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175);
}
#line 2259 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 828 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy175, YNIL, YNIL);
}
#line 2266 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 831 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2273 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 834 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy175, YNIL);
}
#line 2280 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 837 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy175);
}
#line 2287 "parser.c"
        break;
      case 64: /* params ::= */
#line 840 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2294 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 844 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy175 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2303 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 850 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy175 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2312 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 856 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy175 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy175);
}
#line 2321 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 873 "parser.y"
{
    yygotominor.yy175 = YogArray_new(env);
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy175, lineno, id, YNIL);
}
#line 2331 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 879 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy175, lineno, id, YNIL);
    yygotominor.yy175 = yymsp[-2].minor.yy175;
}
#line 2341 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 893 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy175 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy175);
}
#line 2350 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 910 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy175);
    yygotominor.yy175 = Assign_new(env, lineno, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2358 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 933 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy175);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy175)->u.id;
    yygotominor.yy175 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy175, id, yymsp[0].minor.yy175);
}
#line 2367 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
#line 939 "parser.y"
{
    yygotominor.yy175 = yymsp[0].minor.yy0;
}
#line 2374 "parser.c"
        break;
      case 91: /* shift_expr ::= shift_expr LSHIFT match_expr */
      case 93: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 958 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy175);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy175 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy175, id, yymsp[0].minor.yy175);
}
#line 2384 "parser.c"
        break;
      case 95: /* arith_expr ::= arith_expr arith_op term */
      case 98: /* term ::= term term_op factor */
#line 976 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy175);
    yygotominor.yy175 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy175, VAL2ID(yymsp[-1].minor.yy175), yymsp[0].minor.yy175);
}
#line 2393 "parser.c"
        break;
      case 96: /* arith_op ::= PLUS */
      case 97: /* arith_op ::= MINUS */
      case 100: /* term_op ::= STAR */
      case 101: /* term_op ::= DIV */
#line 981 "parser.y"
{
    yygotominor.yy175 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2403 "parser.c"
        break;
      case 102: /* factor ::= MINUS factor */
#line 1003 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy175 = FuncCall_new3(env, lineno, yymsp[0].minor.yy175, id);
}
#line 2412 "parser.c"
        break;
      case 106: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1019 "parser.y"
{
    yygotominor.yy175 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy175), yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2419 "parser.c"
        break;
      case 107: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1022 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-3].minor.yy175);
    yygotominor.yy175 = Subscript_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2427 "parser.c"
        break;
      case 108: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1026 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy175);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy175 = Attr_new(env, lineno, yymsp[-2].minor.yy175, id);
}
#line 2436 "parser.c"
        break;
      case 109: /* atom ::= NAME */
#line 1032 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy175 = Variable_new(env, lineno, id);
}
#line 2445 "parser.c"
        break;
      case 110: /* atom ::= NUMBER */
      case 111: /* atom ::= REGEXP */
      case 112: /* atom ::= STRING */
#line 1037 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy175 = Literal_new(env, lineno, val);
}
#line 2456 "parser.c"
        break;
      case 113: /* atom ::= NIL */
#line 1052 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Literal_new(env, lineno, YNIL);
}
#line 2464 "parser.c"
        break;
      case 114: /* atom ::= TRUE */
#line 1056 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Literal_new(env, lineno, YTRUE);
}
#line 2472 "parser.c"
        break;
      case 115: /* atom ::= FALSE */
#line 1060 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Literal_new(env, lineno, YFALSE);
}
#line 2480 "parser.c"
        break;
      case 116: /* atom ::= LINE */
#line 1064 "parser.y"
{
    unsigned int lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy175 = Literal_new(env, lineno, val);
}
#line 2489 "parser.c"
        break;
      case 117: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1069 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy175 = Array_new(env, lineno, yymsp[-1].minor.yy175);
}
#line 2497 "parser.c"
        break;
      case 121: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 122: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1084 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy175 = BlockArg_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2506 "parser.c"
        break;
      case 124: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1096 "parser.y"
{
    yygotominor.yy175 = yymsp[-1].minor.yy175;
}
#line 2513 "parser.c"
        break;
      case 126: /* excepts ::= excepts except */
#line 1103 "parser.y"
{
    yygotominor.yy175 = Array_push(env, yymsp[-1].minor.yy175, yymsp[0].minor.yy175);
}
#line 2520 "parser.c"
        break;
      case 127: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1107 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy175 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy175, id, yymsp[0].minor.yy175);
}
#line 2530 "parser.c"
        break;
      case 128: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1113 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy175 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy175, NO_EXC_VAR, yymsp[0].minor.yy175);
}
#line 2538 "parser.c"
        break;
      case 129: /* except ::= EXCEPT NEWLINE stmts */
#line 1117 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy175 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy175);
}
#line 2546 "parser.c"
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
