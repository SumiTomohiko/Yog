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
    case NODE_LOGICAL_AND:
        KEEP(logical_and.left);
        KEEP(logical_and.right);
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
#line 630 "parser.c"
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
#define YYNOCODE 104
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy175;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 242
#define YYNRULE 145
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
 /*     0 */     1,  129,  130,   85,   20,   21,   24,   25,   26,  114,
 /*    10 */   183,   70,   57,  101,   50,   66,   60,  122,   23,  181,
 /*    20 */   168,  182,   13,  147,   28,  388,   86,  156,  154,  155,
 /*    30 */   128,  209,   44,   45,  127,  131,  212,   46,   18,  216,
 /*    40 */   184,  185,  186,  187,  188,  189,  190,  191,  160,   84,
 /*    50 */   171,   97,  172,  163,   61,  241,  103,  104,   65,  105,
 /*    60 */   205,   66,   60,  239,   85,  181,  168,  182,   49,  156,
 /*    70 */   154,  155,  100,   65,  105,  148,   66,   60,  134,  219,
 /*    80 */   181,  168,  182,  126,  133,  135,  221,  138,  224,  222,
 /*    90 */   160,   84,  171,   97,  172,  163,   61,  111,  103,  104,
 /*   100 */    65,  105,  199,   66,   60,   43,   85,  181,  168,  182,
 /*   110 */    62,  156,  154,  155,   33,   63,  105,   82,   66,   60,
 /*   120 */    94,   16,  181,  168,  182,   15,  200,  129,  130,  132,
 /*   130 */    16,  199,  160,   84,  171,   97,  172,  163,   61,   41,
 /*   140 */   103,  104,   65,  105,   16,   66,   60,   77,    3,  181,
 /*   150 */   168,  182,   95,  156,  154,  155,  237,  129,  130,  132,
 /*   160 */   124,  125,  136,  140,  142,  230,  236,   34,  222,  141,
 /*   170 */   228,  242,   16,   19,  160,   84,  171,   97,  172,  163,
 /*   180 */    61,  233,  103,  104,   65,  105,  194,   66,   60,   74,
 /*   190 */    37,  181,  168,  182,   87,  156,  154,  155,  177,  129,
 /*   200 */   130,  132,  143,  125,  136,  140,  142,  230,  118,  121,
 /*   210 */   222,  178,  179,  180,  173,  174,  160,   84,  171,   97,
 /*   220 */   172,  163,   61,  109,  103,  104,   65,  105,   39,   66,
 /*   230 */    60,   85,   29,  181,  168,  182,   88,  156,  154,  155,
 /*   240 */    16,  146,    8,   64,   60,   17,  115,  181,  168,  182,
 /*   250 */   137,  139,  226,  175,  176,  216,  203,   27,  160,   84,
 /*   260 */   171,   97,  172,  163,   61,   30,  103,  104,   65,  105,
 /*   270 */   116,   66,   60,   85,  119,  181,  168,  182,   51,  156,
 /*   280 */   154,  155,  129,  146,  213,  214,   59,   17,  207,  181,
 /*   290 */   168,  182,   31,   68,   83,   16,   16,  211,  158,  195,
 /*   300 */   160,   84,  171,   97,  172,  163,   61,   30,  103,  104,
 /*   310 */    65,  105,   85,   66,   60,   85,  217,  181,  168,  182,
 /*   320 */    52,  156,  154,  155,   16,  218,  220,  201,  164,  168,
 /*   330 */   182,  165,  168,  182,   16,   16,  223,  206,  240,  225,
 /*   340 */   227,  157,  160,   84,  171,   97,  172,  163,   61,  229,
 /*   350 */   103,  104,   65,  105,   85,   66,   60,   85,  144,  181,
 /*   360 */   168,  182,  113,  156,  154,  155,    2,   16,    3,    4,
 /*   370 */   166,  168,  182,  167,  168,  182,   32,   34,   36,   40,
 /*   380 */    35,  192,   22,  193,  160,   84,  171,   97,  172,  163,
 /*   390 */    61,   67,  103,  104,   65,  105,    5,   66,   60,    6,
 /*   400 */   198,  181,  168,  182,   89,  156,  154,  155,    7,   69,
 /*   410 */     9,   38,  202,  204,  117,   10,   19,   71,  123,  120,
 /*   420 */    42,  232,   11,   47,  208,   53,  160,   84,  171,   97,
 /*   430 */   172,  163,   61,   72,  103,  104,   65,  105,  210,   66,
 /*   440 */    60,   58,   73,  181,  168,  182,   90,  156,  154,  155,
 /*   450 */    54,   75,   76,   48,   55,   78,   79,   56,   80,   81,
 /*   460 */   234,  235,  238,  149,   12,  389,  389,  389,  160,   84,
 /*   470 */   171,   97,  172,  163,   61,  389,  103,  104,   65,  105,
 /*   480 */   389,   66,   60,  389,  389,  181,  168,  182,   91,  156,
 /*   490 */   154,  155,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   500 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   510 */   160,   84,  171,   97,  172,  163,   61,  389,  103,  104,
 /*   520 */    65,  105,  389,   66,   60,  389,  389,  181,  168,  182,
 /*   530 */   150,  156,  154,  155,  389,  389,  389,  389,  389,  389,
 /*   540 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   550 */   389,  389,  160,   84,  171,   97,  172,  163,   61,  389,
 /*   560 */   103,  104,   65,  105,  389,   66,   60,  389,  389,  181,
 /*   570 */   168,  182,  151,  156,  154,  155,  389,  389,  389,  389,
 /*   580 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   590 */   389,  389,  389,  389,  160,   84,  171,   97,  172,  163,
 /*   600 */    61,  389,  103,  104,   65,  105,  389,   66,   60,  389,
 /*   610 */   389,  181,  168,  182,  152,  156,  154,  155,  389,  389,
 /*   620 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   630 */   389,  389,  389,  389,  389,  389,  160,   84,  171,   97,
 /*   640 */   172,  163,   61,  389,  103,  104,   65,  105,  389,   66,
 /*   650 */    60,  389,  389,  181,  168,  182,   93,  156,  154,  155,
 /*   660 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   670 */   389,  389,  389,  389,  389,  389,  389,  389,  160,   84,
 /*   680 */   171,   97,  172,  163,   61,  389,  103,  104,   65,  105,
 /*   690 */   389,   66,   60,  389,   85,  181,  168,  182,  153,  154,
 /*   700 */   155,   99,  104,   65,  105,  389,   66,   60,  389,  389,
 /*   710 */   181,  168,  182,  389,  389,  389,  389,  389,  389,  160,
 /*   720 */    84,  171,   97,  172,  163,   61,  389,  103,  104,   65,
 /*   730 */   105,  389,   66,   60,  389,   85,  181,  168,  182,  169,
 /*   740 */    98,  389,  103,  104,   65,  105,  389,   66,   60,  389,
 /*   750 */   389,  181,  168,  182,  389,  389,  389,  102,  160,   84,
 /*   760 */   171,   97,  172,  163,   61,  389,  103,  104,   65,  105,
 /*   770 */   389,   66,   60,  389,  389,  181,  168,  182,  108,   85,
 /*   780 */   169,  389,  162,  163,   61,  389,  103,  104,   65,  105,
 /*   790 */   389,   66,   60,  389,  389,  181,  168,  182,  102,  160,
 /*   800 */    84,  171,   97,  172,  163,   61,  389,  103,  104,   65,
 /*   810 */   105,  389,   66,   60,  389,  389,  181,  168,  182,  106,
 /*   820 */    92,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   830 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  160,
 /*   840 */    84,  171,   97,  172,  163,   61,  389,  103,  104,   65,
 /*   850 */   105,  389,   66,   60,  389,  389,  181,  168,  182,   96,
 /*   860 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   870 */   389,  389,  389,  389,  389,  389,  389,  389,  160,   84,
 /*   880 */   171,   97,  172,  163,   61,  389,  103,  104,   65,  105,
 /*   890 */   389,   66,   60,  389,  389,  181,  168,  182,  389,  389,
 /*   900 */   159,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   910 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  160,
 /*   920 */    84,  171,   97,  172,  163,   61,  389,  103,  104,   65,
 /*   930 */   105,  389,   66,   60,  389,  389,  181,  168,  182,  170,
 /*   940 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   950 */   389,  389,  389,  389,  389,  389,  389,  389,  160,   84,
 /*   960 */   171,   97,  172,  163,   61,  389,  103,  104,   65,  105,
 /*   970 */   389,   66,   60,  389,  389,  181,  168,  182,  389,  389,
 /*   980 */   107,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*   990 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  160,
 /*  1000 */    84,  171,   97,  172,  163,   61,  389,  103,  104,   65,
 /*  1010 */   105,  389,   66,   60,  389,  389,  181,  168,  182,  196,
 /*  1020 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*  1030 */   389,  389,  389,  389,  389,  389,  389,  389,  160,   84,
 /*  1040 */   171,   97,  172,  163,   61,  389,  103,  104,   65,  105,
 /*  1050 */   389,   66,   60,  389,  389,  181,  168,  182,  389,  389,
 /*  1060 */   197,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*  1070 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  160,
 /*  1080 */    84,  171,   97,  172,  163,   61,  389,  103,  104,   65,
 /*  1090 */   105,  389,   66,   60,  389,  389,  181,  168,  182,  110,
 /*  1100 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*  1110 */   389,  389,  389,  389,  389,  389,  389,  389,  160,   84,
 /*  1120 */   171,   97,  172,  163,   61,  389,  103,  104,   65,  105,
 /*  1130 */   389,   66,   60,  389,  389,  181,  168,  182,  389,  389,
 /*  1140 */   112,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*  1150 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  160,
 /*  1160 */    84,  171,   97,  172,  163,   61,  389,  103,  104,   65,
 /*  1170 */   105,  389,   66,   60,  389,  389,  181,  168,  182,  215,
 /*  1180 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*  1190 */   389,  389,  389,  389,  389,  389,  389,  389,  160,   84,
 /*  1200 */   171,   97,  172,  163,   61,  389,  103,  104,   65,  105,
 /*  1210 */   389,   66,   60,  389,  389,  181,  168,  182,  389,  389,
 /*  1220 */   231,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*  1230 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  160,
 /*  1240 */    84,  171,   97,  172,  163,   61,  389,  103,  104,   65,
 /*  1250 */   105,  389,   66,   60,  389,  389,  181,  168,  182,  145,
 /*  1260 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*  1270 */   389,  389,  389,  389,  389,  389,  389,  389,  160,   84,
 /*  1280 */   171,   97,  172,  163,   61,  389,  103,  104,   65,  105,
 /*  1290 */    14,   66,   60,  389,  389,  181,  168,  182,  389,  389,
 /*  1300 */   389,  183,  389,  389,  389,  389,  389,  389,  389,   23,
 /*  1310 */   389,  389,  389,  389,  389,  389,  389,  389,  389,  389,
 /*  1320 */   389,  389,  389,   44,   45,  389,  389,  389,   46,   18,
 /*  1330 */   389,  184,  185,  186,  187,  188,  189,  190,  191,   85,
 /*  1340 */   161,   97,  172,  163,   61,  183,  103,  104,   65,  105,
 /*  1350 */   389,   66,   60,   23,  389,  181,  168,  182,  389,  389,
 /*  1360 */   389,  389,  389,  389,  389,  389,  389,   44,   45,  389,
 /*  1370 */   389,  389,   46,   18,  389,  184,  185,  186,  187,  188,
 /*  1380 */   189,  190,  191,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   80,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   90,   61,   92,   93,   19,   20,   96,
 /*    20 */    97,   98,    1,   62,   25,   56,   57,   58,   59,   60,
 /*    30 */    73,   74,   34,   35,   72,   73,   74,   39,   40,   77,
 /*    40 */    42,   43,   44,   45,   46,   47,   48,   49,   79,   80,
 /*    50 */    81,   82,   83,   84,   85,  102,   87,   88,   89,   90,
 /*    60 */    12,   92,   93,  102,   80,   96,   97,   98,   57,   58,
 /*    70 */    59,   60,   88,   89,   90,   54,   92,   93,   73,   74,
 /*    80 */    96,   97,   98,   71,   72,   73,   74,   73,   74,   77,
 /*    90 */    79,   80,   81,   82,   83,   84,   85,   63,   87,   88,
 /*   100 */    89,   90,   68,   92,   93,   95,   80,   96,   97,   98,
 /*   110 */    57,   58,   59,   60,   86,   89,   90,   12,   92,   93,
 /*   120 */    62,    1,   96,   97,   98,    5,   63,   22,   23,   24,
 /*   130 */     1,   68,   79,   80,   81,   82,   83,   84,   85,   94,
 /*   140 */    87,   88,   89,   90,    1,   92,   93,   12,    5,   96,
 /*   150 */    97,   98,   57,   58,   59,   60,   17,   22,   23,   24,
 /*   160 */    69,   70,   71,   72,   73,   74,   27,   28,   77,   73,
 /*   170 */    74,    0,    1,   53,   79,   80,   81,   82,   83,   84,
 /*   180 */    85,   52,   87,   88,   89,   90,  100,   92,   93,   12,
 /*   190 */    91,   96,   97,   98,   57,   58,   59,   60,   23,   22,
 /*   200 */    23,   24,   69,   70,   71,   72,   73,   74,   66,   67,
 /*   210 */    77,   36,   37,   38,   31,   32,   79,   80,   81,   82,
 /*   220 */    83,   84,   85,  101,   87,   88,   89,   90,   40,   92,
 /*   230 */    93,   80,   17,   96,   97,   98,   57,   58,   59,   60,
 /*   240 */     1,   16,    3,   92,   93,   20,   64,   96,   97,   98,
 /*   250 */    72,   73,   74,   34,   35,   77,   12,   18,   79,   80,
 /*   260 */    81,   82,   83,   84,   85,   40,   87,   88,   89,   90,
 /*   270 */    65,   92,   93,   80,   67,   96,   97,   98,   57,   58,
 /*   280 */    59,   60,   22,   16,   75,   76,   93,   20,   74,   96,
 /*   290 */    97,   98,   25,   50,   51,    1,    1,   74,    4,    4,
 /*   300 */    79,   80,   81,   82,   83,   84,   85,   40,   87,   88,
 /*   310 */    89,   90,   80,   92,   93,   80,   76,   96,   97,   98,
 /*   320 */    57,   58,   59,   60,    1,   74,   74,    4,   96,   97,
 /*   330 */    98,   96,   97,   98,    1,    1,   74,    4,    4,   74,
 /*   340 */    74,    4,   79,   80,   81,   82,   83,   84,   85,   74,
 /*   350 */    87,   88,   89,   90,   80,   92,   93,   80,  101,   96,
 /*   360 */    97,   98,   57,   58,   59,   60,    3,    1,    5,    1,
 /*   370 */    96,   97,   98,   96,   97,   98,   26,   28,   30,   33,
 /*   380 */    29,   41,   15,   21,   79,   80,   81,   82,   83,   84,
 /*   390 */    85,   21,   87,   88,   89,   90,    1,   92,   93,    1,
 /*   400 */     4,   96,   97,   98,   57,   58,   59,   60,    1,   12,
 /*   410 */     1,   20,   12,   12,   15,   21,   53,   15,   12,   16,
 /*   420 */    15,   41,    1,   15,   12,   15,   79,   80,   81,   82,
 /*   430 */    83,   84,   85,   15,   87,   88,   89,   90,   12,   92,
 /*   440 */    93,   12,   15,   96,   97,   98,   57,   58,   59,   60,
 /*   450 */    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
 /*   460 */    41,   12,    4,   12,    1,  103,  103,  103,   79,   80,
 /*   470 */    81,   82,   83,   84,   85,  103,   87,   88,   89,   90,
 /*   480 */   103,   92,   93,  103,  103,   96,   97,   98,   57,   58,
 /*   490 */    59,   60,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   500 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   510 */    79,   80,   81,   82,   83,   84,   85,  103,   87,   88,
 /*   520 */    89,   90,  103,   92,   93,  103,  103,   96,   97,   98,
 /*   530 */    57,   58,   59,   60,  103,  103,  103,  103,  103,  103,
 /*   540 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   550 */   103,  103,   79,   80,   81,   82,   83,   84,   85,  103,
 /*   560 */    87,   88,   89,   90,  103,   92,   93,  103,  103,   96,
 /*   570 */    97,   98,   57,   58,   59,   60,  103,  103,  103,  103,
 /*   580 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   590 */   103,  103,  103,  103,   79,   80,   81,   82,   83,   84,
 /*   600 */    85,  103,   87,   88,   89,   90,  103,   92,   93,  103,
 /*   610 */   103,   96,   97,   98,   57,   58,   59,   60,  103,  103,
 /*   620 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   630 */   103,  103,  103,  103,  103,  103,   79,   80,   81,   82,
 /*   640 */    83,   84,   85,  103,   87,   88,   89,   90,  103,   92,
 /*   650 */    93,  103,  103,   96,   97,   98,   57,   58,   59,   60,
 /*   660 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   670 */   103,  103,  103,  103,  103,  103,  103,  103,   79,   80,
 /*   680 */    81,   82,   83,   84,   85,  103,   87,   88,   89,   90,
 /*   690 */   103,   92,   93,  103,   80,   96,   97,   98,   58,   59,
 /*   700 */    60,   87,   88,   89,   90,  103,   92,   93,  103,  103,
 /*   710 */    96,   97,   98,  103,  103,  103,  103,  103,  103,   79,
 /*   720 */    80,   81,   82,   83,   84,   85,  103,   87,   88,   89,
 /*   730 */    90,  103,   92,   93,  103,   80,   96,   97,   98,   60,
 /*   740 */    85,  103,   87,   88,   89,   90,  103,   92,   93,  103,
 /*   750 */   103,   96,   97,   98,  103,  103,  103,   78,   79,   80,
 /*   760 */    81,   82,   83,   84,   85,  103,   87,   88,   89,   90,
 /*   770 */   103,   92,   93,  103,  103,   96,   97,   98,   99,   80,
 /*   780 */    60,  103,   83,   84,   85,  103,   87,   88,   89,   90,
 /*   790 */   103,   92,   93,  103,  103,   96,   97,   98,   78,   79,
 /*   800 */    80,   81,   82,   83,   84,   85,  103,   87,   88,   89,
 /*   810 */    90,  103,   92,   93,  103,  103,   96,   97,   98,   99,
 /*   820 */    60,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   830 */   103,  103,  103,  103,  103,  103,  103,  103,  103,   79,
 /*   840 */    80,   81,   82,   83,   84,   85,  103,   87,   88,   89,
 /*   850 */    90,  103,   92,   93,  103,  103,   96,   97,   98,   60,
 /*   860 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   870 */   103,  103,  103,  103,  103,  103,  103,  103,   79,   80,
 /*   880 */    81,   82,   83,   84,   85,  103,   87,   88,   89,   90,
 /*   890 */   103,   92,   93,  103,  103,   96,   97,   98,  103,  103,
 /*   900 */    60,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   910 */   103,  103,  103,  103,  103,  103,  103,  103,  103,   79,
 /*   920 */    80,   81,   82,   83,   84,   85,  103,   87,   88,   89,
 /*   930 */    90,  103,   92,   93,  103,  103,   96,   97,   98,   60,
 /*   940 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   950 */   103,  103,  103,  103,  103,  103,  103,  103,   79,   80,
 /*   960 */    81,   82,   83,   84,   85,  103,   87,   88,   89,   90,
 /*   970 */   103,   92,   93,  103,  103,   96,   97,   98,  103,  103,
 /*   980 */    60,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   990 */   103,  103,  103,  103,  103,  103,  103,  103,  103,   79,
 /*  1000 */    80,   81,   82,   83,   84,   85,  103,   87,   88,   89,
 /*  1010 */    90,  103,   92,   93,  103,  103,   96,   97,   98,   60,
 /*  1020 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*  1030 */   103,  103,  103,  103,  103,  103,  103,  103,   79,   80,
 /*  1040 */    81,   82,   83,   84,   85,  103,   87,   88,   89,   90,
 /*  1050 */   103,   92,   93,  103,  103,   96,   97,   98,  103,  103,
 /*  1060 */    60,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*  1070 */   103,  103,  103,  103,  103,  103,  103,  103,  103,   79,
 /*  1080 */    80,   81,   82,   83,   84,   85,  103,   87,   88,   89,
 /*  1090 */    90,  103,   92,   93,  103,  103,   96,   97,   98,   60,
 /*  1100 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*  1110 */   103,  103,  103,  103,  103,  103,  103,  103,   79,   80,
 /*  1120 */    81,   82,   83,   84,   85,  103,   87,   88,   89,   90,
 /*  1130 */   103,   92,   93,  103,  103,   96,   97,   98,  103,  103,
 /*  1140 */    60,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*  1150 */   103,  103,  103,  103,  103,  103,  103,  103,  103,   79,
 /*  1160 */    80,   81,   82,   83,   84,   85,  103,   87,   88,   89,
 /*  1170 */    90,  103,   92,   93,  103,  103,   96,   97,   98,   60,
 /*  1180 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*  1190 */   103,  103,  103,  103,  103,  103,  103,  103,   79,   80,
 /*  1200 */    81,   82,   83,   84,   85,  103,   87,   88,   89,   90,
 /*  1210 */   103,   92,   93,  103,  103,   96,   97,   98,  103,  103,
 /*  1220 */    60,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*  1230 */   103,  103,  103,  103,  103,  103,  103,  103,  103,   79,
 /*  1240 */    80,   81,   82,   83,   84,   85,  103,   87,   88,   89,
 /*  1250 */    90,  103,   92,   93,  103,  103,   96,   97,   98,   60,
 /*  1260 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*  1270 */   103,  103,  103,  103,  103,  103,  103,  103,   79,   80,
 /*  1280 */    81,   82,   83,   84,   85,  103,   87,   88,   89,   90,
 /*  1290 */     1,   92,   93,  103,  103,   96,   97,   98,  103,  103,
 /*  1300 */   103,   12,  103,  103,  103,  103,  103,  103,  103,   20,
 /*  1310 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*  1320 */   103,  103,  103,   34,   35,  103,  103,  103,   39,   40,
 /*  1330 */   103,   42,   43,   44,   45,   46,   47,   48,   49,   80,
 /*  1340 */    81,   82,   83,   84,   85,   12,   87,   88,   89,   90,
 /*  1350 */   103,   92,   93,   20,  103,   96,   97,   98,  103,  103,
 /*  1360 */   103,  103,  103,  103,  103,  103,  103,   34,   35,  103,
 /*  1370 */   103,  103,   39,   40,  103,   42,   43,   44,   45,   46,
 /*  1380 */    47,   48,   49,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 152
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2, 1333, 1333, 1289,
 /*    20 */  1333, 1333, 1333, 1333, 1333, 1333, 1333, 1333, 1333, 1333,
 /*    30 */  1333, 1333, 1333, 1333, 1333, 1333, 1333, 1333,  105,  105,
 /*    40 */  1333, 1333,  135, 1333, 1333, 1333, 1333,  177,  177,  120,
 /*    50 */   363,  239,  239,  -21,  -21,  -21,  -21,   48,   -1,  175,
 /*    60 */   175,  139,  143,  183,  219,  183,  219,  243,  188,  215,
 /*    70 */   244,   48,  260,  260,   -1,  260,  260,   -1,  260,  260,
 /*    80 */   260,  260,   -1,  188,  267,  225,  171,  294,  295,  323,
 /*    90 */   333,  129,   21,  334,  337,  366,  368,  350,  349,  351,
 /*   100 */   348,  346,  367,  351,  348,  346,  340,  362,  370,  395,
 /*   110 */   398,  396,  407,  366,  397,  409,  399,  400,  402,  403,
 /*   120 */   401,  403,  406,  391,  394,  405,  408,  410,  418,  412,
 /*   130 */   426,  427,  429,  435,  436,  437,  438,  439,  440,  441,
 /*   140 */   442,  443,  444,  380,  421,  419,  449,  458,  451,  463,
 /*   150 */   366,  366,  366,
};
#define YY_REDUCE_USE_DFLT (-78)
#define YY_REDUCE_MAX 83
static const short yy_reduce_ofst[] = {
 /*     0 */   -31,   11,   53,   95,  137,  179,  221,  263,  305,  347,
 /*    10 */   389,  431,  473,  515,  557,  599,  640,  679,  720,  760,
 /*    20 */   799,  840,  879,  920,  959, 1000, 1039, 1080, 1119, 1160,
 /*    30 */  1199, 1259,  699,  655,  614,  -16,   26,  -77,   91,  133,
 /*    40 */   151,  193,   12,  232,  235,  274,  277,  -38,  178,  -47,
 /*    50 */   -39,   34,   63,  -43,    5,   14,   96,  142,  209,   10,
 /*    60 */    10,   28,   58,   99,   45,   99,   45,   86,  122,  182,
 /*    70 */   205,  207,  214,  223,  240,  251,  252,  240,  262,  265,
 /*    80 */   266,  275,  240,  257,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   245,  245,  245,  245,  245,  245,  245,  245,  245,  245,
 /*    10 */   245,  245,  245,  245,  245,  245,  245,  373,  373,  387,
 /*    20 */   387,  252,  387,  387,  254,  256,  387,  387,  387,  387,
 /*    30 */   387,  387,  387,  387,  387,  387,  387,  387,  306,  306,
 /*    40 */   387,  387,  387,  387,  387,  387,  387,  387,  387,  387,
 /*    50 */   385,  272,  272,  387,  387,  387,  387,  387,  310,  344,
 /*    60 */   343,  327,  385,  336,  342,  335,  341,  375,  378,  268,
 /*    70 */   387,  387,  387,  387,  387,  387,  387,  314,  387,  387,
 /*    80 */   387,  387,  313,  378,  357,  357,  387,  387,  387,  387,
 /*    90 */   387,  387,  387,  387,  387,  386,  387,  323,  328,  332,
 /*   100 */   334,  338,  374,  331,  333,  337,  387,  387,  387,  387,
 /*   110 */   387,  387,  387,  273,  387,  387,  260,  387,  261,  263,
 /*   120 */   387,  262,  387,  387,  387,  290,  282,  278,  276,  387,
 /*   130 */   387,  280,  387,  286,  284,  288,  298,  294,  292,  296,
 /*   140 */   302,  300,  304,  387,  387,  387,  387,  387,  387,  387,
 /*   150 */   382,  383,  384,  244,  246,  247,  243,  248,  251,  253,
 /*   160 */   320,  321,  325,  326,  347,  353,  354,  355,  356,  318,
 /*   170 */   319,  322,  324,  339,  340,  345,  346,  349,  350,  351,
 /*   180 */   352,  348,  358,  362,  363,  364,  365,  366,  367,  368,
 /*   190 */   369,  370,  371,  372,  359,  376,  255,  257,  258,  270,
 /*   200 */   271,  259,  267,  266,  265,  264,  274,  275,  307,  277,
 /*   210 */   308,  279,  281,  309,  311,  312,  316,  317,  283,  285,
 /*   220 */   287,  289,  315,  291,  293,  295,  297,  299,  301,  303,
 /*   230 */   305,  269,  379,  377,  360,  361,  329,  330,  249,  381,
 /*   240 */   250,  380,
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
  "AMPER",         "EQUAL",         "AND_AND",       "LESS",        
  "XOR",           "BAR",           "AND",           "LSHIFT",      
  "RSHIFT",        "EQUAL_TILDA",   "PLUS",          "MINUS",       
  "DIV",           "DIV_DIV",       "PERCENT",       "TILDA",       
  "LBRACKET",      "RBRACKET",      "NUMBER",        "REGEXP",      
  "STRING",        "SYMBOL",        "NIL",           "TRUE",        
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
  "and_expr",      "shift_expr",    "match_expr",    "shift_op",    
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
 /*  83 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /*  84 */ "not_expr ::= comparison",
 /*  85 */ "comparison ::= xor_expr",
 /*  86 */ "comparison ::= xor_expr comp_op xor_expr",
 /*  87 */ "comp_op ::= LESS",
 /*  88 */ "comp_op ::= GREATER",
 /*  89 */ "xor_expr ::= or_expr",
 /*  90 */ "xor_expr ::= xor_expr XOR or_expr",
 /*  91 */ "or_expr ::= and_expr",
 /*  92 */ "or_expr ::= or_expr BAR and_expr",
 /*  93 */ "and_expr ::= shift_expr",
 /*  94 */ "and_expr ::= and_expr AND shift_expr",
 /*  95 */ "shift_expr ::= match_expr",
 /*  96 */ "shift_expr ::= shift_expr shift_op match_expr",
 /*  97 */ "shift_op ::= LSHIFT",
 /*  98 */ "shift_op ::= RSHIFT",
 /*  99 */ "match_expr ::= arith_expr",
 /* 100 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 101 */ "arith_expr ::= term",
 /* 102 */ "arith_expr ::= arith_expr arith_op term",
 /* 103 */ "arith_op ::= PLUS",
 /* 104 */ "arith_op ::= MINUS",
 /* 105 */ "term ::= term term_op factor",
 /* 106 */ "term ::= factor",
 /* 107 */ "term_op ::= STAR",
 /* 108 */ "term_op ::= DIV",
 /* 109 */ "term_op ::= DIV_DIV",
 /* 110 */ "term_op ::= PERCENT",
 /* 111 */ "factor ::= PLUS factor",
 /* 112 */ "factor ::= MINUS factor",
 /* 113 */ "factor ::= TILDA factor",
 /* 114 */ "factor ::= power",
 /* 115 */ "power ::= postfix_expr",
 /* 116 */ "postfix_expr ::= atom",
 /* 117 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 118 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 119 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 120 */ "atom ::= NAME",
 /* 121 */ "atom ::= NUMBER",
 /* 122 */ "atom ::= REGEXP",
 /* 123 */ "atom ::= STRING",
 /* 124 */ "atom ::= SYMBOL",
 /* 125 */ "atom ::= NIL",
 /* 126 */ "atom ::= TRUE",
 /* 127 */ "atom ::= FALSE",
 /* 128 */ "atom ::= LINE",
 /* 129 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 130 */ "atom ::= LPAR expr RPAR",
 /* 131 */ "args_opt ::=",
 /* 132 */ "args_opt ::= args",
 /* 133 */ "blockarg_opt ::=",
 /* 134 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 135 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 136 */ "blockarg_params_opt ::=",
 /* 137 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 138 */ "excepts ::= except",
 /* 139 */ "excepts ::= excepts except",
 /* 140 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 141 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 142 */ "except ::= EXCEPT NEWLINE stmts",
 /* 143 */ "finally_opt ::=",
 /* 144 */ "finally_opt ::= FINALLY stmts",
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
  { 56, 1 },
  { 57, 1 },
  { 57, 3 },
  { 58, 0 },
  { 58, 1 },
  { 58, 1 },
  { 58, 7 },
  { 58, 5 },
  { 58, 5 },
  { 58, 5 },
  { 58, 1 },
  { 58, 2 },
  { 58, 1 },
  { 58, 2 },
  { 58, 1 },
  { 58, 2 },
  { 58, 6 },
  { 58, 6 },
  { 58, 2 },
  { 58, 2 },
  { 66, 1 },
  { 66, 3 },
  { 67, 1 },
  { 67, 3 },
  { 65, 1 },
  { 65, 3 },
  { 64, 0 },
  { 64, 2 },
  { 63, 1 },
  { 63, 5 },
  { 68, 0 },
  { 68, 2 },
  { 59, 7 },
  { 69, 9 },
  { 69, 7 },
  { 69, 7 },
  { 69, 5 },
  { 69, 7 },
  { 69, 5 },
  { 69, 5 },
  { 69, 3 },
  { 69, 7 },
  { 69, 5 },
  { 69, 5 },
  { 69, 3 },
  { 69, 5 },
  { 69, 3 },
  { 69, 3 },
  { 69, 1 },
  { 69, 7 },
  { 69, 5 },
  { 69, 5 },
  { 69, 3 },
  { 69, 5 },
  { 69, 3 },
  { 69, 3 },
  { 69, 1 },
  { 69, 5 },
  { 69, 3 },
  { 69, 3 },
  { 69, 1 },
  { 69, 3 },
  { 69, 1 },
  { 69, 1 },
  { 69, 0 },
  { 74, 2 },
  { 73, 2 },
  { 72, 3 },
  { 75, 0 },
  { 75, 1 },
  { 76, 2 },
  { 70, 1 },
  { 70, 3 },
  { 71, 1 },
  { 71, 3 },
  { 77, 2 },
  { 78, 1 },
  { 78, 3 },
  { 60, 1 },
  { 79, 3 },
  { 79, 1 },
  { 81, 1 },
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
  { 88, 1 },
  { 88, 3 },
  { 89, 1 },
  { 89, 3 },
  { 91, 1 },
  { 91, 1 },
  { 90, 1 },
  { 90, 3 },
  { 92, 1 },
  { 92, 3 },
  { 94, 1 },
  { 94, 1 },
  { 93, 3 },
  { 93, 1 },
  { 95, 1 },
  { 95, 1 },
  { 95, 1 },
  { 95, 1 },
  { 96, 2 },
  { 96, 2 },
  { 96, 2 },
  { 96, 1 },
  { 97, 1 },
  { 80, 1 },
  { 80, 5 },
  { 80, 4 },
  { 80, 3 },
  { 98, 1 },
  { 98, 1 },
  { 98, 1 },
  { 98, 1 },
  { 98, 1 },
  { 98, 1 },
  { 98, 1 },
  { 98, 1 },
  { 98, 1 },
  { 98, 3 },
  { 98, 3 },
  { 99, 0 },
  { 99, 1 },
  { 100, 0 },
  { 100, 5 },
  { 100, 5 },
  { 101, 0 },
  { 101, 3 },
  { 61, 1 },
  { 61, 2 },
  { 102, 6 },
  { 102, 4 },
  { 102, 3 },
  { 62, 0 },
  { 62, 2 },
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
#line 626 "parser.y"
{
    *pval = yymsp[0].minor.yy175;
}
#line 1928 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 138: /* excepts ::= except */
#line 630 "parser.y"
{
    yygotominor.yy175 = make_array_with(env, yymsp[0].minor.yy175);
}
#line 1939 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 633 "parser.y"
{
    yygotominor.yy175 = Array_push(env, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 1949 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 131: /* args_opt ::= */
      case 133: /* blockarg_opt ::= */
      case 136: /* blockarg_params_opt ::= */
      case 143: /* finally_opt ::= */
#line 637 "parser.y"
{
    yygotominor.yy175 = YNIL;
}
#line 1963 "parser.c"
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
      case 84: /* not_expr ::= comparison */
      case 85: /* comparison ::= xor_expr */
      case 89: /* xor_expr ::= or_expr */
      case 91: /* or_expr ::= and_expr */
      case 93: /* and_expr ::= shift_expr */
      case 95: /* shift_expr ::= match_expr */
      case 99: /* match_expr ::= arith_expr */
      case 101: /* arith_expr ::= term */
      case 106: /* term ::= factor */
      case 114: /* factor ::= power */
      case 115: /* power ::= postfix_expr */
      case 116: /* postfix_expr ::= atom */
      case 132: /* args_opt ::= args */
      case 144: /* finally_opt ::= FINALLY stmts */
#line 640 "parser.y"
{
    yygotominor.yy175 = yymsp[0].minor.yy175;
}
#line 1994 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 646 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy175 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy175, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2002 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 650 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy175 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-2].minor.yy175, YNIL, yymsp[-1].minor.yy175);
}
#line 2010 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 654 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy175 = Finally_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2018 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 658 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy175 = While_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2026 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 662 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Break_new(env, lineno, YNIL);
}
#line 2034 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 666 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy175 = Break_new(env, lineno, yymsp[0].minor.yy175);
}
#line 2042 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 670 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Next_new(env, lineno, YNIL);
}
#line 2050 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 674 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy175 = Next_new(env, lineno, yymsp[0].minor.yy175);
}
#line 2058 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 678 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Return_new(env, lineno, YNIL);
}
#line 2066 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 682 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy175 = Return_new(env, lineno, yymsp[0].minor.yy175);
}
#line 2074 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 686 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy175 = If_new(env, lineno, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2082 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 690 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy175 = Klass_new(env, lineno, id, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2091 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 695 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy175 = Nonlocal_new(env, lineno, yymsp[0].minor.yy175);
}
#line 2099 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 699 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy175 = Import_new(env, lineno, yymsp[0].minor.yy175);
}
#line 2107 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 711 "parser.y"
{
    yygotominor.yy175 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2115 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 714 "parser.y"
{
    yygotominor.yy175 = Array_push_token_id(env, yymsp[-2].minor.yy175, yymsp[0].minor.yy0);
}
#line 2123 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 735 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175, yymsp[0].minor.yy175);
    yygotominor.yy175 = make_array_with(env, node);
}
#line 2132 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 748 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy175 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2141 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 754 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-8].minor.yy175, yymsp[-6].minor.yy175, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2148 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 757 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-6].minor.yy175, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL);
}
#line 2155 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 760 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-6].minor.yy175, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175);
}
#line 2162 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 763 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL, YNIL);
}
#line 2169 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 766 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-6].minor.yy175, yymsp[-4].minor.yy175, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2176 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 769 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175, YNIL);
}
#line 2183 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 772 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, YNIL, YNIL, yymsp[0].minor.yy175);
}
#line 2190 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 775 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL, YNIL, YNIL);
}
#line 2197 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 778 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-6].minor.yy175, YNIL, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2204 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 781 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL);
}
#line 2211 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 784 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, YNIL, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175);
}
#line 2218 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 787 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175, YNIL, YNIL);
}
#line 2225 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 790 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-4].minor.yy175, YNIL, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2232 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 793 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-2].minor.yy175, YNIL, YNIL, yymsp[0].minor.yy175, YNIL);
}
#line 2239 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 796 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[-2].minor.yy175, YNIL, YNIL, YNIL, yymsp[0].minor.yy175);
}
#line 2246 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 799 "parser.y"
{
    yygotominor.yy175 = Params_new(env, yymsp[0].minor.yy175, YNIL, YNIL, YNIL, YNIL);
}
#line 2253 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 802 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-6].minor.yy175, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2260 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 805 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL);
}
#line 2267 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 808 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175);
}
#line 2274 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 811 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL, YNIL);
}
#line 2281 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 814 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-4].minor.yy175, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2288 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 817 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175, YNIL);
}
#line 2295 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 820 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[-2].minor.yy175, YNIL, YNIL, yymsp[0].minor.yy175);
}
#line 2302 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 823 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, yymsp[0].minor.yy175, YNIL, YNIL, YNIL);
}
#line 2309 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 826 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2316 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 829 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175, YNIL);
}
#line 2323 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 832 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy175, YNIL, yymsp[0].minor.yy175);
}
#line 2330 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 835 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy175, YNIL, YNIL);
}
#line 2337 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 838 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2344 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 841 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy175, YNIL);
}
#line 2351 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 844 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy175);
}
#line 2358 "parser.c"
        break;
      case 64: /* params ::= */
#line 847 "parser.y"
{
    yygotominor.yy175 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2365 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 851 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy175 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2374 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 857 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy175 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2383 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 863 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy175 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy175);
}
#line 2392 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 880 "parser.y"
{
    yygotominor.yy175 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy175, lineno, id, YNIL);
}
#line 2402 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 886 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy175, lineno, id, YNIL);
    yygotominor.yy175 = yymsp[-2].minor.yy175;
}
#line 2412 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 900 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy175 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy175);
}
#line 2421 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 917 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy175);
    yygotominor.yy175 = Assign_new(env, lineno, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2429 "parser.c"
        break;
      case 83: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 932 "parser.y"
{
    yygotominor.yy175 = YogNode_new(env, NODE_LOGICAL_AND, NODE_LINENO(yymsp[-2].minor.yy175));
    NODE(yygotominor.yy175)->u.logical_and.left = yymsp[-2].minor.yy175;
    NODE(yygotominor.yy175)->u.logical_and.right = yymsp[0].minor.yy175;
}
#line 2438 "parser.c"
        break;
      case 86: /* comparison ::= xor_expr comp_op xor_expr */
#line 945 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy175);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy175)->u.id;
    yygotominor.yy175 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy175, id, yymsp[0].minor.yy175);
}
#line 2447 "parser.c"
        break;
      case 87: /* comp_op ::= LESS */
      case 88: /* comp_op ::= GREATER */
#line 951 "parser.y"
{
    yygotominor.yy175 = yymsp[0].minor.yy0;
}
#line 2455 "parser.c"
        break;
      case 90: /* xor_expr ::= xor_expr XOR or_expr */
      case 92: /* or_expr ::= or_expr BAR and_expr */
      case 94: /* and_expr ::= and_expr AND shift_expr */
#line 961 "parser.y"
{
    yygotominor.yy175 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy175), yymsp[-2].minor.yy175, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy175);
}
#line 2464 "parser.c"
        break;
      case 96: /* shift_expr ::= shift_expr shift_op match_expr */
      case 102: /* arith_expr ::= arith_expr arith_op term */
      case 105: /* term ::= term term_op factor */
#line 982 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy175);
    yygotominor.yy175 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy175, VAL2ID(yymsp[-1].minor.yy175), yymsp[0].minor.yy175);
}
#line 2474 "parser.c"
        break;
      case 97: /* shift_op ::= LSHIFT */
      case 98: /* shift_op ::= RSHIFT */
      case 103: /* arith_op ::= PLUS */
      case 104: /* arith_op ::= MINUS */
      case 107: /* term_op ::= STAR */
      case 108: /* term_op ::= DIV */
      case 109: /* term_op ::= DIV_DIV */
      case 110: /* term_op ::= PERCENT */
#line 987 "parser.y"
{
    yygotominor.yy175 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2488 "parser.c"
        break;
      case 100: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 997 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy175);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy175 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy175, id, yymsp[0].minor.yy175);
}
#line 2497 "parser.c"
        break;
      case 111: /* factor ::= PLUS factor */
#line 1039 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy175 = FuncCall_new3(env, lineno, yymsp[0].minor.yy175, id);
}
#line 2506 "parser.c"
        break;
      case 112: /* factor ::= MINUS factor */
#line 1044 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy175 = FuncCall_new3(env, lineno, yymsp[0].minor.yy175, id);
}
#line 2515 "parser.c"
        break;
      case 113: /* factor ::= TILDA factor */
#line 1049 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy175 = FuncCall_new3(env, lineno, yymsp[0].minor.yy175, id);
}
#line 2524 "parser.c"
        break;
      case 117: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1065 "parser.y"
{
    yygotominor.yy175 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy175), yymsp[-4].minor.yy175, yymsp[-2].minor.yy175, yymsp[0].minor.yy175);
}
#line 2531 "parser.c"
        break;
      case 118: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1068 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy175);
    yygotominor.yy175 = Subscript_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2539 "parser.c"
        break;
      case 119: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1072 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy175);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy175 = Attr_new(env, lineno, yymsp[-2].minor.yy175, id);
}
#line 2548 "parser.c"
        break;
      case 120: /* atom ::= NAME */
#line 1078 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy175 = Variable_new(env, lineno, id);
}
#line 2557 "parser.c"
        break;
      case 121: /* atom ::= NUMBER */
      case 122: /* atom ::= REGEXP */
      case 123: /* atom ::= STRING */
      case 124: /* atom ::= SYMBOL */
#line 1083 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy175 = Literal_new(env, lineno, val);
}
#line 2569 "parser.c"
        break;
      case 125: /* atom ::= NIL */
#line 1103 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Literal_new(env, lineno, YNIL);
}
#line 2577 "parser.c"
        break;
      case 126: /* atom ::= TRUE */
#line 1107 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Literal_new(env, lineno, YTRUE);
}
#line 2585 "parser.c"
        break;
      case 127: /* atom ::= FALSE */
#line 1111 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy175 = Literal_new(env, lineno, YFALSE);
}
#line 2593 "parser.c"
        break;
      case 128: /* atom ::= LINE */
#line 1115 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy175 = Literal_new(env, lineno, val);
}
#line 2602 "parser.c"
        break;
      case 129: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1120 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy175 = Array_new(env, lineno, yymsp[-1].minor.yy175);
}
#line 2610 "parser.c"
        break;
      case 130: /* atom ::= LPAR expr RPAR */
      case 137: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1124 "parser.y"
{
    yygotominor.yy175 = yymsp[-1].minor.yy175;
}
#line 2618 "parser.c"
        break;
      case 134: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 135: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1138 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy175 = BlockArg_new(env, lineno, yymsp[-3].minor.yy175, yymsp[-1].minor.yy175);
}
#line 2627 "parser.c"
        break;
      case 139: /* excepts ::= excepts except */
#line 1157 "parser.y"
{
    yygotominor.yy175 = Array_push(env, yymsp[-1].minor.yy175, yymsp[0].minor.yy175);
}
#line 2634 "parser.c"
        break;
      case 140: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1161 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy175 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy175, id, yymsp[0].minor.yy175);
}
#line 2644 "parser.c"
        break;
      case 141: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1167 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy175 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy175, NO_EXC_VAR, yymsp[0].minor.yy175);
}
#line 2652 "parser.c"
        break;
      case 142: /* except ::= EXCEPT NEWLINE stmts */
#line 1171 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy175 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy175);
}
#line 2660 "parser.c"
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
