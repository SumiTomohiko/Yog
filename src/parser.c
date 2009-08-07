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
    case NODE_IMPORT:
        KEEP(import.names);
        break;
    case NODE_KLASS:
        KEEP(klass.super);
        KEEP(klass.stmts);
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
make_array_with(YogEnv* env, YogVal elem) 
{
    SAVE_ARG(env, elem);

    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

    if (IS_PTR(elem) || IS_SYMBOL(elem)) {
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

    if (IS_PTR(elem) || IS_SYMBOL(elem)) {
        if (!IS_PTR(array)) {
            array = YogArray_new(env);
        }
        YogArray_push(env, array, elem);
    }

    RETURN(env, array);
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
FuncDef_new(YogEnv* env, uint_t lineno, ID name, YogVal params, YogVal stmts) 
{
    SAVE_ARGS2(env, params, stmts);

    YogVal node = YogNode_new(env, NODE_FUNC_DEF, lineno);
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
FuncCall_new2(YogEnv* env, uint_t lineno, YogVal recv, ID name, YogVal arg) 
{
    SAVE_ARGS2(env, recv, arg);
    YogVal postfix = YUNDEF;
    YogVal args = YUNDEF;
    PUSH_LOCALS2(env, postfix, args);

    postfix = Attr_new(env, lineno, recv, name);

    args = YogArray_new(env);
    YogArray_push(env, args, arg);

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
Klass_new(YogEnv* env, uint_t lineno, ID name, YogVal super, YogVal stmts)
{
    SAVE_ARGS2(env, super, stmts);

    YogVal node = YogNode_new(env, NODE_KLASS, lineno);
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
    while (YogLexer_next_token(env, lexer, &token)) {
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

#define TOKEN(token)            PTR_AS(YogToken, (token))
#define TOKEN_ID(token)         TOKEN((token))->u.id
#define TOKEN_LINENO(token)     TOKEN((token))->lineno
#define NODE_LINENO(node)       PTR_AS(YogNode, (node))->lineno
#line 634 "parser.c"
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
#define YYNOCODE 105
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy31;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 244
#define YYNRULE 146
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
 /*     0 */     1,  133,  134,   86,   20,   21,   24,   25,   26,  118,
 /*    10 */   185,   71,   58,  103,  207,   67,   61,  126,   23,  183,
 /*    20 */   171,  184,   28,   51,   44,   83,  391,   87,  160,  158,
 /*    30 */   159,  132,  211,   45,   46,  133,  134,  136,   47,   18,
 /*    40 */   151,  186,  187,  188,  189,  190,  191,  192,  193,  164,
 /*    50 */    85,  105,  106,  174,  166,   62,  115,  107,  108,   66,
 /*    60 */   109,  201,   67,   61,  243,   86,  183,  171,  184,   50,
 /*    70 */   160,  158,  159,  102,   66,  109,   16,   67,   61,  162,
 /*    80 */   241,  183,  171,  184,  130,  137,  139,  223,  138,  221,
 /*    90 */   224,  164,   85,  105,  106,  174,  166,   62,  202,  107,
 /*   100 */   108,   66,  109,  201,   67,   61,  142,  226,  183,  171,
 /*   110 */   184,   63,  160,  158,  159,  128,  129,  140,  144,  146,
 /*   120 */   232,  145,  230,  224,  147,  129,  140,  144,  146,  232,
 /*   130 */   122,  125,  224,  164,   85,  105,  106,  174,  166,   62,
 /*   140 */    95,  107,  108,   66,  109,   16,   67,   61,   86,    3,
 /*   150 */   183,  171,  184,   96,  160,  158,  159,   64,  109,   78,
 /*   160 */    67,   61,  239,   16,  183,  171,  184,   15,   34,  133,
 /*   170 */   134,  136,   16,  238,   35,  164,   85,  105,  106,  174,
 /*   180 */   166,   62,   38,  107,  108,   66,  109,   86,   67,   61,
 /*   190 */    86,   42,  183,  171,  184,   88,  160,  158,  159,   65,
 /*   200 */    61,   75,  196,  183,  171,  184,  167,  171,  184,  215,
 /*   210 */   216,  133,  134,  136,  175,  176,   19,  164,   85,  105,
 /*   220 */   106,  174,  166,   62,  235,  107,  108,   66,  109,   86,
 /*   230 */    67,   61,   86,   40,  183,  171,  184,   89,  160,  158,
 /*   240 */   159,   16,   60,    8,  113,  183,  171,  184,  168,  171,
 /*   250 */   184,  131,  135,  214,  177,  178,  218,  205,   27,  164,
 /*   260 */    85,  105,  106,  174,  166,   62,   29,  107,  108,   66,
 /*   270 */   109,   86,   67,   61,   86,  119,  183,  171,  184,   52,
 /*   280 */   160,  158,  159,  120,    2,  219,    3,  169,  171,  184,
 /*   290 */   170,  171,  184,  141,  143,  228,  244,   16,  218,   69,
 /*   300 */    84,  164,   85,  105,  106,  174,  166,   62,  209,  107,
 /*   310 */   108,   66,  109,  179,   67,   61,  133,  213,  183,  171,
 /*   320 */   184,   53,  160,  158,  159,   13,  123,  180,  181,  182,
 /*   330 */   150,  220,  222,  148,   17,   19,  225,   16,  161,   31,
 /*   340 */   197,  227,  229,  164,   85,  105,  106,  174,  166,   62,
 /*   350 */   231,  107,  108,   66,  109,   30,   67,   61,   16,    4,
 /*   360 */   183,  171,  184,  117,  160,  158,  159,   16,   16,   32,
 /*   370 */   203,  208,  150,   33,   16,   36,   17,  242,   35,  152,
 /*   380 */    22,   37,   41,  194,  195,  164,   85,  105,  106,  174,
 /*   390 */   166,   62,   68,  107,  108,   66,  109,   30,   67,   61,
 /*   400 */     5,    6,  183,  171,  184,   90,  160,  158,  159,  200,
 /*   410 */     7,    9,  121,   70,  204,   39,  234,   10,   72,  206,
 /*   420 */   124,   43,   11,  127,  210,  212,   59,  164,   85,  105,
 /*   430 */   106,  174,  166,   62,   48,  107,  108,   66,  109,   54,
 /*   440 */    67,   61,   55,   73,  183,  171,  184,   91,  160,  158,
 /*   450 */   159,   74,   76,   77,   49,   56,   79,   80,   57,   81,
 /*   460 */    82,  237,  236,  240,  153,   12,  392,  392,  392,  164,
 /*   470 */    85,  105,  106,  174,  166,   62,  392,  107,  108,   66,
 /*   480 */   109,  392,   67,   61,  392,  392,  183,  171,  184,   92,
 /*   490 */   160,  158,  159,  392,  392,  392,  392,  392,  392,  392,
 /*   500 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*   510 */   392,  164,   85,  105,  106,  174,  166,   62,  392,  107,
 /*   520 */   108,   66,  109,  392,   67,   61,  392,  392,  183,  171,
 /*   530 */   184,  154,  160,  158,  159,  392,  392,  392,  392,  392,
 /*   540 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*   550 */   392,  392,  392,  164,   85,  105,  106,  174,  166,   62,
 /*   560 */   392,  107,  108,   66,  109,  392,   67,   61,  392,  392,
 /*   570 */   183,  171,  184,  155,  160,  158,  159,  392,  392,  392,
 /*   580 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*   590 */   392,  392,  392,  392,  392,  164,   85,  105,  106,  174,
 /*   600 */   166,   62,  392,  107,  108,   66,  109,  392,   67,   61,
 /*   610 */   392,  392,  183,  171,  184,  156,  160,  158,  159,  392,
 /*   620 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*   630 */   392,  392,  392,  392,  392,  392,  392,  164,   85,  105,
 /*   640 */   106,  174,  166,   62,  392,  107,  108,   66,  109,  392,
 /*   650 */    67,   61,  392,  392,  183,  171,  184,   94,  160,  158,
 /*   660 */   159,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*   670 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  164,
 /*   680 */    85,  105,  106,  174,  166,   62,  392,  107,  108,   66,
 /*   690 */   109,  392,   67,   61,  392,   86,  183,  171,  184,  157,
 /*   700 */   158,  159,  101,  108,   66,  109,  392,   67,   61,  392,
 /*   710 */   392,  183,  171,  184,  392,  392,  392,  392,  392,  392,
 /*   720 */   164,   85,  105,  106,  174,  166,   62,  392,  107,  108,
 /*   730 */    66,  109,  392,   67,   61,  392,  392,  183,  171,  184,
 /*   740 */    86,  172,   99,  174,  166,   62,  392,  107,  108,   66,
 /*   750 */   109,  392,   67,   61,  392,  392,  183,  171,  184,  104,
 /*   760 */   164,   85,  105,  106,  174,  166,   62,  392,  107,  108,
 /*   770 */    66,  109,  392,   67,   61,  392,  392,  183,  171,  184,
 /*   780 */   112,   86,  172,  392,  165,  166,   62,  392,  107,  108,
 /*   790 */    66,  109,  392,   67,   61,  392,  392,  183,  171,  184,
 /*   800 */   104,  164,   85,  105,  106,  174,  166,   62,  392,  107,
 /*   810 */   108,   66,  109,  392,   67,   61,  392,  392,  183,  171,
 /*   820 */   184,  110,   93,  392,  392,  392,  392,  392,  392,  392,
 /*   830 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*   840 */   392,  164,   85,  105,  106,  174,  166,   62,  392,  107,
 /*   850 */   108,   66,  109,  392,   67,   61,   86,  392,  183,  171,
 /*   860 */   184,  100,   97,  107,  108,   66,  109,  392,   67,   61,
 /*   870 */   392,  392,  183,  171,  184,  392,  392,  392,  392,  392,
 /*   880 */   392,  164,   85,  105,  106,  174,  166,   62,  392,  107,
 /*   890 */   108,   66,  109,  392,   67,   61,  392,  392,  183,  171,
 /*   900 */   184,  392,  392,  163,  392,  392,  392,  392,  392,  392,
 /*   910 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*   920 */   392,  392,  164,   85,  105,  106,  174,  166,   62,  392,
 /*   930 */   107,  108,   66,  109,  392,   67,   61,  392,  392,  183,
 /*   940 */   171,  184,  392,  173,  392,  392,  392,  392,  392,  392,
 /*   950 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*   960 */   392,  392,  164,   85,  105,  106,  174,  166,   62,  392,
 /*   970 */   107,  108,   66,  109,  392,   67,   61,  392,  392,  183,
 /*   980 */   171,  184,  392,  392,  111,  392,  392,  392,  392,  392,
 /*   990 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*  1000 */   392,  392,  392,  164,   85,  105,  106,  174,  166,   62,
 /*  1010 */   392,  107,  108,   66,  109,  392,   67,   61,  392,  392,
 /*  1020 */   183,  171,  184,  392,  198,  392,  392,  392,  392,  392,
 /*  1030 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*  1040 */   392,  392,  392,  164,   85,  105,  106,  174,  166,   62,
 /*  1050 */   392,  107,  108,   66,  109,  392,   67,   61,  392,  392,
 /*  1060 */   183,  171,  184,  392,  392,  199,  392,  392,  392,  392,
 /*  1070 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*  1080 */   392,  392,  392,  392,  164,   85,  105,  106,  174,  166,
 /*  1090 */    62,  392,  107,  108,   66,  109,  392,   67,   61,  392,
 /*  1100 */   392,  183,  171,  184,  392,  114,  392,  392,  392,  392,
 /*  1110 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*  1120 */   392,  392,  392,  392,  164,   85,  105,  106,  174,  166,
 /*  1130 */    62,  392,  107,  108,   66,  109,  392,   67,   61,  392,
 /*  1140 */   392,  183,  171,  184,  392,  392,  116,  392,  392,  392,
 /*  1150 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*  1160 */   392,  392,  392,  392,  392,  164,   85,  105,  106,  174,
 /*  1170 */   166,   62,  392,  107,  108,   66,  109,  392,   67,   61,
 /*  1180 */   392,  392,  183,  171,  184,  392,  217,  392,  392,  392,
 /*  1190 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*  1200 */   392,  392,  392,  392,  392,  164,   85,  105,  106,  174,
 /*  1210 */   166,   62,  392,  107,  108,   66,  109,  392,   67,   61,
 /*  1220 */   392,  392,  183,  171,  184,  392,  392,  233,  392,  392,
 /*  1230 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*  1240 */   392,  392,  392,  392,  392,  392,  164,   85,  105,  106,
 /*  1250 */   174,  166,   62,  392,  107,  108,   66,  109,  392,   67,
 /*  1260 */    61,  392,  392,  183,  171,  184,  392,  149,  392,  392,
 /*  1270 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*  1280 */   392,  392,  392,  392,  392,  392,  164,   85,  105,  106,
 /*  1290 */   174,  166,   62,  392,  107,  108,   66,  109,   14,   67,
 /*  1300 */    61,  392,  392,  183,  171,  184,  392,  392,  392,  185,
 /*  1310 */   392,  392,  392,  392,  392,  392,  392,   23,  392,  392,
 /*  1320 */   392,  392,  392,  392,  392,  392,  392,  392,  392,  392,
 /*  1330 */   392,  392,   45,   46,  392,  392,  392,   47,   18,  392,
 /*  1340 */   186,  187,  188,  189,  190,  191,  192,  193,  392,   86,
 /*  1350 */    98,  106,  174,  166,   62,  185,  107,  108,   66,  109,
 /*  1360 */   392,   67,   61,   23,  392,  183,  171,  184,  392,  392,
 /*  1370 */   392,  392,  392,  392,  392,  392,  392,  392,   45,   46,
 /*  1380 */   392,  392,  392,   47,   18,  392,  186,  187,  188,  189,
 /*  1390 */   190,  191,  192,  193,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   81,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   91,   12,   93,   94,   19,   20,   97,
 /*    20 */    98,   99,   25,   62,   96,   12,   57,   58,   59,   60,
 /*    30 */    61,   74,   75,   35,   36,   22,   23,   24,   40,   41,
 /*    40 */    63,   43,   44,   45,   46,   47,   48,   49,   50,   80,
 /*    50 */    81,   82,   83,   84,   85,   86,   64,   88,   89,   90,
 /*    60 */    91,   69,   93,   94,  103,   81,   97,   98,   99,   58,
 /*    70 */    59,   60,   61,   89,   90,   91,    1,   93,   94,    4,
 /*    80 */   103,   97,   98,   99,   72,   73,   74,   75,   74,   75,
 /*    90 */    78,   80,   81,   82,   83,   84,   85,   86,   64,   88,
 /*   100 */    89,   90,   91,   69,   93,   94,   74,   75,   97,   98,
 /*   110 */    99,   58,   59,   60,   61,   70,   71,   72,   73,   74,
 /*   120 */    75,   74,   75,   78,   70,   71,   72,   73,   74,   75,
 /*   130 */    67,   68,   78,   80,   81,   82,   83,   84,   85,   86,
 /*   140 */    63,   88,   89,   90,   91,    1,   93,   94,   81,    5,
 /*   150 */    97,   98,   99,   58,   59,   60,   61,   90,   91,   12,
 /*   160 */    93,   94,   17,    1,   97,   98,   99,    5,   87,   22,
 /*   170 */    23,   24,    1,   28,   29,   80,   81,   82,   83,   84,
 /*   180 */    85,   86,   92,   88,   89,   90,   91,   81,   93,   94,
 /*   190 */    81,   95,   97,   98,   99,   58,   59,   60,   61,   93,
 /*   200 */    94,   12,  101,   97,   98,   99,   97,   98,   99,   76,
 /*   210 */    77,   22,   23,   24,   32,   33,   54,   80,   81,   82,
 /*   220 */    83,   84,   85,   86,   53,   88,   89,   90,   91,   81,
 /*   230 */    93,   94,   81,   41,   97,   98,   99,   58,   59,   60,
 /*   240 */    61,    1,   94,    3,  102,   97,   98,   99,   97,   98,
 /*   250 */    99,   73,   74,   75,   35,   36,   78,   12,   18,   80,
 /*   260 */    81,   82,   83,   84,   85,   86,   17,   88,   89,   90,
 /*   270 */    91,   81,   93,   94,   81,   65,   97,   98,   99,   58,
 /*   280 */    59,   60,   61,   66,    3,   77,    5,   97,   98,   99,
 /*   290 */    97,   98,   99,   73,   74,   75,    0,    1,   78,   51,
 /*   300 */    52,   80,   81,   82,   83,   84,   85,   86,   75,   88,
 /*   310 */    89,   90,   91,   23,   93,   94,   22,   75,   97,   98,
 /*   320 */    99,   58,   59,   60,   61,    1,   68,   37,   38,   39,
 /*   330 */    16,   75,   75,  102,   20,   54,   75,    1,    4,   25,
 /*   340 */     4,   75,   75,   80,   81,   82,   83,   84,   85,   86,
 /*   350 */    75,   88,   89,   90,   91,   41,   93,   94,    1,    1,
 /*   360 */    97,   98,   99,   58,   59,   60,   61,    1,    1,   26,
 /*   370 */     4,    4,   16,   27,    1,   30,   20,    4,   29,   55,
 /*   380 */    15,   31,   34,   42,   21,   80,   81,   82,   83,   84,
 /*   390 */    85,   86,   21,   88,   89,   90,   91,   41,   93,   94,
 /*   400 */     1,    1,   97,   98,   99,   58,   59,   60,   61,    4,
 /*   410 */     1,    1,   15,   12,   12,   20,   42,   21,   15,   12,
 /*   420 */    16,   15,    1,   12,   12,   12,   12,   80,   81,   82,
 /*   430 */    83,   84,   85,   86,   15,   88,   89,   90,   91,   15,
 /*   440 */    93,   94,   15,   15,   97,   98,   99,   58,   59,   60,
 /*   450 */    61,   15,   15,   15,   15,   15,   15,   15,   15,   15,
 /*   460 */    15,   12,   42,    4,   12,    1,  104,  104,  104,   80,
 /*   470 */    81,   82,   83,   84,   85,   86,  104,   88,   89,   90,
 /*   480 */    91,  104,   93,   94,  104,  104,   97,   98,   99,   58,
 /*   490 */    59,   60,   61,  104,  104,  104,  104,  104,  104,  104,
 /*   500 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*   510 */   104,   80,   81,   82,   83,   84,   85,   86,  104,   88,
 /*   520 */    89,   90,   91,  104,   93,   94,  104,  104,   97,   98,
 /*   530 */    99,   58,   59,   60,   61,  104,  104,  104,  104,  104,
 /*   540 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*   550 */   104,  104,  104,   80,   81,   82,   83,   84,   85,   86,
 /*   560 */   104,   88,   89,   90,   91,  104,   93,   94,  104,  104,
 /*   570 */    97,   98,   99,   58,   59,   60,   61,  104,  104,  104,
 /*   580 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*   590 */   104,  104,  104,  104,  104,   80,   81,   82,   83,   84,
 /*   600 */    85,   86,  104,   88,   89,   90,   91,  104,   93,   94,
 /*   610 */   104,  104,   97,   98,   99,   58,   59,   60,   61,  104,
 /*   620 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*   630 */   104,  104,  104,  104,  104,  104,  104,   80,   81,   82,
 /*   640 */    83,   84,   85,   86,  104,   88,   89,   90,   91,  104,
 /*   650 */    93,   94,  104,  104,   97,   98,   99,   58,   59,   60,
 /*   660 */    61,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*   670 */   104,  104,  104,  104,  104,  104,  104,  104,  104,   80,
 /*   680 */    81,   82,   83,   84,   85,   86,  104,   88,   89,   90,
 /*   690 */    91,  104,   93,   94,  104,   81,   97,   98,   99,   59,
 /*   700 */    60,   61,   88,   89,   90,   91,  104,   93,   94,  104,
 /*   710 */   104,   97,   98,   99,  104,  104,  104,  104,  104,  104,
 /*   720 */    80,   81,   82,   83,   84,   85,   86,  104,   88,   89,
 /*   730 */    90,   91,  104,   93,   94,  104,  104,   97,   98,   99,
 /*   740 */    81,   61,   83,   84,   85,   86,  104,   88,   89,   90,
 /*   750 */    91,  104,   93,   94,  104,  104,   97,   98,   99,   79,
 /*   760 */    80,   81,   82,   83,   84,   85,   86,  104,   88,   89,
 /*   770 */    90,   91,  104,   93,   94,  104,  104,   97,   98,   99,
 /*   780 */   100,   81,   61,  104,   84,   85,   86,  104,   88,   89,
 /*   790 */    90,   91,  104,   93,   94,  104,  104,   97,   98,   99,
 /*   800 */    79,   80,   81,   82,   83,   84,   85,   86,  104,   88,
 /*   810 */    89,   90,   91,  104,   93,   94,  104,  104,   97,   98,
 /*   820 */    99,  100,   61,  104,  104,  104,  104,  104,  104,  104,
 /*   830 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*   840 */   104,   80,   81,   82,   83,   84,   85,   86,  104,   88,
 /*   850 */    89,   90,   91,  104,   93,   94,   81,  104,   97,   98,
 /*   860 */    99,   86,   61,   88,   89,   90,   91,  104,   93,   94,
 /*   870 */   104,  104,   97,   98,   99,  104,  104,  104,  104,  104,
 /*   880 */   104,   80,   81,   82,   83,   84,   85,   86,  104,   88,
 /*   890 */    89,   90,   91,  104,   93,   94,  104,  104,   97,   98,
 /*   900 */    99,  104,  104,   61,  104,  104,  104,  104,  104,  104,
 /*   910 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*   920 */   104,  104,   80,   81,   82,   83,   84,   85,   86,  104,
 /*   930 */    88,   89,   90,   91,  104,   93,   94,  104,  104,   97,
 /*   940 */    98,   99,  104,   61,  104,  104,  104,  104,  104,  104,
 /*   950 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*   960 */   104,  104,   80,   81,   82,   83,   84,   85,   86,  104,
 /*   970 */    88,   89,   90,   91,  104,   93,   94,  104,  104,   97,
 /*   980 */    98,   99,  104,  104,   61,  104,  104,  104,  104,  104,
 /*   990 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*  1000 */   104,  104,  104,   80,   81,   82,   83,   84,   85,   86,
 /*  1010 */   104,   88,   89,   90,   91,  104,   93,   94,  104,  104,
 /*  1020 */    97,   98,   99,  104,   61,  104,  104,  104,  104,  104,
 /*  1030 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*  1040 */   104,  104,  104,   80,   81,   82,   83,   84,   85,   86,
 /*  1050 */   104,   88,   89,   90,   91,  104,   93,   94,  104,  104,
 /*  1060 */    97,   98,   99,  104,  104,   61,  104,  104,  104,  104,
 /*  1070 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*  1080 */   104,  104,  104,  104,   80,   81,   82,   83,   84,   85,
 /*  1090 */    86,  104,   88,   89,   90,   91,  104,   93,   94,  104,
 /*  1100 */   104,   97,   98,   99,  104,   61,  104,  104,  104,  104,
 /*  1110 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*  1120 */   104,  104,  104,  104,   80,   81,   82,   83,   84,   85,
 /*  1130 */    86,  104,   88,   89,   90,   91,  104,   93,   94,  104,
 /*  1140 */   104,   97,   98,   99,  104,  104,   61,  104,  104,  104,
 /*  1150 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*  1160 */   104,  104,  104,  104,  104,   80,   81,   82,   83,   84,
 /*  1170 */    85,   86,  104,   88,   89,   90,   91,  104,   93,   94,
 /*  1180 */   104,  104,   97,   98,   99,  104,   61,  104,  104,  104,
 /*  1190 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*  1200 */   104,  104,  104,  104,  104,   80,   81,   82,   83,   84,
 /*  1210 */    85,   86,  104,   88,   89,   90,   91,  104,   93,   94,
 /*  1220 */   104,  104,   97,   98,   99,  104,  104,   61,  104,  104,
 /*  1230 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*  1240 */   104,  104,  104,  104,  104,  104,   80,   81,   82,   83,
 /*  1250 */    84,   85,   86,  104,   88,   89,   90,   91,  104,   93,
 /*  1260 */    94,  104,  104,   97,   98,   99,  104,   61,  104,  104,
 /*  1270 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*  1280 */   104,  104,  104,  104,  104,  104,   80,   81,   82,   83,
 /*  1290 */    84,   85,   86,  104,   88,   89,   90,   91,    1,   93,
 /*  1300 */    94,  104,  104,   97,   98,   99,  104,  104,  104,   12,
 /*  1310 */   104,  104,  104,  104,  104,  104,  104,   20,  104,  104,
 /*  1320 */   104,  104,  104,  104,  104,  104,  104,  104,  104,  104,
 /*  1330 */   104,  104,   35,   36,  104,  104,  104,   40,   41,  104,
 /*  1340 */    43,   44,   45,   46,   47,   48,   49,   50,  104,   81,
 /*  1350 */    82,   83,   84,   85,   86,   12,   88,   89,   90,   91,
 /*  1360 */   104,   93,   94,   20,  104,   97,   98,   99,  104,  104,
 /*  1370 */   104,  104,  104,  104,  104,  104,  104,  104,   35,   36,
 /*  1380 */   104,  104,  104,   40,   41,  104,   43,   44,   45,   46,
 /*  1390 */    47,   48,   49,   50,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 156
static const short yy_shift_ofst[] = {
 /*     0 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2, 1343, 1343, 1297,
 /*    20 */  1343, 1343, 1343, 1343, 1343, 1343, 1343, 1343, 1343, 1343,
 /*    30 */  1343, 1343, 1343, 1343, 1343, 1343, 1343, 1343, 1343,   13,
 /*    40 */    13, 1343, 1343,  147, 1343, 1343, 1343, 1343,  189,  189,
 /*    50 */   162,  281,  240,  240,  -21,  -21,  -21,  -21,    2,   -3,
 /*    60 */   290,  290,  145,  144,  182,  219,  182,  219,  248,  192,
 /*    70 */   249,  245,    2,  294,  294,   -3,  294,  294,   -3,  294,
 /*    80 */   294,  294,  294,   -3,  192,  314,  356,  296,   75,  336,
 /*    90 */   366,  367,  171,  324,  373,  334,  357,  358,  343,  346,
 /*   100 */   349,  345,  350,  348,  365,  343,  346,  345,  350,  348,
 /*   110 */   341,  363,  371,  399,  400,  405,  409,  357,  401,  410,
 /*   120 */   397,  402,  403,  404,  407,  404,  411,  395,  396,  406,
 /*   130 */   419,  424,  428,  412,  413,  436,  414,  427,  437,  438,
 /*   140 */   439,  440,  441,  442,  443,  444,  445,  374,  421,  420,
 /*   150 */   449,  459,  452,  464,  357,  357,  357,
};
#define YY_REDUCE_USE_DFLT (-79)
#define YY_REDUCE_MAX 84
static const short yy_reduce_ofst[] = {
 /*     0 */   -31,   11,   53,   95,  137,  179,  221,  263,  305,  347,
 /*    10 */   389,  431,  473,  515,  557,  599,  640,  680,  721,  761,
 /*    20 */   801,  842,  882,  923,  963, 1004, 1044, 1085, 1125, 1166,
 /*    30 */  1206, 1268,  659,  700,  775,  614,  -16,   67,  -78,   45,
 /*    40 */    54,  106,  148,   12,  109,  151,  190,  193,  178,  220,
 /*    50 */   -39,  -23,   -8,   34,  -43,   14,   32,   47,   63,  133,
 /*    60 */   -72,  -72,   81,   77,   90,   96,   90,   96,  101,  142,
 /*    70 */   210,  217,  258,  233,  242,  208,  256,  257,  208,  261,
 /*    80 */   266,  267,  275,  208,  231,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   247,  247,  247,  247,  247,  247,  247,  247,  247,  247,
 /*    10 */   247,  247,  247,  247,  247,  247,  247,  376,  376,  390,
 /*    20 */   390,  254,  390,  390,  256,  258,  390,  390,  390,  390,
 /*    30 */   390,  390,  390,  390,  390,  390,  390,  390,  390,  308,
 /*    40 */   308,  390,  390,  390,  390,  390,  390,  390,  390,  390,
 /*    50 */   390,  388,  274,  274,  390,  390,  390,  390,  390,  312,
 /*    60 */   347,  346,  330,  388,  339,  345,  338,  344,  378,  381,
 /*    70 */   270,  390,  390,  390,  390,  390,  390,  390,  316,  390,
 /*    80 */   390,  390,  390,  315,  381,  360,  360,  390,  390,  390,
 /*    90 */   390,  390,  390,  390,  390,  390,  389,  390,  323,  326,
 /*   100 */   331,  335,  337,  341,  377,  324,  325,  334,  336,  340,
 /*   110 */   390,  390,  390,  390,  390,  390,  390,  275,  390,  390,
 /*   120 */   262,  390,  263,  265,  390,  264,  390,  390,  390,  292,
 /*   130 */   284,  280,  278,  390,  390,  282,  390,  288,  286,  290,
 /*   140 */   300,  296,  294,  298,  304,  302,  306,  390,  390,  390,
 /*   150 */   390,  390,  390,  390,  385,  386,  387,  246,  248,  249,
 /*   160 */   245,  250,  253,  255,  322,  328,  329,  350,  356,  357,
 /*   170 */   358,  359,  320,  321,  327,  342,  343,  348,  349,  352,
 /*   180 */   353,  354,  355,  351,  361,  365,  366,  367,  368,  369,
 /*   190 */   370,  371,  372,  373,  374,  375,  362,  379,  257,  259,
 /*   200 */   260,  272,  273,  261,  269,  268,  267,  266,  276,  277,
 /*   210 */   309,  279,  310,  281,  283,  311,  313,  314,  318,  319,
 /*   220 */   285,  287,  289,  291,  317,  293,  295,  297,  299,  301,
 /*   230 */   303,  305,  307,  271,  382,  380,  363,  364,  332,  333,
 /*   240 */   251,  384,  252,  383,
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
  "NAME",          "NONLOCAL",      "IMPORT",        "COMMA",       
  "DOT",           "GREATER",       "ELIF",          "DEF",         
  "LPAR",          "RPAR",          "DOUBLE_STAR",   "STAR",        
  "AMPER",         "EQUAL",         "BAR_BAR",       "AND_AND",     
  "LESS",          "XOR",           "BAR",           "AND",         
  "LSHIFT",        "RSHIFT",        "EQUAL_TILDA",   "PLUS",        
  "MINUS",         "DIV",           "DIV_DIV",       "PERCENT",     
  "TILDA",         "LBRACKET",      "RBRACKET",      "NUMBER",      
  "REGEXP",        "STRING",        "SYMBOL",        "NIL",         
  "TRUE",          "FALSE",         "LINE",          "DO",          
  "LBRACE",        "RBRACE",        "EXCEPT",        "AS",          
  "error",         "module",        "stmts",         "stmt",        
  "func_def",      "expr",          "excepts",       "finally_opt", 
  "if_tail",       "super_opt",     "names",         "dotted_names",
  "dotted_name",   "else_opt",      "params",        "params_without_default",
  "params_with_default",  "block_param",   "var_param",     "kw_param",    
  "param_default_opt",  "param_default",  "param_with_default",  "args",        
  "assign_expr",   "postfix_expr",  "logical_or_expr",  "logical_and_expr",
  "not_expr",      "comparison",    "xor_expr",      "comp_op",     
  "or_expr",       "and_expr",      "shift_expr",    "match_expr",  
  "shift_op",      "arith_expr",    "term",          "arith_op",    
  "term_op",       "factor",        "power",         "atom",        
  "args_opt",      "blockarg_opt",  "blockarg_params_opt",  "except",      
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
 /*  17 */ "stmt ::= CLASS NAME super_opt NEWLINE stmts END",
 /*  18 */ "stmt ::= NONLOCAL names",
 /*  19 */ "stmt ::= IMPORT dotted_names",
 /*  20 */ "dotted_names ::= dotted_name",
 /*  21 */ "dotted_names ::= dotted_names COMMA dotted_name",
 /*  22 */ "dotted_name ::= NAME",
 /*  23 */ "dotted_name ::= dotted_name DOT NAME",
 /*  24 */ "names ::= NAME",
 /*  25 */ "names ::= names COMMA NAME",
 /*  26 */ "super_opt ::=",
 /*  27 */ "super_opt ::= GREATER expr",
 /*  28 */ "if_tail ::= else_opt",
 /*  29 */ "if_tail ::= ELIF expr NEWLINE stmts if_tail",
 /*  30 */ "else_opt ::=",
 /*  31 */ "else_opt ::= ELSE stmts",
 /*  32 */ "func_def ::= DEF NAME LPAR params RPAR stmts END",
 /*  33 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  34 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param",
 /*  35 */ "params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param",
 /*  36 */ "params ::= params_without_default COMMA params_with_default COMMA block_param",
 /*  37 */ "params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param",
 /*  38 */ "params ::= params_without_default COMMA params_with_default COMMA var_param",
 /*  39 */ "params ::= params_without_default COMMA params_with_default COMMA kw_param",
 /*  40 */ "params ::= params_without_default COMMA params_with_default",
 /*  41 */ "params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  42 */ "params ::= params_without_default COMMA block_param COMMA var_param",
 /*  43 */ "params ::= params_without_default COMMA block_param COMMA kw_param",
 /*  44 */ "params ::= params_without_default COMMA block_param",
 /*  45 */ "params ::= params_without_default COMMA var_param COMMA kw_param",
 /*  46 */ "params ::= params_without_default COMMA var_param",
 /*  47 */ "params ::= params_without_default COMMA kw_param",
 /*  48 */ "params ::= params_without_default",
 /*  49 */ "params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param",
 /*  50 */ "params ::= params_with_default COMMA block_param COMMA var_param",
 /*  51 */ "params ::= params_with_default COMMA block_param COMMA kw_param",
 /*  52 */ "params ::= params_with_default COMMA block_param",
 /*  53 */ "params ::= params_with_default COMMA var_param COMMA kw_param",
 /*  54 */ "params ::= params_with_default COMMA var_param",
 /*  55 */ "params ::= params_with_default COMMA kw_param",
 /*  56 */ "params ::= params_with_default",
 /*  57 */ "params ::= block_param COMMA var_param COMMA kw_param",
 /*  58 */ "params ::= block_param COMMA var_param",
 /*  59 */ "params ::= block_param COMMA kw_param",
 /*  60 */ "params ::= block_param",
 /*  61 */ "params ::= var_param COMMA kw_param",
 /*  62 */ "params ::= var_param",
 /*  63 */ "params ::= kw_param",
 /*  64 */ "params ::=",
 /*  65 */ "kw_param ::= DOUBLE_STAR NAME",
 /*  66 */ "var_param ::= STAR NAME",
 /*  67 */ "block_param ::= AMPER NAME param_default_opt",
 /*  68 */ "param_default_opt ::=",
 /*  69 */ "param_default_opt ::= param_default",
 /*  70 */ "param_default ::= EQUAL expr",
 /*  71 */ "params_without_default ::= NAME",
 /*  72 */ "params_without_default ::= params_without_default COMMA NAME",
 /*  73 */ "params_with_default ::= param_with_default",
 /*  74 */ "params_with_default ::= params_with_default COMMA param_with_default",
 /*  75 */ "param_with_default ::= NAME param_default",
 /*  76 */ "args ::= expr",
 /*  77 */ "args ::= args COMMA expr",
 /*  78 */ "expr ::= assign_expr",
 /*  79 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /*  80 */ "assign_expr ::= logical_or_expr",
 /*  81 */ "logical_or_expr ::= logical_and_expr",
 /*  82 */ "logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr",
 /*  83 */ "logical_and_expr ::= not_expr",
 /*  84 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /*  85 */ "not_expr ::= comparison",
 /*  86 */ "comparison ::= xor_expr",
 /*  87 */ "comparison ::= xor_expr comp_op xor_expr",
 /*  88 */ "comp_op ::= LESS",
 /*  89 */ "comp_op ::= GREATER",
 /*  90 */ "xor_expr ::= or_expr",
 /*  91 */ "xor_expr ::= xor_expr XOR or_expr",
 /*  92 */ "or_expr ::= and_expr",
 /*  93 */ "or_expr ::= or_expr BAR and_expr",
 /*  94 */ "and_expr ::= shift_expr",
 /*  95 */ "and_expr ::= and_expr AND shift_expr",
 /*  96 */ "shift_expr ::= match_expr",
 /*  97 */ "shift_expr ::= shift_expr shift_op match_expr",
 /*  98 */ "shift_op ::= LSHIFT",
 /*  99 */ "shift_op ::= RSHIFT",
 /* 100 */ "match_expr ::= arith_expr",
 /* 101 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 102 */ "arith_expr ::= term",
 /* 103 */ "arith_expr ::= arith_expr arith_op term",
 /* 104 */ "arith_op ::= PLUS",
 /* 105 */ "arith_op ::= MINUS",
 /* 106 */ "term ::= term term_op factor",
 /* 107 */ "term ::= factor",
 /* 108 */ "term_op ::= STAR",
 /* 109 */ "term_op ::= DIV",
 /* 110 */ "term_op ::= DIV_DIV",
 /* 111 */ "term_op ::= PERCENT",
 /* 112 */ "factor ::= PLUS factor",
 /* 113 */ "factor ::= MINUS factor",
 /* 114 */ "factor ::= TILDA factor",
 /* 115 */ "factor ::= power",
 /* 116 */ "power ::= postfix_expr",
 /* 117 */ "postfix_expr ::= atom",
 /* 118 */ "postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt",
 /* 119 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 120 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 121 */ "atom ::= NAME",
 /* 122 */ "atom ::= NUMBER",
 /* 123 */ "atom ::= REGEXP",
 /* 124 */ "atom ::= STRING",
 /* 125 */ "atom ::= SYMBOL",
 /* 126 */ "atom ::= NIL",
 /* 127 */ "atom ::= TRUE",
 /* 128 */ "atom ::= FALSE",
 /* 129 */ "atom ::= LINE",
 /* 130 */ "atom ::= LBRACKET args_opt RBRACKET",
 /* 131 */ "atom ::= LPAR expr RPAR",
 /* 132 */ "args_opt ::=",
 /* 133 */ "args_opt ::= args",
 /* 134 */ "blockarg_opt ::=",
 /* 135 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 136 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 137 */ "blockarg_params_opt ::=",
 /* 138 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 139 */ "excepts ::= except",
 /* 140 */ "excepts ::= excepts except",
 /* 141 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 142 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 143 */ "except ::= EXCEPT NEWLINE stmts",
 /* 144 */ "finally_opt ::=",
 /* 145 */ "finally_opt ::= FINALLY stmts",
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
  { 57, 1 },
  { 58, 1 },
  { 58, 3 },
  { 59, 0 },
  { 59, 1 },
  { 59, 1 },
  { 59, 7 },
  { 59, 5 },
  { 59, 5 },
  { 59, 5 },
  { 59, 1 },
  { 59, 2 },
  { 59, 1 },
  { 59, 2 },
  { 59, 1 },
  { 59, 2 },
  { 59, 6 },
  { 59, 6 },
  { 59, 2 },
  { 59, 2 },
  { 67, 1 },
  { 67, 3 },
  { 68, 1 },
  { 68, 3 },
  { 66, 1 },
  { 66, 3 },
  { 65, 0 },
  { 65, 2 },
  { 64, 1 },
  { 64, 5 },
  { 69, 0 },
  { 69, 2 },
  { 60, 7 },
  { 70, 9 },
  { 70, 7 },
  { 70, 7 },
  { 70, 5 },
  { 70, 7 },
  { 70, 5 },
  { 70, 5 },
  { 70, 3 },
  { 70, 7 },
  { 70, 5 },
  { 70, 5 },
  { 70, 3 },
  { 70, 5 },
  { 70, 3 },
  { 70, 3 },
  { 70, 1 },
  { 70, 7 },
  { 70, 5 },
  { 70, 5 },
  { 70, 3 },
  { 70, 5 },
  { 70, 3 },
  { 70, 3 },
  { 70, 1 },
  { 70, 5 },
  { 70, 3 },
  { 70, 3 },
  { 70, 1 },
  { 70, 3 },
  { 70, 1 },
  { 70, 1 },
  { 70, 0 },
  { 75, 2 },
  { 74, 2 },
  { 73, 3 },
  { 76, 0 },
  { 76, 1 },
  { 77, 2 },
  { 71, 1 },
  { 71, 3 },
  { 72, 1 },
  { 72, 3 },
  { 78, 2 },
  { 79, 1 },
  { 79, 3 },
  { 61, 1 },
  { 80, 3 },
  { 80, 1 },
  { 82, 1 },
  { 82, 3 },
  { 83, 1 },
  { 83, 3 },
  { 84, 1 },
  { 85, 1 },
  { 85, 3 },
  { 87, 1 },
  { 87, 1 },
  { 86, 1 },
  { 86, 3 },
  { 88, 1 },
  { 88, 3 },
  { 89, 1 },
  { 89, 3 },
  { 90, 1 },
  { 90, 3 },
  { 92, 1 },
  { 92, 1 },
  { 91, 1 },
  { 91, 3 },
  { 93, 1 },
  { 93, 3 },
  { 95, 1 },
  { 95, 1 },
  { 94, 3 },
  { 94, 1 },
  { 96, 1 },
  { 96, 1 },
  { 96, 1 },
  { 96, 1 },
  { 97, 2 },
  { 97, 2 },
  { 97, 2 },
  { 97, 1 },
  { 98, 1 },
  { 81, 1 },
  { 81, 5 },
  { 81, 4 },
  { 81, 3 },
  { 99, 1 },
  { 99, 1 },
  { 99, 1 },
  { 99, 1 },
  { 99, 1 },
  { 99, 1 },
  { 99, 1 },
  { 99, 1 },
  { 99, 1 },
  { 99, 3 },
  { 99, 3 },
  { 100, 0 },
  { 100, 1 },
  { 101, 0 },
  { 101, 5 },
  { 101, 5 },
  { 102, 0 },
  { 102, 3 },
  { 62, 1 },
  { 62, 2 },
  { 103, 6 },
  { 103, 4 },
  { 103, 3 },
  { 63, 0 },
  { 63, 2 },
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
#line 630 "parser.y"
{
    *pval = yymsp[0].minor.yy31;
}
#line 1936 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 76: /* args ::= expr */
      case 139: /* excepts ::= except */
#line 634 "parser.y"
{
    yygotominor.yy31 = make_array_with(env, yymsp[0].minor.yy31);
}
#line 1947 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
      case 77: /* args ::= args COMMA expr */
#line 637 "parser.y"
{
    yygotominor.yy31 = Array_push(env, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 1957 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 132: /* args_opt ::= */
      case 134: /* blockarg_opt ::= */
      case 137: /* blockarg_params_opt ::= */
      case 144: /* finally_opt ::= */
#line 641 "parser.y"
{
    yygotominor.yy31 = YNIL;
}
#line 1971 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 5: /* stmt ::= expr */
      case 27: /* super_opt ::= GREATER expr */
      case 28: /* if_tail ::= else_opt */
      case 31: /* else_opt ::= ELSE stmts */
      case 69: /* param_default_opt ::= param_default */
      case 70: /* param_default ::= EQUAL expr */
      case 78: /* expr ::= assign_expr */
      case 80: /* assign_expr ::= logical_or_expr */
      case 81: /* logical_or_expr ::= logical_and_expr */
      case 83: /* logical_and_expr ::= not_expr */
      case 85: /* not_expr ::= comparison */
      case 86: /* comparison ::= xor_expr */
      case 90: /* xor_expr ::= or_expr */
      case 92: /* or_expr ::= and_expr */
      case 94: /* and_expr ::= shift_expr */
      case 96: /* shift_expr ::= match_expr */
      case 100: /* match_expr ::= arith_expr */
      case 102: /* arith_expr ::= term */
      case 107: /* term ::= factor */
      case 115: /* factor ::= power */
      case 116: /* power ::= postfix_expr */
      case 117: /* postfix_expr ::= atom */
      case 133: /* args_opt ::= args */
      case 145: /* finally_opt ::= FINALLY stmts */
#line 644 "parser.y"
{
    yygotominor.yy31 = yymsp[0].minor.yy31;
}
#line 2002 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 650 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy31 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy31, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, yymsp[-1].minor.yy31);
}
#line 2010 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 654 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy31 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy31, yymsp[-2].minor.yy31, YNIL, yymsp[-1].minor.yy31);
}
#line 2018 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 658 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy31 = Finally_new(env, lineno, yymsp[-3].minor.yy31, yymsp[-1].minor.yy31);
}
#line 2026 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 662 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy31 = While_new(env, lineno, yymsp[-3].minor.yy31, yymsp[-1].minor.yy31);
}
#line 2034 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 666 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy31 = Break_new(env, lineno, YNIL);
}
#line 2042 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 670 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy31 = Break_new(env, lineno, yymsp[0].minor.yy31);
}
#line 2050 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 674 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy31 = Next_new(env, lineno, YNIL);
}
#line 2058 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 678 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy31 = Next_new(env, lineno, yymsp[0].minor.yy31);
}
#line 2066 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 682 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy31 = Return_new(env, lineno, YNIL);
}
#line 2074 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 686 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy31 = Return_new(env, lineno, yymsp[0].minor.yy31);
}
#line 2082 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 690 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy31 = If_new(env, lineno, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, yymsp[-1].minor.yy31);
}
#line 2090 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 694 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy31 = Klass_new(env, lineno, id, yymsp[-3].minor.yy31, yymsp[-1].minor.yy31);
}
#line 2099 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 699 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy31 = Nonlocal_new(env, lineno, yymsp[0].minor.yy31);
}
#line 2107 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 703 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy31 = Import_new(env, lineno, yymsp[0].minor.yy31);
}
#line 2115 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 715 "parser.y"
{
    yygotominor.yy31 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2123 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 718 "parser.y"
{
    yygotominor.yy31 = Array_push_token_id(env, yymsp[-2].minor.yy31, yymsp[0].minor.yy0);
}
#line 2131 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 739 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy31, yymsp[-1].minor.yy31, yymsp[0].minor.yy31);
    yygotominor.yy31 = make_array_with(env, node);
}
#line 2140 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 752 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy31 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy31, yymsp[-1].minor.yy31);
}
#line 2149 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 758 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-8].minor.yy31, yymsp[-6].minor.yy31, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 2156 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 761 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-6].minor.yy31, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, yymsp[0].minor.yy31, YNIL);
}
#line 2163 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 764 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-6].minor.yy31, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, YNIL, yymsp[0].minor.yy31);
}
#line 2170 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 767 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, yymsp[0].minor.yy31, YNIL, YNIL);
}
#line 2177 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 770 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-6].minor.yy31, yymsp[-4].minor.yy31, YNIL, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 2184 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 773 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, YNIL, yymsp[0].minor.yy31, YNIL);
}
#line 2191 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 776 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, YNIL, YNIL, yymsp[0].minor.yy31);
}
#line 2198 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 779 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-2].minor.yy31, yymsp[0].minor.yy31, YNIL, YNIL, YNIL);
}
#line 2205 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 782 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-6].minor.yy31, YNIL, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 2212 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 785 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-4].minor.yy31, YNIL, yymsp[-2].minor.yy31, yymsp[0].minor.yy31, YNIL);
}
#line 2219 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 788 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-4].minor.yy31, YNIL, yymsp[-2].minor.yy31, YNIL, yymsp[0].minor.yy31);
}
#line 2226 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 791 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-2].minor.yy31, YNIL, yymsp[0].minor.yy31, YNIL, YNIL);
}
#line 2233 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 794 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-4].minor.yy31, YNIL, YNIL, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 2240 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 797 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-2].minor.yy31, YNIL, YNIL, yymsp[0].minor.yy31, YNIL);
}
#line 2247 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 800 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[-2].minor.yy31, YNIL, YNIL, YNIL, yymsp[0].minor.yy31);
}
#line 2254 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 803 "parser.y"
{
    yygotominor.yy31 = Params_new(env, yymsp[0].minor.yy31, YNIL, YNIL, YNIL, YNIL);
}
#line 2261 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 806 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, yymsp[-6].minor.yy31, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 2268 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 809 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, yymsp[0].minor.yy31, YNIL);
}
#line 2275 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 812 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, YNIL, yymsp[0].minor.yy31);
}
#line 2282 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 815 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, yymsp[-2].minor.yy31, yymsp[0].minor.yy31, YNIL, YNIL);
}
#line 2289 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 818 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, yymsp[-4].minor.yy31, YNIL, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 2296 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 821 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, yymsp[-2].minor.yy31, YNIL, yymsp[0].minor.yy31, YNIL);
}
#line 2303 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 824 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, yymsp[-2].minor.yy31, YNIL, YNIL, yymsp[0].minor.yy31);
}
#line 2310 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 827 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, yymsp[0].minor.yy31, YNIL, YNIL, YNIL);
}
#line 2317 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 830 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 2324 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 833 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy31, yymsp[0].minor.yy31, YNIL);
}
#line 2331 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 836 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy31, YNIL, yymsp[0].minor.yy31);
}
#line 2338 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 839 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy31, YNIL, YNIL);
}
#line 2345 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 842 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 2352 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 845 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy31, YNIL);
}
#line 2359 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 848 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy31);
}
#line 2366 "parser.c"
        break;
      case 64: /* params ::= */
#line 851 "parser.y"
{
    yygotominor.yy31 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2373 "parser.c"
        break;
      case 65: /* kw_param ::= DOUBLE_STAR NAME */
#line 855 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy31 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2382 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 861 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy31 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2391 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 867 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy31 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy31);
}
#line 2400 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 884 "parser.y"
{
    yygotominor.yy31 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy31, lineno, id, YNIL);
}
#line 2410 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 890 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy31, lineno, id, YNIL);
    yygotominor.yy31 = yymsp[-2].minor.yy31;
}
#line 2420 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 904 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy31 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy31);
}
#line 2429 "parser.c"
        break;
      case 79: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 921 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy31);
    yygotominor.yy31 = Assign_new(env, lineno, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 2437 "parser.c"
        break;
      case 82: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 932 "parser.y"
{
    yygotominor.yy31 = YogNode_new(env, NODE_LOGICAL_OR, NODE_LINENO(yymsp[-2].minor.yy31));
    NODE(yygotominor.yy31)->u.logical_or.left = yymsp[-2].minor.yy31;
    NODE(yygotominor.yy31)->u.logical_or.right = yymsp[0].minor.yy31;
}
#line 2446 "parser.c"
        break;
      case 84: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 941 "parser.y"
{
    yygotominor.yy31 = YogNode_new(env, NODE_LOGICAL_AND, NODE_LINENO(yymsp[-2].minor.yy31));
    NODE(yygotominor.yy31)->u.logical_and.left = yymsp[-2].minor.yy31;
    NODE(yygotominor.yy31)->u.logical_and.right = yymsp[0].minor.yy31;
}
#line 2455 "parser.c"
        break;
      case 87: /* comparison ::= xor_expr comp_op xor_expr */
#line 954 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy31);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy31)->u.id;
    yygotominor.yy31 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy31, id, yymsp[0].minor.yy31);
}
#line 2464 "parser.c"
        break;
      case 88: /* comp_op ::= LESS */
      case 89: /* comp_op ::= GREATER */
#line 960 "parser.y"
{
    yygotominor.yy31 = yymsp[0].minor.yy0;
}
#line 2472 "parser.c"
        break;
      case 91: /* xor_expr ::= xor_expr XOR or_expr */
      case 93: /* or_expr ::= or_expr BAR and_expr */
      case 95: /* and_expr ::= and_expr AND shift_expr */
#line 970 "parser.y"
{
    yygotominor.yy31 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy31), yymsp[-2].minor.yy31, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy31);
}
#line 2481 "parser.c"
        break;
      case 97: /* shift_expr ::= shift_expr shift_op match_expr */
      case 103: /* arith_expr ::= arith_expr arith_op term */
      case 106: /* term ::= term term_op factor */
#line 991 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy31);
    yygotominor.yy31 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy31, VAL2ID(yymsp[-1].minor.yy31), yymsp[0].minor.yy31);
}
#line 2491 "parser.c"
        break;
      case 98: /* shift_op ::= LSHIFT */
      case 99: /* shift_op ::= RSHIFT */
      case 104: /* arith_op ::= PLUS */
      case 105: /* arith_op ::= MINUS */
      case 108: /* term_op ::= STAR */
      case 109: /* term_op ::= DIV */
      case 110: /* term_op ::= DIV_DIV */
      case 111: /* term_op ::= PERCENT */
#line 996 "parser.y"
{
    yygotominor.yy31 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2505 "parser.c"
        break;
      case 101: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1006 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy31);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy31 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy31, id, yymsp[0].minor.yy31);
}
#line 2514 "parser.c"
        break;
      case 112: /* factor ::= PLUS factor */
#line 1048 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy31 = FuncCall_new3(env, lineno, yymsp[0].minor.yy31, id);
}
#line 2523 "parser.c"
        break;
      case 113: /* factor ::= MINUS factor */
#line 1053 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy31 = FuncCall_new3(env, lineno, yymsp[0].minor.yy31, id);
}
#line 2532 "parser.c"
        break;
      case 114: /* factor ::= TILDA factor */
#line 1058 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy31 = FuncCall_new3(env, lineno, yymsp[0].minor.yy31, id);
}
#line 2541 "parser.c"
        break;
      case 118: /* postfix_expr ::= postfix_expr LPAR args_opt RPAR blockarg_opt */
#line 1074 "parser.y"
{
    yygotominor.yy31 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy31), yymsp[-4].minor.yy31, yymsp[-2].minor.yy31, yymsp[0].minor.yy31);
}
#line 2548 "parser.c"
        break;
      case 119: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1077 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy31);
    yygotominor.yy31 = Subscript_new(env, lineno, yymsp[-3].minor.yy31, yymsp[-1].minor.yy31);
}
#line 2556 "parser.c"
        break;
      case 120: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1081 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy31);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy31 = Attr_new(env, lineno, yymsp[-2].minor.yy31, id);
}
#line 2565 "parser.c"
        break;
      case 121: /* atom ::= NAME */
#line 1087 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy31 = Variable_new(env, lineno, id);
}
#line 2574 "parser.c"
        break;
      case 122: /* atom ::= NUMBER */
      case 123: /* atom ::= REGEXP */
      case 124: /* atom ::= STRING */
      case 125: /* atom ::= SYMBOL */
#line 1092 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy31 = Literal_new(env, lineno, val);
}
#line 2586 "parser.c"
        break;
      case 126: /* atom ::= NIL */
#line 1112 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy31 = Literal_new(env, lineno, YNIL);
}
#line 2594 "parser.c"
        break;
      case 127: /* atom ::= TRUE */
#line 1116 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy31 = Literal_new(env, lineno, YTRUE);
}
#line 2602 "parser.c"
        break;
      case 128: /* atom ::= FALSE */
#line 1120 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy31 = Literal_new(env, lineno, YFALSE);
}
#line 2610 "parser.c"
        break;
      case 129: /* atom ::= LINE */
#line 1124 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy31 = Literal_new(env, lineno, val);
}
#line 2619 "parser.c"
        break;
      case 130: /* atom ::= LBRACKET args_opt RBRACKET */
#line 1129 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy31 = Array_new(env, lineno, yymsp[-1].minor.yy31);
}
#line 2627 "parser.c"
        break;
      case 131: /* atom ::= LPAR expr RPAR */
      case 138: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1133 "parser.y"
{
    yygotominor.yy31 = yymsp[-1].minor.yy31;
}
#line 2635 "parser.c"
        break;
      case 135: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 136: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1147 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy31 = BlockArg_new(env, lineno, yymsp[-3].minor.yy31, yymsp[-1].minor.yy31);
}
#line 2644 "parser.c"
        break;
      case 140: /* excepts ::= excepts except */
#line 1166 "parser.y"
{
    yygotominor.yy31 = Array_push(env, yymsp[-1].minor.yy31, yymsp[0].minor.yy31);
}
#line 2651 "parser.c"
        break;
      case 141: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1170 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy31 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy31, id, yymsp[0].minor.yy31);
}
#line 2661 "parser.c"
        break;
      case 142: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1176 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy31 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy31, NO_EXC_VAR, yymsp[0].minor.yy31);
}
#line 2669 "parser.c"
        break;
      case 143: /* except ::= EXCEPT NEWLINE stmts */
#line 1180 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy31 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy31);
}
#line 2677 "parser.c"
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
  locals.vals[4] = NULL;
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
