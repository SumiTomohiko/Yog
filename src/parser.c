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
#define YYNOCODE 91
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy127;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 216
#define YYNRULE 128
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
 /*     0 */   345,   74,  138,  136,  137,  106,  107,  118,  122,  124,
 /*    10 */   205,  180,   93,  197,  111,  112,   42,  174,   27,   41,
 /*    20 */   138,  136,  137,  142,   72,  156,  144,  145,  146,   55,
 /*    30 */    36,  148,  149,   85,   88,   53,  159,   82,  151,  153,
 /*    40 */   160,  142,   72,  156,  144,  145,  146,   55,   73,  148,
 /*    50 */   149,   85,   88,   53,  159,  215,  151,  153,  160,   52,
 /*    60 */   159,   16,  151,  153,  160,   15,   51,  138,  136,  137,
 /*    70 */    73,  143,  144,  145,  146,   55,  169,  148,  149,   85,
 /*    80 */    88,   53,  159,  211,  151,  153,  160,  129,  142,   72,
 /*    90 */   156,  144,  145,  146,   55,   73,  148,  149,   85,   88,
 /*   100 */    53,  159,   19,  151,  153,  160,   31,  150,   34,  151,
 /*   110 */   153,  160,   73,   83,  138,  136,  137,  147,   73,  148,
 /*   120 */   149,   85,   88,   53,  159,  213,  151,  153,  160,   91,
 /*   130 */   110,  184,  152,  153,  160,  142,   72,  156,  144,  145,
 /*   140 */   146,   55,   14,  148,  149,   85,   88,   53,  159,   16,
 /*   150 */   151,  153,  160,  161,   73,   16,   16,   97,    8,    3,
 /*   160 */    75,  138,  136,  137,   86,   53,  159,   28,  151,  153,
 /*   170 */   160,   38,   18,   26,  162,  163,  164,  165,  166,  167,
 /*   180 */   116,  194,  142,   72,  156,  144,  145,  146,   55,  208,
 /*   190 */   148,  149,   85,   88,   53,  159,  161,  151,  153,  160,
 /*   200 */   108,  115,  117,  196,  120,  199,  197,   76,  138,  136,
 /*   210 */   137,  123,  203,   70,   38,   18,   65,  162,  163,  164,
 /*   220 */   165,  166,  167,  111,  112,  114,  111,  112,  114,  142,
 /*   230 */    72,  156,  144,  145,  146,   55,  178,  148,  149,   85,
 /*   240 */    88,   53,  159,   98,  151,  153,  160,  109,  113,  187,
 /*   250 */   101,    2,  191,    3,   43,  138,  136,  137,  125,  107,
 /*   260 */   118,  122,  124,  205,  100,  103,  197,  175,  119,  121,
 /*   270 */   201,  111,  174,  191,  188,  189,  142,   72,  156,  144,
 /*   280 */   145,  146,   55,  182,  148,  149,   85,   88,   53,  159,
 /*   290 */    19,  151,  153,  160,  128,   62,  157,  158,   17,   56,
 /*   300 */    71,   44,  138,  136,  137,  111,  112,  114,  186,   29,
 /*   310 */   216,   16,   16,   16,   16,  140,  170,  176,   16,   13,
 /*   320 */   126,  181,  192,  142,   72,  156,  144,  145,  146,   55,
 /*   330 */   139,  148,  149,   85,   88,   53,  159,  193,  151,  153,
 /*   340 */   160,   16,  195,  128,  214,  198,   16,   17,   95,  138,
 /*   350 */   136,  137,   30,  200,  202,  204,    4,   32,   29,   35,
 /*   360 */   168,  130,   22,   54,    5,    6,  173,    7,   57,    9,
 /*   370 */   142,   72,  156,  144,  145,  146,   55,  177,  148,  149,
 /*   380 */    85,   88,   53,  159,   99,  151,  153,  160,   59,  179,
 /*   390 */   102,  105,   33,  183,  185,   77,  138,  136,  137,   10,
 /*   400 */    37,   39,   45,   60,   61,   50,   46,   63,  207,   64,
 /*   410 */    40,   11,   47,   66,   67,   48,   68,  142,   72,  156,
 /*   420 */   144,  145,  146,   55,  210,  148,  149,   85,   88,   53,
 /*   430 */   159,   69,  151,  153,  160,  209,  212,  131,   12,  346,
 /*   440 */   346,  346,   78,  138,  136,  137,  346,  346,  346,  346,
 /*   450 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   460 */   346,  346,  346,  346,  142,   72,  156,  144,  145,  146,
 /*   470 */    55,  346,  148,  149,   85,   88,   53,  159,  346,  151,
 /*   480 */   153,  160,  346,  346,  346,  346,  346,  346,  346,   79,
 /*   490 */   138,  136,  137,  346,  346,  346,  346,  346,  346,  346,
 /*   500 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   510 */   346,  142,   72,  156,  144,  145,  146,   55,  346,  148,
 /*   520 */   149,   85,   88,   53,  159,  346,  151,  153,  160,  346,
 /*   530 */   346,  346,  346,  346,  346,  346,  132,  138,  136,  137,
 /*   540 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   550 */   346,  346,  346,  346,  346,  346,  346,  346,  142,   72,
 /*   560 */   156,  144,  145,  146,   55,  346,  148,  149,   85,   88,
 /*   570 */    53,  159,  346,  151,  153,  160,  346,  346,  346,  346,
 /*   580 */   346,  346,  346,  133,  138,  136,  137,  346,  346,  346,
 /*   590 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   600 */   346,  346,  346,  346,  346,  142,   72,  156,  144,  145,
 /*   610 */   146,   55,  346,  148,  149,   85,   88,   53,  159,  346,
 /*   620 */   151,  153,  160,  346,  346,  346,  346,  346,  346,  346,
 /*   630 */   134,  138,  136,  137,  346,  346,  346,  346,  346,  346,
 /*   640 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   650 */   346,  346,  142,   72,  156,  144,  145,  146,   55,  346,
 /*   660 */   148,  149,   85,   88,   53,  159,  346,  151,  153,  160,
 /*   670 */   346,  346,  346,  346,  346,  346,  346,   81,  138,  136,
 /*   680 */   137,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   690 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  142,
 /*   700 */    72,  156,  144,  145,  146,   55,  346,  148,  149,   85,
 /*   710 */    88,   53,  159,    1,  151,  153,  160,   20,   21,   23,
 /*   720 */    24,   25,   96,  161,   58,   49,  346,  346,  346,  346,
 /*   730 */   104,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   740 */   346,   38,   18,  346,  162,  163,  164,  165,  166,  167,
 /*   750 */   135,  136,  137,  346,  346,  346,  346,  346,  346,  346,
 /*   760 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   770 */   346,  142,   72,  156,  144,  145,  146,   55,  346,  148,
 /*   780 */   149,   85,   88,   53,  159,  346,  151,  153,  160,  346,
 /*   790 */   346,  154,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   800 */   346,  346,  346,  346,  346,  346,  346,  346,  346,   87,
 /*   810 */   142,   72,  156,  144,  145,  146,   55,  346,  148,  149,
 /*   820 */    85,   88,   53,  159,  154,  151,  153,  160,   90,  346,
 /*   830 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   840 */   346,  346,   87,  142,   72,  156,  144,  145,  146,   55,
 /*   850 */   346,  148,  149,   85,   88,   53,  159,  346,  151,  153,
 /*   860 */   160,   89,   80,  346,  346,  346,  346,  346,  346,  346,
 /*   870 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   880 */   346,  142,   72,  156,  144,  145,  146,   55,  346,  148,
 /*   890 */   149,   85,   88,   53,  159,   84,  151,  153,  160,  346,
 /*   900 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   910 */   346,  346,  346,  346,  142,   72,  156,  144,  145,  146,
 /*   920 */    55,  346,  148,  149,   85,   88,   53,  159,  346,  151,
 /*   930 */   153,  160,  141,  346,  346,  346,  346,  346,  346,  346,
 /*   940 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   950 */   346,  142,   72,  156,  144,  145,  146,   55,  346,  148,
 /*   960 */   149,   85,   88,   53,  159,  155,  151,  153,  160,  346,
 /*   970 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*   980 */   346,  346,  346,  346,  142,   72,  156,  144,  145,  146,
 /*   990 */    55,  346,  148,  149,   85,   88,   53,  159,  346,  151,
 /*  1000 */   153,  160,  171,  346,  346,  346,  346,  346,  346,  346,
 /*  1010 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*  1020 */   346,  142,   72,  156,  144,  145,  146,   55,  346,  148,
 /*  1030 */   149,   85,   88,   53,  159,  172,  151,  153,  160,  346,
 /*  1040 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*  1050 */   346,  346,  346,  346,  142,   72,  156,  144,  145,  146,
 /*  1060 */    55,  346,  148,  149,   85,   88,   53,  159,  346,  151,
 /*  1070 */   153,  160,   92,  346,  346,  346,  346,  346,  346,  346,
 /*  1080 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*  1090 */   346,  142,   72,  156,  144,  145,  146,   55,  346,  148,
 /*  1100 */   149,   85,   88,   53,  159,   94,  151,  153,  160,  346,
 /*  1110 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*  1120 */   346,  346,  346,  346,  142,   72,  156,  144,  145,  146,
 /*  1130 */    55,  346,  148,  149,   85,   88,   53,  159,  346,  151,
 /*  1140 */   153,  160,  190,  346,  346,  346,  346,  346,  346,  346,
 /*  1150 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*  1160 */   346,  142,   72,  156,  144,  145,  146,   55,  346,  148,
 /*  1170 */   149,   85,   88,   53,  159,  206,  151,  153,  160,  346,
 /*  1180 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*  1190 */   346,  346,  346,  346,  142,   72,  156,  144,  145,  146,
 /*  1200 */    55,  346,  148,  149,   85,   88,   53,  159,  346,  151,
 /*  1210 */   153,  160,  127,  346,  346,  346,  346,  346,  346,  346,
 /*  1220 */   346,  346,  346,  346,  346,  346,  346,  346,  346,  346,
 /*  1230 */   346,  142,   72,  156,  144,  145,  146,   55,  346,  148,
 /*  1240 */   149,   85,   88,   53,  159,  346,  151,  153,  160,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    45,   46,   47,   48,   49,   58,   59,   60,   61,   62,
 /*    10 */    63,   12,   52,   66,   22,   23,   50,   57,   25,   46,
 /*    20 */    47,   48,   49,   68,   69,   70,   71,   72,   73,   74,
 /*    30 */    82,   76,   77,   78,   79,   80,   81,   51,   83,   84,
 /*    40 */    85,   68,   69,   70,   71,   72,   73,   74,   69,   76,
 /*    50 */    77,   78,   79,   80,   81,   89,   83,   84,   85,   80,
 /*    60 */    81,    1,   83,   84,   85,    5,   46,   47,   48,   49,
 /*    70 */    69,   70,   71,   72,   73,   74,   87,   76,   77,   78,
 /*    80 */    79,   80,   81,   26,   83,   84,   85,   51,   68,   69,
 /*    90 */    70,   71,   72,   73,   74,   69,   76,   77,   78,   79,
 /*   100 */    80,   81,   42,   83,   84,   85,   75,   81,   31,   83,
 /*   110 */    84,   85,   69,   46,   47,   48,   49,   74,   69,   76,
 /*   120 */    77,   78,   79,   80,   81,   89,   83,   84,   85,   88,
 /*   130 */    62,   63,   83,   84,   85,   68,   69,   70,   71,   72,
 /*   140 */    73,   74,    1,   76,   77,   78,   79,   80,   81,    1,
 /*   150 */    83,   84,   85,   12,   69,    1,    1,   53,    3,    5,
 /*   160 */    46,   47,   48,   49,   79,   80,   81,   17,   83,   84,
 /*   170 */    85,   30,   31,   18,   33,   34,   35,   36,   37,   38,
 /*   180 */    62,   63,   68,   69,   70,   71,   72,   73,   74,   41,
 /*   190 */    76,   77,   78,   79,   80,   81,   12,   83,   84,   85,
 /*   200 */    60,   61,   62,   63,   62,   63,   66,   46,   47,   48,
 /*   210 */    49,   62,   63,   12,   30,   31,   12,   33,   34,   35,
 /*   220 */    36,   37,   38,   22,   23,   24,   22,   23,   24,   68,
 /*   230 */    69,   70,   71,   72,   73,   74,   12,   76,   77,   78,
 /*   240 */    79,   80,   81,   54,   83,   84,   85,   61,   62,   63,
 /*   250 */    56,    3,   66,    5,   46,   47,   48,   49,   58,   59,
 /*   260 */    60,   61,   62,   63,   55,   56,   66,   52,   61,   62,
 /*   270 */    63,   22,   57,   66,   64,   65,   68,   69,   70,   71,
 /*   280 */    72,   73,   74,   63,   76,   77,   78,   79,   80,   81,
 /*   290 */    42,   83,   84,   85,   16,   12,   29,   30,   20,   39,
 /*   300 */    40,   46,   47,   48,   49,   22,   23,   24,   63,   31,
 /*   310 */     0,    1,    1,    1,    1,    4,    4,    4,    1,    1,
 /*   320 */    88,    4,   65,   68,   69,   70,   71,   72,   73,   74,
 /*   330 */     4,   76,   77,   78,   79,   80,   81,   63,   83,   84,
 /*   340 */    85,    1,   63,   16,    4,   63,    1,   20,   46,   47,
 /*   350 */    48,   49,   25,   63,   63,   63,    1,   27,   31,   28,
 /*   360 */    32,   43,   15,   21,    1,    1,    4,    1,   12,    1,
 /*   370 */    68,   69,   70,   71,   72,   73,   74,   12,   76,   77,
 /*   380 */    78,   79,   80,   81,   15,   83,   84,   85,   15,   12,
 /*   390 */    16,   12,   20,   12,   12,   46,   47,   48,   49,   21,
 /*   400 */    15,   15,   15,   15,   15,   12,   15,   15,   32,   15,
 /*   410 */    15,    1,   15,   15,   15,   15,   15,   68,   69,   70,
 /*   420 */    71,   72,   73,   74,   12,   76,   77,   78,   79,   80,
 /*   430 */    81,   15,   83,   84,   85,   32,    4,   12,    1,   90,
 /*   440 */    90,   90,   46,   47,   48,   49,   90,   90,   90,   90,
 /*   450 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   460 */    90,   90,   90,   90,   68,   69,   70,   71,   72,   73,
 /*   470 */    74,   90,   76,   77,   78,   79,   80,   81,   90,   83,
 /*   480 */    84,   85,   90,   90,   90,   90,   90,   90,   90,   46,
 /*   490 */    47,   48,   49,   90,   90,   90,   90,   90,   90,   90,
 /*   500 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   510 */    90,   68,   69,   70,   71,   72,   73,   74,   90,   76,
 /*   520 */    77,   78,   79,   80,   81,   90,   83,   84,   85,   90,
 /*   530 */    90,   90,   90,   90,   90,   90,   46,   47,   48,   49,
 /*   540 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   550 */    90,   90,   90,   90,   90,   90,   90,   90,   68,   69,
 /*   560 */    70,   71,   72,   73,   74,   90,   76,   77,   78,   79,
 /*   570 */    80,   81,   90,   83,   84,   85,   90,   90,   90,   90,
 /*   580 */    90,   90,   90,   46,   47,   48,   49,   90,   90,   90,
 /*   590 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   600 */    90,   90,   90,   90,   90,   68,   69,   70,   71,   72,
 /*   610 */    73,   74,   90,   76,   77,   78,   79,   80,   81,   90,
 /*   620 */    83,   84,   85,   90,   90,   90,   90,   90,   90,   90,
 /*   630 */    46,   47,   48,   49,   90,   90,   90,   90,   90,   90,
 /*   640 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   650 */    90,   90,   68,   69,   70,   71,   72,   73,   74,   90,
 /*   660 */    76,   77,   78,   79,   80,   81,   90,   83,   84,   85,
 /*   670 */    90,   90,   90,   90,   90,   90,   90,   46,   47,   48,
 /*   680 */    49,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   690 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   68,
 /*   700 */    69,   70,   71,   72,   73,   74,   90,   76,   77,   78,
 /*   710 */    79,   80,   81,    2,   83,   84,   85,    6,    7,    8,
 /*   720 */     9,   10,   11,   12,   13,   14,   90,   90,   90,   90,
 /*   730 */    19,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   740 */    90,   30,   31,   90,   33,   34,   35,   36,   37,   38,
 /*   750 */    47,   48,   49,   90,   90,   90,   90,   90,   90,   90,
 /*   760 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   770 */    90,   68,   69,   70,   71,   72,   73,   74,   90,   76,
 /*   780 */    77,   78,   79,   80,   81,   90,   83,   84,   85,   90,
 /*   790 */    90,   49,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   800 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   67,
 /*   810 */    68,   69,   70,   71,   72,   73,   74,   90,   76,   77,
 /*   820 */    78,   79,   80,   81,   49,   83,   84,   85,   86,   90,
 /*   830 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   840 */    90,   90,   67,   68,   69,   70,   71,   72,   73,   74,
 /*   850 */    90,   76,   77,   78,   79,   80,   81,   90,   83,   84,
 /*   860 */    85,   86,   49,   90,   90,   90,   90,   90,   90,   90,
 /*   870 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   880 */    90,   68,   69,   70,   71,   72,   73,   74,   90,   76,
 /*   890 */    77,   78,   79,   80,   81,   49,   83,   84,   85,   90,
 /*   900 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   910 */    90,   90,   90,   90,   68,   69,   70,   71,   72,   73,
 /*   920 */    74,   90,   76,   77,   78,   79,   80,   81,   90,   83,
 /*   930 */    84,   85,   49,   90,   90,   90,   90,   90,   90,   90,
 /*   940 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   950 */    90,   68,   69,   70,   71,   72,   73,   74,   90,   76,
 /*   960 */    77,   78,   79,   80,   81,   49,   83,   84,   85,   90,
 /*   970 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*   980 */    90,   90,   90,   90,   68,   69,   70,   71,   72,   73,
 /*   990 */    74,   90,   76,   77,   78,   79,   80,   81,   90,   83,
 /*  1000 */    84,   85,   49,   90,   90,   90,   90,   90,   90,   90,
 /*  1010 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*  1020 */    90,   68,   69,   70,   71,   72,   73,   74,   90,   76,
 /*  1030 */    77,   78,   79,   80,   81,   49,   83,   84,   85,   90,
 /*  1040 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*  1050 */    90,   90,   90,   90,   68,   69,   70,   71,   72,   73,
 /*  1060 */    74,   90,   76,   77,   78,   79,   80,   81,   90,   83,
 /*  1070 */    84,   85,   49,   90,   90,   90,   90,   90,   90,   90,
 /*  1080 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*  1090 */    90,   68,   69,   70,   71,   72,   73,   74,   90,   76,
 /*  1100 */    77,   78,   79,   80,   81,   49,   83,   84,   85,   90,
 /*  1110 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*  1120 */    90,   90,   90,   90,   68,   69,   70,   71,   72,   73,
 /*  1130 */    74,   90,   76,   77,   78,   79,   80,   81,   90,   83,
 /*  1140 */    84,   85,   49,   90,   90,   90,   90,   90,   90,   90,
 /*  1150 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*  1160 */    90,   68,   69,   70,   71,   72,   73,   74,   90,   76,
 /*  1170 */    77,   78,   79,   80,   81,   49,   83,   84,   85,   90,
 /*  1180 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*  1190 */    90,   90,   90,   90,   68,   69,   70,   71,   72,   73,
 /*  1200 */    74,   90,   76,   77,   78,   79,   80,   81,   90,   83,
 /*  1210 */    84,   85,   49,   90,   90,   90,   90,   90,   90,   90,
 /*  1220 */    90,   90,   90,   90,   90,   90,   90,   90,   90,   90,
 /*  1230 */    90,   68,   69,   70,   71,   72,   73,   74,   90,   76,
 /*  1240 */    77,   78,   79,   80,   81,   90,   83,   84,   85,
};
#define YY_SHIFT_USE_DFLT (-9)
#define YY_SHIFT_MAX 134
static const short yy_shift_ofst[] = {
 /*     0 */   711,  711,  711,  711,  711,  711,  711,  711,  711,  711,
 /*    10 */   711,  711,  711,  711,  711,  711,  711,  184,  184,  141,
 /*    20 */   184,  184,  184,  184,  184,  184,  184,  184,  184,  184,
 /*    30 */   184,  184,  184,  201,  201,  184,  184,  204,  184,  283,
 /*    40 */   283,   60,  248,  155,  155,   -8,   -8,   -8,   -8,   -1,
 /*    50 */    -7,  154,  267,  267,  260,   57,   77,  150,  224,   -1,
 /*    60 */   249,  249,   -7,  249,  249,   -7,  249,  249,  249,  249,
 /*    70 */    -7,   77,  327,  278,  310,  311,  312,  313,  317,  148,
 /*    80 */   318,  340,  326,  345,  355,  330,  331,  347,  331,  328,
 /*    90 */   342,  363,  364,  362,  366,  345,  356,  368,  369,  365,
 /*   100 */   373,  374,  377,  374,  379,  372,  378,  385,  386,  387,
 /*   110 */   388,  381,  382,  389,  393,  391,  392,  394,  395,  397,
 /*   120 */   398,  399,  400,  401,  416,  376,  410,  403,  412,  432,
 /*   130 */   425,  437,  345,  345,  345,
};
#define YY_REDUCE_USE_DFLT (-54)
#define YY_REDUCE_MAX 71
static const short yy_reduce_ofst[] = {
 /*     0 */   -45,  -27,   20,   67,  114,  161,  208,  255,  302,  349,
 /*    10 */   396,  443,  490,  537,  584,  631,  703,  742,  775,  813,
 /*    20 */   846,  883,  916,  953,  986, 1023, 1056, 1093, 1126, 1163,
 /*    30 */     1,   43,   85,  -53,  200,  -21,   26,  140,   49,  186,
 /*    40 */   207,  -34,   36,  -40,  215,   68,  118,  142,  149,  209,
 /*    50 */   210,  -14,  -52,  -52,  -11,   31,   41,  104,  189,  194,
 /*    60 */   220,  245,  257,  274,  279,  257,  282,  290,  291,  292,
 /*    70 */   257,  232,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   219,  219,  219,  219,  219,  219,  219,  219,  219,  219,
 /*    10 */   219,  219,  219,  219,  219,  219,  219,  330,  330,  344,
 /*    20 */   344,  226,  344,  228,  230,  344,  344,  344,  344,  344,
 /*    30 */   344,  344,  344,  280,  280,  344,  344,  344,  344,  344,
 /*    40 */   344,  344,  342,  246,  246,  344,  344,  344,  344,  344,
 /*    50 */   284,  342,  309,  308,  332,  300,  335,  242,  344,  344,
 /*    60 */   344,  344,  344,  344,  344,  288,  344,  344,  344,  344,
 /*    70 */   287,  335,  317,  317,  344,  344,  344,  344,  344,  344,
 /*    80 */   344,  344,  344,  343,  344,  305,  307,  331,  306,  344,
 /*    90 */   344,  344,  344,  344,  344,  247,  344,  344,  234,  344,
 /*   100 */   235,  237,  344,  236,  344,  344,  344,  264,  256,  252,
 /*   110 */   250,  344,  344,  254,  344,  260,  258,  262,  272,  268,
 /*   120 */   266,  270,  276,  274,  278,  344,  344,  344,  344,  344,
 /*   130 */   344,  344,  339,  340,  341,  218,  220,  221,  217,  222,
 /*   140 */   225,  227,  294,  295,  297,  298,  299,  301,  303,  304,
 /*   150 */   311,  314,  315,  316,  292,  293,  296,  312,  313,  310,
 /*   160 */   318,  322,  323,  324,  325,  326,  327,  328,  329,  319,
 /*   170 */   333,  229,  231,  232,  244,  245,  233,  241,  240,  239,
 /*   180 */   238,  248,  249,  281,  251,  282,  253,  255,  283,  285,
 /*   190 */   286,  290,  291,  257,  259,  261,  263,  289,  265,  267,
 /*   200 */   269,  271,  273,  275,  277,  279,  243,  336,  334,  320,
 /*   210 */   321,  302,  223,  338,  224,  337,
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
  "arith_expr",    "term",          "arith_op",      "factor",      
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
 /*  98 */ "term ::= factor",
 /*  99 */ "factor ::= MINUS factor",
 /* 100 */ "factor ::= power",
 /* 101 */ "power ::= postfix_expr",
 /* 102 */ "postfix_expr ::= atom",
 /* 103 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 104 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 105 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 106 */ "atom ::= NAME",
 /* 107 */ "atom ::= NUMBER",
 /* 108 */ "atom ::= REGEXP",
 /* 109 */ "atom ::= STRING",
 /* 110 */ "atom ::= TRUE",
 /* 111 */ "atom ::= FALSE",
 /* 112 */ "atom ::= LINE",
 /* 113 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 114 */ "args_opt ::=",
 /* 115 */ "args_opt ::= args",
 /* 116 */ "blockarg_opt ::=",
 /* 117 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 118 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 119 */ "blockarg_params_opt ::=",
 /* 120 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 121 */ "excepts ::= except",
 /* 122 */ "excepts ::= excepts except",
 /* 123 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 124 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 125 */ "except ::= EXCEPT NEWLINE stmts",
 /* 126 */ "finally_opt ::=",
 /* 127 */ "finally_opt ::= FINALLY stmts",
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
  { 81, 1 },
  { 83, 2 },
  { 83, 1 },
  { 84, 1 },
  { 69, 1 },
  { 69, 5 },
  { 69, 4 },
  { 69, 3 },
  { 85, 1 },
  { 85, 1 },
  { 85, 1 },
  { 85, 1 },
  { 85, 1 },
  { 85, 1 },
  { 85, 1 },
  { 85, 3 },
  { 86, 0 },
  { 86, 1 },
  { 87, 0 },
  { 87, 5 },
  { 87, 5 },
  { 88, 0 },
  { 88, 3 },
  { 50, 1 },
  { 50, 2 },
  { 89, 6 },
  { 89, 4 },
  { 89, 3 },
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
    *pval = yymsp[0].minor.yy127;
}
#line 1844 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 121: /* excepts ::= except */
#line 616 "parser.y"
{
    yygotominor.yy127 = make_array_with(env, yymsp[0].minor.yy127);
}
#line 1855 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 619 "parser.y"
{
    yygotominor.yy127 = Array_push(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 1865 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 114: /* args_opt ::= */
      case 116: /* blockarg_opt ::= */
      case 119: /* blockarg_params_opt ::= */
      case 126: /* finally_opt ::= */
#line 623 "parser.y"
{
    yygotominor.yy127 = YNIL;
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
      case 98: /* term ::= factor */
      case 100: /* factor ::= power */
      case 101: /* power ::= postfix_expr */
      case 102: /* postfix_expr ::= atom */
      case 115: /* args_opt ::= args */
      case 127: /* finally_opt ::= FINALLY stmts */
#line 626 "parser.y"
{
    yygotominor.yy127 = yymsp[0].minor.yy127;
}
#line 1909 "parser.c"
        break;
      case 5: /* stmt ::= expr */
#line 629 "parser.y"
{
    if (PTR_AS(YogNode, yymsp[0].minor.yy127)->type == NODE_VARIABLE) {
        unsigned int lineno = NODE_LINENO(yymsp[0].minor.yy127);
        ID id = PTR_AS(YogNode, yymsp[0].minor.yy127)->u.variable.id;
        yygotominor.yy127 = CommandCall_new(env, lineno, id, YNIL, YNIL);
    }
    else {
        yygotominor.yy127 = yymsp[0].minor.yy127;
    }
}
#line 1923 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 639 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy127 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[-1].minor.yy127);
}
#line 1931 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 643 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[-1].minor.yy127);
}
#line 1939 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 647 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = Finally_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 1947 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 651 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = While_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 1955 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 655 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Break_new(env, lineno, YNIL);
}
#line 1963 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 659 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Break_new(env, lineno, yymsp[0].minor.yy127);
}
#line 1971 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 663 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Next_new(env, lineno, YNIL);
}
#line 1979 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 667 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Next_new(env, lineno, yymsp[0].minor.yy127);
}
#line 1987 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 671 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Return_new(env, lineno, YNIL);
}
#line 1995 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 675 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Return_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2003 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 679 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy127 = If_new(env, lineno, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2011 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 683 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy127 = Klass_new(env, lineno, id, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2020 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 688 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Nonlocal_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2028 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 692 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Import_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2036 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 704 "parser.y"
{
    yygotominor.yy127 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2044 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 707 "parser.y"
{
    yygotominor.yy127 = Array_push_token_id(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy0);
}
#line 2052 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 728 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127, yymsp[0].minor.yy127);
    yygotominor.yy127 = make_array_with(env, node);
}
#line 2061 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 741 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy127 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2070 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 747 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-8].minor.yy127, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2077 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 750 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2084 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 753 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2091 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 756 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2098 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 759 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2105 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 762 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2112 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 765 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2119 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 768 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL, YNIL);
}
#line 2126 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 771 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2133 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 774 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2140 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 777 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2147 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 780 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2154 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 783 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2161 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 786 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2168 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 789 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, YNIL, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2175 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 792 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[0].minor.yy127, YNIL, YNIL, YNIL, YNIL);
}
#line 2182 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 795 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2189 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 798 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2196 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 801 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2203 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 804 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2210 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 807 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2217 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 810 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2224 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 813 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2231 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 816 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[0].minor.yy127, YNIL, YNIL, YNIL);
}
#line 2238 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 819 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2245 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 822 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2252 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 825 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2259 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 828 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2266 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 831 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2273 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 834 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2280 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 837 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2287 "parser.c"
        break;
      case 64: /* params ::= */
#line 840 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2294 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 844 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2303 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 850 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2312 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 856 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy127);
}
#line 2321 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 873 "parser.y"
{
    yygotominor.yy127 = YogArray_new(env);
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy127, lineno, id, YNIL);
}
#line 2331 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 879 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy127, lineno, id, YNIL);
    yygotominor.yy127 = yymsp[-2].minor.yy127;
}
#line 2341 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 893 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy127);
}
#line 2350 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 910 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    yygotominor.yy127 = Assign_new(env, lineno, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2358 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
      case 95: /* arith_expr ::= arith_expr arith_op term */
#line 933 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy127)->u.id;
    yygotominor.yy127 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy127, id, yymsp[0].minor.yy127);
}
#line 2368 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
      case 96: /* arith_op ::= PLUS */
      case 97: /* arith_op ::= MINUS */
#line 939 "parser.y"
{
    yygotominor.yy127 = yymsp[0].minor.yy0;
}
#line 2377 "parser.c"
        break;
      case 91: /* shift_expr ::= shift_expr LSHIFT match_expr */
      case 93: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 958 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy127 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy127, id, yymsp[0].minor.yy127);
}
#line 2387 "parser.c"
        break;
      case 99: /* factor ::= MINUS factor */
#line 993 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy127 = FuncCall_new3(env, lineno, yymsp[0].minor.yy127, id);
}
#line 2396 "parser.c"
        break;
      case 103: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1009 "parser.y"
{
    yygotominor.yy127 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy127), yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2403 "parser.c"
        break;
      case 104: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1012 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-3].minor.yy127);
    yygotominor.yy127 = Subscript_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2411 "parser.c"
        break;
      case 105: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1016 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Attr_new(env, lineno, yymsp[-2].minor.yy127, id);
}
#line 2420 "parser.c"
        break;
      case 106: /* atom ::= NAME */
#line 1022 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Variable_new(env, lineno, id);
}
#line 2429 "parser.c"
        break;
      case 107: /* atom ::= NUMBER */
      case 108: /* atom ::= REGEXP */
      case 109: /* atom ::= STRING */
#line 1027 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy127 = Literal_new(env, lineno, val);
}
#line 2440 "parser.c"
        break;
      case 110: /* atom ::= TRUE */
#line 1042 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Literal_new(env, lineno, YTRUE);
}
#line 2448 "parser.c"
        break;
      case 111: /* atom ::= FALSE */
#line 1046 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Literal_new(env, lineno, YFALSE);
}
#line 2456 "parser.c"
        break;
      case 112: /* atom ::= LINE */
#line 1050 "parser.y"
{
    unsigned int lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy127 = Literal_new(env, lineno, val);
}
#line 2465 "parser.c"
        break;
      case 113: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1055 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy127 = Array_new(env, lineno, yymsp[-1].minor.yy127);
}
#line 2473 "parser.c"
        break;
      case 117: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 118: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1070 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = BlockArg_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2482 "parser.c"
        break;
      case 120: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1082 "parser.y"
{
    yygotominor.yy127 = yymsp[-1].minor.yy127;
}
#line 2489 "parser.c"
        break;
      case 122: /* excepts ::= excepts except */
#line 1089 "parser.y"
{
    yygotominor.yy127 = Array_push(env, yymsp[-1].minor.yy127, yymsp[0].minor.yy127);
}
#line 2496 "parser.c"
        break;
      case 123: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1093 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy127 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy127, id, yymsp[0].minor.yy127);
}
#line 2506 "parser.c"
        break;
      case 124: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1099 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy127 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy127, NO_EXC_VAR, yymsp[0].minor.yy127);
}
#line 2514 "parser.c"
        break;
      case 125: /* except ::= EXCEPT NEWLINE stmts */
#line 1103 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy127 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy127);
}
#line 2522 "parser.c"
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
