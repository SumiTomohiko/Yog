
%token_prefix TK_

%token_type { YogVal }
%default_type { YogVal }

%extra_argument { YogVal* pval }

%include {
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "yog/array.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/parser.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

typedef struct ParserState ParserState;

static BOOL Parse(struct YogEnv*, YogVal, int_t, YogVal, YogVal*);
static YogVal LemonParser_new(YogEnv*);
static void ParseTrace(FILE*, char*);

static void
YogNode_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogNode* node = PTR_AS(YogNode, ptr);

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
        KEEP(break_.exprs);
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
        KEEP(except_body.types);
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
    case NODE_MODULE:
        KEEP(module.stmts);
        break;
    case NODE_MULTI_ASSIGN:
        KEEP(multi_assign.lhs);
        KEEP(multi_assign.rhs);
        break;
    case NODE_MULTI_ASSIGN_LHS:
        KEEP(multi_assign_lhs.left);
        KEEP(multi_assign_lhs.middle);
        KEEP(multi_assign_lhs.right);
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
        KEEP(return_.exprs);
        break;
    case NODE_SET:
        KEEP(set.elems);
        break;
    case NODE_SUBSCRIPT:
        KEEP(subscript.prefix);
        KEEP(subscript.index);
        break;
    case NODE_SUPER:
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
Super_new(YogEnv* env, uint_t lineno)
{
    SAVE_LOCALS(env);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    node = YogNode_new(env, NODE_SUPER, lineno);

    RETURN(env, node);
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
Params_new(YogEnv* env, YogVal params_without_default, YogVal params_with_default, YogVal var_param, YogVal kw_param, YogVal block_param)
{
    SAVE_ARGS5(env, params_without_default, params_with_default, var_param, kw_param, block_param);

    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

    array = YogArray_new(env);
    if (IS_PTR(params_without_default)) {
        YogArray_extend(env, array, params_without_default);
    }
    if (IS_PTR(params_with_default)) {
        YogArray_extend(env, array, params_with_default);
    }
    if (IS_PTR(var_param)) {
        YogArray_push(env, array, var_param);
    }
    if (IS_PTR(kw_param)) {
        YogArray_push(env, array, kw_param);
    }
    if (IS_PTR(block_param)) {
        YogArray_push(env, array, block_param);
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
ExceptBody_new(YogEnv* env, uint_t lineno, YogVal types, ID var, YogVal stmts)
{
    SAVE_ARGS2(env, types, stmts);

    YogVal node = YogNode_new(env, NODE_EXCEPT_BODY, lineno);
    NODE(node)->u.except_body.types = types;
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
Break_new(YogEnv* env, uint_t lineno, YogVal exprs)
{
    SAVE_ARG(env, exprs);

    YogVal node = YogNode_new(env, NODE_BREAK, lineno);
    NODE(node)->u.break_.exprs = exprs;

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
Return_new(YogEnv* env, uint_t lineno, YogVal exprs)
{
    SAVE_ARG(env, exprs);

    YogVal node = YogNode_new(env, NODE_RETURN, lineno);
    NODE(node)->u.return_.exprs = exprs;

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

static YogVal
MultiAssign_new(YogEnv* env, uint_t lineno, YogVal lhs, YogVal rhs)
{
    SAVE_ARGS2(env, lhs, rhs);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    node = YogNode_new(env, NODE_MULTI_ASSIGN, lineno);
    NODE(node)->u.multi_assign.lhs = lhs;
    NODE(node)->u.multi_assign.rhs = rhs;

    RETURN(env, node);
}

static YogVal
MultiAssignLhs_new(YogEnv* env, uint_t lineno, YogVal left, YogVal middle, YogVal right)
{
    SAVE_ARGS3(env, left, middle, right);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    node = YogNode_new(env, NODE_MULTI_ASSIGN_LHS, lineno);
    NODE(node)->u.multi_assign_lhs.left = left;
    NODE(node)->u.multi_assign_lhs.middle = middle;
    NODE(node)->u.multi_assign_lhs.right = right;

    RETURN(env, node);
}

#define TOKEN(token)            PTR_AS(YogToken, (token))
#define TOKEN_ID(token)         TOKEN((token))->u.id
#define TOKEN_LINENO(token)     TOKEN((token))->lineno
#define NODE_LINENO(node)       PTR_AS(YogNode, (node))->lineno
}   // end of %include

module ::= stmts(A). {
    *pval = A;
}

stmts(A) ::= stmt(B). {
    A = make_array_with(env, B);
}
stmts(A) ::= stmts(B) NEWLINE stmt(C). {
    A = Array_push(env, B, C);
}

stmt(A) ::= /* empty */. {
    A = YNIL;
}
stmt(A) ::= func_def(B). {
    A = B;
}
stmt(A) ::= expr(B). {
    A = B;
}
stmt(A) ::= multi_assign_lhs(B) EQUAL exprs(C). {
    A = MultiAssign_new(env, TOKEN_LINENO(B), B, C);
}
stmt(A) ::= TRY(B) stmts(C) excepts(D) ELSE stmts(E) finally_opt(F) END. {
    uint_t lineno = TOKEN_LINENO(B);
    A = ExceptFinally_new(env, lineno, C, D, E, F);
}
stmt(A) ::= TRY(B) stmts(C) excepts(D) finally_opt(E) END. {
    uint_t lineno = TOKEN_LINENO(B);
    A = ExceptFinally_new(env, lineno, C, D, YNIL, E);
}
stmt(A) ::= TRY(B) stmts(C) FINALLY stmts(D) END. {
    uint_t lineno = TOKEN_LINENO(B);
    A = Finally_new(env, lineno, C, D);
}
stmt(A) ::= WHILE(B) expr(C) NEWLINE stmts(D) END. {
    uint_t lineno = TOKEN_LINENO(B);
    A = While_new(env, lineno, C, D);
}
stmt(A) ::= BREAK(B). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Break_new(env, lineno, YNIL);
}
stmt(A) ::= BREAK(B) exprs(C). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Break_new(env, lineno, C);
}
stmt(A) ::= NEXT(B). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Next_new(env, lineno, YNIL);
}
stmt(A) ::= NEXT(B) expr(C). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Next_new(env, lineno, C);
}
stmt(A) ::= RETURN(B). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Return_new(env, lineno, YNIL);
}
stmt(A) ::= RETURN(B) exprs(C). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Return_new(env, lineno, C);
}
stmt(A) ::= IF(B) expr(C) NEWLINE stmts(D) if_tail(E) END. {
    uint_t lineno = TOKEN_LINENO(B);
    A = If_new(env, lineno, C, D, E);
}
stmt(A) ::= decorators_opt(F) CLASS(B) NAME(C) super_opt(D) NEWLINE stmts(E) END. {
    uint_t lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = Class_new(env, lineno, F, id, D, E);
}
stmt(A) ::= MODULE(B) NAME(C) stmts(D) END. {
    uint_t lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = Module_new(env, lineno, id, D);
}
stmt(A) ::= NONLOCAL(B) names(C). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Nonlocal_new(env, lineno, C);
}
stmt(A) ::= IMPORT(B) dotted_names(C). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Import_new(env, lineno, C);
}
stmt(A) ::= RAISE(B) expr(C). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Raise_new(env, lineno, C);
}

multi_assign_lhs(A) ::= postfix_exprs(B) COMMA postfix_expr(C). {
    YogArray_push(env, B, C);
    A = MultiAssignLhs_new(env, TOKEN_LINENO(B), B, YNIL, YNIL);
}
multi_assign_lhs(A) ::= postfix_exprs(B) COMMA multi_assign_lhs_middle(C). {
    A = MultiAssignLhs_new(env, TOKEN_LINENO(B), B, C, YNIL);
}
multi_assign_lhs(A) ::= postfix_exprs(B) COMMA multi_assign_lhs_middle(C) COMMA postfix_exprs(D). {
    A = MultiAssignLhs_new(env, TOKEN_LINENO(B), B, C, D);
}
multi_assign_lhs(A) ::= multi_assign_lhs_middle(B). {
    A = MultiAssignLhs_new(env, TOKEN_LINENO(B), YNIL, B, YNIL);
}
multi_assign_lhs(A) ::= multi_assign_lhs_middle(B) COMMA postfix_exprs(C). {
    A = MultiAssignLhs_new(env, TOKEN_LINENO(B), YNIL, B, C);
}

postfix_exprs(A) ::= postfix_expr(B). {
    A = make_array_with(env, B);
}
postfix_exprs(A) ::= postfix_exprs(B) COMMA postfix_expr(C). {
    YogArray_push(env, B, C);
    A = B;
}

multi_assign_lhs_middle(A) ::= STAR postfix_expr(B). {
    A = B;
}

dotted_names(A) ::= dotted_name(B). {
    A = make_array_with(env, B);
}
dotted_names(A) ::= dotted_names(B) COMMA dotted_name(C). {
    A = Array_push(env, B, C);
}

dotted_name(A) ::= NAME(B). {
    A = id_token2array(env, B);
}
dotted_name(A) ::= dotted_name(B) DOT NAME(C). {
    A = Array_push_token_id(env, B, C);
}

names(A) ::= NAME(B). {
    A = id_token2array(env, B);
}
names(A) ::= names(B) COMMA NAME(C). {
    A = Array_push_token_id(env, B, C);
}

super_opt(A) ::= /* empty */. {
    A = YNIL;
}
super_opt(A) ::= GREATER expr(B). {
    A = B;
}

if_tail(A) ::= else_opt(B). {
    A = B;
}
if_tail(A) ::= ELIF(B) expr(C) NEWLINE stmts(D) if_tail(E). {
    uint_t lineno = TOKEN_LINENO(B);
    YogVal node = If_new(env, lineno, C, D, E);
    A = make_array_with(env, node);
}

else_opt(A) ::= /* empty */. {
    A = YNIL;
}
else_opt(A) ::= ELSE stmts(B). {
    A = B;
}

func_def(A) ::= decorators_opt(F) DEF(B) NAME(C) LPAR params(D) RPAR stmts(E) END. {
    uint_t lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = FuncDef_new(env, lineno, F, id, D, E);
}

decorators_opt(A) ::= /* empty */. {
    A = YNIL;
}
decorators_opt(A) ::= decorators(B). {
    A = B;
}

decorators(A) ::= decorator(B). {
    A = make_array_with(env, B);
}
decorators(A) ::= decorators(B) decorator(C). {
    A = Array_push(env, B, C);
}

decorator(A) ::= AT expr(B) NEWLINE. {
    A = B;
}

params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA var_param(D) COMMA kw_param(E) COMMA block_param(F). {
    A = Params_new(env, B, C, D, E, F);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA var_param(D) COMMA block_param(E). {
    A = Params_new(env, B, C, D, YNIL, E);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA kw_param(D) COMMA block_param(E). {
    A = Params_new(env, B, C, YNIL, D, E);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA block_param(D). {
    A = Params_new(env, B, C, YNIL, YNIL, D);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA var_param(D) COMMA kw_param(E). {
    A = Params_new(env, B, C, D, E, YNIL);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA var_param(D). {
    A = Params_new(env, B, C, D, YNIL, YNIL);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA kw_param(D). {
    A = Params_new(env, B, C, YNIL, D, YNIL);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C). {
    A = Params_new(env, B, C, YNIL, YNIL, YNIL);
}
params(A) ::= params_without_default(B) COMMA var_param(C) COMMA kw_param(D) COMMA block_param(E). {
    A = Params_new(env, B, YNIL, C, D, E);
}
params(A) ::= params_without_default(B) COMMA var_param(C) COMMA block_param(D). {
    A = Params_new(env, B, YNIL, C, YNIL, D);
}
params(A) ::= params_without_default(B) COMMA kw_param(C) COMMA block_param(D). {
    A = Params_new(env, B, YNIL, YNIL, C, D);
}
params(A) ::= params_without_default(B) COMMA block_param(C). {
    A = Params_new(env, B, YNIL, YNIL, YNIL, C);
}
params(A) ::= params_without_default(B) COMMA var_param(C) COMMA kw_param(D). {
    A = Params_new(env, B, YNIL, C, D, YNIL);
}
params(A) ::= params_without_default(B) COMMA var_param(C). {
    A = Params_new(env, B, YNIL, C, YNIL, YNIL);
}
params(A) ::= params_without_default(B) COMMA kw_param(C). {
    A = Params_new(env, B, YNIL, YNIL, C, YNIL);
}
params(A) ::= params_without_default(B). {
    A = Params_new(env, B, YNIL, YNIL, YNIL, YNIL);
}
params(A) ::= params_with_default(B) COMMA var_param(C) COMMA kw_param(D) COMMA block_param(E). {
    A = Params_new(env, YNIL, B, C, D, E);
}
params(A) ::= params_with_default(B) COMMA var_param(C) COMMA block_param(D). {
    A = Params_new(env, YNIL, B, C, YNIL, D);
}
params(A) ::= params_with_default(B) COMMA kw_param(C) COMMA block_param(D). {
    A = Params_new(env, YNIL, B, YNIL, C, D);
}
params(A) ::= params_with_default(B) COMMA block_param(C). {
    A = Params_new(env, YNIL, B, YNIL, YNIL, C);
}
params(A) ::= params_with_default(B) COMMA var_param(C) COMMA kw_param(D). {
    A = Params_new(env, YNIL, B, C, D, YNIL);
}
params(A) ::= params_with_default(B) COMMA var_param(C). {
    A = Params_new(env, YNIL, B, C, YNIL, YNIL);
}
params(A) ::= params_with_default(B) COMMA kw_param(C). {
    A = Params_new(env, YNIL, B, YNIL, C, YNIL);
}
params(A) ::= params_with_default(B). {
    A = Params_new(env, YNIL, B, YNIL, YNIL, YNIL);
}
params(A) ::= var_param(B) COMMA kw_param(C) COMMA block_param(D). {
    A = Params_new(env, YNIL, YNIL, B, C, D);
}
params(A) ::= var_param(B) COMMA block_param(C). {
    A = Params_new(env, YNIL, YNIL, B, YNIL, C);
}
params(A) ::= kw_param(B) COMMA block_param(C). {
    A = Params_new(env, YNIL, YNIL, YNIL, B, C);
}
params(A) ::= block_param(B). {
    A = Params_new(env, YNIL, YNIL, YNIL, YNIL, B);
}
params(A) ::= var_param(B) COMMA kw_param(C). {
    A = Params_new(env, YNIL, YNIL, B, C, YNIL);
}
params(A) ::= var_param(B). {
    A = Params_new(env, YNIL, YNIL, B, YNIL, YNIL);
}
params(A) ::= kw_param(B). {
    A = Params_new(env, YNIL, YNIL, YNIL, B, YNIL);
}
params(A) ::= /* empty */. {
    A = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}

kw_param(A) ::= STAR_STAR(B) NAME(C). {
    uint_t lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}

var_param(A) ::= STAR(B) NAME(C). {
    uint_t lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}

block_param(A) ::= AND(B) NAME(C) param_default_opt(D). {
    uint_t lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = Param_new(env, NODE_BLOCK_PARAM, lineno, id, D);
}

param_default_opt(A) ::= /* empty */. {
    A = YNIL;
}
param_default_opt(A) ::= param_default(B). {
    A = B;
}

param_default(A) ::= EQUAL expr(B). {
    A = B;
}

params_without_default(A) ::= NAME(B). {
    A = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, B)->u.id;
    ParamArray_push(env, A, lineno, id, YNIL);
}
params_without_default(A) ::= params_without_default(B) COMMA NAME(C). {
    uint_t lineno = TOKEN_LINENO(C);
    ID id = PTR_AS(YogToken, C)->u.id;
    ParamArray_push(env, B, lineno, id, YNIL);
    A = B;
}

params_with_default(A) ::= param_with_default(B). {
    A = make_array_with(env, B);
}
params_with_default(A) ::= params_with_default(B) COMMA param_with_default(C). {
    A = Array_push(env, B, C);
}

param_with_default(A) ::= NAME(B) param_default(C). {
    uint_t lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, B)->u.id;
    A = Param_new(env, NODE_PARAM, lineno, id, C);
}

args(A) ::= /* empty */. {
    A = YNIL;
}
args(A) ::= posargs(B). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, B, YNIL, YNIL, YNIL);
}
args(A) ::= posargs(B) COMMA kwargs(C). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, B, C, YNIL, YNIL);
}
args(A) ::= posargs(B) COMMA kwargs(C) COMMA vararg(D). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, B, C, D, YNIL);
}
args(A) ::= posargs(B) COMMA kwargs(C) COMMA vararg(D) COMMA varkwarg (E). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, B, C, D, E);
}
args(A) ::= posargs(B) COMMA vararg(C). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, B, YNIL, C, YNIL);
}
args(A) ::= posargs(B) COMMA vararg(C) COMMA varkwarg(D). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, B, YNIL, C, D);
}
args(A) ::= posargs(B) COMMA varkwarg(C). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, B, YNIL, YNIL, C);
}
args(A) ::= kwargs(B). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, YNIL, B, YNIL, YNIL);
}
args(A) ::= kwargs(B) COMMA vararg(C). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, YNIL, B, C, YNIL);
}
args(A) ::= kwargs(B) COMMA vararg(C) COMMA varkwarg(D). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, YNIL, B, C, D);
}
args(A) ::= kwargs(B) COMMA varkwarg(C). {
    uint_t lineno = NODE_LINENO(YogArray_at(env, B, 0));
    A = Args_new(env, lineno, YNIL, B, YNIL, C);
}
args(A) ::= vararg(B). {
    uint_t lineno = NODE_LINENO(B);
    A = Args_new(env, lineno, YNIL, YNIL, B, YNIL);
}
args(A) ::= vararg(B) COMMA varkwarg(C). {
    uint_t lineno = NODE_LINENO(B);
    A = Args_new(env, lineno, YNIL, YNIL, B, C);
}
args(A) ::= varkwarg(B). {
    uint_t lineno = NODE_LINENO(B);
    A = Args_new(env, lineno, YNIL, YNIL, YNIL, B);
}

varkwarg(A) ::= STAR_STAR expr(B). {
    A = B;
}

vararg(A) ::= STAR expr(B). {
    A = B;
}

posargs(A) ::= expr(B). {
    A = make_array_with(env, B);
}
posargs(A) ::= posargs(B) COMMA expr(C). {
    YogArray_push(env, B, C);
    A = B;
}

kwargs(A) ::= kwarg(B). {
    A = make_array_with(env, B);
}
kwargs(A) ::= kwargs(B) COMMA kwarg(C). {
    YogArray_push(env, B, C);
    A = B;
}

kwarg(A) ::= NAME(B) COLON expr(C). {
    A = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(B));
    PTR_AS(YogNode, A)->u.kwarg.name = PTR_AS(YogToken, B)->u.id;
    PTR_AS(YogNode, A)->u.kwarg.value = C;
}

expr(A) ::= assign_expr(B). {
    A = B;
}

assign_expr(A) ::= postfix_expr(B) EQUAL logical_or_expr(C). {
    uint_t lineno = NODE_LINENO(B);
    A = Assign_new(env, lineno, B, C);
}
assign_expr(A) ::= postfix_expr(B) augmented_assign_op(C) logical_or_expr(D). {
    A = AugmentedAssign_new(env, NODE_LINENO(B), B, VAL2ID(C), D);
}
assign_expr(A) ::= postfix_expr(B) AND_AND_EQUAL logical_or_expr(C). {
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    uint_t lineno = NODE_LINENO(B);
    expr = LogicalAnd_new(env, lineno, B, C);
    assign = Assign_new(env, lineno, B, expr);

    POP_LOCALS(env);

    A = assign;
}
assign_expr(A) ::= postfix_expr(B) BAR_BAR_EQUAL logical_or_expr(C). {
    YogVal expr = YUNDEF;
    YogVal assign = YUNDEF;
    PUSH_LOCALS2(env, expr, assign);

    uint_t lineno = NODE_LINENO(B);
    expr = LogicalOr_new(env, lineno, B, C);
    assign = Assign_new(env, lineno, B, expr);

    POP_LOCALS(env);

    A = assign;
}
assign_expr(A) ::= logical_or_expr(B). {
    A = B;
}

augmented_assign_op(A) ::= PLUS_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "+"));
}
augmented_assign_op(A) ::= MINUS_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "-"));
}
augmented_assign_op(A) ::= STAR_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "*"));
}
augmented_assign_op(A) ::= DIV_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "/"));
}
augmented_assign_op(A) ::= DIV_DIV_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "//"));
}
augmented_assign_op(A) ::= PERCENT_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "%"));
}
augmented_assign_op(A) ::= BAR_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "|"));
}
augmented_assign_op(A) ::= AND_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "&"));
}
augmented_assign_op(A) ::= XOR_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "^"));
}
augmented_assign_op(A) ::= STAR_STAR_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "**"));
}
augmented_assign_op(A) ::= LSHIFT_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "<<"));
}
augmented_assign_op(A) ::= RSHIFT_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, ">>"));
}

logical_or_expr(A) ::= logical_and_expr(B). {
    A = B;
}
logical_or_expr(A) ::= logical_or_expr(B) BAR_BAR logical_and_expr(C). {
    A = LogicalOr_new(env, NODE_LINENO(B), B, C);
}

logical_and_expr(A) ::= not_expr(B). {
    A = B;
}
logical_and_expr(A) ::= logical_and_expr(B) AND_AND not_expr(C). {
    A = LogicalAnd_new(env, NODE_LINENO(B), B, C);
}

not_expr(A) ::= comparison(B). {
    A = B;
}
not_expr(A) ::= NOT(B) not_expr(C). {
    A = YogNode_new(env, NODE_NOT, NODE_LINENO(B));
    NODE(A)->u.not.expr = C;
}

comparison(A) ::= xor_expr(B). {
    A = B;
}
comparison(A) ::= xor_expr(B) comp_op(C) xor_expr(D). {
    A = FuncCall_new2(env, NODE_LINENO(B), B, VAL2ID(C), D);
}

comp_op(A) ::= EQUAL_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "=="));
}
comp_op(A) ::= NOT_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "!="));
}
comp_op(A) ::= LESS. {
    A = ID2VAL(YogVM_intern(env, env->vm, "<"));
}
comp_op(A) ::= LESS_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "<="));
}
comp_op(A) ::= GREATER. {
    A = ID2VAL(YogVM_intern(env, env->vm, ">"));
}
comp_op(A) ::= GREATER_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, ">="));
}

xor_expr(A) ::= or_expr(B). {
    A = B;
}
xor_expr(A) ::= xor_expr(B) XOR(C) or_expr(D). {
    A = FuncCall_new2(env, NODE_LINENO(B), B, TOKEN_ID(C), D);
}

or_expr(A) ::= and_expr(B). {
    A = B;
}
or_expr(A) ::= or_expr(B) BAR(C) and_expr(D). {
    A = FuncCall_new2(env, NODE_LINENO(B), B, TOKEN_ID(C), D);
}

and_expr(A) ::= shift_expr(B). {
    A = B;
}
and_expr(A) ::= and_expr(B) AND(C) shift_expr(D). {
    A = FuncCall_new2(env, NODE_LINENO(B), B, TOKEN_ID(C), D);
}

shift_expr(A) ::= match_expr(B). {
    A = B;
}
shift_expr(A) ::= shift_expr(B) shift_op(C) match_expr(D). {
    uint_t lineno = NODE_LINENO(B);
    A = FuncCall_new2(env, lineno, B, VAL2ID(C), D);
}

shift_op(A) ::= LSHIFT(B). {
    A = ID2VAL(PTR_AS(YogToken, B)->u.id);
}
shift_op(A) ::= RSHIFT(B). {
    A = ID2VAL(PTR_AS(YogToken, B)->u.id);
}

match_expr(A) ::= arith_expr(B). {
    A = B;
}
match_expr(A) ::= match_expr(B) EQUAL_TILDA(C) arith_expr(D). {
    uint_t lineno = NODE_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = FuncCall_new2(env, lineno, B, id, D);
}

arith_expr(A) ::= term(B). {
    A = B;
}
arith_expr(A) ::= arith_expr(B) arith_op(C) term(D). {
    uint_t lineno = NODE_LINENO(B);
    A = FuncCall_new2(env, lineno, B, VAL2ID(C), D);
}

arith_op(A) ::= PLUS(B). {
    A = ID2VAL(PTR_AS(YogToken, B)->u.id);
}
arith_op(A) ::= MINUS(B). {
    A = ID2VAL(PTR_AS(YogToken, B)->u.id);
}

term(A) ::= term(B) term_op(C) factor(D). {
    uint_t lineno = NODE_LINENO(B);
    A = FuncCall_new2(env, lineno, B, VAL2ID(C), D);
}
term(A) ::= factor(B). {
    A = B;
}

term_op(A) ::= STAR(B). {
    A = ID2VAL(PTR_AS(YogToken, B)->u.id);
}
term_op(A) ::= DIV(B). {
    A = ID2VAL(PTR_AS(YogToken, B)->u.id);
}
term_op(A) ::= DIV_DIV(B). {
    A = ID2VAL(PTR_AS(YogToken, B)->u.id);
}
term_op(A) ::= PERCENT(B). {
    A = ID2VAL(PTR_AS(YogToken, B)->u.id);
}

factor(A) ::= PLUS(B) factor(C). {
    uint_t lineno = NODE_LINENO(B);
    ID id = YogVM_intern(env, env->vm, "+self");
    A = FuncCall_new3(env, lineno, C, id);
}
factor(A) ::= MINUS(B) factor(C). {
    uint_t lineno = NODE_LINENO(B);
    ID id = YogVM_intern(env, env->vm, "-self");
    A = FuncCall_new3(env, lineno, C, id);
}
factor(A) ::= TILDA(B) factor(C). {
    uint_t lineno = NODE_LINENO(B);
    ID id = YogVM_intern(env, env->vm, "~self");
    A = FuncCall_new3(env, lineno, C, id);
}
factor(A) ::= power(B). {
    A = B;
}

power(A) ::= postfix_expr(B). {
    A = B;
}
power(A) ::= postfix_expr(B) STAR_STAR factor(C). {
    uint_t lineno = NODE_LINENO(B);
    ID id = YogVM_intern(env, env->vm, "**");
    A = FuncCall_new2(env, lineno, B, id, C);
}

postfix_expr(A) ::= atom(B). {
    A = B;
}
postfix_expr(A) ::= postfix_expr(B) LPAR args(C) RPAR blockarg_opt(D). {
    A = FuncCall_new(env, NODE_LINENO(B), B, C, D);
}
postfix_expr(A) ::= postfix_expr(B) LBRACKET expr(C) RBRACKET. {
    uint_t lineno = NODE_LINENO(B);
    A = Subscript_new(env, lineno, B, C);
}
postfix_expr(A) ::= postfix_expr(B) DOT name(C). {
    uint_t lineno = NODE_LINENO(B);
    A = Attr_new(env, lineno, B, VAL2ID(C));
}

name(A) ::= NAME(B). {
    A = ID2VAL(PTR_AS(YogToken, B)->u.id);
}
name(A) ::= EQUAL_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "=="));
}
name(A) ::= NOT_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "!="));
}
name(A) ::= LESS. {
    A = ID2VAL(YogVM_intern(env, env->vm, "<"));
}
name(A) ::= LESS_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, "<="));
}
name(A) ::= GREATER. {
    A = ID2VAL(YogVM_intern(env, env->vm, ">"));
}
name(A) ::= GREATER_EQUAL. {
    A = ID2VAL(YogVM_intern(env, env->vm, ">="));
}

atom(A) ::= NAME(B). {
    uint_t lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, B)->u.id;
    A = Variable_new(env, lineno, id);
}
atom(A) ::= NUMBER(B). {
    uint_t lineno = TOKEN_LINENO(B);
    YogVal val = PTR_AS(YogToken, B)->u.val;
    A = Literal_new(env, lineno, val);
}
atom(A) ::= REGEXP(B). {
    uint_t lineno = TOKEN_LINENO(B);
    YogVal val = PTR_AS(YogToken, B)->u.val;
    A = Literal_new(env, lineno, val);
}
atom(A) ::= STRING(B). {
    uint_t lineno = TOKEN_LINENO(B);
    YogVal val = PTR_AS(YogToken, B)->u.val;
    A = Literal_new(env, lineno, val);
}
atom(A) ::= SYMBOL(B). {
    uint_t lineno = TOKEN_LINENO(B);
    YogVal val = PTR_AS(YogToken, B)->u.val;
    A = Literal_new(env, lineno, val);
}
atom(A) ::= NIL(B). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Literal_new(env, lineno, YNIL);
}
atom(A) ::= TRUE(B). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Literal_new(env, lineno, YTRUE);
}
atom(A) ::= FALSE(B). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Literal_new(env, lineno, YFALSE);
}
atom(A) ::= SUPER(B). {
    uint_t lineno = TOKEN_LINENO(B);
    A = Super_new(env, lineno);
}
atom(A) ::= LINE(B). {
    uint_t lineno = PTR_AS(YogToken, B)->lineno;
    YogVal val = INT2VAL(lineno);
    A = Literal_new(env, lineno, val);
}
atom(A) ::= LBRACKET(B) exprs(C) RBRACKET. {
    A = Array_new(env, NODE_LINENO(B), C);
}
atom(A) ::= LBRACKET(B) RBRACKET. {
    A = Array_new(env, NODE_LINENO(B), YNIL);
}
atom(A) ::= LBRACE(B) RBRACE . {
    A = Dict_new(env, NODE_LINENO(B), YNIL);
}
atom(A) ::= LBRACE(B) dict_elems(C) comma_opt RBRACE. {
    A = Dict_new(env, NODE_LINENO(B), C);
}
atom(A) ::= LBRACE(B) exprs(C) RBRACE. {
    A = Set_new(env, NODE_LINENO(B), C);
}
atom(A) ::= LPAR expr(B) RPAR. {
    A = B;
}

exprs(A) ::= expr(B). {
    A = make_array_with(env, B);
}
exprs(A) ::= exprs(B) COMMA expr(C). {
    YogArray_push(env, B, C);
    A = B;
}

dict_elems(A) ::= dict_elem(B). {
    A = make_array_with(env, B);
}
dict_elems(A) ::= dict_elems(B) COMMA dict_elem(C). {
    YogArray_push(env, B, C);
    A = B;
}
dict_elem(A) ::= expr(B) EQUAL_GREATER expr(C). {
    A = DictElem_new(env, NODE_LINENO(B), B, C);
}

comma_opt(A) ::= /* empty */. {
    A = YNIL;
}
comma_opt(A) ::= COMMA(B). {
    A = B;
}

blockarg_opt(A) ::= /* empty */. {
    A = YNIL;
}
blockarg_opt(A) ::= DO(B) blockarg_params_opt(C) NEWLINE stmts(D) END. {
    uint_t lineno = TOKEN_LINENO(B);
    A = BlockArg_new(env, lineno, C, D);
}
blockarg_opt(A) ::= LBRACE(B) blockarg_params_opt(C) NEWLINE stmts(D) RBRACE. {
    uint_t lineno = TOKEN_LINENO(B);
    A = BlockArg_new(env, lineno, C, D);
}

blockarg_params_opt(A) ::= /* empty */. {
    A = YNIL;
}
blockarg_params_opt(A) ::= LBRACKET params(B) RBRACKET. {
    A = B;
}

excepts(A) ::= except(B). {
    A = make_array_with(env, B);
}
excepts(A) ::= excepts(B) except(C). {
    A = Array_push(env, B, C);
}

except(A) ::= EXCEPT(B) exprs(C) AS NAME(D) NEWLINE stmts(E). {
    uint_t lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, D)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    A = ExceptBody_new(env, lineno, C, id, E);
}
except(A) ::= EXCEPT(B) exprs(C) NEWLINE stmts(E). {
    uint_t lineno = TOKEN_LINENO(B);
    A = ExceptBody_new(env, lineno, C, NO_EXC_VAR, E);
}
except(A) ::= EXCEPT(B) NEWLINE stmts(C). {
    uint_t lineno = TOKEN_LINENO(B);
    A = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, C);
}

finally_opt(A) ::= /* empty */. {
    A = YNIL;
}
finally_opt(A) ::= FINALLY stmts(B). {
    A = B;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
