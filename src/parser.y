%{
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

#define KEEP(member)    node->u.member = (*keeper)(env, node->u.member)
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

static YogNode* 
YogNode_new(YogEnv* env, YogParser* parser, YogNodeType type) 
{
    YogNode* node = ALLOC_OBJ(env, YogNode_keep_children, NULL, YogNode);
    node->lineno = parser->lineno;
    node->type = type;

    return node;
}

#define NODE_NEW(type)  YogNode_new(ENV, PARSER, type)

#define LITERAL_NEW(node, val_) do { \
    node = NODE_NEW(NODE_LITERAL); \
    node->u.literal.val = val_; \
} while (0)

#define BLOCK_ARG_NEW(node, params_, stmts_) do { \
    node = NODE_NEW(NODE_BLOCK_ARG); \
    node->u.blockarg.params = params_; \
    node->u.blockarg.stmts = stmts_; \
} while (0)

#define PARAMS_NEW(array, params_without_default, params_with_default, block_param, var_param, kw_param) do { \
    array = YogArray_new(ENV); \
    \
    if (params_without_default != NULL) { \
        YogArray_extend(ENV, array, params_without_default); \
    } \
    \
    if (params_with_default != NULL) { \
        YogArray_extend(ENV, array, params_with_default); \
    } \
    \
    if (block_param != NULL) { \
        YogVal val = PTR2VAL(block_param); \
        YogArray_push(ENV, array, val); \
    } \
    \
    if (var_param != NULL) { \
        YogVal val = PTR2VAL(var_param); \
        YogArray_push(ENV, array, val); \
    } \
    \
    if (kw_param != NULL) { \
        YogVal val = PTR2VAL(kw_param); \
        YogArray_push(ENV, array, val); \
    } \
} while (0)

#define COMMAND_CALL_NEW(node, name_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_COMMAND_CALL); \
    node->u.command_call.name = name_; \
    node->u.command_call.args = args_; \
    node->u.command_call.blockarg = blockarg_; \
} while (0)

#define OBJ_ARRAY_NEW(array, elem) do { \
    if (elem != NULL) { \
        array = YogArray_new(ENV); \
        YogArray_push(ENV, array, PTR2VAL(elem)); \
    } \
    else { \
        array = NULL; \
    } \
} while (0)

#define OBJ_ARRAY_PUSH(result, array, elem) do { \
    if (elem != NULL) { \
        if (array == NULL) { \
            array = YogArray_new(ENV); \
        } \
        YogArray_push(ENV, array, PTR2VAL(elem)); \
    } \
    result = array; \
} while (0)

#define PARAM_NEW(node, type, id, default__) do { \
    node = NODE_NEW(type); \
    node->u.param.name = id; \
    node->u.param.default_ = default__; \
} while (0)

#define PARAM_ARRAY_PUSH(array, id, default_) do { \
    YogNode* node = NULL; \
    PARAM_NEW(node, NODE_PARAM, id, default_); \
    YogArray_push(ENV, array, PTR2VAL(node)); \
} while (0)

#define FUNC_DEF_NEW(node, name_, params_, stmts_) do { \
    node = NODE_NEW(NODE_FUNC_DEF); \
    node->u.funcdef.name = name_; \
    node->u.funcdef.params = params_; \
    node->u.funcdef.stmts = stmts_; \
} while (0)

#define FUNC_CALL_NEW(node, callee_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_FUNC_CALL); \
    node->u.func_call.callee = callee_; \
    node->u.func_call.args = args_; \
    node->u.func_call.blockarg = blockarg_; \
} while (0)

#define VARIABLE_NEW(node, id_) do { \
    node = NODE_NEW(NODE_VARIABLE); \
    node->u.variable.id = id_; \
} while (0)

#define EXCEPT_BODY_NEW(node, type_, var_, stmts_) do { \
    node = NODE_NEW(NODE_EXCEPT_BODY); \
    node->u.except_body.type = type_; \
    node->u.except_body.var = var_; \
    node->u.except_body.stmts = stmts_; \
} while (0)

#define EXCEPT_NEW(node, head_, excepts_, else__) do { \
    node = NODE_NEW(NODE_EXCEPT); \
    node->u.except.head = head_; \
    node->u.except.excepts = excepts_; \
    node->u.except.else_ = else__; \
} while (0)

#define FINALLY_NEW(node, head_, body_) do { \
    node = NODE_NEW(NODE_FINALLY); \
    node->u.finally.head = head_; \
    node->u.finally.body = body_; \
} while (0)

#define EXCEPT_FINALLY_NEW(node, stmts, excepts, else_, finally) do { \
    EXCEPT_NEW(node, stmts, excepts, else_); \
    \
    if (finally != NULL) { \
        YogArray* array = NULL; \
        OBJ_ARRAY_NEW(array, node); \
        FINALLY_NEW(node, array, finally); \
    } \
} while (0)

#define BREAK_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_BREAK); \
    node->u.break_.expr = expr_; \
} while (0)

#define NEXT_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_NEXT); \
    node->u.next.expr = expr_; \
} while (0)

#define RETURN_NEW(node, expr_) do { \
    node = NODE_NEW(NODE_RETURN); \
    node->u.return_.expr = expr_; \
} while (0)

#define METHOD_CALL_NEW(node, recv_, name_, args_, blockarg_) do { \
    node = NODE_NEW(NODE_METHOD_CALL); \
    node->u.method_call.recv = recv_; \
    node->u.method_call.name = name_; \
    node->u.method_call.args = args_; \
    node->u.method_call.blockarg = blockarg_; \
} while (0)

#define METHOD_CALL_NEW1(node, recv, name, arg) do { \
    YogArray* args = YogArray_new(ENV); \
    YogArray_push(ENV, args, PTR2VAL(arg)); \
    METHOD_CALL_NEW(node, recv, name, args, NULL); \
} while (0)

#define IF_NEW(node, test_, stmts_, tail_) do { \
    node = NODE_NEW(NODE_IF); \
    node->u.if_.test = test_; \
    node->u.if_.stmts = stmts_; \
    node->u.if_.tail = tail_; \
} while (0)

#define WHILE_NEW(node, test_, stmts_) do { \
    node = NODE_NEW(NODE_WHILE); \
    node->u.while_.test = test_; \
    node->u.while_.stmts = stmts_; \
} while (0)

#define KLASS_NEW(node, name_, super_, stmts_) do { \
    node = NODE_NEW(NODE_KLASS); \
    node->u.klass.name = name_; \
    node->u.klass.super = super_; \
    node->u.klass.stmts = stmts_; \
} while (0);

#define ASSIGN_NEW(node, left_, right_) do { \
    node = NODE_NEW(NODE_ASSIGN); \
    node->u.assign.left = left_; \
    node->u.assign.right = right_; \
} while (0)

#define SUBSCRIPT_NEW(node, prefix_, index_) do { \
    node = NODE_NEW(NODE_SUBSCRIPT); \
    node->u.subscript.prefix = prefix_; \
    node->u.subscript.index = index_; \
} while (0)

#define ATTR_NEW(node, obj_, name_) do { \
    node = NODE_NEW(NODE_ATTR); \
    node->u.attr.obj = obj_; \
    node->u.attr.name = name_; \
} while (0)

#define NONLOCAL_NEW(node, names_) do { \
    node = NODE_NEW(NODE_NONLOCAL); \
    node->u.nonlocal.names = names_; \
} while (0)
%}

%union {
    struct YogArray* array;
    struct YogNode* node;
    struct YogVal val;
    ID name;
}

%token AMPER
%token AS
%token BAR
%token BREAK
%token CLASS
%token COMMA
%token DEF
%token DIV
%token DO
%token DOT
%token DOUBLE_STAR
%token ELIF
%token ELSE
%token END
%token EQUAL
%token EQUAL_TILDA
%token EXCEPT
%token FINALLY
%token GREATER
%token IF
%token LBRACE
%token LBRACKET
%token LESS
%token LPAR
%token LSHIFT
%token NAME
%token NEWLINE
%token NEXT
%token NONLOCAL
%token NUMBER
%token PLUS
%token RBRACE
%token RBRACKET
%token REGEXP
%token RETURN
%token RPAR
%token STAR
%token STRING
%token TRY
%token WHILE
%token tFALSE
%token tTRUE
%token t__LINE__

%type<array> args
%type<array> args_opt
%type<array> else_opt
%type<array> excepts
%type<array> finally_opt
%type<array> if_tail
%type<array> module
%type<array> names
%type<array> params
%type<array> params_with_default
%type<array> params_without_default
%type<array> stmts
%type<name> EQUAL_TILDA
%type<name> LESS
%type<name> LSHIFT
%type<name> NAME
%type<name> PLUS
%type<name> comp_op
%type<node> and_expr
%type<node> arith_expr
%type<node> assign_expr
%type<node> atom
%type<node> block_param
%type<node> blockarg_opt
%type<node> comparison
%type<node> except
%type<node> expr
%type<node> factor
%type<node> func_def
%type<node> kw_param
%type<node> logical_and_expr
%type<node> logical_or_expr
%type<node> match_expr
%type<node> not_expr
%type<node> or_expr
%type<node> param_default
%type<node> param_default_opt
%type<node> param_with_default
%type<node> postfix_expr
%type<node> power
%type<node> shift_expr
%type<node> stmt
%type<node> super_opt
%type<node> term
%type<node> var_param
%type<node> xor_expr
%type<val> NUMBER
%type<val> REGEXP
%type<val> STRING
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
            $$ = NULL;
        }
        | func_def
        | expr
        | NAME args {
            COMMAND_CALL_NEW($$, $1, $2, NULL);
        }
        /*
        | NAME args DO LPAR params RPAR stmts END {
            YogNode* blockarg = NULL;
            BLOCK_ARG_NEW(blockarg, $5, $7);
            COMMAND_CALL_NEW($$, $1, $2, blockarg);
        }
        */
        | TRY stmts excepts ELSE stmts finally_opt END {
            EXCEPT_FINALLY_NEW($$, $2, $3, $5, $6);
        }
        | TRY stmts excepts finally_opt END {
            EXCEPT_FINALLY_NEW($$, $2, $3, NULL, $4);
        }
        | TRY stmts FINALLY stmts END {
            FINALLY_NEW($$, $2, $4);
        }
        | WHILE expr stmts END {
            WHILE_NEW($$, $2, $3);
        }
        | BREAK {
            BREAK_NEW($$, NULL);
        }
        | BREAK expr {
            BREAK_NEW($$, $2);
        }
        | NEXT {
            NEXT_NEW($$, NULL);
        }
        | NEXT expr {
            NEXT_NEW($$, $2);
        }
        | RETURN {
            RETURN_NEW($$, NULL);
        }
        | RETURN expr {
            RETURN_NEW($$, $2);
        }
        | IF expr stmts if_tail END {
            IF_NEW($$, $2, $3, $4);
        }
        | CLASS NAME super_opt stmts END {
            KLASS_NEW($$, $2, $3, $4);
        }
        | NONLOCAL names {
            NONLOCAL_NEW($$, $2);
        }
        ;
names   : NAME {
            $$ = YogArray_new(ENV);
            YogArray_push(ENV, $$, ID2VAL(NAME));
        }
        | names COMMA NAME {
            YogArray_push(ENV, $1, ID2VAL($3));
            $$ = $1;
        }
        ;
super_opt   : /* empty */ {
                $$ = NULL;
            }
            | GREATER expr {
                $$ = $2;
            }
            ;
if_tail : else_opt
        | ELIF expr stmts if_tail {
            YogNode* node = NULL;
            IF_NEW(node, $2, $3, $4);
            OBJ_ARRAY_NEW($$, node);
        }
        ;
else_opt    : /* empty */ {
                $$ = NULL;
            }
            | ELSE stmts {
                $$ = $2;
            }
            ;
func_def    : DEF NAME LPAR params RPAR stmts END {
                FUNC_DEF_NEW($$, $2, $4, $6);
            }
            ;
params  : params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param {
            PARAMS_NEW($$, $1, $3, $5, $7, $9);
        }
        | params_without_default COMMA params_with_default COMMA block_param COMMA var_param {
            PARAMS_NEW($$, $1, $3, $5, $7, NULL);
        }
        | params_without_default COMMA params_with_default COMMA block_param COMMA kw_param {
            PARAMS_NEW($$, $1, $3, $5, NULL, $7);
        }
        | params_without_default COMMA params_with_default COMMA block_param {
            PARAMS_NEW($$, $1, $3, $5, NULL, NULL);
        }
        | params_without_default COMMA params_with_default COMMA var_param COMMA kw_param {
            PARAMS_NEW($$, $1, $3, NULL, $5, $7);
        }
        | params_without_default COMMA params_with_default COMMA var_param {
            PARAMS_NEW($$, $1, $3, NULL, $5, NULL);
        }
        | params_without_default COMMA params_with_default COMMA kw_param {
            PARAMS_NEW($$, $1, $3, NULL, NULL, $5);
        }
        | params_without_default COMMA params_with_default {
            PARAMS_NEW($$, $1, $3, NULL, NULL, NULL);
        }
        | params_without_default COMMA block_param COMMA var_param COMMA kw_param {
            PARAMS_NEW($$, $1, NULL, $3, $5, $7);
        }
        | params_without_default COMMA block_param COMMA var_param {
            PARAMS_NEW($$, $1, NULL, $3, $5, NULL);
        }
        | params_without_default COMMA block_param COMMA kw_param {
            PARAMS_NEW($$, $1, NULL, $3, NULL, $5);
        }
        | params_without_default COMMA block_param {
            PARAMS_NEW($$, $1, NULL, $3, NULL, NULL);
        }
        | params_without_default COMMA var_param COMMA kw_param {
            PARAMS_NEW($$, $1, NULL, NULL, $3, $5);
        }
        | params_without_default COMMA var_param {
            PARAMS_NEW($$, $1, NULL, NULL, $3, NULL);
        }
        | params_without_default COMMA kw_param {
            PARAMS_NEW($$, $1, NULL, NULL, NULL, $3);
        }
        | params_without_default {
            PARAMS_NEW($$, $1, NULL, NULL, NULL, NULL);
        }
        | params_with_default COMMA block_param COMMA var_param COMMA kw_param {
            PARAMS_NEW($$, NULL, $1, $3, $5, $7);
        }
        | params_with_default COMMA block_param COMMA var_param {
            PARAMS_NEW($$, NULL, $1, $3, $5, NULL);
        }
        | params_with_default COMMA block_param COMMA kw_param {
            PARAMS_NEW($$, NULL, $1, $3, NULL, $5);
        }
        | params_with_default COMMA block_param {
            PARAMS_NEW($$, NULL, $1, $3, NULL, NULL);
        }
        | params_with_default COMMA var_param COMMA kw_param {
            PARAMS_NEW($$, NULL, $1, NULL, $3, $5);
        }
        | params_with_default COMMA var_param {
            PARAMS_NEW($$, NULL, $1, NULL, $3, NULL);
        }
        | params_with_default COMMA kw_param {
            PARAMS_NEW($$, NULL, $1, NULL, NULL, $3);
        }
        | params_with_default {
            PARAMS_NEW($$, NULL, $1, NULL, NULL, NULL);
        }
        | block_param COMMA var_param COMMA kw_param {
            PARAMS_NEW($$, NULL, NULL, $1, $3, $5);
        }
        | block_param COMMA var_param {
            PARAMS_NEW($$, NULL, NULL, $1, $3, NULL);
        }
        | block_param COMMA kw_param {
            PARAMS_NEW($$, NULL, NULL, $1, NULL, $3);
        }
        | block_param {
            PARAMS_NEW($$, NULL, NULL, $1, NULL, NULL);
        }
        | var_param COMMA kw_param {
            PARAMS_NEW($$, NULL, NULL, NULL, $1, $3);
        }
        | var_param {
            PARAMS_NEW($$, NULL, NULL, NULL, $1, NULL);
        }
        | kw_param {
            PARAMS_NEW($$, NULL, NULL, NULL, NULL, $1);
        }
        | /* empty */ {
            $$ = NULL;
        }
        ;
kw_param    : DOUBLE_STAR NAME {
                PARAM_NEW($$, NODE_KW_PARAM, $2, NULL);
            }
            ;
var_param   : STAR NAME {
                PARAM_NEW($$, NODE_VAR_PARAM, $2, NULL);
            }
            ;
block_param     : AMPER NAME param_default_opt {
                    PARAM_NEW($$, NODE_BLOCK_PARAM, $2, $3);
                }
                ;
param_default_opt   : /* empty */ {
                        $$ = NULL;
                    }
                    | param_default
                    ;
param_default   : EQUAL expr {
                    $$ = $2;
                }
                ;
params_without_default  : NAME {
                            $$ = YogArray_new(ENV);
                            PARAM_ARRAY_PUSH($$, $1, NULL);
                        }
                        | params_without_default COMMA NAME {
                            PARAM_ARRAY_PUSH($1, $3, NULL);
                            $$ = $1;
                        }
                        ;
params_with_default     : param_with_default {
                            OBJ_ARRAY_NEW($$, $1);
                        }
                        | params_with_default COMMA param_with_default {
                            OBJ_ARRAY_PUSH($$, $1, $3);
                        }
                        ;
param_with_default  : NAME param_default {
                        PARAM_NEW($$, NODE_PARAM, $1, $2);
                    }
                    ;
args    : expr {
            OBJ_ARRAY_NEW($$, $1);
        }
        | args COMMA expr {
            OBJ_ARRAY_PUSH($$, $1, $3);
        }
        ;
expr    : assign_expr
        ;
assign_expr : postfix_expr EQUAL logical_or_expr {
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
comp_op     : LESS 
            ;
xor_expr    : or_expr
            ;
or_expr : and_expr
        ;
and_expr    : shift_expr
            ;
shift_expr  : match_expr 
            | shift_expr LSHIFT arith_expr {
                METHOD_CALL_NEW1($$, $1, $2, $3);
            }
            ;
match_expr  : arith_expr 
            | match_expr EQUAL_TILDA arith_expr {
                METHOD_CALL_NEW1($$, $1, $2, $3);
            }
            ;
arith_expr  : term
            | arith_expr PLUS term {
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
                | postfix_expr LPAR args_opt RPAR blockarg_opt {
                    if ($1->type == NODE_ATTR) {
                        METHOD_CALL_NEW($$, $1->u.attr.obj, $1->u.attr.name, $3, $5);
                    }
                    else {
                        FUNC_CALL_NEW($$, $1, $3, $5);
                    }
                }
                | postfix_expr LBRACKET expr RBRACKET {
                    SUBSCRIPT_NEW($$, $1, $3);
                }
                | postfix_expr DOT NAME {
                    ATTR_NEW($$, $1, $3);
                }
                ;
atom    : NAME {
            VARIABLE_NEW($$, $1);
        }
        | NUMBER {
            LITERAL_NEW($$, $1);
        }
        | REGEXP {
            LITERAL_NEW($$, $1);
        }
        | STRING {
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
                $$ = NULL;
            }
            | args
            ;
blockarg_opt    : /* empty */ {
                    $$ = NULL;
                }
                | DO LBRACKET params RBRACKET stmts END {
                    BLOCK_ARG_NEW($$, $3, $5);
                }
                ;
excepts : except {
            OBJ_ARRAY_NEW($$, $1);
        }
        | excepts except {
            OBJ_ARRAY_PUSH($$, $1, $2);
        }
        ;
except  : EXCEPT expr AS NAME newline stmts {
            YOG_ASSERT(ENV, $4 != NO_EXC_VAR, "Too many variables.");
            EXCEPT_BODY_NEW($$, $2, $4, $6);
        }
        | EXCEPT expr newline stmts {
            EXCEPT_BODY_NEW($$, $2, NO_EXC_VAR, $4);
        }
        | EXCEPT newline stmts {
            EXCEPT_BODY_NEW($$, NULL, NO_EXC_VAR, $3);
        }
        ;
newline     : NEWLINE {
                PARSER->lineno++;
            }
            ;
finally_opt : /* empty */ {
                $$ = NULL;
            } 
            | FINALLY stmts {
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

void 
YogParser_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogParser* parser = ptr;
#define KEEP(member)    parser->member = (*keeper)(env, parser->member)
    KEEP(lexer);
    KEEP(stmts);
#undef KEEP
}

void 
YogParser_initialize(YogEnv* env, YogParser* parser) 
{
    parser->env = env;
    parser->lexer = NULL;
    parser->stmts = NULL;
    parser->lineno = 1;
}

YogArray* 
YogParser_parse_file(YogEnv* env, YogParser* parser, const char* filename)
{
    YogLexer* lexer = YogLexer_new(env);
    parser->lexer = lexer;
    if (filename != NULL) {
        lexer->fp = fopen(filename, "r");
        YogLexer_read_encoding(env, lexer);
    }
    else {
        lexer->fp = stdin;
    }

    yyparse(parser);

    if (filename != NULL) {
        fclose(lexer->fp);
    }

    return parser->stmts;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
