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

    ID id = YogVM_intern(env, env->vm, "[]");
    node = FuncCall_new2(env, lineno, prefix, id, index);

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
#line 601 "parser.c"
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
#define YYNSTATE 212
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
 /*     0 */   338,   71,  137,  135,  136,  105,  106,  117,  121,  123,
 /*    10 */   201,   92,  171,  193,  110,  111,  170,  170,   40,  137,
 /*    20 */   135,  136,  176,  141,   69,  154,  143,  144,  145,   52,
 /*    30 */    16,  147,  148,   82,   86,   87,  155,  150,  151,  156,
 /*    40 */   141,   69,  154,  143,  144,  145,   52,   70,  147,  148,
 /*    50 */    82,   86,   87,  155,  150,  151,  156,   83,   87,  155,
 /*    60 */   150,  151,  156,   70,   50,  137,  135,  136,  146,  204,
 /*    70 */   147,  148,   82,   86,   87,  155,  150,  151,  156,  109,
 /*    80 */   180,   80,  137,  135,  136,   13,  141,   69,  154,  143,
 /*    90 */   144,  145,   52,   27,  147,  148,   82,   86,   87,  155,
 /*   100 */   150,  151,  156,  141,   69,  154,  143,  144,  145,   52,
 /*   110 */    70,  147,  148,   82,   86,   87,  155,  150,  151,  156,
 /*   120 */    79,   84,  155,  150,  151,  156,  129,   72,  137,  135,
 /*   130 */   136,  124,  106,  117,  121,  123,  201,  115,  190,  193,
 /*   140 */   119,  195,  122,  199,   73,  137,  135,  136,  165,  141,
 /*   150 */    69,  154,  143,  144,  145,   52,  207,  147,  148,   82,
 /*   160 */    86,   87,  155,  150,  151,  156,  141,   69,  154,  143,
 /*   170 */   144,  145,   52,   70,  147,  148,   82,   86,   87,  155,
 /*   180 */   150,  151,  156,   99,  102,  149,  150,  151,  156,   31,
 /*   190 */    42,  137,  135,  136,  107,  114,  116,  192,   16,   16,
 /*   200 */   193,  139,   16,    3,    8,  184,  185,   43,  137,  135,
 /*   210 */   136,   34,  141,   69,  154,  143,  144,  145,   52,   26,
 /*   220 */   147,  148,   82,   86,   87,  155,  150,  151,  156,  141,
 /*   230 */    69,  154,  143,  144,  145,   52,   67,  147,  148,   82,
 /*   240 */    86,   87,  155,  150,  151,  156,  110,  111,  113,   62,
 /*   250 */   212,   16,   59,   94,  137,  135,  136,   53,   68,  110,
 /*   260 */   111,  113,  110,  111,  113,   16,   16,   96,  166,  172,
 /*   270 */    74,  137,  135,  136,   90,  141,   69,  154,  143,  144,
 /*   280 */   145,   52,  174,  147,  148,   82,   86,   87,  155,  150,
 /*   290 */   151,  156,  141,   69,  154,  143,  144,  145,   52,   28,
 /*   300 */   147,  148,   82,   86,   87,  155,  150,  151,  156,  108,
 /*   310 */   112,  183,   16,  100,  187,  177,   75,  137,  135,  136,
 /*   320 */   118,  120,  197,  127,    2,  187,    3,   17,   16,   97,
 /*   330 */   110,  210,   30,   76,  137,  135,  136,   29,  141,   69,
 /*   340 */   154,  143,  144,  145,   52,   41,  147,  148,   82,   86,
 /*   350 */    87,  155,  150,  151,  156,  141,   69,  154,  143,  144,
 /*   360 */   145,   52,   19,  147,  148,   82,   86,   87,  155,  150,
 /*   370 */   151,  156,  127,  188,  178,  182,   17,  189,  191,  131,
 /*   380 */   137,  135,  136,  211,  194,  196,   29,  198,  128,  125,
 /*   390 */   200,  138,   16,    4,   32,   36,  132,  137,  135,  136,
 /*   400 */    35,  141,   69,  154,  143,  144,  145,   52,   22,  147,
 /*   410 */   148,   82,   86,   87,  155,  150,  151,  156,  141,   69,
 /*   420 */   154,  143,  144,  145,   52,  209,  147,  148,   82,   86,
 /*   430 */    87,  155,  150,  151,  156,  164,   51,    5,    6,  169,
 /*   440 */     7,   54,  133,  137,  135,  136,    9,   98,   56,   10,
 /*   450 */   173,   33,  203,  175,  104,   11,  101,  205,   37,   78,
 /*   460 */   137,  135,  136,   38,  141,   69,  154,  143,  144,  145,
 /*   470 */    52,  179,  147,  148,   82,   86,   87,  155,  150,  151,
 /*   480 */   156,  141,   69,  154,  143,  144,  145,   52,   14,  147,
 /*   490 */   148,   82,   86,   87,  155,  150,  151,  156,   44,  157,
 /*   500 */    57,  181,   49,   58,  206,  130,  134,  135,  136,   45,
 /*   510 */    60,   61,   39,   46,   63,   64,   47,   18,   65,  158,
 /*   520 */   159,  160,  161,  162,  163,   66,  152,  141,   69,  154,
 /*   530 */   143,  144,  145,   52,  208,  147,  148,   82,   86,   87,
 /*   540 */   155,  150,  151,  156,   85,  141,   69,  154,  143,  144,
 /*   550 */   145,   52,   12,  147,  148,   82,   86,   87,  155,  150,
 /*   560 */   151,  156,   89,  339,  339,  339,  339,  339,  339,  339,
 /*   570 */   339,  339,  339,  339,  339,  152,   70,  142,  143,  144,
 /*   580 */   145,   52,  339,  147,  148,   82,   86,   87,  155,  150,
 /*   590 */   151,  156,  339,   85,  141,   69,  154,  143,  144,  145,
 /*   600 */    52,  339,  147,  148,   82,   86,   87,  155,  150,  151,
 /*   610 */   156,   88,    1,  157,  339,  339,   20,   21,   23,   24,
 /*   620 */    25,   95,  157,   55,   48,  339,   16,  339,  339,  103,
 /*   630 */    15,   18,  339,  158,  159,  160,  161,  162,  163,  339,
 /*   640 */    18,  339,  158,  159,  160,  161,  162,  163,   77,  339,
 /*   650 */   339,  339,  339,  339,  339,  339,  339,  339,  339,  339,
 /*   660 */   339,  339,  339,  339,  339,   81,   19,  141,   69,  154,
 /*   670 */   143,  144,  145,   52,  339,  147,  148,   82,   86,   87,
 /*   680 */   155,  150,  151,  156,  141,   69,  154,  143,  144,  145,
 /*   690 */    52,  140,  147,  148,   82,   86,   87,  155,  150,  151,
 /*   700 */   156,  339,  339,  339,  339,  339,  339,  339,  339,  339,
 /*   710 */   141,   69,  154,  143,  144,  145,   52,  153,  147,  148,
 /*   720 */    82,   86,   87,  155,  150,  151,  156,  339,  339,  339,
 /*   730 */   339,  339,  339,  339,  167,  339,  141,   69,  154,  143,
 /*   740 */   144,  145,   52,  339,  147,  148,   82,   86,   87,  155,
 /*   750 */   150,  151,  156,  141,   69,  154,  143,  144,  145,   52,
 /*   760 */   168,  147,  148,   82,   86,   87,  155,  150,  151,  156,
 /*   770 */   339,  339,  339,  339,  339,  339,  339,  339,  339,  141,
 /*   780 */    69,  154,  143,  144,  145,   52,   91,  147,  148,   82,
 /*   790 */    86,   87,  155,  150,  151,  156,  339,  339,  339,  339,
 /*   800 */   339,  339,  339,   93,  339,  141,   69,  154,  143,  144,
 /*   810 */   145,   52,  339,  147,  148,   82,   86,   87,  155,  150,
 /*   820 */   151,  156,  141,   69,  154,  143,  144,  145,   52,  186,
 /*   830 */   147,  148,   82,   86,   87,  155,  150,  151,  156,  339,
 /*   840 */   339,  339,  339,  339,  339,  339,  339,  339,  141,   69,
 /*   850 */   154,  143,  144,  145,   52,  202,  147,  148,   82,   86,
 /*   860 */    87,  155,  150,  151,  156,  339,  339,  339,  339,  339,
 /*   870 */   339,  339,  126,  339,  141,   69,  154,  143,  144,  145,
 /*   880 */    52,  339,  147,  148,   82,   86,   87,  155,  150,  151,
 /*   890 */   156,  141,   69,  154,  143,  144,  145,   52,  339,  147,
 /*   900 */   148,   82,   86,   87,  155,  150,  151,  156,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    44,   45,   46,   47,   48,   57,   58,   59,   60,   61,
 /*    10 */    62,   51,   51,   65,   22,   23,   56,   56,   45,   46,
 /*    20 */    47,   48,   12,   67,   68,   69,   70,   71,   72,   73,
 /*    30 */     1,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*    40 */    67,   68,   69,   70,   71,   72,   73,   68,   75,   76,
 /*    50 */    77,   78,   79,   80,   81,   82,   83,   78,   79,   80,
 /*    60 */    81,   82,   83,   68,   45,   46,   47,   48,   73,   40,
 /*    70 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   61,
 /*    80 */    62,   45,   46,   47,   48,    1,   67,   68,   69,   70,
 /*    90 */    71,   72,   73,   25,   75,   76,   77,   78,   79,   80,
 /*   100 */    81,   82,   83,   67,   68,   69,   70,   71,   72,   73,
 /*   110 */    68,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   120 */    50,   79,   80,   81,   82,   83,   42,   45,   46,   47,
 /*   130 */    48,   57,   58,   59,   60,   61,   62,   61,   62,   65,
 /*   140 */    61,   62,   61,   62,   45,   46,   47,   48,   85,   67,
 /*   150 */    68,   69,   70,   71,   72,   73,   26,   75,   76,   77,
 /*   160 */    78,   79,   80,   81,   82,   83,   67,   68,   69,   70,
 /*   170 */    71,   72,   73,   68,   75,   76,   77,   78,   79,   80,
 /*   180 */    81,   82,   83,   54,   55,   80,   81,   82,   83,   74,
 /*   190 */    45,   46,   47,   48,   59,   60,   61,   62,    1,    1,
 /*   200 */    65,    4,    1,    5,    3,   63,   64,   45,   46,   47,
 /*   210 */    48,   30,   67,   68,   69,   70,   71,   72,   73,   18,
 /*   220 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   67,
 /*   230 */    68,   69,   70,   71,   72,   73,   12,   75,   76,   77,
 /*   240 */    78,   79,   80,   81,   82,   83,   22,   23,   24,   12,
 /*   250 */     0,    1,   12,   45,   46,   47,   48,   38,   39,   22,
 /*   260 */    23,   24,   22,   23,   24,    1,    1,   52,    4,    4,
 /*   270 */    45,   46,   47,   48,   86,   67,   68,   69,   70,   71,
 /*   280 */    72,   73,   12,   75,   76,   77,   78,   79,   80,   81,
 /*   290 */    82,   83,   67,   68,   69,   70,   71,   72,   73,   17,
 /*   300 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   60,
 /*   310 */    61,   62,    1,   55,   65,    4,   45,   46,   47,   48,
 /*   320 */    60,   61,   62,   16,    3,   65,    5,   20,    1,   53,
 /*   330 */    22,    4,   25,   45,   46,   47,   48,   30,   67,   68,
 /*   340 */    69,   70,   71,   72,   73,   49,   75,   76,   77,   78,
 /*   350 */    79,   80,   81,   82,   83,   67,   68,   69,   70,   71,
 /*   360 */    72,   73,   41,   75,   76,   77,   78,   79,   80,   81,
 /*   370 */    82,   83,   16,   64,   62,   62,   20,   62,   62,   45,
 /*   380 */    46,   47,   48,   87,   62,   62,   30,   62,   50,   86,
 /*   390 */    62,    4,    1,    1,   27,   29,   45,   46,   47,   48,
 /*   400 */    28,   67,   68,   69,   70,   71,   72,   73,   15,   75,
 /*   410 */    76,   77,   78,   79,   80,   81,   82,   83,   67,   68,
 /*   420 */    69,   70,   71,   72,   73,   87,   75,   76,   77,   78,
 /*   430 */    79,   80,   81,   82,   83,   31,   21,    1,    1,    4,
 /*   440 */     1,   12,   45,   46,   47,   48,    1,   15,   15,   21,
 /*   450 */    12,   20,   31,   12,   12,    1,   16,   31,   15,   45,
 /*   460 */    46,   47,   48,   15,   67,   68,   69,   70,   71,   72,
 /*   470 */    73,   12,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   480 */    83,   67,   68,   69,   70,   71,   72,   73,    1,   75,
 /*   490 */    76,   77,   78,   79,   80,   81,   82,   83,   15,   12,
 /*   500 */    15,   12,   12,   15,   12,   12,   46,   47,   48,   15,
 /*   510 */    15,   15,   15,   15,   15,   15,   15,   30,   15,   32,
 /*   520 */    33,   34,   35,   36,   37,   15,   48,   67,   68,   69,
 /*   530 */    70,   71,   72,   73,    4,   75,   76,   77,   78,   79,
 /*   540 */    80,   81,   82,   83,   66,   67,   68,   69,   70,   71,
 /*   550 */    72,   73,    1,   75,   76,   77,   78,   79,   80,   81,
 /*   560 */    82,   83,   84,   88,   88,   88,   88,   88,   88,   88,
 /*   570 */    88,   88,   88,   88,   88,   48,   68,   69,   70,   71,
 /*   580 */    72,   73,   88,   75,   76,   77,   78,   79,   80,   81,
 /*   590 */    82,   83,   88,   66,   67,   68,   69,   70,   71,   72,
 /*   600 */    73,   88,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   610 */    83,   84,    2,   12,   88,   88,    6,    7,    8,    9,
 /*   620 */    10,   11,   12,   13,   14,   88,    1,   88,   88,   19,
 /*   630 */     5,   30,   88,   32,   33,   34,   35,   36,   37,   88,
 /*   640 */    30,   88,   32,   33,   34,   35,   36,   37,   48,   88,
 /*   650 */    88,   88,   88,   88,   88,   88,   88,   88,   88,   88,
 /*   660 */    88,   88,   88,   88,   88,   48,   41,   67,   68,   69,
 /*   670 */    70,   71,   72,   73,   88,   75,   76,   77,   78,   79,
 /*   680 */    80,   81,   82,   83,   67,   68,   69,   70,   71,   72,
 /*   690 */    73,   48,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   700 */    83,   88,   88,   88,   88,   88,   88,   88,   88,   88,
 /*   710 */    67,   68,   69,   70,   71,   72,   73,   48,   75,   76,
 /*   720 */    77,   78,   79,   80,   81,   82,   83,   88,   88,   88,
 /*   730 */    88,   88,   88,   88,   48,   88,   67,   68,   69,   70,
 /*   740 */    71,   72,   73,   88,   75,   76,   77,   78,   79,   80,
 /*   750 */    81,   82,   83,   67,   68,   69,   70,   71,   72,   73,
 /*   760 */    48,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   770 */    88,   88,   88,   88,   88,   88,   88,   88,   88,   67,
 /*   780 */    68,   69,   70,   71,   72,   73,   48,   75,   76,   77,
 /*   790 */    78,   79,   80,   81,   82,   83,   88,   88,   88,   88,
 /*   800 */    88,   88,   88,   48,   88,   67,   68,   69,   70,   71,
 /*   810 */    72,   73,   88,   75,   76,   77,   78,   79,   80,   81,
 /*   820 */    82,   83,   67,   68,   69,   70,   71,   72,   73,   48,
 /*   830 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   88,
 /*   840 */    88,   88,   88,   88,   88,   88,   88,   88,   67,   68,
 /*   850 */    69,   70,   71,   72,   73,   48,   75,   76,   77,   78,
 /*   860 */    79,   80,   81,   82,   83,   88,   88,   88,   88,   88,
 /*   870 */    88,   88,   48,   88,   67,   68,   69,   70,   71,   72,
 /*   880 */    73,   88,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   890 */    83,   67,   68,   69,   70,   71,   72,   73,   88,   75,
 /*   900 */    76,   77,   78,   79,   80,   81,   82,   83,
};
#define YY_SHIFT_USE_DFLT (-9)
#define YY_SHIFT_MAX 133
static const short yy_shift_ofst[] = {
 /*     0 */   610,  610,  610,  610,  610,  610,  610,  610,  610,  610,
 /*    10 */   610,  610,  610,  610,  610,  610,  610,  601,  601,  487,
 /*    20 */   601,  601,  601,  601,  601,  601,  601,  601,  601,  601,
 /*    30 */   601,  601,  601,  224,  224,  601,  601,  237,  240,  240,
 /*    40 */   625,  321,  201,  201,   -8,   -8,   -8,   -8,   10,   68,
 /*    50 */   198,  219,  130,  181,  282,  270,   10,  308,  308,   68,
 /*    60 */   308,  308,   68,  308,  308,  308,  308,   68,  181,  307,
 /*    70 */   356,  250,  197,  264,  265,  311,   29,   84,  327,  387,
 /*    80 */   391,  392,  367,  372,  366,  393,  372,  366,  404,  415,
 /*    90 */   436,  437,  435,  439,  391,  429,  445,  432,  438,  433,
 /*   100 */   440,  441,  440,  442,  431,  428,  443,  448,  483,  485,
 /*   110 */   459,  489,  488,  490,  494,  495,  496,  497,  498,  499,
 /*   120 */   500,  501,  503,  510,  421,  454,  426,  492,  530,  493,
 /*   130 */   551,  391,  391,  391,
};
#define YY_REDUCE_USE_DFLT (-53)
#define YY_REDUCE_MAX 68
static const short yy_reduce_ofst[] = {
 /*     0 */   -44,  -27,   19,   36,   82,   99,  145,  162,  208,  225,
 /*    10 */   271,  288,  334,  351,  397,  414,  460,  478,  527,  600,
 /*    20 */   617,  643,  669,  686,  712,  738,  755,  781,  807,  824,
 /*    30 */   508,   -5,  -21,  -52,   74,   42,  105,  135,  249,  260,
 /*    40 */   296,  338,  -40,  -39,   18,   76,   79,   81,  129,  142,
 /*    50 */    70,   63,  115,  188,  215,  276,  258,  312,  313,  309,
 /*    60 */   315,  316,  309,  322,  323,  325,  328,  309,  303,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   215,  215,  215,  215,  215,  215,  215,  215,  215,  215,
 /*    10 */   215,  215,  215,  215,  215,  215,  215,  323,  323,  337,
 /*    20 */   337,  222,  337,  224,  226,  337,  337,  337,  337,  337,
 /*    30 */   337,  337,  337,  276,  276,  337,  337,  337,  337,  337,
 /*    40 */   337,  335,  242,  242,  337,  337,  337,  337,  337,  280,
 /*    50 */   335,  325,  296,  328,  238,  337,  337,  337,  337,  337,
 /*    60 */   337,  337,  284,  337,  337,  337,  337,  283,  328,  310,
 /*    70 */   310,  337,  337,  337,  337,  337,  337,  337,  337,  337,
 /*    80 */   336,  337,  301,  303,  305,  324,  302,  304,  337,  337,
 /*    90 */   337,  337,  337,  337,  243,  337,  337,  230,  337,  231,
 /*   100 */   233,  337,  232,  337,  337,  337,  260,  252,  248,  246,
 /*   110 */   337,  337,  250,  337,  256,  254,  258,  268,  264,  262,
 /*   120 */   266,  272,  270,  274,  337,  337,  337,  337,  337,  337,
 /*   130 */   337,  332,  333,  334,  214,  216,  217,  213,  218,  221,
 /*   140 */   223,  290,  291,  293,  294,  295,  297,  299,  300,  307,
 /*   150 */   308,  309,  288,  289,  292,  306,  311,  315,  316,  317,
 /*   160 */   318,  319,  320,  321,  322,  312,  326,  225,  227,  228,
 /*   170 */   240,  241,  229,  237,  236,  235,  234,  244,  245,  277,
 /*   180 */   247,  278,  249,  251,  279,  281,  282,  286,  287,  253,
 /*   190 */   255,  257,  259,  285,  261,  263,  265,  267,  269,  271,
 /*   200 */   273,  275,  239,  329,  327,  313,  314,  298,  219,  331,
 /*   210 */   220,  330,
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
  "EQUAL_TILDA",   "PLUS",          "LBRACKET",      "RBRACKET",    
  "NUMBER",        "REGEXP",        "STRING",        "TRUE",        
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
 /*  95 */ "arith_expr ::= arith_expr PLUS term",
 /*  96 */ "term ::= factor",
 /*  97 */ "factor ::= power",
 /*  98 */ "power ::= postfix_expr",
 /*  99 */ "postfix_expr ::= atom",
 /* 100 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 101 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 102 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 103 */ "atom ::= NAME",
 /* 104 */ "atom ::= NUMBER",
 /* 105 */ "atom ::= REGEXP",
 /* 106 */ "atom ::= STRING",
 /* 107 */ "atom ::= TRUE",
 /* 108 */ "atom ::= FALSE",
 /* 109 */ "atom ::= LINE",
 /* 110 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 111 */ "args_opt ::=",
 /* 112 */ "args_opt ::= args",
 /* 113 */ "blockarg_opt ::=",
 /* 114 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 115 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
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
  { 46, 7 },
  { 46, 5 },
  { 46, 5 },
  { 46, 5 },
  { 46, 1 },
  { 46, 2 },
  { 46, 1 },
  { 46, 2 },
  { 46, 1 },
  { 46, 2 },
  { 46, 6 },
  { 46, 6 },
  { 46, 2 },
  { 46, 2 },
  { 54, 1 },
  { 54, 3 },
  { 55, 1 },
  { 55, 3 },
  { 53, 1 },
  { 53, 3 },
  { 52, 0 },
  { 52, 2 },
  { 51, 1 },
  { 51, 5 },
  { 56, 0 },
  { 56, 2 },
  { 47, 7 },
  { 57, 9 },
  { 57, 7 },
  { 57, 7 },
  { 57, 5 },
  { 57, 7 },
  { 57, 5 },
  { 57, 5 },
  { 57, 3 },
  { 57, 7 },
  { 57, 5 },
  { 57, 5 },
  { 57, 3 },
  { 57, 5 },
  { 57, 3 },
  { 57, 3 },
  { 57, 1 },
  { 57, 7 },
  { 57, 5 },
  { 57, 5 },
  { 57, 3 },
  { 57, 5 },
  { 57, 3 },
  { 57, 3 },
  { 57, 1 },
  { 57, 5 },
  { 57, 3 },
  { 57, 3 },
  { 57, 1 },
  { 57, 3 },
  { 57, 1 },
  { 57, 1 },
  { 57, 0 },
  { 62, 2 },
  { 61, 2 },
  { 60, 3 },
  { 63, 0 },
  { 63, 1 },
  { 64, 2 },
  { 58, 1 },
  { 58, 3 },
  { 59, 1 },
  { 59, 3 },
  { 65, 2 },
  { 66, 1 },
  { 66, 3 },
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
  { 83, 3 },
  { 84, 0 },
  { 84, 1 },
  { 85, 0 },
  { 85, 5 },
  { 85, 5 },
  { 86, 0 },
  { 86, 3 },
  { 49, 1 },
  { 49, 2 },
  { 87, 6 },
  { 87, 4 },
  { 87, 3 },
  { 50, 0 },
  { 50, 2 },
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
#line 597 "parser.y"
{
    *pval = yymsp[0].minor.yy111;
}
#line 1753 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 118: /* excepts ::= except */
#line 601 "parser.y"
{
    yygotominor.yy111 = make_array_with(env, yymsp[0].minor.yy111);
}
#line 1764 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 604 "parser.y"
{
    yygotominor.yy111 = Array_push(env, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 1774 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 111: /* args_opt ::= */
      case 113: /* blockarg_opt ::= */
      case 116: /* blockarg_params_opt ::= */
      case 123: /* finally_opt ::= */
#line 608 "parser.y"
{
    yygotominor.yy111 = YNIL;
}
#line 1788 "parser.c"
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
      case 96: /* term ::= factor */
      case 97: /* factor ::= power */
      case 98: /* power ::= postfix_expr */
      case 99: /* postfix_expr ::= atom */
      case 112: /* args_opt ::= args */
      case 124: /* finally_opt ::= FINALLY stmts */
#line 611 "parser.y"
{
    yygotominor.yy111 = yymsp[0].minor.yy111;
}
#line 1818 "parser.c"
        break;
      case 5: /* stmt ::= expr */
#line 614 "parser.y"
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
#line 1832 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 624 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy111 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1840 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 628 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[-1].minor.yy111);
}
#line 1848 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 632 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = Finally_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1856 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 636 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = While_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1864 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 640 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Break_new(env, lineno, YNIL);
}
#line 1872 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 644 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Break_new(env, lineno, yymsp[0].minor.yy111);
}
#line 1880 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 648 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Next_new(env, lineno, YNIL);
}
#line 1888 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 652 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Next_new(env, lineno, yymsp[0].minor.yy111);
}
#line 1896 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 656 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Return_new(env, lineno, YNIL);
}
#line 1904 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 660 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Return_new(env, lineno, yymsp[0].minor.yy111);
}
#line 1912 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 664 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy111 = If_new(env, lineno, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1920 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 668 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy111 = Klass_new(env, lineno, id, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1929 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 673 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Nonlocal_new(env, lineno, yymsp[0].minor.yy111);
}
#line 1937 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 677 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Import_new(env, lineno, yymsp[0].minor.yy111);
}
#line 1945 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 689 "parser.y"
{
    yygotominor.yy111 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 1953 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 692 "parser.y"
{
    yygotominor.yy111 = Array_push_token_id(env, yymsp[-2].minor.yy111, yymsp[0].minor.yy0);
}
#line 1961 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 713 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111, yymsp[0].minor.yy111);
    yygotominor.yy111 = make_array_with(env, node);
}
#line 1970 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 726 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy111 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 1979 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 732 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-8].minor.yy111, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 1986 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 735 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 1993 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 738 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2000 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 741 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2007 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 744 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2014 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 747 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2021 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 750 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2028 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 753 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL, YNIL, YNIL);
}
#line 2035 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 756 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2042 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 759 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2049 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 762 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2056 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 765 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2063 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 768 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, YNIL, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2070 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 771 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, YNIL, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2077 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 774 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, YNIL, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2084 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 777 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[0].minor.yy111, YNIL, YNIL, YNIL, YNIL);
}
#line 2091 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 780 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2098 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 783 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2105 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 786 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2112 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 789 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2119 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 792 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2126 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 795 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2133 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 798 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-2].minor.yy111, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2140 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 801 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[0].minor.yy111, YNIL, YNIL, YNIL);
}
#line 2147 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 804 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2154 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 807 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2161 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 810 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2168 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 813 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2175 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 816 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2182 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 819 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2189 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 822 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2196 "parser.c"
        break;
      case 64: /* params ::= */
#line 825 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2203 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 829 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2212 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 835 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2221 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 841 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy111);
}
#line 2230 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 858 "parser.y"
{
    yygotominor.yy111 = YogArray_new(env);
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy111, lineno, id, YNIL);
}
#line 2240 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 864 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy111, lineno, id, YNIL);
    yygotominor.yy111 = yymsp[-2].minor.yy111;
}
#line 2250 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 878 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy111);
}
#line 2259 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 895 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    yygotominor.yy111 = Assign_new(env, lineno, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2267 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 918 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy111)->u.id;
    yygotominor.yy111 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy111, id, yymsp[0].minor.yy111);
}
#line 2276 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
#line 924 "parser.y"
{
    yygotominor.yy111 = yymsp[0].minor.yy0;
}
#line 2283 "parser.c"
        break;
      case 91: /* shift_expr ::= shift_expr LSHIFT match_expr */
      case 93: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
      case 95: /* arith_expr ::= arith_expr PLUS term */
#line 943 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy111 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy111, id, yymsp[0].minor.yy111);
}
#line 2294 "parser.c"
        break;
      case 100: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 982 "parser.y"
{
    yygotominor.yy111 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy111), yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2301 "parser.c"
        break;
      case 101: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 985 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-3].minor.yy111);
    yygotominor.yy111 = Subscript_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 2309 "parser.c"
        break;
      case 102: /* postfix_expr ::= postfix_expr DOT NAME */
#line 989 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Attr_new(env, lineno, yymsp[-2].minor.yy111, id);
}
#line 2318 "parser.c"
        break;
      case 103: /* atom ::= NAME */
#line 995 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Variable_new(env, lineno, id);
}
#line 2327 "parser.c"
        break;
      case 104: /* atom ::= NUMBER */
      case 105: /* atom ::= REGEXP */
      case 106: /* atom ::= STRING */
#line 1000 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy111 = Literal_new(env, lineno, val);
}
#line 2338 "parser.c"
        break;
      case 107: /* atom ::= TRUE */
#line 1015 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Literal_new(env, lineno, YTRUE);
}
#line 2346 "parser.c"
        break;
      case 108: /* atom ::= FALSE */
#line 1019 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Literal_new(env, lineno, YFALSE);
}
#line 2354 "parser.c"
        break;
      case 109: /* atom ::= LINE */
#line 1023 "parser.y"
{
    unsigned int lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy111 = Literal_new(env, lineno, val);
}
#line 2363 "parser.c"
        break;
      case 110: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1028 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy111 = Array_new(env, lineno, yymsp[-1].minor.yy111);
}
#line 2371 "parser.c"
        break;
      case 114: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 115: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1043 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = BlockArg_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 2380 "parser.c"
        break;
      case 117: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1055 "parser.y"
{
    yygotominor.yy111 = yymsp[-1].minor.yy111;
}
#line 2387 "parser.c"
        break;
      case 119: /* excepts ::= excepts except */
#line 1062 "parser.y"
{
    yygotominor.yy111 = Array_push(env, yymsp[-1].minor.yy111, yymsp[0].minor.yy111);
}
#line 2394 "parser.c"
        break;
      case 120: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1066 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy111 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy111, id, yymsp[0].minor.yy111);
}
#line 2404 "parser.c"
        break;
      case 121: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1072 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy111 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy111, NO_EXC_VAR, yymsp[0].minor.yy111);
}
#line 2412 "parser.c"
        break;
      case 122: /* except ::= EXCEPT NEWLINE stmts */
#line 1076 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy111 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy111);
}
#line 2420 "parser.c"
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
