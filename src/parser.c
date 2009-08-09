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
    case NODE_DICT:
        KEEP(dict.elems);
        break;
    case NODE_DICT_ELEM:
        KEEP(dict_elem.key);
        KEEP(dict_elem.value);
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

    if (!IS_PTR(elem) && !IS_SYMBOL(elem)) {
        RETURN(env, array);
    }

    if (!IS_PTR(array)) {
        array = YogArray_new(env);
    }
    YogArray_push(env, array, elem);

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

static YogVal
DictElem_new(YogEnv* env, uint_t lineno, YogVal key, YogVal value)
{
    SAVE_ARGS2(env, key, value);
    YogVal elem = YUNDEF;
    PUSH_LOCAL(env, elem);

    elem = YogNode_new(env, NODE_DICT_ELEM, lineno);
    PTR_AS(YogNode, elem)->u.dict_elem.key = key;
    PTR_AS(YogNode, elem)->u.dict_elem.value = value;

    RETURN(env, elem);
}

static YogVal
Dict_new(YogEnv* env, uint_t lineno, YogVal elems)
{
    SAVE_ARG(env, elems);
    YogVal dict = YUNDEF;
    PUSH_LOCAL(env, dict);

    dict = YogNode_new(env, NODE_DICT, lineno);
    PTR_AS(YogNode, dict)->u.dict.elems = elems;

    RETURN(env, dict);
}

#define TOKEN(token)            PTR_AS(YogToken, (token))
#define TOKEN_ID(token)         TOKEN((token))->u.id
#define TOKEN_LINENO(token)     TOKEN((token))->lineno
#define NODE_LINENO(node)       PTR_AS(YogNode, (node))->lineno
#line 673 "parser.c"
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
#define YYNOCODE 111
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy67;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 260
#define YYNRULE 155
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
 /*     0 */     1,  142,  143,   92,   22,   23,   28,   29,   30,  127,
 /*    10 */   195,   77,   63,  109,   56,   72,   66,  135,   26,  193,
 /*    20 */   181,  194,  140,  144,  230,  223,   38,  234,  416,   93,
 /*    30 */   169,  167,  168,   32,   50,   51,  150,  152,  244,   52,
 /*    40 */    19,  234,  196,  197,  198,  199,  200,  201,  202,  203,
 /*    50 */    17,  173,   91,  111,  112,  184,  175,   67,  259,  113,
 /*    60 */   114,   71,  115,   49,   72,   66,  160,   92,  193,  181,
 /*    70 */   194,   55,  169,  167,  168,  108,   71,  115,  255,   72,
 /*    80 */    66,  141,  227,  193,  181,  194,  139,  146,  148,  239,
 /*    90 */   254,   40,  240,  173,   91,  111,  112,  184,  175,   67,
 /*   100 */   124,  113,  114,   71,  115,  217,   72,   66,   39,  257,
 /*   110 */   193,  181,  194,   68,  169,  167,  168,  137,  138,  149,
 /*   120 */   153,  155,  248,  147,  237,  240,  156,  138,  149,  153,
 /*   130 */   155,  248,  151,  242,  240,  173,   91,  111,  112,  184,
 /*   140 */   175,   67,  218,  113,  114,   71,  115,  217,   72,   66,
 /*   150 */    92,  101,  193,  181,  194,  102,  169,  167,  168,   69,
 /*   160 */   115,   89,   72,   66,  154,  246,  193,  181,  194,   43,
 /*   170 */    16,  142,  143,  145,   15,  231,  232,  173,   91,  111,
 /*   180 */   112,  184,  175,   67,   47,  113,  114,   71,  115,   92,
 /*   190 */    72,   66,   92,  212,  193,  181,  194,   94,  169,  167,
 /*   200 */   168,   70,   66,   84,  119,  193,  181,  194,  177,  181,
 /*   210 */   194,  131,  134,  142,  143,  145,  185,  186,   20,  173,
 /*   220 */    91,  111,  112,  184,  175,   67,   21,  113,  114,   71,
 /*   230 */   115,   92,   72,   66,   92,   45,  193,  181,  194,   95,
 /*   240 */   169,  167,  168,   81,   65,  187,  188,  193,  181,  194,
 /*   250 */   178,  181,  194,  142,  143,  145,    2,   33,    3,  260,
 /*   260 */    16,  173,   91,  111,  112,  184,  175,   67,  122,  113,
 /*   270 */   114,   71,  115,   92,   72,   66,   92,   16,  193,  181,
 /*   280 */   194,   57,  169,  167,  168,   16,  128,    8,  129,  179,
 /*   290 */   181,  194,  180,  181,  194,   16,   90,  221,   16,    3,
 /*   300 */    75,  171,   31,  173,   91,  111,  112,  184,  175,   67,
 /*   310 */    21,  113,  114,   71,  115,  189,   72,   66,   13,  132,
 /*   320 */   193,  181,  194,   58,  169,  167,  168,  142,  225,  251,
 /*   330 */   190,  191,  192,   16,  159,  229,  213,  157,   18,  236,
 /*   340 */   235,  238,  241,   35,  243,  173,   91,  111,  112,  184,
 /*   350 */   175,   67,  245,  113,  114,   71,  115,  247,   72,   66,
 /*   360 */    34,   16,  193,  181,  194,  126,  169,  167,  168,   16,
 /*   370 */   170,   16,  219,  159,  224,  161,   16,   18,    4,  258,
 /*   380 */    36,   40,   37,   41,   46,   42,   24,  173,   91,  111,
 /*   390 */   112,  184,  175,   67,   25,  113,  114,   71,  115,   34,
 /*   400 */    72,   66,  204,  205,  193,  181,  194,   96,  169,  167,
 /*   410 */   168,  208,   73,    5,    6,   27,    7,   76,  216,    9,
 /*   420 */   220,  130,   78,  222,   44,  133,  250,   11,  136,  173,
 /*   430 */    91,  111,  112,  184,  175,   67,   48,  113,  114,   71,
 /*   440 */   115,   10,   72,   66,   53,  226,  193,  181,  194,   97,
 /*   450 */   169,  167,  168,   59,   79,  228,   80,   64,   12,   60,
 /*   460 */   252,   82,   83,   54,   61,   85,   86,   62,   87,   88,
 /*   470 */   253,  173,   91,  111,  112,  184,  175,   67,  256,  113,
 /*   480 */   114,   71,  115,  162,   72,   66,  417,  417,  193,  181,
 /*   490 */   194,   98,  169,  167,  168,  417,  417,  417,  417,  417,
 /*   500 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*   510 */   417,  417,  417,  173,   91,  111,  112,  184,  175,   67,
 /*   520 */   417,  113,  114,   71,  115,  417,   72,   66,  417,  417,
 /*   530 */   193,  181,  194,  163,  169,  167,  168,  417,  417,  417,
 /*   540 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*   550 */   417,  417,  417,  417,  417,  173,   91,  111,  112,  184,
 /*   560 */   175,   67,  417,  113,  114,   71,  115,  417,   72,   66,
 /*   570 */   417,  417,  193,  181,  194,  164,  169,  167,  168,  417,
 /*   580 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*   590 */   417,  417,  417,  417,  417,  417,  417,  173,   91,  111,
 /*   600 */   112,  184,  175,   67,  417,  113,  114,   71,  115,  417,
 /*   610 */    72,   66,  417,  417,  193,  181,  194,  165,  169,  167,
 /*   620 */   168,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*   630 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  173,
 /*   640 */    91,  111,  112,  184,  175,   67,  417,  113,  114,   71,
 /*   650 */   115,  417,   72,   66,  417,  417,  193,  181,  194,  100,
 /*   660 */   169,  167,  168,  417,  417,  417,  417,  417,  417,  417,
 /*   670 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*   680 */   417,  173,   91,  111,  112,  184,  175,   67,  417,  113,
 /*   690 */   114,   71,  115,  417,   72,   66,  417,  417,  193,  181,
 /*   700 */   194,   92,  166,  167,  168,  417,  106,  417,  113,  114,
 /*   710 */    71,  115,  417,   72,   66,  417,  417,  193,  181,  194,
 /*   720 */   417,  417,  417,  173,   91,  111,  112,  184,  175,   67,
 /*   730 */   417,  113,  114,   71,  115,  417,   72,   66,  417,  417,
 /*   740 */   193,  181,  194,  417,  417,   92,  120,  105,  184,  175,
 /*   750 */    67,  417,  113,  114,   71,  115,  417,   72,   66,  417,
 /*   760 */   417,  193,  181,  194,  417,  173,   91,  111,  112,  184,
 /*   770 */   175,   67,  417,  113,  114,   71,  115,  417,   72,   66,
 /*   780 */   417,  417,  193,  181,  194,   92,  417,   74,  182,  211,
 /*   790 */   417,  417,  107,  114,   71,  115,  417,   72,   66,  417,
 /*   800 */   417,  193,  181,  194,  417,  417,  110,  173,   91,  111,
 /*   810 */   112,  184,  175,   67,  417,  113,  114,   71,  115,  417,
 /*   820 */    72,   66,  417,  417,  193,  181,  194,  121,  417,  417,
 /*   830 */   182,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*   840 */   417,  417,  417,  417,  417,  417,  417,  417,  110,  173,
 /*   850 */    91,  111,  112,  184,  175,   67,  417,  113,  114,   71,
 /*   860 */   115,  417,   72,   66,  417,  417,  193,  181,  194,  116,
 /*   870 */   417,  120,  417,  417,  417,  417,  417,  417,  417,  417,
 /*   880 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*   890 */   173,   91,  111,  112,  184,  175,   67,  417,  113,  114,
 /*   900 */    71,  115,  417,   72,   66,  117,  417,  193,  181,  194,
 /*   910 */   417,  417,  417,   26,  209,   92,  104,  112,  184,  175,
 /*   920 */    67,   38,  113,  114,   71,  115,  417,   72,   66,   50,
 /*   930 */    51,  193,  181,  194,   52,   19,   14,  196,  197,  198,
 /*   940 */   199,  200,  201,  202,  203,   17,  207,  195,  417,  417,
 /*   950 */   417,  417,  417,  417,  417,   26,  417,  417,  417,  417,
 /*   960 */   417,  417,  417,   38,  417,  417,  417,  417,  417,   99,
 /*   970 */   417,   50,   51,  417,  417,  417,   52,   19,  417,  196,
 /*   980 */   197,  198,  199,  200,  201,  202,  203,   17,  173,   91,
 /*   990 */   111,  112,  184,  175,   67,  103,  113,  114,   71,  115,
 /*  1000 */   417,   72,   66,  417,  417,  193,  181,  194,  417,  417,
 /*  1010 */   417,  417,  417,  417,  173,   91,  111,  112,  184,  175,
 /*  1020 */    67,  417,  113,  114,   71,  115,  417,   72,   66,  417,
 /*  1030 */   417,  193,  181,  194,  172,  417,  417,  417,  417,  417,
 /*  1040 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*  1050 */   417,  417,  417,  173,   91,  111,  112,  184,  175,   67,
 /*  1060 */   183,  113,  114,   71,  115,  417,   72,   66,  417,  417,
 /*  1070 */   193,  181,  194,  417,  417,  417,  417,  417,  417,  173,
 /*  1080 */    91,  111,  112,  184,  175,   67,  417,  113,  114,   71,
 /*  1090 */   115,  417,   72,   66,  417,  417,  193,  181,  194,  206,
 /*  1100 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*  1110 */   417,  417,  417,  417,  417,  417,  417,  417,  173,   91,
 /*  1120 */   111,  112,  184,  175,   67,  118,  113,  114,   71,  115,
 /*  1130 */   417,   72,   66,  417,  417,  193,  181,  194,  417,  417,
 /*  1140 */   417,  417,  417,  417,  173,   91,  111,  112,  184,  175,
 /*  1150 */    67,  417,  113,  114,   71,  115,  417,   72,   66,  417,
 /*  1160 */   417,  193,  181,  194,  210,  417,  417,  417,  417,  417,
 /*  1170 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*  1180 */   417,  417,  417,  173,   91,  111,  112,  184,  175,   67,
 /*  1190 */   214,  113,  114,   71,  115,  417,   72,   66,  417,  417,
 /*  1200 */   193,  181,  194,  417,  417,  417,  417,  417,  417,  173,
 /*  1210 */    91,  111,  112,  184,  175,   67,  417,  113,  114,   71,
 /*  1220 */   115,  417,   72,   66,  417,  417,  193,  181,  194,  215,
 /*  1230 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*  1240 */   417,  417,  417,  417,  417,  417,  417,  417,  173,   91,
 /*  1250 */   111,  112,  184,  175,   67,  123,  113,  114,   71,  115,
 /*  1260 */   417,   72,   66,  417,  417,  193,  181,  194,  417,  417,
 /*  1270 */   417,  417,  417,  417,  173,   91,  111,  112,  184,  175,
 /*  1280 */    67,  417,  113,  114,   71,  115,  417,   72,   66,  417,
 /*  1290 */   417,  193,  181,  194,  125,  417,  417,  417,  417,  417,
 /*  1300 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*  1310 */   417,  417,  417,  173,   91,  111,  112,  184,  175,   67,
 /*  1320 */   233,  113,  114,   71,  115,  417,   72,   66,  417,  417,
 /*  1330 */   193,  181,  194,  417,  417,  417,  417,  417,  417,  173,
 /*  1340 */    91,  111,  112,  184,  175,   67,  417,  113,  114,   71,
 /*  1350 */   115,  417,   72,   66,  417,  417,  193,  181,  194,  249,
 /*  1360 */   417,  417,  417,  417,  417,  417,  417,  417,  417,  417,
 /*  1370 */   417,  417,  417,  417,  417,  417,  417,  417,  173,   91,
 /*  1380 */   111,  112,  184,  175,   67,  158,  113,  114,   71,  115,
 /*  1390 */   417,   72,   66,  417,  417,  193,  181,  194,  417,  417,
 /*  1400 */   417,  417,  417,  417,  173,   91,  111,  112,  184,  175,
 /*  1410 */    67,  417,  113,  114,   71,  115,  417,   72,   66,  195,
 /*  1420 */   417,  193,  181,  194,  417,  417,  417,   26,  417,   92,
 /*  1430 */   417,  417,  174,  175,   67,   38,  113,  114,   71,  115,
 /*  1440 */   417,   72,   66,   50,   51,  193,  181,  194,   52,   19,
 /*  1450 */   417,  196,  197,  198,  199,  200,  201,  202,  203,   17,
 /*  1460 */   417,  117,  417,   92,  417,  417,  176,  175,   67,   26,
 /*  1470 */   113,  114,   71,  115,  417,   72,   66,   38,  417,  193,
 /*  1480 */   181,  194,  417,  417,  417,   50,   51,  417,  417,  417,
 /*  1490 */    52,   19,  417,  196,  197,  198,  199,  200,  201,  202,
 /*  1500 */   203,   17,  417,  195,  417,  417,  417,  417,  417,  417,
 /*  1510 */   417,   26,  417,  417,  417,  417,  417,  417,  417,  417,
 /*  1520 */   417,  417,  417,  417,  417,  417,  417,   50,   51,  417,
 /*  1530 */   417,  417,   52,   19,  417,  196,  197,  198,  199,  200,
 /*  1540 */   201,  202,  203,   17,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   84,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   94,   65,   96,   97,   19,   20,  100,
 /*    20 */   101,  102,   76,   77,   78,   12,   28,   81,   60,   61,
 /*    30 */    62,   63,   64,   25,   36,   37,   76,   77,   78,   41,
 /*    40 */    42,   81,   44,   45,   46,   47,   48,   49,   50,   51,
 /*    50 */    52,   83,   84,   85,   86,   87,   88,   89,  109,   91,
 /*    60 */    92,   93,   94,   99,   96,   97,   66,   84,  100,  101,
 /*    70 */   102,   61,   62,   63,   64,   92,   93,   94,   17,   96,
 /*    80 */    97,   77,   78,  100,  101,  102,   75,   76,   77,   78,
 /*    90 */    29,   30,   81,   83,   84,   85,   86,   87,   88,   89,
 /*   100 */    67,   91,   92,   93,   94,   72,   96,   97,   90,  109,
 /*   110 */   100,  101,  102,   61,   62,   63,   64,   73,   74,   75,
 /*   120 */    76,   77,   78,   77,   78,   81,   73,   74,   75,   76,
 /*   130 */    77,   78,   77,   78,   81,   83,   84,   85,   86,   87,
 /*   140 */    88,   89,   67,   91,   92,   93,   94,   72,   96,   97,
 /*   150 */    84,   66,  100,  101,  102,   61,   62,   63,   64,   93,
 /*   160 */    94,   12,   96,   97,   77,   78,  100,  101,  102,   95,
 /*   170 */     1,   22,   23,   24,    5,   79,   80,   83,   84,   85,
 /*   180 */    86,   87,   88,   89,   98,   91,   92,   93,   94,   84,
 /*   190 */    96,   97,   84,  104,  100,  101,  102,   61,   62,   63,
 /*   200 */    64,   96,   97,   12,  106,  100,  101,  102,  100,  101,
 /*   210 */   102,   70,   71,   22,   23,   24,   33,   34,   15,   83,
 /*   220 */    84,   85,   86,   87,   88,   89,   57,   91,   92,   93,
 /*   230 */    94,   84,   96,   97,   84,   42,  100,  101,  102,   61,
 /*   240 */    62,   63,   64,   12,   97,   36,   37,  100,  101,  102,
 /*   250 */   100,  101,  102,   22,   23,   24,    3,   17,    5,    0,
 /*   260 */     1,   83,   84,   85,   86,   87,   88,   89,  108,   91,
 /*   270 */    92,   93,   94,   84,   96,   97,   84,    1,  100,  101,
 /*   280 */   102,   61,   62,   63,   64,    1,   68,    3,   69,  100,
 /*   290 */   101,  102,  100,  101,  102,    1,   52,   12,    1,    5,
 /*   300 */    56,    4,   18,   83,   84,   85,   86,   87,   88,   89,
 /*   310 */    57,   91,   92,   93,   94,   23,   96,   97,    1,   71,
 /*   320 */   100,  101,  102,   61,   62,   63,   64,   22,   78,   53,
 /*   330 */    38,   39,   40,    1,   16,   78,    4,  108,   20,   78,
 /*   340 */    80,   78,   78,   25,   78,   83,   84,   85,   86,   87,
 /*   350 */    88,   89,   78,   91,   92,   93,   94,   78,   96,   97,
 /*   360 */    42,    1,  100,  101,  102,   61,   62,   63,   64,    1,
 /*   370 */     4,    1,    4,   16,    4,   58,    1,   20,    1,    4,
 /*   380 */    26,   30,   27,   31,   35,   32,   15,   83,   84,   85,
 /*   390 */    86,   87,   88,   89,   55,   91,   92,   93,   94,   42,
 /*   400 */    96,   97,   43,   21,  100,  101,  102,   61,   62,   63,
 /*   410 */    64,   53,   21,    1,    1,   54,    1,   12,    4,    1,
 /*   420 */    12,   15,   15,   12,   20,   16,   43,    1,   12,   83,
 /*   430 */    84,   85,   86,   87,   88,   89,   15,   91,   92,   93,
 /*   440 */    94,   21,   96,   97,   15,   12,  100,  101,  102,   61,
 /*   450 */    62,   63,   64,   15,   15,   12,   15,   12,    1,   15,
 /*   460 */    43,   15,   15,   15,   15,   15,   15,   15,   15,   15,
 /*   470 */    12,   83,   84,   85,   86,   87,   88,   89,    4,   91,
 /*   480 */    92,   93,   94,   12,   96,   97,  110,  110,  100,  101,
 /*   490 */   102,   61,   62,   63,   64,  110,  110,  110,  110,  110,
 /*   500 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*   510 */   110,  110,  110,   83,   84,   85,   86,   87,   88,   89,
 /*   520 */   110,   91,   92,   93,   94,  110,   96,   97,  110,  110,
 /*   530 */   100,  101,  102,   61,   62,   63,   64,  110,  110,  110,
 /*   540 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*   550 */   110,  110,  110,  110,  110,   83,   84,   85,   86,   87,
 /*   560 */    88,   89,  110,   91,   92,   93,   94,  110,   96,   97,
 /*   570 */   110,  110,  100,  101,  102,   61,   62,   63,   64,  110,
 /*   580 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*   590 */   110,  110,  110,  110,  110,  110,  110,   83,   84,   85,
 /*   600 */    86,   87,   88,   89,  110,   91,   92,   93,   94,  110,
 /*   610 */    96,   97,  110,  110,  100,  101,  102,   61,   62,   63,
 /*   620 */    64,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*   630 */   110,  110,  110,  110,  110,  110,  110,  110,  110,   83,
 /*   640 */    84,   85,   86,   87,   88,   89,  110,   91,   92,   93,
 /*   650 */    94,  110,   96,   97,  110,  110,  100,  101,  102,   61,
 /*   660 */    62,   63,   64,  110,  110,  110,  110,  110,  110,  110,
 /*   670 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*   680 */   110,   83,   84,   85,   86,   87,   88,   89,  110,   91,
 /*   690 */    92,   93,   94,  110,   96,   97,  110,  110,  100,  101,
 /*   700 */   102,   84,   62,   63,   64,  110,   89,  110,   91,   92,
 /*   710 */    93,   94,  110,   96,   97,  110,  110,  100,  101,  102,
 /*   720 */   110,  110,  110,   83,   84,   85,   86,   87,   88,   89,
 /*   730 */   110,   91,   92,   93,   94,  110,   96,   97,  110,  110,
 /*   740 */   100,  101,  102,  110,  110,   84,   64,   86,   87,   88,
 /*   750 */    89,  110,   91,   92,   93,   94,  110,   96,   97,  110,
 /*   760 */   110,  100,  101,  102,  110,   83,   84,   85,   86,   87,
 /*   770 */    88,   89,  110,   91,   92,   93,   94,  110,   96,   97,
 /*   780 */   110,  110,  100,  101,  102,   84,  110,  105,   64,  107,
 /*   790 */   110,  110,   91,   92,   93,   94,  110,   96,   97,  110,
 /*   800 */   110,  100,  101,  102,  110,  110,   82,   83,   84,   85,
 /*   810 */    86,   87,   88,   89,  110,   91,   92,   93,   94,  110,
 /*   820 */    96,   97,  110,  110,  100,  101,  102,  103,  110,  110,
 /*   830 */    64,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*   840 */   110,  110,  110,  110,  110,  110,  110,  110,   82,   83,
 /*   850 */    84,   85,   86,   87,   88,   89,  110,   91,   92,   93,
 /*   860 */    94,  110,   96,   97,  110,  110,  100,  101,  102,  103,
 /*   870 */   110,   64,  110,  110,  110,  110,  110,  110,  110,  110,
 /*   880 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*   890 */    83,   84,   85,   86,   87,   88,   89,  110,   91,   92,
 /*   900 */    93,   94,  110,   96,   97,   12,  110,  100,  101,  102,
 /*   910 */   110,  110,  110,   20,  107,   84,   85,   86,   87,   88,
 /*   920 */    89,   28,   91,   92,   93,   94,  110,   96,   97,   36,
 /*   930 */    37,  100,  101,  102,   41,   42,    1,   44,   45,   46,
 /*   940 */    47,   48,   49,   50,   51,   52,   53,   12,  110,  110,
 /*   950 */   110,  110,  110,  110,  110,   20,  110,  110,  110,  110,
 /*   960 */   110,  110,  110,   28,  110,  110,  110,  110,  110,   64,
 /*   970 */   110,   36,   37,  110,  110,  110,   41,   42,  110,   44,
 /*   980 */    45,   46,   47,   48,   49,   50,   51,   52,   83,   84,
 /*   990 */    85,   86,   87,   88,   89,   64,   91,   92,   93,   94,
 /*  1000 */   110,   96,   97,  110,  110,  100,  101,  102,  110,  110,
 /*  1010 */   110,  110,  110,  110,   83,   84,   85,   86,   87,   88,
 /*  1020 */    89,  110,   91,   92,   93,   94,  110,   96,   97,  110,
 /*  1030 */   110,  100,  101,  102,   64,  110,  110,  110,  110,  110,
 /*  1040 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*  1050 */   110,  110,  110,   83,   84,   85,   86,   87,   88,   89,
 /*  1060 */    64,   91,   92,   93,   94,  110,   96,   97,  110,  110,
 /*  1070 */   100,  101,  102,  110,  110,  110,  110,  110,  110,   83,
 /*  1080 */    84,   85,   86,   87,   88,   89,  110,   91,   92,   93,
 /*  1090 */    94,  110,   96,   97,  110,  110,  100,  101,  102,   64,
 /*  1100 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*  1110 */   110,  110,  110,  110,  110,  110,  110,  110,   83,   84,
 /*  1120 */    85,   86,   87,   88,   89,   64,   91,   92,   93,   94,
 /*  1130 */   110,   96,   97,  110,  110,  100,  101,  102,  110,  110,
 /*  1140 */   110,  110,  110,  110,   83,   84,   85,   86,   87,   88,
 /*  1150 */    89,  110,   91,   92,   93,   94,  110,   96,   97,  110,
 /*  1160 */   110,  100,  101,  102,   64,  110,  110,  110,  110,  110,
 /*  1170 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*  1180 */   110,  110,  110,   83,   84,   85,   86,   87,   88,   89,
 /*  1190 */    64,   91,   92,   93,   94,  110,   96,   97,  110,  110,
 /*  1200 */   100,  101,  102,  110,  110,  110,  110,  110,  110,   83,
 /*  1210 */    84,   85,   86,   87,   88,   89,  110,   91,   92,   93,
 /*  1220 */    94,  110,   96,   97,  110,  110,  100,  101,  102,   64,
 /*  1230 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*  1240 */   110,  110,  110,  110,  110,  110,  110,  110,   83,   84,
 /*  1250 */    85,   86,   87,   88,   89,   64,   91,   92,   93,   94,
 /*  1260 */   110,   96,   97,  110,  110,  100,  101,  102,  110,  110,
 /*  1270 */   110,  110,  110,  110,   83,   84,   85,   86,   87,   88,
 /*  1280 */    89,  110,   91,   92,   93,   94,  110,   96,   97,  110,
 /*  1290 */   110,  100,  101,  102,   64,  110,  110,  110,  110,  110,
 /*  1300 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*  1310 */   110,  110,  110,   83,   84,   85,   86,   87,   88,   89,
 /*  1320 */    64,   91,   92,   93,   94,  110,   96,   97,  110,  110,
 /*  1330 */   100,  101,  102,  110,  110,  110,  110,  110,  110,   83,
 /*  1340 */    84,   85,   86,   87,   88,   89,  110,   91,   92,   93,
 /*  1350 */    94,  110,   96,   97,  110,  110,  100,  101,  102,   64,
 /*  1360 */   110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
 /*  1370 */   110,  110,  110,  110,  110,  110,  110,  110,   83,   84,
 /*  1380 */    85,   86,   87,   88,   89,   64,   91,   92,   93,   94,
 /*  1390 */   110,   96,   97,  110,  110,  100,  101,  102,  110,  110,
 /*  1400 */   110,  110,  110,  110,   83,   84,   85,   86,   87,   88,
 /*  1410 */    89,  110,   91,   92,   93,   94,  110,   96,   97,   12,
 /*  1420 */   110,  100,  101,  102,  110,  110,  110,   20,  110,   84,
 /*  1430 */   110,  110,   87,   88,   89,   28,   91,   92,   93,   94,
 /*  1440 */   110,   96,   97,   36,   37,  100,  101,  102,   41,   42,
 /*  1450 */   110,   44,   45,   46,   47,   48,   49,   50,   51,   52,
 /*  1460 */   110,   12,  110,   84,  110,  110,   87,   88,   89,   20,
 /*  1470 */    91,   92,   93,   94,  110,   96,   97,   28,  110,  100,
 /*  1480 */   101,  102,  110,  110,  110,   36,   37,  110,  110,  110,
 /*  1490 */    41,   42,  110,   44,   45,   46,   47,   48,   49,   50,
 /*  1500 */    51,   52,  110,   12,  110,  110,  110,  110,  110,  110,
 /*  1510 */   110,   20,  110,  110,  110,  110,  110,  110,  110,  110,
 /*  1520 */   110,  110,  110,  110,  110,  110,  110,   36,   37,  110,
 /*  1530 */   110,  110,   41,   42,  110,   44,   45,   46,   47,   48,
 /*  1540 */    49,   50,   51,   52,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 165
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,  893, 1407, 1407,
 /*    20 */  1449,  935, 1407, 1407, 1407, 1407, 1407, 1407, 1407, 1407,
 /*    30 */  1407, 1407, 1407, 1407, 1407, 1407, 1407, 1407, 1407, 1491,
 /*    40 */  1491, 1491, 1491, 1491,  149,  149, 1491, 1491,  191, 1491,
 /*    50 */  1491, 1491, 1491,  231,  231,  169,  253,  284,  284,  -21,
 /*    60 */   -21,  -21,  -21,   13,    8,  292,  292,   61,  294,  183,
 /*    70 */   209,  183,  209,  244,  203,  193,  240,  285,   13,  305,
 /*    80 */   305,    8,  305,  305,    8,  305,  305,  305,  305,    8,
 /*    90 */   193,  318,  357,  259,  297,  332,  368,  370,  276,  317,
 /*   100 */   375,  366,  360,  377,  354,  355,  351,  352,  353,  349,
 /*   110 */   371,  354,  355,  352,  353,  349,  359,  339,  382,  358,
 /*   120 */   361,  391,  412,  413,  414,  415,  360,  405,  418,  406,
 /*   130 */   408,  407,  409,  411,  409,  416,  404,  420,  421,  429,
 /*   140 */   438,  439,  433,  443,  441,  445,  444,  446,  447,  448,
 /*   150 */   449,  450,  451,  452,  453,  454,  383,  426,  417,  458,
 /*   160 */   474,  471,  457,  360,  360,  360,
};
#define YY_REDUCE_USE_DFLT (-82)
#define YY_REDUCE_MAX 90
static const short yy_reduce_ofst[] = {
 /*     0 */   -32,   10,   52,   94,  136,  178,  220,  262,  304,  346,
 /*    10 */   388,  430,  472,  514,  556,  598,  640,  682,  724,  766,
 /*    20 */   807,  905,  931,  970,  996, 1035, 1061, 1100, 1126, 1165,
 /*    30 */  1191, 1230, 1256, 1295, 1321,  831,  661, 1345, 1379,  617,
 /*    40 */   701,  -17,   66,  -81,   44,   53,  105,  147,   11,  108,
 /*    50 */   150,  189,  192,  -54,  -40,  -51,    0,   33,   75,    4,
 /*    60 */    46,   55,   87,  141,   96,  -36,  -36,   18,   85,   74,
 /*    70 */    86,   74,   86,   89,   98,  160,  218,  219,  248,  250,
 /*    80 */   257,  260,  261,  263,  260,  264,  266,  274,  279,  260,
 /*    90 */   229,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   263,  263,  263,  263,  263,  263,  263,  263,  263,  263,
 /*    10 */   263,  263,  263,  263,  263,  263,  263,  415,  399,  399,
 /*    20 */   402,  415,  415,  270,  415,  415,  415,  415,  272,  274,
 /*    30 */   415,  415,  415,  415,  415,  415,  415,  415,  415,  415,
 /*    40 */   415,  415,  415,  415,  324,  324,  415,  415,  415,  415,
 /*    50 */   415,  415,  415,  415,  415,  415,  413,  290,  290,  415,
 /*    60 */   415,  415,  415,  415,  328,  364,  363,  347,  413,  356,
 /*    70 */   362,  355,  361,  403,  401,  406,  286,  415,  415,  415,
 /*    80 */   415,  415,  415,  415,  332,  415,  415,  415,  415,  331,
 /*    90 */   406,  377,  377,  415,  415,  415,  415,  415,  415,  415,
 /*   100 */   415,  415,  414,  415,  339,  342,  348,  352,  354,  358,
 /*   110 */   400,  340,  341,  351,  353,  357,  415,  382,  415,  415,
 /*   120 */   415,  415,  415,  415,  415,  415,  291,  415,  415,  278,
 /*   130 */   415,  279,  281,  415,  280,  415,  415,  415,  308,  300,
 /*   140 */   296,  294,  415,  415,  298,  415,  304,  302,  306,  316,
 /*   150 */   312,  310,  314,  320,  318,  322,  415,  415,  415,  415,
 /*   160 */   415,  415,  415,  410,  411,  412,  262,  264,  265,  261,
 /*   170 */   266,  269,  271,  338,  344,  345,  346,  367,  373,  374,
 /*   180 */   375,  376,  336,  337,  343,  359,  360,  365,  366,  369,
 /*   190 */   370,  371,  372,  368,  378,  382,  383,  384,  385,  386,
 /*   200 */   387,  388,  389,  390,  391,  394,  398,  392,  393,  396,
 /*   210 */   397,  395,  379,  404,  273,  275,  276,  288,  289,  277,
 /*   220 */   285,  284,  283,  282,  292,  293,  325,  295,  326,  297,
 /*   230 */   299,  327,  329,  330,  334,  335,  301,  303,  305,  307,
 /*   240 */   333,  309,  311,  313,  315,  317,  319,  321,  323,  287,
 /*   250 */   407,  405,  380,  381,  349,  350,  267,  409,  268,  408,
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
  "LBRACE",        "RBRACE",        "EQUAL_GREATER",  "COLON",       
  "DO",            "EXCEPT",        "AS",            "error",       
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
  "blockarg_opt",  "dict_elems",    "comma_opt",     "dict_elem",   
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
 /* 132 */ "atom ::= LBRACE RBRACE",
 /* 133 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 134 */ "atom ::= LPAR expr RPAR",
 /* 135 */ "dict_elems ::= dict_elem",
 /* 136 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 137 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 138 */ "dict_elem ::= NAME COLON expr",
 /* 139 */ "args_opt ::=",
 /* 140 */ "args_opt ::= args",
 /* 141 */ "comma_opt ::=",
 /* 142 */ "comma_opt ::= COMMA",
 /* 143 */ "blockarg_opt ::=",
 /* 144 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 145 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 146 */ "blockarg_params_opt ::=",
 /* 147 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 148 */ "excepts ::= except",
 /* 149 */ "excepts ::= excepts except",
 /* 150 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 151 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 152 */ "except ::= EXCEPT NEWLINE stmts",
 /* 153 */ "finally_opt ::=",
 /* 154 */ "finally_opt ::= FINALLY stmts",
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
  { 60, 1 },
  { 61, 1 },
  { 61, 3 },
  { 62, 0 },
  { 62, 1 },
  { 62, 1 },
  { 62, 7 },
  { 62, 5 },
  { 62, 5 },
  { 62, 5 },
  { 62, 1 },
  { 62, 2 },
  { 62, 1 },
  { 62, 2 },
  { 62, 1 },
  { 62, 2 },
  { 62, 6 },
  { 62, 6 },
  { 62, 2 },
  { 62, 2 },
  { 70, 1 },
  { 70, 3 },
  { 71, 1 },
  { 71, 3 },
  { 69, 1 },
  { 69, 3 },
  { 68, 0 },
  { 68, 2 },
  { 67, 1 },
  { 67, 5 },
  { 72, 0 },
  { 72, 2 },
  { 63, 7 },
  { 73, 9 },
  { 73, 7 },
  { 73, 7 },
  { 73, 5 },
  { 73, 7 },
  { 73, 5 },
  { 73, 5 },
  { 73, 3 },
  { 73, 7 },
  { 73, 5 },
  { 73, 5 },
  { 73, 3 },
  { 73, 5 },
  { 73, 3 },
  { 73, 3 },
  { 73, 1 },
  { 73, 7 },
  { 73, 5 },
  { 73, 5 },
  { 73, 3 },
  { 73, 5 },
  { 73, 3 },
  { 73, 3 },
  { 73, 1 },
  { 73, 5 },
  { 73, 3 },
  { 73, 3 },
  { 73, 1 },
  { 73, 3 },
  { 73, 1 },
  { 73, 1 },
  { 73, 0 },
  { 78, 2 },
  { 77, 2 },
  { 76, 3 },
  { 79, 0 },
  { 79, 1 },
  { 80, 2 },
  { 74, 1 },
  { 74, 3 },
  { 75, 1 },
  { 75, 3 },
  { 81, 2 },
  { 82, 1 },
  { 82, 3 },
  { 64, 1 },
  { 83, 3 },
  { 83, 1 },
  { 85, 1 },
  { 85, 3 },
  { 86, 1 },
  { 86, 3 },
  { 87, 1 },
  { 87, 2 },
  { 88, 1 },
  { 88, 3 },
  { 90, 1 },
  { 90, 1 },
  { 89, 1 },
  { 89, 3 },
  { 91, 1 },
  { 91, 3 },
  { 92, 1 },
  { 92, 3 },
  { 93, 1 },
  { 93, 3 },
  { 95, 1 },
  { 95, 1 },
  { 94, 1 },
  { 94, 3 },
  { 96, 1 },
  { 96, 3 },
  { 98, 1 },
  { 98, 1 },
  { 97, 3 },
  { 97, 1 },
  { 99, 1 },
  { 99, 1 },
  { 99, 1 },
  { 99, 1 },
  { 100, 2 },
  { 100, 2 },
  { 100, 2 },
  { 100, 1 },
  { 101, 1 },
  { 84, 1 },
  { 84, 5 },
  { 84, 4 },
  { 84, 3 },
  { 102, 1 },
  { 102, 1 },
  { 102, 1 },
  { 102, 1 },
  { 102, 1 },
  { 102, 1 },
  { 102, 1 },
  { 102, 1 },
  { 102, 1 },
  { 102, 3 },
  { 102, 2 },
  { 102, 4 },
  { 102, 3 },
  { 105, 1 },
  { 105, 3 },
  { 107, 3 },
  { 107, 3 },
  { 103, 0 },
  { 103, 1 },
  { 106, 0 },
  { 106, 1 },
  { 104, 0 },
  { 104, 5 },
  { 104, 5 },
  { 108, 0 },
  { 108, 3 },
  { 65, 1 },
  { 65, 2 },
  { 109, 6 },
  { 109, 4 },
  { 109, 3 },
  { 66, 0 },
  { 66, 2 },
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
#line 669 "parser.y"
{
    *pval = yymsp[0].minor.yy67;
}
#line 2028 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 148: /* excepts ::= except */
#line 673 "parser.y"
{
    yygotominor.yy67 = make_array_with(env, yymsp[0].minor.yy67);
}
#line 2039 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 676 "parser.y"
{
    yygotominor.yy67 = Array_push(env, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2049 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 139: /* args_opt ::= */
      case 141: /* comma_opt ::= */
      case 143: /* blockarg_opt ::= */
      case 146: /* blockarg_params_opt ::= */
      case 153: /* finally_opt ::= */
#line 680 "parser.y"
{
    yygotominor.yy67 = YNIL;
}
#line 2064 "parser.c"
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
      case 140: /* args_opt ::= args */
      case 154: /* finally_opt ::= FINALLY stmts */
#line 683 "parser.y"
{
    yygotominor.yy67 = yymsp[0].minor.yy67;
}
#line 2095 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 689 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy67 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy67, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, yymsp[-1].minor.yy67);
}
#line 2103 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 693 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy67 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy67, yymsp[-2].minor.yy67, YNIL, yymsp[-1].minor.yy67);
}
#line 2111 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 697 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy67 = Finally_new(env, lineno, yymsp[-3].minor.yy67, yymsp[-1].minor.yy67);
}
#line 2119 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 701 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy67 = While_new(env, lineno, yymsp[-3].minor.yy67, yymsp[-1].minor.yy67);
}
#line 2127 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 705 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy67 = Break_new(env, lineno, YNIL);
}
#line 2135 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 709 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy67 = Break_new(env, lineno, yymsp[0].minor.yy67);
}
#line 2143 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 713 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy67 = Next_new(env, lineno, YNIL);
}
#line 2151 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 717 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy67 = Next_new(env, lineno, yymsp[0].minor.yy67);
}
#line 2159 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 721 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy67 = Return_new(env, lineno, YNIL);
}
#line 2167 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 725 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy67 = Return_new(env, lineno, yymsp[0].minor.yy67);
}
#line 2175 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 729 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy67 = If_new(env, lineno, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, yymsp[-1].minor.yy67);
}
#line 2183 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 733 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy67 = Klass_new(env, lineno, id, yymsp[-3].minor.yy67, yymsp[-1].minor.yy67);
}
#line 2192 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 738 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy67 = Nonlocal_new(env, lineno, yymsp[0].minor.yy67);
}
#line 2200 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 742 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy67 = Import_new(env, lineno, yymsp[0].minor.yy67);
}
#line 2208 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 754 "parser.y"
{
    yygotominor.yy67 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2216 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 757 "parser.y"
{
    yygotominor.yy67 = Array_push_token_id(env, yymsp[-2].minor.yy67, yymsp[0].minor.yy0);
}
#line 2224 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 778 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy67, yymsp[-1].minor.yy67, yymsp[0].minor.yy67);
    yygotominor.yy67 = make_array_with(env, node);
}
#line 2233 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 791 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy67 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy67, yymsp[-1].minor.yy67);
}
#line 2242 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 797 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-8].minor.yy67, yymsp[-6].minor.yy67, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2249 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 800 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-6].minor.yy67, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, yymsp[0].minor.yy67, YNIL);
}
#line 2256 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 803 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-6].minor.yy67, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, YNIL, yymsp[0].minor.yy67);
}
#line 2263 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 806 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, yymsp[0].minor.yy67, YNIL, YNIL);
}
#line 2270 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 809 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-6].minor.yy67, yymsp[-4].minor.yy67, YNIL, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2277 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 812 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, YNIL, yymsp[0].minor.yy67, YNIL);
}
#line 2284 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 815 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, YNIL, YNIL, yymsp[0].minor.yy67);
}
#line 2291 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 818 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-2].minor.yy67, yymsp[0].minor.yy67, YNIL, YNIL, YNIL);
}
#line 2298 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 821 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-6].minor.yy67, YNIL, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2305 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 824 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-4].minor.yy67, YNIL, yymsp[-2].minor.yy67, yymsp[0].minor.yy67, YNIL);
}
#line 2312 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 827 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-4].minor.yy67, YNIL, yymsp[-2].minor.yy67, YNIL, yymsp[0].minor.yy67);
}
#line 2319 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 830 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-2].minor.yy67, YNIL, yymsp[0].minor.yy67, YNIL, YNIL);
}
#line 2326 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 833 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-4].minor.yy67, YNIL, YNIL, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2333 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 836 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-2].minor.yy67, YNIL, YNIL, yymsp[0].minor.yy67, YNIL);
}
#line 2340 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 839 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[-2].minor.yy67, YNIL, YNIL, YNIL, yymsp[0].minor.yy67);
}
#line 2347 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 842 "parser.y"
{
    yygotominor.yy67 = Params_new(env, yymsp[0].minor.yy67, YNIL, YNIL, YNIL, YNIL);
}
#line 2354 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 845 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, yymsp[-6].minor.yy67, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2361 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 848 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, yymsp[0].minor.yy67, YNIL);
}
#line 2368 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 851 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, YNIL, yymsp[0].minor.yy67);
}
#line 2375 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 854 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, yymsp[-2].minor.yy67, yymsp[0].minor.yy67, YNIL, YNIL);
}
#line 2382 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 857 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, yymsp[-4].minor.yy67, YNIL, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2389 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 860 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, yymsp[-2].minor.yy67, YNIL, yymsp[0].minor.yy67, YNIL);
}
#line 2396 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 863 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, yymsp[-2].minor.yy67, YNIL, YNIL, yymsp[0].minor.yy67);
}
#line 2403 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 866 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, yymsp[0].minor.yy67, YNIL, YNIL, YNIL);
}
#line 2410 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 869 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2417 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 872 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy67, yymsp[0].minor.yy67, YNIL);
}
#line 2424 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 875 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy67, YNIL, yymsp[0].minor.yy67);
}
#line 2431 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 878 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy67, YNIL, YNIL);
}
#line 2438 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 881 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2445 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 884 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy67, YNIL);
}
#line 2452 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 887 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy67);
}
#line 2459 "parser.c"
        break;
      case 64: /* params ::= */
#line 890 "parser.y"
{
    yygotominor.yy67 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2466 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 894 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy67 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2475 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 900 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy67 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2484 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 906 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy67 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy67);
}
#line 2493 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 923 "parser.y"
{
    yygotominor.yy67 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy67, lineno, id, YNIL);
}
#line 2503 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 929 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy67, lineno, id, YNIL);
    yygotominor.yy67 = yymsp[-2].minor.yy67;
}
#line 2513 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 943 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy67 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy67);
}
#line 2522 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 960 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy67);
    yygotominor.yy67 = Assign_new(env, lineno, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2530 "parser.c"
        break;
      case 82: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 971 "parser.y"
{
    yygotominor.yy67 = YogNode_new(env, NODE_LOGICAL_OR, NODE_LINENO(yymsp[-2].minor.yy67));
    NODE(yygotominor.yy67)->u.logical_or.left = yymsp[-2].minor.yy67;
    NODE(yygotominor.yy67)->u.logical_or.right = yymsp[0].minor.yy67;
}
#line 2539 "parser.c"
        break;
      case 84: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 980 "parser.y"
{
    yygotominor.yy67 = YogNode_new(env, NODE_LOGICAL_AND, NODE_LINENO(yymsp[-2].minor.yy67));
    NODE(yygotominor.yy67)->u.logical_and.left = yymsp[-2].minor.yy67;
    NODE(yygotominor.yy67)->u.logical_and.right = yymsp[0].minor.yy67;
}
#line 2548 "parser.c"
        break;
      case 86: /* not_expr ::= NOT not_expr */
#line 989 "parser.y"
{
    yygotominor.yy67 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy67)->u.not.expr = yymsp[0].minor.yy67;
}
#line 2556 "parser.c"
        break;
      case 88: /* comparison ::= xor_expr comp_op xor_expr */
#line 997 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy67);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy67)->u.id;
    yygotominor.yy67 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy67, id, yymsp[0].minor.yy67);
}
#line 2565 "parser.c"
        break;
      case 89: /* comp_op ::= LESS */
      case 90: /* comp_op ::= GREATER */
      case 142: /* comma_opt ::= COMMA */
#line 1003 "parser.y"
{
    yygotominor.yy67 = yymsp[0].minor.yy0;
}
#line 2574 "parser.c"
        break;
      case 92: /* xor_expr ::= xor_expr XOR or_expr */
      case 94: /* or_expr ::= or_expr BAR and_expr */
      case 96: /* and_expr ::= and_expr AND shift_expr */
#line 1013 "parser.y"
{
    yygotominor.yy67 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy67), yymsp[-2].minor.yy67, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy67);
}
#line 2583 "parser.c"
        break;
      case 98: /* shift_expr ::= shift_expr shift_op match_expr */
      case 104: /* arith_expr ::= arith_expr arith_op term */
      case 107: /* term ::= term term_op factor */
#line 1034 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy67);
    yygotominor.yy67 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy67, VAL2ID(yymsp[-1].minor.yy67), yymsp[0].minor.yy67);
}
#line 2593 "parser.c"
        break;
      case 99: /* shift_op ::= LSHIFT */
      case 100: /* shift_op ::= RSHIFT */
      case 105: /* arith_op ::= PLUS */
      case 106: /* arith_op ::= MINUS */
      case 109: /* term_op ::= STAR */
      case 110: /* term_op ::= DIV */
      case 111: /* term_op ::= DIV_DIV */
      case 112: /* term_op ::= PERCENT */
#line 1039 "parser.y"
{
    yygotominor.yy67 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2607 "parser.c"
        break;
      case 102: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1049 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy67);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy67 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy67, id, yymsp[0].minor.yy67);
}
#line 2616 "parser.c"
        break;
      case 113: /* factor ::= PLUS factor */
#line 1091 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy67 = FuncCall_new3(env, lineno, yymsp[0].minor.yy67, id);
}
#line 2625 "parser.c"
        break;
      case 114: /* factor ::= MINUS factor */
#line 1096 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy67 = FuncCall_new3(env, lineno, yymsp[0].minor.yy67, id);
}
#line 2634 "parser.c"
        break;
      case 115: /* factor ::= TILDA factor */
#line 1101 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy67 = FuncCall_new3(env, lineno, yymsp[0].minor.yy67, id);
}
#line 2643 "parser.c"
        break;
      case 119: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1117 "parser.y"
{
    yygotominor.yy67 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy67), yymsp[-4].minor.yy67, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2650 "parser.c"
        break;
      case 120: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1120 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy67);
    yygotominor.yy67 = Subscript_new(env, lineno, yymsp[-3].minor.yy67, yymsp[-1].minor.yy67);
}
#line 2658 "parser.c"
        break;
      case 121: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1124 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy67);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy67 = Attr_new(env, lineno, yymsp[-2].minor.yy67, id);
}
#line 2667 "parser.c"
        break;
      case 122: /* atom ::= NAME */
#line 1130 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy67 = Variable_new(env, lineno, id);
}
#line 2676 "parser.c"
        break;
      case 123: /* atom ::= NUMBER */
      case 124: /* atom ::= REGEXP */
      case 125: /* atom ::= STRING */
      case 126: /* atom ::= SYMBOL */
#line 1135 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy67 = Literal_new(env, lineno, val);
}
#line 2688 "parser.c"
        break;
      case 127: /* atom ::= NIL */
#line 1155 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy67 = Literal_new(env, lineno, YNIL);
}
#line 2696 "parser.c"
        break;
      case 128: /* atom ::= TRUE */
#line 1159 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy67 = Literal_new(env, lineno, YTRUE);
}
#line 2704 "parser.c"
        break;
      case 129: /* atom ::= FALSE */
#line 1163 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy67 = Literal_new(env, lineno, YFALSE);
}
#line 2712 "parser.c"
        break;
      case 130: /* atom ::= LINE */
#line 1167 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy67 = Literal_new(env, lineno, val);
}
#line 2721 "parser.c"
        break;
      case 131: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1172 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy67 = Array_new(env, lineno, yymsp[-1].minor.yy67);
}
#line 2729 "parser.c"
        break;
      case 132: /* atom ::= LBRACE RBRACE */
#line 1176 "parser.y"
{
    yygotominor.yy67 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 2736 "parser.c"
        break;
      case 133: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1179 "parser.y"
{
    yygotominor.yy67 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy67);
}
#line 2743 "parser.c"
        break;
      case 134: /* atom ::= LPAR expr RPAR */
      case 147: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1182 "parser.y"
{
    yygotominor.yy67 = yymsp[-1].minor.yy67;
}
#line 2751 "parser.c"
        break;
      case 135: /* dict_elems ::= dict_elem */
#line 1186 "parser.y"
{
    yygotominor.yy67 = Array_push(env, yygotominor.yy67, yymsp[0].minor.yy67);
}
#line 2758 "parser.c"
        break;
      case 136: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 1189 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
    yygotominor.yy67 = yymsp[-2].minor.yy67;
}
#line 2766 "parser.c"
        break;
      case 137: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1193 "parser.y"
{
    yygotominor.yy67 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy67), yymsp[-2].minor.yy67, yymsp[0].minor.yy67);
}
#line 2773 "parser.c"
        break;
      case 138: /* dict_elem ::= NAME COLON expr */
#line 1196 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy67 = DictElem_new(env, lineno, var, yymsp[0].minor.yy67);
}
#line 2783 "parser.c"
        break;
      case 144: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 145: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1220 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy67 = BlockArg_new(env, lineno, yymsp[-3].minor.yy67, yymsp[-1].minor.yy67);
}
#line 2792 "parser.c"
        break;
      case 149: /* excepts ::= excepts except */
#line 1239 "parser.y"
{
    yygotominor.yy67 = Array_push(env, yymsp[-1].minor.yy67, yymsp[0].minor.yy67);
}
#line 2799 "parser.c"
        break;
      case 150: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1243 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy67 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy67, id, yymsp[0].minor.yy67);
}
#line 2809 "parser.c"
        break;
      case 151: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1249 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy67 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy67, NO_EXC_VAR, yymsp[0].minor.yy67);
}
#line 2817 "parser.c"
        break;
      case 152: /* except ::= EXCEPT NEWLINE stmts */
#line 1253 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy67 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy67);
}
#line 2825 "parser.c"
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
