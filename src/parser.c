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
    case NODE_MODULE:
        KEEP(module.stmts);
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
Module_new(YogEnv* env, uint_t lineno, ID name, YogVal stmts)
{
    SAVE_ARG(env, stmts);
    YogVal module = YUNDEF;
    PUSH_LOCAL(env, module);

    module = YogNode_new(env, NODE_MODULE, lineno);
    PTR_AS(YogNode, module)->u.module.name = name;
    PTR_AS(YogNode, module)->u.module.stmts = stmts;

    RETURN(env, module);
}

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
#line 705 "parser.c"
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
#define YYNOCODE 117
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
#define YYNSTATE 297
#define YYNRULE 177
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
 /*     0 */     3,  139,  244,  237,   24,   25,   33,   34,   35,  147,
 /*    10 */   216,  149,   88,   70,  103,  161,  165,  267,  156,   29,
 /*    20 */   271,  120,  129,   78,  130,   62,   79,   73,   43,  260,
 /*    30 */   214,  202,  215,  163,  164,  247,   55,   56,  171,  173,
 /*    40 */   281,   57,   21,  271,  217,  218,  219,  220,  221,  222,
 /*    50 */   223,  224,   20,  141,  123,  138,  140,  246,  242,  194,
 /*    60 */   102,  126,  127,  205,  196,   74,  114,  128,  129,   78,
 /*    70 */   130,   37,   79,   73,  296,   19,  214,  202,  215,   18,
 /*    80 */   475,  104,  190,  188,  189,  103,  117,  127,  205,  196,
 /*    90 */    74,   54,  128,  129,   78,  130,  180,   79,   73,   44,
 /*   100 */     1,  214,  202,  215,    4,   40,    5,  181,  194,  102,
 /*   110 */   126,  127,  205,  196,   74,  241,  128,  129,   78,  130,
 /*   120 */    19,   79,   73,   39,    5,  214,  202,  215,  160,  167,
 /*   130 */   169,  276,   23,   48,  277,  124,  136,  240,  242,  194,
 /*   140 */   102,  126,  127,  205,  196,   74,   52,  128,  129,   78,
 /*   150 */   130,  137,   79,   73,  100,  294,  214,  202,  215,   23,
 /*   160 */    29,   26,   26,   31,  135,  163,  164,  166,  135,   43,
 /*   170 */   248,   61,  190,  188,  189,   26,   31,   55,   56,  125,
 /*   180 */    31,  237,   57,   21,   95,  217,  218,  219,  220,  221,
 /*   190 */   222,  223,  224,   20,  203,  163,  164,  166,  194,  102,
 /*   200 */   126,  127,  205,  196,   74,   22,  128,  129,   78,  130,
 /*   210 */   180,   79,   73,  144,    1,  214,  202,  215,  253,   75,
 /*   220 */   190,  188,  189,   19,   27,   10,  103,  254,  118,  205,
 /*   230 */   196,   74,  253,  128,  129,   78,  130,   39,   79,   73,
 /*   240 */   133,   36,  214,  202,  215,  239,  194,  102,  126,  127,
 /*   250 */   205,  196,   74,  225,  128,  129,   78,  130,  292,   79,
 /*   260 */    73,  152,  155,  214,  202,  215,  115,  190,  188,  189,
 /*   270 */    16,  291,   45,  103,  162,  264,  195,  196,   74,  243,
 /*   280 */   128,  129,   78,  130,  245,   79,   73,  168,  274,  214,
 /*   290 */   202,  215,   50,  194,  102,  126,  127,  205,  196,   74,
 /*   300 */   142,  128,  129,   78,  130,   38,   79,   73,  172,  279,
 /*   310 */   214,  202,  215,  105,  190,  188,  189,  103,  268,  269,
 /*   320 */   197,  196,   74,  148,  128,  129,   78,  130,  182,   79,
 /*   330 */    73,  175,  283,  214,  202,  215,  101,  206,  207,   86,
 /*   340 */   194,  102,  126,  127,  205,  196,   74,  258,  128,  129,
 /*   350 */    78,  130,  153,   79,   73,  208,  209,  214,  202,  215,
 /*   360 */   107,  190,  188,  189,  103,  297,   19,  103,   19,  119,
 /*   370 */   150,  128,  129,   78,  130,   19,   79,   73,  192,  163,
 /*   380 */   214,  202,  215,  198,  202,  215,  262,  194,  102,  126,
 /*   390 */   127,  205,  196,   74,  266,  128,  129,   78,  130,   19,
 /*   400 */    79,   73,  249,  103,  214,  202,  215,   63,  190,  188,
 /*   410 */   189,  121,   78,  130,   19,   79,   73,  255,  272,  214,
 /*   420 */   202,  215,  288,  158,  159,  170,  174,  176,  285,   19,
 /*   430 */    19,  277,  256,  261,  194,  102,  126,  127,  205,  196,
 /*   440 */    74,  273,  128,  129,   78,  130,  178,   79,   73,  103,
 /*   450 */   275,  214,  202,  215,   64,  190,  188,  189,   76,  130,
 /*   460 */   103,   79,   73,  278,  280,  214,  202,  215,   19,  282,
 /*   470 */   122,  295,   79,   73,  284,  191,  214,  202,  215,   19,
 /*   480 */     6,  194,  102,  126,  127,  205,  196,   74,   41,  128,
 /*   490 */   129,   78,  130,   42,   79,   73,   46,   45,  214,  202,
 /*   500 */   215,  146,  190,  188,  189,   92,  177,  159,  170,  174,
 /*   510 */   176,  285,   51,    2,  277,   47,  163,  164,  166,   65,
 /*   520 */    81,   28,   30,  226,  229,   32,   83,   60,  194,  102,
 /*   530 */   126,  127,  205,  196,   74,   84,  128,  129,   78,  130,
 /*   540 */   103,   79,   73,  103,   85,  214,  202,  215,  108,  190,
 /*   550 */   188,  189,   77,   73,  210,    7,  214,  202,  215,  199,
 /*   560 */   202,  215,   80,    8,  252,    9,   11,   87,   12,  151,
 /*   570 */   211,  212,  213,  257,   89,  194,  102,  126,  127,  205,
 /*   580 */   196,   74,   53,  128,  129,   78,  130,  154,   79,   73,
 /*   590 */   103,  259,  214,  202,  215,  109,  190,  188,  189,  103,
 /*   600 */   157,   58,  103,   72,   13,   49,  214,  202,  215,   66,
 /*   610 */    90,  263,  265,   91,   71,  200,  202,  215,  201,  202,
 /*   620 */   215,   67,  194,  102,  126,  127,  205,  196,   74,   93,
 /*   630 */   128,  129,   78,  130,   94,   79,   73,   59,   68,  214,
 /*   640 */   202,  215,  110,  190,  188,  189,   96,   97,   69,   98,
 /*   650 */    99,  287,   14,  289,  290,  183,  293,   15,  476,  476,
 /*   660 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  194,
 /*   670 */   102,  126,  127,  205,  196,   74,  476,  128,  129,   78,
 /*   680 */   130,  476,   79,   73,  476,  476,  214,  202,  215,  111,
 /*   690 */   190,  188,  189,  476,  476,  476,  476,  476,  476,  476,
 /*   700 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   710 */   476,  476,  476,  476,  476,  476,  194,  102,  126,  127,
 /*   720 */   205,  196,   74,  476,  128,  129,   78,  130,  476,   79,
 /*   730 */    73,  476,  476,  214,  202,  215,  184,  190,  188,  189,
 /*   740 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   750 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   760 */   476,  476,  476,  194,  102,  126,  127,  205,  196,   74,
 /*   770 */   476,  128,  129,   78,  130,  476,   79,   73,  476,  476,
 /*   780 */   214,  202,  215,  185,  190,  188,  189,  476,  476,  476,
 /*   790 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   800 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   810 */   194,  102,  126,  127,  205,  196,   74,  476,  128,  129,
 /*   820 */    78,  130,  476,   79,   73,  476,  476,  214,  202,  215,
 /*   830 */   186,  190,  188,  189,  476,  476,  476,  476,  476,  476,
 /*   840 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   850 */   476,  476,  476,  476,  476,  476,  476,  194,  102,  126,
 /*   860 */   127,  205,  196,   74,  476,  128,  129,   78,  130,  476,
 /*   870 */    79,   73,  476,  476,  214,  202,  215,  113,  190,  188,
 /*   880 */   189,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   890 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   900 */   476,  476,  476,  476,  194,  102,  126,  127,  205,  196,
 /*   910 */    74,  476,  128,  129,   78,  130,  476,   79,   73,  476,
 /*   920 */   476,  214,  202,  215,  187,  188,  189,  476,  476,  476,
 /*   930 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   940 */   476,  476,  476,  476,  476,  476,  134,  476,  476,  476,
 /*   950 */   194,  102,  126,  127,  205,  196,   74,  476,  128,  129,
 /*   960 */    78,  130,  476,   79,   73,  476,  476,  214,  202,  215,
 /*   970 */   194,  102,  126,  127,  205,  196,   74,  476,  128,  129,
 /*   980 */    78,  130,  476,   79,   73,  476,  476,  214,  202,  215,
 /*   990 */   235,  476,   82,  476,  232,  476,  476,  476,  476,  476,
 /*  1000 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*  1010 */   476,  476,  134,  476,  194,  102,  126,  127,  205,  196,
 /*  1020 */    74,  476,  128,  129,   78,  130,  476,   79,   73,  476,
 /*  1030 */   476,  214,  202,  215,  476,  106,  194,  102,  126,  127,
 /*  1040 */   205,  196,   74,  476,  128,  129,   78,  130,  131,   79,
 /*  1050 */    73,  476,  476,  214,  202,  215,  476,   29,  476,  476,
 /*  1060 */   230,  476,  476,  476,  476,  476,   43,  476,  476,  476,
 /*  1070 */   476,  476,  476,  476,   55,   56,  476,  476,  476,   57,
 /*  1080 */    21,  476,  217,  218,  219,  220,  221,  222,  223,  224,
 /*  1090 */    20,  228,  216,  476,  476,  476,  476,  476,  476,  476,
 /*  1100 */   476,   29,  476,  476,  476,  476,  476,  476,  476,  476,
 /*  1110 */    43,  476,  476,  476,  476,  112,  476,  476,   55,   56,
 /*  1120 */   476,  476,  476,   57,   21,  234,  217,  218,  219,  220,
 /*  1130 */   221,  222,  223,  224,   20,   17,  476,  476,  476,  194,
 /*  1140 */   102,  126,  127,  205,  196,   74,  216,  128,  129,   78,
 /*  1150 */   130,  476,   79,   73,  476,   29,  214,  202,  215,  476,
 /*  1160 */   476,  476,  476,  476,   43,  476,  476,  116,  476,  476,
 /*  1170 */   476,  476,   55,   56,  476,  476,  476,   57,   21,  476,
 /*  1180 */   217,  218,  219,  220,  221,  222,  223,  224,   20,  193,
 /*  1190 */   476,  194,  102,  126,  127,  205,  196,   74,  476,  128,
 /*  1200 */   129,   78,  130,  476,   79,   73,  476,  476,  214,  202,
 /*  1210 */   215,  204,  476,  194,  102,  126,  127,  205,  196,   74,
 /*  1220 */   476,  128,  129,   78,  130,  476,   79,   73,  476,  476,
 /*  1230 */   214,  202,  215,  233,  476,  194,  102,  126,  127,  205,
 /*  1240 */   196,   74,  476,  128,  129,   78,  130,  476,   79,   73,
 /*  1250 */   476,  476,  214,  202,  215,  227,  476,  194,  102,  126,
 /*  1260 */   127,  205,  196,   74,  476,  128,  129,   78,  130,  476,
 /*  1270 */    79,   73,  476,  476,  214,  202,  215,  132,  476,  194,
 /*  1280 */   102,  126,  127,  205,  196,   74,  476,  128,  129,   78,
 /*  1290 */   130,  476,   79,   73,  476,  476,  214,  202,  215,  231,
 /*  1300 */   476,  194,  102,  126,  127,  205,  196,   74,  476,  128,
 /*  1310 */   129,   78,  130,  476,   79,   73,  476,  476,  214,  202,
 /*  1320 */   215,  236,  476,  194,  102,  126,  127,  205,  196,   74,
 /*  1330 */   476,  128,  129,   78,  130,  476,   79,   73,  476,  476,
 /*  1340 */   214,  202,  215,  238,  476,  194,  102,  126,  127,  205,
 /*  1350 */   196,   74,  476,  128,  129,   78,  130,  476,   79,   73,
 /*  1360 */   476,  476,  214,  202,  215,  250,  476,  194,  102,  126,
 /*  1370 */   127,  205,  196,   74,  476,  128,  129,   78,  130,  476,
 /*  1380 */    79,   73,  476,  476,  214,  202,  215,  251,  476,  194,
 /*  1390 */   102,  126,  127,  205,  196,   74,  476,  128,  129,   78,
 /*  1400 */   130,  476,   79,   73,  476,  476,  214,  202,  215,  143,
 /*  1410 */   476,  194,  102,  126,  127,  205,  196,   74,  476,  128,
 /*  1420 */   129,   78,  130,  476,   79,   73,  476,  476,  214,  202,
 /*  1430 */   215,  145,  476,  194,  102,  126,  127,  205,  196,   74,
 /*  1440 */   476,  128,  129,   78,  130,  476,   79,   73,  476,  476,
 /*  1450 */   214,  202,  215,  270,  476,  194,  102,  126,  127,  205,
 /*  1460 */   196,   74,  476,  128,  129,   78,  130,  476,   79,   73,
 /*  1470 */   476,  476,  214,  202,  215,  286,  476,  194,  102,  126,
 /*  1480 */   127,  205,  196,   74,  476,  128,  129,   78,  130,  476,
 /*  1490 */    79,   73,  476,  476,  214,  202,  215,  179,  476,  194,
 /*  1500 */   102,  126,  127,  205,  196,   74,  476,  128,  129,   78,
 /*  1510 */   130,  476,   79,   73,  476,  476,  214,  202,  215,  476,
 /*  1520 */   476,  194,  102,  126,  127,  205,  196,   74,  476,  128,
 /*  1530 */   129,   78,  130,  476,   79,   73,  131,  476,  214,  202,
 /*  1540 */   215,  476,  476,  476,  476,   29,  476,  476,  476,  476,
 /*  1550 */   476,  476,  476,  476,   43,  476,  476,  476,  476,  476,
 /*  1560 */   476,  476,   55,   56,  476,  476,  476,   57,   21,  476,
 /*  1570 */   217,  218,  219,  220,  221,  222,  223,  224,   20,  476,
 /*  1580 */   216,  476,  476,  476,  476,  476,  476,  476,  476,   29,
 /*  1590 */   476,  476,  476,  476,  476,  476,  476,  476,   43,  476,
 /*  1600 */   476,  476,  476,  476,  476,  476,   55,   56,  476,  476,
 /*  1610 */   476,   57,   21,  476,  217,  218,  219,  220,  221,  222,
 /*  1620 */   223,  224,   20,  476,  216,  476,  476,  476,  476,  476,
 /*  1630 */   476,  476,  476,   29,  476,  476,  476,  476,  476,  476,
 /*  1640 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*  1650 */    55,   56,  476,  476,  476,   57,   21,  476,  217,  218,
 /*  1660 */   219,  220,  221,  222,  223,  224,   20,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   86,   87,   88,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   15,   90,   77,   78,   79,   20,   21,
 /*    20 */    82,   97,   98,   99,  100,   66,  102,  103,   30,   12,
 /*    30 */   106,  107,  108,   23,   24,   65,   38,   39,   77,   78,
 /*    40 */    79,   43,   44,   82,   46,   47,   48,   49,   50,   51,
 /*    50 */    52,   53,   54,   83,   84,   85,   86,   87,   88,   89,
 /*    60 */    90,   91,   92,   93,   94,   95,   67,   97,   98,   99,
 /*    70 */   100,   26,  102,  103,  115,    1,  106,  107,  108,    5,
 /*    80 */    61,   62,   63,   64,   65,   90,   91,   92,   93,   94,
 /*    90 */    95,  105,   97,   98,   99,  100,   17,  102,  103,   96,
 /*   100 */    21,  106,  107,  108,    3,   26,    5,   67,   89,   90,
 /*   110 */    91,   92,   93,   94,   95,   65,   97,   98,   99,  100,
 /*   120 */     1,  102,  103,   44,    5,  106,  107,  108,   76,   77,
 /*   130 */    78,   79,   58,  101,   82,   85,   86,   87,   88,   89,
 /*   140 */    90,   91,   92,   93,   94,   95,  104,   97,   98,   99,
 /*   150 */   100,   12,  102,  103,   12,  115,  106,  107,  108,   58,
 /*   160 */    21,   23,   23,   24,   12,   23,   24,   25,   12,   30,
 /*   170 */   109,   62,   63,   64,   65,   23,   24,   38,   39,   86,
 /*   180 */    24,   88,   43,   44,   12,   46,   47,   48,   49,   50,
 /*   190 */    51,   52,   53,   54,   87,   23,   24,   25,   89,   90,
 /*   200 */    91,   92,   93,   94,   95,   16,   97,   98,   99,  100,
 /*   210 */    17,  102,  103,   68,   21,  106,  107,  108,   73,   62,
 /*   220 */    63,   64,   65,    1,   16,    3,   90,   68,   92,   93,
 /*   230 */    94,   95,   73,   97,   98,   99,  100,   44,  102,  103,
 /*   240 */   112,   19,  106,  107,  108,   87,   89,   90,   91,   92,
 /*   250 */    93,   94,   95,   45,   97,   98,   99,  100,   18,  102,
 /*   260 */   103,   71,   72,  106,  107,  108,   62,   63,   64,   65,
 /*   270 */     1,   31,   32,   90,   78,   79,   93,   94,   95,   87,
 /*   280 */    97,   98,   99,  100,   87,  102,  103,   78,   79,  106,
 /*   290 */   107,  108,   44,   89,   90,   91,   92,   93,   94,   95,
 /*   300 */   114,   97,   98,   99,  100,   18,  102,  103,   78,   79,
 /*   310 */   106,  107,  108,   62,   63,   64,   65,   90,   80,   81,
 /*   320 */    93,   94,   95,   69,   97,   98,   99,  100,   59,  102,
 /*   330 */   103,   78,   79,  106,  107,  108,   54,   35,   36,   57,
 /*   340 */    89,   90,   91,   92,   93,   94,   95,   12,   97,   98,
 /*   350 */    99,  100,   72,  102,  103,   38,   39,  106,  107,  108,
 /*   360 */    62,   63,   64,   65,   90,    0,    1,   90,    1,   95,
 /*   370 */    70,   97,   98,   99,  100,    1,  102,  103,    4,   23,
 /*   380 */   106,  107,  108,  106,  107,  108,   79,   89,   90,   91,
 /*   390 */    92,   93,   94,   95,   79,   97,   98,   99,  100,    1,
 /*   400 */   102,  103,    4,   90,  106,  107,  108,   62,   63,   64,
 /*   410 */    65,   98,   99,  100,    1,  102,  103,    4,   81,  106,
 /*   420 */   107,  108,   55,   74,   75,   76,   77,   78,   79,    1,
 /*   430 */     1,   82,    4,    4,   89,   90,   91,   92,   93,   94,
 /*   440 */    95,   79,   97,   98,   99,  100,  114,  102,  103,   90,
 /*   450 */    79,  106,  107,  108,   62,   63,   64,   65,   99,  100,
 /*   460 */    90,  102,  103,   79,   79,  106,  107,  108,    1,   79,
 /*   470 */   100,    4,  102,  103,   79,    4,  106,  107,  108,    1,
 /*   480 */     1,   89,   90,   91,   92,   93,   94,   95,   28,   97,
 /*   490 */    98,   99,  100,   29,  102,  103,   33,   32,  106,  107,
 /*   500 */   108,   62,   63,   64,   65,   12,   74,   75,   76,   77,
 /*   510 */    78,   79,   37,   16,   82,   34,   23,   24,   25,   16,
 /*   520 */    16,   27,   56,   22,   55,   27,   16,   16,   89,   90,
 /*   530 */    91,   92,   93,   94,   95,   16,   97,   98,   99,  100,
 /*   540 */    90,  102,  103,   90,   16,  106,  107,  108,   62,   63,
 /*   550 */    64,   65,  102,  103,   24,    1,  106,  107,  108,  106,
 /*   560 */   107,  108,   22,    1,    4,    1,    1,   12,   12,   16,
 /*   570 */    40,   41,   42,   12,   16,   89,   90,   91,   92,   93,
 /*   580 */    94,   95,   16,   97,   98,   99,  100,   17,  102,  103,
 /*   590 */    90,   12,  106,  107,  108,   62,   63,   64,   65,   90,
 /*   600 */    12,   16,   90,  103,   22,   21,  106,  107,  108,   16,
 /*   610 */    16,   12,   12,   16,   12,  106,  107,  108,  106,  107,
 /*   620 */   108,   16,   89,   90,   91,   92,   93,   94,   95,   16,
 /*   630 */    97,   98,   99,  100,   16,  102,  103,   16,   16,  106,
 /*   640 */   107,  108,   62,   63,   64,   65,   16,   16,   16,   16,
 /*   650 */    16,   45,    1,   45,   12,   12,    4,    1,  116,  116,
 /*   660 */   116,  116,  116,  116,  116,  116,  116,  116,  116,   89,
 /*   670 */    90,   91,   92,   93,   94,   95,  116,   97,   98,   99,
 /*   680 */   100,  116,  102,  103,  116,  116,  106,  107,  108,   62,
 /*   690 */    63,   64,   65,  116,  116,  116,  116,  116,  116,  116,
 /*   700 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*   710 */   116,  116,  116,  116,  116,  116,   89,   90,   91,   92,
 /*   720 */    93,   94,   95,  116,   97,   98,   99,  100,  116,  102,
 /*   730 */   103,  116,  116,  106,  107,  108,   62,   63,   64,   65,
 /*   740 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*   750 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*   760 */   116,  116,  116,   89,   90,   91,   92,   93,   94,   95,
 /*   770 */   116,   97,   98,   99,  100,  116,  102,  103,  116,  116,
 /*   780 */   106,  107,  108,   62,   63,   64,   65,  116,  116,  116,
 /*   790 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*   800 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*   810 */    89,   90,   91,   92,   93,   94,   95,  116,   97,   98,
 /*   820 */    99,  100,  116,  102,  103,  116,  116,  106,  107,  108,
 /*   830 */    62,   63,   64,   65,  116,  116,  116,  116,  116,  116,
 /*   840 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*   850 */   116,  116,  116,  116,  116,  116,  116,   89,   90,   91,
 /*   860 */    92,   93,   94,   95,  116,   97,   98,   99,  100,  116,
 /*   870 */   102,  103,  116,  116,  106,  107,  108,   62,   63,   64,
 /*   880 */    65,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*   890 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*   900 */   116,  116,  116,  116,   89,   90,   91,   92,   93,   94,
 /*   910 */    95,  116,   97,   98,   99,  100,  116,  102,  103,  116,
 /*   920 */   116,  106,  107,  108,   63,   64,   65,  116,  116,  116,
 /*   930 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*   940 */   116,  116,  116,  116,  116,  116,   65,  116,  116,  116,
 /*   950 */    89,   90,   91,   92,   93,   94,   95,  116,   97,   98,
 /*   960 */    99,  100,  116,  102,  103,  116,  116,  106,  107,  108,
 /*   970 */    89,   90,   91,   92,   93,   94,   95,  116,   97,   98,
 /*   980 */    99,  100,  116,  102,  103,  116,  116,  106,  107,  108,
 /*   990 */    65,  116,  111,  116,  113,  116,  116,  116,  116,  116,
 /*  1000 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*  1010 */   116,  116,   65,  116,   89,   90,   91,   92,   93,   94,
 /*  1020 */    95,  116,   97,   98,   99,  100,  116,  102,  103,  116,
 /*  1030 */   116,  106,  107,  108,  116,  110,   89,   90,   91,   92,
 /*  1040 */    93,   94,   95,  116,   97,   98,   99,  100,   12,  102,
 /*  1050 */   103,  116,  116,  106,  107,  108,  116,   21,  116,  116,
 /*  1060 */   113,  116,  116,  116,  116,  116,   30,  116,  116,  116,
 /*  1070 */   116,  116,  116,  116,   38,   39,  116,  116,  116,   43,
 /*  1080 */    44,  116,   46,   47,   48,   49,   50,   51,   52,   53,
 /*  1090 */    54,   55,   12,  116,  116,  116,  116,  116,  116,  116,
 /*  1100 */   116,   21,  116,  116,  116,  116,  116,  116,  116,  116,
 /*  1110 */    30,  116,  116,  116,  116,   65,  116,  116,   38,   39,
 /*  1120 */   116,  116,  116,   43,   44,   45,   46,   47,   48,   49,
 /*  1130 */    50,   51,   52,   53,   54,    1,  116,  116,  116,   89,
 /*  1140 */    90,   91,   92,   93,   94,   95,   12,   97,   98,   99,
 /*  1150 */   100,  116,  102,  103,  116,   21,  106,  107,  108,  116,
 /*  1160 */   116,  116,  116,  116,   30,  116,  116,   65,  116,  116,
 /*  1170 */   116,  116,   38,   39,  116,  116,  116,   43,   44,  116,
 /*  1180 */    46,   47,   48,   49,   50,   51,   52,   53,   54,   65,
 /*  1190 */   116,   89,   90,   91,   92,   93,   94,   95,  116,   97,
 /*  1200 */    98,   99,  100,  116,  102,  103,  116,  116,  106,  107,
 /*  1210 */   108,   65,  116,   89,   90,   91,   92,   93,   94,   95,
 /*  1220 */   116,   97,   98,   99,  100,  116,  102,  103,  116,  116,
 /*  1230 */   106,  107,  108,   65,  116,   89,   90,   91,   92,   93,
 /*  1240 */    94,   95,  116,   97,   98,   99,  100,  116,  102,  103,
 /*  1250 */   116,  116,  106,  107,  108,   65,  116,   89,   90,   91,
 /*  1260 */    92,   93,   94,   95,  116,   97,   98,   99,  100,  116,
 /*  1270 */   102,  103,  116,  116,  106,  107,  108,   65,  116,   89,
 /*  1280 */    90,   91,   92,   93,   94,   95,  116,   97,   98,   99,
 /*  1290 */   100,  116,  102,  103,  116,  116,  106,  107,  108,   65,
 /*  1300 */   116,   89,   90,   91,   92,   93,   94,   95,  116,   97,
 /*  1310 */    98,   99,  100,  116,  102,  103,  116,  116,  106,  107,
 /*  1320 */   108,   65,  116,   89,   90,   91,   92,   93,   94,   95,
 /*  1330 */   116,   97,   98,   99,  100,  116,  102,  103,  116,  116,
 /*  1340 */   106,  107,  108,   65,  116,   89,   90,   91,   92,   93,
 /*  1350 */    94,   95,  116,   97,   98,   99,  100,  116,  102,  103,
 /*  1360 */   116,  116,  106,  107,  108,   65,  116,   89,   90,   91,
 /*  1370 */    92,   93,   94,   95,  116,   97,   98,   99,  100,  116,
 /*  1380 */   102,  103,  116,  116,  106,  107,  108,   65,  116,   89,
 /*  1390 */    90,   91,   92,   93,   94,   95,  116,   97,   98,   99,
 /*  1400 */   100,  116,  102,  103,  116,  116,  106,  107,  108,   65,
 /*  1410 */   116,   89,   90,   91,   92,   93,   94,   95,  116,   97,
 /*  1420 */    98,   99,  100,  116,  102,  103,  116,  116,  106,  107,
 /*  1430 */   108,   65,  116,   89,   90,   91,   92,   93,   94,   95,
 /*  1440 */   116,   97,   98,   99,  100,  116,  102,  103,  116,  116,
 /*  1450 */   106,  107,  108,   65,  116,   89,   90,   91,   92,   93,
 /*  1460 */    94,   95,  116,   97,   98,   99,  100,  116,  102,  103,
 /*  1470 */   116,  116,  106,  107,  108,   65,  116,   89,   90,   91,
 /*  1480 */    92,   93,   94,   95,  116,   97,   98,   99,  100,  116,
 /*  1490 */   102,  103,  116,  116,  106,  107,  108,   65,  116,   89,
 /*  1500 */    90,   91,   92,   93,   94,   95,  116,   97,   98,   99,
 /*  1510 */   100,  116,  102,  103,  116,  116,  106,  107,  108,  116,
 /*  1520 */   116,   89,   90,   91,   92,   93,   94,   95,  116,   97,
 /*  1530 */    98,   99,  100,  116,  102,  103,   12,  116,  106,  107,
 /*  1540 */   108,  116,  116,  116,  116,   21,  116,  116,  116,  116,
 /*  1550 */   116,  116,  116,  116,   30,  116,  116,  116,  116,  116,
 /*  1560 */   116,  116,   38,   39,  116,  116,  116,   43,   44,  116,
 /*  1570 */    46,   47,   48,   49,   50,   51,   52,   53,   54,  116,
 /*  1580 */    12,  116,  116,  116,  116,  116,  116,  116,  116,   21,
 /*  1590 */   116,  116,  116,  116,  116,  116,  116,  116,   30,  116,
 /*  1600 */   116,  116,  116,  116,  116,  116,   38,   39,  116,  116,
 /*  1610 */   116,   43,   44,  116,   46,   47,   48,   49,   50,   51,
 /*  1620 */    52,   53,   54,  116,   12,  116,  116,  116,  116,  116,
 /*  1630 */   116,  116,  116,   21,  116,  116,  116,  116,  116,  116,
 /*  1640 */   116,  116,  116,  116,  116,  116,  116,  116,  116,  116,
 /*  1650 */    38,   39,  116,  116,  116,   43,   44,  116,   46,   47,
 /*  1660 */    48,   49,   50,   51,   52,   53,   54,
};
#define YY_SHIFT_USE_DFLT (-3)
#define YY_SHIFT_MAX 186
static const short yy_shift_ofst[] = {
 /*     0 */    -2,  139,  139,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    20 */  1036, 1080, 1524, 1134, 1568, 1568, 1568, 1568, 1568, 1568,
 /*    30 */  1568, 1568, 1568, 1568, 1568, 1568, 1568, 1568, 1568, 1568,
 /*    40 */  1568, 1568, 1568, 1568, 1612, 1612, 1612, 1612, 1612,  142,
 /*    50 */   142, 1612, 1612,  172, 1612, 1612, 1612, 1612,  493,  493,
 /*    60 */   152,   74,  101,  222,  222,  156,   10,   10,   10,   10,
 /*    70 */    17,   45,  530,  530,  240,  119,  302,  317,  302,  317,
 /*    80 */   282,  138,  189,  138,  138,  138,  248,  287,  335,   17,
 /*    90 */   356,  356,   45,  356,  356,   45,  356,  356,  356,  356,
 /*   100 */    45,  248,   79,  193,  365,  374,  208,  398,  413,  428,
 /*   110 */   429,  367,  269,  467,  471,  478,  479,  460,  464,  465,
 /*   120 */   463,  481,  475,  497,  503,  504,  460,  464,  463,  481,
 /*   130 */   475,  494,  501,  469,  466,  498,  510,  498,  511,  519,
 /*   140 */   528,  540,  554,  562,  560,  564,  478,  555,  565,  556,
 /*   150 */   553,  561,  558,  570,  579,  570,  588,  584,  582,  566,
 /*   160 */   585,  593,  594,  599,  600,  597,  602,  605,  613,  618,
 /*   170 */   621,  622,  630,  631,  632,  633,  634,  606,  651,  608,
 /*   180 */   642,  652,  643,  656,  478,  478,  478,
};
#define YY_REDUCE_USE_DFLT (-86)
#define YY_REDUCE_MAX 101
static const short yy_reduce_ofst[] = {
 /*     0 */    19,  -30,   50,  109,  157,  204,  251,  298,  345,  392,
 /*    10 */   439,  486,  533,  580,  627,  674,  721,  768,  815,  861,
 /*    20 */   881,  925,  947, 1050, 1102, 1124, 1146, 1168, 1190, 1212,
 /*    30 */  1234, 1256, 1278, 1300, 1322, 1344, 1366, 1388, 1410, 1432,
 /*    40 */    -5,  136,  183,  227,  274,  -76,  313,  359,  370,  349,
 /*    50 */   432,  450,  500,   52,  277,  453,  509,  512,  -62,  -39,
 /*    60 */   -85,  -41,   40,  145,  159,   93,  196,  209,  230,  253,
 /*    70 */   190,  238,  -14,  -14,    3,   -1,   32,   42,   32,   42,
 /*    80 */    61,  107,  128,  158,  192,  197,  186,  254,  300,  280,
 /*    90 */   307,  315,  337,  362,  371,  337,  384,  385,  390,  395,
 /*   100 */   337,  332,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   300,  374,  474,  300,  300,  300,  300,  300,  300,  300,
 /*    10 */   300,  300,  300,  300,  300,  300,  300,  300,  300,  300,
 /*    20 */   474,  474,  461,  474,  474,  307,  474,  474,  474,  474,
 /*    30 */   474,  474,  474,  309,  311,  474,  474,  474,  474,  474,
 /*    40 */   474,  474,  474,  474,  474,  474,  474,  474,  474,  362,
 /*    50 */   362,  474,  474,  474,  474,  474,  474,  474,  474,  474,
 /*    60 */   474,  474,  472,  328,  328,  474,  474,  474,  474,  474,
 /*    70 */   474,  366,  422,  421,  405,  472,  414,  420,  413,  419,
 /*    80 */   462,  474,  460,  474,  474,  474,  465,  324,  474,  474,
 /*    90 */   474,  474,  474,  474,  474,  370,  474,  474,  474,  474,
 /*   100 */   369,  465,  435,  435,  474,  474,  474,  474,  474,  474,
 /*   110 */   474,  474,  474,  474,  474,  473,  474,  397,  400,  406,
 /*   120 */   410,  412,  416,  375,  376,  377,  398,  399,  409,  411,
 /*   130 */   415,  440,  474,  474,  474,  474,  379,  440,  382,  383,
 /*   140 */   386,  474,  474,  474,  474,  474,  329,  474,  474,  474,
 /*   150 */   316,  474,  317,  319,  474,  318,  474,  474,  474,  346,
 /*   160 */   338,  334,  332,  474,  474,  336,  474,  342,  340,  344,
 /*   170 */   354,  350,  348,  352,  358,  356,  360,  474,  474,  474,
 /*   180 */   474,  474,  474,  474,  469,  470,  471,  299,  301,  302,
 /*   190 */   298,  303,  306,  308,  396,  402,  403,  404,  425,  431,
 /*   200 */   432,  433,  434,  378,  389,  401,  417,  418,  423,  424,
 /*   210 */   427,  428,  429,  430,  426,  436,  440,  441,  442,  443,
 /*   220 */   444,  445,  446,  447,  448,  449,  453,  459,  451,  452,
 /*   230 */   457,  458,  456,  455,  450,  454,  390,  394,  395,  380,
 /*   240 */   381,  392,  393,  384,  385,  387,  388,  391,  437,  463,
 /*   250 */   310,  312,  313,  326,  327,  314,  315,  323,  322,  321,
 /*   260 */   320,  330,  331,  363,  333,  364,  335,  337,  365,  367,
 /*   270 */   368,  372,  373,  339,  341,  343,  345,  371,  347,  349,
 /*   280 */   351,  353,  355,  357,  359,  361,  325,  466,  464,  438,
 /*   290 */   439,  407,  408,  304,  468,  305,  467,
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
  "NAME",          "MODULE",        "NONLOCAL",      "IMPORT",      
  "COMMA",         "DOT",           "GREATER",       "ELIF",        
  "DEF",           "LPAR",          "RPAR",          "STAR_STAR",   
  "STAR",          "AMPER",         "EQUAL",         "COLON",       
  "BAR_BAR",       "AND_AND",       "NOT",           "LESS",        
  "XOR",           "BAR",           "AND",           "LSHIFT",      
  "RSHIFT",        "EQUAL_TILDA",   "PLUS",          "MINUS",       
  "DIV",           "DIV_DIV",       "PERCENT",       "TILDA",       
  "LBRACKET",      "RBRACKET",      "NUMBER",        "REGEXP",      
  "STRING",        "SYMBOL",        "NIL",           "TRUE",        
  "FALSE",         "LINE",          "LBRACE",        "RBRACE",      
  "EQUAL_GREATER",  "DO",            "EXCEPT",        "AS",          
  "error",         "module",        "stmts",         "stmt",        
  "func_def",      "expr",          "excepts",       "finally_opt", 
  "if_tail",       "super_opt",     "names",         "dotted_names",
  "dotted_name",   "else_opt",      "params",        "params_without_default",
  "params_with_default",  "block_param",   "var_param",     "kw_param",    
  "param_default_opt",  "param_default",  "param_with_default",  "args",        
  "posargs",       "kwargs",        "vararg",        "varkwarg",    
  "kwarg",         "assign_expr",   "postfix_expr",  "logical_or_expr",
  "logical_and_expr",  "not_expr",      "comparison",    "xor_expr",    
  "comp_op",       "or_expr",       "and_expr",      "shift_expr",  
  "match_expr",    "shift_op",      "arith_expr",    "term",        
  "arith_op",      "term_op",       "factor",        "power",       
  "atom",          "blockarg_opt",  "exprs",         "dict_elems",  
  "comma_opt",     "dict_elem",     "blockarg_params_opt",  "except",      
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
 /*  18 */ "stmt ::= MODULE NAME stmts END",
 /*  19 */ "stmt ::= NONLOCAL names",
 /*  20 */ "stmt ::= IMPORT dotted_names",
 /*  21 */ "dotted_names ::= dotted_name",
 /*  22 */ "dotted_names ::= dotted_names COMMA dotted_name",
 /*  23 */ "dotted_name ::= NAME",
 /*  24 */ "dotted_name ::= dotted_name DOT NAME",
 /*  25 */ "names ::= NAME",
 /*  26 */ "names ::= names COMMA NAME",
 /*  27 */ "super_opt ::=",
 /*  28 */ "super_opt ::= GREATER expr",
 /*  29 */ "if_tail ::= else_opt",
 /*  30 */ "if_tail ::= ELIF expr NEWLINE stmts if_tail",
 /*  31 */ "else_opt ::=",
 /*  32 */ "else_opt ::= ELSE stmts",
 /*  33 */ "func_def ::= DEF NAME LPAR params RPAR stmts END",
 /*  34 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  35 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param",
 /*  36 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param",
 /*  37 */ "params ::= params_without_default COMMA params_with_default COMMA block_param",
 /*  38 */ "params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param",
 /*  39 */ "params ::= params_without_default COMMA params_with_default COMMA var_param",
 /*  40 */ "params ::= params_without_default COMMA params_with_default COMMA kw_param",
 /*  41 */ "params ::= params_without_default COMMA params_with_default",
 /*  42 */ "params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  43 */ "params ::= params_without_default COMMA block_param COMMA var_param",
 /*  44 */ "params ::= params_without_default COMMA block_param COMMA kw_param",
 /*  45 */ "params ::= params_without_default COMMA block_param",
 /*  46 */ "params ::= params_without_default COMMA var_param COMMA kw_param",
 /*  47 */ "params ::= params_without_default COMMA var_param",
 /*  48 */ "params ::= params_without_default COMMA kw_param",
 /*  49 */ "params ::= params_without_default",
 /*  50 */ "params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  51 */ "params ::= params_with_default COMMA block_param COMMA var_param",
 /*  52 */ "params ::= params_with_default COMMA block_param COMMA kw_param",
 /*  53 */ "params ::= params_with_default COMMA block_param",
 /*  54 */ "params ::= params_with_default COMMA var_param COMMA kw_param",
 /*  55 */ "params ::= params_with_default COMMA var_param",
 /*  56 */ "params ::= params_with_default COMMA kw_param",
 /*  57 */ "params ::= params_with_default",
 /*  58 */ "params ::= block_param COMMA var_param COMMA kw_param",
 /*  59 */ "params ::= block_param COMMA var_param",
 /*  60 */ "params ::= block_param COMMA kw_param",
 /*  61 */ "params ::= block_param",
 /*  62 */ "params ::= var_param COMMA kw_param",
 /*  63 */ "params ::= var_param",
 /*  64 */ "params ::= kw_param",
 /*  65 */ "params ::=",
 /*  66 */ "kw_param ::= STAR_STAR NAME",
 /*  67 */ "var_param ::= STAR NAME",
 /*  68 */ "block_param ::= AMPER NAME param_default_opt",
 /*  69 */ "param_default_opt ::=",
 /*  70 */ "param_default_opt ::= param_default",
 /*  71 */ "param_default ::= EQUAL expr",
 /*  72 */ "params_without_default ::= NAME",
 /*  73 */ "params_without_default ::= params_without_default COMMA NAME",
 /*  74 */ "params_with_default ::= param_with_default",
 /*  75 */ "params_with_default ::= params_with_default COMMA param_with_default",
 /*  76 */ "param_with_default ::= NAME param_default",
 /*  77 */ "args ::=",
 /*  78 */ "args ::= posargs",
 /*  79 */ "args ::= posargs COMMA kwargs",
 /*  80 */ "args ::= posargs COMMA kwargs COMMA vararg",
 /*  81 */ "args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg",
 /*  82 */ "args ::= posargs COMMA vararg",
 /*  83 */ "args ::= posargs COMMA vararg COMMA varkwarg",
 /*  84 */ "args ::= posargs COMMA varkwarg",
 /*  85 */ "args ::= kwargs",
 /*  86 */ "args ::= kwargs COMMA vararg",
 /*  87 */ "args ::= kwargs COMMA vararg COMMA varkwarg",
 /*  88 */ "args ::= kwargs COMMA varkwarg",
 /*  89 */ "args ::= vararg",
 /*  90 */ "args ::= vararg COMMA varkwarg",
 /*  91 */ "args ::= varkwarg",
 /*  92 */ "varkwarg ::= STAR_STAR expr",
 /*  93 */ "vararg ::= STAR expr",
 /*  94 */ "posargs ::= expr",
 /*  95 */ "posargs ::= posargs COMMA expr",
 /*  96 */ "kwargs ::= kwarg",
 /*  97 */ "kwargs ::= kwargs COMMA kwarg",
 /*  98 */ "kwarg ::= NAME COLON expr",
 /*  99 */ "expr ::= assign_expr",
 /* 100 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /* 101 */ "assign_expr ::= logical_or_expr",
 /* 102 */ "logical_or_expr ::= logical_and_expr",
 /* 103 */ "logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr",
 /* 104 */ "logical_and_expr ::= not_expr",
 /* 105 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /* 106 */ "not_expr ::= comparison",
 /* 107 */ "not_expr ::= NOT not_expr",
 /* 108 */ "comparison ::= xor_expr",
 /* 109 */ "comparison ::= xor_expr comp_op xor_expr",
 /* 110 */ "comp_op ::= LESS",
 /* 111 */ "comp_op ::= GREATER",
 /* 112 */ "xor_expr ::= or_expr",
 /* 113 */ "xor_expr ::= xor_expr XOR or_expr",
 /* 114 */ "or_expr ::= and_expr",
 /* 115 */ "or_expr ::= or_expr BAR and_expr",
 /* 116 */ "and_expr ::= shift_expr",
 /* 117 */ "and_expr ::= and_expr AND shift_expr",
 /* 118 */ "shift_expr ::= match_expr",
 /* 119 */ "shift_expr ::= shift_expr shift_op match_expr",
 /* 120 */ "shift_op ::= LSHIFT",
 /* 121 */ "shift_op ::= RSHIFT",
 /* 122 */ "match_expr ::= arith_expr",
 /* 123 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 124 */ "arith_expr ::= term",
 /* 125 */ "arith_expr ::= arith_expr arith_op term",
 /* 126 */ "arith_op ::= PLUS",
 /* 127 */ "arith_op ::= MINUS",
 /* 128 */ "term ::= term term_op factor",
 /* 129 */ "term ::= factor",
 /* 130 */ "term_op ::= STAR",
 /* 131 */ "term_op ::= DIV",
 /* 132 */ "term_op ::= DIV_DIV",
 /* 133 */ "term_op ::= PERCENT",
 /* 134 */ "factor ::= PLUS factor",
 /* 135 */ "factor ::= MINUS factor",
 /* 136 */ "factor ::= TILDA factor",
 /* 137 */ "factor ::= power",
 /* 138 */ "power ::= postfix_expr",
 /* 139 */ "postfix_expr ::= atom",
 /* 140 */ "postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt",
 /* 141 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 142 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 143 */ "atom ::= NAME",
 /* 144 */ "atom ::= NUMBER",
 /* 145 */ "atom ::= REGEXP",
 /* 146 */ "atom ::= STRING",
 /* 147 */ "atom ::= SYMBOL",
 /* 148 */ "atom ::= NIL",
 /* 149 */ "atom ::= TRUE",
 /* 150 */ "atom ::= FALSE",
 /* 151 */ "atom ::= LINE",
 /* 152 */ "atom ::= LBRACKET exprs RBRACKET",
 /* 153 */ "atom ::= LBRACKET RBRACKET",
 /* 154 */ "atom ::= LBRACE RBRACE",
 /* 155 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 156 */ "atom ::= LPAR expr RPAR",
 /* 157 */ "exprs ::= expr",
 /* 158 */ "exprs ::= exprs COMMA expr",
 /* 159 */ "dict_elems ::= dict_elem",
 /* 160 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 161 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 162 */ "dict_elem ::= NAME COLON expr",
 /* 163 */ "comma_opt ::=",
 /* 164 */ "comma_opt ::= COMMA",
 /* 165 */ "blockarg_opt ::=",
 /* 166 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 167 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 168 */ "blockarg_params_opt ::=",
 /* 169 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 170 */ "excepts ::= except",
 /* 171 */ "excepts ::= excepts except",
 /* 172 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 173 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 174 */ "except ::= EXCEPT NEWLINE stmts",
 /* 175 */ "finally_opt ::=",
 /* 176 */ "finally_opt ::= FINALLY stmts",
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
  { 61, 1 },
  { 62, 1 },
  { 62, 3 },
  { 63, 0 },
  { 63, 1 },
  { 63, 1 },
  { 63, 7 },
  { 63, 5 },
  { 63, 5 },
  { 63, 5 },
  { 63, 1 },
  { 63, 2 },
  { 63, 1 },
  { 63, 2 },
  { 63, 1 },
  { 63, 2 },
  { 63, 6 },
  { 63, 6 },
  { 63, 4 },
  { 63, 2 },
  { 63, 2 },
  { 71, 1 },
  { 71, 3 },
  { 72, 1 },
  { 72, 3 },
  { 70, 1 },
  { 70, 3 },
  { 69, 0 },
  { 69, 2 },
  { 68, 1 },
  { 68, 5 },
  { 73, 0 },
  { 73, 2 },
  { 64, 7 },
  { 74, 9 },
  { 74, 7 },
  { 74, 7 },
  { 74, 5 },
  { 74, 7 },
  { 74, 5 },
  { 74, 5 },
  { 74, 3 },
  { 74, 7 },
  { 74, 5 },
  { 74, 5 },
  { 74, 3 },
  { 74, 5 },
  { 74, 3 },
  { 74, 3 },
  { 74, 1 },
  { 74, 7 },
  { 74, 5 },
  { 74, 5 },
  { 74, 3 },
  { 74, 5 },
  { 74, 3 },
  { 74, 3 },
  { 74, 1 },
  { 74, 5 },
  { 74, 3 },
  { 74, 3 },
  { 74, 1 },
  { 74, 3 },
  { 74, 1 },
  { 74, 1 },
  { 74, 0 },
  { 79, 2 },
  { 78, 2 },
  { 77, 3 },
  { 80, 0 },
  { 80, 1 },
  { 81, 2 },
  { 75, 1 },
  { 75, 3 },
  { 76, 1 },
  { 76, 3 },
  { 82, 2 },
  { 83, 0 },
  { 83, 1 },
  { 83, 3 },
  { 83, 5 },
  { 83, 7 },
  { 83, 3 },
  { 83, 5 },
  { 83, 3 },
  { 83, 1 },
  { 83, 3 },
  { 83, 5 },
  { 83, 3 },
  { 83, 1 },
  { 83, 3 },
  { 83, 1 },
  { 87, 2 },
  { 86, 2 },
  { 84, 1 },
  { 84, 3 },
  { 85, 1 },
  { 85, 3 },
  { 88, 3 },
  { 65, 1 },
  { 89, 3 },
  { 89, 1 },
  { 91, 1 },
  { 91, 3 },
  { 92, 1 },
  { 92, 3 },
  { 93, 1 },
  { 93, 2 },
  { 94, 1 },
  { 94, 3 },
  { 96, 1 },
  { 96, 1 },
  { 95, 1 },
  { 95, 3 },
  { 97, 1 },
  { 97, 3 },
  { 98, 1 },
  { 98, 3 },
  { 99, 1 },
  { 99, 3 },
  { 101, 1 },
  { 101, 1 },
  { 100, 1 },
  { 100, 3 },
  { 102, 1 },
  { 102, 3 },
  { 104, 1 },
  { 104, 1 },
  { 103, 3 },
  { 103, 1 },
  { 105, 1 },
  { 105, 1 },
  { 105, 1 },
  { 105, 1 },
  { 106, 2 },
  { 106, 2 },
  { 106, 2 },
  { 106, 1 },
  { 107, 1 },
  { 90, 1 },
  { 90, 5 },
  { 90, 4 },
  { 90, 3 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 3 },
  { 108, 2 },
  { 108, 2 },
  { 108, 4 },
  { 108, 3 },
  { 110, 1 },
  { 110, 3 },
  { 111, 1 },
  { 111, 3 },
  { 113, 3 },
  { 113, 3 },
  { 112, 0 },
  { 112, 1 },
  { 109, 0 },
  { 109, 5 },
  { 109, 5 },
  { 114, 0 },
  { 114, 3 },
  { 66, 1 },
  { 66, 2 },
  { 115, 6 },
  { 115, 4 },
  { 115, 3 },
  { 67, 0 },
  { 67, 2 },
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
#line 701 "parser.y"
{
    *pval = yymsp[0].minor.yy7;
}
#line 2136 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 21: /* dotted_names ::= dotted_name */
      case 74: /* params_with_default ::= param_with_default */
      case 94: /* posargs ::= expr */
      case 96: /* kwargs ::= kwarg */
      case 157: /* exprs ::= expr */
      case 159: /* dict_elems ::= dict_elem */
      case 170: /* excepts ::= except */
#line 705 "parser.y"
{
    yygotominor.yy7 = make_array_with(env, yymsp[0].minor.yy7);
}
#line 2150 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 22: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 75: /* params_with_default ::= params_with_default COMMA param_with_default */
#line 708 "parser.y"
{
    yygotominor.yy7 = Array_push(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2159 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 27: /* super_opt ::= */
      case 31: /* else_opt ::= */
      case 69: /* param_default_opt ::= */
      case 77: /* args ::= */
      case 163: /* comma_opt ::= */
      case 165: /* blockarg_opt ::= */
      case 168: /* blockarg_params_opt ::= */
      case 175: /* finally_opt ::= */
#line 712 "parser.y"
{
    yygotominor.yy7 = YNIL;
}
#line 2174 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 5: /* stmt ::= expr */
      case 28: /* super_opt ::= GREATER expr */
      case 29: /* if_tail ::= else_opt */
      case 32: /* else_opt ::= ELSE stmts */
      case 70: /* param_default_opt ::= param_default */
      case 71: /* param_default ::= EQUAL expr */
      case 92: /* varkwarg ::= STAR_STAR expr */
      case 93: /* vararg ::= STAR expr */
      case 99: /* expr ::= assign_expr */
      case 101: /* assign_expr ::= logical_or_expr */
      case 102: /* logical_or_expr ::= logical_and_expr */
      case 104: /* logical_and_expr ::= not_expr */
      case 106: /* not_expr ::= comparison */
      case 108: /* comparison ::= xor_expr */
      case 112: /* xor_expr ::= or_expr */
      case 114: /* or_expr ::= and_expr */
      case 116: /* and_expr ::= shift_expr */
      case 118: /* shift_expr ::= match_expr */
      case 122: /* match_expr ::= arith_expr */
      case 124: /* arith_expr ::= term */
      case 129: /* term ::= factor */
      case 137: /* factor ::= power */
      case 138: /* power ::= postfix_expr */
      case 139: /* postfix_expr ::= atom */
      case 176: /* finally_opt ::= FINALLY stmts */
#line 715 "parser.y"
{
    yygotominor.yy7 = yymsp[0].minor.yy7;
}
#line 2206 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 721 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy7 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2214 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 725 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[-1].minor.yy7);
}
#line 2222 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 729 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = Finally_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2230 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 733 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = While_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2238 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 737 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Break_new(env, lineno, YNIL);
}
#line 2246 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 741 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Break_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2254 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 745 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Next_new(env, lineno, YNIL);
}
#line 2262 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 749 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Next_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2270 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 753 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Return_new(env, lineno, YNIL);
}
#line 2278 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 757 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Return_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2286 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 761 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy7 = If_new(env, lineno, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2294 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 765 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy7 = Klass_new(env, lineno, id, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2303 "parser.c"
        break;
      case 18: /* stmt ::= MODULE NAME stmts END */
#line 770 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    yygotominor.yy7 = Module_new(env, lineno, id, yymsp[-1].minor.yy7);
}
#line 2312 "parser.c"
        break;
      case 19: /* stmt ::= NONLOCAL names */
#line 775 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Nonlocal_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2320 "parser.c"
        break;
      case 20: /* stmt ::= IMPORT dotted_names */
#line 779 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy7 = Import_new(env, lineno, yymsp[0].minor.yy7);
}
#line 2328 "parser.c"
        break;
      case 23: /* dotted_name ::= NAME */
      case 25: /* names ::= NAME */
#line 791 "parser.y"
{
    yygotominor.yy7 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2336 "parser.c"
        break;
      case 24: /* dotted_name ::= dotted_name DOT NAME */
      case 26: /* names ::= names COMMA NAME */
#line 794 "parser.y"
{
    yygotominor.yy7 = Array_push_token_id(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy0);
}
#line 2344 "parser.c"
        break;
      case 30: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 815 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7, yymsp[0].minor.yy7);
    yygotominor.yy7 = make_array_with(env, node);
}
#line 2353 "parser.c"
        break;
      case 33: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 828 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy7 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2362 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 834 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-8].minor.yy7, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2369 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 837 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2376 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 840 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2383 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 843 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2390 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 846 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2397 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 849 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2404 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 852 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2411 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA params_with_default */
#line 855 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL, YNIL);
}
#line 2418 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 858 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-6].minor.yy7, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2425 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 861 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2432 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 864 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2439 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA block_param */
#line 867 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2446 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 870 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-4].minor.yy7, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2453 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA var_param */
#line 873 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2460 "parser.c"
        break;
      case 48: /* params ::= params_without_default COMMA kw_param */
#line 876 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[-2].minor.yy7, YNIL, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2467 "parser.c"
        break;
      case 49: /* params ::= params_without_default */
#line 879 "parser.y"
{
    yygotominor.yy7 = Params_new(env, yymsp[0].minor.yy7, YNIL, YNIL, YNIL, YNIL);
}
#line 2474 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 882 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2481 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 885 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2488 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 888 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2495 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA block_param */
#line 891 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2502 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 894 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2509 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA var_param */
#line 897 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2516 "parser.c"
        break;
      case 56: /* params ::= params_with_default COMMA kw_param */
#line 900 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2523 "parser.c"
        break;
      case 57: /* params ::= params_with_default */
#line 903 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, yymsp[0].minor.yy7, YNIL, YNIL, YNIL);
}
#line 2530 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 906 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2537 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA var_param */
#line 909 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2544 "parser.c"
        break;
      case 60: /* params ::= block_param COMMA kw_param */
#line 912 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2551 "parser.c"
        break;
      case 61: /* params ::= block_param */
#line 915 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2558 "parser.c"
        break;
      case 62: /* params ::= var_param COMMA kw_param */
#line 918 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2565 "parser.c"
        break;
      case 63: /* params ::= var_param */
#line 921 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2572 "parser.c"
        break;
      case 64: /* params ::= kw_param */
#line 924 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2579 "parser.c"
        break;
      case 65: /* params ::= */
#line 927 "parser.y"
{
    yygotominor.yy7 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2586 "parser.c"
        break;
      case 66: /* kw_param ::= STAR_STAR NAME */
#line 931 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2595 "parser.c"
        break;
      case 67: /* var_param ::= STAR NAME */
#line 937 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2604 "parser.c"
        break;
      case 68: /* block_param ::= AMPER NAME param_default_opt */
#line 943 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy7);
}
#line 2613 "parser.c"
        break;
      case 72: /* params_without_default ::= NAME */
#line 960 "parser.y"
{
    yygotominor.yy7 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy7, lineno, id, YNIL);
}
#line 2623 "parser.c"
        break;
      case 73: /* params_without_default ::= params_without_default COMMA NAME */
#line 966 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy7, lineno, id, YNIL);
    yygotominor.yy7 = yymsp[-2].minor.yy7;
}
#line 2633 "parser.c"
        break;
      case 76: /* param_with_default ::= NAME param_default */
#line 980 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy7 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy7);
}
#line 2642 "parser.c"
        break;
      case 78: /* args ::= posargs */
#line 989 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[0].minor.yy7, YNIL, YNIL, YNIL);
}
#line 2650 "parser.c"
        break;
      case 79: /* args ::= posargs COMMA kwargs */
#line 993 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2658 "parser.c"
        break;
      case 80: /* args ::= posargs COMMA kwargs COMMA vararg */
#line 997 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2666 "parser.c"
        break;
      case 81: /* args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg */
#line 1001 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-6].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-6].minor.yy7, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2674 "parser.c"
        break;
      case 82: /* args ::= posargs COMMA vararg */
#line 1005 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2682 "parser.c"
        break;
      case 83: /* args ::= posargs COMMA vararg COMMA varkwarg */
#line 1009 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-4].minor.yy7, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2690 "parser.c"
        break;
      case 84: /* args ::= posargs COMMA varkwarg */
#line 1013 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, yymsp[-2].minor.yy7, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2698 "parser.c"
        break;
      case 85: /* args ::= kwargs */
#line 1017 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, YNIL, yymsp[0].minor.yy7, YNIL, YNIL);
}
#line 2706 "parser.c"
        break;
      case 86: /* args ::= kwargs COMMA vararg */
#line 1021 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7, YNIL);
}
#line 2714 "parser.c"
        break;
      case 87: /* args ::= kwargs COMMA vararg COMMA varkwarg */
#line 1025 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, YNIL, yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2722 "parser.c"
        break;
      case 88: /* args ::= kwargs COMMA varkwarg */
#line 1029 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy7, 0));
    yygotominor.yy7 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy7, YNIL, yymsp[0].minor.yy7);
}
#line 2730 "parser.c"
        break;
      case 89: /* args ::= vararg */
#line 1033 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy7);
    yygotominor.yy7 = Args_new(env, lineno, YNIL, YNIL, yymsp[0].minor.yy7, YNIL);
}
#line 2738 "parser.c"
        break;
      case 90: /* args ::= vararg COMMA varkwarg */
#line 1037 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    yygotominor.yy7 = Args_new(env, lineno, YNIL, YNIL, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2746 "parser.c"
        break;
      case 91: /* args ::= varkwarg */
#line 1041 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy7);
    yygotominor.yy7 = Args_new(env, lineno, YNIL, YNIL, YNIL, yymsp[0].minor.yy7);
}
#line 2754 "parser.c"
        break;
      case 95: /* posargs ::= posargs COMMA expr */
      case 97: /* kwargs ::= kwargs COMMA kwarg */
      case 158: /* exprs ::= exprs COMMA expr */
      case 160: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 1057 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
    yygotominor.yy7 = yymsp[-2].minor.yy7;
}
#line 2765 "parser.c"
        break;
      case 98: /* kwarg ::= NAME COLON expr */
#line 1070 "parser.y"
{
    yygotominor.yy7 = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(yymsp[-2].minor.yy0));
    PTR_AS(YogNode, yygotominor.yy7)->u.kwarg.name = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    PTR_AS(YogNode, yygotominor.yy7)->u.kwarg.value = yymsp[0].minor.yy7;
}
#line 2774 "parser.c"
        break;
      case 100: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 1080 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    yygotominor.yy7 = Assign_new(env, lineno, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2782 "parser.c"
        break;
      case 103: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 1091 "parser.y"
{
    yygotominor.yy7 = YogNode_new(env, NODE_LOGICAL_OR, NODE_LINENO(yymsp[-2].minor.yy7));
    NODE(yygotominor.yy7)->u.logical_or.left = yymsp[-2].minor.yy7;
    NODE(yygotominor.yy7)->u.logical_or.right = yymsp[0].minor.yy7;
}
#line 2791 "parser.c"
        break;
      case 105: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 1100 "parser.y"
{
    yygotominor.yy7 = YogNode_new(env, NODE_LOGICAL_AND, NODE_LINENO(yymsp[-2].minor.yy7));
    NODE(yygotominor.yy7)->u.logical_and.left = yymsp[-2].minor.yy7;
    NODE(yygotominor.yy7)->u.logical_and.right = yymsp[0].minor.yy7;
}
#line 2800 "parser.c"
        break;
      case 107: /* not_expr ::= NOT not_expr */
#line 1109 "parser.y"
{
    yygotominor.yy7 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy7)->u.not.expr = yymsp[0].minor.yy7;
}
#line 2808 "parser.c"
        break;
      case 109: /* comparison ::= xor_expr comp_op xor_expr */
#line 1117 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy7)->u.id;
    yygotominor.yy7 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy7, id, yymsp[0].minor.yy7);
}
#line 2817 "parser.c"
        break;
      case 110: /* comp_op ::= LESS */
      case 111: /* comp_op ::= GREATER */
      case 164: /* comma_opt ::= COMMA */
#line 1123 "parser.y"
{
    yygotominor.yy7 = yymsp[0].minor.yy0;
}
#line 2826 "parser.c"
        break;
      case 113: /* xor_expr ::= xor_expr XOR or_expr */
      case 115: /* or_expr ::= or_expr BAR and_expr */
      case 117: /* and_expr ::= and_expr AND shift_expr */
#line 1133 "parser.y"
{
    yygotominor.yy7 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy7), yymsp[-2].minor.yy7, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy7);
}
#line 2835 "parser.c"
        break;
      case 119: /* shift_expr ::= shift_expr shift_op match_expr */
      case 125: /* arith_expr ::= arith_expr arith_op term */
      case 128: /* term ::= term term_op factor */
#line 1154 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    yygotominor.yy7 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy7, VAL2ID(yymsp[-1].minor.yy7), yymsp[0].minor.yy7);
}
#line 2845 "parser.c"
        break;
      case 120: /* shift_op ::= LSHIFT */
      case 121: /* shift_op ::= RSHIFT */
      case 126: /* arith_op ::= PLUS */
      case 127: /* arith_op ::= MINUS */
      case 130: /* term_op ::= STAR */
      case 131: /* term_op ::= DIV */
      case 132: /* term_op ::= DIV_DIV */
      case 133: /* term_op ::= PERCENT */
#line 1159 "parser.y"
{
    yygotominor.yy7 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2859 "parser.c"
        break;
      case 123: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1169 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy7 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy7, id, yymsp[0].minor.yy7);
}
#line 2868 "parser.c"
        break;
      case 134: /* factor ::= PLUS factor */
#line 1211 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy7 = FuncCall_new3(env, lineno, yymsp[0].minor.yy7, id);
}
#line 2877 "parser.c"
        break;
      case 135: /* factor ::= MINUS factor */
#line 1216 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy7 = FuncCall_new3(env, lineno, yymsp[0].minor.yy7, id);
}
#line 2886 "parser.c"
        break;
      case 136: /* factor ::= TILDA factor */
#line 1221 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy7 = FuncCall_new3(env, lineno, yymsp[0].minor.yy7, id);
}
#line 2895 "parser.c"
        break;
      case 140: /* postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt */
#line 1237 "parser.y"
{
    yygotominor.yy7 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy7), yymsp[-4].minor.yy7, yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 2902 "parser.c"
        break;
      case 141: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1240 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy7);
    yygotominor.yy7 = Subscript_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 2910 "parser.c"
        break;
      case 142: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1244 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy7);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Attr_new(env, lineno, yymsp[-2].minor.yy7, id);
}
#line 2919 "parser.c"
        break;
      case 143: /* atom ::= NAME */
#line 1250 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy7 = Variable_new(env, lineno, id);
}
#line 2928 "parser.c"
        break;
      case 144: /* atom ::= NUMBER */
      case 145: /* atom ::= REGEXP */
      case 146: /* atom ::= STRING */
      case 147: /* atom ::= SYMBOL */
#line 1255 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy7 = Literal_new(env, lineno, val);
}
#line 2940 "parser.c"
        break;
      case 148: /* atom ::= NIL */
#line 1275 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Literal_new(env, lineno, YNIL);
}
#line 2948 "parser.c"
        break;
      case 149: /* atom ::= TRUE */
#line 1279 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Literal_new(env, lineno, YTRUE);
}
#line 2956 "parser.c"
        break;
      case 150: /* atom ::= FALSE */
#line 1283 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy7 = Literal_new(env, lineno, YFALSE);
}
#line 2964 "parser.c"
        break;
      case 151: /* atom ::= LINE */
#line 1287 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy7 = Literal_new(env, lineno, val);
}
#line 2973 "parser.c"
        break;
      case 152: /* atom ::= LBRACKET exprs RBRACKET */
#line 1292 "parser.y"
{
    yygotominor.yy7 = Array_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy7);
}
#line 2980 "parser.c"
        break;
      case 153: /* atom ::= LBRACKET RBRACKET */
#line 1295 "parser.y"
{
    yygotominor.yy7 = Array_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 2987 "parser.c"
        break;
      case 154: /* atom ::= LBRACE RBRACE */
#line 1298 "parser.y"
{
    yygotominor.yy7 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 2994 "parser.c"
        break;
      case 155: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1301 "parser.y"
{
    yygotominor.yy7 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy7);
}
#line 3001 "parser.c"
        break;
      case 156: /* atom ::= LPAR expr RPAR */
      case 169: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1304 "parser.y"
{
    yygotominor.yy7 = yymsp[-1].minor.yy7;
}
#line 3009 "parser.c"
        break;
      case 161: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1323 "parser.y"
{
    yygotominor.yy7 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy7), yymsp[-2].minor.yy7, yymsp[0].minor.yy7);
}
#line 3016 "parser.c"
        break;
      case 162: /* dict_elem ::= NAME COLON expr */
#line 1326 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy7 = DictElem_new(env, lineno, var, yymsp[0].minor.yy7);
}
#line 3026 "parser.c"
        break;
      case 166: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 167: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1343 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy7 = BlockArg_new(env, lineno, yymsp[-3].minor.yy7, yymsp[-1].minor.yy7);
}
#line 3035 "parser.c"
        break;
      case 171: /* excepts ::= excepts except */
#line 1362 "parser.y"
{
    yygotominor.yy7 = Array_push(env, yymsp[-1].minor.yy7, yymsp[0].minor.yy7);
}
#line 3042 "parser.c"
        break;
      case 172: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1366 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy7 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy7, id, yymsp[0].minor.yy7);
}
#line 3052 "parser.c"
        break;
      case 173: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1372 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy7 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy7, NO_EXC_VAR, yymsp[0].minor.yy7);
}
#line 3060 "parser.c"
        break;
      case 174: /* except ::= EXCEPT NEWLINE stmts */
#line 1376 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy7 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy7);
}
#line 3068 "parser.c"
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
