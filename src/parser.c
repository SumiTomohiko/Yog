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
#define YYNOCODE 90
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy119;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 214
#define YYNRULE 126
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
 /*     0 */   341,   72,  138,  136,  137,  106,  107,  118,  122,  124,
 /*    10 */   203,   93,  173,  195,  111,  112,  172,  172,   41,  138,
 /*    20 */   136,  137,  178,  142,   70,  156,  144,  145,  146,   53,
 /*    30 */    16,  148,  149,   83,   87,   88,  157,  151,  153,  158,
 /*    40 */   142,   70,  156,  144,  145,  146,   53,   71,  148,  149,
 /*    50 */    83,   87,   88,  157,  151,  153,  158,   84,   88,  157,
 /*    60 */   151,  153,  158,   27,   71,   51,  138,  136,  137,  147,
 /*    70 */   206,  148,  149,   83,   87,   88,  157,  151,  153,  158,
 /*    80 */   110,  182,   81,  138,  136,  137,   80,  142,   70,  156,
 /*    90 */   144,  145,  146,   53,  167,  148,  149,   83,   87,   88,
 /*   100 */   157,  151,  153,  158,  142,   70,  156,  144,  145,  146,
 /*   110 */    53,   71,  148,  149,   83,   87,   88,  157,  151,  153,
 /*   120 */   158,  129,   85,  157,  151,  153,  158,  116,  192,   73,
 /*   130 */   138,  136,  137,  125,  107,  118,  122,  124,  203,  120,
 /*   140 */   197,  195,  123,  201,  100,  103,   74,  138,  136,  137,
 /*   150 */   209,  142,   70,  156,  144,  145,  146,   53,  211,  148,
 /*   160 */   149,   83,   87,   88,  157,  151,  153,  158,  142,   70,
 /*   170 */   156,  144,  145,  146,   53,   71,  148,  149,   83,   87,
 /*   180 */    88,  157,  151,  153,  158,  186,  187,  150,  151,  153,
 /*   190 */   158,   31,   71,   43,  138,  136,  137,  108,  115,  117,
 /*   200 */   194,   54,   69,  195,   34,  152,  153,  158,  214,   16,
 /*   210 */    44,  138,  136,  137,   91,  142,   70,  156,  144,  145,
 /*   220 */   146,   53,   28,  148,  149,   83,   87,   88,  157,  151,
 /*   230 */   153,  158,  142,   70,  156,  144,  145,  146,   53,   68,
 /*   240 */   148,  149,   83,   87,   88,  157,  151,  153,  158,  111,
 /*   250 */   112,  114,   63,   97,   16,   60,    8,   95,  138,  136,
 /*   260 */   137,   13,  111,  112,  114,  111,  112,  114,  109,  113,
 /*   270 */   185,   26,   98,  189,   75,  138,  136,  137,  176,  142,
 /*   280 */    70,  156,  144,  145,  146,   53,  101,  148,  149,   83,
 /*   290 */    87,   88,  157,  151,  153,  158,  142,   70,  156,  144,
 /*   300 */   145,  146,   53,  130,  148,  149,   83,   87,   88,  157,
 /*   310 */   151,  153,  158,  119,  121,  199,  128,  111,  189,  180,
 /*   320 */    17,   76,  138,  136,  137,   30,   16,  184,   16,  126,
 /*   330 */    15,   29,    3,   16,   16,  190,  140,  168,   77,  138,
 /*   340 */   136,  137,  191,  142,   70,  156,  144,  145,  146,   53,
 /*   350 */   193,  148,  149,   83,   87,   88,  157,  151,  153,  158,
 /*   360 */   142,   70,  156,  144,  145,  146,   53,   19,  148,  149,
 /*   370 */    83,   87,   88,  157,  151,  153,  158,   16,  128,  196,
 /*   380 */   174,  198,   17,  200,  202,  132,  138,  136,  137,  139,
 /*   390 */    32,   16,    2,   29,    3,   16,   16,    4,  179,  212,
 /*   400 */    35,   22,  133,  138,  136,  137,   36,  142,   70,  156,
 /*   410 */   144,  145,  146,   53,  166,  148,  149,   83,   87,   88,
 /*   420 */   157,  151,  153,  158,  142,   70,  156,  144,  145,  146,
 /*   430 */    53,   19,  148,  149,   83,   87,   88,  157,  151,  153,
 /*   440 */   158,    5,   52,    6,    7,  171,   55,    9,   99,  134,
 /*   450 */   138,  136,  137,   57,  205,  102,  175,  177,   10,  105,
 /*   460 */    11,  207,  210,   37,   39,  181,   79,  138,  136,  137,
 /*   470 */    33,  142,   70,  156,  144,  145,  146,   53,  183,  148,
 /*   480 */   149,   83,   87,   88,  157,  151,  153,  158,  142,   70,
 /*   490 */   156,  144,  145,  146,   53,   45,  148,  149,   83,   87,
 /*   500 */    88,  157,  151,  153,  158,    1,   58,   59,   50,   20,
 /*   510 */    21,   23,   24,   25,   96,  159,   56,   49,  342,   46,
 /*   520 */    61,   62,  104,  135,  136,  137,   40,   47,   64,   65,
 /*   530 */   208,   42,   48,   38,   18,   66,  160,  161,  162,  163,
 /*   540 */   164,  165,   67,  154,  142,   70,  156,  144,  145,  146,
 /*   550 */    53,  131,  148,  149,   83,   87,   88,  157,  151,  153,
 /*   560 */   158,   86,  142,   70,  156,  144,  145,  146,   53,  213,
 /*   570 */   148,  149,   83,   87,   88,  157,  151,  153,  158,   90,
 /*   580 */    12,  342,  342,  154,   71,  143,  144,  145,  146,   53,
 /*   590 */   342,  148,  149,   83,   87,   88,  157,  151,  153,  158,
 /*   600 */   342,   86,  142,   70,  156,  144,  145,  146,   53,   78,
 /*   610 */   148,  149,   83,   87,   88,  157,  151,  153,  158,   89,
 /*   620 */   342,  342,  342,  342,  342,  342,   82,  342,  142,   70,
 /*   630 */   156,  144,  145,  146,   53,  342,  148,  149,   83,   87,
 /*   640 */    88,  157,  151,  153,  158,  142,   70,  156,  144,  145,
 /*   650 */   146,   53,  141,  148,  149,   83,   87,   88,  157,  151,
 /*   660 */   153,  158,  342,  342,  342,  342,  342,  342,  342,  155,
 /*   670 */   342,  142,   70,  156,  144,  145,  146,   53,  342,  148,
 /*   680 */   149,   83,   87,   88,  157,  151,  153,  158,  142,   70,
 /*   690 */   156,  144,  145,  146,   53,  169,  148,  149,   83,   87,
 /*   700 */    88,  157,  151,  153,  158,  342,  342,  342,  342,  342,
 /*   710 */   342,  342,  342,  342,  142,   70,  156,  144,  145,  146,
 /*   720 */    53,  170,  148,  149,   83,   87,   88,  157,  151,  153,
 /*   730 */   158,  342,  342,  342,  342,  342,  342,  342,   92,  342,
 /*   740 */   142,   70,  156,  144,  145,  146,   53,  342,  148,  149,
 /*   750 */    83,   87,   88,  157,  151,  153,  158,  142,   70,  156,
 /*   760 */   144,  145,  146,   53,   94,  148,  149,   83,   87,   88,
 /*   770 */   157,  151,  153,  158,  342,  342,  342,  342,  342,  342,
 /*   780 */   342,  342,  342,  142,   70,  156,  144,  145,  146,   53,
 /*   790 */   188,  148,  149,   83,   87,   88,  157,  151,  153,  158,
 /*   800 */   342,  342,  342,  342,  342,  342,  342,  204,  342,  142,
 /*   810 */    70,  156,  144,  145,  146,   53,  342,  148,  149,   83,
 /*   820 */    87,   88,  157,  151,  153,  158,  142,   70,  156,  144,
 /*   830 */   145,  146,   53,  127,  148,  149,   83,   87,   88,  157,
 /*   840 */   151,  153,  158,  342,  342,  342,  342,  342,  342,  342,
 /*   850 */   342,  342,  142,   70,  156,  144,  145,  146,   53,   14,
 /*   860 */   148,  149,   83,   87,   88,  157,  151,  153,  158,  342,
 /*   870 */   159,  342,  342,  342,  342,  342,  342,  342,  342,  159,
 /*   880 */   342,  342,  342,  342,  342,  342,  342,  342,   38,   18,
 /*   890 */   342,  160,  161,  162,  163,  164,  165,   38,   18,  342,
 /*   900 */   160,  161,  162,  163,  164,  165,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    45,   46,   47,   48,   49,   58,   59,   60,   61,   62,
 /*    10 */    63,   52,   52,   66,   22,   23,   57,   57,   46,   47,
 /*    20 */    48,   49,   12,   68,   69,   70,   71,   72,   73,   74,
 /*    30 */     1,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*    40 */    68,   69,   70,   71,   72,   73,   74,   69,   76,   77,
 /*    50 */    78,   79,   80,   81,   82,   83,   84,   79,   80,   81,
 /*    60 */    82,   83,   84,   25,   69,   46,   47,   48,   49,   74,
 /*    70 */    41,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*    80 */    62,   63,   46,   47,   48,   49,   51,   68,   69,   70,
 /*    90 */    71,   72,   73,   74,   86,   76,   77,   78,   79,   80,
 /*   100 */    81,   82,   83,   84,   68,   69,   70,   71,   72,   73,
 /*   110 */    74,   69,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   120 */    84,   51,   80,   81,   82,   83,   84,   62,   63,   46,
 /*   130 */    47,   48,   49,   58,   59,   60,   61,   62,   63,   62,
 /*   140 */    63,   66,   62,   63,   55,   56,   46,   47,   48,   49,
 /*   150 */    26,   68,   69,   70,   71,   72,   73,   74,   88,   76,
 /*   160 */    77,   78,   79,   80,   81,   82,   83,   84,   68,   69,
 /*   170 */    70,   71,   72,   73,   74,   69,   76,   77,   78,   79,
 /*   180 */    80,   81,   82,   83,   84,   64,   65,   81,   82,   83,
 /*   190 */    84,   75,   69,   46,   47,   48,   49,   60,   61,   62,
 /*   200 */    63,   39,   40,   66,   31,   82,   83,   84,    0,    1,
 /*   210 */    46,   47,   48,   49,   87,   68,   69,   70,   71,   72,
 /*   220 */    73,   74,   17,   76,   77,   78,   79,   80,   81,   82,
 /*   230 */    83,   84,   68,   69,   70,   71,   72,   73,   74,   12,
 /*   240 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   22,
 /*   250 */    23,   24,   12,   53,    1,   12,    3,   46,   47,   48,
 /*   260 */    49,    1,   22,   23,   24,   22,   23,   24,   61,   62,
 /*   270 */    63,   18,   54,   66,   46,   47,   48,   49,   12,   68,
 /*   280 */    69,   70,   71,   72,   73,   74,   56,   76,   77,   78,
 /*   290 */    79,   80,   81,   82,   83,   84,   68,   69,   70,   71,
 /*   300 */    72,   73,   74,   43,   76,   77,   78,   79,   80,   81,
 /*   310 */    82,   83,   84,   61,   62,   63,   16,   22,   66,   63,
 /*   320 */    20,   46,   47,   48,   49,   25,    1,   63,    1,   87,
 /*   330 */     5,   31,    5,    1,    1,   65,    4,    4,   46,   47,
 /*   340 */    48,   49,   63,   68,   69,   70,   71,   72,   73,   74,
 /*   350 */    63,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   360 */    68,   69,   70,   71,   72,   73,   74,   42,   76,   77,
 /*   370 */    78,   79,   80,   81,   82,   83,   84,    1,   16,   63,
 /*   380 */     4,   63,   20,   63,   63,   46,   47,   48,   49,    4,
 /*   390 */    27,    1,    3,   31,    5,    1,    1,    1,    4,    4,
 /*   400 */    28,   15,   46,   47,   48,   49,   29,   68,   69,   70,
 /*   410 */    71,   72,   73,   74,   32,   76,   77,   78,   79,   80,
 /*   420 */    81,   82,   83,   84,   68,   69,   70,   71,   72,   73,
 /*   430 */    74,   42,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   440 */    84,    1,   21,    1,    1,    4,   12,    1,   15,   46,
 /*   450 */    47,   48,   49,   15,   32,   16,   12,   12,   21,   12,
 /*   460 */     1,   32,    4,   15,   15,   12,   46,   47,   48,   49,
 /*   470 */    20,   68,   69,   70,   71,   72,   73,   74,   12,   76,
 /*   480 */    77,   78,   79,   80,   81,   82,   83,   84,   68,   69,
 /*   490 */    70,   71,   72,   73,   74,   15,   76,   77,   78,   79,
 /*   500 */    80,   81,   82,   83,   84,    2,   15,   15,   12,    6,
 /*   510 */     7,    8,    9,   10,   11,   12,   13,   14,   89,   15,
 /*   520 */    15,   15,   19,   47,   48,   49,   15,   15,   15,   15,
 /*   530 */    12,   50,   15,   30,   31,   15,   33,   34,   35,   36,
 /*   540 */    37,   38,   15,   49,   68,   69,   70,   71,   72,   73,
 /*   550 */    74,   12,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   560 */    84,   67,   68,   69,   70,   71,   72,   73,   74,   88,
 /*   570 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   580 */     1,   89,   89,   49,   69,   70,   71,   72,   73,   74,
 /*   590 */    89,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   600 */    89,   67,   68,   69,   70,   71,   72,   73,   74,   49,
 /*   610 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   620 */    89,   89,   89,   89,   89,   89,   49,   89,   68,   69,
 /*   630 */    70,   71,   72,   73,   74,   89,   76,   77,   78,   79,
 /*   640 */    80,   81,   82,   83,   84,   68,   69,   70,   71,   72,
 /*   650 */    73,   74,   49,   76,   77,   78,   79,   80,   81,   82,
 /*   660 */    83,   84,   89,   89,   89,   89,   89,   89,   89,   49,
 /*   670 */    89,   68,   69,   70,   71,   72,   73,   74,   89,   76,
 /*   680 */    77,   78,   79,   80,   81,   82,   83,   84,   68,   69,
 /*   690 */    70,   71,   72,   73,   74,   49,   76,   77,   78,   79,
 /*   700 */    80,   81,   82,   83,   84,   89,   89,   89,   89,   89,
 /*   710 */    89,   89,   89,   89,   68,   69,   70,   71,   72,   73,
 /*   720 */    74,   49,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   730 */    84,   89,   89,   89,   89,   89,   89,   89,   49,   89,
 /*   740 */    68,   69,   70,   71,   72,   73,   74,   89,   76,   77,
 /*   750 */    78,   79,   80,   81,   82,   83,   84,   68,   69,   70,
 /*   760 */    71,   72,   73,   74,   49,   76,   77,   78,   79,   80,
 /*   770 */    81,   82,   83,   84,   89,   89,   89,   89,   89,   89,
 /*   780 */    89,   89,   89,   68,   69,   70,   71,   72,   73,   74,
 /*   790 */    49,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   800 */    89,   89,   89,   89,   89,   89,   89,   49,   89,   68,
 /*   810 */    69,   70,   71,   72,   73,   74,   89,   76,   77,   78,
 /*   820 */    79,   80,   81,   82,   83,   84,   68,   69,   70,   71,
 /*   830 */    72,   73,   74,   49,   76,   77,   78,   79,   80,   81,
 /*   840 */    82,   83,   84,   89,   89,   89,   89,   89,   89,   89,
 /*   850 */    89,   89,   68,   69,   70,   71,   72,   73,   74,    1,
 /*   860 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   89,
 /*   870 */    12,   89,   89,   89,   89,   89,   89,   89,   89,   12,
 /*   880 */    89,   89,   89,   89,   89,   89,   89,   89,   30,   31,
 /*   890 */    89,   33,   34,   35,   36,   37,   38,   30,   31,   89,
 /*   900 */    33,   34,   35,   36,   37,   38,
};
#define YY_SHIFT_USE_DFLT (-9)
#define YY_SHIFT_MAX 134
static const short yy_shift_ofst[] = {
 /*     0 */   503,  503,  503,  503,  503,  503,  503,  503,  503,  503,
 /*    10 */   503,  503,  503,  503,  503,  503,  503,  867,  867,  858,
 /*    20 */   867,  867,  867,  867,  867,  867,  867,  867,  867,  867,
 /*    30 */   867,  867,  867,  227,  227,  867,  867,  240,  867,  243,
 /*    40 */   243,  325,  389,  253,  253,   -8,   -8,   -8,   -8,   10,
 /*    50 */    38,  327,  162,  124,  173,  205,  266,   10,  295,  295,
 /*    60 */    38,  295,  295,   38,  295,  295,  295,  295,   38,  173,
 /*    70 */   300,  362,  208,  332,  333,  376,  394,   29,  260,  395,
 /*    80 */   385,  390,  396,  363,  372,  377,  386,  372,  377,  382,
 /*    90 */   421,  440,  442,  441,  443,  390,  434,  446,  433,  444,
 /*   100 */   438,  439,  445,  439,  447,  450,  437,  448,  449,  480,
 /*   110 */   491,  453,  466,  492,  496,  504,  505,  506,  511,  512,
 /*   120 */   513,  514,  517,  520,  527,  422,  459,  429,  518,  458,
 /*   130 */   539,  579,  390,  390,  390,
};
#define YY_REDUCE_USE_DFLT (-54)
#define YY_REDUCE_MAX 69
static const short yy_reduce_ofst[] = {
 /*     0 */   -45,  -28,   19,   36,   83,  100,  147,  164,  211,  228,
 /*    10 */   275,  292,  339,  356,  403,  420,  476,  494,  534,  560,
 /*    20 */   577,  603,  620,  646,  672,  689,  715,  741,  758,  784,
 /*    30 */   515,   -5,  -22,  -53,   75,   42,  106,  137,  123,  207,
 /*    40 */   252,  481,   70,  -41,  -40,   18,   65,   77,   80,   89,
 /*    50 */   121,   35,    8,  116,  127,  200,  218,  230,  256,  264,
 /*    60 */   270,  279,  287,  270,  316,  318,  320,  321,  270,  242,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   217,  217,  217,  217,  217,  217,  217,  217,  217,  217,
 /*    10 */   217,  217,  217,  217,  217,  217,  217,  326,  326,  340,
 /*    20 */   340,  224,  340,  226,  228,  340,  340,  340,  340,  340,
 /*    30 */   340,  340,  340,  278,  278,  340,  340,  340,  340,  340,
 /*    40 */   340,  340,  338,  244,  244,  340,  340,  340,  340,  340,
 /*    50 */   282,  338,  328,  298,  331,  240,  340,  340,  340,  340,
 /*    60 */   340,  340,  340,  286,  340,  340,  340,  340,  285,  331,
 /*    70 */   313,  313,  340,  340,  340,  340,  340,  340,  340,  340,
 /*    80 */   340,  339,  340,  303,  305,  307,  327,  304,  306,  340,
 /*    90 */   340,  340,  340,  340,  340,  245,  340,  340,  232,  340,
 /*   100 */   233,  235,  340,  234,  340,  340,  340,  262,  254,  250,
 /*   110 */   248,  340,  340,  252,  340,  258,  256,  260,  270,  266,
 /*   120 */   264,  268,  274,  272,  276,  340,  340,  340,  340,  340,
 /*   130 */   340,  340,  335,  336,  337,  216,  218,  219,  215,  220,
 /*   140 */   223,  225,  292,  293,  295,  296,  297,  299,  301,  302,
 /*   150 */   309,  310,  311,  312,  290,  291,  294,  308,  314,  318,
 /*   160 */   319,  320,  321,  322,  323,  324,  325,  315,  329,  227,
 /*   170 */   229,  230,  242,  243,  231,  239,  238,  237,  236,  246,
 /*   180 */   247,  279,  249,  280,  251,  253,  281,  283,  284,  288,
 /*   190 */   289,  255,  257,  259,  261,  287,  263,  265,  267,  269,
 /*   200 */   271,  273,  275,  277,  241,  332,  330,  316,  317,  300,
 /*   210 */   221,  334,  222,  333,
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
  "arith_expr",    "term",          "factor",        "power",       
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
 /*  95 */ "arith_expr ::= arith_expr PLUS term",
 /*  96 */ "term ::= factor",
 /*  97 */ "factor ::= MINUS factor",
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
 /* 111 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 112 */ "args_opt ::=",
 /* 113 */ "args_opt ::= args",
 /* 114 */ "blockarg_opt ::=",
 /* 115 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 116 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 117 */ "blockarg_params_opt ::=",
 /* 118 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 119 */ "excepts ::= except",
 /* 120 */ "excepts ::= excepts except",
 /* 121 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 122 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 123 */ "except ::= EXCEPT NEWLINE stmts",
 /* 124 */ "finally_opt ::=",
 /* 125 */ "finally_opt ::= FINALLY stmts",
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
  { 81, 1 },
  { 82, 2 },
  { 82, 1 },
  { 83, 1 },
  { 69, 1 },
  { 69, 5 },
  { 69, 4 },
  { 69, 3 },
  { 84, 1 },
  { 84, 1 },
  { 84, 1 },
  { 84, 1 },
  { 84, 1 },
  { 84, 1 },
  { 84, 1 },
  { 84, 3 },
  { 85, 0 },
  { 85, 1 },
  { 86, 0 },
  { 86, 5 },
  { 86, 5 },
  { 87, 0 },
  { 87, 3 },
  { 50, 1 },
  { 50, 2 },
  { 88, 6 },
  { 88, 4 },
  { 88, 3 },
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
    *pval = yymsp[0].minor.yy119;
}
#line 1771 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 119: /* excepts ::= except */
#line 616 "parser.y"
{
    yygotominor.yy119 = make_array_with(env, yymsp[0].minor.yy119);
}
#line 1782 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 619 "parser.y"
{
    yygotominor.yy119 = Array_push(env, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 1792 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 112: /* args_opt ::= */
      case 114: /* blockarg_opt ::= */
      case 117: /* blockarg_params_opt ::= */
      case 124: /* finally_opt ::= */
#line 623 "parser.y"
{
    yygotominor.yy119 = YNIL;
}
#line 1806 "parser.c"
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
      case 98: /* factor ::= power */
      case 99: /* power ::= postfix_expr */
      case 100: /* postfix_expr ::= atom */
      case 113: /* args_opt ::= args */
      case 125: /* finally_opt ::= FINALLY stmts */
#line 626 "parser.y"
{
    yygotominor.yy119 = yymsp[0].minor.yy119;
}
#line 1836 "parser.c"
        break;
      case 5: /* stmt ::= expr */
#line 629 "parser.y"
{
    if (PTR_AS(YogNode, yymsp[0].minor.yy119)->type == NODE_VARIABLE) {
        unsigned int lineno = NODE_LINENO(yymsp[0].minor.yy119);
        ID id = PTR_AS(YogNode, yymsp[0].minor.yy119)->u.variable.id;
        yygotominor.yy119 = CommandCall_new(env, lineno, id, YNIL, YNIL);
    }
    else {
        yygotominor.yy119 = yymsp[0].minor.yy119;
    }
}
#line 1850 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 639 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy119 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy119, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, yymsp[-1].minor.yy119);
}
#line 1858 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 643 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy119 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy119, yymsp[-2].minor.yy119, YNIL, yymsp[-1].minor.yy119);
}
#line 1866 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 647 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy119 = Finally_new(env, lineno, yymsp[-3].minor.yy119, yymsp[-1].minor.yy119);
}
#line 1874 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 651 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy119 = While_new(env, lineno, yymsp[-3].minor.yy119, yymsp[-1].minor.yy119);
}
#line 1882 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 655 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy119 = Break_new(env, lineno, YNIL);
}
#line 1890 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 659 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy119 = Break_new(env, lineno, yymsp[0].minor.yy119);
}
#line 1898 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 663 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy119 = Next_new(env, lineno, YNIL);
}
#line 1906 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 667 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy119 = Next_new(env, lineno, yymsp[0].minor.yy119);
}
#line 1914 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 671 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy119 = Return_new(env, lineno, YNIL);
}
#line 1922 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 675 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy119 = Return_new(env, lineno, yymsp[0].minor.yy119);
}
#line 1930 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 679 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy119 = If_new(env, lineno, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, yymsp[-1].minor.yy119);
}
#line 1938 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 683 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy119 = Klass_new(env, lineno, id, yymsp[-3].minor.yy119, yymsp[-1].minor.yy119);
}
#line 1947 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 688 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy119 = Nonlocal_new(env, lineno, yymsp[0].minor.yy119);
}
#line 1955 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 692 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy119 = Import_new(env, lineno, yymsp[0].minor.yy119);
}
#line 1963 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 704 "parser.y"
{
    yygotominor.yy119 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 1971 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 707 "parser.y"
{
    yygotominor.yy119 = Array_push_token_id(env, yymsp[-2].minor.yy119, yymsp[0].minor.yy0);
}
#line 1979 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 728 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy119, yymsp[-1].minor.yy119, yymsp[0].minor.yy119);
    yygotominor.yy119 = make_array_with(env, node);
}
#line 1988 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 741 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy119 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy119, yymsp[-1].minor.yy119);
}
#line 1997 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 747 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-8].minor.yy119, yymsp[-6].minor.yy119, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 2004 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 750 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-6].minor.yy119, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, yymsp[0].minor.yy119, YNIL);
}
#line 2011 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 753 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-6].minor.yy119, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, YNIL, yymsp[0].minor.yy119);
}
#line 2018 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 756 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, yymsp[0].minor.yy119, YNIL, YNIL);
}
#line 2025 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 759 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-6].minor.yy119, yymsp[-4].minor.yy119, YNIL, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 2032 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 762 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, YNIL, yymsp[0].minor.yy119, YNIL);
}
#line 2039 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 765 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, YNIL, YNIL, yymsp[0].minor.yy119);
}
#line 2046 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 768 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-2].minor.yy119, yymsp[0].minor.yy119, YNIL, YNIL, YNIL);
}
#line 2053 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 771 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-6].minor.yy119, YNIL, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 2060 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 774 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-4].minor.yy119, YNIL, yymsp[-2].minor.yy119, yymsp[0].minor.yy119, YNIL);
}
#line 2067 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 777 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-4].minor.yy119, YNIL, yymsp[-2].minor.yy119, YNIL, yymsp[0].minor.yy119);
}
#line 2074 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 780 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-2].minor.yy119, YNIL, yymsp[0].minor.yy119, YNIL, YNIL);
}
#line 2081 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 783 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-4].minor.yy119, YNIL, YNIL, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 2088 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 786 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-2].minor.yy119, YNIL, YNIL, yymsp[0].minor.yy119, YNIL);
}
#line 2095 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 789 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[-2].minor.yy119, YNIL, YNIL, YNIL, yymsp[0].minor.yy119);
}
#line 2102 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 792 "parser.y"
{
    yygotominor.yy119 = Params_new(env, yymsp[0].minor.yy119, YNIL, YNIL, YNIL, YNIL);
}
#line 2109 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 795 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, yymsp[-6].minor.yy119, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 2116 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 798 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, yymsp[0].minor.yy119, YNIL);
}
#line 2123 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 801 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, YNIL, yymsp[0].minor.yy119);
}
#line 2130 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 804 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, yymsp[-2].minor.yy119, yymsp[0].minor.yy119, YNIL, YNIL);
}
#line 2137 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 807 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, yymsp[-4].minor.yy119, YNIL, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 2144 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 810 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, yymsp[-2].minor.yy119, YNIL, yymsp[0].minor.yy119, YNIL);
}
#line 2151 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 813 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, yymsp[-2].minor.yy119, YNIL, YNIL, yymsp[0].minor.yy119);
}
#line 2158 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 816 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, yymsp[0].minor.yy119, YNIL, YNIL, YNIL);
}
#line 2165 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 819 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 2172 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 822 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy119, yymsp[0].minor.yy119, YNIL);
}
#line 2179 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 825 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy119, YNIL, yymsp[0].minor.yy119);
}
#line 2186 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 828 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy119, YNIL, YNIL);
}
#line 2193 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 831 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 2200 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 834 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy119, YNIL);
}
#line 2207 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 837 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy119);
}
#line 2214 "parser.c"
        break;
      case 64: /* params ::= */
#line 840 "parser.y"
{
    yygotominor.yy119 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2221 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 844 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy119 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2230 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 850 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy119 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2239 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 856 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy119 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy119);
}
#line 2248 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 873 "parser.y"
{
    yygotominor.yy119 = YogArray_new(env);
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy119, lineno, id, YNIL);
}
#line 2258 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 879 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy119, lineno, id, YNIL);
    yygotominor.yy119 = yymsp[-2].minor.yy119;
}
#line 2268 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 893 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy119 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy119);
}
#line 2277 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 910 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy119);
    yygotominor.yy119 = Assign_new(env, lineno, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 2285 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 933 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy119);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy119)->u.id;
    yygotominor.yy119 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy119, id, yymsp[0].minor.yy119);
}
#line 2294 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
#line 939 "parser.y"
{
    yygotominor.yy119 = yymsp[0].minor.yy0;
}
#line 2301 "parser.c"
        break;
      case 91: /* shift_expr ::= shift_expr LSHIFT match_expr */
      case 93: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
      case 95: /* arith_expr ::= arith_expr PLUS term */
#line 958 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy119);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy119 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy119, id, yymsp[0].minor.yy119);
}
#line 2312 "parser.c"
        break;
      case 97: /* factor ::= MINUS factor */
#line 986 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy119 = FuncCall_new3(env, lineno, yymsp[0].minor.yy119, id);
}
#line 2321 "parser.c"
        break;
      case 101: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1002 "parser.y"
{
    yygotominor.yy119 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy119), yymsp[-4].minor.yy119, yymsp[-2].minor.yy119, yymsp[0].minor.yy119);
}
#line 2328 "parser.c"
        break;
      case 102: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1005 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-3].minor.yy119);
    yygotominor.yy119 = Subscript_new(env, lineno, yymsp[-3].minor.yy119, yymsp[-1].minor.yy119);
}
#line 2336 "parser.c"
        break;
      case 103: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1009 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy119);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy119 = Attr_new(env, lineno, yymsp[-2].minor.yy119, id);
}
#line 2345 "parser.c"
        break;
      case 104: /* atom ::= NAME */
#line 1015 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy119 = Variable_new(env, lineno, id);
}
#line 2354 "parser.c"
        break;
      case 105: /* atom ::= NUMBER */
      case 106: /* atom ::= REGEXP */
      case 107: /* atom ::= STRING */
#line 1020 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy119 = Literal_new(env, lineno, val);
}
#line 2365 "parser.c"
        break;
      case 108: /* atom ::= TRUE */
#line 1035 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy119 = Literal_new(env, lineno, YTRUE);
}
#line 2373 "parser.c"
        break;
      case 109: /* atom ::= FALSE */
#line 1039 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy119 = Literal_new(env, lineno, YFALSE);
}
#line 2381 "parser.c"
        break;
      case 110: /* atom ::= LINE */
#line 1043 "parser.y"
{
    unsigned int lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy119 = Literal_new(env, lineno, val);
}
#line 2390 "parser.c"
        break;
      case 111: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1048 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy119 = Array_new(env, lineno, yymsp[-1].minor.yy119);
}
#line 2398 "parser.c"
        break;
      case 115: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 116: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1063 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy119 = BlockArg_new(env, lineno, yymsp[-3].minor.yy119, yymsp[-1].minor.yy119);
}
#line 2407 "parser.c"
        break;
      case 118: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1075 "parser.y"
{
    yygotominor.yy119 = yymsp[-1].minor.yy119;
}
#line 2414 "parser.c"
        break;
      case 120: /* excepts ::= excepts except */
#line 1082 "parser.y"
{
    yygotominor.yy119 = Array_push(env, yymsp[-1].minor.yy119, yymsp[0].minor.yy119);
}
#line 2421 "parser.c"
        break;
      case 121: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1086 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy119 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy119, id, yymsp[0].minor.yy119);
}
#line 2431 "parser.c"
        break;
      case 122: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1092 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy119 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy119, NO_EXC_VAR, yymsp[0].minor.yy119);
}
#line 2439 "parser.c"
        break;
      case 123: /* except ::= EXCEPT NEWLINE stmts */
#line 1096 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy119 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy119);
}
#line 2447 "parser.c"
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
