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
    case NODE_CLASS:
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
    case NODE_RAISE:
        KEEP(raise.expr);
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
Class_new(YogEnv* env, uint_t lineno, YogVal decorators, ID name, YogVal super, YogVal stmts)
{
    SAVE_ARGS3(env, decorators, super, stmts);

    YogVal node = YogNode_new(env, NODE_CLASS, lineno);
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

static YogVal
Raise_new(YogEnv* env, uint_t lineno, YogVal expr)
{
    SAVE_ARG(env, expr);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    node = YogNode_new(env, NODE_RAISE, lineno);
    NODE(node)->u.raise.expr = expr;

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
#line 782 "parser.c"
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
#define YYNOCODE 142
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy229;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 340
#define YYNRULE 211
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
 /*     0 */     1,  257,  109,  276,   24,   25,   33,   34,   35,  375,
 /*    10 */   219,  160,   96,   76,   37,  175,  176,  178,   43,  375,
 /*    20 */    28,    2,   38,    3,  144,  147,  256,  258,  208,   78,
 /*    30 */   133,   68,  134,  228,  210,   79,   40,  135,  136,   86,
 /*    40 */   137,  111,   87,   82,   48,   49,  237,  216,  218,  130,
 /*    50 */   136,   86,  137,  158,   87,   82,   60,   61,  237,  216,
 /*    60 */   218,   62,   21,  168,  220,  221,  222,  223,  224,  225,
 /*    70 */   226,  227,   20,  552,  112,  204,  202,  203,  173,  177,
 /*    80 */   288,  117,  310,  292,  111,  339,  146,   18,   98,  280,
 /*    90 */   129,    3,  135,  136,   86,  137,   23,   87,   82,   30,
 /*   100 */    31,  237,  216,  218,  208,   78,  133,  195,  134,  228,
 /*   110 */   210,   79,   59,  135,  136,   86,  137,  110,   87,   82,
 /*   120 */    94,   18,  237,  216,  218,   16,  111,   67,  204,  202,
 /*   130 */   203,  145,  155,  253,  117,  131,   86,  137,  269,   87,
 /*   140 */    82,   98,  280,  237,  216,  218,  170,   57,  270,  171,
 /*   150 */   182,  186,  188,  306,  269,  124,  298,  208,   78,  133,
 /*   160 */   337,  134,  228,  210,   79,  111,  135,  136,   86,  137,
 /*   170 */    18,   87,   82,  206,  111,  237,  216,  218,   85,   82,
 /*   180 */   175,  176,  237,  216,  218,   22,  111,   83,  204,  202,
 /*   190 */   203,  212,  216,  218,  117,  111,   84,  137,   23,   87,
 /*   200 */    82,   98,  280,  237,  216,  218,  132,   18,   87,   82,
 /*   210 */   265,  146,  237,  216,  218,  174,  285,  208,   78,  133,
 /*   220 */    53,  134,  228,  210,   79,   31,  135,  136,   86,  137,
 /*   230 */    26,   87,   82,  322,  264,  237,  216,  218,  180,  295,
 /*   240 */   125,  204,  202,  203,  104,  140,  189,  117,   30,  171,
 /*   250 */   182,  186,  188,  306,   98,  280,  298,  175,  176,  178,
 /*   260 */   311,  318,  319,  320,  321,  323,   50,  316,  184,  300,
 /*   270 */   208,   78,  133,   18,  134,  228,  210,   79,  238,  135,
 /*   280 */   136,   86,  137,   14,   87,   82,  187,  304,  237,  216,
 /*   290 */   218,  111,  150,  260,  253,  312,  313,  314,  315,  317,
 /*   300 */   113,  204,  202,  203,   18,   81,    8,  117,  237,  216,
 /*   310 */   218,  250,  101,  255,   98,  280,  163,  166,  172,  179,
 /*   320 */   181,  297,   26,   36,  298,  175,  176,  178,  289,  290,
 /*   330 */   208,   78,  133,   55,  134,  228,  210,   79,  259,  135,
 /*   340 */   136,   86,  137,  153,   87,   82,  111,  308,  237,  216,
 /*   350 */   218,  229,  230,  116,  204,  202,  203,  111,  340,   18,
 /*   360 */   117,  196,  261,  213,  216,  218,   39,   98,  280,  231,
 /*   370 */   232,  233,  274,  159,  214,  216,  218,  183,  185,  302,
 /*   380 */   245,  161,  292,  208,   78,  133,  164,  134,  228,  210,
 /*   390 */    79,  111,  135,  136,   86,  137,   18,   87,   82,  271,
 /*   400 */    38,  237,  216,  218,  111,  234,  235,  236,  215,  216,
 /*   410 */   218,  278,  175,   69,  204,  202,  203,  283,  287,  294,
 /*   420 */   117,  217,  216,  218,  293,   80,  296,   98,  280,   17,
 /*   430 */    18,   18,   63,  272,  282,   18,  299,  301,  338,  303,
 /*   440 */   305,  190,  205,  208,   78,  133,   18,  134,  228,  210,
 /*   450 */    79,    4,  135,  136,   86,  137,   46,   87,   82,   47,
 /*   460 */    50,  237,  216,  218,   51,   27,   70,  204,  202,  203,
 /*   470 */    52,   41,   56,  117,  239,  242,   19,   29,   71,   90,
 /*   480 */    98,  280,   32,   91,   66,   92,   93,   88,    5,    6,
 /*   490 */   268,    7,   95,    9,   10,  162,  208,   78,  133,  165,
 /*   500 */   134,  228,  210,   79,  279,  135,  136,   86,  137,  553,
 /*   510 */    87,   82,   97,  273,  237,  216,  218,  169,  275,   11,
 /*   520 */    54,   58,  307,  284,   64,   72,  157,  204,  202,  203,
 /*   530 */    99,  286,  100,  117,   77,   73,   12,  102,  103,  309,
 /*   540 */    98,  280,   65,   74,  105,  106,   75,  107,  108,  336,
 /*   550 */   197,   13,  553,  553,  553,  553,  208,   78,  133,  553,
 /*   560 */   134,  228,  210,   79,  553,  135,  136,   86,  137,  553,
 /*   570 */    87,   82,  553,  553,  237,  216,  218,  553,  553,  118,
 /*   580 */   204,  202,  203,  553,  553,  553,  117,  553,  553,  553,
 /*   590 */   553,  553,  553,   98,  280,  553,  553,  553,  553,  553,
 /*   600 */   553,  553,  553,  553,  553,  553,  553,  553,  553,  208,
 /*   610 */    78,  133,  553,  134,  228,  210,   79,  553,  135,  136,
 /*   620 */    86,  137,  553,   87,   82,  553,  553,  237,  216,  218,
 /*   630 */   553,  553,  553,  553,  553,  553,  553,  553,  553,  119,
 /*   640 */   204,  202,  203,  553,  553,  553,  117,  553,  553,  553,
 /*   650 */   553,  553,  553,   98,  280,  553,  553,  553,  553,  553,
 /*   660 */   553,  553,  553,  553,  553,  553,  553,  553,  553,  208,
 /*   670 */    78,  133,  553,  134,  228,  210,   79,  553,  135,  136,
 /*   680 */    86,  137,  553,   87,   82,  553,  553,  237,  216,  218,
 /*   690 */   553,  553,  120,  204,  202,  203,  553,  553,  553,  117,
 /*   700 */   553,  553,  553,  553,  553,  553,   98,  280,  553,  553,
 /*   710 */   553,  553,  553,  553,  553,  553,  553,  553,  553,  553,
 /*   720 */   553,  553,  208,   78,  133,  553,  134,  228,  210,   79,
 /*   730 */   553,  135,  136,   86,  137,  553,   87,   82,  553,  553,
 /*   740 */   237,  216,  218,  553,  553,  553,  553,  553,  553,  553,
 /*   750 */   553,  553,  121,  204,  202,  203,  553,  553,  553,  117,
 /*   760 */   553,  553,  553,  553,  553,  553,   98,  280,  553,  553,
 /*   770 */   553,  553,  553,  553,  553,  553,  553,  553,  553,  553,
 /*   780 */   553,  553,  208,   78,  133,  553,  134,  228,  210,   79,
 /*   790 */   553,  135,  136,   86,  137,  553,   87,   82,  553,  553,
 /*   800 */   237,  216,  218,  553,  553,  198,  204,  202,  203,  553,
 /*   810 */   553,  553,  117,  553,  553,  553,  553,  553,  553,   98,
 /*   820 */   280,  553,  553,  553,  553,  553,  553,  553,  553,  553,
 /*   830 */   553,  553,  553,  553,  553,  208,   78,  133,  553,  134,
 /*   840 */   228,  210,   79,  553,  135,  136,   86,  137,  553,   87,
 /*   850 */    82,  553,  553,  237,  216,  218,  553,  553,  553,  553,
 /*   860 */   553,  553,  553,  553,  553,  199,  204,  202,  203,  553,
 /*   870 */   553,  553,  117,  553,  553,  553,  553,  553,  553,   98,
 /*   880 */   280,  553,  553,  553,  553,  553,  553,  553,  553,  553,
 /*   890 */   553,  553,  553,  553,  553,  208,   78,  133,  553,  134,
 /*   900 */   228,  210,   79,  553,  135,  136,   86,  137,  553,   87,
 /*   910 */    82,  553,  553,  237,  216,  218,  553,  553,  200,  204,
 /*   920 */   202,  203,  553,  553,  553,  117,  553,  553,  553,  553,
 /*   930 */   553,  553,   98,  280,  553,  553,  553,  553,  553,  553,
 /*   940 */   553,  553,  553,  553,  553,  553,  553,  553,  208,   78,
 /*   950 */   133,  553,  134,  228,  210,   79,  553,  135,  136,   86,
 /*   960 */   137,  553,   87,   82,  553,  553,  237,  216,  218,  553,
 /*   970 */   553,  553,  553,  553,  553,  553,  553,  553,  123,  204,
 /*   980 */   202,  203,  553,  553,  553,  117,  553,  553,  553,  553,
 /*   990 */   553,  553,   98,  280,  553,  553,  553,  553,  553,  553,
 /*  1000 */   553,  553,  553,  553,  553,  553,  553,  553,  208,   78,
 /*  1010 */   133,  263,  134,  228,  210,   79,  553,  135,  136,   86,
 /*  1020 */   137,  553,   87,   82,  553,  553,  237,  216,  218,  553,
 /*  1030 */   553,  553,  152,  143,  149,  151,  262,  258,  208,   78,
 /*  1040 */   133,  553,  134,  228,  210,   79,  553,  135,  136,   86,
 /*  1050 */   137,  553,   87,   82,  553,  553,  237,  216,  218,  553,
 /*  1060 */   553,  553,  201,  202,  203,  553,  553,  553,  117,  553,
 /*  1070 */   553,  553,  553,  553,  553,   98,  280,  553,  553,  553,
 /*  1080 */   553,  553,  553,  553,  553,  553,  553,  553,  553,  553,
 /*  1090 */   553,  208,   78,  133,  142,  134,  228,  210,   79,  553,
 /*  1100 */   135,  136,   86,  137,  553,   87,   82,  553,  553,  237,
 /*  1110 */   216,  218,  553,  553,  553,  553,  553,  553,  553,  553,
 /*  1120 */   553,  208,   78,  133,  553,  134,  228,  210,   79,  553,
 /*  1130 */   135,  136,   86,  137,  553,   87,   82,  553,  138,  237,
 /*  1140 */   216,  218,   80,  553,  115,   89,   17,  246,   28,   63,
 /*  1150 */   553,  553,   42,  553,   44,   45,  324,  325,  326,  327,
 /*  1160 */   328,  329,  330,  331,  332,  333,  334,  335,  148,  553,
 /*  1170 */   553,  553,   48,  553,  553,  553,  553,  553,   28,  553,
 /*  1180 */   553,   30,   31,  553,   60,   61,  553,  553,   41,   62,
 /*  1190 */    21,  553,  220,  221,  222,  223,  224,  225,  226,  227,
 /*  1200 */    20,  241,   48,  553,  553,  553,  553,  249,  553,  553,
 /*  1210 */   553,  553,  553,  553,   60,   61,  553,  553,  553,   62,
 /*  1220 */    21,  553,  220,  221,  222,  223,  224,  225,  226,  227,
 /*  1230 */    20,  141,  553,  553,  208,   78,  133,  553,  134,  228,
 /*  1240 */   210,   79,  553,  135,  136,   86,  137,  553,   87,   82,
 /*  1250 */   553,  219,  237,  216,  218,  553,  553,  114,  208,   78,
 /*  1260 */   133,   28,  134,  228,  210,   79,  553,  135,  136,   86,
 /*  1270 */   137,   15,   87,   82,  553,  553,  237,  216,  218,  553,
 /*  1280 */   553,  553,  219,  553,  243,   48,  553,  553,  553,  553,
 /*  1290 */   553,  553,   28,  553,  553,  553,  553,   60,   61,  553,
 /*  1300 */   553,  553,   62,   21,  248,  220,  221,  222,  223,  224,
 /*  1310 */   225,  226,  227,   20,  553,  553,   48,  553,  553,  553,
 /*  1320 */   122,  553,  553,  553,  553,  553,  553,  553,   60,   61,
 /*  1330 */   553,  553,  553,   62,   21,  553,  220,  221,  222,  223,
 /*  1340 */   224,  225,  226,  227,   20,  126,  553,  208,   78,  133,
 /*  1350 */   553,  134,  228,  210,   79,  553,  135,  136,   86,  137,
 /*  1360 */   553,   87,   82,  553,  553,  237,  216,  218,  207,  553,
 /*  1370 */   553,  553,  208,   78,  133,  553,  134,  228,  210,   79,
 /*  1380 */   553,  135,  136,   86,  137,  553,   87,   82,  553,  247,
 /*  1390 */   237,  216,  218,  553,  553,  208,   78,  133,  553,  134,
 /*  1400 */   228,  210,   79,  553,  135,  136,   86,  137,  553,   87,
 /*  1410 */    82,  553,  553,  237,  216,  218,  208,   78,  133,  240,
 /*  1420 */   134,  228,  210,   79,  553,  135,  136,   86,  137,  553,
 /*  1430 */    87,   82,  553,  553,  237,  216,  218,  553,  553,  553,
 /*  1440 */   139,  553,  553,  553,  553,  553,  208,   78,  133,  553,
 /*  1450 */   134,  228,  210,   79,  553,  135,  136,   86,  137,  553,
 /*  1460 */    87,   82,  244,  553,  237,  216,  218,  208,   78,  133,
 /*  1470 */   553,  134,  228,  210,   79,  553,  135,  136,   86,  137,
 /*  1480 */   553,   87,   82,  251,  553,  237,  216,  218,  553,  208,
 /*  1490 */    78,  133,  553,  134,  228,  210,   79,  553,  135,  136,
 /*  1500 */    86,  137,  553,   87,   82,  252,  553,  237,  216,  218,
 /*  1510 */   208,   78,  133,  553,  134,  228,  210,   79,  553,  135,
 /*  1520 */   136,   86,  137,  553,   87,   82,  254,  553,  237,  216,
 /*  1530 */   218,  553,  208,   78,  133,  553,  134,  228,  210,   79,
 /*  1540 */   553,  135,  136,   86,  137,  553,   87,   82,  266,  553,
 /*  1550 */   237,  216,  218,  208,   78,  133,  553,  134,  228,  210,
 /*  1560 */    79,  553,  135,  136,   86,  137,  553,   87,   82,  267,
 /*  1570 */   553,  237,  216,  218,  553,  208,   78,  133,  553,  134,
 /*  1580 */   228,  210,   79,  553,  135,  136,   86,  137,  553,   87,
 /*  1590 */    82,  154,  553,  237,  216,  218,  208,   78,  133,  553,
 /*  1600 */   134,  228,  210,   79,  553,  135,  136,   86,  137,  553,
 /*  1610 */    87,   82,  156,  553,  237,  216,  218,  553,  208,   78,
 /*  1620 */   133,  553,  134,  228,  210,   79,  553,  135,  136,   86,
 /*  1630 */   137,  553,   87,   82,  277,  553,  237,  216,  218,  208,
 /*  1640 */    78,  133,  553,  134,  228,  210,   79,  553,  135,  136,
 /*  1650 */    86,  137,  553,   87,   82,  167,  553,  237,  216,  218,
 /*  1660 */   553,  208,   78,  133,  553,  134,  228,  210,   79,  553,
 /*  1670 */   135,  136,   86,  137,  553,   87,   82,  281,  553,  237,
 /*  1680 */   216,  218,  208,   78,  133,  553,  134,  228,  210,   79,
 /*  1690 */   553,  135,  136,   86,  137,  553,   87,   82,  291,  553,
 /*  1700 */   237,  216,  218,  553,  208,   78,  133,  553,  134,  228,
 /*  1710 */   210,   79,  553,  135,  136,   86,  137,  553,   87,   82,
 /*  1720 */   191,  553,  237,  216,  218,  208,   78,  133,  553,  134,
 /*  1730 */   228,  210,   79,  553,  135,  136,   86,  137,  553,   87,
 /*  1740 */    82,  553,  553,  237,  216,  218,  553,  208,   78,  133,
 /*  1750 */   138,  134,  228,  210,   79,  553,  135,  136,   86,  137,
 /*  1760 */    28,   87,   82,  553,  553,  237,  216,  218,  553,  219,
 /*  1770 */   553,  553,  111,  127,  553,  134,  228,  210,   79,   28,
 /*  1780 */   135,  136,   86,  137,   48,   87,   82,  553,  553,  237,
 /*  1790 */   216,  218,  553,  553,  553,  553,   60,   61,  553,  553,
 /*  1800 */   553,   62,   21,   48,  220,  221,  222,  223,  224,  225,
 /*  1810 */   226,  227,   20,  553,  553,   60,   61,  553,  553,  553,
 /*  1820 */    62,   21,  553,  220,  221,  222,  223,  224,  225,  226,
 /*  1830 */   227,   20,  111,  192,  553,  134,  228,  210,   79,  553,
 /*  1840 */   135,  136,   86,  137,  553,   87,   82,  553,  553,  237,
 /*  1850 */   216,  218,  553,  111,  193,  219,  134,  228,  210,   79,
 /*  1860 */   553,  135,  136,   86,  137,   28,   87,   82,  553,  553,
 /*  1870 */   237,  216,  218,  111,  194,  553,  134,  228,  210,   79,
 /*  1880 */   553,  135,  136,   86,  137,  553,   87,   82,  553,  553,
 /*  1890 */   237,  216,  218,  553,  553,  553,  553,  553,  553,  553,
 /*  1900 */   553,   60,   61,  553,  553,  553,   62,   21,  553,  220,
 /*  1910 */   221,  222,  223,  224,  225,  226,  227,   20,  111,  553,
 /*  1920 */   553,  128,  228,  210,   79,  553,  135,  136,   86,  137,
 /*  1930 */   553,   87,   82,  553,  111,  237,  216,  218,  209,  210,
 /*  1940 */    79,  553,  135,  136,   86,  137,  553,   87,   82,  553,
 /*  1950 */   111,  237,  216,  218,  211,  210,   79,  553,  135,  136,
 /*  1960 */    86,  137,  553,   87,   82,  553,  553,  237,  216,  218,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   85,   12,   12,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   15,   16,   25,   26,   27,  115,   21,
 /*    20 */    22,    3,   24,    5,  108,  109,  110,  111,  112,  113,
 /*    30 */   114,   86,  116,  117,  118,  119,   28,  121,  122,  123,
 /*    40 */   124,  113,  126,  127,   46,  120,  130,  131,  132,  121,
 /*    50 */   122,  123,  124,   11,  126,  127,   58,   59,  130,  131,
 /*    60 */   132,   63,   64,   21,   66,   67,   68,   69,   70,   71,
 /*    70 */    72,   73,   74,   81,   82,   83,   84,   85,  100,  101,
 /*    80 */   102,   89,  134,  105,  113,  140,   12,    1,   96,   97,
 /*    90 */   119,    5,  121,  122,  123,  124,   78,  126,  127,   25,
 /*   100 */    26,  130,  131,  132,  112,  113,  114,   87,  116,  117,
 /*   110 */   118,  119,  129,  121,  122,  123,  124,   74,  126,  127,
 /*   120 */    77,    1,  130,  131,  132,    5,  113,   82,   83,   84,
 /*   130 */    85,  109,   88,  111,   89,  122,  123,  124,   94,  126,
 /*   140 */   127,   96,   97,  130,  131,  132,   95,  128,   88,   98,
 /*   150 */    99,  100,  101,  102,   94,   87,  105,  112,  113,  114,
 /*   160 */   140,  116,  117,  118,  119,  113,  121,  122,  123,  124,
 /*   170 */     1,  126,  127,    4,  113,  130,  131,  132,  126,  127,
 /*   180 */    25,   26,  130,  131,  132,   17,  113,   82,   83,   84,
 /*   190 */    85,  130,  131,  132,   89,  113,  123,  124,   78,  126,
 /*   200 */   127,   96,   97,  130,  131,  132,  124,    1,  126,  127,
 /*   210 */     4,   12,  130,  131,  132,  101,  102,  112,  113,  114,
 /*   220 */   125,  116,  117,  118,  119,   26,  121,  122,  123,  124,
 /*   230 */    17,  126,  127,   19,  133,  130,  131,  132,  101,  102,
 /*   240 */    82,   83,   84,   85,   12,  137,   95,   89,   25,   98,
 /*   250 */    99,  100,  101,  102,   96,   97,  105,   25,   26,   27,
 /*   260 */    12,   47,   48,   49,   50,   51,   52,   19,  101,  102,
 /*   270 */   112,  113,  114,    1,  116,  117,  118,  119,   65,  121,
 /*   280 */   122,  123,  124,    1,  126,  127,  101,  102,  130,  131,
 /*   290 */   132,  113,  109,  110,  111,   47,   48,   49,   50,   51,
 /*   300 */    82,   83,   84,   85,    1,  127,    3,   89,  130,  131,
 /*   310 */   132,  110,   12,  110,   96,   97,   92,   93,   99,  100,
 /*   320 */   101,  102,   17,   20,  105,   25,   26,   27,  103,  104,
 /*   330 */   112,  113,  114,   64,  116,  117,  118,  119,  110,  121,
 /*   340 */   122,  123,  124,  139,  126,  127,  113,   75,  130,  131,
 /*   350 */   132,   55,   56,   82,   83,   84,   85,  113,    0,    1,
 /*   360 */    89,   79,  110,  130,  131,  132,   19,   96,   97,   58,
 /*   370 */    59,   26,   12,   90,  130,  131,  132,  100,  101,  102,
 /*   380 */    75,   91,  105,  112,  113,  114,   93,  116,  117,  118,
 /*   390 */   119,  113,  121,  122,  123,  124,    1,  126,  127,    4,
 /*   400 */    24,  130,  131,  132,  113,   60,   61,   62,  130,  131,
 /*   410 */   132,   97,   25,   82,   83,   84,   85,  102,  102,  102,
 /*   420 */    89,  130,  131,  132,  104,   18,  102,   96,   97,   22,
 /*   430 */     1,    1,   25,    4,    4,    1,  102,  102,    4,  102,
 /*   440 */   102,  139,    4,  112,  113,  114,    1,  116,  117,  118,
 /*   450 */   119,    1,  121,  122,  123,  124,   44,  126,  127,   45,
 /*   460 */    52,  130,  131,  132,   53,   29,   82,   83,   84,   85,
 /*   470 */    54,   64,   57,   89,   23,   75,   17,   76,   17,   17,
 /*   480 */    96,   97,   29,   17,   17,   17,   17,   23,    1,    1,
 /*   490 */     4,    1,   12,    1,   12,   17,  112,  113,  114,   18,
 /*   500 */   116,  117,  118,  119,    1,  121,  122,  123,  124,  141,
 /*   510 */   126,  127,   17,   12,  130,  131,  132,   12,   12,   23,
 /*   520 */    22,   17,   65,   12,   17,   17,   82,   83,   84,   85,
 /*   530 */    17,   12,   17,   89,   12,   17,    1,   17,   17,   65,
 /*   540 */    96,   97,   17,   17,   17,   17,   17,   17,   17,    4,
 /*   550 */    12,    1,  141,  141,  141,  141,  112,  113,  114,  141,
 /*   560 */   116,  117,  118,  119,  141,  121,  122,  123,  124,  141,
 /*   570 */   126,  127,  141,  141,  130,  131,  132,  141,  141,   82,
 /*   580 */    83,   84,   85,  141,  141,  141,   89,  141,  141,  141,
 /*   590 */   141,  141,  141,   96,   97,  141,  141,  141,  141,  141,
 /*   600 */   141,  141,  141,  141,  141,  141,  141,  141,  141,  112,
 /*   610 */   113,  114,  141,  116,  117,  118,  119,  141,  121,  122,
 /*   620 */   123,  124,  141,  126,  127,  141,  141,  130,  131,  132,
 /*   630 */   141,  141,  141,  141,  141,  141,  141,  141,  141,   82,
 /*   640 */    83,   84,   85,  141,  141,  141,   89,  141,  141,  141,
 /*   650 */   141,  141,  141,   96,   97,  141,  141,  141,  141,  141,
 /*   660 */   141,  141,  141,  141,  141,  141,  141,  141,  141,  112,
 /*   670 */   113,  114,  141,  116,  117,  118,  119,  141,  121,  122,
 /*   680 */   123,  124,  141,  126,  127,  141,  141,  130,  131,  132,
 /*   690 */   141,  141,   82,   83,   84,   85,  141,  141,  141,   89,
 /*   700 */   141,  141,  141,  141,  141,  141,   96,   97,  141,  141,
 /*   710 */   141,  141,  141,  141,  141,  141,  141,  141,  141,  141,
 /*   720 */   141,  141,  112,  113,  114,  141,  116,  117,  118,  119,
 /*   730 */   141,  121,  122,  123,  124,  141,  126,  127,  141,  141,
 /*   740 */   130,  131,  132,  141,  141,  141,  141,  141,  141,  141,
 /*   750 */   141,  141,   82,   83,   84,   85,  141,  141,  141,   89,
 /*   760 */   141,  141,  141,  141,  141,  141,   96,   97,  141,  141,
 /*   770 */   141,  141,  141,  141,  141,  141,  141,  141,  141,  141,
 /*   780 */   141,  141,  112,  113,  114,  141,  116,  117,  118,  119,
 /*   790 */   141,  121,  122,  123,  124,  141,  126,  127,  141,  141,
 /*   800 */   130,  131,  132,  141,  141,   82,   83,   84,   85,  141,
 /*   810 */   141,  141,   89,  141,  141,  141,  141,  141,  141,   96,
 /*   820 */    97,  141,  141,  141,  141,  141,  141,  141,  141,  141,
 /*   830 */   141,  141,  141,  141,  141,  112,  113,  114,  141,  116,
 /*   840 */   117,  118,  119,  141,  121,  122,  123,  124,  141,  126,
 /*   850 */   127,  141,  141,  130,  131,  132,  141,  141,  141,  141,
 /*   860 */   141,  141,  141,  141,  141,   82,   83,   84,   85,  141,
 /*   870 */   141,  141,   89,  141,  141,  141,  141,  141,  141,   96,
 /*   880 */    97,  141,  141,  141,  141,  141,  141,  141,  141,  141,
 /*   890 */   141,  141,  141,  141,  141,  112,  113,  114,  141,  116,
 /*   900 */   117,  118,  119,  141,  121,  122,  123,  124,  141,  126,
 /*   910 */   127,  141,  141,  130,  131,  132,  141,  141,   82,   83,
 /*   920 */    84,   85,  141,  141,  141,   89,  141,  141,  141,  141,
 /*   930 */   141,  141,   96,   97,  141,  141,  141,  141,  141,  141,
 /*   940 */   141,  141,  141,  141,  141,  141,  141,  141,  112,  113,
 /*   950 */   114,  141,  116,  117,  118,  119,  141,  121,  122,  123,
 /*   960 */   124,  141,  126,  127,  141,  141,  130,  131,  132,  141,
 /*   970 */   141,  141,  141,  141,  141,  141,  141,  141,   82,   83,
 /*   980 */    84,   85,  141,  141,  141,   89,  141,  141,  141,  141,
 /*   990 */   141,  141,   96,   97,  141,  141,  141,  141,  141,  141,
 /*  1000 */   141,  141,  141,  141,  141,  141,  141,  141,  112,  113,
 /*  1010 */   114,   85,  116,  117,  118,  119,  141,  121,  122,  123,
 /*  1020 */   124,  141,  126,  127,  141,  141,  130,  131,  132,  141,
 /*  1030 */   141,  141,  106,  107,  108,  109,  110,  111,  112,  113,
 /*  1040 */   114,  141,  116,  117,  118,  119,  141,  121,  122,  123,
 /*  1050 */   124,  141,  126,  127,  141,  141,  130,  131,  132,  141,
 /*  1060 */   141,  141,   83,   84,   85,  141,  141,  141,   89,  141,
 /*  1070 */   141,  141,  141,  141,  141,   96,   97,  141,  141,  141,
 /*  1080 */   141,  141,  141,  141,  141,  141,  141,  141,  141,  141,
 /*  1090 */   141,  112,  113,  114,   85,  116,  117,  118,  119,  141,
 /*  1100 */   121,  122,  123,  124,  141,  126,  127,  141,  141,  130,
 /*  1110 */   131,  132,  141,  141,  141,  141,  141,  141,  141,  141,
 /*  1120 */   141,  112,  113,  114,  141,  116,  117,  118,  119,  141,
 /*  1130 */   121,  122,  123,  124,  141,  126,  127,  141,   12,  130,
 /*  1140 */   131,  132,   18,  141,  135,  136,   22,  138,   22,   25,
 /*  1150 */   141,  141,   28,  141,   30,   31,   32,   33,   34,   35,
 /*  1160 */    36,   37,   38,   39,   40,   41,   42,   43,   12,  141,
 /*  1170 */   141,  141,   46,  141,  141,  141,  141,  141,   22,  141,
 /*  1180 */   141,   25,   26,  141,   58,   59,  141,  141,   64,   63,
 /*  1190 */    64,  141,   66,   67,   68,   69,   70,   71,   72,   73,
 /*  1200 */    74,   75,   46,  141,  141,  141,  141,   85,  141,  141,
 /*  1210 */   141,  141,  141,  141,   58,   59,  141,  141,  141,   63,
 /*  1220 */    64,  141,   66,   67,   68,   69,   70,   71,   72,   73,
 /*  1230 */    74,   85,  141,  141,  112,  113,  114,  141,  116,  117,
 /*  1240 */   118,  119,  141,  121,  122,  123,  124,  141,  126,  127,
 /*  1250 */   141,   12,  130,  131,  132,  141,  141,  135,  112,  113,
 /*  1260 */   114,   22,  116,  117,  118,  119,  141,  121,  122,  123,
 /*  1270 */   124,    1,  126,  127,  141,  141,  130,  131,  132,  141,
 /*  1280 */   141,  141,   12,  141,  138,   46,  141,  141,  141,  141,
 /*  1290 */   141,  141,   22,  141,  141,  141,  141,   58,   59,  141,
 /*  1300 */   141,  141,   63,   64,   65,   66,   67,   68,   69,   70,
 /*  1310 */    71,   72,   73,   74,  141,  141,   46,  141,  141,  141,
 /*  1320 */    85,  141,  141,  141,  141,  141,  141,  141,   58,   59,
 /*  1330 */   141,  141,  141,   63,   64,  141,   66,   67,   68,   69,
 /*  1340 */    70,   71,   72,   73,   74,   85,  141,  112,  113,  114,
 /*  1350 */   141,  116,  117,  118,  119,  141,  121,  122,  123,  124,
 /*  1360 */   141,  126,  127,  141,  141,  130,  131,  132,   85,  141,
 /*  1370 */   141,  141,  112,  113,  114,  141,  116,  117,  118,  119,
 /*  1380 */   141,  121,  122,  123,  124,  141,  126,  127,  141,   85,
 /*  1390 */   130,  131,  132,  141,  141,  112,  113,  114,  141,  116,
 /*  1400 */   117,  118,  119,  141,  121,  122,  123,  124,  141,  126,
 /*  1410 */   127,  141,  141,  130,  131,  132,  112,  113,  114,   85,
 /*  1420 */   116,  117,  118,  119,  141,  121,  122,  123,  124,  141,
 /*  1430 */   126,  127,  141,  141,  130,  131,  132,  141,  141,  141,
 /*  1440 */    85,  141,  141,  141,  141,  141,  112,  113,  114,  141,
 /*  1450 */   116,  117,  118,  119,  141,  121,  122,  123,  124,  141,
 /*  1460 */   126,  127,   85,  141,  130,  131,  132,  112,  113,  114,
 /*  1470 */   141,  116,  117,  118,  119,  141,  121,  122,  123,  124,
 /*  1480 */   141,  126,  127,   85,  141,  130,  131,  132,  141,  112,
 /*  1490 */   113,  114,  141,  116,  117,  118,  119,  141,  121,  122,
 /*  1500 */   123,  124,  141,  126,  127,   85,  141,  130,  131,  132,
 /*  1510 */   112,  113,  114,  141,  116,  117,  118,  119,  141,  121,
 /*  1520 */   122,  123,  124,  141,  126,  127,   85,  141,  130,  131,
 /*  1530 */   132,  141,  112,  113,  114,  141,  116,  117,  118,  119,
 /*  1540 */   141,  121,  122,  123,  124,  141,  126,  127,   85,  141,
 /*  1550 */   130,  131,  132,  112,  113,  114,  141,  116,  117,  118,
 /*  1560 */   119,  141,  121,  122,  123,  124,  141,  126,  127,   85,
 /*  1570 */   141,  130,  131,  132,  141,  112,  113,  114,  141,  116,
 /*  1580 */   117,  118,  119,  141,  121,  122,  123,  124,  141,  126,
 /*  1590 */   127,   85,  141,  130,  131,  132,  112,  113,  114,  141,
 /*  1600 */   116,  117,  118,  119,  141,  121,  122,  123,  124,  141,
 /*  1610 */   126,  127,   85,  141,  130,  131,  132,  141,  112,  113,
 /*  1620 */   114,  141,  116,  117,  118,  119,  141,  121,  122,  123,
 /*  1630 */   124,  141,  126,  127,   85,  141,  130,  131,  132,  112,
 /*  1640 */   113,  114,  141,  116,  117,  118,  119,  141,  121,  122,
 /*  1650 */   123,  124,  141,  126,  127,   85,  141,  130,  131,  132,
 /*  1660 */   141,  112,  113,  114,  141,  116,  117,  118,  119,  141,
 /*  1670 */   121,  122,  123,  124,  141,  126,  127,   85,  141,  130,
 /*  1680 */   131,  132,  112,  113,  114,  141,  116,  117,  118,  119,
 /*  1690 */   141,  121,  122,  123,  124,  141,  126,  127,   85,  141,
 /*  1700 */   130,  131,  132,  141,  112,  113,  114,  141,  116,  117,
 /*  1710 */   118,  119,  141,  121,  122,  123,  124,  141,  126,  127,
 /*  1720 */    85,  141,  130,  131,  132,  112,  113,  114,  141,  116,
 /*  1730 */   117,  118,  119,  141,  121,  122,  123,  124,  141,  126,
 /*  1740 */   127,  141,  141,  130,  131,  132,  141,  112,  113,  114,
 /*  1750 */    12,  116,  117,  118,  119,  141,  121,  122,  123,  124,
 /*  1760 */    22,  126,  127,  141,  141,  130,  131,  132,  141,   12,
 /*  1770 */   141,  141,  113,  114,  141,  116,  117,  118,  119,   22,
 /*  1780 */   121,  122,  123,  124,   46,  126,  127,  141,  141,  130,
 /*  1790 */   131,  132,  141,  141,  141,  141,   58,   59,  141,  141,
 /*  1800 */   141,   63,   64,   46,   66,   67,   68,   69,   70,   71,
 /*  1810 */    72,   73,   74,  141,  141,   58,   59,  141,  141,  141,
 /*  1820 */    63,   64,  141,   66,   67,   68,   69,   70,   71,   72,
 /*  1830 */    73,   74,  113,  114,  141,  116,  117,  118,  119,  141,
 /*  1840 */   121,  122,  123,  124,  141,  126,  127,  141,  141,  130,
 /*  1850 */   131,  132,  141,  113,  114,   12,  116,  117,  118,  119,
 /*  1860 */   141,  121,  122,  123,  124,   22,  126,  127,  141,  141,
 /*  1870 */   130,  131,  132,  113,  114,  141,  116,  117,  118,  119,
 /*  1880 */   141,  121,  122,  123,  124,  141,  126,  127,  141,  141,
 /*  1890 */   130,  131,  132,  141,  141,  141,  141,  141,  141,  141,
 /*  1900 */   141,   58,   59,  141,  141,  141,   63,   64,  141,   66,
 /*  1910 */    67,   68,   69,   70,   71,   72,   73,   74,  113,  141,
 /*  1920 */   141,  116,  117,  118,  119,  141,  121,  122,  123,  124,
 /*  1930 */   141,  126,  127,  141,  113,  130,  131,  132,  117,  118,
 /*  1940 */   119,  141,  121,  122,  123,  124,  141,  126,  127,  141,
 /*  1950 */   113,  130,  131,  132,  117,  118,  119,  141,  121,  122,
 /*  1960 */   123,  124,  141,  126,  127,  141,  141,  130,  131,  132,
};
#define YY_SHIFT_USE_DFLT (-11)
#define YY_SHIFT_MAX 200
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2, 1156,   -2, 1156,
 /*    20 */  1126, 1239, 1738, 1270, 1757, 1757, 1757, 1757, 1757, 1757,
 /*    30 */  1757, 1757, 1757, 1757, 1757, 1757, 1757, 1757, 1757, 1757,
 /*    40 */  1757, 1757, 1757, 1757, 1757, 1757, 1757, 1757, 1757, 1843,
 /*    50 */  1843, 1843, 1843, 1843,  -10,  -10, 1843, 1843,  232, 1843,
 /*    60 */  1843, 1843, 1843, 1843,  300,  300,   74,  120,   18,  303,
 /*    70 */   303,  199,  155,  155,  155,  155,   -9,    8, 1124,  214,
 /*    80 */   248,  345,  345,   86,  296,  311,  296,  311,   43,  168,
 /*    90 */   223,  223,  223,  223,  269,  347,  360,   -9,  376,  387,
 /*   100 */   387,    8,  387,  387,    8,  387,  387,  387,  387,    8,
 /*   110 */   269,  407,  358,  169,  213,  305,  206,   42,  395,  429,
 /*   120 */   430,  272,  282,  434,  438,  445,  450,  412,  414,  408,
 /*   130 */   411,  416,  415,  412,  414,  411,  416,  415,  436,  451,
 /*   140 */   400,  401,  401,  459,  461,  462,  453,  466,  453,  467,
 /*   150 */   468,  469,  464,  487,  488,  486,  490,  445,  480,  492,
 /*   160 */   482,  478,  501,  495,  481,  506,  481,  503,  505,  498,
 /*   170 */   496,  504,  507,  508,  513,  511,  519,  515,  522,  518,
 /*   180 */   520,  521,  525,  526,  527,  528,  529,  530,  531,  457,
 /*   190 */   535,  474,  412,  412,  412,  545,  538,  550,  445,  445,
 /*   200 */   445,
};
#define YY_REDUCE_USE_DFLT (-98)
#define YY_REDUCE_MAX 110
static const short yy_reduce_ofst[] = {
 /*     0 */    -8,   45,  105,  158,  218,  271,  331,  384,  444,  497,
 /*    10 */   557,  610,  670,  723,  783,  836,  896,  926,  979,  -84,
 /*    20 */  1009, 1122, 1146, 1235, 1260, 1283, 1304, 1334, 1355, 1377,
 /*    30 */  1398, 1420, 1441, 1463, 1484, 1506, 1527, 1549, 1570, 1592,
 /*    40 */  1613, 1635, 1659, 1719, 1740, 1760, 1805, 1821, 1837,  -29,
 /*    50 */   -72,   13,   73,   82,   51,  151,   52,  178,  219,   61,
 /*    60 */   233,  244,  278,  291,  -22,  277,  183,  -55,   20,   44,
 /*    70 */    60,   22,  114,  137,  167,  185,  224,  225,  -97,  -75,
 /*    80 */   -52,  -17,  -17,   68,   95,   19,   95,   19,  101,  108,
 /*    90 */   201,  203,  228,  252,  204,  283,  290,  293,  314,  315,
 /*   100 */   316,  320,  317,  324,  320,  334,  335,  337,  338,  320,
 /*   110 */   302,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   343,  343,  343,  343,  343,  343,  343,  343,  343,  343,
 /*    10 */   343,  343,  343,  343,  343,  343,  343,  423,  343,  551,
 /*    20 */   551,  551,  538,  551,  551,  350,  551,  551,  551,  551,
 /*    30 */   551,  551,  551,  352,  354,  551,  551,  551,  551,  551,
 /*    40 */   551,  551,  551,  551,  551,  551,  551,  551,  551,  551,
 /*    50 */   551,  551,  551,  551,  411,  411,  551,  551,  551,  551,
 /*    60 */   551,  551,  551,  551,  551,  551,  551,  551,  549,  372,
 /*    70 */   372,  551,  551,  551,  551,  551,  551,  415,  503,  469,
 /*    80 */   551,  490,  489,  549,  482,  488,  481,  487,  539,  537,
 /*    90 */   551,  551,  551,  551,  542,  368,  551,  551,  376,  551,
 /*   100 */   551,  551,  551,  551,  419,  551,  551,  551,  551,  418,
 /*   110 */   542,  503,  551,  551,  551,  551,  551,  551,  551,  551,
 /*   120 */   551,  551,  551,  551,  551,  550,  551,  446,  464,  470,
 /*   130 */   478,  480,  484,  450,  463,  477,  479,  483,  516,  551,
 /*   140 */   551,  551,  531,  424,  425,  426,  551,  428,  516,  431,
 /*   150 */   432,  435,  551,  551,  551,  551,  551,  373,  551,  551,
 /*   160 */   551,  359,  551,  360,  363,  551,  362,  551,  551,  551,
 /*   170 */   551,  395,  387,  383,  381,  551,  551,  385,  551,  391,
 /*   180 */   389,  393,  403,  399,  397,  401,  407,  405,  409,  551,
 /*   190 */   551,  551,  447,  448,  449,  551,  551,  551,  546,  547,
 /*   200 */   548,  342,  344,  345,  341,  346,  349,  351,  445,  466,
 /*   210 */   467,  468,  493,  499,  500,  501,  502,  504,  505,  516,
 /*   220 */   517,  518,  519,  520,  521,  522,  523,  524,  465,  485,
 /*   230 */   486,  491,  492,  495,  496,  497,  498,  494,  525,  530,
 /*   240 */   536,  527,  528,  534,  535,  529,  533,  532,  526,  531,
 /*   250 */   427,  438,  439,  443,  444,  429,  430,  441,  442,  433,
 /*   260 */   434,  436,  437,  440,  506,  540,  353,  355,  356,  370,
 /*   270 */   371,  357,  358,  367,  366,  365,  364,  361,  378,  379,
 /*   280 */   377,  369,  374,  380,  412,  382,  413,  384,  386,  414,
 /*   290 */   416,  417,  421,  422,  388,  390,  392,  394,  420,  396,
 /*   300 */   398,  400,  402,  404,  406,  408,  410,  543,  541,  507,
 /*   310 */   508,  509,  510,  511,  512,  513,  514,  515,  471,  472,
 /*   320 */   473,  474,  475,  476,  451,  452,  453,  454,  455,  456,
 /*   330 */   457,  458,  459,  460,  461,  462,  347,  545,  348,  544,
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
    TRACE("-------------------- dump of stack --------------------");
    int i;
    for (i = 0; i < PTR_AS(yyParser, parser)->yyidx; i++) {
        YogVal val = PTR_AS(yyParser, parser)->yystack[i + 1].minor.yy0;
        TRACE("PTR_AS(yyParser, parser)->yyidx=%d, i=%d, val=%p", PTR_AS(yyParser, parser)->yyidx, i, VAL2PTR(val));
#if 0
        YogVal_print(env, val);
#endif
    }
    TRACE("-------------------- end of stack --------------------");
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
  "RAISE",         "COMMA",         "DOT",           "GREATER",     
  "ELIF",          "DEF",           "LPAR",          "RPAR",        
  "AT",            "STAR_STAR",     "STAR",          "AMPER",       
  "EQUAL",         "COLON",         "AND_AND_EQUAL",  "BAR_BAR_EQUAL",
  "PLUS_EQUAL",    "MINUS_EQUAL",   "STAR_EQUAL",    "DIV_EQUAL",   
  "DIV_DIV_EQUAL",  "PERCENT_EQUAL",  "BAR_EQUAL",     "AND_EQUAL",   
  "XOR_EQUAL",     "STAR_STAR_EQUAL",  "LSHIFT_EQUAL",  "RSHIFT_EQUAL",
  "BAR_BAR",       "AND_AND",       "NOT",           "EQUAL_EQUAL", 
  "NOT_EQUAL",     "LESS",          "LESS_EQUAL",    "GREATER_EQUAL",
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
  "atom",          "blockarg_opt",  "name",          "exprs",       
  "dict_elems",    "comma_opt",     "dict_elem",     "blockarg_params_opt",
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
 /*  17 */ "stmt ::= decorators_opt CLASS NAME super_opt NEWLINE stmts END",
 /*  18 */ "stmt ::= MODULE NAME stmts END",
 /*  19 */ "stmt ::= NONLOCAL names",
 /*  20 */ "stmt ::= IMPORT dotted_names",
 /*  21 */ "stmt ::= RAISE expr",
 /*  22 */ "dotted_names ::= dotted_name",
 /*  23 */ "dotted_names ::= dotted_names COMMA dotted_name",
 /*  24 */ "dotted_name ::= NAME",
 /*  25 */ "dotted_name ::= dotted_name DOT NAME",
 /*  26 */ "names ::= NAME",
 /*  27 */ "names ::= names COMMA NAME",
 /*  28 */ "super_opt ::=",
 /*  29 */ "super_opt ::= GREATER expr",
 /*  30 */ "if_tail ::= else_opt",
 /*  31 */ "if_tail ::= ELIF expr NEWLINE stmts if_tail",
 /*  32 */ "else_opt ::=",
 /*  33 */ "else_opt ::= ELSE stmts",
 /*  34 */ "func_def ::= decorators_opt DEF NAME LPAR params RPAR stmts END",
 /*  35 */ "decorators_opt ::=",
 /*  36 */ "decorators_opt ::= decorators",
 /*  37 */ "decorators ::= decorator",
 /*  38 */ "decorators ::= decorators decorator",
 /*  39 */ "decorator ::= AT expr NEWLINE",
 /*  40 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  41 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param",
 /*  42 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param",
 /*  43 */ "params ::= params_without_default COMMA params_with_default COMMA block_param",
 /*  44 */ "params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param",
 /*  45 */ "params ::= params_without_default COMMA params_with_default COMMA var_param",
 /*  46 */ "params ::= params_without_default COMMA params_with_default COMMA kw_param",
 /*  47 */ "params ::= params_without_default COMMA params_with_default",
 /*  48 */ "params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  49 */ "params ::= params_without_default COMMA block_param COMMA var_param",
 /*  50 */ "params ::= params_without_default COMMA block_param COMMA kw_param",
 /*  51 */ "params ::= params_without_default COMMA block_param",
 /*  52 */ "params ::= params_without_default COMMA var_param COMMA kw_param",
 /*  53 */ "params ::= params_without_default COMMA var_param",
 /*  54 */ "params ::= params_without_default COMMA kw_param",
 /*  55 */ "params ::= params_without_default",
 /*  56 */ "params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  57 */ "params ::= params_with_default COMMA block_param COMMA var_param",
 /*  58 */ "params ::= params_with_default COMMA block_param COMMA kw_param",
 /*  59 */ "params ::= params_with_default COMMA block_param",
 /*  60 */ "params ::= params_with_default COMMA var_param COMMA kw_param",
 /*  61 */ "params ::= params_with_default COMMA var_param",
 /*  62 */ "params ::= params_with_default COMMA kw_param",
 /*  63 */ "params ::= params_with_default",
 /*  64 */ "params ::= block_param COMMA var_param COMMA kw_param",
 /*  65 */ "params ::= block_param COMMA var_param",
 /*  66 */ "params ::= block_param COMMA kw_param",
 /*  67 */ "params ::= block_param",
 /*  68 */ "params ::= var_param COMMA kw_param",
 /*  69 */ "params ::= var_param",
 /*  70 */ "params ::= kw_param",
 /*  71 */ "params ::=",
 /*  72 */ "kw_param ::= STAR_STAR NAME",
 /*  73 */ "var_param ::= STAR NAME",
 /*  74 */ "block_param ::= AMPER NAME param_default_opt",
 /*  75 */ "param_default_opt ::=",
 /*  76 */ "param_default_opt ::= param_default",
 /*  77 */ "param_default ::= EQUAL expr",
 /*  78 */ "params_without_default ::= NAME",
 /*  79 */ "params_without_default ::= params_without_default COMMA NAME",
 /*  80 */ "params_with_default ::= param_with_default",
 /*  81 */ "params_with_default ::= params_with_default COMMA param_with_default",
 /*  82 */ "param_with_default ::= NAME param_default",
 /*  83 */ "args ::=",
 /*  84 */ "args ::= posargs",
 /*  85 */ "args ::= posargs COMMA kwargs",
 /*  86 */ "args ::= posargs COMMA kwargs COMMA vararg",
 /*  87 */ "args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg",
 /*  88 */ "args ::= posargs COMMA vararg",
 /*  89 */ "args ::= posargs COMMA vararg COMMA varkwarg",
 /*  90 */ "args ::= posargs COMMA varkwarg",
 /*  91 */ "args ::= kwargs",
 /*  92 */ "args ::= kwargs COMMA vararg",
 /*  93 */ "args ::= kwargs COMMA vararg COMMA varkwarg",
 /*  94 */ "args ::= kwargs COMMA varkwarg",
 /*  95 */ "args ::= vararg",
 /*  96 */ "args ::= vararg COMMA varkwarg",
 /*  97 */ "args ::= varkwarg",
 /*  98 */ "varkwarg ::= STAR_STAR expr",
 /*  99 */ "vararg ::= STAR expr",
 /* 100 */ "posargs ::= expr",
 /* 101 */ "posargs ::= posargs COMMA expr",
 /* 102 */ "kwargs ::= kwarg",
 /* 103 */ "kwargs ::= kwargs COMMA kwarg",
 /* 104 */ "kwarg ::= NAME COLON expr",
 /* 105 */ "expr ::= assign_expr",
 /* 106 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /* 107 */ "assign_expr ::= postfix_expr augmented_assign_op logical_or_expr",
 /* 108 */ "assign_expr ::= postfix_expr AND_AND_EQUAL logical_or_expr",
 /* 109 */ "assign_expr ::= postfix_expr BAR_BAR_EQUAL logical_or_expr",
 /* 110 */ "assign_expr ::= logical_or_expr",
 /* 111 */ "augmented_assign_op ::= PLUS_EQUAL",
 /* 112 */ "augmented_assign_op ::= MINUS_EQUAL",
 /* 113 */ "augmented_assign_op ::= STAR_EQUAL",
 /* 114 */ "augmented_assign_op ::= DIV_EQUAL",
 /* 115 */ "augmented_assign_op ::= DIV_DIV_EQUAL",
 /* 116 */ "augmented_assign_op ::= PERCENT_EQUAL",
 /* 117 */ "augmented_assign_op ::= BAR_EQUAL",
 /* 118 */ "augmented_assign_op ::= AND_EQUAL",
 /* 119 */ "augmented_assign_op ::= XOR_EQUAL",
 /* 120 */ "augmented_assign_op ::= STAR_STAR_EQUAL",
 /* 121 */ "augmented_assign_op ::= LSHIFT_EQUAL",
 /* 122 */ "augmented_assign_op ::= RSHIFT_EQUAL",
 /* 123 */ "logical_or_expr ::= logical_and_expr",
 /* 124 */ "logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr",
 /* 125 */ "logical_and_expr ::= not_expr",
 /* 126 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /* 127 */ "not_expr ::= comparison",
 /* 128 */ "not_expr ::= NOT not_expr",
 /* 129 */ "comparison ::= xor_expr",
 /* 130 */ "comparison ::= xor_expr comp_op xor_expr",
 /* 131 */ "comp_op ::= EQUAL_EQUAL",
 /* 132 */ "comp_op ::= NOT_EQUAL",
 /* 133 */ "comp_op ::= LESS",
 /* 134 */ "comp_op ::= LESS_EQUAL",
 /* 135 */ "comp_op ::= GREATER",
 /* 136 */ "comp_op ::= GREATER_EQUAL",
 /* 137 */ "xor_expr ::= or_expr",
 /* 138 */ "xor_expr ::= xor_expr XOR or_expr",
 /* 139 */ "or_expr ::= and_expr",
 /* 140 */ "or_expr ::= or_expr BAR and_expr",
 /* 141 */ "and_expr ::= shift_expr",
 /* 142 */ "and_expr ::= and_expr AND shift_expr",
 /* 143 */ "shift_expr ::= match_expr",
 /* 144 */ "shift_expr ::= shift_expr shift_op match_expr",
 /* 145 */ "shift_op ::= LSHIFT",
 /* 146 */ "shift_op ::= RSHIFT",
 /* 147 */ "match_expr ::= arith_expr",
 /* 148 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 149 */ "arith_expr ::= term",
 /* 150 */ "arith_expr ::= arith_expr arith_op term",
 /* 151 */ "arith_op ::= PLUS",
 /* 152 */ "arith_op ::= MINUS",
 /* 153 */ "term ::= term term_op factor",
 /* 154 */ "term ::= factor",
 /* 155 */ "term_op ::= STAR",
 /* 156 */ "term_op ::= DIV",
 /* 157 */ "term_op ::= DIV_DIV",
 /* 158 */ "term_op ::= PERCENT",
 /* 159 */ "factor ::= PLUS factor",
 /* 160 */ "factor ::= MINUS factor",
 /* 161 */ "factor ::= TILDA factor",
 /* 162 */ "factor ::= power",
 /* 163 */ "power ::= postfix_expr",
 /* 164 */ "power ::= postfix_expr STAR_STAR factor",
 /* 165 */ "postfix_expr ::= atom",
 /* 166 */ "postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt",
 /* 167 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 168 */ "postfix_expr ::= postfix_expr DOT name",
 /* 169 */ "name ::= NAME",
 /* 170 */ "name ::= EQUAL_EQUAL",
 /* 171 */ "name ::= NOT_EQUAL",
 /* 172 */ "name ::= LESS",
 /* 173 */ "name ::= LESS_EQUAL",
 /* 174 */ "name ::= GREATER",
 /* 175 */ "name ::= GREATER_EQUAL",
 /* 176 */ "atom ::= NAME",
 /* 177 */ "atom ::= NUMBER",
 /* 178 */ "atom ::= REGEXP",
 /* 179 */ "atom ::= STRING",
 /* 180 */ "atom ::= SYMBOL",
 /* 181 */ "atom ::= NIL",
 /* 182 */ "atom ::= TRUE",
 /* 183 */ "atom ::= FALSE",
 /* 184 */ "atom ::= LINE",
 /* 185 */ "atom ::= LBRACKET exprs RBRACKET",
 /* 186 */ "atom ::= LBRACKET RBRACKET",
 /* 187 */ "atom ::= LBRACE RBRACE",
 /* 188 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 189 */ "atom ::= LBRACE exprs RBRACE",
 /* 190 */ "atom ::= LPAR expr RPAR",
 /* 191 */ "exprs ::= expr",
 /* 192 */ "exprs ::= exprs COMMA expr",
 /* 193 */ "dict_elems ::= dict_elem",
 /* 194 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 195 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 196 */ "dict_elem ::= NAME COLON expr",
 /* 197 */ "comma_opt ::=",
 /* 198 */ "comma_opt ::= COMMA",
 /* 199 */ "blockarg_opt ::=",
 /* 200 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 201 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 202 */ "blockarg_params_opt ::=",
 /* 203 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 204 */ "excepts ::= except",
 /* 205 */ "excepts ::= excepts except",
 /* 206 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 207 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 208 */ "except ::= EXCEPT NEWLINE stmts",
 /* 209 */ "finally_opt ::=",
 /* 210 */ "finally_opt ::= FINALLY stmts",
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
  { 81, 1 },
  { 82, 1 },
  { 82, 3 },
  { 83, 0 },
  { 83, 1 },
  { 83, 1 },
  { 83, 7 },
  { 83, 5 },
  { 83, 5 },
  { 83, 5 },
  { 83, 1 },
  { 83, 2 },
  { 83, 1 },
  { 83, 2 },
  { 83, 1 },
  { 83, 2 },
  { 83, 6 },
  { 83, 7 },
  { 83, 4 },
  { 83, 2 },
  { 83, 2 },
  { 83, 2 },
  { 92, 1 },
  { 92, 3 },
  { 93, 1 },
  { 93, 3 },
  { 91, 1 },
  { 91, 3 },
  { 90, 0 },
  { 90, 2 },
  { 88, 1 },
  { 88, 5 },
  { 94, 0 },
  { 94, 2 },
  { 84, 8 },
  { 89, 0 },
  { 89, 1 },
  { 96, 1 },
  { 96, 2 },
  { 97, 3 },
  { 95, 9 },
  { 95, 7 },
  { 95, 7 },
  { 95, 5 },
  { 95, 7 },
  { 95, 5 },
  { 95, 5 },
  { 95, 3 },
  { 95, 7 },
  { 95, 5 },
  { 95, 5 },
  { 95, 3 },
  { 95, 5 },
  { 95, 3 },
  { 95, 3 },
  { 95, 1 },
  { 95, 7 },
  { 95, 5 },
  { 95, 5 },
  { 95, 3 },
  { 95, 5 },
  { 95, 3 },
  { 95, 3 },
  { 95, 1 },
  { 95, 5 },
  { 95, 3 },
  { 95, 3 },
  { 95, 1 },
  { 95, 3 },
  { 95, 1 },
  { 95, 1 },
  { 95, 0 },
  { 102, 2 },
  { 101, 2 },
  { 100, 3 },
  { 103, 0 },
  { 103, 1 },
  { 104, 2 },
  { 98, 1 },
  { 98, 3 },
  { 99, 1 },
  { 99, 3 },
  { 105, 2 },
  { 106, 0 },
  { 106, 1 },
  { 106, 3 },
  { 106, 5 },
  { 106, 7 },
  { 106, 3 },
  { 106, 5 },
  { 106, 3 },
  { 106, 1 },
  { 106, 3 },
  { 106, 5 },
  { 106, 3 },
  { 106, 1 },
  { 106, 3 },
  { 106, 1 },
  { 110, 2 },
  { 109, 2 },
  { 107, 1 },
  { 107, 3 },
  { 108, 1 },
  { 108, 3 },
  { 111, 3 },
  { 85, 1 },
  { 112, 3 },
  { 112, 3 },
  { 112, 3 },
  { 112, 3 },
  { 112, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 114, 1 },
  { 114, 3 },
  { 116, 1 },
  { 116, 3 },
  { 117, 1 },
  { 117, 2 },
  { 118, 1 },
  { 118, 3 },
  { 120, 1 },
  { 120, 1 },
  { 120, 1 },
  { 120, 1 },
  { 120, 1 },
  { 120, 1 },
  { 119, 1 },
  { 119, 3 },
  { 121, 1 },
  { 121, 3 },
  { 122, 1 },
  { 122, 3 },
  { 123, 1 },
  { 123, 3 },
  { 125, 1 },
  { 125, 1 },
  { 124, 1 },
  { 124, 3 },
  { 126, 1 },
  { 126, 3 },
  { 128, 1 },
  { 128, 1 },
  { 127, 3 },
  { 127, 1 },
  { 129, 1 },
  { 129, 1 },
  { 129, 1 },
  { 129, 1 },
  { 130, 2 },
  { 130, 2 },
  { 130, 2 },
  { 130, 1 },
  { 131, 1 },
  { 131, 3 },
  { 113, 1 },
  { 113, 5 },
  { 113, 4 },
  { 113, 3 },
  { 134, 1 },
  { 134, 1 },
  { 134, 1 },
  { 134, 1 },
  { 134, 1 },
  { 134, 1 },
  { 134, 1 },
  { 132, 1 },
  { 132, 1 },
  { 132, 1 },
  { 132, 1 },
  { 132, 1 },
  { 132, 1 },
  { 132, 1 },
  { 132, 1 },
  { 132, 1 },
  { 132, 3 },
  { 132, 2 },
  { 132, 2 },
  { 132, 4 },
  { 132, 3 },
  { 132, 3 },
  { 135, 1 },
  { 135, 3 },
  { 136, 1 },
  { 136, 3 },
  { 138, 3 },
  { 138, 3 },
  { 137, 0 },
  { 137, 1 },
  { 133, 0 },
  { 133, 5 },
  { 133, 5 },
  { 139, 0 },
  { 139, 3 },
  { 86, 1 },
  { 86, 2 },
  { 140, 6 },
  { 140, 4 },
  { 140, 3 },
  { 87, 0 },
  { 87, 2 },
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
#line 778 "parser.y"
{
    *pval = yymsp[0].minor.yy229;
}
#line 2354 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 22: /* dotted_names ::= dotted_name */
      case 37: /* decorators ::= decorator */
      case 80: /* params_with_default ::= param_with_default */
      case 100: /* posargs ::= expr */
      case 102: /* kwargs ::= kwarg */
      case 191: /* exprs ::= expr */
      case 193: /* dict_elems ::= dict_elem */
      case 204: /* excepts ::= except */
#line 782 "parser.y"
{
    yygotominor.yy229 = make_array_with(env, yymsp[0].minor.yy229);
}
#line 2369 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 23: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 81: /* params_with_default ::= params_with_default COMMA param_with_default */
#line 785 "parser.y"
{
    yygotominor.yy229 = Array_push(env, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2378 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 28: /* super_opt ::= */
      case 32: /* else_opt ::= */
      case 35: /* decorators_opt ::= */
      case 75: /* param_default_opt ::= */
      case 83: /* args ::= */
      case 197: /* comma_opt ::= */
      case 199: /* blockarg_opt ::= */
      case 202: /* blockarg_params_opt ::= */
      case 209: /* finally_opt ::= */
#line 789 "parser.y"
{
    yygotominor.yy229 = YNIL;
}
#line 2394 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 5: /* stmt ::= expr */
      case 29: /* super_opt ::= GREATER expr */
      case 30: /* if_tail ::= else_opt */
      case 33: /* else_opt ::= ELSE stmts */
      case 36: /* decorators_opt ::= decorators */
      case 76: /* param_default_opt ::= param_default */
      case 77: /* param_default ::= EQUAL expr */
      case 98: /* varkwarg ::= STAR_STAR expr */
      case 99: /* vararg ::= STAR expr */
      case 105: /* expr ::= assign_expr */
      case 110: /* assign_expr ::= logical_or_expr */
      case 123: /* logical_or_expr ::= logical_and_expr */
      case 125: /* logical_and_expr ::= not_expr */
      case 127: /* not_expr ::= comparison */
      case 129: /* comparison ::= xor_expr */
      case 137: /* xor_expr ::= or_expr */
      case 139: /* or_expr ::= and_expr */
      case 141: /* and_expr ::= shift_expr */
      case 143: /* shift_expr ::= match_expr */
      case 147: /* match_expr ::= arith_expr */
      case 149: /* arith_expr ::= term */
      case 154: /* term ::= factor */
      case 162: /* factor ::= power */
      case 163: /* power ::= postfix_expr */
      case 165: /* postfix_expr ::= atom */
      case 210: /* finally_opt ::= FINALLY stmts */
#line 792 "parser.y"
{
    yygotominor.yy229 = yymsp[0].minor.yy229;
}
#line 2427 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 798 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy229 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy229, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[-1].minor.yy229);
}
#line 2435 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 802 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy229 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy229, yymsp[-2].minor.yy229, YNIL, yymsp[-1].minor.yy229);
}
#line 2443 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 806 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy229 = Finally_new(env, lineno, yymsp[-3].minor.yy229, yymsp[-1].minor.yy229);
}
#line 2451 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 810 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy229 = While_new(env, lineno, yymsp[-3].minor.yy229, yymsp[-1].minor.yy229);
}
#line 2459 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 814 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy229 = Break_new(env, lineno, YNIL);
}
#line 2467 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 818 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy229 = Break_new(env, lineno, yymsp[0].minor.yy229);
}
#line 2475 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 822 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy229 = Next_new(env, lineno, YNIL);
}
#line 2483 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 826 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy229 = Next_new(env, lineno, yymsp[0].minor.yy229);
}
#line 2491 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 830 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy229 = Return_new(env, lineno, YNIL);
}
#line 2499 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 834 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy229 = Return_new(env, lineno, yymsp[0].minor.yy229);
}
#line 2507 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 838 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy229 = If_new(env, lineno, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[-1].minor.yy229);
}
#line 2515 "parser.c"
        break;
      case 17: /* stmt ::= decorators_opt CLASS NAME super_opt NEWLINE stmts END */
#line 842 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy229 = Class_new(env, lineno, yymsp[-6].minor.yy229, id, yymsp[-3].minor.yy229, yymsp[-1].minor.yy229);
}
#line 2524 "parser.c"
        break;
      case 18: /* stmt ::= MODULE NAME stmts END */
#line 847 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    yygotominor.yy229 = Module_new(env, lineno, id, yymsp[-1].minor.yy229);
}
#line 2533 "parser.c"
        break;
      case 19: /* stmt ::= NONLOCAL names */
#line 852 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy229 = Nonlocal_new(env, lineno, yymsp[0].minor.yy229);
}
#line 2541 "parser.c"
        break;
      case 20: /* stmt ::= IMPORT dotted_names */
#line 856 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy229 = Import_new(env, lineno, yymsp[0].minor.yy229);
}
#line 2549 "parser.c"
        break;
      case 21: /* stmt ::= RAISE expr */
#line 860 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy229 = Raise_new(env, lineno, yymsp[0].minor.yy229);
}
#line 2557 "parser.c"
        break;
      case 24: /* dotted_name ::= NAME */
      case 26: /* names ::= NAME */
#line 872 "parser.y"
{
    yygotominor.yy229 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2565 "parser.c"
        break;
      case 25: /* dotted_name ::= dotted_name DOT NAME */
      case 27: /* names ::= names COMMA NAME */
#line 875 "parser.y"
{
    yygotominor.yy229 = Array_push_token_id(env, yymsp[-2].minor.yy229, yymsp[0].minor.yy0);
}
#line 2573 "parser.c"
        break;
      case 31: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 896 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy229, yymsp[-1].minor.yy229, yymsp[0].minor.yy229);
    yygotominor.yy229 = make_array_with(env, node);
}
#line 2582 "parser.c"
        break;
      case 34: /* func_def ::= decorators_opt DEF NAME LPAR params RPAR stmts END */
#line 909 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy229 = FuncDef_new(env, lineno, yymsp[-7].minor.yy229, id, yymsp[-3].minor.yy229, yymsp[-1].minor.yy229);
}
#line 2591 "parser.c"
        break;
      case 38: /* decorators ::= decorators decorator */
      case 205: /* excepts ::= excepts except */
#line 925 "parser.y"
{
    yygotominor.yy229 = Array_push(env, yymsp[-1].minor.yy229, yymsp[0].minor.yy229);
}
#line 2599 "parser.c"
        break;
      case 39: /* decorator ::= AT expr NEWLINE */
      case 190: /* atom ::= LPAR expr RPAR */
      case 203: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 929 "parser.y"
{
    yygotominor.yy229 = yymsp[-1].minor.yy229;
}
#line 2608 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 933 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-8].minor.yy229, yymsp[-6].minor.yy229, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2615 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 936 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-6].minor.yy229, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229, YNIL);
}
#line 2622 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 939 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-6].minor.yy229, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, YNIL, yymsp[0].minor.yy229);
}
#line 2629 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 942 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229, YNIL, YNIL);
}
#line 2636 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 945 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-6].minor.yy229, yymsp[-4].minor.yy229, YNIL, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2643 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 948 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, YNIL, yymsp[0].minor.yy229, YNIL);
}
#line 2650 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 951 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, YNIL, YNIL, yymsp[0].minor.yy229);
}
#line 2657 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA params_with_default */
#line 954 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-2].minor.yy229, yymsp[0].minor.yy229, YNIL, YNIL, YNIL);
}
#line 2664 "parser.c"
        break;
      case 48: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 957 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-6].minor.yy229, YNIL, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2671 "parser.c"
        break;
      case 49: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 960 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-4].minor.yy229, YNIL, yymsp[-2].minor.yy229, yymsp[0].minor.yy229, YNIL);
}
#line 2678 "parser.c"
        break;
      case 50: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 963 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-4].minor.yy229, YNIL, yymsp[-2].minor.yy229, YNIL, yymsp[0].minor.yy229);
}
#line 2685 "parser.c"
        break;
      case 51: /* params ::= params_without_default COMMA block_param */
#line 966 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-2].minor.yy229, YNIL, yymsp[0].minor.yy229, YNIL, YNIL);
}
#line 2692 "parser.c"
        break;
      case 52: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 969 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-4].minor.yy229, YNIL, YNIL, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2699 "parser.c"
        break;
      case 53: /* params ::= params_without_default COMMA var_param */
#line 972 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-2].minor.yy229, YNIL, YNIL, yymsp[0].minor.yy229, YNIL);
}
#line 2706 "parser.c"
        break;
      case 54: /* params ::= params_without_default COMMA kw_param */
#line 975 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[-2].minor.yy229, YNIL, YNIL, YNIL, yymsp[0].minor.yy229);
}
#line 2713 "parser.c"
        break;
      case 55: /* params ::= params_without_default */
#line 978 "parser.y"
{
    yygotominor.yy229 = Params_new(env, yymsp[0].minor.yy229, YNIL, YNIL, YNIL, YNIL);
}
#line 2720 "parser.c"
        break;
      case 56: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 981 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, yymsp[-6].minor.yy229, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2727 "parser.c"
        break;
      case 57: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 984 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229, YNIL);
}
#line 2734 "parser.c"
        break;
      case 58: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 987 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, YNIL, yymsp[0].minor.yy229);
}
#line 2741 "parser.c"
        break;
      case 59: /* params ::= params_with_default COMMA block_param */
#line 990 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, yymsp[-2].minor.yy229, yymsp[0].minor.yy229, YNIL, YNIL);
}
#line 2748 "parser.c"
        break;
      case 60: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 993 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, yymsp[-4].minor.yy229, YNIL, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2755 "parser.c"
        break;
      case 61: /* params ::= params_with_default COMMA var_param */
#line 996 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, yymsp[-2].minor.yy229, YNIL, yymsp[0].minor.yy229, YNIL);
}
#line 2762 "parser.c"
        break;
      case 62: /* params ::= params_with_default COMMA kw_param */
#line 999 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, yymsp[-2].minor.yy229, YNIL, YNIL, yymsp[0].minor.yy229);
}
#line 2769 "parser.c"
        break;
      case 63: /* params ::= params_with_default */
#line 1002 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, yymsp[0].minor.yy229, YNIL, YNIL, YNIL);
}
#line 2776 "parser.c"
        break;
      case 64: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 1005 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2783 "parser.c"
        break;
      case 65: /* params ::= block_param COMMA var_param */
#line 1008 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy229, yymsp[0].minor.yy229, YNIL);
}
#line 2790 "parser.c"
        break;
      case 66: /* params ::= block_param COMMA kw_param */
#line 1011 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy229, YNIL, yymsp[0].minor.yy229);
}
#line 2797 "parser.c"
        break;
      case 67: /* params ::= block_param */
#line 1014 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy229, YNIL, YNIL);
}
#line 2804 "parser.c"
        break;
      case 68: /* params ::= var_param COMMA kw_param */
#line 1017 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2811 "parser.c"
        break;
      case 69: /* params ::= var_param */
#line 1020 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy229, YNIL);
}
#line 2818 "parser.c"
        break;
      case 70: /* params ::= kw_param */
#line 1023 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy229);
}
#line 2825 "parser.c"
        break;
      case 71: /* params ::= */
#line 1026 "parser.y"
{
    yygotominor.yy229 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2832 "parser.c"
        break;
      case 72: /* kw_param ::= STAR_STAR NAME */
#line 1030 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy229 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2841 "parser.c"
        break;
      case 73: /* var_param ::= STAR NAME */
#line 1036 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy229 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2850 "parser.c"
        break;
      case 74: /* block_param ::= AMPER NAME param_default_opt */
#line 1042 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy229 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy229);
}
#line 2859 "parser.c"
        break;
      case 78: /* params_without_default ::= NAME */
#line 1059 "parser.y"
{
    yygotominor.yy229 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy229, lineno, id, YNIL);
}
#line 2869 "parser.c"
        break;
      case 79: /* params_without_default ::= params_without_default COMMA NAME */
#line 1065 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy229, lineno, id, YNIL);
    yygotominor.yy229 = yymsp[-2].minor.yy229;
}
#line 2879 "parser.c"
        break;
      case 82: /* param_with_default ::= NAME param_default */
#line 1079 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy229 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy229);
}
#line 2888 "parser.c"
        break;
      case 84: /* args ::= posargs */
#line 1088 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, yymsp[0].minor.yy229, YNIL, YNIL, YNIL);
}
#line 2896 "parser.c"
        break;
      case 85: /* args ::= posargs COMMA kwargs */
#line 1092 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, yymsp[-2].minor.yy229, yymsp[0].minor.yy229, YNIL, YNIL);
}
#line 2904 "parser.c"
        break;
      case 86: /* args ::= posargs COMMA kwargs COMMA vararg */
#line 1096 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229, YNIL);
}
#line 2912 "parser.c"
        break;
      case 87: /* args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg */
#line 1100 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-6].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, yymsp[-6].minor.yy229, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2920 "parser.c"
        break;
      case 88: /* args ::= posargs COMMA vararg */
#line 1104 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, yymsp[-2].minor.yy229, YNIL, yymsp[0].minor.yy229, YNIL);
}
#line 2928 "parser.c"
        break;
      case 89: /* args ::= posargs COMMA vararg COMMA varkwarg */
#line 1108 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, yymsp[-4].minor.yy229, YNIL, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2936 "parser.c"
        break;
      case 90: /* args ::= posargs COMMA varkwarg */
#line 1112 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, yymsp[-2].minor.yy229, YNIL, YNIL, yymsp[0].minor.yy229);
}
#line 2944 "parser.c"
        break;
      case 91: /* args ::= kwargs */
#line 1116 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, YNIL, yymsp[0].minor.yy229, YNIL, YNIL);
}
#line 2952 "parser.c"
        break;
      case 92: /* args ::= kwargs COMMA vararg */
#line 1120 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy229, yymsp[0].minor.yy229, YNIL);
}
#line 2960 "parser.c"
        break;
      case 93: /* args ::= kwargs COMMA vararg COMMA varkwarg */
#line 1124 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, YNIL, yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2968 "parser.c"
        break;
      case 94: /* args ::= kwargs COMMA varkwarg */
#line 1128 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy229, 0));
    yygotominor.yy229 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy229, YNIL, yymsp[0].minor.yy229);
}
#line 2976 "parser.c"
        break;
      case 95: /* args ::= vararg */
#line 1132 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy229);
    yygotominor.yy229 = Args_new(env, lineno, YNIL, YNIL, yymsp[0].minor.yy229, YNIL);
}
#line 2984 "parser.c"
        break;
      case 96: /* args ::= vararg COMMA varkwarg */
#line 1136 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy229);
    yygotominor.yy229 = Args_new(env, lineno, YNIL, YNIL, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 2992 "parser.c"
        break;
      case 97: /* args ::= varkwarg */
#line 1140 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy229);
    yygotominor.yy229 = Args_new(env, lineno, YNIL, YNIL, YNIL, yymsp[0].minor.yy229);
}
#line 3000 "parser.c"
        break;
      case 101: /* posargs ::= posargs COMMA expr */
      case 103: /* kwargs ::= kwargs COMMA kwarg */
      case 192: /* exprs ::= exprs COMMA expr */
      case 194: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 1156 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
    yygotominor.yy229 = yymsp[-2].minor.yy229;
}
#line 3011 "parser.c"
        break;
      case 104: /* kwarg ::= NAME COLON expr */
#line 1169 "parser.y"
{
    yygotominor.yy229 = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(yymsp[-2].minor.yy0));
    PTR_AS(YogNode, yygotominor.yy229)->u.kwarg.name = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    PTR_AS(YogNode, yygotominor.yy229)->u.kwarg.value = yymsp[0].minor.yy229;
}
#line 3020 "parser.c"
        break;
      case 106: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 1179 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy229);
    yygotominor.yy229 = Assign_new(env, lineno, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 3028 "parser.c"
        break;
      case 107: /* assign_expr ::= postfix_expr augmented_assign_op logical_or_expr */
#line 1183 "parser.y"
{
    yygotominor.yy229 = AugmentedAssign_new(env, NODE_LINENO(yymsp[-2].minor.yy229), yymsp[-2].minor.yy229, VAL2ID(yymsp[-1].minor.yy229), yymsp[0].minor.yy229);
}
#line 3035 "parser.c"
        break;
      case 108: /* assign_expr ::= postfix_expr AND_AND_EQUAL logical_or_expr */
#line 1186 "parser.y"
{
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy229);
    expr = LogicalAnd_new(env, lineno, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
    assign = Assign_new(env, lineno, yymsp[-2].minor.yy229, expr);

    POP_LOCALS(env);

    yygotominor.yy229 = assign;
}
#line 3052 "parser.c"
        break;
      case 109: /* assign_expr ::= postfix_expr BAR_BAR_EQUAL logical_or_expr */
#line 1199 "parser.y"
{
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy229);
    expr = LogicalOr_new(env, lineno, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
    assign = Assign_new(env, lineno, yymsp[-2].minor.yy229, expr);

    POP_LOCALS(env);

    yygotominor.yy229 = assign;
}
#line 3069 "parser.c"
        break;
      case 111: /* augmented_assign_op ::= PLUS_EQUAL */
#line 1216 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "+"));
}
#line 3076 "parser.c"
        break;
      case 112: /* augmented_assign_op ::= MINUS_EQUAL */
#line 1219 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "-"));
}
#line 3083 "parser.c"
        break;
      case 113: /* augmented_assign_op ::= STAR_EQUAL */
#line 1222 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "*"));
}
#line 3090 "parser.c"
        break;
      case 114: /* augmented_assign_op ::= DIV_EQUAL */
#line 1225 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "/"));
}
#line 3097 "parser.c"
        break;
      case 115: /* augmented_assign_op ::= DIV_DIV_EQUAL */
#line 1228 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "//"));
}
#line 3104 "parser.c"
        break;
      case 116: /* augmented_assign_op ::= PERCENT_EQUAL */
#line 1231 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "%"));
}
#line 3111 "parser.c"
        break;
      case 117: /* augmented_assign_op ::= BAR_EQUAL */
#line 1234 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "|"));
}
#line 3118 "parser.c"
        break;
      case 118: /* augmented_assign_op ::= AND_EQUAL */
#line 1237 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "&"));
}
#line 3125 "parser.c"
        break;
      case 119: /* augmented_assign_op ::= XOR_EQUAL */
#line 1240 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "^"));
}
#line 3132 "parser.c"
        break;
      case 120: /* augmented_assign_op ::= STAR_STAR_EQUAL */
#line 1243 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "**"));
}
#line 3139 "parser.c"
        break;
      case 121: /* augmented_assign_op ::= LSHIFT_EQUAL */
#line 1246 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "<<"));
}
#line 3146 "parser.c"
        break;
      case 122: /* augmented_assign_op ::= RSHIFT_EQUAL */
#line 1249 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, ">>"));
}
#line 3153 "parser.c"
        break;
      case 124: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 1256 "parser.y"
{
    yygotominor.yy229 = LogicalOr_new(env, NODE_LINENO(yymsp[-2].minor.yy229), yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 3160 "parser.c"
        break;
      case 126: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 1263 "parser.y"
{
    yygotominor.yy229 = LogicalAnd_new(env, NODE_LINENO(yymsp[-2].minor.yy229), yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 3167 "parser.c"
        break;
      case 128: /* not_expr ::= NOT not_expr */
#line 1270 "parser.y"
{
    yygotominor.yy229 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy229)->u.not.expr = yymsp[0].minor.yy229;
}
#line 3175 "parser.c"
        break;
      case 130: /* comparison ::= xor_expr comp_op xor_expr */
#line 1278 "parser.y"
{
    yygotominor.yy229 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy229), yymsp[-2].minor.yy229, VAL2ID(yymsp[-1].minor.yy229), yymsp[0].minor.yy229);
}
#line 3182 "parser.c"
        break;
      case 131: /* comp_op ::= EQUAL_EQUAL */
      case 170: /* name ::= EQUAL_EQUAL */
#line 1282 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "=="));
}
#line 3190 "parser.c"
        break;
      case 132: /* comp_op ::= NOT_EQUAL */
      case 171: /* name ::= NOT_EQUAL */
#line 1285 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "!="));
}
#line 3198 "parser.c"
        break;
      case 133: /* comp_op ::= LESS */
      case 172: /* name ::= LESS */
#line 1288 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "<"));
}
#line 3206 "parser.c"
        break;
      case 134: /* comp_op ::= LESS_EQUAL */
      case 173: /* name ::= LESS_EQUAL */
#line 1291 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, "<="));
}
#line 3214 "parser.c"
        break;
      case 135: /* comp_op ::= GREATER */
      case 174: /* name ::= GREATER */
#line 1294 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, ">"));
}
#line 3222 "parser.c"
        break;
      case 136: /* comp_op ::= GREATER_EQUAL */
      case 175: /* name ::= GREATER_EQUAL */
#line 1297 "parser.y"
{
    yygotominor.yy229 = ID2VAL(YogVM_intern(env, env->vm, ">="));
}
#line 3230 "parser.c"
        break;
      case 138: /* xor_expr ::= xor_expr XOR or_expr */
      case 140: /* or_expr ::= or_expr BAR and_expr */
      case 142: /* and_expr ::= and_expr AND shift_expr */
#line 1304 "parser.y"
{
    yygotominor.yy229 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy229), yymsp[-2].minor.yy229, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy229);
}
#line 3239 "parser.c"
        break;
      case 144: /* shift_expr ::= shift_expr shift_op match_expr */
      case 150: /* arith_expr ::= arith_expr arith_op term */
      case 153: /* term ::= term term_op factor */
#line 1325 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy229);
    yygotominor.yy229 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy229, VAL2ID(yymsp[-1].minor.yy229), yymsp[0].minor.yy229);
}
#line 3249 "parser.c"
        break;
      case 145: /* shift_op ::= LSHIFT */
      case 146: /* shift_op ::= RSHIFT */
      case 151: /* arith_op ::= PLUS */
      case 152: /* arith_op ::= MINUS */
      case 155: /* term_op ::= STAR */
      case 156: /* term_op ::= DIV */
      case 157: /* term_op ::= DIV_DIV */
      case 158: /* term_op ::= PERCENT */
      case 169: /* name ::= NAME */
#line 1330 "parser.y"
{
    yygotominor.yy229 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 3264 "parser.c"
        break;
      case 148: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1340 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy229);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy229 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy229, id, yymsp[0].minor.yy229);
}
#line 3273 "parser.c"
        break;
      case 159: /* factor ::= PLUS factor */
#line 1382 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy229 = FuncCall_new3(env, lineno, yymsp[0].minor.yy229, id);
}
#line 3282 "parser.c"
        break;
      case 160: /* factor ::= MINUS factor */
#line 1387 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy229 = FuncCall_new3(env, lineno, yymsp[0].minor.yy229, id);
}
#line 3291 "parser.c"
        break;
      case 161: /* factor ::= TILDA factor */
#line 1392 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy229 = FuncCall_new3(env, lineno, yymsp[0].minor.yy229, id);
}
#line 3300 "parser.c"
        break;
      case 164: /* power ::= postfix_expr STAR_STAR factor */
#line 1404 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy229);
    ID id = YogVM_intern(env, env->vm, "**");
    yygotominor.yy229 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy229, id, yymsp[0].minor.yy229);
}
#line 3309 "parser.c"
        break;
      case 166: /* postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt */
#line 1413 "parser.y"
{
    yygotominor.yy229 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy229), yymsp[-4].minor.yy229, yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 3316 "parser.c"
        break;
      case 167: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1416 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy229);
    yygotominor.yy229 = Subscript_new(env, lineno, yymsp[-3].minor.yy229, yymsp[-1].minor.yy229);
}
#line 3324 "parser.c"
        break;
      case 168: /* postfix_expr ::= postfix_expr DOT name */
#line 1420 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy229);
    yygotominor.yy229 = Attr_new(env, lineno, yymsp[-2].minor.yy229, VAL2ID(yymsp[0].minor.yy229));
}
#line 3332 "parser.c"
        break;
      case 176: /* atom ::= NAME */
#line 1447 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy229 = Variable_new(env, lineno, id);
}
#line 3341 "parser.c"
        break;
      case 177: /* atom ::= NUMBER */
      case 178: /* atom ::= REGEXP */
      case 179: /* atom ::= STRING */
      case 180: /* atom ::= SYMBOL */
#line 1452 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy229 = Literal_new(env, lineno, val);
}
#line 3353 "parser.c"
        break;
      case 181: /* atom ::= NIL */
#line 1472 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy229 = Literal_new(env, lineno, YNIL);
}
#line 3361 "parser.c"
        break;
      case 182: /* atom ::= TRUE */
#line 1476 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy229 = Literal_new(env, lineno, YTRUE);
}
#line 3369 "parser.c"
        break;
      case 183: /* atom ::= FALSE */
#line 1480 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy229 = Literal_new(env, lineno, YFALSE);
}
#line 3377 "parser.c"
        break;
      case 184: /* atom ::= LINE */
#line 1484 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy229 = Literal_new(env, lineno, val);
}
#line 3386 "parser.c"
        break;
      case 185: /* atom ::= LBRACKET exprs RBRACKET */
#line 1489 "parser.y"
{
    yygotominor.yy229 = Array_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy229);
}
#line 3393 "parser.c"
        break;
      case 186: /* atom ::= LBRACKET RBRACKET */
#line 1492 "parser.y"
{
    yygotominor.yy229 = Array_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3400 "parser.c"
        break;
      case 187: /* atom ::= LBRACE RBRACE */
#line 1495 "parser.y"
{
    yygotominor.yy229 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3407 "parser.c"
        break;
      case 188: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1498 "parser.y"
{
    yygotominor.yy229 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy229);
}
#line 3414 "parser.c"
        break;
      case 189: /* atom ::= LBRACE exprs RBRACE */
#line 1501 "parser.y"
{
    yygotominor.yy229 = Set_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy229);
}
#line 3421 "parser.c"
        break;
      case 195: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1523 "parser.y"
{
    yygotominor.yy229 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy229), yymsp[-2].minor.yy229, yymsp[0].minor.yy229);
}
#line 3428 "parser.c"
        break;
      case 196: /* dict_elem ::= NAME COLON expr */
#line 1526 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy229 = DictElem_new(env, lineno, var, yymsp[0].minor.yy229);
}
#line 3438 "parser.c"
        break;
      case 198: /* comma_opt ::= COMMA */
#line 1536 "parser.y"
{
    yygotominor.yy229 = yymsp[0].minor.yy0;
}
#line 3445 "parser.c"
        break;
      case 200: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 201: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1543 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy229 = BlockArg_new(env, lineno, yymsp[-3].minor.yy229, yymsp[-1].minor.yy229);
}
#line 3454 "parser.c"
        break;
      case 206: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1566 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy229 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy229, id, yymsp[0].minor.yy229);
}
#line 3464 "parser.c"
        break;
      case 207: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1572 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy229 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy229, NO_EXC_VAR, yymsp[0].minor.yy229);
}
#line 3472 "parser.c"
        break;
      case 208: /* except ::= EXCEPT NEWLINE stmts */
#line 1576 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy229 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy229);
}
#line 3480 "parser.c"
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
