
%token_prefix TK_

%token_type { YogVal }
%default_type { YogVal }

%extra_argument { YogVal* pval }

%include {
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "yog/error.h"
#include "yog/parser.h"
#include "yog/yog.h"

typedef struct ParserState ParserState;

static void Parse(struct YogEnv*, struct YogVal, int, YogVal, YogVal*);
static YogVal LemonParser_new(YogEnv*);
static void ParseTrace(FILE*, char*);

static void 
YogNode_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogNode* node = ptr;

#define KEEP(member)    do { \
    node->u.member = YogVal_keep(env, node->u.member, keeper); \
} while (0)
    switch (node->type) {
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
    case NODE_KLASS:
        KEEP(klass.super);
        KEEP(klass.stmts);
        break;
    case NODE_LITERAL:
        KEEP(literal.val);
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
YogNode_new(YogEnv* env, YogNodeType type, unsigned int lineno) 
{
    YogNode* node = ALLOC_OBJ(env, YogNode_keep_children, NULL, YogNode);
    node->lineno = lineno;
    node->type = type;

    return PTR2VAL(node);
}

#define NODE_NEW(type, lineno)  YogNode_new(env, (type), (lineno))
#define NODE(v)                 PTR_AS(YogNode, (v))

static YogVal 
Literal_new(YogEnv* env, unsigned int lineno, YogVal val) 
{
    SAVE_ARG(env, val);

    YogVal node = YogNode_new(env, NODE_LITERAL, lineno);
    MODIFY(env, NODE(node)->u.literal.val, val);

    RETURN(env, node);
}

static YogVal 
BlockArg_new(YogEnv* env, unsigned int lineno, YogVal params, YogVal stmts) 
{
    SAVE_ARGS2(env, params, stmts);

    YogVal node = YogNode_new(env, NODE_BLOCK_ARG, lineno);
    MODIFY(env, NODE(node)->u.blockarg.params, params);
    MODIFY(env, NODE(node)->u.blockarg.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Params_new(YogEnv* env, YogVal params_without_default, YogVal params_with_default, YogVal block_param, YogVal var_param, YogVal kw_param) 
{
    SAVE_ARGS5(env, params_without_default, params_with_default, block_param, var_param, kw_param);

    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

    array = YogArray_new(env);
    if (IS_OBJ(params_without_default)) {
        YogArray_extend(env, array, params_without_default);
    }
    if (IS_OBJ(params_with_default)) {
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
CommandCall_new(YogEnv* env, unsigned int lineno, ID name, YogVal args, YogVal blockarg) 
{
    SAVE_ARGS2(env, args, blockarg);

    YogVal node = YogNode_new(env, NODE_COMMAND_CALL, lineno);
    NODE(node)->u.command_call.name = name;
    MODIFY(env, NODE(node)->u.command_call.args, args);
    MODIFY(env, NODE(node)->u.command_call.blockarg, blockarg);

    RETURN(env, node);
}

static YogVal 
Array_new(YogEnv* env, YogVal elem) 
{
    SAVE_ARG(env, elem);

    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

    if (IS_PTR(elem)) {
        array = YogArray_new(env);
        YogArray_push(env, array, elem);
    }
    else {
        array = YNIL;
    }

    RETURN(env, array);
}

static YogVal 
Array_push(YogEnv* env, YogVal array, YogVal elem) 
{
    SAVE_ARGS2(env, array, elem);

    if (IS_PTR(elem)) {
        if (!IS_OBJ(array)) {
            array = YogArray_new(env);
        }
        YogArray_push(env, array, elem);
    }

    RETURN(env, array);
}

static YogVal 
Param_new(YogEnv* env, YogNodeType type, unsigned int lineno, ID id, YogVal default_) 
{
    SAVE_ARG(env, default_);

    YogVal node = YogNode_new(env, type, lineno);
    NODE(node)->u.param.name = id;
    MODIFY(env, NODE(node)->u.param.default_, default_);

    RETURN(env, node);
}

static void 
ParamArray_push(YogEnv* env, YogVal array, unsigned int lineno, ID id, YogVal default_) 
{
    SAVE_ARGS2(env, array, default_);

    YogVal node = Param_new(env, NODE_PARAM, lineno, id, default_);
    YogArray_push(env, array, node);

    RETURN_VOID(env);
}

static YogVal 
FuncDef_new(YogEnv* env, unsigned int lineno, ID name, YogVal params, YogVal stmts) 
{
    SAVE_ARGS2(env, params, stmts);

    YogVal node = YogNode_new(env, NODE_FUNC_DEF, lineno);
    NODE(node)->u.funcdef.name = name;
    MODIFY(env, NODE(node)->u.funcdef.params, params);
    MODIFY(env, NODE(node)->u.funcdef.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
FuncCall_new(YogEnv* env, unsigned int lineno, YogVal callee, YogVal args, YogVal blockarg) 
{
    SAVE_ARGS3(env, callee, args, blockarg);

    YogVal node = NODE_NEW(NODE_FUNC_CALL, lineno);
    MODIFY(env, NODE(node)->u.func_call.callee, callee);
    MODIFY(env, NODE(node)->u.func_call.args, args);
    MODIFY(env, NODE(node)->u.func_call.blockarg, blockarg);

    RETURN(env, node);
}

static YogVal 
Variable_new(YogEnv* env, unsigned int lineno, ID id) 
{
    YogVal node = NODE_NEW(NODE_VARIABLE, lineno);
    NODE(node)->u.variable.id = id;

    return node;
}

static YogVal 
ExceptBody_new(YogEnv* env, unsigned int lineno, YogVal type, ID var, YogVal stmts) 
{
    SAVE_ARGS2(env, type, stmts);

    YogVal node = NODE_NEW(NODE_EXCEPT_BODY, lineno);
    MODIFY(env, NODE(node)->u.except_body.type, type);
    NODE(node)->u.except_body.var = var;
    MODIFY(env, NODE(node)->u.except_body.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Except_new(YogEnv* env, unsigned int lineno, YogVal head, YogVal excepts, YogVal else_) 
{
    SAVE_ARGS3(env, head, excepts, else_);

    YogVal node = NODE_NEW(NODE_EXCEPT, lineno);
    MODIFY(env, NODE(node)->u.except.head, head);
    MODIFY(env, NODE(node)->u.except.excepts, excepts);
    MODIFY(env, NODE(node)->u.except.else_, else_);

    RETURN(env, node);
}

static YogVal 
Finally_new(YogEnv* env, unsigned int lineno, YogVal head, YogVal body) 
{
    SAVE_ARGS2(env, head, body);

    YogVal node = NODE_NEW(NODE_FINALLY, lineno);
    MODIFY(env, NODE(node)->u.finally.head, head);
    MODIFY(env, NODE(node)->u.finally.body, body);

    RETURN(env, node);
}

static YogVal 
ExceptFinally_new(YogEnv* env, unsigned int lineno, YogVal stmts, YogVal excepts, YogVal else_, YogVal finally) 
{
    SAVE_ARGS4(env, stmts, excepts, else_, finally);

    YogVal except = YUNDEF;
    PUSH_LOCAL(env, except);

    except = Except_new(env, lineno, stmts, excepts, else_);

    YogVal node;
    if (IS_OBJ(finally)) {
        YogVal array = Array_new(env, except);
        node = Finally_new(env, lineno, array, finally);
    }
    else {
        node = except;
    }

    RETURN(env, node);
}

static YogVal 
Break_new(YogEnv* env, unsigned int lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = YogNode_new(env, NODE_BREAK, lineno);
    MODIFY(env, NODE(node)->u.break_.expr, expr);

    RETURN(env, node);
}

static YogVal 
Next_new(YogEnv* env, unsigned int lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = NODE_NEW(NODE_NEXT, lineno);
    MODIFY(env, NODE(node)->u.next.expr, expr);

    RETURN(env, node);
}

static YogVal 
Return_new(YogEnv* env, unsigned int lineno, YogVal expr) 
{
    SAVE_ARG(env, expr);

    YogVal node = NODE_NEW(NODE_RETURN, lineno);
    MODIFY(env, NODE(node)->u.return_.expr, expr);

    RETURN(env, node);
}

static YogVal 
MethodCall_new(YogEnv* env, unsigned int lineno, YogVal recv, ID name, YogVal args, YogVal blockarg) 
{
    SAVE_ARGS3(env, recv, args, blockarg);

    YogVal node = NODE_NEW(NODE_METHOD_CALL, lineno);
    MODIFY(env, NODE(node)->u.method_call.recv, recv);
    NODE(node)->u.method_call.name = name;
    MODIFY(env, NODE(node)->u.method_call.args, args);
    MODIFY(env, NODE(node)->u.method_call.blockarg, blockarg);

    RETURN(env, node);
}

#if 0
#define METHOD_CALL_NEW(node, lineno, recv_, name_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_METHOD_CALL, lineno); \
    NODE(node)->u.method_call.recv = recv_; \
    NODE(node)->u.method_call.name = name_; \
    NODE(node)->u.method_call.args = args_; \
    NODE(node)->u.method_call.blockarg = blockarg_; \
} while (0)
#endif

static YogVal 
MethodCall1_new(YogEnv* env, unsigned int lineno, YogVal recv, ID name, YogVal arg) 
{
    SAVE_ARGS2(env, recv, arg);

    YogVal args = YUNDEF;
    PUSH_LOCAL(env, args);
   
    args = YogArray_new(env);
    YogArray_push(env, args, arg);
   
    YogVal node = MethodCall_new(env, lineno, recv, name, args, YNIL);

    RETURN(env, node);
}

static YogVal 
If_new(YogEnv* env, unsigned int lineno, YogVal test, YogVal stmts, YogVal tail)
{
    SAVE_ARGS3(env, test, stmts, tail);

    YogVal node = YogNode_new(env, NODE_IF, lineno);
    MODIFY(env, NODE(node)->u.if_.test, test);
    MODIFY(env, NODE(node)->u.if_.stmts, stmts);
    MODIFY(env, NODE(node)->u.if_.tail, tail);

    RETURN(env, node);
}

static YogVal 
While_new(YogEnv* env, unsigned int lineno, YogVal test, YogVal stmts) 
{
    SAVE_ARGS2(env, test, stmts);

    YogVal node = YogNode_new(env, NODE_WHILE, lineno);
    MODIFY(env, NODE(node)->u.while_.test, test);
    MODIFY(env, NODE(node)->u.while_.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Klass_new(YogEnv* env, unsigned int lineno, ID name, YogVal super, YogVal stmts)
{
    SAVE_ARGS2(env, super, stmts);

    YogVal node = YogNode_new(env, NODE_KLASS, lineno);
    NODE(node)->u.klass.name = name;
    MODIFY(env, NODE(node)->u.klass.super, super);
    MODIFY(env, NODE(node)->u.klass.stmts, stmts);

    RETURN(env, node);
}

static YogVal 
Assign_new(YogEnv* env, unsigned int lineno, YogVal left, YogVal right) 
{
    SAVE_ARGS2(env, left, right);

    YogVal node = NODE_NEW(NODE_ASSIGN, lineno);
    MODIFY(env, NODE(node)->u.assign.left, left);
    MODIFY(env, NODE(node)->u.assign.right, right);

    RETURN(env, node);
}

static YogVal 
Subscript_new(YogEnv* env, unsigned int lineno, YogVal prefix, YogVal index) 
{
    SAVE_ARGS2(env, prefix, index);

    YogVal node = YogNode_new(env, NODE_SUBSCRIPT, lineno);
    MODIFY(env, NODE(node)->u.subscript.prefix, prefix);
    MODIFY(env, NODE(node)->u.subscript.index, index);

    RETURN(env, node);
}

static YogVal 
Attr_new(YogEnv* env, unsigned int lineno, YogVal obj, ID name) 
{
    SAVE_ARG(env, obj);

    YogVal node = YogNode_new(env, NODE_ATTR, lineno);
    MODIFY(env, NODE(node)->u.attr.obj, obj);
    NODE(node)->u.attr.name = name;

    RETURN(env, node);
}

static YogVal 
Nonlocal_new(YogEnv* env, unsigned int lineno, YogVal names) 
{
    SAVE_ARG(env, names);

    YogVal node = YogNode_new(env, NODE_NONLOCAL, lineno);
    MODIFY(env, NODE(node)->u.nonlocal.names, names);

    RETURN(env, node);
}

YogVal 
YogParser_parse_file(YogEnv* env, const char* filename, BOOL debug)
{
    SAVE_LOCALS(env);

    YogVal lexer = YUNDEF;
    YogVal ast = YUNDEF;
    YogVal lemon_parser = YUNDEF;
    YogVal token = YUNDEF;
    PUSH_LOCALS4(env, lexer, ast, lemon_parser, token);

    FILE* fp;
    if (filename != NULL) {
        fp = fopen(filename, "r");
        YOG_ASSERT(env, fp != NULL, "Can't open %s", filename);
    }
    else {
        fp = stdin;
    }

    lexer = YogLexer_new(env);
    PTR_AS(YogLexer, lexer)->fp = fp;
    if (filename != NULL) {
        YogLexer_read_encoding(env, lexer);
    }

    lemon_parser = LemonParser_new(env);
    if (debug) {
        ParseTrace(stdout, "parser> ");
    }
    while (YogLexer_next_token(env, lexer, &token)) {
        unsigned int type = PTR_AS(YogToken, token)->type;
        Parse(env, lemon_parser, type, token, &ast);
    }
    Parse(env, lemon_parser, 0, YNIL, &ast);

    if (filename != NULL) {
        fclose(fp);
    }

    RETURN(env, ast);
}

#define TOKEN_LINENO(token)     PTR_AS(YogToken, (token))->lineno
#define NODE_LINENO(node)       PTR_AS(YogNode, (node))->lineno
}   // end of %include

module ::= stmts(A). {
    *pval = A;
}

stmts(A) ::= stmt(B). {
    A = Array_new(env, B);
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
    if (PTR_AS(YogNode, B)->type == NODE_VARIABLE) {
        unsigned int lineno = NODE_LINENO(B);
        ID id = PTR_AS(YogNode, B)->u.variable.id;
        A = CommandCall_new(env, lineno, id, YNIL, YNIL);
    }
    else {
        A = B;
    }
}
stmt(A) ::= NAME(B) args(C). {
    unsigned int lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, B)->u.id;
    A = CommandCall_new(env, lineno, id, C, YNIL);
}
        /*
        | NAME args DO LPAR params RPAR stmts END {
            YogNode* blockarg = NULL;
            BLOCK_ARG_NEW(blockarg, $5, $7);
            COMMAND_CALL_NEW($$, $1, $2, blockarg);
        }
        */
stmt(A) ::= TRY(B) stmts(C) excepts(D) ELSE stmts(E) finally_opt(F) END. {
    unsigned int lineno = TOKEN_LINENO(B);
    A = ExceptFinally_new(env, lineno, C, D, E, F);
}
stmt(A) ::= TRY(B) stmts(C) excepts(D) finally_opt(E) END. {
    unsigned int lineno = TOKEN_LINENO(B);
    A = ExceptFinally_new(env, lineno, C, D, YNIL, E);
}
stmt(A) ::= TRY(B) stmts(C) FINALLY stmts(D) END. {
    unsigned int lineno = TOKEN_LINENO(B);
    A = Finally_new(env, lineno, C, D);
}
stmt(A) ::= WHILE(B) expr(C) stmts(D) END. {
    unsigned int lineno = TOKEN_LINENO(B);
    A = While_new(env, lineno, C, D);
}
stmt(A) ::= BREAK(B). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = Break_new(env, lineno, YNIL);
}
stmt(A) ::= BREAK(B) expr(C). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = Break_new(env, lineno, C);
}
stmt(A) ::= NEXT(B). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = Next_new(env, lineno, YNIL);
}
stmt(A) ::= NEXT(B) expr(C). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = Next_new(env, lineno, C);
}
stmt(A) ::= RETURN(B). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = Return_new(env, lineno, YNIL);
}
stmt(A) ::= RETURN(B) expr(C). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = Return_new(env, lineno, C);
}
stmt(A) ::= IF(B) expr(C) stmts(D) if_tail(E) END. {
    unsigned int lineno = TOKEN_LINENO(B);
    A = If_new(env, lineno, C, D, E);
}
stmt(A) ::= CLASS(B) NAME(C) super_opt(D) stmts(E) END. {
    unsigned int lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = Klass_new(env, lineno, id, D, E);
}
stmt(A) ::= NONLOCAL(B) names(C). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = Nonlocal_new(env, lineno, C);
}

names(A) ::= NAME(B). {
    A = YogArray_new(env);
    ID id = PTR_AS(YogToken, B)->u.id;
    YogArray_push(env, A, ID2VAL(id));
}
names(A) ::= names(B) COMMA NAME(C). {
    ID id = PTR_AS(YogToken, C)->u.id;
    YogArray_push(env, B, ID2VAL(id));
    A = B;
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
if_tail(A) ::= ELIF(B) expr(C) stmts(D) if_tail(E). {
    unsigned int lineno = TOKEN_LINENO(B);
    YogVal node = If_new(env, lineno, C, D, E);
    A = Array_new(env, node);
}

else_opt(A) ::= /* empty */. {
    A = YNIL;
}
else_opt(A) ::= ELSE stmts(B). {
    A = B;
}

func_def(A) ::= DEF(B) NAME(C) LPAR params(D) RPAR stmts(E) END. {
    unsigned int lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = FuncDef_new(env, lineno, id, D, E);
}

params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA block_param(D) COMMA var_param(E) COMMA kw_param(F). {
    A = Params_new(env, B, C, D, E, F);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA block_param(D) COMMA var_param(E). {
    A = Params_new(env, B, C, D, E, YNIL);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA block_param(D) COMMA kw_param(E). {
    A = Params_new(env, B, C, D, YNIL, E);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA block_param(D). {
    A = Params_new(env, B, C, D, YNIL, YNIL);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA var_param(D) COMMA kw_param(E). {
    A = Params_new(env, B, C, YNIL, D, E);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA var_param(D). {
    A = Params_new(env, B, C, YNIL, D, YNIL);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA kw_param(D). {
    A = Params_new(env, B, C, YNIL, YNIL, D);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C). {
    A = Params_new(env, B, C, YNIL, YNIL, YNIL);
}
params(A) ::= params_without_default(B) COMMA block_param(C) COMMA var_param(D) COMMA kw_param(E). {
    A = Params_new(env, B, YNIL, C, D, E);
}
params(A) ::= params_without_default(B) COMMA block_param(C) COMMA var_param(D). {
    A = Params_new(env, B, YNIL, C, D, YNIL);
}
params(A) ::= params_without_default(B) COMMA block_param(C) COMMA kw_param(D). {
    A = Params_new(env, B, YNIL, C, YNIL, D);
}
params(A) ::= params_without_default(B) COMMA block_param(C). {
    A = Params_new(env, B, YNIL, C, YNIL, YNIL);
}
params(A) ::= params_without_default(B) COMMA var_param(C) COMMA kw_param(D). {
    A = Params_new(env, B, YNIL, YNIL, C, D);
}
params(A) ::= params_without_default(B) COMMA var_param(C). {
    A = Params_new(env, B, YNIL, YNIL, C, YNIL);
}
params(A) ::= params_without_default(B) COMMA kw_param(C). {
    A = Params_new(env, B, YNIL, YNIL, YNIL, C);
}
params(A) ::= params_without_default(B). {
    A = Params_new(env, B, YNIL, YNIL, YNIL, YNIL);
}
params(A) ::= params_with_default(B) COMMA block_param(C) COMMA var_param(D) COMMA kw_param(E). {
    A = Params_new(env, YNIL, B, C, D, E);
}
params(A) ::= params_with_default(B) COMMA block_param(C) COMMA var_param(D). {
    A = Params_new(env, YNIL, B, C, D, YNIL);
}
params(A) ::= params_with_default(B) COMMA block_param(C) COMMA kw_param(D). {
    A = Params_new(env, YNIL, B, C, YNIL, D);
}
params(A) ::= params_with_default(B) COMMA block_param(C). {
    A = Params_new(env, YNIL, B, C, YNIL, YNIL);
}
params(A) ::= params_with_default(B) COMMA var_param(C) COMMA kw_param(D). {
    A = Params_new(env, YNIL, B, YNIL, C, D);
}
params(A) ::= params_with_default(B) COMMA var_param(C). {
    A = Params_new(env, YNIL, B, YNIL, C, YNIL);
}
params(A) ::= params_with_default(B) COMMA kw_param(C). {
    A = Params_new(env, YNIL, B, YNIL, YNIL, C);
}
params(A) ::= params_with_default(B). {
    A = Params_new(env, YNIL, B, YNIL, YNIL, YNIL);
}
params(A) ::= block_param(B) COMMA var_param(C) COMMA kw_param(D). {
    A = Params_new(env, YNIL, YNIL, B, C, D);
}
params(A) ::= block_param(B) COMMA var_param(C). {
    A = Params_new(env, YNIL, YNIL, B, C, YNIL);
}
params(A) ::= block_param(B) COMMA kw_param(C). {
    A = Params_new(env, YNIL, YNIL, B, YNIL, C);
}
params(A) ::= block_param(B). {
    A = Params_new(env, YNIL, YNIL, B, YNIL, YNIL);
}
params(A) ::= var_param(B) COMMA kw_param(C). {
    A = Params_new(env, YNIL, YNIL, YNIL, B, C);
}
params(A) ::= var_param(B). {
    A = Params_new(env, YNIL, YNIL, YNIL, B, YNIL);
}
params(A) ::= kw_param(B). {
    A = Params_new(env, YNIL, YNIL, YNIL, YNIL, B);
}
params(A) ::= /* empty */. {
    A = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}

kw_param(A) ::= DOUBLE_STAR(B) NAME(C). {
    unsigned int lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}

var_param(A) ::= STAR(B) NAME(C). {
    unsigned int lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}

block_param(A) ::= AMPER(B) NAME(C) param_default_opt(D). {
    unsigned int lineno = TOKEN_LINENO(B);
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
    unsigned int lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, B)->u.id;
    ParamArray_push(env, A, lineno, id, YNIL);
}
params_without_default(A) ::= params_without_default(B) COMMA NAME(C). {
    unsigned int lineno = TOKEN_LINENO(C);
    ID id = PTR_AS(YogToken, C)->u.id;
    ParamArray_push(env, B, lineno, id, YNIL);
    A = B;
}

params_with_default(A) ::= param_with_default(B). {
    A = Array_new(env, B);
}
params_with_default(A) ::= params_with_default(B) COMMA param_with_default(C). {
    A = Array_push(env, B, C);
}

param_with_default(A) ::= NAME(B) param_default(C). {
    unsigned int lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, B)->u.id;
    A = Param_new(env, NODE_PARAM, lineno, id, C);
}

args(A) ::= expr(B). {
    A = Array_new(env, B);
}
args(A) ::= args(B) COMMA expr(C). {
    A = Array_push(env, B, C);
}

expr(A) ::= assign_expr(B). {
    A = B;
}

assign_expr(A) ::= postfix_expr(B) EQUAL logical_or_expr(C). {
    unsigned int lineno = NODE_LINENO(B);
    A = Assign_new(env, lineno, B, C);
}
assign_expr(A) ::= logical_or_expr(B). {
    A = B;
}

logical_or_expr(A) ::= logical_and_expr(B). {
    A = B;
}

logical_and_expr(A) ::= not_expr(B). {
    A = B;
}

not_expr(A) ::= comparison(B). {
    A = B;
}

comparison(A) ::= xor_expr(B). {
    A = B;
}
comparison(A) ::= xor_expr(B) comp_op(C) xor_expr(D). {
    unsigned int lineno = NODE_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = MethodCall1_new(env, lineno, B, id, D);
}

comp_op(A) ::= LESS(B). {
    A = B;
}

xor_expr(A) ::= or_expr(B). {
    A = B;
}

or_expr(A) ::= and_expr(B). {
    A = B;
}

and_expr(A) ::= shift_expr(B). {
    A = B;
}

shift_expr(A) ::= match_expr(B). {
    A = B;
}
shift_expr(A) ::= shift_expr(B) LSHIFT(C) arith_expr(D). {
    unsigned int lineno = NODE_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = MethodCall1_new(env, lineno, B, id, D);
}

match_expr(A) ::= arith_expr(B). {
    A = B;
}
match_expr(A) ::= match_expr(B) EQUAL_TILDA(C) arith_expr(D). {
    unsigned int lineno = NODE_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = MethodCall1_new(env, lineno, B, id, D);
}

arith_expr(A) ::= term(B). {
    A = B;
}
arith_expr(A) ::= arith_expr(B) PLUS(C) term(D). {
    unsigned int lineno = NODE_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = MethodCall1_new(env, lineno, B, id, D);
}

term(A) ::= factor(B). {
    A = B;
}

factor(A) ::= power(B). {
    A = B;
}

power(A) ::= postfix_expr(B). {
    A = B;
}

postfix_expr(A) ::= atom(B). {
    A = B;
}
postfix_expr(A) ::= postfix_expr(B) LPAR args_opt(C) RPAR blockarg_opt(D). {
    unsigned int lineno = NODE_LINENO(B);
    if (NODE(B)->type == NODE_ATTR) {
        YogVal recv = NODE(B)->u.attr.obj;
        ID name = NODE(B)->u.attr.name;
        A = MethodCall_new(env, lineno, recv, name, C, D);
    }
    else {
        A = FuncCall_new(env, lineno, B, C, D);
    }
}
postfix_expr(A) ::= postfix_expr(B) LBRACKET expr(C) RBRACKET. {
    unsigned int lineno = NODE_LINENO(B);
    A = Subscript_new(env, lineno, B, C);
}
postfix_expr(A) ::= postfix_expr(B) DOT NAME(C). {
    unsigned int lineno = NODE_LINENO(B);
    ID id = PTR_AS(YogToken, C)->u.id;
    A = Attr_new(env, lineno, B, id);
}

atom(A) ::= NAME(B). {
    unsigned int lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, B)->u.id;
    A = Variable_new(env, lineno, id);
}
atom(A) ::= NUMBER(B). {
    unsigned int lineno = TOKEN_LINENO(B);
    YogVal val = PTR_AS(YogToken, B)->u.val;
    A = Literal_new(env, lineno, val);
}
atom(A) ::= REGEXP(B). {
    unsigned int lineno = TOKEN_LINENO(B);
    YogVal val = PTR_AS(YogToken, B)->u.val;
    A = Literal_new(env, lineno, val);
}
atom(A) ::= STRING(B). {
    unsigned int lineno = TOKEN_LINENO(B);
    YogVal val = PTR_AS(YogToken, B)->u.val;
    A = Literal_new(env, lineno, val);
}
atom(A) ::= TRUE(B). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = Literal_new(env, lineno, YTRUE);
}
atom(A) ::= FALSE(B). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = Literal_new(env, lineno, YFALSE);
}
atom(A) ::= LINE(B). {
    unsigned int lineno = PTR_AS(YogToken, B)->lineno;
    YogVal val = INT2VAL(lineno);
    A = Literal_new(env, lineno, val);
}

args_opt(A) ::= /* empty */. {
    A = YNIL;
}
args_opt(A) ::= args(B). {
    A = B;
}

blockarg_opt(A) ::= /* empty */. {
    A = YNIL;
}
blockarg_opt(A) ::= DO(B) blockarg_params_opt(C) stmts(D) END. {
    unsigned int lineno = TOKEN_LINENO(B);
    A = BlockArg_new(env, lineno, C, D);
}
blockarg_opt(A) ::= LBRACE(B) blockarg_params_opt(C) stmts(D) RBRACE. {
    unsigned int lineno = TOKEN_LINENO(B);
    A = BlockArg_new(env, lineno, C, D);
}

blockarg_params_opt(A) ::= /* empty */. {
    A = YNIL;
}
blockarg_params_opt(A) ::= LBRACKET params(B) RBRACKET. {
    A = B;
}

excepts(A) ::= except(B). {
    A = Array_new(env, B);
}
excepts(A) ::= excepts(B) except(C). {
    A = Array_push(env, B, C);
}

except(A) ::= EXCEPT(B) expr(C) AS NAME(D) NEWLINE stmts(E). {
    unsigned int lineno = TOKEN_LINENO(B);
    ID id = PTR_AS(YogToken, D)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    A = ExceptBody_new(env, lineno, C, id, E);
}
except(A) ::= EXCEPT(B) expr(C) NEWLINE stmts(E). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = ExceptBody_new(env, lineno, C, NO_EXC_VAR, E);
}
except(A) ::= EXCEPT(B) NEWLINE stmts(C). {
    unsigned int lineno = TOKEN_LINENO(B);
    A = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, C);
}

finally_opt(A) ::= /* empty */. {
    A = YNIL;
} 
finally_opt(A) ::= FINALLY stmts(B). {
    A = B;
}
/*
single_input: NEWLINE | simple_stmt | compound_stmt NEWLINE
file_input: (NEWLINE | stmt)* ENDMARKER
eval_input: testlist NEWLINE* ENDMARKER

decorator: '@' dotted_name [ '(' [arglist] ')' ] NEWLINE
decorators: decorator+
decorated: decorators (classdef | funcdef)
funcdef: 'def' NAME parameters ['->' test] ':' suite
parameters: '(' [typedargslist] ')'
typedargslist: ((tfpdef ['=' test] ',')*
                ('*' [tfpdef] (',' tfpdef ['=' test])* [',' '**' tfpdef] | '**' tfpdef)
                | tfpdef ['=' test] (',' tfpdef ['=' test])* [','])
tfpdef: NAME [':' test]
varargslist: ((vfpdef ['=' test] ',')*
              ('*' [vfpdef] (',' vfpdef ['=' test])*  [',' '**' vfpdef] | '**' vfpdef)
              | vfpdef ['=' test] (',' vfpdef ['=' test])* [','])
vfpdef: NAME

stmt: simple_stmt | compound_stmt
simple_stmt: small_stmt (';' small_stmt)* [';'] NEWLINE
small_stmt: (expr_stmt | del_stmt | pass_stmt | flow_stmt |
             import_stmt | global_stmt | nonlocal_stmt | assert_stmt)
expr_stmt: testlist (augassign (yield_expr|testlist) |
                     ('=' (yield_expr|testlist))*)
augassign: ('+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^=' |
            '<<=' | '>>=' | '**=' | '//=')
# For normal assignments, additional restrictions enforced by the interpreter
del_stmt: 'del' exprlist
pass_stmt: 'pass'
flow_stmt: break_stmt | continue_stmt | return_stmt | raise_stmt | yield_stmt
break_stmt: 'break'
continue_stmt: 'continue'
return_stmt: 'return' [testlist]
yield_stmt: yield_expr
raise_stmt: 'raise' [test ['from' test]]
import_stmt: import_name | import_from
import_name: 'import' dotted_as_names
# note below: the ('.' | '...') is necessary because '...' is tokenized as ELLIPSIS
import_from: ('from' (('.' | '...')* dotted_name | ('.' | '...')+)
              'import' ('*' | '(' import_as_names ')' | import_as_names))
import_as_name: NAME ['as' NAME]
dotted_as_name: dotted_name ['as' NAME]
import_as_names: import_as_name (',' import_as_name)* [',']
dotted_as_names: dotted_as_name (',' dotted_as_name)*
dotted_name: NAME ('.' NAME)*
global_stmt: 'global' NAME (',' NAME)*
nonlocal_stmt: 'nonlocal' NAME (',' NAME)*
assert_stmt: 'assert' test [',' test]

compound_stmt: if_stmt | while_stmt | for_stmt | try_stmt | with_stmt | funcdef | classdef | decorated
if_stmt: 'if' test ':' suite ('elif' test ':' suite)* ['else' ':' suite]
while_stmt: 'while' test ':' suite ['else' ':' suite]
for_stmt: 'for' exprlist 'in' testlist ':' suite ['else' ':' suite]
try_stmt: ('try' ':' suite
           ((except_clause ':' suite)+
	    ['else' ':' suite]
	    ['finally' ':' suite] |
	   'finally' ':' suite))
with_stmt: 'with' test [ with_var ] ':' suite
with_var: 'as' expr
# NB compile.c makes sure that the default except clause is last
except_clause: 'except' [test ['as' NAME]]
suite: simple_stmt | NEWLINE INDENT stmt+ DEDENT

test: or_test ['if' or_test 'else' test] | lambdef
test_nocond: or_test | lambdef_nocond
lambdef: 'lambda' [varargslist] ':' test
lambdef_nocond: 'lambda' [varargslist] ':' test_nocond
or_test: and_test ('or' and_test)*
and_test: not_test ('and' not_test)*
not_test: 'not' not_test | comparison
comparison: star_expr (comp_op star_expr)*
comp_op: '<'|'>'|'=='|'>='|'<='|'!='|'in'|'not' 'in'|'is'|'is' 'not'
star_expr: ['*'] expr
expr: xor_expr ('|' xor_expr)*
xor_expr: and_expr ('^' and_expr)*
and_expr: shift_expr ('&' shift_expr)*
shift_expr: arith_expr (('<<'|'>>') arith_expr)*
arith_expr: term (('+'|'-') term)*
term: factor (('*'|'/'|'%'|'//') factor)*
factor: ('+'|'-'|'~') factor | power
power: atom trailer* ['**' factor]
atom: ('(' [yield_expr|testlist_comp] ')' |
       '[' [testlist_comp] ']' |
       '{' [dictorsetmaker] '}' |
       NAME | NUMBER | STRING+ | '...' | 'None' | 'True' | 'False')
testlist_comp: test ( comp_for | (',' test)* [','] )
trailer: '(' [arglist] ')' | '[' subscriptlist ']' | '.' NAME
subscriptlist: subscript (',' subscript)* [',']
subscript: test | [test] ':' [test] [sliceop]
sliceop: ':' [test]
exprlist: star_expr (',' star_expr)* [',']
testlist: test (',' test)* [',']
dictorsetmaker: ( (test ':' test (comp_for | (',' test ':' test)* [','])) |
                  (test (comp_for | (',' test)* [','])) )

classdef: 'class' NAME ['(' [arglist] ')'] ':' suite

arglist: (argument ',')* (argument [',']
                         |'*' test (',' argument)* [',' '**' test] 
                         |'**' test)
argument: test [comp_for] | test '=' test  # Really [keyword '='] test

comp_iter: comp_for | comp_if
comp_for: 'for' exprlist 'in' or_test [comp_iter]
comp_if: 'if' test_nocond [comp_iter]

testlist1: test (',' test)*

# not used in grammar, but may appear in "node" passed from Parser to Compiler
encoding_decl: NAME

yield_expr: 'yield' [testlist]
*/
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
