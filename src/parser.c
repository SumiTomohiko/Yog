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

#define TOKEN(token)            PTR_AS(YogToken, (token))
#define TOKEN_ID(token)         TOKEN((token))->u.id
#define TOKEN_LINENO(token)     TOKEN((token))->lineno
#define NODE_LINENO(node)       PTR_AS(YogNode, (node))->lineno
#line 739 "parser.c"
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
#define YYNOCODE 134
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy191;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 323
#define YYNRULE 197
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
 /*     0 */     1,  190,  271,  105,   24,   25,   33,   34,   35,  357,
 /*    10 */   214,  156,   92,   73,  107,  171,  172,  174,  357,   28,
 /*    20 */   125,   37,  131,  132,   82,  133,  154,   83,   77,  107,
 /*    30 */    18,  232,  211,  213,   16,  164,  521,  108,  199,  197,
 /*    40 */   198,   45,   81,   77,  113,   39,  232,  211,  213,   57,
 /*    50 */    58,   94,  274,  320,   59,   21,   42,  215,  216,  217,
 /*    60 */   218,  219,  220,  221,  222,   20,   56,  203,   75,  129,
 /*    70 */   306,  130,  223,  205,   78,  120,  131,  132,   82,  133,
 /*    80 */   142,   83,   77,  171,  172,  232,  211,  213,   64,  199,
 /*    90 */   197,  198,   30,   31,  188,  113,  305,   47,   17,   65,
 /*   100 */    23,   60,   94,  274,   41,   46,  307,  308,  309,  310,
 /*   110 */   311,  312,  313,  314,  315,  316,  317,  318,  203,   75,
 /*   120 */   129,   50,  130,  223,  205,   78,   54,  131,  132,   82,
 /*   130 */   133,  107,   83,   77,   40,  259,  232,  211,  213,   79,
 /*   140 */   199,  197,  198,  170,  279,   76,  113,  107,  232,  211,
 /*   150 */   213,   14,  322,   94,  274,  126,  132,   82,  133,   18,
 /*   160 */    83,   77,  228,    3,  232,  211,  213,  176,  289,  203,
 /*   170 */    75,  129,   26,  130,  223,  205,   78,   22,  131,  132,
 /*   180 */    82,  133,  106,   83,   77,   90,  100,  232,  211,  213,
 /*   190 */   229,  230,  231,  107,  121,  199,  197,  198,  171,  172,
 /*   200 */   174,  113,  127,   82,  133,  136,   83,   77,   94,  274,
 /*   210 */   232,  211,  213,  166,  233,  151,  167,  178,  182,  184,
 /*   220 */   300,  264,  191,  292,  203,   75,  129,   30,  130,  223,
 /*   230 */   205,   78,  107,  131,  132,   82,  133,  107,   83,   77,
 /*   240 */   180,  294,  232,  211,  213,  109,  199,  197,  198,  207,
 /*   250 */   211,  213,  113,  107,  208,  211,  213,  183,  298,   94,
 /*   260 */   274,  142,  265,   80,  133,  245,   83,   77,  264,  250,
 /*   270 */   232,  211,  213,  149,   31,  203,   75,  129,  254,  130,
 /*   280 */   223,  205,   78,  256,  131,  132,   82,  133,   52,   83,
 /*   290 */    77,  283,  284,  232,  211,  213,  146,  255,  248,   38,
 /*   300 */   112,  199,  197,  198,  159,  162,  185,  113,  107,  167,
 /*   310 */   178,  182,  184,  300,   94,  274,  292,   18,  141,  128,
 /*   320 */   248,   83,   77,  224,  225,  232,  211,  213,  323,   18,
 /*   330 */   203,   75,  129,  155,  130,  223,  205,   78,  107,  131,
 /*   340 */   132,   82,  133,  107,   83,   77,  226,  227,  232,  211,
 /*   350 */   213,   66,  199,  197,  198,  209,  211,  213,  113,  107,
 /*   360 */   210,  211,  213,  269,   18,   94,  274,  201,  157,  168,
 /*   370 */   175,  177,  291,  160,  171,  292,  212,  211,  213,   37,
 /*   380 */   272,  203,   75,  129,  302,  130,  223,  205,   78,  277,
 /*   390 */   131,  132,   82,  133,   18,   83,   77,  260,   97,  232,
 /*   400 */   211,  213,   18,  281,  287,  266,   67,  199,  197,  198,
 /*   410 */   171,  172,  174,  113,  288,    2,  290,    3,  293,   18,
 /*   420 */    94,  274,  267,   26,  295,  169,  173,  282,  188,  297,
 /*   430 */   286,  299,   17,  186,  200,   60,  203,   75,  129,   18,
 /*   440 */   130,  223,  205,   78,    4,  131,  132,   82,  133,   18,
 /*   450 */    83,   77,  276,   47,  232,  211,  213,  153,  199,  197,
 /*   460 */   198,   18,   43,    8,  113,  179,  181,  296,   40,   44,
 /*   470 */   286,   94,  274,   18,   48,  240,  321,   27,   49,   36,
 /*   480 */   234,  237,   53,   23,   29,   19,   68,  203,   75,  129,
 /*   490 */    86,  130,  223,  205,   78,   32,  131,  132,   82,  133,
 /*   500 */    87,   83,   77,   63,   84,  232,  211,  213,   88,   89,
 /*   510 */     5,    6,  114,  199,  197,  198,  263,    7,   91,  113,
 /*   520 */     9,  158,  273,   10,   93,  268,   94,  274,   51,  270,
 /*   530 */   165,  161,   12,  301,  303,   13,   55,   11,   61,   69,
 /*   540 */    95,  278,  203,   75,  129,  304,  130,  223,  205,   78,
 /*   550 */   280,  131,  132,   82,  133,   96,   83,   77,   70,   74,
 /*   560 */   232,  211,  213,  115,  199,  197,  198,   98,   99,   62,
 /*   570 */   113,   71,  101,  102,   72,  103,  104,   94,  274,  319,
 /*   580 */   192,  522,  522,  522,  522,  522,  522,  522,  522,  522,
 /*   590 */   522,  522,  522,  203,   75,  129,  522,  130,  223,  205,
 /*   600 */    78,  522,  131,  132,   82,  133,  522,   83,   77,  522,
 /*   610 */   522,  232,  211,  213,  522,  522,  522,  522,  116,  199,
 /*   620 */   197,  198,  522,  522,  522,  113,  522,  522,  522,  522,
 /*   630 */   522,  522,   94,  274,  522,  522,  522,  522,  522,  522,
 /*   640 */   522,  522,  522,  522,  522,  522,  522,  522,  203,   75,
 /*   650 */   129,  522,  130,  223,  205,   78,  522,  131,  132,   82,
 /*   660 */   133,  522,   83,   77,  522,  522,  232,  211,  213,  117,
 /*   670 */   199,  197,  198,  522,  522,  522,  113,  522,  522,  522,
 /*   680 */   522,  522,  522,   94,  274,  522,  522,  522,  522,  522,
 /*   690 */   522,  522,  522,  522,  522,  522,  522,  522,  522,  203,
 /*   700 */    75,  129,  522,  130,  223,  205,   78,  522,  131,  132,
 /*   710 */    82,  133,  522,   83,   77,  522,  522,  232,  211,  213,
 /*   720 */   522,  522,  522,  522,  193,  199,  197,  198,  522,  522,
 /*   730 */   522,  113,  522,  522,  522,  522,  522,  522,   94,  274,
 /*   740 */   522,  522,  522,  522,  522,  522,  522,  522,  522,  522,
 /*   750 */   522,  522,  522,  522,  203,   75,  129,  522,  130,  223,
 /*   760 */   205,   78,  522,  131,  132,   82,  133,  522,   83,   77,
 /*   770 */   522,  522,  232,  211,  213,  194,  199,  197,  198,  522,
 /*   780 */   522,  522,  113,  522,  522,  522,  522,  522,  522,   94,
 /*   790 */   274,  522,  522,  522,  522,  522,  522,  522,  522,  522,
 /*   800 */   522,  522,  522,  522,  522,  203,   75,  129,  522,  130,
 /*   810 */   223,  205,   78,  522,  131,  132,   82,  133,  522,   83,
 /*   820 */    77,  522,  522,  232,  211,  213,  522,  522,  522,  522,
 /*   830 */   195,  199,  197,  198,  522,  522,  522,  113,  522,  522,
 /*   840 */   522,  522,  522,  522,   94,  274,  522,  522,  522,  522,
 /*   850 */   522,  522,  522,  522,  522,  522,  522,  522,  522,  522,
 /*   860 */   203,   75,  129,  522,  130,  223,  205,   78,  522,  131,
 /*   870 */   132,   82,  133,  522,   83,   77,  522,  522,  232,  211,
 /*   880 */   213,  119,  199,  197,  198,  522,  522,  522,  113,  522,
 /*   890 */   522,  522,  522,  522,  522,   94,  274,  522,  522,  522,
 /*   900 */   522,  522,  522,  522,  522,  522,  522,  522,  522,  522,
 /*   910 */   522,  203,   75,  129,  522,  130,  223,  205,   78,  522,
 /*   920 */   131,  132,   82,  133,  258,   83,   77,  522,  522,  232,
 /*   930 */   211,  213,  522,  522,  522,  522,  522,  522,  522,  522,
 /*   940 */   522,  522,  522,  522,  522,  148,  139,  145,  147,  257,
 /*   950 */   253,  203,   75,  129,  522,  130,  223,  205,   78,  522,
 /*   960 */   131,  132,   82,  133,  522,   83,   77,  522,  522,  232,
 /*   970 */   211,  213,  196,  197,  198,  522,  522,  522,  113,  522,
 /*   980 */   522,  522,  522,  522,  522,   94,  274,  522,  522,  522,
 /*   990 */   522,  522,  522,  522,  522,  522,  522,  522,  522,  252,
 /*  1000 */   522,  203,   75,  129,  522,  130,  223,  205,   78,  522,
 /*  1010 */   131,  132,   82,  133,  522,   83,   77,  522,  522,  232,
 /*  1020 */   211,  213,  140,  143,  251,  253,  203,   75,  129,  138,
 /*  1030 */   130,  223,  205,   78,  522,  131,  132,   82,  133,  522,
 /*  1040 */    83,   77,  522,  522,  232,  211,  213,  522,  522,  522,
 /*  1050 */   522,  522,  522,  522,  522,  522,  203,   75,  129,  522,
 /*  1060 */   130,  223,  205,   78,  522,  131,  132,   82,  133,  522,
 /*  1070 */    83,   77,  522,  144,  232,  211,  213,  522,  111,   85,
 /*  1080 */   522,  241,   28,  522,  522,   30,   31,  522,  522,  522,
 /*  1090 */   522,  522,  522,  522,  522,  522,  522,  522,  522,  522,
 /*  1100 */   522,  522,  244,  522,   45,  522,  522,  522,  522,  522,
 /*  1110 */   522,  522,   57,   58,  522,  522,  522,   59,   21,  522,
 /*  1120 */   215,  216,  217,  218,  219,  220,  221,  222,   20,  203,
 /*  1130 */    75,  129,  522,  130,  223,  205,   78,  137,  131,  132,
 /*  1140 */    82,  133,  522,   83,   77,  522,  134,  232,  211,  213,
 /*  1150 */   522,  110,  522,  522,  522,   28,  522,  522,  522,  522,
 /*  1160 */   522,  522,  522,  522,  203,   75,  129,  522,  130,  223,
 /*  1170 */   205,   78,  522,  131,  132,   82,  133,   45,   83,   77,
 /*  1180 */   522,  522,  232,  211,  213,   57,   58,  522,  522,  238,
 /*  1190 */    59,   21,  522,  215,  216,  217,  218,  219,  220,  221,
 /*  1200 */   222,   20,  236,  214,  522,  522,  522,  522,  522,  522,
 /*  1210 */   522,  522,   28,  522,  522,  522,  522,  522,  522,  107,
 /*  1220 */   123,  522,  130,  223,  205,   78,  522,  131,  132,   82,
 /*  1230 */   133,  522,   83,   77,   45,  522,  232,  211,  213,  522,
 /*  1240 */   522,  522,   57,   58,  118,  522,  522,   59,   21,  243,
 /*  1250 */   215,  216,  217,  218,  219,  220,  221,  222,   20,   15,
 /*  1260 */   522,  522,  522,  522,  522,  522,  522,  522,  522,  522,
 /*  1270 */   214,  203,   75,  129,  522,  130,  223,  205,   78,   28,
 /*  1280 */   131,  132,   82,  133,  522,   83,   77,  522,  522,  232,
 /*  1290 */   211,  213,  522,  522,  522,  522,  522,  522,  522,  522,
 /*  1300 */   522,   45,  522,  522,  522,  522,  522,  522,  522,   57,
 /*  1310 */    58,  522,  522,  122,   59,   21,  522,  215,  216,  217,
 /*  1320 */   218,  219,  220,  221,  222,   20,  522,  522,  522,  522,
 /*  1330 */   522,  522,  522,  522,  202,  522,  522,  522,  522,  522,
 /*  1340 */   203,   75,  129,  522,  130,  223,  205,   78,  522,  131,
 /*  1350 */   132,   82,  133,  522,   83,   77,  242,  522,  232,  211,
 /*  1360 */   213,  203,   75,  129,  522,  130,  223,  205,   78,  522,
 /*  1370 */   131,  132,   82,  133,  522,   83,   77,  522,  522,  232,
 /*  1380 */   211,  213,  522,  203,   75,  129,  522,  130,  223,  205,
 /*  1390 */    78,  522,  131,  132,   82,  133,  235,   83,   77,  522,
 /*  1400 */   522,  232,  211,  213,  522,  522,  522,  522,  522,  522,
 /*  1410 */   522,  522,  522,  522,  522,  522,  522,  522,  522,  135,
 /*  1420 */   522,  522,  522,  203,   75,  129,  522,  130,  223,  205,
 /*  1430 */    78,  522,  131,  132,   82,  133,  522,   83,   77,  522,
 /*  1440 */   239,  232,  211,  213,  522,  522,  203,   75,  129,  522,
 /*  1450 */   130,  223,  205,   78,  522,  131,  132,   82,  133,  522,
 /*  1460 */    83,   77,  246,  522,  232,  211,  213,  203,   75,  129,
 /*  1470 */   522,  130,  223,  205,   78,  522,  131,  132,   82,  133,
 /*  1480 */   522,   83,   77,  522,  522,  232,  211,  213,  522,  203,
 /*  1490 */    75,  129,  522,  130,  223,  205,   78,  522,  131,  132,
 /*  1500 */    82,  133,  247,   83,   77,  522,  522,  232,  211,  213,
 /*  1510 */   522,  522,  522,  522,  522,  522,  522,  522,  522,  522,
 /*  1520 */   522,  522,  522,  522,  522,  249,  522,  522,  522,  203,
 /*  1530 */    75,  129,  522,  130,  223,  205,   78,  522,  131,  132,
 /*  1540 */    82,  133,  522,   83,   77,  522,  261,  232,  211,  213,
 /*  1550 */   522,  522,  203,   75,  129,  522,  130,  223,  205,   78,
 /*  1560 */   522,  131,  132,   82,  133,  522,   83,   77,  262,  522,
 /*  1570 */   232,  211,  213,  203,   75,  129,  522,  130,  223,  205,
 /*  1580 */    78,  522,  131,  132,   82,  133,  522,   83,   77,  522,
 /*  1590 */   522,  232,  211,  213,  522,  203,   75,  129,  522,  130,
 /*  1600 */   223,  205,   78,  522,  131,  132,   82,  133,  150,   83,
 /*  1610 */    77,  522,  522,  232,  211,  213,  522,  522,  522,  522,
 /*  1620 */   522,  522,  522,  522,  522,  522,  522,  522,  522,  522,
 /*  1630 */   522,  152,  522,  522,  522,  203,   75,  129,  522,  130,
 /*  1640 */   223,  205,   78,  522,  131,  132,   82,  133,  522,   83,
 /*  1650 */    77,  522,  163,  232,  211,  213,  522,  522,  203,   75,
 /*  1660 */   129,  522,  130,  223,  205,   78,  522,  131,  132,   82,
 /*  1670 */   133,  522,   83,   77,  275,  522,  232,  211,  213,  203,
 /*  1680 */    75,  129,  522,  130,  223,  205,   78,  522,  131,  132,
 /*  1690 */    82,  133,  522,   83,   77,  522,  522,  232,  211,  213,
 /*  1700 */   522,  203,   75,  129,  522,  130,  223,  205,   78,  522,
 /*  1710 */   131,  132,   82,  133,  285,   83,   77,  522,  522,  232,
 /*  1720 */   211,  213,  522,  522,  522,  522,  522,  522,  522,  522,
 /*  1730 */   522,  522,  522,  522,  522,  522,  522,  187,  522,  522,
 /*  1740 */   522,  203,   75,  129,  522,  130,  223,  205,   78,  522,
 /*  1750 */   131,  132,   82,  133,  134,   83,   77,  522,  522,  232,
 /*  1760 */   211,  213,  522,   28,  203,   75,  129,  522,  130,  223,
 /*  1770 */   205,   78,  522,  131,  132,   82,  133,  522,   83,   77,
 /*  1780 */   522,  214,  232,  211,  213,   45,  522,  522,  522,  522,
 /*  1790 */    28,  522,  522,   57,   58,  522,  522,  522,   59,   21,
 /*  1800 */   522,  215,  216,  217,  218,  219,  220,  221,  222,   20,
 /*  1810 */   522,  522,   45,  522,  522,  522,  522,  522,  522,  522,
 /*  1820 */    57,   58,  522,  522,  522,   59,   21,  522,  215,  216,
 /*  1830 */   217,  218,  219,  220,  221,  222,   20,  522,  522,  522,
 /*  1840 */   522,  522,  214,  522,  107,  189,  522,  130,  223,  205,
 /*  1850 */    78,   28,  131,  132,   82,  133,  522,   83,   77,  522,
 /*  1860 */   522,  232,  211,  213,  522,  522,  107,  522,  522,  124,
 /*  1870 */   223,  205,   78,  522,  131,  132,   82,  133,  522,   83,
 /*  1880 */    77,   57,   58,  232,  211,  213,   59,   21,  522,  215,
 /*  1890 */   216,  217,  218,  219,  220,  221,  222,   20,  522,  522,
 /*  1900 */   107,  522,  522,  522,  204,  205,   78,  522,  131,  132,
 /*  1910 */    82,  133,  522,   83,   77,  522,  107,  232,  211,  213,
 /*  1920 */   206,  205,   78,  522,  131,  132,   82,  133,  522,   83,
 /*  1930 */    77,  522,  522,  232,  211,  213,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   80,   12,   12,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   15,  106,   24,   25,   26,   20,   21,
 /*    20 */   112,   23,  114,  115,  116,  117,   11,  119,  120,  106,
 /*    30 */     1,  123,  124,  125,    5,   20,   74,   75,   76,   77,
 /*    40 */    78,   43,  119,  120,   82,   27,  123,  124,  125,   51,
 /*    50 */    52,   89,   90,  132,   56,   57,  108,   59,   60,   61,
 /*    60 */    62,   63,   64,   65,   66,   67,  122,  105,  106,  107,
 /*    70 */    18,  109,  110,  111,  112,   80,  114,  115,  116,  117,
 /*    80 */    12,  119,  120,   24,   25,  123,  124,  125,   75,   76,
 /*    90 */    77,   78,   24,   25,   17,   82,   44,   45,   21,   79,
 /*   100 */    71,   24,   89,   90,   27,  113,   29,   30,   31,   32,
 /*   110 */    33,   34,   35,   36,   37,   38,   39,   40,  105,  106,
 /*   120 */   107,  118,  109,  110,  111,  112,  121,  114,  115,  116,
 /*   130 */   117,  106,  119,  120,   57,  126,  123,  124,  125,   75,
 /*   140 */    76,   77,   78,   94,   95,  120,   82,  106,  123,  124,
 /*   150 */   125,    1,  132,   89,   90,  114,  115,  116,  117,    1,
 /*   160 */   119,  120,   25,    5,  123,  124,  125,   94,   95,  105,
 /*   170 */   106,  107,   16,  109,  110,  111,  112,   16,  114,  115,
 /*   180 */   116,  117,   67,  119,  120,   70,   12,  123,  124,  125,
 /*   190 */    53,   54,   55,  106,   75,   76,   77,   78,   24,   25,
 /*   200 */    26,   82,  115,  116,  117,  129,  119,  120,   89,   90,
 /*   210 */   123,  124,  125,   88,   58,   81,   91,   92,   93,   94,
 /*   220 */    95,   87,   72,   98,  105,  106,  107,   24,  109,  110,
 /*   230 */   111,  112,  106,  114,  115,  116,  117,  106,  119,  120,
 /*   240 */    94,   95,  123,  124,  125,   75,   76,   77,   78,  123,
 /*   250 */   124,  125,   82,  106,  123,  124,  125,   94,   95,   89,
 /*   260 */    90,   12,   81,  116,  117,  103,  119,  120,   87,  103,
 /*   270 */   123,  124,  125,  131,   25,  105,  106,  107,  103,  109,
 /*   280 */   110,  111,  112,  103,  114,  115,  116,  117,   57,  119,
 /*   290 */   120,   96,   97,  123,  124,  125,  102,  103,  104,   18,
 /*   300 */    75,   76,   77,   78,   85,   86,   88,   82,  106,   91,
 /*   310 */    92,   93,   94,   95,   89,   90,   98,    1,  102,  117,
 /*   320 */   104,  119,  120,   48,   49,  123,  124,  125,    0,    1,
 /*   330 */   105,  106,  107,   83,  109,  110,  111,  112,  106,  114,
 /*   340 */   115,  116,  117,  106,  119,  120,   51,   52,  123,  124,
 /*   350 */   125,   75,   76,   77,   78,  123,  124,  125,   82,  106,
 /*   360 */   123,  124,  125,   12,    1,   89,   90,    4,   84,   92,
 /*   370 */    93,   94,   95,   86,   24,   98,  123,  124,  125,   23,
 /*   380 */    90,  105,  106,  107,   68,  109,  110,  111,  112,   95,
 /*   390 */   114,  115,  116,  117,    1,  119,  120,    4,   12,  123,
 /*   400 */   124,  125,    1,   95,   97,    4,   75,   76,   77,   78,
 /*   410 */    24,   25,   26,   82,   95,    3,   95,    5,   95,    1,
 /*   420 */    89,   90,    4,   16,   95,   93,   94,   95,   17,   95,
 /*   430 */    98,   95,   21,  131,    4,   24,  105,  106,  107,    1,
 /*   440 */   109,  110,  111,  112,    1,  114,  115,  116,  117,    1,
 /*   450 */   119,  120,    4,   45,  123,  124,  125,   75,   76,   77,
 /*   460 */    78,    1,   41,    3,   82,   93,   94,   95,   57,   42,
 /*   470 */    98,   89,   90,    1,   46,   68,    4,   28,   47,   19,
 /*   480 */    22,   68,   50,   71,   69,   16,   16,  105,  106,  107,
 /*   490 */    16,  109,  110,  111,  112,   28,  114,  115,  116,  117,
 /*   500 */    16,  119,  120,   16,   22,  123,  124,  125,   16,   16,
 /*   510 */     1,    1,   75,   76,   77,   78,    4,    1,   12,   82,
 /*   520 */     1,   16,    1,   12,   16,   12,   89,   90,   21,   12,
 /*   530 */    12,   17,    1,   58,   58,    1,   16,   22,   16,   16,
 /*   540 */    16,   12,  105,  106,  107,   12,  109,  110,  111,  112,
 /*   550 */    12,  114,  115,  116,  117,   16,  119,  120,   16,   12,
 /*   560 */   123,  124,  125,   75,   76,   77,   78,   16,   16,   16,
 /*   570 */    82,   16,   16,   16,   16,   16,   16,   89,   90,    4,
 /*   580 */    12,  133,  133,  133,  133,  133,  133,  133,  133,  133,
 /*   590 */   133,  133,  133,  105,  106,  107,  133,  109,  110,  111,
 /*   600 */   112,  133,  114,  115,  116,  117,  133,  119,  120,  133,
 /*   610 */   133,  123,  124,  125,  133,  133,  133,  133,   75,   76,
 /*   620 */    77,   78,  133,  133,  133,   82,  133,  133,  133,  133,
 /*   630 */   133,  133,   89,   90,  133,  133,  133,  133,  133,  133,
 /*   640 */   133,  133,  133,  133,  133,  133,  133,  133,  105,  106,
 /*   650 */   107,  133,  109,  110,  111,  112,  133,  114,  115,  116,
 /*   660 */   117,  133,  119,  120,  133,  133,  123,  124,  125,   75,
 /*   670 */    76,   77,   78,  133,  133,  133,   82,  133,  133,  133,
 /*   680 */   133,  133,  133,   89,   90,  133,  133,  133,  133,  133,
 /*   690 */   133,  133,  133,  133,  133,  133,  133,  133,  133,  105,
 /*   700 */   106,  107,  133,  109,  110,  111,  112,  133,  114,  115,
 /*   710 */   116,  117,  133,  119,  120,  133,  133,  123,  124,  125,
 /*   720 */   133,  133,  133,  133,   75,   76,   77,   78,  133,  133,
 /*   730 */   133,   82,  133,  133,  133,  133,  133,  133,   89,   90,
 /*   740 */   133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
 /*   750 */   133,  133,  133,  133,  105,  106,  107,  133,  109,  110,
 /*   760 */   111,  112,  133,  114,  115,  116,  117,  133,  119,  120,
 /*   770 */   133,  133,  123,  124,  125,   75,   76,   77,   78,  133,
 /*   780 */   133,  133,   82,  133,  133,  133,  133,  133,  133,   89,
 /*   790 */    90,  133,  133,  133,  133,  133,  133,  133,  133,  133,
 /*   800 */   133,  133,  133,  133,  133,  105,  106,  107,  133,  109,
 /*   810 */   110,  111,  112,  133,  114,  115,  116,  117,  133,  119,
 /*   820 */   120,  133,  133,  123,  124,  125,  133,  133,  133,  133,
 /*   830 */    75,   76,   77,   78,  133,  133,  133,   82,  133,  133,
 /*   840 */   133,  133,  133,  133,   89,   90,  133,  133,  133,  133,
 /*   850 */   133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
 /*   860 */   105,  106,  107,  133,  109,  110,  111,  112,  133,  114,
 /*   870 */   115,  116,  117,  133,  119,  120,  133,  133,  123,  124,
 /*   880 */   125,   75,   76,   77,   78,  133,  133,  133,   82,  133,
 /*   890 */   133,  133,  133,  133,  133,   89,   90,  133,  133,  133,
 /*   900 */   133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
 /*   910 */   133,  105,  106,  107,  133,  109,  110,  111,  112,  133,
 /*   920 */   114,  115,  116,  117,   78,  119,  120,  133,  133,  123,
 /*   930 */   124,  125,  133,  133,  133,  133,  133,  133,  133,  133,
 /*   940 */   133,  133,  133,  133,  133,   99,  100,  101,  102,  103,
 /*   950 */   104,  105,  106,  107,  133,  109,  110,  111,  112,  133,
 /*   960 */   114,  115,  116,  117,  133,  119,  120,  133,  133,  123,
 /*   970 */   124,  125,   76,   77,   78,  133,  133,  133,   82,  133,
 /*   980 */   133,  133,  133,  133,  133,   89,   90,  133,  133,  133,
 /*   990 */   133,  133,  133,  133,  133,  133,  133,  133,  133,   78,
 /*  1000 */   133,  105,  106,  107,  133,  109,  110,  111,  112,  133,
 /*  1010 */   114,  115,  116,  117,  133,  119,  120,  133,  133,  123,
 /*  1020 */   124,  125,  101,  102,  103,  104,  105,  106,  107,   78,
 /*  1030 */   109,  110,  111,  112,  133,  114,  115,  116,  117,  133,
 /*  1040 */   119,  120,  133,  133,  123,  124,  125,  133,  133,  133,
 /*  1050 */   133,  133,  133,  133,  133,  133,  105,  106,  107,  133,
 /*  1060 */   109,  110,  111,  112,  133,  114,  115,  116,  117,  133,
 /*  1070 */   119,  120,  133,   12,  123,  124,  125,  133,  127,  128,
 /*  1080 */   133,  130,   21,  133,  133,   24,   25,  133,  133,  133,
 /*  1090 */   133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
 /*  1100 */   133,  133,   78,  133,   43,  133,  133,  133,  133,  133,
 /*  1110 */   133,  133,   51,   52,  133,  133,  133,   56,   57,  133,
 /*  1120 */    59,   60,   61,   62,   63,   64,   65,   66,   67,  105,
 /*  1130 */   106,  107,  133,  109,  110,  111,  112,   78,  114,  115,
 /*  1140 */   116,  117,  133,  119,  120,  133,   12,  123,  124,  125,
 /*  1150 */   133,  127,  133,  133,  133,   21,  133,  133,  133,  133,
 /*  1160 */   133,  133,  133,  133,  105,  106,  107,  133,  109,  110,
 /*  1170 */   111,  112,  133,  114,  115,  116,  117,   43,  119,  120,
 /*  1180 */   133,  133,  123,  124,  125,   51,   52,  133,  133,  130,
 /*  1190 */    56,   57,  133,   59,   60,   61,   62,   63,   64,   65,
 /*  1200 */    66,   67,   68,   12,  133,  133,  133,  133,  133,  133,
 /*  1210 */   133,  133,   21,  133,  133,  133,  133,  133,  133,  106,
 /*  1220 */   107,  133,  109,  110,  111,  112,  133,  114,  115,  116,
 /*  1230 */   117,  133,  119,  120,   43,  133,  123,  124,  125,  133,
 /*  1240 */   133,  133,   51,   52,   78,  133,  133,   56,   57,   58,
 /*  1250 */    59,   60,   61,   62,   63,   64,   65,   66,   67,    1,
 /*  1260 */   133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
 /*  1270 */    12,  105,  106,  107,  133,  109,  110,  111,  112,   21,
 /*  1280 */   114,  115,  116,  117,  133,  119,  120,  133,  133,  123,
 /*  1290 */   124,  125,  133,  133,  133,  133,  133,  133,  133,  133,
 /*  1300 */   133,   43,  133,  133,  133,  133,  133,  133,  133,   51,
 /*  1310 */    52,  133,  133,   78,   56,   57,  133,   59,   60,   61,
 /*  1320 */    62,   63,   64,   65,   66,   67,  133,  133,  133,  133,
 /*  1330 */   133,  133,  133,  133,   78,  133,  133,  133,  133,  133,
 /*  1340 */   105,  106,  107,  133,  109,  110,  111,  112,  133,  114,
 /*  1350 */   115,  116,  117,  133,  119,  120,   78,  133,  123,  124,
 /*  1360 */   125,  105,  106,  107,  133,  109,  110,  111,  112,  133,
 /*  1370 */   114,  115,  116,  117,  133,  119,  120,  133,  133,  123,
 /*  1380 */   124,  125,  133,  105,  106,  107,  133,  109,  110,  111,
 /*  1390 */   112,  133,  114,  115,  116,  117,   78,  119,  120,  133,
 /*  1400 */   133,  123,  124,  125,  133,  133,  133,  133,  133,  133,
 /*  1410 */   133,  133,  133,  133,  133,  133,  133,  133,  133,   78,
 /*  1420 */   133,  133,  133,  105,  106,  107,  133,  109,  110,  111,
 /*  1430 */   112,  133,  114,  115,  116,  117,  133,  119,  120,  133,
 /*  1440 */    78,  123,  124,  125,  133,  133,  105,  106,  107,  133,
 /*  1450 */   109,  110,  111,  112,  133,  114,  115,  116,  117,  133,
 /*  1460 */   119,  120,   78,  133,  123,  124,  125,  105,  106,  107,
 /*  1470 */   133,  109,  110,  111,  112,  133,  114,  115,  116,  117,
 /*  1480 */   133,  119,  120,  133,  133,  123,  124,  125,  133,  105,
 /*  1490 */   106,  107,  133,  109,  110,  111,  112,  133,  114,  115,
 /*  1500 */   116,  117,   78,  119,  120,  133,  133,  123,  124,  125,
 /*  1510 */   133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
 /*  1520 */   133,  133,  133,  133,  133,   78,  133,  133,  133,  105,
 /*  1530 */   106,  107,  133,  109,  110,  111,  112,  133,  114,  115,
 /*  1540 */   116,  117,  133,  119,  120,  133,   78,  123,  124,  125,
 /*  1550 */   133,  133,  105,  106,  107,  133,  109,  110,  111,  112,
 /*  1560 */   133,  114,  115,  116,  117,  133,  119,  120,   78,  133,
 /*  1570 */   123,  124,  125,  105,  106,  107,  133,  109,  110,  111,
 /*  1580 */   112,  133,  114,  115,  116,  117,  133,  119,  120,  133,
 /*  1590 */   133,  123,  124,  125,  133,  105,  106,  107,  133,  109,
 /*  1600 */   110,  111,  112,  133,  114,  115,  116,  117,   78,  119,
 /*  1610 */   120,  133,  133,  123,  124,  125,  133,  133,  133,  133,
 /*  1620 */   133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
 /*  1630 */   133,   78,  133,  133,  133,  105,  106,  107,  133,  109,
 /*  1640 */   110,  111,  112,  133,  114,  115,  116,  117,  133,  119,
 /*  1650 */   120,  133,   78,  123,  124,  125,  133,  133,  105,  106,
 /*  1660 */   107,  133,  109,  110,  111,  112,  133,  114,  115,  116,
 /*  1670 */   117,  133,  119,  120,   78,  133,  123,  124,  125,  105,
 /*  1680 */   106,  107,  133,  109,  110,  111,  112,  133,  114,  115,
 /*  1690 */   116,  117,  133,  119,  120,  133,  133,  123,  124,  125,
 /*  1700 */   133,  105,  106,  107,  133,  109,  110,  111,  112,  133,
 /*  1710 */   114,  115,  116,  117,   78,  119,  120,  133,  133,  123,
 /*  1720 */   124,  125,  133,  133,  133,  133,  133,  133,  133,  133,
 /*  1730 */   133,  133,  133,  133,  133,  133,  133,   78,  133,  133,
 /*  1740 */   133,  105,  106,  107,  133,  109,  110,  111,  112,  133,
 /*  1750 */   114,  115,  116,  117,   12,  119,  120,  133,  133,  123,
 /*  1760 */   124,  125,  133,   21,  105,  106,  107,  133,  109,  110,
 /*  1770 */   111,  112,  133,  114,  115,  116,  117,  133,  119,  120,
 /*  1780 */   133,   12,  123,  124,  125,   43,  133,  133,  133,  133,
 /*  1790 */    21,  133,  133,   51,   52,  133,  133,  133,   56,   57,
 /*  1800 */   133,   59,   60,   61,   62,   63,   64,   65,   66,   67,
 /*  1810 */   133,  133,   43,  133,  133,  133,  133,  133,  133,  133,
 /*  1820 */    51,   52,  133,  133,  133,   56,   57,  133,   59,   60,
 /*  1830 */    61,   62,   63,   64,   65,   66,   67,  133,  133,  133,
 /*  1840 */   133,  133,   12,  133,  106,  107,  133,  109,  110,  111,
 /*  1850 */   112,   21,  114,  115,  116,  117,  133,  119,  120,  133,
 /*  1860 */   133,  123,  124,  125,  133,  133,  106,  133,  133,  109,
 /*  1870 */   110,  111,  112,  133,  114,  115,  116,  117,  133,  119,
 /*  1880 */   120,   51,   52,  123,  124,  125,   56,   57,  133,   59,
 /*  1890 */    60,   61,   62,   63,   64,   65,   66,   67,  133,  133,
 /*  1900 */   106,  133,  133,  133,  110,  111,  112,  133,  114,  115,
 /*  1910 */   116,  117,  133,  119,  120,  133,  106,  123,  124,  125,
 /*  1920 */   110,  111,  112,  133,  114,  115,  116,  117,  133,  119,
 /*  1930 */   120,  133,  133,  123,  124,  125,
};
#define YY_SHIFT_USE_DFLT (-11)
#define YY_SHIFT_MAX 195
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2, 1061,   -2, 1061,
 /*    20 */  1134, 1191, 1742, 1258, 1769, 1769, 1769, 1769, 1769, 1769,
 /*    30 */  1769, 1769, 1769, 1769, 1769, 1769, 1769, 1769, 1769, 1769,
 /*    40 */  1769, 1769, 1769, 1769, 1769, 1769, 1830, 1830, 1830, 1830,
 /*    50 */  1830,   -9,   -9, 1830, 1830,  174, 1830, 1830, 1830, 1830,
 /*    60 */  1830,  386,  386,   68,   29,  412,  460,  460,  249,   59,
 /*    70 */    59,   59,   59,  -10,   18,   77,  137,  137,   52,  158,
 /*    80 */   275,  295,  275,  295,  115,  161,  203,  203,  203,  203,
 /*    90 */   231,  281,  351,  -10,  356,  350,  350,   18,  350,  350,
 /*   100 */    18,  350,  350,  350,  350,   18,  231,  411,  328,  363,
 /*   110 */   156,  407,  393,   15,  401,  418,  448,  316,  150,  472,
 /*   120 */   430,  438,  443,  421,  427,  408,  428,  431,  432,  421,
 /*   130 */   427,  428,  431,  432,  449,  458,  413,  415,  415,  469,
 /*   140 */   470,  474,  467,  484,  467,  487,  492,  493,  482,  509,
 /*   150 */   510,  512,  516,  438,  506,  519,  511,  505,  513,  508,
 /*   160 */   514,  517,  514,  521,  518,  507,  515,  520,  522,  523,
 /*   170 */   524,  529,  538,  539,  547,  542,  551,  552,  553,  555,
 /*   180 */   556,  557,  558,  559,  560,  475,  531,  476,  533,  421,
 /*   190 */   575,  568,  534,  438,  438,  438,
};
#define YY_REDUCE_USE_DFLT (-93)
#define YY_REDUCE_MAX 106
static const short yy_reduce_ofst[] = {
 /*     0 */   -38,   13,   64,  119,  170,  225,  276,  331,  382,  437,
 /*    10 */   488,  543,  594,  649,  700,  755,  806,  846,  896,  921,
 /*    20 */   951, 1024, 1059, 1166, 1235, 1256, 1278, 1318, 1341, 1362,
 /*    30 */  1384, 1424, 1447, 1468, 1490, 1530, 1553, 1574, 1596, 1636,
 /*    40 */  1659, 1113, 1738, 1760, 1794, 1810,  -92,   41,   87,  147,
 /*    50 */   202,  125,  218,  -77,   25,  277,  126,  131,  232,  237,
 /*    60 */   253,  332,  372,  194,   20,  -79,  134,  181,  216,   49,
 /*    70 */    73,  146,  163,  219,  195,  -52,  -56,  -56,   -8,   -5,
 /*    80 */     3,    5,    3,    5,    9,   76,  162,  166,  175,  180,
 /*    90 */   142,  250,  284,  287,  290,  294,  308,  307,  319,  321,
 /*   100 */   307,  323,  329,  334,  336,  307,  302,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   326,  326,  326,  326,  326,  326,  326,  326,  326,  326,
 /*    10 */   326,  326,  326,  326,  326,  326,  326,  405,  326,  520,
 /*    20 */   520,  520,  507,  520,  520,  333,  520,  520,  520,  520,
 /*    30 */   520,  520,  520,  335,  337,  520,  520,  520,  520,  520,
 /*    40 */   520,  520,  520,  520,  520,  520,  520,  520,  520,  520,
 /*    50 */   520,  393,  393,  520,  520,  520,  520,  520,  520,  520,
 /*    60 */   520,  520,  520,  520,  520,  518,  354,  354,  520,  520,
 /*    70 */   520,  520,  520,  520,  397,  479,  466,  465,  449,  518,
 /*    80 */   458,  464,  457,  463,  508,  506,  520,  520,  520,  520,
 /*    90 */   511,  350,  520,  520,  358,  520,  520,  520,  520,  520,
 /*   100 */   401,  520,  520,  520,  520,  400,  511,  479,  520,  520,
 /*   110 */   520,  520,  520,  520,  520,  520,  520,  520,  520,  520,
 /*   120 */   520,  519,  520,  428,  444,  450,  454,  456,  460,  430,
 /*   130 */   443,  453,  455,  459,  485,  520,  520,  520,  500,  406,
 /*   140 */   407,  408,  520,  410,  485,  413,  414,  417,  520,  520,
 /*   150 */   520,  520,  520,  355,  520,  520,  520,  342,  520,  343,
 /*   160 */   345,  520,  344,  520,  520,  520,  520,  377,  369,  365,
 /*   170 */   363,  520,  520,  367,  520,  373,  371,  375,  385,  381,
 /*   180 */   379,  383,  389,  387,  391,  520,  520,  520,  520,  429,
 /*   190 */   520,  520,  520,  515,  516,  517,  325,  327,  328,  324,
 /*   200 */   329,  332,  334,  427,  446,  447,  448,  469,  475,  476,
 /*   210 */   477,  478,  480,  481,  485,  486,  487,  488,  489,  490,
 /*   220 */   491,  492,  493,  445,  461,  462,  467,  468,  471,  472,
 /*   230 */   473,  474,  470,  494,  499,  505,  496,  497,  503,  504,
 /*   240 */   498,  502,  501,  495,  500,  409,  420,  421,  425,  426,
 /*   250 */   411,  412,  423,  424,  415,  416,  418,  419,  422,  482,
 /*   260 */   509,  336,  338,  339,  352,  353,  340,  341,  349,  348,
 /*   270 */   347,  346,  360,  361,  359,  351,  356,  362,  394,  364,
 /*   280 */   395,  366,  368,  396,  398,  399,  403,  404,  370,  372,
 /*   290 */   374,  376,  402,  378,  380,  382,  384,  386,  388,  390,
 /*   300 */   392,  512,  510,  483,  484,  451,  452,  431,  432,  433,
 /*   310 */   434,  435,  436,  437,  438,  439,  440,  441,  442,  330,
 /*   320 */   514,  331,  513,
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
  "COLON",         "PLUS_EQUAL",    "MINUS_EQUAL",   "STAR_EQUAL",  
  "DIV_EQUAL",     "DIV_DIV_EQUAL",  "PERCENT_EQUAL",  "BAR_EQUAL",   
  "AND_EQUAL",     "XOR_EQUAL",     "STAR_STAR_EQUAL",  "LSHIFT_EQUAL",
  "RSHIFT_EQUAL",  "BAR_BAR",       "AND_AND",       "NOT",         
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
  "augmented_assign_op",  "logical_and_expr",  "not_expr",      "comparison",  
  "xor_expr",      "comp_op",       "or_expr",       "and_expr",    
  "shift_expr",    "match_expr",    "shift_op",      "arith_expr",  
  "term",          "arith_op",      "term_op",       "factor",      
  "power",         "atom",          "blockarg_opt",  "exprs",       
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
 /* 107 */ "assign_expr ::= logical_or_expr",
 /* 108 */ "augmented_assign_op ::= PLUS_EQUAL",
 /* 109 */ "augmented_assign_op ::= MINUS_EQUAL",
 /* 110 */ "augmented_assign_op ::= STAR_EQUAL",
 /* 111 */ "augmented_assign_op ::= DIV_EQUAL",
 /* 112 */ "augmented_assign_op ::= DIV_DIV_EQUAL",
 /* 113 */ "augmented_assign_op ::= PERCENT_EQUAL",
 /* 114 */ "augmented_assign_op ::= BAR_EQUAL",
 /* 115 */ "augmented_assign_op ::= AND_EQUAL",
 /* 116 */ "augmented_assign_op ::= XOR_EQUAL",
 /* 117 */ "augmented_assign_op ::= STAR_STAR_EQUAL",
 /* 118 */ "augmented_assign_op ::= LSHIFT_EQUAL",
 /* 119 */ "augmented_assign_op ::= RSHIFT_EQUAL",
 /* 120 */ "logical_or_expr ::= logical_and_expr",
 /* 121 */ "logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr",
 /* 122 */ "logical_and_expr ::= not_expr",
 /* 123 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /* 124 */ "not_expr ::= comparison",
 /* 125 */ "not_expr ::= NOT not_expr",
 /* 126 */ "comparison ::= xor_expr",
 /* 127 */ "comparison ::= xor_expr comp_op xor_expr",
 /* 128 */ "comp_op ::= LESS",
 /* 129 */ "comp_op ::= GREATER",
 /* 130 */ "xor_expr ::= or_expr",
 /* 131 */ "xor_expr ::= xor_expr XOR or_expr",
 /* 132 */ "or_expr ::= and_expr",
 /* 133 */ "or_expr ::= or_expr BAR and_expr",
 /* 134 */ "and_expr ::= shift_expr",
 /* 135 */ "and_expr ::= and_expr AND shift_expr",
 /* 136 */ "shift_expr ::= match_expr",
 /* 137 */ "shift_expr ::= shift_expr shift_op match_expr",
 /* 138 */ "shift_op ::= LSHIFT",
 /* 139 */ "shift_op ::= RSHIFT",
 /* 140 */ "match_expr ::= arith_expr",
 /* 141 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 142 */ "arith_expr ::= term",
 /* 143 */ "arith_expr ::= arith_expr arith_op term",
 /* 144 */ "arith_op ::= PLUS",
 /* 145 */ "arith_op ::= MINUS",
 /* 146 */ "term ::= term term_op factor",
 /* 147 */ "term ::= factor",
 /* 148 */ "term_op ::= STAR",
 /* 149 */ "term_op ::= DIV",
 /* 150 */ "term_op ::= DIV_DIV",
 /* 151 */ "term_op ::= PERCENT",
 /* 152 */ "factor ::= PLUS factor",
 /* 153 */ "factor ::= MINUS factor",
 /* 154 */ "factor ::= TILDA factor",
 /* 155 */ "factor ::= power",
 /* 156 */ "power ::= postfix_expr",
 /* 157 */ "power ::= postfix_expr STAR_STAR factor",
 /* 158 */ "postfix_expr ::= atom",
 /* 159 */ "postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt",
 /* 160 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 161 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 162 */ "atom ::= NAME",
 /* 163 */ "atom ::= NUMBER",
 /* 164 */ "atom ::= REGEXP",
 /* 165 */ "atom ::= STRING",
 /* 166 */ "atom ::= SYMBOL",
 /* 167 */ "atom ::= NIL",
 /* 168 */ "atom ::= TRUE",
 /* 169 */ "atom ::= FALSE",
 /* 170 */ "atom ::= LINE",
 /* 171 */ "atom ::= LBRACKET exprs RBRACKET",
 /* 172 */ "atom ::= LBRACKET RBRACKET",
 /* 173 */ "atom ::= LBRACE RBRACE",
 /* 174 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 175 */ "atom ::= LBRACE exprs RBRACE",
 /* 176 */ "atom ::= LPAR expr RPAR",
 /* 177 */ "exprs ::= expr",
 /* 178 */ "exprs ::= exprs COMMA expr",
 /* 179 */ "dict_elems ::= dict_elem",
 /* 180 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 181 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 182 */ "dict_elem ::= NAME COLON expr",
 /* 183 */ "comma_opt ::=",
 /* 184 */ "comma_opt ::= COMMA",
 /* 185 */ "blockarg_opt ::=",
 /* 186 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 187 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 188 */ "blockarg_params_opt ::=",
 /* 189 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 190 */ "excepts ::= except",
 /* 191 */ "excepts ::= excepts except",
 /* 192 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 193 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 194 */ "except ::= EXCEPT NEWLINE stmts",
 /* 195 */ "finally_opt ::=",
 /* 196 */ "finally_opt ::= FINALLY stmts",
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
  { 74, 1 },
  { 75, 1 },
  { 75, 3 },
  { 76, 0 },
  { 76, 1 },
  { 76, 1 },
  { 76, 7 },
  { 76, 5 },
  { 76, 5 },
  { 76, 5 },
  { 76, 1 },
  { 76, 2 },
  { 76, 1 },
  { 76, 2 },
  { 76, 1 },
  { 76, 2 },
  { 76, 6 },
  { 76, 7 },
  { 76, 4 },
  { 76, 2 },
  { 76, 2 },
  { 85, 1 },
  { 85, 3 },
  { 86, 1 },
  { 86, 3 },
  { 84, 1 },
  { 84, 3 },
  { 83, 0 },
  { 83, 2 },
  { 81, 1 },
  { 81, 5 },
  { 87, 0 },
  { 87, 2 },
  { 77, 8 },
  { 82, 0 },
  { 82, 1 },
  { 89, 1 },
  { 89, 2 },
  { 90, 3 },
  { 88, 9 },
  { 88, 7 },
  { 88, 7 },
  { 88, 5 },
  { 88, 7 },
  { 88, 5 },
  { 88, 5 },
  { 88, 3 },
  { 88, 7 },
  { 88, 5 },
  { 88, 5 },
  { 88, 3 },
  { 88, 5 },
  { 88, 3 },
  { 88, 3 },
  { 88, 1 },
  { 88, 7 },
  { 88, 5 },
  { 88, 5 },
  { 88, 3 },
  { 88, 5 },
  { 88, 3 },
  { 88, 3 },
  { 88, 1 },
  { 88, 5 },
  { 88, 3 },
  { 88, 3 },
  { 88, 1 },
  { 88, 3 },
  { 88, 1 },
  { 88, 1 },
  { 88, 0 },
  { 95, 2 },
  { 94, 2 },
  { 93, 3 },
  { 96, 0 },
  { 96, 1 },
  { 97, 2 },
  { 91, 1 },
  { 91, 3 },
  { 92, 1 },
  { 92, 3 },
  { 98, 2 },
  { 99, 0 },
  { 99, 1 },
  { 99, 3 },
  { 99, 5 },
  { 99, 7 },
  { 99, 3 },
  { 99, 5 },
  { 99, 3 },
  { 99, 1 },
  { 99, 3 },
  { 99, 5 },
  { 99, 3 },
  { 99, 1 },
  { 99, 3 },
  { 99, 1 },
  { 103, 2 },
  { 102, 2 },
  { 100, 1 },
  { 100, 3 },
  { 101, 1 },
  { 101, 3 },
  { 104, 3 },
  { 78, 1 },
  { 105, 3 },
  { 105, 3 },
  { 105, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 108, 1 },
  { 107, 1 },
  { 107, 3 },
  { 109, 1 },
  { 109, 3 },
  { 110, 1 },
  { 110, 2 },
  { 111, 1 },
  { 111, 3 },
  { 113, 1 },
  { 113, 1 },
  { 112, 1 },
  { 112, 3 },
  { 114, 1 },
  { 114, 3 },
  { 115, 1 },
  { 115, 3 },
  { 116, 1 },
  { 116, 3 },
  { 118, 1 },
  { 118, 1 },
  { 117, 1 },
  { 117, 3 },
  { 119, 1 },
  { 119, 3 },
  { 121, 1 },
  { 121, 1 },
  { 120, 3 },
  { 120, 1 },
  { 122, 1 },
  { 122, 1 },
  { 122, 1 },
  { 122, 1 },
  { 123, 2 },
  { 123, 2 },
  { 123, 2 },
  { 123, 1 },
  { 124, 1 },
  { 124, 3 },
  { 106, 1 },
  { 106, 5 },
  { 106, 4 },
  { 106, 3 },
  { 125, 1 },
  { 125, 1 },
  { 125, 1 },
  { 125, 1 },
  { 125, 1 },
  { 125, 1 },
  { 125, 1 },
  { 125, 1 },
  { 125, 1 },
  { 125, 3 },
  { 125, 2 },
  { 125, 2 },
  { 125, 4 },
  { 125, 3 },
  { 125, 3 },
  { 127, 1 },
  { 127, 3 },
  { 128, 1 },
  { 128, 3 },
  { 130, 3 },
  { 130, 3 },
  { 129, 0 },
  { 129, 1 },
  { 126, 0 },
  { 126, 5 },
  { 126, 5 },
  { 131, 0 },
  { 131, 3 },
  { 79, 1 },
  { 79, 2 },
  { 132, 6 },
  { 132, 4 },
  { 132, 3 },
  { 80, 0 },
  { 80, 2 },
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
#line 735 "parser.y"
{
    *pval = yymsp[0].minor.yy191;
}
#line 2272 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 21: /* dotted_names ::= dotted_name */
      case 36: /* decorators ::= decorator */
      case 79: /* params_with_default ::= param_with_default */
      case 99: /* posargs ::= expr */
      case 101: /* kwargs ::= kwarg */
      case 177: /* exprs ::= expr */
      case 179: /* dict_elems ::= dict_elem */
      case 190: /* excepts ::= except */
#line 739 "parser.y"
{
    yygotominor.yy191 = make_array_with(env, yymsp[0].minor.yy191);
}
#line 2287 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 22: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 80: /* params_with_default ::= params_with_default COMMA param_with_default */
#line 742 "parser.y"
{
    yygotominor.yy191 = Array_push(env, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2296 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 27: /* super_opt ::= */
      case 31: /* else_opt ::= */
      case 34: /* decorators_opt ::= */
      case 74: /* param_default_opt ::= */
      case 82: /* args ::= */
      case 183: /* comma_opt ::= */
      case 185: /* blockarg_opt ::= */
      case 188: /* blockarg_params_opt ::= */
      case 195: /* finally_opt ::= */
#line 746 "parser.y"
{
    yygotominor.yy191 = YNIL;
}
#line 2312 "parser.c"
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
      case 107: /* assign_expr ::= logical_or_expr */
      case 120: /* logical_or_expr ::= logical_and_expr */
      case 122: /* logical_and_expr ::= not_expr */
      case 124: /* not_expr ::= comparison */
      case 126: /* comparison ::= xor_expr */
      case 130: /* xor_expr ::= or_expr */
      case 132: /* or_expr ::= and_expr */
      case 134: /* and_expr ::= shift_expr */
      case 136: /* shift_expr ::= match_expr */
      case 140: /* match_expr ::= arith_expr */
      case 142: /* arith_expr ::= term */
      case 147: /* term ::= factor */
      case 155: /* factor ::= power */
      case 156: /* power ::= postfix_expr */
      case 158: /* postfix_expr ::= atom */
      case 196: /* finally_opt ::= FINALLY stmts */
#line 749 "parser.y"
{
    yygotominor.yy191 = yymsp[0].minor.yy191;
}
#line 2345 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 755 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy191 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy191, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[-1].minor.yy191);
}
#line 2353 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 759 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy191 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy191, yymsp[-2].minor.yy191, YNIL, yymsp[-1].minor.yy191);
}
#line 2361 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 763 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy191 = Finally_new(env, lineno, yymsp[-3].minor.yy191, yymsp[-1].minor.yy191);
}
#line 2369 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 767 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy191 = While_new(env, lineno, yymsp[-3].minor.yy191, yymsp[-1].minor.yy191);
}
#line 2377 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 771 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy191 = Break_new(env, lineno, YNIL);
}
#line 2385 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 775 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy191 = Break_new(env, lineno, yymsp[0].minor.yy191);
}
#line 2393 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 779 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy191 = Next_new(env, lineno, YNIL);
}
#line 2401 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 783 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy191 = Next_new(env, lineno, yymsp[0].minor.yy191);
}
#line 2409 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 787 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy191 = Return_new(env, lineno, YNIL);
}
#line 2417 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 791 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy191 = Return_new(env, lineno, yymsp[0].minor.yy191);
}
#line 2425 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 795 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy191 = If_new(env, lineno, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[-1].minor.yy191);
}
#line 2433 "parser.c"
        break;
      case 17: /* stmt ::= decorators_opt CLASS NAME super_opt NEWLINE stmts END */
#line 799 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy191 = Klass_new(env, lineno, yymsp[-6].minor.yy191, id, yymsp[-3].minor.yy191, yymsp[-1].minor.yy191);
}
#line 2442 "parser.c"
        break;
      case 18: /* stmt ::= MODULE NAME stmts END */
#line 804 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    yygotominor.yy191 = Module_new(env, lineno, id, yymsp[-1].minor.yy191);
}
#line 2451 "parser.c"
        break;
      case 19: /* stmt ::= NONLOCAL names */
#line 809 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy191 = Nonlocal_new(env, lineno, yymsp[0].minor.yy191);
}
#line 2459 "parser.c"
        break;
      case 20: /* stmt ::= IMPORT dotted_names */
#line 813 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy191 = Import_new(env, lineno, yymsp[0].minor.yy191);
}
#line 2467 "parser.c"
        break;
      case 23: /* dotted_name ::= NAME */
      case 25: /* names ::= NAME */
#line 825 "parser.y"
{
    yygotominor.yy191 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2475 "parser.c"
        break;
      case 24: /* dotted_name ::= dotted_name DOT NAME */
      case 26: /* names ::= names COMMA NAME */
#line 828 "parser.y"
{
    yygotominor.yy191 = Array_push_token_id(env, yymsp[-2].minor.yy191, yymsp[0].minor.yy0);
}
#line 2483 "parser.c"
        break;
      case 30: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 849 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy191, yymsp[-1].minor.yy191, yymsp[0].minor.yy191);
    yygotominor.yy191 = make_array_with(env, node);
}
#line 2492 "parser.c"
        break;
      case 33: /* func_def ::= decorators_opt DEF NAME LPAR params RPAR stmts END */
#line 862 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy191 = FuncDef_new(env, lineno, yymsp[-7].minor.yy191, id, yymsp[-3].minor.yy191, yymsp[-1].minor.yy191);
}
#line 2501 "parser.c"
        break;
      case 37: /* decorators ::= decorators decorator */
      case 191: /* excepts ::= excepts except */
#line 878 "parser.y"
{
    yygotominor.yy191 = Array_push(env, yymsp[-1].minor.yy191, yymsp[0].minor.yy191);
}
#line 2509 "parser.c"
        break;
      case 38: /* decorator ::= AT expr NEWLINE */
      case 176: /* atom ::= LPAR expr RPAR */
      case 189: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 882 "parser.y"
{
    yygotominor.yy191 = yymsp[-1].minor.yy191;
}
#line 2518 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 886 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-8].minor.yy191, yymsp[-6].minor.yy191, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2525 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 889 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-6].minor.yy191, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191, YNIL);
}
#line 2532 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 892 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-6].minor.yy191, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, YNIL, yymsp[0].minor.yy191);
}
#line 2539 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 895 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191, YNIL, YNIL);
}
#line 2546 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 898 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-6].minor.yy191, yymsp[-4].minor.yy191, YNIL, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2553 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 901 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, YNIL, yymsp[0].minor.yy191, YNIL);
}
#line 2560 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 904 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, YNIL, YNIL, yymsp[0].minor.yy191);
}
#line 2567 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA params_with_default */
#line 907 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-2].minor.yy191, yymsp[0].minor.yy191, YNIL, YNIL, YNIL);
}
#line 2574 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 910 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-6].minor.yy191, YNIL, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2581 "parser.c"
        break;
      case 48: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 913 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-4].minor.yy191, YNIL, yymsp[-2].minor.yy191, yymsp[0].minor.yy191, YNIL);
}
#line 2588 "parser.c"
        break;
      case 49: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 916 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-4].minor.yy191, YNIL, yymsp[-2].minor.yy191, YNIL, yymsp[0].minor.yy191);
}
#line 2595 "parser.c"
        break;
      case 50: /* params ::= params_without_default COMMA block_param */
#line 919 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-2].minor.yy191, YNIL, yymsp[0].minor.yy191, YNIL, YNIL);
}
#line 2602 "parser.c"
        break;
      case 51: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 922 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-4].minor.yy191, YNIL, YNIL, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2609 "parser.c"
        break;
      case 52: /* params ::= params_without_default COMMA var_param */
#line 925 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-2].minor.yy191, YNIL, YNIL, yymsp[0].minor.yy191, YNIL);
}
#line 2616 "parser.c"
        break;
      case 53: /* params ::= params_without_default COMMA kw_param */
#line 928 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[-2].minor.yy191, YNIL, YNIL, YNIL, yymsp[0].minor.yy191);
}
#line 2623 "parser.c"
        break;
      case 54: /* params ::= params_without_default */
#line 931 "parser.y"
{
    yygotominor.yy191 = Params_new(env, yymsp[0].minor.yy191, YNIL, YNIL, YNIL, YNIL);
}
#line 2630 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 934 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, yymsp[-6].minor.yy191, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2637 "parser.c"
        break;
      case 56: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 937 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191, YNIL);
}
#line 2644 "parser.c"
        break;
      case 57: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 940 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, YNIL, yymsp[0].minor.yy191);
}
#line 2651 "parser.c"
        break;
      case 58: /* params ::= params_with_default COMMA block_param */
#line 943 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, yymsp[-2].minor.yy191, yymsp[0].minor.yy191, YNIL, YNIL);
}
#line 2658 "parser.c"
        break;
      case 59: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 946 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, yymsp[-4].minor.yy191, YNIL, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2665 "parser.c"
        break;
      case 60: /* params ::= params_with_default COMMA var_param */
#line 949 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, yymsp[-2].minor.yy191, YNIL, yymsp[0].minor.yy191, YNIL);
}
#line 2672 "parser.c"
        break;
      case 61: /* params ::= params_with_default COMMA kw_param */
#line 952 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, yymsp[-2].minor.yy191, YNIL, YNIL, yymsp[0].minor.yy191);
}
#line 2679 "parser.c"
        break;
      case 62: /* params ::= params_with_default */
#line 955 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, yymsp[0].minor.yy191, YNIL, YNIL, YNIL);
}
#line 2686 "parser.c"
        break;
      case 63: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 958 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2693 "parser.c"
        break;
      case 64: /* params ::= block_param COMMA var_param */
#line 961 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy191, yymsp[0].minor.yy191, YNIL);
}
#line 2700 "parser.c"
        break;
      case 65: /* params ::= block_param COMMA kw_param */
#line 964 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy191, YNIL, yymsp[0].minor.yy191);
}
#line 2707 "parser.c"
        break;
      case 66: /* params ::= block_param */
#line 967 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy191, YNIL, YNIL);
}
#line 2714 "parser.c"
        break;
      case 67: /* params ::= var_param COMMA kw_param */
#line 970 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2721 "parser.c"
        break;
      case 68: /* params ::= var_param */
#line 973 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy191, YNIL);
}
#line 2728 "parser.c"
        break;
      case 69: /* params ::= kw_param */
#line 976 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy191);
}
#line 2735 "parser.c"
        break;
      case 70: /* params ::= */
#line 979 "parser.y"
{
    yygotominor.yy191 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2742 "parser.c"
        break;
      case 71: /* kw_param ::= STAR_STAR NAME */
#line 983 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy191 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2751 "parser.c"
        break;
      case 72: /* var_param ::= STAR NAME */
#line 989 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy191 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2760 "parser.c"
        break;
      case 73: /* block_param ::= AMPER NAME param_default_opt */
#line 995 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy191 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy191);
}
#line 2769 "parser.c"
        break;
      case 77: /* params_without_default ::= NAME */
#line 1012 "parser.y"
{
    yygotominor.yy191 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy191, lineno, id, YNIL);
}
#line 2779 "parser.c"
        break;
      case 78: /* params_without_default ::= params_without_default COMMA NAME */
#line 1018 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy191, lineno, id, YNIL);
    yygotominor.yy191 = yymsp[-2].minor.yy191;
}
#line 2789 "parser.c"
        break;
      case 81: /* param_with_default ::= NAME param_default */
#line 1032 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy191 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy191);
}
#line 2798 "parser.c"
        break;
      case 83: /* args ::= posargs */
#line 1041 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, yymsp[0].minor.yy191, YNIL, YNIL, YNIL);
}
#line 2806 "parser.c"
        break;
      case 84: /* args ::= posargs COMMA kwargs */
#line 1045 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, yymsp[-2].minor.yy191, yymsp[0].minor.yy191, YNIL, YNIL);
}
#line 2814 "parser.c"
        break;
      case 85: /* args ::= posargs COMMA kwargs COMMA vararg */
#line 1049 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191, YNIL);
}
#line 2822 "parser.c"
        break;
      case 86: /* args ::= posargs COMMA kwargs COMMA vararg COMMA varkwarg */
#line 1053 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-6].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, yymsp[-6].minor.yy191, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2830 "parser.c"
        break;
      case 87: /* args ::= posargs COMMA vararg */
#line 1057 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, yymsp[-2].minor.yy191, YNIL, yymsp[0].minor.yy191, YNIL);
}
#line 2838 "parser.c"
        break;
      case 88: /* args ::= posargs COMMA vararg COMMA varkwarg */
#line 1061 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, yymsp[-4].minor.yy191, YNIL, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2846 "parser.c"
        break;
      case 89: /* args ::= posargs COMMA varkwarg */
#line 1065 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, yymsp[-2].minor.yy191, YNIL, YNIL, yymsp[0].minor.yy191);
}
#line 2854 "parser.c"
        break;
      case 90: /* args ::= kwargs */
#line 1069 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, YNIL, yymsp[0].minor.yy191, YNIL, YNIL);
}
#line 2862 "parser.c"
        break;
      case 91: /* args ::= kwargs COMMA vararg */
#line 1073 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy191, yymsp[0].minor.yy191, YNIL);
}
#line 2870 "parser.c"
        break;
      case 92: /* args ::= kwargs COMMA vararg COMMA varkwarg */
#line 1077 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-4].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, YNIL, yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2878 "parser.c"
        break;
      case 93: /* args ::= kwargs COMMA varkwarg */
#line 1081 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy191, 0));
    yygotominor.yy191 = Args_new(env, lineno, YNIL, yymsp[-2].minor.yy191, YNIL, yymsp[0].minor.yy191);
}
#line 2886 "parser.c"
        break;
      case 94: /* args ::= vararg */
#line 1085 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy191);
    yygotominor.yy191 = Args_new(env, lineno, YNIL, YNIL, yymsp[0].minor.yy191, YNIL);
}
#line 2894 "parser.c"
        break;
      case 95: /* args ::= vararg COMMA varkwarg */
#line 1089 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy191);
    yygotominor.yy191 = Args_new(env, lineno, YNIL, YNIL, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2902 "parser.c"
        break;
      case 96: /* args ::= varkwarg */
#line 1093 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[0].minor.yy191);
    yygotominor.yy191 = Args_new(env, lineno, YNIL, YNIL, YNIL, yymsp[0].minor.yy191);
}
#line 2910 "parser.c"
        break;
      case 100: /* posargs ::= posargs COMMA expr */
      case 102: /* kwargs ::= kwargs COMMA kwarg */
      case 178: /* exprs ::= exprs COMMA expr */
      case 180: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 1109 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
    yygotominor.yy191 = yymsp[-2].minor.yy191;
}
#line 2921 "parser.c"
        break;
      case 103: /* kwarg ::= NAME COLON expr */
#line 1122 "parser.y"
{
    yygotominor.yy191 = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(yymsp[-2].minor.yy0));
    PTR_AS(YogNode, yygotominor.yy191)->u.kwarg.name = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    PTR_AS(YogNode, yygotominor.yy191)->u.kwarg.value = yymsp[0].minor.yy191;
}
#line 2930 "parser.c"
        break;
      case 105: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 1132 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy191);
    yygotominor.yy191 = Assign_new(env, lineno, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 2938 "parser.c"
        break;
      case 106: /* assign_expr ::= postfix_expr augmented_assign_op logical_or_expr */
#line 1136 "parser.y"
{
    yygotominor.yy191 = AugmentedAssign_new(env, NODE_LINENO(yymsp[-2].minor.yy191), yymsp[-2].minor.yy191, VAL2ID(yymsp[-1].minor.yy191), yymsp[0].minor.yy191);
}
#line 2945 "parser.c"
        break;
      case 108: /* augmented_assign_op ::= PLUS_EQUAL */
#line 1143 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "+"));
}
#line 2952 "parser.c"
        break;
      case 109: /* augmented_assign_op ::= MINUS_EQUAL */
#line 1146 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "-"));
}
#line 2959 "parser.c"
        break;
      case 110: /* augmented_assign_op ::= STAR_EQUAL */
#line 1149 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "*"));
}
#line 2966 "parser.c"
        break;
      case 111: /* augmented_assign_op ::= DIV_EQUAL */
#line 1152 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "/"));
}
#line 2973 "parser.c"
        break;
      case 112: /* augmented_assign_op ::= DIV_DIV_EQUAL */
#line 1155 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "//"));
}
#line 2980 "parser.c"
        break;
      case 113: /* augmented_assign_op ::= PERCENT_EQUAL */
#line 1158 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "%"));
}
#line 2987 "parser.c"
        break;
      case 114: /* augmented_assign_op ::= BAR_EQUAL */
#line 1161 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "|"));
}
#line 2994 "parser.c"
        break;
      case 115: /* augmented_assign_op ::= AND_EQUAL */
#line 1164 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "&"));
}
#line 3001 "parser.c"
        break;
      case 116: /* augmented_assign_op ::= XOR_EQUAL */
#line 1167 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "^"));
}
#line 3008 "parser.c"
        break;
      case 117: /* augmented_assign_op ::= STAR_STAR_EQUAL */
#line 1170 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "**"));
}
#line 3015 "parser.c"
        break;
      case 118: /* augmented_assign_op ::= LSHIFT_EQUAL */
#line 1173 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, "<<"));
}
#line 3022 "parser.c"
        break;
      case 119: /* augmented_assign_op ::= RSHIFT_EQUAL */
#line 1176 "parser.y"
{
    yygotominor.yy191 = ID2VAL(YogVM_intern(env, env->vm, ">>"));
}
#line 3029 "parser.c"
        break;
      case 121: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 1183 "parser.y"
{
    yygotominor.yy191 = YogNode_new(env, NODE_LOGICAL_OR, NODE_LINENO(yymsp[-2].minor.yy191));
    NODE(yygotominor.yy191)->u.logical_or.left = yymsp[-2].minor.yy191;
    NODE(yygotominor.yy191)->u.logical_or.right = yymsp[0].minor.yy191;
}
#line 3038 "parser.c"
        break;
      case 123: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 1192 "parser.y"
{
    yygotominor.yy191 = YogNode_new(env, NODE_LOGICAL_AND, NODE_LINENO(yymsp[-2].minor.yy191));
    NODE(yygotominor.yy191)->u.logical_and.left = yymsp[-2].minor.yy191;
    NODE(yygotominor.yy191)->u.logical_and.right = yymsp[0].minor.yy191;
}
#line 3047 "parser.c"
        break;
      case 125: /* not_expr ::= NOT not_expr */
#line 1201 "parser.y"
{
    yygotominor.yy191 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy191)->u.not.expr = yymsp[0].minor.yy191;
}
#line 3055 "parser.c"
        break;
      case 127: /* comparison ::= xor_expr comp_op xor_expr */
#line 1209 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy191);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy191)->u.id;
    yygotominor.yy191 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy191, id, yymsp[0].minor.yy191);
}
#line 3064 "parser.c"
        break;
      case 128: /* comp_op ::= LESS */
      case 129: /* comp_op ::= GREATER */
      case 184: /* comma_opt ::= COMMA */
#line 1215 "parser.y"
{
    yygotominor.yy191 = yymsp[0].minor.yy0;
}
#line 3073 "parser.c"
        break;
      case 131: /* xor_expr ::= xor_expr XOR or_expr */
      case 133: /* or_expr ::= or_expr BAR and_expr */
      case 135: /* and_expr ::= and_expr AND shift_expr */
#line 1225 "parser.y"
{
    yygotominor.yy191 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy191), yymsp[-2].minor.yy191, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy191);
}
#line 3082 "parser.c"
        break;
      case 137: /* shift_expr ::= shift_expr shift_op match_expr */
      case 143: /* arith_expr ::= arith_expr arith_op term */
      case 146: /* term ::= term term_op factor */
#line 1246 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy191);
    yygotominor.yy191 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy191, VAL2ID(yymsp[-1].minor.yy191), yymsp[0].minor.yy191);
}
#line 3092 "parser.c"
        break;
      case 138: /* shift_op ::= LSHIFT */
      case 139: /* shift_op ::= RSHIFT */
      case 144: /* arith_op ::= PLUS */
      case 145: /* arith_op ::= MINUS */
      case 148: /* term_op ::= STAR */
      case 149: /* term_op ::= DIV */
      case 150: /* term_op ::= DIV_DIV */
      case 151: /* term_op ::= PERCENT */
#line 1251 "parser.y"
{
    yygotominor.yy191 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 3106 "parser.c"
        break;
      case 141: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1261 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy191);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy191 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy191, id, yymsp[0].minor.yy191);
}
#line 3115 "parser.c"
        break;
      case 152: /* factor ::= PLUS factor */
#line 1303 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy191 = FuncCall_new3(env, lineno, yymsp[0].minor.yy191, id);
}
#line 3124 "parser.c"
        break;
      case 153: /* factor ::= MINUS factor */
#line 1308 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy191 = FuncCall_new3(env, lineno, yymsp[0].minor.yy191, id);
}
#line 3133 "parser.c"
        break;
      case 154: /* factor ::= TILDA factor */
#line 1313 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy191 = FuncCall_new3(env, lineno, yymsp[0].minor.yy191, id);
}
#line 3142 "parser.c"
        break;
      case 157: /* power ::= postfix_expr STAR_STAR factor */
#line 1325 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy191);
    ID id = YogVM_intern(env, env->vm, "**");
    yygotominor.yy191 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy191, id, yymsp[0].minor.yy191);
}
#line 3151 "parser.c"
        break;
      case 159: /* postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt */
#line 1334 "parser.y"
{
    yygotominor.yy191 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy191), yymsp[-4].minor.yy191, yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 3158 "parser.c"
        break;
      case 160: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1337 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy191);
    yygotominor.yy191 = Subscript_new(env, lineno, yymsp[-3].minor.yy191, yymsp[-1].minor.yy191);
}
#line 3166 "parser.c"
        break;
      case 161: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1341 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy191);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy191 = Attr_new(env, lineno, yymsp[-2].minor.yy191, id);
}
#line 3175 "parser.c"
        break;
      case 162: /* atom ::= NAME */
#line 1347 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy191 = Variable_new(env, lineno, id);
}
#line 3184 "parser.c"
        break;
      case 163: /* atom ::= NUMBER */
      case 164: /* atom ::= REGEXP */
      case 165: /* atom ::= STRING */
      case 166: /* atom ::= SYMBOL */
#line 1352 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy191 = Literal_new(env, lineno, val);
}
#line 3196 "parser.c"
        break;
      case 167: /* atom ::= NIL */
#line 1372 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy191 = Literal_new(env, lineno, YNIL);
}
#line 3204 "parser.c"
        break;
      case 168: /* atom ::= TRUE */
#line 1376 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy191 = Literal_new(env, lineno, YTRUE);
}
#line 3212 "parser.c"
        break;
      case 169: /* atom ::= FALSE */
#line 1380 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy191 = Literal_new(env, lineno, YFALSE);
}
#line 3220 "parser.c"
        break;
      case 170: /* atom ::= LINE */
#line 1384 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy191 = Literal_new(env, lineno, val);
}
#line 3229 "parser.c"
        break;
      case 171: /* atom ::= LBRACKET exprs RBRACKET */
#line 1389 "parser.y"
{
    yygotominor.yy191 = Array_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy191);
}
#line 3236 "parser.c"
        break;
      case 172: /* atom ::= LBRACKET RBRACKET */
#line 1392 "parser.y"
{
    yygotominor.yy191 = Array_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3243 "parser.c"
        break;
      case 173: /* atom ::= LBRACE RBRACE */
#line 1395 "parser.y"
{
    yygotominor.yy191 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 3250 "parser.c"
        break;
      case 174: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1398 "parser.y"
{
    yygotominor.yy191 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy191);
}
#line 3257 "parser.c"
        break;
      case 175: /* atom ::= LBRACE exprs RBRACE */
#line 1401 "parser.y"
{
    yygotominor.yy191 = Set_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy191);
}
#line 3264 "parser.c"
        break;
      case 181: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1423 "parser.y"
{
    yygotominor.yy191 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy191), yymsp[-2].minor.yy191, yymsp[0].minor.yy191);
}
#line 3271 "parser.c"
        break;
      case 182: /* dict_elem ::= NAME COLON expr */
#line 1426 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy191 = DictElem_new(env, lineno, var, yymsp[0].minor.yy191);
}
#line 3281 "parser.c"
        break;
      case 186: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 187: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1443 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy191 = BlockArg_new(env, lineno, yymsp[-3].minor.yy191, yymsp[-1].minor.yy191);
}
#line 3290 "parser.c"
        break;
      case 192: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1466 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy191 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy191, id, yymsp[0].minor.yy191);
}
#line 3300 "parser.c"
        break;
      case 193: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1472 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy191 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy191, NO_EXC_VAR, yymsp[0].minor.yy191);
}
#line 3308 "parser.c"
        break;
      case 194: /* except ::= EXCEPT NEWLINE stmts */
#line 1476 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy191 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy191);
}
#line 3316 "parser.c"
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
