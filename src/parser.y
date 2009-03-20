
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
YogNode_new(YogEnv* env, YogNodeType type) 
{
    YogNode* node = ALLOC_OBJ(env, YogNode_keep_children, NULL, YogNode);
    node->lineno = 0;
    node->type = type;

    return PTR2VAL(node);
}

#define NODE_NEW(type)  YogNode_new(env, type)
#define NODE(v)         PTR_AS(YogNode, (v))

#define LITERAL_NEW(node, val_) do { \
    node = NODE_NEW(NODE_LITERAL); \
    NODE(node)->u.literal.val = val_; \
} while (0)

#define BLOCK_ARG_NEW(node, params_, stmts_) do { \
    node = NODE_NEW(NODE_BLOCK_ARG); \
    NODE(node)->u.blockarg.params = params_; \
    NODE(node)->u.blockarg.stmts = stmts_; \
} while (0)

#define PARAMS_NEW(array, params_without_default, params_with_default, block_param, var_param, kw_param) do { \
    array = YogArray_new(env); \
    \
    if (IS_OBJ(params_without_default)) { \
        YogArray_extend(env, array, params_without_default); \
    } \
    \
    if (IS_OBJ(params_with_default)) { \
        YogArray_extend(env, array, params_with_default); \
    } \
    \
    if (IS_PTR(block_param)) { \
        YogArray_push(env, array, block_param); \
    } \
    \
    if (IS_PTR(var_param)) { \
        YogArray_push(env, array, var_param); \
    } \
    \
    if (IS_PTR(kw_param)) { \
        YogArray_push(env, array, kw_param); \
    } \
} while (0)

#define COMMAND_CALL_NEW(node, name_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_COMMAND_CALL); \
    NODE(node)->u.command_call.name = name_; \
    NODE(node)->u.command_call.args = args_; \
    NODE(node)->u.command_call.blockarg = blockarg_; \
} while (0)

#define OBJ_ARRAY_NEW(array, elem) do { \
    if (IS_PTR(elem)) { \
        array = YogArray_new(env); \
        YogArray_push(env, array, elem); \
    } \
    else { \
        array = YNIL; \
    } \
} while (0)

#define OBJ_ARRAY_PUSH(result, array, elem) do { \
    if (IS_PTR(elem)) { \
        if (!IS_OBJ(array)) { \
            YogVal a = YogArray_new(env); \
            array = a; \
        } \
        YogArray_push(env, array, elem); \
    } \
    result = array; \
} while (0)

#define PARAM_NEW(node, type, id, default__) do { \
    node = NODE_NEW(type); \
    NODE(node)->u.param.name = id; \
    NODE(node)->u.param.default_ = default__; \
} while (0)

#define PARAM_ARRAY_PUSH(array, id, default_) do { \
    YogVal node = YUNDEF; \
    PARAM_NEW(node, NODE_PARAM, id, default_); \
    YogArray_push(env, array, node); \
} while (0)

#define FUNC_DEF_NEW(node, name_, params_, stmts_) do { \
    node = NODE_NEW(NODE_FUNC_DEF); \
    NODE(node)->u.funcdef.name = name_; \
    NODE(node)->u.funcdef.params = params_; \
    NODE(node)->u.funcdef.stmts = stmts_; \
} while (0)

#define FUNC_CALL_NEW(node, callee_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_FUNC_CALL); \
    NODE(node)->u.func_call.callee = callee_; \
    NODE(node)->u.func_call.args = args_; \
    NODE(node)->u.func_call.blockarg = blockarg_; \
} while (0)

#define VARIABLE_NEW(node, id_) do { \
    node = NODE_NEW(NODE_VARIABLE); \
    NODE(node)->u.variable.id = id_; \
} while (0)

#define EXCEPT_BODY_NEW(node, type_, var_, stmts_) do { \
    node = NODE_NEW(NODE_EXCEPT_BODY); \
    NODE(node)->u.except_body.type = type_; \
    NODE(node)->u.except_body.var = var_; \
    NODE(node)->u.except_body.stmts = stmts_; \
} while (0)

#define EXCEPT_NEW(node, head_, excepts_, else__) do { \
    node = NODE_NEW(NODE_EXCEPT); \
    NODE(node)->u.except.head = head_; \
    NODE(node)->u.except.excepts = excepts_; \
    NODE(node)->u.except.else_ = else__; \
} while (0)

#define FINALLY_NEW(node, head_, body_) do { \
    node = NODE_NEW(NODE_FINALLY); \
    NODE(node)->u.finally.head = head_; \
    NODE(node)->u.finally.body = body_; \
} while (0)

#define EXCEPT_FINALLY_NEW(node, stmts, excepts, else_, finally) do { \
    EXCEPT_NEW(node, stmts, excepts, else_); \
    \
    if (IS_OBJ(finally)) { \
        YogVal array = YUNDEF; \
        OBJ_ARRAY_NEW(array, node); \
        FINALLY_NEW(node, array, finally); \
    } \
} while (0)

#define BREAK_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_BREAK); \
    NODE(node)->u.break_.expr = expr_; \
} while (0)

#define NEXT_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_NEXT); \
    NODE(node)->u.next.expr = expr_; \
} while (0)

#define RETURN_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_RETURN); \
    NODE(node)->u.return_.expr = expr_; \
} while (0)

#define METHOD_CALL_NEW(node, recv_, name_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_METHOD_CALL); \
    NODE(node)->u.method_call.recv = recv_; \
    NODE(node)->u.method_call.name = name_; \
    NODE(node)->u.method_call.args = args_; \
    NODE(node)->u.method_call.blockarg = blockarg_; \
} while (0)

#define METHOD_CALL_NEW1(node, recv, name, arg) do { \
    YogVal args = YUNDEF; \
    PUSH_LOCAL(env, args); \
    \
    args = YogArray_new(env); \
    YogArray_push(env, args, arg); \
    \
    YogVal blockarg = YNIL; \
    METHOD_CALL_NEW(node, recv, name, args, blockarg); \
    \
    POP_LOCALS(env); \
} while (0)

#define IF_NEW(node, test_, stmts_, tail_) do { \
    node = NODE_NEW(NODE_IF); \
    NODE(node)->u.if_.test = test_; \
    NODE(node)->u.if_.stmts = stmts_; \
    NODE(node)->u.if_.tail = tail_; \
} while (0)

#define WHILE_NEW(node, test_, stmts_) do { \
    node = NODE_NEW(NODE_WHILE); \
    NODE(node)->u.while_.test = test_; \
    NODE(node)->u.while_.stmts = stmts_; \
} while (0)

#define KLASS_NEW(node, name_, super_, stmts_) do { \
    node = NODE_NEW(NODE_KLASS); \
    NODE(node)->u.klass.name = name_; \
    NODE(node)->u.klass.super = super_; \
    NODE(node)->u.klass.stmts = stmts_; \
} while (0);

#define ASSIGN_NEW(node, left_, right_) do { \
    node = NODE_NEW(NODE_ASSIGN); \
    NODE(node)->u.assign.left = left_; \
    NODE(node)->u.assign.right = right_; \
} while (0)

#define SUBSCRIPT_NEW(node, prefix_, index_) do { \
    node = NODE_NEW(NODE_SUBSCRIPT); \
    NODE(node)->u.subscript.prefix = prefix_; \
    NODE(node)->u.subscript.index = index_; \
} while (0)

#define ATTR_NEW(node, obj_, name_) do { \
    node = NODE_NEW(NODE_ATTR); \
    NODE(node)->u.attr.obj = obj_; \
    NODE(node)->u.attr.name = name_; \
} while (0)

#define NONLOCAL_NEW(node, names_) do { \
    node = NODE_NEW(NODE_NONLOCAL); \
    NODE(node)->u.nonlocal.names = names_; \
} while (0)

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
        YogVm_gc(env, ENV_VM(env));
        YogVm_gc(env, ENV_VM(env));
        Parse(env, lemon_parser, type, token, &ast);
    }
    Parse(env, lemon_parser, 0, YNIL, &ast);

    if (filename != NULL) {
        fclose(fp);
    }

    RETURN(env, ast);
}
}   // end of %include

module ::= stmts(A). {
    *pval = A;
}

stmts(A) ::= stmt(B). {
    OBJ_ARRAY_NEW(A, B);
}
stmts(A) ::= stmts(B) newline stmt(C). {
    OBJ_ARRAY_PUSH(A, B, C);
}

stmt(A) ::= /* empty */. {
    A = YNIL;
}
stmt(A) ::= func_def(B). {
    A = B;
}
stmt(A) ::= expr(B). {
    if (PTR_AS(YogNode, B)->type == NODE_VARIABLE) {
        YogVal args = YNIL;
        YogVal blockarg = YNIL;
        COMMAND_CALL_NEW(A, PTR_AS(YogNode, B)->u.variable.id, args, blockarg);
    }
    else {
        A = B;
    }
}
stmt(A) ::= NAME(B) args(C). {
    YogVal blockarg = YNIL;
    COMMAND_CALL_NEW(A, PTR_AS(YogToken, B)->u.id, C, blockarg);
}
        /*
        | NAME args DO LPAR params RPAR stmts END {
            YogNode* blockarg = NULL;
            BLOCK_ARG_NEW(blockarg, $5, $7);
            COMMAND_CALL_NEW($$, $1, $2, blockarg);
        }
        */
stmt(A) ::= TRY stmts(B) excepts(C) ELSE stmts(D) finally_opt(E) END. {
    EXCEPT_FINALLY_NEW(A, B, C, D, E);
}
stmt(A) ::= TRY stmts(B) excepts(C) finally_opt(D) END. {
    YogVal finally = YNIL;
    EXCEPT_FINALLY_NEW(A, B, C, finally, D);
}
stmt(A) ::= TRY stmts(B) FINALLY stmts(C) END. {
    FINALLY_NEW(A, B, C);
}
stmt(A) ::= WHILE expr(B) stmts(C) END. {
    WHILE_NEW(A, B, C);
}
stmt(A) ::= BREAK. {
    YogVal expr = YNIL;
    BREAK_NEW(A, expr);
}
stmt(A) ::= BREAK expr(B). {
    BREAK_NEW(A, B);
}
stmt(A) ::= NEXT. {
    YogVal expr = YNIL;
    NEXT_NEW(A, expr);
}
stmt(A) ::= NEXT expr(B). {
    NEXT_NEW(A, B);
}
stmt(A) ::= RETURN. {
    YogVal expr = YNIL;
    RETURN_NEW(A, expr);
}
stmt(A) ::= RETURN expr(B). {
    RETURN_NEW(A, B);
}
stmt(A) ::= IF expr(B) stmts(C) if_tail(D) END. {
    IF_NEW(A, B, C, D);
}
stmt(A) ::= CLASS NAME(B) super_opt(C) stmts(D) END. {
    KLASS_NEW(A, PTR_AS(YogToken, B)->u.id, C, D);
}
stmt(A) ::= NONLOCAL names(B). {
    NONLOCAL_NEW(A, B);
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
if_tail(A) ::= ELIF expr(B) stmts(C) if_tail(D). {
    YogVal node = YUNDEF;
    IF_NEW(node, B, C, D);
    OBJ_ARRAY_NEW(A, node);
}

else_opt(A) ::= /* empty */. {
    A = YNIL;
}
else_opt(A) ::= ELSE stmts(B). {
    A = B;
}

func_def(A) ::= DEF NAME(B) LPAR params(C) RPAR stmts(D) END. {
    FUNC_DEF_NEW(A, PTR_AS(YogToken, B)->u.id, C, D);
}

params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA block_param(D) COMMA var_param(E) COMMA kw_param(F). {
    PARAMS_NEW(A, B, C, D, E, F);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA block_param(D) COMMA var_param(E). {
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, B, C, D, E, kw_param);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA block_param(D) COMMA kw_param(E). {
    YogVal var_param = YNIL;
    PARAMS_NEW(A, B, C, D, var_param, E);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA block_param(D). {
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, B, C, D, var_param, kw_param);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA var_param(D) COMMA kw_param(E). {
    YogVal block_param = YNIL;
    PARAMS_NEW(A, B, C, block_param, D, E);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA var_param(D). {
    YogVal block_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, B, C, block_param, D, kw_param);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C) COMMA kw_param(D). {
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(A, B, C, block_param, var_param, D);
}
params(A) ::= params_without_default(B) COMMA params_with_default(C). {
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, B, C, block_param, var_param, kw_param);
}
params(A) ::= params_without_default(B) COMMA block_param(C) COMMA var_param(D) COMMA kw_param(E). {
    YogVal params_with_default = YNIL;
    PARAMS_NEW(A, B, params_with_default, C, D, E);
}
params(A) ::= params_without_default(B) COMMA block_param(C) COMMA var_param(D). {
    YogVal params_with_default = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, B, params_with_default, C, D, kw_param);
}
params(A) ::= params_without_default(B) COMMA block_param(C) COMMA kw_param(D). {
    YogVal params_with_default = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(A, B, params_with_default, C, var_param, D);
}
params(A) ::= params_without_default(B) COMMA block_param(C). {
    YogVal params_with_default = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, B, params_with_default, C, var_param, kw_param);
}
params(A) ::= params_without_default(B) COMMA var_param(C) COMMA kw_param(D). {
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    PARAMS_NEW(A, B, params_with_default, block_param, C, D);
}
params(A) ::= params_without_default(B) COMMA var_param(C). {
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, B, params_with_default, block_param, C, kw_param);
}
params(A) ::= params_without_default(B) COMMA kw_param(C). {
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(A, B, params_with_default, block_param, var_param, C);
}
params(A) ::= params_without_default(B). {
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, B, params_with_default, block_param, var_param, kw_param);
}
params(A) ::= params_with_default(B) COMMA block_param(C) COMMA var_param(D) COMMA kw_param(E). {
    YogVal params_without_default = YNIL;
    PARAMS_NEW(A, params_without_default, B, C, D, E);
}
params(A) ::= params_with_default(B) COMMA block_param(C) COMMA var_param(D). {
    YogVal params_without_default = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, params_without_default, B, C, D, kw_param);
}
params(A) ::= params_with_default(B) COMMA block_param(C) COMMA kw_param(D). {
    YogVal params_without_default = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(A, params_without_default, B, C, var_param, D);
}
params(A) ::= params_with_default(B) COMMA block_param(C). {
    YogVal params_without_default = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, params_without_default, B, C, var_param, kw_param);
}
params(A) ::= params_with_default(B) COMMA var_param(C) COMMA kw_param(D). {
    YogVal params_without_default = YNIL;
    YogVal block_param = YNIL;
    PARAMS_NEW(A, params_without_default, B, block_param, C, D);
}
params(A) ::= params_with_default(B) COMMA var_param(C). {
    YogVal params_without_default = YNIL;
    YogVal block_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, params_without_default, B, block_param, C, kw_param);
}
params(A) ::= params_with_default(B) COMMA kw_param(C). {
    YogVal params_without_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(A, params_without_default, B, block_param, var_param, C);
}
params(A) ::= params_with_default(B). {
    YogVal params_without_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, params_without_default, B, block_param, var_param, kw_param);
}
params(A) ::= block_param(B) COMMA var_param(C) COMMA kw_param(D). {
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    PARAMS_NEW(A, params_without_default, params_with_default, B, C, D);
}
params(A) ::= block_param(B) COMMA var_param(C). {
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, params_without_default, params_with_default, B, C, kw_param);
}
params(A) ::= block_param(B) COMMA kw_param(C). {
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(A, params_without_default, params_with_default, B, var_param, C);
}
params(A) ::= block_param(B). {
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, params_without_default, params_with_default, B, var_param, kw_param);
}
params(A) ::= var_param(B) COMMA kw_param(C). {
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    PARAMS_NEW(A, params_without_default, params_with_default, block_param, B, C);
}
params(A) ::= var_param(B). {
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, params_without_default, params_with_default, block_param, B, kw_param);
}
params(A) ::= kw_param(B). {
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(A, params_without_default, params_with_default, block_param, var_param, B);
}
params(A) ::= /* empty */. {
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(A, params_without_default, params_with_default, block_param, var_param, kw_param);
}

kw_param(A) ::= DOUBLE_STAR NAME(B). {
    YogVal default_ = YNIL;
    PARAM_NEW(A, NODE_KW_PARAM, PTR_AS(YogToken, B)->u.id, default_);
}

var_param(A) ::= STAR NAME(B). {
    YogVal default_ = YNIL;
    PARAM_NEW(A, NODE_VAR_PARAM, PTR_AS(YogToken, B)->u.id, default_);
}

block_param(A) ::= AMPER NAME(B) param_default_opt(C). {
    PARAM_NEW(A, NODE_BLOCK_PARAM, PTR_AS(YogToken, B)->u.id, C);
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
    ID id = PTR_AS(YogToken, B)->u.id;
    PARAM_ARRAY_PUSH(A, id, YNIL);
}
params_without_default(A) ::= params_without_default(B) COMMA NAME(C). {
    ID id = PTR_AS(YogToken, C)->u.id;
    PARAM_ARRAY_PUSH(B, id, YNIL);
    A = B;
}

params_with_default(A) ::= param_with_default(B). {
    OBJ_ARRAY_NEW(A, B);
}
params_with_default(A) ::= params_with_default(B) COMMA param_with_default(C). {
    OBJ_ARRAY_PUSH(A, B, C);
}

param_with_default(A) ::= NAME(B) param_default(C). {
    ID id = PTR_AS(YogToken, B)->u.id;
    PARAM_NEW(A, NODE_PARAM, id, C);
}

args(A) ::= expr(B). {
    OBJ_ARRAY_NEW(A, B);
}
args(A) ::= args(B) COMMA expr(C). {
    OBJ_ARRAY_PUSH(A, B, C);
}

expr(A) ::= assign_expr(B). {
    A = B;
}

assign_expr(A) ::= postfix_expr(B) EQUAL logical_or_expr(C). {
    ASSIGN_NEW(A, B, C);
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
    ID id = PTR_AS(YogToken, C)->u.id;
    METHOD_CALL_NEW1(A, B, id, D);
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
    ID id = PTR_AS(YogToken, C)->u.id;
    METHOD_CALL_NEW1(A, B, id, D);
}

match_expr(A) ::= arith_expr(B). {
    A = B;
}
match_expr(A) ::= match_expr(B) EQUAL_TILDA(C) arith_expr(D). {
    ID id = PTR_AS(YogToken, C)->u.id;
    METHOD_CALL_NEW1(A, B, id, D);
}

arith_expr(A) ::= term(B). {
    A = B;
}
arith_expr(A) ::= arith_expr(B) PLUS(C) term(D). {
    ID id = PTR_AS(YogToken, C)->u.id;
    METHOD_CALL_NEW1(A, B, id, D);
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
    if (NODE(B)->type == NODE_ATTR) {
        METHOD_CALL_NEW(A, NODE(B)->u.attr.obj, NODE(B)->u.attr.name, C, D);
    }
    else {
        FUNC_CALL_NEW(A, B, C, D);
    }
}
postfix_expr(A) ::= postfix_expr(B) LBRACKET expr(C) RBRACKET. {
    SUBSCRIPT_NEW(A, B, C);
}
postfix_expr(A) ::= postfix_expr(B) DOT NAME(C). {
    ID id = PTR_AS(YogToken, C)->u.id;
    ATTR_NEW(A, B, id);
}

atom(A) ::= NAME(B). {
    ID id = PTR_AS(YogToken, B)->u.id;
    VARIABLE_NEW(A, id);
}
atom(A) ::= NUMBER(B). {
    YogVal val = PTR_AS(YogToken, B)->u.val;
    PUSH_LOCAL(env, val);
    LITERAL_NEW(A, val);
    POP_LOCALS(env);
}
atom(A) ::= REGEXP(B). {
    YogVal val = PTR_AS(YogToken, B)->u.val;
    PUSH_LOCAL(env, val);
    LITERAL_NEW(A, val);
    POP_LOCALS(env);
}
atom(A) ::= STRING(B). {
    YogVal val = PTR_AS(YogToken, B)->u.val;
    PUSH_LOCAL(env, val);
    LITERAL_NEW(A, val);
    POP_LOCALS(env);
}
atom(A) ::= TRUE. {
    LITERAL_NEW(A, YTRUE);
}
atom(A) ::= FALSE. {
    LITERAL_NEW(A, YFALSE);
}
atom(A) ::= LINE(B). {
    unsigned int lineno = PTR_AS(YogToken, B)->lineno;
    YogVal val = INT2VAL(lineno);
    LITERAL_NEW(A, val);
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
blockarg_opt(A) ::= DO blockarg_params_opt(B) stmts(C) END. {
    BLOCK_ARG_NEW(A, B, C);
}
blockarg_opt(A) ::= LBRACE blockarg_params_opt(B) stmts(C) RBRACE. {
    BLOCK_ARG_NEW(A, B, C);
}

blockarg_params_opt(A) ::= /* empty */. {
    A = YNIL;
}
blockarg_params_opt(A) ::= LBRACKET params(B) RBRACKET. {
    A = B;
}

excepts(A) ::= except(B). {
    OBJ_ARRAY_NEW(A, B);
}
excepts(A) ::= excepts(B) except(C). {
    OBJ_ARRAY_PUSH(A, B, C);
}

except(A) ::= EXCEPT expr(B) AS NAME(C) newline stmts(D). {
    ID id = PTR_AS(YogToken, C)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    EXCEPT_BODY_NEW(A, B, id, D);
}
except(A) ::= EXCEPT expr(B) newline stmts(D). {
    EXCEPT_BODY_NEW(A, B, NO_EXC_VAR, D);
}
except(A) ::= EXCEPT newline stmts(B). {
    EXCEPT_BODY_NEW(A, YNIL, NO_EXC_VAR, B);
}

newline(A) ::= NEWLINE(B). {
    A = B;
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
