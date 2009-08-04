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
#define YYNOCODE 100
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy1;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 235
#define YYNRULE 141
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
 /*     0 */     1,  123,  124,  198,   20,   21,   24,   25,   26,  108,
 /*    10 */   176,   67,   54,  120,  127,  129,  214,  116,   23,  215,
 /*    20 */    28,  377,   83,  150,  148,  149,  230,  121,  125,  205,
 /*    30 */    42,   43,  209,   47,   18,  229,  177,  178,  179,  180,
 /*    40 */   181,  182,  183,  184,  154,   81,  166,  156,  157,  158,
 /*    50 */    59,  176,   94,   98,   62,   99,  140,   63,   57,   23,
 /*    60 */    17,  174,  163,  175,   46,  150,  148,  149,  131,  133,
 /*    70 */   219,   42,   43,  209,  234,   18,   30,  177,  178,  179,
 /*    80 */   180,  181,  182,  183,  184,   41,  154,   81,  166,  156,
 /*    90 */   157,  158,   59,  105,   94,   98,   62,   99,  192,   63,
 /*   100 */    57,   91,   82,  174,  163,  175,   58,  150,  148,  149,
 /*   110 */    95,   62,   99,   79,   63,   57,  193,   32,  174,  163,
 /*   120 */   175,  192,  141,  123,  124,  126,  206,  207,  154,   81,
 /*   130 */   166,  156,  157,  158,   59,   35,   94,   98,   62,   99,
 /*   140 */    16,   63,   57,   82,    3,  174,  163,  175,   92,  150,
 /*   150 */   148,  149,   60,   99,   74,   63,   57,  171,   39,  174,
 /*   160 */   163,  175,  232,   16,  123,  124,  126,   15,  172,  173,
 /*   170 */   154,   81,  166,  156,  157,  158,   59,  187,   94,   98,
 /*   180 */    62,   99,   37,   63,   57,   82,  103,  174,  163,  175,
 /*   190 */    84,  150,  148,  149,   71,   96,  109,   63,   57,  122,
 /*   200 */   202,  174,  163,  175,  123,  124,  126,   16,   29,    8,
 /*   210 */   196,   19,  154,   81,  166,  156,  157,  158,   59,  113,
 /*   220 */    94,   98,   62,   99,   27,   63,   57,  128,  212,  174,
 /*   230 */   163,  175,   85,  150,  148,  149,  118,  119,  130,  134,
 /*   240 */   136,  223,  132,  217,  215,  137,  119,  130,  134,  136,
 /*   250 */   223,  112,  115,  215,  154,   81,  166,  156,  157,  158,
 /*   260 */    59,  110,   94,   98,   62,   99,   82,   63,   57,   82,
 /*   270 */   123,  174,  163,  175,   48,  150,  148,  149,   61,   57,
 /*   280 */   167,  168,  174,  163,  175,  160,  163,  175,    2,  200,
 /*   290 */     3,  135,  221,  169,  170,  204,  154,   81,  166,  156,
 /*   300 */   157,  158,   59,  210,   94,   98,   62,   99,   82,   63,
 /*   310 */    57,   82,  211,  174,  163,  175,   49,  150,  148,  149,
 /*   320 */   213,   56,   65,   80,  174,  163,  175,  161,  163,  175,
 /*   330 */   235,   16,   16,   16,   19,  152,  188,  216,  154,   81,
 /*   340 */   166,  156,  157,  158,   59,   16,   94,   98,   62,   99,
 /*   350 */    82,   63,   57,  218,  220,  174,  163,  175,  107,  150,
 /*   360 */   148,  149,   16,   13,  222,  194,  162,  163,  175,   16,
 /*   370 */    16,  151,  199,  233,  138,   16,    4,   33,   22,   34,
 /*   380 */   154,   81,  166,  156,  157,  158,   59,   38,   94,   98,
 /*   390 */    62,   99,  226,   63,   57,  185,    5,  174,  163,  175,
 /*   400 */    86,  150,  148,  149,  186,   64,    6,  191,    7,   66,
 /*   410 */     9,  111,  142,  195,   68,   10,  114,   40,  197,   44,
 /*   420 */   117,   11,  154,   81,  166,  156,  157,  158,   59,  201,
 /*   430 */    94,   98,   62,   99,   36,   63,   57,  203,   50,  174,
 /*   440 */   163,  175,   87,  150,  148,  149,   55,   69,   70,  225,
 /*   450 */    51,   72,   73,   45,   52,   75,   76,   53,   77,   78,
 /*   460 */   228,  227,  231,  143,  154,   81,  166,  156,  157,  158,
 /*   470 */    59,   12,   94,   98,   62,   99,  378,   63,   57,  378,
 /*   480 */   378,  174,  163,  175,   88,  150,  148,  149,  378,  378,
 /*   490 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*   500 */   378,  378,  378,  378,  378,  378,  154,   81,  166,  156,
 /*   510 */   157,  158,   59,  378,   94,   98,   62,   99,  378,   63,
 /*   520 */    57,  378,  378,  174,  163,  175,  144,  150,  148,  149,
 /*   530 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*   540 */   378,  378,  378,  378,  378,  378,  378,  378,  154,   81,
 /*   550 */   166,  156,  157,  158,   59,  378,   94,   98,   62,   99,
 /*   560 */   378,   63,   57,  378,  378,  174,  163,  175,  145,  150,
 /*   570 */   148,  149,  378,  378,  378,  378,  378,  378,  378,  378,
 /*   580 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*   590 */   154,   81,  166,  156,  157,  158,   59,  378,   94,   98,
 /*   600 */    62,   99,  378,   63,   57,  378,  378,  174,  163,  175,
 /*   610 */   146,  150,  148,  149,  378,  378,  378,  378,  378,  378,
 /*   620 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*   630 */   378,  378,  154,   81,  166,  156,  157,  158,   59,  378,
 /*   640 */    94,   98,   62,   99,  378,   63,   57,  378,  378,  174,
 /*   650 */   163,  175,   90,  150,  148,  149,  378,  378,  378,  378,
 /*   660 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*   670 */   378,  378,  378,  378,  154,   81,  166,  156,  157,  158,
 /*   680 */    59,  378,   94,   98,   62,   99,  378,   63,   57,  378,
 /*   690 */   378,  174,  163,  175,  140,  147,  148,  149,   17,  378,
 /*   700 */   378,  378,  378,   31,  378,  378,  378,  378,  378,  378,
 /*   710 */   378,  378,  378,  378,   30,  378,  154,   81,  166,  156,
 /*   720 */   157,  158,   59,  378,   94,   98,   62,   99,  378,   63,
 /*   730 */    57,  164,  378,  174,  163,  175,  378,  378,  378,  378,
 /*   740 */   378,  378,  378,  378,  378,  378,  378,  378,  378,   97,
 /*   750 */   154,   81,  166,  156,  157,  158,   59,  378,   94,   98,
 /*   760 */    62,   99,  378,   63,   57,  378,  378,  174,  163,  175,
 /*   770 */   102,  378,   82,  164,  378,  378,  378,  159,  378,   94,
 /*   780 */    98,   62,   99,  378,   63,   57,  378,  378,  174,  163,
 /*   790 */   175,   97,  154,   81,  166,  156,  157,  158,   59,  378,
 /*   800 */    94,   98,   62,   99,  378,   63,   57,   89,  378,  174,
 /*   810 */   163,  175,  100,  378,  378,  378,  378,  378,  378,  378,
 /*   820 */   378,  378,  378,  378,  378,  378,  154,   81,  166,  156,
 /*   830 */   157,  158,   59,  378,   94,   98,   62,   99,  378,   63,
 /*   840 */    57,  378,  378,  174,  163,  175,  378,  378,   93,  378,
 /*   850 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*   860 */   378,  378,  378,  378,  378,  378,  378,  154,   81,  166,
 /*   870 */   156,  157,  158,   59,  378,   94,   98,   62,   99,  378,
 /*   880 */    63,   57,  378,  153,  174,  163,  175,  378,  378,  378,
 /*   890 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*   900 */   378,  378,  154,   81,  166,  156,  157,  158,   59,  378,
 /*   910 */    94,   98,   62,   99,  378,   63,   57,  378,  378,  174,
 /*   920 */   163,  175,  378,  378,  165,  378,  378,  378,  378,  378,
 /*   930 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*   940 */   378,  378,  378,  154,   81,  166,  156,  157,  158,   59,
 /*   950 */   378,   94,   98,   62,   99,  378,   63,   57,  378,  101,
 /*   960 */   174,  163,  175,  378,  378,  378,  378,  378,  378,  378,
 /*   970 */   378,  378,  378,  378,  378,  378,  378,  378,  154,   81,
 /*   980 */   166,  156,  157,  158,   59,  378,   94,   98,   62,   99,
 /*   990 */   378,   63,   57,  378,  378,  174,  163,  175,  378,  378,
 /*  1000 */   189,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*  1010 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  154,
 /*  1020 */    81,  166,  156,  157,  158,   59,  378,   94,   98,   62,
 /*  1030 */    99,  378,   63,   57,  378,  190,  174,  163,  175,  378,
 /*  1040 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*  1050 */   378,  378,  378,  378,  154,   81,  166,  156,  157,  158,
 /*  1060 */    59,  378,   94,   98,   62,   99,  378,   63,   57,  378,
 /*  1070 */   378,  174,  163,  175,  378,  378,  104,  378,  378,  378,
 /*  1080 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*  1090 */   378,  378,  378,  378,  378,  154,   81,  166,  156,  157,
 /*  1100 */   158,   59,  378,   94,   98,   62,   99,  378,   63,   57,
 /*  1110 */   378,  106,  174,  163,  175,  378,  378,  378,  378,  378,
 /*  1120 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*  1130 */   154,   81,  166,  156,  157,  158,   59,  378,   94,   98,
 /*  1140 */    62,   99,  378,   63,   57,  378,  378,  174,  163,  175,
 /*  1150 */   378,  378,  208,  378,  378,  378,  378,  378,  378,  378,
 /*  1160 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*  1170 */   378,  154,   81,  166,  156,  157,  158,   59,  378,   94,
 /*  1180 */    98,   62,   99,  378,   63,   57,  378,  224,  174,  163,
 /*  1190 */   175,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*  1200 */   378,  378,  378,  378,  378,  378,  154,   81,  166,  156,
 /*  1210 */   157,  158,   59,  378,   94,   98,   62,   99,  378,   63,
 /*  1220 */    57,  378,  378,  174,  163,  175,  378,  378,  139,  378,
 /*  1230 */   378,  378,  378,  378,  378,  378,  378,  378,  378,  378,
 /*  1240 */   378,  378,  378,  378,  378,  378,  378,  154,   81,  166,
 /*  1250 */   156,  157,  158,   59,  378,   94,   98,   62,   99,  378,
 /*  1260 */    63,   57,   14,  378,  174,  163,  175,   82,  155,  156,
 /*  1270 */   157,  158,   59,  176,   94,   98,   62,   99,  378,   63,
 /*  1280 */    57,   23,  378,  174,  163,  175,  378,  378,  378,  378,
 /*  1290 */   378,  378,  378,   42,   43,  378,  378,   18,  378,  177,
 /*  1300 */   178,  179,  180,  181,  182,  183,  184,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   12,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   67,   68,   69,   70,   19,   20,   73,
 /*    20 */    25,   52,   53,   54,   55,   56,   17,   68,   69,   70,
 /*    30 */    32,   33,   73,   57,   36,   26,   38,   39,   40,   41,
 /*    40 */    42,   43,   44,   45,   75,   76,   77,   78,   79,   80,
 /*    50 */    81,   12,   83,   84,   85,   86,   16,   88,   89,   20,
 /*    60 */    20,   92,   93,   94,   53,   54,   55,   56,   68,   69,
 /*    70 */    70,   32,   33,   73,   98,   36,   36,   38,   39,   40,
 /*    80 */    41,   42,   43,   44,   45,   91,   75,   76,   77,   78,
 /*    90 */    79,   80,   81,   59,   83,   84,   85,   86,   64,   88,
 /*   100 */    89,   58,   76,   92,   93,   94,   53,   54,   55,   56,
 /*   110 */    84,   85,   86,   12,   88,   89,   59,   82,   92,   93,
 /*   120 */    94,   64,   58,   22,   23,   24,   71,   72,   75,   76,
 /*   130 */    77,   78,   79,   80,   81,   87,   83,   84,   85,   86,
 /*   140 */     1,   88,   89,   76,    5,   92,   93,   94,   53,   54,
 /*   150 */    55,   56,   85,   86,   12,   88,   89,   23,   90,   92,
 /*   160 */    93,   94,   98,    1,   22,   23,   24,    5,   34,   35,
 /*   170 */    75,   76,   77,   78,   79,   80,   81,   96,   83,   84,
 /*   180 */    85,   86,   36,   88,   89,   76,   97,   92,   93,   94,
 /*   190 */    53,   54,   55,   56,   12,   86,   60,   88,   89,   69,
 /*   200 */    70,   92,   93,   94,   22,   23,   24,    1,   17,    3,
 /*   210 */    12,   49,   75,   76,   77,   78,   79,   80,   81,   63,
 /*   220 */    83,   84,   85,   86,   18,   88,   89,   69,   70,   92,
 /*   230 */    93,   94,   53,   54,   55,   56,   65,   66,   67,   68,
 /*   240 */    69,   70,   69,   70,   73,   65,   66,   67,   68,   69,
 /*   250 */    70,   62,   63,   73,   75,   76,   77,   78,   79,   80,
 /*   260 */    81,   61,   83,   84,   85,   86,   76,   88,   89,   76,
 /*   270 */    22,   92,   93,   94,   53,   54,   55,   56,   88,   89,
 /*   280 */    29,   30,   92,   93,   94,   92,   93,   94,    3,   70,
 /*   290 */     5,   69,   70,   32,   33,   70,   75,   76,   77,   78,
 /*   300 */    79,   80,   81,   72,   83,   84,   85,   86,   76,   88,
 /*   310 */    89,   76,   70,   92,   93,   94,   53,   54,   55,   56,
 /*   320 */    70,   89,   46,   47,   92,   93,   94,   92,   93,   94,
 /*   330 */     0,    1,    1,    1,   49,    4,    4,   70,   75,   76,
 /*   340 */    77,   78,   79,   80,   81,    1,   83,   84,   85,   86,
 /*   350 */    76,   88,   89,   70,   70,   92,   93,   94,   53,   54,
 /*   360 */    55,   56,    1,    1,   70,    4,   92,   93,   94,    1,
 /*   370 */     1,    4,    4,    4,   97,    1,    1,   27,   15,   28,
 /*   380 */    75,   76,   77,   78,   79,   80,   81,   31,   83,   84,
 /*   390 */    85,   86,   48,   88,   89,   37,    1,   92,   93,   94,
 /*   400 */    53,   54,   55,   56,   21,   21,    1,    4,    1,   12,
 /*   410 */     1,   15,   50,   12,   15,   21,   16,   15,   12,   15,
 /*   420 */    12,    1,   75,   76,   77,   78,   79,   80,   81,   12,
 /*   430 */    83,   84,   85,   86,   20,   88,   89,   12,   15,   92,
 /*   440 */    93,   94,   53,   54,   55,   56,   12,   15,   15,   37,
 /*   450 */    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
 /*   460 */    12,   37,    4,   12,   75,   76,   77,   78,   79,   80,
 /*   470 */    81,    1,   83,   84,   85,   86,   99,   88,   89,   99,
 /*   480 */    99,   92,   93,   94,   53,   54,   55,   56,   99,   99,
 /*   490 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*   500 */    99,   99,   99,   99,   99,   99,   75,   76,   77,   78,
 /*   510 */    79,   80,   81,   99,   83,   84,   85,   86,   99,   88,
 /*   520 */    89,   99,   99,   92,   93,   94,   53,   54,   55,   56,
 /*   530 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*   540 */    99,   99,   99,   99,   99,   99,   99,   99,   75,   76,
 /*   550 */    77,   78,   79,   80,   81,   99,   83,   84,   85,   86,
 /*   560 */    99,   88,   89,   99,   99,   92,   93,   94,   53,   54,
 /*   570 */    55,   56,   99,   99,   99,   99,   99,   99,   99,   99,
 /*   580 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*   590 */    75,   76,   77,   78,   79,   80,   81,   99,   83,   84,
 /*   600 */    85,   86,   99,   88,   89,   99,   99,   92,   93,   94,
 /*   610 */    53,   54,   55,   56,   99,   99,   99,   99,   99,   99,
 /*   620 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*   630 */    99,   99,   75,   76,   77,   78,   79,   80,   81,   99,
 /*   640 */    83,   84,   85,   86,   99,   88,   89,   99,   99,   92,
 /*   650 */    93,   94,   53,   54,   55,   56,   99,   99,   99,   99,
 /*   660 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*   670 */    99,   99,   99,   99,   75,   76,   77,   78,   79,   80,
 /*   680 */    81,   99,   83,   84,   85,   86,   99,   88,   89,   99,
 /*   690 */    99,   92,   93,   94,   16,   54,   55,   56,   20,   99,
 /*   700 */    99,   99,   99,   25,   99,   99,   99,   99,   99,   99,
 /*   710 */    99,   99,   99,   99,   36,   99,   75,   76,   77,   78,
 /*   720 */    79,   80,   81,   99,   83,   84,   85,   86,   99,   88,
 /*   730 */    89,   56,   99,   92,   93,   94,   99,   99,   99,   99,
 /*   740 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   74,
 /*   750 */    75,   76,   77,   78,   79,   80,   81,   99,   83,   84,
 /*   760 */    85,   86,   99,   88,   89,   99,   99,   92,   93,   94,
 /*   770 */    95,   99,   76,   56,   99,   99,   99,   81,   99,   83,
 /*   780 */    84,   85,   86,   99,   88,   89,   99,   99,   92,   93,
 /*   790 */    94,   74,   75,   76,   77,   78,   79,   80,   81,   99,
 /*   800 */    83,   84,   85,   86,   99,   88,   89,   56,   99,   92,
 /*   810 */    93,   94,   95,   99,   99,   99,   99,   99,   99,   99,
 /*   820 */    99,   99,   99,   99,   99,   99,   75,   76,   77,   78,
 /*   830 */    79,   80,   81,   99,   83,   84,   85,   86,   99,   88,
 /*   840 */    89,   99,   99,   92,   93,   94,   99,   99,   56,   99,
 /*   850 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*   860 */    99,   99,   99,   99,   99,   99,   99,   75,   76,   77,
 /*   870 */    78,   79,   80,   81,   99,   83,   84,   85,   86,   99,
 /*   880 */    88,   89,   99,   56,   92,   93,   94,   99,   99,   99,
 /*   890 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*   900 */    99,   99,   75,   76,   77,   78,   79,   80,   81,   99,
 /*   910 */    83,   84,   85,   86,   99,   88,   89,   99,   99,   92,
 /*   920 */    93,   94,   99,   99,   56,   99,   99,   99,   99,   99,
 /*   930 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*   940 */    99,   99,   99,   75,   76,   77,   78,   79,   80,   81,
 /*   950 */    99,   83,   84,   85,   86,   99,   88,   89,   99,   56,
 /*   960 */    92,   93,   94,   99,   99,   99,   99,   99,   99,   99,
 /*   970 */    99,   99,   99,   99,   99,   99,   99,   99,   75,   76,
 /*   980 */    77,   78,   79,   80,   81,   99,   83,   84,   85,   86,
 /*   990 */    99,   88,   89,   99,   99,   92,   93,   94,   99,   99,
 /*  1000 */    56,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*  1010 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   75,
 /*  1020 */    76,   77,   78,   79,   80,   81,   99,   83,   84,   85,
 /*  1030 */    86,   99,   88,   89,   99,   56,   92,   93,   94,   99,
 /*  1040 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*  1050 */    99,   99,   99,   99,   75,   76,   77,   78,   79,   80,
 /*  1060 */    81,   99,   83,   84,   85,   86,   99,   88,   89,   99,
 /*  1070 */    99,   92,   93,   94,   99,   99,   56,   99,   99,   99,
 /*  1080 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*  1090 */    99,   99,   99,   99,   99,   75,   76,   77,   78,   79,
 /*  1100 */    80,   81,   99,   83,   84,   85,   86,   99,   88,   89,
 /*  1110 */    99,   56,   92,   93,   94,   99,   99,   99,   99,   99,
 /*  1120 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*  1130 */    75,   76,   77,   78,   79,   80,   81,   99,   83,   84,
 /*  1140 */    85,   86,   99,   88,   89,   99,   99,   92,   93,   94,
 /*  1150 */    99,   99,   56,   99,   99,   99,   99,   99,   99,   99,
 /*  1160 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*  1170 */    99,   75,   76,   77,   78,   79,   80,   81,   99,   83,
 /*  1180 */    84,   85,   86,   99,   88,   89,   99,   56,   92,   93,
 /*  1190 */    94,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*  1200 */    99,   99,   99,   99,   99,   99,   75,   76,   77,   78,
 /*  1210 */    79,   80,   81,   99,   83,   84,   85,   86,   99,   88,
 /*  1220 */    89,   99,   99,   92,   93,   94,   99,   99,   56,   99,
 /*  1230 */    99,   99,   99,   99,   99,   99,   99,   99,   99,   99,
 /*  1240 */    99,   99,   99,   99,   99,   99,   99,   75,   76,   77,
 /*  1250 */    78,   79,   80,   81,   99,   83,   84,   85,   86,   99,
 /*  1260 */    88,   89,    1,   99,   92,   93,   94,   76,   77,   78,
 /*  1270 */    79,   80,   81,   12,   83,   84,   85,   86,   99,   88,
 /*  1280 */    89,   20,   99,   92,   93,   94,   99,   99,   99,   99,
 /*  1290 */    99,   99,   99,   32,   33,   99,   99,   36,   99,   38,
 /*  1300 */    39,   40,   41,   42,   43,   44,   45,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 146
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   39,   39, 1261,
 /*    20 */    39,   39,   39,   39,   39,   39,   39,   39,   39,   39,
 /*    30 */    39,   39,   39,   39,   39,   39,  101,  101,   39,   39,
 /*    40 */   142,   39,   39,   39,  182,  182,  162,  285,  206,  206,
 /*    50 */   -21,  -21,  -21,  -21,   -9,   -5,  134,  134,  139,    9,
 /*    60 */   251,  261,  251,  261,  276,  146,  191,  198,   -9,  248,
 /*    70 */   248,   -5,  248,  248,   -5,  248,  248,  248,  248,   -5,
 /*    80 */   146,  678,   40,  330,  331,  332,  361,  368,  344,  362,
 /*    90 */   369,  367,  374,  375,  350,  351,  356,  363,  351,  356,
 /*   100 */   358,  383,  384,  395,  405,  403,  407,  374,  397,  409,
 /*   110 */   396,  401,  399,  400,  406,  400,  408,  414,  394,  402,
 /*   120 */   404,  423,  432,  417,  425,  433,  434,  435,  436,  437,
 /*   130 */   438,  439,  440,  441,  442,  443,  444,  412,  420,  424,
 /*   140 */   448,  458,  451,  470,  374,  374,  374,
};
#define YY_REDUCE_USE_DFLT (-55)
#define YY_REDUCE_MAX 80
static const short yy_reduce_ofst[] = {
 /*     0 */   -31,   11,   53,   95,  137,  179,  221,  263,  305,  347,
 /*    10 */   389,  431,  473,  515,  557,  599,  641,  675,  717,  751,
 /*    20 */   792,  827,  868,  903,  944,  979, 1020, 1055, 1096, 1131,
 /*    30 */  1172, 1191,  696,   26,   67,  109,  171,  180,  190,  232,
 /*    40 */   -54,  193,  235,  274,  -41,    0,  -24,   64,   34,   57,
 /*    50 */   130,  158,  173,  222,  189,   55,   -6,   -6,   43,   35,
 /*    60 */    48,   68,   48,   68,   81,   89,  136,  200,  156,  219,
 /*    70 */   225,  231,  242,  250,  231,  267,  283,  284,  294,  231,
 /*    80 */   277,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   238,  238,  238,  238,  238,  238,  238,  238,  238,  238,
 /*    10 */   238,  238,  238,  238,  238,  238,  238,  362,  362,  376,
 /*    20 */   376,  245,  376,  376,  247,  249,  376,  376,  376,  376,
 /*    30 */   376,  376,  376,  376,  376,  376,  299,  299,  376,  376,
 /*    40 */   376,  376,  376,  376,  376,  376,  376,  374,  265,  265,
 /*    50 */   376,  376,  376,  376,  376,  303,  335,  334,  374,  319,
 /*    60 */   327,  333,  326,  332,  364,  367,  261,  376,  376,  376,
 /*    70 */   376,  376,  376,  376,  307,  376,  376,  376,  376,  306,
 /*    80 */   367,  346,  346,  376,  376,  376,  376,  376,  376,  376,
 /*    90 */   376,  376,  375,  376,  323,  325,  329,  363,  324,  328,
 /*   100 */   376,  376,  376,  376,  376,  376,  376,  266,  376,  376,
 /*   110 */   253,  376,  254,  256,  376,  255,  376,  376,  376,  283,
 /*   120 */   275,  271,  269,  376,  376,  273,  376,  279,  277,  281,
 /*   130 */   291,  287,  285,  289,  295,  293,  297,  376,  376,  376,
 /*   140 */   376,  376,  376,  376,  371,  372,  373,  237,  239,  240,
 /*   150 */   236,  241,  244,  246,  313,  314,  316,  317,  318,  320,
 /*   160 */   338,  343,  344,  345,  311,  312,  315,  330,  331,  336,
 /*   170 */   337,  340,  341,  342,  339,  347,  351,  352,  353,  354,
 /*   180 */   355,  356,  357,  358,  359,  360,  361,  348,  365,  248,
 /*   190 */   250,  251,  263,  264,  252,  260,  259,  258,  257,  267,
 /*   200 */   268,  300,  270,  301,  272,  274,  302,  304,  305,  309,
 /*   210 */   310,  276,  278,  280,  282,  308,  284,  286,  288,  290,
 /*   220 */   292,  294,  296,  298,  262,  368,  366,  349,  350,  321,
 /*   230 */   322,  242,  370,  243,  369,
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
  "AND",           "LSHIFT",        "RSHIFT",        "EQUAL_TILDA", 
  "PLUS",          "MINUS",         "DIV",           "DIV_DIV",     
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
 /*  83 */ "not_expr ::= comparison",
 /*  84 */ "comparison ::= xor_expr",
 /*  85 */ "comparison ::= xor_expr comp_op xor_expr",
 /*  86 */ "comp_op ::= LESS",
 /*  87 */ "comp_op ::= GREATER",
 /*  88 */ "xor_expr ::= or_expr",
 /*  89 */ "or_expr ::= and_expr",
 /*  90 */ "or_expr ::= or_expr BAR and_expr",
 /*  91 */ "and_expr ::= shift_expr",
 /*  92 */ "and_expr ::= and_expr AND shift_expr",
 /*  93 */ "shift_expr ::= match_expr",
 /*  94 */ "shift_expr ::= shift_expr shift_op match_expr",
 /*  95 */ "shift_op ::= LSHIFT",
 /*  96 */ "shift_op ::= RSHIFT",
 /*  97 */ "match_expr ::= arith_expr",
 /*  98 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /*  99 */ "arith_expr ::= term",
 /* 100 */ "arith_expr ::= arith_expr arith_op term",
 /* 101 */ "arith_op ::= PLUS",
 /* 102 */ "arith_op ::= MINUS",
 /* 103 */ "term ::= term term_op factor",
 /* 104 */ "term ::= factor",
 /* 105 */ "term_op ::= STAR",
 /* 106 */ "term_op ::= DIV",
 /* 107 */ "term_op ::= DIV_DIV",
 /* 108 */ "factor ::= PLUS factor",
 /* 109 */ "factor ::= MINUS factor",
 /* 110 */ "factor ::= power",
 /* 111 */ "power ::= postfix_expr",
 /* 112 */ "postfix_expr ::= atom",
 /* 113 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 114 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 115 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 116 */ "atom ::= NAME",
 /* 117 */ "atom ::= NUMBER",
 /* 118 */ "atom ::= REGEXP",
 /* 119 */ "atom ::= STRING",
 /* 120 */ "atom ::= SYMBOL",
 /* 121 */ "atom ::= NIL",
 /* 122 */ "atom ::= TRUE",
 /* 123 */ "atom ::= FALSE",
 /* 124 */ "atom ::= LINE",
 /* 125 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 126 */ "atom ::= LPAR expr RPAR",
 /* 127 */ "args_opt ::=",
 /* 128 */ "args_opt ::= args",
 /* 129 */ "blockarg_opt ::=",
 /* 130 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 131 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 132 */ "blockarg_params_opt ::=",
 /* 133 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 134 */ "excepts ::= except",
 /* 135 */ "excepts ::= excepts except",
 /* 136 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 137 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 138 */ "except ::= EXCEPT NEWLINE stmts",
 /* 139 */ "finally_opt ::=",
 /* 140 */ "finally_opt ::= FINALLY stmts",
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
  { 52, 1 },
  { 53, 1 },
  { 53, 3 },
  { 54, 0 },
  { 54, 1 },
  { 54, 1 },
  { 54, 7 },
  { 54, 5 },
  { 54, 5 },
  { 54, 5 },
  { 54, 1 },
  { 54, 2 },
  { 54, 1 },
  { 54, 2 },
  { 54, 1 },
  { 54, 2 },
  { 54, 6 },
  { 54, 6 },
  { 54, 2 },
  { 54, 2 },
  { 62, 1 },
  { 62, 3 },
  { 63, 1 },
  { 63, 3 },
  { 61, 1 },
  { 61, 3 },
  { 60, 0 },
  { 60, 2 },
  { 59, 1 },
  { 59, 5 },
  { 64, 0 },
  { 64, 2 },
  { 55, 7 },
  { 65, 9 },
  { 65, 7 },
  { 65, 7 },
  { 65, 5 },
  { 65, 7 },
  { 65, 5 },
  { 65, 5 },
  { 65, 3 },
  { 65, 7 },
  { 65, 5 },
  { 65, 5 },
  { 65, 3 },
  { 65, 5 },
  { 65, 3 },
  { 65, 3 },
  { 65, 1 },
  { 65, 7 },
  { 65, 5 },
  { 65, 5 },
  { 65, 3 },
  { 65, 5 },
  { 65, 3 },
  { 65, 3 },
  { 65, 1 },
  { 65, 5 },
  { 65, 3 },
  { 65, 3 },
  { 65, 1 },
  { 65, 3 },
  { 65, 1 },
  { 65, 1 },
  { 65, 0 },
  { 70, 2 },
  { 69, 2 },
  { 68, 3 },
  { 71, 0 },
  { 71, 1 },
  { 72, 2 },
  { 66, 1 },
  { 66, 3 },
  { 67, 1 },
  { 67, 3 },
  { 73, 2 },
  { 74, 1 },
  { 74, 3 },
  { 56, 1 },
  { 75, 3 },
  { 75, 1 },
  { 77, 1 },
  { 78, 1 },
  { 79, 1 },
  { 80, 1 },
  { 80, 3 },
  { 82, 1 },
  { 82, 1 },
  { 81, 1 },
  { 83, 1 },
  { 83, 3 },
  { 84, 1 },
  { 84, 3 },
  { 85, 1 },
  { 85, 3 },
  { 87, 1 },
  { 87, 1 },
  { 86, 1 },
  { 86, 3 },
  { 88, 1 },
  { 88, 3 },
  { 90, 1 },
  { 90, 1 },
  { 89, 3 },
  { 89, 1 },
  { 91, 1 },
  { 91, 1 },
  { 91, 1 },
  { 92, 2 },
  { 92, 2 },
  { 92, 1 },
  { 93, 1 },
  { 76, 1 },
  { 76, 5 },
  { 76, 4 },
  { 76, 3 },
  { 94, 1 },
  { 94, 1 },
  { 94, 1 },
  { 94, 1 },
  { 94, 1 },
  { 94, 1 },
  { 94, 1 },
  { 94, 1 },
  { 94, 1 },
  { 94, 3 },
  { 94, 3 },
  { 95, 0 },
  { 95, 1 },
  { 96, 0 },
  { 96, 5 },
  { 96, 5 },
  { 97, 0 },
  { 97, 3 },
  { 57, 1 },
  { 57, 2 },
  { 98, 6 },
  { 98, 4 },
  { 98, 3 },
  { 58, 0 },
  { 58, 2 },
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
    *pval = yymsp[0].minor.yy1;
}
#line 1897 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 134: /* excepts ::= except */
#line 626 "parser.y"
{
    yygotominor.yy1 = make_array_with(env, yymsp[0].minor.yy1);
}
#line 1908 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 629 "parser.y"
{
    yygotominor.yy1 = Array_push(env, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 1918 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 127: /* args_opt ::= */
      case 129: /* blockarg_opt ::= */
      case 132: /* blockarg_params_opt ::= */
      case 139: /* finally_opt ::= */
#line 633 "parser.y"
{
    yygotominor.yy1 = YNIL;
}
#line 1932 "parser.c"
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
      case 93: /* shift_expr ::= match_expr */
      case 97: /* match_expr ::= arith_expr */
      case 99: /* arith_expr ::= term */
      case 104: /* term ::= factor */
      case 110: /* factor ::= power */
      case 111: /* power ::= postfix_expr */
      case 112: /* postfix_expr ::= atom */
      case 128: /* args_opt ::= args */
      case 140: /* finally_opt ::= FINALLY stmts */
#line 636 "parser.y"
{
    yygotominor.yy1 = yymsp[0].minor.yy1;
}
#line 1963 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 642 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy1 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy1, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, yymsp[-1].minor.yy1);
}
#line 1971 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 646 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy1 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy1, yymsp[-2].minor.yy1, YNIL, yymsp[-1].minor.yy1);
}
#line 1979 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 650 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy1 = Finally_new(env, lineno, yymsp[-3].minor.yy1, yymsp[-1].minor.yy1);
}
#line 1987 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 654 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy1 = While_new(env, lineno, yymsp[-3].minor.yy1, yymsp[-1].minor.yy1);
}
#line 1995 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 658 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy1 = Break_new(env, lineno, YNIL);
}
#line 2003 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 662 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy1 = Break_new(env, lineno, yymsp[0].minor.yy1);
}
#line 2011 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 666 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy1 = Next_new(env, lineno, YNIL);
}
#line 2019 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 670 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy1 = Next_new(env, lineno, yymsp[0].minor.yy1);
}
#line 2027 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 674 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy1 = Return_new(env, lineno, YNIL);
}
#line 2035 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 678 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy1 = Return_new(env, lineno, yymsp[0].minor.yy1);
}
#line 2043 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 682 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy1 = If_new(env, lineno, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, yymsp[-1].minor.yy1);
}
#line 2051 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 686 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy1 = Klass_new(env, lineno, id, yymsp[-3].minor.yy1, yymsp[-1].minor.yy1);
}
#line 2060 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 691 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy1 = Nonlocal_new(env, lineno, yymsp[0].minor.yy1);
}
#line 2068 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 695 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy1 = Import_new(env, lineno, yymsp[0].minor.yy1);
}
#line 2076 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 707 "parser.y"
{
    yygotominor.yy1 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2084 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 710 "parser.y"
{
    yygotominor.yy1 = Array_push_token_id(env, yymsp[-2].minor.yy1, yymsp[0].minor.yy0);
}
#line 2092 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 731 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy1, yymsp[-1].minor.yy1, yymsp[0].minor.yy1);
    yygotominor.yy1 = make_array_with(env, node);
}
#line 2101 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 744 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy1 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy1, yymsp[-1].minor.yy1);
}
#line 2110 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 750 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-8].minor.yy1, yymsp[-6].minor.yy1, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 2117 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 753 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-6].minor.yy1, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, yymsp[0].minor.yy1, YNIL);
}
#line 2124 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 756 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-6].minor.yy1, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, YNIL, yymsp[0].minor.yy1);
}
#line 2131 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 759 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, yymsp[0].minor.yy1, YNIL, YNIL);
}
#line 2138 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 762 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-6].minor.yy1, yymsp[-4].minor.yy1, YNIL, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 2145 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 765 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, YNIL, yymsp[0].minor.yy1, YNIL);
}
#line 2152 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 768 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, YNIL, YNIL, yymsp[0].minor.yy1);
}
#line 2159 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 771 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-2].minor.yy1, yymsp[0].minor.yy1, YNIL, YNIL, YNIL);
}
#line 2166 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 774 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-6].minor.yy1, YNIL, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 2173 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 777 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-4].minor.yy1, YNIL, yymsp[-2].minor.yy1, yymsp[0].minor.yy1, YNIL);
}
#line 2180 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 780 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-4].minor.yy1, YNIL, yymsp[-2].minor.yy1, YNIL, yymsp[0].minor.yy1);
}
#line 2187 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 783 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-2].minor.yy1, YNIL, yymsp[0].minor.yy1, YNIL, YNIL);
}
#line 2194 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 786 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-4].minor.yy1, YNIL, YNIL, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 2201 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 789 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-2].minor.yy1, YNIL, YNIL, yymsp[0].minor.yy1, YNIL);
}
#line 2208 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 792 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[-2].minor.yy1, YNIL, YNIL, YNIL, yymsp[0].minor.yy1);
}
#line 2215 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 795 "parser.y"
{
    yygotominor.yy1 = Params_new(env, yymsp[0].minor.yy1, YNIL, YNIL, YNIL, YNIL);
}
#line 2222 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 798 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, yymsp[-6].minor.yy1, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 2229 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 801 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, yymsp[0].minor.yy1, YNIL);
}
#line 2236 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 804 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, YNIL, yymsp[0].minor.yy1);
}
#line 2243 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 807 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, yymsp[-2].minor.yy1, yymsp[0].minor.yy1, YNIL, YNIL);
}
#line 2250 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 810 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, yymsp[-4].minor.yy1, YNIL, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 2257 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 813 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, yymsp[-2].minor.yy1, YNIL, yymsp[0].minor.yy1, YNIL);
}
#line 2264 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 816 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, yymsp[-2].minor.yy1, YNIL, YNIL, yymsp[0].minor.yy1);
}
#line 2271 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 819 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, yymsp[0].minor.yy1, YNIL, YNIL, YNIL);
}
#line 2278 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 822 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 2285 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 825 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy1, yymsp[0].minor.yy1, YNIL);
}
#line 2292 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 828 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy1, YNIL, yymsp[0].minor.yy1);
}
#line 2299 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 831 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy1, YNIL, YNIL);
}
#line 2306 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 834 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 2313 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 837 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy1, YNIL);
}
#line 2320 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 840 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy1);
}
#line 2327 "parser.c"
        break;
      case 64: /* params ::= */
#line 843 "parser.y"
{
    yygotominor.yy1 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2334 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 847 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy1 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2343 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 853 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy1 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2352 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 859 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy1 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy1);
}
#line 2361 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 876 "parser.y"
{
    yygotominor.yy1 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy1, lineno, id, YNIL);
}
#line 2371 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 882 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy1, lineno, id, YNIL);
    yygotominor.yy1 = yymsp[-2].minor.yy1;
}
#line 2381 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 896 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy1 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy1);
}
#line 2390 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 913 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy1);
    yygotominor.yy1 = Assign_new(env, lineno, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 2398 "parser.c"
        break;
      case 85: /* comparison ::= xor_expr comp_op xor_expr */
#line 936 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy1);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy1)->u.id;
    yygotominor.yy1 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy1, id, yymsp[0].minor.yy1);
}
#line 2407 "parser.c"
        break;
      case 86: /* comp_op ::= LESS */
      case 87: /* comp_op ::= GREATER */
#line 942 "parser.y"
{
    yygotominor.yy1 = yymsp[0].minor.yy0;
}
#line 2415 "parser.c"
        break;
      case 90: /* or_expr ::= or_expr BAR and_expr */
      case 92: /* and_expr ::= and_expr AND shift_expr */
#line 956 "parser.y"
{
    yygotominor.yy1 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy1), yymsp[-2].minor.yy1, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy1);
}
#line 2423 "parser.c"
        break;
      case 94: /* shift_expr ::= shift_expr shift_op match_expr */
      case 100: /* arith_expr ::= arith_expr arith_op term */
      case 103: /* term ::= term term_op factor */
#line 970 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy1);
    yygotominor.yy1 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy1, VAL2ID(yymsp[-1].minor.yy1), yymsp[0].minor.yy1);
}
#line 2433 "parser.c"
        break;
      case 95: /* shift_op ::= LSHIFT */
      case 96: /* shift_op ::= RSHIFT */
      case 101: /* arith_op ::= PLUS */
      case 102: /* arith_op ::= MINUS */
      case 105: /* term_op ::= STAR */
      case 106: /* term_op ::= DIV */
      case 107: /* term_op ::= DIV_DIV */
#line 975 "parser.y"
{
    yygotominor.yy1 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2446 "parser.c"
        break;
      case 98: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 985 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy1);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy1 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy1, id, yymsp[0].minor.yy1);
}
#line 2455 "parser.c"
        break;
      case 108: /* factor ::= PLUS factor */
#line 1024 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy1 = FuncCall_new3(env, lineno, yymsp[0].minor.yy1, id);
}
#line 2464 "parser.c"
        break;
      case 109: /* factor ::= MINUS factor */
#line 1029 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy1 = FuncCall_new3(env, lineno, yymsp[0].minor.yy1, id);
}
#line 2473 "parser.c"
        break;
      case 113: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1045 "parser.y"
{
    yygotominor.yy1 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy1), yymsp[-4].minor.yy1, yymsp[-2].minor.yy1, yymsp[0].minor.yy1);
}
#line 2480 "parser.c"
        break;
      case 114: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1048 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy1);
    yygotominor.yy1 = Subscript_new(env, lineno, yymsp[-3].minor.yy1, yymsp[-1].minor.yy1);
}
#line 2488 "parser.c"
        break;
      case 115: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1052 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy1);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy1 = Attr_new(env, lineno, yymsp[-2].minor.yy1, id);
}
#line 2497 "parser.c"
        break;
      case 116: /* atom ::= NAME */
#line 1058 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy1 = Variable_new(env, lineno, id);
}
#line 2506 "parser.c"
        break;
      case 117: /* atom ::= NUMBER */
      case 118: /* atom ::= REGEXP */
      case 119: /* atom ::= STRING */
      case 120: /* atom ::= SYMBOL */
#line 1063 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy1 = Literal_new(env, lineno, val);
}
#line 2518 "parser.c"
        break;
      case 121: /* atom ::= NIL */
#line 1083 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy1 = Literal_new(env, lineno, YNIL);
}
#line 2526 "parser.c"
        break;
      case 122: /* atom ::= TRUE */
#line 1087 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy1 = Literal_new(env, lineno, YTRUE);
}
#line 2534 "parser.c"
        break;
      case 123: /* atom ::= FALSE */
#line 1091 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy1 = Literal_new(env, lineno, YFALSE);
}
#line 2542 "parser.c"
        break;
      case 124: /* atom ::= LINE */
#line 1095 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy1 = Literal_new(env, lineno, val);
}
#line 2551 "parser.c"
        break;
      case 125: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1100 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy1 = Array_new(env, lineno, yymsp[-1].minor.yy1);
}
#line 2559 "parser.c"
        break;
      case 126: /* atom ::= LPAR expr RPAR */
      case 133: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1104 "parser.y"
{
    yygotominor.yy1 = yymsp[-1].minor.yy1;
}
#line 2567 "parser.c"
        break;
      case 130: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 131: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1118 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy1 = BlockArg_new(env, lineno, yymsp[-3].minor.yy1, yymsp[-1].minor.yy1);
}
#line 2576 "parser.c"
        break;
      case 135: /* excepts ::= excepts except */
#line 1137 "parser.y"
{
    yygotominor.yy1 = Array_push(env, yymsp[-1].minor.yy1, yymsp[0].minor.yy1);
}
#line 2583 "parser.c"
        break;
      case 136: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1141 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy1 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy1, id, yymsp[0].minor.yy1);
}
#line 2593 "parser.c"
        break;
      case 137: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1147 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy1 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy1, NO_EXC_VAR, yymsp[0].minor.yy1);
}
#line 2601 "parser.c"
        break;
      case 138: /* except ::= EXCEPT NEWLINE stmts */
#line 1151 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy1 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy1);
}
#line 2609 "parser.c"
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
