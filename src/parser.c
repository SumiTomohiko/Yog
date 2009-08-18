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
    case NODE_SET:
        KEEP(set.elems);
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
    while (YogLexer_next_token(env, lexer, filename, &token)) {
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

static YogVal
Set_new(YogEnv* env, uint_t lineno, YogVal elems)
{
    SAVE_ARG(env, elems);
    YogVal set = YUNDEF;
    PUSH_LOCAL(env, set);

    set = YogNode_new(env, NODE_SET, lineno);
    PTR_AS(YogNode, set)->u.set.elems = elems;

    RETURN(env, set);
}

static YogVal
AugmentedAssign_new(YogEnv* env, uint_t lineno, YogVal left, ID name, YogVal right)
{
    SAVE_ARGS2(env, left, right);
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    expr = FuncCall_new2(env, lineno, left, name, right);
    assign = Assign_new(env, lineno, left, expr);

    RETURN(env, assign);
}

static YogVal
LogicalOr_new(YogEnv* env, uint_t lineno, YogVal left, YogVal right)
{
    SAVE_ARGS2(env, left, right);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    node = YogNode_new(env, NODE_LOGICAL_OR, lineno);
    NODE(node)->u.logical_or.left = left;
    NODE(node)->u.logical_or.right = right;

    RETURN(env, node);
}

static YogVal
LogicalAnd_new(YogEnv* env, uint_t lineno, YogVal left, YogVal right)
{
    SAVE_ARGS2(env, left, right);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    node = YogNode_new(env, NODE_LOGICAL_AND, lineno);
    NODE(node)->u.logical_and.left = left;
    NODE(node)->u.logical_and.right = right;

    RETURN(env, node);
}

#define TOKEN(token)            PTR_AS(YogToken, (token))
#define TOKEN_ID(token)         TOKEN((token))->u.id
#define TOKEN_LINENO(token)     TOKEN((token))->lineno
#define NODE_LINENO(node)       PTR_AS(YogNode, (node))->lineno
#line 767 "parser.c"
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
#define YYNOCODE 136
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
#define YYNSTATE 327
#define YYNRULE 199
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
 /*     0 */     1,  275,   67,  107,   24,   25,   33,   34,   35,  361,
 /*    10 */   218,  158,   94,   75,  109,  173,  174,  176,  361,   28,
 /*    20 */   127,   37,  133,  134,   84,  135,  108,   85,   79,   92,
 /*    30 */   109,  236,  215,  217,  148,  259,  252,   39,  527,  110,
 /*    40 */   203,  201,  202,   47,   78,  144,  115,  236,  215,  217,
 /*    50 */   194,   59,   60,   96,  278,  326,   61,   21,   31,  219,
 /*    60 */   220,  221,  222,  223,  224,  225,  226,   20,   42,  207,
 /*    70 */    77,  131,   58,  132,  227,  209,   80,  156,  133,  134,
 /*    80 */    84,  135,  109,   85,   79,  310,  166,  236,  215,  217,
 /*    90 */    66,  203,  201,  202,   48,   83,   79,  115,  109,  236,
 /*   100 */   215,  217,  324,  122,   96,  278,  128,  134,   84,  135,
 /*   110 */    18,   85,   79,  309,   49,  236,  215,  217,  173,  174,
 /*   120 */   207,   77,  131,   52,  132,  227,  209,   80,   56,  133,
 /*   130 */   134,   84,  135,   22,   85,   79,  172,  283,  236,  215,
 /*   140 */   217,   81,  203,  201,  202,  178,  293,  168,  115,  109,
 /*   150 */   169,  180,  184,  186,  304,   96,  278,  296,  129,   84,
 /*   160 */   135,   18,   85,   79,  232,    3,  236,  215,  217,  161,
 /*   170 */   164,  207,   77,  131,   26,  132,  227,  209,   80,  306,
 /*   180 */   133,  134,   84,  135,   18,   85,   79,  205,   18,  236,
 /*   190 */   215,  217,   16,  263,  233,  234,  235,  109,  123,  203,
 /*   200 */   201,  202,  182,  298,  138,  115,  109,   82,  135,  144,
 /*   210 */    85,   79,   96,  278,  236,  215,  217,  130,  237,   85,
 /*   220 */    79,   30,   31,  236,  215,  217,  185,  302,  207,   77,
 /*   230 */   131,   30,  132,  227,  209,   80,  109,  133,  134,   84,
 /*   240 */   135,   18,   85,   79,  264,  102,  236,  215,  217,  111,
 /*   250 */   203,  201,  202,  211,  215,  217,  115,  173,  174,  176,
 /*   260 */    23,  190,   26,   96,  278,   17,  187,  249,   62,  169,
 /*   270 */   180,  184,  186,  304,  228,  229,  296,  287,  288,  207,
 /*   280 */    77,  131,  254,  132,  227,  209,   80,  258,  133,  134,
 /*   290 */    84,  135,   14,   85,   79,  153,  269,  236,  215,  217,
 /*   300 */   109,  268,  268,   40,  230,  231,  114,  203,  201,  202,
 /*   310 */   171,  175,  286,  115,  109,  290,  244,  212,  215,  217,
 /*   320 */    96,  278,  327,   18,  170,  177,  179,  295,  260,  151,
 /*   330 */   296,  213,  215,  217,   54,   38,  207,   77,  131,  157,
 /*   340 */   132,  227,  209,   80,  109,  133,  134,   84,  135,  109,
 /*   350 */    85,   79,  273,  159,  236,  215,  217,   68,  203,  201,
 /*   360 */   202,  214,  215,  217,  115,  195,  216,  215,  217,   99,
 /*   370 */   162,   96,  278,    2,  143,    3,  252,   18,   37,    8,
 /*   380 */   276,  173,  174,  176,  173,  291,  281,  207,   77,  131,
 /*   390 */   285,  132,  227,  209,   80,   36,  133,  134,   84,  135,
 /*   400 */    18,   85,   79,  270,  292,  236,  215,  217,  181,  183,
 /*   410 */   300,  294,  297,  290,   69,  203,  201,  202,  299,   18,
 /*   420 */   301,  115,  271,   18,   18,  303,  280,  325,   96,  278,
 /*   430 */   188,  204,   18,    4,   45,   50,   46,   49,   51,   27,
 /*   440 */    29,  238,  241,   23,  207,   77,  131,   55,  132,  227,
 /*   450 */   209,   80,   19,  133,  134,   84,  135,   70,   85,   79,
 /*   460 */    88,   32,  236,  215,  217,  155,  203,  201,  202,   89,
 /*   470 */    65,   90,  115,   91,   86,    5,    6,  267,    7,   96,
 /*   480 */   278,   93,    9,   10,  160,  277,   95,   53,  272,  163,
 /*   490 */   274,  167,  282,   12,  305,  207,   77,  131,   57,  132,
 /*   500 */   227,  209,   80,   63,  133,  134,   84,  135,   11,   85,
 /*   510 */    79,  284,  323,  236,  215,  217,   71,   97,   98,   76,
 /*   520 */    72,  100,  116,  203,  201,  202,  101,   64,   73,  115,
 /*   530 */   103,  104,   74,  105,  106,  308,   96,  278,  196,   13,
 /*   540 */   528,  528,  528,  307,  528,  528,  528,  528,  528,  528,
 /*   550 */   528,  528,  207,   77,  131,  528,  132,  227,  209,   80,
 /*   560 */   528,  133,  134,   84,  135,  528,   85,   79,  528,  528,
 /*   570 */   236,  215,  217,  117,  203,  201,  202,  528,  528,  528,
 /*   580 */   115,  528,  528,  528,  528,  528,  528,   96,  278,  528,
 /*   590 */   528,  528,  528,  528,  528,  528,  528,  528,  528,  528,
 /*   600 */   528,  528,  528,  207,   77,  131,  528,  132,  227,  209,
 /*   610 */    80,  528,  133,  134,   84,  135,  528,   85,   79,  528,
 /*   620 */   528,  236,  215,  217,  528,  528,  528,  528,  528,  528,
 /*   630 */   118,  203,  201,  202,  528,  528,  528,  115,  528,  528,
 /*   640 */   528,  528,  528,  528,   96,  278,  528,  528,  528,  528,
 /*   650 */   528,  528,  528,  528,  528,  528,  528,  528,  528,  528,
 /*   660 */   207,   77,  131,  528,  132,  227,  209,   80,  528,  133,
 /*   670 */   134,   84,  135,  528,   85,   79,  528,  528,  236,  215,
 /*   680 */   217,  119,  203,  201,  202,  528,  528,  528,  115,  528,
 /*   690 */   528,  528,  528,  528,  528,   96,  278,  528,  528,  528,
 /*   700 */   528,  528,  528,  528,  528,  528,  528,  528,  528,  528,
 /*   710 */   528,  207,   77,  131,  528,  132,  227,  209,   80,  528,
 /*   720 */   133,  134,   84,  135,  528,   85,   79,  528,  528,  236,
 /*   730 */   215,  217,  528,  528,  528,  528,  528,  528,  197,  203,
 /*   740 */   201,  202,  528,  528,  528,  115,  528,  528,  528,  528,
 /*   750 */   528,  528,   96,  278,  528,  528,  528,  528,  528,  528,
 /*   760 */   528,  528,  528,  528,  528,  528,  528,  528,  207,   77,
 /*   770 */   131,  528,  132,  227,  209,   80,  528,  133,  134,   84,
 /*   780 */   135,  528,   85,   79,  528,  528,  236,  215,  217,  198,
 /*   790 */   203,  201,  202,  528,  528,  528,  115,  528,  528,  528,
 /*   800 */   528,  528,  528,   96,  278,  528,  528,  528,  528,  528,
 /*   810 */   528,  528,  528,  528,  528,  528,  528,  528,  528,  207,
 /*   820 */    77,  131,  528,  132,  227,  209,   80,  528,  133,  134,
 /*   830 */    84,  135,  528,   85,   79,  528,  528,  236,  215,  217,
 /*   840 */   528,  528,  528,  528,  528,  528,  199,  203,  201,  202,
 /*   850 */   528,  528,  528,  115,  528,  528,  528,  528,  528,  528,
 /*   860 */    96,  278,  528,  528,  528,  528,  528,  528,  528,  528,
 /*   870 */   528,  528,  528,  528,  528,  528,  207,   77,  131,  528,
 /*   880 */   132,  227,  209,   80,  528,  133,  134,   84,  135,  528,
 /*   890 */    85,   79,  528,  528,  236,  215,  217,  121,  203,  201,
 /*   900 */   202,  528,  528,  528,  115,  528,  528,  528,  528,  528,
 /*   910 */   528,   96,  278,  528,  528,  528,  528,  528,  528,  528,
 /*   920 */   528,  528,  528,  528,  528,  528,  528,  207,   77,  131,
 /*   930 */   262,  132,  227,  209,   80,  528,  133,  134,   84,  135,
 /*   940 */   528,   85,   79,  528,  528,  236,  215,  217,  528,  528,
 /*   950 */   528,  150,  141,  147,  149,  261,  257,  207,   77,  131,
 /*   960 */   528,  132,  227,  209,   80,  528,  133,  134,   84,  135,
 /*   970 */   528,   85,   79,  528,  528,  236,  215,  217,  200,  201,
 /*   980 */   202,  528,  528,  528,  115,  528,  528,  528,  528,  528,
 /*   990 */   528,   96,  278,  528,  528,  528,  528,  528,  528,  528,
 /*  1000 */   528,  528,  528,  528,  528,  528,  528,  207,   77,  131,
 /*  1010 */   256,  132,  227,  209,   80,  528,  133,  134,   84,  135,
 /*  1020 */   528,   85,   79,  528,  528,  236,  215,  217,  528,  528,
 /*  1030 */   528,  528,  140,  142,  145,  255,  257,  207,   77,  131,
 /*  1040 */   528,  132,  227,  209,   80,  528,  133,  134,   84,  135,
 /*  1050 */   528,   85,   79,  528,  528,  236,  215,  217,  528,  207,
 /*  1060 */    77,  131,  528,  132,  227,  209,   80,  528,  133,  134,
 /*  1070 */    84,  135,  528,   85,   79,  528,  528,  236,  215,  217,
 /*  1080 */   528,  113,   87,  190,  245,  528,  528,   17,  528,  528,
 /*  1090 */    62,  528,  528,   41,  528,   43,   44,  311,  312,  313,
 /*  1100 */   314,  315,  316,  317,  318,  319,  320,  321,  322,  146,
 /*  1110 */   528,  528,  528,  528,  528,  528,  528,  528,   28,  528,
 /*  1120 */   528,   30,   31,  528,  528,   40,  528,  528,  528,  528,
 /*  1130 */   528,  528,  528,  528,  528,  528,  528,  528,  528,  528,
 /*  1140 */   248,  528,   47,  528,  528,  528,  528,  528,  528,  528,
 /*  1150 */    59,   60,  528,  528,  528,   61,   21,  528,  219,  220,
 /*  1160 */   221,  222,  223,  224,  225,  226,   20,  207,   77,  131,
 /*  1170 */   528,  132,  227,  209,   80,  528,  133,  134,   84,  135,
 /*  1180 */   139,   85,   79,  528,  528,  236,  215,  217,  528,  112,
 /*  1190 */   528,  528,  528,  528,  528,  136,  528,  528,  528,  528,
 /*  1200 */   528,  528,  528,  528,   28,  528,  528,  207,   77,  131,
 /*  1210 */   528,  132,  227,  209,   80,  528,  133,  134,   84,  135,
 /*  1220 */   528,   85,   79,  528,  218,  236,  215,  217,   47,  528,
 /*  1230 */   528,  528,  242,   28,  528,  528,   59,   60,  528,  528,
 /*  1240 */   528,   61,   21,   15,  219,  220,  221,  222,  223,  224,
 /*  1250 */   225,  226,   20,  240,  218,  528,  528,   47,  528,  528,
 /*  1260 */   528,  528,  528,   28,  528,   59,   60,  528,  528,  528,
 /*  1270 */    61,   21,  247,  219,  220,  221,  222,  223,  224,  225,
 /*  1280 */   226,   20,  528,  528,  528,  120,  528,   47,  528,  528,
 /*  1290 */   528,  528,  528,  528,  528,   59,   60,  528,  528,  528,
 /*  1300 */    61,   21,  528,  219,  220,  221,  222,  223,  224,  225,
 /*  1310 */   226,   20,  207,   77,  131,  528,  132,  227,  209,   80,
 /*  1320 */   528,  133,  134,   84,  135,  124,   85,   79,  528,  528,
 /*  1330 */   236,  215,  217,  528,  528,  528,  528,  528,  528,  528,
 /*  1340 */   528,  528,  528,  528,  528,  528,  206,  528,  528,  528,
 /*  1350 */   528,  528,  207,   77,  131,  528,  132,  227,  209,   80,
 /*  1360 */   528,  133,  134,   84,  135,  528,   85,   79,  246,  528,
 /*  1370 */   236,  215,  217,  207,   77,  131,  528,  132,  227,  209,
 /*  1380 */    80,  528,  133,  134,   84,  135,  528,   85,   79,  239,
 /*  1390 */   528,  236,  215,  217,  528,  207,   77,  131,  528,  132,
 /*  1400 */   227,  209,   80,  528,  133,  134,   84,  135,  528,   85,
 /*  1410 */    79,  137,  528,  236,  215,  217,  207,   77,  131,  528,
 /*  1420 */   132,  227,  209,   80,  528,  133,  134,   84,  135,  528,
 /*  1430 */    85,   79,  528,  243,  236,  215,  217,  528,  207,   77,
 /*  1440 */   131,  528,  132,  227,  209,   80,  528,  133,  134,   84,
 /*  1450 */   135,  528,   85,   79,  250,  528,  236,  215,  217,  528,
 /*  1460 */   207,   77,  131,  528,  132,  227,  209,   80,  528,  133,
 /*  1470 */   134,   84,  135,  528,   85,   79,  251,  528,  236,  215,
 /*  1480 */   217,  207,   77,  131,  528,  132,  227,  209,   80,  528,
 /*  1490 */   133,  134,   84,  135,  528,   85,   79,  253,  528,  236,
 /*  1500 */   215,  217,  528,  207,   77,  131,  528,  132,  227,  209,
 /*  1510 */    80,  528,  133,  134,   84,  135,  528,   85,   79,  265,
 /*  1520 */   528,  236,  215,  217,  207,   77,  131,  528,  132,  227,
 /*  1530 */   209,   80,  528,  133,  134,   84,  135,  528,   85,   79,
 /*  1540 */   528,  266,  236,  215,  217,  528,  207,   77,  131,  528,
 /*  1550 */   132,  227,  209,   80,  528,  133,  134,   84,  135,  528,
 /*  1560 */    85,   79,  152,  528,  236,  215,  217,  528,  207,   77,
 /*  1570 */   131,  528,  132,  227,  209,   80,  528,  133,  134,   84,
 /*  1580 */   135,  528,   85,   79,  154,  528,  236,  215,  217,  207,
 /*  1590 */    77,  131,  528,  132,  227,  209,   80,  528,  133,  134,
 /*  1600 */    84,  135,  528,   85,   79,  165,  528,  236,  215,  217,
 /*  1610 */   528,  207,   77,  131,  528,  132,  227,  209,   80,  528,
 /*  1620 */   133,  134,   84,  135,  528,   85,   79,  279,  528,  236,
 /*  1630 */   215,  217,  207,   77,  131,  528,  132,  227,  209,   80,
 /*  1640 */   528,  133,  134,   84,  135,  528,   85,   79,  528,  289,
 /*  1650 */   236,  215,  217,  528,  207,   77,  131,  528,  132,  227,
 /*  1660 */   209,   80,  528,  133,  134,   84,  135,  528,   85,   79,
 /*  1670 */   189,  528,  236,  215,  217,  528,  207,   77,  131,  528,
 /*  1680 */   132,  227,  209,   80,  528,  133,  134,   84,  135,  528,
 /*  1690 */    85,   79,  528,  528,  236,  215,  217,  207,   77,  131,
 /*  1700 */   528,  132,  227,  209,   80,  528,  133,  134,   84,  135,
 /*  1710 */   136,   85,   79,  528,  528,  236,  215,  217,  528,   28,
 /*  1720 */   109,  125,  528,  132,  227,  209,   80,  528,  133,  134,
 /*  1730 */    84,  135,  528,   85,   79,  528,  109,  236,  215,  217,
 /*  1740 */   208,  209,   80,   47,  133,  134,   84,  135,  528,   85,
 /*  1750 */    79,   59,   60,  236,  215,  217,   61,   21,  528,  219,
 /*  1760 */   220,  221,  222,  223,  224,  225,  226,   20,  528,  218,
 /*  1770 */   528,  528,  528,  528,  528,  528,  528,  528,   28,  109,
 /*  1780 */   191,  528,  132,  227,  209,   80,  528,  133,  134,   84,
 /*  1790 */   135,  528,   85,   79,  528,  109,  236,  215,  217,  210,
 /*  1800 */   209,   80,   47,  133,  134,   84,  135,  528,   85,   79,
 /*  1810 */    59,   60,  236,  215,  217,   61,   21,  528,  219,  220,
 /*  1820 */   221,  222,  223,  224,  225,  226,   20,  528,  109,  192,
 /*  1830 */   528,  132,  227,  209,   80,  218,  133,  134,   84,  135,
 /*  1840 */   528,   85,   79,  528,   28,  236,  215,  217,  109,  193,
 /*  1850 */   528,  132,  227,  209,   80,  528,  133,  134,   84,  135,
 /*  1860 */   528,   85,   79,  528,  528,  236,  215,  217,  528,  528,
 /*  1870 */   528,  528,  528,  528,  528,  528,   59,   60,  528,  528,
 /*  1880 */   528,   61,   21,  528,  219,  220,  221,  222,  223,  224,
 /*  1890 */   225,  226,   20,  109,  528,  528,  126,  227,  209,   80,
 /*  1900 */   528,  133,  134,   84,  135,  528,   85,   79,  528,  528,
 /*  1910 */   236,  215,  217,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   12,   81,   12,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   15,  108,   24,   25,   26,   20,   21,
 /*    20 */   114,   23,  116,  117,  118,  119,   69,  121,  122,   72,
 /*    30 */   108,  125,  126,  127,  104,  105,  106,   27,   76,   77,
 /*    40 */    78,   79,   80,   45,  122,   12,   84,  125,  126,  127,
 /*    50 */    82,   53,   54,   91,   92,  134,   58,   59,   25,   61,
 /*    60 */    62,   63,   64,   65,   66,   67,   68,   69,  110,  107,
 /*    70 */   108,  109,  124,  111,  112,  113,  114,   11,  116,  117,
 /*    80 */   118,  119,  108,  121,  122,   18,   20,  125,  126,  127,
 /*    90 */    77,   78,   79,   80,  115,  121,  122,   84,  108,  125,
 /*   100 */   126,  127,  134,   82,   91,   92,  116,  117,  118,  119,
 /*   110 */     1,  121,  122,   46,   47,  125,  126,  127,   24,   25,
 /*   120 */   107,  108,  109,  120,  111,  112,  113,  114,  123,  116,
 /*   130 */   117,  118,  119,   16,  121,  122,   96,   97,  125,  126,
 /*   140 */   127,   77,   78,   79,   80,   96,   97,   90,   84,  108,
 /*   150 */    93,   94,   95,   96,   97,   91,   92,  100,  117,  118,
 /*   160 */   119,    1,  121,  122,   25,    5,  125,  126,  127,   87,
 /*   170 */    88,  107,  108,  109,   16,  111,  112,  113,  114,   70,
 /*   180 */   116,  117,  118,  119,    1,  121,  122,    4,    1,  125,
 /*   190 */   126,  127,    5,  128,   55,   56,   57,  108,   77,   78,
 /*   200 */    79,   80,   96,   97,  131,   84,  108,  118,  119,   12,
 /*   210 */   121,  122,   91,   92,  125,  126,  127,  119,   60,  121,
 /*   220 */   122,   24,   25,  125,  126,  127,   96,   97,  107,  108,
 /*   230 */   109,   24,  111,  112,  113,  114,  108,  116,  117,  118,
 /*   240 */   119,    1,  121,  122,    4,   12,  125,  126,  127,   77,
 /*   250 */    78,   79,   80,  125,  126,  127,   84,   24,   25,   26,
 /*   260 */    73,   17,   16,   91,   92,   21,   90,  105,   24,   93,
 /*   270 */    94,   95,   96,   97,   50,   51,  100,   98,   99,  107,
 /*   280 */   108,  109,  105,  111,  112,  113,  114,  105,  116,  117,
 /*   290 */   118,  119,    1,  121,  122,   83,   83,  125,  126,  127,
 /*   300 */   108,   89,   89,   59,   53,   54,   77,   78,   79,   80,
 /*   310 */    95,   96,   97,   84,  108,  100,   70,  125,  126,  127,
 /*   320 */    91,   92,    0,    1,   94,   95,   96,   97,  105,  133,
 /*   330 */   100,  125,  126,  127,   59,   18,  107,  108,  109,   85,
 /*   340 */   111,  112,  113,  114,  108,  116,  117,  118,  119,  108,
 /*   350 */   121,  122,   12,   86,  125,  126,  127,   77,   78,   79,
 /*   360 */    80,  125,  126,  127,   84,   74,  125,  126,  127,   12,
 /*   370 */    88,   91,   92,    3,  104,    5,  106,    1,   23,    3,
 /*   380 */    92,   24,   25,   26,   24,   99,   97,  107,  108,  109,
 /*   390 */    97,  111,  112,  113,  114,   19,  116,  117,  118,  119,
 /*   400 */     1,  121,  122,    4,   97,  125,  126,  127,   95,   96,
 /*   410 */    97,   97,   97,  100,   77,   78,   79,   80,   97,    1,
 /*   420 */    97,   84,    4,    1,    1,   97,    4,    4,   91,   92,
 /*   430 */   133,    4,    1,    1,   43,   48,   44,   47,   49,   28,
 /*   440 */    71,   22,   70,   73,  107,  108,  109,   52,  111,  112,
 /*   450 */   113,  114,   16,  116,  117,  118,  119,   16,  121,  122,
 /*   460 */    16,   28,  125,  126,  127,   77,   78,   79,   80,   16,
 /*   470 */    16,   16,   84,   16,   22,    1,    1,    4,    1,   91,
 /*   480 */    92,   12,    1,   12,   16,    1,   16,   21,   12,   17,
 /*   490 */    12,   12,   12,    1,   60,  107,  108,  109,   16,  111,
 /*   500 */   112,  113,  114,   16,  116,  117,  118,  119,   22,  121,
 /*   510 */   122,   12,    4,  125,  126,  127,   16,   16,   16,   12,
 /*   520 */    16,   16,   77,   78,   79,   80,   16,   16,   16,   84,
 /*   530 */    16,   16,   16,   16,   16,   12,   91,   92,   12,    1,
 /*   540 */   135,  135,  135,   60,  135,  135,  135,  135,  135,  135,
 /*   550 */   135,  135,  107,  108,  109,  135,  111,  112,  113,  114,
 /*   560 */   135,  116,  117,  118,  119,  135,  121,  122,  135,  135,
 /*   570 */   125,  126,  127,   77,   78,   79,   80,  135,  135,  135,
 /*   580 */    84,  135,  135,  135,  135,  135,  135,   91,   92,  135,
 /*   590 */   135,  135,  135,  135,  135,  135,  135,  135,  135,  135,
 /*   600 */   135,  135,  135,  107,  108,  109,  135,  111,  112,  113,
 /*   610 */   114,  135,  116,  117,  118,  119,  135,  121,  122,  135,
 /*   620 */   135,  125,  126,  127,  135,  135,  135,  135,  135,  135,
 /*   630 */    77,   78,   79,   80,  135,  135,  135,   84,  135,  135,
 /*   640 */   135,  135,  135,  135,   91,   92,  135,  135,  135,  135,
 /*   650 */   135,  135,  135,  135,  135,  135,  135,  135,  135,  135,
 /*   660 */   107,  108,  109,  135,  111,  112,  113,  114,  135,  116,
 /*   670 */   117,  118,  119,  135,  121,  122,  135,  135,  125,  126,
 /*   680 */   127,   77,   78,   79,   80,  135,  135,  135,   84,  135,
 /*   690 */   135,  135,  135,  135,  135,   91,   92,  135,  135,  135,
 /*   700 */   135,  135,  135,  135,  135,  135,  135,  135,  135,  135,
 /*   710 */   135,  107,  108,  109,  135,  111,  112,  113,  114,  135,
 /*   720 */   116,  117,  118,  119,  135,  121,  122,  135,  135,  125,
 /*   730 */   126,  127,  135,  135,  135,  135,  135,  135,   77,   78,
 /*   740 */    79,   80,  135,  135,  135,   84,  135,  135,  135,  135,
 /*   750 */   135,  135,   91,   92,  135,  135,  135,  135,  135,  135,
 /*   760 */   135,  135,  135,  135,  135,  135,  135,  135,  107,  108,
 /*   770 */   109,  135,  111,  112,  113,  114,  135,  116,  117,  118,
 /*   780 */   119,  135,  121,  122,  135,  135,  125,  126,  127,   77,
 /*   790 */    78,   79,   80,  135,  135,  135,   84,  135,  135,  135,
 /*   800 */   135,  135,  135,   91,   92,  135,  135,  135,  135,  135,
 /*   810 */   135,  135,  135,  135,  135,  135,  135,  135,  135,  107,
 /*   820 */   108,  109,  135,  111,  112,  113,  114,  135,  116,  117,
 /*   830 */   118,  119,  135,  121,  122,  135,  135,  125,  126,  127,
 /*   840 */   135,  135,  135,  135,  135,  135,   77,   78,   79,   80,
 /*   850 */   135,  135,  135,   84,  135,  135,  135,  135,  135,  135,
 /*   860 */    91,   92,  135,  135,  135,  135,  135,  135,  135,  135,
 /*   870 */   135,  135,  135,  135,  135,  135,  107,  108,  109,  135,
 /*   880 */   111,  112,  113,  114,  135,  116,  117,  118,  119,  135,
 /*   890 */   121,  122,  135,  135,  125,  126,  127,   77,   78,   79,
 /*   900 */    80,  135,  135,  135,   84,  135,  135,  135,  135,  135,
 /*   910 */   135,   91,   92,  135,  135,  135,  135,  135,  135,  135,
 /*   920 */   135,  135,  135,  135,  135,  135,  135,  107,  108,  109,
 /*   930 */    80,  111,  112,  113,  114,  135,  116,  117,  118,  119,
 /*   940 */   135,  121,  122,  135,  135,  125,  126,  127,  135,  135,
 /*   950 */   135,  101,  102,  103,  104,  105,  106,  107,  108,  109,
 /*   960 */   135,  111,  112,  113,  114,  135,  116,  117,  118,  119,
 /*   970 */   135,  121,  122,  135,  135,  125,  126,  127,   78,   79,
 /*   980 */    80,  135,  135,  135,   84,  135,  135,  135,  135,  135,
 /*   990 */   135,   91,   92,  135,  135,  135,  135,  135,  135,  135,
 /*  1000 */   135,  135,  135,  135,  135,  135,  135,  107,  108,  109,
 /*  1010 */    80,  111,  112,  113,  114,  135,  116,  117,  118,  119,
 /*  1020 */   135,  121,  122,  135,  135,  125,  126,  127,  135,  135,
 /*  1030 */   135,  135,   80,  103,  104,  105,  106,  107,  108,  109,
 /*  1040 */   135,  111,  112,  113,  114,  135,  116,  117,  118,  119,
 /*  1050 */   135,  121,  122,  135,  135,  125,  126,  127,  135,  107,
 /*  1060 */   108,  109,  135,  111,  112,  113,  114,  135,  116,  117,
 /*  1070 */   118,  119,  135,  121,  122,  135,  135,  125,  126,  127,
 /*  1080 */   135,  129,  130,   17,  132,  135,  135,   21,  135,  135,
 /*  1090 */    24,  135,  135,   27,  135,   29,   30,   31,   32,   33,
 /*  1100 */    34,   35,   36,   37,   38,   39,   40,   41,   42,   12,
 /*  1110 */   135,  135,  135,  135,  135,  135,  135,  135,   21,  135,
 /*  1120 */   135,   24,   25,  135,  135,   59,  135,  135,  135,  135,
 /*  1130 */   135,  135,  135,  135,  135,  135,  135,  135,  135,  135,
 /*  1140 */    80,  135,   45,  135,  135,  135,  135,  135,  135,  135,
 /*  1150 */    53,   54,  135,  135,  135,   58,   59,  135,   61,   62,
 /*  1160 */    63,   64,   65,   66,   67,   68,   69,  107,  108,  109,
 /*  1170 */   135,  111,  112,  113,  114,  135,  116,  117,  118,  119,
 /*  1180 */    80,  121,  122,  135,  135,  125,  126,  127,  135,  129,
 /*  1190 */   135,  135,  135,  135,  135,   12,  135,  135,  135,  135,
 /*  1200 */   135,  135,  135,  135,   21,  135,  135,  107,  108,  109,
 /*  1210 */   135,  111,  112,  113,  114,  135,  116,  117,  118,  119,
 /*  1220 */   135,  121,  122,  135,   12,  125,  126,  127,   45,  135,
 /*  1230 */   135,  135,  132,   21,  135,  135,   53,   54,  135,  135,
 /*  1240 */   135,   58,   59,    1,   61,   62,   63,   64,   65,   66,
 /*  1250 */    67,   68,   69,   70,   12,  135,  135,   45,  135,  135,
 /*  1260 */   135,  135,  135,   21,  135,   53,   54,  135,  135,  135,
 /*  1270 */    58,   59,   60,   61,   62,   63,   64,   65,   66,   67,
 /*  1280 */    68,   69,  135,  135,  135,   80,  135,   45,  135,  135,
 /*  1290 */   135,  135,  135,  135,  135,   53,   54,  135,  135,  135,
 /*  1300 */    58,   59,  135,   61,   62,   63,   64,   65,   66,   67,
 /*  1310 */    68,   69,  107,  108,  109,  135,  111,  112,  113,  114,
 /*  1320 */   135,  116,  117,  118,  119,   80,  121,  122,  135,  135,
 /*  1330 */   125,  126,  127,  135,  135,  135,  135,  135,  135,  135,
 /*  1340 */   135,  135,  135,  135,  135,  135,   80,  135,  135,  135,
 /*  1350 */   135,  135,  107,  108,  109,  135,  111,  112,  113,  114,
 /*  1360 */   135,  116,  117,  118,  119,  135,  121,  122,   80,  135,
 /*  1370 */   125,  126,  127,  107,  108,  109,  135,  111,  112,  113,
 /*  1380 */   114,  135,  116,  117,  118,  119,  135,  121,  122,   80,
 /*  1390 */   135,  125,  126,  127,  135,  107,  108,  109,  135,  111,
 /*  1400 */   112,  113,  114,  135,  116,  117,  118,  119,  135,  121,
 /*  1410 */   122,   80,  135,  125,  126,  127,  107,  108,  109,  135,
 /*  1420 */   111,  112,  113,  114,  135,  116,  117,  118,  119,  135,
 /*  1430 */   121,  122,  135,   80,  125,  126,  127,  135,  107,  108,
 /*  1440 */   109,  135,  111,  112,  113,  114,  135,  116,  117,  118,
 /*  1450 */   119,  135,  121,  122,   80,  135,  125,  126,  127,  135,
 /*  1460 */   107,  108,  109,  135,  111,  112,  113,  114,  135,  116,
 /*  1470 */   117,  118,  119,  135,  121,  122,   80,  135,  125,  126,
 /*  1480 */   127,  107,  108,  109,  135,  111,  112,  113,  114,  135,
 /*  1490 */   116,  117,  118,  119,  135,  121,  122,   80,  135,  125,
 /*  1500 */   126,  127,  135,  107,  108,  109,  135,  111,  112,  113,
 /*  1510 */   114,  135,  116,  117,  118,  119,  135,  121,  122,   80,
 /*  1520 */   135,  125,  126,  127,  107,  108,  109,  135,  111,  112,
 /*  1530 */   113,  114,  135,  116,  117,  118,  119,  135,  121,  122,
 /*  1540 */   135,   80,  125,  126,  127,  135,  107,  108,  109,  135,
 /*  1550 */   111,  112,  113,  114,  135,  116,  117,  118,  119,  135,
 /*  1560 */   121,  122,   80,  135,  125,  126,  127,  135,  107,  108,
 /*  1570 */   109,  135,  111,  112,  113,  114,  135,  116,  117,  118,
 /*  1580 */   119,  135,  121,  122,   80,  135,  125,  126,  127,  107,
 /*  1590 */   108,  109,  135,  111,  112,  113,  114,  135,  116,  117,
 /*  1600 */   118,  119,  135,  121,  122,   80,  135,  125,  126,  127,
 /*  1610 */   135,  107,  108,  109,  135,  111,  112,  113,  114,  135,
 /*  1620 */   116,  117,  118,  119,  135,  121,  122,   80,  135,  125,
 /*  1630 */   126,  127,  107,  108,  109,  135,  111,  112,  113,  114,
 /*  1640 */   135,  116,  117,  118,  119,  135,  121,  122,  135,   80,
 /*  1650 */   125,  126,  127,  135,  107,  108,  109,  135,  111,  112,
 /*  1660 */   113,  114,  135,  116,  117,  118,  119,  135,  121,  122,
 /*  1670 */    80,  135,  125,  126,  127,  135,  107,  108,  109,  135,
 /*  1680 */   111,  112,  113,  114,  135,  116,  117,  118,  119,  135,
 /*  1690 */   121,  122,  135,  135,  125,  126,  127,  107,  108,  109,
 /*  1700 */   135,  111,  112,  113,  114,  135,  116,  117,  118,  119,
 /*  1710 */    12,  121,  122,  135,  135,  125,  126,  127,  135,   21,
 /*  1720 */   108,  109,  135,  111,  112,  113,  114,  135,  116,  117,
 /*  1730 */   118,  119,  135,  121,  122,  135,  108,  125,  126,  127,
 /*  1740 */   112,  113,  114,   45,  116,  117,  118,  119,  135,  121,
 /*  1750 */   122,   53,   54,  125,  126,  127,   58,   59,  135,   61,
 /*  1760 */    62,   63,   64,   65,   66,   67,   68,   69,  135,   12,
 /*  1770 */   135,  135,  135,  135,  135,  135,  135,  135,   21,  108,
 /*  1780 */   109,  135,  111,  112,  113,  114,  135,  116,  117,  118,
 /*  1790 */   119,  135,  121,  122,  135,  108,  125,  126,  127,  112,
 /*  1800 */   113,  114,   45,  116,  117,  118,  119,  135,  121,  122,
 /*  1810 */    53,   54,  125,  126,  127,   58,   59,  135,   61,   62,
 /*  1820 */    63,   64,   65,   66,   67,   68,   69,  135,  108,  109,
 /*  1830 */   135,  111,  112,  113,  114,   12,  116,  117,  118,  119,
 /*  1840 */   135,  121,  122,  135,   21,  125,  126,  127,  108,  109,
 /*  1850 */   135,  111,  112,  113,  114,  135,  116,  117,  118,  119,
 /*  1860 */   135,  121,  122,  135,  135,  125,  126,  127,  135,  135,
 /*  1870 */   135,  135,  135,  135,  135,  135,   53,   54,  135,  135,
 /*  1880 */   135,   58,   59,  135,   61,   62,   63,   64,   65,   66,
 /*  1890 */    67,   68,   69,  108,  135,  135,  111,  112,  113,  114,
 /*  1900 */   135,  116,  117,  118,  119,  135,  121,  122,  135,  135,
 /*  1910 */   125,  126,  127,
};
#define YY_SHIFT_USE_DFLT (-44)
#define YY_SHIFT_MAX 199
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2, 1097,   -2, 1097,
 /*    20 */  1183, 1212, 1698, 1242, 1757, 1757, 1757, 1757, 1757, 1757,
 /*    30 */  1757, 1757, 1757, 1757, 1757, 1757, 1757, 1757, 1757, 1757,
 /*    40 */  1757, 1757, 1757, 1757, 1757, 1757, 1757, 1757, 1823, 1823,
 /*    50 */  1823, 1823, 1823,   -9,   -9, 1823, 1823,  233, 1823, 1823,
 /*    60 */  1823, 1823, 1823,  357,  357,  197,  187,  370,  376,  376,
 /*    70 */    33,   94,   94,   94,   94,  -11,   10, 1066,  139,  139,
 /*    80 */    67,  160,  224,  251,  224,  251,  -43,  117,  207,  207,
 /*    90 */   207,  207,  275,  317,  340,  -11,  355,  360,  360,   10,
 /*   100 */   360,  360,   10,  360,  360,  360,  360,   10,  275,  244,
 /*   110 */   322,  183,  158,  246,  240,   66,  399,  418,  422,  109,
 /*   120 */   291,  423,  427,  431,  432,  391,  392,  390,  387,  389,
 /*   130 */   395,  391,  392,  387,  389,  395,  411,  419,  372,  369,
 /*   140 */   369,  436,  441,  444,  433,  453,  433,  454,  455,  457,
 /*   150 */   452,  474,  475,  473,  477,  431,  469,  481,  471,  468,
 /*   160 */   476,  470,  472,  478,  472,  484,  479,  466,  486,  482,
 /*   170 */   487,  500,  501,  480,  499,  502,  507,  504,  505,  510,
 /*   180 */   511,  512,  514,  515,  516,  517,  518,  434,  492,  483,
 /*   190 */   523,  391,  391,  391,  508,  526,  538,  431,  431,  431,
};
#define YY_REDUCE_USE_DFLT (-95)
#define YY_REDUCE_MAX 108
static const short yy_reduce_ofst[] = {
 /*     0 */   -38,   13,   64,  121,  172,  229,  280,  337,  388,  445,
 /*    10 */   496,  553,  604,  661,  712,  769,  820,  850,  900,  930,
 /*    20 */   952, 1060, 1100, 1205, 1245, 1266, 1288, 1309, 1331, 1353,
 /*    30 */  1374, 1396, 1417, 1439, 1461, 1482, 1504, 1525, 1547, 1569,
 /*    40 */  1590, 1612, 1671, 1720, 1740, 1785, 1628, 1687,  -94,  -10,
 /*    50 */    41,   89,   98,   57,  176,  -26,  -78,  230,  128,  192,
 /*    60 */   206,  236,  241,  215,  313,  -70,  -79,  -32,  212,  213,
 /*    70 */   270,   40,   49,  106,  130,   82,  179,  -42,  -52,  -52,
 /*    80 */   -21,   21,    3,    5,    3,    5,   65,   73,  162,  177,
 /*    90 */   182,  223,  196,  254,  267,  282,  288,  289,  293,  286,
 /*   100 */   307,  314,  286,  315,  321,  323,  328,  286,  297,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   330,  330,  330,  330,  330,  330,  330,  330,  330,  330,
 /*    10 */   330,  330,  330,  330,  330,  330,  330,  409,  330,  526,
 /*    20 */   526,  526,  513,  526,  526,  337,  526,  526,  526,  526,
 /*    30 */   526,  526,  526,  339,  341,  526,  526,  526,  526,  526,
 /*    40 */   526,  526,  526,  526,  526,  526,  526,  526,  526,  526,
 /*    50 */   526,  526,  526,  397,  397,  526,  526,  526,  526,  526,
 /*    60 */   526,  526,  526,  526,  526,  526,  526,  524,  358,  358,
 /*    70 */   526,  526,  526,  526,  526,  526,  401,  485,  472,  471,
 /*    80 */   455,  524,  464,  470,  463,  469,  514,  512,  526,  526,
 /*    90 */   526,  526,  517,  354,  526,  526,  362,  526,  526,  526,
 /*   100 */   526,  526,  405,  526,  526,  526,  526,  404,  517,  485,
 /*   110 */   526,  526,  526,  526,  526,  526,  526,  526,  526,  526,
 /*   120 */   526,  526,  526,  525,  526,  432,  450,  456,  460,  462,
 /*   130 */   466,  436,  449,  459,  461,  465,  491,  526,  526,  526,
 /*   140 */   506,  410,  411,  412,  526,  414,  491,  417,  418,  421,
 /*   150 */   526,  526,  526,  526,  526,  359,  526,  526,  526,  346,
 /*   160 */   526,  347,  349,  526,  348,  526,  526,  526,  526,  381,
 /*   170 */   373,  369,  367,  526,  526,  371,  526,  377,  375,  379,
 /*   180 */   389,  385,  383,  387,  393,  391,  395,  526,  526,  526,
 /*   190 */   526,  433,  434,  435,  526,  526,  526,  521,  522,  523,
 /*   200 */   329,  331,  332,  328,  333,  336,  338,  431,  452,  453,
 /*   210 */   454,  475,  481,  482,  483,  484,  486,  487,  491,  492,
 /*   220 */   493,  494,  495,  496,  497,  498,  499,  451,  467,  468,
 /*   230 */   473,  474,  477,  478,  479,  480,  476,  500,  505,  511,
 /*   240 */   502,  503,  509,  510,  504,  508,  507,  501,  506,  413,
 /*   250 */   424,  425,  429,  430,  415,  416,  427,  428,  419,  420,
 /*   260 */   422,  423,  426,  488,  515,  340,  342,  343,  356,  357,
 /*   270 */   344,  345,  353,  352,  351,  350,  364,  365,  363,  355,
 /*   280 */   360,  366,  398,  368,  399,  370,  372,  400,  402,  403,
 /*   290 */   407,  408,  374,  376,  378,  380,  406,  382,  384,  386,
 /*   300 */   388,  390,  392,  394,  396,  518,  516,  489,  490,  457,
 /*   310 */   458,  437,  438,  439,  440,  441,  442,  443,  444,  445,
 /*   320 */   446,  447,  448,  334,  520,  335,  519,
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
  "COLON",         "AND_AND_EQUAL",  "BAR_BAR_EQUAL",  "PLUS_EQUAL",  
  "MINUS_EQUAL",   "STAR_EQUAL",    "DIV_EQUAL",     "DIV_DIV_EQUAL",
  "PERCENT_EQUAL",  "BAR_EQUAL",     "AND_EQUAL",     "XOR_EQUAL",   
  "STAR_STAR_EQUAL",  "LSHIFT_EQUAL",  "RSHIFT_EQUAL",  "BAR_BAR",     
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
  "decorators_opt",  "super_opt",     "names",         "dotted_names",
  "dotted_name",   "else_opt",      "params",        "decorators",  
  "decorator",     "params_without_default",  "params_with_default",  "block_param", 
  "var_param",     "kw_param",      "param_default_opt",  "param_default",
  "param_with_default",  "args",          "posargs",       "kwargs",      
  "vararg",        "varkwarg",      "kwarg",         "assign_expr", 
  "postfix_expr",  "logical_or_expr",  "augmented_assign_op",  "logical_and_expr",
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
 /* 106 */ "assign_expr ::= postfix_expr augmented_assign_op logical_or_expr",
 /* 107 */ "assign_expr ::= postfix_expr AND_AND_EQUAL logical_or_expr",
 /* 108 */ "assign_expr ::= postfix_expr BAR_BAR_EQUAL logical_or_expr",
 /* 109 */ "assign_expr ::= logical_or_expr",
 /* 110 */ "augmented_assign_op ::= PLUS_EQUAL",
 /* 111 */ "augmented_assign_op ::= MINUS_EQUAL",
 /* 112 */ "augmented_assign_op ::= STAR_EQUAL",
 /* 113 */ "augmented_assign_op ::= DIV_EQUAL",
 /* 114 */ "augmented_assign_op ::= DIV_DIV_EQUAL",
 /* 115 */ "augmented_assign_op ::= PERCENT_EQUAL",
 /* 116 */ "augmented_assign_op ::= BAR_EQUAL",
 /* 117 */ "augmented_assign_op ::= AND_EQUAL",
 /* 118 */ "augmented_assign_op ::= XOR_EQUAL",
 /* 119 */ "augmented_assign_op ::= STAR_STAR_EQUAL",
 /* 120 */ "augmented_assign_op ::= LSHIFT_EQUAL",
 /* 121 */ "augmented_assign_op ::= RSHIFT_EQUAL",
 /* 122 */ "logical_or_expr ::= logical_and_expr",
 /* 123 */ "logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr",
 /* 124 */ "logical_and_expr ::= not_expr",
 /* 125 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /* 126 */ "not_expr ::= comparison",
 /* 127 */ "not_expr ::= NOT not_expr",
 /* 128 */ "comparison ::= xor_expr",
 /* 129 */ "comparison ::= xor_expr comp_op xor_expr",
 /* 130 */ "comp_op ::= LESS",
 /* 131 */ "comp_op ::= GREATER",
 /* 132 */ "xor_expr ::= or_expr",
 /* 133 */ "xor_expr ::= xor_expr XOR or_expr",
 /* 134 */ "or_expr ::= and_expr",
 /* 135 */ "or_expr ::= or_expr BAR and_expr",
 /* 136 */ "and_expr ::= shift_expr",
 /* 137 */ "and_expr ::= and_expr AND shift_expr",
 /* 138 */ "shift_expr ::= match_expr",
 /* 139 */ "shift_expr ::= shift_expr shift_op match_expr",
 /* 140 */ "shift_op ::= LSHIFT",
 /* 141 */ "shift_op ::= RSHIFT",
 /* 142 */ "match_expr ::= arith_expr",
 /* 143 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 144 */ "arith_expr ::= term",
 /* 145 */ "arith_expr ::= arith_expr arith_op term",
 /* 146 */ "arith_op ::= PLUS",
 /* 147 */ "arith_op ::= MINUS",
 /* 148 */ "term ::= term term_op factor",
 /* 149 */ "term ::= factor",
 /* 150 */ "term_op ::= STAR",
 /* 151 */ "term_op ::= DIV",
 /* 152 */ "term_op ::= DIV_DIV",
 /* 153 */ "term_op ::= PERCENT",
 /* 154 */ "factor ::= PLUS factor",
 /* 155 */ "factor ::= MINUS factor",
 /* 156 */ "factor ::= TILDA factor",
 /* 157 */ "factor ::= power",
 /* 158 */ "power ::= postfix_expr",
 /* 159 */ "power ::= postfix_expr STAR_STAR factor",
 /* 160 */ "postfix_expr ::= atom",
 /* 161 */ "postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt",
 /* 162 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 163 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 164 */ "atom ::= NAME",
 /* 165 */ "atom ::= NUMBER",
 /* 166 */ "atom ::= REGEXP",
 /* 167 */ "atom ::= STRING",
 /* 168 */ "atom ::= SYMBOL",
 /* 169 */ "atom ::= NIL",
 /* 170 */ "atom ::= TRUE",
 /* 171 */ "atom ::= FALSE",
 /* 172 */ "atom ::= LINE",
 /* 173 */ "atom ::= LBRACKET exprs RBRACKET",
 /* 174 */ "atom ::= LBRACKET RBRACKET",
 /* 175 */ "atom ::= LBRACE RBRACE",
 /* 176 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 177 */ "atom ::= LBRACE exprs RBRACE",
 /* 178 */ "atom ::= LPAR expr RPAR",
 /* 179 */ "exprs ::= expr",
 /* 180 */ "exprs ::= exprs COMMA expr",
 /* 181 */ "dict_elems ::= dict_elem",
 /* 182 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 183 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 184 */ "dict_elem ::= NAME COLON expr",
 /* 185 */ "comma_opt ::=",
 /* 186 */ "comma_opt ::= COMMA",
 /* 187 */ "blockarg_opt ::=",
 /* 188 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 189 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 190 */ "blockarg_params_opt ::=",
 /* 191 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 192 */ "excepts ::= except",
 /* 193 */ "excepts ::= excepts except",
 /* 194 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 195 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 196 */ "except ::= EXCEPT NEWLINE stmts",
 /* 197 */ "finally_opt ::=",
 /* 198 */ "finally_opt ::= FINALLY stmts",
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
  { 76, 1 },
  { 77, 1 },
  { 77, 3 },
  { 78, 0 },
  { 78, 1 },
  { 78, 1 },
  { 78, 7 },
  { 78, 5 },
  { 78, 5 },
  { 78, 5 },
  { 78, 1 },
  { 78, 2 },
  { 78, 1 },
  { 78, 2 },
  { 78, 1 },
  { 78, 2 },
  { 78, 6 },
  { 78, 7 },
  { 78, 4 },
  { 78, 2 },
  { 78, 2 },
  { 87, 1 },
  { 87, 3 },
  { 88, 1 },
  { 88, 3 },
  { 86, 1 },
  { 86, 3 },
  { 85, 0 },
  { 85, 2 },
  { 83, 1 },
  { 83, 5 },
  { 89, 0 },
  { 89, 2 },
  { 79, 8 },
  { 84, 0 },
  { 84, 1 },
  { 91, 1 },
  { 91, 2 },
  { 92, 3 },
  { 90, 9 },
  { 90, 7 },
  { 90, 7 },
  { 90, 5 },
  { 90, 7 },
  { 90, 5 },
  { 90, 5 },
  { 90, 3 },
  { 90, 7 },
  { 90, 5 },
  { 90, 5 },
  { 90, 3 },
  { 90, 5 },
  { 90, 3 },
  { 90, 3 },
  { 90, 1 },
  { 90, 7 },
  { 90, 5 },
  { 90, 5 },
  { 90, 3 },
  { 90, 5 },
  { 90, 3 },
  { 90, 3 },
  { 90, 1 },
  { 90, 5 },
  { 90, 3 },
  { 90, 3 },
  { 90, 1 },
  { 90, 3 },
  { 90, 1 },
  { 90, 1 },
  { 90, 0 },
  { 97, 2 },
  { 96, 2 },
  { 95, 3 },
  { 98, 0 },
  { 98, 1 },
  { 99, 2 },
  { 93, 1 },
  { 93, 3 },
  { 94, 1 },
  { 94, 3 },
  { 100, 2 },
  { 101, 0 },
  { 101, 1 },
  { 101, 3 },
  { 101, 5 },
  { 101, 7 },
  { 101, 3 },
  { 101, 5 },
  { 101, 3 },
  { 101, 1 },
  { 101, 3 },
  { 101, 5 },
  { 101, 3 },
  { 101, 1 },
  { 101, 3 },
  { 101, 1 },
  { 105, 2 },
  { 104, 2 },
  { 102, 1 },
  { 102, 3 },
  { 103, 1 },
  { 103, 3 },
  { 106, 3 },
  { 80, 1 },
  { 107, 3 },
  { 107, 3 },
  { 107, 3 },
  { 107, 3 },
  { 107, 1 },
  { 110, 1 },
  { 110, 1 },
  { 110, 1 },
  { 110, 1 },
  { 110, 1 },
  { 110, 1 },
  { 110, 1 },
  { 110, 1 },
  { 110, 1 },
  { 110, 1 },
  { 110, 1 },
  { 110, 1 },
  { 109, 1 },
  { 109, 3 },
  { 111, 1 },
  { 111, 3 },
  { 112, 1 },
  { 112, 2 },
  { 113, 1 },
  { 113, 3 },
  { 115, 1 },
  { 115, 1 },
  { 114, 1 },
  { 114, 3 },
  { 116, 1 },
  { 116, 3 },
  { 117, 1 },
  { 117, 3 },
  { 118, 1 },
  { 118, 3 },
  { 120, 1 },
  { 120, 1 },
  { 119, 1 },
  { 119, 3 },
  { 121, 1 },
  { 121, 3 },
  { 123, 1 },
  { 123, 1 },
  { 122, 3 },
  { 122, 1 },
  { 124, 1 },
  { 124, 1 },
  { 124, 1 },
  { 124, 1 },
  { 125, 2 },
  { 125, 2 },
  { 125, 2 },
  { 125, 1 },
  { 126, 1 },
  { 126, 3 },
  { 108, 1 },
  { 108, 5 },
  { 108, 4 },
  { 108, 3 },
  { 127, 1 },
  { 127, 1 },
  { 127, 1 },
  { 127, 1 },
  { 127, 1 },
  { 127, 1 },
  { 127, 1 },
  { 127, 1 },
  { 127, 1 },
  { 127, 3 },
  { 127, 2 },
  { 127, 2 },
  { 127, 4 },
  { 127, 3 },
  { 127, 3 },
  { 129, 1 },
  { 129, 3 },
  { 130, 1 },
  { 130, 3 },
  { 132, 3 },
  { 132, 3 },
  { 131, 0 },
  { 131, 1 },
  { 128, 0 },
  { 128, 5 },
  { 128, 5 },
  { 133, 0 },
  { 133, 3 },
  { 81, 1 },
  { 81, 2 },
  { 134, 6 },
  { 134, 4 },
  { 134, 3 },
  { 82, 0 },
  { 82, 2 },
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
#line 763 "parser.y"
{
    *pval = yymsp[0].minor.yy127;
}
#line 2300 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 21: /* dotted_names ::= dotted_name */
      case 36: /* decorators ::= decorator */
      case 79: /* params_with_default ::= param_with_default */
      case 99: /* posargs ::= expr */
      case 101: /* kwargs ::= kwarg */
      case 179: /* exprs ::= expr */
      case 181: /* dict_elems ::= dict_elem */
      case 192: /* excepts ::= except */
#line 767 "parser.y"
{
    yygotominor.yy127 = make_array_with(env, yymsp[0].minor.yy127);
}
#line 2315 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 22: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 80: /* params_with_default ::= params_with_default COMMA param_with_default */
#line 770 "parser.y"
{
    yygotominor.yy127 = Array_push(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2324 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 27: /* super_opt ::= */
      case 31: /* else_opt ::= */
      case 34: /* decorators_opt ::= */
      case 74: /* param_default_opt ::= */
      case 82: /* args ::= */
      case 185: /* comma_opt ::= */
      case 187: /* blockarg_opt ::= */
      case 190: /* blockarg_params_opt ::= */
      case 197: /* finally_opt ::= */
#line 774 "parser.y"
{
    yygotominor.yy127 = YNIL;
}
#line 2340 "parser.c"
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
      case 109: /* assign_expr ::= logical_or_expr */
      case 122: /* logical_or_expr ::= logical_and_expr */
      case 124: /* logical_and_expr ::= not_expr */
      case 126: /* not_expr ::= comparison */
      case 128: /* comparison ::= xor_expr */
      case 132: /* xor_expr ::= or_expr */
      case 134: /* or_expr ::= and_expr */
      case 136: /* and_expr ::= shift_expr */
      case 138: /* shift_expr ::= match_expr */
      case 142: /* match_expr ::= arith_expr */
      case 144: /* arith_expr ::= term */
      case 149: /* term ::= factor */
      case 157: /* factor ::= power */
      case 158: /* power ::= postfix_expr */
      case 160: /* postfix_expr ::= atom */
      case 198: /* finally_opt ::= FINALLY stmts */
#line 777 "parser.y"
{
    yygotominor.yy127 = yymsp[0].minor.yy127;
}
#line 2373 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 783 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy127 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2381 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 787 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[-1].minor.yy127);
}
#line 2389 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 791 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = Finally_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2397 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 795 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = While_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2405 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 799 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Break_new(env, lineno, YNIL);
}
#line 2413 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 803 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Break_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2421 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 807 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Next_new(env, lineno, YNIL);
}
#line 2429 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 811 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Next_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2437 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 815 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Return_new(env, lineno, YNIL);
}
#line 2445 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 819 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Return_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2453 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 823 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy127 = If_new(env, lineno, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2461 "parser.c"
        break;
      case 17: /* stmt ::= decorators_opt CLASS NAME super_opt NEWLINE stmts END */
#line 827 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy127 = Klass_new(env, lineno, yymsp[-6].minor.yy127, id, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2470 "parser.c"
        break;
      case 18: /* stmt ::= MODULE NAME stmts END */
#line 832 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    yygotominor.yy127 = Module_new(env, lineno, id, yymsp[-1].minor.yy127);
}
#line 2479 "parser.c"
        break;
      case 19: /* stmt ::= NONLOCAL names */
#line 837 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Nonlocal_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2487 "parser.c"
        break;
      case 20: /* stmt ::= IMPORT dotted_names */
#line 841 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy127 = Import_new(env, lineno, yymsp[0].minor.yy127);
}
#line 2495 "parser.c"
        break;
      case 23: /* dotted_name ::= NAME */
      case 25: /* names ::= NAME */
#line 853 "parser.y"
{
    yygotominor.yy127 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2503 "parser.c"
        break;
      case 24: /* dotted_name ::= dotted_name DOT NAME */
      case 26: /* names ::= names COMMA NAME */
#line 856 "parser.y"
{
    yygotominor.yy127 = Array_push_token_id(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy0);
}
#line 2511 "parser.c"
        break;
      case 30: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 877 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127, yymsp[0].minor.yy127);
    yygotominor.yy127 = make_array_with(env, node);
}
#line 2520 "parser.c"
        break;
      case 33: /* func_def ::= decorators_opt DEF NAME LPAR params RPAR stmts END */
#line 890 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy127 = FuncDef_new(env, lineno, yymsp[-7].minor.yy127, id, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 2529 "parser.c"
        break;
      case 37: /* decorators ::= decorators decorator */
      case 193: /* excepts ::= excepts except */
#line 906 "parser.y"
{
    yygotominor.yy127 = Array_push(env, yymsp[-1].minor.yy127, yymsp[0].minor.yy127);
}
#line 2537 "parser.c"
        break;
      case 38: /* decorator ::= AT expr NEWLINE */
      case 178: /* atom ::= LPAR expr RPAR */
      case 191: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 910 "parser.y"
{
    yygotominor.yy127 = yymsp[-1].minor.yy127;
}
#line 2546 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 914 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-8].minor.yy127, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2553 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 917 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2560 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 920 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2567 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 923 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2574 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 926 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2581 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 929 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2588 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 932 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2595 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA params_with_default */
#line 935 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL, YNIL);
}
#line 2602 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 938 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-6].minor.yy127, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2609 "parser.c"
        break;
      case 48: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 941 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2616 "parser.c"
        break;
      case 49: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 944 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2623 "parser.c"
        break;
      case 50: /* params ::= params_without_default COMMA block_param */
#line 947 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2630 "parser.c"
        break;
      case 51: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 950 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-4].minor.yy127, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2637 "parser.c"
        break;
      case 52: /* params ::= params_without_default COMMA var_param */
#line 953 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2644 "parser.c"
        break;
      case 53: /* params ::= params_without_default COMMA kw_param */
#line 956 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[-2].minor.yy127, YNIL, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2651 "parser.c"
        break;
      case 54: /* params ::= params_without_default */
#line 959 "parser.y"
{
    yygotominor.yy127 = Params_new(env, yymsp[0].minor.yy127, YNIL, YNIL, YNIL, YNIL);
}
#line 2658 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 962 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2665 "parser.c"
        break;
      case 56: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 965 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2672 "parser.c"
        break;
      case 57: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 968 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2679 "parser.c"
        break;
      case 58: /* params ::= params_with_default COMMA block_param */
#line 971 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2686 "parser.c"
        break;
      case 59: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 974 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2693 "parser.c"
        break;
      case 60: /* params ::= params_with_default COMMA var_param */
#line 977 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2700 "parser.c"
        break;
      case 61: /* params ::= params_with_default COMMA kw_param */
#line 980 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2707 "parser.c"
        break;
      case 62: /* params ::= params_with_default */
#line 983 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, yymsp[0].minor.yy127, YNIL, YNIL, YNIL);
}
#line 2714 "parser.c"
        break;
      case 63: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 986 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2721 "parser.c"
        break;
      case 64: /* params ::= block_param COMMA var_param */
#line 989 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2728 "parser.c"
        break;
      case 65: /* params ::= block_param COMMA kw_param */
#line 992 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2735 "parser.c"
        break;
      case 66: /* params ::= block_param */
#line 995 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2742 "parser.c"
        break;
      case 67: /* params ::= var_param COMMA kw_param */
#line 998 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2749 "parser.c"
        break;
      case 68: /* params ::= var_param */
#line 1001 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2756 "parser.c"
        break;
      case 69: /* params ::= kw_param */
#line 1004 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2763 "parser.c"
        break;
      case 70: /* params ::= */
#line 1007 "parser.y"
{
    yygotominor.yy127 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2770 "parser.c"
        break;
      case 71: /* kw_param ::= STAR_STAR NAME */
#line 1011 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2779 "parser.c"
        break;
      case 72: /* var_param ::= STAR NAME */
#line 1017 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2788 "parser.c"
        break;
      case 73: /* block_param ::= AMPER NAME param_default_opt */
#line 1023 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy127);
}
#line 2797 "parser.c"
        break;
      case 77: /* params_without_default ::= NAME */
#line 1040 "parser.y"
{
    yygotominor.yy127 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy127, lineno, id, YNIL);
}
#line 2807 "parser.c"
        break;
      case 78: /* params_without_default ::= params_without_default COMMA NAME */
#line 1046 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy127, lineno, id, YNIL);
    yygotominor.yy127 = yymsp[-2].minor.yy127;
}
#line 2817 "parser.c"
        break;
      case 81: /* param_with_default ::= NAME param_default */
#line 1060 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy127 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy127);
}
#line 2826 "parser.c"
        break;
      case 83: /* args ::= posargs */
#line 1069 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[0].minor.yy127, YNIL, YNIL, YNIL);
}
#line 2834 "parser.c"
        break;
      case 84: /* args ::= posargs COMMA kwargs */
#line 1073 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2842 "parser.c"
        break;
      case 85: /* args ::= posargs COMMA kwargs COMMA vararg */
#line 1077 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2850 "parser.c"
        break;
      case 86: /* args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg */
#line 1081 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-6].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-6].minor.yy127, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2858 "parser.c"
        break;
      case 87: /* args ::= posargs COMMA vararg */
#line 1085 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2866 "parser.c"
        break;
      case 88: /* args ::= posargs COMMA vararg COMMA varkwarg */
#line 1089 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-4].minor.yy127, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2874 "parser.c"
        break;
      case 89: /* args ::= posargs COMMA varkwarg */
#line 1093 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, yymsp[-2].minor.yy127, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2882 "parser.c"
        break;
      case 90: /* args ::= kwargs */
#line 1097 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, YNIL, yymsp[0].minor.yy127, YNIL, YNIL);
}
#line 2890 "parser.c"
        break;
      case 91: /* args ::= kwargs COMMA vararg */
#line 1101 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127, YNIL);
}
#line 2898 "parser.c"
        break;
      case 92: /* args ::= kwargs COMMA vararg COMMA varkwarg */
#line 1105 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, YNIL, yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2906 "parser.c"
        break;
      case 93: /* args ::= kwargs COMMA varkwarg */
#line 1109 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy127, 0));
    yygotominor.yy127 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy127, YNIL, yymsp[0].minor.yy127);
}
#line 2914 "parser.c"
        break;
      case 94: /* args ::= vararg */
#line 1113 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy127);
    yygotominor.yy127 = Args_new(env, lineno, YNIL, YNIL, yymsp[0].minor.yy127, YNIL);
}
#line 2922 "parser.c"
        break;
      case 95: /* args ::= vararg COMMA varkwarg */
#line 1117 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    yygotominor.yy127 = Args_new(env, lineno, YNIL, YNIL, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2930 "parser.c"
        break;
      case 96: /* args ::= varkwarg */
#line 1121 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy127);
    yygotominor.yy127 = Args_new(env, lineno, YNIL, YNIL, YNIL, yymsp[0].minor.yy127);
}
#line 2938 "parser.c"
        break;
      case 100: /* posargs ::= posargs COMMA expr */
      case 102: /* kwargs ::= kwargs COMMA kwarg */
      case 180: /* exprs ::= exprs COMMA expr */
      case 182: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 1137 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
    yygotominor.yy127 = yymsp[-2].minor.yy127;
}
#line 2949 "parser.c"
        break;
      case 103: /* kwarg ::= NAME COLON expr */
#line 1150 "parser.y"
{
    yygotominor.yy127 = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(yymsp[-2].minor.yy0));
    PTR_AS(YogNode, yygotominor.yy127)->u.kwarg.name = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    PTR_AS(YogNode, yygotominor.yy127)->u.kwarg.value = yymsp[0].minor.yy127;
}
#line 2958 "parser.c"
        break;
      case 105: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 1160 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    yygotominor.yy127 = Assign_new(env, lineno, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 2966 "parser.c"
        break;
      case 106: /* assign_expr ::= postfix_expr augmented_assign_op logical_or_expr */
#line 1164 "parser.y"
{
    yygotominor.yy127 = AugmentedAssign_new(env, NODE_LINENO(yymsp[-2].minor.yy127), yymsp[-2].minor.yy127, VAL2ID(yymsp[-1].minor.yy127), yymsp[0].minor.yy127);
}
#line 2973 "parser.c"
        break;
      case 107: /* assign_expr ::= postfix_expr AND_AND_EQUAL logical_or_expr */
#line 1167 "parser.y"
{
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    expr = LogicalAnd_new(env, lineno, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
    assign = Assign_new(env, lineno, yymsp[-2].minor.yy127, expr);

    POP_LOCALS(env);

    yygotominor.yy127 = assign;
}
#line 2990 "parser.c"
        break;
      case 108: /* assign_expr ::= postfix_expr BAR_BAR_EQUAL logical_or_expr */
#line 1180 "parser.y"
{
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    expr = LogicalOr_new(env, lineno, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
    assign = Assign_new(env, lineno, yymsp[-2].minor.yy127, expr);

    POP_LOCALS(env);

    yygotominor.yy127 = assign;
}
#line 3007 "parser.c"
        break;
      case 110: /* augmented_assign_op ::= PLUS_EQUAL */
#line 1197 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "+"));
}
#line 3014 "parser.c"
        break;
      case 111: /* augmented_assign_op ::= MINUS_EQUAL */
#line 1200 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "-"));
}
#line 3021 "parser.c"
        break;
      case 112: /* augmented_assign_op ::= STAR_EQUAL */
#line 1203 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "*"));
}
#line 3028 "parser.c"
        break;
      case 113: /* augmented_assign_op ::= DIV_EQUAL */
#line 1206 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "/"));
}
#line 3035 "parser.c"
        break;
      case 114: /* augmented_assign_op ::= DIV_DIV_EQUAL */
#line 1209 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "//"));
}
#line 3042 "parser.c"
        break;
      case 115: /* augmented_assign_op ::= PERCENT_EQUAL */
#line 1212 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "%"));
}
#line 3049 "parser.c"
        break;
      case 116: /* augmented_assign_op ::= BAR_EQUAL */
#line 1215 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "|"));
}
#line 3056 "parser.c"
        break;
      case 117: /* augmented_assign_op ::= AND_EQUAL */
#line 1218 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "&"));
}
#line 3063 "parser.c"
        break;
      case 118: /* augmented_assign_op ::= XOR_EQUAL */
#line 1221 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "^"));
}
#line 3070 "parser.c"
        break;
      case 119: /* augmented_assign_op ::= STAR_STAR_EQUAL */
#line 1224 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "**"));
}
#line 3077 "parser.c"
        break;
      case 120: /* augmented_assign_op ::= LSHIFT_EQUAL */
#line 1227 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, "<<"));
}
#line 3084 "parser.c"
        break;
      case 121: /* augmented_assign_op ::= RSHIFT_EQUAL */
#line 1230 "parser.y"
{
    yygotominor.yy127 = ID2VAL(YogVM_intern(env, env->vm, ">>"));
}
#line 3091 "parser.c"
        break;
      case 123: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 1237 "parser.y"
{
    yygotominor.yy127 = LogicalOr_new(env, NODE_LINENO(yymsp[-2].minor.yy127), yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 3098 "parser.c"
        break;
      case 125: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 1244 "parser.y"
{
    yygotominor.yy127 = LogicalAnd_new(env, NODE_LINENO(yymsp[-2].minor.yy127), yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 3105 "parser.c"
        break;
      case 127: /* not_expr ::= NOT not_expr */
#line 1251 "parser.y"
{
    yygotominor.yy127 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy127)->u.not.expr = yymsp[0].minor.yy127;
}
#line 3113 "parser.c"
        break;
      case 129: /* comparison ::= xor_expr comp_op xor_expr */
#line 1259 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy127)->u.id;
    yygotominor.yy127 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy127, id, yymsp[0].minor.yy127);
}
#line 3122 "parser.c"
        break;
      case 130: /* comp_op ::= LESS */
      case 131: /* comp_op ::= GREATER */
      case 186: /* comma_opt ::= COMMA */
#line 1265 "parser.y"
{
    yygotominor.yy127 = yymsp[0].minor.yy0;
}
#line 3131 "parser.c"
        break;
      case 133: /* xor_expr ::= xor_expr XOR or_expr */
      case 135: /* or_expr ::= or_expr BAR and_expr */
      case 137: /* and_expr ::= and_expr AND shift_expr */
#line 1275 "parser.y"
{
    yygotominor.yy127 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy127), yymsp[-2].minor.yy127, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy127);
}
#line 3140 "parser.c"
        break;
      case 139: /* shift_expr ::= shift_expr shift_op match_expr */
      case 145: /* arith_expr ::= arith_expr arith_op term */
      case 148: /* term ::= term term_op factor */
#line 1296 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    yygotominor.yy127 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy127, VAL2ID(yymsp[-1].minor.yy127), yymsp[0].minor.yy127);
}
#line 3150 "parser.c"
        break;
      case 140: /* shift_op ::= LSHIFT */
      case 141: /* shift_op ::= RSHIFT */
      case 146: /* arith_op ::= PLUS */
      case 147: /* arith_op ::= MINUS */
      case 150: /* term_op ::= STAR */
      case 151: /* term_op ::= DIV */
      case 152: /* term_op ::= DIV_DIV */
      case 153: /* term_op ::= PERCENT */
#line 1301 "parser.y"
{
    yygotominor.yy127 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 3164 "parser.c"
        break;
      case 143: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1311 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy127 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy127, id, yymsp[0].minor.yy127);
}
#line 3173 "parser.c"
        break;
      case 154: /* factor ::= PLUS factor */
#line 1353 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy127 = FuncCall_new3(env, lineno, yymsp[0].minor.yy127, id);
}
#line 3182 "parser.c"
        break;
      case 155: /* factor ::= MINUS factor */
#line 1358 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy127 = FuncCall_new3(env, lineno, yymsp[0].minor.yy127, id);
}
#line 3191 "parser.c"
        break;
      case 156: /* factor ::= TILDA factor */
#line 1363 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy127 = FuncCall_new3(env, lineno, yymsp[0].minor.yy127, id);
}
#line 3200 "parser.c"
        break;
      case 159: /* power ::= postfix_expr STAR_STAR factor */
#line 1375 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    ID id = YogVM_intern(env, env->vm, "**");
    yygotominor.yy127 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy127, id, yymsp[0].minor.yy127);
}
#line 3209 "parser.c"
        break;
      case 161: /* postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt */
#line 1384 "parser.y"
{
    yygotominor.yy127 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy127), yymsp[-4].minor.yy127, yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 3216 "parser.c"
        break;
      case 162: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1387 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy127);
    yygotominor.yy127 = Subscript_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 3224 "parser.c"
        break;
      case 163: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1391 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy127);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Attr_new(env, lineno, yymsp[-2].minor.yy127, id);
}
#line 3233 "parser.c"
        break;
      case 164: /* atom ::= NAME */
#line 1397 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy127 = Variable_new(env, lineno, id);
}
#line 3242 "parser.c"
        break;
      case 165: /* atom ::= NUMBER */
      case 166: /* atom ::= REGEXP */
      case 167: /* atom ::= STRING */
      case 168: /* atom ::= SYMBOL */
#line 1402 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy127 = Literal_new(env, lineno, val);
}
#line 3254 "parser.c"
        break;
      case 169: /* atom ::= NIL */
#line 1422 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Literal_new(env, lineno, YNIL);
}
#line 3262 "parser.c"
        break;
      case 170: /* atom ::= TRUE */
#line 1426 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Literal_new(env, lineno, YTRUE);
}
#line 3270 "parser.c"
        break;
      case 171: /* atom ::= FALSE */
#line 1430 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy127 = Literal_new(env, lineno, YFALSE);
}
#line 3278 "parser.c"
        break;
      case 172: /* atom ::= LINE */
#line 1434 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy127 = Literal_new(env, lineno, val);
}
#line 3287 "parser.c"
        break;
      case 173: /* atom ::= LBRACKET exprs RBRACKET */
#line 1439 "parser.y"
{
    yygotominor.yy127 = Array_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy127);
}
#line 3294 "parser.c"
        break;
      case 174: /* atom ::= LBRACKET RBRACKET */
#line 1442 "parser.y"
{
    yygotominor.yy127 = Array_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3301 "parser.c"
        break;
      case 175: /* atom ::= LBRACE RBRACE */
#line 1445 "parser.y"
{
    yygotominor.yy127 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3308 "parser.c"
        break;
      case 176: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1448 "parser.y"
{
    yygotominor.yy127 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy127);
}
#line 3315 "parser.c"
        break;
      case 177: /* atom ::= LBRACE exprs RBRACE */
#line 1451 "parser.y"
{
    yygotominor.yy127 = Set_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy127);
}
#line 3322 "parser.c"
        break;
      case 183: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1473 "parser.y"
{
    yygotominor.yy127 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy127), yymsp[-2].minor.yy127, yymsp[0].minor.yy127);
}
#line 3329 "parser.c"
        break;
      case 184: /* dict_elem ::= NAME COLON expr */
#line 1476 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy127 = DictElem_new(env, lineno, var, yymsp[0].minor.yy127);
}
#line 3339 "parser.c"
        break;
      case 188: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 189: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1493 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy127 = BlockArg_new(env, lineno, yymsp[-3].minor.yy127, yymsp[-1].minor.yy127);
}
#line 3348 "parser.c"
        break;
      case 194: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1516 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy127 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy127, id, yymsp[0].minor.yy127);
}
#line 3358 "parser.c"
        break;
      case 195: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1522 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy127 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy127, NO_EXC_VAR, yymsp[0].minor.yy127);
}
#line 3366 "parser.c"
        break;
      case 196: /* except ::= EXCEPT NEWLINE stmts */
#line 1526 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy127 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy127);
}
#line 3374 "parser.c"
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
