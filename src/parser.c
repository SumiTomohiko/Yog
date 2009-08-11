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
        KEEP(args.varkwarg);
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
Args_new(YogEnv* env, uint_t lineno, YogVal posargs, YogVal kwargs, YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS4(env, posargs, kwargs, vararg, varkwarg);
    YogVal args = YUNDEF;
    PUSH_LOCAL(env, args);

    args = YogNode_new(env, NODE_ARGS, lineno);
    NODE(args)->u.args.posargs = posargs;
    NODE(args)->u.args.kwargs = kwargs;
    NODE(args)->u.args.vararg = vararg;
    NODE(args)->u.args.varkwarg = varkwarg;

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

    args = Args_new(env, lineno, posargs, YNIL, YNIL, YNIL);

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
#line 688 "parser.c"
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
#define YYNOCODE 116
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
#define YYNSTATE 293
#define YYNRULE 176
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
 /*     0 */     3,  137,  241,  234,   23,   24,   32,   33,   34,  145,
 /*    10 */   213,   87,   69,  102,  158,  162,  263,  153,   28,  267,
 /*    20 */   118,  127,   77,  128,   61,   78,   72,   42,  256,  211,
 /*    30 */   199,  212,  160,  161,  244,   54,   55,  168,  170,  277,
 /*    40 */    56,   20,  267,  214,  215,  216,  217,  218,  219,  220,
 /*    50 */   221,   19,  139,  121,  136,  138,  243,  239,  191,  101,
 /*    60 */   124,  125,  202,  193,   73,   36,  126,  127,   77,  128,
 /*    70 */   177,   78,   72,  292,    1,  211,  199,  212,  470,  103,
 /*    80 */   187,  185,  186,  102,  115,  125,  202,  193,   73,   53,
 /*    90 */   126,  127,   77,  128,  177,   78,   72,   38,    1,  211,
 /*   100 */   199,  212,  123,   39,  234,  178,  191,  101,  124,  125,
 /*   110 */   202,  193,   73,  238,  126,  127,   77,  128,   18,   78,
 /*   120 */    72,   38,    5,  211,  199,  212,  157,  164,  166,  272,
 /*   130 */   159,  260,  273,  122,  134,  237,  239,  191,  101,  124,
 /*   140 */   125,  202,  193,   73,   47,  126,  127,   77,  128,  135,
 /*   150 */    78,   72,   99,  290,  211,  199,  212,   28,  100,   25,
 /*   160 */    30,   85,  160,  161,  163,  133,   42,  142,   60,  187,
 /*   170 */   185,  186,  250,   18,   54,   55,   30,   17,   43,   56,
 /*   180 */    20,   94,  214,  215,  216,  217,  218,  219,  220,  221,
 /*   190 */    19,  160,  161,  163,  112,  191,  101,  124,  125,  202,
 /*   200 */   193,   73,  251,  126,  127,   77,  128,  250,   78,   72,
 /*   210 */    91,   51,  211,  199,  212,  200,   74,  187,  185,  186,
 /*   220 */   160,  161,  163,  102,  288,  116,  202,  193,   73,   22,
 /*   230 */   126,  127,   77,  128,  245,   78,   72,  287,   44,  211,
 /*   240 */   199,  212,   25,  191,  101,  124,  125,  202,  193,   73,
 /*   250 */    21,  126,  127,   77,  128,  131,   78,   72,  165,  270,
 /*   260 */   211,  199,  212,  113,  187,  185,  186,  169,  275,  102,
 /*   270 */   172,  279,  192,  193,   73,  236,  126,  127,   77,  128,
 /*   280 */   240,   78,   72,  149,  152,  211,  199,  212,  264,  265,
 /*   290 */   191,  101,  124,  125,  202,  193,   73,   26,  126,  127,
 /*   300 */    77,  128,   18,   78,   72,  189,  133,  211,  199,  212,
 /*   310 */   104,  187,  185,  186,  242,  102,   25,   30,  194,  193,
 /*   320 */    73,   49,  126,  127,   77,  128,  222,   78,   72,  203,
 /*   330 */   204,  211,  199,  212,  205,  206,  140,  191,  101,  124,
 /*   340 */   125,  202,  193,   73,   37,  126,  127,   77,  128,  146,
 /*   350 */    78,   72,  293,   18,  211,  199,  212,  106,  187,  185,
 /*   360 */   186,    4,  102,    5,  150,  207,  254,  117,  147,  126,
 /*   370 */   127,   77,  128,   18,   78,   72,  246,   18,  211,  199,
 /*   380 */   212,  208,  209,  210,  191,  101,  124,  125,  202,  193,
 /*   390 */    73,  160,  126,  127,   77,  128,   18,   78,   72,  252,
 /*   400 */   102,  211,  199,  212,   62,  187,  185,  186,  119,   77,
 /*   410 */   128,   18,   78,   72,  257,   22,  211,  199,  212,  155,
 /*   420 */   156,  167,  171,  173,  281,  258,   18,  273,   10,  262,
 /*   430 */   284,  191,  101,  124,  125,  202,  193,   73,  269,  126,
 /*   440 */   127,   77,  128,   35,   78,   72,  102,   15,  211,  199,
 /*   450 */   212,   63,  187,  185,  186,   75,  128,  102,   78,   72,
 /*   460 */   268,  271,  211,  199,  212,   18,  274,  120,  291,   78,
 /*   470 */    72,  276,  278,  211,  199,  212,  280,  175,  191,  101,
 /*   480 */   124,  125,  202,  193,   73,  188,  126,  127,   77,  128,
 /*   490 */    18,   78,   72,    6,   40,  211,  199,  212,  144,  187,
 /*   500 */   185,  186,  102,   41,  179,  174,  156,  167,  171,  173,
 /*   510 */   281,   44,   46,  273,   76,   72,    2,   45,  211,  199,
 /*   520 */   212,   64,   50,   80,  223,  191,  101,  124,  125,  202,
 /*   530 */   193,   73,  226,  126,  127,   77,  128,   27,   78,   72,
 /*   540 */   102,   31,  211,  199,  212,  107,  187,  185,  186,  102,
 /*   550 */    29,   82,  102,   71,   59,   83,  211,  199,  212,   84,
 /*   560 */     7,    8,   79,    9,   86,  195,  199,  212,  196,  199,
 /*   570 */   212,  249,  191,  101,  124,  125,  202,  193,   73,  148,
 /*   580 */   126,  127,   77,  128,   11,   78,   72,  102,  253,  211,
 /*   590 */   199,  212,  108,  187,  185,  186,  102,   88,  255,  154,
 /*   600 */   151,   48,  259,  197,  199,  212,   12,   52,   57,   65,
 /*   610 */    89,  261,  198,  199,  212,   90,   70,   66,   92,  191,
 /*   620 */   101,  124,  125,  202,  193,   73,   93,  126,  127,   77,
 /*   630 */   128,   58,   78,   72,   67,   95,  211,  199,  212,  109,
 /*   640 */   187,  185,  186,   96,   68,   97,   98,   13,  286,  283,
 /*   650 */   285,  289,  180,   14,  471,  471,  471,  471,  471,  471,
 /*   660 */   471,  471,  471,  471,  471,  471,  191,  101,  124,  125,
 /*   670 */   202,  193,   73,  471,  126,  127,   77,  128,  471,   78,
 /*   680 */    72,  471,  471,  211,  199,  212,  181,  187,  185,  186,
 /*   690 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   700 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   710 */   471,  471,  471,  191,  101,  124,  125,  202,  193,   73,
 /*   720 */   471,  126,  127,   77,  128,  471,   78,   72,  471,  471,
 /*   730 */   211,  199,  212,  182,  187,  185,  186,  471,  471,  471,
 /*   740 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   750 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   760 */   191,  101,  124,  125,  202,  193,   73,  471,  126,  127,
 /*   770 */    77,  128,  471,   78,   72,  471,  471,  211,  199,  212,
 /*   780 */   183,  187,  185,  186,  471,  471,  471,  471,  471,  471,
 /*   790 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   800 */   471,  471,  471,  471,  471,  471,  471,  191,  101,  124,
 /*   810 */   125,  202,  193,   73,  471,  126,  127,   77,  128,  471,
 /*   820 */    78,   72,  471,  471,  211,  199,  212,  111,  187,  185,
 /*   830 */   186,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   840 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   850 */   471,  471,  471,  471,  191,  101,  124,  125,  202,  193,
 /*   860 */    73,  471,  126,  127,   77,  128,  471,   78,   72,  471,
 /*   870 */   471,  211,  199,  212,  184,  185,  186,  471,  471,  471,
 /*   880 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   890 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   900 */   191,  101,  124,  125,  202,  193,   73,  471,  126,  127,
 /*   910 */    77,  128,  471,   78,   72,  471,  471,  211,  199,  212,
 /*   920 */   132,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   930 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   940 */   471,  471,  471,  471,  191,  101,  124,  125,  202,  193,
 /*   950 */    73,  471,  126,  127,   77,  128,  471,   78,   72,  471,
 /*   960 */   471,  211,  199,  212,  471,  232,   81,  471,  229,  471,
 /*   970 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*   980 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  191,
 /*   990 */   101,  124,  125,  202,  193,   73,  471,  126,  127,   77,
 /*  1000 */   128,  471,   78,   72,  471,  471,  211,  199,  212,  132,
 /*  1010 */   105,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*  1020 */   471,  471,  471,  471,  471,  471,  471,  471,  471,  471,
 /*  1030 */   471,  471,  471,  191,  101,  124,  125,  202,  193,   73,
 /*  1040 */   471,  126,  127,   77,  128,  471,   78,   72,  129,  471,
 /*  1050 */   211,  199,  212,  471,  471,  471,   28,  227,  471,  471,
 /*  1060 */   471,  471,  471,  471,  471,   42,  471,  471,  110,  471,
 /*  1070 */   471,  471,  471,   54,   55,  471,  471,  471,   56,   20,
 /*  1080 */   471,  214,  215,  216,  217,  218,  219,  220,  221,   19,
 /*  1090 */   225,  213,  191,  101,  124,  125,  202,  193,   73,   28,
 /*  1100 */   126,  127,   77,  128,  471,   78,   72,  471,   42,  211,
 /*  1110 */   199,  212,  471,  114,  471,  471,   54,   55,  471,  471,
 /*  1120 */   471,   56,   20,  231,  214,  215,  216,  217,  218,  219,
 /*  1130 */   220,  221,   19,   16,  471,  471,  471,  191,  101,  124,
 /*  1140 */   125,  202,  193,   73,  213,  126,  127,   77,  128,  471,
 /*  1150 */    78,   72,   28,  471,  211,  199,  212,  471,  471,  471,
 /*  1160 */   471,   42,  190,  471,  471,  471,  471,  471,  471,   54,
 /*  1170 */    55,  471,  471,  471,   56,   20,  471,  214,  215,  216,
 /*  1180 */   217,  218,  219,  220,  221,   19,  191,  101,  124,  125,
 /*  1190 */   202,  193,   73,  201,  126,  127,   77,  128,  471,   78,
 /*  1200 */    72,  471,  471,  211,  199,  212,  471,  471,  471,  471,
 /*  1210 */   471,  471,  471,  230,  471,  471,  471,  191,  101,  124,
 /*  1220 */   125,  202,  193,   73,  471,  126,  127,   77,  128,  471,
 /*  1230 */    78,   72,  471,  471,  211,  199,  212,  191,  101,  124,
 /*  1240 */   125,  202,  193,   73,  224,  126,  127,   77,  128,  471,
 /*  1250 */    78,   72,  471,  471,  211,  199,  212,  471,  471,  471,
 /*  1260 */   471,  471,  471,  471,  130,  471,  471,  471,  191,  101,
 /*  1270 */   124,  125,  202,  193,   73,  471,  126,  127,   77,  128,
 /*  1280 */   471,   78,   72,  471,  228,  211,  199,  212,  191,  101,
 /*  1290 */   124,  125,  202,  193,   73,  471,  126,  127,   77,  128,
 /*  1300 */   471,   78,   72,  471,  233,  211,  199,  212,  191,  101,
 /*  1310 */   124,  125,  202,  193,   73,  471,  126,  127,   77,  128,
 /*  1320 */   471,   78,   72,  471,  471,  211,  199,  212,  191,  101,
 /*  1330 */   124,  125,  202,  193,   73,  235,  126,  127,   77,  128,
 /*  1340 */   471,   78,   72,  471,  471,  211,  199,  212,  471,  471,
 /*  1350 */   471,  471,  471,  471,  471,  247,  471,  471,  471,  191,
 /*  1360 */   101,  124,  125,  202,  193,   73,  471,  126,  127,   77,
 /*  1370 */   128,  471,   78,   72,  471,  248,  211,  199,  212,  191,
 /*  1380 */   101,  124,  125,  202,  193,   73,  471,  126,  127,   77,
 /*  1390 */   128,  471,   78,   72,  471,  141,  211,  199,  212,  191,
 /*  1400 */   101,  124,  125,  202,  193,   73,  471,  126,  127,   77,
 /*  1410 */   128,  471,   78,   72,  471,  471,  211,  199,  212,  191,
 /*  1420 */   101,  124,  125,  202,  193,   73,  143,  126,  127,   77,
 /*  1430 */   128,  471,   78,   72,  471,  471,  211,  199,  212,  471,
 /*  1440 */   471,  471,  471,  471,  471,  471,  266,  471,  471,  471,
 /*  1450 */   191,  101,  124,  125,  202,  193,   73,  471,  126,  127,
 /*  1460 */    77,  128,  471,   78,   72,  471,  282,  211,  199,  212,
 /*  1470 */   191,  101,  124,  125,  202,  193,   73,  471,  126,  127,
 /*  1480 */    77,  128,  471,   78,   72,  471,  176,  211,  199,  212,
 /*  1490 */   191,  101,  124,  125,  202,  193,   73,  471,  126,  127,
 /*  1500 */    77,  128,  471,   78,   72,  471,  471,  211,  199,  212,
 /*  1510 */   191,  101,  124,  125,  202,  193,   73,  129,  126,  127,
 /*  1520 */    77,  128,  471,   78,   72,   28,  471,  211,  199,  212,
 /*  1530 */   471,  471,  471,  471,   42,  471,  471,  471,  471,  471,
 /*  1540 */   471,  471,   54,   55,  471,  471,  471,   56,   20,  471,
 /*  1550 */   214,  215,  216,  217,  218,  219,  220,  221,   19,  471,
 /*  1560 */   213,  471,  471,  471,  471,  471,  471,  471,   28,  471,
 /*  1570 */   471,  471,  471,  471,  471,  471,  471,   42,  471,  471,
 /*  1580 */   471,  213,  471,  471,  471,   54,   55,  471,  471,   28,
 /*  1590 */    56,   20,  471,  214,  215,  216,  217,  218,  219,  220,
 /*  1600 */   221,   19,  471,  471,  471,  471,   54,   55,  471,  471,
 /*  1610 */   471,   56,   20,  471,  214,  215,  216,  217,  218,  219,
 /*  1620 */   220,  221,   19,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   85,   86,   87,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   89,   76,   77,   78,   19,   20,   81,
 /*    20 */    96,   97,   98,   99,   65,  101,  102,   29,   12,  105,
 /*    30 */   106,  107,   22,   23,   64,   37,   38,   76,   77,   78,
 /*    40 */    42,   43,   81,   45,   46,   47,   48,   49,   50,   51,
 /*    50 */    52,   53,   82,   83,   84,   85,   86,   87,   88,   89,
 /*    60 */    90,   91,   92,   93,   94,   25,   96,   97,   98,   99,
 /*    70 */    16,  101,  102,  114,   20,  105,  106,  107,   60,   61,
 /*    80 */    62,   63,   64,   89,   90,   91,   92,   93,   94,  104,
 /*    90 */    96,   97,   98,   99,   16,  101,  102,   43,   20,  105,
 /*   100 */   106,  107,   85,   25,   87,   66,   88,   89,   90,   91,
 /*   110 */    92,   93,   94,   64,   96,   97,   98,   99,    1,  101,
 /*   120 */   102,   43,    5,  105,  106,  107,   75,   76,   77,   78,
 /*   130 */    77,   78,   81,   84,   85,   86,   87,   88,   89,   90,
 /*   140 */    91,   92,   93,   94,  100,   96,   97,   98,   99,   12,
 /*   150 */   101,  102,   12,  114,  105,  106,  107,   20,   53,   22,
 /*   160 */    23,   56,   22,   23,   24,   12,   29,   67,   61,   62,
 /*   170 */    63,   64,   72,    1,   37,   38,   23,    5,   95,   42,
 /*   180 */    43,   12,   45,   46,   47,   48,   49,   50,   51,   52,
 /*   190 */    53,   22,   23,   24,   66,   88,   89,   90,   91,   92,
 /*   200 */    93,   94,   67,   96,   97,   98,   99,   72,  101,  102,
 /*   210 */    12,  103,  105,  106,  107,   86,   61,   62,   63,   64,
 /*   220 */    22,   23,   24,   89,   17,   91,   92,   93,   94,   57,
 /*   230 */    96,   97,   98,   99,  108,  101,  102,   30,   31,  105,
 /*   240 */   106,  107,   22,   88,   89,   90,   91,   92,   93,   94,
 /*   250 */    15,   96,   97,   98,   99,  111,  101,  102,   77,   78,
 /*   260 */   105,  106,  107,   61,   62,   63,   64,   77,   78,   89,
 /*   270 */    77,   78,   92,   93,   94,   86,   96,   97,   98,   99,
 /*   280 */    86,  101,  102,   70,   71,  105,  106,  107,   79,   80,
 /*   290 */    88,   89,   90,   91,   92,   93,   94,   15,   96,   97,
 /*   300 */    98,   99,    1,  101,  102,    4,   12,  105,  106,  107,
 /*   310 */    61,   62,   63,   64,   86,   89,   22,   23,   92,   93,
 /*   320 */    94,   43,   96,   97,   98,   99,   44,  101,  102,   34,
 /*   330 */    35,  105,  106,  107,   37,   38,  113,   88,   89,   90,
 /*   340 */    91,   92,   93,   94,   17,   96,   97,   98,   99,   68,
 /*   350 */   101,  102,    0,    1,  105,  106,  107,   61,   62,   63,
 /*   360 */    64,    3,   89,    5,   71,   23,   12,   94,   69,   96,
 /*   370 */    97,   98,   99,    1,  101,  102,    4,    1,  105,  106,
 /*   380 */   107,   39,   40,   41,   88,   89,   90,   91,   92,   93,
 /*   390 */    94,   22,   96,   97,   98,   99,    1,  101,  102,    4,
 /*   400 */    89,  105,  106,  107,   61,   62,   63,   64,   97,   98,
 /*   410 */    99,    1,  101,  102,    4,   57,  105,  106,  107,   73,
 /*   420 */    74,   75,   76,   77,   78,   78,    1,   81,    3,   78,
 /*   430 */    54,   88,   89,   90,   91,   92,   93,   94,   78,   96,
 /*   440 */    97,   98,   99,   18,  101,  102,   89,    1,  105,  106,
 /*   450 */   107,   61,   62,   63,   64,   98,   99,   89,  101,  102,
 /*   460 */    80,   78,  105,  106,  107,    1,   78,   99,    4,  101,
 /*   470 */   102,   78,   78,  105,  106,  107,   78,  113,   88,   89,
 /*   480 */    90,   91,   92,   93,   94,    4,   96,   97,   98,   99,
 /*   490 */     1,  101,  102,    1,   27,  105,  106,  107,   61,   62,
 /*   500 */    63,   64,   89,   28,   58,   73,   74,   75,   76,   77,
 /*   510 */    78,   31,   33,   81,  101,  102,   15,   32,  105,  106,
 /*   520 */   107,   15,   36,   15,   21,   88,   89,   90,   91,   92,
 /*   530 */    93,   94,   54,   96,   97,   98,   99,   26,  101,  102,
 /*   540 */    89,   26,  105,  106,  107,   61,   62,   63,   64,   89,
 /*   550 */    55,   15,   89,  102,   15,   15,  105,  106,  107,   15,
 /*   560 */     1,    1,   21,    1,   12,  105,  106,  107,  105,  106,
 /*   570 */   107,    4,   88,   89,   90,   91,   92,   93,   94,   15,
 /*   580 */    96,   97,   98,   99,    1,  101,  102,   89,   12,  105,
 /*   590 */   106,  107,   61,   62,   63,   64,   89,   15,   12,   12,
 /*   600 */    16,   20,   12,  105,  106,  107,   21,   15,   15,   15,
 /*   610 */    15,   12,  105,  106,  107,   15,   12,   15,   15,   88,
 /*   620 */    89,   90,   91,   92,   93,   94,   15,   96,   97,   98,
 /*   630 */    99,   15,  101,  102,   15,   15,  105,  106,  107,   61,
 /*   640 */    62,   63,   64,   15,   15,   15,   15,    1,   12,   44,
 /*   650 */    44,    4,   12,    1,  115,  115,  115,  115,  115,  115,
 /*   660 */   115,  115,  115,  115,  115,  115,   88,   89,   90,   91,
 /*   670 */    92,   93,   94,  115,   96,   97,   98,   99,  115,  101,
 /*   680 */   102,  115,  115,  105,  106,  107,   61,   62,   63,   64,
 /*   690 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   700 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   710 */   115,  115,  115,   88,   89,   90,   91,   92,   93,   94,
 /*   720 */   115,   96,   97,   98,   99,  115,  101,  102,  115,  115,
 /*   730 */   105,  106,  107,   61,   62,   63,   64,  115,  115,  115,
 /*   740 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   750 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   760 */    88,   89,   90,   91,   92,   93,   94,  115,   96,   97,
 /*   770 */    98,   99,  115,  101,  102,  115,  115,  105,  106,  107,
 /*   780 */    61,   62,   63,   64,  115,  115,  115,  115,  115,  115,
 /*   790 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   800 */   115,  115,  115,  115,  115,  115,  115,   88,   89,   90,
 /*   810 */    91,   92,   93,   94,  115,   96,   97,   98,   99,  115,
 /*   820 */   101,  102,  115,  115,  105,  106,  107,   61,   62,   63,
 /*   830 */    64,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   840 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   850 */   115,  115,  115,  115,   88,   89,   90,   91,   92,   93,
 /*   860 */    94,  115,   96,   97,   98,   99,  115,  101,  102,  115,
 /*   870 */   115,  105,  106,  107,   62,   63,   64,  115,  115,  115,
 /*   880 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   890 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   900 */    88,   89,   90,   91,   92,   93,   94,  115,   96,   97,
 /*   910 */    98,   99,  115,  101,  102,  115,  115,  105,  106,  107,
 /*   920 */    64,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   930 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   940 */   115,  115,  115,  115,   88,   89,   90,   91,   92,   93,
 /*   950 */    94,  115,   96,   97,   98,   99,  115,  101,  102,  115,
 /*   960 */   115,  105,  106,  107,  115,   64,  110,  115,  112,  115,
 /*   970 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*   980 */   115,  115,  115,  115,  115,  115,  115,  115,  115,   88,
 /*   990 */    89,   90,   91,   92,   93,   94,  115,   96,   97,   98,
 /*  1000 */    99,  115,  101,  102,  115,  115,  105,  106,  107,   64,
 /*  1010 */   109,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*  1020 */   115,  115,  115,  115,  115,  115,  115,  115,  115,  115,
 /*  1030 */   115,  115,  115,   88,   89,   90,   91,   92,   93,   94,
 /*  1040 */   115,   96,   97,   98,   99,  115,  101,  102,   12,  115,
 /*  1050 */   105,  106,  107,  115,  115,  115,   20,  112,  115,  115,
 /*  1060 */   115,  115,  115,  115,  115,   29,  115,  115,   64,  115,
 /*  1070 */   115,  115,  115,   37,   38,  115,  115,  115,   42,   43,
 /*  1080 */   115,   45,   46,   47,   48,   49,   50,   51,   52,   53,
 /*  1090 */    54,   12,   88,   89,   90,   91,   92,   93,   94,   20,
 /*  1100 */    96,   97,   98,   99,  115,  101,  102,  115,   29,  105,
 /*  1110 */   106,  107,  115,   64,  115,  115,   37,   38,  115,  115,
 /*  1120 */   115,   42,   43,   44,   45,   46,   47,   48,   49,   50,
 /*  1130 */    51,   52,   53,    1,  115,  115,  115,   88,   89,   90,
 /*  1140 */    91,   92,   93,   94,   12,   96,   97,   98,   99,  115,
 /*  1150 */   101,  102,   20,  115,  105,  106,  107,  115,  115,  115,
 /*  1160 */   115,   29,   64,  115,  115,  115,  115,  115,  115,   37,
 /*  1170 */    38,  115,  115,  115,   42,   43,  115,   45,   46,   47,
 /*  1180 */    48,   49,   50,   51,   52,   53,   88,   89,   90,   91,
 /*  1190 */    92,   93,   94,   64,   96,   97,   98,   99,  115,  101,
 /*  1200 */   102,  115,  115,  105,  106,  107,  115,  115,  115,  115,
 /*  1210 */   115,  115,  115,   64,  115,  115,  115,   88,   89,   90,
 /*  1220 */    91,   92,   93,   94,  115,   96,   97,   98,   99,  115,
 /*  1230 */   101,  102,  115,  115,  105,  106,  107,   88,   89,   90,
 /*  1240 */    91,   92,   93,   94,   64,   96,   97,   98,   99,  115,
 /*  1250 */   101,  102,  115,  115,  105,  106,  107,  115,  115,  115,
 /*  1260 */   115,  115,  115,  115,   64,  115,  115,  115,   88,   89,
 /*  1270 */    90,   91,   92,   93,   94,  115,   96,   97,   98,   99,
 /*  1280 */   115,  101,  102,  115,   64,  105,  106,  107,   88,   89,
 /*  1290 */    90,   91,   92,   93,   94,  115,   96,   97,   98,   99,
 /*  1300 */   115,  101,  102,  115,   64,  105,  106,  107,   88,   89,
 /*  1310 */    90,   91,   92,   93,   94,  115,   96,   97,   98,   99,
 /*  1320 */   115,  101,  102,  115,  115,  105,  106,  107,   88,   89,
 /*  1330 */    90,   91,   92,   93,   94,   64,   96,   97,   98,   99,
 /*  1340 */   115,  101,  102,  115,  115,  105,  106,  107,  115,  115,
 /*  1350 */   115,  115,  115,  115,  115,   64,  115,  115,  115,   88,
 /*  1360 */    89,   90,   91,   92,   93,   94,  115,   96,   97,   98,
 /*  1370 */    99,  115,  101,  102,  115,   64,  105,  106,  107,   88,
 /*  1380 */    89,   90,   91,   92,   93,   94,  115,   96,   97,   98,
 /*  1390 */    99,  115,  101,  102,  115,   64,  105,  106,  107,   88,
 /*  1400 */    89,   90,   91,   92,   93,   94,  115,   96,   97,   98,
 /*  1410 */    99,  115,  101,  102,  115,  115,  105,  106,  107,   88,
 /*  1420 */    89,   90,   91,   92,   93,   94,   64,   96,   97,   98,
 /*  1430 */    99,  115,  101,  102,  115,  115,  105,  106,  107,  115,
 /*  1440 */   115,  115,  115,  115,  115,  115,   64,  115,  115,  115,
 /*  1450 */    88,   89,   90,   91,   92,   93,   94,  115,   96,   97,
 /*  1460 */    98,   99,  115,  101,  102,  115,   64,  105,  106,  107,
 /*  1470 */    88,   89,   90,   91,   92,   93,   94,  115,   96,   97,
 /*  1480 */    98,   99,  115,  101,  102,  115,   64,  105,  106,  107,
 /*  1490 */    88,   89,   90,   91,   92,   93,   94,  115,   96,   97,
 /*  1500 */    98,   99,  115,  101,  102,  115,  115,  105,  106,  107,
 /*  1510 */    88,   89,   90,   91,   92,   93,   94,   12,   96,   97,
 /*  1520 */    98,   99,  115,  101,  102,   20,  115,  105,  106,  107,
 /*  1530 */   115,  115,  115,  115,   29,  115,  115,  115,  115,  115,
 /*  1540 */   115,  115,   37,   38,  115,  115,  115,   42,   43,  115,
 /*  1550 */    45,   46,   47,   48,   49,   50,   51,   52,   53,  115,
 /*  1560 */    12,  115,  115,  115,  115,  115,  115,  115,   20,  115,
 /*  1570 */   115,  115,  115,  115,  115,  115,  115,   29,  115,  115,
 /*  1580 */   115,   12,  115,  115,  115,   37,   38,  115,  115,   20,
 /*  1590 */    42,   43,  115,   45,   46,   47,   48,   49,   50,   51,
 /*  1600 */    52,   53,  115,  115,  115,  115,   37,   38,  115,  115,
 /*  1610 */   115,   42,   43,  115,   45,   46,   47,   48,   49,   50,
 /*  1620 */    51,   52,   53,
};
#define YY_SHIFT_USE_DFLT (-3)
#define YY_SHIFT_MAX 183
static const short yy_shift_ofst[] = {
 /*     0 */    -2,  137,  137,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2, 1036,
 /*    20 */  1079, 1505, 1132, 1548, 1548, 1548, 1548, 1548, 1548, 1548,
 /*    30 */  1548, 1548, 1548, 1548, 1548, 1548, 1548, 1548, 1548, 1548,
 /*    40 */  1548, 1548, 1548, 1569, 1569, 1569, 1569, 1569,  140,  140,
 /*    50 */  1569, 1569,  169, 1569, 1569, 1569, 1569,  198,  198,  294,
 /*    60 */   172,  358,  425,  425,  153,   10,   10,   10,   10,   16,
 /*    70 */    40,  342,  342,  207,  117,  295,  297,  295,  297,  105,
 /*    80 */   220,  235,  220,  220,  220,  278,  327,  354,   16,  369,
 /*    90 */   369,   40,  369,  369,   40,  369,  369,  369,  369,   40,
 /*   100 */   278,   78,   54,  352,  301,  282,  372,  395,  410,  376,
 /*   110 */   446,  464,  481,  489,  492,  467,  475,  480,  485,  479,
 /*   120 */   486,  501,  506,  508,  467,  475,  485,  479,  486,  511,
 /*   130 */   503,  478,  495,  515,  536,  515,  539,  540,  544,  541,
 /*   140 */   559,  560,  567,  562,  489,  552,  583,  564,  576,  582,
 /*   150 */   584,  586,  584,  587,  581,  585,  592,  593,  594,  595,
 /*   160 */   590,  599,  600,  604,  602,  603,  611,  616,  619,  620,
 /*   170 */   628,  629,  630,  631,  605,  646,  606,  636,  647,  640,
 /*   180 */   652,  489,  489,  489,
};
#define YY_REDUCE_USE_DFLT (-85)
#define YY_REDUCE_MAX 100
static const short yy_reduce_ofst[] = {
 /*     0 */    18,  -30,   49,  107,  155,  202,  249,  296,  343,  390,
 /*    10 */   437,  484,  531,  578,  625,  672,  719,  766,  812,  856,
 /*    20 */   901,  945, 1004, 1049, 1098, 1129, 1149, 1180, 1200, 1220,
 /*    30 */  1240, 1271, 1291, 1311, 1331, 1362, 1382, 1402, 1422,   -6,
 /*    40 */   134,  180,  226,  273,  -76,  311,  357,  368,  346,  432,
 /*    50 */   413,  451,   51,  460,  463,  498,  507,  -62,  -39,  -84,
 /*    60 */   -41,   39,  100,  135,   17,   53,  181,  190,  193,  213,
 /*    70 */   209,  -15,  -15,   83,  128,   44,  108,   44,  108,  126,
 /*    80 */   129,  144,  189,  194,  228,  223,  281,  299,  293,  347,
 /*    90 */   351,  380,  360,  383,  380,  388,  393,  394,  398,  380,
 /*   100 */   364,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   296,  369,  469,  296,  296,  296,  296,  296,  296,  296,
 /*    10 */   296,  296,  296,  296,  296,  296,  296,  296,  296,  469,
 /*    20 */   469,  456,  469,  469,  303,  469,  469,  469,  469,  469,
 /*    30 */   469,  469,  305,  307,  469,  469,  469,  469,  469,  469,
 /*    40 */   469,  469,  469,  469,  469,  469,  469,  469,  357,  357,
 /*    50 */   469,  469,  469,  469,  469,  469,  469,  469,  469,  469,
 /*    60 */   469,  467,  323,  323,  469,  469,  469,  469,  469,  469,
 /*    70 */   361,  417,  416,  400,  467,  409,  415,  408,  414,  457,
 /*    80 */   469,  455,  469,  469,  469,  460,  319,  469,  469,  469,
 /*    90 */   469,  469,  469,  469,  365,  469,  469,  469,  469,  364,
 /*   100 */   460,  430,  430,  469,  469,  469,  469,  469,  469,  469,
 /*   110 */   469,  469,  469,  468,  469,  392,  395,  401,  405,  407,
 /*   120 */   411,  370,  371,  372,  393,  394,  404,  406,  410,  435,
 /*   130 */   469,  469,  469,  469,  374,  435,  377,  378,  381,  469,
 /*   140 */   469,  469,  469,  469,  324,  469,  469,  311,  469,  312,
 /*   150 */   314,  469,  313,  469,  469,  469,  341,  333,  329,  327,
 /*   160 */   469,  469,  331,  469,  337,  335,  339,  349,  345,  343,
 /*   170 */   347,  353,  351,  355,  469,  469,  469,  469,  469,  469,
 /*   180 */   469,  464,  465,  466,  295,  297,  298,  294,  299,  302,
 /*   190 */   304,  391,  397,  398,  399,  420,  426,  427,  428,  429,
 /*   200 */   373,  384,  396,  412,  413,  418,  419,  422,  423,  424,
 /*   210 */   425,  421,  431,  435,  436,  437,  438,  439,  440,  441,
 /*   220 */   442,  443,  444,  448,  454,  446,  447,  452,  453,  451,
 /*   230 */   450,  445,  449,  385,  389,  390,  375,  376,  387,  388,
 /*   240 */   379,  380,  382,  383,  386,  432,  458,  306,  308,  309,
 /*   250 */   321,  322,  310,  318,  317,  316,  315,  325,  326,  358,
 /*   260 */   328,  359,  330,  332,  360,  362,  363,  367,  368,  334,
 /*   270 */   336,  338,  340,  366,  342,  344,  346,  348,  350,  352,
 /*   280 */   354,  356,  320,  461,  459,  433,  434,  402,  403,  300,
 /*   290 */   463,  301,  462,
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
  "kwargs",        "vararg",        "varkwarg",      "kwarg",       
  "assign_expr",   "postfix_expr",  "logical_or_expr",  "logical_and_expr",
  "not_expr",      "comparison",    "xor_expr",      "comp_op",     
  "or_expr",       "and_expr",      "shift_expr",    "match_expr",  
  "shift_op",      "arith_expr",    "term",          "arith_op",    
  "term_op",       "factor",        "power",         "atom",        
  "blockarg_opt",  "exprs",         "dict_elems",    "comma_opt",   
  "dict_elem",     "blockarg_params_opt",  "except",      
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
 /*  80 */ "args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg",
 /*  81 */ "args ::= posargs COMMA vararg",
 /*  82 */ "args ::= posargs COMMA vararg COMMA varkwarg",
 /*  83 */ "args ::= posargs COMMA varkwarg",
 /*  84 */ "args ::= kwargs",
 /*  85 */ "args ::= kwargs COMMA vararg",
 /*  86 */ "args ::= kwargs COMMA vararg COMMA varkwarg",
 /*  87 */ "args ::= kwargs COMMA varkwarg",
 /*  88 */ "args ::= vararg",
 /*  89 */ "args ::= vararg COMMA varkwarg",
 /*  90 */ "args ::= varkwarg",
 /*  91 */ "varkwarg ::= STAR_STAR expr",
 /*  92 */ "vararg ::= STAR expr",
 /*  93 */ "posargs ::= expr",
 /*  94 */ "posargs ::= posargs COMMA expr",
 /*  95 */ "kwargs ::= kwarg",
 /*  96 */ "kwargs ::= kwargs COMMA kwarg",
 /*  97 */ "kwarg ::= NAME COLON expr",
 /*  98 */ "expr ::= assign_expr",
 /*  99 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /* 100 */ "assign_expr ::= logical_or_expr",
 /* 101 */ "logical_or_expr ::= logical_and_expr",
 /* 102 */ "logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr",
 /* 103 */ "logical_and_expr ::= not_expr",
 /* 104 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /* 105 */ "not_expr ::= comparison",
 /* 106 */ "not_expr ::= NOT not_expr",
 /* 107 */ "comparison ::= xor_expr",
 /* 108 */ "comparison ::= xor_expr comp_op xor_expr",
 /* 109 */ "comp_op ::= LESS",
 /* 110 */ "comp_op ::= GREATER",
 /* 111 */ "xor_expr ::= or_expr",
 /* 112 */ "xor_expr ::= xor_expr XOR or_expr",
 /* 113 */ "or_expr ::= and_expr",
 /* 114 */ "or_expr ::= or_expr BAR and_expr",
 /* 115 */ "and_expr ::= shift_expr",
 /* 116 */ "and_expr ::= and_expr AND shift_expr",
 /* 117 */ "shift_expr ::= match_expr",
 /* 118 */ "shift_expr ::= shift_expr shift_op match_expr",
 /* 119 */ "shift_op ::= LSHIFT",
 /* 120 */ "shift_op ::= RSHIFT",
 /* 121 */ "match_expr ::= arith_expr",
 /* 122 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 123 */ "arith_expr ::= term",
 /* 124 */ "arith_expr ::= arith_expr arith_op term",
 /* 125 */ "arith_op ::= PLUS",
 /* 126 */ "arith_op ::= MINUS",
 /* 127 */ "term ::= term term_op factor",
 /* 128 */ "term ::= factor",
 /* 129 */ "term_op ::= STAR",
 /* 130 */ "term_op ::= DIV",
 /* 131 */ "term_op ::= DIV_DIV",
 /* 132 */ "term_op ::= PERCENT",
 /* 133 */ "factor ::= PLUS factor",
 /* 134 */ "factor ::= MINUS factor",
 /* 135 */ "factor ::= TILDA factor",
 /* 136 */ "factor ::= power",
 /* 137 */ "power ::= postfix_expr",
 /* 138 */ "postfix_expr ::= atom",
 /* 139 */ "postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt",
 /* 140 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 141 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 142 */ "atom ::= NAME",
 /* 143 */ "atom ::= NUMBER",
 /* 144 */ "atom ::= REGEXP",
 /* 145 */ "atom ::= STRING",
 /* 146 */ "atom ::= SYMBOL",
 /* 147 */ "atom ::= NIL",
 /* 148 */ "atom ::= TRUE",
 /* 149 */ "atom ::= FALSE",
 /* 150 */ "atom ::= LINE",
 /* 151 */ "atom ::= LBRACKET exprs RBRACKET",
 /* 152 */ "atom ::= LBRACKET RBRACKET",
 /* 153 */ "atom ::= LBRACE RBRACE",
 /* 154 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 155 */ "atom ::= LPAR expr RPAR",
 /* 156 */ "exprs ::= expr",
 /* 157 */ "exprs ::= exprs COMMA expr",
 /* 158 */ "dict_elems ::= dict_elem",
 /* 159 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 160 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 161 */ "dict_elem ::= NAME COLON expr",
 /* 162 */ "comma_opt ::=",
 /* 163 */ "comma_opt ::= COMMA",
 /* 164 */ "blockarg_opt ::=",
 /* 165 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 166 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 167 */ "blockarg_params_opt ::=",
 /* 168 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 169 */ "excepts ::= except",
 /* 170 */ "excepts ::= excepts except",
 /* 171 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 172 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 173 */ "except ::= EXCEPT NEWLINE stmts",
 /* 174 */ "finally_opt ::=",
 /* 175 */ "finally_opt ::= FINALLY stmts",
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
  { 82, 7 },
  { 82, 3 },
  { 82, 5 },
  { 82, 3 },
  { 82, 1 },
  { 82, 3 },
  { 82, 5 },
  { 82, 3 },
  { 82, 1 },
  { 82, 3 },
  { 82, 1 },
  { 86, 2 },
  { 85, 2 },
  { 83, 1 },
  { 83, 3 },
  { 84, 1 },
  { 84, 3 },
  { 87, 3 },
  { 64, 1 },
  { 88, 3 },
  { 88, 1 },
  { 90, 1 },
  { 90, 3 },
  { 91, 1 },
  { 91, 3 },
  { 92, 1 },
  { 92, 2 },
  { 93, 1 },
  { 93, 3 },
  { 95, 1 },
  { 95, 1 },
  { 94, 1 },
  { 94, 3 },
  { 96, 1 },
  { 96, 3 },
  { 97, 1 },
  { 97, 3 },
  { 98, 1 },
  { 98, 3 },
  { 100, 1 },
  { 100, 1 },
  { 99, 1 },
  { 99, 3 },
  { 101, 1 },
  { 101, 3 },
  { 103, 1 },
  { 103, 1 },
  { 102, 3 },
  { 102, 1 },
  { 104, 1 },
  { 104, 1 },
  { 104, 1 },
  { 104, 1 },
  { 105, 2 },
  { 105, 2 },
  { 105, 2 },
  { 105, 1 },
  { 106, 1 },
  { 89, 1 },
  { 89, 5 },
  { 89, 4 },
  { 89, 3 },
  { 107, 1 },
  { 107, 1 },
  { 107, 1 },
  { 107, 1 },
  { 107, 1 },
  { 107, 1 },
  { 107, 1 },
  { 107, 1 },
  { 107, 1 },
  { 107, 3 },
  { 107, 2 },
  { 107, 2 },
  { 107, 4 },
  { 107, 3 },
  { 109, 1 },
  { 109, 3 },
  { 110, 1 },
  { 110, 3 },
  { 112, 3 },
  { 112, 3 },
  { 111, 0 },
  { 111, 1 },
  { 108, 0 },
  { 108, 5 },
  { 108, 5 },
  { 113, 0 },
  { 113, 3 },
  { 65, 1 },
  { 65, 2 },
  { 114, 6 },
  { 114, 4 },
  { 114, 3 },
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
#line 684 "parser.y"
{
    *pval = yymsp[0].minor.yy7;
}
#line 2109 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 93: /* posargs ::= expr */
      case 95: /* kwargs ::= kwarg */
      case 156: /* exprs ::= expr */
      case 158: /* dict_elems ::= dict_elem */
      case 169: /* excepts ::= except */
#line 688 "parser.y"
{
    yygotominor.yy7 = make_array_with(env, yymsp[0].minor.yy7);
}
#line 2123 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
#line 691 "parser.y"
{
    yygotominor.yy7 = Array_push(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2132 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 76: /* args ::= */
      case 162: /* comma_opt ::= */
      case 164: /* blockarg_opt ::= */
      case 167: /* blockarg_params_opt ::= */
      case 174: /* finally_opt ::= */
#line 695 "parser.y"
{
    yygotominor.yy7 = YNIL;
}
#line 2147 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 5: /* stmt ::= expr */
      case 27: /* super_opt ::= GREATER expr */
      case 28: /* if_tail ::= else_opt */
      case 31: /* else_opt ::= ELSE stmts */
      case 69: /* param_default_opt ::= param_default */
      case 70: /* param_default ::= EQUAL expr */
      case 91: /* varkwarg ::= STAR_STAR expr */
      case 92: /* vararg ::= STAR expr */
      case 98: /* expr ::= assign_expr */
      case 100: /* assign_expr ::= logical_or_expr */
      case 101: /* logical_or_expr ::= logical_and_expr */
      case 103: /* logical_and_expr ::= not_expr */
      case 105: /* not_expr ::= comparison */
      case 107: /* comparison ::= xor_expr */
      case 111: /* xor_expr ::= or_expr */
      case 113: /* or_expr ::= and_expr */
      case 115: /* and_expr ::= shift_expr */
      case 117: /* shift_expr ::= match_expr */
      case 121: /* match_expr ::= arith_expr */
      case 123: /* arith_expr ::= term */
      case 128: /* term ::= factor */
      case 136: /* factor ::= power */
      case 137: /* power ::= postfix_expr */
      case 138: /* postfix_expr ::= atom */
      case 175: /* finally_opt ::= FINALLY stmts */
#line 698 "parser.y"
{
    yygotominor.yy7 = yymsp[0].minor.yy7;
}
#line 2179 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 704 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy7 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2187 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 708 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[-1].minor.yy7);
}
#line 2195 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 712 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = Finally_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2203 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 716 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = While_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2211 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 720 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Break_new(env, lineno, YNIL);
}
#line 2219 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 724 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Break_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2227 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 728 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Next_new(env, lineno, YNIL);
}
#line 2235 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 732 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Next_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2243 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 736 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Return_new(env, lineno, YNIL);
}
#line 2251 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 740 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Return_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2259 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 744 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy7 = If_new(env, lineno, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2267 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 748 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy7 = Klass_new(env, lineno, id, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2276 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 753 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Nonlocal_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2284 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 757 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Import_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2292 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 769 "parser.y"
{
    yygotominor.yy7 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2300 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 772 "parser.y"
{
    yygotominor.yy7 = Array_push_token_id(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy0);
}
#line 2308 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 793 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7, yymsp[0].minor.yy7);
    yygotominor.yy7 = make_array_with(env, node);
}
#line 2317 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 806 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy7 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2326 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 812 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-8].minor.yy7, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2333 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 815 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2340 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 818 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2347 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 821 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2354 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 824 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2361 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 827 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2368 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 830 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2375 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 833 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL, YNIL);
}
#line 2382 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 836 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2389 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 839 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2396 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 842 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2403 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 845 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2410 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 848 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2417 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 851 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2424 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 854 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, YNIL, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2431 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 857 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[0].minor.yy7, YNIL, YNIL, YNIL, YNIL);
}
#line 2438 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 860 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2445 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 863 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2452 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 866 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2459 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 869 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2466 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 872 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2473 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 875 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2480 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 878 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2487 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 881 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[0].minor.yy7, YNIL, YNIL, YNIL);
}
#line 2494 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 884 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2501 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 887 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2508 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 890 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2515 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 893 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2522 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 896 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2529 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 899 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2536 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 902 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2543 "parser.c"
        break;
      case 64: /* params ::= */
#line 905 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2550 "parser.c"
        break;
      case 65: /* kw_param ::= STAR_STAR NAME */
#line 909 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2559 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 915 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2568 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 921 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy7);
}
#line 2577 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 938 "parser.y"
{
    yygotominor.yy7 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy7, lineno, id, YNIL);
}
#line 2587 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 944 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy7, lineno, id, YNIL);
    yygotominor.yy7 = yymsp[-2].minor.yy7;
}
#line 2597 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 958 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy7);
}
#line 2606 "parser.c"
        break;
      case 77: /* args ::= posargs */
#line 967 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[0].minor.yy7, YNIL, YNIL, YNIL);
}
#line 2614 "parser.c"
        break;
      case 78: /* args ::= posargs COMMA kwargs */
#line 971 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2622 "parser.c"
        break;
      case 79: /* args ::= posargs COMMA kwargs COMMA vararg */
#line 975 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2630 "parser.c"
        break;
      case 80: /* args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg */
#line 979 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-6].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2638 "parser.c"
        break;
      case 81: /* args ::= posargs COMMA vararg */
#line 983 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2646 "parser.c"
        break;
      case 82: /* args ::= posargs COMMA vararg COMMA varkwarg */
#line 987 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2654 "parser.c"
        break;
      case 83: /* args ::= posargs COMMA varkwarg */
#line 991 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2662 "parser.c"
        break;
      case 84: /* args ::= kwargs */
#line 995 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, YNIL, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2670 "parser.c"
        break;
      case 85: /* args ::= kwargs COMMA vararg */
#line 999 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2678 "parser.c"
        break;
      case 86: /* args ::= kwargs COMMA vararg COMMA varkwarg */
#line 1003 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2686 "parser.c"
        break;
      case 87: /* args ::= kwargs COMMA varkwarg */
#line 1007 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2694 "parser.c"
        break;
      case 88: /* args ::= vararg */
#line 1011 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy7);
    yygotominor.yy7 = Args_new(env, lineno, YNIL, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2702 "parser.c"
        break;
      case 89: /* args ::= vararg COMMA varkwarg */
#line 1015 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    yygotominor.yy7 = Args_new(env, lineno, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2710 "parser.c"
        break;
      case 90: /* args ::= varkwarg */
#line 1019 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy7);
    yygotominor.yy7 = Args_new(env, lineno, YNIL, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2718 "parser.c"
        break;
      case 94: /* posargs ::= posargs COMMA expr */
      case 96: /* kwargs ::= kwargs COMMA kwarg */
      case 157: /* exprs ::= exprs COMMA expr */
      case 159: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 1035 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
    yygotominor.yy7 = yymsp[-2].minor.yy7;
}
#line 2729 "parser.c"
        break;
      case 97: /* kwarg ::= NAME COLON expr */
#line 1048 "parser.y"
{
    yygotominor.yy7 = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(yymsp[-2].minor.yy0));
    PTR_AS(YogNode, yygotominor.yy7)->u.kwarg.name = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    PTR_AS(YogNode, yygotominor.yy7)->u.kwarg.value = yymsp[0].minor.yy7;
}
#line 2738 "parser.c"
        break;
      case 99: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 1058 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    yygotominor.yy7 = Assign_new(env, lineno, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2746 "parser.c"
        break;
      case 102: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 1069 "parser.y"
{
    yygotominor.yy7 = YogNode_new(env, NODE_LOGICAL_OR, NODE_LINENO(yymsp[-2].minor.yy7));
    NODE(yygotominor.yy7)->u.logical_or.left = yymsp[-2].minor.yy7;
    NODE(yygotominor.yy7)->u.logical_or.right = yymsp[0].minor.yy7;
}
#line 2755 "parser.c"
        break;
      case 104: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 1078 "parser.y"
{
    yygotominor.yy7 = YogNode_new(env, NODE_LOGICAL_AND, NODE_LINENO(yymsp[-2].minor.yy7));
    NODE(yygotominor.yy7)->u.logical_and.left = yymsp[-2].minor.yy7;
    NODE(yygotominor.yy7)->u.logical_and.right = yymsp[0].minor.yy7;
}
#line 2764 "parser.c"
        break;
      case 106: /* not_expr ::= NOT not_expr */
#line 1087 "parser.y"
{
    yygotominor.yy7 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy7)->u.not.expr = yymsp[0].minor.yy7;
}
#line 2772 "parser.c"
        break;
      case 108: /* comparison ::= xor_expr comp_op xor_expr */
#line 1095 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy7)->u.id;
    yygotominor.yy7 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy7, id, yymsp[0].minor.yy7);
}
#line 2781 "parser.c"
        break;
      case 109: /* comp_op ::= LESS */
      case 110: /* comp_op ::= GREATER */
      case 163: /* comma_opt ::= COMMA */
#line 1101 "parser.y"
{
    yygotominor.yy7 = yymsp[0].minor.yy0;
}
#line 2790 "parser.c"
        break;
      case 112: /* xor_expr ::= xor_expr XOR or_expr */
      case 114: /* or_expr ::= or_expr BAR and_expr */
      case 116: /* and_expr ::= and_expr AND shift_expr */
#line 1111 "parser.y"
{
    yygotominor.yy7 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy7), yymsp[-2].minor.yy7, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy7);
}
#line 2799 "parser.c"
        break;
      case 118: /* shift_expr ::= shift_expr shift_op match_expr */
      case 124: /* arith_expr ::= arith_expr arith_op term */
      case 127: /* term ::= term term_op factor */
#line 1132 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    yygotominor.yy7 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy7, VAL2ID(yymsp[-1].minor.yy7), yymsp[0].minor.yy7);
}
#line 2809 "parser.c"
        break;
      case 119: /* shift_op ::= LSHIFT */
      case 120: /* shift_op ::= RSHIFT */
      case 125: /* arith_op ::= PLUS */
      case 126: /* arith_op ::= MINUS */
      case 129: /* term_op ::= STAR */
      case 130: /* term_op ::= DIV */
      case 131: /* term_op ::= DIV_DIV */
      case 132: /* term_op ::= PERCENT */
#line 1137 "parser.y"
{
    yygotominor.yy7 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2823 "parser.c"
        break;
      case 122: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1147 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy7 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy7, id, yymsp[0].minor.yy7);
}
#line 2832 "parser.c"
        break;
      case 133: /* factor ::= PLUS factor */
#line 1189 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy7 = FuncCall_new3(env, lineno, yymsp[0].minor.yy7, id);
}
#line 2841 "parser.c"
        break;
      case 134: /* factor ::= MINUS factor */
#line 1194 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy7 = FuncCall_new3(env, lineno, yymsp[0].minor.yy7, id);
}
#line 2850 "parser.c"
        break;
      case 135: /* factor ::= TILDA factor */
#line 1199 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy7 = FuncCall_new3(env, lineno, yymsp[0].minor.yy7, id);
}
#line 2859 "parser.c"
        break;
      case 139: /* postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt */
#line 1215 "parser.y"
{
    yygotominor.yy7 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy7), yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2866 "parser.c"
        break;
      case 140: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1218 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy7);
    yygotominor.yy7 = Subscript_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2874 "parser.c"
        break;
      case 141: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1222 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Attr_new(env, lineno, yymsp[-2].minor.yy7, id);
}
#line 2883 "parser.c"
        break;
      case 142: /* atom ::= NAME */
#line 1228 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Variable_new(env, lineno, id);
}
#line 2892 "parser.c"
        break;
      case 143: /* atom ::= NUMBER */
      case 144: /* atom ::= REGEXP */
      case 145: /* atom ::= STRING */
      case 146: /* atom ::= SYMBOL */
#line 1233 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy7 = Literal_new(env, lineno, val);
}
#line 2904 "parser.c"
        break;
      case 147: /* atom ::= NIL */
#line 1253 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Literal_new(env, lineno, YNIL);
}
#line 2912 "parser.c"
        break;
      case 148: /* atom ::= TRUE */
#line 1257 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Literal_new(env, lineno, YTRUE);
}
#line 2920 "parser.c"
        break;
      case 149: /* atom ::= FALSE */
#line 1261 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Literal_new(env, lineno, YFALSE);
}
#line 2928 "parser.c"
        break;
      case 150: /* atom ::= LINE */
#line 1265 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy7 = Literal_new(env, lineno, val);
}
#line 2937 "parser.c"
        break;
      case 151: /* atom ::= LBRACKET exprs RBRACKET */
#line 1270 "parser.y"
{
    yygotominor.yy7 = Array_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy7);
}
#line 2944 "parser.c"
        break;
      case 152: /* atom ::= LBRACKET RBRACKET */
#line 1273 "parser.y"
{
    yygotominor.yy7 = Array_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 2951 "parser.c"
        break;
      case 153: /* atom ::= LBRACE RBRACE */
#line 1276 "parser.y"
{
    yygotominor.yy7 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 2958 "parser.c"
        break;
      case 154: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1279 "parser.y"
{
    yygotominor.yy7 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy7);
}
#line 2965 "parser.c"
        break;
      case 155: /* atom ::= LPAR expr RPAR */
      case 168: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1282 "parser.y"
{
    yygotominor.yy7 = yymsp[-1].minor.yy7;
}
#line 2973 "parser.c"
        break;
      case 160: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1301 "parser.y"
{
    yygotominor.yy7 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy7), yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2980 "parser.c"
        break;
      case 161: /* dict_elem ::= NAME COLON expr */
#line 1304 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy7 = DictElem_new(env, lineno, var, yymsp[0].minor.yy7);
}
#line 2990 "parser.c"
        break;
      case 165: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 166: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1321 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = BlockArg_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2999 "parser.c"
        break;
      case 170: /* excepts ::= excepts except */
#line 1340 "parser.y"
{
    yygotominor.yy7 = Array_push(env, yymsp[-1].minor.yy7, yymsp[0].minor.yy7);
}
#line 3006 "parser.c"
        break;
      case 171: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1344 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy7 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy7, id, yymsp[0].minor.yy7);
}
#line 3016 "parser.c"
        break;
      case 172: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1350 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy7 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy7, NO_EXC_VAR, yymsp[0].minor.yy7);
}
#line 3024 "parser.c"
        break;
      case 173: /* except ::= EXCEPT NEWLINE stmts */
#line 1354 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy7 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy7);
}
#line 3032 "parser.c"
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
