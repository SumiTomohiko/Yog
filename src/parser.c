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
#define YYNOCODE 137
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy111;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 328
#define YYNRULE 200
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
 /*     0 */     1,  173,  174,  262,   24,   25,   33,   34,   35,  362,
 /*    10 */   218,  158,   94,   75,  170,  177,  179,  295,  362,   28,
 /*    20 */   296,   37,  172,  283,  150,  141,  147,  149,  261,  257,
 /*    30 */   207,   77,  131,  144,  132,  227,  209,   78,  156,  133,
 /*    40 */   134,   84,  135,   47,   85,   80,   31,  166,  236,  215,
 /*    50 */   217,  194,   59,   60,  171,  175,  286,   61,   21,  290,
 /*    60 */   219,  220,  221,  222,  223,  224,  225,  226,   20,  529,
 /*    70 */   110,  203,  201,  202,  181,  183,  300,  115,  275,  290,
 /*    80 */   109,  148,  259,  252,   96,  278,  127,   26,  133,  134,
 /*    90 */    84,  135,   18,   85,   80,  232,   16,  236,  215,  217,
 /*   100 */   207,   77,  131,  325,  132,  227,  209,   78,   39,  133,
 /*   110 */   134,   84,  135,   18,   85,   80,  109,    3,  236,  215,
 /*   120 */   217,   42,   66,  203,  201,  202,  233,  234,  235,  115,
 /*   130 */    79,  109,  237,  236,  215,  217,   96,  278,   58,  128,
 /*   140 */   134,   84,  135,  122,   85,   80,   67,   18,  236,  215,
 /*   150 */   217,   48,  207,   77,  131,  311,  132,  227,  209,   78,
 /*   160 */    52,  133,  134,   84,  135,   23,   85,   80,  153,  269,
 /*   170 */   236,  215,  217,   56,  268,  268,  287,  288,  109,   81,
 /*   180 */   203,  201,  202,  309,  310,   49,  115,  129,   84,  135,
 /*   190 */   263,   85,   80,   96,  278,  236,  215,  217,  168,  327,
 /*   200 */    14,  169,  180,  184,  186,  304,  228,  229,  296,  207,
 /*   210 */    77,  131,   22,  132,  227,  209,   78,  306,  133,  134,
 /*   220 */    84,  135,  138,   85,   80,  178,  293,  236,  215,  217,
 /*   230 */   109,  123,  203,  201,  202,  182,  298,   30,  115,  109,
 /*   240 */    82,  135,  144,   85,   80,   96,  278,  236,  215,  217,
 /*   250 */   130,  249,   85,   80,   30,   31,  236,  215,  217,  185,
 /*   260 */   302,  207,   77,  131,  254,  132,  227,  209,   78,  258,
 /*   270 */   133,  134,   84,  135,  195,   85,   80,  161,  164,  236,
 /*   280 */   215,  217,  230,  231,  143,  151,  252,  260,  111,  203,
 /*   290 */   201,  202,  109,   54,  187,  115,  109,  169,  180,  184,
 /*   300 */   186,  304,   96,  278,  296,   83,   80,  328,   18,  236,
 /*   310 */   215,  217,   38,  211,  215,  217,  273,  157,  207,   77,
 /*   320 */   131,  159,  132,  227,  209,   78,  162,  133,  134,   84,
 /*   330 */   135,   37,   85,   80,  109,  276,  236,  215,  217,   26,
 /*   340 */   114,  203,  201,  202,    2,  107,    3,  115,  109,  102,
 /*   350 */   173,  212,  215,  217,   96,  278,  281,  173,  174,  176,
 /*   360 */   291,  173,  174,  176,  285,  213,  215,  217,  292,  294,
 /*   370 */   207,   77,  131,  297,  132,  227,  209,   78,  299,  133,
 /*   380 */   134,   84,  135,   18,   85,   80,  205,  301,  236,  215,
 /*   390 */   217,  109,   18,   99,  244,  264,  303,   68,  203,  201,
 /*   400 */   202,  109,  188,  204,  115,  173,  174,  176,  214,  215,
 /*   410 */   217,   96,  278,  108,   18,   23,   92,  190,  216,  215,
 /*   420 */   217,   17,   18,   18,   62,  270,  271,  207,   77,  131,
 /*   430 */     4,  132,  227,  209,   78,   45,  133,  134,   84,  135,
 /*   440 */    18,   85,   80,  280,   49,  236,  215,  217,   46,   69,
 /*   450 */   203,  201,  202,   18,   18,    8,  115,  326,   50,   55,
 /*   460 */    40,   51,   27,   96,  278,  238,  241,   19,   32,   29,
 /*   470 */    70,   36,   88,   89,   65,   90,   91,   86,    5,  207,
 /*   480 */    77,  131,    6,  132,  227,  209,   78,  267,  133,  134,
 /*   490 */    84,  135,    7,   85,   80,    9,   93,  236,  215,  217,
 /*   500 */    10,  160,  272,   95,  274,  277,  155,  203,  201,  202,
 /*   510 */   163,  167,  282,  115,   57,  530,   53,   63,   11,   71,
 /*   520 */    96,  278,   97,  284,   98,   76,   72,  305,  100,   12,
 /*   530 */   101,   64,  307,   73,  103,  104,  207,   77,  131,   74,
 /*   540 */   132,  227,  209,   78,  308,  133,  134,   84,  135,  324,
 /*   550 */    85,   80,  105,   13,  236,  215,  217,  106,  116,  203,
 /*   560 */   201,  202,  196,  530,  530,  115,  530,  530,  530,  530,
 /*   570 */   530,  530,   96,  278,  530,  530,  530,  530,  530,  530,
 /*   580 */   530,  530,  530,  530,  530,  530,  530,  530,  207,   77,
 /*   590 */   131,  530,  132,  227,  209,   78,  530,  133,  134,   84,
 /*   600 */   135,  530,   85,   80,  530,  530,  236,  215,  217,  530,
 /*   610 */   530,  530,  530,  530,  530,  117,  203,  201,  202,  530,
 /*   620 */   530,  530,  115,  530,  530,  530,  530,  530,  530,   96,
 /*   630 */   278,  530,  530,  530,  530,  530,  530,  530,  530,  530,
 /*   640 */   530,  530,  530,  530,  530,  207,   77,  131,  530,  132,
 /*   650 */   227,  209,   78,  530,  133,  134,   84,  135,  530,   85,
 /*   660 */    80,  530,  530,  236,  215,  217,  530,  118,  203,  201,
 /*   670 */   202,  530,  530,  530,  115,  530,  530,  530,  530,  530,
 /*   680 */   530,   96,  278,  530,  530,  530,  530,  530,  530,  530,
 /*   690 */   530,  530,  530,  530,  530,  530,  530,  207,   77,  131,
 /*   700 */   530,  132,  227,  209,   78,  530,  133,  134,   84,  135,
 /*   710 */   530,   85,   80,  530,  530,  236,  215,  217,  530,  530,
 /*   720 */   530,  530,  530,  530,  119,  203,  201,  202,  530,  530,
 /*   730 */   530,  115,  530,  530,  530,  530,  530,  530,   96,  278,
 /*   740 */   530,  530,  530,  530,  530,  530,  530,  530,  530,  530,
 /*   750 */   530,  530,  530,  530,  207,   77,  131,  530,  132,  227,
 /*   760 */   209,   78,  530,  133,  134,   84,  135,  530,   85,   80,
 /*   770 */   530,  530,  236,  215,  217,  530,  197,  203,  201,  202,
 /*   780 */   530,  530,  530,  115,  530,  530,  530,  530,  530,  530,
 /*   790 */    96,  278,  530,  530,  530,  530,  530,  530,  530,  530,
 /*   800 */   530,  530,  530,  530,  530,  530,  207,   77,  131,  530,
 /*   810 */   132,  227,  209,   78,  530,  133,  134,   84,  135,  530,
 /*   820 */    85,   80,  530,  530,  236,  215,  217,  530,  530,  530,
 /*   830 */   530,  530,  530,  198,  203,  201,  202,  530,  530,  530,
 /*   840 */   115,  530,  530,  530,  530,  530,  530,   96,  278,  530,
 /*   850 */   530,  530,  530,  530,  530,  530,  530,  530,  530,  530,
 /*   860 */   530,  530,  530,  207,   77,  131,  530,  132,  227,  209,
 /*   870 */    78,  530,  133,  134,   84,  135,  530,   85,   80,  530,
 /*   880 */   530,  236,  215,  217,  530,  199,  203,  201,  202,  530,
 /*   890 */   530,  530,  115,  530,  530,  530,  530,  530,  530,   96,
 /*   900 */   278,  530,  530,  530,  530,  530,  530,  530,  530,  530,
 /*   910 */   530,  530,  530,  530,  530,  207,   77,  131,  530,  132,
 /*   920 */   227,  209,   78,  530,  133,  134,   84,  135,  530,   85,
 /*   930 */    80,  530,  530,  236,  215,  217,  530,  530,  530,  530,
 /*   940 */   530,  530,  121,  203,  201,  202,  530,  530,  530,  115,
 /*   950 */   530,  530,  530,  530,  530,  530,   96,  278,  530,  530,
 /*   960 */   530,  530,  530,  530,  530,  530,  530,  530,  530,  530,
 /*   970 */   530,  530,  207,   77,  131,  530,  132,  227,  209,   78,
 /*   980 */   530,  133,  134,   84,  135,  530,   85,   80,  530,  530,
 /*   990 */   236,  215,  217,  530,  530,  200,  201,  202,  530,  530,
 /*  1000 */   530,  115,  530,  530,  530,  530,  530,  530,   96,  278,
 /*  1010 */   530,  530,  530,  530,  530,  530,  530,  530,  530,  530,
 /*  1020 */   530,  530,  530,  530,  207,   77,  131,  256,  132,  227,
 /*  1030 */   209,   78,  530,  133,  134,   84,  135,  530,   85,   80,
 /*  1040 */   530,  530,  236,  215,  217,  530,  530,  530,  530,  530,
 /*  1050 */   142,  145,  255,  257,  207,   77,  131,  530,  132,  227,
 /*  1060 */   209,   78,  530,  133,  134,   84,  135,  530,   85,   80,
 /*  1070 */   530,  530,  236,  215,  217,  530,  530,  140,  190,  530,
 /*  1080 */   530,  530,   17,  530,  530,   62,  530,  530,   41,  530,
 /*  1090 */    43,   44,  312,  313,  314,  315,  316,  317,  318,  319,
 /*  1100 */   320,  321,  322,  323,  207,   77,  131,  530,  132,  227,
 /*  1110 */   209,   78,  530,  133,  134,   84,  135,  248,   85,   80,
 /*  1120 */   530,   40,  236,  215,  217,  530,  113,   87,  146,  245,
 /*  1130 */   530,  530,  530,  530,  530,  530,  530,   28,  530,  530,
 /*  1140 */    30,   31,  530,  530,  207,   77,  131,  530,  132,  227,
 /*  1150 */   209,   78,  530,  133,  134,   84,  135,  530,   85,   80,
 /*  1160 */   139,   47,  236,  215,  217,  530,  112,  530,  530,  530,
 /*  1170 */    59,   60,  530,  530,  530,   61,   21,  530,  219,  220,
 /*  1180 */   221,  222,  223,  224,  225,  226,   20,  207,   77,  131,
 /*  1190 */   530,  132,  227,  209,   78,  136,  133,  134,   84,  135,
 /*  1200 */   530,   85,   80,  530,   28,  236,  215,  217,  530,  530,
 /*  1210 */   530,  530,  242,  109,  125,  530,  132,  227,  209,   78,
 /*  1220 */   530,  133,  134,   84,  135,  218,   85,   80,   47,  530,
 /*  1230 */   236,  215,  217,  530,   28,  530,  530,   59,   60,  530,
 /*  1240 */   530,  530,   61,   21,   15,  219,  220,  221,  222,  223,
 /*  1250 */   224,  225,  226,   20,  240,  218,  530,  530,   47,  530,
 /*  1260 */   530,  530,  530,  530,   28,  530,  530,   59,   60,  530,
 /*  1270 */   530,  530,   61,   21,  247,  219,  220,  221,  222,  223,
 /*  1280 */   224,  225,  226,   20,  530,  530,  530,  120,   47,  530,
 /*  1290 */   530,  530,  530,  530,  530,  530,  530,   59,   60,  530,
 /*  1300 */   530,  530,   61,   21,  530,  219,  220,  221,  222,  223,
 /*  1310 */   224,  225,  226,   20,  207,   77,  131,  530,  132,  227,
 /*  1320 */   209,   78,  530,  133,  134,   84,  135,  124,   85,   80,
 /*  1330 */   530,  530,  236,  215,  217,  530,  530,  530,  530,  530,
 /*  1340 */   530,  530,  530,  530,  530,  530,  530,  530,  530,  206,
 /*  1350 */   530,  530,  530,  530,  207,   77,  131,  530,  132,  227,
 /*  1360 */   209,   78,  530,  133,  134,   84,  135,  530,   85,   80,
 /*  1370 */   246,  530,  236,  215,  217,  530,  207,   77,  131,  530,
 /*  1380 */   132,  227,  209,   78,  530,  133,  134,   84,  135,  530,
 /*  1390 */    85,   80,  239,  530,  236,  215,  217,  207,   77,  131,
 /*  1400 */   530,  132,  227,  209,   78,  530,  133,  134,   84,  135,
 /*  1410 */   530,   85,   80,  530,  137,  236,  215,  217,  530,  207,
 /*  1420 */    77,  131,  530,  132,  227,  209,   78,  530,  133,  134,
 /*  1430 */    84,  135,  530,   85,   80,  530,  243,  236,  215,  217,
 /*  1440 */   530,  207,   77,  131,  530,  132,  227,  209,   78,  530,
 /*  1450 */   133,  134,   84,  135,  530,   85,   80,  530,  250,  236,
 /*  1460 */   215,  217,  530,  207,   77,  131,  530,  132,  227,  209,
 /*  1470 */    78,  530,  133,  134,   84,  135,  530,   85,   80,  251,
 /*  1480 */   530,  236,  215,  217,  530,  207,   77,  131,  530,  132,
 /*  1490 */   227,  209,   78,  530,  133,  134,   84,  135,  530,   85,
 /*  1500 */    80,  253,  530,  236,  215,  217,  207,   77,  131,  530,
 /*  1510 */   132,  227,  209,   78,  530,  133,  134,   84,  135,  530,
 /*  1520 */    85,   80,  530,  265,  236,  215,  217,  530,  207,   77,
 /*  1530 */   131,  530,  132,  227,  209,   78,  530,  133,  134,   84,
 /*  1540 */   135,  530,   85,   80,  530,  266,  236,  215,  217,  530,
 /*  1550 */   207,   77,  131,  530,  132,  227,  209,   78,  530,  133,
 /*  1560 */   134,   84,  135,  530,   85,   80,  530,  152,  236,  215,
 /*  1570 */   217,  530,  207,   77,  131,  530,  132,  227,  209,   78,
 /*  1580 */   530,  133,  134,   84,  135,  530,   85,   80,  154,  530,
 /*  1590 */   236,  215,  217,  530,  207,   77,  131,  530,  132,  227,
 /*  1600 */   209,   78,  530,  133,  134,   84,  135,  530,   85,   80,
 /*  1610 */   165,  530,  236,  215,  217,  207,   77,  131,  530,  132,
 /*  1620 */   227,  209,   78,  530,  133,  134,   84,  135,  530,   85,
 /*  1630 */    80,  530,  279,  236,  215,  217,  530,  207,   77,  131,
 /*  1640 */   530,  132,  227,  209,   78,  530,  133,  134,   84,  135,
 /*  1650 */   530,   85,   80,  530,  289,  236,  215,  217,  530,  207,
 /*  1660 */    77,  131,  530,  132,  227,  209,   78,  530,  133,  134,
 /*  1670 */    84,  135,  530,   85,   80,  530,  189,  236,  215,  217,
 /*  1680 */   530,  207,   77,  131,  530,  132,  227,  209,   78,  530,
 /*  1690 */   133,  134,   84,  135,  530,   85,   80,  530,  530,  236,
 /*  1700 */   215,  217,  530,  207,   77,  131,  530,  132,  227,  209,
 /*  1710 */    78,  530,  133,  134,   84,  135,  136,   85,   80,  530,
 /*  1720 */   530,  236,  215,  217,  530,   28,  109,  191,  530,  132,
 /*  1730 */   227,  209,   78,  530,  133,  134,   84,  135,  530,   85,
 /*  1740 */    80,  530,  530,  236,  215,  217,  530,  530,  530,   47,
 /*  1750 */   530,  530,  530,  530,  530,  530,  530,  530,   59,   60,
 /*  1760 */   530,  530,  530,   61,   21,  530,  219,  220,  221,  222,
 /*  1770 */   223,  224,  225,  226,   20,  530,  218,  530,  109,  192,
 /*  1780 */   530,  132,  227,  209,   78,   28,  133,  134,   84,  135,
 /*  1790 */   530,   85,   80,  530,  530,  236,  215,  217,  530,  530,
 /*  1800 */   530,  530,  530,  530,  530,  530,  530,  530,  218,   47,
 /*  1810 */   530,  530,  530,  530,  530,  530,  530,   28,   59,   60,
 /*  1820 */   530,  530,  530,   61,   21,  530,  219,  220,  221,  222,
 /*  1830 */   223,  224,  225,  226,   20,  109,  193,  530,  132,  227,
 /*  1840 */   209,   78,  530,  133,  134,   84,  135,  530,   85,   80,
 /*  1850 */    59,   60,  236,  215,  217,   61,   21,  530,  219,  220,
 /*  1860 */   221,  222,  223,  224,  225,  226,   20,  530,  109,  530,
 /*  1870 */   530,  126,  227,  209,   78,  530,  133,  134,   84,  135,
 /*  1880 */   530,   85,   80,  530,  109,  236,  215,  217,  208,  209,
 /*  1890 */    78,  530,  133,  134,   84,  135,  530,   85,   80,  530,
 /*  1900 */   530,  236,  215,  217,  530,  530,  109,  530,  530,  530,
 /*  1910 */   210,  209,   78,  530,  133,  134,   84,  135,  530,   85,
 /*  1920 */    80,  530,  530,  236,  215,  217,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   24,   25,   81,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   15,   95,   96,   97,   98,   20,   21,
 /*    20 */   101,   23,   97,   98,  102,  103,  104,  105,  106,  107,
 /*    30 */   108,  109,  110,   12,  112,  113,  114,  115,   11,  117,
 /*    40 */   118,  119,  120,   45,  122,  123,   25,   20,  126,  127,
 /*    50 */   128,   83,   54,   55,   96,   97,   98,   59,   60,  101,
 /*    60 */    62,   63,   64,   65,   66,   67,   68,   69,   70,   77,
 /*    70 */    78,   79,   80,   81,   96,   97,   98,   85,   12,  101,
 /*    80 */   109,  105,  106,  107,   92,   93,  115,   16,  117,  118,
 /*    90 */   119,  120,    1,  122,  123,   25,    5,  126,  127,  128,
 /*   100 */   108,  109,  110,  135,  112,  113,  114,  115,   27,  117,
 /*   110 */   118,  119,  120,    1,  122,  123,  109,    5,  126,  127,
 /*   120 */   128,  111,   78,   79,   80,   81,   56,   57,   58,   85,
 /*   130 */   123,  109,   61,  126,  127,  128,   92,   93,  125,  117,
 /*   140 */   118,  119,  120,   83,  122,  123,   82,    1,  126,  127,
 /*   150 */   128,  116,  108,  109,  110,   18,  112,  113,  114,  115,
 /*   160 */   121,  117,  118,  119,  120,   74,  122,  123,   84,   84,
 /*   170 */   126,  127,  128,  124,   90,   90,   99,  100,  109,   78,
 /*   180 */    79,   80,   81,   46,   47,   48,   85,  118,  119,  120,
 /*   190 */   129,  122,  123,   92,   93,  126,  127,  128,   91,  135,
 /*   200 */     1,   94,   95,   96,   97,   98,   51,   52,  101,  108,
 /*   210 */   109,  110,   16,  112,  113,  114,  115,   71,  117,  118,
 /*   220 */   119,  120,  132,  122,  123,   97,   98,  126,  127,  128,
 /*   230 */   109,   78,   79,   80,   81,   97,   98,   24,   85,  109,
 /*   240 */   119,  120,   12,  122,  123,   92,   93,  126,  127,  128,
 /*   250 */   120,  106,  122,  123,   24,   25,  126,  127,  128,   97,
 /*   260 */    98,  108,  109,  110,  106,  112,  113,  114,  115,  106,
 /*   270 */   117,  118,  119,  120,   75,  122,  123,   88,   89,  126,
 /*   280 */   127,  128,   54,   55,  105,  134,  107,  106,   78,   79,
 /*   290 */    80,   81,  109,   60,   91,   85,  109,   94,   95,   96,
 /*   300 */    97,   98,   92,   93,  101,  122,  123,    0,    1,  126,
 /*   310 */   127,  128,   18,  126,  127,  128,   12,   86,  108,  109,
 /*   320 */   110,   87,  112,  113,  114,  115,   89,  117,  118,  119,
 /*   330 */   120,   23,  122,  123,  109,   93,  126,  127,  128,   16,
 /*   340 */    78,   79,   80,   81,    3,   12,    5,   85,  109,   12,
 /*   350 */    24,  126,  127,  128,   92,   93,   98,   24,   25,   26,
 /*   360 */   100,   24,   25,   26,   98,  126,  127,  128,   98,   98,
 /*   370 */   108,  109,  110,   98,  112,  113,  114,  115,   98,  117,
 /*   380 */   118,  119,  120,    1,  122,  123,    4,   98,  126,  127,
 /*   390 */   128,  109,    1,   12,   71,    4,   98,   78,   79,   80,
 /*   400 */    81,  109,  134,    4,   85,   24,   25,   26,  126,  127,
 /*   410 */   128,   92,   93,   70,    1,   74,   73,   17,  126,  127,
 /*   420 */   128,   21,    1,    1,   24,    4,    4,  108,  109,  110,
 /*   430 */     1,  112,  113,  114,  115,   43,  117,  118,  119,  120,
 /*   440 */     1,  122,  123,    4,   48,  126,  127,  128,   44,   78,
 /*   450 */    79,   80,   81,    1,    1,    3,   85,    4,   49,   53,
 /*   460 */    60,   50,   28,   92,   93,   22,   71,   16,   28,   72,
 /*   470 */    16,   19,   16,   16,   16,   16,   16,   22,    1,  108,
 /*   480 */   109,  110,    1,  112,  113,  114,  115,    4,  117,  118,
 /*   490 */   119,  120,    1,  122,  123,    1,   12,  126,  127,  128,
 /*   500 */    12,   16,   12,   16,   12,    1,   78,   79,   80,   81,
 /*   510 */    17,   12,   12,   85,   16,  136,   21,   16,   22,   16,
 /*   520 */    92,   93,   16,   12,   16,   12,   16,   61,   16,    1,
 /*   530 */    16,   16,   61,   16,   16,   16,  108,  109,  110,   16,
 /*   540 */   112,  113,  114,  115,   12,  117,  118,  119,  120,    4,
 /*   550 */   122,  123,   16,    1,  126,  127,  128,   16,   78,   79,
 /*   560 */    80,   81,   12,  136,  136,   85,  136,  136,  136,  136,
 /*   570 */   136,  136,   92,   93,  136,  136,  136,  136,  136,  136,
 /*   580 */   136,  136,  136,  136,  136,  136,  136,  136,  108,  109,
 /*   590 */   110,  136,  112,  113,  114,  115,  136,  117,  118,  119,
 /*   600 */   120,  136,  122,  123,  136,  136,  126,  127,  128,  136,
 /*   610 */   136,  136,  136,  136,  136,   78,   79,   80,   81,  136,
 /*   620 */   136,  136,   85,  136,  136,  136,  136,  136,  136,   92,
 /*   630 */    93,  136,  136,  136,  136,  136,  136,  136,  136,  136,
 /*   640 */   136,  136,  136,  136,  136,  108,  109,  110,  136,  112,
 /*   650 */   113,  114,  115,  136,  117,  118,  119,  120,  136,  122,
 /*   660 */   123,  136,  136,  126,  127,  128,  136,   78,   79,   80,
 /*   670 */    81,  136,  136,  136,   85,  136,  136,  136,  136,  136,
 /*   680 */   136,   92,   93,  136,  136,  136,  136,  136,  136,  136,
 /*   690 */   136,  136,  136,  136,  136,  136,  136,  108,  109,  110,
 /*   700 */   136,  112,  113,  114,  115,  136,  117,  118,  119,  120,
 /*   710 */   136,  122,  123,  136,  136,  126,  127,  128,  136,  136,
 /*   720 */   136,  136,  136,  136,   78,   79,   80,   81,  136,  136,
 /*   730 */   136,   85,  136,  136,  136,  136,  136,  136,   92,   93,
 /*   740 */   136,  136,  136,  136,  136,  136,  136,  136,  136,  136,
 /*   750 */   136,  136,  136,  136,  108,  109,  110,  136,  112,  113,
 /*   760 */   114,  115,  136,  117,  118,  119,  120,  136,  122,  123,
 /*   770 */   136,  136,  126,  127,  128,  136,   78,   79,   80,   81,
 /*   780 */   136,  136,  136,   85,  136,  136,  136,  136,  136,  136,
 /*   790 */    92,   93,  136,  136,  136,  136,  136,  136,  136,  136,
 /*   800 */   136,  136,  136,  136,  136,  136,  108,  109,  110,  136,
 /*   810 */   112,  113,  114,  115,  136,  117,  118,  119,  120,  136,
 /*   820 */   122,  123,  136,  136,  126,  127,  128,  136,  136,  136,
 /*   830 */   136,  136,  136,   78,   79,   80,   81,  136,  136,  136,
 /*   840 */    85,  136,  136,  136,  136,  136,  136,   92,   93,  136,
 /*   850 */   136,  136,  136,  136,  136,  136,  136,  136,  136,  136,
 /*   860 */   136,  136,  136,  108,  109,  110,  136,  112,  113,  114,
 /*   870 */   115,  136,  117,  118,  119,  120,  136,  122,  123,  136,
 /*   880 */   136,  126,  127,  128,  136,   78,   79,   80,   81,  136,
 /*   890 */   136,  136,   85,  136,  136,  136,  136,  136,  136,   92,
 /*   900 */    93,  136,  136,  136,  136,  136,  136,  136,  136,  136,
 /*   910 */   136,  136,  136,  136,  136,  108,  109,  110,  136,  112,
 /*   920 */   113,  114,  115,  136,  117,  118,  119,  120,  136,  122,
 /*   930 */   123,  136,  136,  126,  127,  128,  136,  136,  136,  136,
 /*   940 */   136,  136,   78,   79,   80,   81,  136,  136,  136,   85,
 /*   950 */   136,  136,  136,  136,  136,  136,   92,   93,  136,  136,
 /*   960 */   136,  136,  136,  136,  136,  136,  136,  136,  136,  136,
 /*   970 */   136,  136,  108,  109,  110,  136,  112,  113,  114,  115,
 /*   980 */   136,  117,  118,  119,  120,  136,  122,  123,  136,  136,
 /*   990 */   126,  127,  128,  136,  136,   79,   80,   81,  136,  136,
 /*  1000 */   136,   85,  136,  136,  136,  136,  136,  136,   92,   93,
 /*  1010 */   136,  136,  136,  136,  136,  136,  136,  136,  136,  136,
 /*  1020 */   136,  136,  136,  136,  108,  109,  110,   81,  112,  113,
 /*  1030 */   114,  115,  136,  117,  118,  119,  120,  136,  122,  123,
 /*  1040 */   136,  136,  126,  127,  128,  136,  136,  136,  136,  136,
 /*  1050 */   104,  105,  106,  107,  108,  109,  110,  136,  112,  113,
 /*  1060 */   114,  115,  136,  117,  118,  119,  120,  136,  122,  123,
 /*  1070 */   136,  136,  126,  127,  128,  136,  136,   81,   17,  136,
 /*  1080 */   136,  136,   21,  136,  136,   24,  136,  136,   27,  136,
 /*  1090 */    29,   30,   31,   32,   33,   34,   35,   36,   37,   38,
 /*  1100 */    39,   40,   41,   42,  108,  109,  110,  136,  112,  113,
 /*  1110 */   114,  115,  136,  117,  118,  119,  120,   81,  122,  123,
 /*  1120 */   136,   60,  126,  127,  128,  136,  130,  131,   12,  133,
 /*  1130 */   136,  136,  136,  136,  136,  136,  136,   21,  136,  136,
 /*  1140 */    24,   25,  136,  136,  108,  109,  110,  136,  112,  113,
 /*  1150 */   114,  115,  136,  117,  118,  119,  120,  136,  122,  123,
 /*  1160 */    81,   45,  126,  127,  128,  136,  130,  136,  136,  136,
 /*  1170 */    54,   55,  136,  136,  136,   59,   60,  136,   62,   63,
 /*  1180 */    64,   65,   66,   67,   68,   69,   70,  108,  109,  110,
 /*  1190 */   136,  112,  113,  114,  115,   12,  117,  118,  119,  120,
 /*  1200 */   136,  122,  123,  136,   21,  126,  127,  128,  136,  136,
 /*  1210 */   136,  136,  133,  109,  110,  136,  112,  113,  114,  115,
 /*  1220 */   136,  117,  118,  119,  120,   12,  122,  123,   45,  136,
 /*  1230 */   126,  127,  128,  136,   21,  136,  136,   54,   55,  136,
 /*  1240 */   136,  136,   59,   60,    1,   62,   63,   64,   65,   66,
 /*  1250 */    67,   68,   69,   70,   71,   12,  136,  136,   45,  136,
 /*  1260 */   136,  136,  136,  136,   21,  136,  136,   54,   55,  136,
 /*  1270 */   136,  136,   59,   60,   61,   62,   63,   64,   65,   66,
 /*  1280 */    67,   68,   69,   70,  136,  136,  136,   81,   45,  136,
 /*  1290 */   136,  136,  136,  136,  136,  136,  136,   54,   55,  136,
 /*  1300 */   136,  136,   59,   60,  136,   62,   63,   64,   65,   66,
 /*  1310 */    67,   68,   69,   70,  108,  109,  110,  136,  112,  113,
 /*  1320 */   114,  115,  136,  117,  118,  119,  120,   81,  122,  123,
 /*  1330 */   136,  136,  126,  127,  128,  136,  136,  136,  136,  136,
 /*  1340 */   136,  136,  136,  136,  136,  136,  136,  136,  136,   81,
 /*  1350 */   136,  136,  136,  136,  108,  109,  110,  136,  112,  113,
 /*  1360 */   114,  115,  136,  117,  118,  119,  120,  136,  122,  123,
 /*  1370 */    81,  136,  126,  127,  128,  136,  108,  109,  110,  136,
 /*  1380 */   112,  113,  114,  115,  136,  117,  118,  119,  120,  136,
 /*  1390 */   122,  123,   81,  136,  126,  127,  128,  108,  109,  110,
 /*  1400 */   136,  112,  113,  114,  115,  136,  117,  118,  119,  120,
 /*  1410 */   136,  122,  123,  136,   81,  126,  127,  128,  136,  108,
 /*  1420 */   109,  110,  136,  112,  113,  114,  115,  136,  117,  118,
 /*  1430 */   119,  120,  136,  122,  123,  136,   81,  126,  127,  128,
 /*  1440 */   136,  108,  109,  110,  136,  112,  113,  114,  115,  136,
 /*  1450 */   117,  118,  119,  120,  136,  122,  123,  136,   81,  126,
 /*  1460 */   127,  128,  136,  108,  109,  110,  136,  112,  113,  114,
 /*  1470 */   115,  136,  117,  118,  119,  120,  136,  122,  123,   81,
 /*  1480 */   136,  126,  127,  128,  136,  108,  109,  110,  136,  112,
 /*  1490 */   113,  114,  115,  136,  117,  118,  119,  120,  136,  122,
 /*  1500 */   123,   81,  136,  126,  127,  128,  108,  109,  110,  136,
 /*  1510 */   112,  113,  114,  115,  136,  117,  118,  119,  120,  136,
 /*  1520 */   122,  123,  136,   81,  126,  127,  128,  136,  108,  109,
 /*  1530 */   110,  136,  112,  113,  114,  115,  136,  117,  118,  119,
 /*  1540 */   120,  136,  122,  123,  136,   81,  126,  127,  128,  136,
 /*  1550 */   108,  109,  110,  136,  112,  113,  114,  115,  136,  117,
 /*  1560 */   118,  119,  120,  136,  122,  123,  136,   81,  126,  127,
 /*  1570 */   128,  136,  108,  109,  110,  136,  112,  113,  114,  115,
 /*  1580 */   136,  117,  118,  119,  120,  136,  122,  123,   81,  136,
 /*  1590 */   126,  127,  128,  136,  108,  109,  110,  136,  112,  113,
 /*  1600 */   114,  115,  136,  117,  118,  119,  120,  136,  122,  123,
 /*  1610 */    81,  136,  126,  127,  128,  108,  109,  110,  136,  112,
 /*  1620 */   113,  114,  115,  136,  117,  118,  119,  120,  136,  122,
 /*  1630 */   123,  136,   81,  126,  127,  128,  136,  108,  109,  110,
 /*  1640 */   136,  112,  113,  114,  115,  136,  117,  118,  119,  120,
 /*  1650 */   136,  122,  123,  136,   81,  126,  127,  128,  136,  108,
 /*  1660 */   109,  110,  136,  112,  113,  114,  115,  136,  117,  118,
 /*  1670 */   119,  120,  136,  122,  123,  136,   81,  126,  127,  128,
 /*  1680 */   136,  108,  109,  110,  136,  112,  113,  114,  115,  136,
 /*  1690 */   117,  118,  119,  120,  136,  122,  123,  136,  136,  126,
 /*  1700 */   127,  128,  136,  108,  109,  110,  136,  112,  113,  114,
 /*  1710 */   115,  136,  117,  118,  119,  120,   12,  122,  123,  136,
 /*  1720 */   136,  126,  127,  128,  136,   21,  109,  110,  136,  112,
 /*  1730 */   113,  114,  115,  136,  117,  118,  119,  120,  136,  122,
 /*  1740 */   123,  136,  136,  126,  127,  128,  136,  136,  136,   45,
 /*  1750 */   136,  136,  136,  136,  136,  136,  136,  136,   54,   55,
 /*  1760 */   136,  136,  136,   59,   60,  136,   62,   63,   64,   65,
 /*  1770 */    66,   67,   68,   69,   70,  136,   12,  136,  109,  110,
 /*  1780 */   136,  112,  113,  114,  115,   21,  117,  118,  119,  120,
 /*  1790 */   136,  122,  123,  136,  136,  126,  127,  128,  136,  136,
 /*  1800 */   136,  136,  136,  136,  136,  136,  136,  136,   12,   45,
 /*  1810 */   136,  136,  136,  136,  136,  136,  136,   21,   54,   55,
 /*  1820 */   136,  136,  136,   59,   60,  136,   62,   63,   64,   65,
 /*  1830 */    66,   67,   68,   69,   70,  109,  110,  136,  112,  113,
 /*  1840 */   114,  115,  136,  117,  118,  119,  120,  136,  122,  123,
 /*  1850 */    54,   55,  126,  127,  128,   59,   60,  136,   62,   63,
 /*  1860 */    64,   65,   66,   67,   68,   69,   70,  136,  109,  136,
 /*  1870 */   136,  112,  113,  114,  115,  136,  117,  118,  119,  120,
 /*  1880 */   136,  122,  123,  136,  109,  126,  127,  128,  113,  114,
 /*  1890 */   115,  136,  117,  118,  119,  120,  136,  122,  123,  136,
 /*  1900 */   136,  126,  127,  128,  136,  136,  109,  136,  136,  136,
 /*  1910 */   113,  114,  115,  136,  117,  118,  119,  120,  136,  122,
 /*  1920 */   123,  136,  136,  126,  127,  128,
};
#define YY_SHIFT_USE_DFLT (-24)
#define YY_SHIFT_MAX 199
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2, 1116,   -2, 1116,
 /*    20 */  1183, 1213, 1704, 1243, 1764, 1764, 1764, 1764, 1764, 1764,
 /*    30 */  1764, 1764, 1764, 1764, 1764, 1764, 1764, 1764, 1764, 1764,
 /*    40 */  1764, 1764, 1764, 1764, 1764, 1764, 1764, 1764, 1796, 1796,
 /*    50 */  1796, 1796, 1796,  333,  333, 1796, 1796,  337, 1796, 1796,
 /*    60 */  1796, 1796, 1796,  381,  381,  230,   91,  341,  452,  452,
 /*    70 */    21,  -23,  -23,  -23,  -23,   66,   81, 1061,  137,   70,
 /*    80 */    70,  112,  155,  228,  155,  228,  343,  196,  213,  213,
 /*    90 */   213,  213,  233,  294,  304,   66,  308,  326,  326,   81,
 /*   100 */   326,  326,   81,  326,  326,  326,  326,   81,  233,  400,
 /*   110 */   307,  382,   71,  323,  391,   27,  421,  422,  439,  146,
 /*   120 */   199,  453,  399,  413,  429,  392,  404,  396,  409,  411,
 /*   130 */   406,  392,  404,  409,  411,  406,  434,  443,  395,  397,
 /*   140 */   397,  451,  454,  456,  440,  457,  440,  458,  459,  460,
 /*   150 */   455,  477,  481,  483,  491,  413,  484,  494,  488,  485,
 /*   160 */   490,  487,  493,  492,  493,  504,  499,  495,  496,  498,
 /*   170 */   501,  503,  506,  500,  511,  508,  513,  510,  512,  514,
 /*   180 */   515,  517,  518,  519,  523,  536,  541,  466,  528,  471,
 /*   190 */   532,  392,  392,  392,  545,  550,  552,  413,  413,  413,
};
#define YY_REDUCE_USE_DFLT (-82)
#define YY_REDUCE_MAX 108
static const short yy_reduce_ofst[] = {
 /*     0 */    -8,   44,  101,  153,  210,  262,  319,  371,  428,  480,
 /*    10 */   537,  589,  646,  698,  755,  807,  864,  -78,  916,  946,
 /*    20 */   996, 1036, 1079, 1206, 1246, 1268, 1289, 1311, 1333, 1355,
 /*    30 */  1377, 1398, 1420, 1442, 1464, 1486, 1507, 1529, 1551, 1573,
 /*    40 */  1595, 1104, 1617, 1669, 1726, 1759, 1775, 1797,  -29,   22,
 /*    50 */    69,  121,  130,  107,  203,  183,    7,  -81,  187,  225,
 /*    60 */   239,  282,  292,  -42,  -22,  -24,   64,  -32,   84,   85,
 /*    70 */   179,  -75,  128,  138,  162,  189,   77,   10,   35,   13,
 /*    80 */    13,   60,   39,   49,   39,   49,   61,   90,  145,  158,
 /*    90 */   163,  181,  151,  231,  234,  237,  242,  258,  266,  260,
 /*   100 */   270,  271,  260,  275,  280,  289,  298,  260,  268,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   331,  331,  331,  331,  331,  331,  331,  331,  331,  331,
 /*    10 */   331,  331,  331,  331,  331,  331,  331,  410,  331,  528,
 /*    20 */   528,  528,  515,  528,  528,  338,  528,  528,  528,  528,
 /*    30 */   528,  528,  528,  340,  342,  528,  528,  528,  528,  528,
 /*    40 */   528,  528,  528,  528,  528,  528,  528,  528,  528,  528,
 /*    50 */   528,  528,  528,  398,  398,  528,  528,  528,  528,  528,
 /*    60 */   528,  528,  528,  528,  528,  528,  528,  526,  359,  359,
 /*    70 */   528,  528,  528,  528,  528,  528,  402,  487,  456,  474,
 /*    80 */   473,  526,  466,  472,  465,  471,  516,  514,  528,  528,
 /*    90 */   528,  528,  519,  355,  528,  528,  363,  528,  528,  528,
 /*   100 */   528,  528,  406,  528,  528,  528,  528,  405,  519,  487,
 /*   110 */   528,  528,  528,  528,  528,  528,  528,  528,  528,  528,
 /*   120 */   528,  528,  528,  527,  528,  433,  451,  457,  462,  464,
 /*   130 */   468,  437,  450,  461,  463,  467,  493,  528,  528,  528,
 /*   140 */   508,  411,  412,  413,  528,  415,  493,  418,  419,  422,
 /*   150 */   528,  528,  528,  528,  528,  360,  528,  528,  528,  347,
 /*   160 */   528,  348,  350,  528,  349,  528,  528,  528,  528,  382,
 /*   170 */   374,  370,  368,  528,  528,  372,  528,  378,  376,  380,
 /*   180 */   390,  386,  384,  388,  394,  392,  396,  528,  528,  528,
 /*   190 */   528,  434,  435,  436,  528,  528,  528,  523,  524,  525,
 /*   200 */   330,  332,  333,  329,  334,  337,  339,  432,  453,  454,
 /*   210 */   455,  477,  483,  484,  485,  486,  488,  489,  493,  494,
 /*   220 */   495,  496,  497,  498,  499,  500,  501,  452,  469,  470,
 /*   230 */   475,  476,  479,  480,  481,  482,  478,  502,  507,  513,
 /*   240 */   504,  505,  511,  512,  506,  510,  509,  503,  508,  414,
 /*   250 */   425,  426,  430,  431,  416,  417,  428,  429,  420,  421,
 /*   260 */   423,  424,  427,  490,  517,  341,  343,  344,  357,  358,
 /*   270 */   345,  346,  354,  353,  352,  351,  365,  366,  364,  356,
 /*   280 */   361,  367,  399,  369,  400,  371,  373,  401,  403,  404,
 /*   290 */   408,  409,  375,  377,  379,  381,  407,  383,  385,  387,
 /*   300 */   389,  391,  393,  395,  397,  520,  518,  491,  492,  458,
 /*   310 */   459,  460,  438,  439,  440,  441,  442,  443,  444,  445,
 /*   320 */   446,  447,  448,  449,  335,  522,  336,  521,
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
  "AND_AND",       "NOT",           "EQUAL_EQUAL",   "LESS",        
  "XOR",           "BAR",           "AND",           "LSHIFT",      
  "RSHIFT",        "EQUAL_TILDA",   "PLUS",          "MINUS",       
  "DIV",           "DIV_DIV",       "PERCENT",       "TILDA",       
  "LBRACKET",      "RBRACKET",      "NUMBER",        "REGEXP",      
  "STRING",        "SYMBOL",        "NIL",           "TRUE",        
  "FALSE",         "LINE",          "LBRACE",        "RBRACE",      
  "EQUAL_GREATER",  "DO",            "EXCEPT",        "AS",          
  "error",         "module",        "stmts",         "stmt",        
  "func_def",      "expr",          "excepts",       "finally_opt", 
  "if_tail",       "decorators_opt",  "super_opt",     "names",       
  "dotted_names",  "dotted_name",   "else_opt",      "params",      
  "decorators",    "decorator",     "params_without_default",  "params_with_default",
  "block_param",   "var_param",     "kw_param",      "param_default_opt",
  "param_default",  "param_with_default",  "args",          "posargs",     
  "kwargs",        "vararg",        "varkwarg",      "kwarg",       
  "assign_expr",   "postfix_expr",  "logical_or_expr",  "augmented_assign_op",
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
 /* 130 */ "comp_op ::= EQUAL_EQUAL",
 /* 131 */ "comp_op ::= LESS",
 /* 132 */ "comp_op ::= GREATER",
 /* 133 */ "xor_expr ::= or_expr",
 /* 134 */ "xor_expr ::= xor_expr XOR or_expr",
 /* 135 */ "or_expr ::= and_expr",
 /* 136 */ "or_expr ::= or_expr BAR and_expr",
 /* 137 */ "and_expr ::= shift_expr",
 /* 138 */ "and_expr ::= and_expr AND shift_expr",
 /* 139 */ "shift_expr ::= match_expr",
 /* 140 */ "shift_expr ::= shift_expr shift_op match_expr",
 /* 141 */ "shift_op ::= LSHIFT",
 /* 142 */ "shift_op ::= RSHIFT",
 /* 143 */ "match_expr ::= arith_expr",
 /* 144 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 145 */ "arith_expr ::= term",
 /* 146 */ "arith_expr ::= arith_expr arith_op term",
 /* 147 */ "arith_op ::= PLUS",
 /* 148 */ "arith_op ::= MINUS",
 /* 149 */ "term ::= term term_op factor",
 /* 150 */ "term ::= factor",
 /* 151 */ "term_op ::= STAR",
 /* 152 */ "term_op ::= DIV",
 /* 153 */ "term_op ::= DIV_DIV",
 /* 154 */ "term_op ::= PERCENT",
 /* 155 */ "factor ::= PLUS factor",
 /* 156 */ "factor ::= MINUS factor",
 /* 157 */ "factor ::= TILDA factor",
 /* 158 */ "factor ::= power",
 /* 159 */ "power ::= postfix_expr",
 /* 160 */ "power ::= postfix_expr STAR_STAR factor",
 /* 161 */ "postfix_expr ::= atom",
 /* 162 */ "postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt",
 /* 163 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 164 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 165 */ "atom ::= NAME",
 /* 166 */ "atom ::= NUMBER",
 /* 167 */ "atom ::= REGEXP",
 /* 168 */ "atom ::= STRING",
 /* 169 */ "atom ::= SYMBOL",
 /* 170 */ "atom ::= NIL",
 /* 171 */ "atom ::= TRUE",
 /* 172 */ "atom ::= FALSE",
 /* 173 */ "atom ::= LINE",
 /* 174 */ "atom ::= LBRACKET exprs RBRACKET",
 /* 175 */ "atom ::= LBRACKET RBRACKET",
 /* 176 */ "atom ::= LBRACE RBRACE",
 /* 177 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 178 */ "atom ::= LBRACE exprs RBRACE",
 /* 179 */ "atom ::= LPAR expr RPAR",
 /* 180 */ "exprs ::= expr",
 /* 181 */ "exprs ::= exprs COMMA expr",
 /* 182 */ "dict_elems ::= dict_elem",
 /* 183 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 184 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 185 */ "dict_elem ::= NAME COLON expr",
 /* 186 */ "comma_opt ::=",
 /* 187 */ "comma_opt ::= COMMA",
 /* 188 */ "blockarg_opt ::=",
 /* 189 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 190 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 191 */ "blockarg_params_opt ::=",
 /* 192 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 193 */ "excepts ::= except",
 /* 194 */ "excepts ::= excepts except",
 /* 195 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 196 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 197 */ "except ::= EXCEPT NEWLINE stmts",
 /* 198 */ "finally_opt ::=",
 /* 199 */ "finally_opt ::= FINALLY stmts",
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
  { 77, 1 },
  { 78, 1 },
  { 78, 3 },
  { 79, 0 },
  { 79, 1 },
  { 79, 1 },
  { 79, 7 },
  { 79, 5 },
  { 79, 5 },
  { 79, 5 },
  { 79, 1 },
  { 79, 2 },
  { 79, 1 },
  { 79, 2 },
  { 79, 1 },
  { 79, 2 },
  { 79, 6 },
  { 79, 7 },
  { 79, 4 },
  { 79, 2 },
  { 79, 2 },
  { 88, 1 },
  { 88, 3 },
  { 89, 1 },
  { 89, 3 },
  { 87, 1 },
  { 87, 3 },
  { 86, 0 },
  { 86, 2 },
  { 84, 1 },
  { 84, 5 },
  { 90, 0 },
  { 90, 2 },
  { 80, 8 },
  { 85, 0 },
  { 85, 1 },
  { 92, 1 },
  { 92, 2 },
  { 93, 3 },
  { 91, 9 },
  { 91, 7 },
  { 91, 7 },
  { 91, 5 },
  { 91, 7 },
  { 91, 5 },
  { 91, 5 },
  { 91, 3 },
  { 91, 7 },
  { 91, 5 },
  { 91, 5 },
  { 91, 3 },
  { 91, 5 },
  { 91, 3 },
  { 91, 3 },
  { 91, 1 },
  { 91, 7 },
  { 91, 5 },
  { 91, 5 },
  { 91, 3 },
  { 91, 5 },
  { 91, 3 },
  { 91, 3 },
  { 91, 1 },
  { 91, 5 },
  { 91, 3 },
  { 91, 3 },
  { 91, 1 },
  { 91, 3 },
  { 91, 1 },
  { 91, 1 },
  { 91, 0 },
  { 98, 2 },
  { 97, 2 },
  { 96, 3 },
  { 99, 0 },
  { 99, 1 },
  { 100, 2 },
  { 94, 1 },
  { 94, 3 },
  { 95, 1 },
  { 95, 3 },
  { 101, 2 },
  { 102, 0 },
  { 102, 1 },
  { 102, 3 },
  { 102, 5 },
  { 102, 7 },
  { 102, 3 },
  { 102, 5 },
  { 102, 3 },
  { 102, 1 },
  { 102, 3 },
  { 102, 5 },
  { 102, 3 },
  { 102, 1 },
  { 102, 3 },
  { 102, 1 },
  { 106, 2 },
  { 105, 2 },
  { 103, 1 },
  { 103, 3 },
  { 104, 1 },
  { 104, 3 },
  { 107, 3 },
  { 81, 1 },
  { 108, 3 },
  { 108, 3 },
  { 108, 3 },
  { 108, 3 },
  { 108, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 110, 1 },
  { 110, 3 },
  { 112, 1 },
  { 112, 3 },
  { 113, 1 },
  { 113, 2 },
  { 114, 1 },
  { 114, 3 },
  { 116, 1 },
  { 116, 1 },
  { 116, 1 },
  { 115, 1 },
  { 115, 3 },
  { 117, 1 },
  { 117, 3 },
  { 118, 1 },
  { 118, 3 },
  { 119, 1 },
  { 119, 3 },
  { 121, 1 },
  { 121, 1 },
  { 120, 1 },
  { 120, 3 },
  { 122, 1 },
  { 122, 3 },
  { 124, 1 },
  { 124, 1 },
  { 123, 3 },
  { 123, 1 },
  { 125, 1 },
  { 125, 1 },
  { 125, 1 },
  { 125, 1 },
  { 126, 2 },
  { 126, 2 },
  { 126, 2 },
  { 126, 1 },
  { 127, 1 },
  { 127, 3 },
  { 109, 1 },
  { 109, 5 },
  { 109, 4 },
  { 109, 3 },
  { 128, 1 },
  { 128, 1 },
  { 128, 1 },
  { 128, 1 },
  { 128, 1 },
  { 128, 1 },
  { 128, 1 },
  { 128, 1 },
  { 128, 1 },
  { 128, 3 },
  { 128, 2 },
  { 128, 2 },
  { 128, 4 },
  { 128, 3 },
  { 128, 3 },
  { 130, 1 },
  { 130, 3 },
  { 131, 1 },
  { 131, 3 },
  { 133, 3 },
  { 133, 3 },
  { 132, 0 },
  { 132, 1 },
  { 129, 0 },
  { 129, 5 },
  { 129, 5 },
  { 134, 0 },
  { 134, 3 },
  { 82, 1 },
  { 82, 2 },
  { 135, 6 },
  { 135, 4 },
  { 135, 3 },
  { 83, 0 },
  { 83, 2 },
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
    *pval = yymsp[0].minor.yy111;
}
#line 2304 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 21: /* dotted_names ::= dotted_name */
      case 36: /* decorators ::= decorator */
      case 79: /* params_with_default ::= param_with_default */
      case 99: /* posargs ::= expr */
      case 101: /* kwargs ::= kwarg */
      case 180: /* exprs ::= expr */
      case 182: /* dict_elems ::= dict_elem */
      case 193: /* excepts ::= except */
#line 767 "parser.y"
{
    yygotominor.yy111 = make_array_with(env, yymsp[0].minor.yy111);
}
#line 2319 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 22: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 80: /* params_with_default ::= params_with_default COMMA param_with_default */
#line 770 "parser.y"
{
    yygotominor.yy111 = Array_push(env, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2328 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 27: /* super_opt ::= */
      case 31: /* else_opt ::= */
      case 34: /* decorators_opt ::= */
      case 74: /* param_default_opt ::= */
      case 82: /* args ::= */
      case 186: /* comma_opt ::= */
      case 188: /* blockarg_opt ::= */
      case 191: /* blockarg_params_opt ::= */
      case 198: /* finally_opt ::= */
#line 774 "parser.y"
{
    yygotominor.yy111 = YNIL;
}
#line 2344 "parser.c"
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
      case 133: /* xor_expr ::= or_expr */
      case 135: /* or_expr ::= and_expr */
      case 137: /* and_expr ::= shift_expr */
      case 139: /* shift_expr ::= match_expr */
      case 143: /* match_expr ::= arith_expr */
      case 145: /* arith_expr ::= term */
      case 150: /* term ::= factor */
      case 158: /* factor ::= power */
      case 159: /* power ::= postfix_expr */
      case 161: /* postfix_expr ::= atom */
      case 199: /* finally_opt ::= FINALLY stmts */
#line 777 "parser.y"
{
    yygotominor.yy111 = yymsp[0].minor.yy111;
}
#line 2377 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 783 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy111 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[-1].minor.yy111);
}
#line 2385 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 787 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[-1].minor.yy111);
}
#line 2393 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 791 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = Finally_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 2401 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 795 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = While_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 2409 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 799 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Break_new(env, lineno, YNIL);
}
#line 2417 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 803 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Break_new(env, lineno, yymsp[0].minor.yy111);
}
#line 2425 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 807 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Next_new(env, lineno, YNIL);
}
#line 2433 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 811 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Next_new(env, lineno, yymsp[0].minor.yy111);
}
#line 2441 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 815 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Return_new(env, lineno, YNIL);
}
#line 2449 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 819 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Return_new(env, lineno, yymsp[0].minor.yy111);
}
#line 2457 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 823 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy111 = If_new(env, lineno, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[-1].minor.yy111);
}
#line 2465 "parser.c"
        break;
      case 17: /* stmt ::= decorators_opt CLASS NAME super_opt NEWLINE stmts END */
#line 827 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy111 = Klass_new(env, lineno, yymsp[-6].minor.yy111, id, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 2474 "parser.c"
        break;
      case 18: /* stmt ::= MODULE NAME stmts END */
#line 832 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    yygotominor.yy111 = Module_new(env, lineno, id, yymsp[-1].minor.yy111);
}
#line 2483 "parser.c"
        break;
      case 19: /* stmt ::= NONLOCAL names */
#line 837 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Nonlocal_new(env, lineno, yymsp[0].minor.yy111);
}
#line 2491 "parser.c"
        break;
      case 20: /* stmt ::= IMPORT dotted_names */
#line 841 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy111 = Import_new(env, lineno, yymsp[0].minor.yy111);
}
#line 2499 "parser.c"
        break;
      case 23: /* dotted_name ::= NAME */
      case 25: /* names ::= NAME */
#line 853 "parser.y"
{
    yygotominor.yy111 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2507 "parser.c"
        break;
      case 24: /* dotted_name ::= dotted_name DOT NAME */
      case 26: /* names ::= names COMMA NAME */
#line 856 "parser.y"
{
    yygotominor.yy111 = Array_push_token_id(env, yymsp[-2].minor.yy111, yymsp[0].minor.yy0);
}
#line 2515 "parser.c"
        break;
      case 30: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 877 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111, yymsp[0].minor.yy111);
    yygotominor.yy111 = make_array_with(env, node);
}
#line 2524 "parser.c"
        break;
      case 33: /* func_def ::= decorators_opt DEF NAME LPAR params RPAR stmts END */
#line 890 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy111 = FuncDef_new(env, lineno, yymsp[-7].minor.yy111, id, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 2533 "parser.c"
        break;
      case 37: /* decorators ::= decorators decorator */
      case 194: /* excepts ::= excepts except */
#line 906 "parser.y"
{
    yygotominor.yy111 = Array_push(env, yymsp[-1].minor.yy111, yymsp[0].minor.yy111);
}
#line 2541 "parser.c"
        break;
      case 38: /* decorator ::= AT expr NEWLINE */
      case 179: /* atom ::= LPAR expr RPAR */
      case 192: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 910 "parser.y"
{
    yygotominor.yy111 = yymsp[-1].minor.yy111;
}
#line 2550 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 914 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-8].minor.yy111, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2557 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 917 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2564 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 920 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2571 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 923 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2578 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 926 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2585 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 929 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2592 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 932 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2599 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA params_with_default */
#line 935 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL, YNIL, YNIL);
}
#line 2606 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 938 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-6].minor.yy111, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2613 "parser.c"
        break;
      case 48: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 941 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2620 "parser.c"
        break;
      case 49: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 944 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2627 "parser.c"
        break;
      case 50: /* params ::= params_without_default COMMA block_param */
#line 947 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2634 "parser.c"
        break;
      case 51: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 950 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-4].minor.yy111, YNIL, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2641 "parser.c"
        break;
      case 52: /* params ::= params_without_default COMMA var_param */
#line 953 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, YNIL, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2648 "parser.c"
        break;
      case 53: /* params ::= params_without_default COMMA kw_param */
#line 956 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[-2].minor.yy111, YNIL, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2655 "parser.c"
        break;
      case 54: /* params ::= params_without_default */
#line 959 "parser.y"
{
    yygotominor.yy111 = Params_new(env, yymsp[0].minor.yy111, YNIL, YNIL, YNIL, YNIL);
}
#line 2662 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 962 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2669 "parser.c"
        break;
      case 56: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 965 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2676 "parser.c"
        break;
      case 57: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 968 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2683 "parser.c"
        break;
      case 58: /* params ::= params_with_default COMMA block_param */
#line 971 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2690 "parser.c"
        break;
      case 59: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 974 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2697 "parser.c"
        break;
      case 60: /* params ::= params_with_default COMMA var_param */
#line 977 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2704 "parser.c"
        break;
      case 61: /* params ::= params_with_default COMMA kw_param */
#line 980 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[-2].minor.yy111, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2711 "parser.c"
        break;
      case 62: /* params ::= params_with_default */
#line 983 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, yymsp[0].minor.yy111, YNIL, YNIL, YNIL);
}
#line 2718 "parser.c"
        break;
      case 63: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 986 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2725 "parser.c"
        break;
      case 64: /* params ::= block_param COMMA var_param */
#line 989 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2732 "parser.c"
        break;
      case 65: /* params ::= block_param COMMA kw_param */
#line 992 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2739 "parser.c"
        break;
      case 66: /* params ::= block_param */
#line 995 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2746 "parser.c"
        break;
      case 67: /* params ::= var_param COMMA kw_param */
#line 998 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2753 "parser.c"
        break;
      case 68: /* params ::= var_param */
#line 1001 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2760 "parser.c"
        break;
      case 69: /* params ::= kw_param */
#line 1004 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2767 "parser.c"
        break;
      case 70: /* params ::= */
#line 1007 "parser.y"
{
    yygotominor.yy111 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2774 "parser.c"
        break;
      case 71: /* kw_param ::= STAR_STAR NAME */
#line 1011 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2783 "parser.c"
        break;
      case 72: /* var_param ::= STAR NAME */
#line 1017 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2792 "parser.c"
        break;
      case 73: /* block_param ::= AMPER NAME param_default_opt */
#line 1023 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy111);
}
#line 2801 "parser.c"
        break;
      case 77: /* params_without_default ::= NAME */
#line 1040 "parser.y"
{
    yygotominor.yy111 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy111, lineno, id, YNIL);
}
#line 2811 "parser.c"
        break;
      case 78: /* params_without_default ::= params_without_default COMMA NAME */
#line 1046 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy111, lineno, id, YNIL);
    yygotominor.yy111 = yymsp[-2].minor.yy111;
}
#line 2821 "parser.c"
        break;
      case 81: /* param_with_default ::= NAME param_default */
#line 1060 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy111 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy111);
}
#line 2830 "parser.c"
        break;
      case 83: /* args ::= posargs */
#line 1069 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, yymsp[0].minor.yy111, YNIL, YNIL, YNIL);
}
#line 2838 "parser.c"
        break;
      case 84: /* args ::= posargs COMMA kwargs */
#line 1073 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2846 "parser.c"
        break;
      case 85: /* args ::= posargs COMMA kwargs COMMA vararg */
#line 1077 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2854 "parser.c"
        break;
      case 86: /* args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg */
#line 1081 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-6].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, yymsp[-6].minor.yy111, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2862 "parser.c"
        break;
      case 87: /* args ::= posargs COMMA vararg */
#line 1085 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2870 "parser.c"
        break;
      case 88: /* args ::= posargs COMMA vararg COMMA varkwarg */
#line 1089 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, yymsp[-4].minor.yy111, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2878 "parser.c"
        break;
      case 89: /* args ::= posargs COMMA varkwarg */
#line 1093 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, yymsp[-2].minor.yy111, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2886 "parser.c"
        break;
      case 90: /* args ::= kwargs */
#line 1097 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, YNIL, yymsp[0].minor.yy111, YNIL, YNIL);
}
#line 2894 "parser.c"
        break;
      case 91: /* args ::= kwargs COMMA vararg */
#line 1101 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111, YNIL);
}
#line 2902 "parser.c"
        break;
      case 92: /* args ::= kwargs COMMA vararg COMMA varkwarg */
#line 1105 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, YNIL, yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2910 "parser.c"
        break;
      case 93: /* args ::= kwargs COMMA varkwarg */
#line 1109 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy111, 0));
    yygotominor.yy111 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy111, YNIL, yymsp[0].minor.yy111);
}
#line 2918 "parser.c"
        break;
      case 94: /* args ::= vararg */
#line 1113 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy111);
    yygotominor.yy111 = Args_new(env, lineno, YNIL, YNIL, yymsp[0].minor.yy111, YNIL);
}
#line 2926 "parser.c"
        break;
      case 95: /* args ::= vararg COMMA varkwarg */
#line 1117 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    yygotominor.yy111 = Args_new(env, lineno, YNIL, YNIL, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2934 "parser.c"
        break;
      case 96: /* args ::= varkwarg */
#line 1121 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy111);
    yygotominor.yy111 = Args_new(env, lineno, YNIL, YNIL, YNIL, yymsp[0].minor.yy111);
}
#line 2942 "parser.c"
        break;
      case 100: /* posargs ::= posargs COMMA expr */
      case 102: /* kwargs ::= kwargs COMMA kwarg */
      case 181: /* exprs ::= exprs COMMA expr */
      case 183: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 1137 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
    yygotominor.yy111 = yymsp[-2].minor.yy111;
}
#line 2953 "parser.c"
        break;
      case 103: /* kwarg ::= NAME COLON expr */
#line 1150 "parser.y"
{
    yygotominor.yy111 = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(yymsp[-2].minor.yy0));
    PTR_AS(YogNode, yygotominor.yy111)->u.kwarg.name = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    PTR_AS(YogNode, yygotominor.yy111)->u.kwarg.value = yymsp[0].minor.yy111;
}
#line 2962 "parser.c"
        break;
      case 105: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 1160 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    yygotominor.yy111 = Assign_new(env, lineno, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 2970 "parser.c"
        break;
      case 106: /* assign_expr ::= postfix_expr augmented_assign_op logical_or_expr */
#line 1164 "parser.y"
{
    yygotominor.yy111 = AugmentedAssign_new(env, NODE_LINENO(yymsp[-2].minor.yy111), yymsp[-2].minor.yy111, VAL2ID(yymsp[-1].minor.yy111), yymsp[0].minor.yy111);
}
#line 2977 "parser.c"
        break;
      case 107: /* assign_expr ::= postfix_expr AND_AND_EQUAL logical_or_expr */
#line 1167 "parser.y"
{
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    expr = LogicalAnd_new(env, lineno, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
    assign = Assign_new(env, lineno, yymsp[-2].minor.yy111, expr);

    POP_LOCALS(env);

    yygotominor.yy111 = assign;
}
#line 2994 "parser.c"
        break;
      case 108: /* assign_expr ::= postfix_expr BAR_BAR_EQUAL logical_or_expr */
#line 1180 "parser.y"
{
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    expr = LogicalOr_new(env, lineno, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
    assign = Assign_new(env, lineno, yymsp[-2].minor.yy111, expr);

    POP_LOCALS(env);

    yygotominor.yy111 = assign;
}
#line 3011 "parser.c"
        break;
      case 110: /* augmented_assign_op ::= PLUS_EQUAL */
#line 1197 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "+"));
}
#line 3018 "parser.c"
        break;
      case 111: /* augmented_assign_op ::= MINUS_EQUAL */
#line 1200 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "-"));
}
#line 3025 "parser.c"
        break;
      case 112: /* augmented_assign_op ::= STAR_EQUAL */
#line 1203 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "*"));
}
#line 3032 "parser.c"
        break;
      case 113: /* augmented_assign_op ::= DIV_EQUAL */
#line 1206 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "/"));
}
#line 3039 "parser.c"
        break;
      case 114: /* augmented_assign_op ::= DIV_DIV_EQUAL */
#line 1209 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "//"));
}
#line 3046 "parser.c"
        break;
      case 115: /* augmented_assign_op ::= PERCENT_EQUAL */
#line 1212 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "%"));
}
#line 3053 "parser.c"
        break;
      case 116: /* augmented_assign_op ::= BAR_EQUAL */
#line 1215 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "|"));
}
#line 3060 "parser.c"
        break;
      case 117: /* augmented_assign_op ::= AND_EQUAL */
#line 1218 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "&"));
}
#line 3067 "parser.c"
        break;
      case 118: /* augmented_assign_op ::= XOR_EQUAL */
#line 1221 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "^"));
}
#line 3074 "parser.c"
        break;
      case 119: /* augmented_assign_op ::= STAR_STAR_EQUAL */
#line 1224 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "**"));
}
#line 3081 "parser.c"
        break;
      case 120: /* augmented_assign_op ::= LSHIFT_EQUAL */
#line 1227 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, "<<"));
}
#line 3088 "parser.c"
        break;
      case 121: /* augmented_assign_op ::= RSHIFT_EQUAL */
#line 1230 "parser.y"
{
    yygotominor.yy111 = ID2VAL(YogVM_intern(env, env->vm, ">>"));
}
#line 3095 "parser.c"
        break;
      case 123: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 1237 "parser.y"
{
    yygotominor.yy111 = LogicalOr_new(env, NODE_LINENO(yymsp[-2].minor.yy111), yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 3102 "parser.c"
        break;
      case 125: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 1244 "parser.y"
{
    yygotominor.yy111 = LogicalAnd_new(env, NODE_LINENO(yymsp[-2].minor.yy111), yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 3109 "parser.c"
        break;
      case 127: /* not_expr ::= NOT not_expr */
#line 1251 "parser.y"
{
    yygotominor.yy111 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy111)->u.not.expr = yymsp[0].minor.yy111;
}
#line 3117 "parser.c"
        break;
      case 129: /* comparison ::= xor_expr comp_op xor_expr */
#line 1259 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy111)->u.id;
    yygotominor.yy111 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy111, id, yymsp[0].minor.yy111);
}
#line 3126 "parser.c"
        break;
      case 130: /* comp_op ::= EQUAL_EQUAL */
      case 131: /* comp_op ::= LESS */
      case 132: /* comp_op ::= GREATER */
      case 187: /* comma_opt ::= COMMA */
#line 1265 "parser.y"
{
    yygotominor.yy111 = yymsp[0].minor.yy0;
}
#line 3136 "parser.c"
        break;
      case 134: /* xor_expr ::= xor_expr XOR or_expr */
      case 136: /* or_expr ::= or_expr BAR and_expr */
      case 138: /* and_expr ::= and_expr AND shift_expr */
#line 1278 "parser.y"
{
    yygotominor.yy111 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy111), yymsp[-2].minor.yy111, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy111);
}
#line 3145 "parser.c"
        break;
      case 140: /* shift_expr ::= shift_expr shift_op match_expr */
      case 146: /* arith_expr ::= arith_expr arith_op term */
      case 149: /* term ::= term term_op factor */
#line 1299 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    yygotominor.yy111 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy111, VAL2ID(yymsp[-1].minor.yy111), yymsp[0].minor.yy111);
}
#line 3155 "parser.c"
        break;
      case 141: /* shift_op ::= LSHIFT */
      case 142: /* shift_op ::= RSHIFT */
      case 147: /* arith_op ::= PLUS */
      case 148: /* arith_op ::= MINUS */
      case 151: /* term_op ::= STAR */
      case 152: /* term_op ::= DIV */
      case 153: /* term_op ::= DIV_DIV */
      case 154: /* term_op ::= PERCENT */
#line 1304 "parser.y"
{
    yygotominor.yy111 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 3169 "parser.c"
        break;
      case 144: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1314 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy111 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy111, id, yymsp[0].minor.yy111);
}
#line 3178 "parser.c"
        break;
      case 155: /* factor ::= PLUS factor */
#line 1356 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy111 = FuncCall_new3(env, lineno, yymsp[0].minor.yy111, id);
}
#line 3187 "parser.c"
        break;
      case 156: /* factor ::= MINUS factor */
#line 1361 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy111 = FuncCall_new3(env, lineno, yymsp[0].minor.yy111, id);
}
#line 3196 "parser.c"
        break;
      case 157: /* factor ::= TILDA factor */
#line 1366 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy111 = FuncCall_new3(env, lineno, yymsp[0].minor.yy111, id);
}
#line 3205 "parser.c"
        break;
      case 160: /* power ::= postfix_expr STAR_STAR factor */
#line 1378 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    ID id = YogVM_intern(env, env->vm, "**");
    yygotominor.yy111 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy111, id, yymsp[0].minor.yy111);
}
#line 3214 "parser.c"
        break;
      case 162: /* postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt */
#line 1387 "parser.y"
{
    yygotominor.yy111 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy111), yymsp[-4].minor.yy111, yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 3221 "parser.c"
        break;
      case 163: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1390 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy111);
    yygotominor.yy111 = Subscript_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 3229 "parser.c"
        break;
      case 164: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1394 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy111);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Attr_new(env, lineno, yymsp[-2].minor.yy111, id);
}
#line 3238 "parser.c"
        break;
      case 165: /* atom ::= NAME */
#line 1400 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy111 = Variable_new(env, lineno, id);
}
#line 3247 "parser.c"
        break;
      case 166: /* atom ::= NUMBER */
      case 167: /* atom ::= REGEXP */
      case 168: /* atom ::= STRING */
      case 169: /* atom ::= SYMBOL */
#line 1405 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy111 = Literal_new(env, lineno, val);
}
#line 3259 "parser.c"
        break;
      case 170: /* atom ::= NIL */
#line 1425 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Literal_new(env, lineno, YNIL);
}
#line 3267 "parser.c"
        break;
      case 171: /* atom ::= TRUE */
#line 1429 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Literal_new(env, lineno, YTRUE);
}
#line 3275 "parser.c"
        break;
      case 172: /* atom ::= FALSE */
#line 1433 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy111 = Literal_new(env, lineno, YFALSE);
}
#line 3283 "parser.c"
        break;
      case 173: /* atom ::= LINE */
#line 1437 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy111 = Literal_new(env, lineno, val);
}
#line 3292 "parser.c"
        break;
      case 174: /* atom ::= LBRACKET exprs RBRACKET */
#line 1442 "parser.y"
{
    yygotominor.yy111 = Array_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy111);
}
#line 3299 "parser.c"
        break;
      case 175: /* atom ::= LBRACKET RBRACKET */
#line 1445 "parser.y"
{
    yygotominor.yy111 = Array_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3306 "parser.c"
        break;
      case 176: /* atom ::= LBRACE RBRACE */
#line 1448 "parser.y"
{
    yygotominor.yy111 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3313 "parser.c"
        break;
      case 177: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1451 "parser.y"
{
    yygotominor.yy111 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy111);
}
#line 3320 "parser.c"
        break;
      case 178: /* atom ::= LBRACE exprs RBRACE */
#line 1454 "parser.y"
{
    yygotominor.yy111 = Set_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy111);
}
#line 3327 "parser.c"
        break;
      case 184: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1476 "parser.y"
{
    yygotominor.yy111 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy111), yymsp[-2].minor.yy111, yymsp[0].minor.yy111);
}
#line 3334 "parser.c"
        break;
      case 185: /* dict_elem ::= NAME COLON expr */
#line 1479 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy111 = DictElem_new(env, lineno, var, yymsp[0].minor.yy111);
}
#line 3344 "parser.c"
        break;
      case 189: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 190: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1496 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy111 = BlockArg_new(env, lineno, yymsp[-3].minor.yy111, yymsp[-1].minor.yy111);
}
#line 3353 "parser.c"
        break;
      case 195: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1519 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy111 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy111, id, yymsp[0].minor.yy111);
}
#line 3363 "parser.c"
        break;
      case 196: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1525 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy111 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy111, NO_EXC_VAR, yymsp[0].minor.yy111);
}
#line 3371 "parser.c"
        break;
      case 197: /* except ::= EXCEPT NEWLINE stmts */
#line 1529 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy111 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy111);
}
#line 3379 "parser.c"
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
