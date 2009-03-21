/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 9 "parser.y"

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
        PUSH_LOCAL(env, array); \
        OBJ_ARRAY_NEW(array, node); \
        FINALLY_NEW(node, array, finally); \
        POP_LOCALS(env); \
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
#line 383 "parser.c"
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
#ifndef INTERFACE
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
#define YYNOCODE 87
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy171;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 198
#define YYNRULE 121
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
 /*     0 */   320,   50,  149,   27,  123,  124,   82,  127,  128,  129,
 /*    10 */   130,   59,   16,  132,  133,   84,   87,   89,  139,  135,
 /*    20 */   136,  140,  126,   81,  138,  128,  129,  130,   59,  148,
 /*    30 */   132,  133,   84,   87,   89,  139,  135,  136,  140,   52,
 /*    40 */   149,   82,  123,  124,  103,  104,  131,    4,  132,  133,
 /*    50 */    84,   87,   89,  139,  135,  136,  140,  141,   12,   14,
 /*    60 */   126,   81,  138,  128,  129,  130,   59,    8,  132,  133,
 /*    70 */    84,   87,   89,  139,  135,  136,  140,   40,  149,   26,
 /*    80 */   123,  124,  194,  151,   17,  151,  142,  143,  144,  145,
 /*    90 */   146,  147,  141,   18,   29,  151,  121,  151,  126,   81,
 /*   100 */   138,  128,  129,  130,   59,  151,  132,  133,   84,   87,
 /*   110 */    89,  139,  135,  136,  140,   44,  149,   82,  123,  124,
 /*   120 */   197,  142,  143,  144,  145,  146,  147,   31,   85,  139,
 /*   130 */   135,  136,  140,  102,  166,   33,  126,   81,  138,  128,
 /*   140 */   129,  130,   59,    1,  132,  133,   84,   87,   89,  139,
 /*   150 */   135,  136,  140,   61,  149,   82,  123,  124,   98,   99,
 /*   160 */   110,  114,  116,  187,  108,  176,  179,  134,  135,  136,
 /*   170 */   140,  112,  181,  151,  126,   81,  138,  128,  129,  130,
 /*   180 */    59,   28,  132,  133,   84,   87,   89,  139,  135,  136,
 /*   190 */   140,   53,  149,  162,  123,  124,  119,   99,  110,  114,
 /*   200 */   116,  187,   16,    9,  179,  101,  105,  169,   92,   94,
 /*   210 */   173,  158,  126,   81,  138,  128,  129,  130,   59,  103,
 /*   220 */   132,  133,   84,   87,   89,  139,  135,  136,  140,   41,
 /*   230 */   149,  164,  123,  124,  100,  107,  109,  178,  117,  168,
 /*   240 */   179,   16,  111,  113,  183,   43,    3,  173,    4,  150,
 /*   250 */   126,   81,  138,  128,  129,  130,   59,   75,  132,  133,
 /*   260 */    84,   87,   89,  139,  135,  136,  140,   42,  149,   17,
 /*   270 */   123,  124,  190,   16,   30,  103,  104,  106,   91,   29,
 /*   280 */   192,  121,   18,  198,  115,  185,  151,  175,  126,   81,
 /*   290 */   138,  128,  129,  130,   59,   70,  132,  133,   84,   87,
 /*   300 */    89,  139,  135,  136,  140,   62,  149,  174,  123,  124,
 /*   310 */    16,  170,  171,  103,  104,  106,  159,   60,   80,  158,
 /*   320 */   153,  160,  163,  191,  151,   11,  126,   81,  138,  128,
 /*   330 */   129,  130,   59,   67,  132,  133,   84,   87,   89,  139,
 /*   340 */   135,  136,  140,   54,  149,  177,  123,  124,  180,  118,
 /*   350 */   151,  103,  104,  106,  182,   15,  184,  151,  151,  151,
 /*   360 */   151,  186,   20,   34,  126,   81,  138,  128,  129,  130,
 /*   370 */    59,   36,  132,  133,   84,   87,   89,  139,  135,  136,
 /*   380 */   140,   55,  149,   35,  123,  124,   51,  152,  157,   63,
 /*   390 */    95,  161,   97,   32,   37,   38,   10,  165,   45,   65,
 /*   400 */   167,   49,  126,   81,  138,  128,  129,  130,   59,  189,
 /*   410 */   132,  133,   84,   87,   89,  139,  135,  136,  140,   77,
 /*   420 */   149,   66,  123,  124,   46,   68,   76,   69,   39,   47,
 /*   430 */    71,   72,   48,   73,   74,  193,  196,  195,  321,  321,
 /*   440 */   126,   81,  138,  128,  129,  130,   59,  321,  132,  133,
 /*   450 */    84,   87,   89,  139,  135,  136,  140,   78,  149,  321,
 /*   460 */   123,  124,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   470 */   321,  321,  321,  321,  321,  321,  321,  321,  126,   81,
 /*   480 */   138,  128,  129,  130,   59,  321,  132,  133,   84,   87,
 /*   490 */    89,  139,  135,  136,  140,   79,  149,  321,  123,  124,
 /*   500 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   510 */   321,  321,  321,  321,  321,  321,  126,   81,  138,  128,
 /*   520 */   129,  130,   59,  321,  132,  133,   84,   87,   89,  139,
 /*   530 */   135,  136,  140,   57,  149,  321,  123,  124,  321,  321,
 /*   540 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   550 */   321,  321,  321,  321,  126,   81,  138,  128,  129,  130,
 /*   560 */    59,  321,  132,  133,   84,   87,   89,  139,  135,  136,
 /*   570 */   140,   58,  149,  321,  123,  124,  321,  321,  321,  321,
 /*   580 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   590 */   321,  321,  126,   81,  138,  128,  129,  130,   59,  321,
 /*   600 */   132,  133,   84,   87,   89,  139,  135,  136,  140,  122,
 /*   610 */   321,  123,  124,  321,  321,  321,  321,  321,  321,  321,
 /*   620 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  126,
 /*   630 */    81,  138,  128,  129,  130,   59,  321,  132,  133,   84,
 /*   640 */    87,   89,  139,  135,  136,  140,  137,   86,  321,  321,
 /*   650 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   660 */   321,  321,  321,  126,   81,  138,  128,  129,  130,   59,
 /*   670 */   321,  132,  133,   84,   87,   89,  139,  135,  136,  140,
 /*   680 */    90,   13,  321,   56,  321,  321,  321,  321,  321,  321,
 /*   690 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   700 */   126,   81,  138,  128,  129,  130,   59,  321,  132,  133,
 /*   710 */    84,   87,   89,  139,  135,  136,  140,  137,   83,  321,
 /*   720 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   730 */   321,  321,  321,  321,  126,   81,  138,  128,  129,  130,
 /*   740 */    59,   82,  132,  133,   84,   87,   89,  139,  135,  136,
 /*   750 */   140,  125,   88,  139,  135,  136,  140,  321,  321,  321,
 /*   760 */   321,  321,  321,  321,  321,  321,  321,  321,  126,   81,
 /*   770 */   138,  128,  129,  130,   59,  321,  132,  133,   84,   87,
 /*   780 */    89,  139,  135,  136,  140,    5,  321,  321,  321,  321,
 /*   790 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   800 */   321,  321,  126,   81,  138,  128,  129,  130,   59,  321,
 /*   810 */   132,  133,   84,   87,   89,  139,  135,  136,  140,  154,
 /*   820 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   830 */   321,  321,  321,  321,  321,  321,  126,   81,  138,  128,
 /*   840 */   129,  130,   59,  321,  132,  133,   84,   87,   89,  139,
 /*   850 */   135,  136,  140,  155,  321,  321,  321,  321,  321,  321,
 /*   860 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   870 */   126,   81,  138,  128,  129,  130,   59,  321,  132,  133,
 /*   880 */    84,   87,   89,  139,  135,  136,  140,  156,  321,  321,
 /*   890 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   900 */   321,  321,  321,  321,  126,   81,  138,  128,  129,  130,
 /*   910 */    59,  321,  132,  133,   84,   87,   89,  139,  135,  136,
 /*   920 */   140,    6,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   930 */   321,  321,  321,  321,  321,  321,  321,  321,  126,   81,
 /*   940 */   138,  128,  129,  130,   59,  321,  132,  133,   84,   87,
 /*   950 */    89,  139,  135,  136,  140,    7,  321,  321,  321,  321,
 /*   960 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*   970 */   321,  321,  126,   81,  138,  128,  129,  130,   59,  321,
 /*   980 */   132,  133,   84,   87,   89,  139,  135,  136,  140,  172,
 /*   990 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*  1000 */   321,  321,  321,  321,  321,  321,  126,   81,  138,  128,
 /*  1010 */   129,  130,   59,  321,  132,  133,   84,   87,   89,  139,
 /*  1020 */   135,  136,  140,  188,  321,  321,  321,  321,  321,  321,
 /*  1030 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*  1040 */   126,   81,  138,  128,  129,  130,   59,  321,  132,  133,
 /*  1050 */    84,   87,   89,  139,  135,  136,  140,  120,  321,  321,
 /*  1060 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*  1070 */   321,  321,  321,  321,  126,   81,  138,  128,  129,  130,
 /*  1080 */    59,  321,  132,  133,   84,   87,   89,  139,  135,  136,
 /*  1090 */   140,   19,    2,  321,  321,  321,   21,   22,   23,   24,
 /*  1100 */    25,   93,   64,  321,  321,  321,   96,  321,  321,  321,
 /*  1110 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*  1120 */   142,  143,  144,  145,  146,  147,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    43,   44,   45,   22,   47,   48,   66,   67,   68,   69,
 /*    10 */    70,   71,   46,   73,   74,   75,   76,   77,   78,   79,
 /*    20 */    80,   81,   65,   66,   67,   68,   69,   70,   71,   83,
 /*    30 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   44,
 /*    40 */    45,   66,   47,   48,   19,   20,   71,    5,   73,   74,
 /*    50 */    75,   76,   77,   78,   79,   80,   81,    1,   46,    5,
 /*    60 */    65,   66,   67,   68,   69,   70,   71,    3,   73,   74,
 /*    70 */    75,   76,   77,   78,   79,   80,   81,   44,   45,   15,
 /*    80 */    47,   48,   38,   41,   17,   41,   30,   31,   32,   33,
 /*    90 */    34,   35,    1,   39,   27,   41,   29,   41,   65,   66,
 /*   100 */    67,   68,   69,   70,   71,   41,   73,   74,   75,   76,
 /*   110 */    77,   78,   79,   80,   81,   44,   45,   66,   47,   48,
 /*   120 */    23,   30,   31,   32,   33,   34,   35,   72,   77,   78,
 /*   130 */    79,   80,   81,   60,   61,   27,   65,   66,   67,   68,
 /*   140 */    69,   70,   71,   84,   73,   74,   75,   76,   77,   78,
 /*   150 */    79,   80,   81,   44,   45,   66,   47,   48,   56,   57,
 /*   160 */    58,   59,   60,   61,   60,   61,   64,   78,   79,   80,
 /*   170 */    81,   60,   61,   41,   65,   66,   67,   68,   69,   70,
 /*   180 */    71,   14,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   190 */    81,   44,   45,    1,   47,   48,   56,   57,   58,   59,
 /*   200 */    60,   61,   46,   53,   64,   59,   60,   61,   52,   54,
 /*   210 */    64,   55,   65,   66,   67,   68,   69,   70,   71,   19,
 /*   220 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   44,
 /*   230 */    45,   61,   47,   48,   58,   59,   60,   61,   51,   61,
 /*   240 */    64,   46,   59,   60,   61,   50,    3,   64,    5,    4,
 /*   250 */    65,   66,   67,   68,   69,   70,   71,    1,   73,   74,
 /*   260 */    75,   76,   77,   78,   79,   80,   81,   44,   45,   17,
 /*   270 */    47,   48,   85,   46,   22,   19,   20,   21,   51,   27,
 /*   280 */    85,   29,   39,    0,   60,   61,   41,   61,   65,   66,
 /*   290 */    67,   68,   69,   70,   71,    1,   73,   74,   75,   76,
 /*   300 */    77,   78,   79,   80,   81,   44,   45,   63,   47,   48,
 /*   310 */    46,   62,   63,   19,   20,   21,   52,   36,   37,   55,
 /*   320 */     4,    4,    4,    4,   41,   46,   65,   66,   67,   68,
 /*   330 */    69,   70,   71,    1,   73,   74,   75,   76,   77,   78,
 /*   340 */    79,   80,   81,   44,   45,   61,   47,   48,   61,   40,
 /*   350 */    41,   19,   20,   21,   61,   84,   61,   41,   41,   41,
 /*   360 */    41,   61,   13,   24,   65,   66,   67,   68,   69,   70,
 /*   370 */    71,   26,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   380 */    81,   44,   45,   25,   47,   48,   18,    4,    4,    1,
 /*   390 */    13,    1,    1,   17,   13,   13,   18,    1,   13,   13,
 /*   400 */     1,    1,   65,   66,   67,   68,   69,   70,   71,    4,
 /*   410 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   44,
 /*   420 */    45,   13,   47,   48,   13,   13,    1,   13,   13,   13,
 /*   430 */    13,   13,   13,   13,   13,   28,    1,   28,   86,   86,
 /*   440 */    65,   66,   67,   68,   69,   70,   71,   86,   73,   74,
 /*   450 */    75,   76,   77,   78,   79,   80,   81,   44,   45,   86,
 /*   460 */    47,   48,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   470 */    86,   86,   86,   86,   86,   86,   86,   86,   65,   66,
 /*   480 */    67,   68,   69,   70,   71,   86,   73,   74,   75,   76,
 /*   490 */    77,   78,   79,   80,   81,   44,   45,   86,   47,   48,
 /*   500 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   510 */    86,   86,   86,   86,   86,   86,   65,   66,   67,   68,
 /*   520 */    69,   70,   71,   86,   73,   74,   75,   76,   77,   78,
 /*   530 */    79,   80,   81,   44,   45,   86,   47,   48,   86,   86,
 /*   540 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   550 */    86,   86,   86,   86,   65,   66,   67,   68,   69,   70,
 /*   560 */    71,   86,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   570 */    81,   44,   45,   86,   47,   48,   86,   86,   86,   86,
 /*   580 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   590 */    86,   86,   65,   66,   67,   68,   69,   70,   71,   86,
 /*   600 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   45,
 /*   610 */    86,   47,   48,   86,   86,   86,   86,   86,   86,   86,
 /*   620 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   65,
 /*   630 */    66,   67,   68,   69,   70,   71,   86,   73,   74,   75,
 /*   640 */    76,   77,   78,   79,   80,   81,   48,   49,   86,   86,
 /*   650 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   660 */    86,   86,   86,   65,   66,   67,   68,   69,   70,   71,
 /*   670 */    86,   73,   74,   75,   76,   77,   78,   79,   80,   81,
 /*   680 */    82,   46,   86,   48,   86,   86,   86,   86,   86,   86,
 /*   690 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   700 */    65,   66,   67,   68,   69,   70,   71,   86,   73,   74,
 /*   710 */    75,   76,   77,   78,   79,   80,   81,   48,   49,   86,
 /*   720 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   730 */    86,   86,   86,   86,   65,   66,   67,   68,   69,   70,
 /*   740 */    71,   66,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   750 */    81,   48,   77,   78,   79,   80,   81,   86,   86,   86,
 /*   760 */    86,   86,   86,   86,   86,   86,   86,   86,   65,   66,
 /*   770 */    67,   68,   69,   70,   71,   86,   73,   74,   75,   76,
 /*   780 */    77,   78,   79,   80,   81,   48,   86,   86,   86,   86,
 /*   790 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   800 */    86,   86,   65,   66,   67,   68,   69,   70,   71,   86,
 /*   810 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   48,
 /*   820 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   830 */    86,   86,   86,   86,   86,   86,   65,   66,   67,   68,
 /*   840 */    69,   70,   71,   86,   73,   74,   75,   76,   77,   78,
 /*   850 */    79,   80,   81,   48,   86,   86,   86,   86,   86,   86,
 /*   860 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   870 */    65,   66,   67,   68,   69,   70,   71,   86,   73,   74,
 /*   880 */    75,   76,   77,   78,   79,   80,   81,   48,   86,   86,
 /*   890 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   900 */    86,   86,   86,   86,   65,   66,   67,   68,   69,   70,
 /*   910 */    71,   86,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   920 */    81,   48,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   930 */    86,   86,   86,   86,   86,   86,   86,   86,   65,   66,
 /*   940 */    67,   68,   69,   70,   71,   86,   73,   74,   75,   76,
 /*   950 */    77,   78,   79,   80,   81,   48,   86,   86,   86,   86,
 /*   960 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*   970 */    86,   86,   65,   66,   67,   68,   69,   70,   71,   86,
 /*   980 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   48,
 /*   990 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*  1000 */    86,   86,   86,   86,   86,   86,   65,   66,   67,   68,
 /*  1010 */    69,   70,   71,   86,   73,   74,   75,   76,   77,   78,
 /*  1020 */    79,   80,   81,   48,   86,   86,   86,   86,   86,   86,
 /*  1030 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*  1040 */    65,   66,   67,   68,   69,   70,   71,   86,   73,   74,
 /*  1050 */    75,   76,   77,   78,   79,   80,   81,   48,   86,   86,
 /*  1060 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*  1070 */    86,   86,   86,   86,   65,   66,   67,   68,   69,   70,
 /*  1080 */    71,   86,   73,   74,   75,   76,   77,   78,   79,   80,
 /*  1090 */    81,    1,    2,   86,   86,   86,    6,    7,    8,    9,
 /*  1100 */    10,   11,   12,   86,   86,   86,   16,   86,   86,   86,
 /*  1110 */    86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
 /*  1120 */    30,   31,   32,   33,   34,   35,
};
#define YY_SHIFT_USE_DFLT (-20)
#define YY_SHIFT_MAX 121
static const short yy_shift_ofst[] = {
 /*     0 */  1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
 /*    10 */  1090, 1090, 1090, 1090, 1090, 1090, 1090,   91,   56,   91,
 /*    20 */    91,   91,   91,   91,   91,   91,   91,   91,   91,   91,
 /*    30 */    91,   91,  256,  256,   91,   91,   91,  294,  332,  332,
 /*    40 */    54,   64,   64,  243,   42,   25,   25,   25,   25,  -19,
 /*    50 */   283,  281,  245,  316,  317,  318,  309,  319,   44,   97,
 /*    60 */   108,  132,  132,  167,  192,  200,  200,  -19,  200,  200,
 /*    70 */   -19,  200,  200,  200,  200,  -19,  132,  132,  132,  132,
 /*    80 */   108,  252,   67,  349,  339,  345,  349,  358,  345,  345,
 /*    90 */   368,  383,  384,  388,  377,  390,  391,  376,  378,  381,
 /*   100 */   382,  385,  386,  396,  399,  408,  400,  411,  412,  414,
 /*   110 */   415,  416,  417,  418,  419,  420,  421,  405,  425,  407,
 /*   120 */   409,  435,
};
#define YY_REDUCE_USE_DFLT (-61)
#define YY_REDUCE_MAX 80
static const short yy_reduce_ofst[] = {
 /*     0 */   -43,   -5,   33,   71,  109,  147,  185,  223,  261,  299,
 /*    10 */   337,  375,  413,  451,  489,  527,  564,  598,  635,  669,
 /*    20 */   703,  737,  771,  805,  839,  873,  907,  941,  975, 1009,
 /*    30 */   -60,  -25,  102,  140,   51,  675,   89,  176,  146,  183,
 /*    40 */   195,  156,  264,  187,  227,   73,  104,  111,  224,  249,
 /*    50 */   -34,  -54,  -34,  -34,  -34,  -34,   12,  -34,  -34,   55,
 /*    60 */    59,  -34,  -34,  150,  155,  170,  178,  244,  226,  284,
 /*    70 */   244,  287,  293,  295,  300,  244,  279,  -34,  -34,  -34,
 /*    80 */   271,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   201,  201,  201,  201,  201,  201,  201,  201,  201,  201,
 /*    10 */   201,  201,  201,  201,  201,  201,  201,  304,  319,  297,
 /*    20 */   319,  319,  209,  211,  213,  319,  319,  319,  319,  319,
 /*    30 */   319,  319,  258,  258,  319,  319,  319,  319,  319,  319,
 /*    40 */   319,  224,  224,  317,  317,  319,  319,  319,  319,  262,
 /*    50 */   319,  306,  319,  319,  319,  319,  319,  319,  319,  278,
 /*    60 */   309,  318,  225,  220,  319,  319,  319,  319,  319,  319,
 /*    70 */   266,  319,  319,  319,  319,  265,  319,  313,  314,  315,
 /*    80 */   309,  292,  292,  204,  283,  285,  305,  284,  287,  286,
 /*    90 */   319,  319,  319,  319,  217,  319,  319,  319,  319,  242,
 /*   100 */   234,  230,  228,  319,  319,  232,  319,  238,  236,  240,
 /*   110 */   250,  246,  244,  248,  254,  252,  256,  319,  319,  319,
 /*   120 */   319,  319,  200,  202,  203,  271,  272,  273,  275,  276,
 /*   130 */   277,  279,  281,  282,  289,  290,  291,  270,  274,  288,
 /*   140 */   293,  297,  298,  299,  300,  301,  302,  303,  294,  199,
 /*   150 */   307,  316,  205,  208,  210,  212,  214,  215,  222,  223,
 /*   160 */   216,  219,  218,  226,  227,  259,  229,  260,  231,  233,
 /*   170 */   261,  263,  264,  268,  269,  235,  237,  239,  241,  267,
 /*   180 */   243,  245,  247,  249,  251,  253,  255,  257,  221,  206,
 /*   190 */   312,  207,  311,  310,  308,  295,  296,  280,
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
#ifdef YYFALLBACK
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
#ifdef YYTRACKMAXSTACKDEPTH
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
        DPRINTF("PTR_AS(yyParser, parser)->yyidx=%d, i=%d", PTR_AS(yyParser, parser)->yyidx, i);
        YogVal val = PTR_AS(yyParser, parser)->yystack[i + 1].minor.yy0;
        YogVal_print(env, val);
    }
    DPRINTF("-------------------- end of stack --------------------");
}
#endif

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
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

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "NAME",          "TRY",           "ELSE",        
  "END",           "FINALLY",       "WHILE",         "BREAK",       
  "NEXT",          "RETURN",        "IF",            "CLASS",       
  "NONLOCAL",      "COMMA",         "GREATER",       "ELIF",        
  "DEF",           "LPAR",          "RPAR",          "DOUBLE_STAR", 
  "STAR",          "AMPER",         "EQUAL",         "LESS",        
  "LSHIFT",        "EQUAL_TILDA",   "PLUS",          "LBRACKET",    
  "RBRACKET",      "DOT",           "NUMBER",        "REGEXP",      
  "STRING",        "TRUE",          "FALSE",         "LINE",        
  "DO",            "LBRACE",        "RBRACE",        "EXCEPT",      
  "AS",            "NEWLINE",       "error",         "module",      
  "stmts",         "stmt",          "newline",       "func_def",    
  "expr",          "args",          "excepts",       "finally_opt", 
  "if_tail",       "super_opt",     "names",         "else_opt",    
  "params",        "params_without_default",  "params_with_default",  "block_param", 
  "var_param",     "kw_param",      "param_default_opt",  "param_default",
  "param_with_default",  "assign_expr",   "postfix_expr",  "logical_or_expr",
  "logical_and_expr",  "not_expr",      "comparison",    "xor_expr",    
  "comp_op",       "or_expr",       "and_expr",      "shift_expr",  
  "match_expr",    "arith_expr",    "term",          "factor",      
  "power",         "atom",          "args_opt",      "blockarg_opt",
  "blockarg_params_opt",  "except",      
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "module ::= stmts",
 /*   1 */ "stmts ::= stmt",
 /*   2 */ "stmts ::= stmts newline stmt",
 /*   3 */ "stmt ::=",
 /*   4 */ "stmt ::= func_def",
 /*   5 */ "stmt ::= expr",
 /*   6 */ "stmt ::= NAME args",
 /*   7 */ "stmt ::= TRY stmts excepts ELSE stmts finally_opt END",
 /*   8 */ "stmt ::= TRY stmts excepts finally_opt END",
 /*   9 */ "stmt ::= TRY stmts FINALLY stmts END",
 /*  10 */ "stmt ::= WHILE expr stmts END",
 /*  11 */ "stmt ::= BREAK",
 /*  12 */ "stmt ::= BREAK expr",
 /*  13 */ "stmt ::= NEXT",
 /*  14 */ "stmt ::= NEXT expr",
 /*  15 */ "stmt ::= RETURN",
 /*  16 */ "stmt ::= RETURN expr",
 /*  17 */ "stmt ::= IF expr stmts if_tail END",
 /*  18 */ "stmt ::= CLASS NAME super_opt stmts END",
 /*  19 */ "stmt ::= NONLOCAL names",
 /*  20 */ "names ::= NAME",
 /*  21 */ "names ::= names COMMA NAME",
 /*  22 */ "super_opt ::=",
 /*  23 */ "super_opt ::= GREATER expr",
 /*  24 */ "if_tail ::= else_opt",
 /*  25 */ "if_tail ::= ELIF expr stmts if_tail",
 /*  26 */ "else_opt ::=",
 /*  27 */ "else_opt ::= ELSE stmts",
 /*  28 */ "func_def ::= DEF NAME LPAR params RPAR stmts END",
 /*  29 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  30 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param",
 /*  31 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param",
 /*  32 */ "params ::= params_without_default COMMA params_with_default COMMA block_param",
 /*  33 */ "params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param",
 /*  34 */ "params ::= params_without_default COMMA params_with_default COMMA var_param",
 /*  35 */ "params ::= params_without_default COMMA params_with_default COMMA kw_param",
 /*  36 */ "params ::= params_without_default COMMA params_with_default",
 /*  37 */ "params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  38 */ "params ::= params_without_default COMMA block_param COMMA var_param",
 /*  39 */ "params ::= params_without_default COMMA block_param COMMA kw_param",
 /*  40 */ "params ::= params_without_default COMMA block_param",
 /*  41 */ "params ::= params_without_default COMMA var_param COMMA kw_param",
 /*  42 */ "params ::= params_without_default COMMA var_param",
 /*  43 */ "params ::= params_without_default COMMA kw_param",
 /*  44 */ "params ::= params_without_default",
 /*  45 */ "params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  46 */ "params ::= params_with_default COMMA block_param COMMA var_param",
 /*  47 */ "params ::= params_with_default COMMA block_param COMMA kw_param",
 /*  48 */ "params ::= params_with_default COMMA block_param",
 /*  49 */ "params ::= params_with_default COMMA var_param COMMA kw_param",
 /*  50 */ "params ::= params_with_default COMMA var_param",
 /*  51 */ "params ::= params_with_default COMMA kw_param",
 /*  52 */ "params ::= params_with_default",
 /*  53 */ "params ::= block_param COMMA var_param COMMA kw_param",
 /*  54 */ "params ::= block_param COMMA var_param",
 /*  55 */ "params ::= block_param COMMA kw_param",
 /*  56 */ "params ::= block_param",
 /*  57 */ "params ::= var_param COMMA kw_param",
 /*  58 */ "params ::= var_param",
 /*  59 */ "params ::= kw_param",
 /*  60 */ "params ::=",
 /*  61 */ "kw_param ::= DOUBLE_STAR NAME",
 /*  62 */ "var_param ::= STAR NAME",
 /*  63 */ "block_param ::= AMPER NAME param_default_opt",
 /*  64 */ "param_default_opt ::=",
 /*  65 */ "param_default_opt ::= param_default",
 /*  66 */ "param_default ::= EQUAL expr",
 /*  67 */ "params_without_default ::= NAME",
 /*  68 */ "params_without_default ::= params_without_default COMMA NAME",
 /*  69 */ "params_with_default ::= param_with_default",
 /*  70 */ "params_with_default ::= params_with_default COMMA param_with_default",
 /*  71 */ "param_with_default ::= NAME param_default",
 /*  72 */ "args ::= expr",
 /*  73 */ "args ::= args COMMA expr",
 /*  74 */ "expr ::= assign_expr",
 /*  75 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /*  76 */ "assign_expr ::= logical_or_expr",
 /*  77 */ "logical_or_expr ::= logical_and_expr",
 /*  78 */ "logical_and_expr ::= not_expr",
 /*  79 */ "not_expr ::= comparison",
 /*  80 */ "comparison ::= xor_expr",
 /*  81 */ "comparison ::= xor_expr comp_op xor_expr",
 /*  82 */ "comp_op ::= LESS",
 /*  83 */ "xor_expr ::= or_expr",
 /*  84 */ "or_expr ::= and_expr",
 /*  85 */ "and_expr ::= shift_expr",
 /*  86 */ "shift_expr ::= match_expr",
 /*  87 */ "shift_expr ::= shift_expr LSHIFT arith_expr",
 /*  88 */ "match_expr ::= arith_expr",
 /*  89 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /*  90 */ "arith_expr ::= term",
 /*  91 */ "arith_expr ::= arith_expr PLUS term",
 /*  92 */ "term ::= factor",
 /*  93 */ "factor ::= power",
 /*  94 */ "power ::= postfix_expr",
 /*  95 */ "postfix_expr ::= atom",
 /*  96 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /*  97 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /*  98 */ "postfix_expr ::= postfix_expr DOT NAME",
 /*  99 */ "atom ::= NAME",
 /* 100 */ "atom ::= NUMBER",
 /* 101 */ "atom ::= REGEXP",
 /* 102 */ "atom ::= STRING",
 /* 103 */ "atom ::= TRUE",
 /* 104 */ "atom ::= FALSE",
 /* 105 */ "atom ::= LINE",
 /* 106 */ "args_opt ::=",
 /* 107 */ "args_opt ::= args",
 /* 108 */ "blockarg_opt ::=",
 /* 109 */ "blockarg_opt ::= DO blockarg_params_opt stmts END",
 /* 110 */ "blockarg_opt ::= LBRACE blockarg_params_opt stmts RBRACE",
 /* 111 */ "blockarg_params_opt ::=",
 /* 112 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 113 */ "excepts ::= except",
 /* 114 */ "excepts ::= excepts except",
 /* 115 */ "except ::= EXCEPT expr AS NAME newline stmts",
 /* 116 */ "except ::= EXCEPT expr newline stmts",
 /* 117 */ "except ::= EXCEPT newline stmts",
 /* 118 */ "newline ::= NEWLINE",
 /* 119 */ "finally_opt ::=",
 /* 120 */ "finally_opt ::= FINALLY stmts",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

static void 
LemonParser_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    yyParser* pParser = ptr;
    int i;
    for (i = 0; i < pParser->yyidx; i++) {
        YogVal minor = pParser->yystack[i + 1].minor.yy0;
        pParser->yystack[i + 1].minor.yy0 = YogVal_keep(env, minor, keeper);
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
  yyParser *pParser = ALLOC_OBJ(env, LemonParser_keep_children, NULL, yyParser);
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#else
    int i;
    for (i = 0; i < YYSTACKDEPTH; i++) {
        pParser->yystack[i].minor.yy0 = YUNDEF;
    }
#endif
  }
  return PTR2VAL(pParser);
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
#ifndef NDEBUG
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
#ifdef YYTRACKMAXSTACKDEPTH
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
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(parser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
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
#ifdef YYERRORSYMBOL
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
#ifdef YYERRORSYMBOL
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
#ifndef NDEBUG
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
  YogVal parser, 
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  PTR_AS(yyParser, parser)->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
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
#ifndef NDEBUG
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
  { 43, 1 },
  { 44, 1 },
  { 44, 3 },
  { 45, 0 },
  { 45, 1 },
  { 45, 1 },
  { 45, 2 },
  { 45, 7 },
  { 45, 5 },
  { 45, 5 },
  { 45, 4 },
  { 45, 1 },
  { 45, 2 },
  { 45, 1 },
  { 45, 2 },
  { 45, 1 },
  { 45, 2 },
  { 45, 5 },
  { 45, 5 },
  { 45, 2 },
  { 54, 1 },
  { 54, 3 },
  { 53, 0 },
  { 53, 2 },
  { 52, 1 },
  { 52, 4 },
  { 55, 0 },
  { 55, 2 },
  { 47, 7 },
  { 56, 9 },
  { 56, 7 },
  { 56, 7 },
  { 56, 5 },
  { 56, 7 },
  { 56, 5 },
  { 56, 5 },
  { 56, 3 },
  { 56, 7 },
  { 56, 5 },
  { 56, 5 },
  { 56, 3 },
  { 56, 5 },
  { 56, 3 },
  { 56, 3 },
  { 56, 1 },
  { 56, 7 },
  { 56, 5 },
  { 56, 5 },
  { 56, 3 },
  { 56, 5 },
  { 56, 3 },
  { 56, 3 },
  { 56, 1 },
  { 56, 5 },
  { 56, 3 },
  { 56, 3 },
  { 56, 1 },
  { 56, 3 },
  { 56, 1 },
  { 56, 1 },
  { 56, 0 },
  { 61, 2 },
  { 60, 2 },
  { 59, 3 },
  { 62, 0 },
  { 62, 1 },
  { 63, 2 },
  { 57, 1 },
  { 57, 3 },
  { 58, 1 },
  { 58, 3 },
  { 64, 2 },
  { 49, 1 },
  { 49, 3 },
  { 48, 1 },
  { 65, 3 },
  { 65, 1 },
  { 67, 1 },
  { 68, 1 },
  { 69, 1 },
  { 70, 1 },
  { 70, 3 },
  { 72, 1 },
  { 71, 1 },
  { 73, 1 },
  { 74, 1 },
  { 75, 1 },
  { 75, 3 },
  { 76, 1 },
  { 76, 3 },
  { 77, 1 },
  { 77, 3 },
  { 78, 1 },
  { 79, 1 },
  { 80, 1 },
  { 66, 1 },
  { 66, 5 },
  { 66, 4 },
  { 66, 3 },
  { 81, 1 },
  { 81, 1 },
  { 81, 1 },
  { 81, 1 },
  { 81, 1 },
  { 81, 1 },
  { 81, 1 },
  { 82, 0 },
  { 82, 1 },
  { 83, 0 },
  { 83, 4 },
  { 83, 4 },
  { 84, 0 },
  { 84, 3 },
  { 50, 1 },
  { 50, 2 },
  { 85, 6 },
  { 85, 4 },
  { 85, 3 },
  { 46, 1 },
  { 51, 0 },
  { 51, 2 },
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
  locals.vals[4] = NULL;
  PUSH_LOCAL_TABLE(env, locals);

  ParseARG_FETCH;
#ifndef NDEBUG
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
#line 385 "parser.y"
{
    *pval = yymsp[0].minor.yy171;
}
#line 1567 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 69: /* params_with_default ::= param_with_default */
      case 72: /* args ::= expr */
      case 113: /* excepts ::= except */
#line 389 "parser.y"
{
    OBJ_ARRAY_NEW(yygotominor.yy171, yymsp[0].minor.yy171);
}
#line 1577 "parser.c"
        break;
      case 2: /* stmts ::= stmts newline stmt */
      case 70: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 73: /* args ::= args COMMA expr */
#line 392 "parser.y"
{
    OBJ_ARRAY_PUSH(yygotominor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
}
#line 1586 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 22: /* super_opt ::= */
      case 26: /* else_opt ::= */
      case 64: /* param_default_opt ::= */
      case 106: /* args_opt ::= */
      case 108: /* blockarg_opt ::= */
      case 111: /* blockarg_params_opt ::= */
      case 119: /* finally_opt ::= */
#line 396 "parser.y"
{
    yygotominor.yy171 = YNIL;
}
#line 1600 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 23: /* super_opt ::= GREATER expr */
      case 24: /* if_tail ::= else_opt */
      case 27: /* else_opt ::= ELSE stmts */
      case 65: /* param_default_opt ::= param_default */
      case 66: /* param_default ::= EQUAL expr */
      case 74: /* expr ::= assign_expr */
      case 76: /* assign_expr ::= logical_or_expr */
      case 77: /* logical_or_expr ::= logical_and_expr */
      case 78: /* logical_and_expr ::= not_expr */
      case 79: /* not_expr ::= comparison */
      case 80: /* comparison ::= xor_expr */
      case 83: /* xor_expr ::= or_expr */
      case 84: /* or_expr ::= and_expr */
      case 85: /* and_expr ::= shift_expr */
      case 86: /* shift_expr ::= match_expr */
      case 88: /* match_expr ::= arith_expr */
      case 90: /* arith_expr ::= term */
      case 92: /* term ::= factor */
      case 93: /* factor ::= power */
      case 94: /* power ::= postfix_expr */
      case 95: /* postfix_expr ::= atom */
      case 107: /* args_opt ::= args */
      case 120: /* finally_opt ::= FINALLY stmts */
#line 399 "parser.y"
{
    yygotominor.yy171 = yymsp[0].minor.yy171;
}
#line 1630 "parser.c"
        break;
      case 5: /* stmt ::= expr */
#line 402 "parser.y"
{
    if (PTR_AS(YogNode, yymsp[0].minor.yy171)->type == NODE_VARIABLE) {
        YogVal args = YNIL;
        YogVal blockarg = YNIL;
        COMMAND_CALL_NEW(yygotominor.yy171, PTR_AS(YogNode, yymsp[0].minor.yy171)->u.variable.id, args, blockarg);
    }
    else {
        yygotominor.yy171 = yymsp[0].minor.yy171;
    }
}
#line 1644 "parser.c"
        break;
      case 6: /* stmt ::= NAME args */
#line 412 "parser.y"
{
    YogVal blockarg = YNIL;
    COMMAND_CALL_NEW(yygotominor.yy171, PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id, yymsp[0].minor.yy171, blockarg);
}
#line 1652 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 423 "parser.y"
{
    EXCEPT_FINALLY_NEW(yygotominor.yy171, yymsp[-5].minor.yy171, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, yymsp[-1].minor.yy171);
}
#line 1659 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts excepts finally_opt END */
#line 426 "parser.y"
{
    YogVal finally = YNIL;
    EXCEPT_FINALLY_NEW(yygotominor.yy171, yymsp[-3].minor.yy171, yymsp[-2].minor.yy171, finally, yymsp[-1].minor.yy171);
}
#line 1667 "parser.c"
        break;
      case 9: /* stmt ::= TRY stmts FINALLY stmts END */
#line 430 "parser.y"
{
    FINALLY_NEW(yygotominor.yy171, yymsp[-3].minor.yy171, yymsp[-1].minor.yy171);
}
#line 1674 "parser.c"
        break;
      case 10: /* stmt ::= WHILE expr stmts END */
#line 433 "parser.y"
{
    WHILE_NEW(yygotominor.yy171, yymsp[-2].minor.yy171, yymsp[-1].minor.yy171);
}
#line 1681 "parser.c"
        break;
      case 11: /* stmt ::= BREAK */
#line 436 "parser.y"
{
    YogVal expr = YNIL;
    BREAK_NEW(yygotominor.yy171, expr);
}
#line 1689 "parser.c"
        break;
      case 12: /* stmt ::= BREAK expr */
#line 440 "parser.y"
{
    BREAK_NEW(yygotominor.yy171, yymsp[0].minor.yy171);
}
#line 1696 "parser.c"
        break;
      case 13: /* stmt ::= NEXT */
#line 443 "parser.y"
{
    YogVal expr = YNIL;
    NEXT_NEW(yygotominor.yy171, expr);
}
#line 1704 "parser.c"
        break;
      case 14: /* stmt ::= NEXT expr */
#line 447 "parser.y"
{
    NEXT_NEW(yygotominor.yy171, yymsp[0].minor.yy171);
}
#line 1711 "parser.c"
        break;
      case 15: /* stmt ::= RETURN */
#line 450 "parser.y"
{
    YogVal expr = YNIL;
    RETURN_NEW(yygotominor.yy171, expr);
}
#line 1719 "parser.c"
        break;
      case 16: /* stmt ::= RETURN expr */
#line 454 "parser.y"
{
    RETURN_NEW(yygotominor.yy171, yymsp[0].minor.yy171);
}
#line 1726 "parser.c"
        break;
      case 17: /* stmt ::= IF expr stmts if_tail END */
#line 457 "parser.y"
{
    IF_NEW(yygotominor.yy171, yymsp[-3].minor.yy171, yymsp[-2].minor.yy171, yymsp[-1].minor.yy171);
}
#line 1733 "parser.c"
        break;
      case 18: /* stmt ::= CLASS NAME super_opt stmts END */
#line 460 "parser.y"
{
    KLASS_NEW(yygotominor.yy171, PTR_AS(YogToken, yymsp[-3].minor.yy0)->u.id, yymsp[-2].minor.yy171, yymsp[-1].minor.yy171);
}
#line 1740 "parser.c"
        break;
      case 19: /* stmt ::= NONLOCAL names */
#line 463 "parser.y"
{
    NONLOCAL_NEW(yygotominor.yy171, yymsp[0].minor.yy171);
}
#line 1747 "parser.c"
        break;
      case 20: /* names ::= NAME */
#line 467 "parser.y"
{
    yygotominor.yy171 = YogArray_new(env);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    YogArray_push(env, yygotominor.yy171, ID2VAL(id));
}
#line 1756 "parser.c"
        break;
      case 21: /* names ::= names COMMA NAME */
#line 472 "parser.y"
{
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    YogArray_push(env, yymsp[-2].minor.yy171, ID2VAL(id));
    yygotominor.yy171 = yymsp[-2].minor.yy171;
}
#line 1765 "parser.c"
        break;
      case 25: /* if_tail ::= ELIF expr stmts if_tail */
#line 488 "parser.y"
{
    YogVal node = YUNDEF;
    IF_NEW(node, yymsp[-2].minor.yy171, yymsp[-1].minor.yy171, yymsp[0].minor.yy171);
    OBJ_ARRAY_NEW(yygotominor.yy171, node);
}
#line 1774 "parser.c"
        break;
      case 28: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 501 "parser.y"
{
    FUNC_DEF_NEW(yygotominor.yy171, PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id, yymsp[-3].minor.yy171, yymsp[-1].minor.yy171);
}
#line 1781 "parser.c"
        break;
      case 29: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 505 "parser.y"
{
    PARAMS_NEW(yygotominor.yy171, yymsp[-8].minor.yy171, yymsp[-6].minor.yy171, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
}
#line 1788 "parser.c"
        break;
      case 30: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 508 "parser.y"
{
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-6].minor.yy171, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171, kw_param);
}
#line 1796 "parser.c"
        break;
      case 31: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 512 "parser.y"
{
    YogVal var_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-6].minor.yy171, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, var_param, yymsp[0].minor.yy171);
}
#line 1804 "parser.c"
        break;
      case 32: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 516 "parser.y"
{
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171, var_param, kw_param);
}
#line 1813 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 521 "parser.y"
{
    YogVal block_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-6].minor.yy171, yymsp[-4].minor.yy171, block_param, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
}
#line 1821 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 525 "parser.y"
{
    YogVal block_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, block_param, yymsp[0].minor.yy171, kw_param);
}
#line 1830 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 530 "parser.y"
{
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, block_param, var_param, yymsp[0].minor.yy171);
}
#line 1839 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default */
#line 535 "parser.y"
{
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171, block_param, var_param, kw_param);
}
#line 1849 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 541 "parser.y"
{
    YogVal params_with_default = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-6].minor.yy171, params_with_default, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
}
#line 1857 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 545 "parser.y"
{
    YogVal params_with_default = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-4].minor.yy171, params_with_default, yymsp[-2].minor.yy171, yymsp[0].minor.yy171, kw_param);
}
#line 1866 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 550 "parser.y"
{
    YogVal params_with_default = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-4].minor.yy171, params_with_default, yymsp[-2].minor.yy171, var_param, yymsp[0].minor.yy171);
}
#line 1875 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA block_param */
#line 555 "parser.y"
{
    YogVal params_with_default = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-2].minor.yy171, params_with_default, yymsp[0].minor.yy171, var_param, kw_param);
}
#line 1885 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 561 "parser.y"
{
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-4].minor.yy171, params_with_default, block_param, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
}
#line 1894 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA var_param */
#line 566 "parser.y"
{
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-2].minor.yy171, params_with_default, block_param, yymsp[0].minor.yy171, kw_param);
}
#line 1904 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA kw_param */
#line 572 "parser.y"
{
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[-2].minor.yy171, params_with_default, block_param, var_param, yymsp[0].minor.yy171);
}
#line 1914 "parser.c"
        break;
      case 44: /* params ::= params_without_default */
#line 578 "parser.y"
{
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, yymsp[0].minor.yy171, params_with_default, block_param, var_param, kw_param);
}
#line 1925 "parser.c"
        break;
      case 45: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 585 "parser.y"
{
    YogVal params_without_default = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, yymsp[-6].minor.yy171, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
}
#line 1933 "parser.c"
        break;
      case 46: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 589 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171, kw_param);
}
#line 1942 "parser.c"
        break;
      case 47: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 594 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, var_param, yymsp[0].minor.yy171);
}
#line 1951 "parser.c"
        break;
      case 48: /* params ::= params_with_default COMMA block_param */
#line 599 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, yymsp[-2].minor.yy171, yymsp[0].minor.yy171, var_param, kw_param);
}
#line 1961 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 605 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal block_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, yymsp[-4].minor.yy171, block_param, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
}
#line 1970 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA var_param */
#line 610 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal block_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, yymsp[-2].minor.yy171, block_param, yymsp[0].minor.yy171, kw_param);
}
#line 1980 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA kw_param */
#line 616 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, yymsp[-2].minor.yy171, block_param, var_param, yymsp[0].minor.yy171);
}
#line 1990 "parser.c"
        break;
      case 52: /* params ::= params_with_default */
#line 622 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, yymsp[0].minor.yy171, block_param, var_param, kw_param);
}
#line 2001 "parser.c"
        break;
      case 53: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 629 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, params_with_default, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
}
#line 2010 "parser.c"
        break;
      case 54: /* params ::= block_param COMMA var_param */
#line 634 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, params_with_default, yymsp[-2].minor.yy171, yymsp[0].minor.yy171, kw_param);
}
#line 2020 "parser.c"
        break;
      case 55: /* params ::= block_param COMMA kw_param */
#line 640 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, params_with_default, yymsp[-2].minor.yy171, var_param, yymsp[0].minor.yy171);
}
#line 2030 "parser.c"
        break;
      case 56: /* params ::= block_param */
#line 646 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, params_with_default, yymsp[0].minor.yy171, var_param, kw_param);
}
#line 2041 "parser.c"
        break;
      case 57: /* params ::= var_param COMMA kw_param */
#line 653 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, params_with_default, block_param, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
}
#line 2051 "parser.c"
        break;
      case 58: /* params ::= var_param */
#line 659 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, params_with_default, block_param, yymsp[0].minor.yy171, kw_param);
}
#line 2062 "parser.c"
        break;
      case 59: /* params ::= kw_param */
#line 666 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, params_with_default, block_param, var_param, yymsp[0].minor.yy171);
}
#line 2073 "parser.c"
        break;
      case 60: /* params ::= */
#line 673 "parser.y"
{
    YogVal params_without_default = YNIL;
    YogVal params_with_default = YNIL;
    YogVal block_param = YNIL;
    YogVal var_param = YNIL;
    YogVal kw_param = YNIL;
    PARAMS_NEW(yygotominor.yy171, params_without_default, params_with_default, block_param, var_param, kw_param);
}
#line 2085 "parser.c"
        break;
      case 61: /* kw_param ::= DOUBLE_STAR NAME */
#line 682 "parser.y"
{
    YogVal default_ = YNIL;
    PARAM_NEW(yygotominor.yy171, NODE_KW_PARAM, PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id, default_);
}
#line 2093 "parser.c"
        break;
      case 62: /* var_param ::= STAR NAME */
#line 687 "parser.y"
{
    YogVal default_ = YNIL;
    PARAM_NEW(yygotominor.yy171, NODE_VAR_PARAM, PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id, default_);
}
#line 2101 "parser.c"
        break;
      case 63: /* block_param ::= AMPER NAME param_default_opt */
#line 692 "parser.y"
{
    PARAM_NEW(yygotominor.yy171, NODE_BLOCK_PARAM, PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id, yymsp[0].minor.yy171);
}
#line 2108 "parser.c"
        break;
      case 67: /* params_without_default ::= NAME */
#line 707 "parser.y"
{
    yygotominor.yy171 = YogArray_new(env);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    PARAM_ARRAY_PUSH(yygotominor.yy171, id, YNIL);
}
#line 2117 "parser.c"
        break;
      case 68: /* params_without_default ::= params_without_default COMMA NAME */
#line 712 "parser.y"
{
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    PARAM_ARRAY_PUSH(yymsp[-2].minor.yy171, id, YNIL);
    yygotominor.yy171 = yymsp[-2].minor.yy171;
}
#line 2126 "parser.c"
        break;
      case 71: /* param_with_default ::= NAME param_default */
#line 725 "parser.y"
{
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    PARAM_NEW(yygotominor.yy171, NODE_PARAM, id, yymsp[0].minor.yy171);
}
#line 2134 "parser.c"
        break;
      case 75: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 741 "parser.y"
{
    ASSIGN_NEW(yygotominor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
}
#line 2141 "parser.c"
        break;
      case 81: /* comparison ::= xor_expr comp_op xor_expr */
#line 763 "parser.y"
{
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy171)->u.id;
    METHOD_CALL_NEW1(yygotominor.yy171, yymsp[-2].minor.yy171, id, yymsp[0].minor.yy171);
}
#line 2149 "parser.c"
        break;
      case 82: /* comp_op ::= LESS */
      case 118: /* newline ::= NEWLINE */
#line 768 "parser.y"
{
    yygotominor.yy171 = yymsp[0].minor.yy0;
}
#line 2157 "parser.c"
        break;
      case 87: /* shift_expr ::= shift_expr LSHIFT arith_expr */
      case 89: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
      case 91: /* arith_expr ::= arith_expr PLUS term */
#line 787 "parser.y"
{
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    METHOD_CALL_NEW1(yygotominor.yy171, yymsp[-2].minor.yy171, id, yymsp[0].minor.yy171);
}
#line 2167 "parser.c"
        break;
      case 96: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 823 "parser.y"
{
    if (NODE(yymsp[-4].minor.yy171)->type == NODE_ATTR) {
        METHOD_CALL_NEW(yygotominor.yy171, NODE(yymsp[-4].minor.yy171)->u.attr.obj, NODE(yymsp[-4].minor.yy171)->u.attr.name, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
    }
    else {
        FUNC_CALL_NEW(yygotominor.yy171, yymsp[-4].minor.yy171, yymsp[-2].minor.yy171, yymsp[0].minor.yy171);
    }
}
#line 2179 "parser.c"
        break;
      case 97: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 831 "parser.y"
{
    SUBSCRIPT_NEW(yygotominor.yy171, yymsp[-3].minor.yy171, yymsp[-1].minor.yy171);
}
#line 2186 "parser.c"
        break;
      case 98: /* postfix_expr ::= postfix_expr DOT NAME */
#line 834 "parser.y"
{
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ATTR_NEW(yygotominor.yy171, yymsp[-2].minor.yy171, id);
}
#line 2194 "parser.c"
        break;
      case 99: /* atom ::= NAME */
#line 839 "parser.y"
{
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    VARIABLE_NEW(yygotominor.yy171, id);
}
#line 2202 "parser.c"
        break;
      case 100: /* atom ::= NUMBER */
      case 101: /* atom ::= REGEXP */
      case 102: /* atom ::= STRING */
#line 843 "parser.y"
{
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    PUSH_LOCAL(env, val);
    LITERAL_NEW(yygotominor.yy171, val);
    POP_LOCALS(env);
}
#line 2214 "parser.c"
        break;
      case 103: /* atom ::= TRUE */
#line 861 "parser.y"
{
    LITERAL_NEW(yygotominor.yy171, YTRUE);
}
#line 2221 "parser.c"
        break;
      case 104: /* atom ::= FALSE */
#line 864 "parser.y"
{
    LITERAL_NEW(yygotominor.yy171, YFALSE);
}
#line 2228 "parser.c"
        break;
      case 105: /* atom ::= LINE */
#line 867 "parser.y"
{
    unsigned int lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    LITERAL_NEW(yygotominor.yy171, val);
}
#line 2237 "parser.c"
        break;
      case 109: /* blockarg_opt ::= DO blockarg_params_opt stmts END */
      case 110: /* blockarg_opt ::= LBRACE blockarg_params_opt stmts RBRACE */
#line 883 "parser.y"
{
    BLOCK_ARG_NEW(yygotominor.yy171, yymsp[-2].minor.yy171, yymsp[-1].minor.yy171);
}
#line 2245 "parser.c"
        break;
      case 112: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 893 "parser.y"
{
    yygotominor.yy171 = yymsp[-1].minor.yy171;
}
#line 2252 "parser.c"
        break;
      case 114: /* excepts ::= excepts except */
#line 900 "parser.y"
{
    OBJ_ARRAY_PUSH(yygotominor.yy171, yymsp[-1].minor.yy171, yymsp[0].minor.yy171);
}
#line 2259 "parser.c"
        break;
      case 115: /* except ::= EXCEPT expr AS NAME newline stmts */
#line 904 "parser.y"
{
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    EXCEPT_BODY_NEW(yygotominor.yy171, yymsp[-4].minor.yy171, id, yymsp[0].minor.yy171);
}
#line 2268 "parser.c"
        break;
      case 116: /* except ::= EXCEPT expr newline stmts */
#line 909 "parser.y"
{
    EXCEPT_BODY_NEW(yygotominor.yy171, yymsp[-2].minor.yy171, NO_EXC_VAR, yymsp[0].minor.yy171);
}
#line 2275 "parser.c"
        break;
      case 117: /* except ::= EXCEPT newline stmts */
#line 912 "parser.y"
{
    EXCEPT_BODY_NEW(yygotominor.yy171, YNIL, NO_EXC_VAR, yymsp[0].minor.yy171);
}
#line 2282 "parser.c"
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  PTR_AS(yyParser, parser)->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
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
      yy_shift(parser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(parser);
  }

  RETURN_VOID(env);
}

/*
** The following code executes when the parse fails
*/
static void yy_parse_failed(
    YogVal parser
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while (0 <= PTR_AS(yyParser, parser)->yyidx) yy_pop_parser_stack(parser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  YogVal parser, 
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  YogVal parser
){
  ParseARG_FETCH;
#ifndef NDEBUG
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
**
** Outputs:
** None.
*/
static void Parse(
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
  locals.vals[4] = NULL;
  PUSH_LOCAL_TABLE(env, locals);

  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif

  /* (re)initialize the parser, if necessary */
  if (PTR_AS(yyParser, parser)->yyidx < 0) {
#if YYSTACKDEPTH<=0
    if (PTR_AS(yyParser, parser)->yystksz <= 0) {
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(parser, &yyminorunion);
      RETURN_VOID(env);
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

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(parser,(YYCODETYPE)yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(parser,yyact,yymajor,&yyminorunion);
      PTR_AS(yyParser, parser)->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(env, parser, yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if (PTR_AS(yyParser, parser)->yyerrcnt < 0) {
        yy_syntax_error(parser, yymajor, yyminorunion);
      }
      int yyidx = PTR_AS(yyParser, parser)->yyidx;
      yymx = PTR_AS(yyParser, parser)->yystack[yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(parser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          (0 <= PTR_AS(yyParser, parser)->yyidx) &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        PTR_AS(yyParser, parser)->yystack[PTR_AS(yyParser, parser)->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(parser);
        }
        if ((PTR_AS(yyParser, parser)->yyidx < 0) || (yymajor == 0)) {
          yy_destructor(parser, (YYCODETYPE)yymajor, &yyminorunion);
          yy_parse_failed(parser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(parser, yyact, YYERRORSYMBOL, &u2);
        }
      }
      PTR_AS(yyParser, parser)->yyerrcnt = 3;
      yyerrorhit = 1;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if (PTR_AS(yyParser, parser)->yyerrcnt <= 0) {
        yy_syntax_error(parser, yymajor, yyminorunion);
      }
      PTR_AS(yyParser, parser)->yyerrcnt = 3;
      yy_destructor(parser, (YYCODETYPE)yymajor, &yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(parser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && PTR_AS(yyParser, parser)->yyidx>=0 );

  RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
