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
#define YYNOCODE 102
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy167;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 238
#define YYNRULE 143
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
 /*     0 */     1,  126,  127,   83,   20,   21,   24,   25,   26,  111,
 /*    10 */   179,   68,   55,   98,  201,   64,   58,  119,   23,  177,
 /*    20 */   165,  178,   28,  382,   84,  153,  151,  152,  124,  128,
 /*    30 */   208,   43,   44,  212,   16,   48,   18,  155,  180,  181,
 /*    40 */   182,  183,  184,  185,  186,  187,  157,   82,  168,  159,
 /*    50 */   160,  161,   59,  108,  100,  101,   63,  102,  195,   64,
 /*    60 */    58,   42,   83,  177,  165,  178,   47,  153,  151,  152,
 /*    70 */    97,   63,  102,   16,   64,   58,  237,   15,  177,  165,
 /*    80 */   178,  123,  130,  132,  217,  125,  205,  218,  157,   82,
 /*    90 */   168,  159,  160,  161,   59,   92,  100,  101,   63,  102,
 /*   100 */   143,   64,   58,   83,   17,  177,  165,  178,   60,  153,
 /*   110 */   151,  152,   61,  102,   80,   64,   58,  131,  215,  177,
 /*   120 */   165,  178,   30,   19,  126,  127,  129,  135,  220,   32,
 /*   130 */   157,   82,  168,  159,  160,  161,   59,  196,  100,  101,
 /*   140 */    63,  102,  195,   64,   58,  138,  224,  177,  165,  178,
 /*   150 */    93,  153,  151,  152,   36,  173,  121,  122,  133,  137,
 /*   160 */   139,  226,   16,    2,  218,    3,    3,  174,  175,  176,
 /*   170 */   144,   40,  157,   82,  168,  159,  160,  161,   59,   13,
 /*   180 */   100,  101,   63,  102,  190,   64,   58,  233,   38,  177,
 /*   190 */   165,  178,   85,  153,  151,  152,  232,   33,  140,  122,
 /*   200 */   133,  137,  139,  226,  115,  118,  218,  209,  210,   29,
 /*   210 */   235,   19,  169,  170,  157,   82,  168,  159,  160,  161,
 /*   220 */    59,  106,  100,  101,   63,  102,   83,   64,   58,   83,
 /*   230 */   145,  177,  165,  178,   86,  153,  151,  152,   62,   58,
 /*   240 */    75,  199,  177,  165,  178,  162,  165,  178,  171,  172,
 /*   250 */   126,  127,  129,   66,   81,  126,  157,   82,  168,  159,
 /*   260 */   160,  161,   59,  112,  100,  101,   63,  102,   83,   64,
 /*   270 */    58,   83,  113,  177,  165,  178,   49,  153,  151,  152,
 /*   280 */    72,   57,  238,   16,  177,  165,  178,  163,  165,  178,
 /*   290 */   126,  127,  129,   16,   16,  116,  191,  197,  157,   82,
 /*   300 */   168,  159,  160,  161,   59,  213,  100,  101,   63,  102,
 /*   310 */    83,   64,   58,  203,  207,  177,  165,  178,   50,  153,
 /*   320 */   151,  152,   16,   16,    8,  214,  164,  165,  178,  134,
 /*   330 */   136,  222,   16,   16,  212,  202,  236,  216,  219,   27,
 /*   340 */   157,   82,  168,  159,  160,  161,   59,  221,  100,  101,
 /*   350 */    63,  102,  223,   64,   58,  225,  141,  177,  165,  178,
 /*   360 */   110,  153,  151,  152,  154,   16,    4,  143,   33,   35,
 /*   370 */    34,   17,  229,   22,  188,   39,   31,  189,   65,    5,
 /*   380 */     6,  194,  157,   82,  168,  159,  160,  161,   59,   30,
 /*   390 */   100,  101,   63,  102,    7,   64,   58,   67,    9,  177,
 /*   400 */   165,  178,   87,  153,  151,  152,  114,  198,  117,   69,
 /*   410 */   200,  120,  228,  204,   11,  230,   37,   41,   45,   51,
 /*   420 */    70,   10,  206,   71,  157,   82,  168,  159,  160,  161,
 /*   430 */    59,   56,  100,  101,   63,  102,   52,   64,   58,   73,
 /*   440 */    74,  177,  165,  178,   88,  153,  151,  152,   46,   53,
 /*   450 */    76,   77,   54,   78,   79,  231,  234,  146,   12,  383,
 /*   460 */   383,  383,  383,  383,  383,  383,  157,   82,  168,  159,
 /*   470 */   160,  161,   59,  383,  100,  101,   63,  102,  383,   64,
 /*   480 */    58,  383,  383,  177,  165,  178,   89,  153,  151,  152,
 /*   490 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   500 */   383,  383,  383,  383,  383,  383,  383,  383,  157,   82,
 /*   510 */   168,  159,  160,  161,   59,  383,  100,  101,   63,  102,
 /*   520 */   383,   64,   58,  383,  383,  177,  165,  178,  147,  153,
 /*   530 */   151,  152,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   540 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   550 */   157,   82,  168,  159,  160,  161,   59,  383,  100,  101,
 /*   560 */    63,  102,  383,   64,   58,  383,  383,  177,  165,  178,
 /*   570 */   148,  153,  151,  152,  383,  383,  383,  383,  383,  383,
 /*   580 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   590 */   383,  383,  157,   82,  168,  159,  160,  161,   59,  383,
 /*   600 */   100,  101,   63,  102,  383,   64,   58,  383,  383,  177,
 /*   610 */   165,  178,  149,  153,  151,  152,  383,  383,  383,  383,
 /*   620 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   630 */   383,  383,  383,  383,  157,   82,  168,  159,  160,  161,
 /*   640 */    59,  383,  100,  101,   63,  102,  383,   64,   58,  383,
 /*   650 */   383,  177,  165,  178,   91,  153,  151,  152,  383,  383,
 /*   660 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   670 */   383,  383,  383,  383,  383,  383,  157,   82,  168,  159,
 /*   680 */   160,  161,   59,  383,  100,  101,   63,  102,  383,   64,
 /*   690 */    58,  383,   83,  177,  165,  178,  150,  151,  152,   96,
 /*   700 */   101,   63,  102,  383,   64,   58,  383,  383,  177,  165,
 /*   710 */   178,  383,  383,  383,  383,  383,  383,  157,   82,  168,
 /*   720 */   159,  160,  161,   59,  383,  100,  101,   63,  102,  383,
 /*   730 */    64,   58,  383,   83,  177,  165,  178,  166,   95,  383,
 /*   740 */   100,  101,   63,  102,  383,   64,   58,  383,  383,  177,
 /*   750 */   165,  178,  383,  383,  383,   99,  157,   82,  168,  159,
 /*   760 */   160,  161,   59,  383,  100,  101,   63,  102,  383,   64,
 /*   770 */    58,  383,  383,  177,  165,  178,  105,  166,  383,  383,
 /*   780 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   790 */   383,  383,  383,  383,  383,   99,  157,   82,  168,  159,
 /*   800 */   160,  161,   59,  383,  100,  101,   63,  102,  383,   64,
 /*   810 */    58,  383,  383,  177,  165,  178,  103,   90,  383,  383,
 /*   820 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   830 */   383,  383,  383,  383,  383,  383,  157,   82,  168,  159,
 /*   840 */   160,  161,   59,  383,  100,  101,   63,  102,  383,   64,
 /*   850 */    58,  383,  383,  177,  165,  178,   94,  383,  383,  383,
 /*   860 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   870 */   383,  383,  383,  383,  383,  157,   82,  168,  159,  160,
 /*   880 */   161,   59,  383,  100,  101,   63,  102,  383,   64,   58,
 /*   890 */   383,  383,  177,  165,  178,  156,  383,  383,  383,  383,
 /*   900 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   910 */   383,  383,  383,  383,  157,   82,  168,  159,  160,  161,
 /*   920 */    59,  383,  100,  101,   63,  102,  383,   64,   58,  383,
 /*   930 */   383,  177,  165,  178,  167,  383,  383,  383,  383,  383,
 /*   940 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   950 */   383,  383,  383,  157,   82,  168,  159,  160,  161,   59,
 /*   960 */   383,  100,  101,   63,  102,  383,   64,   58,  383,  383,
 /*   970 */   177,  165,  178,  104,  383,  383,  383,  383,  383,  383,
 /*   980 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*   990 */   383,  383,  157,   82,  168,  159,  160,  161,   59,  383,
 /*  1000 */   100,  101,   63,  102,  383,   64,   58,  383,  383,  177,
 /*  1010 */   165,  178,  192,  383,  383,  383,  383,  383,  383,  383,
 /*  1020 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*  1030 */   383,  157,   82,  168,  159,  160,  161,   59,  383,  100,
 /*  1040 */   101,   63,  102,  383,   64,   58,  383,  383,  177,  165,
 /*  1050 */   178,  193,  383,  383,  383,  383,  383,  383,  383,  383,
 /*  1060 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*  1070 */   157,   82,  168,  159,  160,  161,   59,  383,  100,  101,
 /*  1080 */    63,  102,  383,   64,   58,  383,  383,  177,  165,  178,
 /*  1090 */   107,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*  1100 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  157,
 /*  1110 */    82,  168,  159,  160,  161,   59,  383,  100,  101,   63,
 /*  1120 */   102,  383,   64,   58,  383,  383,  177,  165,  178,  109,
 /*  1130 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*  1140 */   383,  383,  383,  383,  383,  383,  383,  383,  157,   82,
 /*  1150 */   168,  159,  160,  161,   59,  383,  100,  101,   63,  102,
 /*  1160 */   383,   64,   58,  383,  383,  177,  165,  178,  211,  383,
 /*  1170 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*  1180 */   383,  383,  383,  383,  383,  383,  383,  157,   82,  168,
 /*  1190 */   159,  160,  161,   59,  383,  100,  101,   63,  102,  383,
 /*  1200 */    64,   58,  383,  383,  177,  165,  178,  227,  383,  383,
 /*  1210 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*  1220 */   383,  383,  383,  383,  383,  383,  157,   82,  168,  159,
 /*  1230 */   160,  161,   59,  383,  100,  101,   63,  102,  383,   64,
 /*  1240 */    58,  383,  383,  177,  165,  178,  142,  383,  383,  383,
 /*  1250 */   383,  383,  383,  383,  383,  383,  383,  383,  383,  383,
 /*  1260 */   383,  383,  383,  383,  383,  157,   82,  168,  159,  160,
 /*  1270 */   161,   59,  383,  100,  101,   63,  102,  383,   64,   58,
 /*  1280 */    14,  383,  177,  165,  178,   83,  158,  159,  160,  161,
 /*  1290 */    59,  179,  100,  101,   63,  102,  383,   64,   58,   23,
 /*  1300 */   383,  177,  165,  178,  383,  383,  179,  383,  383,  383,
 /*  1310 */   383,  383,   43,   44,   23,  383,  383,   18,  383,  180,
 /*  1320 */   181,  182,  183,  184,  185,  186,  187,   43,   44,  383,
 /*  1330 */   383,  383,   18,  383,  180,  181,  182,  183,  184,  185,
 /*  1340 */   186,  187,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   78,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   88,   12,   90,   91,   19,   20,   94,
 /*    20 */    95,   96,   25,   54,   55,   56,   57,   58,   70,   71,
 /*    30 */    72,   33,   34,   75,    1,   59,   38,    4,   40,   41,
 /*    40 */    42,   43,   44,   45,   46,   47,   77,   78,   79,   80,
 /*    50 */    81,   82,   83,   61,   85,   86,   87,   88,   66,   90,
 /*    60 */    91,   93,   78,   94,   95,   96,   55,   56,   57,   58,
 /*    70 */    86,   87,   88,    1,   90,   91,  100,    5,   94,   95,
 /*    80 */    96,   69,   70,   71,   72,   71,   72,   75,   77,   78,
 /*    90 */    79,   80,   81,   82,   83,   60,   85,   86,   87,   88,
 /*   100 */    16,   90,   91,   78,   20,   94,   95,   96,   55,   56,
 /*   110 */    57,   58,   87,   88,   12,   90,   91,   71,   72,   94,
 /*   120 */    95,   96,   38,   51,   22,   23,   24,   71,   72,   84,
 /*   130 */    77,   78,   79,   80,   81,   82,   83,   61,   85,   86,
 /*   140 */    87,   88,   66,   90,   91,   71,   72,   94,   95,   96,
 /*   150 */    55,   56,   57,   58,   89,   23,   67,   68,   69,   70,
 /*   160 */    71,   72,    1,    3,   75,    5,    5,   35,   36,   37,
 /*   170 */    60,   92,   77,   78,   79,   80,   81,   82,   83,    1,
 /*   180 */    85,   86,   87,   88,   98,   90,   91,   17,   38,   94,
 /*   190 */    95,   96,   55,   56,   57,   58,   26,   27,   67,   68,
 /*   200 */    69,   70,   71,   72,   64,   65,   75,   73,   74,   17,
 /*   210 */   100,   51,   30,   31,   77,   78,   79,   80,   81,   82,
 /*   220 */    83,   99,   85,   86,   87,   88,   78,   90,   91,   78,
 /*   230 */    52,   94,   95,   96,   55,   56,   57,   58,   90,   91,
 /*   240 */    12,   12,   94,   95,   96,   94,   95,   96,   33,   34,
 /*   250 */    22,   23,   24,   48,   49,   22,   77,   78,   79,   80,
 /*   260 */    81,   82,   83,   62,   85,   86,   87,   88,   78,   90,
 /*   270 */    91,   78,   63,   94,   95,   96,   55,   56,   57,   58,
 /*   280 */    12,   91,    0,    1,   94,   95,   96,   94,   95,   96,
 /*   290 */    22,   23,   24,    1,    1,   65,    4,    4,   77,   78,
 /*   300 */    79,   80,   81,   82,   83,   74,   85,   86,   87,   88,
 /*   310 */    78,   90,   91,   72,   72,   94,   95,   96,   55,   56,
 /*   320 */    57,   58,    1,    1,    3,   72,   94,   95,   96,   70,
 /*   330 */    71,   72,    1,    1,   75,    4,    4,   72,   72,   18,
 /*   340 */    77,   78,   79,   80,   81,   82,   83,   72,   85,   86,
 /*   350 */    87,   88,   72,   90,   91,   72,   99,   94,   95,   96,
 /*   360 */    55,   56,   57,   58,    4,    1,    1,   16,   27,   29,
 /*   370 */    28,   20,   50,   15,   39,   32,   25,   21,   21,    1,
 /*   380 */     1,    4,   77,   78,   79,   80,   81,   82,   83,   38,
 /*   390 */    85,   86,   87,   88,    1,   90,   91,   12,    1,   94,
 /*   400 */    95,   96,   55,   56,   57,   58,   15,   12,   16,   15,
 /*   410 */    12,   12,   39,   12,    1,   39,   20,   15,   15,   15,
 /*   420 */    15,   21,   12,   15,   77,   78,   79,   80,   81,   82,
 /*   430 */    83,   12,   85,   86,   87,   88,   15,   90,   91,   15,
 /*   440 */    15,   94,   95,   96,   55,   56,   57,   58,   15,   15,
 /*   450 */    15,   15,   15,   15,   15,   12,    4,   12,    1,  101,
 /*   460 */   101,  101,  101,  101,  101,  101,   77,   78,   79,   80,
 /*   470 */    81,   82,   83,  101,   85,   86,   87,   88,  101,   90,
 /*   480 */    91,  101,  101,   94,   95,   96,   55,   56,   57,   58,
 /*   490 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   500 */   101,  101,  101,  101,  101,  101,  101,  101,   77,   78,
 /*   510 */    79,   80,   81,   82,   83,  101,   85,   86,   87,   88,
 /*   520 */   101,   90,   91,  101,  101,   94,   95,   96,   55,   56,
 /*   530 */    57,   58,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   540 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   550 */    77,   78,   79,   80,   81,   82,   83,  101,   85,   86,
 /*   560 */    87,   88,  101,   90,   91,  101,  101,   94,   95,   96,
 /*   570 */    55,   56,   57,   58,  101,  101,  101,  101,  101,  101,
 /*   580 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   590 */   101,  101,   77,   78,   79,   80,   81,   82,   83,  101,
 /*   600 */    85,   86,   87,   88,  101,   90,   91,  101,  101,   94,
 /*   610 */    95,   96,   55,   56,   57,   58,  101,  101,  101,  101,
 /*   620 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   630 */   101,  101,  101,  101,   77,   78,   79,   80,   81,   82,
 /*   640 */    83,  101,   85,   86,   87,   88,  101,   90,   91,  101,
 /*   650 */   101,   94,   95,   96,   55,   56,   57,   58,  101,  101,
 /*   660 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   670 */   101,  101,  101,  101,  101,  101,   77,   78,   79,   80,
 /*   680 */    81,   82,   83,  101,   85,   86,   87,   88,  101,   90,
 /*   690 */    91,  101,   78,   94,   95,   96,   56,   57,   58,   85,
 /*   700 */    86,   87,   88,  101,   90,   91,  101,  101,   94,   95,
 /*   710 */    96,  101,  101,  101,  101,  101,  101,   77,   78,   79,
 /*   720 */    80,   81,   82,   83,  101,   85,   86,   87,   88,  101,
 /*   730 */    90,   91,  101,   78,   94,   95,   96,   58,   83,  101,
 /*   740 */    85,   86,   87,   88,  101,   90,   91,  101,  101,   94,
 /*   750 */    95,   96,  101,  101,  101,   76,   77,   78,   79,   80,
 /*   760 */    81,   82,   83,  101,   85,   86,   87,   88,  101,   90,
 /*   770 */    91,  101,  101,   94,   95,   96,   97,   58,  101,  101,
 /*   780 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   790 */   101,  101,  101,  101,  101,   76,   77,   78,   79,   80,
 /*   800 */    81,   82,   83,  101,   85,   86,   87,   88,  101,   90,
 /*   810 */    91,  101,  101,   94,   95,   96,   97,   58,  101,  101,
 /*   820 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   830 */   101,  101,  101,  101,  101,  101,   77,   78,   79,   80,
 /*   840 */    81,   82,   83,  101,   85,   86,   87,   88,  101,   90,
 /*   850 */    91,  101,  101,   94,   95,   96,   58,  101,  101,  101,
 /*   860 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   870 */   101,  101,  101,  101,  101,   77,   78,   79,   80,   81,
 /*   880 */    82,   83,  101,   85,   86,   87,   88,  101,   90,   91,
 /*   890 */   101,  101,   94,   95,   96,   58,  101,  101,  101,  101,
 /*   900 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   910 */   101,  101,  101,  101,   77,   78,   79,   80,   81,   82,
 /*   920 */    83,  101,   85,   86,   87,   88,  101,   90,   91,  101,
 /*   930 */   101,   94,   95,   96,   58,  101,  101,  101,  101,  101,
 /*   940 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   950 */   101,  101,  101,   77,   78,   79,   80,   81,   82,   83,
 /*   960 */   101,   85,   86,   87,   88,  101,   90,   91,  101,  101,
 /*   970 */    94,   95,   96,   58,  101,  101,  101,  101,  101,  101,
 /*   980 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*   990 */   101,  101,   77,   78,   79,   80,   81,   82,   83,  101,
 /*  1000 */    85,   86,   87,   88,  101,   90,   91,  101,  101,   94,
 /*  1010 */    95,   96,   58,  101,  101,  101,  101,  101,  101,  101,
 /*  1020 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*  1030 */   101,   77,   78,   79,   80,   81,   82,   83,  101,   85,
 /*  1040 */    86,   87,   88,  101,   90,   91,  101,  101,   94,   95,
 /*  1050 */    96,   58,  101,  101,  101,  101,  101,  101,  101,  101,
 /*  1060 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*  1070 */    77,   78,   79,   80,   81,   82,   83,  101,   85,   86,
 /*  1080 */    87,   88,  101,   90,   91,  101,  101,   94,   95,   96,
 /*  1090 */    58,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*  1100 */   101,  101,  101,  101,  101,  101,  101,  101,  101,   77,
 /*  1110 */    78,   79,   80,   81,   82,   83,  101,   85,   86,   87,
 /*  1120 */    88,  101,   90,   91,  101,  101,   94,   95,   96,   58,
 /*  1130 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*  1140 */   101,  101,  101,  101,  101,  101,  101,  101,   77,   78,
 /*  1150 */    79,   80,   81,   82,   83,  101,   85,   86,   87,   88,
 /*  1160 */   101,   90,   91,  101,  101,   94,   95,   96,   58,  101,
 /*  1170 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*  1180 */   101,  101,  101,  101,  101,  101,  101,   77,   78,   79,
 /*  1190 */    80,   81,   82,   83,  101,   85,   86,   87,   88,  101,
 /*  1200 */    90,   91,  101,  101,   94,   95,   96,   58,  101,  101,
 /*  1210 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*  1220 */   101,  101,  101,  101,  101,  101,   77,   78,   79,   80,
 /*  1230 */    81,   82,   83,  101,   85,   86,   87,   88,  101,   90,
 /*  1240 */    91,  101,  101,   94,   95,   96,   58,  101,  101,  101,
 /*  1250 */   101,  101,  101,  101,  101,  101,  101,  101,  101,  101,
 /*  1260 */   101,  101,  101,  101,  101,   77,   78,   79,   80,   81,
 /*  1270 */    82,   83,  101,   85,   86,   87,   88,  101,   90,   91,
 /*  1280 */     1,  101,   94,   95,   96,   78,   79,   80,   81,   82,
 /*  1290 */    83,   12,   85,   86,   87,   88,  101,   90,   91,   20,
 /*  1300 */   101,   94,   95,   96,  101,  101,   12,  101,  101,  101,
 /*  1310 */   101,  101,   33,   34,   20,  101,  101,   38,  101,   40,
 /*  1320 */    41,   42,   43,   44,   45,   46,   47,   33,   34,  101,
 /*  1330 */   101,  101,   38,  101,   40,   41,   42,   43,   44,   45,
 /*  1340 */    46,   47,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 149
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2, 1294, 1294, 1279,
 /*    20 */  1294, 1294, 1294, 1294, 1294, 1294, 1294, 1294, 1294, 1294,
 /*    30 */  1294, 1294, 1294, 1294, 1294, 1294, 1294,  102,  102, 1294,
 /*    40 */  1294,  228, 1294, 1294, 1294,  268,  268,   72,  160,  321,
 /*    50 */   321,  -21,  -21,  -21,  -21,    2,   -3,  132,  132,  170,
 /*    60 */   161,  182,  215,  182,  215,  205,  150,  192,  229,    2,
 /*    70 */   233,  233,   -3,  233,  233,   -3,  233,  233,  233,  233,
 /*    80 */    -3,  150,  351,   84,  282,   33,  292,  293,  331,  322,
 /*    90 */   178,  332,  360,  364,  365,  341,  342,  340,  343,  358,
 /*   100 */   342,  340,  343,  335,  356,  357,  378,  379,  377,  393,
 /*   110 */   364,  385,  397,  391,  395,  394,  392,  398,  392,  399,
 /*   120 */   396,  400,  402,  403,  404,  405,  401,  410,  408,  419,
 /*   130 */   421,  424,  425,  433,  434,  435,  436,  437,  438,  439,
 /*   140 */   373,  413,  376,  443,  452,  445,  457,  364,  364,  364,
};
#define YY_REDUCE_USE_DFLT (-76)
#define YY_REDUCE_MAX 81
static const short yy_reduce_ofst[] = {
 /*     0 */   -31,   11,   53,   95,  137,  179,  221,  263,  305,  347,
 /*    10 */   389,  431,  473,  515,  557,  599,  640,  679,  719,  759,
 /*    20 */   798,  837,  876,  915,  954,  993, 1032, 1071, 1110, 1149,
 /*    30 */  1188, 1207,  655,  614,  -16,   25,  -75,   89,  131,  148,
 /*    40 */   190,   12,  151,  193,  232,  -42,  259,  -24,  110,   -8,
 /*    50 */    76,   14,   46,   56,   74,  140,  134,  -32,  -32,   45,
 /*    60 */    35,   65,   79,   65,   79,   86,  122,  201,  209,  230,
 /*    70 */   241,  242,  231,  253,  265,  231,  266,  275,  280,  283,
 /*    80 */   231,  257,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   241,  241,  241,  241,  241,  241,  241,  241,  241,  241,
 /*    10 */   241,  241,  241,  241,  241,  241,  241,  367,  367,  381,
 /*    20 */   381,  248,  381,  381,  250,  252,  381,  381,  381,  381,
 /*    30 */   381,  381,  381,  381,  381,  381,  381,  302,  302,  381,
 /*    40 */   381,  381,  381,  381,  381,  381,  381,  381,  379,  268,
 /*    50 */   268,  381,  381,  381,  381,  381,  306,  339,  338,  322,
 /*    60 */   379,  331,  337,  330,  336,  369,  372,  264,  381,  381,
 /*    70 */   381,  381,  381,  381,  381,  310,  381,  381,  381,  381,
 /*    80 */   309,  372,  351,  351,  381,  381,  381,  381,  381,  381,
 /*    90 */   381,  381,  381,  380,  381,  323,  327,  329,  333,  368,
 /*   100 */   326,  328,  332,  381,  381,  381,  381,  381,  381,  381,
 /*   110 */   269,  381,  381,  256,  381,  257,  259,  381,  258,  381,
 /*   120 */   381,  381,  286,  278,  274,  272,  381,  381,  276,  381,
 /*   130 */   282,  280,  284,  294,  290,  288,  292,  298,  296,  300,
 /*   140 */   381,  381,  381,  381,  381,  381,  381,  376,  377,  378,
 /*   150 */   240,  242,  243,  239,  244,  247,  249,  316,  317,  319,
 /*   160 */   320,  321,  342,  348,  349,  350,  314,  315,  318,  334,
 /*   170 */   335,  340,  341,  344,  345,  346,  347,  343,  352,  356,
 /*   180 */   357,  358,  359,  360,  361,  362,  363,  364,  365,  366,
 /*   190 */   353,  370,  251,  253,  254,  266,  267,  255,  263,  262,
 /*   200 */   261,  260,  270,  271,  303,  273,  304,  275,  277,  305,
 /*   210 */   307,  308,  312,  313,  279,  281,  283,  285,  311,  287,
 /*   220 */   289,  291,  293,  295,  297,  299,  301,  265,  373,  371,
 /*   230 */   354,  355,  324,  325,  245,  375,  246,  374,
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
  "AMPER",         "EQUAL",         "LESS",          "XOR",         
  "BAR",           "AND",           "LSHIFT",        "RSHIFT",      
  "EQUAL_TILDA",   "PLUS",          "MINUS",         "DIV",         
  "DIV_DIV",       "PERCENT",       "LBRACKET",      "RBRACKET",    
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
 /*  89 */ "xor_expr ::= xor_expr XOR or_expr",
 /*  90 */ "or_expr ::= and_expr",
 /*  91 */ "or_expr ::= or_expr BAR and_expr",
 /*  92 */ "and_expr ::= shift_expr",
 /*  93 */ "and_expr ::= and_expr AND shift_expr",
 /*  94 */ "shift_expr ::= match_expr",
 /*  95 */ "shift_expr ::= shift_expr shift_op match_expr",
 /*  96 */ "shift_op ::= LSHIFT",
 /*  97 */ "shift_op ::= RSHIFT",
 /*  98 */ "match_expr ::= arith_expr",
 /*  99 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 100 */ "arith_expr ::= term",
 /* 101 */ "arith_expr ::= arith_expr arith_op term",
 /* 102 */ "arith_op ::= PLUS",
 /* 103 */ "arith_op ::= MINUS",
 /* 104 */ "term ::= term term_op factor",
 /* 105 */ "term ::= factor",
 /* 106 */ "term_op ::= STAR",
 /* 107 */ "term_op ::= DIV",
 /* 108 */ "term_op ::= DIV_DIV",
 /* 109 */ "term_op ::= PERCENT",
 /* 110 */ "factor ::= PLUS factor",
 /* 111 */ "factor ::= MINUS factor",
 /* 112 */ "factor ::= power",
 /* 113 */ "power ::= postfix_expr",
 /* 114 */ "postfix_expr ::= atom",
 /* 115 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 116 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 117 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 118 */ "atom ::= NAME",
 /* 119 */ "atom ::= NUMBER",
 /* 120 */ "atom ::= REGEXP",
 /* 121 */ "atom ::= STRING",
 /* 122 */ "atom ::= SYMBOL",
 /* 123 */ "atom ::= NIL",
 /* 124 */ "atom ::= TRUE",
 /* 125 */ "atom ::= FALSE",
 /* 126 */ "atom ::= LINE",
 /* 127 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 128 */ "atom ::= LPAR expr RPAR",
 /* 129 */ "args_opt ::=",
 /* 130 */ "args_opt ::= args",
 /* 131 */ "blockarg_opt ::=",
 /* 132 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 133 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 134 */ "blockarg_params_opt ::=",
 /* 135 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 136 */ "excepts ::= except",
 /* 137 */ "excepts ::= excepts except",
 /* 138 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 139 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 140 */ "except ::= EXCEPT NEWLINE stmts",
 /* 141 */ "finally_opt ::=",
 /* 142 */ "finally_opt ::= FINALLY stmts",
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
  { 54, 1 },
  { 55, 1 },
  { 55, 3 },
  { 56, 0 },
  { 56, 1 },
  { 56, 1 },
  { 56, 7 },
  { 56, 5 },
  { 56, 5 },
  { 56, 5 },
  { 56, 1 },
  { 56, 2 },
  { 56, 1 },
  { 56, 2 },
  { 56, 1 },
  { 56, 2 },
  { 56, 6 },
  { 56, 6 },
  { 56, 2 },
  { 56, 2 },
  { 64, 1 },
  { 64, 3 },
  { 65, 1 },
  { 65, 3 },
  { 63, 1 },
  { 63, 3 },
  { 62, 0 },
  { 62, 2 },
  { 61, 1 },
  { 61, 5 },
  { 66, 0 },
  { 66, 2 },
  { 57, 7 },
  { 67, 9 },
  { 67, 7 },
  { 67, 7 },
  { 67, 5 },
  { 67, 7 },
  { 67, 5 },
  { 67, 5 },
  { 67, 3 },
  { 67, 7 },
  { 67, 5 },
  { 67, 5 },
  { 67, 3 },
  { 67, 5 },
  { 67, 3 },
  { 67, 3 },
  { 67, 1 },
  { 67, 7 },
  { 67, 5 },
  { 67, 5 },
  { 67, 3 },
  { 67, 5 },
  { 67, 3 },
  { 67, 3 },
  { 67, 1 },
  { 67, 5 },
  { 67, 3 },
  { 67, 3 },
  { 67, 1 },
  { 67, 3 },
  { 67, 1 },
  { 67, 1 },
  { 67, 0 },
  { 72, 2 },
  { 71, 2 },
  { 70, 3 },
  { 73, 0 },
  { 73, 1 },
  { 74, 2 },
  { 68, 1 },
  { 68, 3 },
  { 69, 1 },
  { 69, 3 },
  { 75, 2 },
  { 76, 1 },
  { 76, 3 },
  { 58, 1 },
  { 77, 3 },
  { 77, 1 },
  { 79, 1 },
  { 80, 1 },
  { 81, 1 },
  { 82, 1 },
  { 82, 3 },
  { 84, 1 },
  { 84, 1 },
  { 83, 1 },
  { 83, 3 },
  { 85, 1 },
  { 85, 3 },
  { 86, 1 },
  { 86, 3 },
  { 87, 1 },
  { 87, 3 },
  { 89, 1 },
  { 89, 1 },
  { 88, 1 },
  { 88, 3 },
  { 90, 1 },
  { 90, 3 },
  { 92, 1 },
  { 92, 1 },
  { 91, 3 },
  { 91, 1 },
  { 93, 1 },
  { 93, 1 },
  { 93, 1 },
  { 93, 1 },
  { 94, 2 },
  { 94, 2 },
  { 94, 1 },
  { 95, 1 },
  { 78, 1 },
  { 78, 5 },
  { 78, 4 },
  { 78, 3 },
  { 96, 1 },
  { 96, 1 },
  { 96, 1 },
  { 96, 1 },
  { 96, 1 },
  { 96, 1 },
  { 96, 1 },
  { 96, 1 },
  { 96, 1 },
  { 96, 3 },
  { 96, 3 },
  { 97, 0 },
  { 97, 1 },
  { 98, 0 },
  { 98, 5 },
  { 98, 5 },
  { 99, 0 },
  { 99, 3 },
  { 59, 1 },
  { 59, 2 },
  { 100, 6 },
  { 100, 4 },
  { 100, 3 },
  { 60, 0 },
  { 60, 2 },
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
    *pval = yymsp[0].minor.yy167;
}
#line 1910 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 136: /* excepts ::= except */
#line 626 "parser.y"
{
    yygotominor.yy167 = make_array_with(env, yymsp[0].minor.yy167);
}
#line 1921 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 629 "parser.y"
{
    yygotominor.yy167 = Array_push(env, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 1931 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 129: /* args_opt ::= */
      case 131: /* blockarg_opt ::= */
      case 134: /* blockarg_params_opt ::= */
      case 141: /* finally_opt ::= */
#line 633 "parser.y"
{
    yygotominor.yy167 = YNIL;
}
#line 1945 "parser.c"
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
      case 90: /* or_expr ::= and_expr */
      case 92: /* and_expr ::= shift_expr */
      case 94: /* shift_expr ::= match_expr */
      case 98: /* match_expr ::= arith_expr */
      case 100: /* arith_expr ::= term */
      case 105: /* term ::= factor */
      case 112: /* factor ::= power */
      case 113: /* power ::= postfix_expr */
      case 114: /* postfix_expr ::= atom */
      case 130: /* args_opt ::= args */
      case 142: /* finally_opt ::= FINALLY stmts */
#line 636 "parser.y"
{
    yygotominor.yy167 = yymsp[0].minor.yy167;
}
#line 1976 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 642 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy167 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy167, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, yymsp[-1].minor.yy167);
}
#line 1984 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 646 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy167 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy167, yymsp[-2].minor.yy167, YNIL, yymsp[-1].minor.yy167);
}
#line 1992 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 650 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy167 = Finally_new(env, lineno, yymsp[-3].minor.yy167, yymsp[-1].minor.yy167);
}
#line 2000 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 654 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy167 = While_new(env, lineno, yymsp[-3].minor.yy167, yymsp[-1].minor.yy167);
}
#line 2008 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 658 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy167 = Break_new(env, lineno, YNIL);
}
#line 2016 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 662 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy167 = Break_new(env, lineno, yymsp[0].minor.yy167);
}
#line 2024 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 666 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy167 = Next_new(env, lineno, YNIL);
}
#line 2032 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 670 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy167 = Next_new(env, lineno, yymsp[0].minor.yy167);
}
#line 2040 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 674 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy167 = Return_new(env, lineno, YNIL);
}
#line 2048 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 678 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy167 = Return_new(env, lineno, yymsp[0].minor.yy167);
}
#line 2056 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 682 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy167 = If_new(env, lineno, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, yymsp[-1].minor.yy167);
}
#line 2064 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 686 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy167 = Klass_new(env, lineno, id, yymsp[-3].minor.yy167, yymsp[-1].minor.yy167);
}
#line 2073 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 691 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy167 = Nonlocal_new(env, lineno, yymsp[0].minor.yy167);
}
#line 2081 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 695 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy167 = Import_new(env, lineno, yymsp[0].minor.yy167);
}
#line 2089 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 707 "parser.y"
{
    yygotominor.yy167 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2097 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 710 "parser.y"
{
    yygotominor.yy167 = Array_push_token_id(env, yymsp[-2].minor.yy167, yymsp[0].minor.yy0);
}
#line 2105 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 731 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy167, yymsp[-1].minor.yy167, yymsp[0].minor.yy167);
    yygotominor.yy167 = make_array_with(env, node);
}
#line 2114 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 744 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy167 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy167, yymsp[-1].minor.yy167);
}
#line 2123 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 750 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-8].minor.yy167, yymsp[-6].minor.yy167, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 2130 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 753 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-6].minor.yy167, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, yymsp[0].minor.yy167, YNIL);
}
#line 2137 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 756 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-6].minor.yy167, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, YNIL, yymsp[0].minor.yy167);
}
#line 2144 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 759 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, yymsp[0].minor.yy167, YNIL, YNIL);
}
#line 2151 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 762 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-6].minor.yy167, yymsp[-4].minor.yy167, YNIL, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 2158 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 765 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, YNIL, yymsp[0].minor.yy167, YNIL);
}
#line 2165 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 768 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, YNIL, YNIL, yymsp[0].minor.yy167);
}
#line 2172 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 771 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-2].minor.yy167, yymsp[0].minor.yy167, YNIL, YNIL, YNIL);
}
#line 2179 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 774 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-6].minor.yy167, YNIL, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 2186 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 777 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-4].minor.yy167, YNIL, yymsp[-2].minor.yy167, yymsp[0].minor.yy167, YNIL);
}
#line 2193 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 780 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-4].minor.yy167, YNIL, yymsp[-2].minor.yy167, YNIL, yymsp[0].minor.yy167);
}
#line 2200 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 783 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-2].minor.yy167, YNIL, yymsp[0].minor.yy167, YNIL, YNIL);
}
#line 2207 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 786 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-4].minor.yy167, YNIL, YNIL, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 2214 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 789 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-2].minor.yy167, YNIL, YNIL, yymsp[0].minor.yy167, YNIL);
}
#line 2221 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 792 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[-2].minor.yy167, YNIL, YNIL, YNIL, yymsp[0].minor.yy167);
}
#line 2228 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 795 "parser.y"
{
    yygotominor.yy167 = Params_new(env, yymsp[0].minor.yy167, YNIL, YNIL, YNIL, YNIL);
}
#line 2235 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 798 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, yymsp[-6].minor.yy167, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 2242 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 801 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, yymsp[0].minor.yy167, YNIL);
}
#line 2249 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 804 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, YNIL, yymsp[0].minor.yy167);
}
#line 2256 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 807 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, yymsp[-2].minor.yy167, yymsp[0].minor.yy167, YNIL, YNIL);
}
#line 2263 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 810 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, yymsp[-4].minor.yy167, YNIL, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 2270 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 813 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, yymsp[-2].minor.yy167, YNIL, yymsp[0].minor.yy167, YNIL);
}
#line 2277 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 816 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, yymsp[-2].minor.yy167, YNIL, YNIL, yymsp[0].minor.yy167);
}
#line 2284 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 819 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, yymsp[0].minor.yy167, YNIL, YNIL, YNIL);
}
#line 2291 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 822 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 2298 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 825 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy167, yymsp[0].minor.yy167, YNIL);
}
#line 2305 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 828 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy167, YNIL, yymsp[0].minor.yy167);
}
#line 2312 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 831 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy167, YNIL, YNIL);
}
#line 2319 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 834 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 2326 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 837 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy167, YNIL);
}
#line 2333 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 840 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy167);
}
#line 2340 "parser.c"
        break;
      case 64: /* params ::= */
#line 843 "parser.y"
{
    yygotominor.yy167 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2347 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 847 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy167 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2356 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 853 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy167 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2365 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 859 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy167 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy167);
}
#line 2374 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 876 "parser.y"
{
    yygotominor.yy167 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy167, lineno, id, YNIL);
}
#line 2384 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 882 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy167, lineno, id, YNIL);
    yygotominor.yy167 = yymsp[-2].minor.yy167;
}
#line 2394 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 896 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy167 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy167);
}
#line 2403 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 913 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy167);
    yygotominor.yy167 = Assign_new(env, lineno, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 2411 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 936 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy167);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy167)->u.id;
    yygotominor.yy167 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy167, id, yymsp[0].minor.yy167);
}
#line 2420 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
      case 87: /* comp_op ::= GREATER */
#line 942 "parser.y"
{
    yygotominor.yy167 = yymsp[0].minor.yy0;
}
#line 2428 "parser.c"
        break;
      case 89: /* xor_expr ::= xor_expr XOR or_expr */
      case 91: /* or_expr ::= or_expr BAR and_expr */
      case 93: /* and_expr ::= and_expr AND shift_expr */
#line 952 "parser.y"
{
    yygotominor.yy167 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy167), yymsp[-2].minor.yy167, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy167);
}
#line 2437 "parser.c"
        break;
      case 95: /* shift_expr ::= shift_expr shift_op match_expr */
      case 101: /* arith_expr ::= arith_expr arith_op term */
      case 104: /* term ::= term term_op factor */
#line 973 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy167);
    yygotominor.yy167 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy167, VAL2ID(yymsp[-1].minor.yy167), yymsp[0].minor.yy167);
}
#line 2447 "parser.c"
        break;
      case 96: /* shift_op ::= LSHIFT */
      case 97: /* shift_op ::= RSHIFT */
      case 102: /* arith_op ::= PLUS */
      case 103: /* arith_op ::= MINUS */
      case 106: /* term_op ::= STAR */
      case 107: /* term_op ::= DIV */
      case 108: /* term_op ::= DIV_DIV */
      case 109: /* term_op ::= PERCENT */
#line 978 "parser.y"
{
    yygotominor.yy167 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2461 "parser.c"
        break;
      case 99: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 988 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy167);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy167 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy167, id, yymsp[0].minor.yy167);
}
#line 2470 "parser.c"
        break;
      case 110: /* factor ::= PLUS factor */
#line 1030 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy167 = FuncCall_new3(env, lineno, yymsp[0].minor.yy167, id);
}
#line 2479 "parser.c"
        break;
      case 111: /* factor ::= MINUS factor */
#line 1035 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy167 = FuncCall_new3(env, lineno, yymsp[0].minor.yy167, id);
}
#line 2488 "parser.c"
        break;
      case 115: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1051 "parser.y"
{
    yygotominor.yy167 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy167), yymsp[-4].minor.yy167, yymsp[-2].minor.yy167, yymsp[0].minor.yy167);
}
#line 2495 "parser.c"
        break;
      case 116: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1054 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy167);
    yygotominor.yy167 = Subscript_new(env, lineno, yymsp[-3].minor.yy167, yymsp[-1].minor.yy167);
}
#line 2503 "parser.c"
        break;
      case 117: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1058 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy167);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy167 = Attr_new(env, lineno, yymsp[-2].minor.yy167, id);
}
#line 2512 "parser.c"
        break;
      case 118: /* atom ::= NAME */
#line 1064 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy167 = Variable_new(env, lineno, id);
}
#line 2521 "parser.c"
        break;
      case 119: /* atom ::= NUMBER */
      case 120: /* atom ::= REGEXP */
      case 121: /* atom ::= STRING */
      case 122: /* atom ::= SYMBOL */
#line 1069 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy167 = Literal_new(env, lineno, val);
}
#line 2533 "parser.c"
        break;
      case 123: /* atom ::= NIL */
#line 1089 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy167 = Literal_new(env, lineno, YNIL);
}
#line 2541 "parser.c"
        break;
      case 124: /* atom ::= TRUE */
#line 1093 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy167 = Literal_new(env, lineno, YTRUE);
}
#line 2549 "parser.c"
        break;
      case 125: /* atom ::= FALSE */
#line 1097 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy167 = Literal_new(env, lineno, YFALSE);
}
#line 2557 "parser.c"
        break;
      case 126: /* atom ::= LINE */
#line 1101 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy167 = Literal_new(env, lineno, val);
}
#line 2566 "parser.c"
        break;
      case 127: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1106 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy167 = Array_new(env, lineno, yymsp[-1].minor.yy167);
}
#line 2574 "parser.c"
        break;
      case 128: /* atom ::= LPAR expr RPAR */
      case 135: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1110 "parser.y"
{
    yygotominor.yy167 = yymsp[-1].minor.yy167;
}
#line 2582 "parser.c"
        break;
      case 132: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 133: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1124 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy167 = BlockArg_new(env, lineno, yymsp[-3].minor.yy167, yymsp[-1].minor.yy167);
}
#line 2591 "parser.c"
        break;
      case 137: /* excepts ::= excepts except */
#line 1143 "parser.y"
{
    yygotominor.yy167 = Array_push(env, yymsp[-1].minor.yy167, yymsp[0].minor.yy167);
}
#line 2598 "parser.c"
        break;
      case 138: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1147 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy167 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy167, id, yymsp[0].minor.yy167);
}
#line 2608 "parser.c"
        break;
      case 139: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1153 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy167 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy167, NO_EXC_VAR, yymsp[0].minor.yy167);
}
#line 2616 "parser.c"
        break;
      case 140: /* except ::= EXCEPT NEWLINE stmts */
#line 1157 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy167 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy167);
}
#line 2624 "parser.c"
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
