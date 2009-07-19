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
#define YYNOCODE 92
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
#define YYNSTATE 219
#define YYNRULE 130
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
 /*     0 */   350,   77,  141,  139,  140,   76,  146,  147,  148,  149,
 /*    10 */    56,  132,  151,  152,   88,   91,   54,   58,  183,   43,
 /*    20 */   162,  155,  163,  145,   75,  158,  147,  148,  149,   56,
 /*    30 */    27,  151,  152,   88,   91,   54,   58,   73,   85,  162,
 /*    40 */   155,  163,   42,  141,  139,  140,   76,  114,  115,  117,
 /*    50 */   216,  150,   36,  151,  152,   88,   91,   54,   58,  218,
 /*    60 */    14,  162,  155,  163,  145,   75,  158,  147,  148,  149,
 /*    70 */    56,  164,  151,  152,   88,   91,   54,   58,  114,  115,
 /*    80 */   162,  155,  163,   52,  141,  139,  140,   76,   16,   39,
 /*    90 */    18,  143,  165,  166,  167,  168,  169,  170,   53,   58,
 /*   100 */   113,  187,  162,  155,  163,  145,   75,  158,  147,  148,
 /*   110 */   149,   56,  172,  151,  152,   88,   91,   54,   58,  164,
 /*   120 */   214,  162,  155,  163,   86,  141,  139,  140,  109,  110,
 /*   130 */   121,  125,  127,  208,   96,   31,  200,   39,   18,  177,
 /*   140 */   165,  166,  167,  168,  169,  170,  145,   75,  158,  147,
 /*   150 */   148,  149,   56,  161,  151,  152,   88,   91,   54,   58,
 /*   160 */    76,   94,  162,  155,  163,   78,  141,  139,  140,  131,
 /*   170 */    89,   54,   58,   17,   16,  162,  155,  163,  128,  110,
 /*   180 */   121,  125,  127,  208,   29,   38,  200,  145,   75,  158,
 /*   190 */   147,  148,  149,   56,   34,  151,  152,   88,   91,   54,
 /*   200 */    58,   76,   28,  162,  155,  163,   79,  141,  139,  140,
 /*   210 */   112,  116,  190,   57,  211,  194,  162,  155,  163,  111,
 /*   220 */   118,  120,  199,   16,    2,  200,    3,   15,  145,   75,
 /*   230 */   158,  147,  148,  149,   56,  181,  151,  152,   88,   91,
 /*   240 */    54,   58,   68,  100,  162,  155,  163,   44,  141,  139,
 /*   250 */   140,   76,  114,  115,  117,  122,  124,  204,   16,   16,
 /*   260 */   194,    8,    3,   19,   19,  101,  153,  155,  163,  145,
 /*   270 */    75,  158,  147,  148,  149,   56,   26,  151,  152,   88,
 /*   280 */    91,   54,   58,   76,   65,  162,  155,  163,   45,  141,
 /*   290 */   139,  140,  178,  104,  114,  115,  117,  177,  154,  155,
 /*   300 */   163,  119,  197,  123,  202,  126,  206,  103,  106,  114,
 /*   310 */   145,   75,  158,  147,  148,  149,   56,  185,  151,  152,
 /*   320 */    88,   91,   54,   58,  191,  192,  162,  155,  163,   98,
 /*   330 */   141,  139,  140,  131,  159,  160,   13,   17,   59,   74,
 /*   340 */   219,   16,   30,   16,   16,  189,  173,  179,   29,  195,
 /*   350 */   129,  145,   75,  158,  147,  148,  149,   56,  196,  151,
 /*   360 */   152,   88,   91,   54,   58,  198,  201,  162,  155,  163,
 /*   370 */    80,  141,  139,  140,   16,  203,   16,  184,  133,  217,
 /*   380 */   142,  205,  207,   16,    4,   32,   22,   35,  171,   55,
 /*   390 */     5,    6,  145,   75,  158,  147,  148,  149,   56,  176,
 /*   400 */   151,  152,   88,   91,   54,   58,    7,  102,  162,  155,
 /*   410 */   163,   81,  141,  139,  140,   60,    9,   62,   33,  180,
 /*   420 */   105,  182,  108,  210,   11,  212,   37,   40,   46,   10,
 /*   430 */   186,   63,  188,  145,   75,  158,  147,  148,  149,   56,
 /*   440 */    51,  151,  152,   88,   91,   54,   58,   64,   47,  162,
 /*   450 */   155,  163,   82,  141,  139,  140,   66,   67,   41,   48,
 /*   460 */    69,   70,  215,   49,   71,   72,  213,  134,   12,  351,
 /*   470 */   351,  351,  351,  351,  145,   75,  158,  147,  148,  149,
 /*   480 */    56,  351,  151,  152,   88,   91,   54,   58,  351,  351,
 /*   490 */   162,  155,  163,  135,  141,  139,  140,  351,  351,  351,
 /*   500 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   510 */   351,  351,  351,  351,  351,  145,   75,  158,  147,  148,
 /*   520 */   149,   56,  351,  151,  152,   88,   91,   54,   58,  351,
 /*   530 */   351,  162,  155,  163,  136,  141,  139,  140,  351,  351,
 /*   540 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   550 */   351,  351,  351,  351,  351,  351,  145,   75,  158,  147,
 /*   560 */   148,  149,   56,  351,  151,  152,   88,   91,   54,   58,
 /*   570 */   351,  351,  162,  155,  163,  137,  141,  139,  140,  351,
 /*   580 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   590 */   351,  351,  351,  351,  351,  351,  351,  145,   75,  158,
 /*   600 */   147,  148,  149,   56,  351,  151,  152,   88,   91,   54,
 /*   610 */    58,  351,  351,  162,  155,  163,   84,  141,  139,  140,
 /*   620 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   630 */   351,  351,  351,  351,  351,  351,  351,  351,  145,   75,
 /*   640 */   158,  147,  148,  149,   56,  351,  151,  152,   88,   91,
 /*   650 */    54,   58,  351,    1,  162,  155,  163,   20,   21,   23,
 /*   660 */    24,   25,   99,  164,   61,   50,  351,  351,  351,  351,
 /*   670 */   107,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   680 */   351,   39,   18,  351,  165,  166,  167,  168,  169,  170,
 /*   690 */   138,  139,  140,  351,  351,  351,  351,  351,  351,  351,
 /*   700 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   710 */   351,  145,   75,  158,  147,  148,  149,   56,  351,  151,
 /*   720 */   152,   88,   91,   54,   58,  351,  351,  162,  155,  163,
 /*   730 */   351,  156,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   740 */   351,  351,  351,  351,  351,  351,  351,  351,  351,   90,
 /*   750 */   145,   75,  158,  147,  148,  149,   56,  351,  151,  152,
 /*   760 */    88,   91,   54,   58,  156,  351,  162,  155,  163,   93,
 /*   770 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   780 */   351,  351,   90,  145,   75,  158,  147,  148,  149,   56,
 /*   790 */   351,  151,  152,   88,   91,   54,   58,  351,  351,  162,
 /*   800 */   155,  163,   92,   83,  351,  351,  351,  351,  351,  351,
 /*   810 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   820 */   351,  351,  145,   75,  158,  147,  148,  149,   56,  351,
 /*   830 */   151,  152,   88,   91,   54,   58,   87,  351,  162,  155,
 /*   840 */   163,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   850 */   351,  351,  351,  351,  351,  145,   75,  158,  147,  148,
 /*   860 */   149,   56,  351,  151,  152,   88,   91,   54,   58,  351,
 /*   870 */   351,  162,  155,  163,  144,  351,  351,  351,  351,  351,
 /*   880 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   890 */   351,  351,  351,  145,   75,  158,  147,  148,  149,   56,
 /*   900 */   351,  151,  152,   88,   91,   54,   58,  157,  351,  162,
 /*   910 */   155,  163,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   920 */   351,  351,  351,  351,  351,  351,  145,   75,  158,  147,
 /*   930 */   148,  149,   56,  351,  151,  152,   88,   91,   54,   58,
 /*   940 */   351,  351,  162,  155,  163,  174,  351,  351,  351,  351,
 /*   950 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*   960 */   351,  351,  351,  351,  145,   75,  158,  147,  148,  149,
 /*   970 */    56,  351,  151,  152,   88,   91,   54,   58,  175,  351,
 /*   980 */   162,  155,  163,  351,  351,  351,  351,  351,  351,  351,
 /*   990 */   351,  351,  351,  351,  351,  351,  351,  145,   75,  158,
 /*  1000 */   147,  148,  149,   56,  351,  151,  152,   88,   91,   54,
 /*  1010 */    58,  351,  351,  162,  155,  163,   95,  351,  351,  351,
 /*  1020 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*  1030 */   351,  351,  351,  351,  351,  145,   75,  158,  147,  148,
 /*  1040 */   149,   56,  351,  151,  152,   88,   91,   54,   58,   97,
 /*  1050 */   351,  162,  155,  163,  351,  351,  351,  351,  351,  351,
 /*  1060 */   351,  351,  351,  351,  351,  351,  351,  351,  145,   75,
 /*  1070 */   158,  147,  148,  149,   56,  351,  151,  152,   88,   91,
 /*  1080 */    54,   58,  351,  351,  162,  155,  163,  193,  351,  351,
 /*  1090 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*  1100 */   351,  351,  351,  351,  351,  351,  145,   75,  158,  147,
 /*  1110 */   148,  149,   56,  351,  151,  152,   88,   91,   54,   58,
 /*  1120 */   209,  351,  162,  155,  163,  351,  351,  351,  351,  351,
 /*  1130 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  145,
 /*  1140 */    75,  158,  147,  148,  149,   56,  351,  151,  152,   88,
 /*  1150 */    91,   54,   58,  351,  351,  162,  155,  163,  130,  351,
 /*  1160 */   351,  351,  351,  351,  351,  351,  351,  351,  351,  351,
 /*  1170 */   351,  351,  351,  351,  351,  351,  351,  145,   75,  158,
 /*  1180 */   147,  148,  149,   56,  351,  151,  152,   88,   91,   54,
 /*  1190 */    58,  351,  351,  162,  155,  163,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    45,   46,   47,   48,   49,   69,   70,   71,   72,   73,
 /*    10 */    74,   51,   76,   77,   78,   79,   80,   81,   12,   50,
 /*    20 */    84,   85,   86,   68,   69,   70,   71,   72,   73,   74,
 /*    30 */    25,   76,   77,   78,   79,   80,   81,   12,   51,   84,
 /*    40 */    85,   86,   46,   47,   48,   49,   69,   22,   23,   24,
 /*    50 */    90,   74,   82,   76,   77,   78,   79,   80,   81,   90,
 /*    60 */     1,   84,   85,   86,   68,   69,   70,   71,   72,   73,
 /*    70 */    74,   12,   76,   77,   78,   79,   80,   81,   22,   23,
 /*    80 */    84,   85,   86,   46,   47,   48,   49,   69,    1,   30,
 /*    90 */    31,    4,   33,   34,   35,   36,   37,   38,   80,   81,
 /*   100 */    62,   63,   84,   85,   86,   68,   69,   70,   71,   72,
 /*   110 */    73,   74,   88,   76,   77,   78,   79,   80,   81,   12,
 /*   120 */    26,   84,   85,   86,   46,   47,   48,   49,   58,   59,
 /*   130 */    60,   61,   62,   63,   52,   75,   66,   30,   31,   57,
 /*   140 */    33,   34,   35,   36,   37,   38,   68,   69,   70,   71,
 /*   150 */    72,   73,   74,   23,   76,   77,   78,   79,   80,   81,
 /*   160 */    69,   89,   84,   85,   86,   46,   47,   48,   49,   16,
 /*   170 */    79,   80,   81,   20,    1,   84,   85,   86,   58,   59,
 /*   180 */    60,   61,   62,   63,   31,   83,   66,   68,   69,   70,
 /*   190 */    71,   72,   73,   74,   31,   76,   77,   78,   79,   80,
 /*   200 */    81,   69,   17,   84,   85,   86,   46,   47,   48,   49,
 /*   210 */    61,   62,   63,   81,   41,   66,   84,   85,   86,   60,
 /*   220 */    61,   62,   63,    1,    3,   66,    5,    5,   68,   69,
 /*   230 */    70,   71,   72,   73,   74,   12,   76,   77,   78,   79,
 /*   240 */    80,   81,   12,   53,   84,   85,   86,   46,   47,   48,
 /*   250 */    49,   69,   22,   23,   24,   61,   62,   63,    1,    1,
 /*   260 */    66,    3,    5,   42,   42,   54,   84,   85,   86,   68,
 /*   270 */    69,   70,   71,   72,   73,   74,   18,   76,   77,   78,
 /*   280 */    79,   80,   81,   69,   12,   84,   85,   86,   46,   47,
 /*   290 */    48,   49,   52,   56,   22,   23,   24,   57,   84,   85,
 /*   300 */    86,   62,   63,   62,   63,   62,   63,   55,   56,   22,
 /*   310 */    68,   69,   70,   71,   72,   73,   74,   63,   76,   77,
 /*   320 */    78,   79,   80,   81,   64,   65,   84,   85,   86,   46,
 /*   330 */    47,   48,   49,   16,   29,   30,    1,   20,   39,   40,
 /*   340 */     0,    1,   25,    1,    1,   63,    4,    4,   31,   65,
 /*   350 */    89,   68,   69,   70,   71,   72,   73,   74,   63,   76,
 /*   360 */    77,   78,   79,   80,   81,   63,   63,   84,   85,   86,
 /*   370 */    46,   47,   48,   49,    1,   63,    1,    4,   43,    4,
 /*   380 */     4,   63,   63,    1,    1,   27,   15,   28,   32,   21,
 /*   390 */     1,    1,   68,   69,   70,   71,   72,   73,   74,    4,
 /*   400 */    76,   77,   78,   79,   80,   81,    1,   15,   84,   85,
 /*   410 */    86,   46,   47,   48,   49,   12,    1,   15,   20,   12,
 /*   420 */    16,   12,   12,   32,    1,   32,   15,   15,   15,   21,
 /*   430 */    12,   15,   12,   68,   69,   70,   71,   72,   73,   74,
 /*   440 */    12,   76,   77,   78,   79,   80,   81,   15,   15,   84,
 /*   450 */    85,   86,   46,   47,   48,   49,   15,   15,   15,   15,
 /*   460 */    15,   15,    4,   15,   15,   15,   12,   12,    1,   91,
 /*   470 */    91,   91,   91,   91,   68,   69,   70,   71,   72,   73,
 /*   480 */    74,   91,   76,   77,   78,   79,   80,   81,   91,   91,
 /*   490 */    84,   85,   86,   46,   47,   48,   49,   91,   91,   91,
 /*   500 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   510 */    91,   91,   91,   91,   91,   68,   69,   70,   71,   72,
 /*   520 */    73,   74,   91,   76,   77,   78,   79,   80,   81,   91,
 /*   530 */    91,   84,   85,   86,   46,   47,   48,   49,   91,   91,
 /*   540 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   550 */    91,   91,   91,   91,   91,   91,   68,   69,   70,   71,
 /*   560 */    72,   73,   74,   91,   76,   77,   78,   79,   80,   81,
 /*   570 */    91,   91,   84,   85,   86,   46,   47,   48,   49,   91,
 /*   580 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   590 */    91,   91,   91,   91,   91,   91,   91,   68,   69,   70,
 /*   600 */    71,   72,   73,   74,   91,   76,   77,   78,   79,   80,
 /*   610 */    81,   91,   91,   84,   85,   86,   46,   47,   48,   49,
 /*   620 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   630 */    91,   91,   91,   91,   91,   91,   91,   91,   68,   69,
 /*   640 */    70,   71,   72,   73,   74,   91,   76,   77,   78,   79,
 /*   650 */    80,   81,   91,    2,   84,   85,   86,    6,    7,    8,
 /*   660 */     9,   10,   11,   12,   13,   14,   91,   91,   91,   91,
 /*   670 */    19,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   680 */    91,   30,   31,   91,   33,   34,   35,   36,   37,   38,
 /*   690 */    47,   48,   49,   91,   91,   91,   91,   91,   91,   91,
 /*   700 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   710 */    91,   68,   69,   70,   71,   72,   73,   74,   91,   76,
 /*   720 */    77,   78,   79,   80,   81,   91,   91,   84,   85,   86,
 /*   730 */    91,   49,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   740 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   67,
 /*   750 */    68,   69,   70,   71,   72,   73,   74,   91,   76,   77,
 /*   760 */    78,   79,   80,   81,   49,   91,   84,   85,   86,   87,
 /*   770 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   780 */    91,   91,   67,   68,   69,   70,   71,   72,   73,   74,
 /*   790 */    91,   76,   77,   78,   79,   80,   81,   91,   91,   84,
 /*   800 */    85,   86,   87,   49,   91,   91,   91,   91,   91,   91,
 /*   810 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   820 */    91,   91,   68,   69,   70,   71,   72,   73,   74,   91,
 /*   830 */    76,   77,   78,   79,   80,   81,   49,   91,   84,   85,
 /*   840 */    86,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   850 */    91,   91,   91,   91,   91,   68,   69,   70,   71,   72,
 /*   860 */    73,   74,   91,   76,   77,   78,   79,   80,   81,   91,
 /*   870 */    91,   84,   85,   86,   49,   91,   91,   91,   91,   91,
 /*   880 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   890 */    91,   91,   91,   68,   69,   70,   71,   72,   73,   74,
 /*   900 */    91,   76,   77,   78,   79,   80,   81,   49,   91,   84,
 /*   910 */    85,   86,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   920 */    91,   91,   91,   91,   91,   91,   68,   69,   70,   71,
 /*   930 */    72,   73,   74,   91,   76,   77,   78,   79,   80,   81,
 /*   940 */    91,   91,   84,   85,   86,   49,   91,   91,   91,   91,
 /*   950 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*   960 */    91,   91,   91,   91,   68,   69,   70,   71,   72,   73,
 /*   970 */    74,   91,   76,   77,   78,   79,   80,   81,   49,   91,
 /*   980 */    84,   85,   86,   91,   91,   91,   91,   91,   91,   91,
 /*   990 */    91,   91,   91,   91,   91,   91,   91,   68,   69,   70,
 /*  1000 */    71,   72,   73,   74,   91,   76,   77,   78,   79,   80,
 /*  1010 */    81,   91,   91,   84,   85,   86,   49,   91,   91,   91,
 /*  1020 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*  1030 */    91,   91,   91,   91,   91,   68,   69,   70,   71,   72,
 /*  1040 */    73,   74,   91,   76,   77,   78,   79,   80,   81,   49,
 /*  1050 */    91,   84,   85,   86,   91,   91,   91,   91,   91,   91,
 /*  1060 */    91,   91,   91,   91,   91,   91,   91,   91,   68,   69,
 /*  1070 */    70,   71,   72,   73,   74,   91,   76,   77,   78,   79,
 /*  1080 */    80,   81,   91,   91,   84,   85,   86,   49,   91,   91,
 /*  1090 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*  1100 */    91,   91,   91,   91,   91,   91,   68,   69,   70,   71,
 /*  1110 */    72,   73,   74,   91,   76,   77,   78,   79,   80,   81,
 /*  1120 */    49,   91,   84,   85,   86,   91,   91,   91,   91,   91,
 /*  1130 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   68,
 /*  1140 */    69,   70,   71,   72,   73,   74,   91,   76,   77,   78,
 /*  1150 */    79,   80,   81,   91,   91,   84,   85,   86,   49,   91,
 /*  1160 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*  1170 */    91,   91,   91,   91,   91,   91,   91,   68,   69,   70,
 /*  1180 */    71,   72,   73,   74,   91,   76,   77,   78,   79,   80,
 /*  1190 */    81,   91,   91,   84,   85,   86,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 137
static const short yy_shift_ofst[] = {
 /*     0 */   651,  651,  651,  651,  651,  651,  651,  651,  651,  651,
 /*    10 */   651,  651,  651,  651,  651,  651,  651,  107,  107,   59,
 /*    20 */   107,  107,  107,  107,  107,  107,  107,  107,  107,  107,
 /*    30 */   107,  107,  107,   25,   25,  107,  107,  230,  107,  107,
 /*    40 */   272,  272,  222,  221,  258,  258,   56,   56,   56,   56,
 /*    50 */     6,    5,  257,  305,  305,  299,   94,  130,  130,  163,
 /*    60 */   185,  223,    6,  287,  287,    5,  287,  287,    5,  287,
 /*    70 */   287,  287,  287,    5,  163,  317,  153,  340,   87,  342,
 /*    80 */   343,  373,  173,  335,  375,  376,  382,  383,  358,  359,
 /*    90 */   371,  359,  356,  368,  389,  390,  395,  405,  382,  403,
 /*   100 */   415,  392,  407,  402,  404,  409,  404,  410,  398,  408,
 /*   110 */   411,  412,  413,  416,  418,  420,  432,  428,  433,  441,
 /*   120 */   442,  443,  444,  445,  446,  448,  449,  450,  391,  423,
 /*   130 */   393,  454,  458,  455,  467,  382,  382,  382,
};
#define YY_REDUCE_USE_DFLT (-65)
#define YY_REDUCE_MAX 74
static const short yy_reduce_ofst[] = {
 /*     0 */   -45,   -4,   37,   78,  119,  160,  201,  242,  283,  324,
 /*    10 */   365,  406,  447,  488,  529,  570,  643,  682,  715,  754,
 /*    20 */   787,  825,  858,  896,  929,  967, 1000, 1038, 1071, 1109,
 /*    30 */   -64,  -23,   91,   70,  120,   18,  132,  159,  182,  214,
 /*    40 */   149,  194,  -31,  -40,   82,  240,   38,  239,  241,  243,
 /*    50 */   252,  260,  -13,  -30,  -30,   24,   60,  102,  102,   72,
 /*    60 */   190,  211,  237,  254,  282,  284,  295,  302,  284,  303,
 /*    70 */   312,  318,  319,  284,  261,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   222,  222,  222,  222,  222,  222,  222,  222,  222,  222,
 /*    10 */   222,  222,  222,  222,  222,  222,  222,  335,  335,  349,
 /*    20 */   349,  229,  349,  231,  233,  349,  349,  349,  349,  349,
 /*    30 */   349,  349,  349,  283,  283,  349,  349,  349,  349,  349,
 /*    40 */   349,  349,  349,  347,  249,  249,  349,  349,  349,  349,
 /*    50 */   349,  287,  347,  312,  311,  337,  303,  314,  313,  340,
 /*    60 */   245,  349,  349,  349,  349,  349,  349,  349,  291,  349,
 /*    70 */   349,  349,  349,  290,  340,  322,  322,  349,  349,  349,
 /*    80 */   349,  349,  349,  349,  349,  349,  348,  349,  308,  310,
 /*    90 */   336,  309,  349,  349,  349,  349,  349,  349,  250,  349,
 /*   100 */   349,  237,  349,  238,  240,  349,  239,  349,  349,  349,
 /*   110 */   267,  259,  255,  253,  349,  349,  257,  349,  263,  261,
 /*   120 */   265,  275,  271,  269,  273,  279,  277,  281,  349,  349,
 /*   130 */   349,  349,  349,  349,  349,  344,  345,  346,  221,  223,
 /*   140 */   224,  220,  225,  228,  230,  297,  298,  300,  301,  302,
 /*   150 */   304,  306,  307,  317,  320,  321,  295,  296,  299,  315,
 /*   160 */   316,  319,  318,  323,  327,  328,  329,  330,  331,  332,
 /*   170 */   333,  334,  324,  338,  232,  234,  235,  247,  248,  236,
 /*   180 */   244,  243,  242,  241,  251,  252,  284,  254,  285,  256,
 /*   190 */   258,  286,  288,  289,  293,  294,  260,  262,  264,  266,
 /*   200 */   292,  268,  270,  272,  274,  276,  278,  280,  282,  246,
 /*   210 */   341,  339,  325,  326,  305,  226,  343,  227,  342,
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
  "EQUAL_TILDA",   "PLUS",          "MINUS",         "LBRACKET",    
  "RBRACKET",      "NUMBER",        "REGEXP",        "STRING",      
  "TRUE",          "FALSE",         "LINE",          "DO",          
  "LBRACE",        "RBRACE",        "EXCEPT",        "AS",          
  "error",         "module",        "stmts",         "stmt",        
  "func_def",      "expr",          "excepts",       "finally_opt", 
  "if_tail",       "super_opt",     "names",         "dotted_names",
  "dotted_name",   "else_opt",      "params",        "params_without_default",
  "params_with_default",  "block_param",   "var_param",     "kw_param",    
  "param_default_opt",  "param_default",  "param_with_default",  "args",        
  "assign_expr",   "postfix_expr",  "logical_or_expr",  "logical_and_expr",
  "not_expr",      "comparison",    "xor_expr",      "comp_op",     
  "or_expr",       "and_expr",      "shift_expr",    "match_expr",  
  "arith_expr",    "term",          "arith_op",      "term_op",     
  "factor",        "power",         "atom",          "args_opt",    
  "blockarg_opt",  "blockarg_params_opt",  "except",      
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
 /* 101 */ "factor ::= MINUS factor",
 /* 102 */ "factor ::= power",
 /* 103 */ "power ::= postfix_expr",
 /* 104 */ "postfix_expr ::= atom",
 /* 105 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 106 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 107 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 108 */ "atom ::= NAME",
 /* 109 */ "atom ::= NUMBER",
 /* 110 */ "atom ::= REGEXP",
 /* 111 */ "atom ::= STRING",
 /* 112 */ "atom ::= TRUE",
 /* 113 */ "atom ::= FALSE",
 /* 114 */ "atom ::= LINE",
 /* 115 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 116 */ "args_opt ::=",
 /* 117 */ "args_opt ::= args",
 /* 118 */ "blockarg_opt ::=",
 /* 119 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 120 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 121 */ "blockarg_params_opt ::=",
 /* 122 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 123 */ "excepts ::= except",
 /* 124 */ "excepts ::= excepts except",
 /* 125 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 126 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 127 */ "except ::= EXCEPT NEWLINE stmts",
 /* 128 */ "finally_opt ::=",
 /* 129 */ "finally_opt ::= FINALLY stmts",
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
  { 45, 1 },
  { 46, 1 },
  { 46, 3 },
  { 47, 0 },
  { 47, 1 },
  { 47, 1 },
  { 47, 7 },
  { 47, 5 },
  { 47, 5 },
  { 47, 5 },
  { 47, 1 },
  { 47, 2 },
  { 47, 1 },
  { 47, 2 },
  { 47, 1 },
  { 47, 2 },
  { 47, 6 },
  { 47, 6 },
  { 47, 2 },
  { 47, 2 },
  { 55, 1 },
  { 55, 3 },
  { 56, 1 },
  { 56, 3 },
  { 54, 1 },
  { 54, 3 },
  { 53, 0 },
  { 53, 2 },
  { 52, 1 },
  { 52, 5 },
  { 57, 0 },
  { 57, 2 },
  { 48, 7 },
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
  { 67, 1 },
  { 67, 3 },
  { 49, 1 },
  { 68, 3 },
  { 68, 1 },
  { 70, 1 },
  { 71, 1 },
  { 72, 1 },
  { 73, 1 },
  { 73, 3 },
  { 75, 1 },
  { 74, 1 },
  { 76, 1 },
  { 77, 1 },
  { 78, 1 },
  { 78, 3 },
  { 79, 1 },
  { 79, 3 },
  { 80, 1 },
  { 80, 3 },
  { 82, 1 },
  { 82, 1 },
  { 81, 3 },
  { 81, 1 },
  { 83, 1 },
  { 84, 2 },
  { 84, 1 },
  { 85, 1 },
  { 69, 1 },
  { 69, 5 },
  { 69, 4 },
  { 69, 3 },
  { 86, 1 },
  { 86, 1 },
  { 86, 1 },
  { 86, 1 },
  { 86, 1 },
  { 86, 1 },
  { 86, 1 },
  { 86, 3 },
  { 87, 0 },
  { 87, 1 },
  { 88, 0 },
  { 88, 5 },
  { 88, 5 },
  { 89, 0 },
  { 89, 3 },
  { 50, 1 },
  { 50, 2 },
  { 90, 6 },
  { 90, 4 },
  { 90, 3 },
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
#line 612 "parser.y"
{
    *pval = yymsp[0].minor.yy135;
}
#line 1838 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 123: /* excepts ::= except */
#line 616 "parser.y"
{
    yygotominor.yy135 = make_array_with(env, yymsp[0].minor.yy135);
}
#line 1849 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 619 "parser.y"
{
    yygotominor.yy135 = Array_push(env, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 1859 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 116: /* args_opt ::= */
      case 118: /* blockarg_opt ::= */
      case 121: /* blockarg_params_opt ::= */
      case 128: /* finally_opt ::= */
#line 623 "parser.y"
{
    yygotominor.yy135 = YNIL;
}
#line 1873 "parser.c"
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
      case 102: /* factor ::= power */
      case 103: /* power ::= postfix_expr */
      case 104: /* postfix_expr ::= atom */
      case 117: /* args_opt ::= args */
      case 129: /* finally_opt ::= FINALLY stmts */
#line 626 "parser.y"
{
    yygotominor.yy135 = yymsp[0].minor.yy135;
}
#line 1903 "parser.c"
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
#line 1917 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 639 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy135 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy135, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[-1].minor.yy135);
}
#line 1925 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 643 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy135 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-2].minor.yy135, YNIL, yymsp[-1].minor.yy135);
}
#line 1933 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 647 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy135 = Finally_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 1941 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 651 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy135 = While_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 1949 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 655 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Break_new(env, lineno, YNIL);
}
#line 1957 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 659 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy135 = Break_new(env, lineno, yymsp[0].minor.yy135);
}
#line 1965 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 663 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Next_new(env, lineno, YNIL);
}
#line 1973 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 667 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy135 = Next_new(env, lineno, yymsp[0].minor.yy135);
}
#line 1981 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 671 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Return_new(env, lineno, YNIL);
}
#line 1989 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 675 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy135 = Return_new(env, lineno, yymsp[0].minor.yy135);
}
#line 1997 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 679 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy135 = If_new(env, lineno, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[-1].minor.yy135);
}
#line 2005 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 683 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy135 = Klass_new(env, lineno, id, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 2014 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 688 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy135 = Nonlocal_new(env, lineno, yymsp[0].minor.yy135);
}
#line 2022 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 692 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy135 = Import_new(env, lineno, yymsp[0].minor.yy135);
}
#line 2030 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 704 "parser.y"
{
    yygotominor.yy135 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2038 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 707 "parser.y"
{
    yygotominor.yy135 = Array_push_token_id(env, yymsp[-2].minor.yy135, yymsp[0].minor.yy0);
}
#line 2046 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 728 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135, yymsp[0].minor.yy135);
    yygotominor.yy135 = make_array_with(env, node);
}
#line 2055 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 741 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy135 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 2064 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 747 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-8].minor.yy135, yymsp[-6].minor.yy135, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2071 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 750 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-6].minor.yy135, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL);
}
#line 2078 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 753 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-6].minor.yy135, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135);
}
#line 2085 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 756 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL, YNIL);
}
#line 2092 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 759 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-6].minor.yy135, yymsp[-4].minor.yy135, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2099 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 762 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135, YNIL);
}
#line 2106 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 765 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, YNIL, YNIL, yymsp[0].minor.yy135);
}
#line 2113 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 768 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL, YNIL, YNIL);
}
#line 2120 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 771 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-6].minor.yy135, YNIL, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2127 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 774 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL);
}
#line 2134 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 777 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, YNIL, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135);
}
#line 2141 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 780 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135, YNIL, YNIL);
}
#line 2148 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 783 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-4].minor.yy135, YNIL, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2155 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 786 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-2].minor.yy135, YNIL, YNIL, yymsp[0].minor.yy135, YNIL);
}
#line 2162 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 789 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[-2].minor.yy135, YNIL, YNIL, YNIL, yymsp[0].minor.yy135);
}
#line 2169 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 792 "parser.y"
{
    yygotominor.yy135 = Params_new(env, yymsp[0].minor.yy135, YNIL, YNIL, YNIL, YNIL);
}
#line 2176 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 795 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-6].minor.yy135, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2183 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 798 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL);
}
#line 2190 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 801 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135);
}
#line 2197 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 804 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL, YNIL);
}
#line 2204 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 807 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-4].minor.yy135, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2211 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 810 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135, YNIL);
}
#line 2218 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 813 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[-2].minor.yy135, YNIL, YNIL, yymsp[0].minor.yy135);
}
#line 2225 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 816 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, yymsp[0].minor.yy135, YNIL, YNIL, YNIL);
}
#line 2232 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 819 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2239 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 822 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135, YNIL);
}
#line 2246 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 825 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy135, YNIL, yymsp[0].minor.yy135);
}
#line 2253 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 828 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy135, YNIL, YNIL);
}
#line 2260 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 831 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2267 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 834 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy135, YNIL);
}
#line 2274 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 837 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy135);
}
#line 2281 "parser.c"
        break;
      case 64: /* params ::= */
#line 840 "parser.y"
{
    yygotominor.yy135 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2288 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 844 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy135 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2297 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 850 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy135 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2306 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 856 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy135 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy135);
}
#line 2315 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 873 "parser.y"
{
    yygotominor.yy135 = YogArray_new(env);
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy135, lineno, id, YNIL);
}
#line 2325 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 879 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy135, lineno, id, YNIL);
    yygotominor.yy135 = yymsp[-2].minor.yy135;
}
#line 2335 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 893 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy135 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy135);
}
#line 2344 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 910 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy135);
    yygotominor.yy135 = Assign_new(env, lineno, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2352 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 933 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy135);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy135)->u.id;
    yygotominor.yy135 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy135, id, yymsp[0].minor.yy135);
}
#line 2361 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
#line 939 "parser.y"
{
    yygotominor.yy135 = yymsp[0].minor.yy0;
}
#line 2368 "parser.c"
        break;
      case 91: /* shift_expr ::= shift_expr LSHIFT match_expr */
      case 93: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 958 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy135);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy135 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy135, id, yymsp[0].minor.yy135);
}
#line 2378 "parser.c"
        break;
      case 95: /* arith_expr ::= arith_expr arith_op term */
      case 98: /* term ::= term term_op factor */
#line 976 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy135);
    yygotominor.yy135 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy135, VAL2ID(yymsp[-1].minor.yy135), yymsp[0].minor.yy135);
}
#line 2387 "parser.c"
        break;
      case 96: /* arith_op ::= PLUS */
      case 97: /* arith_op ::= MINUS */
      case 100: /* term_op ::= STAR */
#line 981 "parser.y"
{
    yygotominor.yy135 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2396 "parser.c"
        break;
      case 101: /* factor ::= MINUS factor */
#line 1000 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy135 = FuncCall_new3(env, lineno, yymsp[0].minor.yy135, id);
}
#line 2405 "parser.c"
        break;
      case 105: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1016 "parser.y"
{
    yygotominor.yy135 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy135), yymsp[-4].minor.yy135, yymsp[-2].minor.yy135, yymsp[0].minor.yy135);
}
#line 2412 "parser.c"
        break;
      case 106: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1019 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-3].minor.yy135);
    yygotominor.yy135 = Subscript_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 2420 "parser.c"
        break;
      case 107: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1023 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy135);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy135 = Attr_new(env, lineno, yymsp[-2].minor.yy135, id);
}
#line 2429 "parser.c"
        break;
      case 108: /* atom ::= NAME */
#line 1029 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy135 = Variable_new(env, lineno, id);
}
#line 2438 "parser.c"
        break;
      case 109: /* atom ::= NUMBER */
      case 110: /* atom ::= REGEXP */
      case 111: /* atom ::= STRING */
#line 1034 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy135 = Literal_new(env, lineno, val);
}
#line 2449 "parser.c"
        break;
      case 112: /* atom ::= TRUE */
#line 1049 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Literal_new(env, lineno, YTRUE);
}
#line 2457 "parser.c"
        break;
      case 113: /* atom ::= FALSE */
#line 1053 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy135 = Literal_new(env, lineno, YFALSE);
}
#line 2465 "parser.c"
        break;
      case 114: /* atom ::= LINE */
#line 1057 "parser.y"
{
    unsigned int lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy135 = Literal_new(env, lineno, val);
}
#line 2474 "parser.c"
        break;
      case 115: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1062 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy135 = Array_new(env, lineno, yymsp[-1].minor.yy135);
}
#line 2482 "parser.c"
        break;
      case 119: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 120: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1077 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy135 = BlockArg_new(env, lineno, yymsp[-3].minor.yy135, yymsp[-1].minor.yy135);
}
#line 2491 "parser.c"
        break;
      case 122: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1089 "parser.y"
{
    yygotominor.yy135 = yymsp[-1].minor.yy135;
}
#line 2498 "parser.c"
        break;
      case 124: /* excepts ::= excepts except */
#line 1096 "parser.y"
{
    yygotominor.yy135 = Array_push(env, yymsp[-1].minor.yy135, yymsp[0].minor.yy135);
}
#line 2505 "parser.c"
        break;
      case 125: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1100 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy135 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy135, id, yymsp[0].minor.yy135);
}
#line 2515 "parser.c"
        break;
      case 126: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1106 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy135 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy135, NO_EXC_VAR, yymsp[0].minor.yy135);
}
#line 2523 "parser.c"
        break;
      case 127: /* except ::= EXCEPT NEWLINE stmts */
#line 1110 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy135 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy135);
}
#line 2531 "parser.c"
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
