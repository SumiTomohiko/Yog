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
        KEEP(funcdef.decorators);
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
        KEEP(klass.decorators);
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
FuncDef_new(YogEnv* env, uint_t lineno, YogVal decorators, ID name, YogVal params, YogVal stmts)
{
    SAVE_ARGS3(env, decorators, params, stmts);

    YogVal node = YogNode_new(env, NODE_FUNC_DEF, lineno);
    NODE(node)->u.funcdef.decorators = decorators;
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
Klass_new(YogEnv* env, uint_t lineno, YogVal decorators, ID name, YogVal super, YogVal stmts)
{
    SAVE_ARGS3(env, decorators, super, stmts);

    YogVal node = YogNode_new(env, NODE_KLASS, lineno);
    NODE(node)->u.klass.decorators = decorators;
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
#line 709 "parser.c"
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
#define YYNOCODE 121
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy127;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 304
#define YYNRULE 182
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
 /*     0 */     1,   27,  102,  185,   24,   25,   33,   34,   35,  338,
 /*    10 */   220,  152,   89,   71,  167,  168,  170,  105,  338,   29,
 /*    20 */   128,   37,  241,  138,  487,  106,  194,  192,  193,   44,
 /*    30 */    73,  229,  110,  218,  206,  219,   31,   56,   57,   91,
 /*    40 */   267,  103,   58,   21,   87,  221,  222,  223,  224,  225,
 /*    50 */   226,  227,  228,   20,  301,  198,  104,  129,  130,  209,
 /*    60 */   200,   75,  264,  131,  132,   79,  133,  105,   80,   74,
 /*    70 */   167,  168,  218,  206,  219,   62,  194,  192,  193,   78,
 /*    80 */    74,   63,  110,  218,  206,  219,  105,   39,  138,   91,
 /*    90 */   267,  122,   55,  131,  132,   79,  133,   45,   80,   74,
 /*   100 */    26,   31,  218,  206,  219,  198,  104,  129,  130,  209,
 /*   110 */   200,   75,  117,  131,  132,   79,  133,   18,   80,   74,
 /*   120 */   105,    3,  218,  206,  219,   76,  194,  192,  193,  142,
 /*   130 */   248,  241,  110,  303,  105,   49,  202,  206,  219,   91,
 /*   140 */   267,  123,  132,   79,  133,   18,   80,   74,  147,   16,
 /*   150 */   218,  206,  219,  150,  257,  198,  104,  129,  130,  209,
 /*   160 */   200,   75,  160,  131,  132,   79,  133,  184,   80,   74,
 /*   170 */   252,   17,  218,  206,  219,  118,  194,  192,  193,  166,
 /*   180 */   272,   53,  110,  207,  105,  164,  171,  173,  284,   91,
 /*   190 */   267,  285,  124,   79,  133,   40,   80,   74,  258,   26,
 /*   200 */   218,  206,  219,   23,  257,  198,  104,  129,  130,  209,
 /*   210 */   200,   75,   18,  131,  132,   79,  133,   22,   80,   74,
 /*   220 */   105,   14,  218,  206,  219,  107,  194,  192,  193,    2,
 /*   230 */    18,    3,  110,  196,  105,  136,  203,  206,  219,   91,
 /*   240 */   267,  172,  282,   77,  133,  243,   80,   74,  176,  287,
 /*   250 */   218,  206,  219,  155,  158,  198,  104,  129,  130,  209,
 /*   260 */   200,   75,  247,  131,  132,   79,  133,  295,   80,   74,
 /*   270 */   105,  145,  218,  206,  219,  109,  194,  192,  193,   97,
 /*   280 */   186,   18,  110,    8,  105,   23,  204,  206,  219,   91,
 /*   290 */   267,  167,  168,  170,  125,   18,   80,   74,  253,   36,
 /*   300 */   218,  206,  219,  179,  291,  198,  104,  129,  130,  209,
 /*   310 */   200,   75,  151,  131,  132,   79,  133,  105,   80,   74,
 /*   320 */   214,  249,  218,  206,  219,   64,  194,  192,  193,  276,
 /*   330 */   277,   51,  110,  205,  206,  219,  215,  216,  217,   91,
 /*   340 */   267,  162,  210,  211,  163,  174,  178,  180,  293,  212,
 /*   350 */   213,  285,  304,   18,   38,  198,  104,  129,  130,  209,
 /*   360 */   200,   75,  262,  131,  132,   79,  133,   18,   80,   74,
 /*   370 */   259,   94,  218,  206,  219,   65,  194,  192,  193,  153,
 /*   380 */    37,  265,  110,  167,  168,  170,  280,  156,  299,   91,
 /*   390 */   267,  181,  167,  182,  163,  174,  178,  180,  293,  270,
 /*   400 */   274,  285,  298,   46,  281,  198,  104,  129,  130,  209,
 /*   410 */   200,   75,  283,  131,  132,   79,  133,   18,   80,   74,
 /*   420 */   260,  286,  218,  206,  219,  149,  194,  192,  193,  165,
 /*   430 */   169,  275,  110,   18,  279,   18,  269,  288,  302,   91,
 /*   440 */   267,  175,  177,  289,  290,  195,  279,  292,   18,    4,
 /*   450 */    42,   46,   43,   47,   19,  198,  104,  129,  130,  209,
 /*   460 */   200,   75,   48,  131,  132,   79,  133,   52,   80,   74,
 /*   470 */    66,   82,  218,  206,  219,  111,  194,  192,  193,   28,
 /*   480 */   230,  233,  110,   30,  184,   32,   84,   61,   17,   91,
 /*   490 */   267,   85,   86,    5,   41,   81,    6,  256,    7,   88,
 /*   500 */     9,   10,  154,  261,   90,  198,  104,  129,  130,  209,
 /*   510 */   200,   75,   40,  131,  132,   79,  133,  157,   80,   74,
 /*   520 */   263,  266,  218,  206,  219,  112,  194,  192,  193,  161,
 /*   530 */    50,   54,  110,   11,   59,   67,  271,   92,  273,   91,
 /*   540 */   267,   93,   72,   68,   95,   96,   12,   60,   69,   98,
 /*   550 */    99,   70,  100,  101,  297,  198,  104,  129,  130,  209,
 /*   560 */   200,   75,  294,  131,  132,   79,  133,  300,   80,   74,
 /*   570 */   187,   13,  218,  206,  219,  113,  194,  192,  193,  488,
 /*   580 */   488,  488,  110,  488,  296,  488,  488,  488,  488,   91,
 /*   590 */   267,  488,  488,  488,  488,  488,  488,  488,  488,  488,
 /*   600 */   488,  488,  488,  488,  488,  198,  104,  129,  130,  209,
 /*   610 */   200,   75,  488,  131,  132,   79,  133,  488,   80,   74,
 /*   620 */   488,  488,  218,  206,  219,  114,  194,  192,  193,  488,
 /*   630 */   488,  488,  110,  488,  488,  488,  488,  488,  488,   91,
 /*   640 */   267,  488,  488,  488,  488,  488,  488,  488,  488,  488,
 /*   650 */   488,  488,  488,  488,  488,  198,  104,  129,  130,  209,
 /*   660 */   200,   75,  488,  131,  132,   79,  133,  488,   80,   74,
 /*   670 */   488,  488,  218,  206,  219,  188,  194,  192,  193,  488,
 /*   680 */   488,  488,  110,  488,  488,  488,  488,  488,  488,   91,
 /*   690 */   267,  488,  488,  488,  488,  488,  488,  488,  488,  488,
 /*   700 */   488,  488,  488,  488,  488,  198,  104,  129,  130,  209,
 /*   710 */   200,   75,  488,  131,  132,   79,  133,  488,   80,   74,
 /*   720 */   488,  488,  218,  206,  219,  189,  194,  192,  193,  488,
 /*   730 */   488,  488,  110,  488,  488,  488,  488,  488,  488,   91,
 /*   740 */   267,  488,  488,  488,  488,  488,  488,  488,  488,  488,
 /*   750 */   488,  488,  488,  488,  488,  198,  104,  129,  130,  209,
 /*   760 */   200,   75,  488,  131,  132,   79,  133,  488,   80,   74,
 /*   770 */   488,  488,  218,  206,  219,  190,  194,  192,  193,  488,
 /*   780 */   488,  488,  110,  488,  488,  488,  488,  488,  488,   91,
 /*   790 */   267,  488,  488,  488,  488,  488,  488,  488,  488,  488,
 /*   800 */   488,  488,  488,  488,  488,  198,  104,  129,  130,  209,
 /*   810 */   200,   75,  488,  131,  132,   79,  133,  488,   80,   74,
 /*   820 */   488,  488,  218,  206,  219,  116,  194,  192,  193,  488,
 /*   830 */   488,  488,  110,  488,  488,  488,  488,  488,  488,   91,
 /*   840 */   267,  488,  488,  488,  488,  488,  488,  488,  488,  115,
 /*   850 */   488,  488,  488,  488,  488,  198,  104,  129,  130,  209,
 /*   860 */   200,   75,  488,  131,  132,   79,  133,  488,   80,   74,
 /*   870 */   488,  488,  218,  206,  219,  251,  198,  104,  129,  130,
 /*   880 */   209,  200,   75,  488,  131,  132,   79,  133,  488,   80,
 /*   890 */    74,  488,  488,  218,  206,  219,  144,  126,  141,  143,
 /*   900 */   250,  246,  198,  104,  129,  130,  209,  200,   75,  488,
 /*   910 */   131,  132,   79,  133,  488,   80,   74,  488,  488,  218,
 /*   920 */   206,  219,  191,  192,  193,  488,  488,  488,  110,  488,
 /*   930 */   488,  488,  488,  488,  488,   91,  267,  488,  488,  488,
 /*   940 */   488,  488,  488,  488,  488,  488,  488,  488,  488,  488,
 /*   950 */   488,  198,  104,  129,  130,  209,  200,   75,  488,  131,
 /*   960 */   132,   79,  133,  245,   80,   74,  488,  488,  218,  206,
 /*   970 */   219,  488,  488,  488,  488,  488,  488,  488,  488,  488,
 /*   980 */   488,  488,  488,  137,  488,  488,  127,  139,  244,  246,
 /*   990 */   198,  104,  129,  130,  209,  200,   75,  488,  131,  132,
 /*  1000 */    79,  133,  488,   80,   74,  488,  488,  218,  206,  219,
 /*  1010 */   198,  104,  129,  130,  209,  200,   75,  488,  131,  132,
 /*  1020 */    79,  133,  488,   80,   74,  488,  140,  218,  206,  219,
 /*  1030 */   488,  488,   83,  488,  236,   29,  488,  488,   26,   31,
 /*  1040 */   488,  488,  488,  488,  488,   44,  488,  488,  488,  488,
 /*  1050 */   239,  488,  488,   56,   57,  488,  488,  488,   58,   21,
 /*  1060 */   488,  221,  222,  223,  224,  225,  226,  227,  228,   20,
 /*  1070 */   488,  488,  488,  488,  137,  488,  488,  198,  104,  129,
 /*  1080 */   130,  209,  200,   75,  488,  131,  132,   79,  133,  488,
 /*  1090 */    80,   74,  488,  488,  218,  206,  219,  488,  108,  488,
 /*  1100 */   488,  198,  104,  129,  130,  209,  200,   75,  134,  131,
 /*  1110 */   132,   79,  133,  488,   80,   74,  488,   29,  218,  206,
 /*  1120 */   219,  488,  488,  488,  488,  234,  488,   44,  220,  488,
 /*  1130 */   488,  488,  488,  488,  488,   56,   57,   29,  488,  488,
 /*  1140 */    58,   21,  488,  221,  222,  223,  224,  225,  226,  227,
 /*  1150 */   228,   20,  232,  220,  488,   56,   57,  488,  488,  488,
 /*  1160 */    58,   21,   29,  221,  222,  223,  224,  225,  226,  227,
 /*  1170 */   228,   20,   44,  488,  119,  488,  488,  488,  488,  488,
 /*  1180 */    56,   57,  488,  488,  488,   58,   21,  238,  221,  222,
 /*  1190 */   223,  224,  225,  226,  227,  228,   20,   15,  488,  488,
 /*  1200 */   488,  198,  104,  129,  130,  209,  200,   75,  220,  131,
 /*  1210 */   132,   79,  133,  488,   80,   74,  488,   29,  218,  206,
 /*  1220 */   219,  488,  488,  488,  488,  488,  488,   44,  488,  197,
 /*  1230 */   488,  488,  488,  488,  488,   56,   57,  488,  488,  488,
 /*  1240 */    58,   21,  488,  221,  222,  223,  224,  225,  226,  227,
 /*  1250 */   228,   20,  208,  488,  488,  488,  198,  104,  129,  130,
 /*  1260 */   209,  200,   75,  488,  131,  132,   79,  133,  488,   80,
 /*  1270 */    74,  488,  237,  218,  206,  219,  488,  488,  488,  198,
 /*  1280 */   104,  129,  130,  209,  200,   75,  488,  131,  132,   79,
 /*  1290 */   133,  488,   80,   74,  231,  488,  218,  206,  219,  198,
 /*  1300 */   104,  129,  130,  209,  200,   75,  488,  131,  132,   79,
 /*  1310 */   133,  488,   80,   74,  135,  488,  218,  206,  219,  488,
 /*  1320 */   488,  198,  104,  129,  130,  209,  200,   75,  488,  131,
 /*  1330 */   132,   79,  133,  488,   80,   74,  235,  488,  218,  206,
 /*  1340 */   219,  198,  104,  129,  130,  209,  200,   75,  488,  131,
 /*  1350 */   132,   79,  133,  488,   80,   74,  240,  488,  218,  206,
 /*  1360 */   219,  488,  488,  198,  104,  129,  130,  209,  200,   75,
 /*  1370 */   488,  131,  132,   79,  133,  488,   80,   74,  488,  242,
 /*  1380 */   218,  206,  219,  198,  104,  129,  130,  209,  200,   75,
 /*  1390 */   488,  131,  132,   79,  133,  488,   80,   74,  488,  254,
 /*  1400 */   218,  206,  219,  488,  488,  488,  198,  104,  129,  130,
 /*  1410 */   209,  200,   75,  488,  131,  132,   79,  133,  488,   80,
 /*  1420 */    74,  255,  488,  218,  206,  219,  198,  104,  129,  130,
 /*  1430 */   209,  200,   75,  488,  131,  132,   79,  133,  488,   80,
 /*  1440 */    74,  146,  488,  218,  206,  219,  488,  488,  198,  104,
 /*  1450 */   129,  130,  209,  200,   75,  488,  131,  132,   79,  133,
 /*  1460 */   488,   80,   74,  148,  488,  218,  206,  219,  198,  104,
 /*  1470 */   129,  130,  209,  200,   75,  488,  131,  132,   79,  133,
 /*  1480 */   488,   80,   74,  159,  488,  218,  206,  219,  488,  488,
 /*  1490 */   198,  104,  129,  130,  209,  200,   75,  488,  131,  132,
 /*  1500 */    79,  133,  488,   80,   74,  488,  268,  218,  206,  219,
 /*  1510 */   198,  104,  129,  130,  209,  200,   75,  488,  131,  132,
 /*  1520 */    79,  133,  488,   80,   74,  488,  278,  218,  206,  219,
 /*  1530 */   488,  488,  488,  198,  104,  129,  130,  209,  200,   75,
 /*  1540 */   488,  131,  132,   79,  133,  488,   80,   74,  183,  488,
 /*  1550 */   218,  206,  219,  198,  104,  129,  130,  209,  200,   75,
 /*  1560 */   488,  131,  132,   79,  133,  488,   80,   74,  488,  488,
 /*  1570 */   218,  206,  219,  488,  488,  198,  104,  129,  130,  209,
 /*  1580 */   200,   75,  488,  131,  132,   79,  133,  488,   80,   74,
 /*  1590 */   134,  488,  218,  206,  219,  488,  488,  488,  488,   29,
 /*  1600 */   488,  488,  488,  105,  120,  130,  209,  200,   75,   44,
 /*  1610 */   131,  132,   79,  133,  488,   80,   74,   56,   57,  218,
 /*  1620 */   206,  219,   58,   21,  488,  221,  222,  223,  224,  225,
 /*  1630 */   226,  227,  228,   20,  488,  220,  488,  488,  488,  488,
 /*  1640 */   488,  488,  488,  105,   29,  121,  209,  200,   75,  488,
 /*  1650 */   131,  132,   79,  133,   44,   80,   74,  488,  488,  218,
 /*  1660 */   206,  219,   56,   57,  488,  488,  488,   58,   21,  488,
 /*  1670 */   221,  222,  223,  224,  225,  226,  227,  228,   20,  105,
 /*  1680 */   488,  488,  199,  200,   75,  488,  131,  132,   79,  133,
 /*  1690 */   488,   80,   74,  488,  488,  218,  206,  219,  105,  488,
 /*  1700 */   488,  201,  200,   75,  488,  131,  132,   79,  133,  488,
 /*  1710 */    80,   74,  488,  488,  218,  206,  219,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   16,   12,   68,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   15,   24,   25,   26,   94,   20,   21,
 /*    20 */    90,   23,   92,   12,   62,   63,   64,   65,   66,   31,
 /*    30 */   107,   46,   70,  110,  111,  112,   25,   39,   40,   77,
 /*    40 */    78,   55,   44,   45,   58,   47,   48,   49,   50,   51,
 /*    50 */    52,   53,   54,   55,  119,   93,   94,   95,   96,   97,
 /*    60 */    98,   99,   12,  101,  102,  103,  104,   94,  106,  107,
 /*    70 */    24,   25,  110,  111,  112,   63,   64,   65,   66,  106,
 /*    80 */   107,   67,   70,  110,  111,  112,   94,   27,   12,   77,
 /*    90 */    78,   99,  109,  101,  102,  103,  104,  100,  106,  107,
 /*   100 */    24,   25,  110,  111,  112,   93,   94,   95,   96,   97,
 /*   110 */    98,   99,   68,  101,  102,  103,  104,    1,  106,  107,
 /*   120 */    94,    5,  110,  111,  112,   63,   64,   65,   66,   90,
 /*   130 */    91,   92,   70,  119,   94,  105,  110,  111,  112,   77,
 /*   140 */    78,  101,  102,  103,  104,    1,  106,  107,   69,    5,
 /*   150 */   110,  111,  112,   11,   75,   93,   94,   95,   96,   97,
 /*   160 */    98,   99,   20,  101,  102,  103,  104,   17,  106,  107,
 /*   170 */   113,   21,  110,  111,  112,   63,   64,   65,   66,   82,
 /*   180 */    83,  108,   70,   91,   94,   80,   81,   82,   83,   77,
 /*   190 */    78,   86,  102,  103,  104,   45,  106,  107,   69,   24,
 /*   200 */   110,  111,  112,   59,   75,   93,   94,   95,   96,   97,
 /*   210 */    98,   99,    1,  101,  102,  103,  104,   16,  106,  107,
 /*   220 */    94,    1,  110,  111,  112,   63,   64,   65,   66,    3,
 /*   230 */     1,    5,   70,    4,   94,  116,  110,  111,  112,   77,
 /*   240 */    78,   82,   83,  103,  104,   91,  106,  107,   82,   83,
 /*   250 */   110,  111,  112,   73,   74,   93,   94,   95,   96,   97,
 /*   260 */    98,   99,   91,  101,  102,  103,  104,   56,  106,  107,
 /*   270 */    94,  118,  110,  111,  112,   63,   64,   65,   66,   12,
 /*   280 */    60,    1,   70,    3,   94,   59,  110,  111,  112,   77,
 /*   290 */    78,   24,   25,   26,  104,    1,  106,  107,    4,   19,
 /*   300 */   110,  111,  112,   82,   83,   93,   94,   95,   96,   97,
 /*   310 */    98,   99,   71,  101,  102,  103,  104,   94,  106,  107,
 /*   320 */    25,   91,  110,  111,  112,   63,   64,   65,   66,   84,
 /*   330 */    85,   45,   70,  110,  111,  112,   41,   42,   43,   77,
 /*   340 */    78,   76,   36,   37,   79,   80,   81,   82,   83,   39,
 /*   350 */    40,   86,    0,    1,   18,   93,   94,   95,   96,   97,
 /*   360 */    98,   99,   12,  101,  102,  103,  104,    1,  106,  107,
 /*   370 */     4,   12,  110,  111,  112,   63,   64,   65,   66,   72,
 /*   380 */    23,   78,   70,   24,   25,   26,   85,   74,   18,   77,
 /*   390 */    78,   76,   24,  118,   79,   80,   81,   82,   83,   83,
 /*   400 */    83,   86,   32,   33,   83,   93,   94,   95,   96,   97,
 /*   410 */    98,   99,   83,  101,  102,  103,  104,    1,  106,  107,
 /*   420 */     4,   83,  110,  111,  112,   63,   64,   65,   66,   81,
 /*   430 */    82,   83,   70,    1,   86,    1,    4,   83,    4,   77,
 /*   440 */    78,   81,   82,   83,   83,    4,   86,   83,    1,    1,
 /*   450 */    29,   33,   30,   34,   16,   93,   94,   95,   96,   97,
 /*   460 */    98,   99,   35,  101,  102,  103,  104,   38,  106,  107,
 /*   470 */    16,   16,  110,  111,  112,   63,   64,   65,   66,   28,
 /*   480 */    22,   56,   70,   57,   17,   28,   16,   16,   21,   77,
 /*   490 */    78,   16,   16,    1,   27,   22,    1,    4,    1,   12,
 /*   500 */     1,   12,   16,   12,   16,   93,   94,   95,   96,   97,
 /*   510 */    98,   99,   45,  101,  102,  103,  104,   17,  106,  107,
 /*   520 */    12,    1,  110,  111,  112,   63,   64,   65,   66,   12,
 /*   530 */    21,   16,   70,   22,   16,   16,   12,   16,   12,   77,
 /*   540 */    78,   16,   12,   16,   16,   16,    1,   16,   16,   16,
 /*   550 */    16,   16,   16,   16,   12,   93,   94,   95,   96,   97,
 /*   560 */    98,   99,   46,  101,  102,  103,  104,    4,  106,  107,
 /*   570 */    12,    1,  110,  111,  112,   63,   64,   65,   66,  120,
 /*   580 */   120,  120,   70,  120,   46,  120,  120,  120,  120,   77,
 /*   590 */    78,  120,  120,  120,  120,  120,  120,  120,  120,  120,
 /*   600 */   120,  120,  120,  120,  120,   93,   94,   95,   96,   97,
 /*   610 */    98,   99,  120,  101,  102,  103,  104,  120,  106,  107,
 /*   620 */   120,  120,  110,  111,  112,   63,   64,   65,   66,  120,
 /*   630 */   120,  120,   70,  120,  120,  120,  120,  120,  120,   77,
 /*   640 */    78,  120,  120,  120,  120,  120,  120,  120,  120,  120,
 /*   650 */   120,  120,  120,  120,  120,   93,   94,   95,   96,   97,
 /*   660 */    98,   99,  120,  101,  102,  103,  104,  120,  106,  107,
 /*   670 */   120,  120,  110,  111,  112,   63,   64,   65,   66,  120,
 /*   680 */   120,  120,   70,  120,  120,  120,  120,  120,  120,   77,
 /*   690 */    78,  120,  120,  120,  120,  120,  120,  120,  120,  120,
 /*   700 */   120,  120,  120,  120,  120,   93,   94,   95,   96,   97,
 /*   710 */    98,   99,  120,  101,  102,  103,  104,  120,  106,  107,
 /*   720 */   120,  120,  110,  111,  112,   63,   64,   65,   66,  120,
 /*   730 */   120,  120,   70,  120,  120,  120,  120,  120,  120,   77,
 /*   740 */    78,  120,  120,  120,  120,  120,  120,  120,  120,  120,
 /*   750 */   120,  120,  120,  120,  120,   93,   94,   95,   96,   97,
 /*   760 */    98,   99,  120,  101,  102,  103,  104,  120,  106,  107,
 /*   770 */   120,  120,  110,  111,  112,   63,   64,   65,   66,  120,
 /*   780 */   120,  120,   70,  120,  120,  120,  120,  120,  120,   77,
 /*   790 */    78,  120,  120,  120,  120,  120,  120,  120,  120,  120,
 /*   800 */   120,  120,  120,  120,  120,   93,   94,   95,   96,   97,
 /*   810 */    98,   99,  120,  101,  102,  103,  104,  120,  106,  107,
 /*   820 */   120,  120,  110,  111,  112,   63,   64,   65,   66,  120,
 /*   830 */   120,  120,   70,  120,  120,  120,  120,  120,  120,   77,
 /*   840 */    78,  120,  120,  120,  120,  120,  120,  120,  120,   66,
 /*   850 */   120,  120,  120,  120,  120,   93,   94,   95,   96,   97,
 /*   860 */    98,   99,  120,  101,  102,  103,  104,  120,  106,  107,
 /*   870 */   120,  120,  110,  111,  112,   66,   93,   94,   95,   96,
 /*   880 */    97,   98,   99,  120,  101,  102,  103,  104,  120,  106,
 /*   890 */   107,  120,  120,  110,  111,  112,   87,   88,   89,   90,
 /*   900 */    91,   92,   93,   94,   95,   96,   97,   98,   99,  120,
 /*   910 */   101,  102,  103,  104,  120,  106,  107,  120,  120,  110,
 /*   920 */   111,  112,   64,   65,   66,  120,  120,  120,   70,  120,
 /*   930 */   120,  120,  120,  120,  120,   77,   78,  120,  120,  120,
 /*   940 */   120,  120,  120,  120,  120,  120,  120,  120,  120,  120,
 /*   950 */   120,   93,   94,   95,   96,   97,   98,   99,  120,  101,
 /*   960 */   102,  103,  104,   66,  106,  107,  120,  120,  110,  111,
 /*   970 */   112,  120,  120,  120,  120,  120,  120,  120,  120,  120,
 /*   980 */   120,  120,  120,   66,  120,  120,   89,   90,   91,   92,
 /*   990 */    93,   94,   95,   96,   97,   98,   99,  120,  101,  102,
 /*  1000 */   103,  104,  120,  106,  107,  120,  120,  110,  111,  112,
 /*  1010 */    93,   94,   95,   96,   97,   98,   99,  120,  101,  102,
 /*  1020 */   103,  104,  120,  106,  107,  120,   12,  110,  111,  112,
 /*  1030 */   120,  120,  115,  120,  117,   21,  120,  120,   24,   25,
 /*  1040 */   120,  120,  120,  120,  120,   31,  120,  120,  120,  120,
 /*  1050 */    66,  120,  120,   39,   40,  120,  120,  120,   44,   45,
 /*  1060 */   120,   47,   48,   49,   50,   51,   52,   53,   54,   55,
 /*  1070 */   120,  120,  120,  120,   66,  120,  120,   93,   94,   95,
 /*  1080 */    96,   97,   98,   99,  120,  101,  102,  103,  104,  120,
 /*  1090 */   106,  107,  120,  120,  110,  111,  112,  120,  114,  120,
 /*  1100 */   120,   93,   94,   95,   96,   97,   98,   99,   12,  101,
 /*  1110 */   102,  103,  104,  120,  106,  107,  120,   21,  110,  111,
 /*  1120 */   112,  120,  120,  120,  120,  117,  120,   31,   12,  120,
 /*  1130 */   120,  120,  120,  120,  120,   39,   40,   21,  120,  120,
 /*  1140 */    44,   45,  120,   47,   48,   49,   50,   51,   52,   53,
 /*  1150 */    54,   55,   56,   12,  120,   39,   40,  120,  120,  120,
 /*  1160 */    44,   45,   21,   47,   48,   49,   50,   51,   52,   53,
 /*  1170 */    54,   55,   31,  120,   66,  120,  120,  120,  120,  120,
 /*  1180 */    39,   40,  120,  120,  120,   44,   45,   46,   47,   48,
 /*  1190 */    49,   50,   51,   52,   53,   54,   55,    1,  120,  120,
 /*  1200 */   120,   93,   94,   95,   96,   97,   98,   99,   12,  101,
 /*  1210 */   102,  103,  104,  120,  106,  107,  120,   21,  110,  111,
 /*  1220 */   112,  120,  120,  120,  120,  120,  120,   31,  120,   66,
 /*  1230 */   120,  120,  120,  120,  120,   39,   40,  120,  120,  120,
 /*  1240 */    44,   45,  120,   47,   48,   49,   50,   51,   52,   53,
 /*  1250 */    54,   55,   66,  120,  120,  120,   93,   94,   95,   96,
 /*  1260 */    97,   98,   99,  120,  101,  102,  103,  104,  120,  106,
 /*  1270 */   107,  120,   66,  110,  111,  112,  120,  120,  120,   93,
 /*  1280 */    94,   95,   96,   97,   98,   99,  120,  101,  102,  103,
 /*  1290 */   104,  120,  106,  107,   66,  120,  110,  111,  112,   93,
 /*  1300 */    94,   95,   96,   97,   98,   99,  120,  101,  102,  103,
 /*  1310 */   104,  120,  106,  107,   66,  120,  110,  111,  112,  120,
 /*  1320 */   120,   93,   94,   95,   96,   97,   98,   99,  120,  101,
 /*  1330 */   102,  103,  104,  120,  106,  107,   66,  120,  110,  111,
 /*  1340 */   112,   93,   94,   95,   96,   97,   98,   99,  120,  101,
 /*  1350 */   102,  103,  104,  120,  106,  107,   66,  120,  110,  111,
 /*  1360 */   112,  120,  120,   93,   94,   95,   96,   97,   98,   99,
 /*  1370 */   120,  101,  102,  103,  104,  120,  106,  107,  120,   66,
 /*  1380 */   110,  111,  112,   93,   94,   95,   96,   97,   98,   99,
 /*  1390 */   120,  101,  102,  103,  104,  120,  106,  107,  120,   66,
 /*  1400 */   110,  111,  112,  120,  120,  120,   93,   94,   95,   96,
 /*  1410 */    97,   98,   99,  120,  101,  102,  103,  104,  120,  106,
 /*  1420 */   107,   66,  120,  110,  111,  112,   93,   94,   95,   96,
 /*  1430 */    97,   98,   99,  120,  101,  102,  103,  104,  120,  106,
 /*  1440 */   107,   66,  120,  110,  111,  112,  120,  120,   93,   94,
 /*  1450 */    95,   96,   97,   98,   99,  120,  101,  102,  103,  104,
 /*  1460 */   120,  106,  107,   66,  120,  110,  111,  112,   93,   94,
 /*  1470 */    95,   96,   97,   98,   99,  120,  101,  102,  103,  104,
 /*  1480 */   120,  106,  107,   66,  120,  110,  111,  112,  120,  120,
 /*  1490 */    93,   94,   95,   96,   97,   98,   99,  120,  101,  102,
 /*  1500 */   103,  104,  120,  106,  107,  120,   66,  110,  111,  112,
 /*  1510 */    93,   94,   95,   96,   97,   98,   99,  120,  101,  102,
 /*  1520 */   103,  104,  120,  106,  107,  120,   66,  110,  111,  112,
 /*  1530 */   120,  120,  120,   93,   94,   95,   96,   97,   98,   99,
 /*  1540 */   120,  101,  102,  103,  104,  120,  106,  107,   66,  120,
 /*  1550 */   110,  111,  112,   93,   94,   95,   96,   97,   98,   99,
 /*  1560 */   120,  101,  102,  103,  104,  120,  106,  107,  120,  120,
 /*  1570 */   110,  111,  112,  120,  120,   93,   94,   95,   96,   97,
 /*  1580 */    98,   99,  120,  101,  102,  103,  104,  120,  106,  107,
 /*  1590 */    12,  120,  110,  111,  112,  120,  120,  120,  120,   21,
 /*  1600 */   120,  120,  120,   94,   95,   96,   97,   98,   99,   31,
 /*  1610 */   101,  102,  103,  104,  120,  106,  107,   39,   40,  110,
 /*  1620 */   111,  112,   44,   45,  120,   47,   48,   49,   50,   51,
 /*  1630 */    52,   53,   54,   55,  120,   12,  120,  120,  120,  120,
 /*  1640 */   120,  120,  120,   94,   21,   96,   97,   98,   99,  120,
 /*  1650 */   101,  102,  103,  104,   31,  106,  107,  120,  120,  110,
 /*  1660 */   111,  112,   39,   40,  120,  120,  120,   44,   45,  120,
 /*  1670 */    47,   48,   49,   50,   51,   52,   53,   54,   55,   94,
 /*  1680 */   120,  120,   97,   98,   99,  120,  101,  102,  103,  104,
 /*  1690 */   120,  106,  107,  120,  120,  110,  111,  112,   94,  120,
 /*  1700 */   120,   97,   98,   99,  120,  101,  102,  103,  104,  120,
 /*  1710 */   106,  107,  120,  120,  110,  111,  112,
};
#define YY_SHIFT_USE_DFLT (-16)
#define YY_SHIFT_MAX 190
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2, 1014,   -2, 1014,
 /*    20 */  1096, 1141, 1578, 1196, 1623, 1623, 1623, 1623, 1623, 1623,
 /*    30 */  1623, 1623, 1623, 1623, 1623, 1623, 1623, 1623, 1623, 1623,
 /*    40 */  1623, 1623, 1623, 1623, 1623, 1116, 1116, 1116, 1116, 1116,
 /*    50 */   -10,  -10, 1116, 1116,  267, 1116, 1116, 1116, 1116,  359,
 /*    60 */   359,   76,  144,  226,  280,  280,   11,   46,   46,   46,
 /*    70 */    46,   50,   60,  295,  295,  370,  116,  306,  310,  306,
 /*    80 */   310,  -14,  175,  201,  175,  175,  175,  286,  336,  350,
 /*    90 */    50,  357,  368,  368,   60,  368,  368,   60,  368,  368,
 /*   100 */   368,  368,   60,  286,  467,  150,  352,  229,  -15,  294,
 /*   110 */   142,  366,  416,  432,  211,  220,  434,  441,  447,  448,
 /*   120 */   421,  422,  418,  419,  427,  429,  438,  454,  455,  421,
 /*   130 */   422,  419,  427,  429,  451,  458,  425,  426,  457,  470,
 /*   140 */   457,  471,  475,  476,  473,  492,  495,  493,  497,  447,
 /*   150 */   487,  499,  489,  486,  491,  488,  500,  508,  500,  520,
 /*   160 */   517,  509,  511,  515,  518,  519,  521,  524,  526,  525,
 /*   170 */   530,  527,  528,  529,  531,  532,  533,  534,  535,  536,
 /*   180 */   537,  516,  545,  538,  542,  563,  558,  570,  447,  447,
 /*   190 */   447,
};
#define YY_REDUCE_USE_DFLT (-78)
#define YY_REDUCE_MAX 103
static const short yy_reduce_ofst[] = {
 /*     0 */   -38,   12,   62,  112,  162,  212,  262,  312,  362,  412,
 /*    10 */   462,  512,  562,  612,  662,  712,  762,  809,  858,  897,
 /*    20 */   917,  984, 1008,  783, 1108, 1163, 1186, 1206, 1228, 1248,
 /*    30 */  1270, 1290, 1313, 1333, 1355, 1375, 1397, 1417, 1440, 1460,
 /*    40 */  1482, 1509, 1549, 1585, 1604,   -8,   40,   90,  140,  190,
 /*    50 */   265,  315,  -27,  -77,  105,   26,  126,  176,  223,  348,
 /*    60 */   360,   39,   14,  -65,   79,  129,  -70,   97,  159,  166,
 /*    70 */   221,  180,  245,  -17,  -17,   -3,   44,   30,   73,   30,
 /*    80 */    73,   57,   92,  119,  154,  171,  230,  153,  241,  307,
 /*    90 */   313,  303,  316,  317,  301,  321,  329,  301,  338,  354,
 /*   100 */   361,  364,  301,  275,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   307,  307,  307,  307,  307,  307,  307,  307,  307,  307,
 /*    10 */   307,  307,  307,  307,  307,  307,  307,  386,  307,  486,
 /*    20 */   486,  486,  473,  486,  486,  314,  486,  486,  486,  486,
 /*    30 */   486,  486,  486,  316,  318,  486,  486,  486,  486,  486,
 /*    40 */   486,  486,  486,  486,  486,  486,  486,  486,  486,  486,
 /*    50 */   374,  374,  486,  486,  486,  486,  486,  486,  486,  486,
 /*    60 */   486,  486,  486,  484,  335,  335,  486,  486,  486,  486,
 /*    70 */   486,  486,  378,  434,  433,  417,  484,  426,  432,  425,
 /*    80 */   431,  474,  486,  472,  486,  486,  486,  477,  331,  486,
 /*    90 */   486,  339,  486,  486,  486,  486,  486,  382,  486,  486,
 /*   100 */   486,  486,  381,  477,  447,  447,  486,  486,  486,  486,
 /*   110 */   486,  486,  486,  486,  486,  486,  486,  486,  485,  486,
 /*   120 */   409,  412,  418,  422,  424,  428,  387,  388,  389,  410,
 /*   130 */   411,  421,  423,  427,  452,  486,  486,  486,  486,  391,
 /*   140 */   452,  394,  395,  398,  486,  486,  486,  486,  486,  336,
 /*   150 */   486,  486,  486,  323,  486,  324,  326,  486,  325,  486,
 /*   160 */   486,  486,  486,  358,  350,  346,  344,  486,  486,  348,
 /*   170 */   486,  354,  352,  356,  366,  362,  360,  364,  370,  368,
 /*   180 */   372,  486,  486,  486,  486,  486,  486,  486,  481,  482,
 /*   190 */   483,  306,  308,  309,  305,  310,  313,  315,  408,  414,
 /*   200 */   415,  416,  437,  443,  444,  445,  446,  390,  401,  413,
 /*   210 */   429,  430,  435,  436,  439,  440,  441,  442,  438,  448,
 /*   220 */   452,  453,  454,  455,  456,  457,  458,  459,  460,  461,
 /*   230 */   465,  471,  463,  464,  469,  470,  468,  467,  462,  466,
 /*   240 */   402,  406,  407,  392,  393,  404,  405,  396,  397,  399,
 /*   250 */   400,  403,  449,  475,  317,  319,  320,  333,  334,  321,
 /*   260 */   322,  330,  329,  328,  327,  341,  342,  340,  332,  337,
 /*   270 */   343,  375,  345,  376,  347,  349,  377,  379,  380,  384,
 /*   280 */   385,  351,  353,  355,  357,  383,  359,  361,  363,  365,
 /*   290 */   367,  369,  371,  373,  478,  476,  450,  451,  419,  420,
 /*   300 */   311,  480,  312,  479,
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
  "DEF",           "LPAR",          "RPAR",          "AT",          
  "STAR_STAR",     "STAR",          "AMPER",         "EQUAL",       
  "COLON",         "BAR_BAR",       "AND_AND",       "NOT",         
  "LESS",          "XOR",           "BAR",           "AND",         
  "LSHIFT",        "RSHIFT",        "EQUAL_TILDA",   "PLUS",        
  "MINUS",         "DIV",           "DIV_DIV",       "PERCENT",     
  "TILDA",         "LBRACKET",      "RBRACKET",      "NUMBER",      
  "REGEXP",        "STRING",        "SYMBOL",        "NIL",         
  "TRUE",          "FALSE",         "LINE",          "LBRACE",      
  "RBRACE",        "EQUAL_GREATER",  "DO",            "EXCEPT",      
  "AS",            "error",         "module",        "stmts",       
  "stmt",          "func_def",      "expr",          "excepts",     
  "finally_opt",   "if_tail",       "decorators_opt",  "super_opt",   
  "names",         "dotted_names",  "dotted_name",   "else_opt",    
  "params",        "decorators",    "decorator",     "params_without_default",
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
 /*  17 */ "stmt ::= decorators_opt CLASS NAME super_opt NEWLINE stmts END",
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
 /*  33 */ "func_def ::= decorators_opt DEF NAME LPAR params RPAR stmts END",
 /*  34 */ "decorators_opt ::=",
 /*  35 */ "decorators_opt ::= decorators",
 /*  36 */ "decorators ::= decorator",
 /*  37 */ "decorators ::= decorators decorator",
 /*  38 */ "decorator ::= AT expr NEWLINE",
 /*  39 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  40 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param",
 /*  41 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param",
 /*  42 */ "params ::= params_without_default COMMA params_with_default COMMA block_param",
 /*  43 */ "params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param",
 /*  44 */ "params ::= params_without_default COMMA params_with_default COMMA var_param",
 /*  45 */ "params ::= params_without_default COMMA params_with_default COMMA kw_param",
 /*  46 */ "params ::= params_without_default COMMA params_with_default",
 /*  47 */ "params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  48 */ "params ::= params_without_default COMMA block_param COMMA var_param",
 /*  49 */ "params ::= params_without_default COMMA block_param COMMA kw_param",
 /*  50 */ "params ::= params_without_default COMMA block_param",
 /*  51 */ "params ::= params_without_default COMMA var_param COMMA kw_param",
 /*  52 */ "params ::= params_without_default COMMA var_param",
 /*  53 */ "params ::= params_without_default COMMA kw_param",
 /*  54 */ "params ::= params_without_default",
 /*  55 */ "params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  56 */ "params ::= params_with_default COMMA block_param COMMA var_param",
 /*  57 */ "params ::= params_with_default COMMA block_param COMMA kw_param",
 /*  58 */ "params ::= params_with_default COMMA block_param",
 /*  59 */ "params ::= params_with_default COMMA var_param COMMA kw_param",
 /*  60 */ "params ::= params_with_default COMMA var_param",
 /*  61 */ "params ::= params_with_default COMMA kw_param",
 /*  62 */ "params ::= params_with_default",
 /*  63 */ "params ::= block_param COMMA var_param COMMA kw_param",
 /*  64 */ "params ::= block_param COMMA var_param",
 /*  65 */ "params ::= block_param COMMA kw_param",
 /*  66 */ "params ::= block_param",
 /*  67 */ "params ::= var_param COMMA kw_param",
 /*  68 */ "params ::= var_param",
 /*  69 */ "params ::= kw_param",
 /*  70 */ "params ::=",
 /*  71 */ "kw_param ::= STAR_STAR NAME",
 /*  72 */ "var_param ::= STAR NAME",
 /*  73 */ "block_param ::= AMPER NAME param_default_opt",
 /*  74 */ "param_default_opt ::=",
 /*  75 */ "param_default_opt ::= param_default",
 /*  76 */ "param_default ::= EQUAL expr",
 /*  77 */ "params_without_default ::= NAME",
 /*  78 */ "params_without_default ::= params_without_default COMMA NAME",
 /*  79 */ "params_with_default ::= param_with_default",
 /*  80 */ "params_with_default ::= params_with_default COMMA param_with_default",
 /*  81 */ "param_with_default ::= NAME param_default",
 /*  82 */ "args ::=",
 /*  83 */ "args ::= posargs",
 /*  84 */ "args ::= posargs COMMA kwargs",
 /*  85 */ "args ::= posargs COMMA kwargs COMMA vararg",
 /*  86 */ "args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg",
 /*  87 */ "args ::= posargs COMMA vararg",
 /*  88 */ "args ::= posargs COMMA vararg COMMA varkwarg",
 /*  89 */ "args ::= posargs COMMA varkwarg",
 /*  90 */ "args ::= kwargs",
 /*  91 */ "args ::= kwargs COMMA vararg",
 /*  92 */ "args ::= kwargs COMMA vararg COMMA varkwarg",
 /*  93 */ "args ::= kwargs COMMA varkwarg",
 /*  94 */ "args ::= vararg",
 /*  95 */ "args ::= vararg COMMA varkwarg",
 /*  96 */ "args ::= varkwarg",
 /*  97 */ "varkwarg ::= STAR_STAR expr",
 /*  98 */ "vararg ::= STAR expr",
 /*  99 */ "posargs ::= expr",
 /* 100 */ "posargs ::= posargs COMMA expr",
 /* 101 */ "kwargs ::= kwarg",
 /* 102 */ "kwargs ::= kwargs COMMA kwarg",
 /* 103 */ "kwarg ::= NAME COLON expr",
 /* 104 */ "expr ::= assign_expr",
 /* 105 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /* 106 */ "assign_expr ::= logical_or_expr",
 /* 107 */ "logical_or_expr ::= logical_and_expr",
 /* 108 */ "logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr",
 /* 109 */ "logical_and_expr ::= not_expr",
 /* 110 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /* 111 */ "not_expr ::= comparison",
 /* 112 */ "not_expr ::= NOT not_expr",
 /* 113 */ "comparison ::= xor_expr",
 /* 114 */ "comparison ::= xor_expr comp_op xor_expr",
 /* 115 */ "comp_op ::= LESS",
 /* 116 */ "comp_op ::= GREATER",
 /* 117 */ "xor_expr ::= or_expr",
 /* 118 */ "xor_expr ::= xor_expr XOR or_expr",
 /* 119 */ "or_expr ::= and_expr",
 /* 120 */ "or_expr ::= or_expr BAR and_expr",
 /* 121 */ "and_expr ::= shift_expr",
 /* 122 */ "and_expr ::= and_expr AND shift_expr",
 /* 123 */ "shift_expr ::= match_expr",
 /* 124 */ "shift_expr ::= shift_expr shift_op match_expr",
 /* 125 */ "shift_op ::= LSHIFT",
 /* 126 */ "shift_op ::= RSHIFT",
 /* 127 */ "match_expr ::= arith_expr",
 /* 128 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 129 */ "arith_expr ::= term",
 /* 130 */ "arith_expr ::= arith_expr arith_op term",
 /* 131 */ "arith_op ::= PLUS",
 /* 132 */ "arith_op ::= MINUS",
 /* 133 */ "term ::= term term_op factor",
 /* 134 */ "term ::= factor",
 /* 135 */ "term_op ::= STAR",
 /* 136 */ "term_op ::= DIV",
 /* 137 */ "term_op ::= DIV_DIV",
 /* 138 */ "term_op ::= PERCENT",
 /* 139 */ "factor ::= PLUS factor",
 /* 140 */ "factor ::= MINUS factor",
 /* 141 */ "factor ::= TILDA factor",
 /* 142 */ "factor ::= power",
 /* 143 */ "power ::= postfix_expr",
 /* 144 */ "postfix_expr ::= atom",
 /* 145 */ "postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt",
 /* 146 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 147 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 148 */ "atom ::= NAME",
 /* 149 */ "atom ::= NUMBER",
 /* 150 */ "atom ::= REGEXP",
 /* 151 */ "atom ::= STRING",
 /* 152 */ "atom ::= SYMBOL",
 /* 153 */ "atom ::= NIL",
 /* 154 */ "atom ::= TRUE",
 /* 155 */ "atom ::= FALSE",
 /* 156 */ "atom ::= LINE",
 /* 157 */ "atom ::= LBRACKET exprs RBRACKET",
 /* 158 */ "atom ::= LBRACKET RBRACKET",
 /* 159 */ "atom ::= LBRACE RBRACE",
 /* 160 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 161 */ "atom ::= LPAR expr RPAR",
 /* 162 */ "exprs ::= expr",
 /* 163 */ "exprs ::= exprs COMMA expr",
 /* 164 */ "dict_elems ::= dict_elem",
 /* 165 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 166 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 167 */ "dict_elem ::= NAME COLON expr",
 /* 168 */ "comma_opt ::=",
 /* 169 */ "comma_opt ::= COMMA",
 /* 170 */ "blockarg_opt ::=",
 /* 171 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 172 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 173 */ "blockarg_params_opt ::=",
 /* 174 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 175 */ "excepts ::= except",
 /* 176 */ "excepts ::= excepts except",
 /* 177 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 178 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 179 */ "except ::= EXCEPT NEWLINE stmts",
 /* 180 */ "finally_opt ::=",
 /* 181 */ "finally_opt ::= FINALLY stmts",
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
  { 62, 1 },
  { 63, 1 },
  { 63, 3 },
  { 64, 0 },
  { 64, 1 },
  { 64, 1 },
  { 64, 7 },
  { 64, 5 },
  { 64, 5 },
  { 64, 5 },
  { 64, 1 },
  { 64, 2 },
  { 64, 1 },
  { 64, 2 },
  { 64, 1 },
  { 64, 2 },
  { 64, 6 },
  { 64, 7 },
  { 64, 4 },
  { 64, 2 },
  { 64, 2 },
  { 73, 1 },
  { 73, 3 },
  { 74, 1 },
  { 74, 3 },
  { 72, 1 },
  { 72, 3 },
  { 71, 0 },
  { 71, 2 },
  { 69, 1 },
  { 69, 5 },
  { 75, 0 },
  { 75, 2 },
  { 65, 8 },
  { 70, 0 },
  { 70, 1 },
  { 77, 1 },
  { 77, 2 },
  { 78, 3 },
  { 76, 9 },
  { 76, 7 },
  { 76, 7 },
  { 76, 5 },
  { 76, 7 },
  { 76, 5 },
  { 76, 5 },
  { 76, 3 },
  { 76, 7 },
  { 76, 5 },
  { 76, 5 },
  { 76, 3 },
  { 76, 5 },
  { 76, 3 },
  { 76, 3 },
  { 76, 1 },
  { 76, 7 },
  { 76, 5 },
  { 76, 5 },
  { 76, 3 },
  { 76, 5 },
  { 76, 3 },
  { 76, 3 },
  { 76, 1 },
  { 76, 5 },
  { 76, 3 },
  { 76, 3 },
  { 76, 1 },
  { 76, 3 },
  { 76, 1 },
  { 76, 1 },
  { 76, 0 },
  { 83, 2 },
  { 82, 2 },
  { 81, 3 },
  { 84, 0 },
  { 84, 1 },
  { 85, 2 },
  { 79, 1 },
  { 79, 3 },
  { 80, 1 },
  { 80, 3 },
  { 86, 2 },
  { 87, 0 },
  { 87, 1 },
  { 87, 3 },
  { 87, 5 },
  { 87, 7 },
  { 87, 3 },
  { 87, 5 },
  { 87, 3 },
  { 87, 1 },
  { 87, 3 },
  { 87, 5 },
  { 87, 3 },
  { 87, 1 },
  { 87, 3 },
  { 87, 1 },
  { 91, 2 },
  { 90, 2 },
  { 88, 1 },
  { 88, 3 },
  { 89, 1 },
  { 89, 3 },
  { 92, 3 },
  { 66, 1 },
  { 93, 3 },
  { 93, 1 },
  { 95, 1 },
  { 95, 3 },
  { 96, 1 },
  { 96, 3 },
  { 97, 1 },
  { 97, 2 },
  { 98, 1 },
  { 98, 3 },
  { 100, 1 },
  { 100, 1 },
  { 99, 1 },
  { 99, 3 },
  { 101, 1 },
  { 101, 3 },
  { 102, 1 },
  { 102, 3 },
  { 103, 1 },
  { 103, 3 },
  { 105, 1 },
  { 105, 1 },
  { 104, 1 },
  { 104, 3 },
  { 106, 1 },
  { 106, 3 },
  { 108, 1 },
  { 108, 1 },
  { 107, 3 },
  { 107, 1 },
  { 109, 1 },
  { 109, 1 },
  { 109, 1 },
  { 109, 1 },
  { 110, 2 },
  { 110, 2 },
  { 110, 2 },
  { 110, 1 },
  { 111, 1 },
  { 94, 1 },
  { 94, 5 },
  { 94, 4 },
  { 94, 3 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 3 },
  { 112, 2 },
  { 112, 2 },
  { 112, 4 },
  { 112, 3 },
  { 114, 1 },
  { 114, 3 },
  { 115, 1 },
  { 115, 3 },
  { 117, 3 },
  { 117, 3 },
  { 116, 0 },
  { 116, 1 },
  { 113, 0 },
  { 113, 5 },
  { 113, 5 },
  { 118, 0 },
  { 118, 3 },
  { 67, 1 },
  { 67, 2 },
  { 119, 6 },
  { 119, 4 },
  { 119, 3 },
  { 68, 0 },
  { 68, 2 },
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
#line 705 "parser.y"
{
    *pval = yymsp[0].minor.yy127;
}
#line 2163 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 21: /* dotted_names ::= dotted_name */
      case 36: /* decorators ::= decorator */
      case 79: /* params_with_default ::= param_with_default */
      case 99: /* posargs ::= expr */
      case 101: /* kwargs ::= kwarg */
      case 162: /* exprs ::= expr */
      case 164: /* dict_elems ::= dict_elem */
      case 175: /* excepts ::= except */
#line 709 "parser.y"
{
    yygotominor.yy127 = make_array_with(env, yymsp[0].minor.yy127);
}
#line 2178 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 22: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 80: /* params_with_default ::= params_with_default COMMA param_with_default */
#line 712 "parser.y"
{
    yygotominor.yy127 = Array_push(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2187 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 27: /* super_opt ::= */
      case 31: /* else_opt ::= */
      case 34: /* decorators_opt ::= */
      case 74: /* param_default_opt ::= */
      case 82: /* args ::= */
      case 168: /* comma_opt ::= */
      case 170: /* blockarg_opt ::= */
      case 173: /* blockarg_params_opt ::= */
      case 180: /* finally_opt ::= */
#line 716 "parser.y"
{
    yygotominor.yy127 = YNIL;
}
#line 2203 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 5: /* stmt ::= expr */
      case 28: /* super_opt ::= GREATER expr */
      case 29: /* if_tail ::= else_opt */
      case 32: /* else_opt ::= ELSE stmts */
      case 35: /* decorators_opt ::= decorators */
      case 75: /* param_default_opt ::= param_default */
      case 76: /* param_default ::= EQUAL expr */
      case 97: /* varkwarg ::= STAR_STAR expr */
      case 98: /* vararg ::= STAR expr */
      case 104: /* expr ::= assign_expr */
      case 106: /* assign_expr ::= logical_or_expr */
      case 107: /* logical_or_expr ::= logical_and_expr */
      case 109: /* logical_and_expr ::= not_expr */
      case 111: /* not_expr ::= comparison */
      case 113: /* comparison ::= xor_expr */
      case 117: /* xor_expr ::= or_expr */
      case 119: /* or_expr ::= and_expr */
      case 121: /* and_expr ::= shift_expr */
      case 123: /* shift_expr ::= match_expr */
      case 127: /* match_expr ::= arith_expr */
      case 129: /* arith_expr ::= term */
      case 134: /* term ::= factor */
      case 142: /* factor ::= power */
      case 143: /* power ::= postfix_expr */
      case 144: /* postfix_expr ::= atom */
      case 181: /* finally_opt ::= FINALLY stmts */
#line 719 "parser.y"
{
    yygotominor.yy127 = yymsp[0].minor.yy127;
}
#line 2236 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 725 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy127 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2244 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 729 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[-1].minor.yy127);
}
#line 2252 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 733 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = Finally_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2260 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 737 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = While_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2268 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 741 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Break_new(env, lineno, YNIL);
}
#line 2276 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 745 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Break_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2284 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 749 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Next_new(env, lineno, YNIL);
}
#line 2292 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 753 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Next_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2300 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 757 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Return_new(env, lineno, YNIL);
}
#line 2308 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 761 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Return_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2316 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 765 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy127 = If_new(env, lineno, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2324 "parser.c"
        break;
      case 17: /* stmt ::= decorators_opt CLASS NAME super_opt NEWLINE stmts END */
#line 769 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy127 = Klass_new(env, lineno, yymsp[-6].minor.yy127, id, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2333 "parser.c"
        break;
      case 18: /* stmt ::= MODULE NAME stmts END */
#line 774 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    yygotominor.yy127 = Module_new(env, lineno, id, yymsp[-1].minor.yy127);
}
#line 2342 "parser.c"
        break;
      case 19: /* stmt ::= NONLOCAL names */
#line 779 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Nonlocal_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2350 "parser.c"
        break;
      case 20: /* stmt ::= IMPORT dotted_names */
#line 783 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Import_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2358 "parser.c"
        break;
      case 23: /* dotted_name ::= NAME */
      case 25: /* names ::= NAME */
#line 795 "parser.y"
{
    yygotominor.yy127 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2366 "parser.c"
        break;
      case 24: /* dotted_name ::= dotted_name DOT NAME */
      case 26: /* names ::= names COMMA NAME */
#line 798 "parser.y"
{
    yygotominor.yy127 = Array_push_token_id(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy0);
}
#line 2374 "parser.c"
        break;
      case 30: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 819 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127, yymsp[0].minor.yy127);
    yygotominor.yy127 = make_array_with(env, node);
}
#line 2383 "parser.c"
        break;
      case 33: /* func_def ::= decorators_opt DEF NAME LPAR params RPAR stmts END */
#line 832 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy127 = FuncDef_new(env, lineno, yymsp[-7].minor.yy127, id, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2392 "parser.c"
        break;
      case 37: /* decorators ::= decorators decorator */
      case 176: /* excepts ::= excepts except */
#line 848 "parser.y"
{
    yygotominor.yy127 = Array_push(env, yymsp[-1].minor.yy127, yymsp[0].minor.yy127);
}
#line 2400 "parser.c"
        break;
      case 38: /* decorator ::= AT expr NEWLINE */
      case 161: /* atom ::= LPAR expr RPAR */
      case 174: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 852 "parser.y"
{
    yygotominor.yy127 = yymsp[-1].minor.yy127;
}
#line 2409 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 856 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-8].minor.yy127, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2416 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 859 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2423 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 862 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2430 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 865 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2437 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 868 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2444 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 871 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2451 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 874 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2458 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA params_with_default */
#line 877 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL, YNIL);
}
#line 2465 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 880 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2472 "parser.c"
        break;
      case 48: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 883 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2479 "parser.c"
        break;
      case 49: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 886 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2486 "parser.c"
        break;
      case 50: /* params ::= params_without_default COMMA block_param */
#line 889 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2493 "parser.c"
        break;
      case 51: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 892 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2500 "parser.c"
        break;
      case 52: /* params ::= params_without_default COMMA var_param */
#line 895 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2507 "parser.c"
        break;
      case 53: /* params ::= params_without_default COMMA kw_param */
#line 898 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, YNIL, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2514 "parser.c"
        break;
      case 54: /* params ::= params_without_default */
#line 901 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[0].minor.yy127, YNIL, YNIL, YNIL, YNIL);
}
#line 2521 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 904 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2528 "parser.c"
        break;
      case 56: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 907 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2535 "parser.c"
        break;
      case 57: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 910 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2542 "parser.c"
        break;
      case 58: /* params ::= params_with_default COMMA block_param */
#line 913 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2549 "parser.c"
        break;
      case 59: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 916 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2556 "parser.c"
        break;
      case 60: /* params ::= params_with_default COMMA var_param */
#line 919 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2563 "parser.c"
        break;
      case 61: /* params ::= params_with_default COMMA kw_param */
#line 922 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2570 "parser.c"
        break;
      case 62: /* params ::= params_with_default */
#line 925 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[0].minor.yy127, YNIL, YNIL, YNIL);
}
#line 2577 "parser.c"
        break;
      case 63: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 928 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2584 "parser.c"
        break;
      case 64: /* params ::= block_param COMMA var_param */
#line 931 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2591 "parser.c"
        break;
      case 65: /* params ::= block_param COMMA kw_param */
#line 934 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2598 "parser.c"
        break;
      case 66: /* params ::= block_param */
#line 937 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2605 "parser.c"
        break;
      case 67: /* params ::= var_param COMMA kw_param */
#line 940 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2612 "parser.c"
        break;
      case 68: /* params ::= var_param */
#line 943 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2619 "parser.c"
        break;
      case 69: /* params ::= kw_param */
#line 946 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2626 "parser.c"
        break;
      case 70: /* params ::= */
#line 949 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2633 "parser.c"
        break;
      case 71: /* kw_param ::= STAR_STAR NAME */
#line 953 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2642 "parser.c"
        break;
      case 72: /* var_param ::= STAR NAME */
#line 959 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2651 "parser.c"
        break;
      case 73: /* block_param ::= AMPER NAME param_default_opt */
#line 965 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy127);
}
#line 2660 "parser.c"
        break;
      case 77: /* params_without_default ::= NAME */
#line 982 "parser.y"
{
    yygotominor.yy127 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy127, lineno, id, YNIL);
}
#line 2670 "parser.c"
        break;
      case 78: /* params_without_default ::= params_without_default COMMA NAME */
#line 988 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy127, lineno, id, YNIL);
    yygotominor.yy127 = yymsp[-2].minor.yy127;
}
#line 2680 "parser.c"
        break;
      case 81: /* param_with_default ::= NAME param_default */
#line 1002 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy127);
}
#line 2689 "parser.c"
        break;
      case 83: /* args ::= posargs */
#line 1011 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[0].minor.yy127, YNIL, YNIL, YNIL);
}
#line 2697 "parser.c"
        break;
      case 84: /* args ::= posargs COMMA kwargs */
#line 1015 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2705 "parser.c"
        break;
      case 85: /* args ::= posargs COMMA kwargs COMMA vararg */
#line 1019 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2713 "parser.c"
        break;
      case 86: /* args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg */
#line 1023 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-6].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2721 "parser.c"
        break;
      case 87: /* args ::= posargs COMMA vararg */
#line 1027 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2729 "parser.c"
        break;
      case 88: /* args ::= posargs COMMA vararg COMMA varkwarg */
#line 1031 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2737 "parser.c"
        break;
      case 89: /* args ::= posargs COMMA varkwarg */
#line 1035 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2745 "parser.c"
        break;
      case 90: /* args ::= kwargs */
#line 1039 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, YNIL, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2753 "parser.c"
        break;
      case 91: /* args ::= kwargs COMMA vararg */
#line 1043 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2761 "parser.c"
        break;
      case 92: /* args ::= kwargs COMMA vararg COMMA varkwarg */
#line 1047 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2769 "parser.c"
        break;
      case 93: /* args ::= kwargs COMMA varkwarg */
#line 1051 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2777 "parser.c"
        break;
      case 94: /* args ::= vararg */
#line 1055 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy127);
    yygotominor.yy127 = Args_new(env, lineno, YNIL, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2785 "parser.c"
        break;
      case 95: /* args ::= vararg COMMA varkwarg */
#line 1059 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    yygotominor.yy127 = Args_new(env, lineno, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2793 "parser.c"
        break;
      case 96: /* args ::= varkwarg */
#line 1063 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy127);
    yygotominor.yy127 = Args_new(env, lineno, YNIL, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2801 "parser.c"
        break;
      case 100: /* posargs ::= posargs COMMA expr */
      case 102: /* kwargs ::= kwargs COMMA kwarg */
      case 163: /* exprs ::= exprs COMMA expr */
      case 165: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 1079 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
    yygotominor.yy127 = yymsp[-2].minor.yy127;
}
#line 2812 "parser.c"
        break;
      case 103: /* kwarg ::= NAME COLON expr */
#line 1092 "parser.y"
{
    yygotominor.yy127 = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(yymsp[-2].minor.yy0));
    PTR_AS(YogNode, yygotominor.yy127)->u.kwarg.name = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    PTR_AS(YogNode, yygotominor.yy127)->u.kwarg.value = yymsp[0].minor.yy127;
}
#line 2821 "parser.c"
        break;
      case 105: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 1102 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    yygotominor.yy127 = Assign_new(env, lineno, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2829 "parser.c"
        break;
      case 108: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 1113 "parser.y"
{
    yygotominor.yy127 = YogNode_new(env, NODE_LOGICAL_OR, NODE_LINENO(yymsp[-2].minor.yy127));
    NODE(yygotominor.yy127)->u.logical_or.left = yymsp[-2].minor.yy127;
    NODE(yygotominor.yy127)->u.logical_or.right = yymsp[0].minor.yy127;
}
#line 2838 "parser.c"
        break;
      case 110: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 1122 "parser.y"
{
    yygotominor.yy127 = YogNode_new(env, NODE_LOGICAL_AND, NODE_LINENO(yymsp[-2].minor.yy127));
    NODE(yygotominor.yy127)->u.logical_and.left = yymsp[-2].minor.yy127;
    NODE(yygotominor.yy127)->u.logical_and.right = yymsp[0].minor.yy127;
}
#line 2847 "parser.c"
        break;
      case 112: /* not_expr ::= NOT not_expr */
#line 1131 "parser.y"
{
    yygotominor.yy127 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy127)->u.not.expr = yymsp[0].minor.yy127;
}
#line 2855 "parser.c"
        break;
      case 114: /* comparison ::= xor_expr comp_op xor_expr */
#line 1139 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy127)->u.id;
    yygotominor.yy127 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy127, id, yymsp[0].minor.yy127);
}
#line 2864 "parser.c"
        break;
      case 115: /* comp_op ::= LESS */
      case 116: /* comp_op ::= GREATER */
      case 169: /* comma_opt ::= COMMA */
#line 1145 "parser.y"
{
    yygotominor.yy127 = yymsp[0].minor.yy0;
}
#line 2873 "parser.c"
        break;
      case 118: /* xor_expr ::= xor_expr XOR or_expr */
      case 120: /* or_expr ::= or_expr BAR and_expr */
      case 122: /* and_expr ::= and_expr AND shift_expr */
#line 1155 "parser.y"
{
    yygotominor.yy127 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy127), yymsp[-2].minor.yy127, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy127);
}
#line 2882 "parser.c"
        break;
      case 124: /* shift_expr ::= shift_expr shift_op match_expr */
      case 130: /* arith_expr ::= arith_expr arith_op term */
      case 133: /* term ::= term term_op factor */
#line 1176 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    yygotominor.yy127 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy127, VAL2ID(yymsp[-1].minor.yy127), yymsp[0].minor.yy127);
}
#line 2892 "parser.c"
        break;
      case 125: /* shift_op ::= LSHIFT */
      case 126: /* shift_op ::= RSHIFT */
      case 131: /* arith_op ::= PLUS */
      case 132: /* arith_op ::= MINUS */
      case 135: /* term_op ::= STAR */
      case 136: /* term_op ::= DIV */
      case 137: /* term_op ::= DIV_DIV */
      case 138: /* term_op ::= PERCENT */
#line 1181 "parser.y"
{
    yygotominor.yy127 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2906 "parser.c"
        break;
      case 128: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1191 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy127 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy127, id, yymsp[0].minor.yy127);
}
#line 2915 "parser.c"
        break;
      case 139: /* factor ::= PLUS factor */
#line 1233 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy127 = FuncCall_new3(env, lineno, yymsp[0].minor.yy127, id);
}
#line 2924 "parser.c"
        break;
      case 140: /* factor ::= MINUS factor */
#line 1238 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy127 = FuncCall_new3(env, lineno, yymsp[0].minor.yy127, id);
}
#line 2933 "parser.c"
        break;
      case 141: /* factor ::= TILDA factor */
#line 1243 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy127 = FuncCall_new3(env, lineno, yymsp[0].minor.yy127, id);
}
#line 2942 "parser.c"
        break;
      case 145: /* postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt */
#line 1259 "parser.y"
{
    yygotominor.yy127 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy127), yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2949 "parser.c"
        break;
      case 146: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1262 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy127);
    yygotominor.yy127 = Subscript_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2957 "parser.c"
        break;
      case 147: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1266 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Attr_new(env, lineno, yymsp[-2].minor.yy127, id);
}
#line 2966 "parser.c"
        break;
      case 148: /* atom ::= NAME */
#line 1272 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Variable_new(env, lineno, id);
}
#line 2975 "parser.c"
        break;
      case 149: /* atom ::= NUMBER */
      case 150: /* atom ::= REGEXP */
      case 151: /* atom ::= STRING */
      case 152: /* atom ::= SYMBOL */
#line 1277 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy127 = Literal_new(env, lineno, val);
}
#line 2987 "parser.c"
        break;
      case 153: /* atom ::= NIL */
#line 1297 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Literal_new(env, lineno, YNIL);
}
#line 2995 "parser.c"
        break;
      case 154: /* atom ::= TRUE */
#line 1301 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Literal_new(env, lineno, YTRUE);
}
#line 3003 "parser.c"
        break;
      case 155: /* atom ::= FALSE */
#line 1305 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Literal_new(env, lineno, YFALSE);
}
#line 3011 "parser.c"
        break;
      case 156: /* atom ::= LINE */
#line 1309 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy127 = Literal_new(env, lineno, val);
}
#line 3020 "parser.c"
        break;
      case 157: /* atom ::= LBRACKET exprs RBRACKET */
#line 1314 "parser.y"
{
    yygotominor.yy127 = Array_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy127);
}
#line 3027 "parser.c"
        break;
      case 158: /* atom ::= LBRACKET RBRACKET */
#line 1317 "parser.y"
{
    yygotominor.yy127 = Array_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3034 "parser.c"
        break;
      case 159: /* atom ::= LBRACE RBRACE */
#line 1320 "parser.y"
{
    yygotominor.yy127 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3041 "parser.c"
        break;
      case 160: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1323 "parser.y"
{
    yygotominor.yy127 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy127);
}
#line 3048 "parser.c"
        break;
      case 166: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1345 "parser.y"
{
    yygotominor.yy127 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy127), yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 3055 "parser.c"
        break;
      case 167: /* dict_elem ::= NAME COLON expr */
#line 1348 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy127 = DictElem_new(env, lineno, var, yymsp[0].minor.yy127);
}
#line 3065 "parser.c"
        break;
      case 171: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 172: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1365 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = BlockArg_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 3074 "parser.c"
        break;
      case 177: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1388 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy127 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy127, id, yymsp[0].minor.yy127);
}
#line 3084 "parser.c"
        break;
      case 178: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1394 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy127 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy127, NO_EXC_VAR, yymsp[0].minor.yy127);
}
#line 3092 "parser.c"
        break;
      case 179: /* except ::= EXCEPT NEWLINE stmts */
#line 1398 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy127 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy127);
}
#line 3100 "parser.c"
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
