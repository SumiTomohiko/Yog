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
#define YYNOCODE 103
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy43;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 240
#define YYNRULE 144
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
 /*     0 */     1,  127,  128,   84,   20,   21,   24,   25,   26,  112,
 /*    10 */   181,   69,   56,   99,  145,   65,   59,  120,   23,  179,
 /*    20 */   167,  180,  126,  207,  385,   85,  154,  152,  153,  132,
 /*    30 */   217,   43,   44,  125,  129,  210,   45,   18,  214,  182,
 /*    40 */   183,  184,  185,  186,  187,  188,  189,  158,   83,  170,
 /*    50 */   160,  161,  162,   60,  237,  101,  102,   64,  103,   16,
 /*    60 */    65,   59,  156,   84,  179,  167,  180,   48,  154,  152,
 /*    70 */   153,   98,   64,  103,   16,   65,   59,   49,    3,  179,
 /*    80 */   167,  180,  124,  131,  133,  219,  136,  222,  220,  158,
 /*    90 */    83,  170,  160,  161,  162,   60,  203,  101,  102,   64,
 /*   100 */   103,  144,   65,   59,   84,   17,  179,  167,  180,   61,
 /*   110 */   154,  152,  153,   62,  103,   81,   65,   59,  239,   16,
 /*   120 */   179,  167,  180,   15,   30,  127,  128,  130,  139,  226,
 /*   130 */    16,  158,   83,  170,  160,  161,  162,   60,  109,  101,
 /*   140 */   102,   64,  103,  197,   65,   59,  116,  119,  179,  167,
 /*   150 */   180,   94,  154,  152,  153,  235,   28,   76,  122,  123,
 /*   160 */   134,  138,  140,  228,  234,   33,  220,  127,  128,  130,
 /*   170 */    19,  171,  172,  158,   83,  170,  160,  161,  162,   60,
 /*   180 */   231,  101,  102,   64,  103,   42,   65,   59,  211,  212,
 /*   190 */   179,  167,  180,   86,  154,  152,  153,  173,  174,   73,
 /*   200 */   141,  123,  134,  138,  140,  228,  240,   16,  220,  127,
 /*   210 */   128,  130,   67,   82,   93,  158,   83,  170,  160,  161,
 /*   220 */   162,   60,  198,  101,  102,   64,  103,  197,   65,   59,
 /*   230 */    84,   32,  179,  167,  180,   87,  154,  152,  153,  135,
 /*   240 */   137,  224,   63,   59,  214,   13,  179,  167,  180,   16,
 /*   250 */    16,   16,  193,  199,  204,   36,  192,  158,   83,  170,
 /*   260 */   160,  161,  162,   60,   40,  101,  102,   64,  103,   38,
 /*   270 */    65,   59,   84,  107,  179,  167,  180,   50,  154,  152,
 /*   280 */   153,  175,   29,  144,    2,   58,    3,   17,  179,  167,
 /*   290 */   180,  113,   31,  176,  177,  178,  201,  146,  114,  158,
 /*   300 */    83,  170,  160,  161,  162,   60,   30,  101,  102,   64,
 /*   310 */   103,  117,   65,   59,   84,  127,  179,  167,  180,   51,
 /*   320 */   154,  152,  153,   16,   16,    8,  205,  238,  209,  215,
 /*   330 */   163,  167,  180,   19,  216,  218,  142,  221,  155,  223,
 /*   340 */    27,  158,   83,  170,  160,  161,  162,   60,  225,  101,
 /*   350 */   102,   64,  103,  227,   65,   59,   84,   16,  179,  167,
 /*   360 */   180,  111,  154,  152,  153,    4,   33,   35,   34,   39,
 /*   370 */    22,  190,  164,  167,  180,  191,    5,    6,   66,    7,
 /*   380 */    68,  196,    9,  158,   83,  170,  160,  161,  162,   60,
 /*   390 */   118,  101,  102,   64,  103,  115,   65,   59,   84,  200,
 /*   400 */   179,  167,  180,   88,  154,  152,  153,   70,  202,  121,
 /*   410 */    37,  206,   10,   41,  165,  167,  180,   46,   52,   71,
 /*   420 */   208,   72,   57,  230,   53,  158,   83,  170,  160,  161,
 /*   430 */   162,   60,   74,  101,  102,   64,  103,   75,   65,   59,
 /*   440 */    84,   47,  179,  167,  180,   89,  154,  152,  153,   54,
 /*   450 */    77,   78,   55,   79,   80,   11,  166,  167,  180,  232,
 /*   460 */   233,  236,  147,   12,  386,  386,  386,  158,   83,  170,
 /*   470 */   160,  161,  162,   60,  386,  101,  102,   64,  103,  386,
 /*   480 */    65,   59,  386,  386,  179,  167,  180,   90,  154,  152,
 /*   490 */   153,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   500 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  158,
 /*   510 */    83,  170,  160,  161,  162,   60,  386,  101,  102,   64,
 /*   520 */   103,  386,   65,   59,  386,  386,  179,  167,  180,  148,
 /*   530 */   154,  152,  153,  386,  386,  386,  386,  386,  386,  386,
 /*   540 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   550 */   386,  158,   83,  170,  160,  161,  162,   60,  386,  101,
 /*   560 */   102,   64,  103,  386,   65,   59,  386,  386,  179,  167,
 /*   570 */   180,  149,  154,  152,  153,  386,  386,  386,  386,  386,
 /*   580 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   590 */   386,  386,  386,  158,   83,  170,  160,  161,  162,   60,
 /*   600 */   386,  101,  102,   64,  103,  386,   65,   59,  386,  386,
 /*   610 */   179,  167,  180,  150,  154,  152,  153,  386,  386,  386,
 /*   620 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   630 */   386,  386,  386,  386,  386,  158,   83,  170,  160,  161,
 /*   640 */   162,   60,  386,  101,  102,   64,  103,  386,   65,   59,
 /*   650 */   386,  386,  179,  167,  180,   92,  154,  152,  153,  386,
 /*   660 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   670 */   386,  386,  386,  386,  386,  386,  386,  158,   83,  170,
 /*   680 */   160,  161,  162,   60,  386,  101,  102,   64,  103,  386,
 /*   690 */    65,   59,  386,   84,  179,  167,  180,  151,  152,  153,
 /*   700 */    97,  102,   64,  103,  386,   65,   59,  386,  386,  179,
 /*   710 */   167,  180,  386,  386,  386,  386,  386,  386,  158,   83,
 /*   720 */   170,  160,  161,  162,   60,  386,  101,  102,   64,  103,
 /*   730 */   386,   65,   59,  386,   84,  179,  167,  180,  168,   96,
 /*   740 */   386,  101,  102,   64,  103,  386,   65,   59,  386,  386,
 /*   750 */   179,  167,  180,  386,  386,  386,  100,  158,   83,  170,
 /*   760 */   160,  161,  162,   60,  386,  101,  102,   64,  103,  386,
 /*   770 */    65,   59,  386,  386,  179,  167,  180,  106,  168,  386,
 /*   780 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   790 */   386,  386,  386,  386,  386,  386,  100,  158,   83,  170,
 /*   800 */   160,  161,  162,   60,  386,  101,  102,   64,  103,  386,
 /*   810 */    65,   59,  386,  386,  179,  167,  180,  104,   91,  386,
 /*   820 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   830 */   386,  386,  386,  386,  386,  386,  386,  158,   83,  170,
 /*   840 */   160,  161,  162,   60,  386,  101,  102,   64,  103,  386,
 /*   850 */    65,   59,  386,  386,  179,  167,  180,   95,  386,  386,
 /*   860 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   870 */   386,  386,  386,  386,  386,  386,  158,   83,  170,  160,
 /*   880 */   161,  162,   60,  386,  101,  102,   64,  103,  386,   65,
 /*   890 */    59,  386,  386,  179,  167,  180,  386,  157,  386,  386,
 /*   900 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   910 */   386,  386,  386,  386,  386,  386,  158,   83,  170,  160,
 /*   920 */   161,  162,   60,  386,  101,  102,   64,  103,  386,   65,
 /*   930 */    59,  386,  386,  179,  167,  180,  169,  386,  386,  386,
 /*   940 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   950 */   386,  386,  386,  386,  386,  158,   83,  170,  160,  161,
 /*   960 */   162,   60,  386,  101,  102,   64,  103,  386,   65,   59,
 /*   970 */   386,  386,  179,  167,  180,  386,  105,  386,  386,  386,
 /*   980 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*   990 */   386,  386,  386,  386,  386,  158,   83,  170,  160,  161,
 /*  1000 */   162,   60,  386,  101,  102,   64,  103,  386,   65,   59,
 /*  1010 */   386,  386,  179,  167,  180,  194,  386,  386,  386,  386,
 /*  1020 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*  1030 */   386,  386,  386,  386,  158,   83,  170,  160,  161,  162,
 /*  1040 */    60,  386,  101,  102,   64,  103,  386,   65,   59,  386,
 /*  1050 */   386,  179,  167,  180,  386,  195,  386,  386,  386,  386,
 /*  1060 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*  1070 */   386,  386,  386,  386,  158,   83,  170,  160,  161,  162,
 /*  1080 */    60,  386,  101,  102,   64,  103,  386,   65,   59,  386,
 /*  1090 */   386,  179,  167,  180,  108,  386,  386,  386,  386,  386,
 /*  1100 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*  1110 */   386,  386,  386,  158,   83,  170,  160,  161,  162,   60,
 /*  1120 */   386,  101,  102,   64,  103,  386,   65,   59,  386,  386,
 /*  1130 */   179,  167,  180,  386,  110,  386,  386,  386,  386,  386,
 /*  1140 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*  1150 */   386,  386,  386,  158,   83,  170,  160,  161,  162,   60,
 /*  1160 */   386,  101,  102,   64,  103,  386,   65,   59,  386,  386,
 /*  1170 */   179,  167,  180,  213,  386,  386,  386,  386,  386,  386,
 /*  1180 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*  1190 */   386,  386,  158,   83,  170,  160,  161,  162,   60,  386,
 /*  1200 */   101,  102,   64,  103,  386,   65,   59,  386,  386,  179,
 /*  1210 */   167,  180,  386,  229,  386,  386,  386,  386,  386,  386,
 /*  1220 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*  1230 */   386,  386,  158,   83,  170,  160,  161,  162,   60,  386,
 /*  1240 */   101,  102,   64,  103,  386,   65,   59,  386,  386,  179,
 /*  1250 */   167,  180,  143,  386,  386,  386,  386,  386,  386,  386,
 /*  1260 */   386,  386,  386,  386,  386,  386,  386,  386,  386,  386,
 /*  1270 */   386,  158,   83,  170,  160,  161,  162,   60,  386,  101,
 /*  1280 */   102,   64,  103,   14,   65,   59,  386,  386,  179,  167,
 /*  1290 */   180,  386,  386,  386,  181,  386,  386,  386,  386,  386,
 /*  1300 */   386,  386,   23,  386,  386,  386,  386,  386,  386,  386,
 /*  1310 */   386,  386,  386,  386,  386,   43,   44,  386,  386,  386,
 /*  1320 */    45,   18,  386,  182,  183,  184,  185,  186,  187,  188,
 /*  1330 */   189,   84,  159,  160,  161,  162,   60,  181,  101,  102,
 /*  1340 */    64,  103,  386,   65,   59,   23,  386,  179,  167,  180,
 /*  1350 */   386,  386,  386,  386,  386,  386,  386,  386,   43,   44,
 /*  1360 */   386,  386,  386,   45,   18,  386,  182,  183,  184,  185,
 /*  1370 */   186,  187,  188,  189,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   79,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   89,   61,   91,   92,   19,   20,   95,
 /*    20 */    96,   97,   72,   73,   55,   56,   57,   58,   59,   72,
 /*    30 */    73,   33,   34,   71,   72,   73,   38,   39,   76,   41,
 /*    40 */    42,   43,   44,   45,   46,   47,   48,   78,   79,   80,
 /*    50 */    81,   82,   83,   84,  101,   86,   87,   88,   89,    1,
 /*    60 */    91,   92,    4,   79,   95,   96,   97,   56,   57,   58,
 /*    70 */    59,   87,   88,   89,    1,   91,   92,   60,    5,   95,
 /*    80 */    96,   97,   70,   71,   72,   73,   72,   73,   76,   78,
 /*    90 */    79,   80,   81,   82,   83,   84,   12,   86,   87,   88,
 /*   100 */    89,   16,   91,   92,   79,   20,   95,   96,   97,   56,
 /*   110 */    57,   58,   59,   88,   89,   12,   91,   92,  101,    1,
 /*   120 */    95,   96,   97,    5,   39,   22,   23,   24,   72,   73,
 /*   130 */     1,   78,   79,   80,   81,   82,   83,   84,   62,   86,
 /*   140 */    87,   88,   89,   67,   91,   92,   65,   66,   95,   96,
 /*   150 */    97,   56,   57,   58,   59,   17,   25,   12,   68,   69,
 /*   160 */    70,   71,   72,   73,   26,   27,   76,   22,   23,   24,
 /*   170 */    52,   30,   31,   78,   79,   80,   81,   82,   83,   84,
 /*   180 */    51,   86,   87,   88,   89,   94,   91,   92,   74,   75,
 /*   190 */    95,   96,   97,   56,   57,   58,   59,   33,   34,   12,
 /*   200 */    68,   69,   70,   71,   72,   73,    0,    1,   76,   22,
 /*   210 */    23,   24,   49,   50,   61,   78,   79,   80,   81,   82,
 /*   220 */    83,   84,   62,   86,   87,   88,   89,   67,   91,   92,
 /*   230 */    79,   85,   95,   96,   97,   56,   57,   58,   59,   71,
 /*   240 */    72,   73,   91,   92,   76,    1,   95,   96,   97,    1,
 /*   250 */     1,    1,    4,    4,    4,   90,   99,   78,   79,   80,
 /*   260 */    81,   82,   83,   84,   93,   86,   87,   88,   89,   39,
 /*   270 */    91,   92,   79,  100,   95,   96,   97,   56,   57,   58,
 /*   280 */    59,   23,   17,   16,    3,   92,    5,   20,   95,   96,
 /*   290 */    97,   63,   25,   35,   36,   37,   12,   53,   64,   78,
 /*   300 */    79,   80,   81,   82,   83,   84,   39,   86,   87,   88,
 /*   310 */    89,   66,   91,   92,   79,   22,   95,   96,   97,   56,
 /*   320 */    57,   58,   59,    1,    1,    3,   73,    4,   73,   75,
 /*   330 */    95,   96,   97,   52,   73,   73,  100,   73,    4,   73,
 /*   340 */    18,   78,   79,   80,   81,   82,   83,   84,   73,   86,
 /*   350 */    87,   88,   89,   73,   91,   92,   79,    1,   95,   96,
 /*   360 */    97,   56,   57,   58,   59,    1,   27,   29,   28,   32,
 /*   370 */    15,   40,   95,   96,   97,   21,    1,    1,   21,    1,
 /*   380 */    12,    4,    1,   78,   79,   80,   81,   82,   83,   84,
 /*   390 */    16,   86,   87,   88,   89,   15,   91,   92,   79,   12,
 /*   400 */    95,   96,   97,   56,   57,   58,   59,   15,   12,   12,
 /*   410 */    20,   12,   21,   15,   95,   96,   97,   15,   15,   15,
 /*   420 */    12,   15,   12,   40,   15,   78,   79,   80,   81,   82,
 /*   430 */    83,   84,   15,   86,   87,   88,   89,   15,   91,   92,
 /*   440 */    79,   15,   95,   96,   97,   56,   57,   58,   59,   15,
 /*   450 */    15,   15,   15,   15,   15,    1,   95,   96,   97,   40,
 /*   460 */    12,    4,   12,    1,  102,  102,  102,   78,   79,   80,
 /*   470 */    81,   82,   83,   84,  102,   86,   87,   88,   89,  102,
 /*   480 */    91,   92,  102,  102,   95,   96,   97,   56,   57,   58,
 /*   490 */    59,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   500 */   102,  102,  102,  102,  102,  102,  102,  102,  102,   78,
 /*   510 */    79,   80,   81,   82,   83,   84,  102,   86,   87,   88,
 /*   520 */    89,  102,   91,   92,  102,  102,   95,   96,   97,   56,
 /*   530 */    57,   58,   59,  102,  102,  102,  102,  102,  102,  102,
 /*   540 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   550 */   102,   78,   79,   80,   81,   82,   83,   84,  102,   86,
 /*   560 */    87,   88,   89,  102,   91,   92,  102,  102,   95,   96,
 /*   570 */    97,   56,   57,   58,   59,  102,  102,  102,  102,  102,
 /*   580 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   590 */   102,  102,  102,   78,   79,   80,   81,   82,   83,   84,
 /*   600 */   102,   86,   87,   88,   89,  102,   91,   92,  102,  102,
 /*   610 */    95,   96,   97,   56,   57,   58,   59,  102,  102,  102,
 /*   620 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   630 */   102,  102,  102,  102,  102,   78,   79,   80,   81,   82,
 /*   640 */    83,   84,  102,   86,   87,   88,   89,  102,   91,   92,
 /*   650 */   102,  102,   95,   96,   97,   56,   57,   58,   59,  102,
 /*   660 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   670 */   102,  102,  102,  102,  102,  102,  102,   78,   79,   80,
 /*   680 */    81,   82,   83,   84,  102,   86,   87,   88,   89,  102,
 /*   690 */    91,   92,  102,   79,   95,   96,   97,   57,   58,   59,
 /*   700 */    86,   87,   88,   89,  102,   91,   92,  102,  102,   95,
 /*   710 */    96,   97,  102,  102,  102,  102,  102,  102,   78,   79,
 /*   720 */    80,   81,   82,   83,   84,  102,   86,   87,   88,   89,
 /*   730 */   102,   91,   92,  102,   79,   95,   96,   97,   59,   84,
 /*   740 */   102,   86,   87,   88,   89,  102,   91,   92,  102,  102,
 /*   750 */    95,   96,   97,  102,  102,  102,   77,   78,   79,   80,
 /*   760 */    81,   82,   83,   84,  102,   86,   87,   88,   89,  102,
 /*   770 */    91,   92,  102,  102,   95,   96,   97,   98,   59,  102,
 /*   780 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   790 */   102,  102,  102,  102,  102,  102,   77,   78,   79,   80,
 /*   800 */    81,   82,   83,   84,  102,   86,   87,   88,   89,  102,
 /*   810 */    91,   92,  102,  102,   95,   96,   97,   98,   59,  102,
 /*   820 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   830 */   102,  102,  102,  102,  102,  102,  102,   78,   79,   80,
 /*   840 */    81,   82,   83,   84,  102,   86,   87,   88,   89,  102,
 /*   850 */    91,   92,  102,  102,   95,   96,   97,   59,  102,  102,
 /*   860 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   870 */   102,  102,  102,  102,  102,  102,   78,   79,   80,   81,
 /*   880 */    82,   83,   84,  102,   86,   87,   88,   89,  102,   91,
 /*   890 */    92,  102,  102,   95,   96,   97,  102,   59,  102,  102,
 /*   900 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   910 */   102,  102,  102,  102,  102,  102,   78,   79,   80,   81,
 /*   920 */    82,   83,   84,  102,   86,   87,   88,   89,  102,   91,
 /*   930 */    92,  102,  102,   95,   96,   97,   59,  102,  102,  102,
 /*   940 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   950 */   102,  102,  102,  102,  102,   78,   79,   80,   81,   82,
 /*   960 */    83,   84,  102,   86,   87,   88,   89,  102,   91,   92,
 /*   970 */   102,  102,   95,   96,   97,  102,   59,  102,  102,  102,
 /*   980 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*   990 */   102,  102,  102,  102,  102,   78,   79,   80,   81,   82,
 /*  1000 */    83,   84,  102,   86,   87,   88,   89,  102,   91,   92,
 /*  1010 */   102,  102,   95,   96,   97,   59,  102,  102,  102,  102,
 /*  1020 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*  1030 */   102,  102,  102,  102,   78,   79,   80,   81,   82,   83,
 /*  1040 */    84,  102,   86,   87,   88,   89,  102,   91,   92,  102,
 /*  1050 */   102,   95,   96,   97,  102,   59,  102,  102,  102,  102,
 /*  1060 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*  1070 */   102,  102,  102,  102,   78,   79,   80,   81,   82,   83,
 /*  1080 */    84,  102,   86,   87,   88,   89,  102,   91,   92,  102,
 /*  1090 */   102,   95,   96,   97,   59,  102,  102,  102,  102,  102,
 /*  1100 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*  1110 */   102,  102,  102,   78,   79,   80,   81,   82,   83,   84,
 /*  1120 */   102,   86,   87,   88,   89,  102,   91,   92,  102,  102,
 /*  1130 */    95,   96,   97,  102,   59,  102,  102,  102,  102,  102,
 /*  1140 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*  1150 */   102,  102,  102,   78,   79,   80,   81,   82,   83,   84,
 /*  1160 */   102,   86,   87,   88,   89,  102,   91,   92,  102,  102,
 /*  1170 */    95,   96,   97,   59,  102,  102,  102,  102,  102,  102,
 /*  1180 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*  1190 */   102,  102,   78,   79,   80,   81,   82,   83,   84,  102,
 /*  1200 */    86,   87,   88,   89,  102,   91,   92,  102,  102,   95,
 /*  1210 */    96,   97,  102,   59,  102,  102,  102,  102,  102,  102,
 /*  1220 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*  1230 */   102,  102,   78,   79,   80,   81,   82,   83,   84,  102,
 /*  1240 */    86,   87,   88,   89,  102,   91,   92,  102,  102,   95,
 /*  1250 */    96,   97,   59,  102,  102,  102,  102,  102,  102,  102,
 /*  1260 */   102,  102,  102,  102,  102,  102,  102,  102,  102,  102,
 /*  1270 */   102,   78,   79,   80,   81,   82,   83,   84,  102,   86,
 /*  1280 */    87,   88,   89,    1,   91,   92,  102,  102,   95,   96,
 /*  1290 */    97,  102,  102,  102,   12,  102,  102,  102,  102,  102,
 /*  1300 */   102,  102,   20,  102,  102,  102,  102,  102,  102,  102,
 /*  1310 */   102,  102,  102,  102,  102,   33,   34,  102,  102,  102,
 /*  1320 */    38,   39,  102,   41,   42,   43,   44,   45,   46,   47,
 /*  1330 */    48,   79,   80,   81,   82,   83,   84,   12,   86,   87,
 /*  1340 */    88,   89,  102,   91,   92,   20,  102,   95,   96,   97,
 /*  1350 */   102,  102,  102,  102,  102,  102,  102,  102,   33,   34,
 /*  1360 */   102,  102,  102,   38,   39,  102,   41,   42,   43,   44,
 /*  1370 */    45,   46,   47,   48,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 150
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2, 1325, 1325, 1282,
 /*    20 */  1325, 1325, 1325, 1325, 1325, 1325, 1325, 1325, 1325, 1325,
 /*    30 */  1325, 1325, 1325, 1325, 1325, 1325, 1325,  103,  103, 1325,
 /*    40 */  1325,  145, 1325, 1325, 1325, 1325,  187,  187,  118,  281,
 /*    50 */   322,  322,  -21,  -21,  -21,  -21,   84,  131,  258,  258,
 /*    60 */   138,   73,  141,  164,  141,  164,  163,  230,  265,  284,
 /*    70 */    84,  293,  293,  131,  293,  293,  131,  293,  293,  293,
 /*    80 */   293,  131,  230,  267,   85,  206,   58,  248,  249,  250,
 /*    90 */   129,  244,  323,  334,  356,  364,  339,  340,  338,  337,
 /*   100 */   355,  340,  338,  337,  331,  354,  357,  375,  376,  377,
 /*   110 */   378,  356,  368,  381,  380,  387,  392,  374,  396,  374,
 /*   120 */   397,  390,  391,  398,  402,  403,  404,  399,  408,  406,
 /*   130 */   410,  409,  417,  422,  426,  434,  435,  436,  437,  438,
 /*   140 */   439,  383,  454,  419,  448,  457,  450,  462,  356,  356,
 /*   150 */   356,
};
#define YY_REDUCE_USE_DFLT (-77)
#define YY_REDUCE_MAX 82
static const short yy_reduce_ofst[] = {
 /*     0 */   -31,   11,   53,   95,  137,  179,  221,  263,  305,  347,
 /*    10 */   389,  431,  473,  515,  557,  599,  640,  679,  719,  759,
 /*    20 */   798,  838,  877,  917,  956,  996, 1035, 1075, 1114, 1154,
 /*    30 */  1193, 1252,  655,  614,  -16,   25,  -76,   90,  132,  151,
 /*    40 */   193,   12,  235,  277,  319,  361,  -38,  168,   17,  -47,
 /*    50 */    76,  160,  -50,  -43,   14,   56,   81,  114,   91,   91,
 /*    60 */   146,  153,  165,  171,  165,  171,  157,  173,  228,  234,
 /*    70 */   245,  253,  255,  254,  261,  262,  254,  264,  266,  275,
 /*    80 */   280,  254,  236,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   243,  243,  243,  243,  243,  243,  243,  243,  243,  243,
 /*    10 */   243,  243,  243,  243,  243,  243,  243,  370,  370,  384,
 /*    20 */   384,  250,  384,  384,  252,  254,  384,  384,  384,  384,
 /*    30 */   384,  384,  384,  384,  384,  384,  384,  304,  304,  384,
 /*    40 */   384,  384,  384,  384,  384,  384,  384,  384,  384,  382,
 /*    50 */   270,  270,  384,  384,  384,  384,  384,  308,  341,  340,
 /*    60 */   324,  382,  333,  339,  332,  338,  372,  375,  266,  384,
 /*    70 */   384,  384,  384,  384,  384,  384,  312,  384,  384,  384,
 /*    80 */   384,  311,  375,  354,  354,  384,  384,  384,  384,  384,
 /*    90 */   384,  384,  384,  384,  383,  384,  325,  329,  331,  335,
 /*   100 */   371,  328,  330,  334,  384,  384,  384,  384,  384,  384,
 /*   110 */   384,  271,  384,  384,  258,  384,  259,  261,  384,  260,
 /*   120 */   384,  384,  384,  288,  280,  276,  274,  384,  384,  278,
 /*   130 */   384,  284,  282,  286,  296,  292,  290,  294,  300,  298,
 /*   140 */   302,  384,  384,  384,  384,  384,  384,  384,  379,  380,
 /*   150 */   381,  242,  244,  245,  241,  246,  249,  251,  318,  319,
 /*   160 */   321,  322,  323,  344,  350,  351,  352,  353,  316,  317,
 /*   170 */   320,  336,  337,  342,  343,  346,  347,  348,  349,  345,
 /*   180 */   355,  359,  360,  361,  362,  363,  364,  365,  366,  367,
 /*   190 */   368,  369,  356,  373,  253,  255,  256,  268,  269,  257,
 /*   200 */   265,  264,  263,  262,  272,  273,  305,  275,  306,  277,
 /*   210 */   279,  307,  309,  310,  314,  315,  281,  283,  285,  287,
 /*   220 */   313,  289,  291,  293,  295,  297,  299,  301,  303,  267,
 /*   230 */   376,  374,  357,  358,  326,  327,  247,  378,  248,  377,
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
  "DIV_DIV",       "PERCENT",       "TILDA",         "LBRACKET",    
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
 /* 112 */ "factor ::= TILDA factor",
 /* 113 */ "factor ::= power",
 /* 114 */ "power ::= postfix_expr",
 /* 115 */ "postfix_expr ::= atom",
 /* 116 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 117 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 118 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 119 */ "atom ::= NAME",
 /* 120 */ "atom ::= NUMBER",
 /* 121 */ "atom ::= REGEXP",
 /* 122 */ "atom ::= STRING",
 /* 123 */ "atom ::= SYMBOL",
 /* 124 */ "atom ::= NIL",
 /* 125 */ "atom ::= TRUE",
 /* 126 */ "atom ::= FALSE",
 /* 127 */ "atom ::= LINE",
 /* 128 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 129 */ "atom ::= LPAR expr RPAR",
 /* 130 */ "args_opt ::=",
 /* 131 */ "args_opt ::= args",
 /* 132 */ "blockarg_opt ::=",
 /* 133 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 134 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 135 */ "blockarg_params_opt ::=",
 /* 136 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 137 */ "excepts ::= except",
 /* 138 */ "excepts ::= excepts except",
 /* 139 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 140 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 141 */ "except ::= EXCEPT NEWLINE stmts",
 /* 142 */ "finally_opt ::=",
 /* 143 */ "finally_opt ::= FINALLY stmts",
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
  { 55, 1 },
  { 56, 1 },
  { 56, 3 },
  { 57, 0 },
  { 57, 1 },
  { 57, 1 },
  { 57, 7 },
  { 57, 5 },
  { 57, 5 },
  { 57, 5 },
  { 57, 1 },
  { 57, 2 },
  { 57, 1 },
  { 57, 2 },
  { 57, 1 },
  { 57, 2 },
  { 57, 6 },
  { 57, 6 },
  { 57, 2 },
  { 57, 2 },
  { 65, 1 },
  { 65, 3 },
  { 66, 1 },
  { 66, 3 },
  { 64, 1 },
  { 64, 3 },
  { 63, 0 },
  { 63, 2 },
  { 62, 1 },
  { 62, 5 },
  { 67, 0 },
  { 67, 2 },
  { 58, 7 },
  { 68, 9 },
  { 68, 7 },
  { 68, 7 },
  { 68, 5 },
  { 68, 7 },
  { 68, 5 },
  { 68, 5 },
  { 68, 3 },
  { 68, 7 },
  { 68, 5 },
  { 68, 5 },
  { 68, 3 },
  { 68, 5 },
  { 68, 3 },
  { 68, 3 },
  { 68, 1 },
  { 68, 7 },
  { 68, 5 },
  { 68, 5 },
  { 68, 3 },
  { 68, 5 },
  { 68, 3 },
  { 68, 3 },
  { 68, 1 },
  { 68, 5 },
  { 68, 3 },
  { 68, 3 },
  { 68, 1 },
  { 68, 3 },
  { 68, 1 },
  { 68, 1 },
  { 68, 0 },
  { 73, 2 },
  { 72, 2 },
  { 71, 3 },
  { 74, 0 },
  { 74, 1 },
  { 75, 2 },
  { 69, 1 },
  { 69, 3 },
  { 70, 1 },
  { 70, 3 },
  { 76, 2 },
  { 77, 1 },
  { 77, 3 },
  { 59, 1 },
  { 78, 3 },
  { 78, 1 },
  { 80, 1 },
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
  { 87, 1 },
  { 87, 3 },
  { 88, 1 },
  { 88, 3 },
  { 90, 1 },
  { 90, 1 },
  { 89, 1 },
  { 89, 3 },
  { 91, 1 },
  { 91, 3 },
  { 93, 1 },
  { 93, 1 },
  { 92, 3 },
  { 92, 1 },
  { 94, 1 },
  { 94, 1 },
  { 94, 1 },
  { 94, 1 },
  { 95, 2 },
  { 95, 2 },
  { 95, 2 },
  { 95, 1 },
  { 96, 1 },
  { 79, 1 },
  { 79, 5 },
  { 79, 4 },
  { 79, 3 },
  { 97, 1 },
  { 97, 1 },
  { 97, 1 },
  { 97, 1 },
  { 97, 1 },
  { 97, 1 },
  { 97, 1 },
  { 97, 1 },
  { 97, 1 },
  { 97, 3 },
  { 97, 3 },
  { 98, 0 },
  { 98, 1 },
  { 99, 0 },
  { 99, 5 },
  { 99, 5 },
  { 100, 0 },
  { 100, 3 },
  { 60, 1 },
  { 60, 2 },
  { 101, 6 },
  { 101, 4 },
  { 101, 3 },
  { 61, 0 },
  { 61, 2 },
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
    *pval = yymsp[0].minor.yy43;
}
#line 1919 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 137: /* excepts ::= except */
#line 626 "parser.y"
{
    yygotominor.yy43 = make_array_with(env, yymsp[0].minor.yy43);
}
#line 1930 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 629 "parser.y"
{
    yygotominor.yy43 = Array_push(env, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 1940 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 130: /* args_opt ::= */
      case 132: /* blockarg_opt ::= */
      case 135: /* blockarg_params_opt ::= */
      case 142: /* finally_opt ::= */
#line 633 "parser.y"
{
    yygotominor.yy43 = YNIL;
}
#line 1954 "parser.c"
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
      case 113: /* factor ::= power */
      case 114: /* power ::= postfix_expr */
      case 115: /* postfix_expr ::= atom */
      case 131: /* args_opt ::= args */
      case 143: /* finally_opt ::= FINALLY stmts */
#line 636 "parser.y"
{
    yygotominor.yy43 = yymsp[0].minor.yy43;
}
#line 1985 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 642 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy43 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy43, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, yymsp[-1].minor.yy43);
}
#line 1993 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 646 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy43 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy43, yymsp[-2].minor.yy43, YNIL, yymsp[-1].minor.yy43);
}
#line 2001 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 650 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy43 = Finally_new(env, lineno, yymsp[-3].minor.yy43, yymsp[-1].minor.yy43);
}
#line 2009 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 654 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy43 = While_new(env, lineno, yymsp[-3].minor.yy43, yymsp[-1].minor.yy43);
}
#line 2017 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 658 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy43 = Break_new(env, lineno, YNIL);
}
#line 2025 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 662 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy43 = Break_new(env, lineno, yymsp[0].minor.yy43);
}
#line 2033 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 666 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy43 = Next_new(env, lineno, YNIL);
}
#line 2041 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 670 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy43 = Next_new(env, lineno, yymsp[0].minor.yy43);
}
#line 2049 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 674 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy43 = Return_new(env, lineno, YNIL);
}
#line 2057 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 678 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy43 = Return_new(env, lineno, yymsp[0].minor.yy43);
}
#line 2065 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 682 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy43 = If_new(env, lineno, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, yymsp[-1].minor.yy43);
}
#line 2073 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 686 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy43 = Klass_new(env, lineno, id, yymsp[-3].minor.yy43, yymsp[-1].minor.yy43);
}
#line 2082 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 691 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy43 = Nonlocal_new(env, lineno, yymsp[0].minor.yy43);
}
#line 2090 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 695 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy43 = Import_new(env, lineno, yymsp[0].minor.yy43);
}
#line 2098 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 707 "parser.y"
{
    yygotominor.yy43 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2106 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 710 "parser.y"
{
    yygotominor.yy43 = Array_push_token_id(env, yymsp[-2].minor.yy43, yymsp[0].minor.yy0);
}
#line 2114 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 731 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy43, yymsp[-1].minor.yy43, yymsp[0].minor.yy43);
    yygotominor.yy43 = make_array_with(env, node);
}
#line 2123 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 744 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy43 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy43, yymsp[-1].minor.yy43);
}
#line 2132 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 750 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-8].minor.yy43, yymsp[-6].minor.yy43, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 2139 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 753 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-6].minor.yy43, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, yymsp[0].minor.yy43, YNIL);
}
#line 2146 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 756 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-6].minor.yy43, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, YNIL, yymsp[0].minor.yy43);
}
#line 2153 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 759 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, yymsp[0].minor.yy43, YNIL, YNIL);
}
#line 2160 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 762 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-6].minor.yy43, yymsp[-4].minor.yy43, YNIL, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 2167 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 765 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, YNIL, yymsp[0].minor.yy43, YNIL);
}
#line 2174 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 768 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, YNIL, YNIL, yymsp[0].minor.yy43);
}
#line 2181 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 771 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-2].minor.yy43, yymsp[0].minor.yy43, YNIL, YNIL, YNIL);
}
#line 2188 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 774 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-6].minor.yy43, YNIL, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 2195 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 777 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-4].minor.yy43, YNIL, yymsp[-2].minor.yy43, yymsp[0].minor.yy43, YNIL);
}
#line 2202 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 780 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-4].minor.yy43, YNIL, yymsp[-2].minor.yy43, YNIL, yymsp[0].minor.yy43);
}
#line 2209 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 783 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-2].minor.yy43, YNIL, yymsp[0].minor.yy43, YNIL, YNIL);
}
#line 2216 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 786 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-4].minor.yy43, YNIL, YNIL, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 2223 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 789 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-2].minor.yy43, YNIL, YNIL, yymsp[0].minor.yy43, YNIL);
}
#line 2230 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 792 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[-2].minor.yy43, YNIL, YNIL, YNIL, yymsp[0].minor.yy43);
}
#line 2237 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 795 "parser.y"
{
    yygotominor.yy43 = Params_new(env, yymsp[0].minor.yy43, YNIL, YNIL, YNIL, YNIL);
}
#line 2244 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 798 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, yymsp[-6].minor.yy43, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 2251 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 801 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, yymsp[0].minor.yy43, YNIL);
}
#line 2258 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 804 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, YNIL, yymsp[0].minor.yy43);
}
#line 2265 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 807 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, yymsp[-2].minor.yy43, yymsp[0].minor.yy43, YNIL, YNIL);
}
#line 2272 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 810 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, yymsp[-4].minor.yy43, YNIL, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 2279 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 813 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, yymsp[-2].minor.yy43, YNIL, yymsp[0].minor.yy43, YNIL);
}
#line 2286 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 816 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, yymsp[-2].minor.yy43, YNIL, YNIL, yymsp[0].minor.yy43);
}
#line 2293 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 819 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, yymsp[0].minor.yy43, YNIL, YNIL, YNIL);
}
#line 2300 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 822 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 2307 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 825 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy43, yymsp[0].minor.yy43, YNIL);
}
#line 2314 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 828 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy43, YNIL, yymsp[0].minor.yy43);
}
#line 2321 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 831 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy43, YNIL, YNIL);
}
#line 2328 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 834 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 2335 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 837 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy43, YNIL);
}
#line 2342 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 840 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy43);
}
#line 2349 "parser.c"
        break;
      case 64: /* params ::= */
#line 843 "parser.y"
{
    yygotominor.yy43 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2356 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 847 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy43 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2365 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 853 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy43 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2374 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 859 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy43 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy43);
}
#line 2383 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 876 "parser.y"
{
    yygotominor.yy43 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy43, lineno, id, YNIL);
}
#line 2393 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 882 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy43, lineno, id, YNIL);
    yygotominor.yy43 = yymsp[-2].minor.yy43;
}
#line 2403 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 896 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy43 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy43);
}
#line 2412 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 913 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy43);
    yygotominor.yy43 = Assign_new(env, lineno, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 2420 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 936 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy43);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy43)->u.id;
    yygotominor.yy43 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy43, id, yymsp[0].minor.yy43);
}
#line 2429 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
      case 87: /* comp_op ::= GREATER */
#line 942 "parser.y"
{
    yygotominor.yy43 = yymsp[0].minor.yy0;
}
#line 2437 "parser.c"
        break;
      case 89: /* xor_expr ::= xor_expr XOR or_expr */
      case 91: /* or_expr ::= or_expr BAR and_expr */
      case 93: /* and_expr ::= and_expr AND shift_expr */
#line 952 "parser.y"
{
    yygotominor.yy43 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy43), yymsp[-2].minor.yy43, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy43);
}
#line 2446 "parser.c"
        break;
      case 95: /* shift_expr ::= shift_expr shift_op match_expr */
      case 101: /* arith_expr ::= arith_expr arith_op term */
      case 104: /* term ::= term term_op factor */
#line 973 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy43);
    yygotominor.yy43 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy43, VAL2ID(yymsp[-1].minor.yy43), yymsp[0].minor.yy43);
}
#line 2456 "parser.c"
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
    yygotominor.yy43 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2470 "parser.c"
        break;
      case 99: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 988 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy43);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy43 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy43, id, yymsp[0].minor.yy43);
}
#line 2479 "parser.c"
        break;
      case 110: /* factor ::= PLUS factor */
#line 1030 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy43 = FuncCall_new3(env, lineno, yymsp[0].minor.yy43, id);
}
#line 2488 "parser.c"
        break;
      case 111: /* factor ::= MINUS factor */
#line 1035 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy43 = FuncCall_new3(env, lineno, yymsp[0].minor.yy43, id);
}
#line 2497 "parser.c"
        break;
      case 112: /* factor ::= TILDA factor */
#line 1040 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy43 = FuncCall_new3(env, lineno, yymsp[0].minor.yy43, id);
}
#line 2506 "parser.c"
        break;
      case 116: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1056 "parser.y"
{
    yygotominor.yy43 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy43), yymsp[-4].minor.yy43, yymsp[-2].minor.yy43, yymsp[0].minor.yy43);
}
#line 2513 "parser.c"
        break;
      case 117: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1059 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy43);
    yygotominor.yy43 = Subscript_new(env, lineno, yymsp[-3].minor.yy43, yymsp[-1].minor.yy43);
}
#line 2521 "parser.c"
        break;
      case 118: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1063 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy43);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy43 = Attr_new(env, lineno, yymsp[-2].minor.yy43, id);
}
#line 2530 "parser.c"
        break;
      case 119: /* atom ::= NAME */
#line 1069 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy43 = Variable_new(env, lineno, id);
}
#line 2539 "parser.c"
        break;
      case 120: /* atom ::= NUMBER */
      case 121: /* atom ::= REGEXP */
      case 122: /* atom ::= STRING */
      case 123: /* atom ::= SYMBOL */
#line 1074 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy43 = Literal_new(env, lineno, val);
}
#line 2551 "parser.c"
        break;
      case 124: /* atom ::= NIL */
#line 1094 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy43 = Literal_new(env, lineno, YNIL);
}
#line 2559 "parser.c"
        break;
      case 125: /* atom ::= TRUE */
#line 1098 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy43 = Literal_new(env, lineno, YTRUE);
}
#line 2567 "parser.c"
        break;
      case 126: /* atom ::= FALSE */
#line 1102 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy43 = Literal_new(env, lineno, YFALSE);
}
#line 2575 "parser.c"
        break;
      case 127: /* atom ::= LINE */
#line 1106 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy43 = Literal_new(env, lineno, val);
}
#line 2584 "parser.c"
        break;
      case 128: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1111 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy43 = Array_new(env, lineno, yymsp[-1].minor.yy43);
}
#line 2592 "parser.c"
        break;
      case 129: /* atom ::= LPAR expr RPAR */
      case 136: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1115 "parser.y"
{
    yygotominor.yy43 = yymsp[-1].minor.yy43;
}
#line 2600 "parser.c"
        break;
      case 133: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 134: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1129 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy43 = BlockArg_new(env, lineno, yymsp[-3].minor.yy43, yymsp[-1].minor.yy43);
}
#line 2609 "parser.c"
        break;
      case 138: /* excepts ::= excepts except */
#line 1148 "parser.y"
{
    yygotominor.yy43 = Array_push(env, yymsp[-1].minor.yy43, yymsp[0].minor.yy43);
}
#line 2616 "parser.c"
        break;
      case 139: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1152 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy43 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy43, id, yymsp[0].minor.yy43);
}
#line 2626 "parser.c"
        break;
      case 140: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1158 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy43 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy43, NO_EXC_VAR, yymsp[0].minor.yy43);
}
#line 2634 "parser.c"
        break;
      case 141: /* except ::= EXCEPT NEWLINE stmts */
#line 1162 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy43 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy43);
}
#line 2642 "parser.c"
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
