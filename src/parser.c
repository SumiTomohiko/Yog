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
#define YYNOCODE 101
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy7;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 237
#define YYNRULE 142
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
 /*    10 */   178,   68,   55,   98,  200,   64,   59,  119,   23,  176,
 /*    20 */   165,  177,  380,   84,  153,  151,  152,  123,  130,  132,
 /*    30 */   216,   43,   44,  217,   28,   18,   32,  179,  180,  181,
 /*    40 */   182,  183,  184,  185,  186,  157,   82,  168,  159,  160,
 /*    50 */   161,   57,  178,  100,  101,   63,  102,   16,   64,   59,
 /*    60 */    23,    3,  176,  165,  177,   47,  153,  151,  152,   42,
 /*    70 */   124,  128,  207,   43,   44,  211,    2,   18,    3,  179,
 /*    80 */   180,  181,  182,  183,  184,  185,  186,  157,   82,  168,
 /*    90 */   159,  160,  161,   57,  108,  100,  101,   63,  102,  194,
 /*   100 */    64,   59,  144,   83,  176,  165,  177,   92,   60,  153,
 /*   110 */   151,  152,   61,  102,   80,   64,   59,   75,   36,  176,
 /*   120 */   165,  177,   40,   19,  126,  127,  129,  126,  127,  129,
 /*   130 */   157,   82,  168,  159,  160,  161,   57,  106,  100,  101,
 /*   140 */    63,  102,  234,   64,   59,   38,   83,  176,  165,  177,
 /*   150 */    93,  153,  151,  152,   97,   63,  102,   72,   64,   59,
 /*   160 */   195,  232,  176,  165,  177,  194,   29,  126,  127,  129,
 /*   170 */   231,   33,  157,   82,  168,  159,  160,  161,   57,  189,
 /*   180 */   100,  101,   63,  102,  112,   64,   59,  125,  204,  176,
 /*   190 */   165,  177,  198,   85,  153,  151,  152,  121,  122,  133,
 /*   200 */   137,  139,  225,  131,  214,  217,  140,  122,  133,  137,
 /*   210 */   139,  225,  135,  219,  217,  157,   82,  168,  159,  160,
 /*   220 */   161,   57,  113,  100,  101,   63,  102,   83,   64,   59,
 /*   230 */    83,  116,  176,  165,  177,   86,  153,  151,  152,   62,
 /*   240 */    59,  126,   48,  176,  165,  177,  162,  165,  177,  202,
 /*   250 */    16,  134,  136,  221,   15,  141,  211,  157,   82,  168,
 /*   260 */   159,  160,  161,   57,  206,  100,  101,   63,  102,   83,
 /*   270 */    64,   59,   83,  213,  176,  165,  177,   83,   49,  153,
 /*   280 */   151,  152,   58,  236,  173,  176,  165,  177,  163,  165,
 /*   290 */   177,  138,  223,  164,  165,  177,  174,  175,  212,   19,
 /*   300 */   157,   82,  168,  159,  160,  161,   57,  215,  100,  101,
 /*   310 */    63,  102,  143,   64,   59,  218,   17,  176,  165,  177,
 /*   320 */    50,  153,  151,  152,   16,  220,    8,   16,  143,  222,
 /*   330 */   115,  118,   17,   30,  208,  209,   16,   31,  171,  172,
 /*   340 */   224,   27,  157,   82,  168,  159,  160,  161,   57,   30,
 /*   350 */   100,  101,   63,  102,  154,   64,   59,  169,  170,  176,
 /*   360 */   165,  177,   13,  110,  153,  151,  152,   66,   81,  237,
 /*   370 */    16,   16,   16,   16,  155,  190,  196,   16,   16,    4,
 /*   380 */   201,  235,   33,   35,  228,  157,   82,  168,  159,  160,
 /*   390 */   161,   57,   34,  100,  101,   63,  102,   39,   64,   59,
 /*   400 */    22,  187,  176,  165,  177,   87,  153,  151,  152,  188,
 /*   410 */    65,    5,  145,    6,    7,  193,   67,    9,  114,  197,
 /*   420 */    69,  199,  117,   10,  227,   11,  120,  157,   82,  168,
 /*   430 */   159,  160,  161,   57,  203,  100,  101,   63,  102,   37,
 /*   440 */    64,   59,   41,   45,  176,  165,  177,   51,   88,  153,
 /*   450 */   151,  152,   70,  205,   71,   56,  229,   52,  233,   73,
 /*   460 */    74,   46,   53,   76,   77,   54,   78,   79,  230,  146,
 /*   470 */   157,   82,  168,  159,  160,  161,   57,   12,  100,  101,
 /*   480 */    63,  102,  381,   64,   59,  381,  381,  176,  165,  177,
 /*   490 */    89,  153,  151,  152,  381,  381,  381,  381,  381,  381,
 /*   500 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   510 */   381,  381,  157,   82,  168,  159,  160,  161,   57,  381,
 /*   520 */   100,  101,   63,  102,  381,   64,   59,  381,  381,  176,
 /*   530 */   165,  177,  381,  147,  153,  151,  152,  381,  381,  381,
 /*   540 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   550 */   381,  381,  381,  381,  381,  157,   82,  168,  159,  160,
 /*   560 */   161,   57,  381,  100,  101,   63,  102,  381,   64,   59,
 /*   570 */   381,  381,  176,  165,  177,  148,  153,  151,  152,  381,
 /*   580 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   590 */   381,  381,  381,  381,  381,  381,  381,  157,   82,  168,
 /*   600 */   159,  160,  161,   57,  381,  100,  101,   63,  102,  381,
 /*   610 */    64,   59,  381,  381,  176,  165,  177,  381,  149,  153,
 /*   620 */   151,  152,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   630 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   640 */   157,   82,  168,  159,  160,  161,   57,  381,  100,  101,
 /*   650 */    63,  102,  381,   64,   59,  381,  381,  176,  165,  177,
 /*   660 */    91,  153,  151,  152,  381,  381,  381,  381,  381,  381,
 /*   670 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   680 */   381,  381,  157,   82,  168,  159,  160,  161,   57,  381,
 /*   690 */   100,  101,   63,  102,  381,   64,   59,  381,   83,  176,
 /*   700 */   165,  177,  150,  151,  152,   96,  101,   63,  102,  381,
 /*   710 */    64,   59,  381,  381,  176,  165,  177,  381,  381,  381,
 /*   720 */   381,  381,  381,  157,   82,  168,  159,  160,  161,   57,
 /*   730 */   381,  100,  101,   63,  102,  381,   64,   59,  381,   83,
 /*   740 */   176,  165,  177,  166,   95,  381,  100,  101,   63,  102,
 /*   750 */   381,   64,   59,  381,  381,  176,  165,  177,  381,  381,
 /*   760 */   381,   99,  157,   82,  168,  159,  160,  161,   57,  381,
 /*   770 */   100,  101,   63,  102,  381,   64,   59,  381,  381,  176,
 /*   780 */   165,  177,  105,  166,  381,  381,  381,  381,  381,  381,
 /*   790 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   800 */   381,   99,  157,   82,  168,  159,  160,  161,   57,  381,
 /*   810 */   100,  101,   63,  102,  381,   64,   59,  381,  381,  176,
 /*   820 */   165,  177,  103,   90,  381,  381,  381,  381,  381,  381,
 /*   830 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   840 */   381,  381,  157,   82,  168,  159,  160,  161,   57,  381,
 /*   850 */   100,  101,   63,  102,  381,   64,   59,  381,  381,  176,
 /*   860 */   165,  177,   94,  381,  381,  381,  381,  381,  381,  381,
 /*   870 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   880 */   381,  157,   82,  168,  159,  160,  161,   57,  381,  100,
 /*   890 */   101,   63,  102,  381,   64,   59,  381,  381,  176,  165,
 /*   900 */   177,  156,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   910 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   920 */   157,   82,  168,  159,  160,  161,   57,  381,  100,  101,
 /*   930 */    63,  102,  381,   64,   59,  381,  381,  176,  165,  177,
 /*   940 */   167,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   950 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  157,
 /*   960 */    82,  168,  159,  160,  161,   57,  381,  100,  101,   63,
 /*   970 */   102,  381,   64,   59,  381,  381,  176,  165,  177,  104,
 /*   980 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*   990 */   381,  381,  381,  381,  381,  381,  381,  381,  157,   82,
 /*  1000 */   168,  159,  160,  161,   57,  381,  100,  101,   63,  102,
 /*  1010 */   381,   64,   59,  381,  381,  176,  165,  177,  191,  381,
 /*  1020 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*  1030 */   381,  381,  381,  381,  381,  381,  381,  157,   82,  168,
 /*  1040 */   159,  160,  161,   57,  381,  100,  101,   63,  102,  381,
 /*  1050 */    64,   59,  381,  381,  176,  165,  177,  192,  381,  381,
 /*  1060 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*  1070 */   381,  381,  381,  381,  381,  381,  157,   82,  168,  159,
 /*  1080 */   160,  161,   57,  381,  100,  101,   63,  102,  381,   64,
 /*  1090 */    59,  381,  381,  176,  165,  177,  107,  381,  381,  381,
 /*  1100 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*  1110 */   381,  381,  381,  381,  381,  157,   82,  168,  159,  160,
 /*  1120 */   161,   57,  381,  100,  101,   63,  102,  381,   64,   59,
 /*  1130 */   381,  381,  176,  165,  177,  109,  381,  381,  381,  381,
 /*  1140 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*  1150 */   381,  381,  381,  381,  157,   82,  168,  159,  160,  161,
 /*  1160 */    57,  381,  100,  101,   63,  102,  381,   64,   59,  381,
 /*  1170 */   381,  176,  165,  177,  210,  381,  381,  381,  381,  381,
 /*  1180 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*  1190 */   381,  381,  381,  157,   82,  168,  159,  160,  161,   57,
 /*  1200 */   381,  100,  101,   63,  102,  381,   64,   59,  381,  381,
 /*  1210 */   176,  165,  177,  226,  381,  381,  381,  381,  381,  381,
 /*  1220 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*  1230 */   381,  381,  157,   82,  168,  159,  160,  161,   57,  381,
 /*  1240 */   100,  101,   63,  102,  381,   64,   59,  381,  381,  176,
 /*  1250 */   165,  177,  142,  381,  381,  381,  381,  381,  381,  381,
 /*  1260 */   381,  381,  381,  381,  381,  381,  381,  381,  381,  381,
 /*  1270 */   381,  157,   82,  168,  159,  160,  161,   57,  381,  100,
 /*  1280 */   101,   63,  102,  381,   64,   59,   14,  381,  176,  165,
 /*  1290 */   177,   83,  158,  159,  160,  161,   57,  178,  100,  101,
 /*  1300 */    63,  102,  381,   64,   59,   23,  381,  176,  165,  177,
 /*  1310 */   381,  381,  381,  381,  381,  381,  381,  381,   43,   44,
 /*  1320 */   381,  381,   18,  381,  179,  180,  181,  182,  183,  184,
 /*  1330 */   185,  186,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   77,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   87,   12,   89,   90,   19,   20,   93,
 /*    20 */    94,   95,   53,   54,   55,   56,   57,   68,   69,   70,
 /*    30 */    71,   33,   34,   74,   25,   37,   83,   39,   40,   41,
 /*    40 */    42,   43,   44,   45,   46,   76,   77,   78,   79,   80,
 /*    50 */    81,   82,   12,   84,   85,   86,   87,    1,   89,   90,
 /*    60 */    20,    5,   93,   94,   95,   54,   55,   56,   57,   92,
 /*    70 */    69,   70,   71,   33,   34,   74,    3,   37,    5,   39,
 /*    80 */    40,   41,   42,   43,   44,   45,   46,   76,   77,   78,
 /*    90 */    79,   80,   81,   82,   60,   84,   85,   86,   87,   65,
 /*   100 */    89,   90,   59,   77,   93,   94,   95,   59,   54,   55,
 /*   110 */    56,   57,   86,   87,   12,   89,   90,   12,   88,   93,
 /*   120 */    94,   95,   91,   50,   22,   23,   24,   22,   23,   24,
 /*   130 */    76,   77,   78,   79,   80,   81,   82,   98,   84,   85,
 /*   140 */    86,   87,   99,   89,   90,   37,   77,   93,   94,   95,
 /*   150 */    54,   55,   56,   57,   85,   86,   87,   12,   89,   90,
 /*   160 */    60,   17,   93,   94,   95,   65,   17,   22,   23,   24,
 /*   170 */    26,   27,   76,   77,   78,   79,   80,   81,   82,   97,
 /*   180 */    84,   85,   86,   87,   61,   89,   90,   70,   71,   93,
 /*   190 */    94,   95,   12,   54,   55,   56,   57,   66,   67,   68,
 /*   200 */    69,   70,   71,   70,   71,   74,   66,   67,   68,   69,
 /*   210 */    70,   71,   70,   71,   74,   76,   77,   78,   79,   80,
 /*   220 */    81,   82,   62,   84,   85,   86,   87,   77,   89,   90,
 /*   230 */    77,   64,   93,   94,   95,   54,   55,   56,   57,   89,
 /*   240 */    90,   22,   58,   93,   94,   95,   93,   94,   95,   71,
 /*   250 */     1,   69,   70,   71,    5,   98,   74,   76,   77,   78,
 /*   260 */    79,   80,   81,   82,   71,   84,   85,   86,   87,   77,
 /*   270 */    89,   90,   77,   71,   93,   94,   95,   77,   54,   55,
 /*   280 */    56,   57,   90,   99,   23,   93,   94,   95,   93,   94,
 /*   290 */    95,   70,   71,   93,   94,   95,   35,   36,   73,   50,
 /*   300 */    76,   77,   78,   79,   80,   81,   82,   71,   84,   85,
 /*   310 */    86,   87,   16,   89,   90,   71,   20,   93,   94,   95,
 /*   320 */    54,   55,   56,   57,    1,   71,    3,    1,   16,   71,
 /*   330 */    63,   64,   20,   37,   72,   73,    1,   25,   33,   34,
 /*   340 */    71,   18,   76,   77,   78,   79,   80,   81,   82,   37,
 /*   350 */    84,   85,   86,   87,    4,   89,   90,   30,   31,   93,
 /*   360 */    94,   95,    1,   54,   55,   56,   57,   47,   48,    0,
 /*   370 */     1,    1,    1,    1,    4,    4,    4,    1,    1,    1,
 /*   380 */     4,    4,   27,   29,   49,   76,   77,   78,   79,   80,
 /*   390 */    81,   82,   28,   84,   85,   86,   87,   32,   89,   90,
 /*   400 */    15,   38,   93,   94,   95,   54,   55,   56,   57,   21,
 /*   410 */    21,    1,   51,    1,    1,    4,   12,    1,   15,   12,
 /*   420 */    15,   12,   16,   21,   38,    1,   12,   76,   77,   78,
 /*   430 */    79,   80,   81,   82,   12,   84,   85,   86,   87,   20,
 /*   440 */    89,   90,   15,   15,   93,   94,   95,   15,   54,   55,
 /*   450 */    56,   57,   15,   12,   15,   12,   38,   15,    4,   15,
 /*   460 */    15,   15,   15,   15,   15,   15,   15,   15,   12,   12,
 /*   470 */    76,   77,   78,   79,   80,   81,   82,    1,   84,   85,
 /*   480 */    86,   87,  100,   89,   90,  100,  100,   93,   94,   95,
 /*   490 */    54,   55,   56,   57,  100,  100,  100,  100,  100,  100,
 /*   500 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   510 */   100,  100,   76,   77,   78,   79,   80,   81,   82,  100,
 /*   520 */    84,   85,   86,   87,  100,   89,   90,  100,  100,   93,
 /*   530 */    94,   95,  100,   54,   55,   56,   57,  100,  100,  100,
 /*   540 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   550 */   100,  100,  100,  100,  100,   76,   77,   78,   79,   80,
 /*   560 */    81,   82,  100,   84,   85,   86,   87,  100,   89,   90,
 /*   570 */   100,  100,   93,   94,   95,   54,   55,   56,   57,  100,
 /*   580 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   590 */   100,  100,  100,  100,  100,  100,  100,   76,   77,   78,
 /*   600 */    79,   80,   81,   82,  100,   84,   85,   86,   87,  100,
 /*   610 */    89,   90,  100,  100,   93,   94,   95,  100,   54,   55,
 /*   620 */    56,   57,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   630 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   640 */    76,   77,   78,   79,   80,   81,   82,  100,   84,   85,
 /*   650 */    86,   87,  100,   89,   90,  100,  100,   93,   94,   95,
 /*   660 */    54,   55,   56,   57,  100,  100,  100,  100,  100,  100,
 /*   670 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   680 */   100,  100,   76,   77,   78,   79,   80,   81,   82,  100,
 /*   690 */    84,   85,   86,   87,  100,   89,   90,  100,   77,   93,
 /*   700 */    94,   95,   55,   56,   57,   84,   85,   86,   87,  100,
 /*   710 */    89,   90,  100,  100,   93,   94,   95,  100,  100,  100,
 /*   720 */   100,  100,  100,   76,   77,   78,   79,   80,   81,   82,
 /*   730 */   100,   84,   85,   86,   87,  100,   89,   90,  100,   77,
 /*   740 */    93,   94,   95,   57,   82,  100,   84,   85,   86,   87,
 /*   750 */   100,   89,   90,  100,  100,   93,   94,   95,  100,  100,
 /*   760 */   100,   75,   76,   77,   78,   79,   80,   81,   82,  100,
 /*   770 */    84,   85,   86,   87,  100,   89,   90,  100,  100,   93,
 /*   780 */    94,   95,   96,   57,  100,  100,  100,  100,  100,  100,
 /*   790 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   800 */   100,   75,   76,   77,   78,   79,   80,   81,   82,  100,
 /*   810 */    84,   85,   86,   87,  100,   89,   90,  100,  100,   93,
 /*   820 */    94,   95,   96,   57,  100,  100,  100,  100,  100,  100,
 /*   830 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   840 */   100,  100,   76,   77,   78,   79,   80,   81,   82,  100,
 /*   850 */    84,   85,   86,   87,  100,   89,   90,  100,  100,   93,
 /*   860 */    94,   95,   57,  100,  100,  100,  100,  100,  100,  100,
 /*   870 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   880 */   100,   76,   77,   78,   79,   80,   81,   82,  100,   84,
 /*   890 */    85,   86,   87,  100,   89,   90,  100,  100,   93,   94,
 /*   900 */    95,   57,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   910 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   920 */    76,   77,   78,   79,   80,   81,   82,  100,   84,   85,
 /*   930 */    86,   87,  100,   89,   90,  100,  100,   93,   94,   95,
 /*   940 */    57,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   950 */   100,  100,  100,  100,  100,  100,  100,  100,  100,   76,
 /*   960 */    77,   78,   79,   80,   81,   82,  100,   84,   85,   86,
 /*   970 */    87,  100,   89,   90,  100,  100,   93,   94,   95,   57,
 /*   980 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*   990 */   100,  100,  100,  100,  100,  100,  100,  100,   76,   77,
 /*  1000 */    78,   79,   80,   81,   82,  100,   84,   85,   86,   87,
 /*  1010 */   100,   89,   90,  100,  100,   93,   94,   95,   57,  100,
 /*  1020 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*  1030 */   100,  100,  100,  100,  100,  100,  100,   76,   77,   78,
 /*  1040 */    79,   80,   81,   82,  100,   84,   85,   86,   87,  100,
 /*  1050 */    89,   90,  100,  100,   93,   94,   95,   57,  100,  100,
 /*  1060 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*  1070 */   100,  100,  100,  100,  100,  100,   76,   77,   78,   79,
 /*  1080 */    80,   81,   82,  100,   84,   85,   86,   87,  100,   89,
 /*  1090 */    90,  100,  100,   93,   94,   95,   57,  100,  100,  100,
 /*  1100 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*  1110 */   100,  100,  100,  100,  100,   76,   77,   78,   79,   80,
 /*  1120 */    81,   82,  100,   84,   85,   86,   87,  100,   89,   90,
 /*  1130 */   100,  100,   93,   94,   95,   57,  100,  100,  100,  100,
 /*  1140 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*  1150 */   100,  100,  100,  100,   76,   77,   78,   79,   80,   81,
 /*  1160 */    82,  100,   84,   85,   86,   87,  100,   89,   90,  100,
 /*  1170 */   100,   93,   94,   95,   57,  100,  100,  100,  100,  100,
 /*  1180 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*  1190 */   100,  100,  100,   76,   77,   78,   79,   80,   81,   82,
 /*  1200 */   100,   84,   85,   86,   87,  100,   89,   90,  100,  100,
 /*  1210 */    93,   94,   95,   57,  100,  100,  100,  100,  100,  100,
 /*  1220 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*  1230 */   100,  100,   76,   77,   78,   79,   80,   81,   82,  100,
 /*  1240 */    84,   85,   86,   87,  100,   89,   90,  100,  100,   93,
 /*  1250 */    94,   95,   57,  100,  100,  100,  100,  100,  100,  100,
 /*  1260 */   100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
 /*  1270 */   100,   76,   77,   78,   79,   80,   81,   82,  100,   84,
 /*  1280 */    85,   86,   87,  100,   89,   90,    1,  100,   93,   94,
 /*  1290 */    95,   77,   78,   79,   80,   81,   82,   12,   84,   85,
 /*  1300 */    86,   87,  100,   89,   90,   20,  100,   93,   94,   95,
 /*  1310 */   100,  100,  100,  100,  100,  100,  100,  100,   33,   34,
 /*  1320 */   100,  100,   37,  100,   39,   40,   41,   42,   43,   44,
 /*  1330 */    45,   46,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 149
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   40,   40, 1285,
 /*    20 */    40,   40,   40,   40,   40,   40,   40,   40,   40,   40,
 /*    30 */    40,   40,   40,   40,   40,   40,   40,  102,  102,   40,
 /*    40 */    40,  105,   40,   40,   40,  145,  145,  249,   73,  323,
 /*    50 */   323,  -21,  -21,  -21,  -21,    2,    9,  144,  261,  261,
 /*    60 */    56,  327,  305,  327,  305,  320,  108,  149,  180,    2,
 /*    70 */   219,  219,    9,  219,  219,    9,  219,  219,  219,  219,
 /*    80 */     9,  108,  312,  296,  369,  370,  371,  372,  376,  335,
 /*    90 */   361,  377,  350,  326,  378,  355,  364,  354,  365,  385,
 /*   100 */   364,  354,  365,  363,  388,  389,  410,  412,  411,  413,
 /*   110 */   326,  404,  416,  403,  407,  405,  406,  409,  406,  414,
 /*   120 */   419,  402,  427,  428,  432,  437,  422,  441,  439,  443,
 /*   130 */   442,  444,  445,  446,  447,  448,  449,  450,  451,  452,
 /*   140 */   386,  424,  418,  456,  454,  457,  476,  326,  326,  326,
};
#define YY_REDUCE_USE_DFLT (-75)
#define YY_REDUCE_MAX 81
static const short yy_reduce_ofst[] = {
 /*     0 */   -31,   11,   54,   96,  139,  181,  224,  266,  309,  351,
 /*    10 */   394,  436,  479,  521,  564,  606,  647,  686,  726,  766,
 /*    20 */   805,  844,  883,  922,  961, 1000, 1039, 1078, 1117, 1156,
 /*    30 */  1195, 1214,  662,  621,   69,   26,  -74,  131,  140,  150,
 /*    40 */   192,  -41,  153,  195,  200,    1,  182,  184,   43,   34,
 /*    50 */   100,  117,  133,  142,  221,  267,  262,  -47,  -23,  -23,
 /*    60 */    48,   30,   31,   30,   31,   82,   39,  123,  160,  167,
 /*    70 */   178,  193,  225,  202,  236,  225,  244,  254,  258,  269,
 /*    80 */   225,  157,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   240,  240,  240,  240,  240,  240,  240,  240,  240,  240,
 /*    10 */   240,  240,  240,  240,  240,  240,  240,  365,  365,  379,
 /*    20 */   379,  247,  379,  379,  249,  251,  379,  379,  379,  379,
 /*    30 */   379,  379,  379,  379,  379,  379,  379,  301,  301,  379,
 /*    40 */   379,  379,  379,  379,  379,  379,  379,  379,  377,  267,
 /*    50 */   267,  379,  379,  379,  379,  379,  305,  321,  338,  337,
 /*    60 */   377,  330,  336,  329,  335,  367,  370,  263,  379,  379,
 /*    70 */   379,  379,  379,  379,  379,  309,  379,  379,  379,  379,
 /*    80 */   308,  370,  349,  349,  379,  379,  379,  379,  379,  379,
 /*    90 */   379,  379,  379,  378,  379,  322,  326,  328,  332,  366,
 /*   100 */   325,  327,  331,  379,  379,  379,  379,  379,  379,  379,
 /*   110 */   268,  379,  379,  255,  379,  256,  258,  379,  257,  379,
 /*   120 */   379,  379,  285,  277,  273,  271,  379,  379,  275,  379,
 /*   130 */   281,  279,  283,  293,  289,  287,  291,  297,  295,  299,
 /*   140 */   379,  379,  379,  379,  379,  379,  379,  374,  375,  376,
 /*   150 */   239,  241,  242,  238,  243,  246,  248,  315,  316,  318,
 /*   160 */   319,  320,  341,  346,  347,  348,  313,  314,  317,  333,
 /*   170 */   334,  339,  340,  343,  344,  345,  342,  350,  354,  355,
 /*   180 */   356,  357,  358,  359,  360,  361,  362,  363,  364,  351,
 /*   190 */   368,  250,  252,  253,  265,  266,  254,  262,  261,  260,
 /*   200 */   259,  269,  270,  302,  272,  303,  274,  276,  304,  306,
 /*   210 */   307,  311,  312,  278,  280,  282,  284,  310,  286,  288,
 /*   220 */   290,  292,  294,  296,  298,  300,  264,  371,  369,  352,
 /*   230 */   353,  323,  324,  244,  373,  245,  372,
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
  "shift_op",      "arith_expr",    "term",          "arith_op",    
  "term_op",       "factor",        "power",         "atom",        
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
 /* 109 */ "factor ::= PLUS factor",
 /* 110 */ "factor ::= MINUS factor",
 /* 111 */ "factor ::= power",
 /* 112 */ "power ::= postfix_expr",
 /* 113 */ "postfix_expr ::= atom",
 /* 114 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 115 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 116 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 117 */ "atom ::= NAME",
 /* 118 */ "atom ::= NUMBER",
 /* 119 */ "atom ::= REGEXP",
 /* 120 */ "atom ::= STRING",
 /* 121 */ "atom ::= SYMBOL",
 /* 122 */ "atom ::= NIL",
 /* 123 */ "atom ::= TRUE",
 /* 124 */ "atom ::= FALSE",
 /* 125 */ "atom ::= LINE",
 /* 126 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 127 */ "atom ::= LPAR expr RPAR",
 /* 128 */ "args_opt ::=",
 /* 129 */ "args_opt ::= args",
 /* 130 */ "blockarg_opt ::=",
 /* 131 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 132 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 133 */ "blockarg_params_opt ::=",
 /* 134 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 135 */ "excepts ::= except",
 /* 136 */ "excepts ::= excepts except",
 /* 137 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 138 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 139 */ "except ::= EXCEPT NEWLINE stmts",
 /* 140 */ "finally_opt ::=",
 /* 141 */ "finally_opt ::= FINALLY stmts",
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
  { 53, 1 },
  { 54, 1 },
  { 54, 3 },
  { 55, 0 },
  { 55, 1 },
  { 55, 1 },
  { 55, 7 },
  { 55, 5 },
  { 55, 5 },
  { 55, 5 },
  { 55, 1 },
  { 55, 2 },
  { 55, 1 },
  { 55, 2 },
  { 55, 1 },
  { 55, 2 },
  { 55, 6 },
  { 55, 6 },
  { 55, 2 },
  { 55, 2 },
  { 63, 1 },
  { 63, 3 },
  { 64, 1 },
  { 64, 3 },
  { 62, 1 },
  { 62, 3 },
  { 61, 0 },
  { 61, 2 },
  { 60, 1 },
  { 60, 5 },
  { 65, 0 },
  { 65, 2 },
  { 56, 7 },
  { 66, 9 },
  { 66, 7 },
  { 66, 7 },
  { 66, 5 },
  { 66, 7 },
  { 66, 5 },
  { 66, 5 },
  { 66, 3 },
  { 66, 7 },
  { 66, 5 },
  { 66, 5 },
  { 66, 3 },
  { 66, 5 },
  { 66, 3 },
  { 66, 3 },
  { 66, 1 },
  { 66, 7 },
  { 66, 5 },
  { 66, 5 },
  { 66, 3 },
  { 66, 5 },
  { 66, 3 },
  { 66, 3 },
  { 66, 1 },
  { 66, 5 },
  { 66, 3 },
  { 66, 3 },
  { 66, 1 },
  { 66, 3 },
  { 66, 1 },
  { 66, 1 },
  { 66, 0 },
  { 71, 2 },
  { 70, 2 },
  { 69, 3 },
  { 72, 0 },
  { 72, 1 },
  { 73, 2 },
  { 67, 1 },
  { 67, 3 },
  { 68, 1 },
  { 68, 3 },
  { 74, 2 },
  { 75, 1 },
  { 75, 3 },
  { 57, 1 },
  { 76, 3 },
  { 76, 1 },
  { 78, 1 },
  { 79, 1 },
  { 80, 1 },
  { 81, 1 },
  { 81, 3 },
  { 83, 1 },
  { 83, 1 },
  { 82, 1 },
  { 82, 3 },
  { 84, 1 },
  { 84, 3 },
  { 85, 1 },
  { 85, 3 },
  { 86, 1 },
  { 86, 3 },
  { 88, 1 },
  { 88, 1 },
  { 87, 1 },
  { 87, 3 },
  { 89, 1 },
  { 89, 3 },
  { 91, 1 },
  { 91, 1 },
  { 90, 3 },
  { 90, 1 },
  { 92, 1 },
  { 92, 1 },
  { 92, 1 },
  { 93, 2 },
  { 93, 2 },
  { 93, 1 },
  { 94, 1 },
  { 77, 1 },
  { 77, 5 },
  { 77, 4 },
  { 77, 3 },
  { 95, 1 },
  { 95, 1 },
  { 95, 1 },
  { 95, 1 },
  { 95, 1 },
  { 95, 1 },
  { 95, 1 },
  { 95, 1 },
  { 95, 1 },
  { 95, 3 },
  { 95, 3 },
  { 96, 0 },
  { 96, 1 },
  { 97, 0 },
  { 97, 5 },
  { 97, 5 },
  { 98, 0 },
  { 98, 3 },
  { 58, 1 },
  { 58, 2 },
  { 99, 6 },
  { 99, 4 },
  { 99, 3 },
  { 59, 0 },
  { 59, 2 },
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
    *pval = yymsp[0].minor.yy7;
}
#line 1905 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 135: /* excepts ::= except */
#line 626 "parser.y"
{
    yygotominor.yy7 = make_array_with(env, yymsp[0].minor.yy7);
}
#line 1916 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 629 "parser.y"
{
    yygotominor.yy7 = Array_push(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 1926 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 128: /* args_opt ::= */
      case 130: /* blockarg_opt ::= */
      case 133: /* blockarg_params_opt ::= */
      case 140: /* finally_opt ::= */
#line 633 "parser.y"
{
    yygotominor.yy7 = YNIL;
}
#line 1940 "parser.c"
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
      case 111: /* factor ::= power */
      case 112: /* power ::= postfix_expr */
      case 113: /* postfix_expr ::= atom */
      case 129: /* args_opt ::= args */
      case 141: /* finally_opt ::= FINALLY stmts */
#line 636 "parser.y"
{
    yygotominor.yy7 = yymsp[0].minor.yy7;
}
#line 1971 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 642 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy7 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[-1].minor.yy7);
}
#line 1979 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 646 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[-1].minor.yy7);
}
#line 1987 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 650 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = Finally_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 1995 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 654 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = While_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2003 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 658 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Break_new(env, lineno, YNIL);
}
#line 2011 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 662 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Break_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2019 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 666 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Next_new(env, lineno, YNIL);
}
#line 2027 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 670 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Next_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2035 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 674 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Return_new(env, lineno, YNIL);
}
#line 2043 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 678 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Return_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2051 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 682 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy7 = If_new(env, lineno, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2059 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 686 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy7 = Klass_new(env, lineno, id, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2068 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 691 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Nonlocal_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2076 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 695 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Import_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2084 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 707 "parser.y"
{
    yygotominor.yy7 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2092 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 710 "parser.y"
{
    yygotominor.yy7 = Array_push_token_id(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy0);
}
#line 2100 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 731 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7, yymsp[0].minor.yy7);
    yygotominor.yy7 = make_array_with(env, node);
}
#line 2109 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 744 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy7 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2118 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 750 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-8].minor.yy7, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2125 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 753 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2132 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 756 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2139 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 759 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2146 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 762 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2153 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 765 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2160 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 768 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2167 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 771 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL, YNIL);
}
#line 2174 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 774 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2181 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 777 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2188 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 780 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2195 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 783 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2202 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 786 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2209 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 789 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2216 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 792 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, YNIL, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2223 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 795 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[0].minor.yy7, YNIL, YNIL, YNIL, YNIL);
}
#line 2230 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 798 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2237 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 801 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2244 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 804 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2251 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 807 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2258 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 810 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2265 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 813 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2272 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 816 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2279 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 819 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[0].minor.yy7, YNIL, YNIL, YNIL);
}
#line 2286 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 822 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2293 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 825 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2300 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 828 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2307 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 831 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2314 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 834 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2321 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 837 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2328 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 840 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2335 "parser.c"
        break;
      case 64: /* params ::= */
#line 843 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2342 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 847 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2351 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 853 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2360 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 859 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy7);
}
#line 2369 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 876 "parser.y"
{
    yygotominor.yy7 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy7, lineno, id, YNIL);
}
#line 2379 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 882 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy7, lineno, id, YNIL);
    yygotominor.yy7 = yymsp[-2].minor.yy7;
}
#line 2389 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 896 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy7);
}
#line 2398 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 913 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    yygotominor.yy7 = Assign_new(env, lineno, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2406 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 936 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy7)->u.id;
    yygotominor.yy7 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy7, id, yymsp[0].minor.yy7);
}
#line 2415 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
      case 87: /* comp_op ::= GREATER */
#line 942 "parser.y"
{
    yygotominor.yy7 = yymsp[0].minor.yy0;
}
#line 2423 "parser.c"
        break;
      case 89: /* xor_expr ::= xor_expr XOR or_expr */
      case 91: /* or_expr ::= or_expr BAR and_expr */
      case 93: /* and_expr ::= and_expr AND shift_expr */
#line 952 "parser.y"
{
    yygotominor.yy7 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy7), yymsp[-2].minor.yy7, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy7);
}
#line 2432 "parser.c"
        break;
      case 95: /* shift_expr ::= shift_expr shift_op match_expr */
      case 101: /* arith_expr ::= arith_expr arith_op term */
      case 104: /* term ::= term term_op factor */
#line 973 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    yygotominor.yy7 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy7, VAL2ID(yymsp[-1].minor.yy7), yymsp[0].minor.yy7);
}
#line 2442 "parser.c"
        break;
      case 96: /* shift_op ::= LSHIFT */
      case 97: /* shift_op ::= RSHIFT */
      case 102: /* arith_op ::= PLUS */
      case 103: /* arith_op ::= MINUS */
      case 106: /* term_op ::= STAR */
      case 107: /* term_op ::= DIV */
      case 108: /* term_op ::= DIV_DIV */
#line 978 "parser.y"
{
    yygotominor.yy7 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2455 "parser.c"
        break;
      case 99: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 988 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy7 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy7, id, yymsp[0].minor.yy7);
}
#line 2464 "parser.c"
        break;
      case 109: /* factor ::= PLUS factor */
#line 1027 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy7 = FuncCall_new3(env, lineno, yymsp[0].minor.yy7, id);
}
#line 2473 "parser.c"
        break;
      case 110: /* factor ::= MINUS factor */
#line 1032 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy7 = FuncCall_new3(env, lineno, yymsp[0].minor.yy7, id);
}
#line 2482 "parser.c"
        break;
      case 114: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1048 "parser.y"
{
    yygotominor.yy7 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy7), yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2489 "parser.c"
        break;
      case 115: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1051 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy7);
    yygotominor.yy7 = Subscript_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2497 "parser.c"
        break;
      case 116: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1055 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Attr_new(env, lineno, yymsp[-2].minor.yy7, id);
}
#line 2506 "parser.c"
        break;
      case 117: /* atom ::= NAME */
#line 1061 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Variable_new(env, lineno, id);
}
#line 2515 "parser.c"
        break;
      case 118: /* atom ::= NUMBER */
      case 119: /* atom ::= REGEXP */
      case 120: /* atom ::= STRING */
      case 121: /* atom ::= SYMBOL */
#line 1066 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy7 = Literal_new(env, lineno, val);
}
#line 2527 "parser.c"
        break;
      case 122: /* atom ::= NIL */
#line 1086 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Literal_new(env, lineno, YNIL);
}
#line 2535 "parser.c"
        break;
      case 123: /* atom ::= TRUE */
#line 1090 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Literal_new(env, lineno, YTRUE);
}
#line 2543 "parser.c"
        break;
      case 124: /* atom ::= FALSE */
#line 1094 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Literal_new(env, lineno, YFALSE);
}
#line 2551 "parser.c"
        break;
      case 125: /* atom ::= LINE */
#line 1098 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy7 = Literal_new(env, lineno, val);
}
#line 2560 "parser.c"
        break;
      case 126: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1103 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy7 = Array_new(env, lineno, yymsp[-1].minor.yy7);
}
#line 2568 "parser.c"
        break;
      case 127: /* atom ::= LPAR expr RPAR */
      case 134: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1107 "parser.y"
{
    yygotominor.yy7 = yymsp[-1].minor.yy7;
}
#line 2576 "parser.c"
        break;
      case 131: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 132: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1121 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = BlockArg_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2585 "parser.c"
        break;
      case 136: /* excepts ::= excepts except */
#line 1140 "parser.y"
{
    yygotominor.yy7 = Array_push(env, yymsp[-1].minor.yy7, yymsp[0].minor.yy7);
}
#line 2592 "parser.c"
        break;
      case 137: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1144 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy7 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy7, id, yymsp[0].minor.yy7);
}
#line 2602 "parser.c"
        break;
      case 138: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1150 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy7 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy7, NO_EXC_VAR, yymsp[0].minor.yy7);
}
#line 2610 "parser.c"
        break;
      case 139: /* except ::= EXCEPT NEWLINE stmts */
#line 1154 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy7 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy7);
}
#line 2618 "parser.c"
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
