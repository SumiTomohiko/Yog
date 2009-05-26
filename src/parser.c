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

    if (IS_PTR(elem)) {
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

    if (IS_PTR(elem)) {
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

YogVal 
YogParser_parse_file(YogEnv* env, const char* filename, BOOL debug)
{
    SAVE_LOCALS(env);

    YogVal lexer = YUNDEF;
    YogVal ast = YUNDEF;
    YogVal lemon_parser = YUNDEF;
    YogVal token = YUNDEF;
    PUSH_LOCALS4(env, lexer, ast, lemon_parser, token);

    FILE* fp;
    if (filename != NULL) {
        fp = fopen(filename, "r");
        YOG_ASSERT(env, fp != NULL, "Can't open %s", filename);
    }
    else {
        fp = stdin;
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

#define TOKEN_LINENO(token)     PTR_AS(YogToken, (token))->lineno
#define NODE_LINENO(node)       PTR_AS(YogNode, (node))->lineno
#line 561 "parser.c"
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
#define YYNOCODE 86
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy77;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 197
#define YYNRULE 120
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
 /*     0 */   318,   69,  149,  123,  124,   68,  127,  128,  129,  130,
 /*    10 */    51,   41,  132,  133,   78,   81,   83,  139,  135,  136,
 /*    20 */   140,  126,   67,  138,  128,  129,  130,   51,   27,  132,
 /*    30 */   133,   78,   81,   83,  139,  135,  136,  140,   70,  149,
 /*    40 */   123,  124,   68,   97,  101,  168,  191,  131,  172,  132,
 /*    50 */   133,   78,   81,   83,  139,  135,  136,  140,  126,   67,
 /*    60 */   138,  128,  129,  130,   51,  148,  132,  133,   78,   81,
 /*    70 */    83,  139,  135,  136,  140,   40,  149,  123,  124,   94,
 /*    80 */    95,  106,  110,  112,  186,   85,   16,  178,  107,  109,
 /*    90 */   182,   14,   31,  172,  196,  126,   67,  138,  128,  129,
 /*   100 */   130,   51,   33,  132,  133,   78,   81,   83,  139,  135,
 /*   110 */   136,  140,   50,  149,  123,  124,  119,   95,  106,  110,
 /*   120 */   112,  186,   99,  100,  178,   19,    3,  141,    4,  113,
 /*   130 */    98,  165,  126,   67,  138,  128,  129,  130,   51,    1,
 /*   140 */   132,  133,   78,   81,   83,  139,  135,  136,  140,   86,
 /*   150 */   149,  123,  124,   68,   52,   66,  142,  143,  144,  145,
 /*   160 */   146,  147,   19,  189,   79,  139,  135,  136,  140,  126,
 /*   170 */    67,  138,  128,  129,  130,   51,   28,  132,  133,   78,
 /*   180 */    81,   83,  139,  135,  136,  140,   71,  149,  123,  124,
 /*   190 */    96,  103,  105,  177,   87,  158,  178,  157,  157,   16,
 /*   200 */   104,  175,  197,   16,  108,  180,  126,   67,  138,  128,
 /*   210 */   129,  130,   51,   65,  132,  133,   78,   81,   83,  139,
 /*   220 */   135,  136,  140,   42,  149,  123,  124,   68,  111,  184,
 /*   230 */     9,   99,  100,  102,  169,  170,  161,  193,   82,  139,
 /*   240 */   135,  136,  140,  126,   67,  138,  128,  129,  130,   51,
 /*   250 */    60,  132,  133,   78,   81,   83,  139,  135,  136,  140,
 /*   260 */    43,  149,  123,  124,   17,   90,   16,   17,   99,  100,
 /*   270 */   102,    4,   30,  163,   29,   99,  121,   29,  167,  121,
 /*   280 */   126,   67,  138,  128,  129,  130,   51,   57,  132,  133,
 /*   290 */    78,   81,   83,  139,  135,  136,  140,   88,  149,  123,
 /*   300 */   124,   16,   12,  173,    8,   99,  100,  102,   16,   16,
 /*   310 */    16,   16,  150,  152,  159,  162,   26,  126,   67,  138,
 /*   320 */   128,  129,  130,   51,  174,  132,  133,   78,   81,   83,
 /*   330 */   139,  135,  136,  140,   72,  149,  123,  124,  176,   16,
 /*   340 */   179,  181,  114,  190,  183,   15,  185,   20,   34,   36,
 /*   350 */    35,   49,  151,   16,  126,   67,  138,  128,  129,  130,
 /*   360 */    51,  156,  132,  133,   78,   81,   83,  139,  135,  136,
 /*   370 */   140,   73,  149,  123,  124,   53,   91,  160,   93,   32,
 /*   380 */    37,   38,   10,   44,   55,  164,  166,   48,   56,  115,
 /*   390 */    11,  126,   67,  138,  128,  129,  130,   51,  188,  132,
 /*   400 */   133,   78,   81,   83,  139,  135,  136,  140,  116,  149,
 /*   410 */   123,  124,   45,  195,   58,  192,   59,   39,   46,   61,
 /*   420 */    62,   47,   63,   64,  194,  319,  319,  319,  126,   67,
 /*   430 */   138,  128,  129,  130,   51,  319,  132,  133,   78,   81,
 /*   440 */    83,  139,  135,  136,  140,  117,  149,  123,  124,  319,
 /*   450 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   460 */   319,  319,  319,  319,  319,  126,   67,  138,  128,  129,
 /*   470 */   130,   51,  319,  132,  133,   78,   81,   83,  139,  135,
 /*   480 */   136,  140,  118,  149,  123,  124,  319,  319,  319,  319,
 /*   490 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   500 */   319,  319,  126,   67,  138,  128,  129,  130,   51,  319,
 /*   510 */   132,  133,   78,   81,   83,  139,  135,  136,  140,   75,
 /*   520 */   149,  123,  124,  319,  319,  319,  319,  319,  319,  319,
 /*   530 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  126,
 /*   540 */    67,  138,  128,  129,  130,   51,  319,  132,  133,   78,
 /*   550 */    81,   83,  139,  135,  136,  140,   76,  149,  123,  124,
 /*   560 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   570 */   319,  319,  319,  319,  319,  319,  126,   67,  138,  128,
 /*   580 */   129,  130,   51,  319,  132,  133,   78,   81,   83,  139,
 /*   590 */   135,  136,  140,  122,  123,  124,  319,  319,  319,  319,
 /*   600 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   610 */   319,  319,  126,   67,  138,  128,  129,  130,   51,   68,
 /*   620 */   132,  133,   78,   81,   83,  139,  135,  136,  140,  137,
 /*   630 */    80,  134,  135,  136,  140,  319,  319,  319,  319,  319,
 /*   640 */   319,  319,  319,  319,  319,  319,  126,   67,  138,  128,
 /*   650 */   129,  130,   51,  319,  132,  133,   78,   81,   83,  139,
 /*   660 */   135,  136,  140,   84,  137,   77,  319,  319,  319,  319,
 /*   670 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   680 */   319,  126,   67,  138,  128,  129,  130,   51,  319,  132,
 /*   690 */   133,   78,   81,   83,  139,  135,  136,  140,   74,  319,
 /*   700 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   710 */   319,  319,  319,  319,  319,  126,   67,  138,  128,  129,
 /*   720 */   130,   51,  319,  132,  133,   78,   81,   83,  139,  135,
 /*   730 */   136,  140,  125,  319,  319,  319,  319,  319,  319,  319,
 /*   740 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  126,
 /*   750 */    67,  138,  128,  129,  130,   51,  319,  132,  133,   78,
 /*   760 */    81,   83,  139,  135,  136,  140,    5,  319,  319,  319,
 /*   770 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   780 */   319,  319,  319,  126,   67,  138,  128,  129,  130,   51,
 /*   790 */   319,  132,  133,   78,   81,   83,  139,  135,  136,  140,
 /*   800 */   153,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   810 */   319,  319,  319,  319,  319,  319,  319,  126,   67,  138,
 /*   820 */   128,  129,  130,   51,  319,  132,  133,   78,   81,   83,
 /*   830 */   139,  135,  136,  140,  154,  319,  319,  319,  319,  319,
 /*   840 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   850 */   319,  126,   67,  138,  128,  129,  130,   51,  319,  132,
 /*   860 */   133,   78,   81,   83,  139,  135,  136,  140,  155,  319,
 /*   870 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   880 */   319,  319,  319,  319,  319,  126,   67,  138,  128,  129,
 /*   890 */   130,   51,  319,  132,  133,   78,   81,   83,  139,  135,
 /*   900 */   136,  140,    6,  319,  319,  319,  319,  319,  319,  319,
 /*   910 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  126,
 /*   920 */    67,  138,  128,  129,  130,   51,  319,  132,  133,   78,
 /*   930 */    81,   83,  139,  135,  136,  140,    7,  319,  319,  319,
 /*   940 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   950 */   319,  319,  319,  126,   67,  138,  128,  129,  130,   51,
 /*   960 */   319,  132,  133,   78,   81,   83,  139,  135,  136,  140,
 /*   970 */   171,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*   980 */   319,  319,  319,  319,  319,  319,  319,  126,   67,  138,
 /*   990 */   128,  129,  130,   51,  319,  132,  133,   78,   81,   83,
 /*  1000 */   139,  135,  136,  140,  187,  319,  319,  319,  319,  319,
 /*  1010 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*  1020 */   319,  126,   67,  138,  128,  129,  130,   51,  319,  132,
 /*  1030 */   133,   78,   81,   83,  139,  135,  136,  140,  120,  319,
 /*  1040 */   319,  319,  319,  319,  319,  319,  319,  319,  319,  319,
 /*  1050 */   319,  319,  319,  319,  319,  126,   67,  138,  128,  129,
 /*  1060 */   130,   51,  141,  132,  133,   78,   81,   83,  139,  135,
 /*  1070 */   136,  140,   18,    2,  319,  319,  319,   21,   22,   23,
 /*  1080 */    24,   25,   89,   54,   13,  141,  319,   92,  319,  319,
 /*  1090 */   319,  142,  143,  144,  145,  146,  147,  319,  319,  319,
 /*  1100 */   319,  142,  143,  144,  145,  146,  147,  319,  319,  319,
 /*  1110 */   319,  319,  319,  319,  142,  143,  144,  145,  146,  147,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    43,   44,   45,   46,   47,   65,   66,   67,   68,   69,
 /*    10 */    70,   49,   72,   73,   74,   75,   76,   77,   78,   79,
 /*    20 */    80,   64,   65,   66,   67,   68,   69,   70,   23,   72,
 /*    30 */    73,   74,   75,   76,   77,   78,   79,   80,   44,   45,
 /*    40 */    46,   47,   65,   58,   59,   60,   84,   70,   63,   72,
 /*    50 */    73,   74,   75,   76,   77,   78,   79,   80,   64,   65,
 /*    60 */    66,   67,   68,   69,   70,   82,   72,   73,   74,   75,
 /*    70 */    76,   77,   78,   79,   80,   44,   45,   46,   47,   55,
 /*    80 */    56,   57,   58,   59,   60,   50,    1,   63,   58,   59,
 /*    90 */    60,    6,   71,   63,   24,   64,   65,   66,   67,   68,
 /*   100 */    69,   70,   28,   72,   73,   74,   75,   76,   77,   78,
 /*   110 */    79,   80,   44,   45,   46,   47,   55,   56,   57,   58,
 /*   120 */    59,   60,   20,   21,   63,   40,    4,    2,    6,   50,
 /*   130 */    59,   60,   64,   65,   66,   67,   68,   69,   70,   83,
 /*   140 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   44,
 /*   150 */    45,   46,   47,   65,   37,   38,   31,   32,   33,   34,
 /*   160 */    35,   36,   40,   84,   76,   77,   78,   79,   80,   64,
 /*   170 */    65,   66,   67,   68,   69,   70,   15,   72,   73,   74,
 /*   180 */    75,   76,   77,   78,   79,   80,   44,   45,   46,   47,
 /*   190 */    57,   58,   59,   60,   51,   51,   63,   54,   54,    1,
 /*   200 */    59,   60,    0,    1,   59,   60,   64,   65,   66,   67,
 /*   210 */    68,   69,   70,    2,   72,   73,   74,   75,   76,   77,
 /*   220 */    78,   79,   80,   44,   45,   46,   47,   65,   59,   60,
 /*   230 */    52,   20,   21,   22,   61,   62,    2,   39,   76,   77,
 /*   240 */    78,   79,   80,   64,   65,   66,   67,   68,   69,   70,
 /*   250 */     2,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   260 */    44,   45,   46,   47,   18,   53,    1,   18,   20,   21,
 /*   270 */    22,    6,   23,   60,   28,   20,   30,   28,   60,   30,
 /*   280 */    64,   65,   66,   67,   68,   69,   70,    2,   72,   73,
 /*   290 */    74,   75,   76,   77,   78,   79,   80,   44,   45,   46,
 /*   300 */    47,    1,    1,   62,    4,   20,   21,   22,    1,    1,
 /*   310 */     1,    1,    5,    5,    5,    5,   16,   64,   65,   66,
 /*   320 */    67,   68,   69,   70,   60,   72,   73,   74,   75,   76,
 /*   330 */    77,   78,   79,   80,   44,   45,   46,   47,   60,    1,
 /*   340 */    60,   60,   41,    5,   60,   83,   60,   14,   25,   27,
 /*   350 */    26,   19,    5,    1,   64,   65,   66,   67,   68,   69,
 /*   360 */    70,    5,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   370 */    80,   44,   45,   46,   47,    2,   14,    2,    2,   18,
 /*   380 */    14,   14,   19,   14,   14,    2,    2,    2,   14,    2,
 /*   390 */     1,   64,   65,   66,   67,   68,   69,   70,    5,   72,
 /*   400 */    73,   74,   75,   76,   77,   78,   79,   80,   44,   45,
 /*   410 */    46,   47,   14,    2,   14,   29,   14,   14,   14,   14,
 /*   420 */    14,   14,   14,   14,   29,   85,   85,   85,   64,   65,
 /*   430 */    66,   67,   68,   69,   70,   85,   72,   73,   74,   75,
 /*   440 */    76,   77,   78,   79,   80,   44,   45,   46,   47,   85,
 /*   450 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   460 */    85,   85,   85,   85,   85,   64,   65,   66,   67,   68,
 /*   470 */    69,   70,   85,   72,   73,   74,   75,   76,   77,   78,
 /*   480 */    79,   80,   44,   45,   46,   47,   85,   85,   85,   85,
 /*   490 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   500 */    85,   85,   64,   65,   66,   67,   68,   69,   70,   85,
 /*   510 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   44,
 /*   520 */    45,   46,   47,   85,   85,   85,   85,   85,   85,   85,
 /*   530 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   64,
 /*   540 */    65,   66,   67,   68,   69,   70,   85,   72,   73,   74,
 /*   550 */    75,   76,   77,   78,   79,   80,   44,   45,   46,   47,
 /*   560 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   570 */    85,   85,   85,   85,   85,   85,   64,   65,   66,   67,
 /*   580 */    68,   69,   70,   85,   72,   73,   74,   75,   76,   77,
 /*   590 */    78,   79,   80,   45,   46,   47,   85,   85,   85,   85,
 /*   600 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   610 */    85,   85,   64,   65,   66,   67,   68,   69,   70,   65,
 /*   620 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   47,
 /*   630 */    48,   77,   78,   79,   80,   85,   85,   85,   85,   85,
 /*   640 */    85,   85,   85,   85,   85,   85,   64,   65,   66,   67,
 /*   650 */    68,   69,   70,   85,   72,   73,   74,   75,   76,   77,
 /*   660 */    78,   79,   80,   81,   47,   48,   85,   85,   85,   85,
 /*   670 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   680 */    85,   64,   65,   66,   67,   68,   69,   70,   85,   72,
 /*   690 */    73,   74,   75,   76,   77,   78,   79,   80,   47,   85,
 /*   700 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   710 */    85,   85,   85,   85,   85,   64,   65,   66,   67,   68,
 /*   720 */    69,   70,   85,   72,   73,   74,   75,   76,   77,   78,
 /*   730 */    79,   80,   47,   85,   85,   85,   85,   85,   85,   85,
 /*   740 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   64,
 /*   750 */    65,   66,   67,   68,   69,   70,   85,   72,   73,   74,
 /*   760 */    75,   76,   77,   78,   79,   80,   47,   85,   85,   85,
 /*   770 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   780 */    85,   85,   85,   64,   65,   66,   67,   68,   69,   70,
 /*   790 */    85,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   800 */    47,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   810 */    85,   85,   85,   85,   85,   85,   85,   64,   65,   66,
 /*   820 */    67,   68,   69,   70,   85,   72,   73,   74,   75,   76,
 /*   830 */    77,   78,   79,   80,   47,   85,   85,   85,   85,   85,
 /*   840 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   850 */    85,   64,   65,   66,   67,   68,   69,   70,   85,   72,
 /*   860 */    73,   74,   75,   76,   77,   78,   79,   80,   47,   85,
 /*   870 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   880 */    85,   85,   85,   85,   85,   64,   65,   66,   67,   68,
 /*   890 */    69,   70,   85,   72,   73,   74,   75,   76,   77,   78,
 /*   900 */    79,   80,   47,   85,   85,   85,   85,   85,   85,   85,
 /*   910 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   64,
 /*   920 */    65,   66,   67,   68,   69,   70,   85,   72,   73,   74,
 /*   930 */    75,   76,   77,   78,   79,   80,   47,   85,   85,   85,
 /*   940 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   950 */    85,   85,   85,   64,   65,   66,   67,   68,   69,   70,
 /*   960 */    85,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   970 */    47,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*   980 */    85,   85,   85,   85,   85,   85,   85,   64,   65,   66,
 /*   990 */    67,   68,   69,   70,   85,   72,   73,   74,   75,   76,
 /*  1000 */    77,   78,   79,   80,   47,   85,   85,   85,   85,   85,
 /*  1010 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*  1020 */    85,   64,   65,   66,   67,   68,   69,   70,   85,   72,
 /*  1030 */    73,   74,   75,   76,   77,   78,   79,   80,   47,   85,
 /*  1040 */    85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*  1050 */    85,   85,   85,   85,   85,   64,   65,   66,   67,   68,
 /*  1060 */    69,   70,    2,   72,   73,   74,   75,   76,   77,   78,
 /*  1070 */    79,   80,    2,    3,   85,   85,   85,    7,    8,    9,
 /*  1080 */    10,   11,   12,   13,    1,    2,   85,   17,   85,   85,
 /*  1090 */    85,   31,   32,   33,   34,   35,   36,   85,   85,   85,
 /*  1100 */    85,   31,   32,   33,   34,   35,   36,   85,   85,   85,
 /*  1110 */    85,   85,   85,   85,   31,   32,   33,   34,   35,   36,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 121
static const short yy_shift_ofst[] = {
 /*     0 */  1070, 1070, 1070, 1070, 1070, 1070, 1070, 1070, 1070, 1070,
 /*    10 */  1070, 1070, 1070, 1070, 1070, 1070, 1070, 1060, 1060, 1083,
 /*    20 */  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
 /*    30 */  1060, 1060,  211,  211,  125,  125,  125,  248,  285,  285,
 /*    40 */    85,  122,  300,  300,  102,  102,  102,  102,    5,  117,
 /*    50 */   265,   70,   74,  161,  234,  255,  255,    5,  255,  255,
 /*    60 */     5,  255,  255,  255,  255,    5,   74,  249,  246,  202,
 /*    70 */   307,  308,  309,  310,  301,  338,  198,  333,  323,  322,
 /*    80 */   333,  324,  322,  322,  332,  347,  352,  356,  352,  373,
 /*    90 */   362,  375,  376,  361,  363,  366,  367,  369,  370,  383,
 /*   100 */   384,  374,  385,  398,  400,  402,  403,  404,  405,  406,
 /*   110 */   407,  408,  409,  393,  387,  389,  352,  352,  352,  386,
 /*   120 */   395,  411,
};
#define YY_REDUCE_USE_DFLT (-61)
#define YY_REDUCE_MAX 66
static const short yy_reduce_ofst[] = {
 /*     0 */   -43,   -6,   31,   68,  105,  142,  179,  216,  253,  290,
 /*    10 */   327,  364,  401,  438,  475,  512,  548,  582,  617,  651,
 /*    20 */   685,  719,  753,  787,  821,  855,  889,  923,  957,  991,
 /*    30 */   -60,  -23,   24,   61,   88,  162,  554,  133,  -15,   30,
 /*    40 */   -38,   79,  143,  144,   71,  141,  145,  169,  173,  -17,
 /*    50 */    35,   21,   56,  178,  212,  213,  218,  241,  264,  278,
 /*    60 */   241,  280,  281,  284,  286,  241,  262,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   200,  200,  200,  200,  200,  200,  200,  200,  200,  200,
 /*    10 */   200,  200,  200,  200,  200,  200,  200,  303,  296,  317,
 /*    20 */   317,  317,  208,  210,  212,  317,  317,  317,  317,  317,
 /*    30 */   317,  317,  257,  257,  317,  317,  317,  317,  317,  317,
 /*    40 */   317,  315,  223,  223,  317,  317,  317,  317,  261,  305,
 /*    50 */   315,  277,  308,  219,  317,  317,  317,  317,  317,  317,
 /*    60 */   265,  317,  317,  317,  317,  264,  308,  291,  291,  317,
 /*    70 */   317,  317,  317,  317,  317,  317,  317,  203,  282,  284,
 /*    80 */   304,  283,  286,  285,  317,  317,  316,  317,  224,  317,
 /*    90 */   216,  317,  317,  317,  317,  241,  233,  229,  227,  317,
 /*   100 */   317,  231,  317,  237,  235,  239,  249,  245,  243,  247,
 /*   110 */   253,  251,  255,  317,  317,  317,  312,  313,  314,  317,
 /*   120 */   317,  317,  199,  201,  202,  270,  271,  272,  274,  275,
 /*   130 */   276,  278,  280,  281,  288,  289,  290,  269,  273,  287,
 /*   140 */   292,  296,  297,  298,  299,  300,  301,  302,  293,  198,
 /*   150 */   306,  204,  207,  209,  211,  213,  214,  221,  222,  215,
 /*   160 */   218,  217,  225,  226,  258,  228,  259,  230,  232,  260,
 /*   170 */   262,  263,  267,  268,  234,  236,  238,  240,  266,  242,
 /*   180 */   244,  246,  248,  250,  252,  254,  256,  220,  205,  311,
 /*   190 */   206,  310,  309,  307,  294,  295,  279,
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
  "CLASS",         "NONLOCAL",      "COMMA",         "GREATER",     
  "ELIF",          "DEF",           "LPAR",          "RPAR",        
  "DOUBLE_STAR",   "STAR",          "AMPER",         "EQUAL",       
  "LESS",          "LSHIFT",        "EQUAL_TILDA",   "PLUS",        
  "LBRACKET",      "RBRACKET",      "DOT",           "NUMBER",      
  "REGEXP",        "STRING",        "TRUE",          "FALSE",       
  "LINE",          "DO",            "LBRACE",        "RBRACE",      
  "EXCEPT",        "AS",            "error",         "module",      
  "stmts",         "stmt",          "func_def",      "expr",        
  "args",          "excepts",       "finally_opt",   "if_tail",     
  "super_opt",     "names",         "else_opt",      "params",      
  "params_without_default",  "params_with_default",  "block_param",   "var_param",   
  "kw_param",      "param_default_opt",  "param_default",  "param_with_default",
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
 /*  20 */ "names ::= NAME",
 /*  21 */ "names ::= names COMMA NAME",
 /*  22 */ "super_opt ::=",
 /*  23 */ "super_opt ::= GREATER expr",
 /*  24 */ "if_tail ::= else_opt",
 /*  25 */ "if_tail ::= ELIF expr stmts if_tail",
 /*  26 */ "else_opt ::=",
 /*  27 */ "else_opt ::= ELSE stmts",
 /*  28 */ "func_def ::= DEF NAME LPAR params RPAR stmts END",
 /*  29 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  30 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param",
 /*  31 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param",
 /*  32 */ "params ::= params_without_default COMMA params_with_default COMMA block_param",
 /*  33 */ "params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param",
 /*  34 */ "params ::= params_without_default COMMA params_with_default COMMA var_param",
 /*  35 */ "params ::= params_without_default COMMA params_with_default COMMA kw_param",
 /*  36 */ "params ::= params_without_default COMMA params_with_default",
 /*  37 */ "params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  38 */ "params ::= params_without_default COMMA block_param COMMA var_param",
 /*  39 */ "params ::= params_without_default COMMA block_param COMMA kw_param",
 /*  40 */ "params ::= params_without_default COMMA block_param",
 /*  41 */ "params ::= params_without_default COMMA var_param COMMA kw_param",
 /*  42 */ "params ::= params_without_default COMMA var_param",
 /*  43 */ "params ::= params_without_default COMMA kw_param",
 /*  44 */ "params ::= params_without_default",
 /*  45 */ "params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  46 */ "params ::= params_with_default COMMA block_param COMMA var_param",
 /*  47 */ "params ::= params_with_default COMMA block_param COMMA kw_param",
 /*  48 */ "params ::= params_with_default COMMA block_param",
 /*  49 */ "params ::= params_with_default COMMA var_param COMMA kw_param",
 /*  50 */ "params ::= params_with_default COMMA var_param",
 /*  51 */ "params ::= params_with_default COMMA kw_param",
 /*  52 */ "params ::= params_with_default",
 /*  53 */ "params ::= block_param COMMA var_param COMMA kw_param",
 /*  54 */ "params ::= block_param COMMA var_param",
 /*  55 */ "params ::= block_param COMMA kw_param",
 /*  56 */ "params ::= block_param",
 /*  57 */ "params ::= var_param COMMA kw_param",
 /*  58 */ "params ::= var_param",
 /*  59 */ "params ::= kw_param",
 /*  60 */ "params ::=",
 /*  61 */ "kw_param ::= DOUBLE_STAR NAME",
 /*  62 */ "var_param ::= STAR NAME",
 /*  63 */ "block_param ::= AMPER NAME param_default_opt",
 /*  64 */ "param_default_opt ::=",
 /*  65 */ "param_default_opt ::= param_default",
 /*  66 */ "param_default ::= EQUAL expr",
 /*  67 */ "params_without_default ::= NAME",
 /*  68 */ "params_without_default ::= params_without_default COMMA NAME",
 /*  69 */ "params_with_default ::= param_with_default",
 /*  70 */ "params_with_default ::= params_with_default COMMA param_with_default",
 /*  71 */ "param_with_default ::= NAME param_default",
 /*  72 */ "args ::= expr",
 /*  73 */ "args ::= args COMMA expr",
 /*  74 */ "expr ::= assign_expr",
 /*  75 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /*  76 */ "assign_expr ::= logical_or_expr",
 /*  77 */ "logical_or_expr ::= logical_and_expr",
 /*  78 */ "logical_and_expr ::= not_expr",
 /*  79 */ "not_expr ::= comparison",
 /*  80 */ "comparison ::= xor_expr",
 /*  81 */ "comparison ::= xor_expr comp_op xor_expr",
 /*  82 */ "comp_op ::= LESS",
 /*  83 */ "xor_expr ::= or_expr",
 /*  84 */ "or_expr ::= and_expr",
 /*  85 */ "and_expr ::= shift_expr",
 /*  86 */ "shift_expr ::= match_expr",
 /*  87 */ "shift_expr ::= shift_expr LSHIFT arith_expr",
 /*  88 */ "match_expr ::= arith_expr",
 /*  89 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /*  90 */ "arith_expr ::= term",
 /*  91 */ "arith_expr ::= arith_expr PLUS term",
 /*  92 */ "term ::= factor",
 /*  93 */ "factor ::= power",
 /*  94 */ "power ::= postfix_expr",
 /*  95 */ "postfix_expr ::= atom",
 /*  96 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /*  97 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /*  98 */ "postfix_expr ::= postfix_expr DOT NAME",
 /*  99 */ "atom ::= NAME",
 /* 100 */ "atom ::= NUMBER",
 /* 101 */ "atom ::= REGEXP",
 /* 102 */ "atom ::= STRING",
 /* 103 */ "atom ::= TRUE",
 /* 104 */ "atom ::= FALSE",
 /* 105 */ "atom ::= LINE",
 /* 106 */ "args_opt ::=",
 /* 107 */ "args_opt ::= args",
 /* 108 */ "blockarg_opt ::=",
 /* 109 */ "blockarg_opt ::= DO blockarg_params_opt stmts END",
 /* 110 */ "blockarg_opt ::= LBRACE blockarg_params_opt stmts RBRACE",
 /* 111 */ "blockarg_params_opt ::=",
 /* 112 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 113 */ "excepts ::= except",
 /* 114 */ "excepts ::= excepts except",
 /* 115 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 116 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 117 */ "except ::= EXCEPT NEWLINE stmts",
 /* 118 */ "finally_opt ::=",
 /* 119 */ "finally_opt ::= FINALLY stmts",
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
  { 43, 1 },
  { 44, 1 },
  { 44, 3 },
  { 45, 0 },
  { 45, 1 },
  { 45, 1 },
  { 45, 2 },
  { 45, 7 },
  { 45, 5 },
  { 45, 5 },
  { 45, 4 },
  { 45, 1 },
  { 45, 2 },
  { 45, 1 },
  { 45, 2 },
  { 45, 1 },
  { 45, 2 },
  { 45, 5 },
  { 45, 5 },
  { 45, 2 },
  { 53, 1 },
  { 53, 3 },
  { 52, 0 },
  { 52, 2 },
  { 51, 1 },
  { 51, 4 },
  { 54, 0 },
  { 54, 2 },
  { 46, 7 },
  { 55, 9 },
  { 55, 7 },
  { 55, 7 },
  { 55, 5 },
  { 55, 7 },
  { 55, 5 },
  { 55, 5 },
  { 55, 3 },
  { 55, 7 },
  { 55, 5 },
  { 55, 5 },
  { 55, 3 },
  { 55, 5 },
  { 55, 3 },
  { 55, 3 },
  { 55, 1 },
  { 55, 7 },
  { 55, 5 },
  { 55, 5 },
  { 55, 3 },
  { 55, 5 },
  { 55, 3 },
  { 55, 3 },
  { 55, 1 },
  { 55, 5 },
  { 55, 3 },
  { 55, 3 },
  { 55, 1 },
  { 55, 3 },
  { 55, 1 },
  { 55, 1 },
  { 55, 0 },
  { 60, 2 },
  { 59, 2 },
  { 58, 3 },
  { 61, 0 },
  { 61, 1 },
  { 62, 2 },
  { 56, 1 },
  { 56, 3 },
  { 57, 1 },
  { 57, 3 },
  { 63, 2 },
  { 48, 1 },
  { 48, 3 },
  { 47, 1 },
  { 64, 3 },
  { 64, 1 },
  { 66, 1 },
  { 67, 1 },
  { 68, 1 },
  { 69, 1 },
  { 69, 3 },
  { 71, 1 },
  { 70, 1 },
  { 72, 1 },
  { 73, 1 },
  { 74, 1 },
  { 74, 3 },
  { 75, 1 },
  { 75, 3 },
  { 76, 1 },
  { 76, 3 },
  { 77, 1 },
  { 78, 1 },
  { 79, 1 },
  { 65, 1 },
  { 65, 5 },
  { 65, 4 },
  { 65, 3 },
  { 80, 1 },
  { 80, 1 },
  { 80, 1 },
  { 80, 1 },
  { 80, 1 },
  { 80, 1 },
  { 80, 1 },
  { 81, 0 },
  { 81, 1 },
  { 82, 0 },
  { 82, 4 },
  { 82, 4 },
  { 83, 0 },
  { 83, 3 },
  { 49, 1 },
  { 49, 2 },
  { 84, 6 },
  { 84, 4 },
  { 84, 3 },
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
#line 557 "parser.y"
{
    *pval = yymsp[0].minor.yy77;
}
#line 1742 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 69: /* params_with_default ::= param_with_default */
      case 72: /* args ::= expr */
      case 113: /* excepts ::= except */
#line 561 "parser.y"
{
    yygotominor.yy77 = Array_new(env, yymsp[0].minor.yy77);
}
#line 1752 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 70: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 73: /* args ::= args COMMA expr */
#line 564 "parser.y"
{
    yygotominor.yy77 = Array_push(env, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 1761 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 22: /* super_opt ::= */
      case 26: /* else_opt ::= */
      case 64: /* param_default_opt ::= */
      case 106: /* args_opt ::= */
      case 108: /* blockarg_opt ::= */
      case 111: /* blockarg_params_opt ::= */
      case 118: /* finally_opt ::= */
#line 568 "parser.y"
{
    yygotominor.yy77 = YNIL;
}
#line 1775 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 23: /* super_opt ::= GREATER expr */
      case 24: /* if_tail ::= else_opt */
      case 27: /* else_opt ::= ELSE stmts */
      case 65: /* param_default_opt ::= param_default */
      case 66: /* param_default ::= EQUAL expr */
      case 74: /* expr ::= assign_expr */
      case 76: /* assign_expr ::= logical_or_expr */
      case 77: /* logical_or_expr ::= logical_and_expr */
      case 78: /* logical_and_expr ::= not_expr */
      case 79: /* not_expr ::= comparison */
      case 80: /* comparison ::= xor_expr */
      case 83: /* xor_expr ::= or_expr */
      case 84: /* or_expr ::= and_expr */
      case 85: /* and_expr ::= shift_expr */
      case 86: /* shift_expr ::= match_expr */
      case 88: /* match_expr ::= arith_expr */
      case 90: /* arith_expr ::= term */
      case 92: /* term ::= factor */
      case 93: /* factor ::= power */
      case 94: /* power ::= postfix_expr */
      case 95: /* postfix_expr ::= atom */
      case 107: /* args_opt ::= args */
      case 119: /* finally_opt ::= FINALLY stmts */
#line 571 "parser.y"
{
    yygotominor.yy77 = yymsp[0].minor.yy77;
}
#line 1805 "parser.c"
        break;
      case 5: /* stmt ::= expr */
#line 574 "parser.y"
{
    if (PTR_AS(YogNode, yymsp[0].minor.yy77)->type == NODE_VARIABLE) {
        unsigned int lineno = NODE_LINENO(yymsp[0].minor.yy77);
        ID id = PTR_AS(YogNode, yymsp[0].minor.yy77)->u.variable.id;
        yygotominor.yy77 = CommandCall_new(env, lineno, id, YNIL, YNIL);
    }
    else {
        yygotominor.yy77 = yymsp[0].minor.yy77;
    }
}
#line 1819 "parser.c"
        break;
      case 6: /* stmt ::= NAME args */
#line 584 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy77 = CommandCall_new(env, lineno, id, yymsp[0].minor.yy77, YNIL);
}
#line 1828 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 596 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy77 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy77, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[-1].minor.yy77);
}
#line 1836 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts excepts finally_opt END */
#line 600 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy77 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy77, yymsp[-2].minor.yy77, YNIL, yymsp[-1].minor.yy77);
}
#line 1844 "parser.c"
        break;
      case 9: /* stmt ::= TRY stmts FINALLY stmts END */
#line 604 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy77 = Finally_new(env, lineno, yymsp[-3].minor.yy77, yymsp[-1].minor.yy77);
}
#line 1852 "parser.c"
        break;
      case 10: /* stmt ::= WHILE expr stmts END */
#line 608 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy77 = While_new(env, lineno, yymsp[-2].minor.yy77, yymsp[-1].minor.yy77);
}
#line 1860 "parser.c"
        break;
      case 11: /* stmt ::= BREAK */
#line 612 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Break_new(env, lineno, YNIL);
}
#line 1868 "parser.c"
        break;
      case 12: /* stmt ::= BREAK expr */
#line 616 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy77 = Break_new(env, lineno, yymsp[0].minor.yy77);
}
#line 1876 "parser.c"
        break;
      case 13: /* stmt ::= NEXT */
#line 620 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Next_new(env, lineno, YNIL);
}
#line 1884 "parser.c"
        break;
      case 14: /* stmt ::= NEXT expr */
#line 624 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy77 = Next_new(env, lineno, yymsp[0].minor.yy77);
}
#line 1892 "parser.c"
        break;
      case 15: /* stmt ::= RETURN */
#line 628 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Return_new(env, lineno, YNIL);
}
#line 1900 "parser.c"
        break;
      case 16: /* stmt ::= RETURN expr */
#line 632 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy77 = Return_new(env, lineno, yymsp[0].minor.yy77);
}
#line 1908 "parser.c"
        break;
      case 17: /* stmt ::= IF expr stmts if_tail END */
#line 636 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy77 = If_new(env, lineno, yymsp[-3].minor.yy77, yymsp[-2].minor.yy77, yymsp[-1].minor.yy77);
}
#line 1916 "parser.c"
        break;
      case 18: /* stmt ::= CLASS NAME super_opt stmts END */
#line 640 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-3].minor.yy0)->u.id;
    yygotominor.yy77 = Klass_new(env, lineno, id, yymsp[-2].minor.yy77, yymsp[-1].minor.yy77);
}
#line 1925 "parser.c"
        break;
      case 19: /* stmt ::= NONLOCAL names */
#line 645 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy77 = Nonlocal_new(env, lineno, yymsp[0].minor.yy77);
}
#line 1933 "parser.c"
        break;
      case 20: /* names ::= NAME */
#line 650 "parser.y"
{
    yygotominor.yy77 = YogArray_new(env);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    YogArray_push(env, yygotominor.yy77, ID2VAL(id));
}
#line 1942 "parser.c"
        break;
      case 21: /* names ::= names COMMA NAME */
#line 655 "parser.y"
{
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    YogArray_push(env, yymsp[-2].minor.yy77, ID2VAL(id));
    yygotominor.yy77 = yymsp[-2].minor.yy77;
}
#line 1951 "parser.c"
        break;
      case 25: /* if_tail ::= ELIF expr stmts if_tail */
#line 671 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-2].minor.yy77, yymsp[-1].minor.yy77, yymsp[0].minor.yy77);
    yygotominor.yy77 = Array_new(env, node);
}
#line 1960 "parser.c"
        break;
      case 28: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 684 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy77 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy77, yymsp[-1].minor.yy77);
}
#line 1969 "parser.c"
        break;
      case 29: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 690 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-8].minor.yy77, yymsp[-6].minor.yy77, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 1976 "parser.c"
        break;
      case 30: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 693 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-6].minor.yy77, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL);
}
#line 1983 "parser.c"
        break;
      case 31: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 696 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-6].minor.yy77, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77);
}
#line 1990 "parser.c"
        break;
      case 32: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 699 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL, YNIL);
}
#line 1997 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 702 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-6].minor.yy77, yymsp[-4].minor.yy77, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2004 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 705 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77, YNIL);
}
#line 2011 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 708 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, YNIL, YNIL, yymsp[0].minor.yy77);
}
#line 2018 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default */
#line 711 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL, YNIL, YNIL);
}
#line 2025 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 714 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-6].minor.yy77, YNIL, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2032 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 717 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL);
}
#line 2039 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 720 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, YNIL, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77);
}
#line 2046 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA block_param */
#line 723 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77, YNIL, YNIL);
}
#line 2053 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 726 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, YNIL, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2060 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA var_param */
#line 729 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-2].minor.yy77, YNIL, YNIL, yymsp[0].minor.yy77, YNIL);
}
#line 2067 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA kw_param */
#line 732 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-2].minor.yy77, YNIL, YNIL, YNIL, yymsp[0].minor.yy77);
}
#line 2074 "parser.c"
        break;
      case 44: /* params ::= params_without_default */
#line 735 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[0].minor.yy77, YNIL, YNIL, YNIL, YNIL);
}
#line 2081 "parser.c"
        break;
      case 45: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 738 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-6].minor.yy77, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2088 "parser.c"
        break;
      case 46: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 741 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL);
}
#line 2095 "parser.c"
        break;
      case 47: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 744 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77);
}
#line 2102 "parser.c"
        break;
      case 48: /* params ::= params_with_default COMMA block_param */
#line 747 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL, YNIL);
}
#line 2109 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 750 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-4].minor.yy77, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2116 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA var_param */
#line 753 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77, YNIL);
}
#line 2123 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA kw_param */
#line 756 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-2].minor.yy77, YNIL, YNIL, yymsp[0].minor.yy77);
}
#line 2130 "parser.c"
        break;
      case 52: /* params ::= params_with_default */
#line 759 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[0].minor.yy77, YNIL, YNIL, YNIL);
}
#line 2137 "parser.c"
        break;
      case 53: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 762 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2144 "parser.c"
        break;
      case 54: /* params ::= block_param COMMA var_param */
#line 765 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL);
}
#line 2151 "parser.c"
        break;
      case 55: /* params ::= block_param COMMA kw_param */
#line 768 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77);
}
#line 2158 "parser.c"
        break;
      case 56: /* params ::= block_param */
#line 771 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy77, YNIL, YNIL);
}
#line 2165 "parser.c"
        break;
      case 57: /* params ::= var_param COMMA kw_param */
#line 774 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2172 "parser.c"
        break;
      case 58: /* params ::= var_param */
#line 777 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy77, YNIL);
}
#line 2179 "parser.c"
        break;
      case 59: /* params ::= kw_param */
#line 780 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy77);
}
#line 2186 "parser.c"
        break;
      case 60: /* params ::= */
#line 783 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2193 "parser.c"
        break;
      case 61: /* kw_param ::= DOUBLE_STAR NAME */
#line 787 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy77 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2202 "parser.c"
        break;
      case 62: /* var_param ::= STAR NAME */
#line 793 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy77 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2211 "parser.c"
        break;
      case 63: /* block_param ::= AMPER NAME param_default_opt */
#line 799 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy77 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy77);
}
#line 2220 "parser.c"
        break;
      case 67: /* params_without_default ::= NAME */
#line 816 "parser.y"
{
    yygotominor.yy77 = YogArray_new(env);
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy77, lineno, id, YNIL);
}
#line 2230 "parser.c"
        break;
      case 68: /* params_without_default ::= params_without_default COMMA NAME */
#line 822 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy77, lineno, id, YNIL);
    yygotominor.yy77 = yymsp[-2].minor.yy77;
}
#line 2240 "parser.c"
        break;
      case 71: /* param_with_default ::= NAME param_default */
#line 836 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy77 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy77);
}
#line 2249 "parser.c"
        break;
      case 75: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 853 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy77);
    yygotominor.yy77 = Assign_new(env, lineno, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2257 "parser.c"
        break;
      case 81: /* comparison ::= xor_expr comp_op xor_expr */
#line 876 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy77);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy77)->u.id;
    yygotominor.yy77 = MethodCall1_new(env, lineno, yymsp[-2].minor.yy77, id, yymsp[0].minor.yy77);
}
#line 2266 "parser.c"
        break;
      case 82: /* comp_op ::= LESS */
#line 882 "parser.y"
{
    yygotominor.yy77 = yymsp[0].minor.yy0;
}
#line 2273 "parser.c"
        break;
      case 87: /* shift_expr ::= shift_expr LSHIFT arith_expr */
      case 89: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
      case 91: /* arith_expr ::= arith_expr PLUS term */
#line 901 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy77);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy77 = MethodCall1_new(env, lineno, yymsp[-2].minor.yy77, id, yymsp[0].minor.yy77);
}
#line 2284 "parser.c"
        break;
      case 96: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 940 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-4].minor.yy77);
    if (NODE(yymsp[-4].minor.yy77)->type == NODE_ATTR) {
        YogVal recv = NODE(yymsp[-4].minor.yy77)->u.attr.obj;
        ID name = NODE(yymsp[-4].minor.yy77)->u.attr.name;
        yygotominor.yy77 = MethodCall_new(env, lineno, recv, name, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
    }
    else {
        yygotominor.yy77 = FuncCall_new(env, lineno, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
    }
}
#line 2299 "parser.c"
        break;
      case 97: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 951 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-3].minor.yy77);
    yygotominor.yy77 = Subscript_new(env, lineno, yymsp[-3].minor.yy77, yymsp[-1].minor.yy77);
}
#line 2307 "parser.c"
        break;
      case 98: /* postfix_expr ::= postfix_expr DOT NAME */
#line 955 "parser.y"
{
    unsigned int lineno = NODE_LINENO(yymsp[-2].minor.yy77);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy77 = Attr_new(env, lineno, yymsp[-2].minor.yy77, id);
}
#line 2316 "parser.c"
        break;
      case 99: /* atom ::= NAME */
#line 961 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy77 = Variable_new(env, lineno, id);
}
#line 2325 "parser.c"
        break;
      case 100: /* atom ::= NUMBER */
      case 101: /* atom ::= REGEXP */
      case 102: /* atom ::= STRING */
#line 966 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy77 = Literal_new(env, lineno, val);
}
#line 2336 "parser.c"
        break;
      case 103: /* atom ::= TRUE */
#line 981 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Literal_new(env, lineno, YTRUE);
}
#line 2344 "parser.c"
        break;
      case 104: /* atom ::= FALSE */
#line 985 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Literal_new(env, lineno, YFALSE);
}
#line 2352 "parser.c"
        break;
      case 105: /* atom ::= LINE */
#line 989 "parser.y"
{
    unsigned int lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy77 = Literal_new(env, lineno, val);
}
#line 2361 "parser.c"
        break;
      case 109: /* blockarg_opt ::= DO blockarg_params_opt stmts END */
      case 110: /* blockarg_opt ::= LBRACE blockarg_params_opt stmts RBRACE */
#line 1005 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy77 = BlockArg_new(env, lineno, yymsp[-2].minor.yy77, yymsp[-1].minor.yy77);
}
#line 2370 "parser.c"
        break;
      case 112: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1017 "parser.y"
{
    yygotominor.yy77 = yymsp[-1].minor.yy77;
}
#line 2377 "parser.c"
        break;
      case 114: /* excepts ::= excepts except */
#line 1024 "parser.y"
{
    yygotominor.yy77 = Array_push(env, yymsp[-1].minor.yy77, yymsp[0].minor.yy77);
}
#line 2384 "parser.c"
        break;
      case 115: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1028 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy77 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy77, id, yymsp[0].minor.yy77);
}
#line 2394 "parser.c"
        break;
      case 116: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1034 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy77 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy77, NO_EXC_VAR, yymsp[0].minor.yy77);
}
#line 2402 "parser.c"
        break;
      case 117: /* except ::= EXCEPT NEWLINE stmts */
#line 1038 "parser.y"
{
    unsigned int lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy77 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy77);
}
#line 2410 "parser.c"
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
