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
    case NODE_LOGICAL_OR:
        KEEP(logical_or.left);
        KEEP(logical_or.right);
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
    case NODE_NOT:
        KEEP(not.expr);
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
#line 637 "parser.c"
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
#define YYNOCODE 106
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy37;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval
#define YYNSTATE 246
#define YYNRULE 147
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
 /*     0 */     1,  134,  135,   87,   20,   21,   24,   25,   26,  119,
 /*    10 */   187,   72,   59,  104,  209,   68,   62,  127,   23,  185,
 /*    20 */   173,  186,  152,   45,   52,   28,   34,  394,   88,  161,
 /*    30 */   159,  160,  133,  213,   46,   47,  132,  136,  216,   48,
 /*    40 */    18,  220,  188,  189,  190,  191,  192,  193,  194,  195,
 /*    50 */   165,   86,  106,  107,  176,  167,   63,   96,  108,  109,
 /*    60 */    67,  110,  243,   68,   62,  245,   87,  185,  173,  186,
 /*    70 */    51,  161,  159,  160,  103,   67,  110,   16,   68,   62,
 /*    80 */    35,   15,  185,  173,  186,  131,  138,  140,  225,  139,
 /*    90 */   223,  226,  165,   86,  106,  107,  176,  167,   63,   39,
 /*   100 */   108,  109,   67,  110,   43,   68,   62,  143,  228,  185,
 /*   110 */   173,  186,   64,  161,  159,  160,  129,  130,  141,  145,
 /*   120 */   147,  234,  146,  232,  226,  148,  130,  141,  145,  147,
 /*   130 */   234,   19,  198,  226,  165,   86,  106,  107,  176,  167,
 /*   140 */    63,   41,  108,  109,   67,  110,  151,   68,   62,   87,
 /*   150 */    17,  185,  173,  186,   97,  161,  159,  160,   65,  110,
 /*   160 */   114,   68,   62,  217,  218,  185,  173,  186,  142,  144,
 /*   170 */   230,   16,   30,  220,   29,    3,  165,   86,  106,  107,
 /*   180 */   176,  167,   63,  120,  108,  109,   67,  110,   87,   68,
 /*   190 */    62,  121,   87,  185,  173,  186,   89,  161,  159,  160,
 /*   200 */    66,   62,   84,  124,  185,  173,  186,  151,  169,  173,
 /*   210 */   186,   17,  134,  135,  137,    2,   31,    3,  165,   86,
 /*   220 */   106,  107,  176,  167,   63,  207,  108,  109,   67,  110,
 /*   230 */    87,   68,   62,   30,  181,  185,  173,  186,   90,  161,
 /*   240 */   159,  160,   79,   61,  211,   76,  185,  173,  186,  182,
 /*   250 */   183,  184,  134,  135,  137,  134,  135,  137,  179,  180,
 /*   260 */   165,   86,  106,  107,  176,  167,   63,   19,  108,  109,
 /*   270 */    67,  110,  134,   68,   62,   87,  215,  185,  173,  186,
 /*   280 */    53,  161,  159,  160,   16,  241,    8,  123,  126,  116,
 /*   290 */   204,  170,  173,  186,  203,  203,   13,  240,   36,  177,
 /*   300 */   178,   27,  165,   86,  106,  107,  176,  167,   63,  221,
 /*   310 */   108,  109,   67,  110,   87,   68,   62,   16,   87,  185,
 /*   320 */   173,  186,   54,  161,  159,  160,   70,   85,  246,   16,
 /*   330 */   171,  173,  186,  222,  172,  173,  186,   16,   16,   16,
 /*   340 */   163,  199,  205,  224,  165,   86,  106,  107,  176,  167,
 /*   350 */    63,  153,  108,  109,   67,  110,   16,   68,   62,  210,
 /*   360 */   227,  185,  173,  186,  118,  161,  159,  160,   16,  229,
 /*   370 */   237,  244,  231,  233,  149,  162,   16,    4,   32,   36,
 /*   380 */    33,   37,  196,   38,   22,  197,  165,   86,  106,  107,
 /*   390 */   176,  167,   63,   42,  108,  109,   67,  110,   69,   68,
 /*   400 */    62,    5,    6,  185,  173,  186,   91,  161,  159,  160,
 /*   410 */   202,    7,    9,  122,   71,  206,   40,  236,   10,   73,
 /*   420 */   208,  125,   44,   11,  128,  212,  214,   60,  165,   86,
 /*   430 */   106,  107,  176,  167,   63,   49,  108,  109,   67,  110,
 /*   440 */    55,   68,   62,   56,   74,  185,  173,  186,   92,  161,
 /*   450 */   159,  160,   75,   77,   78,   50,   57,   80,   81,   58,
 /*   460 */    82,   83,  238,  239,  242,  154,   12,  395,  395,  395,
 /*   470 */   165,   86,  106,  107,  176,  167,   63,  395,  108,  109,
 /*   480 */    67,  110,  395,   68,   62,  395,  395,  185,  173,  186,
 /*   490 */    93,  161,  159,  160,  395,  395,  395,  395,  395,  395,
 /*   500 */   395,  395,  395,  395,  395,  395,  395,  395,  395,  395,
 /*   510 */   395,  395,  165,   86,  106,  107,  176,  167,   63,  395,
 /*   520 */   108,  109,   67,  110,  395,   68,   62,  395,  395,  185,
 /*   530 */   173,  186,  155,  161,  159,  160,  395,  395,  395,  395,
 /*   540 */   395,  395,  395,  395,  395,  395,  395,  395,  395,  395,
 /*   550 */   395,  395,  395,  395,  165,   86,  106,  107,  176,  167,
 /*   560 */    63,  395,  108,  109,   67,  110,  395,   68,   62,  395,
 /*   570 */   395,  185,  173,  186,  156,  161,  159,  160,  395,  395,
 /*   580 */   395,  395,  395,  395,  395,  395,  395,  395,  395,  395,
 /*   590 */   395,  395,  395,  395,  395,  395,  165,   86,  106,  107,
 /*   600 */   176,  167,   63,  395,  108,  109,   67,  110,  395,   68,
 /*   610 */    62,  395,  395,  185,  173,  186,  157,  161,  159,  160,
 /*   620 */   395,  395,  395,  395,  395,  395,  395,  395,  395,  395,
 /*   630 */   395,  395,  395,  395,  395,  395,  395,  395,  165,   86,
 /*   640 */   106,  107,  176,  167,   63,  395,  108,  109,   67,  110,
 /*   650 */   395,   68,   62,  395,  395,  185,  173,  186,   95,  161,
 /*   660 */   159,  160,  395,  395,  395,  395,  395,  395,  395,  395,
 /*   670 */   395,  395,  395,  395,  395,  395,  395,  395,  395,  395,
 /*   680 */   165,   86,  106,  107,  176,  167,   63,  395,  108,  109,
 /*   690 */    67,  110,  395,   68,   62,  395,  395,  185,  173,  186,
 /*   700 */   158,  159,  160,  395,  395,  395,  395,  395,  395,  395,
 /*   710 */   395,  395,  187,  395,  395,  395,  395,  395,  395,  395,
 /*   720 */    23,  165,   86,  106,  107,  176,  167,   63,   34,  108,
 /*   730 */   109,   67,  110,  395,   68,   62,   46,   47,  185,  173,
 /*   740 */   186,   48,   18,  174,  188,  189,  190,  191,  192,  193,
 /*   750 */   194,  195,  395,  395,  395,  395,  395,  395,  395,  395,
 /*   760 */   395,  105,  165,   86,  106,  107,  176,  167,   63,  395,
 /*   770 */   108,  109,   67,  110,  395,   68,   62,  395,  395,  185,
 /*   780 */   173,  186,  113,   87,  174,  100,  176,  167,   63,  395,
 /*   790 */   108,  109,   67,  110,  395,   68,   62,  395,  395,  185,
 /*   800 */   173,  186,  105,  165,   86,  106,  107,  176,  167,   63,
 /*   810 */   395,  108,  109,   67,  110,  395,   68,   62,   87,  395,
 /*   820 */   185,  173,  186,  111,   94,  102,  109,   67,  110,  395,
 /*   830 */    68,   62,  395,  395,  185,  173,  186,  395,  395,  395,
 /*   840 */   395,  395,  395,  165,   86,  106,  107,  176,  167,   63,
 /*   850 */   395,  108,  109,   67,  110,  395,   68,   62,  395,  395,
 /*   860 */   185,  173,  186,  395,   14,   98,  395,  395,  395,  395,
 /*   870 */   395,  395,  395,  395,  395,  187,  395,  395,  395,  395,
 /*   880 */   395,  395,  395,   23,  165,   86,  106,  107,  176,  167,
 /*   890 */    63,   34,  108,  109,   67,  110,  395,   68,   62,   46,
 /*   900 */    47,  185,  173,  186,   48,   18,  164,  188,  189,  190,
 /*   910 */   191,  192,  193,  194,  195,  395,  187,  395,  395,  395,
 /*   920 */   395,  395,  395,  395,   23,  165,   86,  106,  107,  176,
 /*   930 */   167,   63,  395,  108,  109,   67,  110,  395,   68,   62,
 /*   940 */    46,   47,  185,  173,  186,   48,   18,  175,  188,  189,
 /*   950 */   190,  191,  192,  193,  194,  195,  395,  395,  395,  395,
 /*   960 */   395,  395,  395,  395,  395,  395,  165,   86,  106,  107,
 /*   970 */   176,  167,   63,  395,  108,  109,   67,  110,  395,   68,
 /*   980 */    62,  395,  395,  185,  173,  186,  395,   87,  112,  395,
 /*   990 */   166,  167,   63,  395,  108,  109,   67,  110,  395,   68,
 /*  1000 */    62,  395,  395,  185,  173,  186,  395,  165,   86,  106,
 /*  1010 */   107,  176,  167,   63,  395,  108,  109,   67,  110,  395,
 /*  1020 */    68,   62,  395,  395,  185,  173,  186,   87,  395,  200,
 /*  1030 */   168,  167,   63,  395,  108,  109,   67,  110,  395,   68,
 /*  1040 */    62,  395,  395,  185,  173,  186,  395,  395,  165,   86,
 /*  1050 */   106,  107,  176,  167,   63,  395,  108,  109,   67,  110,
 /*  1060 */   395,   68,   62,  395,  395,  185,  173,  186,   87,  395,
 /*  1070 */   201,  395,  395,  101,  395,  108,  109,   67,  110,  395,
 /*  1080 */    68,   62,  395,  395,  185,  173,  186,  395,  395,  165,
 /*  1090 */    86,  106,  107,  176,  167,   63,  395,  108,  109,   67,
 /*  1100 */   110,  395,   68,   62,  395,  395,  185,  173,  186,  395,
 /*  1110 */   395,  115,  395,  395,  395,  395,  395,  395,  395,  395,
 /*  1120 */   395,  395,  395,  395,  395,  395,  395,  395,  395,  395,
 /*  1130 */   165,   86,  106,  107,  176,  167,   63,  395,  108,  109,
 /*  1140 */    67,  110,  395,   68,   62,  395,  395,  185,  173,  186,
 /*  1150 */   395,  395,  117,  395,  395,  395,  395,  395,  395,  395,
 /*  1160 */   395,  395,  395,  395,  395,  395,  395,  395,  395,  395,
 /*  1170 */   395,  165,   86,  106,  107,  176,  167,   63,  395,  108,
 /*  1180 */   109,   67,  110,  395,   68,   62,  395,  395,  185,  173,
 /*  1190 */   186,  395,  395,  219,  395,  395,  395,  395,  395,  395,
 /*  1200 */   395,  395,  395,  395,  395,  395,  395,  395,  395,  395,
 /*  1210 */   395,  395,  165,   86,  106,  107,  176,  167,   63,  395,
 /*  1220 */   108,  109,   67,  110,  395,   68,   62,  395,  395,  185,
 /*  1230 */   173,  186,  395,  395,  235,  395,  395,  395,  395,  395,
 /*  1240 */   395,  395,  395,  395,  395,  395,  395,  395,  395,  395,
 /*  1250 */   395,  395,  395,  165,   86,  106,  107,  176,  167,   63,
 /*  1260 */   395,  108,  109,   67,  110,  395,   68,   62,  395,  395,
 /*  1270 */   185,  173,  186,  395,  395,  150,  395,  395,  395,  395,
 /*  1280 */   395,  395,  395,  395,  395,  395,  395,  395,  395,  395,
 /*  1290 */   395,  395,  395,  395,  165,   86,  106,  107,  176,  167,
 /*  1300 */    63,  395,  108,  109,   67,  110,  395,   68,   62,  395,
 /*  1310 */   395,  185,  173,  186,   87,   99,  107,  176,  167,   63,
 /*  1320 */   395,  108,  109,   67,  110,  395,   68,   62,  395,  395,
 /*  1330 */   185,  173,  186,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   82,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   92,   12,   94,   95,   19,   20,   98,
 /*    20 */    99,  100,   64,   97,   63,   25,   28,   58,   59,   60,
 /*    30 */    61,   62,   75,   76,   36,   37,   74,   75,   76,   41,
 /*    40 */    42,   79,   44,   45,   46,   47,   48,   49,   50,   51,
 /*    50 */    81,   82,   83,   84,   85,   86,   87,   64,   89,   90,
 /*    60 */    91,   92,  104,   94,   95,  104,   82,   98,   99,  100,
 /*    70 */    59,   60,   61,   62,   90,   91,   92,    1,   94,   95,
 /*    80 */    88,    5,   98,   99,  100,   73,   74,   75,   76,   75,
 /*    90 */    76,   79,   81,   82,   83,   84,   85,   86,   87,   93,
 /*   100 */    89,   90,   91,   92,   96,   94,   95,   75,   76,   98,
 /*   110 */    99,  100,   59,   60,   61,   62,   71,   72,   73,   74,
 /*   120 */    75,   76,   75,   76,   79,   71,   72,   73,   74,   75,
 /*   130 */    76,   55,  102,   79,   81,   82,   83,   84,   85,   86,
 /*   140 */    87,   42,   89,   90,   91,   92,   16,   94,   95,   82,
 /*   150 */    20,   98,   99,  100,   59,   60,   61,   62,   91,   92,
 /*   160 */   103,   94,   95,   77,   78,   98,   99,  100,   74,   75,
 /*   170 */    76,    1,   42,   79,   17,    5,   81,   82,   83,   84,
 /*   180 */    85,   86,   87,   66,   89,   90,   91,   92,   82,   94,
 /*   190 */    95,   67,   82,   98,   99,  100,   59,   60,   61,   62,
 /*   200 */    94,   95,   12,   69,   98,   99,  100,   16,   98,   99,
 /*   210 */   100,   20,   22,   23,   24,    3,   25,    5,   81,   82,
 /*   220 */    83,   84,   85,   86,   87,   12,   89,   90,   91,   92,
 /*   230 */    82,   94,   95,   42,   23,   98,   99,  100,   59,   60,
 /*   240 */    61,   62,   12,   95,   76,   12,   98,   99,  100,   38,
 /*   250 */    39,   40,   22,   23,   24,   22,   23,   24,   36,   37,
 /*   260 */    81,   82,   83,   84,   85,   86,   87,   55,   89,   90,
 /*   270 */    91,   92,   22,   94,   95,   82,   76,   98,   99,  100,
 /*   280 */    59,   60,   61,   62,    1,   17,    3,   68,   69,   65,
 /*   290 */    65,   98,   99,  100,   70,   70,    1,   29,   30,   33,
 /*   300 */    34,   18,   81,   82,   83,   84,   85,   86,   87,   78,
 /*   310 */    89,   90,   91,   92,   82,   94,   95,    1,   82,   98,
 /*   320 */    99,  100,   59,   60,   61,   62,   52,   53,    0,    1,
 /*   330 */    98,   99,  100,   76,   98,   99,  100,    1,    1,    1,
 /*   340 */     4,    4,    4,   76,   81,   82,   83,   84,   85,   86,
 /*   350 */    87,   56,   89,   90,   91,   92,    1,   94,   95,    4,
 /*   360 */    76,   98,   99,  100,   59,   60,   61,   62,    1,   76,
 /*   370 */    54,    4,   76,   76,  103,    4,    1,    1,   26,   30,
 /*   380 */    27,   31,   43,   32,   15,   21,   81,   82,   83,   84,
 /*   390 */    85,   86,   87,   35,   89,   90,   91,   92,   21,   94,
 /*   400 */    95,    1,    1,   98,   99,  100,   59,   60,   61,   62,
 /*   410 */     4,    1,    1,   15,   12,   12,   20,   43,   21,   15,
 /*   420 */    12,   16,   15,    1,   12,   12,   12,   12,   81,   82,
 /*   430 */    83,   84,   85,   86,   87,   15,   89,   90,   91,   92,
 /*   440 */    15,   94,   95,   15,   15,   98,   99,  100,   59,   60,
 /*   450 */    61,   62,   15,   15,   15,   15,   15,   15,   15,   15,
 /*   460 */    15,   15,   43,   12,    4,   12,    1,  105,  105,  105,
 /*   470 */    81,   82,   83,   84,   85,   86,   87,  105,   89,   90,
 /*   480 */    91,   92,  105,   94,   95,  105,  105,   98,   99,  100,
 /*   490 */    59,   60,   61,   62,  105,  105,  105,  105,  105,  105,
 /*   500 */   105,  105,  105,  105,  105,  105,  105,  105,  105,  105,
 /*   510 */   105,  105,   81,   82,   83,   84,   85,   86,   87,  105,
 /*   520 */    89,   90,   91,   92,  105,   94,   95,  105,  105,   98,
 /*   530 */    99,  100,   59,   60,   61,   62,  105,  105,  105,  105,
 /*   540 */   105,  105,  105,  105,  105,  105,  105,  105,  105,  105,
 /*   550 */   105,  105,  105,  105,   81,   82,   83,   84,   85,   86,
 /*   560 */    87,  105,   89,   90,   91,   92,  105,   94,   95,  105,
 /*   570 */   105,   98,   99,  100,   59,   60,   61,   62,  105,  105,
 /*   580 */   105,  105,  105,  105,  105,  105,  105,  105,  105,  105,
 /*   590 */   105,  105,  105,  105,  105,  105,   81,   82,   83,   84,
 /*   600 */    85,   86,   87,  105,   89,   90,   91,   92,  105,   94,
 /*   610 */    95,  105,  105,   98,   99,  100,   59,   60,   61,   62,
 /*   620 */   105,  105,  105,  105,  105,  105,  105,  105,  105,  105,
 /*   630 */   105,  105,  105,  105,  105,  105,  105,  105,   81,   82,
 /*   640 */    83,   84,   85,   86,   87,  105,   89,   90,   91,   92,
 /*   650 */   105,   94,   95,  105,  105,   98,   99,  100,   59,   60,
 /*   660 */    61,   62,  105,  105,  105,  105,  105,  105,  105,  105,
 /*   670 */   105,  105,  105,  105,  105,  105,  105,  105,  105,  105,
 /*   680 */    81,   82,   83,   84,   85,   86,   87,  105,   89,   90,
 /*   690 */    91,   92,  105,   94,   95,  105,  105,   98,   99,  100,
 /*   700 */    60,   61,   62,  105,  105,  105,  105,  105,  105,  105,
 /*   710 */   105,  105,   12,  105,  105,  105,  105,  105,  105,  105,
 /*   720 */    20,   81,   82,   83,   84,   85,   86,   87,   28,   89,
 /*   730 */    90,   91,   92,  105,   94,   95,   36,   37,   98,   99,
 /*   740 */   100,   41,   42,   62,   44,   45,   46,   47,   48,   49,
 /*   750 */    50,   51,  105,  105,  105,  105,  105,  105,  105,  105,
 /*   760 */   105,   80,   81,   82,   83,   84,   85,   86,   87,  105,
 /*   770 */    89,   90,   91,   92,  105,   94,   95,  105,  105,   98,
 /*   780 */    99,  100,  101,   82,   62,   84,   85,   86,   87,  105,
 /*   790 */    89,   90,   91,   92,  105,   94,   95,  105,  105,   98,
 /*   800 */    99,  100,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   810 */   105,   89,   90,   91,   92,  105,   94,   95,   82,  105,
 /*   820 */    98,   99,  100,  101,   62,   89,   90,   91,   92,  105,
 /*   830 */    94,   95,  105,  105,   98,   99,  100,  105,  105,  105,
 /*   840 */   105,  105,  105,   81,   82,   83,   84,   85,   86,   87,
 /*   850 */   105,   89,   90,   91,   92,  105,   94,   95,  105,  105,
 /*   860 */    98,   99,  100,  105,    1,   62,  105,  105,  105,  105,
 /*   870 */   105,  105,  105,  105,  105,   12,  105,  105,  105,  105,
 /*   880 */   105,  105,  105,   20,   81,   82,   83,   84,   85,   86,
 /*   890 */    87,   28,   89,   90,   91,   92,  105,   94,   95,   36,
 /*   900 */    37,   98,   99,  100,   41,   42,   62,   44,   45,   46,
 /*   910 */    47,   48,   49,   50,   51,  105,   12,  105,  105,  105,
 /*   920 */   105,  105,  105,  105,   20,   81,   82,   83,   84,   85,
 /*   930 */    86,   87,  105,   89,   90,   91,   92,  105,   94,   95,
 /*   940 */    36,   37,   98,   99,  100,   41,   42,   62,   44,   45,
 /*   950 */    46,   47,   48,   49,   50,   51,  105,  105,  105,  105,
 /*   960 */   105,  105,  105,  105,  105,  105,   81,   82,   83,   84,
 /*   970 */    85,   86,   87,  105,   89,   90,   91,   92,  105,   94,
 /*   980 */    95,  105,  105,   98,   99,  100,  105,   82,   62,  105,
 /*   990 */    85,   86,   87,  105,   89,   90,   91,   92,  105,   94,
 /*  1000 */    95,  105,  105,   98,   99,  100,  105,   81,   82,   83,
 /*  1010 */    84,   85,   86,   87,  105,   89,   90,   91,   92,  105,
 /*  1020 */    94,   95,  105,  105,   98,   99,  100,   82,  105,   62,
 /*  1030 */    85,   86,   87,  105,   89,   90,   91,   92,  105,   94,
 /*  1040 */    95,  105,  105,   98,   99,  100,  105,  105,   81,   82,
 /*  1050 */    83,   84,   85,   86,   87,  105,   89,   90,   91,   92,
 /*  1060 */   105,   94,   95,  105,  105,   98,   99,  100,   82,  105,
 /*  1070 */    62,  105,  105,   87,  105,   89,   90,   91,   92,  105,
 /*  1080 */    94,   95,  105,  105,   98,   99,  100,  105,  105,   81,
 /*  1090 */    82,   83,   84,   85,   86,   87,  105,   89,   90,   91,
 /*  1100 */    92,  105,   94,   95,  105,  105,   98,   99,  100,  105,
 /*  1110 */   105,   62,  105,  105,  105,  105,  105,  105,  105,  105,
 /*  1120 */   105,  105,  105,  105,  105,  105,  105,  105,  105,  105,
 /*  1130 */    81,   82,   83,   84,   85,   86,   87,  105,   89,   90,
 /*  1140 */    91,   92,  105,   94,   95,  105,  105,   98,   99,  100,
 /*  1150 */   105,  105,   62,  105,  105,  105,  105,  105,  105,  105,
 /*  1160 */   105,  105,  105,  105,  105,  105,  105,  105,  105,  105,
 /*  1170 */   105,   81,   82,   83,   84,   85,   86,   87,  105,   89,
 /*  1180 */    90,   91,   92,  105,   94,   95,  105,  105,   98,   99,
 /*  1190 */   100,  105,  105,   62,  105,  105,  105,  105,  105,  105,
 /*  1200 */   105,  105,  105,  105,  105,  105,  105,  105,  105,  105,
 /*  1210 */   105,  105,   81,   82,   83,   84,   85,   86,   87,  105,
 /*  1220 */    89,   90,   91,   92,  105,   94,   95,  105,  105,   98,
 /*  1230 */    99,  100,  105,  105,   62,  105,  105,  105,  105,  105,
 /*  1240 */   105,  105,  105,  105,  105,  105,  105,  105,  105,  105,
 /*  1250 */   105,  105,  105,   81,   82,   83,   84,   85,   86,   87,
 /*  1260 */   105,   89,   90,   91,   92,  105,   94,   95,  105,  105,
 /*  1270 */    98,   99,  100,  105,  105,   62,  105,  105,  105,  105,
 /*  1280 */   105,  105,  105,  105,  105,  105,  105,  105,  105,  105,
 /*  1290 */   105,  105,  105,  105,   81,   82,   83,   84,   85,   86,
 /*  1300 */    87,  105,   89,   90,   91,   92,  105,   94,   95,  105,
 /*  1310 */   105,   98,   99,  100,   82,   83,   84,   85,   86,   87,
 /*  1320 */   105,   89,   90,   91,   92,  105,   94,   95,  105,  105,
 /*  1330 */    98,   99,  100,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 157
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,  700,  700,  863,
 /*    20 */   700,  700,  700,  700,  700,  700,  700,  700,  700,  700,
 /*    30 */   700,  700,  700,  700,  700,  904,  904,  904,  904,  904,
 /*    40 */   190,  190,  904,  904,  230,  904,  904,  904,  904,  233,
 /*    50 */   233,   76,  212,  283,  283,  -21,  -21,  -21,  -21,    2,
 /*    60 */     0,  211,  211,  268,  170,  266,  222,  266,  222,  274,
 /*    70 */    99,  157,  213,    2,  250,  250,    0,  250,  250,    0,
 /*    80 */   250,  250,  250,  250,    0,   99,  191,  130,  328,  336,
 /*    90 */   337,  338,  355,  316,  295,  367,  371,  375,  376,  352,
 /*   100 */   353,  349,  350,  351,  358,  369,  352,  353,  350,  351,
 /*   110 */   358,  339,  364,  377,  400,  401,  406,  410,  375,  402,
 /*   120 */   411,  398,  403,  404,  405,  408,  405,  412,  396,  397,
 /*   130 */   407,  420,  425,  429,  413,  414,  437,  415,  428,  438,
 /*   140 */   439,  440,  441,  442,  443,  444,  445,  446,  374,  422,
 /*   150 */   419,  451,  460,  453,  465,  375,  375,  375,
};
#define YY_REDUCE_USE_DFLT (-80)
#define YY_REDUCE_MAX 85
static const short yy_reduce_ofst[] = {
 /*     0 */   -31,   11,   53,   95,  137,  179,  221,  263,  305,  347,
 /*    10 */   389,  431,  473,  515,  557,  599,  640,  681,  722,  762,
 /*    20 */   803,  844,  885,  926,  967, 1008, 1049, 1090, 1131, 1172,
 /*    30 */  1213, 1232,  701,  905,  945,  986,  736,  -16,   67,  -79,
 /*    40 */    45,   54,  106,  148,   12,  110,  193,  232,  236,  -38,
 /*    50 */    94,  -39,  -42,  224,  225,  -43,   14,   32,   47,  219,
 /*    60 */    86,  -74,  -74,   -8,   -7,    6,    8,    6,    8,   30,
 /*    70 */    57,  117,  124,  134,  168,  200,  231,  257,  267,  231,
 /*    80 */   284,  293,  296,  297,  231,  271,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   249,  249,  249,  249,  249,  249,  249,  249,  249,  249,
 /*    10 */   249,  249,  249,  249,  249,  249,  249,  379,  379,  393,
 /*    20 */   393,  256,  393,  393,  258,  260,  393,  393,  393,  393,
 /*    30 */   393,  393,  393,  393,  393,  393,  393,  393,  393,  393,
 /*    40 */   310,  310,  393,  393,  393,  393,  393,  393,  393,  393,
 /*    50 */   393,  393,  391,  276,  276,  393,  393,  393,  393,  393,
 /*    60 */   314,  350,  349,  333,  391,  342,  348,  341,  347,  381,
 /*    70 */   384,  272,  393,  393,  393,  393,  393,  393,  393,  318,
 /*    80 */   393,  393,  393,  393,  317,  384,  363,  363,  393,  393,
 /*    90 */   393,  393,  393,  393,  393,  393,  393,  392,  393,  325,
 /*   100 */   328,  334,  338,  340,  344,  380,  326,  327,  337,  339,
 /*   110 */   343,  393,  393,  393,  393,  393,  393,  393,  277,  393,
 /*   120 */   393,  264,  393,  265,  267,  393,  266,  393,  393,  393,
 /*   130 */   294,  286,  282,  280,  393,  393,  284,  393,  290,  288,
 /*   140 */   292,  302,  298,  296,  300,  306,  304,  308,  393,  393,
 /*   150 */   393,  393,  393,  393,  393,  388,  389,  390,  248,  250,
 /*   160 */   251,  247,  252,  255,  257,  324,  330,  331,  332,  353,
 /*   170 */   359,  360,  361,  362,  322,  323,  329,  345,  346,  351,
 /*   180 */   352,  355,  356,  357,  358,  354,  364,  368,  369,  370,
 /*   190 */   371,  372,  373,  374,  375,  376,  377,  378,  365,  382,
 /*   200 */   259,  261,  262,  274,  275,  263,  271,  270,  269,  268,
 /*   210 */   278,  279,  311,  281,  312,  283,  285,  313,  315,  316,
 /*   220 */   320,  321,  287,  289,  291,  293,  319,  295,  297,  299,
 /*   230 */   301,  303,  305,  307,  309,  273,  385,  383,  366,  367,
 /*   240 */   335,  336,  253,  387,  254,  386,
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
  "AMPER",         "EQUAL",         "BAR_BAR",       "AND_AND",
  "NOT",           "LESS",          "XOR",           "BAR",
  "AND",           "LSHIFT",        "RSHIFT",        "EQUAL_TILDA",
  "PLUS",          "MINUS",         "DIV",           "DIV_DIV",
  "PERCENT",       "TILDA",         "LBRACKET",      "RBRACKET",
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
 /*  82 */ "logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr",
 /*  83 */ "logical_and_expr ::= not_expr",
 /*  84 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /*  85 */ "not_expr ::= comparison",
 /*  86 */ "not_expr ::= NOT not_expr",
 /*  87 */ "comparison ::= xor_expr",
 /*  88 */ "comparison ::= xor_expr comp_op xor_expr",
 /*  89 */ "comp_op ::= LESS",
 /*  90 */ "comp_op ::= GREATER",
 /*  91 */ "xor_expr ::= or_expr",
 /*  92 */ "xor_expr ::= xor_expr XOR or_expr",
 /*  93 */ "or_expr ::= and_expr",
 /*  94 */ "or_expr ::= or_expr BAR and_expr",
 /*  95 */ "and_expr ::= shift_expr",
 /*  96 */ "and_expr ::= and_expr AND shift_expr",
 /*  97 */ "shift_expr ::= match_expr",
 /*  98 */ "shift_expr ::= shift_expr shift_op match_expr",
 /*  99 */ "shift_op ::= LSHIFT",
 /* 100 */ "shift_op ::= RSHIFT",
 /* 101 */ "match_expr ::= arith_expr",
 /* 102 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 103 */ "arith_expr ::= term",
 /* 104 */ "arith_expr ::= arith_expr arith_op term",
 /* 105 */ "arith_op ::= PLUS",
 /* 106 */ "arith_op ::= MINUS",
 /* 107 */ "term ::= term term_op factor",
 /* 108 */ "term ::= factor",
 /* 109 */ "term_op ::= STAR",
 /* 110 */ "term_op ::= DIV",
 /* 111 */ "term_op ::= DIV_DIV",
 /* 112 */ "term_op ::= PERCENT",
 /* 113 */ "factor ::= PLUS factor",
 /* 114 */ "factor ::= MINUS factor",
 /* 115 */ "factor ::= TILDA factor",
 /* 116 */ "factor ::= power",
 /* 117 */ "power ::= postfix_expr",
 /* 118 */ "postfix_expr ::= atom",
 /* 119 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 120 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 121 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 122 */ "atom ::= NAME",
 /* 123 */ "atom ::= NUMBER",
 /* 124 */ "atom ::= REGEXP",
 /* 125 */ "atom ::= STRING",
 /* 126 */ "atom ::= SYMBOL",
 /* 127 */ "atom ::= NIL",
 /* 128 */ "atom ::= TRUE",
 /* 129 */ "atom ::= FALSE",
 /* 130 */ "atom ::= LINE",
 /* 131 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 132 */ "atom ::= LPAR expr RPAR",
 /* 133 */ "args_opt ::=",
 /* 134 */ "args_opt ::= args",
 /* 135 */ "blockarg_opt ::=",
 /* 136 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 137 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 138 */ "blockarg_params_opt ::=",
 /* 139 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 140 */ "excepts ::= except",
 /* 141 */ "excepts ::= excepts except",
 /* 142 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 143 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 144 */ "except ::= EXCEPT NEWLINE stmts",
 /* 145 */ "finally_opt ::=",
 /* 146 */ "finally_opt ::= FINALLY stmts",
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
  { 58, 1 },
  { 59, 1 },
  { 59, 3 },
  { 60, 0 },
  { 60, 1 },
  { 60, 1 },
  { 60, 7 },
  { 60, 5 },
  { 60, 5 },
  { 60, 5 },
  { 60, 1 },
  { 60, 2 },
  { 60, 1 },
  { 60, 2 },
  { 60, 1 },
  { 60, 2 },
  { 60, 6 },
  { 60, 6 },
  { 60, 2 },
  { 60, 2 },
  { 68, 1 },
  { 68, 3 },
  { 69, 1 },
  { 69, 3 },
  { 67, 1 },
  { 67, 3 },
  { 66, 0 },
  { 66, 2 },
  { 65, 1 },
  { 65, 5 },
  { 70, 0 },
  { 70, 2 },
  { 61, 7 },
  { 71, 9 },
  { 71, 7 },
  { 71, 7 },
  { 71, 5 },
  { 71, 7 },
  { 71, 5 },
  { 71, 5 },
  { 71, 3 },
  { 71, 7 },
  { 71, 5 },
  { 71, 5 },
  { 71, 3 },
  { 71, 5 },
  { 71, 3 },
  { 71, 3 },
  { 71, 1 },
  { 71, 7 },
  { 71, 5 },
  { 71, 5 },
  { 71, 3 },
  { 71, 5 },
  { 71, 3 },
  { 71, 3 },
  { 71, 1 },
  { 71, 5 },
  { 71, 3 },
  { 71, 3 },
  { 71, 1 },
  { 71, 3 },
  { 71, 1 },
  { 71, 1 },
  { 71, 0 },
  { 76, 2 },
  { 75, 2 },
  { 74, 3 },
  { 77, 0 },
  { 77, 1 },
  { 78, 2 },
  { 72, 1 },
  { 72, 3 },
  { 73, 1 },
  { 73, 3 },
  { 79, 2 },
  { 80, 1 },
  { 80, 3 },
  { 62, 1 },
  { 81, 3 },
  { 81, 1 },
  { 83, 1 },
  { 83, 3 },
  { 84, 1 },
  { 84, 3 },
  { 85, 1 },
  { 85, 2 },
  { 86, 1 },
  { 86, 3 },
  { 88, 1 },
  { 88, 1 },
  { 87, 1 },
  { 87, 3 },
  { 89, 1 },
  { 89, 3 },
  { 90, 1 },
  { 90, 3 },
  { 91, 1 },
  { 91, 3 },
  { 93, 1 },
  { 93, 1 },
  { 92, 1 },
  { 92, 3 },
  { 94, 1 },
  { 94, 3 },
  { 96, 1 },
  { 96, 1 },
  { 95, 3 },
  { 95, 1 },
  { 97, 1 },
  { 97, 1 },
  { 97, 1 },
  { 97, 1 },
  { 98, 2 },
  { 98, 2 },
  { 98, 2 },
  { 98, 1 },
  { 99, 1 },
  { 82, 1 },
  { 82, 5 },
  { 82, 4 },
  { 82, 3 },
  { 100, 1 },
  { 100, 1 },
  { 100, 1 },
  { 100, 1 },
  { 100, 1 },
  { 100, 1 },
  { 100, 1 },
  { 100, 1 },
  { 100, 1 },
  { 100, 3 },
  { 100, 3 },
  { 101, 0 },
  { 101, 1 },
  { 102, 0 },
  { 102, 5 },
  { 102, 5 },
  { 103, 0 },
  { 103, 3 },
  { 63, 1 },
  { 63, 2 },
  { 104, 6 },
  { 104, 4 },
  { 104, 3 },
  { 64, 0 },
  { 64, 2 },
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
#line 633 "parser.y"
{
    *pval = yymsp[0].minor.yy37;
}
#line 1930 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 140: /* excepts ::= except */
#line 637 "parser.y"
{
    yygotominor.yy37 = make_array_with(env, yymsp[0].minor.yy37);
}
#line 1941 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 640 "parser.y"
{
    yygotominor.yy37 = Array_push(env, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 1951 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 133: /* args_opt ::= */
      case 135: /* blockarg_opt ::= */
      case 138: /* blockarg_params_opt ::= */
      case 145: /* finally_opt ::= */
#line 644 "parser.y"
{
    yygotominor.yy37 = YNIL;
}
#line 1965 "parser.c"
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
      case 83: /* logical_and_expr ::= not_expr */
      case 85: /* not_expr ::= comparison */
      case 87: /* comparison ::= xor_expr */
      case 91: /* xor_expr ::= or_expr */
      case 93: /* or_expr ::= and_expr */
      case 95: /* and_expr ::= shift_expr */
      case 97: /* shift_expr ::= match_expr */
      case 101: /* match_expr ::= arith_expr */
      case 103: /* arith_expr ::= term */
      case 108: /* term ::= factor */
      case 116: /* factor ::= power */
      case 117: /* power ::= postfix_expr */
      case 118: /* postfix_expr ::= atom */
      case 134: /* args_opt ::= args */
      case 146: /* finally_opt ::= FINALLY stmts */
#line 647 "parser.y"
{
    yygotominor.yy37 = yymsp[0].minor.yy37;
}
#line 1996 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 653 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy37 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy37, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, yymsp[-1].minor.yy37);
}
#line 2004 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 657 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy37 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy37, yymsp[-2].minor.yy37, YNIL, yymsp[-1].minor.yy37);
}
#line 2012 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 661 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy37 = Finally_new(env, lineno, yymsp[-3].minor.yy37, yymsp[-1].minor.yy37);
}
#line 2020 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 665 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy37 = While_new(env, lineno, yymsp[-3].minor.yy37, yymsp[-1].minor.yy37);
}
#line 2028 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 669 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy37 = Break_new(env, lineno, YNIL);
}
#line 2036 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 673 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy37 = Break_new(env, lineno, yymsp[0].minor.yy37);
}
#line 2044 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 677 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy37 = Next_new(env, lineno, YNIL);
}
#line 2052 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 681 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy37 = Next_new(env, lineno, yymsp[0].minor.yy37);
}
#line 2060 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 685 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy37 = Return_new(env, lineno, YNIL);
}
#line 2068 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 689 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy37 = Return_new(env, lineno, yymsp[0].minor.yy37);
}
#line 2076 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 693 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy37 = If_new(env, lineno, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, yymsp[-1].minor.yy37);
}
#line 2084 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 697 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy37 = Klass_new(env, lineno, id, yymsp[-3].minor.yy37, yymsp[-1].minor.yy37);
}
#line 2093 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 702 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy37 = Nonlocal_new(env, lineno, yymsp[0].minor.yy37);
}
#line 2101 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 706 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy37 = Import_new(env, lineno, yymsp[0].minor.yy37);
}
#line 2109 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 718 "parser.y"
{
    yygotominor.yy37 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2117 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 721 "parser.y"
{
    yygotominor.yy37 = Array_push_token_id(env, yymsp[-2].minor.yy37, yymsp[0].minor.yy0);
}
#line 2125 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 742 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy37, yymsp[-1].minor.yy37, yymsp[0].minor.yy37);
    yygotominor.yy37 = make_array_with(env, node);
}
#line 2134 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 755 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy37 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy37, yymsp[-1].minor.yy37);
}
#line 2143 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 761 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-8].minor.yy37, yymsp[-6].minor.yy37, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 2150 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 764 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-6].minor.yy37, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, yymsp[0].minor.yy37, YNIL);
}
#line 2157 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 767 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-6].minor.yy37, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, YNIL, yymsp[0].minor.yy37);
}
#line 2164 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 770 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, yymsp[0].minor.yy37, YNIL, YNIL);
}
#line 2171 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 773 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-6].minor.yy37, yymsp[-4].minor.yy37, YNIL, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 2178 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 776 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, YNIL, yymsp[0].minor.yy37, YNIL);
}
#line 2185 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 779 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, YNIL, YNIL, yymsp[0].minor.yy37);
}
#line 2192 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 782 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-2].minor.yy37, yymsp[0].minor.yy37, YNIL, YNIL, YNIL);
}
#line 2199 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 785 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-6].minor.yy37, YNIL, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 2206 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 788 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-4].minor.yy37, YNIL, yymsp[-2].minor.yy37, yymsp[0].minor.yy37, YNIL);
}
#line 2213 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 791 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-4].minor.yy37, YNIL, yymsp[-2].minor.yy37, YNIL, yymsp[0].minor.yy37);
}
#line 2220 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 794 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-2].minor.yy37, YNIL, yymsp[0].minor.yy37, YNIL, YNIL);
}
#line 2227 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 797 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-4].minor.yy37, YNIL, YNIL, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 2234 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 800 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-2].minor.yy37, YNIL, YNIL, yymsp[0].minor.yy37, YNIL);
}
#line 2241 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 803 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[-2].minor.yy37, YNIL, YNIL, YNIL, yymsp[0].minor.yy37);
}
#line 2248 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 806 "parser.y"
{
    yygotominor.yy37 = Params_new(env, yymsp[0].minor.yy37, YNIL, YNIL, YNIL, YNIL);
}
#line 2255 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 809 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, yymsp[-6].minor.yy37, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 2262 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 812 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, yymsp[0].minor.yy37, YNIL);
}
#line 2269 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 815 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, YNIL, yymsp[0].minor.yy37);
}
#line 2276 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 818 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, yymsp[-2].minor.yy37, yymsp[0].minor.yy37, YNIL, YNIL);
}
#line 2283 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 821 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, yymsp[-4].minor.yy37, YNIL, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 2290 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 824 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, yymsp[-2].minor.yy37, YNIL, yymsp[0].minor.yy37, YNIL);
}
#line 2297 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 827 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, yymsp[-2].minor.yy37, YNIL, YNIL, yymsp[0].minor.yy37);
}
#line 2304 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 830 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, yymsp[0].minor.yy37, YNIL, YNIL, YNIL);
}
#line 2311 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 833 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 2318 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 836 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy37, yymsp[0].minor.yy37, YNIL);
}
#line 2325 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 839 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy37, YNIL, yymsp[0].minor.yy37);
}
#line 2332 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 842 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy37, YNIL, YNIL);
}
#line 2339 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 845 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 2346 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 848 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy37, YNIL);
}
#line 2353 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 851 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy37);
}
#line 2360 "parser.c"
        break;
      case 64: /* params ::= */
#line 854 "parser.y"
{
    yygotominor.yy37 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2367 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 858 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy37 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2376 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 864 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy37 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2385 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 870 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy37 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy37);
}
#line 2394 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 887 "parser.y"
{
    yygotominor.yy37 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy37, lineno, id, YNIL);
}
#line 2404 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 893 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy37, lineno, id, YNIL);
    yygotominor.yy37 = yymsp[-2].minor.yy37;
}
#line 2414 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 907 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy37 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy37);
}
#line 2423 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 924 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy37);
    yygotominor.yy37 = Assign_new(env, lineno, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 2431 "parser.c"
        break;
      case 82: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 935 "parser.y"
{
    yygotominor.yy37 = YogNode_new(env, NODE_LOGICAL_OR, NODE_LINENO(yymsp[-2].minor.yy37));
    NODE(yygotominor.yy37)->u.logical_or.left = yymsp[-2].minor.yy37;
    NODE(yygotominor.yy37)->u.logical_or.right = yymsp[0].minor.yy37;
}
#line 2440 "parser.c"
        break;
      case 84: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 944 "parser.y"
{
    yygotominor.yy37 = YogNode_new(env, NODE_LOGICAL_AND, NODE_LINENO(yymsp[-2].minor.yy37));
    NODE(yygotominor.yy37)->u.logical_and.left = yymsp[-2].minor.yy37;
    NODE(yygotominor.yy37)->u.logical_and.right = yymsp[0].minor.yy37;
}
#line 2449 "parser.c"
        break;
      case 86: /* not_expr ::= NOT not_expr */
#line 953 "parser.y"
{
    yygotominor.yy37 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy37)->u.not.expr = yymsp[0].minor.yy37;
}
#line 2457 "parser.c"
        break;
      case 88: /* comparison ::= xor_expr comp_op xor_expr */
#line 961 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy37);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy37)->u.id;
    yygotominor.yy37 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy37, id, yymsp[0].minor.yy37);
}
#line 2466 "parser.c"
        break;
      case 89: /* comp_op ::= LESS */
      case 90: /* comp_op ::= GREATER */
#line 967 "parser.y"
{
    yygotominor.yy37 = yymsp[0].minor.yy0;
}
#line 2474 "parser.c"
        break;
      case 92: /* xor_expr ::= xor_expr XOR or_expr */
      case 94: /* or_expr ::= or_expr BAR and_expr */
      case 96: /* and_expr ::= and_expr AND shift_expr */
#line 977 "parser.y"
{
    yygotominor.yy37 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy37), yymsp[-2].minor.yy37, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy37);
}
#line 2483 "parser.c"
        break;
      case 98: /* shift_expr ::= shift_expr shift_op match_expr */
      case 104: /* arith_expr ::= arith_expr arith_op term */
      case 107: /* term ::= term term_op factor */
#line 998 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy37);
    yygotominor.yy37 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy37, VAL2ID(yymsp[-1].minor.yy37), yymsp[0].minor.yy37);
}
#line 2493 "parser.c"
        break;
      case 99: /* shift_op ::= LSHIFT */
      case 100: /* shift_op ::= RSHIFT */
      case 105: /* arith_op ::= PLUS */
      case 106: /* arith_op ::= MINUS */
      case 109: /* term_op ::= STAR */
      case 110: /* term_op ::= DIV */
      case 111: /* term_op ::= DIV_DIV */
      case 112: /* term_op ::= PERCENT */
#line 1003 "parser.y"
{
    yygotominor.yy37 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2507 "parser.c"
        break;
      case 102: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1013 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy37);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy37 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy37, id, yymsp[0].minor.yy37);
}
#line 2516 "parser.c"
        break;
      case 113: /* factor ::= PLUS factor */
#line 1055 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy37 = FuncCall_new3(env, lineno, yymsp[0].minor.yy37, id);
}
#line 2525 "parser.c"
        break;
      case 114: /* factor ::= MINUS factor */
#line 1060 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy37 = FuncCall_new3(env, lineno, yymsp[0].minor.yy37, id);
}
#line 2534 "parser.c"
        break;
      case 115: /* factor ::= TILDA factor */
#line 1065 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy37 = FuncCall_new3(env, lineno, yymsp[0].minor.yy37, id);
}
#line 2543 "parser.c"
        break;
      case 119: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1081 "parser.y"
{
    yygotominor.yy37 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy37), yymsp[-4].minor.yy37, yymsp[-2].minor.yy37, yymsp[0].minor.yy37);
}
#line 2550 "parser.c"
        break;
      case 120: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1084 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy37);
    yygotominor.yy37 = Subscript_new(env, lineno, yymsp[-3].minor.yy37, yymsp[-1].minor.yy37);
}
#line 2558 "parser.c"
        break;
      case 121: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1088 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy37);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy37 = Attr_new(env, lineno, yymsp[-2].minor.yy37, id);
}
#line 2567 "parser.c"
        break;
      case 122: /* atom ::= NAME */
#line 1094 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy37 = Variable_new(env, lineno, id);
}
#line 2576 "parser.c"
        break;
      case 123: /* atom ::= NUMBER */
      case 124: /* atom ::= REGEXP */
      case 125: /* atom ::= STRING */
      case 126: /* atom ::= SYMBOL */
#line 1099 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy37 = Literal_new(env, lineno, val);
}
#line 2588 "parser.c"
        break;
      case 127: /* atom ::= NIL */
#line 1119 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy37 = Literal_new(env, lineno, YNIL);
}
#line 2596 "parser.c"
        break;
      case 128: /* atom ::= TRUE */
#line 1123 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy37 = Literal_new(env, lineno, YTRUE);
}
#line 2604 "parser.c"
        break;
      case 129: /* atom ::= FALSE */
#line 1127 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy37 = Literal_new(env, lineno, YFALSE);
}
#line 2612 "parser.c"
        break;
      case 130: /* atom ::= LINE */
#line 1131 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy37 = Literal_new(env, lineno, val);
}
#line 2621 "parser.c"
        break;
      case 131: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1136 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy37 = Array_new(env, lineno, yymsp[-1].minor.yy37);
}
#line 2629 "parser.c"
        break;
      case 132: /* atom ::= LPAR expr RPAR */
      case 139: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1140 "parser.y"
{
    yygotominor.yy37 = yymsp[-1].minor.yy37;
}
#line 2637 "parser.c"
        break;
      case 136: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 137: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1154 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy37 = BlockArg_new(env, lineno, yymsp[-3].minor.yy37, yymsp[-1].minor.yy37);
}
#line 2646 "parser.c"
        break;
      case 141: /* excepts ::= excepts except */
#line 1173 "parser.y"
{
    yygotominor.yy37 = Array_push(env, yymsp[-1].minor.yy37, yymsp[0].minor.yy37);
}
#line 2653 "parser.c"
        break;
      case 142: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1177 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy37 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy37, id, yymsp[0].minor.yy37);
}
#line 2663 "parser.c"
        break;
      case 143: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1183 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy37 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy37, NO_EXC_VAR, yymsp[0].minor.yy37);
}
#line 2671 "parser.c"
        break;
      case 144: /* except ::= EXCEPT NEWLINE stmts */
#line 1187 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy37 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy37);
}
#line 2679 "parser.c"
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
