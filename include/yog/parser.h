#if !defined(YOG_PARSER_H_INCLUDED)
#define YOG_PARSER_H_INCLUDED

#include <stdio.h>
#include "yog/yog.h"

struct YogToken {
    uint_t type;
    union {
        ID id;
        YogVal val;
    } u;
    uint_t lineno;
};

typedef struct YogToken YogToken;

enum YogBinop {
    /**
     * Don't change this order. compile_visit_binop in src/compile.c depends on
     * this order.
     */
    BINOP_ADD,
    BINOP_SUBTRACT,
    BINOP_MULTIPLY,
    BINOP_DIVIDE,
    BINOP_FLOOR_DIVIDE,
    BINOP_SEARCH,
    BINOP_LSHIFT,
    BINOP_RSHIFT,
    BINOP_MODULO,
    BINOP_POWER,
    BINOP_AND,
    BINOP_OR,
    BINOP_XOR,
    BINOP_LESS,
    BINOP_GREATER,
    BINOP_LESS_EQUAL,
    BINOP_GREATER_EQUAL,
    BINOP_EQUAL,
    BINOP_NOT_EQUAL,
    BINOP_UFO,
};

typedef enum YogBinop YogBinop;

enum YogNodeType {
    NODE_ARGS,
    NODE_ARRAY,
    NODE_ASSIGN,
    NODE_ATTR,
    NODE_BINOP,
    NODE_BLOCK_ARG,
    NODE_BLOCK_PARAM,
    NODE_BREAK,
    NODE_CLASS,
    NODE_CONDITIONAL,
    NODE_DICT,
    NODE_DICT_ELEM,
    NODE_EXCEPT,
    NODE_EXCEPT_BODY,
    NODE_FINALLY,
    NODE_FROM,
    NODE_FUNC_CALL,
    NODE_FUNC_DEF,
    NODE_IF,
    NODE_IMPORT,
    NODE_IMPORTED_ATTR,
    NODE_KW_ARG,
    NODE_KW_PARAM,
    NODE_LITERAL,
    NODE_LOGICAL_AND,
    NODE_LOGICAL_OR,
    NODE_MODULE,
    NODE_MULTI_ASSIGN,
    NODE_MULTI_ASSIGN_LHS,
    NODE_NEXT,
    NODE_NONLOCAL,
    NODE_NOT,
    NODE_PARAM,
    NODE_RAISE,
    NODE_RETURN,
    NODE_SET,
    NODE_SUBSCRIPT,
    NODE_SUPER,
    NODE_VARIABLE,
    NODE_VAR_PARAM,
    NODE_WHILE,
};

typedef enum YogNodeType YogNodeType;

struct YogNode {
    YogNodeType type;
    uint_t lineno;
    union {
        struct {
            YogVal posargs;
            YogVal kwargs;
            YogVal vararg;
            YogVal varkwarg;
            YogVal block;
        } args;
        struct {
            YogVal elems;
        } array;
        struct {
            YogVal left;
            YogVal right;
        } assign;
        struct {
            YogVal obj;
            ID name;
        } attr;
        struct {
            YogBinop op;
            YogVal left;
            YogVal right;
        } binop;
        struct {
            YogVal params;
            YogVal stmts;
            YogVal var_tbl;
        } blockarg;
        struct {
            YogVal exprs;
        } break_;
        struct {
            YogVal test;
            YogVal true_expr;
            YogVal false_expr;
        } conditional;
        struct {
            YogVal elems;
        } dict;
        struct {
            YogVal key;
            YogVal value;
        } dict_elem;
        struct {
            YogVal head;
            YogVal excepts;
            YogVal else_;
        } except;
        struct {
            YogVal types;
            ID var;
            YogVal stmts;
        } except_body;
        struct {
            YogVal head;
            YogVal body;
        } finally;
        struct {
            YogVal pkg;
            YogVal attrs;
        } from;
        struct {
            YogVal callee;
            YogVal args;
            YogVal blockarg;
        } func_call;
        struct {
            YogVal decorators;
            ID name;
            YogVal params;
            YogVal stmts;
        } funcdef;
        struct {
            YogVal test;
            YogVal stmts;
            YogVal tail;
        } if_;
        struct {
            YogVal name;
            YogVal as;
        } import;
        struct {
            ID name;
            YogVal as;
        } imported_attr;
        struct {
            YogVal decorators;
            ID name;
            YogVal super;
            YogVal stmts;
        } klass;
        struct {
            ID name;
            YogVal value;
        } kwarg;
        struct {
            YogVal val;
        } literal;
        struct {
            YogVal left;
            YogVal right;
        } logical_and;
        struct {
            YogVal left;
            YogVal right;
        } logical_or;
        struct {
            ID name;
            YogVal stmts;
        } module;
        struct {
            YogVal lhs;
            YogVal rhs;
        } multi_assign;
        struct {
            YogVal left;
            YogVal middle;
            YogVal right;
        } multi_assign_lhs;
        struct {
            YogVal exprs;
        } next;
        struct {
            YogVal names;
        } nonlocal;
        struct {
            YogVal expr;
        } not;
        struct {
            ID name;
            YogVal default_;
        } param;
        struct {
            YogVal expr;
        } raise;
        struct {
            YogVal exprs;
        } return_;
        struct {
            YogVal elems;
        } set;
        struct {
            YogVal prefix;
            YogVal index;
        } subscript;
        struct {
            ID id;
        } variable;
        struct {
            YogVal test;
            YogVal stmts;
        } while_;
    } u;
};

#define NO_EXC_VAR  (UINT_MAX)

typedef struct YogNode YogNode;

enum YogLexerState {
    LS_EXPR,
    LS_NAME,
    LS_OP,
};

typedef enum YogLexerState YogLexerState;

struct YogLexer {
    enum YogLexerState state;
    FILE* fp;
    YogVal line;
    uint_t next_index;
    YogVal buffer;
    uint_t lineno;
    YogVal heredoc_queue;
    uint_t paren_depth;
    YogVal encoding;
    YogVal ended_type_stack;
    YogVal paren_depth_stack;
};

#define LEXER_LINE(lexer)       PTR_AS(YogLexer, (lexer))->line
#define LEXER_ENCODING(lexer)   PTR_AS(YogString, LEXER_LINE(lexer))->encoding

typedef struct YogLexer YogLexer;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/lexer.c */
YogVal YogLexer_new(YogEnv*);
YogVal YogLexer_next_token(YogEnv*, YogVal, YogHandle*);
void YogLexer_read_encoding(YogEnv*, YogVal);
void YogLexer_set_encoding(YogEnv*, YogVal, YogVal);

/* src/parser.y */
YogVal YogParser_parse(YogEnv*, YogVal);
YogVal YogParser_parse_file(YogEnv*, FILE*, YogHandle*, BOOL);
YogVal YogParser_parse_stdin(YogEnv*, YogHandle*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
