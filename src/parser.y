%{
/* TODO: replace yacc to lemon.c */
#include <stdio.h>
#include "yog/error.h"
#include "yog/parser.h"
#include "yog/yog.h"

#define YYPARSE_PARAM   parser
#define PARSER          ((YogParser*)YYPARSE_PARAM)
#define YYLEX_PARAM     (PARSER)->lexer
#define ENV             (PARSER)->env

static void 
yyerror(char* s)
{
    fprintf(stderr, "%s\n", s);
}

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
        node->u.literal.val = YogVal_keep(env, node->u.literal.val, keeper);
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
YogNode_new(YogEnv* env, YogParser* parser, YogNodeType type) 
{
    YogNode* node = ALLOC_OBJ(env, YogNode_keep_children, NULL, YogNode);
    node->lineno = parser->lineno;
    node->type = type;

    return PTR2VAL(node);
}

#define NODE_NEW(type)  YogNode_new(ENV, PARSER, type)
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
    array = YogArray_new(ENV); \
    \
    if (IS_OBJ(params_without_default)) { \
        YogArray_extend(ENV, array, params_without_default); \
    } \
    \
    if (IS_OBJ(params_with_default)) { \
        YogArray_extend(ENV, array, params_with_default); \
    } \
    \
    if (IS_PTR(block_param)) { \
        YogArray_push(ENV, array, block_param); \
    } \
    \
    if (IS_PTR(var_param)) { \
        YogArray_push(ENV, array, var_param); \
    } \
    \
    if (IS_PTR(kw_param)) { \
        YogArray_push(ENV, array, kw_param); \
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
        array = YogArray_new(ENV); \
        YogArray_push(ENV, array, elem); \
    } \
    else { \
        array = YNIL; \
    } \
} while (0)

#define OBJ_ARRAY_PUSH(result, array, elem) do { \
    if (IS_PTR(elem)) { \
        if (!IS_OBJ(array)) { \
            array = YogArray_new(ENV); \
        } \
        YogArray_push(ENV, array, elem); \
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
    YogArray_push(ENV, array, node); \
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
    if (IS_PTR(finally)) { \
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
    YogVal args = YogArray_new(ENV); \
    YogArray_push(ENV, args, arg); \
    METHOD_CALL_NEW(node, recv, name, args, YNIL); \
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
%}

%union {
    struct YogVal val;
    ID name;
    unsigned int lineno;
}

%token tAMPER
%token tAS
%token tBAR
%token tBREAK
%token tCLASS
%token tCOMMA
%token tDEF
%token tDIV
%token tDO
%token tDOT
%token tDOUBLE_STAR
%token tELIF
%token tELSE
%token tEND
%token tEQUAL
%token tEQUAL_TILDA
%token tEXCEPT
%token tFINALLY
%token tGREATER
%token tIF
%token tLBRACE
%token tLBRACKET
%token tLESS
%token tLPAR
%token tLSHIFT
%token tNAME
%token tNEWLINE
%token tNEXT
%token tNONLOCAL
%token tNUMBER
%token tPLUS
%token tRBRACE
%token tRBRACKET
%token tREGEXP
%token tRETURN
%token tRPAR
%token tSTAR
%token tSTRING
%token tTRY
%token tWHILE
%token tFALSE
%token tTRUE
%token t__LINE__

%type<name> comp_op
%type<name> tEQUAL_TILDA
%type<name> tLESS
%type<name> tLSHIFT
%type<name> tNAME
%type<name> tPLUS
%type<val> and_expr
%type<val> args
%type<val> args_opt
%type<val> arith_expr
%type<val> assign_expr
%type<val> atom
%type<val> block_param
%type<val> blockarg_opt
%type<val> blockarg_params_opt
%type<val> comparison
%type<val> else_opt
%type<val> except
%type<val> excepts
%type<val> expr
%type<val> factor
%type<val> finally_opt
%type<val> func_def
%type<val> if_tail
%type<val> kw_param
%type<val> logical_and_expr
%type<val> logical_or_expr
%type<val> match_expr
%type<val> module
%type<val> names
%type<val> not_expr
%type<val> or_expr
%type<val> param_default
%type<val> param_default_opt
%type<val> param_with_default
%type<val> params
%type<val> params_with_default
%type<val> params_without_default
%type<val> postfix_expr
%type<val> power
%type<val> shift_expr
%type<val> stmt
%type<val> stmts
%type<val> super_opt
%type<val> tNUMBER
%type<val> tREGEXP
%type<val> tSTRING
%type<val> term
%type<val> var_param
%type<val> xor_expr
%%
module  : stmts {
            PARSER->stmts = $1;
        }
        ;
stmts   : stmt {
            OBJ_ARRAY_NEW($$, $1);
        }
        | stmts newline stmt {
            OBJ_ARRAY_PUSH($$, $1, $3);
        }
        ;
stmt    : /* empty */ {
            $$ = YNIL;
        }
        | func_def
        | expr {
            if (PTR_AS(YogNode, $1)->type == NODE_VARIABLE) {
                COMMAND_CALL_NEW($$, PTR_AS(YogNode, $1)->u.variable.id, YNIL, YNIL);
            }
            else {
                $$ = $1;
            }
        }
        | tNAME args {
            COMMAND_CALL_NEW($$, $1, $2, YNIL);
        }
        /*
        | NAME args DO LPAR params RPAR stmts END {
            YogNode* blockarg = NULL;
            BLOCK_ARG_NEW(blockarg, $5, $7);
            COMMAND_CALL_NEW($$, $1, $2, blockarg);
        }
        */
        | tTRY stmts excepts tELSE stmts finally_opt tEND {
            EXCEPT_FINALLY_NEW($$, $2, $3, $5, $6);
        }
        | tTRY stmts excepts finally_opt tEND {
            EXCEPT_FINALLY_NEW($$, $2, $3, YNIL, $4);
        }
        | tTRY stmts tFINALLY stmts tEND {
            FINALLY_NEW($$, $2, $4);
        }
        | tWHILE expr stmts tEND {
            WHILE_NEW($$, $2, $3);
        }
        | tBREAK {
            BREAK_NEW($$, YNIL);
        }
        | tBREAK expr {
            BREAK_NEW($$, $2);
        }
        | tNEXT {
            NEXT_NEW($$, YNIL);
        }
        | tNEXT expr {
            NEXT_NEW($$, $2);
        }
        | tRETURN {
            RETURN_NEW($$, YNIL);
        }
        | tRETURN expr {
            RETURN_NEW($$, $2);
        }
        | tIF expr stmts if_tail tEND {
            IF_NEW($$, $2, $3, $4);
        }
        | tCLASS { $<lineno>$ = PARSER->lineno; } tNAME super_opt stmts tEND {
            KLASS_NEW($$, $3, $4, $5);
            NODE($$)->lineno = $<lineno>2;
        }
        | tNONLOCAL names {
            NONLOCAL_NEW($$, $2);
        }
        ;
names   : tNAME {
            $$ = YogArray_new(ENV);
            YogArray_push(ENV, $$, ID2VAL($1));
        }
        | names tCOMMA tNAME {
            YogArray_push(ENV, $1, ID2VAL($3));
            $$ = $1;
        }
        ;
super_opt   : /* empty */ {
                $$ = YNIL;
            }
            | tGREATER expr {
                $$ = $2;
            }
            ;
if_tail : else_opt
        | tELIF expr stmts if_tail {
            YogVal node = YUNDEF;
            IF_NEW(node, $2, $3, $4);
            OBJ_ARRAY_NEW($$, node);
        }
        ;
else_opt    : /* empty */ {
                $$ = YNIL;
            }
            | tELSE stmts {
                $$ = $2;
            }
            ;
func_def    : tDEF tNAME tLPAR params tRPAR stmts tEND {
                FUNC_DEF_NEW($$, $2, $4, $6);
            }
            ;
params  : params_without_default tCOMMA params_with_default tCOMMA block_param tCOMMA var_param tCOMMA kw_param {
            PARAMS_NEW($$, $1, $3, $5, $7, $9);
        }
        | params_without_default tCOMMA params_with_default tCOMMA block_param tCOMMA var_param {
            PARAMS_NEW($$, $1, $3, $5, $7, YNIL);
        }
        | params_without_default tCOMMA params_with_default tCOMMA block_param tCOMMA kw_param {
            PARAMS_NEW($$, $1, $3, $5, YNIL, $7);
        }
        | params_without_default tCOMMA params_with_default tCOMMA block_param {
            PARAMS_NEW($$, $1, $3, $5, YNIL, YNIL);
        }
        | params_without_default tCOMMA params_with_default tCOMMA var_param tCOMMA kw_param {
            PARAMS_NEW($$, $1, $3, YNIL, $5, $7);
        }
        | params_without_default tCOMMA params_with_default tCOMMA var_param {
            PARAMS_NEW($$, $1, $3, YNIL, $5, YNIL);
        }
        | params_without_default tCOMMA params_with_default tCOMMA kw_param {
            PARAMS_NEW($$, $1, $3, YNIL, YNIL, $5);
        }
        | params_without_default tCOMMA params_with_default {
            PARAMS_NEW($$, $1, $3, YNIL, YNIL, YNIL);
        }
        | params_without_default tCOMMA block_param tCOMMA var_param tCOMMA kw_param {
            PARAMS_NEW($$, $1, YNIL, $3, $5, $7);
        }
        | params_without_default tCOMMA block_param tCOMMA var_param {
            PARAMS_NEW($$, $1, YNIL, $3, $5, YNIL);
        }
        | params_without_default tCOMMA block_param tCOMMA kw_param {
            PARAMS_NEW($$, $1, YNIL, $3, YNIL, $5);
        }
        | params_without_default tCOMMA block_param {
            PARAMS_NEW($$, $1, YNIL, $3, YNIL, YNIL);
        }
        | params_without_default tCOMMA var_param tCOMMA kw_param {
            PARAMS_NEW($$, $1, YNIL, YNIL, $3, $5);
        }
        | params_without_default tCOMMA var_param {
            PARAMS_NEW($$, $1, YNIL, YNIL, $3, YNIL);
        }
        | params_without_default tCOMMA kw_param {
            PARAMS_NEW($$, $1, YNIL, YNIL, YNIL, $3);
        }
        | params_without_default {
            PARAMS_NEW($$, $1, YNIL, YNIL, YNIL, YNIL);
        }
        | params_with_default tCOMMA block_param tCOMMA var_param tCOMMA kw_param {
            PARAMS_NEW($$, YNIL, $1, $3, $5, $7);
        }
        | params_with_default tCOMMA block_param tCOMMA var_param {
            PARAMS_NEW($$, YNIL, $1, $3, $5, YNIL);
        }
        | params_with_default tCOMMA block_param tCOMMA kw_param {
            PARAMS_NEW($$, YNIL, $1, $3, YNIL, $5);
        }
        | params_with_default tCOMMA block_param {
            PARAMS_NEW($$, YNIL, $1, $3, YNIL, YNIL);
        }
        | params_with_default tCOMMA var_param tCOMMA kw_param {
            PARAMS_NEW($$, YNIL, $1, YNIL, $3, $5);
        }
        | params_with_default tCOMMA var_param {
            PARAMS_NEW($$, YNIL, $1, YNIL, $3, YNIL);
        }
        | params_with_default tCOMMA kw_param {
            PARAMS_NEW($$, YNIL, $1, YNIL, YNIL, $3);
        }
        | params_with_default {
            PARAMS_NEW($$, YNIL, $1, YNIL, YNIL, YNIL);
        }
        | block_param tCOMMA var_param tCOMMA kw_param {
            PARAMS_NEW($$, YNIL, YNIL, $1, $3, $5);
        }
        | block_param tCOMMA var_param {
            PARAMS_NEW($$, YNIL, YNIL, $1, $3, YNIL);
        }
        | block_param tCOMMA kw_param {
            PARAMS_NEW($$, YNIL, YNIL, $1, YNIL, $3);
        }
        | block_param {
            PARAMS_NEW($$, YNIL, YNIL, $1, YNIL, YNIL);
        }
        | var_param tCOMMA kw_param {
            PARAMS_NEW($$, YNIL, YNIL, YNIL, $1, $3);
        }
        | var_param {
            PARAMS_NEW($$, YNIL, YNIL, YNIL, $1, YNIL);
        }
        | kw_param {
            PARAMS_NEW($$, YNIL, YNIL, YNIL, YNIL, $1);
        }
        | /* empty */ {
            $$ = YNIL;
        }
        ;
kw_param    : tDOUBLE_STAR tNAME {
                PARAM_NEW($$, NODE_KW_PARAM, $2, YNIL);
            }
            ;
var_param   : tSTAR tNAME {
                PARAM_NEW($$, NODE_VAR_PARAM, $2, YNIL);
            }
            ;
block_param     : tAMPER tNAME param_default_opt {
                    PARAM_NEW($$, NODE_BLOCK_PARAM, $2, $3);
                }
                ;
param_default_opt   : /* empty */ {
                        $$ = YNIL;
                    }
                    | param_default
                    ;
param_default   : tEQUAL expr {
                    $$ = $2;
                }
                ;
params_without_default  : tNAME {
                            $$ = YogArray_new(ENV);
                            PARAM_ARRAY_PUSH($$, $1, YNIL);
                        }
                        | params_without_default tCOMMA tNAME {
                            PARAM_ARRAY_PUSH($1, $3, YNIL);
                            $$ = $1;
                        }
                        ;
params_with_default     : param_with_default {
                            OBJ_ARRAY_NEW($$, $1);
                        }
                        | params_with_default tCOMMA param_with_default {
                            OBJ_ARRAY_PUSH($$, $1, $3);
                        }
                        ;
param_with_default  : tNAME param_default {
                        PARAM_NEW($$, NODE_PARAM, $1, $2);
                    }
                    ;
args    : expr {
            OBJ_ARRAY_NEW($$, $1);
        }
        | args tCOMMA expr {
            OBJ_ARRAY_PUSH($$, $1, $3);
        }
        ;
expr    : assign_expr
        ;
assign_expr : postfix_expr tEQUAL logical_or_expr {
                ASSIGN_NEW($$, $1, $3);
            }
            | logical_or_expr
            ;
logical_or_expr : logical_and_expr
                ;
logical_and_expr    : not_expr
                    ;
not_expr    : comparison
            ;
comparison  : xor_expr
            | xor_expr comp_op xor_expr {
                METHOD_CALL_NEW1($$, $1, $2, $3);
            }
            ;
comp_op     : tLESS 
            ;
xor_expr    : or_expr
            ;
or_expr : and_expr
        ;
and_expr    : shift_expr
            ;
shift_expr  : match_expr 
            | shift_expr tLSHIFT arith_expr {
                METHOD_CALL_NEW1($$, $1, $2, $3);
            }
            ;
match_expr  : arith_expr 
            | match_expr tEQUAL_TILDA arith_expr {
                METHOD_CALL_NEW1($$, $1, $2, $3);
            }
            ;
arith_expr  : term
            | arith_expr tPLUS term {
                METHOD_CALL_NEW1($$, $1, $2, $3);
            }
            ;
term    : factor
        ;
factor  : power
        ;
power   : postfix_expr
        ;
postfix_expr    : atom 
                | postfix_expr { $<lineno>$ = PARSER->lineno; } tLPAR args_opt tRPAR blockarg_opt {
                    if (NODE($1)->type == NODE_ATTR) {
                        METHOD_CALL_NEW($$, NODE($1)->u.attr.obj, NODE($1)->u.attr.name, $4, $6);
                    }
                    else {
                        FUNC_CALL_NEW($$, $1, $4, $6);
                    }
                    NODE($$)->lineno = $<lineno>2;
                }
                | postfix_expr tLBRACKET expr tRBRACKET {
                    SUBSCRIPT_NEW($$, $1, $3);
                }
                | postfix_expr tDOT tNAME {
                    ATTR_NEW($$, $1, $3);
                }
                ;
atom    : tNAME {
            VARIABLE_NEW($$, $1);
        }
        | tNUMBER {
            LITERAL_NEW($$, $1);
        }
        | tREGEXP {
            LITERAL_NEW($$, $1);
        }
        | tSTRING {
            LITERAL_NEW($$, $1);
        }
        | tTRUE {
            LITERAL_NEW($$, YTRUE);
        }
        | tFALSE {
            LITERAL_NEW($$, YFALSE);
        }
        | t__LINE__ {
            int lineno = PARSER->lineno;
            YogVal val = INT2VAL(lineno);
            LITERAL_NEW($$, val);
        }
        ;
args_opt    : /* empty */ {
                $$ = YNIL;
            }
            | args
            ;
blockarg_opt    : /* empty */ {
                    $$ = YNIL;
                }
                | tDO blockarg_params_opt stmts tEND {
                    BLOCK_ARG_NEW($$, $2, $3);
                }
                | tLBRACE blockarg_params_opt stmts tRBRACE {
                    BLOCK_ARG_NEW($$, $2, $3);
                }
                ;
blockarg_params_opt     : /* empty */ {
                            $$ = YNIL;
                        }
                        | tLBRACKET params tRBRACKET {
                            $$ = $2;
                        }
                        ;
excepts : except {
            OBJ_ARRAY_NEW($$, $1);
        }
        | excepts except {
            OBJ_ARRAY_PUSH($$, $1, $2);
        }
        ;
except  : tEXCEPT expr tAS tNAME newline stmts {
            YOG_ASSERT(ENV, $4 != NO_EXC_VAR, "Too many variables.");
            EXCEPT_BODY_NEW($$, $2, $4, $6);
        }
        | tEXCEPT expr newline stmts {
            EXCEPT_BODY_NEW($$, $2, NO_EXC_VAR, $4);
        }
        | tEXCEPT newline stmts {
            EXCEPT_BODY_NEW($$, YNIL, NO_EXC_VAR, $3);
        }
        ;
newline     : tNEWLINE {
                PARSER->lineno++;
            }
            ;
finally_opt : /* empty */ {
                $$ = YNIL;
            } 
            | tFINALLY stmts {
                $$ = $2;
            }
            ;
%%
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

static void 
YogParser_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogParser* parser = ptr;
#define KEEP(member)    do { \
    parser->member = YogVal_keep(env, parser->member, keeper); \
} while (0)
    KEEP(lexer);
    KEEP(stmts);
#undef KEEP
}

static YogVal 
YogParser_new(YogEnv* env) 
{
    YogParser* parser = ALLOC_OBJ(env, YogParser_keep_children, NULL, YogParser);
    parser->env = env;
    parser->lexer = YUNDEF;
    parser->stmts = YUNDEF;
    parser->lineno = 1;

    return PTR2VAL(parser);
}

YogVal 
YogParser_parse_file(YogEnv* env, const char* filename)
{
    SAVE_LOCALS(env);

    YogVal parser = YUNDEF;
    YogVal lexer = YUNDEF;
    PUSH_LOCALS2(env, parser, lexer);

    parser = YogParser_new(env);
    lexer = YogLexer_new(env);
    PTR_AS(YogParser, parser)->lexer = lexer;
    if (filename != NULL) {
        PTR_AS(YogLexer, lexer)->fp = fopen(filename, "r");
        YogLexer_read_encoding(env, lexer);
    }
    else {
        PTR_AS(YogLexer, lexer)->fp = stdin;
    }

    BOOL old_disable_gc = ENV_VM(env)->disable_gc;
    ENV_VM(env)->disable_gc = TRUE;
    yyparse(PTR_AS(YogParser, parser));
    ENV_VM(env)->disable_gc = old_disable_gc;

    if (filename != NULL) {
        fclose(PTR_AS(YogLexer, lexer)->fp);
    }

    RETURN(env, PTR_AS(YogParser, parser)->stmts);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
