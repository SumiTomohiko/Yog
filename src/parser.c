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

static BOOL Parse(struct YogEnv*, YogVal, int_t, YogVal, YogVal*);
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
    NODE(node)->u.literal.val = val;

    RETURN(env, node);
}

static YogVal 
BlockArg_new(YogEnv* env, uint_t lineno, YogVal params, YogVal stmts) 
{
    SAVE_ARGS2(env, params, stmts);

    YogVal node = YogNode_new(env, NODE_BLOCK_ARG, lineno);
    NODE(node)->u.blockarg.params = params;
    NODE(node)->u.blockarg.stmts = stmts;

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
    NODE(node)->u.param.default_ = default_;

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
    NODE(node)->u.funcdef.params = params;
    NODE(node)->u.funcdef.stmts = stmts;

    RETURN(env, node);
}

static YogVal 
FuncCall_new(YogEnv* env, uint_t lineno, YogVal callee, YogVal args, YogVal blockarg) 
{
    SAVE_ARGS3(env, callee, args, blockarg);

    YogVal node = NODE_NEW(NODE_FUNC_CALL, lineno);
    NODE(node)->u.func_call.callee = callee;
    NODE(node)->u.func_call.args = args;
    NODE(node)->u.func_call.blockarg = blockarg;

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
    NODE(node)->u.except_body.type = type;
    NODE(node)->u.except_body.var = var;
    NODE(node)->u.except_body.stmts = stmts;

    RETURN(env, node);
}

static YogVal 
Except_new(YogEnv* env, uint_t lineno, YogVal head, YogVal excepts, YogVal else_) 
{
    SAVE_ARGS3(env, head, excepts, else_);

    YogVal node = NODE_NEW(NODE_EXCEPT, lineno);
    NODE(node)->u.except.head = head;
    NODE(node)->u.except.excepts = excepts;
    NODE(node)->u.except.else_ = else_;

    RETURN(env, node);
}

static YogVal 
Finally_new(YogEnv* env, uint_t lineno, YogVal head, YogVal body) 
{
    SAVE_ARGS2(env, head, body);

    YogVal node = NODE_NEW(NODE_FINALLY, lineno);
    NODE(node)->u.finally.head = head;
    NODE(node)->u.finally.body = body;

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
    NODE(node)->u.break_.expr = expr;

    RETURN(env, node);
}

static YogVal 
Next_new(YogEnv* env, uint_t lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = NODE_NEW(NODE_NEXT, lineno);
    NODE(node)->u.next.expr = expr;

    RETURN(env, node);
}

static YogVal 
Return_new(YogEnv* env, uint_t lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = NODE_NEW(NODE_RETURN, lineno);
    NODE(node)->u.return_.expr = expr;

    RETURN(env, node);
}

static YogVal 
Attr_new(YogEnv* env, uint_t lineno, YogVal obj, ID name) 
{
    SAVE_ARG(env, obj);

    YogVal node = YogNode_new(env, NODE_ATTR, lineno);
    NODE(node)->u.attr.obj = obj;
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
    NODE(node)->u.if_.test = test;
    NODE(node)->u.if_.stmts = stmts;
    NODE(node)->u.if_.tail = tail;

    RETURN(env, node);
}

static YogVal 
While_new(YogEnv* env, uint_t lineno, YogVal test, YogVal stmts) 
{
    SAVE_ARGS2(env, test, stmts);

    YogVal node = YogNode_new(env, NODE_WHILE, lineno);
    NODE(node)->u.while_.test = test;
    NODE(node)->u.while_.stmts = stmts;

    RETURN(env, node);
}

static YogVal 
Klass_new(YogEnv* env, uint_t lineno, ID name, YogVal super, YogVal stmts)
{
    SAVE_ARGS2(env, super, stmts);

    YogVal node = YogNode_new(env, NODE_KLASS, lineno);
    NODE(node)->u.klass.name = name;
    NODE(node)->u.klass.super = super;
    NODE(node)->u.klass.stmts = stmts;

    RETURN(env, node);
}

static YogVal 
Assign_new(YogEnv* env, uint_t lineno, YogVal left, YogVal right) 
{
    SAVE_ARGS2(env, left, right);

    YogVal node = NODE_NEW(NODE_ASSIGN, lineno);
    NODE(node)->u.assign.left = left;
    NODE(node)->u.assign.right = right;

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
    NODE(node)->u.nonlocal.names = names;

    RETURN(env, node);
}

static YogVal
Import_new(YogEnv* env, uint_t lineno, YogVal names)
{
    SAVE_ARG(env, names);

    YogVal node = YogNode_new(env, NODE_IMPORT, lineno);
    NODE(node)->u.import.names = names;

    RETURN(env, node);
}

static void
push_token(YogEnv* env, YogVal parser, YogVal lexer, YogVal token, const char* filename, YogVal* ast)
{
    SAVE_ARGS3(env, parser, lexer, token);

    uint_t type = PTR_AS(YogToken, token)->type;
    if (Parse(env, parser, type, token, ast)) {
        RETURN_VOID(env);
    }

    YogError_raise_SyntaxError(env, "file \"%s\", line %u: invalid syntax", filename, PTR_AS(YogLexer, lexer)->lineno);

    /* NOTREACHED */
    RETURN_VOID(env);
}

static YogVal
parse(YogEnv* env, YogVal lexer, const char* filename, BOOL debug)
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
        push_token(env, lemon_parser, lexer, token, filename, &ast);
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

    ast = parse(env, lexer, "<stdin>", FALSE);

    RETURN(env, ast);
}

YogVal 
YogParser_parse_file(YogEnv* env, FILE* fp, const char* filename, BOOL debug)
{
    YOG_ASSERT(env, fp != NULL, "file pointer is NULL");

    SAVE_LOCALS(env);
    YogVal lexer = YUNDEF;
    YogVal ast = YUNDEF;
    PUSH_LOCALS2(env, lexer, ast);

    lexer = YogLexer_new(env);
    PTR_AS(YogLexer, lexer)->fp = fp;
    YogLexer_read_encoding(env, lexer);

    ast = parse(env, lexer, filename, debug);

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
#line 624 "parser.c"
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
#define YYNOCODE 98
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy123;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 231
#define YYNRULE 139
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
 /*     0 */     1,  117,  118,   76,   20,   21,   24,   25,   26,  102,
 /*    10 */   170,   64,   52,  117,  118,  120,  192,  110,   23,  371,
 /*    20 */    80,  144,  142,  143,  114,  121,  123,  208,   40,   41,
 /*    30 */   209,   28,   18,   39,  171,  172,  173,  174,  175,  176,
 /*    40 */   177,  178,  148,   78,  162,  150,  151,  152,   57,  170,
 /*    50 */   154,  155,   58,   93,   32,   60,   55,   23,   33,  168,
 /*    60 */   159,  169,   44,  144,  142,  143,   88,   40,   41,  116,
 /*    70 */   196,   18,  165,  171,  172,  173,  174,  175,  176,  177,
 /*    80 */   178,  166,  167,   45,  148,   78,  162,  150,  151,  152,
 /*    90 */    57,   37,  154,  155,   58,   93,  134,   60,   55,  181,
 /*   100 */    17,  168,  159,  169,   56,  144,  142,  143,  112,  113,
 /*   110 */   124,  128,  130,  217,   30,   13,  209,  131,  113,  124,
 /*   120 */   128,  130,  217,   35,  230,  209,  148,   78,  162,  150,
 /*   130 */   151,  152,   57,   99,  154,  155,   58,   93,  186,   60,
 /*   140 */    55,   97,   79,  168,  159,  169,   89,  144,  142,  143,
 /*   150 */    16,  187,   91,   71,   60,   55,  186,   16,  168,  159,
 /*   160 */   169,   15,  136,  117,  118,  120,  122,  206,  148,   78,
 /*   170 */   162,  150,  151,  152,   57,  103,  154,  155,   58,   93,
 /*   180 */    79,   60,   55,   79,   29,  168,  159,  169,   81,  144,
 /*   190 */   142,  143,   59,   55,   68,  220,  168,  159,  169,  156,
 /*   200 */   159,  169,  104,   19,  117,  118,  120,    2,  226,    3,
 /*   210 */   148,   78,  162,  150,  151,  152,   57,  225,  154,  155,
 /*   220 */    58,   93,   79,   60,   55,  126,  211,  168,  159,  169,
 /*   230 */    82,  144,  142,  143,  190,   54,  117,  135,  168,  159,
 /*   240 */   169,  115,  119,  199,  129,  215,  203,  106,  109,  200,
 /*   250 */   201,   19,  148,   78,  162,  150,  151,  152,   57,  107,
 /*   260 */   154,  155,   58,   93,   79,   60,   55,   79,  194,  168,
 /*   270 */   159,  169,   46,  144,  142,  143,   16,  228,    8,  198,
 /*   280 */   157,  159,  169,  158,  159,  169,  125,  127,  213,  223,
 /*   290 */   224,  203,  205,   27,  148,   78,  162,  150,  151,  152,
 /*   300 */    57,  204,  154,  155,   58,   93,  207,   60,   55,  163,
 /*   310 */   164,  168,  159,  169,   47,  144,  142,  143,   16,  210,
 /*   320 */    62,   77,    3,  231,   16,  134,  212,   16,  214,   17,
 /*   330 */   146,  132,   16,  216,   31,  182,  148,   78,  162,  150,
 /*   340 */   151,  152,   57,   30,  154,  155,   58,   93,  145,   60,
 /*   350 */    55,   16,    4,  168,  159,  169,  101,  144,  142,  143,
 /*   360 */    16,   16,   16,  188,  193,  229,   36,   22,  179,  180,
 /*   370 */    61,    5,    6,  185,    7,   63,    9,  105,  148,   78,
 /*   380 */   162,  150,  151,  152,   57,   34,  154,  155,   58,   93,
 /*   390 */   189,   60,   55,  108,   65,  168,  159,  169,   83,  144,
 /*   400 */   142,  143,  191,  111,   10,  372,   11,   38,   42,   48,
 /*   410 */    66,  195,  197,   67,   53,  219,   49,  221,   69,   70,
 /*   420 */   148,   78,  162,  150,  151,  152,   57,   43,  154,  155,
 /*   430 */    58,   93,   50,   60,   55,   72,   73,  168,  159,  169,
 /*   440 */    84,  144,  142,  143,   51,   74,  222,   75,  227,  137,
 /*   450 */    12,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   460 */   372,  372,  148,   78,  162,  150,  151,  152,   57,  372,
 /*   470 */   154,  155,   58,   93,  372,   60,   55,  372,  372,  168,
 /*   480 */   159,  169,   85,  144,  142,  143,  372,  372,  372,  372,
 /*   490 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   500 */   372,  372,  372,  372,  148,   78,  162,  150,  151,  152,
 /*   510 */    57,  372,  154,  155,   58,   93,  372,   60,   55,  372,
 /*   520 */   372,  168,  159,  169,  138,  144,  142,  143,  372,  372,
 /*   530 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   540 */   372,  372,  372,  372,  372,  372,  148,   78,  162,  150,
 /*   550 */   151,  152,   57,  372,  154,  155,   58,   93,  372,   60,
 /*   560 */    55,  372,  372,  168,  159,  169,  139,  144,  142,  143,
 /*   570 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   580 */   372,  372,  372,  372,  372,  372,  372,  372,  148,   78,
 /*   590 */   162,  150,  151,  152,   57,  372,  154,  155,   58,   93,
 /*   600 */   372,   60,   55,  372,  372,  168,  159,  169,  140,  144,
 /*   610 */   142,  143,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   620 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   630 */   148,   78,  162,  150,  151,  152,   57,  372,  154,  155,
 /*   640 */    58,   93,  372,   60,   55,  372,  372,  168,  159,  169,
 /*   650 */    87,  144,  142,  143,  372,  372,  372,  372,  372,  372,
 /*   660 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   670 */   372,  372,  148,   78,  162,  150,  151,  152,   57,  372,
 /*   680 */   154,  155,   58,   93,  372,   60,   55,  372,  372,  168,
 /*   690 */   159,  169,  141,  142,  143,  372,  372,  372,  372,  372,
 /*   700 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   710 */   372,  372,  372,  148,   78,  162,  150,  151,  152,   57,
 /*   720 */   372,  154,  155,   58,   93,  372,   60,   55,  160,  372,
 /*   730 */   168,  159,  169,  372,  372,  372,  372,  372,  372,  372,
 /*   740 */   372,  372,  372,  372,  372,  372,   92,  148,   78,  162,
 /*   750 */   150,  151,  152,   57,  372,  154,  155,   58,   93,  372,
 /*   760 */    60,   55,  372,  372,  168,  159,  169,   96,  160,  372,
 /*   770 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   780 */   372,  372,  372,  372,  372,  372,   92,  148,   78,  162,
 /*   790 */   150,  151,  152,   57,  372,  154,  155,   58,   93,  372,
 /*   800 */    60,   55,   86,  372,  168,  159,  169,   94,  372,  372,
 /*   810 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   820 */   372,  148,   78,  162,  150,  151,  152,   57,  372,  154,
 /*   830 */   155,   58,   93,  372,   60,   55,  372,  372,  168,  159,
 /*   840 */   169,   90,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   850 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   860 */   148,   78,  162,  150,  151,  152,   57,  372,  154,  155,
 /*   870 */    58,   93,  372,   60,   55,   79,  147,  168,  159,  169,
 /*   880 */   153,  372,  154,  155,   58,   93,  372,   60,   55,  372,
 /*   890 */   372,  168,  159,  169,  372,  148,   78,  162,  150,  151,
 /*   900 */   152,   57,  372,  154,  155,   58,   93,  372,   60,   55,
 /*   910 */   372,  372,  168,  159,  169,  161,  372,  372,  372,  372,
 /*   920 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*   930 */   372,  372,  372,  372,  148,   78,  162,  150,  151,  152,
 /*   940 */    57,  372,  154,  155,   58,   93,  372,   60,   55,  372,
 /*   950 */    95,  168,  159,  169,  372,  372,  372,  372,  372,  372,
 /*   960 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  148,
 /*   970 */    78,  162,  150,  151,  152,   57,  372,  154,  155,   58,
 /*   980 */    93,  372,   60,   55,  372,  372,  168,  159,  169,  183,
 /*   990 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*  1000 */   372,  372,  372,  372,  372,  372,  372,  372,  148,   78,
 /*  1010 */   162,  150,  151,  152,   57,  372,  154,  155,   58,   93,
 /*  1020 */   372,   60,   55,  372,  184,  168,  159,  169,  372,  372,
 /*  1030 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*  1040 */   372,  372,  372,  148,   78,  162,  150,  151,  152,   57,
 /*  1050 */   372,  154,  155,   58,   93,  372,   60,   55,  372,  372,
 /*  1060 */   168,  159,  169,   98,  372,  372,  372,  372,  372,  372,
 /*  1070 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*  1080 */   372,  372,  148,   78,  162,  150,  151,  152,   57,  372,
 /*  1090 */   154,  155,   58,   93,  372,   60,   55,  372,  100,  168,
 /*  1100 */   159,  169,  372,  372,  372,  372,  372,  372,  372,  372,
 /*  1110 */   372,  372,  372,  372,  372,  372,  372,  148,   78,  162,
 /*  1120 */   150,  151,  152,   57,  372,  154,  155,   58,   93,  372,
 /*  1130 */    60,   55,  372,  372,  168,  159,  169,  202,  372,  372,
 /*  1140 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*  1150 */   372,  372,  372,  372,  372,  372,  148,   78,  162,  150,
 /*  1160 */   151,  152,   57,  372,  154,  155,   58,   93,  372,   60,
 /*  1170 */    55,  372,  218,  168,  159,  169,  372,  372,  372,  372,
 /*  1180 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*  1190 */   372,  148,   78,  162,  150,  151,  152,   57,  372,  154,
 /*  1200 */   155,   58,   93,  372,   60,   55,  372,  372,  168,  159,
 /*  1210 */   169,  133,  372,  372,  372,  372,  372,  372,  372,  372,
 /*  1220 */   372,  372,  372,  372,  372,  372,  372,  372,  372,  372,
 /*  1230 */   148,   78,  162,  150,  151,  152,   57,  372,  154,  155,
 /*  1240 */    58,   93,  372,   60,   55,   14,  372,  168,  159,  169,
 /*  1250 */    79,  149,  150,  151,  152,   57,  170,  154,  155,   58,
 /*  1260 */    93,  372,   60,   55,   23,  372,  168,  159,  169,  372,
 /*  1270 */   372,  372,  372,  372,   40,   41,  372,  372,   18,  372,
 /*  1280 */   171,  172,  173,  174,  175,  176,  177,  178,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   12,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   22,   23,   24,   12,   19,   20,   50,
 /*    20 */    51,   52,   53,   54,   65,   66,   67,   68,   30,   31,
 /*    30 */    71,   25,   34,   89,   36,   37,   38,   39,   40,   41,
 /*    40 */    42,   43,   73,   74,   75,   76,   77,   78,   79,   12,
 /*    50 */    81,   82,   83,   84,   80,   86,   87,   20,   85,   90,
 /*    60 */    91,   92,   51,   52,   53,   54,   56,   30,   31,   67,
 /*    70 */    68,   34,   23,   36,   37,   38,   39,   40,   41,   42,
 /*    80 */    43,   32,   33,   55,   73,   74,   75,   76,   77,   78,
 /*    90 */    79,   88,   81,   82,   83,   84,   16,   86,   87,   94,
 /*   100 */    20,   90,   91,   92,   51,   52,   53,   54,   63,   64,
 /*   110 */    65,   66,   67,   68,   34,    1,   71,   63,   64,   65,
 /*   120 */    66,   67,   68,   34,   96,   71,   73,   74,   75,   76,
 /*   130 */    77,   78,   79,   57,   81,   82,   83,   84,   62,   86,
 /*   140 */    87,   95,   74,   90,   91,   92,   51,   52,   53,   54,
 /*   150 */     1,   57,   84,   12,   86,   87,   62,    1,   90,   91,
 /*   160 */    92,    5,   48,   22,   23,   24,   67,   68,   73,   74,
 /*   170 */    75,   76,   77,   78,   79,   58,   81,   82,   83,   84,
 /*   180 */    74,   86,   87,   74,   17,   90,   91,   92,   51,   52,
 /*   190 */    53,   54,   86,   87,   12,   46,   90,   91,   92,   90,
 /*   200 */    91,   92,   59,   47,   22,   23,   24,    3,   17,    5,
 /*   210 */    73,   74,   75,   76,   77,   78,   79,   26,   81,   82,
 /*   220 */    83,   84,   74,   86,   87,   67,   68,   90,   91,   92,
 /*   230 */    51,   52,   53,   54,   12,   87,   22,   56,   90,   91,
 /*   240 */    92,   66,   67,   68,   67,   68,   71,   60,   61,   69,
 /*   250 */    70,   47,   73,   74,   75,   76,   77,   78,   79,   61,
 /*   260 */    81,   82,   83,   84,   74,   86,   87,   74,   68,   90,
 /*   270 */    91,   92,   51,   52,   53,   54,    1,   96,    3,   68,
 /*   280 */    90,   91,   92,   90,   91,   92,   66,   67,   68,   27,
 /*   290 */    28,   71,   68,   18,   73,   74,   75,   76,   77,   78,
 /*   300 */    79,   70,   81,   82,   83,   84,   68,   86,   87,   30,
 /*   310 */    31,   90,   91,   92,   51,   52,   53,   54,    1,   68,
 /*   320 */    44,   45,    5,    0,    1,   16,   68,    1,   68,   20,
 /*   330 */     4,   95,    1,   68,   25,    4,   73,   74,   75,   76,
 /*   340 */    77,   78,   79,   34,   81,   82,   83,   84,    4,   86,
 /*   350 */    87,    1,    1,   90,   91,   92,   51,   52,   53,   54,
 /*   360 */     1,    1,    1,    4,    4,    4,   29,   15,   35,   21,
 /*   370 */    21,    1,    1,    4,    1,   12,    1,   15,   73,   74,
 /*   380 */    75,   76,   77,   78,   79,   20,   81,   82,   83,   84,
 /*   390 */    12,   86,   87,   16,   15,   90,   91,   92,   51,   52,
 /*   400 */    53,   54,   12,   12,   21,   97,    1,   15,   15,   15,
 /*   410 */    15,   12,   12,   15,   12,   35,   15,   35,   15,   15,
 /*   420 */    73,   74,   75,   76,   77,   78,   79,   15,   81,   82,
 /*   430 */    83,   84,   15,   86,   87,   15,   15,   90,   91,   92,
 /*   440 */    51,   52,   53,   54,   15,   15,   12,   15,    4,   12,
 /*   450 */     1,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   460 */    97,   97,   73,   74,   75,   76,   77,   78,   79,   97,
 /*   470 */    81,   82,   83,   84,   97,   86,   87,   97,   97,   90,
 /*   480 */    91,   92,   51,   52,   53,   54,   97,   97,   97,   97,
 /*   490 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   500 */    97,   97,   97,   97,   73,   74,   75,   76,   77,   78,
 /*   510 */    79,   97,   81,   82,   83,   84,   97,   86,   87,   97,
 /*   520 */    97,   90,   91,   92,   51,   52,   53,   54,   97,   97,
 /*   530 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   540 */    97,   97,   97,   97,   97,   97,   73,   74,   75,   76,
 /*   550 */    77,   78,   79,   97,   81,   82,   83,   84,   97,   86,
 /*   560 */    87,   97,   97,   90,   91,   92,   51,   52,   53,   54,
 /*   570 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   580 */    97,   97,   97,   97,   97,   97,   97,   97,   73,   74,
 /*   590 */    75,   76,   77,   78,   79,   97,   81,   82,   83,   84,
 /*   600 */    97,   86,   87,   97,   97,   90,   91,   92,   51,   52,
 /*   610 */    53,   54,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   620 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   630 */    73,   74,   75,   76,   77,   78,   79,   97,   81,   82,
 /*   640 */    83,   84,   97,   86,   87,   97,   97,   90,   91,   92,
 /*   650 */    51,   52,   53,   54,   97,   97,   97,   97,   97,   97,
 /*   660 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   670 */    97,   97,   73,   74,   75,   76,   77,   78,   79,   97,
 /*   680 */    81,   82,   83,   84,   97,   86,   87,   97,   97,   90,
 /*   690 */    91,   92,   52,   53,   54,   97,   97,   97,   97,   97,
 /*   700 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   710 */    97,   97,   97,   73,   74,   75,   76,   77,   78,   79,
 /*   720 */    97,   81,   82,   83,   84,   97,   86,   87,   54,   97,
 /*   730 */    90,   91,   92,   97,   97,   97,   97,   97,   97,   97,
 /*   740 */    97,   97,   97,   97,   97,   97,   72,   73,   74,   75,
 /*   750 */    76,   77,   78,   79,   97,   81,   82,   83,   84,   97,
 /*   760 */    86,   87,   97,   97,   90,   91,   92,   93,   54,   97,
 /*   770 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   780 */    97,   97,   97,   97,   97,   97,   72,   73,   74,   75,
 /*   790 */    76,   77,   78,   79,   97,   81,   82,   83,   84,   97,
 /*   800 */    86,   87,   54,   97,   90,   91,   92,   93,   97,   97,
 /*   810 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   820 */    97,   73,   74,   75,   76,   77,   78,   79,   97,   81,
 /*   830 */    82,   83,   84,   97,   86,   87,   97,   97,   90,   91,
 /*   840 */    92,   54,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   850 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   860 */    73,   74,   75,   76,   77,   78,   79,   97,   81,   82,
 /*   870 */    83,   84,   97,   86,   87,   74,   54,   90,   91,   92,
 /*   880 */    79,   97,   81,   82,   83,   84,   97,   86,   87,   97,
 /*   890 */    97,   90,   91,   92,   97,   73,   74,   75,   76,   77,
 /*   900 */    78,   79,   97,   81,   82,   83,   84,   97,   86,   87,
 /*   910 */    97,   97,   90,   91,   92,   54,   97,   97,   97,   97,
 /*   920 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   930 */    97,   97,   97,   97,   73,   74,   75,   76,   77,   78,
 /*   940 */    79,   97,   81,   82,   83,   84,   97,   86,   87,   97,
 /*   950 */    54,   90,   91,   92,   97,   97,   97,   97,   97,   97,
 /*   960 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   73,
 /*   970 */    74,   75,   76,   77,   78,   79,   97,   81,   82,   83,
 /*   980 */    84,   97,   86,   87,   97,   97,   90,   91,   92,   54,
 /*   990 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*  1000 */    97,   97,   97,   97,   97,   97,   97,   97,   73,   74,
 /*  1010 */    75,   76,   77,   78,   79,   97,   81,   82,   83,   84,
 /*  1020 */    97,   86,   87,   97,   54,   90,   91,   92,   97,   97,
 /*  1030 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*  1040 */    97,   97,   97,   73,   74,   75,   76,   77,   78,   79,
 /*  1050 */    97,   81,   82,   83,   84,   97,   86,   87,   97,   97,
 /*  1060 */    90,   91,   92,   54,   97,   97,   97,   97,   97,   97,
 /*  1070 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*  1080 */    97,   97,   73,   74,   75,   76,   77,   78,   79,   97,
 /*  1090 */    81,   82,   83,   84,   97,   86,   87,   97,   54,   90,
 /*  1100 */    91,   92,   97,   97,   97,   97,   97,   97,   97,   97,
 /*  1110 */    97,   97,   97,   97,   97,   97,   97,   73,   74,   75,
 /*  1120 */    76,   77,   78,   79,   97,   81,   82,   83,   84,   97,
 /*  1130 */    86,   87,   97,   97,   90,   91,   92,   54,   97,   97,
 /*  1140 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*  1150 */    97,   97,   97,   97,   97,   97,   73,   74,   75,   76,
 /*  1160 */    77,   78,   79,   97,   81,   82,   83,   84,   97,   86,
 /*  1170 */    87,   97,   54,   90,   91,   92,   97,   97,   97,   97,
 /*  1180 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*  1190 */    97,   73,   74,   75,   76,   77,   78,   79,   97,   81,
 /*  1200 */    82,   83,   84,   97,   86,   87,   97,   97,   90,   91,
 /*  1210 */    92,   54,   97,   97,   97,   97,   97,   97,   97,   97,
 /*  1220 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*  1230 */    73,   74,   75,   76,   77,   78,   79,   97,   81,   82,
 /*  1240 */    83,   84,   97,   86,   87,    1,   97,   90,   91,   92,
 /*  1250 */    74,   75,   76,   77,   78,   79,   12,   81,   82,   83,
 /*  1260 */    84,   97,   86,   87,   20,   97,   90,   91,   92,   97,
 /*  1270 */    97,   97,   97,   97,   30,   31,   97,   97,   34,   97,
 /*  1280 */    36,   37,   38,   39,   40,   41,   42,   43,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 140
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   37,   37, 1244,
 /*    20 */    37,   37,   37,   37,   37,   37,   37,   37,   37,   37,
 /*    30 */    37,   37,   37,   37,   -9,   -9,   37,   37,  141,   37,
 /*    40 */    37,   37,  182,  182,  156,  204,  275,  275,  -21,  -21,
 /*    50 */   -21,  -21,    4,    6,   49,   49,  317,  191,  262,  279,
 /*    60 */   279,  276,   89,  167,  222,    4,  214,  214,    6,  214,
 /*    70 */   214,    6,  214,  214,  214,  214,    6,   89,  309,   80,
 /*    80 */   323,  326,  331,  359,  360,  149,  114,  361,  344,  350,
 /*    90 */   351,  337,  352,  337,  333,  348,  349,  370,  371,  369,
 /*   100 */   373,  350,  363,  375,  362,  378,  379,  377,  390,  377,
 /*   110 */   391,  365,  383,  392,  393,  394,  395,  399,  400,  398,
 /*   120 */   402,  401,  403,  404,  412,  417,  420,  421,  429,  430,
 /*   130 */   432,  380,  405,  382,  434,  444,  437,  449,  350,  350,
 /*   140 */   350,
};
#define YY_REDUCE_USE_DFLT (-57)
#define YY_REDUCE_MAX 77
static const short yy_reduce_ofst[] = {
 /*     0 */   -31,   11,   53,   95,  137,  179,  221,  263,  305,  347,
 /*    10 */   389,  431,  473,  515,  557,  599,  640,  674,  714,  748,
 /*    20 */   787,  822,  861,  896,  935,  970, 1009, 1044, 1083, 1118,
 /*    30 */  1157, 1176,  801,   68,   45,   54,  106,  148,  -41,  109,
 /*    40 */   190,  193,  175,  220,   28,  181,   76,   94,    2,   99,
 /*    50 */   158,  177,  187,  180,  -56,  -56,   10,  -26,  -27,    3,
 /*    60 */     3,    5,   46,  117,  143,  198,  200,  211,  231,  224,
 /*    70 */   238,  231,  251,  258,  260,  265,  231,  236,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   234,  234,  234,  234,  234,  234,  234,  234,  234,  234,
 /*    10 */   234,  234,  234,  234,  234,  234,  234,  356,  356,  370,
 /*    20 */   370,  241,  370,  370,  243,  245,  370,  370,  370,  370,
 /*    30 */   370,  370,  370,  370,  295,  295,  370,  370,  370,  370,
 /*    40 */   370,  370,  370,  370,  370,  368,  261,  261,  370,  370,
 /*    50 */   370,  370,  370,  299,  329,  328,  368,  315,  321,  327,
 /*    60 */   326,  358,  361,  257,  370,  370,  370,  370,  370,  370,
 /*    70 */   370,  303,  370,  370,  370,  370,  302,  361,  340,  340,
 /*    80 */   370,  370,  370,  370,  370,  370,  370,  370,  370,  369,
 /*    90 */   370,  323,  357,  322,  370,  370,  370,  370,  370,  370,
 /*   100 */   370,  262,  370,  370,  249,  370,  250,  252,  370,  251,
 /*   110 */   370,  370,  370,  279,  271,  267,  265,  370,  370,  269,
 /*   120 */   370,  275,  273,  277,  287,  283,  281,  285,  291,  289,
 /*   130 */   293,  370,  370,  370,  370,  370,  370,  370,  365,  366,
 /*   140 */   367,  233,  235,  236,  232,  237,  240,  242,  309,  310,
 /*   150 */   312,  313,  314,  316,  319,  320,  332,  337,  338,  339,
 /*   160 */   307,  308,  311,  330,  331,  334,  335,  336,  333,  341,
 /*   170 */   345,  346,  347,  348,  349,  350,  351,  352,  353,  354,
 /*   180 */   355,  342,  359,  244,  246,  247,  259,  260,  248,  256,
 /*   190 */   255,  254,  253,  263,  264,  296,  266,  297,  268,  270,
 /*   200 */   298,  300,  301,  305,  306,  272,  274,  276,  278,  304,
 /*   210 */   280,  282,  284,  286,  288,  290,  292,  294,  258,  362,
 /*   220 */   360,  343,  344,  324,  325,  317,  318,  238,  364,  239,
 /*   230 */   363,
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
  "RSHIFT",        "EQUAL_TILDA",   "PLUS",          "MINUS",       
  "DIV",           "DIV_DIV",       "LBRACKET",      "RBRACKET",    
  "NUMBER",        "REGEXP",        "STRING",        "SYMBOL",      
  "NIL",           "TRUE",          "FALSE",         "LINE",        
  "DO",            "LBRACE",        "RBRACE",        "EXCEPT",      
  "AS",            "error",         "module",        "stmts",       
  "stmt",          "func_def",      "expr",          "excepts",     
  "finally_opt",   "if_tail",       "super_opt",     "names",       
  "dotted_names",  "dotted_name",   "else_opt",      "params",      
  "params_without_default",  "params_with_default",  "block_param",   "var_param",   
  "kw_param",      "param_default_opt",  "param_default",  "param_with_default",
  "args",          "assign_expr",   "postfix_expr",  "logical_or_expr",
  "logical_and_expr",  "not_expr",      "comparison",    "xor_expr",    
  "comp_op",       "or_expr",       "and_expr",      "shift_expr",  
  "match_expr",    "shift_op",      "arith_expr",    "term",        
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
 /*  87 */ "comp_op ::= GREATER",
 /*  88 */ "xor_expr ::= or_expr",
 /*  89 */ "or_expr ::= and_expr",
 /*  90 */ "and_expr ::= shift_expr",
 /*  91 */ "shift_expr ::= match_expr",
 /*  92 */ "shift_expr ::= shift_expr shift_op match_expr",
 /*  93 */ "shift_op ::= LSHIFT",
 /*  94 */ "shift_op ::= RSHIFT",
 /*  95 */ "match_expr ::= arith_expr",
 /*  96 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /*  97 */ "arith_expr ::= term",
 /*  98 */ "arith_expr ::= arith_expr arith_op term",
 /*  99 */ "arith_op ::= PLUS",
 /* 100 */ "arith_op ::= MINUS",
 /* 101 */ "term ::= term term_op factor",
 /* 102 */ "term ::= factor",
 /* 103 */ "term_op ::= STAR",
 /* 104 */ "term_op ::= DIV",
 /* 105 */ "term_op ::= DIV_DIV",
 /* 106 */ "factor ::= PLUS factor",
 /* 107 */ "factor ::= MINUS factor",
 /* 108 */ "factor ::= power",
 /* 109 */ "power ::= postfix_expr",
 /* 110 */ "postfix_expr ::= atom",
 /* 111 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 112 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 113 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 114 */ "atom ::= NAME",
 /* 115 */ "atom ::= NUMBER",
 /* 116 */ "atom ::= REGEXP",
 /* 117 */ "atom ::= STRING",
 /* 118 */ "atom ::= SYMBOL",
 /* 119 */ "atom ::= NIL",
 /* 120 */ "atom ::= TRUE",
 /* 121 */ "atom ::= FALSE",
 /* 122 */ "atom ::= LINE",
 /* 123 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 124 */ "atom ::= LPAR expr RPAR",
 /* 125 */ "args_opt ::=",
 /* 126 */ "args_opt ::= args",
 /* 127 */ "blockarg_opt ::=",
 /* 128 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 129 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 130 */ "blockarg_params_opt ::=",
 /* 131 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 132 */ "excepts ::= except",
 /* 133 */ "excepts ::= excepts except",
 /* 134 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 135 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 136 */ "except ::= EXCEPT NEWLINE stmts",
 /* 137 */ "finally_opt ::=",
 /* 138 */ "finally_opt ::= FINALLY stmts",
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
  { 50, 1 },
  { 51, 1 },
  { 51, 3 },
  { 52, 0 },
  { 52, 1 },
  { 52, 1 },
  { 52, 7 },
  { 52, 5 },
  { 52, 5 },
  { 52, 5 },
  { 52, 1 },
  { 52, 2 },
  { 52, 1 },
  { 52, 2 },
  { 52, 1 },
  { 52, 2 },
  { 52, 6 },
  { 52, 6 },
  { 52, 2 },
  { 52, 2 },
  { 60, 1 },
  { 60, 3 },
  { 61, 1 },
  { 61, 3 },
  { 59, 1 },
  { 59, 3 },
  { 58, 0 },
  { 58, 2 },
  { 57, 1 },
  { 57, 5 },
  { 62, 0 },
  { 62, 2 },
  { 53, 7 },
  { 63, 9 },
  { 63, 7 },
  { 63, 7 },
  { 63, 5 },
  { 63, 7 },
  { 63, 5 },
  { 63, 5 },
  { 63, 3 },
  { 63, 7 },
  { 63, 5 },
  { 63, 5 },
  { 63, 3 },
  { 63, 5 },
  { 63, 3 },
  { 63, 3 },
  { 63, 1 },
  { 63, 7 },
  { 63, 5 },
  { 63, 5 },
  { 63, 3 },
  { 63, 5 },
  { 63, 3 },
  { 63, 3 },
  { 63, 1 },
  { 63, 5 },
  { 63, 3 },
  { 63, 3 },
  { 63, 1 },
  { 63, 3 },
  { 63, 1 },
  { 63, 1 },
  { 63, 0 },
  { 68, 2 },
  { 67, 2 },
  { 66, 3 },
  { 69, 0 },
  { 69, 1 },
  { 70, 2 },
  { 64, 1 },
  { 64, 3 },
  { 65, 1 },
  { 65, 3 },
  { 71, 2 },
  { 72, 1 },
  { 72, 3 },
  { 54, 1 },
  { 73, 3 },
  { 73, 1 },
  { 75, 1 },
  { 76, 1 },
  { 77, 1 },
  { 78, 1 },
  { 78, 3 },
  { 80, 1 },
  { 80, 1 },
  { 79, 1 },
  { 81, 1 },
  { 82, 1 },
  { 83, 1 },
  { 83, 3 },
  { 85, 1 },
  { 85, 1 },
  { 84, 1 },
  { 84, 3 },
  { 86, 1 },
  { 86, 3 },
  { 88, 1 },
  { 88, 1 },
  { 87, 3 },
  { 87, 1 },
  { 89, 1 },
  { 89, 1 },
  { 89, 1 },
  { 90, 2 },
  { 90, 2 },
  { 90, 1 },
  { 91, 1 },
  { 74, 1 },
  { 74, 5 },
  { 74, 4 },
  { 74, 3 },
  { 92, 1 },
  { 92, 1 },
  { 92, 1 },
  { 92, 1 },
  { 92, 1 },
  { 92, 1 },
  { 92, 1 },
  { 92, 1 },
  { 92, 1 },
  { 92, 3 },
  { 92, 3 },
  { 93, 0 },
  { 93, 1 },
  { 94, 0 },
  { 94, 5 },
  { 94, 5 },
  { 95, 0 },
  { 95, 3 },
  { 55, 1 },
  { 55, 2 },
  { 96, 6 },
  { 96, 4 },
  { 96, 3 },
  { 56, 0 },
  { 56, 2 },
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
#line 620 "parser.y"
{
    *pval = yymsp[0].minor.yy123;
}
#line 1886 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 132: /* excepts ::= except */
#line 624 "parser.y"
{
    yygotominor.yy123 = make_array_with(env, yymsp[0].minor.yy123);
}
#line 1897 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 627 "parser.y"
{
    yygotominor.yy123 = Array_push(env, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 1907 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 125: /* args_opt ::= */
      case 127: /* blockarg_opt ::= */
      case 130: /* blockarg_params_opt ::= */
      case 137: /* finally_opt ::= */
#line 631 "parser.y"
{
    yygotominor.yy123 = YNIL;
}
#line 1921 "parser.c"
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
      case 88: /* xor_expr ::= or_expr */
      case 89: /* or_expr ::= and_expr */
      case 90: /* and_expr ::= shift_expr */
      case 91: /* shift_expr ::= match_expr */
      case 95: /* match_expr ::= arith_expr */
      case 97: /* arith_expr ::= term */
      case 102: /* term ::= factor */
      case 108: /* factor ::= power */
      case 109: /* power ::= postfix_expr */
      case 110: /* postfix_expr ::= atom */
      case 126: /* args_opt ::= args */
      case 138: /* finally_opt ::= FINALLY stmts */
#line 634 "parser.y"
{
    yygotominor.yy123 = yymsp[0].minor.yy123;
}
#line 1952 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 640 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy123 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy123, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, yymsp[-1].minor.yy123);
}
#line 1960 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 644 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy123 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy123, yymsp[-2].minor.yy123, YNIL, yymsp[-1].minor.yy123);
}
#line 1968 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 648 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy123 = Finally_new(env, lineno, yymsp[-3].minor.yy123, yymsp[-1].minor.yy123);
}
#line 1976 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 652 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy123 = While_new(env, lineno, yymsp[-3].minor.yy123, yymsp[-1].minor.yy123);
}
#line 1984 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 656 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy123 = Break_new(env, lineno, YNIL);
}
#line 1992 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 660 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy123 = Break_new(env, lineno, yymsp[0].minor.yy123);
}
#line 2000 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 664 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy123 = Next_new(env, lineno, YNIL);
}
#line 2008 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 668 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy123 = Next_new(env, lineno, yymsp[0].minor.yy123);
}
#line 2016 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 672 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy123 = Return_new(env, lineno, YNIL);
}
#line 2024 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 676 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy123 = Return_new(env, lineno, yymsp[0].minor.yy123);
}
#line 2032 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 680 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy123 = If_new(env, lineno, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, yymsp[-1].minor.yy123);
}
#line 2040 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 684 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy123 = Klass_new(env, lineno, id, yymsp[-3].minor.yy123, yymsp[-1].minor.yy123);
}
#line 2049 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 689 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy123 = Nonlocal_new(env, lineno, yymsp[0].minor.yy123);
}
#line 2057 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 693 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy123 = Import_new(env, lineno, yymsp[0].minor.yy123);
}
#line 2065 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 705 "parser.y"
{
    yygotominor.yy123 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2073 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 708 "parser.y"
{
    yygotominor.yy123 = Array_push_token_id(env, yymsp[-2].minor.yy123, yymsp[0].minor.yy0);
}
#line 2081 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 729 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy123, yymsp[-1].minor.yy123, yymsp[0].minor.yy123);
    yygotominor.yy123 = make_array_with(env, node);
}
#line 2090 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 742 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy123 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy123, yymsp[-1].minor.yy123);
}
#line 2099 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 748 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-8].minor.yy123, yymsp[-6].minor.yy123, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 2106 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 751 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-6].minor.yy123, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, yymsp[0].minor.yy123, YNIL);
}
#line 2113 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 754 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-6].minor.yy123, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, YNIL, yymsp[0].minor.yy123);
}
#line 2120 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 757 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, yymsp[0].minor.yy123, YNIL, YNIL);
}
#line 2127 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 760 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-6].minor.yy123, yymsp[-4].minor.yy123, YNIL, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 2134 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 763 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, YNIL, yymsp[0].minor.yy123, YNIL);
}
#line 2141 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 766 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, YNIL, YNIL, yymsp[0].minor.yy123);
}
#line 2148 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 769 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-2].minor.yy123, yymsp[0].minor.yy123, YNIL, YNIL, YNIL);
}
#line 2155 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 772 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-6].minor.yy123, YNIL, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 2162 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 775 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-4].minor.yy123, YNIL, yymsp[-2].minor.yy123, yymsp[0].minor.yy123, YNIL);
}
#line 2169 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 778 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-4].minor.yy123, YNIL, yymsp[-2].minor.yy123, YNIL, yymsp[0].minor.yy123);
}
#line 2176 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 781 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-2].minor.yy123, YNIL, yymsp[0].minor.yy123, YNIL, YNIL);
}
#line 2183 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 784 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-4].minor.yy123, YNIL, YNIL, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 2190 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 787 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-2].minor.yy123, YNIL, YNIL, yymsp[0].minor.yy123, YNIL);
}
#line 2197 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 790 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[-2].minor.yy123, YNIL, YNIL, YNIL, yymsp[0].minor.yy123);
}
#line 2204 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 793 "parser.y"
{
    yygotominor.yy123 = Params_new(env, yymsp[0].minor.yy123, YNIL, YNIL, YNIL, YNIL);
}
#line 2211 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 796 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, yymsp[-6].minor.yy123, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 2218 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 799 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, yymsp[0].minor.yy123, YNIL);
}
#line 2225 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 802 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, YNIL, yymsp[0].minor.yy123);
}
#line 2232 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 805 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, yymsp[-2].minor.yy123, yymsp[0].minor.yy123, YNIL, YNIL);
}
#line 2239 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 808 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, yymsp[-4].minor.yy123, YNIL, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 2246 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 811 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, yymsp[-2].minor.yy123, YNIL, yymsp[0].minor.yy123, YNIL);
}
#line 2253 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 814 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, yymsp[-2].minor.yy123, YNIL, YNIL, yymsp[0].minor.yy123);
}
#line 2260 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 817 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, yymsp[0].minor.yy123, YNIL, YNIL, YNIL);
}
#line 2267 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 820 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 2274 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 823 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy123, yymsp[0].minor.yy123, YNIL);
}
#line 2281 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 826 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy123, YNIL, yymsp[0].minor.yy123);
}
#line 2288 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 829 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy123, YNIL, YNIL);
}
#line 2295 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 832 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 2302 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 835 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy123, YNIL);
}
#line 2309 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 838 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy123);
}
#line 2316 "parser.c"
        break;
      case 64: /* params ::= */
#line 841 "parser.y"
{
    yygotominor.yy123 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2323 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 845 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy123 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2332 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 851 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy123 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2341 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 857 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy123 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy123);
}
#line 2350 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 874 "parser.y"
{
    yygotominor.yy123 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy123, lineno, id, YNIL);
}
#line 2360 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 880 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy123, lineno, id, YNIL);
    yygotominor.yy123 = yymsp[-2].minor.yy123;
}
#line 2370 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 894 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy123 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy123);
}
#line 2379 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 911 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy123);
    yygotominor.yy123 = Assign_new(env, lineno, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 2387 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 934 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy123);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy123)->u.id;
    yygotominor.yy123 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy123, id, yymsp[0].minor.yy123);
}
#line 2396 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
      case 87: /* comp_op ::= GREATER */
#line 940 "parser.y"
{
    yygotominor.yy123 = yymsp[0].minor.yy0;
}
#line 2404 "parser.c"
        break;
      case 92: /* shift_expr ::= shift_expr shift_op match_expr */
      case 98: /* arith_expr ::= arith_expr arith_op term */
      case 101: /* term ::= term term_op factor */
#line 962 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy123);
    yygotominor.yy123 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy123, VAL2ID(yymsp[-1].minor.yy123), yymsp[0].minor.yy123);
}
#line 2414 "parser.c"
        break;
      case 93: /* shift_op ::= LSHIFT */
      case 94: /* shift_op ::= RSHIFT */
      case 99: /* arith_op ::= PLUS */
      case 100: /* arith_op ::= MINUS */
      case 103: /* term_op ::= STAR */
      case 104: /* term_op ::= DIV */
      case 105: /* term_op ::= DIV_DIV */
#line 967 "parser.y"
{
    yygotominor.yy123 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2427 "parser.c"
        break;
      case 96: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 977 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy123);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy123 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy123, id, yymsp[0].minor.yy123);
}
#line 2436 "parser.c"
        break;
      case 106: /* factor ::= PLUS factor */
#line 1016 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy123 = FuncCall_new3(env, lineno, yymsp[0].minor.yy123, id);
}
#line 2445 "parser.c"
        break;
      case 107: /* factor ::= MINUS factor */
#line 1021 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy123 = FuncCall_new3(env, lineno, yymsp[0].minor.yy123, id);
}
#line 2454 "parser.c"
        break;
      case 111: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1037 "parser.y"
{
    yygotominor.yy123 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy123), yymsp[-4].minor.yy123, yymsp[-2].minor.yy123, yymsp[0].minor.yy123);
}
#line 2461 "parser.c"
        break;
      case 112: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1040 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy123);
    yygotominor.yy123 = Subscript_new(env, lineno, yymsp[-3].minor.yy123, yymsp[-1].minor.yy123);
}
#line 2469 "parser.c"
        break;
      case 113: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1044 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy123);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy123 = Attr_new(env, lineno, yymsp[-2].minor.yy123, id);
}
#line 2478 "parser.c"
        break;
      case 114: /* atom ::= NAME */
#line 1050 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy123 = Variable_new(env, lineno, id);
}
#line 2487 "parser.c"
        break;
      case 115: /* atom ::= NUMBER */
      case 116: /* atom ::= REGEXP */
      case 117: /* atom ::= STRING */
      case 118: /* atom ::= SYMBOL */
#line 1055 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy123 = Literal_new(env, lineno, val);
}
#line 2499 "parser.c"
        break;
      case 119: /* atom ::= NIL */
#line 1075 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy123 = Literal_new(env, lineno, YNIL);
}
#line 2507 "parser.c"
        break;
      case 120: /* atom ::= TRUE */
#line 1079 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy123 = Literal_new(env, lineno, YTRUE);
}
#line 2515 "parser.c"
        break;
      case 121: /* atom ::= FALSE */
#line 1083 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy123 = Literal_new(env, lineno, YFALSE);
}
#line 2523 "parser.c"
        break;
      case 122: /* atom ::= LINE */
#line 1087 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy123 = Literal_new(env, lineno, val);
}
#line 2532 "parser.c"
        break;
      case 123: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1092 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy123 = Array_new(env, lineno, yymsp[-1].minor.yy123);
}
#line 2540 "parser.c"
        break;
      case 124: /* atom ::= LPAR expr RPAR */
      case 131: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1096 "parser.y"
{
    yygotominor.yy123 = yymsp[-1].minor.yy123;
}
#line 2548 "parser.c"
        break;
      case 128: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 129: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1110 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy123 = BlockArg_new(env, lineno, yymsp[-3].minor.yy123, yymsp[-1].minor.yy123);
}
#line 2557 "parser.c"
        break;
      case 133: /* excepts ::= excepts except */
#line 1129 "parser.y"
{
    yygotominor.yy123 = Array_push(env, yymsp[-1].minor.yy123, yymsp[0].minor.yy123);
}
#line 2564 "parser.c"
        break;
      case 134: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1133 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy123 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy123, id, yymsp[0].minor.yy123);
}
#line 2574 "parser.c"
        break;
      case 135: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1139 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy123 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy123, NO_EXC_VAR, yymsp[0].minor.yy123);
}
#line 2582 "parser.c"
        break;
      case 136: /* except ::= EXCEPT NEWLINE stmts */
#line 1143 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy123 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy123);
}
#line 2590 "parser.c"
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
*/
static BOOL Parse(
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
      RETURN(env, FALSE);
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
      RETURN(env, FALSE);
    }
  }while( yymajor!=YYNOCODE && PTR_AS(yyParser, parser)->yyidx>=0 );

  RETURN(env, TRUE);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
