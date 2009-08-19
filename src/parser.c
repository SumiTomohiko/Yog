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

    YogVal node = YogNode_new(env, NODE_FUNC_CALL, lineno);
    NODE(node)->u.func_call.callee = callee;
    NODE(node)->u.func_call.args = args;
    NODE(node)->u.func_call.blockarg = blockarg;

    RETURN(env, node);
}

static YogVal
Variable_new(YogEnv* env, uint_t lineno, ID id)
{
    YogVal node = YogNode_new(env, NODE_VARIABLE, lineno);
    NODE(node)->u.variable.id = id;

    return node;
}

static YogVal
ExceptBody_new(YogEnv* env, uint_t lineno, YogVal type, ID var, YogVal stmts)
{
    SAVE_ARGS2(env, type, stmts);

    YogVal node = YogNode_new(env, NODE_EXCEPT_BODY, lineno);
    NODE(node)->u.except_body.type = type;
    NODE(node)->u.except_body.var = var;
    NODE(node)->u.except_body.stmts = stmts;

    RETURN(env, node);
}

static YogVal
Except_new(YogEnv* env, uint_t lineno, YogVal head, YogVal excepts, YogVal else_)
{
    SAVE_ARGS3(env, head, excepts, else_);

    YogVal node = YogNode_new(env, NODE_EXCEPT, lineno);
    NODE(node)->u.except.head = head;
    NODE(node)->u.except.excepts = excepts;
    NODE(node)->u.except.else_ = else_;

    RETURN(env, node);
}

static YogVal
Finally_new(YogEnv* env, uint_t lineno, YogVal head, YogVal body)
{
    SAVE_ARGS2(env, head, body);

    YogVal node = YogNode_new(env, NODE_FINALLY, lineno);
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

    YogVal node = YogNode_new(env, NODE_NEXT, lineno);
    NODE(node)->u.next.expr = expr;

    RETURN(env, node);
}

static YogVal
Return_new(YogEnv* env, uint_t lineno, YogVal expr)
{
    SAVE_ARG(env, expr);

    YogVal node = YogNode_new(env, NODE_RETURN, lineno);
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

    YogVal node = YogNode_new(env, NODE_ASSIGN, lineno);
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

    node = YogNode_new(env, NODE_SUBSCRIPT, lineno);
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
#line 766 "parser.c"
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
#define YYNOCODE 141
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy247;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 338
#define YYNRULE 210
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
 /*     0 */     1,  173,  283,  262,   24,   25,   33,   34,   35,  372,
 /*    10 */   218,  159,   95,   75,  171,  178,  180,  295,  372,   28,
 /*    20 */   296,   37,  179,  293,  151,  142,  148,  150,  261,  257,
 /*    30 */   207,   77,  132,  145,  133,  227,  209,   78,  275,  134,
 /*    40 */   135,   85,  136,   47,   86,   81,   31,   18,  236,  215,
 /*    50 */   217,    3,  172,  176,  286,   59,   60,  290,  194,   39,
 /*    60 */    61,   21,   67,  219,  220,  221,  222,  223,  224,  225,
 /*    70 */   226,   20,  549,  111,  203,  201,  202,  182,  184,  300,
 /*    80 */   116,   42,  290,  110,  154,   18,  145,   97,  278,  128,
 /*    90 */   268,  134,  135,   85,  136,  232,   86,   81,   30,   31,
 /*   100 */   236,  215,  217,  207,   77,  132,   26,  133,  227,  209,
 /*   110 */    78,  335,  134,  135,   85,  136,  337,   86,   81,  110,
 /*   120 */    18,  236,  215,  217,   16,   66,  203,  201,  202,  233,
 /*   130 */   234,  235,  116,   80,  110,   48,  236,  215,  217,   97,
 /*   140 */   278,  308,  129,  135,   85,  136,   14,   86,   81,  174,
 /*   150 */   175,  236,  215,  217,  237,  207,   77,  132,  306,  133,
 /*   160 */   227,  209,   78,  110,  134,  135,   85,  136,  109,   86,
 /*   170 */    81,   93,  269,  236,  215,  217,   84,   81,  268,  123,
 /*   180 */   236,  215,  217,   58,  110,   82,  203,  201,  202,  149,
 /*   190 */   259,  252,  116,  130,   85,  136,   23,   86,   81,   97,
 /*   200 */   278,  236,  215,  217,  169,  183,  298,  170,  181,  185,
 /*   210 */   187,  304,  186,  302,  296,  207,   77,  132,   52,  133,
 /*   220 */   227,  209,   78,  195,  134,  135,   85,  136,   56,   86,
 /*   230 */    81,  162,  165,  236,  215,  217,  110,  124,  203,  201,
 /*   240 */   202,    2,  139,    3,  116,  110,   83,  136,  157,   86,
 /*   250 */    81,   97,  278,  236,  215,  217,  131,  167,   86,   81,
 /*   260 */   287,  288,  236,  215,  217,  228,  229,  207,   77,  132,
 /*   270 */   263,  133,  227,  209,   78,  110,  134,  135,   85,  136,
 /*   280 */    22,   86,   81,  230,  231,  236,  215,  217,  320,  144,
 /*   290 */    26,  252,  211,  215,  217,  338,   18,  112,  203,  201,
 /*   300 */   202,   30,   18,  188,  116,  205,  170,  181,  185,  187,
 /*   310 */   304,   97,  278,  296,  249,   23,  316,  317,  318,  319,
 /*   320 */   321,   49,   18,   18,  309,  264,  270,  207,   77,  132,
 /*   330 */   314,  133,  227,  209,   78,   54,  134,  135,   85,  136,
 /*   340 */    18,   86,   81,  271,  254,  236,  215,  217,  244,  115,
 /*   350 */   203,  201,  202,  110,  108,  258,  116,  110,  310,  311,
 /*   360 */   312,  313,  315,   97,  278,  260,  174,  175,  177,  152,
 /*   370 */   212,  215,  217,   38,  213,  215,  217,  273,  158,  207,
 /*   380 */    77,  132,  160,  133,  227,  209,   78,  110,  134,  135,
 /*   390 */    85,  136,   18,   86,   81,  280,  163,  236,  215,  217,
 /*   400 */   110,  103,   37,  276,  214,  215,  217,  174,  291,   68,
 /*   410 */   203,  201,  202,  174,  175,  177,  116,  216,  215,  217,
 /*   420 */   281,  285,  100,   97,  278,   18,  292,  294,  336,  189,
 /*   430 */   297,  299,  301,  303,  174,  175,  177,  204,   18,  207,
 /*   440 */    77,  132,    4,  133,  227,  209,   78,   45,  134,  135,
 /*   450 */    85,  136,   46,   86,   81,   50,   49,  236,  215,  217,
 /*   460 */    51,   69,  203,  201,  202,   18,   79,    8,  116,   55,
 /*   470 */    17,   27,  238,   62,  241,   97,  278,   29,   19,   70,
 /*   480 */    32,   89,   90,   36,   65,   91,   92,    5,    6,   87,
 /*   490 */     7,  207,   77,  132,  267,  133,  227,  209,   78,    9,
 /*   500 */   134,  135,   85,  136,  161,   86,   81,   94,   10,  236,
 /*   510 */   215,  217,   40,  272,  164,   96,  274,  277,  168,  282,
 /*   520 */   284,  156,  203,  201,  202,   11,   53,   57,  116,   63,
 /*   530 */    71,   98,   76,   99,   72,   97,  278,  101,  102,   64,
 /*   540 */    73,  104,  105,   74,  106,  107,  305,   12,  307,  334,
 /*   550 */   196,  207,   77,  132,   13,  133,  227,  209,   78,  550,
 /*   560 */   134,  135,   85,  136,  550,   86,   81,  550,  550,  236,
 /*   570 */   215,  217,  550,  117,  203,  201,  202,  550,  550,  550,
 /*   580 */   116,  550,  550,  550,  550,  550,  550,   97,  278,  550,
 /*   590 */   550,  550,  550,  550,  550,  550,  550,  550,  550,  550,
 /*   600 */   550,  550,  550,  207,   77,  132,  550,  133,  227,  209,
 /*   610 */    78,  550,  134,  135,   85,  136,  550,   86,   81,  550,
 /*   620 */   550,  236,  215,  217,  550,  550,  550,  550,  550,  550,
 /*   630 */   550,  550,  550,  118,  203,  201,  202,  550,  550,  550,
 /*   640 */   116,  550,  550,  550,  550,  550,  550,   97,  278,  550,
 /*   650 */   550,  550,  550,  550,  550,  550,  550,  550,  550,  550,
 /*   660 */   550,  550,  550,  207,   77,  132,  550,  133,  227,  209,
 /*   670 */    78,  550,  134,  135,   85,  136,  550,   86,   81,  550,
 /*   680 */   550,  236,  215,  217,  550,  119,  203,  201,  202,  550,
 /*   690 */   550,  550,  116,  550,  550,  550,  550,  550,  550,   97,
 /*   700 */   278,  550,  550,  550,  550,  550,  550,  550,  550,  550,
 /*   710 */   550,  550,  550,  550,  550,  207,   77,  132,  550,  133,
 /*   720 */   227,  209,   78,  550,  134,  135,   85,  136,  550,   86,
 /*   730 */    81,  550,  550,  236,  215,  217,  550,  550,  550,  550,
 /*   740 */   550,  550,  550,  550,  550,  120,  203,  201,  202,  550,
 /*   750 */   550,  550,  116,  550,  550,  550,  550,  550,  550,   97,
 /*   760 */   278,  550,  550,  550,  550,  550,  550,  550,  550,  550,
 /*   770 */   550,  550,  550,  550,  550,  207,   77,  132,  550,  133,
 /*   780 */   227,  209,   78,  550,  134,  135,   85,  136,  550,   86,
 /*   790 */    81,  550,  550,  236,  215,  217,  550,  197,  203,  201,
 /*   800 */   202,  550,  550,  550,  116,  550,  550,  550,  550,  550,
 /*   810 */   550,   97,  278,  550,  550,  550,  550,  550,  550,  550,
 /*   820 */   550,  550,  550,  550,  550,  550,  550,  207,   77,  132,
 /*   830 */   550,  133,  227,  209,   78,  550,  134,  135,   85,  136,
 /*   840 */   550,   86,   81,  550,  550,  236,  215,  217,  550,  550,
 /*   850 */   550,  550,  550,  550,  550,  550,  550,  198,  203,  201,
 /*   860 */   202,  550,  550,  550,  116,  550,  550,  550,  550,  550,
 /*   870 */   550,   97,  278,  550,  550,  550,  550,  550,  550,  550,
 /*   880 */   550,  550,  550,  550,  550,  550,  550,  207,   77,  132,
 /*   890 */   550,  133,  227,  209,   78,  550,  134,  135,   85,  136,
 /*   900 */   550,   86,   81,  550,  550,  236,  215,  217,  550,  199,
 /*   910 */   203,  201,  202,  550,  550,  550,  116,  550,  550,  550,
 /*   920 */   550,  550,  550,   97,  278,  550,  550,  550,  550,  550,
 /*   930 */   550,  550,  550,  550,  550,  550,  550,  550,  550,  207,
 /*   940 */    77,  132,  550,  133,  227,  209,   78,  550,  134,  135,
 /*   950 */    85,  136,  550,   86,   81,  550,  550,  236,  215,  217,
 /*   960 */   550,  550,  550,  550,  550,  550,  550,  550,  550,  122,
 /*   970 */   203,  201,  202,  550,  550,  550,  116,  550,  550,  550,
 /*   980 */   550,  550,  550,   97,  278,  550,  550,  550,  550,  550,
 /*   990 */   550,  550,  550,  550,  550,  550,  550,  550,  550,  207,
 /*  1000 */    77,  132,  550,  133,  227,  209,   78,  550,  134,  135,
 /*  1010 */    85,  136,  550,   86,   81,  550,  550,  236,  215,  217,
 /*  1020 */   550,  550,  200,  201,  202,  550,  550,  550,  116,  550,
 /*  1030 */   550,  550,  550,  550,  550,   97,  278,  550,  550,  550,
 /*  1040 */   550,  550,  550,  550,  550,  550,  550,  550,  550,  550,
 /*  1050 */   550,  207,   77,  132,  256,  133,  227,  209,   78,  550,
 /*  1060 */   134,  135,   85,  136,  550,   86,   81,  550,  550,  236,
 /*  1070 */   215,  217,  550,  550,  550,  550,  550,  143,  146,  255,
 /*  1080 */   257,  207,   77,  132,  141,  133,  227,  209,   78,  550,
 /*  1090 */   134,  135,   85,  136,  550,   86,   81,  550,  550,  236,
 /*  1100 */   215,  217,  550,  550,  550,  550,  550,  550,  550,  550,
 /*  1110 */   550,  207,   77,  132,  550,  133,  227,  209,   78,  550,
 /*  1120 */   134,  135,   85,  136,  550,   86,   81,  550,  550,  236,
 /*  1130 */   215,  217,  550,  218,  114,   88,   79,  245,  550,  550,
 /*  1140 */    17,  550,   28,   62,  550,  550,   41,  550,   43,   44,
 /*  1150 */   322,  323,  324,  325,  326,  327,  328,  329,  330,  331,
 /*  1160 */   332,  333,  147,  550,  550,  550,   47,  550,  550,  550,
 /*  1170 */   550,   28,  550,  550,   30,   31,  550,  550,   59,   60,
 /*  1180 */   550,  550,   40,   61,   21,  247,  219,  220,  221,  222,
 /*  1190 */   223,  224,  225,  226,   20,   47,  550,  248,  550,  550,
 /*  1200 */   550,  550,  550,  550,  550,  550,  550,   59,   60,  550,
 /*  1210 */   550,  550,   61,   21,  550,  219,  220,  221,  222,  223,
 /*  1220 */   224,  225,  226,   20,  207,   77,  132,  550,  133,  227,
 /*  1230 */   209,   78,  550,  134,  135,   85,  136,  140,   86,   81,
 /*  1240 */   550,  550,  236,  215,  217,  550,  550,  113,  137,  550,
 /*  1250 */   550,  550,  550,  550,  550,  550,  550,   28,  550,  550,
 /*  1260 */   550,  550,  550,  550,  207,   77,  132,  550,  133,  227,
 /*  1270 */   209,   78,  550,  134,  135,   85,  136,   15,   86,   81,
 /*  1280 */   550,   47,  236,  215,  217,  550,  550,  550,  218,  550,
 /*  1290 */   242,  550,  550,   59,   60,  550,  550,   28,   61,   21,
 /*  1300 */   550,  219,  220,  221,  222,  223,  224,  225,  226,   20,
 /*  1310 */   240,  550,  550,  550,  550,  550,  550,  550,  550,  550,
 /*  1320 */   550,   47,  550,  121,  550,  550,  550,  550,  550,  550,
 /*  1330 */   550,  550,  550,   59,   60,  550,  550,  550,   61,   21,
 /*  1340 */   550,  219,  220,  221,  222,  223,  224,  225,  226,   20,
 /*  1350 */   207,   77,  132,  125,  133,  227,  209,   78,  550,  134,
 /*  1360 */   135,   85,  136,  550,   86,   81,  550,  550,  236,  215,
 /*  1370 */   217,  550,  550,  550,  550,  206,  550,  550,  550,  550,
 /*  1380 */   207,   77,  132,  550,  133,  227,  209,   78,  550,  134,
 /*  1390 */   135,   85,  136,  550,   86,   81,  550,  550,  236,  215,
 /*  1400 */   217,  550,  207,   77,  132,  550,  133,  227,  209,   78,
 /*  1410 */   550,  134,  135,   85,  136,  550,   86,   81,  246,  550,
 /*  1420 */   236,  215,  217,  110,  126,  550,  133,  227,  209,   78,
 /*  1430 */   550,  134,  135,   85,  136,  550,   86,   81,  550,  239,
 /*  1440 */   236,  215,  217,  550,  550,  207,   77,  132,  550,  133,
 /*  1450 */   227,  209,   78,  550,  134,  135,   85,  136,  550,   86,
 /*  1460 */    81,  550,  550,  236,  215,  217,  207,   77,  132,  138,
 /*  1470 */   133,  227,  209,   78,  550,  134,  135,   85,  136,  550,
 /*  1480 */    86,   81,  550,  550,  236,  215,  217,  550,  550,  550,
 /*  1490 */   550,  243,  550,  550,  550,  550,  207,   77,  132,  550,
 /*  1500 */   133,  227,  209,   78,  550,  134,  135,   85,  136,  550,
 /*  1510 */    86,   81,  550,  550,  236,  215,  217,  550,  207,   77,
 /*  1520 */   132,  550,  133,  227,  209,   78,  550,  134,  135,   85,
 /*  1530 */   136,  250,   86,   81,  550,  550,  236,  215,  217,  550,
 /*  1540 */   550,  550,  550,  550,  550,  550,  550,  550,  550,  550,
 /*  1550 */   550,  550,  251,  550,  550,  550,  550,  550,  207,   77,
 /*  1560 */   132,  550,  133,  227,  209,   78,  550,  134,  135,   85,
 /*  1570 */   136,  550,   86,   81,  550,  550,  236,  215,  217,  207,
 /*  1580 */    77,  132,  253,  133,  227,  209,   78,  550,  134,  135,
 /*  1590 */    85,  136,  550,   86,   81,  550,  550,  236,  215,  217,
 /*  1600 */   550,  550,  550,  265,  550,  550,  550,  550,  550,  207,
 /*  1610 */    77,  132,  550,  133,  227,  209,   78,  550,  134,  135,
 /*  1620 */    85,  136,  550,   86,   81,  550,  550,  236,  215,  217,
 /*  1630 */   207,   77,  132,  550,  133,  227,  209,   78,  550,  134,
 /*  1640 */   135,   85,  136,  266,   86,   81,  550,  550,  236,  215,
 /*  1650 */   217,  550,  550,  550,  550,  550,  550,  550,  550,  550,
 /*  1660 */   550,  550,  550,  550,  153,  550,  550,  550,  550,  550,
 /*  1670 */   207,   77,  132,  550,  133,  227,  209,   78,  550,  134,
 /*  1680 */   135,   85,  136,  550,   86,   81,  550,  550,  236,  215,
 /*  1690 */   217,  207,   77,  132,  155,  133,  227,  209,   78,  550,
 /*  1700 */   134,  135,   85,  136,  550,   86,   81,  550,  550,  236,
 /*  1710 */   215,  217,  550,  550,  550,  166,  550,  550,  550,  550,
 /*  1720 */   550,  207,   77,  132,  550,  133,  227,  209,   78,  550,
 /*  1730 */   134,  135,   85,  136,  550,   86,   81,  550,  550,  236,
 /*  1740 */   215,  217,  207,   77,  132,  550,  133,  227,  209,   78,
 /*  1750 */   550,  134,  135,   85,  136,  279,   86,   81,  550,  550,
 /*  1760 */   236,  215,  217,  550,  550,  550,  550,  550,  550,  550,
 /*  1770 */   550,  550,  550,  550,  550,  550,  289,  550,  550,  550,
 /*  1780 */   550,  550,  207,   77,  132,  550,  133,  227,  209,   78,
 /*  1790 */   550,  134,  135,   85,  136,  550,   86,   81,  550,  550,
 /*  1800 */   236,  215,  217,  207,   77,  132,  190,  133,  227,  209,
 /*  1810 */    78,  550,  134,  135,   85,  136,  137,   86,   81,  550,
 /*  1820 */   550,  236,  215,  217,  550,   28,  550,  550,  550,  550,
 /*  1830 */   550,  550,  550,  207,   77,  132,  550,  133,  227,  209,
 /*  1840 */    78,  550,  134,  135,   85,  136,  218,   86,   81,   47,
 /*  1850 */   550,  236,  215,  217,  550,   28,  550,  550,  550,  550,
 /*  1860 */   550,   59,   60,  550,  550,  550,   61,   21,  550,  219,
 /*  1870 */   220,  221,  222,  223,  224,  225,  226,   20,  550,   47,
 /*  1880 */   550,  550,  550,  550,  550,  550,  550,  550,  550,  550,
 /*  1890 */   550,   59,   60,  550,  550,  550,   61,   21,  550,  219,
 /*  1900 */   220,  221,  222,  223,  224,  225,  226,   20,  550,  110,
 /*  1910 */   191,  550,  133,  227,  209,   78,  550,  134,  135,   85,
 /*  1920 */   136,  550,   86,   81,  550,  550,  236,  215,  217,  110,
 /*  1930 */   192,  550,  133,  227,  209,   78,  218,  134,  135,   85,
 /*  1940 */   136,  550,   86,   81,  550,   28,  236,  215,  217,  550,
 /*  1950 */   110,  193,  550,  133,  227,  209,   78,  550,  134,  135,
 /*  1960 */    85,  136,  550,   86,   81,  550,  110,  236,  215,  217,
 /*  1970 */   208,  209,   78,  550,  134,  135,   85,  136,  550,   86,
 /*  1980 */    81,   59,   60,  236,  215,  217,   61,   21,  550,  219,
 /*  1990 */   220,  221,  222,  223,  224,  225,  226,   20,  550,  550,
 /*  2000 */   550,  550,  550,  550,  110,  550,  550,  127,  227,  209,
 /*  2010 */    78,  550,  134,  135,   85,  136,  550,   86,   81,  550,
 /*  2020 */   110,  236,  215,  217,  210,  209,   78,  550,  134,  135,
 /*  2030 */    85,  136,  550,   86,   81,  550,  550,  236,  215,  217,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,  100,  101,   84,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   15,   98,   99,  100,  101,   20,   21,
 /*    20 */   104,   23,  100,  101,  105,  106,  107,  108,  109,  110,
 /*    30 */   111,  112,  113,   12,  115,  116,  117,  118,   12,  120,
 /*    40 */   121,  122,  123,   45,  125,  126,   25,    1,  129,  130,
 /*    50 */   131,    5,   99,  100,  101,   57,   58,  104,   86,   27,
 /*    60 */    62,   63,   85,   65,   66,   67,   68,   69,   70,   71,
 /*    70 */    72,   73,   80,   81,   82,   83,   84,   99,  100,  101,
 /*    80 */    88,  114,  104,  112,   87,    1,   12,   95,   96,  118,
 /*    90 */    93,  120,  121,  122,  123,   25,  125,  126,   24,   25,
 /*   100 */   129,  130,  131,  111,  112,  113,   16,  115,  116,  117,
 /*   110 */   118,  139,  120,  121,  122,  123,  139,  125,  126,  112,
 /*   120 */     1,  129,  130,  131,    5,   81,   82,   83,   84,   59,
 /*   130 */    60,   61,   88,  126,  112,  119,  129,  130,  131,   95,
 /*   140 */    96,  133,  120,  121,  122,  123,    1,  125,  126,   24,
 /*   150 */    25,  129,  130,  131,   64,  111,  112,  113,   74,  115,
 /*   160 */   116,  117,  118,  112,  120,  121,  122,  123,   73,  125,
 /*   170 */   126,   76,   87,  129,  130,  131,  125,  126,   93,   86,
 /*   180 */   129,  130,  131,  128,  112,   81,   82,   83,   84,  108,
 /*   190 */   109,  110,   88,  121,  122,  123,   77,  125,  126,   95,
 /*   200 */    96,  129,  130,  131,   94,  100,  101,   97,   98,   99,
 /*   210 */   100,  101,  100,  101,  104,  111,  112,  113,  124,  115,
 /*   220 */   116,  117,  118,   78,  120,  121,  122,  123,  127,  125,
 /*   230 */   126,   91,   92,  129,  130,  131,  112,   81,   82,   83,
 /*   240 */    84,    3,  136,    5,   88,  112,  122,  123,   11,  125,
 /*   250 */   126,   95,   96,  129,  130,  131,  123,   20,  125,  126,
 /*   260 */   102,  103,  129,  130,  131,   54,   55,  111,  112,  113,
 /*   270 */   132,  115,  116,  117,  118,  112,  120,  121,  122,  123,
 /*   280 */    16,  125,  126,   57,   58,  129,  130,  131,   18,  108,
 /*   290 */    16,  110,  129,  130,  131,    0,    1,   81,   82,   83,
 /*   300 */    84,   24,    1,   94,   88,    4,   97,   98,   99,  100,
 /*   310 */   101,   95,   96,  104,  109,   77,   46,   47,   48,   49,
 /*   320 */    50,   51,    1,    1,   12,    4,    4,  111,  112,  113,
 /*   330 */    18,  115,  116,  117,  118,   63,  120,  121,  122,  123,
 /*   340 */     1,  125,  126,    4,  109,  129,  130,  131,   74,   81,
 /*   350 */    82,   83,   84,  112,   12,  109,   88,  112,   46,   47,
 /*   360 */    48,   49,   50,   95,   96,  109,   24,   25,   26,  138,
 /*   370 */   129,  130,  131,   18,  129,  130,  131,   12,   89,  111,
 /*   380 */   112,  113,   90,  115,  116,  117,  118,  112,  120,  121,
 /*   390 */   122,  123,    1,  125,  126,    4,   92,  129,  130,  131,
 /*   400 */   112,   12,   23,   96,  129,  130,  131,   24,  103,   81,
 /*   410 */    82,   83,   84,   24,   25,   26,   88,  129,  130,  131,
 /*   420 */   101,  101,   12,   95,   96,    1,  101,  101,    4,  138,
 /*   430 */   101,  101,  101,  101,   24,   25,   26,    4,    1,  111,
 /*   440 */   112,  113,    1,  115,  116,  117,  118,   43,  120,  121,
 /*   450 */   122,  123,   44,  125,  126,   52,   51,  129,  130,  131,
 /*   460 */    53,   81,   82,   83,   84,    1,   17,    3,   88,   56,
 /*   470 */    21,   28,   22,   24,   74,   95,   96,   75,   16,   16,
 /*   480 */    28,   16,   16,   19,   16,   16,   16,    1,    1,   22,
 /*   490 */     1,  111,  112,  113,    4,  115,  116,  117,  118,    1,
 /*   500 */   120,  121,  122,  123,   16,  125,  126,   12,   12,  129,
 /*   510 */   130,  131,   63,   12,   17,   16,   12,    1,   12,   12,
 /*   520 */    12,   81,   82,   83,   84,   22,   21,   16,   88,   16,
 /*   530 */    16,   16,   12,   16,   16,   95,   96,   16,   16,   16,
 /*   540 */    16,   16,   16,   16,   16,   16,   64,    1,   64,    4,
 /*   550 */    12,  111,  112,  113,    1,  115,  116,  117,  118,  140,
 /*   560 */   120,  121,  122,  123,  140,  125,  126,  140,  140,  129,
 /*   570 */   130,  131,  140,   81,   82,   83,   84,  140,  140,  140,
 /*   580 */    88,  140,  140,  140,  140,  140,  140,   95,   96,  140,
 /*   590 */   140,  140,  140,  140,  140,  140,  140,  140,  140,  140,
 /*   600 */   140,  140,  140,  111,  112,  113,  140,  115,  116,  117,
 /*   610 */   118,  140,  120,  121,  122,  123,  140,  125,  126,  140,
 /*   620 */   140,  129,  130,  131,  140,  140,  140,  140,  140,  140,
 /*   630 */   140,  140,  140,   81,   82,   83,   84,  140,  140,  140,
 /*   640 */    88,  140,  140,  140,  140,  140,  140,   95,   96,  140,
 /*   650 */   140,  140,  140,  140,  140,  140,  140,  140,  140,  140,
 /*   660 */   140,  140,  140,  111,  112,  113,  140,  115,  116,  117,
 /*   670 */   118,  140,  120,  121,  122,  123,  140,  125,  126,  140,
 /*   680 */   140,  129,  130,  131,  140,   81,   82,   83,   84,  140,
 /*   690 */   140,  140,   88,  140,  140,  140,  140,  140,  140,   95,
 /*   700 */    96,  140,  140,  140,  140,  140,  140,  140,  140,  140,
 /*   710 */   140,  140,  140,  140,  140,  111,  112,  113,  140,  115,
 /*   720 */   116,  117,  118,  140,  120,  121,  122,  123,  140,  125,
 /*   730 */   126,  140,  140,  129,  130,  131,  140,  140,  140,  140,
 /*   740 */   140,  140,  140,  140,  140,   81,   82,   83,   84,  140,
 /*   750 */   140,  140,   88,  140,  140,  140,  140,  140,  140,   95,
 /*   760 */    96,  140,  140,  140,  140,  140,  140,  140,  140,  140,
 /*   770 */   140,  140,  140,  140,  140,  111,  112,  113,  140,  115,
 /*   780 */   116,  117,  118,  140,  120,  121,  122,  123,  140,  125,
 /*   790 */   126,  140,  140,  129,  130,  131,  140,   81,   82,   83,
 /*   800 */    84,  140,  140,  140,   88,  140,  140,  140,  140,  140,
 /*   810 */   140,   95,   96,  140,  140,  140,  140,  140,  140,  140,
 /*   820 */   140,  140,  140,  140,  140,  140,  140,  111,  112,  113,
 /*   830 */   140,  115,  116,  117,  118,  140,  120,  121,  122,  123,
 /*   840 */   140,  125,  126,  140,  140,  129,  130,  131,  140,  140,
 /*   850 */   140,  140,  140,  140,  140,  140,  140,   81,   82,   83,
 /*   860 */    84,  140,  140,  140,   88,  140,  140,  140,  140,  140,
 /*   870 */   140,   95,   96,  140,  140,  140,  140,  140,  140,  140,
 /*   880 */   140,  140,  140,  140,  140,  140,  140,  111,  112,  113,
 /*   890 */   140,  115,  116,  117,  118,  140,  120,  121,  122,  123,
 /*   900 */   140,  125,  126,  140,  140,  129,  130,  131,  140,   81,
 /*   910 */    82,   83,   84,  140,  140,  140,   88,  140,  140,  140,
 /*   920 */   140,  140,  140,   95,   96,  140,  140,  140,  140,  140,
 /*   930 */   140,  140,  140,  140,  140,  140,  140,  140,  140,  111,
 /*   940 */   112,  113,  140,  115,  116,  117,  118,  140,  120,  121,
 /*   950 */   122,  123,  140,  125,  126,  140,  140,  129,  130,  131,
 /*   960 */   140,  140,  140,  140,  140,  140,  140,  140,  140,   81,
 /*   970 */    82,   83,   84,  140,  140,  140,   88,  140,  140,  140,
 /*   980 */   140,  140,  140,   95,   96,  140,  140,  140,  140,  140,
 /*   990 */   140,  140,  140,  140,  140,  140,  140,  140,  140,  111,
 /*  1000 */   112,  113,  140,  115,  116,  117,  118,  140,  120,  121,
 /*  1010 */   122,  123,  140,  125,  126,  140,  140,  129,  130,  131,
 /*  1020 */   140,  140,   82,   83,   84,  140,  140,  140,   88,  140,
 /*  1030 */   140,  140,  140,  140,  140,   95,   96,  140,  140,  140,
 /*  1040 */   140,  140,  140,  140,  140,  140,  140,  140,  140,  140,
 /*  1050 */   140,  111,  112,  113,   84,  115,  116,  117,  118,  140,
 /*  1060 */   120,  121,  122,  123,  140,  125,  126,  140,  140,  129,
 /*  1070 */   130,  131,  140,  140,  140,  140,  140,  107,  108,  109,
 /*  1080 */   110,  111,  112,  113,   84,  115,  116,  117,  118,  140,
 /*  1090 */   120,  121,  122,  123,  140,  125,  126,  140,  140,  129,
 /*  1100 */   130,  131,  140,  140,  140,  140,  140,  140,  140,  140,
 /*  1110 */   140,  111,  112,  113,  140,  115,  116,  117,  118,  140,
 /*  1120 */   120,  121,  122,  123,  140,  125,  126,  140,  140,  129,
 /*  1130 */   130,  131,  140,   12,  134,  135,   17,  137,  140,  140,
 /*  1140 */    21,  140,   21,   24,  140,  140,   27,  140,   29,   30,
 /*  1150 */    31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
 /*  1160 */    41,   42,   12,  140,  140,  140,   45,  140,  140,  140,
 /*  1170 */   140,   21,  140,  140,   24,   25,  140,  140,   57,   58,
 /*  1180 */   140,  140,   63,   62,   63,   64,   65,   66,   67,   68,
 /*  1190 */    69,   70,   71,   72,   73,   45,  140,   84,  140,  140,
 /*  1200 */   140,  140,  140,  140,  140,  140,  140,   57,   58,  140,
 /*  1210 */   140,  140,   62,   63,  140,   65,   66,   67,   68,   69,
 /*  1220 */    70,   71,   72,   73,  111,  112,  113,  140,  115,  116,
 /*  1230 */   117,  118,  140,  120,  121,  122,  123,   84,  125,  126,
 /*  1240 */   140,  140,  129,  130,  131,  140,  140,  134,   12,  140,
 /*  1250 */   140,  140,  140,  140,  140,  140,  140,   21,  140,  140,
 /*  1260 */   140,  140,  140,  140,  111,  112,  113,  140,  115,  116,
 /*  1270 */   117,  118,  140,  120,  121,  122,  123,    1,  125,  126,
 /*  1280 */   140,   45,  129,  130,  131,  140,  140,  140,   12,  140,
 /*  1290 */   137,  140,  140,   57,   58,  140,  140,   21,   62,   63,
 /*  1300 */   140,   65,   66,   67,   68,   69,   70,   71,   72,   73,
 /*  1310 */    74,  140,  140,  140,  140,  140,  140,  140,  140,  140,
 /*  1320 */   140,   45,  140,   84,  140,  140,  140,  140,  140,  140,
 /*  1330 */   140,  140,  140,   57,   58,  140,  140,  140,   62,   63,
 /*  1340 */   140,   65,   66,   67,   68,   69,   70,   71,   72,   73,
 /*  1350 */   111,  112,  113,   84,  115,  116,  117,  118,  140,  120,
 /*  1360 */   121,  122,  123,  140,  125,  126,  140,  140,  129,  130,
 /*  1370 */   131,  140,  140,  140,  140,   84,  140,  140,  140,  140,
 /*  1380 */   111,  112,  113,  140,  115,  116,  117,  118,  140,  120,
 /*  1390 */   121,  122,  123,  140,  125,  126,  140,  140,  129,  130,
 /*  1400 */   131,  140,  111,  112,  113,  140,  115,  116,  117,  118,
 /*  1410 */   140,  120,  121,  122,  123,  140,  125,  126,   84,  140,
 /*  1420 */   129,  130,  131,  112,  113,  140,  115,  116,  117,  118,
 /*  1430 */   140,  120,  121,  122,  123,  140,  125,  126,  140,   84,
 /*  1440 */   129,  130,  131,  140,  140,  111,  112,  113,  140,  115,
 /*  1450 */   116,  117,  118,  140,  120,  121,  122,  123,  140,  125,
 /*  1460 */   126,  140,  140,  129,  130,  131,  111,  112,  113,   84,
 /*  1470 */   115,  116,  117,  118,  140,  120,  121,  122,  123,  140,
 /*  1480 */   125,  126,  140,  140,  129,  130,  131,  140,  140,  140,
 /*  1490 */   140,   84,  140,  140,  140,  140,  111,  112,  113,  140,
 /*  1500 */   115,  116,  117,  118,  140,  120,  121,  122,  123,  140,
 /*  1510 */   125,  126,  140,  140,  129,  130,  131,  140,  111,  112,
 /*  1520 */   113,  140,  115,  116,  117,  118,  140,  120,  121,  122,
 /*  1530 */   123,   84,  125,  126,  140,  140,  129,  130,  131,  140,
 /*  1540 */   140,  140,  140,  140,  140,  140,  140,  140,  140,  140,
 /*  1550 */   140,  140,   84,  140,  140,  140,  140,  140,  111,  112,
 /*  1560 */   113,  140,  115,  116,  117,  118,  140,  120,  121,  122,
 /*  1570 */   123,  140,  125,  126,  140,  140,  129,  130,  131,  111,
 /*  1580 */   112,  113,   84,  115,  116,  117,  118,  140,  120,  121,
 /*  1590 */   122,  123,  140,  125,  126,  140,  140,  129,  130,  131,
 /*  1600 */   140,  140,  140,   84,  140,  140,  140,  140,  140,  111,
 /*  1610 */   112,  113,  140,  115,  116,  117,  118,  140,  120,  121,
 /*  1620 */   122,  123,  140,  125,  126,  140,  140,  129,  130,  131,
 /*  1630 */   111,  112,  113,  140,  115,  116,  117,  118,  140,  120,
 /*  1640 */   121,  122,  123,   84,  125,  126,  140,  140,  129,  130,
 /*  1650 */   131,  140,  140,  140,  140,  140,  140,  140,  140,  140,
 /*  1660 */   140,  140,  140,  140,   84,  140,  140,  140,  140,  140,
 /*  1670 */   111,  112,  113,  140,  115,  116,  117,  118,  140,  120,
 /*  1680 */   121,  122,  123,  140,  125,  126,  140,  140,  129,  130,
 /*  1690 */   131,  111,  112,  113,   84,  115,  116,  117,  118,  140,
 /*  1700 */   120,  121,  122,  123,  140,  125,  126,  140,  140,  129,
 /*  1710 */   130,  131,  140,  140,  140,   84,  140,  140,  140,  140,
 /*  1720 */   140,  111,  112,  113,  140,  115,  116,  117,  118,  140,
 /*  1730 */   120,  121,  122,  123,  140,  125,  126,  140,  140,  129,
 /*  1740 */   130,  131,  111,  112,  113,  140,  115,  116,  117,  118,
 /*  1750 */   140,  120,  121,  122,  123,   84,  125,  126,  140,  140,
 /*  1760 */   129,  130,  131,  140,  140,  140,  140,  140,  140,  140,
 /*  1770 */   140,  140,  140,  140,  140,  140,   84,  140,  140,  140,
 /*  1780 */   140,  140,  111,  112,  113,  140,  115,  116,  117,  118,
 /*  1790 */   140,  120,  121,  122,  123,  140,  125,  126,  140,  140,
 /*  1800 */   129,  130,  131,  111,  112,  113,   84,  115,  116,  117,
 /*  1810 */   118,  140,  120,  121,  122,  123,   12,  125,  126,  140,
 /*  1820 */   140,  129,  130,  131,  140,   21,  140,  140,  140,  140,
 /*  1830 */   140,  140,  140,  111,  112,  113,  140,  115,  116,  117,
 /*  1840 */   118,  140,  120,  121,  122,  123,   12,  125,  126,   45,
 /*  1850 */   140,  129,  130,  131,  140,   21,  140,  140,  140,  140,
 /*  1860 */   140,   57,   58,  140,  140,  140,   62,   63,  140,   65,
 /*  1870 */    66,   67,   68,   69,   70,   71,   72,   73,  140,   45,
 /*  1880 */   140,  140,  140,  140,  140,  140,  140,  140,  140,  140,
 /*  1890 */   140,   57,   58,  140,  140,  140,   62,   63,  140,   65,
 /*  1900 */    66,   67,   68,   69,   70,   71,   72,   73,  140,  112,
 /*  1910 */   113,  140,  115,  116,  117,  118,  140,  120,  121,  122,
 /*  1920 */   123,  140,  125,  126,  140,  140,  129,  130,  131,  112,
 /*  1930 */   113,  140,  115,  116,  117,  118,   12,  120,  121,  122,
 /*  1940 */   123,  140,  125,  126,  140,   21,  129,  130,  131,  140,
 /*  1950 */   112,  113,  140,  115,  116,  117,  118,  140,  120,  121,
 /*  1960 */   122,  123,  140,  125,  126,  140,  112,  129,  130,  131,
 /*  1970 */   116,  117,  118,  140,  120,  121,  122,  123,  140,  125,
 /*  1980 */   126,   57,   58,  129,  130,  131,   62,   63,  140,   65,
 /*  1990 */    66,   67,   68,   69,   70,   71,   72,   73,  140,  140,
 /*  2000 */   140,  140,  140,  140,  112,  140,  140,  115,  116,  117,
 /*  2010 */   118,  140,  120,  121,  122,  123,  140,  125,  126,  140,
 /*  2020 */   112,  129,  130,  131,  116,  117,  118,  140,  120,  121,
 /*  2030 */   122,  123,  140,  125,  126,  140,  140,  129,  130,  131,
};
#define YY_SHIFT_USE_DFLT (-3)
#define YY_SHIFT_MAX 199
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2, 1150,   -2, 1150,
 /*    20 */  1236, 1121, 1804, 1276, 1834, 1834, 1834, 1834, 1834, 1834,
 /*    30 */  1834, 1834, 1834, 1834, 1834, 1834, 1834, 1834, 1834, 1834,
 /*    40 */  1834, 1834, 1834, 1834, 1834, 1834, 1834, 1834, 1924, 1924,
 /*    50 */  1924, 1924, 1924,  342,  342, 1924, 1924,  389, 1924, 1924,
 /*    60 */  1924, 1924, 1924,  410,  410,   74,  119,  238,  464,  464,
 /*    70 */    21,  125,  125,  125,  125,   26,   32, 1119,  270,  312,
 /*    80 */    70,   70,   46,  211,  226,  211,  226,   95,  264,  277,
 /*    90 */   277,  277,  277,  272,  355,  365,   26,  379,  383,  383,
 /*   100 */    32,  383,  383,   32,  383,  383,  383,  383,   32,  272,
 /*   110 */   449,  295,  301,   90,  274,  321,  237,  322,  339,  391,
 /*   120 */    84,  145,  424,  433,  437,  441,  404,  408,  405,  403,
 /*   130 */   407,  413,  404,  408,  403,  407,  413,  443,  450,  400,
 /*   140 */   402,  402,  462,  463,  465,  452,  466,  452,  468,  469,
 /*   150 */   470,  467,  486,  487,  490,  489,  437,  495,  498,  496,
 /*   160 */   488,  501,  499,  497,  504,  497,  516,  506,  505,  503,
 /*   170 */   511,  513,  514,  515,  507,  508,  517,  520,  518,  521,
 /*   180 */   522,  523,  524,  525,  526,  527,  528,  529,  482,  546,
 /*   190 */   484,  404,  404,  404,  545,  538,  553,  437,  437,  437,
};
#define YY_REDUCE_USE_DFLT (-100)
#define YY_REDUCE_MAX 109
static const short yy_reduce_ofst[] = {
 /*     0 */    -8,   44,  104,  156,  216,  268,  328,  380,  440,  492,
 /*    10 */   552,  604,  664,  716,  776,  828,  888,  -81,  940,  970,
 /*    20 */  1000, 1113, 1153, 1239, 1269, 1291, 1334, 1355, 1385, 1407,
 /*    30 */  1447, 1468, 1498, 1519, 1559, 1580, 1610, 1631, 1671, 1692,
 /*    40 */  1722, 1311, 1797, 1817, 1838, 1892, 1854, 1908,  -29,   22,
 /*    50 */    72,  124,  133,  110,  209,   51,    7,  -84,  163,  241,
 /*    60 */   245,  275,  288,  -47,  -22,   81,  -23,  -28,   -3,   85,
 /*    70 */   181,  -99,  -78,  105,  112,  140,  158,  -33,   16,    8,
 /*    80 */    55,   55,   93,   94,  101,   94,  101,  138,  106,  205,
 /*    90 */   235,  246,  256,  231,  289,  292,  304,  307,  319,  320,
 /*   100 */   305,  325,  326,  305,  329,  330,  331,  332,  305,  291,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   341,  341,  341,  341,  341,  341,  341,  341,  341,  341,
 /*    10 */   341,  341,  341,  341,  341,  341,  341,  420,  341,  548,
 /*    20 */   548,  548,  535,  548,  548,  348,  548,  548,  548,  548,
 /*    30 */   548,  548,  548,  350,  352,  548,  548,  548,  548,  548,
 /*    40 */   548,  548,  548,  548,  548,  548,  548,  548,  548,  548,
 /*    50 */   548,  548,  548,  408,  408,  548,  548,  548,  548,  548,
 /*    60 */   548,  548,  548,  548,  548,  548,  548,  546,  369,  369,
 /*    70 */   548,  548,  548,  548,  548,  548,  412,  500,  466,  548,
 /*    80 */   487,  486,  546,  479,  485,  478,  484,  536,  534,  548,
 /*    90 */   548,  548,  548,  539,  365,  548,  548,  373,  548,  548,
 /*   100 */   548,  548,  548,  416,  548,  548,  548,  548,  415,  539,
 /*   110 */   500,  548,  548,  548,  548,  548,  548,  548,  548,  548,
 /*   120 */   548,  548,  548,  548,  547,  548,  443,  461,  467,  475,
 /*   130 */   477,  481,  447,  460,  474,  476,  480,  513,  548,  548,
 /*   140 */   548,  528,  421,  422,  423,  548,  425,  513,  428,  429,
 /*   150 */   432,  548,  548,  548,  548,  548,  370,  548,  548,  548,
 /*   160 */   357,  548,  358,  360,  548,  359,  548,  548,  548,  548,
 /*   170 */   392,  384,  380,  378,  548,  548,  382,  548,  388,  386,
 /*   180 */   390,  400,  396,  394,  398,  404,  402,  406,  548,  548,
 /*   190 */   548,  444,  445,  446,  548,  548,  548,  543,  544,  545,
 /*   200 */   340,  342,  343,  339,  344,  347,  349,  442,  463,  464,
 /*   210 */   465,  490,  496,  497,  498,  499,  501,  502,  513,  514,
 /*   220 */   515,  516,  517,  518,  519,  520,  521,  462,  482,  483,
 /*   230 */   488,  489,  492,  493,  494,  495,  491,  522,  527,  533,
 /*   240 */   524,  525,  531,  532,  526,  530,  529,  523,  528,  424,
 /*   250 */   435,  436,  440,  441,  426,  427,  438,  439,  430,  431,
 /*   260 */   433,  434,  437,  503,  537,  351,  353,  354,  367,  368,
 /*   270 */   355,  356,  364,  363,  362,  361,  375,  376,  374,  366,
 /*   280 */   371,  377,  409,  379,  410,  381,  383,  411,  413,  414,
 /*   290 */   418,  419,  385,  387,  389,  391,  417,  393,  395,  397,
 /*   300 */   399,  401,  403,  405,  407,  540,  538,  504,  505,  506,
 /*   310 */   507,  508,  509,  510,  511,  512,  468,  469,  470,  471,
 /*   320 */   472,  473,  448,  449,  450,  451,  452,  453,  454,  455,
 /*   330 */   456,  457,  458,  459,  345,  542,  346,  541,
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
  "AND_AND",       "NOT",           "EQUAL_EQUAL",   "NOT_EQUAL",   
  "LESS",          "LESS_EQUAL",    "GREATER_EQUAL",  "XOR",         
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
  "blockarg_opt",  "name",          "exprs",         "dict_elems",  
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
 /* 131 */ "comp_op ::= NOT_EQUAL",
 /* 132 */ "comp_op ::= LESS",
 /* 133 */ "comp_op ::= LESS_EQUAL",
 /* 134 */ "comp_op ::= GREATER",
 /* 135 */ "comp_op ::= GREATER_EQUAL",
 /* 136 */ "xor_expr ::= or_expr",
 /* 137 */ "xor_expr ::= xor_expr XOR or_expr",
 /* 138 */ "or_expr ::= and_expr",
 /* 139 */ "or_expr ::= or_expr BAR and_expr",
 /* 140 */ "and_expr ::= shift_expr",
 /* 141 */ "and_expr ::= and_expr AND shift_expr",
 /* 142 */ "shift_expr ::= match_expr",
 /* 143 */ "shift_expr ::= shift_expr shift_op match_expr",
 /* 144 */ "shift_op ::= LSHIFT",
 /* 145 */ "shift_op ::= RSHIFT",
 /* 146 */ "match_expr ::= arith_expr",
 /* 147 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 148 */ "arith_expr ::= term",
 /* 149 */ "arith_expr ::= arith_expr arith_op term",
 /* 150 */ "arith_op ::= PLUS",
 /* 151 */ "arith_op ::= MINUS",
 /* 152 */ "term ::= term term_op factor",
 /* 153 */ "term ::= factor",
 /* 154 */ "term_op ::= STAR",
 /* 155 */ "term_op ::= DIV",
 /* 156 */ "term_op ::= DIV_DIV",
 /* 157 */ "term_op ::= PERCENT",
 /* 158 */ "factor ::= PLUS factor",
 /* 159 */ "factor ::= MINUS factor",
 /* 160 */ "factor ::= TILDA factor",
 /* 161 */ "factor ::= power",
 /* 162 */ "power ::= postfix_expr",
 /* 163 */ "power ::= postfix_expr STAR_STAR factor",
 /* 164 */ "postfix_expr ::= atom",
 /* 165 */ "postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt",
 /* 166 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 167 */ "postfix_expr ::= postfix_expr DOT name",
 /* 168 */ "name ::= NAME",
 /* 169 */ "name ::= EQUAL_EQUAL",
 /* 170 */ "name ::= NOT_EQUAL",
 /* 171 */ "name ::= LESS",
 /* 172 */ "name ::= LESS_EQUAL",
 /* 173 */ "name ::= GREATER",
 /* 174 */ "name ::= GREATER_EQUAL",
 /* 175 */ "atom ::= NAME",
 /* 176 */ "atom ::= NUMBER",
 /* 177 */ "atom ::= REGEXP",
 /* 178 */ "atom ::= STRING",
 /* 179 */ "atom ::= SYMBOL",
 /* 180 */ "atom ::= NIL",
 /* 181 */ "atom ::= TRUE",
 /* 182 */ "atom ::= FALSE",
 /* 183 */ "atom ::= LINE",
 /* 184 */ "atom ::= LBRACKET exprs RBRACKET",
 /* 185 */ "atom ::= LBRACKET RBRACKET",
 /* 186 */ "atom ::= LBRACE RBRACE",
 /* 187 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 188 */ "atom ::= LBRACE exprs RBRACE",
 /* 189 */ "atom ::= LPAR expr RPAR",
 /* 190 */ "exprs ::= expr",
 /* 191 */ "exprs ::= exprs COMMA expr",
 /* 192 */ "dict_elems ::= dict_elem",
 /* 193 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 194 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 195 */ "dict_elem ::= NAME COLON expr",
 /* 196 */ "comma_opt ::=",
 /* 197 */ "comma_opt ::= COMMA",
 /* 198 */ "blockarg_opt ::=",
 /* 199 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 200 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 201 */ "blockarg_params_opt ::=",
 /* 202 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 203 */ "excepts ::= except",
 /* 204 */ "excepts ::= excepts except",
 /* 205 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 206 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 207 */ "except ::= EXCEPT NEWLINE stmts",
 /* 208 */ "finally_opt ::=",
 /* 209 */ "finally_opt ::= FINALLY stmts",
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
  { 80, 1 },
  { 81, 1 },
  { 81, 3 },
  { 82, 0 },
  { 82, 1 },
  { 82, 1 },
  { 82, 7 },
  { 82, 5 },
  { 82, 5 },
  { 82, 5 },
  { 82, 1 },
  { 82, 2 },
  { 82, 1 },
  { 82, 2 },
  { 82, 1 },
  { 82, 2 },
  { 82, 6 },
  { 82, 7 },
  { 82, 4 },
  { 82, 2 },
  { 82, 2 },
  { 91, 1 },
  { 91, 3 },
  { 92, 1 },
  { 92, 3 },
  { 90, 1 },
  { 90, 3 },
  { 89, 0 },
  { 89, 2 },
  { 87, 1 },
  { 87, 5 },
  { 93, 0 },
  { 93, 2 },
  { 83, 8 },
  { 88, 0 },
  { 88, 1 },
  { 95, 1 },
  { 95, 2 },
  { 96, 3 },
  { 94, 9 },
  { 94, 7 },
  { 94, 7 },
  { 94, 5 },
  { 94, 7 },
  { 94, 5 },
  { 94, 5 },
  { 94, 3 },
  { 94, 7 },
  { 94, 5 },
  { 94, 5 },
  { 94, 3 },
  { 94, 5 },
  { 94, 3 },
  { 94, 3 },
  { 94, 1 },
  { 94, 7 },
  { 94, 5 },
  { 94, 5 },
  { 94, 3 },
  { 94, 5 },
  { 94, 3 },
  { 94, 3 },
  { 94, 1 },
  { 94, 5 },
  { 94, 3 },
  { 94, 3 },
  { 94, 1 },
  { 94, 3 },
  { 94, 1 },
  { 94, 1 },
  { 94, 0 },
  { 101, 2 },
  { 100, 2 },
  { 99, 3 },
  { 102, 0 },
  { 102, 1 },
  { 103, 2 },
  { 97, 1 },
  { 97, 3 },
  { 98, 1 },
  { 98, 3 },
  { 104, 2 },
  { 105, 0 },
  { 105, 1 },
  { 105, 3 },
  { 105, 5 },
  { 105, 7 },
  { 105, 3 },
  { 105, 5 },
  { 105, 3 },
  { 105, 1 },
  { 105, 3 },
  { 105, 5 },
  { 105, 3 },
  { 105, 1 },
  { 105, 3 },
  { 105, 1 },
  { 109, 2 },
  { 108, 2 },
  { 106, 1 },
  { 106, 3 },
  { 107, 1 },
  { 107, 3 },
  { 110, 3 },
  { 84, 1 },
  { 111, 3 },
  { 111, 3 },
  { 111, 3 },
  { 111, 3 },
  { 111, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 113, 1 },
  { 113, 3 },
  { 115, 1 },
  { 115, 3 },
  { 116, 1 },
  { 116, 2 },
  { 117, 1 },
  { 117, 3 },
  { 119, 1 },
  { 119, 1 },
  { 119, 1 },
  { 119, 1 },
  { 119, 1 },
  { 119, 1 },
  { 118, 1 },
  { 118, 3 },
  { 120, 1 },
  { 120, 3 },
  { 121, 1 },
  { 121, 3 },
  { 122, 1 },
  { 122, 3 },
  { 124, 1 },
  { 124, 1 },
  { 123, 1 },
  { 123, 3 },
  { 125, 1 },
  { 125, 3 },
  { 127, 1 },
  { 127, 1 },
  { 126, 3 },
  { 126, 1 },
  { 128, 1 },
  { 128, 1 },
  { 128, 1 },
  { 128, 1 },
  { 129, 2 },
  { 129, 2 },
  { 129, 2 },
  { 129, 1 },
  { 130, 1 },
  { 130, 3 },
  { 112, 1 },
  { 112, 5 },
  { 112, 4 },
  { 112, 3 },
  { 133, 1 },
  { 133, 1 },
  { 133, 1 },
  { 133, 1 },
  { 133, 1 },
  { 133, 1 },
  { 133, 1 },
  { 131, 1 },
  { 131, 1 },
  { 131, 1 },
  { 131, 1 },
  { 131, 1 },
  { 131, 1 },
  { 131, 1 },
  { 131, 1 },
  { 131, 1 },
  { 131, 3 },
  { 131, 2 },
  { 131, 2 },
  { 131, 4 },
  { 131, 3 },
  { 131, 3 },
  { 134, 1 },
  { 134, 3 },
  { 135, 1 },
  { 135, 3 },
  { 137, 3 },
  { 137, 3 },
  { 136, 0 },
  { 136, 1 },
  { 132, 0 },
  { 132, 5 },
  { 132, 5 },
  { 138, 0 },
  { 138, 3 },
  { 85, 1 },
  { 85, 2 },
  { 139, 6 },
  { 139, 4 },
  { 139, 3 },
  { 86, 0 },
  { 86, 2 },
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
#line 762 "parser.y"
{
    *pval = yymsp[0].minor.yy247;
}
#line 2347 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 21: /* dotted_names ::= dotted_name */
      case 36: /* decorators ::= decorator */
      case 79: /* params_with_default ::= param_with_default */
      case 99: /* posargs ::= expr */
      case 101: /* kwargs ::= kwarg */
      case 190: /* exprs ::= expr */
      case 192: /* dict_elems ::= dict_elem */
      case 203: /* excepts ::= except */
#line 766 "parser.y"
{
    yygotominor.yy247 = make_array_with(env, yymsp[0].minor.yy247);
}
#line 2362 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 22: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 80: /* params_with_default ::= params_with_default COMMA param_with_default */
#line 769 "parser.y"
{
    yygotominor.yy247 = Array_push(env, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2371 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 27: /* super_opt ::= */
      case 31: /* else_opt ::= */
      case 34: /* decorators_opt ::= */
      case 74: /* param_default_opt ::= */
      case 82: /* args ::= */
      case 196: /* comma_opt ::= */
      case 198: /* blockarg_opt ::= */
      case 201: /* blockarg_params_opt ::= */
      case 208: /* finally_opt ::= */
#line 773 "parser.y"
{
    yygotominor.yy247 = YNIL;
}
#line 2387 "parser.c"
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
      case 136: /* xor_expr ::= or_expr */
      case 138: /* or_expr ::= and_expr */
      case 140: /* and_expr ::= shift_expr */
      case 142: /* shift_expr ::= match_expr */
      case 146: /* match_expr ::= arith_expr */
      case 148: /* arith_expr ::= term */
      case 153: /* term ::= factor */
      case 161: /* factor ::= power */
      case 162: /* power ::= postfix_expr */
      case 164: /* postfix_expr ::= atom */
      case 209: /* finally_opt ::= FINALLY stmts */
#line 776 "parser.y"
{
    yygotominor.yy247 = yymsp[0].minor.yy247;
}
#line 2420 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 782 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy247 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy247, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[-1].minor.yy247);
}
#line 2428 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 786 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy247 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy247, yymsp[-2].minor.yy247, YNIL, yymsp[-1].minor.yy247);
}
#line 2436 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 790 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy247 = Finally_new(env, lineno, yymsp[-3].minor.yy247, yymsp[-1].minor.yy247);
}
#line 2444 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 794 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy247 = While_new(env, lineno, yymsp[-3].minor.yy247, yymsp[-1].minor.yy247);
}
#line 2452 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 798 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy247 = Break_new(env, lineno, YNIL);
}
#line 2460 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 802 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy247 = Break_new(env, lineno, yymsp[0].minor.yy247);
}
#line 2468 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 806 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy247 = Next_new(env, lineno, YNIL);
}
#line 2476 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 810 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy247 = Next_new(env, lineno, yymsp[0].minor.yy247);
}
#line 2484 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 814 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy247 = Return_new(env, lineno, YNIL);
}
#line 2492 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 818 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy247 = Return_new(env, lineno, yymsp[0].minor.yy247);
}
#line 2500 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 822 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy247 = If_new(env, lineno, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[-1].minor.yy247);
}
#line 2508 "parser.c"
        break;
      case 17: /* stmt ::= decorators_opt CLASS NAME super_opt NEWLINE stmts END */
#line 826 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy247 = Klass_new(env, lineno, yymsp[-6].minor.yy247, id, yymsp[-3].minor.yy247, yymsp[-1].minor.yy247);
}
#line 2517 "parser.c"
        break;
      case 18: /* stmt ::= MODULE NAME stmts END */
#line 831 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    yygotominor.yy247 = Module_new(env, lineno, id, yymsp[-1].minor.yy247);
}
#line 2526 "parser.c"
        break;
      case 19: /* stmt ::= NONLOCAL names */
#line 836 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy247 = Nonlocal_new(env, lineno, yymsp[0].minor.yy247);
}
#line 2534 "parser.c"
        break;
      case 20: /* stmt ::= IMPORT dotted_names */
#line 840 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy247 = Import_new(env, lineno, yymsp[0].minor.yy247);
}
#line 2542 "parser.c"
        break;
      case 23: /* dotted_name ::= NAME */
      case 25: /* names ::= NAME */
#line 852 "parser.y"
{
    yygotominor.yy247 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2550 "parser.c"
        break;
      case 24: /* dotted_name ::= dotted_name DOT NAME */
      case 26: /* names ::= names COMMA NAME */
#line 855 "parser.y"
{
    yygotominor.yy247 = Array_push_token_id(env, yymsp[-2].minor.yy247, yymsp[0].minor.yy0);
}
#line 2558 "parser.c"
        break;
      case 30: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 876 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy247, yymsp[-1].minor.yy247, yymsp[0].minor.yy247);
    yygotominor.yy247 = make_array_with(env, node);
}
#line 2567 "parser.c"
        break;
      case 33: /* func_def ::= decorators_opt DEF NAME LPAR params RPAR stmts END */
#line 889 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy247 = FuncDef_new(env, lineno, yymsp[-7].minor.yy247, id, yymsp[-3].minor.yy247, yymsp[-1].minor.yy247);
}
#line 2576 "parser.c"
        break;
      case 37: /* decorators ::= decorators decorator */
      case 204: /* excepts ::= excepts except */
#line 905 "parser.y"
{
    yygotominor.yy247 = Array_push(env, yymsp[-1].minor.yy247, yymsp[0].minor.yy247);
}
#line 2584 "parser.c"
        break;
      case 38: /* decorator ::= AT expr NEWLINE */
      case 189: /* atom ::= LPAR expr RPAR */
      case 202: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 909 "parser.y"
{
    yygotominor.yy247 = yymsp[-1].minor.yy247;
}
#line 2593 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 913 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-8].minor.yy247, yymsp[-6].minor.yy247, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2600 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 916 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-6].minor.yy247, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247, YNIL);
}
#line 2607 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 919 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-6].minor.yy247, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, YNIL, yymsp[0].minor.yy247);
}
#line 2614 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 922 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247, YNIL, YNIL);
}
#line 2621 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 925 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-6].minor.yy247, yymsp[-4].minor.yy247, YNIL, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2628 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 928 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, YNIL, yymsp[0].minor.yy247, YNIL);
}
#line 2635 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 931 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, YNIL, YNIL, yymsp[0].minor.yy247);
}
#line 2642 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA params_with_default */
#line 934 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-2].minor.yy247, yymsp[0].minor.yy247, YNIL, YNIL, YNIL);
}
#line 2649 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 937 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-6].minor.yy247, YNIL, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2656 "parser.c"
        break;
      case 48: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 940 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-4].minor.yy247, YNIL, yymsp[-2].minor.yy247, yymsp[0].minor.yy247, YNIL);
}
#line 2663 "parser.c"
        break;
      case 49: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 943 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-4].minor.yy247, YNIL, yymsp[-2].minor.yy247, YNIL, yymsp[0].minor.yy247);
}
#line 2670 "parser.c"
        break;
      case 50: /* params ::= params_without_default COMMA block_param */
#line 946 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-2].minor.yy247, YNIL, yymsp[0].minor.yy247, YNIL, YNIL);
}
#line 2677 "parser.c"
        break;
      case 51: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 949 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-4].minor.yy247, YNIL, YNIL, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2684 "parser.c"
        break;
      case 52: /* params ::= params_without_default COMMA var_param */
#line 952 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-2].minor.yy247, YNIL, YNIL, yymsp[0].minor.yy247, YNIL);
}
#line 2691 "parser.c"
        break;
      case 53: /* params ::= params_without_default COMMA kw_param */
#line 955 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[-2].minor.yy247, YNIL, YNIL, YNIL, yymsp[0].minor.yy247);
}
#line 2698 "parser.c"
        break;
      case 54: /* params ::= params_without_default */
#line 958 "parser.y"
{
    yygotominor.yy247 = Params_new(env, yymsp[0].minor.yy247, YNIL, YNIL, YNIL, YNIL);
}
#line 2705 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 961 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, yymsp[-6].minor.yy247, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2712 "parser.c"
        break;
      case 56: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 964 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247, YNIL);
}
#line 2719 "parser.c"
        break;
      case 57: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 967 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, YNIL, yymsp[0].minor.yy247);
}
#line 2726 "parser.c"
        break;
      case 58: /* params ::= params_with_default COMMA block_param */
#line 970 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, yymsp[-2].minor.yy247, yymsp[0].minor.yy247, YNIL, YNIL);
}
#line 2733 "parser.c"
        break;
      case 59: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 973 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, yymsp[-4].minor.yy247, YNIL, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2740 "parser.c"
        break;
      case 60: /* params ::= params_with_default COMMA var_param */
#line 976 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, yymsp[-2].minor.yy247, YNIL, yymsp[0].minor.yy247, YNIL);
}
#line 2747 "parser.c"
        break;
      case 61: /* params ::= params_with_default COMMA kw_param */
#line 979 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, yymsp[-2].minor.yy247, YNIL, YNIL, yymsp[0].minor.yy247);
}
#line 2754 "parser.c"
        break;
      case 62: /* params ::= params_with_default */
#line 982 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, yymsp[0].minor.yy247, YNIL, YNIL, YNIL);
}
#line 2761 "parser.c"
        break;
      case 63: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 985 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2768 "parser.c"
        break;
      case 64: /* params ::= block_param COMMA var_param */
#line 988 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy247, yymsp[0].minor.yy247, YNIL);
}
#line 2775 "parser.c"
        break;
      case 65: /* params ::= block_param COMMA kw_param */
#line 991 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy247, YNIL, yymsp[0].minor.yy247);
}
#line 2782 "parser.c"
        break;
      case 66: /* params ::= block_param */
#line 994 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy247, YNIL, YNIL);
}
#line 2789 "parser.c"
        break;
      case 67: /* params ::= var_param COMMA kw_param */
#line 997 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2796 "parser.c"
        break;
      case 68: /* params ::= var_param */
#line 1000 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy247, YNIL);
}
#line 2803 "parser.c"
        break;
      case 69: /* params ::= kw_param */
#line 1003 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy247);
}
#line 2810 "parser.c"
        break;
      case 70: /* params ::= */
#line 1006 "parser.y"
{
    yygotominor.yy247 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2817 "parser.c"
        break;
      case 71: /* kw_param ::= STAR_STAR NAME */
#line 1010 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy247 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2826 "parser.c"
        break;
      case 72: /* var_param ::= STAR NAME */
#line 1016 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy247 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2835 "parser.c"
        break;
      case 73: /* block_param ::= AMPER NAME param_default_opt */
#line 1022 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy247 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy247);
}
#line 2844 "parser.c"
        break;
      case 77: /* params_without_default ::= NAME */
#line 1039 "parser.y"
{
    yygotominor.yy247 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy247, lineno, id, YNIL);
}
#line 2854 "parser.c"
        break;
      case 78: /* params_without_default ::= params_without_default COMMA NAME */
#line 1045 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy247, lineno, id, YNIL);
    yygotominor.yy247 = yymsp[-2].minor.yy247;
}
#line 2864 "parser.c"
        break;
      case 81: /* param_with_default ::= NAME param_default */
#line 1059 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy247 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy247);
}
#line 2873 "parser.c"
        break;
      case 83: /* args ::= posargs */
#line 1068 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, yymsp[0].minor.yy247, YNIL, YNIL, YNIL);
}
#line 2881 "parser.c"
        break;
      case 84: /* args ::= posargs COMMA kwargs */
#line 1072 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, yymsp[-2].minor.yy247, yymsp[0].minor.yy247, YNIL, YNIL);
}
#line 2889 "parser.c"
        break;
      case 85: /* args ::= posargs COMMA kwargs COMMA vararg */
#line 1076 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247, YNIL);
}
#line 2897 "parser.c"
        break;
      case 86: /* args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg */
#line 1080 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-6].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, yymsp[-6].minor.yy247, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2905 "parser.c"
        break;
      case 87: /* args ::= posargs COMMA vararg */
#line 1084 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, yymsp[-2].minor.yy247, YNIL, yymsp[0].minor.yy247, YNIL);
}
#line 2913 "parser.c"
        break;
      case 88: /* args ::= posargs COMMA vararg COMMA varkwarg */
#line 1088 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, yymsp[-4].minor.yy247, YNIL, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2921 "parser.c"
        break;
      case 89: /* args ::= posargs COMMA varkwarg */
#line 1092 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, yymsp[-2].minor.yy247, YNIL, YNIL, yymsp[0].minor.yy247);
}
#line 2929 "parser.c"
        break;
      case 90: /* args ::= kwargs */
#line 1096 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, YNIL, yymsp[0].minor.yy247, YNIL, YNIL);
}
#line 2937 "parser.c"
        break;
      case 91: /* args ::= kwargs COMMA vararg */
#line 1100 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy247, yymsp[0].minor.yy247, YNIL);
}
#line 2945 "parser.c"
        break;
      case 92: /* args ::= kwargs COMMA vararg COMMA varkwarg */
#line 1104 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, YNIL, yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2953 "parser.c"
        break;
      case 93: /* args ::= kwargs COMMA varkwarg */
#line 1108 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy247, 0));
    yygotominor.yy247 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy247, YNIL, yymsp[0].minor.yy247);
}
#line 2961 "parser.c"
        break;
      case 94: /* args ::= vararg */
#line 1112 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy247);
    yygotominor.yy247 = Args_new(env, lineno, YNIL, YNIL, yymsp[0].minor.yy247, YNIL);
}
#line 2969 "parser.c"
        break;
      case 95: /* args ::= vararg COMMA varkwarg */
#line 1116 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy247);
    yygotominor.yy247 = Args_new(env, lineno, YNIL, YNIL, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 2977 "parser.c"
        break;
      case 96: /* args ::= varkwarg */
#line 1120 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy247);
    yygotominor.yy247 = Args_new(env, lineno, YNIL, YNIL, YNIL, yymsp[0].minor.yy247);
}
#line 2985 "parser.c"
        break;
      case 100: /* posargs ::= posargs COMMA expr */
      case 102: /* kwargs ::= kwargs COMMA kwarg */
      case 191: /* exprs ::= exprs COMMA expr */
      case 193: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 1136 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
    yygotominor.yy247 = yymsp[-2].minor.yy247;
}
#line 2996 "parser.c"
        break;
      case 103: /* kwarg ::= NAME COLON expr */
#line 1149 "parser.y"
{
    yygotominor.yy247 = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(yymsp[-2].minor.yy0));
    PTR_AS(YogNode, yygotominor.yy247)->u.kwarg.name = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    PTR_AS(YogNode, yygotominor.yy247)->u.kwarg.value = yymsp[0].minor.yy247;
}
#line 3005 "parser.c"
        break;
      case 105: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 1159 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy247);
    yygotominor.yy247 = Assign_new(env, lineno, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 3013 "parser.c"
        break;
      case 106: /* assign_expr ::= postfix_expr augmented_assign_op logical_or_expr */
#line 1163 "parser.y"
{
    yygotominor.yy247 = AugmentedAssign_new(env, NODE_LINENO(yymsp[-2].minor.yy247), yymsp[-2].minor.yy247, VAL2ID(yymsp[-1].minor.yy247), yymsp[0].minor.yy247);
}
#line 3020 "parser.c"
        break;
      case 107: /* assign_expr ::= postfix_expr AND_AND_EQUAL logical_or_expr */
#line 1166 "parser.y"
{
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy247);
    expr = LogicalAnd_new(env, lineno, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
    assign = Assign_new(env, lineno, yymsp[-2].minor.yy247, expr);

    POP_LOCALS(env);

    yygotominor.yy247 = assign;
}
#line 3037 "parser.c"
        break;
      case 108: /* assign_expr ::= postfix_expr BAR_BAR_EQUAL logical_or_expr */
#line 1179 "parser.y"
{
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy247);
    expr = LogicalOr_new(env, lineno, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
    assign = Assign_new(env, lineno, yymsp[-2].minor.yy247, expr);

    POP_LOCALS(env);

    yygotominor.yy247 = assign;
}
#line 3054 "parser.c"
        break;
      case 110: /* augmented_assign_op ::= PLUS_EQUAL */
#line 1196 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "+"));
}
#line 3061 "parser.c"
        break;
      case 111: /* augmented_assign_op ::= MINUS_EQUAL */
#line 1199 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "-"));
}
#line 3068 "parser.c"
        break;
      case 112: /* augmented_assign_op ::= STAR_EQUAL */
#line 1202 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "*"));
}
#line 3075 "parser.c"
        break;
      case 113: /* augmented_assign_op ::= DIV_EQUAL */
#line 1205 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "/"));
}
#line 3082 "parser.c"
        break;
      case 114: /* augmented_assign_op ::= DIV_DIV_EQUAL */
#line 1208 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "//"));
}
#line 3089 "parser.c"
        break;
      case 115: /* augmented_assign_op ::= PERCENT_EQUAL */
#line 1211 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "%"));
}
#line 3096 "parser.c"
        break;
      case 116: /* augmented_assign_op ::= BAR_EQUAL */
#line 1214 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "|"));
}
#line 3103 "parser.c"
        break;
      case 117: /* augmented_assign_op ::= AND_EQUAL */
#line 1217 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "&"));
}
#line 3110 "parser.c"
        break;
      case 118: /* augmented_assign_op ::= XOR_EQUAL */
#line 1220 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "^"));
}
#line 3117 "parser.c"
        break;
      case 119: /* augmented_assign_op ::= STAR_STAR_EQUAL */
#line 1223 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "**"));
}
#line 3124 "parser.c"
        break;
      case 120: /* augmented_assign_op ::= LSHIFT_EQUAL */
#line 1226 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "<<"));
}
#line 3131 "parser.c"
        break;
      case 121: /* augmented_assign_op ::= RSHIFT_EQUAL */
#line 1229 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, ">>"));
}
#line 3138 "parser.c"
        break;
      case 123: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 1236 "parser.y"
{
    yygotominor.yy247 = LogicalOr_new(env, NODE_LINENO(yymsp[-2].minor.yy247), yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 3145 "parser.c"
        break;
      case 125: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 1243 "parser.y"
{
    yygotominor.yy247 = LogicalAnd_new(env, NODE_LINENO(yymsp[-2].minor.yy247), yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 3152 "parser.c"
        break;
      case 127: /* not_expr ::= NOT not_expr */
#line 1250 "parser.y"
{
    yygotominor.yy247 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy247)->u.not.expr = yymsp[0].minor.yy247;
}
#line 3160 "parser.c"
        break;
      case 129: /* comparison ::= xor_expr comp_op xor_expr */
#line 1258 "parser.y"
{
    yygotominor.yy247 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy247), yymsp[-2].minor.yy247, VAL2ID(yymsp[-1].minor.yy247), yymsp[0].minor.yy247);
}
#line 3167 "parser.c"
        break;
      case 130: /* comp_op ::= EQUAL_EQUAL */
      case 169: /* name ::= EQUAL_EQUAL */
#line 1262 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "=="));
}
#line 3175 "parser.c"
        break;
      case 131: /* comp_op ::= NOT_EQUAL */
      case 170: /* name ::= NOT_EQUAL */
#line 1265 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "!="));
}
#line 3183 "parser.c"
        break;
      case 132: /* comp_op ::= LESS */
      case 171: /* name ::= LESS */
#line 1268 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "<"));
}
#line 3191 "parser.c"
        break;
      case 133: /* comp_op ::= LESS_EQUAL */
      case 172: /* name ::= LESS_EQUAL */
#line 1271 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, "<="));
}
#line 3199 "parser.c"
        break;
      case 134: /* comp_op ::= GREATER */
      case 173: /* name ::= GREATER */
#line 1274 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, ">"));
}
#line 3207 "parser.c"
        break;
      case 135: /* comp_op ::= GREATER_EQUAL */
      case 174: /* name ::= GREATER_EQUAL */
#line 1277 "parser.y"
{
    yygotominor.yy247 = ID2VAL(YogVM_intern(env, env->vm, ">="));
}
#line 3215 "parser.c"
        break;
      case 137: /* xor_expr ::= xor_expr XOR or_expr */
      case 139: /* or_expr ::= or_expr BAR and_expr */
      case 141: /* and_expr ::= and_expr AND shift_expr */
#line 1284 "parser.y"
{
    yygotominor.yy247 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy247), yymsp[-2].minor.yy247, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy247);
}
#line 3224 "parser.c"
        break;
      case 143: /* shift_expr ::= shift_expr shift_op match_expr */
      case 149: /* arith_expr ::= arith_expr arith_op term */
      case 152: /* term ::= term term_op factor */
#line 1305 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy247);
    yygotominor.yy247 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy247, VAL2ID(yymsp[-1].minor.yy247), yymsp[0].minor.yy247);
}
#line 3234 "parser.c"
        break;
      case 144: /* shift_op ::= LSHIFT */
      case 145: /* shift_op ::= RSHIFT */
      case 150: /* arith_op ::= PLUS */
      case 151: /* arith_op ::= MINUS */
      case 154: /* term_op ::= STAR */
      case 155: /* term_op ::= DIV */
      case 156: /* term_op ::= DIV_DIV */
      case 157: /* term_op ::= PERCENT */
      case 168: /* name ::= NAME */
#line 1310 "parser.y"
{
    yygotominor.yy247 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 3249 "parser.c"
        break;
      case 147: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1320 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy247);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy247 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy247, id, yymsp[0].minor.yy247);
}
#line 3258 "parser.c"
        break;
      case 158: /* factor ::= PLUS factor */
#line 1362 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy247 = FuncCall_new3(env, lineno, yymsp[0].minor.yy247, id);
}
#line 3267 "parser.c"
        break;
      case 159: /* factor ::= MINUS factor */
#line 1367 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy247 = FuncCall_new3(env, lineno, yymsp[0].minor.yy247, id);
}
#line 3276 "parser.c"
        break;
      case 160: /* factor ::= TILDA factor */
#line 1372 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy247 = FuncCall_new3(env, lineno, yymsp[0].minor.yy247, id);
}
#line 3285 "parser.c"
        break;
      case 163: /* power ::= postfix_expr STAR_STAR factor */
#line 1384 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy247);
    ID id = YogVM_intern(env, env->vm, "**");
    yygotominor.yy247 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy247, id, yymsp[0].minor.yy247);
}
#line 3294 "parser.c"
        break;
      case 165: /* postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt */
#line 1393 "parser.y"
{
    yygotominor.yy247 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy247), yymsp[-4].minor.yy247, yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 3301 "parser.c"
        break;
      case 166: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1396 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy247);
    yygotominor.yy247 = Subscript_new(env, lineno, yymsp[-3].minor.yy247, yymsp[-1].minor.yy247);
}
#line 3309 "parser.c"
        break;
      case 167: /* postfix_expr ::= postfix_expr DOT name */
#line 1400 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy247);
    yygotominor.yy247 = Attr_new(env, lineno, yymsp[-2].minor.yy247, VAL2ID(yymsp[0].minor.yy247));
}
#line 3317 "parser.c"
        break;
      case 175: /* atom ::= NAME */
#line 1427 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy247 = Variable_new(env, lineno, id);
}
#line 3326 "parser.c"
        break;
      case 176: /* atom ::= NUMBER */
      case 177: /* atom ::= REGEXP */
      case 178: /* atom ::= STRING */
      case 179: /* atom ::= SYMBOL */
#line 1432 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy247 = Literal_new(env, lineno, val);
}
#line 3338 "parser.c"
        break;
      case 180: /* atom ::= NIL */
#line 1452 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy247 = Literal_new(env, lineno, YNIL);
}
#line 3346 "parser.c"
        break;
      case 181: /* atom ::= TRUE */
#line 1456 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy247 = Literal_new(env, lineno, YTRUE);
}
#line 3354 "parser.c"
        break;
      case 182: /* atom ::= FALSE */
#line 1460 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy247 = Literal_new(env, lineno, YFALSE);
}
#line 3362 "parser.c"
        break;
      case 183: /* atom ::= LINE */
#line 1464 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy247 = Literal_new(env, lineno, val);
}
#line 3371 "parser.c"
        break;
      case 184: /* atom ::= LBRACKET exprs RBRACKET */
#line 1469 "parser.y"
{
    yygotominor.yy247 = Array_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy247);
}
#line 3378 "parser.c"
        break;
      case 185: /* atom ::= LBRACKET RBRACKET */
#line 1472 "parser.y"
{
    yygotominor.yy247 = Array_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3385 "parser.c"
        break;
      case 186: /* atom ::= LBRACE RBRACE */
#line 1475 "parser.y"
{
    yygotominor.yy247 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3392 "parser.c"
        break;
      case 187: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1478 "parser.y"
{
    yygotominor.yy247 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy247);
}
#line 3399 "parser.c"
        break;
      case 188: /* atom ::= LBRACE exprs RBRACE */
#line 1481 "parser.y"
{
    yygotominor.yy247 = Set_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy247);
}
#line 3406 "parser.c"
        break;
      case 194: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1503 "parser.y"
{
    yygotominor.yy247 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy247), yymsp[-2].minor.yy247, yymsp[0].minor.yy247);
}
#line 3413 "parser.c"
        break;
      case 195: /* dict_elem ::= NAME COLON expr */
#line 1506 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy247 = DictElem_new(env, lineno, var, yymsp[0].minor.yy247);
}
#line 3423 "parser.c"
        break;
      case 197: /* comma_opt ::= COMMA */
#line 1516 "parser.y"
{
    yygotominor.yy247 = yymsp[0].minor.yy0;
}
#line 3430 "parser.c"
        break;
      case 199: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 200: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1523 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy247 = BlockArg_new(env, lineno, yymsp[-3].minor.yy247, yymsp[-1].minor.yy247);
}
#line 3439 "parser.c"
        break;
      case 205: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1546 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy247 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy247, id, yymsp[0].minor.yy247);
}
#line 3449 "parser.c"
        break;
      case 206: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1552 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy247 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy247, NO_EXC_VAR, yymsp[0].minor.yy247);
}
#line 3457 "parser.c"
        break;
      case 207: /* except ::= EXCEPT NEWLINE stmts */
#line 1556 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy247 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy247);
}
#line 3465 "parser.c"
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
