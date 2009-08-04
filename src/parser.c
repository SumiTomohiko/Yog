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

#define TOKEN(token)            PTR_AS(YogToken, (token))
#define TOKEN_ID(token)         TOKEN((token))->u.id
#define TOKEN_LINENO(token)     TOKEN((token))->lineno
#define NODE_LINENO(node)       PTR_AS(YogNode, (node))->lineno
#line 626 "parser.c"
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
#define YYNOCODE 99
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy163;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 233
#define YYNRULE 140
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
 /*     0 */     1,  119,  120,  194,   20,   21,   24,   25,   26,  104,
 /*    10 */   172,   65,   53,  116,  123,  125,  210,  112,   23,  211,
 /*    20 */   374,   81,  146,  144,  145,  228,  117,  121,  201,   41,
 /*    30 */    42,  205,  137,   18,  227,  173,  174,  175,  176,  177,
 /*    40 */   178,  179,  180,  150,   79,  163,  152,  153,  154,   58,
 /*    50 */   172,   92,  164,   59,   95,  136,   61,   56,   23,   17,
 /*    60 */   170,  160,  171,   45,  146,  144,  145,   16,   28,   41,
 /*    70 */    42,   15,  230,   18,   30,  173,  174,  175,  176,  177,
 /*    80 */   178,  179,  180,  118,  198,  150,   79,  163,  152,  153,
 /*    90 */   154,   58,  101,   92,  164,   59,   95,  188,   61,   56,
 /*   100 */    80,   46,  170,  160,  171,   57,  146,  144,  145,   77,
 /*   110 */    93,   40,   61,   56,   19,  167,  170,  160,  171,  119,
 /*   120 */   120,  122,    2,   32,    3,  168,  169,  150,   79,  163,
 /*   130 */   152,  153,  154,   58,  189,   92,  164,   59,   95,  188,
 /*   140 */    61,   56,  232,   89,  170,  160,  171,   90,  146,  144,
 /*   150 */   145,  114,  115,  126,  130,  132,  219,  124,  208,  211,
 /*   160 */   133,  115,  126,  130,  132,  219,   34,   19,  211,  150,
 /*   170 */    79,  163,  152,  153,  154,   58,  183,   92,  164,   59,
 /*   180 */    95,   80,   61,   56,   80,   36,  170,  160,  171,   82,
 /*   190 */   146,  144,  145,   60,   56,   72,   38,  170,  160,  171,
 /*   200 */   157,  160,  171,  128,  213,  119,  120,  122,  131,  217,
 /*   210 */    99,  150,   79,  163,  152,  153,  154,   58,   29,   92,
 /*   220 */   164,   59,   95,   80,   61,   56,   80,  105,  170,  160,
 /*   230 */   171,   83,  146,  144,  145,   69,   55,  108,  111,  170,
 /*   240 */   160,  171,  158,  160,  171,  119,  120,  122,  202,  203,
 /*   250 */   225,  226,  192,  150,   79,  163,  152,  153,  154,   58,
 /*   260 */   106,   92,  164,   59,   95,   80,   61,   56,  165,  166,
 /*   270 */   170,  160,  171,   47,  146,  144,  145,   16,   13,    8,
 /*   280 */   109,  159,  160,  171,  127,  129,  215,   16,  119,  205,
 /*   290 */   206,    3,   63,   78,   27,  150,   79,  163,  152,  153,
 /*   300 */   154,   58,  196,   92,  164,   59,   95,   16,   61,   56,
 /*   310 */   233,   16,  170,  160,  171,   48,  146,  144,  145,   16,
 /*   320 */    16,   16,  148,  184,  190,  136,  138,   16,   16,   17,
 /*   330 */   195,  231,  134,  200,   31,  147,  207,  150,   79,  163,
 /*   340 */   152,  153,  154,   58,   30,   92,  164,   59,   95,  209,
 /*   350 */    61,   56,  212,  222,  170,  160,  171,  103,  146,  144,
 /*   360 */   145,  214,  216,  218,   16,    4,   33,   37,   22,  181,
 /*   370 */   182,   62,    5,    6,  187,    7,    9,   64,  191,  150,
 /*   380 */    79,  163,  152,  153,  154,   58,  229,   92,  164,   59,
 /*   390 */    95,  107,   61,   56,   66,  193,  170,  160,  171,   84,
 /*   400 */   146,  144,  145,  113,  110,   35,  375,   10,   39,   43,
 /*   410 */    49,   67,  197,  199,   68,   54,  221,   50,   11,   70,
 /*   420 */    71,  150,   79,  163,  152,  153,  154,   58,   44,   92,
 /*   430 */   164,   59,   95,   51,   61,   56,   73,   74,  170,  160,
 /*   440 */   171,   85,  146,  144,  145,   52,  223,   75,   76,  224,
 /*   450 */   139,   12,  375,  375,  375,  375,  375,  375,  375,  375,
 /*   460 */   375,  375,  375,  150,   79,  163,  152,  153,  154,   58,
 /*   470 */   375,   92,  164,   59,   95,  375,   61,   56,  375,  375,
 /*   480 */   170,  160,  171,   86,  146,  144,  145,  375,  375,  375,
 /*   490 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*   500 */   375,  375,  375,  375,  375,  150,   79,  163,  152,  153,
 /*   510 */   154,   58,  375,   92,  164,   59,   95,  375,   61,   56,
 /*   520 */   375,  375,  170,  160,  171,  140,  146,  144,  145,  375,
 /*   530 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*   540 */   375,  375,  375,  375,  375,  375,  375,  150,   79,  163,
 /*   550 */   152,  153,  154,   58,  375,   92,  164,   59,   95,  375,
 /*   560 */    61,   56,  375,  375,  170,  160,  171,  141,  146,  144,
 /*   570 */   145,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*   580 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  150,
 /*   590 */    79,  163,  152,  153,  154,   58,  375,   92,  164,   59,
 /*   600 */    95,  375,   61,   56,  375,  375,  170,  160,  171,  142,
 /*   610 */   146,  144,  145,  375,  375,  375,  375,  375,  375,  375,
 /*   620 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*   630 */   375,  150,   79,  163,  152,  153,  154,   58,  375,   92,
 /*   640 */   164,   59,   95,  375,   61,   56,  375,  375,  170,  160,
 /*   650 */   171,   88,  146,  144,  145,  375,  375,  375,  375,  375,
 /*   660 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*   670 */   375,  375,  375,  150,   79,  163,  152,  153,  154,   58,
 /*   680 */   375,   92,  164,   59,   95,  375,   61,   56,   80,  375,
 /*   690 */   170,  160,  171,  143,  144,  145,  156,   59,   95,  375,
 /*   700 */    61,   56,  375,  375,  170,  160,  171,  375,  375,  375,
 /*   710 */   375,  375,  375,  375,  150,   79,  163,  152,  153,  154,
 /*   720 */    58,  375,   92,  164,   59,   95,  375,   61,   56,  161,
 /*   730 */   375,  170,  160,  171,  375,  375,  375,  375,  375,  375,
 /*   740 */   375,  375,  375,  375,  375,  375,  375,   94,  150,   79,
 /*   750 */   163,  152,  153,  154,   58,  375,   92,  164,   59,   95,
 /*   760 */   375,   61,   56,  375,  375,  170,  160,  171,   98,   80,
 /*   770 */   161,  375,  375,  375,  155,  375,   92,  164,   59,   95,
 /*   780 */   375,   61,   56,  375,  375,  170,  160,  171,   94,  150,
 /*   790 */    79,  163,  152,  153,  154,   58,  375,   92,  164,   59,
 /*   800 */    95,  375,   61,   56,   87,  375,  170,  160,  171,   96,
 /*   810 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*   820 */   375,  375,  375,  150,   79,  163,  152,  153,  154,   58,
 /*   830 */   375,   92,  164,   59,   95,  375,   61,   56,  375,  375,
 /*   840 */   170,  160,  171,  375,   91,  375,  375,  375,  375,  375,
 /*   850 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*   860 */   375,  375,  375,  150,   79,  163,  152,  153,  154,   58,
 /*   870 */   375,   92,  164,   59,   95,  375,   61,   56,  375,  149,
 /*   880 */   170,  160,  171,  375,  375,  375,  375,  375,  375,  375,
 /*   890 */   375,  375,  375,  375,  375,  375,  375,  375,  150,   79,
 /*   900 */   163,  152,  153,  154,   58,  375,   92,  164,   59,   95,
 /*   910 */   375,   61,   56,  375,  375,  170,  160,  171,  375,  162,
 /*   920 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*   930 */   375,  375,  375,  375,  375,  375,  375,  375,  150,   79,
 /*   940 */   163,  152,  153,  154,   58,  375,   92,  164,   59,   95,
 /*   950 */   375,   61,   56,  375,   97,  170,  160,  171,  375,  375,
 /*   960 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*   970 */   375,  375,  375,  150,   79,  163,  152,  153,  154,   58,
 /*   980 */   375,   92,  164,   59,   95,  375,   61,   56,  375,  375,
 /*   990 */   170,  160,  171,  375,  185,  375,  375,  375,  375,  375,
 /*  1000 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*  1010 */   375,  375,  375,  150,   79,  163,  152,  153,  154,   58,
 /*  1020 */   375,   92,  164,   59,   95,  375,   61,   56,  375,  186,
 /*  1030 */   170,  160,  171,  375,  375,  375,  375,  375,  375,  375,
 /*  1040 */   375,  375,  375,  375,  375,  375,  375,  375,  150,   79,
 /*  1050 */   163,  152,  153,  154,   58,  375,   92,  164,   59,   95,
 /*  1060 */   375,   61,   56,  375,  375,  170,  160,  171,  375,  100,
 /*  1070 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*  1080 */   375,  375,  375,  375,  375,  375,  375,  375,  150,   79,
 /*  1090 */   163,  152,  153,  154,   58,  375,   92,  164,   59,   95,
 /*  1100 */   375,   61,   56,  375,  102,  170,  160,  171,  375,  375,
 /*  1110 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*  1120 */   375,  375,  375,  150,   79,  163,  152,  153,  154,   58,
 /*  1130 */   375,   92,  164,   59,   95,  375,   61,   56,  375,  375,
 /*  1140 */   170,  160,  171,  375,  204,  375,  375,  375,  375,  375,
 /*  1150 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*  1160 */   375,  375,  375,  150,   79,  163,  152,  153,  154,   58,
 /*  1170 */   375,   92,  164,   59,   95,  375,   61,   56,  375,  220,
 /*  1180 */   170,  160,  171,  375,  375,  375,  375,  375,  375,  375,
 /*  1190 */   375,  375,  375,  375,  375,  375,  375,  375,  150,   79,
 /*  1200 */   163,  152,  153,  154,   58,  375,   92,  164,   59,   95,
 /*  1210 */   375,   61,   56,  375,  375,  170,  160,  171,  375,  135,
 /*  1220 */   375,  375,  375,  375,  375,  375,  375,  375,  375,  375,
 /*  1230 */   375,  375,  375,  375,  375,  375,  375,  375,  150,   79,
 /*  1240 */   163,  152,  153,  154,   58,  375,   92,  164,   59,   95,
 /*  1250 */   375,   61,   56,   14,  375,  170,  160,  171,   80,  151,
 /*  1260 */   152,  153,  154,   58,  172,   92,  164,   59,   95,  375,
 /*  1270 */    61,   56,   23,  375,  170,  160,  171,  375,  375,  375,
 /*  1280 */   375,  375,  375,   41,   42,  375,  375,   18,  375,  173,
 /*  1290 */   174,  175,  176,  177,  178,  179,  180,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   12,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   66,   67,   68,   69,   19,   20,   72,
 /*    20 */    51,   52,   53,   54,   55,   17,   67,   68,   69,   31,
 /*    30 */    32,   72,   57,   35,   26,   37,   38,   39,   40,   41,
 /*    40 */    42,   43,   44,   74,   75,   76,   77,   78,   79,   80,
 /*    50 */    12,   82,   83,   84,   85,   16,   87,   88,   20,   20,
 /*    60 */    91,   92,   93,   52,   53,   54,   55,    1,   25,   31,
 /*    70 */    32,    5,   97,   35,   35,   37,   38,   39,   40,   41,
 /*    80 */    42,   43,   44,   68,   69,   74,   75,   76,   77,   78,
 /*    90 */    79,   80,   58,   82,   83,   84,   85,   63,   87,   88,
 /*   100 */    75,   56,   91,   92,   93,   52,   53,   54,   55,   12,
 /*   110 */    85,   90,   87,   88,   48,   23,   91,   92,   93,   22,
 /*   120 */    23,   24,    3,   81,    5,   33,   34,   74,   75,   76,
 /*   130 */    77,   78,   79,   80,   58,   82,   83,   84,   85,   63,
 /*   140 */    87,   88,   97,   57,   91,   92,   93,   52,   53,   54,
 /*   150 */    55,   64,   65,   66,   67,   68,   69,   68,   69,   72,
 /*   160 */    64,   65,   66,   67,   68,   69,   86,   48,   72,   74,
 /*   170 */    75,   76,   77,   78,   79,   80,   95,   82,   83,   84,
 /*   180 */    85,   75,   87,   88,   75,   35,   91,   92,   93,   52,
 /*   190 */    53,   54,   55,   87,   88,   12,   89,   91,   92,   93,
 /*   200 */    91,   92,   93,   68,   69,   22,   23,   24,   68,   69,
 /*   210 */    96,   74,   75,   76,   77,   78,   79,   80,   17,   82,
 /*   220 */    83,   84,   85,   75,   87,   88,   75,   59,   91,   92,
 /*   230 */    93,   52,   53,   54,   55,   12,   88,   61,   62,   91,
 /*   240 */    92,   93,   91,   92,   93,   22,   23,   24,   70,   71,
 /*   250 */    28,   29,   12,   74,   75,   76,   77,   78,   79,   80,
 /*   260 */    60,   82,   83,   84,   85,   75,   87,   88,   31,   32,
 /*   270 */    91,   92,   93,   52,   53,   54,   55,    1,    1,    3,
 /*   280 */    62,   91,   92,   93,   67,   68,   69,    1,   22,   72,
 /*   290 */    71,    5,   45,   46,   18,   74,   75,   76,   77,   78,
 /*   300 */    79,   80,   69,   82,   83,   84,   85,    1,   87,   88,
 /*   310 */     0,    1,   91,   92,   93,   52,   53,   54,   55,    1,
 /*   320 */     1,    1,    4,    4,    4,   16,   49,    1,    1,   20,
 /*   330 */     4,    4,   96,   69,   25,    4,   69,   74,   75,   76,
 /*   340 */    77,   78,   79,   80,   35,   82,   83,   84,   85,   69,
 /*   350 */    87,   88,   69,   47,   91,   92,   93,   52,   53,   54,
 /*   360 */    55,   69,   69,   69,    1,    1,   27,   30,   15,   36,
 /*   370 */    21,   21,    1,    1,    4,    1,    1,   12,   12,   74,
 /*   380 */    75,   76,   77,   78,   79,   80,    4,   82,   83,   84,
 /*   390 */    85,   15,   87,   88,   15,   12,   91,   92,   93,   52,
 /*   400 */    53,   54,   55,   12,   16,   20,   98,   21,   15,   15,
 /*   410 */    15,   15,   12,   12,   15,   12,   36,   15,    1,   15,
 /*   420 */    15,   74,   75,   76,   77,   78,   79,   80,   15,   82,
 /*   430 */    83,   84,   85,   15,   87,   88,   15,   15,   91,   92,
 /*   440 */    93,   52,   53,   54,   55,   15,   36,   15,   15,   12,
 /*   450 */    12,    1,   98,   98,   98,   98,   98,   98,   98,   98,
 /*   460 */    98,   98,   98,   74,   75,   76,   77,   78,   79,   80,
 /*   470 */    98,   82,   83,   84,   85,   98,   87,   88,   98,   98,
 /*   480 */    91,   92,   93,   52,   53,   54,   55,   98,   98,   98,
 /*   490 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*   500 */    98,   98,   98,   98,   98,   74,   75,   76,   77,   78,
 /*   510 */    79,   80,   98,   82,   83,   84,   85,   98,   87,   88,
 /*   520 */    98,   98,   91,   92,   93,   52,   53,   54,   55,   98,
 /*   530 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*   540 */    98,   98,   98,   98,   98,   98,   98,   74,   75,   76,
 /*   550 */    77,   78,   79,   80,   98,   82,   83,   84,   85,   98,
 /*   560 */    87,   88,   98,   98,   91,   92,   93,   52,   53,   54,
 /*   570 */    55,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*   580 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   74,
 /*   590 */    75,   76,   77,   78,   79,   80,   98,   82,   83,   84,
 /*   600 */    85,   98,   87,   88,   98,   98,   91,   92,   93,   52,
 /*   610 */    53,   54,   55,   98,   98,   98,   98,   98,   98,   98,
 /*   620 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*   630 */    98,   74,   75,   76,   77,   78,   79,   80,   98,   82,
 /*   640 */    83,   84,   85,   98,   87,   88,   98,   98,   91,   92,
 /*   650 */    93,   52,   53,   54,   55,   98,   98,   98,   98,   98,
 /*   660 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*   670 */    98,   98,   98,   74,   75,   76,   77,   78,   79,   80,
 /*   680 */    98,   82,   83,   84,   85,   98,   87,   88,   75,   98,
 /*   690 */    91,   92,   93,   53,   54,   55,   83,   84,   85,   98,
 /*   700 */    87,   88,   98,   98,   91,   92,   93,   98,   98,   98,
 /*   710 */    98,   98,   98,   98,   74,   75,   76,   77,   78,   79,
 /*   720 */    80,   98,   82,   83,   84,   85,   98,   87,   88,   55,
 /*   730 */    98,   91,   92,   93,   98,   98,   98,   98,   98,   98,
 /*   740 */    98,   98,   98,   98,   98,   98,   98,   73,   74,   75,
 /*   750 */    76,   77,   78,   79,   80,   98,   82,   83,   84,   85,
 /*   760 */    98,   87,   88,   98,   98,   91,   92,   93,   94,   75,
 /*   770 */    55,   98,   98,   98,   80,   98,   82,   83,   84,   85,
 /*   780 */    98,   87,   88,   98,   98,   91,   92,   93,   73,   74,
 /*   790 */    75,   76,   77,   78,   79,   80,   98,   82,   83,   84,
 /*   800 */    85,   98,   87,   88,   55,   98,   91,   92,   93,   94,
 /*   810 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*   820 */    98,   98,   98,   74,   75,   76,   77,   78,   79,   80,
 /*   830 */    98,   82,   83,   84,   85,   98,   87,   88,   98,   98,
 /*   840 */    91,   92,   93,   98,   55,   98,   98,   98,   98,   98,
 /*   850 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*   860 */    98,   98,   98,   74,   75,   76,   77,   78,   79,   80,
 /*   870 */    98,   82,   83,   84,   85,   98,   87,   88,   98,   55,
 /*   880 */    91,   92,   93,   98,   98,   98,   98,   98,   98,   98,
 /*   890 */    98,   98,   98,   98,   98,   98,   98,   98,   74,   75,
 /*   900 */    76,   77,   78,   79,   80,   98,   82,   83,   84,   85,
 /*   910 */    98,   87,   88,   98,   98,   91,   92,   93,   98,   55,
 /*   920 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*   930 */    98,   98,   98,   98,   98,   98,   98,   98,   74,   75,
 /*   940 */    76,   77,   78,   79,   80,   98,   82,   83,   84,   85,
 /*   950 */    98,   87,   88,   98,   55,   91,   92,   93,   98,   98,
 /*   960 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*   970 */    98,   98,   98,   74,   75,   76,   77,   78,   79,   80,
 /*   980 */    98,   82,   83,   84,   85,   98,   87,   88,   98,   98,
 /*   990 */    91,   92,   93,   98,   55,   98,   98,   98,   98,   98,
 /*  1000 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*  1010 */    98,   98,   98,   74,   75,   76,   77,   78,   79,   80,
 /*  1020 */    98,   82,   83,   84,   85,   98,   87,   88,   98,   55,
 /*  1030 */    91,   92,   93,   98,   98,   98,   98,   98,   98,   98,
 /*  1040 */    98,   98,   98,   98,   98,   98,   98,   98,   74,   75,
 /*  1050 */    76,   77,   78,   79,   80,   98,   82,   83,   84,   85,
 /*  1060 */    98,   87,   88,   98,   98,   91,   92,   93,   98,   55,
 /*  1070 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*  1080 */    98,   98,   98,   98,   98,   98,   98,   98,   74,   75,
 /*  1090 */    76,   77,   78,   79,   80,   98,   82,   83,   84,   85,
 /*  1100 */    98,   87,   88,   98,   55,   91,   92,   93,   98,   98,
 /*  1110 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*  1120 */    98,   98,   98,   74,   75,   76,   77,   78,   79,   80,
 /*  1130 */    98,   82,   83,   84,   85,   98,   87,   88,   98,   98,
 /*  1140 */    91,   92,   93,   98,   55,   98,   98,   98,   98,   98,
 /*  1150 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*  1160 */    98,   98,   98,   74,   75,   76,   77,   78,   79,   80,
 /*  1170 */    98,   82,   83,   84,   85,   98,   87,   88,   98,   55,
 /*  1180 */    91,   92,   93,   98,   98,   98,   98,   98,   98,   98,
 /*  1190 */    98,   98,   98,   98,   98,   98,   98,   98,   74,   75,
 /*  1200 */    76,   77,   78,   79,   80,   98,   82,   83,   84,   85,
 /*  1210 */    98,   87,   88,   98,   98,   91,   92,   93,   98,   55,
 /*  1220 */    98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
 /*  1230 */    98,   98,   98,   98,   98,   98,   98,   98,   74,   75,
 /*  1240 */    76,   77,   78,   79,   80,   98,   82,   83,   84,   85,
 /*  1250 */    98,   87,   88,    1,   98,   91,   92,   93,   75,   76,
 /*  1260 */    77,   78,   79,   80,   12,   82,   83,   84,   85,   98,
 /*  1270 */    87,   88,   20,   98,   91,   92,   93,   98,   98,   98,
 /*  1280 */    98,   98,   98,   31,   32,   98,   98,   35,   98,   37,
 /*  1290 */    38,   39,   40,   41,   42,   43,   44,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 142
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   38,   38, 1252,
 /*    20 */    38,   38,   38,   38,   38,   38,   38,   38,   38,   38,
 /*    30 */    38,   38,   38,   38,   38,   97,   97,   38,   38,  183,
 /*    40 */    38,   38,   38,  223,  223,   66,  119,  276,  276,  -21,
 /*    50 */   -21,  -21,  -21,   -9,   43,   92,   92,  286,    8,  222,
 /*    60 */   237,  237,  247,  150,  201,  240,   -9,  266,  266,   43,
 /*    70 */   266,  266,   43,  266,  266,  266,  266,   43,  150,  309,
 /*    80 */    39,  310,  318,  319,  320,  326,  306,  277,  327,  331,
 /*    90 */   363,  364,  339,  337,  353,  337,  333,  349,  350,  371,
 /*   100 */   372,  370,  374,  363,  365,  375,  376,  366,  379,  388,
 /*   110 */   383,  388,  391,  385,  386,  393,  394,  395,  396,  400,
 /*   120 */   401,  399,  403,  402,  404,  405,  413,  418,  421,  422,
 /*   130 */   430,  432,  433,  380,  417,  410,  437,  382,  438,  450,
 /*   140 */   363,  363,  363,
};
#define YY_REDUCE_USE_DFLT (-54)
#define YY_REDUCE_MAX 78
static const short yy_reduce_ofst[] = {
 /*     0 */   -31,   11,   53,   95,  137,  179,  221,  263,  305,  347,
 /*    10 */   389,  431,  473,  515,  557,  599,  640,  674,  715,  749,
 /*    20 */   789,  824,  864,  899,  939,  974, 1014, 1049, 1089, 1124,
 /*    30 */  1164, 1183,  694,  613,   25,   87,   96,  106,  148,  -53,
 /*    40 */   109,  151,  190,  -41,  217,   45,  -25,   34,   76,   15,
 /*    50 */    89,  135,  140,  176,  178,   21,   21,   86,   42,   80,
 /*    60 */   107,  107,   81,  114,  168,  200,  218,  233,  264,  219,
 /*    70 */   267,  280,  219,  283,  292,  293,  294,  219,  236,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   236,  236,  236,  236,  236,  236,  236,  236,  236,  236,
 /*    10 */   236,  236,  236,  236,  236,  236,  236,  359,  359,  373,
 /*    20 */   373,  243,  373,  373,  245,  247,  373,  373,  373,  373,
 /*    30 */   373,  373,  373,  373,  373,  297,  297,  373,  373,  373,
 /*    40 */   373,  373,  373,  373,  373,  373,  371,  263,  263,  373,
 /*    50 */   373,  373,  373,  373,  301,  332,  331,  371,  317,  324,
 /*    60 */   330,  329,  361,  364,  259,  373,  373,  373,  373,  373,
 /*    70 */   373,  373,  305,  373,  373,  373,  373,  304,  364,  343,
 /*    80 */   343,  373,  373,  373,  373,  373,  373,  373,  373,  373,
 /*    90 */   372,  373,  321,  326,  360,  325,  373,  373,  373,  373,
 /*   100 */   373,  373,  373,  264,  373,  373,  251,  373,  252,  254,
 /*   110 */   373,  253,  373,  373,  373,  281,  273,  269,  267,  373,
 /*   120 */   373,  271,  373,  277,  275,  279,  289,  285,  283,  287,
 /*   130 */   293,  291,  295,  373,  373,  373,  373,  373,  373,  373,
 /*   140 */   368,  369,  370,  235,  237,  238,  234,  239,  242,  244,
 /*   150 */   311,  312,  314,  315,  316,  318,  323,  335,  340,  341,
 /*   160 */   342,  309,  310,  313,  322,  333,  334,  337,  338,  339,
 /*   170 */   336,  344,  348,  349,  350,  351,  352,  353,  354,  355,
 /*   180 */   356,  357,  358,  345,  362,  246,  248,  249,  261,  262,
 /*   190 */   250,  258,  257,  256,  255,  265,  266,  298,  268,  299,
 /*   200 */   270,  272,  300,  302,  303,  307,  308,  274,  276,  278,
 /*   210 */   280,  306,  282,  284,  286,  288,  290,  292,  294,  296,
 /*   220 */   260,  365,  363,  346,  347,  327,  328,  319,  320,  240,
 /*   230 */   367,  241,  366,
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
  "AMPER",         "EQUAL",         "LESS",          "BAR",         
  "LSHIFT",        "RSHIFT",        "EQUAL_TILDA",   "PLUS",        
  "MINUS",         "DIV",           "DIV_DIV",       "LBRACKET",    
  "RBRACKET",      "NUMBER",        "REGEXP",        "STRING",      
  "SYMBOL",        "NIL",           "TRUE",          "FALSE",       
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
  "shift_expr",    "match_expr",    "shift_op",      "arith_expr",  
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
 /*  87 */ "comp_op ::= GREATER",
 /*  88 */ "xor_expr ::= or_expr",
 /*  89 */ "or_expr ::= and_expr",
 /*  90 */ "or_expr ::= or_expr BAR and_expr",
 /*  91 */ "and_expr ::= shift_expr",
 /*  92 */ "shift_expr ::= match_expr",
 /*  93 */ "shift_expr ::= shift_expr shift_op match_expr",
 /*  94 */ "shift_op ::= LSHIFT",
 /*  95 */ "shift_op ::= RSHIFT",
 /*  96 */ "match_expr ::= arith_expr",
 /*  97 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /*  98 */ "arith_expr ::= term",
 /*  99 */ "arith_expr ::= arith_expr arith_op term",
 /* 100 */ "arith_op ::= PLUS",
 /* 101 */ "arith_op ::= MINUS",
 /* 102 */ "term ::= term term_op factor",
 /* 103 */ "term ::= factor",
 /* 104 */ "term_op ::= STAR",
 /* 105 */ "term_op ::= DIV",
 /* 106 */ "term_op ::= DIV_DIV",
 /* 107 */ "factor ::= PLUS factor",
 /* 108 */ "factor ::= MINUS factor",
 /* 109 */ "factor ::= power",
 /* 110 */ "power ::= postfix_expr",
 /* 111 */ "postfix_expr ::= atom",
 /* 112 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 113 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 114 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 115 */ "atom ::= NAME",
 /* 116 */ "atom ::= NUMBER",
 /* 117 */ "atom ::= REGEXP",
 /* 118 */ "atom ::= STRING",
 /* 119 */ "atom ::= SYMBOL",
 /* 120 */ "atom ::= NIL",
 /* 121 */ "atom ::= TRUE",
 /* 122 */ "atom ::= FALSE",
 /* 123 */ "atom ::= LINE",
 /* 124 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 125 */ "atom ::= LPAR expr RPAR",
 /* 126 */ "args_opt ::=",
 /* 127 */ "args_opt ::= args",
 /* 128 */ "blockarg_opt ::=",
 /* 129 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 130 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 131 */ "blockarg_params_opt ::=",
 /* 132 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 133 */ "excepts ::= except",
 /* 134 */ "excepts ::= excepts except",
 /* 135 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 136 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 137 */ "except ::= EXCEPT NEWLINE stmts",
 /* 138 */ "finally_opt ::=",
 /* 139 */ "finally_opt ::= FINALLY stmts",
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
  { 51, 1 },
  { 52, 1 },
  { 52, 3 },
  { 53, 0 },
  { 53, 1 },
  { 53, 1 },
  { 53, 7 },
  { 53, 5 },
  { 53, 5 },
  { 53, 5 },
  { 53, 1 },
  { 53, 2 },
  { 53, 1 },
  { 53, 2 },
  { 53, 1 },
  { 53, 2 },
  { 53, 6 },
  { 53, 6 },
  { 53, 2 },
  { 53, 2 },
  { 61, 1 },
  { 61, 3 },
  { 62, 1 },
  { 62, 3 },
  { 60, 1 },
  { 60, 3 },
  { 59, 0 },
  { 59, 2 },
  { 58, 1 },
  { 58, 5 },
  { 63, 0 },
  { 63, 2 },
  { 54, 7 },
  { 64, 9 },
  { 64, 7 },
  { 64, 7 },
  { 64, 5 },
  { 64, 7 },
  { 64, 5 },
  { 64, 5 },
  { 64, 3 },
  { 64, 7 },
  { 64, 5 },
  { 64, 5 },
  { 64, 3 },
  { 64, 5 },
  { 64, 3 },
  { 64, 3 },
  { 64, 1 },
  { 64, 7 },
  { 64, 5 },
  { 64, 5 },
  { 64, 3 },
  { 64, 5 },
  { 64, 3 },
  { 64, 3 },
  { 64, 1 },
  { 64, 5 },
  { 64, 3 },
  { 64, 3 },
  { 64, 1 },
  { 64, 3 },
  { 64, 1 },
  { 64, 1 },
  { 64, 0 },
  { 69, 2 },
  { 68, 2 },
  { 67, 3 },
  { 70, 0 },
  { 70, 1 },
  { 71, 2 },
  { 65, 1 },
  { 65, 3 },
  { 66, 1 },
  { 66, 3 },
  { 72, 2 },
  { 73, 1 },
  { 73, 3 },
  { 55, 1 },
  { 74, 3 },
  { 74, 1 },
  { 76, 1 },
  { 77, 1 },
  { 78, 1 },
  { 79, 1 },
  { 79, 3 },
  { 81, 1 },
  { 81, 1 },
  { 80, 1 },
  { 82, 1 },
  { 82, 3 },
  { 83, 1 },
  { 84, 1 },
  { 84, 3 },
  { 86, 1 },
  { 86, 1 },
  { 85, 1 },
  { 85, 3 },
  { 87, 1 },
  { 87, 3 },
  { 89, 1 },
  { 89, 1 },
  { 88, 3 },
  { 88, 1 },
  { 90, 1 },
  { 90, 1 },
  { 90, 1 },
  { 91, 2 },
  { 91, 2 },
  { 91, 1 },
  { 92, 1 },
  { 75, 1 },
  { 75, 5 },
  { 75, 4 },
  { 75, 3 },
  { 93, 1 },
  { 93, 1 },
  { 93, 1 },
  { 93, 1 },
  { 93, 1 },
  { 93, 1 },
  { 93, 1 },
  { 93, 1 },
  { 93, 1 },
  { 93, 3 },
  { 93, 3 },
  { 94, 0 },
  { 94, 1 },
  { 95, 0 },
  { 95, 5 },
  { 95, 5 },
  { 96, 0 },
  { 96, 3 },
  { 56, 1 },
  { 56, 2 },
  { 97, 6 },
  { 97, 4 },
  { 97, 3 },
  { 57, 0 },
  { 57, 2 },
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
#line 622 "parser.y"
{
    *pval = yymsp[0].minor.yy163;
}
#line 1892 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 133: /* excepts ::= except */
#line 626 "parser.y"
{
    yygotominor.yy163 = make_array_with(env, yymsp[0].minor.yy163);
}
#line 1903 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 629 "parser.y"
{
    yygotominor.yy163 = Array_push(env, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 1913 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 126: /* args_opt ::= */
      case 128: /* blockarg_opt ::= */
      case 131: /* blockarg_params_opt ::= */
      case 138: /* finally_opt ::= */
#line 633 "parser.y"
{
    yygotominor.yy163 = YNIL;
}
#line 1927 "parser.c"
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
      case 91: /* and_expr ::= shift_expr */
      case 92: /* shift_expr ::= match_expr */
      case 96: /* match_expr ::= arith_expr */
      case 98: /* arith_expr ::= term */
      case 103: /* term ::= factor */
      case 109: /* factor ::= power */
      case 110: /* power ::= postfix_expr */
      case 111: /* postfix_expr ::= atom */
      case 127: /* args_opt ::= args */
      case 139: /* finally_opt ::= FINALLY stmts */
#line 636 "parser.y"
{
    yygotominor.yy163 = yymsp[0].minor.yy163;
}
#line 1958 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 642 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy163 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy163, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, yymsp[-1].minor.yy163);
}
#line 1966 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 646 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy163 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy163, yymsp[-2].minor.yy163, YNIL, yymsp[-1].minor.yy163);
}
#line 1974 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 650 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy163 = Finally_new(env, lineno, yymsp[-3].minor.yy163, yymsp[-1].minor.yy163);
}
#line 1982 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 654 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy163 = While_new(env, lineno, yymsp[-3].minor.yy163, yymsp[-1].minor.yy163);
}
#line 1990 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 658 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy163 = Break_new(env, lineno, YNIL);
}
#line 1998 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 662 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy163 = Break_new(env, lineno, yymsp[0].minor.yy163);
}
#line 2006 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 666 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy163 = Next_new(env, lineno, YNIL);
}
#line 2014 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 670 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy163 = Next_new(env, lineno, yymsp[0].minor.yy163);
}
#line 2022 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 674 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy163 = Return_new(env, lineno, YNIL);
}
#line 2030 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 678 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy163 = Return_new(env, lineno, yymsp[0].minor.yy163);
}
#line 2038 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 682 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy163 = If_new(env, lineno, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, yymsp[-1].minor.yy163);
}
#line 2046 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 686 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy163 = Klass_new(env, lineno, id, yymsp[-3].minor.yy163, yymsp[-1].minor.yy163);
}
#line 2055 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 691 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy163 = Nonlocal_new(env, lineno, yymsp[0].minor.yy163);
}
#line 2063 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 695 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy163 = Import_new(env, lineno, yymsp[0].minor.yy163);
}
#line 2071 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 707 "parser.y"
{
    yygotominor.yy163 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2079 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 710 "parser.y"
{
    yygotominor.yy163 = Array_push_token_id(env, yymsp[-2].minor.yy163, yymsp[0].minor.yy0);
}
#line 2087 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 731 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy163, yymsp[-1].minor.yy163, yymsp[0].minor.yy163);
    yygotominor.yy163 = make_array_with(env, node);
}
#line 2096 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 744 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy163 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy163, yymsp[-1].minor.yy163);
}
#line 2105 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 750 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-8].minor.yy163, yymsp[-6].minor.yy163, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 2112 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 753 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-6].minor.yy163, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, yymsp[0].minor.yy163, YNIL);
}
#line 2119 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 756 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-6].minor.yy163, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, YNIL, yymsp[0].minor.yy163);
}
#line 2126 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 759 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, yymsp[0].minor.yy163, YNIL, YNIL);
}
#line 2133 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 762 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-6].minor.yy163, yymsp[-4].minor.yy163, YNIL, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 2140 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 765 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, YNIL, yymsp[0].minor.yy163, YNIL);
}
#line 2147 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 768 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, YNIL, YNIL, yymsp[0].minor.yy163);
}
#line 2154 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 771 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-2].minor.yy163, yymsp[0].minor.yy163, YNIL, YNIL, YNIL);
}
#line 2161 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 774 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-6].minor.yy163, YNIL, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 2168 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 777 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-4].minor.yy163, YNIL, yymsp[-2].minor.yy163, yymsp[0].minor.yy163, YNIL);
}
#line 2175 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 780 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-4].minor.yy163, YNIL, yymsp[-2].minor.yy163, YNIL, yymsp[0].minor.yy163);
}
#line 2182 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 783 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-2].minor.yy163, YNIL, yymsp[0].minor.yy163, YNIL, YNIL);
}
#line 2189 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 786 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-4].minor.yy163, YNIL, YNIL, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 2196 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 789 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-2].minor.yy163, YNIL, YNIL, yymsp[0].minor.yy163, YNIL);
}
#line 2203 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 792 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[-2].minor.yy163, YNIL, YNIL, YNIL, yymsp[0].minor.yy163);
}
#line 2210 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 795 "parser.y"
{
    yygotominor.yy163 = Params_new(env, yymsp[0].minor.yy163, YNIL, YNIL, YNIL, YNIL);
}
#line 2217 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 798 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, yymsp[-6].minor.yy163, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 2224 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 801 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, yymsp[0].minor.yy163, YNIL);
}
#line 2231 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 804 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, YNIL, yymsp[0].minor.yy163);
}
#line 2238 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 807 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, yymsp[-2].minor.yy163, yymsp[0].minor.yy163, YNIL, YNIL);
}
#line 2245 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 810 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, yymsp[-4].minor.yy163, YNIL, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 2252 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 813 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, yymsp[-2].minor.yy163, YNIL, yymsp[0].minor.yy163, YNIL);
}
#line 2259 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 816 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, yymsp[-2].minor.yy163, YNIL, YNIL, yymsp[0].minor.yy163);
}
#line 2266 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 819 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, yymsp[0].minor.yy163, YNIL, YNIL, YNIL);
}
#line 2273 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 822 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 2280 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 825 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy163, yymsp[0].minor.yy163, YNIL);
}
#line 2287 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 828 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy163, YNIL, yymsp[0].minor.yy163);
}
#line 2294 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 831 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy163, YNIL, YNIL);
}
#line 2301 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 834 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 2308 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 837 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy163, YNIL);
}
#line 2315 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 840 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy163);
}
#line 2322 "parser.c"
        break;
      case 64: /* params ::= */
#line 843 "parser.y"
{
    yygotominor.yy163 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2329 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 847 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy163 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2338 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 853 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy163 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2347 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 859 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy163 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy163);
}
#line 2356 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 876 "parser.y"
{
    yygotominor.yy163 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy163, lineno, id, YNIL);
}
#line 2366 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 882 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy163, lineno, id, YNIL);
    yygotominor.yy163 = yymsp[-2].minor.yy163;
}
#line 2376 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 896 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy163 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy163);
}
#line 2385 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 913 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy163);
    yygotominor.yy163 = Assign_new(env, lineno, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 2393 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 936 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy163);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy163)->u.id;
    yygotominor.yy163 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy163, id, yymsp[0].minor.yy163);
}
#line 2402 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
      case 87: /* comp_op ::= GREATER */
#line 942 "parser.y"
{
    yygotominor.yy163 = yymsp[0].minor.yy0;
}
#line 2410 "parser.c"
        break;
      case 90: /* or_expr ::= or_expr BAR and_expr */
#line 956 "parser.y"
{
    yygotominor.yy163 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy163), yymsp[-2].minor.yy163, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy163);
}
#line 2417 "parser.c"
        break;
      case 93: /* shift_expr ::= shift_expr shift_op match_expr */
      case 99: /* arith_expr ::= arith_expr arith_op term */
      case 102: /* term ::= term term_op factor */
#line 967 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy163);
    yygotominor.yy163 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy163, VAL2ID(yymsp[-1].minor.yy163), yymsp[0].minor.yy163);
}
#line 2427 "parser.c"
        break;
      case 94: /* shift_op ::= LSHIFT */
      case 95: /* shift_op ::= RSHIFT */
      case 100: /* arith_op ::= PLUS */
      case 101: /* arith_op ::= MINUS */
      case 104: /* term_op ::= STAR */
      case 105: /* term_op ::= DIV */
      case 106: /* term_op ::= DIV_DIV */
#line 972 "parser.y"
{
    yygotominor.yy163 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2440 "parser.c"
        break;
      case 97: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 982 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy163);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy163 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy163, id, yymsp[0].minor.yy163);
}
#line 2449 "parser.c"
        break;
      case 107: /* factor ::= PLUS factor */
#line 1021 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy163 = FuncCall_new3(env, lineno, yymsp[0].minor.yy163, id);
}
#line 2458 "parser.c"
        break;
      case 108: /* factor ::= MINUS factor */
#line 1026 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy163 = FuncCall_new3(env, lineno, yymsp[0].minor.yy163, id);
}
#line 2467 "parser.c"
        break;
      case 112: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1042 "parser.y"
{
    yygotominor.yy163 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy163), yymsp[-4].minor.yy163, yymsp[-2].minor.yy163, yymsp[0].minor.yy163);
}
#line 2474 "parser.c"
        break;
      case 113: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1045 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy163);
    yygotominor.yy163 = Subscript_new(env, lineno, yymsp[-3].minor.yy163, yymsp[-1].minor.yy163);
}
#line 2482 "parser.c"
        break;
      case 114: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1049 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy163);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy163 = Attr_new(env, lineno, yymsp[-2].minor.yy163, id);
}
#line 2491 "parser.c"
        break;
      case 115: /* atom ::= NAME */
#line 1055 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy163 = Variable_new(env, lineno, id);
}
#line 2500 "parser.c"
        break;
      case 116: /* atom ::= NUMBER */
      case 117: /* atom ::= REGEXP */
      case 118: /* atom ::= STRING */
      case 119: /* atom ::= SYMBOL */
#line 1060 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy163 = Literal_new(env, lineno, val);
}
#line 2512 "parser.c"
        break;
      case 120: /* atom ::= NIL */
#line 1080 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy163 = Literal_new(env, lineno, YNIL);
}
#line 2520 "parser.c"
        break;
      case 121: /* atom ::= TRUE */
#line 1084 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy163 = Literal_new(env, lineno, YTRUE);
}
#line 2528 "parser.c"
        break;
      case 122: /* atom ::= FALSE */
#line 1088 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy163 = Literal_new(env, lineno, YFALSE);
}
#line 2536 "parser.c"
        break;
      case 123: /* atom ::= LINE */
#line 1092 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy163 = Literal_new(env, lineno, val);
}
#line 2545 "parser.c"
        break;
      case 124: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1097 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy163 = Array_new(env, lineno, yymsp[-1].minor.yy163);
}
#line 2553 "parser.c"
        break;
      case 125: /* atom ::= LPAR expr RPAR */
      case 132: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1101 "parser.y"
{
    yygotominor.yy163 = yymsp[-1].minor.yy163;
}
#line 2561 "parser.c"
        break;
      case 129: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 130: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1115 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy163 = BlockArg_new(env, lineno, yymsp[-3].minor.yy163, yymsp[-1].minor.yy163);
}
#line 2570 "parser.c"
        break;
      case 134: /* excepts ::= excepts except */
#line 1134 "parser.y"
{
    yygotominor.yy163 = Array_push(env, yymsp[-1].minor.yy163, yymsp[0].minor.yy163);
}
#line 2577 "parser.c"
        break;
      case 135: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1138 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy163 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy163, id, yymsp[0].minor.yy163);
}
#line 2587 "parser.c"
        break;
      case 136: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1144 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy163 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy163, NO_EXC_VAR, yymsp[0].minor.yy163);
}
#line 2595 "parser.c"
        break;
      case 137: /* except ::= EXCEPT NEWLINE stmts */
#line 1148 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy163 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy163);
}
#line 2603 "parser.c"
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
