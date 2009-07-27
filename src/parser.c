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
#include "yog/string.h"
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
YogNode_new(YogEnv* env, YogNodeType type, uint_t lineno) 
{
    YogVal node = ALLOC_OBJ(env, YogNode_keep_children, NULL, YogNode);
    PTR_AS(YogNode, node)->lineno = lineno;
    PTR_AS(YogNode, node)->type = type;

    return node;
}

#define NODE_NEW(type, lineno)  YogNode_new(env, (type), (lineno))
#define NODE(v)                 PTR_AS(YogNode, (v))

static YogVal 
Literal_new(YogEnv* env, uint_t lineno, YogVal val) 
{
    SAVE_ARG(env, val);

    YogVal node = YogNode_new(env, NODE_LITERAL, lineno);
    MODIFY(env, NODE(node)->u.literal.val, val);

    RETURN(env, node);
}

static YogVal 
BlockArg_new(YogEnv* env, uint_t lineno, YogVal params, YogVal stmts) 
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
Array_new(YogEnv* env, uint_t lineno, YogVal elems)
{
    SAVE_ARG(env, elems);

    YogVal node = YogNode_new(env, NODE_ARRAY, lineno);
    NODE(node)->u.array.elems = elems;

    RETURN(env, node);
}

static YogVal 
Param_new(YogEnv* env, YogNodeType type, uint_t lineno, ID id, YogVal default_) 
{
    SAVE_ARG(env, default_);

    YogVal node = YogNode_new(env, type, lineno);
    NODE(node)->u.param.name = id;
    MODIFY(env, NODE(node)->u.param.default_, default_);

    RETURN(env, node);
}

static void 
ParamArray_push(YogEnv* env, YogVal array, uint_t lineno, ID id, YogVal default_) 
{
    SAVE_ARGS2(env, array, default_);

    YogVal node = Param_new(env, NODE_PARAM, lineno, id, default_);
    YogArray_push(env, array, node);

    RETURN_VOID(env);
}

static YogVal 
FuncDef_new(YogEnv* env, uint_t lineno, ID name, YogVal params, YogVal stmts) 
{
    SAVE_ARGS2(env, params, stmts);

    YogVal node = YogNode_new(env, NODE_FUNC_DEF, lineno);
    NODE(node)->u.funcdef.name = name;
    MODIFY(env, NODE(node)->u.funcdef.params, params);
    MODIFY(env, NODE(node)->u.funcdef.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
FuncCall_new(YogEnv* env, uint_t lineno, YogVal callee, YogVal args, YogVal blockarg) 
{
    SAVE_ARGS3(env, callee, args, blockarg);

    YogVal node = NODE_NEW(NODE_FUNC_CALL, lineno);
    MODIFY(env, NODE(node)->u.func_call.callee, callee);
    MODIFY(env, NODE(node)->u.func_call.args, args);
    MODIFY(env, NODE(node)->u.func_call.blockarg, blockarg);

    RETURN(env, node);
}

static YogVal 
Variable_new(YogEnv* env, uint_t lineno, ID id) 
{
    YogVal node = NODE_NEW(NODE_VARIABLE, lineno);
    NODE(node)->u.variable.id = id;

    return node;
}

static YogVal 
ExceptBody_new(YogEnv* env, uint_t lineno, YogVal type, ID var, YogVal stmts) 
{
    SAVE_ARGS2(env, type, stmts);

    YogVal node = NODE_NEW(NODE_EXCEPT_BODY, lineno);
    MODIFY(env, NODE(node)->u.except_body.type, type);
    NODE(node)->u.except_body.var = var;
    MODIFY(env, NODE(node)->u.except_body.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Except_new(YogEnv* env, uint_t lineno, YogVal head, YogVal excepts, YogVal else_) 
{
    SAVE_ARGS3(env, head, excepts, else_);

    YogVal node = NODE_NEW(NODE_EXCEPT, lineno);
    MODIFY(env, NODE(node)->u.except.head, head);
    MODIFY(env, NODE(node)->u.except.excepts, excepts);
    MODIFY(env, NODE(node)->u.except.else_, else_);

    RETURN(env, node);
}

static YogVal 
Finally_new(YogEnv* env, uint_t lineno, YogVal head, YogVal body) 
{
    SAVE_ARGS2(env, head, body);

    YogVal node = NODE_NEW(NODE_FINALLY, lineno);
    MODIFY(env, NODE(node)->u.finally.head, head);
    MODIFY(env, NODE(node)->u.finally.body, body);

    RETURN(env, node);
}

static YogVal 
ExceptFinally_new(YogEnv* env, uint_t lineno, YogVal stmts, YogVal excepts, YogVal else_, YogVal finally) 
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
Break_new(YogEnv* env, uint_t lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = YogNode_new(env, NODE_BREAK, lineno);
    MODIFY(env, NODE(node)->u.break_.expr, expr);

    RETURN(env, node);
}

static YogVal 
Next_new(YogEnv* env, uint_t lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = NODE_NEW(NODE_NEXT, lineno);
    MODIFY(env, NODE(node)->u.next.expr, expr);

    RETURN(env, node);
}

static YogVal 
Return_new(YogEnv* env, uint_t lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = NODE_NEW(NODE_RETURN, lineno);
    MODIFY(env, NODE(node)->u.return_.expr, expr);

    RETURN(env, node);
}

static YogVal 
Attr_new(YogEnv* env, uint_t lineno, YogVal obj, ID name) 
{
    SAVE_ARG(env, obj);

    YogVal node = YogNode_new(env, NODE_ATTR, lineno);
    MODIFY(env, NODE(node)->u.attr.obj, obj);
    NODE(node)->u.attr.name = name;

    RETURN(env, node);
}

static YogVal 
FuncCall_new2(YogEnv* env, uint_t lineno, YogVal recv, ID name, YogVal arg) 
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
FuncCall_new3(YogEnv* env, uint_t lineno, YogVal recv, ID name)
{
    SAVE_ARG(env, recv);
    YogVal postfix = YUNDEF;
    PUSH_LOCAL(env, postfix);

    postfix = Attr_new(env, lineno, recv, name);

    YogVal node = FuncCall_new(env, lineno, postfix, YNIL, YNIL);

    RETURN(env, node);
}

static YogVal 
If_new(YogEnv* env, uint_t lineno, YogVal test, YogVal stmts, YogVal tail)
{
    SAVE_ARGS3(env, test, stmts, tail);

    YogVal node = YogNode_new(env, NODE_IF, lineno);
    MODIFY(env, NODE(node)->u.if_.test, test);
    MODIFY(env, NODE(node)->u.if_.stmts, stmts);
    MODIFY(env, NODE(node)->u.if_.tail, tail);

    RETURN(env, node);
}

static YogVal 
While_new(YogEnv* env, uint_t lineno, YogVal test, YogVal stmts) 
{
    SAVE_ARGS2(env, test, stmts);

    YogVal node = YogNode_new(env, NODE_WHILE, lineno);
    MODIFY(env, NODE(node)->u.while_.test, test);
    MODIFY(env, NODE(node)->u.while_.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Klass_new(YogEnv* env, uint_t lineno, ID name, YogVal super, YogVal stmts)
{
    SAVE_ARGS2(env, super, stmts);

    YogVal node = YogNode_new(env, NODE_KLASS, lineno);
    NODE(node)->u.klass.name = name;
    MODIFY(env, NODE(node)->u.klass.super, super);
    MODIFY(env, NODE(node)->u.klass.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Assign_new(YogEnv* env, uint_t lineno, YogVal left, YogVal right) 
{
    SAVE_ARGS2(env, left, right);

    YogVal node = NODE_NEW(NODE_ASSIGN, lineno);
    MODIFY(env, NODE(node)->u.assign.left, left);
    MODIFY(env, NODE(node)->u.assign.right, right);

    RETURN(env, node);
}

static YogVal 
Subscript_new(YogEnv* env, uint_t lineno, YogVal prefix, YogVal index) 
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
Nonlocal_new(YogEnv* env, uint_t lineno, YogVal names) 
{
    SAVE_ARG(env, names);

    YogVal node = YogNode_new(env, NODE_NONLOCAL, lineno);
    MODIFY(env, NODE(node)->u.nonlocal.names, names);

    RETURN(env, node);
}

static YogVal
Import_new(YogEnv* env, uint_t lineno, YogVal names)
{
    SAVE_ARG(env, names);

    YogVal node = YogNode_new(env, NODE_IMPORT, lineno);
    MODIFY(env, NODE(node)->u.import.names, names);

    RETURN(env, node);
}

static YogVal
parse(YogEnv* env, YogVal lexer, BOOL debug)
{
    SAVE_ARG(env, lexer);
    YogVal ast = YUNDEF;
    YogVal lemon_parser = YUNDEF;
    YogVal token = YUNDEF;
    PUSH_LOCALS3(env, ast, lemon_parser, token);

    lemon_parser = LemonParser_new(env);
    if (debug) {
        ParseTrace(stdout, "parser> ");
    }
    while (YogLexer_next_token(env, lexer, &token)) {
        uint_t type = PTR_AS(YogToken, token)->type;
        Parse(env, lemon_parser, type, token, &ast);
    }
    Parse(env, lemon_parser, 0, YNIL, &ast);

    RETURN(env, ast);
}

YogVal
YogParser_parse(YogEnv* env, YogVal src)
{
    SAVE_ARG(env, src);
    YogVal lexer = YUNDEF;
    YogVal ast = YUNDEF;
    PUSH_LOCALS2(env, lexer, ast);

    lexer = YogLexer_new(env);
    PTR_AS(YogLexer, lexer)->line = src;
    PTR_AS(YogLexer, lexer)->lineno++;
    YogLexer_set_encoding(env, lexer, PTR_AS(YogString, src)->encoding);

    ast = parse(env, lexer, FALSE);

    RETURN(env, ast);
}

YogVal 
YogParser_parse_file(YogEnv* env, FILE* fp, BOOL debug)
{
    YOG_ASSERT(env, fp != NULL, "file pointer is NULL");

    SAVE_LOCALS(env);
    YogVal lexer = YUNDEF;
    YogVal ast = YUNDEF;
    PUSH_LOCALS2(env, lexer, ast);

    lexer = YogLexer_new(env);
    PTR_AS(YogLexer, lexer)->fp = fp;
    YogLexer_read_encoding(env, lexer);

    ast = parse(env, lexer, debug);

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
#line 609 "parser.c"
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
#define YYNOCODE 96
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
#define YYNSTATE 228
#define YYNRULE 136
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
 /*     0 */     1,   45,  192,   75,   20,   21,   24,   25,   26,  102,
 /*    10 */   170,   63,   52,  117,  118,  120,   39,  110,   23,  365,
 /*    20 */    79,  144,  142,  143,  115,  119,  199,   40,   41,  203,
 /*    30 */    28,   18,  135,  171,  172,  173,  174,  175,  176,  177,
 /*    40 */   178,  227,  148,   77,  162,  150,  151,  152,   60,   87,
 /*    50 */   154,  155,   90,   93,   58,   55,  117,  118,  168,  159,
 /*    60 */   169,   44,  144,  142,  143,   78,  149,  150,  151,  152,
 /*    70 */    60,  225,  154,  155,   90,   93,   58,   55,   16,   14,
 /*    80 */   168,  159,  169,  148,   77,  162,  150,  151,  152,   60,
 /*    90 */   170,  154,  155,   90,   93,   58,   55,  223,   23,  168,
 /*   100 */   159,  169,   56,  144,  142,  143,   16,   40,   41,   16,
 /*   110 */    15,   18,  146,  171,  172,  173,  174,  175,  176,  177,
 /*   120 */   178,   16,  220,    8,  148,   77,  162,  150,  151,  152,
 /*   130 */    60,  170,  154,  155,   90,   93,   58,   55,   27,   23,
 /*   140 */   168,  159,  169,   88,  144,  142,  143,   78,   40,   41,
 /*   150 */    16,   19,   18,  182,  171,  172,  173,  174,  175,  176,
 /*   160 */   177,  178,  156,  159,  169,  148,   77,  162,  150,  151,
 /*   170 */   152,   60,   37,  154,  155,   90,   93,   58,   55,   70,
 /*   180 */   181,  168,  159,  169,   80,  144,  142,  143,   78,  117,
 /*   190 */   118,  120,   32,  153,   35,  154,  155,   90,   93,   58,
 /*   200 */    55,  116,  196,  168,  159,  169,  148,   77,  162,  150,
 /*   210 */   151,  152,   60,   29,  154,  155,   90,   93,   58,   55,
 /*   220 */    78,   97,  168,  159,  169,   81,  144,  142,  143,  103,
 /*   230 */    91,   58,   55,  106,  109,  168,  159,  169,  112,  113,
 /*   240 */   124,  128,  130,  217,  122,  206,  209,  148,   77,  162,
 /*   250 */   150,  151,  152,   60,  190,  154,  155,   90,   93,   58,
 /*   260 */    55,  163,  164,  168,  159,  169,   46,  144,  142,  143,
 /*   270 */   131,  113,  124,  128,  130,  217,  126,  211,  209,  114,
 /*   280 */   121,  123,  208,  129,  215,  209,  200,  201,  148,   77,
 /*   290 */   162,  150,  151,  152,   60,  104,  154,  155,   90,   93,
 /*   300 */    58,   55,   78,  107,  168,  159,  169,   47,  144,  142,
 /*   310 */   143,   78,  165,   57,   55,   61,   76,  168,  159,  169,
 /*   320 */   166,  167,    2,   54,    3,  117,  168,  159,  169,  148,
 /*   330 */    77,  162,  150,  151,  152,   60,  194,  154,  155,   90,
 /*   340 */    93,   58,   55,   78,   67,  168,  159,  169,  101,  144,
 /*   350 */   142,  143,   78,   13,  117,  118,  120,   99,  157,  159,
 /*   360 */   169,  187,  186,  228,   16,   19,  186,  158,  159,  169,
 /*   370 */   148,   77,  162,  150,  151,  152,   60,  198,  154,  155,
 /*   380 */    90,   93,   58,   55,  204,  205,  168,  159,  169,   82,
 /*   390 */   144,  142,  143,  125,  127,  213,   16,  207,  203,  136,
 /*   400 */     3,  210,   16,  212,   16,  188,   16,  193,  214,  226,
 /*   410 */   216,  148,   77,  162,  150,  151,  152,   60,  132,  154,
 /*   420 */   155,   90,   93,   58,   55,   16,  145,  168,  159,  169,
 /*   430 */    83,  144,  142,  143,  134,    4,   33,  180,   17,   36,
 /*   440 */    22,  179,    5,   31,   59,    6,  185,    7,   62,    9,
 /*   450 */   105,   30,  148,   77,  162,  150,  151,  152,   60,  189,
 /*   460 */   154,  155,   90,   93,   58,   55,   64,  191,  168,  159,
 /*   470 */   169,   84,  144,  142,  143,  134,  108,  111,   34,   17,
 /*   480 */    11,   38,   10,   42,   48,   65,  195,  197,   66,   53,
 /*   490 */   219,   49,   30,  148,   77,  162,  150,  151,  152,   60,
 /*   500 */    68,  154,  155,   90,   93,   58,   55,   69,   43,  168,
 /*   510 */   159,  169,  138,  144,  142,  143,   50,   71,  221,   72,
 /*   520 */    51,   73,   74,  222,  224,  137,   12,  366,  366,  366,
 /*   530 */   366,  366,  366,  366,  148,   77,  162,  150,  151,  152,
 /*   540 */    60,  366,  154,  155,   90,   93,   58,   55,  366,  366,
 /*   550 */   168,  159,  169,  139,  144,  142,  143,  366,  366,  366,
 /*   560 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*   570 */   366,  366,  366,  366,  366,  148,   77,  162,  150,  151,
 /*   580 */   152,   60,  366,  154,  155,   90,   93,   58,   55,  366,
 /*   590 */   366,  168,  159,  169,  140,  144,  142,  143,  366,  366,
 /*   600 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*   610 */   366,  366,  366,  366,  366,  366,  148,   77,  162,  150,
 /*   620 */   151,  152,   60,  366,  154,  155,   90,   93,   58,   55,
 /*   630 */   366,  366,  168,  159,  169,   86,  144,  142,  143,  366,
 /*   640 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*   650 */   366,  366,  366,  366,  366,  366,  366,  148,   77,  162,
 /*   660 */   150,  151,  152,   60,  366,  154,  155,   90,   93,   58,
 /*   670 */    55,  366,  366,  168,  159,  169,  141,  142,  143,  366,
 /*   680 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*   690 */   366,  366,  366,  366,  366,  366,  366,  148,   77,  162,
 /*   700 */   150,  151,  152,   60,  366,  154,  155,   90,   93,   58,
 /*   710 */    55,  160,  366,  168,  159,  169,  366,  366,  366,  366,
 /*   720 */   366,  366,  366,  366,  366,  366,  366,  366,  366,   92,
 /*   730 */   148,   77,  162,  150,  151,  152,   60,  366,  154,  155,
 /*   740 */    90,   93,   58,   55,  366,  366,  168,  159,  169,   96,
 /*   750 */   366,  160,  366,  366,  366,  366,  366,  366,  366,  366,
 /*   760 */   366,  366,  366,  366,  366,  366,  366,  366,  366,   92,
 /*   770 */   148,   77,  162,  150,  151,  152,   60,  366,  154,  155,
 /*   780 */    90,   93,   58,   55,   85,  366,  168,  159,  169,   94,
 /*   790 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*   800 */   366,  366,  366,  148,   77,  162,  150,  151,  152,   60,
 /*   810 */   366,  154,  155,   90,   93,   58,   55,  366,  366,  168,
 /*   820 */   159,  169,  366,   89,  366,  366,  366,  366,  366,  366,
 /*   830 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*   840 */   366,  366,  148,   77,  162,  150,  151,  152,   60,  366,
 /*   850 */   154,  155,   90,   93,   58,   55,  366,  147,  168,  159,
 /*   860 */   169,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*   870 */   366,  366,  366,  366,  366,  366,  148,   77,  162,  150,
 /*   880 */   151,  152,   60,  366,  154,  155,   90,   93,   58,   55,
 /*   890 */   366,  366,  168,  159,  169,  366,  161,  366,  366,  366,
 /*   900 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*   910 */   366,  366,  366,  366,  366,  148,   77,  162,  150,  151,
 /*   920 */   152,   60,  366,  154,  155,   90,   93,   58,   55,  366,
 /*   930 */    95,  168,  159,  169,  366,  366,  366,  366,  366,  366,
 /*   940 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  148,
 /*   950 */    77,  162,  150,  151,  152,   60,  366,  154,  155,   90,
 /*   960 */    93,   58,   55,  366,  366,  168,  159,  169,  366,  183,
 /*   970 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*   980 */   366,  366,  366,  366,  366,  366,  366,  366,  148,   77,
 /*   990 */   162,  150,  151,  152,   60,  366,  154,  155,   90,   93,
 /*  1000 */    58,   55,  366,  184,  168,  159,  169,  366,  366,  366,
 /*  1010 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*  1020 */   366,  366,  148,   77,  162,  150,  151,  152,   60,  366,
 /*  1030 */   154,  155,   90,   93,   58,   55,  366,  366,  168,  159,
 /*  1040 */   169,  366,   98,  366,  366,  366,  366,  366,  366,  366,
 /*  1050 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*  1060 */   366,  148,   77,  162,  150,  151,  152,   60,  366,  154,
 /*  1070 */   155,   90,   93,   58,   55,  366,  100,  168,  159,  169,
 /*  1080 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*  1090 */   366,  366,  366,  366,  366,  148,   77,  162,  150,  151,
 /*  1100 */   152,   60,  366,  154,  155,   90,   93,   58,   55,  366,
 /*  1110 */   366,  168,  159,  169,  366,  202,  366,  366,  366,  366,
 /*  1120 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*  1130 */   366,  366,  366,  366,  148,   77,  162,  150,  151,  152,
 /*  1140 */    60,  366,  154,  155,   90,   93,   58,   55,  366,  218,
 /*  1150 */   168,  159,  169,  366,  366,  366,  366,  366,  366,  366,
 /*  1160 */   366,  366,  366,  366,  366,  366,  366,  366,  148,   77,
 /*  1170 */   162,  150,  151,  152,   60,  366,  154,  155,   90,   93,
 /*  1180 */    58,   55,  366,  366,  168,  159,  169,  366,  133,  366,
 /*  1190 */   366,  366,  366,  366,  366,  366,  366,  366,  366,  366,
 /*  1200 */   366,  366,  366,  366,  366,  366,  366,  148,   77,  162,
 /*  1210 */   150,  151,  152,   60,  366,  154,  155,   90,   93,   58,
 /*  1220 */    55,  366,  366,  168,  159,  169,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   54,   12,   12,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   22,   23,   24,   87,   19,   20,   49,
 /*    20 */    50,   51,   52,   53,   65,   66,   67,   29,   30,   70,
 /*    30 */    25,   33,   55,   35,   36,   37,   38,   39,   40,   41,
 /*    40 */    42,   94,   72,   73,   74,   75,   76,   77,   78,   55,
 /*    50 */    80,   81,   82,   83,   84,   85,   22,   23,   88,   89,
 /*    60 */    90,   50,   51,   52,   53,   73,   74,   75,   76,   77,
 /*    70 */    78,   94,   80,   81,   82,   83,   84,   85,    1,    1,
 /*    80 */    88,   89,   90,   72,   73,   74,   75,   76,   77,   78,
 /*    90 */    12,   80,   81,   82,   83,   84,   85,   26,   20,   88,
 /*   100 */    89,   90,   50,   51,   52,   53,    1,   29,   30,    1,
 /*   110 */     5,   33,    4,   35,   36,   37,   38,   39,   40,   41,
 /*   120 */    42,    1,   45,    3,   72,   73,   74,   75,   76,   77,
 /*   130 */    78,   12,   80,   81,   82,   83,   84,   85,   18,   20,
 /*   140 */    88,   89,   90,   50,   51,   52,   53,   73,   29,   30,
 /*   150 */     1,   46,   33,    4,   35,   36,   37,   38,   39,   40,
 /*   160 */    41,   42,   88,   89,   90,   72,   73,   74,   75,   76,
 /*   170 */    77,   78,   86,   80,   81,   82,   83,   84,   85,   12,
 /*   180 */    92,   88,   89,   90,   50,   51,   52,   53,   73,   22,
 /*   190 */    23,   24,   79,   78,   33,   80,   81,   82,   83,   84,
 /*   200 */    85,   66,   67,   88,   89,   90,   72,   73,   74,   75,
 /*   210 */    76,   77,   78,   17,   80,   81,   82,   83,   84,   85,
 /*   220 */    73,   93,   88,   89,   90,   50,   51,   52,   53,   57,
 /*   230 */    83,   84,   85,   59,   60,   88,   89,   90,   62,   63,
 /*   240 */    64,   65,   66,   67,   66,   67,   70,   72,   73,   74,
 /*   250 */    75,   76,   77,   78,   12,   80,   81,   82,   83,   84,
 /*   260 */    85,   29,   30,   88,   89,   90,   50,   51,   52,   53,
 /*   270 */    62,   63,   64,   65,   66,   67,   66,   67,   70,   64,
 /*   280 */    65,   66,   67,   66,   67,   70,   68,   69,   72,   73,
 /*   290 */    74,   75,   76,   77,   78,   58,   80,   81,   82,   83,
 /*   300 */    84,   85,   73,   60,   88,   89,   90,   50,   51,   52,
 /*   310 */    53,   73,   23,   84,   85,   43,   44,   88,   89,   90,
 /*   320 */    31,   32,    3,   85,    5,   22,   88,   89,   90,   72,
 /*   330 */    73,   74,   75,   76,   77,   78,   67,   80,   81,   82,
 /*   340 */    83,   84,   85,   73,   12,   88,   89,   90,   50,   51,
 /*   350 */    52,   53,   73,    1,   22,   23,   24,   56,   88,   89,
 /*   360 */    90,   56,   61,    0,    1,   46,   61,   88,   89,   90,
 /*   370 */    72,   73,   74,   75,   76,   77,   78,   67,   80,   81,
 /*   380 */    82,   83,   84,   85,   69,   67,   88,   89,   90,   50,
 /*   390 */    51,   52,   53,   65,   66,   67,    1,   67,   70,   47,
 /*   400 */     5,   67,    1,   67,    1,    4,    1,    4,   67,    4,
 /*   410 */    67,   72,   73,   74,   75,   76,   77,   78,   93,   80,
 /*   420 */    81,   82,   83,   84,   85,    1,    4,   88,   89,   90,
 /*   430 */    50,   51,   52,   53,   16,    1,   27,   21,   20,   28,
 /*   440 */    15,   34,    1,   25,   21,    1,    4,    1,   12,    1,
 /*   450 */    15,   33,   72,   73,   74,   75,   76,   77,   78,   12,
 /*   460 */    80,   81,   82,   83,   84,   85,   15,   12,   88,   89,
 /*   470 */    90,   50,   51,   52,   53,   16,   16,   12,   20,   20,
 /*   480 */     1,   15,   21,   15,   15,   15,   12,   12,   15,   12,
 /*   490 */    34,   15,   33,   72,   73,   74,   75,   76,   77,   78,
 /*   500 */    15,   80,   81,   82,   83,   84,   85,   15,   15,   88,
 /*   510 */    89,   90,   50,   51,   52,   53,   15,   15,   34,   15,
 /*   520 */    15,   15,   15,   12,    4,   12,    1,   95,   95,   95,
 /*   530 */    95,   95,   95,   95,   72,   73,   74,   75,   76,   77,
 /*   540 */    78,   95,   80,   81,   82,   83,   84,   85,   95,   95,
 /*   550 */    88,   89,   90,   50,   51,   52,   53,   95,   95,   95,
 /*   560 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*   570 */    95,   95,   95,   95,   95,   72,   73,   74,   75,   76,
 /*   580 */    77,   78,   95,   80,   81,   82,   83,   84,   85,   95,
 /*   590 */    95,   88,   89,   90,   50,   51,   52,   53,   95,   95,
 /*   600 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*   610 */    95,   95,   95,   95,   95,   95,   72,   73,   74,   75,
 /*   620 */    76,   77,   78,   95,   80,   81,   82,   83,   84,   85,
 /*   630 */    95,   95,   88,   89,   90,   50,   51,   52,   53,   95,
 /*   640 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*   650 */    95,   95,   95,   95,   95,   95,   95,   72,   73,   74,
 /*   660 */    75,   76,   77,   78,   95,   80,   81,   82,   83,   84,
 /*   670 */    85,   95,   95,   88,   89,   90,   51,   52,   53,   95,
 /*   680 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*   690 */    95,   95,   95,   95,   95,   95,   95,   72,   73,   74,
 /*   700 */    75,   76,   77,   78,   95,   80,   81,   82,   83,   84,
 /*   710 */    85,   53,   95,   88,   89,   90,   95,   95,   95,   95,
 /*   720 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   71,
 /*   730 */    72,   73,   74,   75,   76,   77,   78,   95,   80,   81,
 /*   740 */    82,   83,   84,   85,   95,   95,   88,   89,   90,   91,
 /*   750 */    95,   53,   95,   95,   95,   95,   95,   95,   95,   95,
 /*   760 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   71,
 /*   770 */    72,   73,   74,   75,   76,   77,   78,   95,   80,   81,
 /*   780 */    82,   83,   84,   85,   53,   95,   88,   89,   90,   91,
 /*   790 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*   800 */    95,   95,   95,   72,   73,   74,   75,   76,   77,   78,
 /*   810 */    95,   80,   81,   82,   83,   84,   85,   95,   95,   88,
 /*   820 */    89,   90,   95,   53,   95,   95,   95,   95,   95,   95,
 /*   830 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*   840 */    95,   95,   72,   73,   74,   75,   76,   77,   78,   95,
 /*   850 */    80,   81,   82,   83,   84,   85,   95,   53,   88,   89,
 /*   860 */    90,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*   870 */    95,   95,   95,   95,   95,   95,   72,   73,   74,   75,
 /*   880 */    76,   77,   78,   95,   80,   81,   82,   83,   84,   85,
 /*   890 */    95,   95,   88,   89,   90,   95,   53,   95,   95,   95,
 /*   900 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*   910 */    95,   95,   95,   95,   95,   72,   73,   74,   75,   76,
 /*   920 */    77,   78,   95,   80,   81,   82,   83,   84,   85,   95,
 /*   930 */    53,   88,   89,   90,   95,   95,   95,   95,   95,   95,
 /*   940 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   72,
 /*   950 */    73,   74,   75,   76,   77,   78,   95,   80,   81,   82,
 /*   960 */    83,   84,   85,   95,   95,   88,   89,   90,   95,   53,
 /*   970 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*   980 */    95,   95,   95,   95,   95,   95,   95,   95,   72,   73,
 /*   990 */    74,   75,   76,   77,   78,   95,   80,   81,   82,   83,
 /*  1000 */    84,   85,   95,   53,   88,   89,   90,   95,   95,   95,
 /*  1010 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*  1020 */    95,   95,   72,   73,   74,   75,   76,   77,   78,   95,
 /*  1030 */    80,   81,   82,   83,   84,   85,   95,   95,   88,   89,
 /*  1040 */    90,   95,   53,   95,   95,   95,   95,   95,   95,   95,
 /*  1050 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*  1060 */    95,   72,   73,   74,   75,   76,   77,   78,   95,   80,
 /*  1070 */    81,   82,   83,   84,   85,   95,   53,   88,   89,   90,
 /*  1080 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*  1090 */    95,   95,   95,   95,   95,   72,   73,   74,   75,   76,
 /*  1100 */    77,   78,   95,   80,   81,   82,   83,   84,   85,   95,
 /*  1110 */    95,   88,   89,   90,   95,   53,   95,   95,   95,   95,
 /*  1120 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*  1130 */    95,   95,   95,   95,   72,   73,   74,   75,   76,   77,
 /*  1140 */    78,   95,   80,   81,   82,   83,   84,   85,   95,   53,
 /*  1150 */    88,   89,   90,   95,   95,   95,   95,   95,   95,   95,
 /*  1160 */    95,   95,   95,   95,   95,   95,   95,   95,   72,   73,
 /*  1170 */    74,   75,   76,   77,   78,   95,   80,   81,   82,   83,
 /*  1180 */    84,   85,   95,   95,   88,   89,   90,   95,   53,   95,
 /*  1190 */    95,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*  1200 */    95,   95,   95,   95,   95,   95,   95,   72,   73,   74,
 /*  1210 */    75,   76,   77,   78,   95,   80,   81,   82,   83,   84,
 /*  1220 */    85,   95,   95,   88,   89,   90,
};
#define YY_SHIFT_USE_DFLT (-11)
#define YY_SHIFT_MAX 140
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,  119,  119,   78,
 /*    20 */   119,  119,  119,  119,  119,  119,  119,  119,  119,  119,
 /*    30 */   119,  119,  119,  119,   -9,   -9,  119,  119,  167,  119,
 /*    40 */   119,  119,  332,  332,  105,  319,  120,  120,   34,   34,
 /*    50 */    34,   34,  -10,    5,  289,  289,  395,  232,  232,  272,
 /*    60 */    71,  161,  196,  242,  -10,  303,  303,    5,  303,  303,
 /*    70 */     5,  303,  303,  303,  303,    5,  161,  418,  459,  363,
 /*    80 */   108,  149,  401,  403,   77,  352,  405,  422,  424,  434,
 /*    90 */   409,  411,  425,  411,  407,  416,  423,  441,  444,  442,
 /*   100 */   446,  424,  436,  448,  435,  447,  451,  460,  455,  460,
 /*   110 */   465,  458,  461,  466,  468,  469,  470,  474,  475,  473,
 /*   120 */   477,  476,  485,  492,  493,  501,  502,  504,  505,  506,
 /*   130 */   507,  456,  479,  484,  511,  520,  513,  525,  424,  424,
 /*   140 */   424,
};
#define YY_REDUCE_USE_DFLT (-72)
#define YY_REDUCE_MAX 76
static const short yy_reduce_ofst[] = {
 /*     0 */   -30,   11,   52,   93,  134,  175,  216,  257,  298,  339,
 /*    10 */   380,  421,  462,  503,  544,  585,  625,  658,  698,  731,
 /*    20 */   770,  804,  843,  877,  916,  950,  989, 1023, 1062, 1096,
 /*    30 */  1135,   -8,  115,  147,  176,  208,  229,  238,  215,   74,
 /*    40 */   270,  279,  -41,  328,  -53,  -23,  301,  305,  135,  178,
 /*    50 */   210,  217,  174,  218,  -71,  -71,   -6,   86,   86,   88,
 /*    60 */   113,  128,  172,  237,  243,  269,  310,  315,  318,  330,
 /*    70 */   315,  334,  336,  341,  343,  315,  325,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   231,  231,  231,  231,  231,  231,  231,  231,  231,  231,
 /*    10 */   231,  231,  231,  231,  231,  231,  231,  350,  350,  364,
 /*    20 */   364,  238,  364,  364,  240,  242,  364,  364,  364,  364,
 /*    30 */   364,  364,  364,  364,  292,  292,  364,  364,  364,  364,
 /*    40 */   364,  364,  364,  364,  364,  362,  258,  258,  364,  364,
 /*    50 */   364,  364,  364,  296,  323,  322,  362,  321,  320,  352,
 /*    60 */   312,  355,  254,  364,  364,  364,  364,  364,  364,  364,
 /*    70 */   300,  364,  364,  364,  364,  299,  355,  334,  334,  364,
 /*    80 */   364,  364,  364,  364,  364,  364,  364,  364,  363,  364,
 /*    90 */   317,  319,  351,  318,  364,  364,  364,  364,  364,  364,
 /*   100 */   364,  259,  364,  364,  246,  364,  247,  249,  364,  248,
 /*   110 */   364,  364,  364,  276,  268,  264,  262,  364,  364,  266,
 /*   120 */   364,  272,  270,  274,  284,  280,  278,  282,  288,  286,
 /*   130 */   290,  364,  364,  364,  364,  364,  364,  364,  359,  360,
 /*   140 */   361,  230,  232,  233,  229,  234,  237,  239,  306,  307,
 /*   150 */   309,  310,  311,  313,  315,  316,  326,  331,  332,  333,
 /*   160 */   304,  305,  308,  324,  325,  328,  329,  330,  327,  335,
 /*   170 */   339,  340,  341,  342,  343,  344,  345,  346,  347,  348,
 /*   180 */   349,  336,  353,  241,  243,  244,  256,  257,  245,  253,
 /*   190 */   252,  251,  250,  260,  261,  293,  263,  294,  265,  267,
 /*   200 */   295,  297,  298,  302,  303,  269,  271,  273,  275,  301,
 /*   210 */   277,  279,  281,  283,  285,  287,  289,  291,  255,  356,
 /*   220 */   354,  337,  338,  314,  235,  358,  236,  357,
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
  "REGEXP",        "STRING",        "SYMBOL",        "NIL",         
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
 /* 101 */ "term_op ::= DIV",
 /* 102 */ "term_op ::= DIV_DIV",
 /* 103 */ "factor ::= PLUS factor",
 /* 104 */ "factor ::= MINUS factor",
 /* 105 */ "factor ::= power",
 /* 106 */ "power ::= postfix_expr",
 /* 107 */ "postfix_expr ::= atom",
 /* 108 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 109 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 110 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 111 */ "atom ::= NAME",
 /* 112 */ "atom ::= NUMBER",
 /* 113 */ "atom ::= REGEXP",
 /* 114 */ "atom ::= STRING",
 /* 115 */ "atom ::= SYMBOL",
 /* 116 */ "atom ::= NIL",
 /* 117 */ "atom ::= TRUE",
 /* 118 */ "atom ::= FALSE",
 /* 119 */ "atom ::= LINE",
 /* 120 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 121 */ "atom ::= LPAR expr RPAR",
 /* 122 */ "args_opt ::=",
 /* 123 */ "args_opt ::= args",
 /* 124 */ "blockarg_opt ::=",
 /* 125 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 126 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 127 */ "blockarg_params_opt ::=",
 /* 128 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 129 */ "excepts ::= except",
 /* 130 */ "excepts ::= excepts except",
 /* 131 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 132 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 133 */ "except ::= EXCEPT NEWLINE stmts",
 /* 134 */ "finally_opt ::=",
 /* 135 */ "finally_opt ::= FINALLY stmts",
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
  uint_t yyidx = PTR_AS(yyParser, parser)->yyidx;
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
  { 49, 1 },
  { 50, 1 },
  { 50, 3 },
  { 51, 0 },
  { 51, 1 },
  { 51, 1 },
  { 51, 7 },
  { 51, 5 },
  { 51, 5 },
  { 51, 5 },
  { 51, 1 },
  { 51, 2 },
  { 51, 1 },
  { 51, 2 },
  { 51, 1 },
  { 51, 2 },
  { 51, 6 },
  { 51, 6 },
  { 51, 2 },
  { 51, 2 },
  { 59, 1 },
  { 59, 3 },
  { 60, 1 },
  { 60, 3 },
  { 58, 1 },
  { 58, 3 },
  { 57, 0 },
  { 57, 2 },
  { 56, 1 },
  { 56, 5 },
  { 61, 0 },
  { 61, 2 },
  { 52, 7 },
  { 62, 9 },
  { 62, 7 },
  { 62, 7 },
  { 62, 5 },
  { 62, 7 },
  { 62, 5 },
  { 62, 5 },
  { 62, 3 },
  { 62, 7 },
  { 62, 5 },
  { 62, 5 },
  { 62, 3 },
  { 62, 5 },
  { 62, 3 },
  { 62, 3 },
  { 62, 1 },
  { 62, 7 },
  { 62, 5 },
  { 62, 5 },
  { 62, 3 },
  { 62, 5 },
  { 62, 3 },
  { 62, 3 },
  { 62, 1 },
  { 62, 5 },
  { 62, 3 },
  { 62, 3 },
  { 62, 1 },
  { 62, 3 },
  { 62, 1 },
  { 62, 1 },
  { 62, 0 },
  { 67, 2 },
  { 66, 2 },
  { 65, 3 },
  { 68, 0 },
  { 68, 1 },
  { 69, 2 },
  { 63, 1 },
  { 63, 3 },
  { 64, 1 },
  { 64, 3 },
  { 70, 2 },
  { 71, 1 },
  { 71, 3 },
  { 53, 1 },
  { 72, 3 },
  { 72, 1 },
  { 74, 1 },
  { 75, 1 },
  { 76, 1 },
  { 77, 1 },
  { 77, 3 },
  { 79, 1 },
  { 78, 1 },
  { 80, 1 },
  { 81, 1 },
  { 82, 1 },
  { 82, 3 },
  { 83, 1 },
  { 83, 3 },
  { 84, 1 },
  { 84, 3 },
  { 86, 1 },
  { 86, 1 },
  { 85, 3 },
  { 85, 1 },
  { 87, 1 },
  { 87, 1 },
  { 87, 1 },
  { 88, 2 },
  { 88, 2 },
  { 88, 1 },
  { 89, 1 },
  { 73, 1 },
  { 73, 5 },
  { 73, 4 },
  { 73, 3 },
  { 90, 1 },
  { 90, 1 },
  { 90, 1 },
  { 90, 1 },
  { 90, 1 },
  { 90, 1 },
  { 90, 1 },
  { 90, 1 },
  { 90, 1 },
  { 90, 3 },
  { 90, 3 },
  { 91, 0 },
  { 91, 1 },
  { 92, 0 },
  { 92, 5 },
  { 92, 5 },
  { 93, 0 },
  { 93, 3 },
  { 54, 1 },
  { 54, 2 },
  { 94, 6 },
  { 94, 4 },
  { 94, 3 },
  { 55, 0 },
  { 55, 2 },
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
#line 605 "parser.y"
{
    *pval = yymsp[0].minor.yy77;
}
#line 1852 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 129: /* excepts ::= except */
#line 609 "parser.y"
{
    yygotominor.yy77 = make_array_with(env, yymsp[0].minor.yy77);
}
#line 1863 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 612 "parser.y"
{
    yygotominor.yy77 = Array_push(env, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 1873 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 122: /* args_opt ::= */
      case 124: /* blockarg_opt ::= */
      case 127: /* blockarg_params_opt ::= */
      case 134: /* finally_opt ::= */
#line 616 "parser.y"
{
    yygotominor.yy77 = YNIL;
}
#line 1887 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 5: /* stmt ::= expr */
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
      case 105: /* factor ::= power */
      case 106: /* power ::= postfix_expr */
      case 107: /* postfix_expr ::= atom */
      case 123: /* args_opt ::= args */
      case 135: /* finally_opt ::= FINALLY stmts */
#line 619 "parser.y"
{
    yygotominor.yy77 = yymsp[0].minor.yy77;
}
#line 1918 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 625 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy77 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy77, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[-1].minor.yy77);
}
#line 1926 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 629 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy77 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy77, yymsp[-2].minor.yy77, YNIL, yymsp[-1].minor.yy77);
}
#line 1934 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 633 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy77 = Finally_new(env, lineno, yymsp[-3].minor.yy77, yymsp[-1].minor.yy77);
}
#line 1942 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 637 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy77 = While_new(env, lineno, yymsp[-3].minor.yy77, yymsp[-1].minor.yy77);
}
#line 1950 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 641 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Break_new(env, lineno, YNIL);
}
#line 1958 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 645 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy77 = Break_new(env, lineno, yymsp[0].minor.yy77);
}
#line 1966 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 649 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Next_new(env, lineno, YNIL);
}
#line 1974 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 653 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy77 = Next_new(env, lineno, yymsp[0].minor.yy77);
}
#line 1982 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 657 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Return_new(env, lineno, YNIL);
}
#line 1990 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 661 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy77 = Return_new(env, lineno, yymsp[0].minor.yy77);
}
#line 1998 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 665 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy77 = If_new(env, lineno, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[-1].minor.yy77);
}
#line 2006 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 669 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy77 = Klass_new(env, lineno, id, yymsp[-3].minor.yy77, yymsp[-1].minor.yy77);
}
#line 2015 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 674 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy77 = Nonlocal_new(env, lineno, yymsp[0].minor.yy77);
}
#line 2023 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 678 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy77 = Import_new(env, lineno, yymsp[0].minor.yy77);
}
#line 2031 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 690 "parser.y"
{
    yygotominor.yy77 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2039 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 693 "parser.y"
{
    yygotominor.yy77 = Array_push_token_id(env, yymsp[-2].minor.yy77, yymsp[0].minor.yy0);
}
#line 2047 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 714 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy77, yymsp[-1].minor.yy77, yymsp[0].minor.yy77);
    yygotominor.yy77 = make_array_with(env, node);
}
#line 2056 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 727 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy77 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy77, yymsp[-1].minor.yy77);
}
#line 2065 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 733 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-8].minor.yy77, yymsp[-6].minor.yy77, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2072 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 736 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-6].minor.yy77, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL);
}
#line 2079 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 739 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-6].minor.yy77, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77);
}
#line 2086 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 742 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL, YNIL);
}
#line 2093 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 745 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-6].minor.yy77, yymsp[-4].minor.yy77, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2100 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 748 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77, YNIL);
}
#line 2107 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 751 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, YNIL, YNIL, yymsp[0].minor.yy77);
}
#line 2114 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 754 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL, YNIL, YNIL);
}
#line 2121 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 757 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-6].minor.yy77, YNIL, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2128 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 760 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL);
}
#line 2135 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 763 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, YNIL, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77);
}
#line 2142 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 766 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77, YNIL, YNIL);
}
#line 2149 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 769 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-4].minor.yy77, YNIL, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2156 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 772 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-2].minor.yy77, YNIL, YNIL, yymsp[0].minor.yy77, YNIL);
}
#line 2163 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 775 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[-2].minor.yy77, YNIL, YNIL, YNIL, yymsp[0].minor.yy77);
}
#line 2170 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 778 "parser.y"
{
    yygotominor.yy77 = Params_new(env, yymsp[0].minor.yy77, YNIL, YNIL, YNIL, YNIL);
}
#line 2177 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 781 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-6].minor.yy77, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2184 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 784 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL);
}
#line 2191 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 787 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77);
}
#line 2198 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 790 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL, YNIL);
}
#line 2205 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 793 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-4].minor.yy77, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2212 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 796 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77, YNIL);
}
#line 2219 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 799 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[-2].minor.yy77, YNIL, YNIL, yymsp[0].minor.yy77);
}
#line 2226 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 802 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, yymsp[0].minor.yy77, YNIL, YNIL, YNIL);
}
#line 2233 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 805 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2240 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 808 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77, YNIL);
}
#line 2247 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 811 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy77, YNIL, yymsp[0].minor.yy77);
}
#line 2254 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 814 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy77, YNIL, YNIL);
}
#line 2261 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 817 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2268 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 820 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy77, YNIL);
}
#line 2275 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 823 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy77);
}
#line 2282 "parser.c"
        break;
      case 64: /* params ::= */
#line 826 "parser.y"
{
    yygotominor.yy77 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2289 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 830 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy77 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2298 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 836 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy77 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2307 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 842 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy77 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy77);
}
#line 2316 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 859 "parser.y"
{
    yygotominor.yy77 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy77, lineno, id, YNIL);
}
#line 2326 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 865 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy77, lineno, id, YNIL);
    yygotominor.yy77 = yymsp[-2].minor.yy77;
}
#line 2336 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 879 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy77 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy77);
}
#line 2345 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 896 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy77);
    yygotominor.yy77 = Assign_new(env, lineno, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2353 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 919 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy77);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy77)->u.id;
    yygotominor.yy77 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy77, id, yymsp[0].minor.yy77);
}
#line 2362 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
#line 925 "parser.y"
{
    yygotominor.yy77 = yymsp[0].minor.yy0;
}
#line 2369 "parser.c"
        break;
      case 91: /* shift_expr ::= shift_expr LSHIFT match_expr */
      case 93: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 944 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy77);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy77 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy77, id, yymsp[0].minor.yy77);
}
#line 2379 "parser.c"
        break;
      case 95: /* arith_expr ::= arith_expr arith_op term */
      case 98: /* term ::= term term_op factor */
#line 962 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy77);
    yygotominor.yy77 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy77, VAL2ID(yymsp[-1].minor.yy77), yymsp[0].minor.yy77);
}
#line 2388 "parser.c"
        break;
      case 96: /* arith_op ::= PLUS */
      case 97: /* arith_op ::= MINUS */
      case 100: /* term_op ::= STAR */
      case 101: /* term_op ::= DIV */
      case 102: /* term_op ::= DIV_DIV */
#line 967 "parser.y"
{
    yygotominor.yy77 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2399 "parser.c"
        break;
      case 103: /* factor ::= PLUS factor */
#line 992 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy77 = FuncCall_new3(env, lineno, yymsp[0].minor.yy77, id);
}
#line 2408 "parser.c"
        break;
      case 104: /* factor ::= MINUS factor */
#line 997 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy77 = FuncCall_new3(env, lineno, yymsp[0].minor.yy77, id);
}
#line 2417 "parser.c"
        break;
      case 108: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1013 "parser.y"
{
    yygotominor.yy77 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy77), yymsp[-4].minor.yy77, yymsp[-2].minor.yy77, yymsp[0].minor.yy77);
}
#line 2424 "parser.c"
        break;
      case 109: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1016 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy77);
    yygotominor.yy77 = Subscript_new(env, lineno, yymsp[-3].minor.yy77, yymsp[-1].minor.yy77);
}
#line 2432 "parser.c"
        break;
      case 110: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1020 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy77);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy77 = Attr_new(env, lineno, yymsp[-2].minor.yy77, id);
}
#line 2441 "parser.c"
        break;
      case 111: /* atom ::= NAME */
#line 1026 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy77 = Variable_new(env, lineno, id);
}
#line 2450 "parser.c"
        break;
      case 112: /* atom ::= NUMBER */
      case 113: /* atom ::= REGEXP */
      case 114: /* atom ::= STRING */
      case 115: /* atom ::= SYMBOL */
#line 1031 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy77 = Literal_new(env, lineno, val);
}
#line 2462 "parser.c"
        break;
      case 116: /* atom ::= NIL */
#line 1051 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Literal_new(env, lineno, YNIL);
}
#line 2470 "parser.c"
        break;
      case 117: /* atom ::= TRUE */
#line 1055 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Literal_new(env, lineno, YTRUE);
}
#line 2478 "parser.c"
        break;
      case 118: /* atom ::= FALSE */
#line 1059 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy77 = Literal_new(env, lineno, YFALSE);
}
#line 2486 "parser.c"
        break;
      case 119: /* atom ::= LINE */
#line 1063 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy77 = Literal_new(env, lineno, val);
}
#line 2495 "parser.c"
        break;
      case 120: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1068 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy77 = Array_new(env, lineno, yymsp[-1].minor.yy77);
}
#line 2503 "parser.c"
        break;
      case 121: /* atom ::= LPAR expr RPAR */
      case 128: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1072 "parser.y"
{
    yygotominor.yy77 = yymsp[-1].minor.yy77;
}
#line 2511 "parser.c"
        break;
      case 125: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 126: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1086 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy77 = BlockArg_new(env, lineno, yymsp[-3].minor.yy77, yymsp[-1].minor.yy77);
}
#line 2520 "parser.c"
        break;
      case 130: /* excepts ::= excepts except */
#line 1105 "parser.y"
{
    yygotominor.yy77 = Array_push(env, yymsp[-1].minor.yy77, yymsp[0].minor.yy77);
}
#line 2527 "parser.c"
        break;
      case 131: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1109 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy77 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy77, id, yymsp[0].minor.yy77);
}
#line 2537 "parser.c"
        break;
      case 132: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1115 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy77 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy77, NO_EXC_VAR, yymsp[0].minor.yy77);
}
#line 2545 "parser.c"
        break;
      case 133: /* except ::= EXCEPT NEWLINE stmts */
#line 1119 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy77 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy77);
}
#line 2553 "parser.c"
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
