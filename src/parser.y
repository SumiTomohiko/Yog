%{
#include <stdio.h>
#include "yog/yog.h"

static YogEnv* parsing_env = NULL;
static YogVm* parsing_vm = NULL;
static YogArray* parsed_tree = NULL;

void 
Yog_set_parsing_env(YogEnv* env) 
{
    parsing_env = env;
    parsing_vm = ENV_VM(env);
}

YogEnv*
Yog_get_parsing_env()
{
    return parsing_env;
}

YogArray* 
Yog_get_parsed_tree() 
{
    return parsed_tree;
}

#define ENV parsing_env
#define VM  parsing_vm

static void 
yyerror(char* s)
{
    fprintf(stderr, "%s\n", s);
}

static YogNode* 
YogNode_new(YogEnv* env, YogNodeType type) 
{
    YogNode* node = ALLOC_OBJ(env, GCOBJ_NODE, YogNode);
    node->type = type;

    return node;
}

#define NODE_NEW(type)                          YogNode_new(ENV, type)

#define COMMAND_CALL_NEW(result, name)          do { \
    result = NODE_NEW(NODE_COMMAND_CALL); \
    NODE_COMMAND(result) = name; \
} while (0)

#define OBJ_ARRAY_NEW(array, elem)  do { \
    array = YogArray_new(ENV); \
    YogArray_push(ENV, array, YogVal_gcobj(YOGGCOBJ(elem))); \
} while (0)

#define OBJ_ARRAY_PUSH(result, array, elem)     do { \
    if (elem != NULL) { \
        YogArray_push(ENV, array, YogVal_gcobj(YOGGCOBJ(elem))); \
    } \
    result = array; \
} while (0)

#define SYMBOL_ARRAY_PUSH(result, id)   do { \
    YogVal val = YogVal_symbol(id); \
    YogArray_push(ENV, result, val); \
} while (0)

#define FUNC_DEF_NEW(node, name, params, stmts)     do { \
    node = NODE_NEW(NODE_FUNC_DEF); \
    NODE_NAME(node) = name; \
    NODE_PARAMS(node) = params; \
    NODE_STMTS(node) = stmts; \
} while (0)

/* XXX: To avoid warning. Better way? */
int yylex(void);
%}

%union {
    YogArray* array;
    YogNode* node;
    YogVal val;
    ID name;
}

%token COMMA
%token DEF
%token END
%token EQUAL
%token LPAR
%token NAME
%token NEWLINE
%token NUMBER
%token PLUS
%token RPAR

%type<array> args
%type<array> module
%type<array> params
%type<array> stmts
%type<name> NAME
%type<node> and_expr
%type<node> arith_expr
%type<node> assign_expr
%type<node> atom
%type<node> comparison
%type<node> expr
%type<node> factor
%type<node> func_def
%type<node> logical_and_expr
%type<node> logical_or_expr
%type<node> not_expr
%type<node> or_expr
%type<node> power
%type<node> shift_expr
%type<node> stmt
%type<node> term
%type<node> xor_expr
%type<val> NUMBER
%%
module  : stmts {
            parsed_tree = $1;
        }
        ;
stmts   : stmt {
            OBJ_ARRAY_NEW($$, $1);
        }
        | stmts NEWLINE stmt {
            OBJ_ARRAY_PUSH($$, $1, $3);
        }
        ;
stmt    : /* empty */ {
            $$ = NULL;
        }
        | func_def
        | expr
        | NAME args {
            COMMAND_CALL_NEW($$, $1);
            NODE_ARGS($$) = $2;
        }
        ;
func_def    : DEF NAME LPAR params RPAR stmts END {
                FUNC_DEF_NEW($$, $2, $4, $6);
            }
            | DEF NAME LPAR RPAR stmts END {
                FUNC_DEF_NEW($$, $2, NULL, $5);
            }
            ;
params  : NAME {
            $$ = YogArray_new(ENV);
            SYMBOL_ARRAY_PUSH($$, $1);
        }
        | params COMMA NAME {
            SYMBOL_ARRAY_PUSH($1, $3);
            $$ = $1;
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
assign_expr : NAME EQUAL logical_or_expr {
                YogNode* node = NODE_NEW(NODE_ASSIGN);
                NODE_LEFT(node) = $1;
                NODE_RIGHT(node) = $3;
                $$ = node;
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
            ;
xor_expr    : or_expr
            ;
or_expr : and_expr
        ;
and_expr    : shift_expr
            ;
shift_expr  : arith_expr
            ;
arith_expr  : term
            | arith_expr PLUS term {
                YogArray* args = YogArray_new(ENV);
                YogArray_push(ENV, args, YogVal_gcobj(YOGGCOBJ($3)));

                YogNode* node = NODE_NEW(NODE_METHOD_CALL);
                NODE_RECEIVER(node) = $1;
                NODE_METHOD(node) = YogVm_intern(ENV, VM, "+");
                NODE_ARGS(node) = args;

                $$ = node;
            }
            ;
term    : factor
        ;
factor  : power
        ;
power   : atom
        ;
atom    : NAME {
            YogNode* node = NODE_NEW(NODE_VARIABLE);
            NODE_ID(node) = $1;
            $$ = node;
        }
        | NUMBER {
            YogNode* node = NODE_NEW(NODE_LITERAL);
            NODE_VAL(node) = $1;
            $$ = node;
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
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
