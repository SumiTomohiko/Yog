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
    case NODE_ARGS:
        KEEP(args.posargs);
        KEEP(args.kwargs);
        KEEP(args.vararg);
        break;
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
    case NODE_KW_ARG:
        KEEP(kwarg.value);
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
make_array_with(YogEnv* env, YogVal elem)
{
    return Array_push(env, YNIL, elem);
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
Args_new(YogEnv* env, uint_t lineno, YogVal posargs, YogVal kwargs, YogVal vararg)
{
    SAVE_ARGS3(env, posargs, kwargs, vararg);
    YogVal args = YUNDEF;
    PUSH_LOCAL(env, args);

    args = YogNode_new(env, NODE_ARGS, lineno);
    NODE(args)->u.args.posargs = posargs;
    NODE(args)->u.args.kwargs = kwargs;
    NODE(args)->u.args.vararg = vararg;

    RETURN(env, args);
}

static YogVal
FuncCall_new2(YogEnv* env, uint_t lineno, YogVal recv, ID name, YogVal arg)
{
    SAVE_ARGS2(env, recv, arg);
    YogVal postfix = YUNDEF;
    YogVal posargs = YUNDEF;
    YogVal args = YUNDEF;
    PUSH_LOCALS2(env, postfix, args);

    postfix = Attr_new(env, lineno, recv, name);

    posargs = YogArray_new(env);
    YogArray_push(env, posargs, arg);

    args = Args_new(env, lineno, posargs, YNIL, YNIL);

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
#line 686 "parser.c"
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
#define YYNOCODE 115
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy115;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 280
#define YYNRULE 168
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
 /*     0 */     2,  191,  224,  243,   23,   24,   31,   32,   33,  136,
 /*    10 */   204,   82,   68,   97,  149,  153,  250,  144,   28,  254,
 /*    20 */   113,  121,   76,  122,   59,   77,   71,   41,   35,  202,
 /*    30 */   190,  203,  229,  224,  231,   53,   54,  159,  161,  264,
 /*    40 */    55,   20,  254,  205,  206,  207,  208,  209,  210,  211,
 /*    50 */   212,   19,  130,  116,  129,  230,  228,  182,   96,  118,
 /*    60 */   119,  193,  184,   72,  133,  120,  121,   76,  122,  237,
 /*    70 */    77,   71,  279,  169,  202,  190,  203,   52,  449,   98,
 /*    80 */   178,  176,  177,   97,  110,  119,  193,  184,   72,  275,
 /*    90 */   120,  121,   76,  122,   18,   77,   71,  107,   16,  202,
 /*   100 */   190,  203,  274,   43,  204,  182,   96,  118,  119,  193,
 /*   110 */   184,   72,   28,  120,  121,   76,  122,   42,   77,   71,
 /*   120 */   277,   94,  202,  190,  203,   58,  178,  176,  177,   53,
 /*   130 */    54,  151,  152,  154,   55,   20,   89,  205,  206,  207,
 /*   140 */   208,  209,  210,  211,  212,   19,  151,  152,  154,   46,
 /*   150 */    22,  182,   96,  118,  119,  193,  184,   72,  238,  120,
 /*   160 */   121,   76,  122,  237,   77,   71,  151,  152,  202,  190,
 /*   170 */   203,   73,  178,  176,  177,   97,   18,  111,  193,  184,
 /*   180 */    72,   50,  120,  121,   76,  122,  168,   77,   71,  232,
 /*   190 */     1,  202,  190,  203,  125,   38,   48,  182,   96,  118,
 /*   200 */   119,  193,  184,   72,   21,  120,  121,   76,  122,   18,
 /*   210 */    77,   71,  180,   37,  202,  190,  203,  108,  178,  176,
 /*   220 */   177,  140,  143,   97,  150,  247,  183,  184,   72,  271,
 /*   230 */   120,  121,   76,  122,  131,   77,   71,  156,  257,  202,
 /*   240 */   190,  203,   36,  182,   96,  118,  119,  193,  184,   72,
 /*   250 */   137,  120,  121,   76,  122,  241,   77,   71,  160,  262,
 /*   260 */   202,  190,  203,   99,  178,  176,  177,   97,  138,  168,
 /*   270 */   185,  184,   72,    1,  120,  121,   76,  122,  151,   77,
 /*   280 */    71,  163,  266,  202,  190,  203,  251,  252,  141,  182,
 /*   290 */    96,  118,  119,  193,  184,   72,   37,  120,  121,   76,
 /*   300 */   122,  198,   77,   71,  245,  127,  202,  190,  203,  101,
 /*   310 */   178,  176,  177,  196,  197,   97,   25,  199,  200,  201,
 /*   320 */   112,   26,  120,  121,   76,  122,   18,   77,   71,  249,
 /*   330 */     4,  202,  190,  203,  255,  182,   96,  118,  119,  193,
 /*   340 */   184,   72,  256,  120,  121,   76,  122,  258,   77,   71,
 /*   350 */   213,   97,  202,  190,  203,   60,  178,  176,  177,  114,
 /*   360 */    76,  122,  261,   77,   71,  194,  195,  202,  190,  203,
 /*   370 */   263,  146,  147,  158,  162,  164,  268,   95,  265,  260,
 /*   380 */    80,  182,   96,  118,  119,  193,  184,   72,  267,  120,
 /*   390 */   121,   76,  122,  166,   77,   71,   97,  179,  202,  190,
 /*   400 */   203,   61,  178,  176,  177,   74,  122,   18,   77,   71,
 /*   410 */   280,   18,  202,  190,  203,  165,  147,  158,  162,  164,
 /*   420 */   268,    5,   18,  260,    9,   39,   43,  182,   96,  118,
 /*   430 */   119,  193,  184,   72,   44,  120,  121,   76,  122,   34,
 /*   440 */    77,   71,   97,   14,  202,  190,  203,  135,  178,  176,
 /*   450 */   177,   86,  115,   97,   77,   71,   40,   45,  202,  190,
 /*   460 */   203,  151,  152,  154,   18,   75,   71,  233,   17,  202,
 /*   470 */   190,  203,   49,  182,   96,  118,  119,  193,  184,   72,
 /*   480 */    62,  120,  121,   76,  122,   97,   77,   71,   27,  214,
 /*   490 */   202,  190,  203,  102,  178,  176,  177,   97,   70,  217,
 /*   500 */   170,  202,  190,  203,  148,  155,  157,  259,   30,    3,
 /*   510 */   260,    4,   29,  186,  190,  203,   63,   78,    6,  182,
 /*   520 */    96,  118,  119,  193,  184,   72,    7,  120,  121,   76,
 /*   530 */   122,   97,   77,   71,   97,  236,  202,  190,  203,  103,
 /*   540 */   178,  176,  177,   97,    8,   81,   10,  187,  190,  203,
 /*   550 */   188,  190,  203,   18,   18,   18,  239,  244,  278,  189,
 /*   560 */   190,  203,  139,   22,   83,  182,   96,  118,  119,  193,
 /*   570 */   184,   72,  240,  120,  121,   76,  122,  142,   77,   71,
 /*   580 */   242,  145,  202,  190,  203,  104,  178,  176,  177,   51,
 /*   590 */    56,   47,   64,   11,   84,  246,  248,   85,   69,  270,
 /*   600 */    65,   12,   87,   88,   57,   66,   90,   91,   67,   92,
 /*   610 */    93,  182,   96,  118,  119,  193,  184,   72,  273,  120,
 /*   620 */   121,   76,  122,  276,   77,   71,  171,   13,  202,  190,
 /*   630 */   203,  172,  178,  176,  177,  450,  450,  450,  450,  450,
 /*   640 */   272,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*   650 */   450,  450,  450,  450,  450,  450,  450,  182,   96,  118,
 /*   660 */   119,  193,  184,   72,  450,  120,  121,   76,  122,  450,
 /*   670 */    77,   71,  450,  450,  202,  190,  203,  173,  178,  176,
 /*   680 */   177,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*   690 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*   700 */   450,  450,  450,  182,   96,  118,  119,  193,  184,   72,
 /*   710 */   450,  120,  121,   76,  122,  450,   77,   71,  450,  450,
 /*   720 */   202,  190,  203,  174,  178,  176,  177,  450,  450,  450,
 /*   730 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*   740 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  182,
 /*   750 */    96,  118,  119,  193,  184,   72,  450,  120,  121,   76,
 /*   760 */   122,  450,   77,   71,  450,  450,  202,  190,  203,  106,
 /*   770 */   178,  176,  177,  450,  450,  450,  450,  450,  450,  450,
 /*   780 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*   790 */   450,  450,  450,  450,  450,  182,   96,  118,  119,  193,
 /*   800 */   184,   72,  450,  120,  121,   76,  122,  450,   77,   71,
 /*   810 */   450,  450,  202,  190,  203,  227,  450,  450,  450,  450,
 /*   820 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*   830 */   450,  450,  450,  450,  450,  117,  226,  228,  182,   96,
 /*   840 */   118,  119,  193,  184,   72,  450,  120,  121,   76,  122,
 /*   850 */   450,   77,   71,  450,  450,  202,  190,  203,  175,  176,
 /*   860 */   177,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*   870 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*   880 */   126,  450,  450,  182,   96,  118,  119,  193,  184,   72,
 /*   890 */   450,  120,  121,   76,  122,  450,   77,   71,  450,  450,
 /*   900 */   202,  190,  203,  182,   96,  118,  119,  193,  184,   72,
 /*   910 */   450,  120,  121,   76,  122,  450,   77,   71,  450,  450,
 /*   920 */   202,  190,  203,  223,  450,   79,  450,  220,  450,  450,
 /*   930 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*   940 */   450,  450,  450,  450,  450,  126,  182,   96,  118,  119,
 /*   950 */   193,  184,   72,  450,  120,  121,   76,  122,  450,   77,
 /*   960 */    71,  450,  450,  202,  190,  203,  450,  100,  182,   96,
 /*   970 */   118,  119,  193,  184,   72,  450,  120,  121,   76,  122,
 /*   980 */   128,   77,   71,  450,  450,  202,  190,  203,   28,  450,
 /*   990 */   450,   25,  218,  450,  450,  450,  450,   41,  450,  450,
 /*  1000 */   450,  450,  450,  450,  450,   53,   54,  450,  450,  450,
 /*  1010 */    55,   20,  450,  205,  206,  207,  208,  209,  210,  211,
 /*  1020 */   212,   19,  450,  123,  450,  450,  450,  450,  450,  450,
 /*  1030 */   450,   28,  450,  450,  450,  450,  450,  450,  450,  450,
 /*  1040 */    41,  450,  450,  450,  105,  450,  450,  450,   53,   54,
 /*  1050 */   450,  450,  450,   55,   20,  450,  205,  206,  207,  208,
 /*  1060 */   209,  210,  211,  212,   19,  216,  204,  182,   96,  118,
 /*  1070 */   119,  193,  184,   72,   28,  120,  121,   76,  122,  450,
 /*  1080 */    77,   71,  450,   41,  202,  190,  203,  450,  450,  450,
 /*  1090 */   450,   53,   54,  450,  450,  450,   55,   20,  222,  205,
 /*  1100 */   206,  207,  208,  209,  210,  211,  212,   19,   15,  450,
 /*  1110 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  204,
 /*  1120 */   450,  450,  450,  450,  450,  450,  450,   28,  450,  450,
 /*  1130 */   450,  450,  450,  450,  450,  450,   41,  450,  109,  450,
 /*  1140 */   450,  450,  450,  450,   53,   54,  450,  450,  450,   55,
 /*  1150 */    20,  450,  205,  206,  207,  208,  209,  210,  211,  212,
 /*  1160 */    19,  182,   96,  118,  119,  193,  184,   72,  181,  120,
 /*  1170 */   121,   76,  122,  450,   77,   71,  450,  450,  202,  190,
 /*  1180 */   203,  123,  450,  450,  450,  450,  450,  450,  450,   28,
 /*  1190 */   450,  182,   96,  118,  119,  193,  184,   72,   41,  120,
 /*  1200 */   121,   76,  122,  192,   77,   71,   53,   54,  202,  190,
 /*  1210 */   203,   55,   20,  450,  205,  206,  207,  208,  209,  210,
 /*  1220 */   211,  212,   19,  221,  450,  450,  182,   96,  118,  119,
 /*  1230 */   193,  184,   72,  450,  120,  121,   76,  122,  450,   77,
 /*  1240 */    71,  450,  450,  202,  190,  203,  182,   96,  118,  119,
 /*  1250 */   193,  184,   72,  450,  120,  121,   76,  122,  215,   77,
 /*  1260 */    71,  450,  450,  202,  190,  203,  450,  450,  450,  450,
 /*  1270 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*  1280 */   450,  182,   96,  118,  119,  193,  184,   72,  124,  120,
 /*  1290 */   121,   76,  122,  450,   77,   71,  450,  450,  202,  190,
 /*  1300 */   203,  450,  450,  450,  450,  450,  450,  450,  219,  450,
 /*  1310 */   450,  182,   96,  118,  119,  193,  184,   72,  450,  120,
 /*  1320 */   121,   76,  122,  450,   77,   71,  450,  450,  202,  190,
 /*  1330 */   203,  182,   96,  118,  119,  193,  184,   72,  450,  120,
 /*  1340 */   121,   76,  122,  450,   77,   71,  225,  450,  202,  190,
 /*  1350 */   203,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*  1360 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  182,
 /*  1370 */    96,  118,  119,  193,  184,   72,  234,  120,  121,   76,
 /*  1380 */   122,  450,   77,   71,  450,  450,  202,  190,  203,  450,
 /*  1390 */   450,  450,  450,  450,  450,  450,  235,  450,  450,  182,
 /*  1400 */    96,  118,  119,  193,  184,   72,  450,  120,  121,   76,
 /*  1410 */   122,  450,   77,   71,  450,  450,  202,  190,  203,  182,
 /*  1420 */    96,  118,  119,  193,  184,   72,  450,  120,  121,   76,
 /*  1430 */   122,  450,   77,   71,  132,  450,  202,  190,  203,  450,
 /*  1440 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*  1450 */   450,  450,  450,  450,  450,  450,  450,  182,   96,  118,
 /*  1460 */   119,  193,  184,   72,  134,  120,  121,   76,  122,  450,
 /*  1470 */    77,   71,  450,  450,  202,  190,  203,  450,  450,  450,
 /*  1480 */   450,  450,  450,  450,  253,  450,  450,  182,   96,  118,
 /*  1490 */   119,  193,  184,   72,  450,  120,  121,   76,  122,  450,
 /*  1500 */    77,   71,  450,  450,  202,  190,  203,  182,   96,  118,
 /*  1510 */   119,  193,  184,   72,  450,  120,  121,   76,  122,  450,
 /*  1520 */    77,   71,  269,  450,  202,  190,  203,  450,  450,  450,
 /*  1530 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  450,
 /*  1540 */   450,  450,  450,  450,  450,  182,   96,  118,  119,  193,
 /*  1550 */   184,   72,  167,  120,  121,   76,  122,  450,   77,   71,
 /*  1560 */   450,  450,  202,  190,  203,  204,  450,  450,  450,  450,
 /*  1570 */   450,  450,  450,   28,  450,  182,   96,  118,  119,  193,
 /*  1580 */   184,   72,   41,  120,  121,   76,  122,  450,   77,   71,
 /*  1590 */    53,   54,  202,  190,  203,   55,   20,  450,  205,  206,
 /*  1600 */   207,  208,  209,  210,  211,  212,   19,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   85,   86,   12,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   88,   76,   77,   78,   19,   20,   81,
 /*    20 */    95,   96,   97,   98,   65,  100,  101,   29,   25,  104,
 /*    30 */   105,  106,   85,   86,   64,   37,   38,   76,   77,   78,
 /*    40 */    42,   43,   81,   45,   46,   47,   48,   49,   50,   51,
 /*    50 */    52,   53,   82,   83,   84,   85,   86,   87,   88,   89,
 /*    60 */    90,   91,   92,   93,   67,   95,   96,   97,   98,   72,
 /*    70 */   100,  101,  113,   66,  104,  105,  106,  103,   60,   61,
 /*    80 */    62,   63,   64,   88,   89,   90,   91,   92,   93,   17,
 /*    90 */    95,   96,   97,   98,    1,  100,  101,   66,    5,  104,
 /*   100 */   105,  106,   30,   31,   12,   87,   88,   89,   90,   91,
 /*   110 */    92,   93,   20,   95,   96,   97,   98,   94,  100,  101,
 /*   120 */   113,   12,  104,  105,  106,   61,   62,   63,   64,   37,
 /*   130 */    38,   22,   23,   24,   42,   43,   12,   45,   46,   47,
 /*   140 */    48,   49,   50,   51,   52,   53,   22,   23,   24,   99,
 /*   150 */    57,   87,   88,   89,   90,   91,   92,   93,   67,   95,
 /*   160 */    96,   97,   98,   72,  100,  101,   22,   23,  104,  105,
 /*   170 */   106,   61,   62,   63,   64,   88,    1,   90,   91,   92,
 /*   180 */    93,  102,   95,   96,   97,   98,   16,  100,  101,  107,
 /*   190 */    20,  104,  105,  106,  110,   25,   43,   87,   88,   89,
 /*   200 */    90,   91,   92,   93,   15,   95,   96,   97,   98,    1,
 /*   210 */   100,  101,    4,   43,  104,  105,  106,   61,   62,   63,
 /*   220 */    64,   70,   71,   88,   77,   78,   91,   92,   93,   54,
 /*   230 */    95,   96,   97,   98,  112,  100,  101,   77,   78,  104,
 /*   240 */   105,  106,   17,   87,   88,   89,   90,   91,   92,   93,
 /*   250 */    68,   95,   96,   97,   98,   12,  100,  101,   77,   78,
 /*   260 */   104,  105,  106,   61,   62,   63,   64,   88,   69,   16,
 /*   270 */    91,   92,   93,   20,   95,   96,   97,   98,   22,  100,
 /*   280 */   101,   77,   78,  104,  105,  106,   79,   80,   71,   87,
 /*   290 */    88,   89,   90,   91,   92,   93,   43,   95,   96,   97,
 /*   300 */    98,   23,  100,  101,   78,   12,  104,  105,  106,   61,
 /*   310 */    62,   63,   64,   37,   38,   88,   23,   39,   40,   41,
 /*   320 */    93,   15,   95,   96,   97,   98,    1,  100,  101,   78,
 /*   330 */     5,  104,  105,  106,   80,   87,   88,   89,   90,   91,
 /*   340 */    92,   93,   78,   95,   96,   97,   98,   78,  100,  101,
 /*   350 */    44,   88,  104,  105,  106,   61,   62,   63,   64,   96,
 /*   360 */    97,   98,   78,  100,  101,   34,   35,  104,  105,  106,
 /*   370 */    78,   73,   74,   75,   76,   77,   78,   53,   78,   81,
 /*   380 */    56,   87,   88,   89,   90,   91,   92,   93,   78,   95,
 /*   390 */    96,   97,   98,  112,  100,  101,   88,    4,  104,  105,
 /*   400 */   106,   61,   62,   63,   64,   97,   98,    1,  100,  101,
 /*   410 */     0,    1,  104,  105,  106,   73,   74,   75,   76,   77,
 /*   420 */    78,    1,    1,   81,    3,   27,   31,   87,   88,   89,
 /*   430 */    90,   91,   92,   93,   32,   95,   96,   97,   98,   18,
 /*   440 */   100,  101,   88,    1,  104,  105,  106,   61,   62,   63,
 /*   450 */    64,   12,   98,   88,  100,  101,   28,   33,  104,  105,
 /*   460 */   106,   22,   23,   24,    1,  100,  101,    4,   15,  104,
 /*   470 */   105,  106,   36,   87,   88,   89,   90,   91,   92,   93,
 /*   480 */    15,   95,   96,   97,   98,   88,  100,  101,   26,   21,
 /*   490 */   104,  105,  106,   61,   62,   63,   64,   88,  101,   54,
 /*   500 */    58,  104,  105,  106,   75,   76,   77,   78,   26,    3,
 /*   510 */    81,    5,   55,  104,  105,  106,   15,   21,    1,   87,
 /*   520 */    88,   89,   90,   91,   92,   93,    1,   95,   96,   97,
 /*   530 */    98,   88,  100,  101,   88,    4,  104,  105,  106,   61,
 /*   540 */    62,   63,   64,   88,    1,   12,    1,  104,  105,  106,
 /*   550 */   104,  105,  106,    1,    1,    1,    4,    4,    4,  104,
 /*   560 */   105,  106,   15,   57,   15,   87,   88,   89,   90,   91,
 /*   570 */    92,   93,   12,   95,   96,   97,   98,   16,  100,  101,
 /*   580 */    12,   12,  104,  105,  106,   61,   62,   63,   64,   15,
 /*   590 */    15,   20,   15,   21,   15,   12,   12,   15,   12,   44,
 /*   600 */    15,    1,   15,   15,   15,   15,   15,   15,   15,   15,
 /*   610 */    15,   87,   88,   89,   90,   91,   92,   93,   12,   95,
 /*   620 */    96,   97,   98,    4,  100,  101,   12,    1,  104,  105,
 /*   630 */   106,   61,   62,   63,   64,  114,  114,  114,  114,  114,
 /*   640 */    44,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*   650 */   114,  114,  114,  114,  114,  114,  114,   87,   88,   89,
 /*   660 */    90,   91,   92,   93,  114,   95,   96,   97,   98,  114,
 /*   670 */   100,  101,  114,  114,  104,  105,  106,   61,   62,   63,
 /*   680 */    64,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*   690 */   114,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*   700 */   114,  114,  114,   87,   88,   89,   90,   91,   92,   93,
 /*   710 */   114,   95,   96,   97,   98,  114,  100,  101,  114,  114,
 /*   720 */   104,  105,  106,   61,   62,   63,   64,  114,  114,  114,
 /*   730 */   114,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*   740 */   114,  114,  114,  114,  114,  114,  114,  114,  114,   87,
 /*   750 */    88,   89,   90,   91,   92,   93,  114,   95,   96,   97,
 /*   760 */    98,  114,  100,  101,  114,  114,  104,  105,  106,   61,
 /*   770 */    62,   63,   64,  114,  114,  114,  114,  114,  114,  114,
 /*   780 */   114,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*   790 */   114,  114,  114,  114,  114,   87,   88,   89,   90,   91,
 /*   800 */    92,   93,  114,   95,   96,   97,   98,  114,  100,  101,
 /*   810 */   114,  114,  104,  105,  106,   64,  114,  114,  114,  114,
 /*   820 */   114,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*   830 */   114,  114,  114,  114,  114,   84,   85,   86,   87,   88,
 /*   840 */    89,   90,   91,   92,   93,  114,   95,   96,   97,   98,
 /*   850 */   114,  100,  101,  114,  114,  104,  105,  106,   62,   63,
 /*   860 */    64,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*   870 */   114,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*   880 */    64,  114,  114,   87,   88,   89,   90,   91,   92,   93,
 /*   890 */   114,   95,   96,   97,   98,  114,  100,  101,  114,  114,
 /*   900 */   104,  105,  106,   87,   88,   89,   90,   91,   92,   93,
 /*   910 */   114,   95,   96,   97,   98,  114,  100,  101,  114,  114,
 /*   920 */   104,  105,  106,   64,  114,  109,  114,  111,  114,  114,
 /*   930 */   114,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*   940 */   114,  114,  114,  114,  114,   64,   87,   88,   89,   90,
 /*   950 */    91,   92,   93,  114,   95,   96,   97,   98,  114,  100,
 /*   960 */   101,  114,  114,  104,  105,  106,  114,  108,   87,   88,
 /*   970 */    89,   90,   91,   92,   93,  114,   95,   96,   97,   98,
 /*   980 */    12,  100,  101,  114,  114,  104,  105,  106,   20,  114,
 /*   990 */   114,   23,  111,  114,  114,  114,  114,   29,  114,  114,
 /*  1000 */   114,  114,  114,  114,  114,   37,   38,  114,  114,  114,
 /*  1010 */    42,   43,  114,   45,   46,   47,   48,   49,   50,   51,
 /*  1020 */    52,   53,  114,   12,  114,  114,  114,  114,  114,  114,
 /*  1030 */   114,   20,  114,  114,  114,  114,  114,  114,  114,  114,
 /*  1040 */    29,  114,  114,  114,   64,  114,  114,  114,   37,   38,
 /*  1050 */   114,  114,  114,   42,   43,  114,   45,   46,   47,   48,
 /*  1060 */    49,   50,   51,   52,   53,   54,   12,   87,   88,   89,
 /*  1070 */    90,   91,   92,   93,   20,   95,   96,   97,   98,  114,
 /*  1080 */   100,  101,  114,   29,  104,  105,  106,  114,  114,  114,
 /*  1090 */   114,   37,   38,  114,  114,  114,   42,   43,   44,   45,
 /*  1100 */    46,   47,   48,   49,   50,   51,   52,   53,    1,  114,
 /*  1110 */   114,  114,  114,  114,  114,  114,  114,  114,  114,   12,
 /*  1120 */   114,  114,  114,  114,  114,  114,  114,   20,  114,  114,
 /*  1130 */   114,  114,  114,  114,  114,  114,   29,  114,   64,  114,
 /*  1140 */   114,  114,  114,  114,   37,   38,  114,  114,  114,   42,
 /*  1150 */    43,  114,   45,   46,   47,   48,   49,   50,   51,   52,
 /*  1160 */    53,   87,   88,   89,   90,   91,   92,   93,   64,   95,
 /*  1170 */    96,   97,   98,  114,  100,  101,  114,  114,  104,  105,
 /*  1180 */   106,   12,  114,  114,  114,  114,  114,  114,  114,   20,
 /*  1190 */   114,   87,   88,   89,   90,   91,   92,   93,   29,   95,
 /*  1200 */    96,   97,   98,   64,  100,  101,   37,   38,  104,  105,
 /*  1210 */   106,   42,   43,  114,   45,   46,   47,   48,   49,   50,
 /*  1220 */    51,   52,   53,   64,  114,  114,   87,   88,   89,   90,
 /*  1230 */    91,   92,   93,  114,   95,   96,   97,   98,  114,  100,
 /*  1240 */   101,  114,  114,  104,  105,  106,   87,   88,   89,   90,
 /*  1250 */    91,   92,   93,  114,   95,   96,   97,   98,   64,  100,
 /*  1260 */   101,  114,  114,  104,  105,  106,  114,  114,  114,  114,
 /*  1270 */   114,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*  1280 */   114,   87,   88,   89,   90,   91,   92,   93,   64,   95,
 /*  1290 */    96,   97,   98,  114,  100,  101,  114,  114,  104,  105,
 /*  1300 */   106,  114,  114,  114,  114,  114,  114,  114,   64,  114,
 /*  1310 */   114,   87,   88,   89,   90,   91,   92,   93,  114,   95,
 /*  1320 */    96,   97,   98,  114,  100,  101,  114,  114,  104,  105,
 /*  1330 */   106,   87,   88,   89,   90,   91,   92,   93,  114,   95,
 /*  1340 */    96,   97,   98,  114,  100,  101,   64,  114,  104,  105,
 /*  1350 */   106,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*  1360 */   114,  114,  114,  114,  114,  114,  114,  114,  114,   87,
 /*  1370 */    88,   89,   90,   91,   92,   93,   64,   95,   96,   97,
 /*  1380 */    98,  114,  100,  101,  114,  114,  104,  105,  106,  114,
 /*  1390 */   114,  114,  114,  114,  114,  114,   64,  114,  114,   87,
 /*  1400 */    88,   89,   90,   91,   92,   93,  114,   95,   96,   97,
 /*  1410 */    98,  114,  100,  101,  114,  114,  104,  105,  106,   87,
 /*  1420 */    88,   89,   90,   91,   92,   93,  114,   95,   96,   97,
 /*  1430 */    98,  114,  100,  101,   64,  114,  104,  105,  106,  114,
 /*  1440 */   114,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*  1450 */   114,  114,  114,  114,  114,  114,  114,   87,   88,   89,
 /*  1460 */    90,   91,   92,   93,   64,   95,   96,   97,   98,  114,
 /*  1470 */   100,  101,  114,  114,  104,  105,  106,  114,  114,  114,
 /*  1480 */   114,  114,  114,  114,   64,  114,  114,   87,   88,   89,
 /*  1490 */    90,   91,   92,   93,  114,   95,   96,   97,   98,  114,
 /*  1500 */   100,  101,  114,  114,  104,  105,  106,   87,   88,   89,
 /*  1510 */    90,   91,   92,   93,  114,   95,   96,   97,   98,  114,
 /*  1520 */   100,  101,   64,  114,  104,  105,  106,  114,  114,  114,
 /*  1530 */   114,  114,  114,  114,  114,  114,  114,  114,  114,  114,
 /*  1540 */   114,  114,  114,  114,  114,   87,   88,   89,   90,   91,
 /*  1550 */    92,   93,   64,   95,   96,   97,   98,  114,  100,  101,
 /*  1560 */   114,  114,  104,  105,  106,   12,  114,  114,  114,  114,
 /*  1570 */   114,  114,  114,   20,  114,   87,   88,   89,   90,   91,
 /*  1580 */    92,   93,   29,   95,   96,   97,   98,  114,  100,  101,
 /*  1590 */    37,   38,  104,  105,  106,   42,   43,  114,   45,   46,
 /*  1600 */    47,   48,   49,   50,   51,   52,   53,
};
#define YY_SHIFT_USE_DFLT (-10)
#define YY_SHIFT_MAX 174
static const short yy_shift_ofst[] = {
 /*     0 */    -2,  968,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,  968,   -2, 1011,
 /*    20 */  1054, 1169, 1107, 1553, 1553, 1553, 1553, 1553, 1553, 1553,
 /*    30 */  1553, 1553, 1553, 1553, 1553, 1553, 1553, 1553, 1553, 1553,
 /*    40 */  1553, 1553,   92,   92,   92,   92,   92,  109,  109,   92,
 /*    50 */    92,  124,   92,   92,   92,   92,  439,  439,   93,  506,
 /*    60 */   421,  421,  293,  293,  144,  144,  144,  144,   -9,    3,
 /*    70 */   278,  278,   72,  325,  331,  276,  331,  276,  324,  189,
 /*    80 */   153,  225,  243,   -9,  256,  256,    3,  256,  256,    3,
 /*    90 */   256,  256,  256,  256,    3,  153,  170,  253,  410,  208,
 /*   100 */   306,  463,  552,  553,  175,  442,  554,  393,  406,  420,
 /*   110 */   398,  428,  395,  402,  424,  436,  453,  465,  398,  428,
 /*   120 */   402,  424,  436,  462,  468,  445,  457,  482,  482,  501,
 /*   130 */   496,  517,  525,  531,  543,  406,  533,  545,  547,  560,
 /*   140 */   549,  561,  568,  561,  569,  571,  572,  574,  575,  577,
 /*   150 */   579,  583,  584,  582,  586,  585,  587,  588,  589,  590,
 /*   160 */   591,  592,  593,  594,  595,  555,  600,  596,  606,  619,
 /*   170 */   614,  626,  406,  406,  406,
};
#define YY_REDUCE_USE_DFLT (-85)
#define YY_REDUCE_MAX 95
static const short yy_reduce_ofst[] = {
 /*     0 */    18,  -30,   64,  110,  156,  202,  248,  294,  340,  386,
 /*    10 */   432,  478,  524,  570,  616,  662,  708,  751,  796,  816,
 /*    20 */   859,  881,  980, 1074, 1104, 1139, 1159, 1194, 1224, 1244,
 /*    30 */  1282, 1312, 1332, 1370, 1400, 1420, 1458, 1488,   -5,   87,
 /*    40 */   135,  179,  227,  -75,  263,  308,  354,  298,  342,  365,
 /*    50 */   397,  429,  409,  443,  446,  455,  -62,  -39,  -41,    7,
 /*    60 */    -3,   91,  -84,  -53,  147,  160,  181,  204,  151,  207,
 /*    70 */   -26,  -26,   23,   31,   50,   79,   50,   79,   82,   84,
 /*    80 */   122,  182,  199,  217,  226,  251,  254,  264,  269,  254,
 /*    90 */   284,  292,  300,  310,  254,  281,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   283,  356,  283,  283,  283,  283,  283,  283,  283,  283,
 /*    10 */   283,  283,  283,  283,  283,  283,  283,  448,  283,  448,
 /*    20 */   448,  435,  448,  448,  290,  448,  448,  448,  448,  448,
 /*    30 */   448,  292,  294,  448,  448,  448,  448,  448,  448,  448,
 /*    40 */   448,  448,  448,  448,  448,  448,  448,  344,  344,  448,
 /*    50 */   448,  448,  448,  448,  448,  448,  448,  448,  448,  446,
 /*    60 */   310,  310,  448,  448,  448,  448,  448,  448,  448,  348,
 /*    70 */   396,  395,  379,  446,  388,  394,  387,  393,  436,  434,
 /*    80 */   439,  306,  448,  448,  448,  448,  448,  448,  448,  352,
 /*    90 */   448,  448,  448,  448,  351,  439,  409,  409,  448,  448,
 /*   100 */   448,  448,  448,  448,  448,  448,  448,  448,  447,  448,
 /*   110 */   371,  374,  380,  384,  386,  390,  357,  358,  372,  373,
 /*   120 */   383,  385,  389,  414,  448,  448,  448,  448,  414,  361,
 /*   130 */   448,  448,  448,  448,  448,  311,  448,  448,  298,  448,
 /*   140 */   299,  301,  448,  300,  448,  448,  448,  328,  320,  316,
 /*   150 */   314,  448,  448,  318,  448,  324,  322,  326,  336,  332,
 /*   160 */   330,  334,  340,  338,  342,  448,  448,  448,  448,  448,
 /*   170 */   448,  448,  443,  444,  445,  282,  284,  285,  281,  286,
 /*   180 */   289,  291,  370,  376,  377,  378,  399,  405,  406,  407,
 /*   190 */   408,  359,  364,  375,  391,  392,  397,  398,  401,  402,
 /*   200 */   403,  404,  400,  410,  414,  415,  416,  417,  418,  419,
 /*   210 */   420,  421,  422,  423,  427,  433,  425,  426,  431,  432,
 /*   220 */   430,  429,  424,  428,  368,  369,  360,  366,  367,  362,
 /*   230 */   363,  365,  411,  437,  293,  295,  296,  308,  309,  297,
 /*   240 */   305,  304,  303,  302,  312,  313,  345,  315,  346,  317,
 /*   250 */   319,  347,  349,  350,  354,  355,  321,  323,  325,  327,
 /*   260 */   353,  329,  331,  333,  335,  337,  339,  341,  343,  307,
 /*   270 */   440,  438,  412,  413,  381,  382,  287,  442,  288,  441,
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
  "LPAR",          "RPAR",          "STAR_STAR",     "STAR",        
  "AMPER",         "EQUAL",         "COLON",         "BAR_BAR",     
  "AND_AND",       "NOT",           "LESS",          "XOR",         
  "BAR",           "AND",           "LSHIFT",        "RSHIFT",      
  "EQUAL_TILDA",   "PLUS",          "MINUS",         "DIV",         
  "DIV_DIV",       "PERCENT",       "TILDA",         "LBRACKET",    
  "RBRACKET",      "NUMBER",        "REGEXP",        "STRING",      
  "SYMBOL",        "NIL",           "TRUE",          "FALSE",       
  "LINE",          "LBRACE",        "RBRACE",        "EQUAL_GREATER",
  "DO",            "EXCEPT",        "AS",            "error",       
  "module",        "stmts",         "stmt",          "func_def",    
  "expr",          "excepts",       "finally_opt",   "if_tail",     
  "super_opt",     "names",         "dotted_names",  "dotted_name", 
  "else_opt",      "params",        "params_without_default",  "params_with_default",
  "block_param",   "var_param",     "kw_param",      "param_default_opt",
  "param_default",  "param_with_default",  "args",          "posargs",     
  "kwargs",        "vararg",        "kwarg",         "assign_expr", 
  "postfix_expr",  "logical_or_expr",  "logical_and_expr",  "not_expr",    
  "comparison",    "xor_expr",      "comp_op",       "or_expr",     
  "and_expr",      "shift_expr",    "match_expr",    "shift_op",    
  "arith_expr",    "term",          "arith_op",      "term_op",     
  "factor",        "power",         "atom",          "blockarg_opt",
  "exprs",         "dict_elems",    "comma_opt",     "dict_elem",   
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
 /*  65 */ "kw_param ::= STAR_STAR NAME",
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
 /*  76 */ "args ::=",
 /*  77 */ "args ::= posargs",
 /*  78 */ "args ::= posargs COMMA kwargs",
 /*  79 */ "args ::= posargs COMMA kwargs COMMA vararg",
 /*  80 */ "args ::= posargs COMMA vararg",
 /*  81 */ "args ::= kwargs",
 /*  82 */ "args ::= kwargs COMMA vararg",
 /*  83 */ "args ::= vararg",
 /*  84 */ "vararg ::= STAR expr",
 /*  85 */ "posargs ::= expr",
 /*  86 */ "posargs ::= posargs COMMA expr",
 /*  87 */ "kwargs ::= kwarg",
 /*  88 */ "kwargs ::= kwargs COMMA kwarg",
 /*  89 */ "kwarg ::= NAME COLON expr",
 /*  90 */ "expr ::= assign_expr",
 /*  91 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /*  92 */ "assign_expr ::= logical_or_expr",
 /*  93 */ "logical_or_expr ::= logical_and_expr",
 /*  94 */ "logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr",
 /*  95 */ "logical_and_expr ::= not_expr",
 /*  96 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /*  97 */ "not_expr ::= comparison",
 /*  98 */ "not_expr ::= NOT not_expr",
 /*  99 */ "comparison ::= xor_expr",
 /* 100 */ "comparison ::= xor_expr comp_op xor_expr",
 /* 101 */ "comp_op ::= LESS",
 /* 102 */ "comp_op ::= GREATER",
 /* 103 */ "xor_expr ::= or_expr",
 /* 104 */ "xor_expr ::= xor_expr XOR or_expr",
 /* 105 */ "or_expr ::= and_expr",
 /* 106 */ "or_expr ::= or_expr BAR and_expr",
 /* 107 */ "and_expr ::= shift_expr",
 /* 108 */ "and_expr ::= and_expr AND shift_expr",
 /* 109 */ "shift_expr ::= match_expr",
 /* 110 */ "shift_expr ::= shift_expr shift_op match_expr",
 /* 111 */ "shift_op ::= LSHIFT",
 /* 112 */ "shift_op ::= RSHIFT",
 /* 113 */ "match_expr ::= arith_expr",
 /* 114 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 115 */ "arith_expr ::= term",
 /* 116 */ "arith_expr ::= arith_expr arith_op term",
 /* 117 */ "arith_op ::= PLUS",
 /* 118 */ "arith_op ::= MINUS",
 /* 119 */ "term ::= term term_op factor",
 /* 120 */ "term ::= factor",
 /* 121 */ "term_op ::= STAR",
 /* 122 */ "term_op ::= DIV",
 /* 123 */ "term_op ::= DIV_DIV",
 /* 124 */ "term_op ::= PERCENT",
 /* 125 */ "factor ::= PLUS factor",
 /* 126 */ "factor ::= MINUS factor",
 /* 127 */ "factor ::= TILDA factor",
 /* 128 */ "factor ::= power",
 /* 129 */ "power ::= postfix_expr",
 /* 130 */ "postfix_expr ::= atom",
 /* 131 */ "postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt",
 /* 132 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 133 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 134 */ "atom ::= NAME",
 /* 135 */ "atom ::= NUMBER",
 /* 136 */ "atom ::= REGEXP",
 /* 137 */ "atom ::= STRING",
 /* 138 */ "atom ::= SYMBOL",
 /* 139 */ "atom ::= NIL",
 /* 140 */ "atom ::= TRUE",
 /* 141 */ "atom ::= FALSE",
 /* 142 */ "atom ::= LINE",
 /* 143 */ "atom ::= LBRACKET exprs RBRACKET",
 /* 144 */ "atom ::= LBRACKET RBRACKET",
 /* 145 */ "atom ::= LBRACE RBRACE",
 /* 146 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 147 */ "atom ::= LPAR expr RPAR",
 /* 148 */ "exprs ::= expr",
 /* 149 */ "exprs ::= exprs COMMA expr",
 /* 150 */ "dict_elems ::= dict_elem",
 /* 151 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 152 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 153 */ "dict_elem ::= NAME COLON expr",
 /* 154 */ "comma_opt ::=",
 /* 155 */ "comma_opt ::= COMMA",
 /* 156 */ "blockarg_opt ::=",
 /* 157 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 158 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 159 */ "blockarg_params_opt ::=",
 /* 160 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 161 */ "excepts ::= except",
 /* 162 */ "excepts ::= excepts except",
 /* 163 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 164 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 165 */ "except ::= EXCEPT NEWLINE stmts",
 /* 166 */ "finally_opt ::=",
 /* 167 */ "finally_opt ::= FINALLY stmts",
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
  { 82, 0 },
  { 82, 1 },
  { 82, 3 },
  { 82, 5 },
  { 82, 3 },
  { 82, 1 },
  { 82, 3 },
  { 82, 1 },
  { 85, 2 },
  { 83, 1 },
  { 83, 3 },
  { 84, 1 },
  { 84, 3 },
  { 86, 3 },
  { 64, 1 },
  { 87, 3 },
  { 87, 1 },
  { 89, 1 },
  { 89, 3 },
  { 90, 1 },
  { 90, 3 },
  { 91, 1 },
  { 91, 2 },
  { 92, 1 },
  { 92, 3 },
  { 94, 1 },
  { 94, 1 },
  { 93, 1 },
  { 93, 3 },
  { 95, 1 },
  { 95, 3 },
  { 96, 1 },
  { 96, 3 },
  { 97, 1 },
  { 97, 3 },
  { 99, 1 },
  { 99, 1 },
  { 98, 1 },
  { 98, 3 },
  { 100, 1 },
  { 100, 3 },
  { 102, 1 },
  { 102, 1 },
  { 101, 3 },
  { 101, 1 },
  { 103, 1 },
  { 103, 1 },
  { 103, 1 },
  { 103, 1 },
  { 104, 2 },
  { 104, 2 },
  { 104, 2 },
  { 104, 1 },
  { 105, 1 },
  { 88, 1 },
  { 88, 5 },
  { 88, 4 },
  { 88, 3 },
  { 106, 1 },
  { 106, 1 },
  { 106, 1 },
  { 106, 1 },
  { 106, 1 },
  { 106, 1 },
  { 106, 1 },
  { 106, 1 },
  { 106, 1 },
  { 106, 3 },
  { 106, 2 },
  { 106, 2 },
  { 106, 4 },
  { 106, 3 },
  { 108, 1 },
  { 108, 3 },
  { 109, 1 },
  { 109, 3 },
  { 111, 3 },
  { 111, 3 },
  { 110, 0 },
  { 110, 1 },
  { 107, 0 },
  { 107, 5 },
  { 107, 5 },
  { 112, 0 },
  { 112, 3 },
  { 65, 1 },
  { 65, 2 },
  { 113, 6 },
  { 113, 4 },
  { 113, 3 },
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
#line 682 "parser.y"
{
    *pval = yymsp[0].minor.yy115;
}
#line 2083 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 85: /* posargs ::= expr */
      case 87: /* kwargs ::= kwarg */
      case 148: /* exprs ::= expr */
      case 150: /* dict_elems ::= dict_elem */
      case 161: /* excepts ::= except */
#line 686 "parser.y"
{
    yygotominor.yy115 = make_array_with(env, yymsp[0].minor.yy115);
}
#line 2097 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
#line 689 "parser.y"
{
    yygotominor.yy115 = Array_push(env, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2106 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 76: /* args ::= */
      case 154: /* comma_opt ::= */
      case 156: /* blockarg_opt ::= */
      case 159: /* blockarg_params_opt ::= */
      case 166: /* finally_opt ::= */
#line 693 "parser.y"
{
    yygotominor.yy115 = YNIL;
}
#line 2121 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 5: /* stmt ::= expr */
      case 27: /* super_opt ::= GREATER expr */
      case 28: /* if_tail ::= else_opt */
      case 31: /* else_opt ::= ELSE stmts */
      case 69: /* param_default_opt ::= param_default */
      case 70: /* param_default ::= EQUAL expr */
      case 84: /* vararg ::= STAR expr */
      case 90: /* expr ::= assign_expr */
      case 92: /* assign_expr ::= logical_or_expr */
      case 93: /* logical_or_expr ::= logical_and_expr */
      case 95: /* logical_and_expr ::= not_expr */
      case 97: /* not_expr ::= comparison */
      case 99: /* comparison ::= xor_expr */
      case 103: /* xor_expr ::= or_expr */
      case 105: /* or_expr ::= and_expr */
      case 107: /* and_expr ::= shift_expr */
      case 109: /* shift_expr ::= match_expr */
      case 113: /* match_expr ::= arith_expr */
      case 115: /* arith_expr ::= term */
      case 120: /* term ::= factor */
      case 128: /* factor ::= power */
      case 129: /* power ::= postfix_expr */
      case 130: /* postfix_expr ::= atom */
      case 167: /* finally_opt ::= FINALLY stmts */
#line 696 "parser.y"
{
    yygotominor.yy115 = yymsp[0].minor.yy115;
}
#line 2152 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 702 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy115 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy115, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[-1].minor.yy115);
}
#line 2160 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 706 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy115 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy115, yymsp[-2].minor.yy115, YNIL, yymsp[-1].minor.yy115);
}
#line 2168 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 710 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy115 = Finally_new(env, lineno, yymsp[-3].minor.yy115, yymsp[-1].minor.yy115);
}
#line 2176 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 714 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy115 = While_new(env, lineno, yymsp[-3].minor.yy115, yymsp[-1].minor.yy115);
}
#line 2184 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 718 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy115 = Break_new(env, lineno, YNIL);
}
#line 2192 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 722 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy115 = Break_new(env, lineno, yymsp[0].minor.yy115);
}
#line 2200 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 726 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy115 = Next_new(env, lineno, YNIL);
}
#line 2208 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 730 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy115 = Next_new(env, lineno, yymsp[0].minor.yy115);
}
#line 2216 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 734 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy115 = Return_new(env, lineno, YNIL);
}
#line 2224 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 738 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy115 = Return_new(env, lineno, yymsp[0].minor.yy115);
}
#line 2232 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 742 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy115 = If_new(env, lineno, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[-1].minor.yy115);
}
#line 2240 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 746 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy115 = Klass_new(env, lineno, id, yymsp[-3].minor.yy115, yymsp[-1].minor.yy115);
}
#line 2249 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 751 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy115 = Nonlocal_new(env, lineno, yymsp[0].minor.yy115);
}
#line 2257 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 755 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy115 = Import_new(env, lineno, yymsp[0].minor.yy115);
}
#line 2265 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 767 "parser.y"
{
    yygotominor.yy115 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2273 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 770 "parser.y"
{
    yygotominor.yy115 = Array_push_token_id(env, yymsp[-2].minor.yy115, yymsp[0].minor.yy0);
}
#line 2281 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 791 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy115, yymsp[-1].minor.yy115, yymsp[0].minor.yy115);
    yygotominor.yy115 = make_array_with(env, node);
}
#line 2290 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 804 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy115 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy115, yymsp[-1].minor.yy115);
}
#line 2299 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 810 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-8].minor.yy115, yymsp[-6].minor.yy115, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2306 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 813 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-6].minor.yy115, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[0].minor.yy115, YNIL);
}
#line 2313 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 816 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-6].minor.yy115, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, YNIL, yymsp[0].minor.yy115);
}
#line 2320 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 819 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[0].minor.yy115, YNIL, YNIL);
}
#line 2327 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 822 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-6].minor.yy115, yymsp[-4].minor.yy115, YNIL, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2334 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 825 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, YNIL, yymsp[0].minor.yy115, YNIL);
}
#line 2341 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 828 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, YNIL, YNIL, yymsp[0].minor.yy115);
}
#line 2348 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 831 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-2].minor.yy115, yymsp[0].minor.yy115, YNIL, YNIL, YNIL);
}
#line 2355 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 834 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-6].minor.yy115, YNIL, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2362 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 837 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-4].minor.yy115, YNIL, yymsp[-2].minor.yy115, yymsp[0].minor.yy115, YNIL);
}
#line 2369 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 840 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-4].minor.yy115, YNIL, yymsp[-2].minor.yy115, YNIL, yymsp[0].minor.yy115);
}
#line 2376 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 843 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-2].minor.yy115, YNIL, yymsp[0].minor.yy115, YNIL, YNIL);
}
#line 2383 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 846 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-4].minor.yy115, YNIL, YNIL, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2390 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 849 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-2].minor.yy115, YNIL, YNIL, yymsp[0].minor.yy115, YNIL);
}
#line 2397 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 852 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[-2].minor.yy115, YNIL, YNIL, YNIL, yymsp[0].minor.yy115);
}
#line 2404 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 855 "parser.y"
{
    yygotominor.yy115 = Params_new(env, yymsp[0].minor.yy115, YNIL, YNIL, YNIL, YNIL);
}
#line 2411 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 858 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, yymsp[-6].minor.yy115, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2418 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 861 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[0].minor.yy115, YNIL);
}
#line 2425 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 864 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, YNIL, yymsp[0].minor.yy115);
}
#line 2432 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 867 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, yymsp[-2].minor.yy115, yymsp[0].minor.yy115, YNIL, YNIL);
}
#line 2439 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 870 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, yymsp[-4].minor.yy115, YNIL, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2446 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 873 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, yymsp[-2].minor.yy115, YNIL, yymsp[0].minor.yy115, YNIL);
}
#line 2453 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 876 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, yymsp[-2].minor.yy115, YNIL, YNIL, yymsp[0].minor.yy115);
}
#line 2460 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 879 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, yymsp[0].minor.yy115, YNIL, YNIL, YNIL);
}
#line 2467 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 882 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2474 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 885 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy115, yymsp[0].minor.yy115, YNIL);
}
#line 2481 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 888 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy115, YNIL, yymsp[0].minor.yy115);
}
#line 2488 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 891 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy115, YNIL, YNIL);
}
#line 2495 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 894 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2502 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 897 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy115, YNIL);
}
#line 2509 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 900 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy115);
}
#line 2516 "parser.c"
        break;
      case 64: /* params ::= */
#line 903 "parser.y"
{
    yygotominor.yy115 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2523 "parser.c"
        break;
      case 65: /* kw_param ::= STAR_STAR NAME */
#line 907 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy115 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2532 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 913 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy115 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2541 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 919 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy115 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy115);
}
#line 2550 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 936 "parser.y"
{
    yygotominor.yy115 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy115, lineno, id, YNIL);
}
#line 2560 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 942 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy115, lineno, id, YNIL);
    yygotominor.yy115 = yymsp[-2].minor.yy115;
}
#line 2570 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 956 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy115 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy115);
}
#line 2579 "parser.c"
        break;
      case 77: /* args ::= posargs */
#line 965 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy115, 0));
    yygotominor.yy115 = Args_new(env, lineno, yymsp[0].minor.yy115, YNIL, YNIL);
}
#line 2587 "parser.c"
        break;
      case 78: /* args ::= posargs COMMA kwargs */
#line 969 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy115, 0));
    yygotominor.yy115 = Args_new(env, lineno, yymsp[-2].minor.yy115, yymsp[0].minor.yy115, YNIL);
}
#line 2595 "parser.c"
        break;
      case 79: /* args ::= posargs COMMA kwargs COMMA vararg */
#line 973 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy115, 0));
    yygotominor.yy115 = Args_new(env, lineno, yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2603 "parser.c"
        break;
      case 80: /* args ::= posargs COMMA vararg */
#line 977 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy115, 0));
    yygotominor.yy115 = Args_new(env, lineno, yymsp[-2].minor.yy115, YNIL, yymsp[0].minor.yy115);
}
#line 2611 "parser.c"
        break;
      case 81: /* args ::= kwargs */
#line 981 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy115, 0));
    yygotominor.yy115 = Args_new(env, lineno, YNIL, yymsp[0].minor.yy115, YNIL);
}
#line 2619 "parser.c"
        break;
      case 82: /* args ::= kwargs COMMA vararg */
#line 985 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy115, 0));
    yygotominor.yy115 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2627 "parser.c"
        break;
      case 83: /* args ::= vararg */
#line 989 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy115);
    yygotominor.yy115 = Args_new(env, lineno, YNIL, YNIL, yymsp[0].minor.yy115);
}
#line 2635 "parser.c"
        break;
      case 86: /* posargs ::= posargs COMMA expr */
      case 88: /* kwargs ::= kwargs COMMA kwarg */
      case 149: /* exprs ::= exprs COMMA expr */
      case 151: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 1001 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
    yygotominor.yy115 = yymsp[-2].minor.yy115;
}
#line 2646 "parser.c"
        break;
      case 89: /* kwarg ::= NAME COLON expr */
#line 1014 "parser.y"
{
    yygotominor.yy115 = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(yymsp[-2].minor.yy0));
    PTR_AS(YogNode, yygotominor.yy115)->u.kwarg.name = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    PTR_AS(YogNode, yygotominor.yy115)->u.kwarg.value = yymsp[0].minor.yy115;
}
#line 2655 "parser.c"
        break;
      case 91: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 1024 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy115);
    yygotominor.yy115 = Assign_new(env, lineno, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2663 "parser.c"
        break;
      case 94: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 1035 "parser.y"
{
    yygotominor.yy115 = YogNode_new(env, NODE_LOGICAL_OR, NODE_LINENO(yymsp[-2].minor.yy115));
    NODE(yygotominor.yy115)->u.logical_or.left = yymsp[-2].minor.yy115;
    NODE(yygotominor.yy115)->u.logical_or.right = yymsp[0].minor.yy115;
}
#line 2672 "parser.c"
        break;
      case 96: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 1044 "parser.y"
{
    yygotominor.yy115 = YogNode_new(env, NODE_LOGICAL_AND, NODE_LINENO(yymsp[-2].minor.yy115));
    NODE(yygotominor.yy115)->u.logical_and.left = yymsp[-2].minor.yy115;
    NODE(yygotominor.yy115)->u.logical_and.right = yymsp[0].minor.yy115;
}
#line 2681 "parser.c"
        break;
      case 98: /* not_expr ::= NOT not_expr */
#line 1053 "parser.y"
{
    yygotominor.yy115 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy115)->u.not.expr = yymsp[0].minor.yy115;
}
#line 2689 "parser.c"
        break;
      case 100: /* comparison ::= xor_expr comp_op xor_expr */
#line 1061 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy115);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy115)->u.id;
    yygotominor.yy115 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy115, id, yymsp[0].minor.yy115);
}
#line 2698 "parser.c"
        break;
      case 101: /* comp_op ::= LESS */
      case 102: /* comp_op ::= GREATER */
      case 155: /* comma_opt ::= COMMA */
#line 1067 "parser.y"
{
    yygotominor.yy115 = yymsp[0].minor.yy0;
}
#line 2707 "parser.c"
        break;
      case 104: /* xor_expr ::= xor_expr XOR or_expr */
      case 106: /* or_expr ::= or_expr BAR and_expr */
      case 108: /* and_expr ::= and_expr AND shift_expr */
#line 1077 "parser.y"
{
    yygotominor.yy115 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy115), yymsp[-2].minor.yy115, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy115);
}
#line 2716 "parser.c"
        break;
      case 110: /* shift_expr ::= shift_expr shift_op match_expr */
      case 116: /* arith_expr ::= arith_expr arith_op term */
      case 119: /* term ::= term term_op factor */
#line 1098 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy115);
    yygotominor.yy115 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy115, VAL2ID(yymsp[-1].minor.yy115), yymsp[0].minor.yy115);
}
#line 2726 "parser.c"
        break;
      case 111: /* shift_op ::= LSHIFT */
      case 112: /* shift_op ::= RSHIFT */
      case 117: /* arith_op ::= PLUS */
      case 118: /* arith_op ::= MINUS */
      case 121: /* term_op ::= STAR */
      case 122: /* term_op ::= DIV */
      case 123: /* term_op ::= DIV_DIV */
      case 124: /* term_op ::= PERCENT */
#line 1103 "parser.y"
{
    yygotominor.yy115 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2740 "parser.c"
        break;
      case 114: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1113 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy115);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy115 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy115, id, yymsp[0].minor.yy115);
}
#line 2749 "parser.c"
        break;
      case 125: /* factor ::= PLUS factor */
#line 1155 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy115 = FuncCall_new3(env, lineno, yymsp[0].minor.yy115, id);
}
#line 2758 "parser.c"
        break;
      case 126: /* factor ::= MINUS factor */
#line 1160 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy115 = FuncCall_new3(env, lineno, yymsp[0].minor.yy115, id);
}
#line 2767 "parser.c"
        break;
      case 127: /* factor ::= TILDA factor */
#line 1165 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy115 = FuncCall_new3(env, lineno, yymsp[0].minor.yy115, id);
}
#line 2776 "parser.c"
        break;
      case 131: /* postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt */
#line 1181 "parser.y"
{
    yygotominor.yy115 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy115), yymsp[-4].minor.yy115, yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2783 "parser.c"
        break;
      case 132: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1184 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy115);
    yygotominor.yy115 = Subscript_new(env, lineno, yymsp[-3].minor.yy115, yymsp[-1].minor.yy115);
}
#line 2791 "parser.c"
        break;
      case 133: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1188 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy115);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy115 = Attr_new(env, lineno, yymsp[-2].minor.yy115, id);
}
#line 2800 "parser.c"
        break;
      case 134: /* atom ::= NAME */
#line 1194 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy115 = Variable_new(env, lineno, id);
}
#line 2809 "parser.c"
        break;
      case 135: /* atom ::= NUMBER */
      case 136: /* atom ::= REGEXP */
      case 137: /* atom ::= STRING */
      case 138: /* atom ::= SYMBOL */
#line 1199 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy115 = Literal_new(env, lineno, val);
}
#line 2821 "parser.c"
        break;
      case 139: /* atom ::= NIL */
#line 1219 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy115 = Literal_new(env, lineno, YNIL);
}
#line 2829 "parser.c"
        break;
      case 140: /* atom ::= TRUE */
#line 1223 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy115 = Literal_new(env, lineno, YTRUE);
}
#line 2837 "parser.c"
        break;
      case 141: /* atom ::= FALSE */
#line 1227 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy115 = Literal_new(env, lineno, YFALSE);
}
#line 2845 "parser.c"
        break;
      case 142: /* atom ::= LINE */
#line 1231 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy115 = Literal_new(env, lineno, val);
}
#line 2854 "parser.c"
        break;
      case 143: /* atom ::= LBRACKET exprs RBRACKET */
#line 1236 "parser.y"
{
    yygotominor.yy115 = Array_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy115);
}
#line 2861 "parser.c"
        break;
      case 144: /* atom ::= LBRACKET RBRACKET */
#line 1239 "parser.y"
{
    yygotominor.yy115 = Array_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 2868 "parser.c"
        break;
      case 145: /* atom ::= LBRACE RBRACE */
#line 1242 "parser.y"
{
    yygotominor.yy115 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 2875 "parser.c"
        break;
      case 146: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1245 "parser.y"
{
    yygotominor.yy115 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy115);
}
#line 2882 "parser.c"
        break;
      case 147: /* atom ::= LPAR expr RPAR */
      case 160: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1248 "parser.y"
{
    yygotominor.yy115 = yymsp[-1].minor.yy115;
}
#line 2890 "parser.c"
        break;
      case 152: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1267 "parser.y"
{
    yygotominor.yy115 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy115), yymsp[-2].minor.yy115, yymsp[0].minor.yy115);
}
#line 2897 "parser.c"
        break;
      case 153: /* dict_elem ::= NAME COLON expr */
#line 1270 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy115 = DictElem_new(env, lineno, var, yymsp[0].minor.yy115);
}
#line 2907 "parser.c"
        break;
      case 157: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 158: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1287 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy115 = BlockArg_new(env, lineno, yymsp[-3].minor.yy115, yymsp[-1].minor.yy115);
}
#line 2916 "parser.c"
        break;
      case 162: /* excepts ::= excepts except */
#line 1306 "parser.y"
{
    yygotominor.yy115 = Array_push(env, yymsp[-1].minor.yy115, yymsp[0].minor.yy115);
}
#line 2923 "parser.c"
        break;
      case 163: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1310 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy115 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy115, id, yymsp[0].minor.yy115);
}
#line 2933 "parser.c"
        break;
      case 164: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1316 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy115 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy115, NO_EXC_VAR, yymsp[0].minor.yy115);
}
#line 2941 "parser.c"
        break;
      case 165: /* except ::= EXCEPT NEWLINE stmts */
#line 1320 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy115 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy115);
}
#line 2949 "parser.c"
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
