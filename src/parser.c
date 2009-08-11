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
Args_new(YogEnv* env, uint_t lineno, YogVal posargs, YogVal kwargs)
{
    SAVE_ARGS2(env, posargs, kwargs);
    YogVal args = YUNDEF;
    PUSH_LOCAL(env, args);

    args = YogNode_new(env, NODE_ARGS, lineno);
    PTR_AS(YogNode, args)->u.args.posargs = posargs;
    PTR_AS(YogNode, args)->u.args.kwargs = kwargs;

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

    args = Args_new(env, lineno, posargs, YNIL);

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

#define TOKEN(token)            PTR_AS(YogToken, (token))
#define TOKEN_ID(token)         TOKEN((token))->u.id
#define TOKEN_LINENO(token)     TOKEN((token))->lineno
#define NODE_LINENO(node)       PTR_AS(YogNode, (node))->lineno
#line 684 "parser.c"
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
#define YYNOCODE 114
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE  YogVal 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  YogVal yy101;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL  YogVal* pval ;
#define ParseARG_PDECL , YogVal* pval 
#define ParseARG_FETCH  YogVal* pval  = PTR_AS(yyParser, parser)->pval 
#define ParseARG_STORE PTR_AS(yyParser, parser)->pval  = pval 
#define YYNSTATE 273
#define YYNRULE 163
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
 /*     0 */     2,  149,  150,   95,   23,   24,   30,   31,   32,  134,
 /*    10 */   202,   80,   65,  113,  236,   74,   68,  142,   28,  200,
 /*    20 */   188,  201,   17,    3,  167,    4,   16,   40,  437,   96,
 /*    30 */   176,  174,  175,  148,  240,   52,   53,  147,  151,  243,
 /*    40 */    54,   20,  247,  203,  204,  205,  206,  207,  208,  209,
 /*    50 */   210,   18,  154,  250,  180,   94,  117,  118,  191,  182,
 /*    60 */    69,  224,  119,  120,   73,  121,   58,   74,   68,   26,
 /*    70 */   270,  200,  188,  201,  138,  141,   34,   22,   22,  128,
 /*    80 */   114,  127,  223,  180,   94,  117,  118,  191,  182,   69,
 /*    90 */   131,  119,  120,   73,  121,  230,   74,   68,  211,   95,
 /*   100 */   200,  188,  201,   57,  176,  174,  175,  112,   73,  121,
 /*   110 */    92,   74,   68,  272,  231,  200,  188,  201,   51,  230,
 /*   120 */   149,  150,  152,   70,  176,  174,  175,   41,  180,   94,
 /*   130 */   117,  118,  191,  182,   69,   14,  119,  120,   73,  121,
 /*   140 */   105,   74,   68,  158,  255,  200,  188,  201,  180,   94,
 /*   150 */   117,  118,  191,  182,   69,   17,  119,  120,   73,  121,
 /*   160 */    49,   74,   68,  161,  259,  200,  188,  201,  106,  176,
 /*   170 */   174,  175,   87,  144,  145,  156,  160,  162,  261,  192,
 /*   180 */   193,  253,  149,  150,  152,  244,  245,   45,   97,  176,
 /*   190 */   174,  175,  168,  180,   94,  117,  118,  191,  182,   69,
 /*   200 */   225,  119,  120,   73,  121,   17,   74,   68,  264,    4,
 /*   210 */   200,  188,  201,  180,   94,  117,  118,  191,  182,   69,
 /*   220 */   116,  119,  120,   73,  121,   17,   74,   68,  178,   95,
 /*   230 */   200,  188,  201,   99,  176,  174,  175,  189,   71,  121,
 /*   240 */    84,   74,   68,  194,  195,  200,  188,  201,  273,   17,
 /*   250 */   149,  150,  152,   59,  176,  174,  175,   21,  180,   94,
 /*   260 */   117,  118,  191,  182,   69,  124,  119,  120,   73,  121,
 /*   270 */    93,   74,   68,   78,   47,  200,  188,  201,  180,   94,
 /*   280 */   117,  118,  191,  182,   69,  129,  119,  120,   73,  121,
 /*   290 */    17,   74,   68,  226,   35,  200,  188,  201,   60,  176,
 /*   300 */   174,  175,  268,  163,  145,  156,  160,  162,  261,  135,
 /*   310 */   234,  253,  157,  159,  257,  267,   42,  247,  133,  176,
 /*   320 */   174,  175,  136,  180,   94,  117,  118,  191,  182,   69,
 /*   330 */   139,  119,  120,   73,  121,   17,   74,   68,  232,  149,
 /*   340 */   200,  188,  201,  180,   94,  117,  118,  191,  182,   69,
 /*   350 */   238,  119,  120,   73,  121,   95,   74,   68,  242,  248,
 /*   360 */   200,  188,  201,  100,  176,  174,  175,   72,   68,  249,
 /*   370 */   251,  200,  188,  201,  146,  153,  155,  252,   17,   17,
 /*   380 */   253,  237,  271,  101,  176,  174,  175,  254,  180,   94,
 /*   390 */   117,  118,  191,  182,   69,  164,  119,  120,   73,  121,
 /*   400 */   177,   74,   68,  256,  258,  200,  188,  201,  180,   94,
 /*   410 */   117,  118,  191,  182,   69,   17,  119,  120,   73,  121,
 /*   420 */    95,   74,   68,  260,   95,  200,  188,  201,  102,  176,
 /*   430 */   174,  175,    5,   67,   38,   42,  200,  188,  201,   39,
 /*   440 */   184,  188,  201,   17,   43,    9,   44,   25,  170,  176,
 /*   450 */   174,  175,   48,  180,   94,  117,  118,  191,  182,   69,
 /*   460 */    33,  119,  120,   73,  121,   19,   74,   68,   76,   27,
 /*   470 */   200,  188,  201,  180,   94,  117,  118,  191,  182,   69,
 /*   480 */   212,  119,  120,   73,  121,   95,   74,   68,  215,   95,
 /*   490 */   200,  188,  201,  171,  176,  174,  175,   75,  229,   29,
 /*   500 */     8,  185,  188,  201,    6,  186,  188,  201,    7,   10,
 /*   510 */    79,  137,  233,  172,  176,  174,  175,   81,  180,   94,
 /*   520 */   117,  118,  191,  182,   69,  140,  119,  120,   73,  121,
 /*   530 */   235,   74,   68,  143,   46,  200,  188,  201,  180,   94,
 /*   540 */   117,  118,  191,  182,   69,   11,  119,  120,   73,  121,
 /*   550 */    95,   74,   68,  196,   50,  200,  188,  201,  104,  176,
 /*   560 */   174,  175,   55,   61,   82,  239,  187,  188,  201,  197,
 /*   570 */   198,  199,  166,  241,   66,   83,    1,  263,   62,  173,
 /*   580 */   174,  175,   85,  180,   94,  117,  118,  191,  182,   69,
 /*   590 */    86,  119,  120,   73,  121,   56,   74,   68,   63,   36,
 /*   600 */   200,  188,  201,  180,   94,  117,  118,  191,  182,   69,
 /*   610 */    88,  119,  120,   73,  121,   89,   74,   68,   64,   90,
 /*   620 */   200,  188,  201,  125,   95,  108,  118,  191,  182,   69,
 /*   630 */    91,  119,  120,   73,  121,   12,   74,   68,  265,  266,
 /*   640 */   200,  188,  201,  269,  169,  180,   94,  117,  118,  191,
 /*   650 */   182,   69,  222,  119,  120,   73,  121,   13,   74,   68,
 /*   660 */   438,  438,  200,  188,  201,  438,  438,   77,  438,  218,
 /*   670 */   438,  438,  115,  223,  180,   94,  117,  118,  191,  182,
 /*   680 */    69,  221,  119,  120,   73,  121,  438,   74,   68,  438,
 /*   690 */   438,  200,  188,  201,  438,  438,  438,  438,  438,  438,
 /*   700 */   438,  438,  438,  180,   94,  117,  118,  191,  182,   69,
 /*   710 */   125,  119,  120,   73,  121,  438,   74,   68,  438,  438,
 /*   720 */   200,  188,  201,  438,   98,  438,  438,  438,  438,  438,
 /*   730 */   438,  438,  180,   94,  117,  118,  191,  182,   69,  438,
 /*   740 */   119,  120,   73,  121,  438,   74,   68,  122,  438,  200,
 /*   750 */   188,  201,  438,  438,  438,   28,  216,  438,  438,  438,
 /*   760 */   438,  438,  438,  438,   40,  438,  438,  438,  438,  103,
 /*   770 */   438,  438,   52,   53,  438,  438,  438,   54,   20,  438,
 /*   780 */   203,  204,  205,  206,  207,  208,  209,  210,   18,  214,
 /*   790 */   202,  180,   94,  117,  118,  191,  182,   69,   28,  119,
 /*   800 */   120,   73,  121,  438,   74,   68,  438,   40,  200,  188,
 /*   810 */   201,  438,  438,  438,  107,   52,   53,  438,  438,  438,
 /*   820 */    54,   20,  220,  203,  204,  205,  206,  207,  208,  209,
 /*   830 */   210,   18,   15,  438,  438,  438,  180,   94,  117,  118,
 /*   840 */   191,  182,   69,  202,  119,  120,   73,  121,  166,   74,
 /*   850 */    68,   28,    1,  200,  188,  201,  438,   37,  438,  438,
 /*   860 */    40,  438,  438,  179,  438,  438,  438,  438,   52,   53,
 /*   870 */   438,  438,  438,   54,   20,   36,  203,  204,  205,  206,
 /*   880 */   207,  208,  209,  210,   18,  180,   94,  117,  118,  191,
 /*   890 */   182,   69,  190,  119,  120,   73,  121,  438,   74,   68,
 /*   900 */   438,  438,  200,  188,  201,  438,  438,  438,  438,  438,
 /*   910 */   438,  438,  219,  438,  180,   94,  117,  118,  191,  182,
 /*   920 */    69,  438,  119,  120,   73,  121,  438,   74,   68,  438,
 /*   930 */   438,  200,  188,  201,  180,   94,  117,  118,  191,  182,
 /*   940 */    69,  213,  119,  120,   73,  121,  438,   74,   68,  438,
 /*   950 */   438,  200,  188,  201,  438,  438,  438,  438,  438,  438,
 /*   960 */   438,  123,  438,  180,   94,  117,  118,  191,  182,   69,
 /*   970 */   438,  119,  120,   73,  121,  438,   74,   68,  438,  438,
 /*   980 */   200,  188,  201,  180,   94,  117,  118,  191,  182,   69,
 /*   990 */   217,  119,  120,   73,  121,  438,   74,   68,  438,  438,
 /*  1000 */   200,  188,  201,  438,  438,  438,  438,  438,  438,  438,
 /*  1010 */   227,  438,  180,   94,  117,  118,  191,  182,   69,  438,
 /*  1020 */   119,  120,   73,  121,  438,   74,   68,  438,  438,  200,
 /*  1030 */   188,  201,  180,   94,  117,  118,  191,  182,   69,  228,
 /*  1040 */   119,  120,   73,  121,  438,   74,   68,  438,  438,  200,
 /*  1050 */   188,  201,  438,  438,  438,  438,  438,  438,  438,  130,
 /*  1060 */   438,  180,   94,  117,  118,  191,  182,   69,  438,  119,
 /*  1070 */   120,   73,  121,  438,   74,   68,  438,  438,  200,  188,
 /*  1080 */   201,  180,   94,  117,  118,  191,  182,   69,  132,  119,
 /*  1090 */   120,   73,  121,  438,   74,   68,  438,  438,  200,  188,
 /*  1100 */   201,  438,  438,  438,  438,  438,  438,  438,  246,  438,
 /*  1110 */   180,   94,  117,  118,  191,  182,   69,  438,  119,  120,
 /*  1120 */    73,  121,  438,   74,   68,  438,  438,  200,  188,  201,
 /*  1130 */   180,   94,  117,  118,  191,  182,   69,  262,  119,  120,
 /*  1140 */    73,  121,  438,   74,   68,  438,  438,  200,  188,  201,
 /*  1150 */   438,  438,  438,  438,  438,  438,  438,  165,  438,  180,
 /*  1160 */    94,  117,  118,  191,  182,   69,  438,  119,  120,   73,
 /*  1170 */   121,  438,   74,   68,  438,  438,  200,  188,  201,  180,
 /*  1180 */    94,  117,  118,  191,  182,   69,  126,  119,  120,   73,
 /*  1190 */   121,  438,   74,   68,   28,  438,  200,  188,  201,  438,
 /*  1200 */   438,  438,  438,   40,  438,  438,  438,  438,  438,  438,
 /*  1210 */   438,   52,   53,  438,  438,  438,   54,   20,  438,  203,
 /*  1220 */   204,  205,  206,  207,  208,  209,  210,   18,  438,  122,
 /*  1230 */   438,   95,  438,  109,  191,  182,   69,   28,  119,  120,
 /*  1240 */    73,  121,  438,   74,   68,  438,   40,  200,  188,  201,
 /*  1250 */   438,  438,  438,  438,   52,   53,  438,  438,  438,   54,
 /*  1260 */    20,  438,  203,  204,  205,  206,  207,  208,  209,  210,
 /*  1270 */    18,  438,  202,  438,   95,  438,  438,  181,  182,   69,
 /*  1280 */    28,  119,  120,   73,  121,  438,   74,   68,  438,   40,
 /*  1290 */   200,  188,  201,  202,  438,  438,  438,   52,   53,  438,
 /*  1300 */   438,   28,   54,   20,  438,  203,  204,  205,  206,  207,
 /*  1310 */   208,  209,  210,   18,  438,  438,  438,  438,   52,   53,
 /*  1320 */   438,  438,  438,   54,   20,  438,  203,  204,  205,  206,
 /*  1330 */   207,  208,  209,  210,   18,   95,  438,  438,  183,  182,
 /*  1340 */    69,  438,  119,  120,   73,  121,  438,   74,   68,  438,
 /*  1350 */    95,  200,  188,  201,  438,  110,  438,  119,  120,   73,
 /*  1360 */   121,  438,   74,   68,  438,   95,  200,  188,  201,  438,
 /*  1370 */   438,  438,  111,  120,   73,  121,  438,   74,   68,  438,
 /*  1380 */   438,  200,  188,  201,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   22,   23,   87,    6,    7,    8,    9,   10,   11,
 /*    10 */    12,   13,   14,   97,   12,   99,  100,   19,   20,  103,
 /*    20 */   104,  105,    1,    3,   66,    5,    5,   29,   60,   61,
 /*    30 */    62,   63,   64,   77,   78,   37,   38,   76,   77,   78,
 /*    40 */    42,   43,   81,   45,   46,   47,   48,   49,   50,   51,
 /*    50 */    52,   53,   77,   78,   86,   87,   88,   89,   90,   91,
 /*    60 */    92,   64,   94,   95,   96,   97,   65,   99,  100,   15,
 /*    70 */   112,  103,  104,  105,   70,   71,   25,   57,   57,   82,
 /*    80 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*    90 */    67,   94,   95,   96,   97,   72,   99,  100,   44,   87,
 /*   100 */   103,  104,  105,   61,   62,   63,   64,   95,   96,   97,
 /*   110 */    12,   99,  100,  112,   67,  103,  104,  105,  102,   72,
 /*   120 */    22,   23,   24,   61,   62,   63,   64,   93,   86,   87,
 /*   130 */    88,   89,   90,   91,   92,    1,   94,   95,   96,   97,
 /*   140 */    66,   99,  100,   77,   78,  103,  104,  105,   86,   87,
 /*   150 */    88,   89,   90,   91,   92,    1,   94,   95,   96,   97,
 /*   160 */   101,   99,  100,   77,   78,  103,  104,  105,   61,   62,
 /*   170 */    63,   64,   12,   73,   74,   75,   76,   77,   78,   34,
 /*   180 */    35,   81,   22,   23,   24,   79,   80,   98,   61,   62,
 /*   190 */    63,   64,   58,   86,   87,   88,   89,   90,   91,   92,
 /*   200 */   106,   94,   95,   96,   97,    1,   99,  100,   54,    5,
 /*   210 */   103,  104,  105,   86,   87,   88,   89,   90,   91,   92,
 /*   220 */    12,   94,   95,   96,   97,    1,   99,  100,    4,   87,
 /*   230 */   103,  104,  105,   61,   62,   63,   64,   85,   96,   97,
 /*   240 */    12,   99,  100,   37,   38,  103,  104,  105,    0,    1,
 /*   250 */    22,   23,   24,   61,   62,   63,   64,   15,   86,   87,
 /*   260 */    88,   89,   90,   91,   92,  109,   94,   95,   96,   97,
 /*   270 */    53,   99,  100,   56,   43,  103,  104,  105,   86,   87,
 /*   280 */    88,   89,   90,   91,   92,  111,   94,   95,   96,   97,
 /*   290 */     1,   99,  100,    4,   17,  103,  104,  105,   61,   62,
 /*   300 */    63,   64,   17,   73,   74,   75,   76,   77,   78,   68,
 /*   310 */    12,   81,   76,   77,   78,   30,   31,   81,   61,   62,
 /*   320 */    63,   64,   69,   86,   87,   88,   89,   90,   91,   92,
 /*   330 */    71,   94,   95,   96,   97,    1,   99,  100,    4,   22,
 /*   340 */   103,  104,  105,   86,   87,   88,   89,   90,   91,   92,
 /*   350 */    78,   94,   95,   96,   97,   87,   99,  100,   78,   80,
 /*   360 */   103,  104,  105,   61,   62,   63,   64,   99,  100,   78,
 /*   370 */    78,  103,  104,  105,   75,   76,   77,   78,    1,    1,
 /*   380 */    81,    4,    4,   61,   62,   63,   64,   78,   86,   87,
 /*   390 */    88,   89,   90,   91,   92,  111,   94,   95,   96,   97,
 /*   400 */     4,   99,  100,   78,   78,  103,  104,  105,   86,   87,
 /*   410 */    88,   89,   90,   91,   92,    1,   94,   95,   96,   97,
 /*   420 */    87,   99,  100,   78,   87,  103,  104,  105,   61,   62,
 /*   430 */    63,   64,    1,  100,   27,   31,  103,  104,  105,   28,
 /*   440 */   103,  104,  105,    1,   32,    3,   33,   26,   61,   62,
 /*   450 */    63,   64,   36,   86,   87,   88,   89,   90,   91,   92,
 /*   460 */    18,   94,   95,   96,   97,   15,   99,  100,   15,   26,
 /*   470 */   103,  104,  105,   86,   87,   88,   89,   90,   91,   92,
 /*   480 */    21,   94,   95,   96,   97,   87,   99,  100,   54,   87,
 /*   490 */   103,  104,  105,   61,   62,   63,   64,   21,    4,   55,
 /*   500 */     1,  103,  104,  105,    1,  103,  104,  105,    1,    1,
 /*   510 */    12,   15,   12,   61,   62,   63,   64,   15,   86,   87,
 /*   520 */    88,   89,   90,   91,   92,   16,   94,   95,   96,   97,
 /*   530 */    12,   99,  100,   12,   20,  103,  104,  105,   86,   87,
 /*   540 */    88,   89,   90,   91,   92,   21,   94,   95,   96,   97,
 /*   550 */    87,   99,  100,   23,   15,  103,  104,  105,   61,   62,
 /*   560 */    63,   64,   15,   15,   15,   12,  103,  104,  105,   39,
 /*   570 */    40,   41,   16,   12,   12,   15,   20,   44,   15,   62,
 /*   580 */    63,   64,   15,   86,   87,   88,   89,   90,   91,   92,
 /*   590 */    15,   94,   95,   96,   97,   15,   99,  100,   15,   43,
 /*   600 */   103,  104,  105,   86,   87,   88,   89,   90,   91,   92,
 /*   610 */    15,   94,   95,   96,   97,   15,   99,  100,   15,   15,
 /*   620 */   103,  104,  105,   64,   87,   88,   89,   90,   91,   92,
 /*   630 */    15,   94,   95,   96,   97,    1,   99,  100,   44,   12,
 /*   640 */   103,  104,  105,    4,   12,   86,   87,   88,   89,   90,
 /*   650 */    91,   92,   64,   94,   95,   96,   97,    1,   99,  100,
 /*   660 */   113,  113,  103,  104,  105,  113,  113,  108,  113,  110,
 /*   670 */   113,  113,   84,   85,   86,   87,   88,   89,   90,   91,
 /*   680 */    92,   64,   94,   95,   96,   97,  113,   99,  100,  113,
 /*   690 */   113,  103,  104,  105,  113,  113,  113,  113,  113,  113,
 /*   700 */   113,  113,  113,   86,   87,   88,   89,   90,   91,   92,
 /*   710 */    64,   94,   95,   96,   97,  113,   99,  100,  113,  113,
 /*   720 */   103,  104,  105,  113,  107,  113,  113,  113,  113,  113,
 /*   730 */   113,  113,   86,   87,   88,   89,   90,   91,   92,  113,
 /*   740 */    94,   95,   96,   97,  113,   99,  100,   12,  113,  103,
 /*   750 */   104,  105,  113,  113,  113,   20,  110,  113,  113,  113,
 /*   760 */   113,  113,  113,  113,   29,  113,  113,  113,  113,   64,
 /*   770 */   113,  113,   37,   38,  113,  113,  113,   42,   43,  113,
 /*   780 */    45,   46,   47,   48,   49,   50,   51,   52,   53,   54,
 /*   790 */    12,   86,   87,   88,   89,   90,   91,   92,   20,   94,
 /*   800 */    95,   96,   97,  113,   99,  100,  113,   29,  103,  104,
 /*   810 */   105,  113,  113,  113,   64,   37,   38,  113,  113,  113,
 /*   820 */    42,   43,   44,   45,   46,   47,   48,   49,   50,   51,
 /*   830 */    52,   53,    1,  113,  113,  113,   86,   87,   88,   89,
 /*   840 */    90,   91,   92,   12,   94,   95,   96,   97,   16,   99,
 /*   850 */   100,   20,   20,  103,  104,  105,  113,   25,  113,  113,
 /*   860 */    29,  113,  113,   64,  113,  113,  113,  113,   37,   38,
 /*   870 */   113,  113,  113,   42,   43,   43,   45,   46,   47,   48,
 /*   880 */    49,   50,   51,   52,   53,   86,   87,   88,   89,   90,
 /*   890 */    91,   92,   64,   94,   95,   96,   97,  113,   99,  100,
 /*   900 */   113,  113,  103,  104,  105,  113,  113,  113,  113,  113,
 /*   910 */   113,  113,   64,  113,   86,   87,   88,   89,   90,   91,
 /*   920 */    92,  113,   94,   95,   96,   97,  113,   99,  100,  113,
 /*   930 */   113,  103,  104,  105,   86,   87,   88,   89,   90,   91,
 /*   940 */    92,   64,   94,   95,   96,   97,  113,   99,  100,  113,
 /*   950 */   113,  103,  104,  105,  113,  113,  113,  113,  113,  113,
 /*   960 */   113,   64,  113,   86,   87,   88,   89,   90,   91,   92,
 /*   970 */   113,   94,   95,   96,   97,  113,   99,  100,  113,  113,
 /*   980 */   103,  104,  105,   86,   87,   88,   89,   90,   91,   92,
 /*   990 */    64,   94,   95,   96,   97,  113,   99,  100,  113,  113,
 /*  1000 */   103,  104,  105,  113,  113,  113,  113,  113,  113,  113,
 /*  1010 */    64,  113,   86,   87,   88,   89,   90,   91,   92,  113,
 /*  1020 */    94,   95,   96,   97,  113,   99,  100,  113,  113,  103,
 /*  1030 */   104,  105,   86,   87,   88,   89,   90,   91,   92,   64,
 /*  1040 */    94,   95,   96,   97,  113,   99,  100,  113,  113,  103,
 /*  1050 */   104,  105,  113,  113,  113,  113,  113,  113,  113,   64,
 /*  1060 */   113,   86,   87,   88,   89,   90,   91,   92,  113,   94,
 /*  1070 */    95,   96,   97,  113,   99,  100,  113,  113,  103,  104,
 /*  1080 */   105,   86,   87,   88,   89,   90,   91,   92,   64,   94,
 /*  1090 */    95,   96,   97,  113,   99,  100,  113,  113,  103,  104,
 /*  1100 */   105,  113,  113,  113,  113,  113,  113,  113,   64,  113,
 /*  1110 */    86,   87,   88,   89,   90,   91,   92,  113,   94,   95,
 /*  1120 */    96,   97,  113,   99,  100,  113,  113,  103,  104,  105,
 /*  1130 */    86,   87,   88,   89,   90,   91,   92,   64,   94,   95,
 /*  1140 */    96,   97,  113,   99,  100,  113,  113,  103,  104,  105,
 /*  1150 */   113,  113,  113,  113,  113,  113,  113,   64,  113,   86,
 /*  1160 */    87,   88,   89,   90,   91,   92,  113,   94,   95,   96,
 /*  1170 */    97,  113,   99,  100,  113,  113,  103,  104,  105,   86,
 /*  1180 */    87,   88,   89,   90,   91,   92,   12,   94,   95,   96,
 /*  1190 */    97,  113,   99,  100,   20,  113,  103,  104,  105,  113,
 /*  1200 */   113,  113,  113,   29,  113,  113,  113,  113,  113,  113,
 /*  1210 */   113,   37,   38,  113,  113,  113,   42,   43,  113,   45,
 /*  1220 */    46,   47,   48,   49,   50,   51,   52,   53,  113,   12,
 /*  1230 */   113,   87,  113,   89,   90,   91,   92,   20,   94,   95,
 /*  1240 */    96,   97,  113,   99,  100,  113,   29,  103,  104,  105,
 /*  1250 */   113,  113,  113,  113,   37,   38,  113,  113,  113,   42,
 /*  1260 */    43,  113,   45,   46,   47,   48,   49,   50,   51,   52,
 /*  1270 */    53,  113,   12,  113,   87,  113,  113,   90,   91,   92,
 /*  1280 */    20,   94,   95,   96,   97,  113,   99,  100,  113,   29,
 /*  1290 */   103,  104,  105,   12,  113,  113,  113,   37,   38,  113,
 /*  1300 */   113,   20,   42,   43,  113,   45,   46,   47,   48,   49,
 /*  1310 */    50,   51,   52,   53,  113,  113,  113,  113,   37,   38,
 /*  1320 */   113,  113,  113,   42,   43,  113,   45,   46,   47,   48,
 /*  1330 */    49,   50,   51,   52,   53,   87,  113,  113,   90,   91,
 /*  1340 */    92,  113,   94,   95,   96,   97,  113,   99,  100,  113,
 /*  1350 */    87,  103,  104,  105,  113,   92,  113,   94,   95,   96,
 /*  1360 */    97,  113,   99,  100,  113,   87,  103,  104,  105,  113,
 /*  1370 */   113,  113,   94,   95,   96,   97,  113,   99,  100,  113,
 /*  1380 */   113,  103,  104,  105,
};
#define YY_SHIFT_USE_DFLT (-22)
#define YY_SHIFT_MAX 172
static const short yy_shift_ofst[] = {
 /*     0 */    -2, 1174,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,
 /*    10 */    -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2,  735, 1174,
 /*    20 */   778, 1217,  831, 1260, 1260, 1260, 1260, 1260, 1260, 1260,
 /*    30 */  1260, 1260, 1260, 1260, 1260, 1260, 1260, 1260, 1260, 1260,
 /*    40 */  1260, 1281, 1281, 1281, 1281, 1281,   98,   98, 1281, 1281,
 /*    50 */   160, 1281, 1281, 1281, 1281,  228,  228,   21,   20,  442,
 /*    60 */   442,  -21,  -21,  -21,  -21,    2,   51,  530,  530,  285,
 /*    70 */   204,  145,  206,  145,  206,  217,  208,  242,  231,  277,
 /*    80 */   298,    2,  317,  317,   51,  317,  317,   51,  317,  317,
 /*    90 */   317,  317,   51,  231,  832,  556,  248,  224,   54,  289,
 /*   100 */   334,  377,  154,  134,  378,  396,  414,  431,  407,  411,
 /*   110 */   404,  412,  413,  416,  450,  453,  421,  407,  411,  412,
 /*   120 */   413,  416,  443,  459,  434,  444,  421,  453,  476,  503,
 /*   130 */   507,  494,  499,  414,  498,  508,  496,  500,  502,  509,
 /*   140 */   518,  509,  521,  514,  524,  539,  547,  548,  549,  553,
 /*   150 */   561,  560,  562,  563,  567,  575,  580,  583,  595,  600,
 /*   160 */   603,  604,  615,  533,  634,  594,  627,  639,  632,  656,
 /*   170 */   414,  414,  414,
};
#define YY_REDUCE_USE_DFLT (-85)
#define YY_REDUCE_MAX 93
static const short yy_reduce_ofst[] = {
 /*     0 */   -32,   -3,   42,   62,  107,  127,  172,  192,  237,  257,
 /*    10 */   302,  322,  367,  387,  432,  452,  497,  517,  559,  588,
 /*    20 */   617,  646,  705,  750,  799,  828,  848,  877,  897,  926,
 /*    30 */   946,  975,  995, 1024, 1044, 1073, 1093,  537, 1144, 1187,
 /*    40 */  1248, 1263, 1278,   12,  142,  -84,  100,  230,  268,  333,
 /*    50 */   299,  337,  398,  402,  463,  -39,  236,    1,  -42,   23,
 /*    60 */    47,  -44,  -25,   66,   86,    4,  106,   16,   16,   34,
 /*    70 */    74,   89,   59,   89,   59,   94,  152,  156,  174,  241,
 /*    80 */   253,  259,  272,  280,  279,  291,  292,  279,  309,  325,
 /*    90 */   326,  345,  279,  284,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   276,  349,  276,  276,  276,  276,  276,  276,  276,  276,
 /*    10 */   276,  276,  276,  276,  276,  276,  276,  276,  436,  436,
 /*    20 */   436,  423,  436,  436,  283,  436,  436,  436,  436,  436,
 /*    30 */   285,  287,  436,  436,  436,  436,  436,  436,  436,  436,
 /*    40 */   436,  436,  436,  436,  436,  436,  337,  337,  436,  436,
 /*    50 */   436,  436,  436,  436,  436,  436,  436,  436,  434,  303,
 /*    60 */   303,  436,  436,  436,  436,  436,  341,  384,  383,  367,
 /*    70 */   434,  376,  382,  375,  381,  424,  436,  422,  427,  299,
 /*    80 */   436,  436,  436,  436,  436,  436,  436,  345,  436,  436,
 /*    90 */   436,  436,  344,  427,  397,  397,  436,  436,  436,  436,
 /*   100 */   436,  436,  436,  436,  436,  436,  435,  436,  359,  362,
 /*   110 */   368,  372,  374,  378,  350,  351,  436,  360,  361,  371,
 /*   120 */   373,  377,  402,  436,  436,  436,  402,  352,  436,  436,
 /*   130 */   436,  436,  436,  304,  436,  436,  291,  436,  292,  294,
 /*   140 */   436,  293,  436,  436,  436,  321,  313,  309,  307,  436,
 /*   150 */   436,  311,  436,  317,  315,  319,  329,  325,  323,  327,
 /*   160 */   333,  331,  335,  436,  436,  436,  436,  436,  436,  436,
 /*   170 */   431,  432,  433,  275,  277,  278,  274,  279,  282,  284,
 /*   180 */   358,  364,  365,  366,  387,  393,  394,  395,  396,  356,
 /*   190 */   357,  363,  379,  380,  385,  386,  389,  390,  391,  392,
 /*   200 */   388,  398,  402,  403,  404,  405,  406,  407,  408,  409,
 /*   210 */   410,  411,  415,  421,  413,  414,  419,  420,  418,  417,
 /*   220 */   412,  416,  354,  355,  353,  399,  425,  286,  288,  289,
 /*   230 */   301,  302,  290,  298,  297,  296,  295,  305,  306,  338,
 /*   240 */   308,  339,  310,  312,  340,  342,  343,  347,  348,  314,
 /*   250 */   316,  318,  320,  346,  322,  324,  326,  328,  330,  332,
 /*   260 */   334,  336,  300,  428,  426,  400,  401,  369,  370,  280,
 /*   270 */   430,  281,  429,
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
  "LPAR",          "RPAR",          "STAR_STAR",     "STAR",        
  "AMPER",         "EQUAL",         "COLON",         "BAR_BAR",     
  "AND_AND",       "NOT",           "LESS",          "XOR",         
  "BAR",           "AND",           "LSHIFT",        "RSHIFT",      
  "EQUAL_TILDA",   "PLUS",          "MINUS",         "DIV",         
  "DIV_DIV",       "PERCENT",       "TILDA",         "LBRACKET",    
  "RBRACKET",      "NUMBER",        "REGEXP",        "STRING",      
  "SYMBOL",        "NIL",           "TRUE",          "FALSE",       
  "LINE",          "LBRACE",        "RBRACE",        "EQUAL_GREATER",
  "DO",            "EXCEPT",        "AS",            "error",       
  "module",        "stmts",         "stmt",          "func_def",    
  "expr",          "excepts",       "finally_opt",   "if_tail",     
  "super_opt",     "names",         "dotted_names",  "dotted_name", 
  "else_opt",      "params",        "params_without_default",  "params_with_default",
  "block_param",   "var_param",     "kw_param",      "param_default_opt",
  "param_default",  "param_with_default",  "args",          "posargs",     
  "kwargs",        "kwarg",         "assign_expr",   "postfix_expr",
  "logical_or_expr",  "logical_and_expr",  "not_expr",      "comparison",  
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
 /*  65 */ "kw_param ::= STAR_STAR NAME",
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
 /*  76 */ "args ::=",
 /*  77 */ "args ::= posargs",
 /*  78 */ "args ::= posargs COMMA kwargs",
 /*  79 */ "args ::= kwargs",
 /*  80 */ "posargs ::= expr",
 /*  81 */ "posargs ::= posargs COMMA expr",
 /*  82 */ "kwargs ::= kwarg",
 /*  83 */ "kwargs ::= kwargs COMMA kwarg",
 /*  84 */ "kwarg ::= NAME COLON expr",
 /*  85 */ "expr ::= assign_expr",
 /*  86 */ "assign_expr ::= postfix_expr EQUAL logical_or_expr",
 /*  87 */ "assign_expr ::= logical_or_expr",
 /*  88 */ "logical_or_expr ::= logical_and_expr",
 /*  89 */ "logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr",
 /*  90 */ "logical_and_expr ::= not_expr",
 /*  91 */ "logical_and_expr ::= logical_and_expr AND_AND not_expr",
 /*  92 */ "not_expr ::= comparison",
 /*  93 */ "not_expr ::= NOT not_expr",
 /*  94 */ "comparison ::= xor_expr",
 /*  95 */ "comparison ::= xor_expr comp_op xor_expr",
 /*  96 */ "comp_op ::= LESS",
 /*  97 */ "comp_op ::= GREATER",
 /*  98 */ "xor_expr ::= or_expr",
 /*  99 */ "xor_expr ::= xor_expr XOR or_expr",
 /* 100 */ "or_expr ::= and_expr",
 /* 101 */ "or_expr ::= or_expr BAR and_expr",
 /* 102 */ "and_expr ::= shift_expr",
 /* 103 */ "and_expr ::= and_expr AND shift_expr",
 /* 104 */ "shift_expr ::= match_expr",
 /* 105 */ "shift_expr ::= shift_expr shift_op match_expr",
 /* 106 */ "shift_op ::= LSHIFT",
 /* 107 */ "shift_op ::= RSHIFT",
 /* 108 */ "match_expr ::= arith_expr",
 /* 109 */ "match_expr ::= match_expr EQUAL_TILDA arith_expr",
 /* 110 */ "arith_expr ::= term",
 /* 111 */ "arith_expr ::= arith_expr arith_op term",
 /* 112 */ "arith_op ::= PLUS",
 /* 113 */ "arith_op ::= MINUS",
 /* 114 */ "term ::= term term_op factor",
 /* 115 */ "term ::= factor",
 /* 116 */ "term_op ::= STAR",
 /* 117 */ "term_op ::= DIV",
 /* 118 */ "term_op ::= DIV_DIV",
 /* 119 */ "term_op ::= PERCENT",
 /* 120 */ "factor ::= PLUS factor",
 /* 121 */ "factor ::= MINUS factor",
 /* 122 */ "factor ::= TILDA factor",
 /* 123 */ "factor ::= power",
 /* 124 */ "power ::= postfix_expr",
 /* 125 */ "postfix_expr ::= atom",
 /* 126 */ "postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt",
 /* 127 */ "postfix_expr ::= postfix_expr LBRACKET expr RBRACKET",
 /* 128 */ "postfix_expr ::= postfix_expr DOT NAME",
 /* 129 */ "atom ::= NAME",
 /* 130 */ "atom ::= NUMBER",
 /* 131 */ "atom ::= REGEXP",
 /* 132 */ "atom ::= STRING",
 /* 133 */ "atom ::= SYMBOL",
 /* 134 */ "atom ::= NIL",
 /* 135 */ "atom ::= TRUE",
 /* 136 */ "atom ::= FALSE",
 /* 137 */ "atom ::= LINE",
 /* 138 */ "atom ::= LBRACKET exprs RBRACKET",
 /* 139 */ "atom ::= LBRACKET RBRACKET",
 /* 140 */ "atom ::= LBRACE RBRACE",
 /* 141 */ "atom ::= LBRACE dict_elems comma_opt RBRACE",
 /* 142 */ "atom ::= LPAR expr RPAR",
 /* 143 */ "exprs ::= expr",
 /* 144 */ "exprs ::= exprs COMMA expr",
 /* 145 */ "dict_elems ::= dict_elem",
 /* 146 */ "dict_elems ::= dict_elems COMMA dict_elem",
 /* 147 */ "dict_elem ::= expr EQUAL_GREATER expr",
 /* 148 */ "dict_elem ::= NAME COLON expr",
 /* 149 */ "comma_opt ::=",
 /* 150 */ "comma_opt ::= COMMA",
 /* 151 */ "blockarg_opt ::=",
 /* 152 */ "blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END",
 /* 153 */ "blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE",
 /* 154 */ "blockarg_params_opt ::=",
 /* 155 */ "blockarg_params_opt ::= LBRACKET params RBRACKET",
 /* 156 */ "excepts ::= except",
 /* 157 */ "excepts ::= excepts except",
 /* 158 */ "except ::= EXCEPT expr AS NAME NEWLINE stmts",
 /* 159 */ "except ::= EXCEPT expr NEWLINE stmts",
 /* 160 */ "except ::= EXCEPT NEWLINE stmts",
 /* 161 */ "finally_opt ::=",
 /* 162 */ "finally_opt ::= FINALLY stmts",
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
  { 60, 1 },
  { 61, 1 },
  { 61, 3 },
  { 62, 0 },
  { 62, 1 },
  { 62, 1 },
  { 62, 7 },
  { 62, 5 },
  { 62, 5 },
  { 62, 5 },
  { 62, 1 },
  { 62, 2 },
  { 62, 1 },
  { 62, 2 },
  { 62, 1 },
  { 62, 2 },
  { 62, 6 },
  { 62, 6 },
  { 62, 2 },
  { 62, 2 },
  { 70, 1 },
  { 70, 3 },
  { 71, 1 },
  { 71, 3 },
  { 69, 1 },
  { 69, 3 },
  { 68, 0 },
  { 68, 2 },
  { 67, 1 },
  { 67, 5 },
  { 72, 0 },
  { 72, 2 },
  { 63, 7 },
  { 73, 9 },
  { 73, 7 },
  { 73, 7 },
  { 73, 5 },
  { 73, 7 },
  { 73, 5 },
  { 73, 5 },
  { 73, 3 },
  { 73, 7 },
  { 73, 5 },
  { 73, 5 },
  { 73, 3 },
  { 73, 5 },
  { 73, 3 },
  { 73, 3 },
  { 73, 1 },
  { 73, 7 },
  { 73, 5 },
  { 73, 5 },
  { 73, 3 },
  { 73, 5 },
  { 73, 3 },
  { 73, 3 },
  { 73, 1 },
  { 73, 5 },
  { 73, 3 },
  { 73, 3 },
  { 73, 1 },
  { 73, 3 },
  { 73, 1 },
  { 73, 1 },
  { 73, 0 },
  { 78, 2 },
  { 77, 2 },
  { 76, 3 },
  { 79, 0 },
  { 79, 1 },
  { 80, 2 },
  { 74, 1 },
  { 74, 3 },
  { 75, 1 },
  { 75, 3 },
  { 81, 2 },
  { 82, 0 },
  { 82, 1 },
  { 82, 3 },
  { 82, 1 },
  { 83, 1 },
  { 83, 3 },
  { 84, 1 },
  { 84, 3 },
  { 85, 3 },
  { 64, 1 },
  { 86, 3 },
  { 86, 1 },
  { 88, 1 },
  { 88, 3 },
  { 89, 1 },
  { 89, 3 },
  { 90, 1 },
  { 90, 2 },
  { 91, 1 },
  { 91, 3 },
  { 93, 1 },
  { 93, 1 },
  { 92, 1 },
  { 92, 3 },
  { 94, 1 },
  { 94, 3 },
  { 95, 1 },
  { 95, 3 },
  { 96, 1 },
  { 96, 3 },
  { 98, 1 },
  { 98, 1 },
  { 97, 1 },
  { 97, 3 },
  { 99, 1 },
  { 99, 3 },
  { 101, 1 },
  { 101, 1 },
  { 100, 3 },
  { 100, 1 },
  { 102, 1 },
  { 102, 1 },
  { 102, 1 },
  { 102, 1 },
  { 103, 2 },
  { 103, 2 },
  { 103, 2 },
  { 103, 1 },
  { 104, 1 },
  { 87, 1 },
  { 87, 5 },
  { 87, 4 },
  { 87, 3 },
  { 105, 1 },
  { 105, 1 },
  { 105, 1 },
  { 105, 1 },
  { 105, 1 },
  { 105, 1 },
  { 105, 1 },
  { 105, 1 },
  { 105, 1 },
  { 105, 3 },
  { 105, 2 },
  { 105, 2 },
  { 105, 4 },
  { 105, 3 },
  { 107, 1 },
  { 107, 3 },
  { 108, 1 },
  { 108, 3 },
  { 110, 3 },
  { 110, 3 },
  { 109, 0 },
  { 109, 1 },
  { 106, 0 },
  { 106, 5 },
  { 106, 5 },
  { 111, 0 },
  { 111, 3 },
  { 65, 1 },
  { 65, 2 },
  { 112, 6 },
  { 112, 4 },
  { 112, 3 },
  { 66, 0 },
  { 66, 2 },
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
#line 680 "parser.y"
{
    *pval = yymsp[0].minor.yy101;
}
#line 2027 "parser.c"
        break;
      case 1: /* stmts ::= stmt */
      case 20: /* dotted_names ::= dotted_name */
      case 73: /* params_with_default ::= param_with_default */
      case 80: /* posargs ::= expr */
      case 82: /* kwargs ::= kwarg */
      case 143: /* exprs ::= expr */
      case 145: /* dict_elems ::= dict_elem */
      case 156: /* excepts ::= except */
#line 684 "parser.y"
{
    yygotominor.yy101 = make_array_with(env, yymsp[0].minor.yy101);
}
#line 2041 "parser.c"
        break;
      case 2: /* stmts ::= stmts NEWLINE stmt */
      case 21: /* dotted_names ::= dotted_names COMMA dotted_name */
      case 74: /* params_with_default ::= params_with_default COMMA param_with_default */
#line 687 "parser.y"
{
    yygotominor.yy101 = Array_push(env, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2050 "parser.c"
        break;
      case 3: /* stmt ::= */
      case 26: /* super_opt ::= */
      case 30: /* else_opt ::= */
      case 68: /* param_default_opt ::= */
      case 76: /* args ::= */
      case 149: /* comma_opt ::= */
      case 151: /* blockarg_opt ::= */
      case 154: /* blockarg_params_opt ::= */
      case 161: /* finally_opt ::= */
#line 691 "parser.y"
{
    yygotominor.yy101 = YNIL;
}
#line 2065 "parser.c"
        break;
      case 4: /* stmt ::= func_def */
      case 5: /* stmt ::= expr */
      case 27: /* super_opt ::= GREATER expr */
      case 28: /* if_tail ::= else_opt */
      case 31: /* else_opt ::= ELSE stmts */
      case 69: /* param_default_opt ::= param_default */
      case 70: /* param_default ::= EQUAL expr */
      case 85: /* expr ::= assign_expr */
      case 87: /* assign_expr ::= logical_or_expr */
      case 88: /* logical_or_expr ::= logical_and_expr */
      case 90: /* logical_and_expr ::= not_expr */
      case 92: /* not_expr ::= comparison */
      case 94: /* comparison ::= xor_expr */
      case 98: /* xor_expr ::= or_expr */
      case 100: /* or_expr ::= and_expr */
      case 102: /* and_expr ::= shift_expr */
      case 104: /* shift_expr ::= match_expr */
      case 108: /* match_expr ::= arith_expr */
      case 110: /* arith_expr ::= term */
      case 115: /* term ::= factor */
      case 123: /* factor ::= power */
      case 124: /* power ::= postfix_expr */
      case 125: /* postfix_expr ::= atom */
      case 162: /* finally_opt ::= FINALLY stmts */
#line 694 "parser.y"
{
    yygotominor.yy101 = yymsp[0].minor.yy101;
}
#line 2095 "parser.c"
        break;
      case 6: /* stmt ::= TRY stmts excepts ELSE stmts finally_opt END */
#line 700 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    yygotominor.yy101 = ExceptFinally_new(env, lineno, yymsp[-5].minor.yy101, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, yymsp[-1].minor.yy101);
}
#line 2103 "parser.c"
        break;
      case 7: /* stmt ::= TRY stmts excepts finally_opt END */
#line 704 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy101 = ExceptFinally_new(env, lineno, yymsp[-3].minor.yy101, yymsp[-2].minor.yy101, YNIL, yymsp[-1].minor.yy101);
}
#line 2111 "parser.c"
        break;
      case 8: /* stmt ::= TRY stmts FINALLY stmts END */
#line 708 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy101 = Finally_new(env, lineno, yymsp[-3].minor.yy101, yymsp[-1].minor.yy101);
}
#line 2119 "parser.c"
        break;
      case 9: /* stmt ::= WHILE expr NEWLINE stmts END */
#line 712 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy101 = While_new(env, lineno, yymsp[-3].minor.yy101, yymsp[-1].minor.yy101);
}
#line 2127 "parser.c"
        break;
      case 10: /* stmt ::= BREAK */
#line 716 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy101 = Break_new(env, lineno, YNIL);
}
#line 2135 "parser.c"
        break;
      case 11: /* stmt ::= BREAK expr */
#line 720 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy101 = Break_new(env, lineno, yymsp[0].minor.yy101);
}
#line 2143 "parser.c"
        break;
      case 12: /* stmt ::= NEXT */
#line 724 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy101 = Next_new(env, lineno, YNIL);
}
#line 2151 "parser.c"
        break;
      case 13: /* stmt ::= NEXT expr */
#line 728 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy101 = Next_new(env, lineno, yymsp[0].minor.yy101);
}
#line 2159 "parser.c"
        break;
      case 14: /* stmt ::= RETURN */
#line 732 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy101 = Return_new(env, lineno, YNIL);
}
#line 2167 "parser.c"
        break;
      case 15: /* stmt ::= RETURN expr */
#line 736 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy101 = Return_new(env, lineno, yymsp[0].minor.yy101);
}
#line 2175 "parser.c"
        break;
      case 16: /* stmt ::= IF expr NEWLINE stmts if_tail END */
#line 740 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    yygotominor.yy101 = If_new(env, lineno, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, yymsp[-1].minor.yy101);
}
#line 2183 "parser.c"
        break;
      case 17: /* stmt ::= CLASS NAME super_opt NEWLINE stmts END */
#line 744 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-4].minor.yy0)->u.id;
    yygotominor.yy101 = Klass_new(env, lineno, id, yymsp[-3].minor.yy101, yymsp[-1].minor.yy101);
}
#line 2192 "parser.c"
        break;
      case 18: /* stmt ::= NONLOCAL names */
#line 749 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy101 = Nonlocal_new(env, lineno, yymsp[0].minor.yy101);
}
#line 2200 "parser.c"
        break;
      case 19: /* stmt ::= IMPORT dotted_names */
#line 753 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    yygotominor.yy101 = Import_new(env, lineno, yymsp[0].minor.yy101);
}
#line 2208 "parser.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 24: /* names ::= NAME */
#line 765 "parser.y"
{
    yygotominor.yy101 = id_token2array(env, yymsp[0].minor.yy0);
}
#line 2216 "parser.c"
        break;
      case 23: /* dotted_name ::= dotted_name DOT NAME */
      case 25: /* names ::= names COMMA NAME */
#line 768 "parser.y"
{
    yygotominor.yy101 = Array_push_token_id(env, yymsp[-2].minor.yy101, yymsp[0].minor.yy0);
}
#line 2224 "parser.c"
        break;
      case 29: /* if_tail ::= ELIF expr NEWLINE stmts if_tail */
#line 789 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    YogVal node = If_new(env, lineno, yymsp[-3].minor.yy101, yymsp[-1].minor.yy101, yymsp[0].minor.yy101);
    yygotominor.yy101 = make_array_with(env, node);
}
#line 2233 "parser.c"
        break;
      case 32: /* func_def ::= DEF NAME LPAR params RPAR stmts END */
#line 802 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-6].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-5].minor.yy0)->u.id;
    yygotominor.yy101 = FuncDef_new(env, lineno, id, yymsp[-3].minor.yy101, yymsp[-1].minor.yy101);
}
#line 2242 "parser.c"
        break;
      case 33: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 808 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-8].minor.yy101, yymsp[-6].minor.yy101, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2249 "parser.c"
        break;
      case 34: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA var_param */
#line 811 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-6].minor.yy101, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, yymsp[0].minor.yy101, YNIL);
}
#line 2256 "parser.c"
        break;
      case 35: /* params ::= params_without_default COMMA params_with_default COMMA block_param COMMA kw_param */
#line 814 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-6].minor.yy101, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, YNIL, yymsp[0].minor.yy101);
}
#line 2263 "parser.c"
        break;
      case 36: /* params ::= params_without_default COMMA params_with_default COMMA block_param */
#line 817 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, yymsp[0].minor.yy101, YNIL, YNIL);
}
#line 2270 "parser.c"
        break;
      case 37: /* params ::= params_without_default COMMA params_with_default COMMA var_param COMMA kw_param */
#line 820 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-6].minor.yy101, yymsp[-4].minor.yy101, YNIL, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2277 "parser.c"
        break;
      case 38: /* params ::= params_without_default COMMA params_with_default COMMA var_param */
#line 823 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, YNIL, yymsp[0].minor.yy101, YNIL);
}
#line 2284 "parser.c"
        break;
      case 39: /* params ::= params_without_default COMMA params_with_default COMMA kw_param */
#line 826 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, YNIL, YNIL, yymsp[0].minor.yy101);
}
#line 2291 "parser.c"
        break;
      case 40: /* params ::= params_without_default COMMA params_with_default */
#line 829 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-2].minor.yy101, yymsp[0].minor.yy101, YNIL, YNIL, YNIL);
}
#line 2298 "parser.c"
        break;
      case 41: /* params ::= params_without_default COMMA block_param COMMA var_param COMMA kw_param */
#line 832 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-6].minor.yy101, YNIL, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2305 "parser.c"
        break;
      case 42: /* params ::= params_without_default COMMA block_param COMMA var_param */
#line 835 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-4].minor.yy101, YNIL, yymsp[-2].minor.yy101, yymsp[0].minor.yy101, YNIL);
}
#line 2312 "parser.c"
        break;
      case 43: /* params ::= params_without_default COMMA block_param COMMA kw_param */
#line 838 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-4].minor.yy101, YNIL, yymsp[-2].minor.yy101, YNIL, yymsp[0].minor.yy101);
}
#line 2319 "parser.c"
        break;
      case 44: /* params ::= params_without_default COMMA block_param */
#line 841 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-2].minor.yy101, YNIL, yymsp[0].minor.yy101, YNIL, YNIL);
}
#line 2326 "parser.c"
        break;
      case 45: /* params ::= params_without_default COMMA var_param COMMA kw_param */
#line 844 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-4].minor.yy101, YNIL, YNIL, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2333 "parser.c"
        break;
      case 46: /* params ::= params_without_default COMMA var_param */
#line 847 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-2].minor.yy101, YNIL, YNIL, yymsp[0].minor.yy101, YNIL);
}
#line 2340 "parser.c"
        break;
      case 47: /* params ::= params_without_default COMMA kw_param */
#line 850 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[-2].minor.yy101, YNIL, YNIL, YNIL, yymsp[0].minor.yy101);
}
#line 2347 "parser.c"
        break;
      case 48: /* params ::= params_without_default */
#line 853 "parser.y"
{
    yygotominor.yy101 = Params_new(env, yymsp[0].minor.yy101, YNIL, YNIL, YNIL, YNIL);
}
#line 2354 "parser.c"
        break;
      case 49: /* params ::= params_with_default COMMA block_param COMMA var_param COMMA kw_param */
#line 856 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, yymsp[-6].minor.yy101, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2361 "parser.c"
        break;
      case 50: /* params ::= params_with_default COMMA block_param COMMA var_param */
#line 859 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, yymsp[0].minor.yy101, YNIL);
}
#line 2368 "parser.c"
        break;
      case 51: /* params ::= params_with_default COMMA block_param COMMA kw_param */
#line 862 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, YNIL, yymsp[0].minor.yy101);
}
#line 2375 "parser.c"
        break;
      case 52: /* params ::= params_with_default COMMA block_param */
#line 865 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, yymsp[-2].minor.yy101, yymsp[0].minor.yy101, YNIL, YNIL);
}
#line 2382 "parser.c"
        break;
      case 53: /* params ::= params_with_default COMMA var_param COMMA kw_param */
#line 868 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, yymsp[-4].minor.yy101, YNIL, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2389 "parser.c"
        break;
      case 54: /* params ::= params_with_default COMMA var_param */
#line 871 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, yymsp[-2].minor.yy101, YNIL, yymsp[0].minor.yy101, YNIL);
}
#line 2396 "parser.c"
        break;
      case 55: /* params ::= params_with_default COMMA kw_param */
#line 874 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, yymsp[-2].minor.yy101, YNIL, YNIL, yymsp[0].minor.yy101);
}
#line 2403 "parser.c"
        break;
      case 56: /* params ::= params_with_default */
#line 877 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, yymsp[0].minor.yy101, YNIL, YNIL, YNIL);
}
#line 2410 "parser.c"
        break;
      case 57: /* params ::= block_param COMMA var_param COMMA kw_param */
#line 880 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, YNIL, yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2417 "parser.c"
        break;
      case 58: /* params ::= block_param COMMA var_param */
#line 883 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy101, yymsp[0].minor.yy101, YNIL);
}
#line 2424 "parser.c"
        break;
      case 59: /* params ::= block_param COMMA kw_param */
#line 886 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, YNIL, yymsp[-2].minor.yy101, YNIL, yymsp[0].minor.yy101);
}
#line 2431 "parser.c"
        break;
      case 60: /* params ::= block_param */
#line 889 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, YNIL, yymsp[0].minor.yy101, YNIL, YNIL);
}
#line 2438 "parser.c"
        break;
      case 61: /* params ::= var_param COMMA kw_param */
#line 892 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, YNIL, YNIL, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2445 "parser.c"
        break;
      case 62: /* params ::= var_param */
#line 895 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, YNIL, YNIL, yymsp[0].minor.yy101, YNIL);
}
#line 2452 "parser.c"
        break;
      case 63: /* params ::= kw_param */
#line 898 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, YNIL, YNIL, YNIL, yymsp[0].minor.yy101);
}
#line 2459 "parser.c"
        break;
      case 64: /* params ::= */
#line 901 "parser.y"
{
    yygotominor.yy101 = Params_new(env, YNIL, YNIL, YNIL, YNIL, YNIL);
}
#line 2466 "parser.c"
        break;
      case 65: /* kw_param ::= STAR_STAR NAME */
#line 905 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy101 = Param_new(env, NODE_KW_PARAM, lineno, id, YNIL);
}
#line 2475 "parser.c"
        break;
      case 66: /* var_param ::= STAR NAME */
#line 911 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy101 = Param_new(env, NODE_VAR_PARAM, lineno, id, YNIL);
}
#line 2484 "parser.c"
        break;
      case 67: /* block_param ::= AMPER NAME param_default_opt */
#line 917 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy101 = Param_new(env, NODE_BLOCK_PARAM, lineno, id, yymsp[0].minor.yy101);
}
#line 2493 "parser.c"
        break;
      case 71: /* params_without_default ::= NAME */
#line 934 "parser.y"
{
    yygotominor.yy101 = YogArray_new(env);
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yygotominor.yy101, lineno, id, YNIL);
}
#line 2503 "parser.c"
        break;
      case 72: /* params_without_default ::= params_without_default COMMA NAME */
#line 940 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    ParamArray_push(env, yymsp[-2].minor.yy101, lineno, id, YNIL);
    yygotominor.yy101 = yymsp[-2].minor.yy101;
}
#line 2513 "parser.c"
        break;
      case 75: /* param_with_default ::= NAME param_default */
#line 954 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-1].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy101 = Param_new(env, NODE_PARAM, lineno, id, yymsp[0].minor.yy101);
}
#line 2522 "parser.c"
        break;
      case 77: /* args ::= posargs */
#line 963 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy101, 0));
    yygotominor.yy101 = Args_new(env, lineno, yymsp[0].minor.yy101, YNIL);
}
#line 2530 "parser.c"
        break;
      case 78: /* args ::= posargs COMMA kwargs */
#line 967 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[-2].minor.yy101, 0));
    yygotominor.yy101 = Args_new(env, lineno, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2538 "parser.c"
        break;
      case 79: /* args ::= kwargs */
#line 971 "parser.y"
{
    uint_t lineno = NODE_LINENO(YogArray_at(env, yymsp[0].minor.yy101, 0));
    yygotominor.yy101 = Args_new(env, lineno, YNIL, yymsp[0].minor.yy101);
}
#line 2546 "parser.c"
        break;
      case 81: /* posargs ::= posargs COMMA expr */
      case 83: /* kwargs ::= kwargs COMMA kwarg */
      case 144: /* exprs ::= exprs COMMA expr */
      case 146: /* dict_elems ::= dict_elems COMMA dict_elem */
#line 979 "parser.y"
{
    YogArray_push(env, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
    yygotominor.yy101 = yymsp[-2].minor.yy101;
}
#line 2557 "parser.c"
        break;
      case 84: /* kwarg ::= NAME COLON expr */
#line 992 "parser.y"
{
    yygotominor.yy101 = YogNode_new(env, NODE_KW_ARG, TOKEN_LINENO(yymsp[-2].minor.yy0));
    PTR_AS(YogNode, yygotominor.yy101)->u.kwarg.name = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    PTR_AS(YogNode, yygotominor.yy101)->u.kwarg.value = yymsp[0].minor.yy101;
}
#line 2566 "parser.c"
        break;
      case 86: /* assign_expr ::= postfix_expr EQUAL logical_or_expr */
#line 1002 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy101);
    yygotominor.yy101 = Assign_new(env, lineno, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2574 "parser.c"
        break;
      case 89: /* logical_or_expr ::= logical_or_expr BAR_BAR logical_and_expr */
#line 1013 "parser.y"
{
    yygotominor.yy101 = YogNode_new(env, NODE_LOGICAL_OR, NODE_LINENO(yymsp[-2].minor.yy101));
    NODE(yygotominor.yy101)->u.logical_or.left = yymsp[-2].minor.yy101;
    NODE(yygotominor.yy101)->u.logical_or.right = yymsp[0].minor.yy101;
}
#line 2583 "parser.c"
        break;
      case 91: /* logical_and_expr ::= logical_and_expr AND_AND not_expr */
#line 1022 "parser.y"
{
    yygotominor.yy101 = YogNode_new(env, NODE_LOGICAL_AND, NODE_LINENO(yymsp[-2].minor.yy101));
    NODE(yygotominor.yy101)->u.logical_and.left = yymsp[-2].minor.yy101;
    NODE(yygotominor.yy101)->u.logical_and.right = yymsp[0].minor.yy101;
}
#line 2592 "parser.c"
        break;
      case 93: /* not_expr ::= NOT not_expr */
#line 1031 "parser.y"
{
    yygotominor.yy101 = YogNode_new(env, NODE_NOT, NODE_LINENO(yymsp[-1].minor.yy0));
    NODE(yygotominor.yy101)->u.not.expr = yymsp[0].minor.yy101;
}
#line 2600 "parser.c"
        break;
      case 95: /* comparison ::= xor_expr comp_op xor_expr */
#line 1039 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy101);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy101)->u.id;
    yygotominor.yy101 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy101, id, yymsp[0].minor.yy101);
}
#line 2609 "parser.c"
        break;
      case 96: /* comp_op ::= LESS */
      case 97: /* comp_op ::= GREATER */
      case 150: /* comma_opt ::= COMMA */
#line 1045 "parser.y"
{
    yygotominor.yy101 = yymsp[0].minor.yy0;
}
#line 2618 "parser.c"
        break;
      case 99: /* xor_expr ::= xor_expr XOR or_expr */
      case 101: /* or_expr ::= or_expr BAR and_expr */
      case 103: /* and_expr ::= and_expr AND shift_expr */
#line 1055 "parser.y"
{
    yygotominor.yy101 = FuncCall_new2(env, NODE_LINENO(yymsp[-2].minor.yy101), yymsp[-2].minor.yy101, TOKEN_ID(yymsp[-1].minor.yy0), yymsp[0].minor.yy101);
}
#line 2627 "parser.c"
        break;
      case 105: /* shift_expr ::= shift_expr shift_op match_expr */
      case 111: /* arith_expr ::= arith_expr arith_op term */
      case 114: /* term ::= term term_op factor */
#line 1076 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy101);
    yygotominor.yy101 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy101, VAL2ID(yymsp[-1].minor.yy101), yymsp[0].minor.yy101);
}
#line 2637 "parser.c"
        break;
      case 106: /* shift_op ::= LSHIFT */
      case 107: /* shift_op ::= RSHIFT */
      case 112: /* arith_op ::= PLUS */
      case 113: /* arith_op ::= MINUS */
      case 116: /* term_op ::= STAR */
      case 117: /* term_op ::= DIV */
      case 118: /* term_op ::= DIV_DIV */
      case 119: /* term_op ::= PERCENT */
#line 1081 "parser.y"
{
    yygotominor.yy101 = ID2VAL(PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id);
}
#line 2651 "parser.c"
        break;
      case 109: /* match_expr ::= match_expr EQUAL_TILDA arith_expr */
#line 1091 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy101);
    ID id = PTR_AS(YogToken, yymsp[-1].minor.yy0)->u.id;
    yygotominor.yy101 = FuncCall_new2(env, lineno, yymsp[-2].minor.yy101, id, yymsp[0].minor.yy101);
}
#line 2660 "parser.c"
        break;
      case 120: /* factor ::= PLUS factor */
#line 1133 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "+self");
    yygotominor.yy101 = FuncCall_new3(env, lineno, yymsp[0].minor.yy101, id);
}
#line 2669 "parser.c"
        break;
      case 121: /* factor ::= MINUS factor */
#line 1138 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "-self");
    yygotominor.yy101 = FuncCall_new3(env, lineno, yymsp[0].minor.yy101, id);
}
#line 2678 "parser.c"
        break;
      case 122: /* factor ::= TILDA factor */
#line 1143 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-1].minor.yy0);
    ID id = YogVM_intern(env, env->vm, "~self");
    yygotominor.yy101 = FuncCall_new3(env, lineno, yymsp[0].minor.yy101, id);
}
#line 2687 "parser.c"
        break;
      case 126: /* postfix_expr ::= postfix_expr LPAR args RPAR blockarg_opt */
#line 1159 "parser.y"
{
    yygotominor.yy101 = FuncCall_new(env, NODE_LINENO(yymsp[-4].minor.yy101), yymsp[-4].minor.yy101, yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2694 "parser.c"
        break;
      case 127: /* postfix_expr ::= postfix_expr LBRACKET expr RBRACKET */
#line 1162 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-3].minor.yy101);
    yygotominor.yy101 = Subscript_new(env, lineno, yymsp[-3].minor.yy101, yymsp[-1].minor.yy101);
}
#line 2702 "parser.c"
        break;
      case 128: /* postfix_expr ::= postfix_expr DOT NAME */
#line 1166 "parser.y"
{
    uint_t lineno = NODE_LINENO(yymsp[-2].minor.yy101);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy101 = Attr_new(env, lineno, yymsp[-2].minor.yy101, id);
}
#line 2711 "parser.c"
        break;
      case 129: /* atom ::= NAME */
#line 1172 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.id;
    yygotominor.yy101 = Variable_new(env, lineno, id);
}
#line 2720 "parser.c"
        break;
      case 130: /* atom ::= NUMBER */
      case 131: /* atom ::= REGEXP */
      case 132: /* atom ::= STRING */
      case 133: /* atom ::= SYMBOL */
#line 1177 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    YogVal val = PTR_AS(YogToken, yymsp[0].minor.yy0)->u.val;
    yygotominor.yy101 = Literal_new(env, lineno, val);
}
#line 2732 "parser.c"
        break;
      case 134: /* atom ::= NIL */
#line 1197 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy101 = Literal_new(env, lineno, YNIL);
}
#line 2740 "parser.c"
        break;
      case 135: /* atom ::= TRUE */
#line 1201 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy101 = Literal_new(env, lineno, YTRUE);
}
#line 2748 "parser.c"
        break;
      case 136: /* atom ::= FALSE */
#line 1205 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[0].minor.yy0);
    yygotominor.yy101 = Literal_new(env, lineno, YFALSE);
}
#line 2756 "parser.c"
        break;
      case 137: /* atom ::= LINE */
#line 1209 "parser.y"
{
    uint_t lineno = PTR_AS(YogToken, yymsp[0].minor.yy0)->lineno;
    YogVal val = INT2VAL(lineno);
    yygotominor.yy101 = Literal_new(env, lineno, val);
}
#line 2765 "parser.c"
        break;
      case 138: /* atom ::= LBRACKET exprs RBRACKET */
#line 1214 "parser.y"
{
    yygotominor.yy101 = Array_new(env, NODE_LINENO(yymsp[-2].minor.yy0), yymsp[-1].minor.yy101);
}
#line 2772 "parser.c"
        break;
      case 139: /* atom ::= LBRACKET RBRACKET */
#line 1217 "parser.y"
{
    yygotominor.yy101 = Array_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 2779 "parser.c"
        break;
      case 140: /* atom ::= LBRACE RBRACE */
#line 1220 "parser.y"
{
    yygotominor.yy101 = Dict_new(env, NODE_LINENO(yymsp[-1].minor.yy0), YNIL);
}
#line 2786 "parser.c"
        break;
      case 141: /* atom ::= LBRACE dict_elems comma_opt RBRACE */
#line 1223 "parser.y"
{
    yygotominor.yy101 = Dict_new(env, NODE_LINENO(yymsp[-3].minor.yy0), yymsp[-2].minor.yy101);
}
#line 2793 "parser.c"
        break;
      case 142: /* atom ::= LPAR expr RPAR */
      case 155: /* blockarg_params_opt ::= LBRACKET params RBRACKET */
#line 1226 "parser.y"
{
    yygotominor.yy101 = yymsp[-1].minor.yy101;
}
#line 2801 "parser.c"
        break;
      case 147: /* dict_elem ::= expr EQUAL_GREATER expr */
#line 1245 "parser.y"
{
    yygotominor.yy101 = DictElem_new(env, NODE_LINENO(yymsp[-2].minor.yy101), yymsp[-2].minor.yy101, yymsp[0].minor.yy101);
}
#line 2808 "parser.c"
        break;
      case 148: /* dict_elem ::= NAME COLON expr */
#line 1248 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YogVal var = Literal_new(env, lineno, ID2VAL(id));
    yygotominor.yy101 = DictElem_new(env, lineno, var, yymsp[0].minor.yy101);
}
#line 2818 "parser.c"
        break;
      case 152: /* blockarg_opt ::= DO blockarg_params_opt NEWLINE stmts END */
      case 153: /* blockarg_opt ::= LBRACE blockarg_params_opt NEWLINE stmts RBRACE */
#line 1265 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-4].minor.yy0);
    yygotominor.yy101 = BlockArg_new(env, lineno, yymsp[-3].minor.yy101, yymsp[-1].minor.yy101);
}
#line 2827 "parser.c"
        break;
      case 157: /* excepts ::= excepts except */
#line 1284 "parser.y"
{
    yygotominor.yy101 = Array_push(env, yymsp[-1].minor.yy101, yymsp[0].minor.yy101);
}
#line 2834 "parser.c"
        break;
      case 158: /* except ::= EXCEPT expr AS NAME NEWLINE stmts */
#line 1288 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-5].minor.yy0);
    ID id = PTR_AS(YogToken, yymsp[-2].minor.yy0)->u.id;
    YOG_ASSERT(env, id != NO_EXC_VAR, "Too many variables.");
    yygotominor.yy101 = ExceptBody_new(env, lineno, yymsp[-4].minor.yy101, id, yymsp[0].minor.yy101);
}
#line 2844 "parser.c"
        break;
      case 159: /* except ::= EXCEPT expr NEWLINE stmts */
#line 1294 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-3].minor.yy0);
    yygotominor.yy101 = ExceptBody_new(env, lineno, yymsp[-2].minor.yy101, NO_EXC_VAR, yymsp[0].minor.yy101);
}
#line 2852 "parser.c"
        break;
      case 160: /* except ::= EXCEPT NEWLINE stmts */
#line 1298 "parser.y"
{
    uint_t lineno = TOKEN_LINENO(yymsp[-2].minor.yy0);
    yygotominor.yy101 = ExceptBody_new(env, lineno, YNIL, NO_EXC_VAR, yymsp[0].minor.yy101);
}
#line 2860 "parser.c"
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
