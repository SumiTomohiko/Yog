#include "yog/config.h"
#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#include <limits.h>
#if defined(HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <stdarg.h>
#if defined(HAVE_STDINT_H)
#   include <stdint.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yog/arg.h"
#include "yog/array.h"
#include "yog/binary.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/inst.h"
#include "yog/misc.h"
#include "yog/opcodes.h"
#include "yog/parser.h"
#include "yog/string.h"
#include "yog/string.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"

typedef struct AstVisitor AstVisitor;

typedef void (*VisitNode)(YogEnv*, AstVisitor*, YogVal, YogVal);
typedef void (*VisitArray)(YogEnv*, AstVisitor*, YogVal, YogVal);

struct AstVisitor {
    VisitArray visit_stmts;
    VisitNode visit_array;
    VisitNode visit_assign;
    VisitNode visit_attr;
    VisitNode visit_block;
    VisitNode visit_break;
    VisitNode visit_dict;
    VisitNode visit_except;
    VisitNode visit_except_body;
    VisitNode visit_finally;
    VisitNode visit_func_call;
    VisitNode visit_func_def;
    VisitNode visit_if;
    VisitNode visit_import;
    VisitNode visit_class;
    VisitNode visit_literal;
    VisitNode visit_logical_and;
    VisitNode visit_logical_or;
    VisitNode visit_module;
    VisitNode visit_multi_assign;
    VisitNode visit_multi_assign_lhs;
    VisitNode visit_next;
    VisitNode visit_nonlocal;
    VisitNode visit_not;
    VisitNode visit_raise;
    VisitNode visit_return;
    VisitNode visit_set;
    VisitNode visit_stmt;
    VisitNode visit_subscript;
    VisitNode visit_super;
    VisitNode visit_variable;
    VisitNode visit_while;
};

struct ScanVarData {
    YogVal var_tbl;
};

typedef struct ScanVarData ScanVarData;

#define SCAN_VAR_DATA(v)    PTR_AS(ScanVarData, (v))

struct ScanVarEntry {
    uint_t index;
    int_t flags;
};

#define VAR_ASSIGNED        (0x01)
#define VAR_PARAM           (0x02)
#define VAR_NONLOCAL        (0x04)
#define VAR_USED            (0x08)
#define IS_ASSIGNED(flags)  ((flags) & VAR_ASSIGNED)
#define IS_PARAM(flags)     ((flags) & VAR_PARAM)
#define IS_NONLOCAL(flags)  ((flags) & VAR_NONLOCAL)
#define IS_USED(flags)      ((flags) & VAR_USED)

typedef struct ScanVarEntry ScanVarEntry;

#define SCAN_VAR_ENTRY(v)   PTR_AS(ScanVarEntry, (v))

enum VarType {
    VT_GLOBAL,
    VT_LOCAL,
    VT_NONLOCAL,
};

typedef enum VarType VarType;

struct Var {
    VarType type;
    union {
        struct {
            uint_t index;
        } local;
        struct {
            uint_t level;
            uint_t index;
        } nonlocal;
    } u;
};

typedef struct Var Var;

#define VAR(v)  PTR_AS(Var, (v))

enum Context {
    CTX_BLOCK,
    CTX_FUNC,
    CTX_CLASS,
    CTX_MODULE,
    CTX_PKG,
};

typedef enum Context Context;

struct FinallyListEntry {
    YogVal prev;
    YogVal node;
};

typedef struct FinallyListEntry FinallyListEntry;

#define FINALLY_LIST_ENTRY(v)   PTR_AS(FinallyListEntry, (v))

struct ExceptionTableEntry {
    YogVal next;

    YogVal from;
    YogVal to;
    YogVal target;
};

typedef struct ExceptionTableEntry ExceptionTableEntry;

#define EXCEPTION_TABLE_ENTRY(v)    PTR_AS(ExceptionTableEntry, (v))

struct TryListEntry {
    YogVal prev;
    YogVal node;
    YogVal exc_tbl;
};

typedef struct TryListEntry TryListEntry;

#define TRY_LIST_ENTRY(v)   PTR_AS(TryListEntry, (v))

struct CompileData {
    enum Context ctx;
    YogVal vars;
    YogVal const2index;
    YogVal last_inst;
    YogVal exc_tbl;
    YogVal exc_tbl_last;

    YogVal label_while_start;
    YogVal label_while_end;
    YogVal finally_list;
    YogVal try_list;
    YogVal label_last_ret;

    YogVal filename;
    ID class_name;

    YogVal outer;
    uint_t max_outer_depth;

    YogVal cur_stmt;
    BOOL interactive;
};

typedef struct CompileData CompileData;

#define COMPILE_DATA(v)     PTR_AS(CompileData, (v))

#define PUSH_EXCEPTION_TABLE_ENTRY(data, entry) do { \
    YogVal last = COMPILE_DATA((data))->exc_tbl_last; \
    EXCEPTION_TABLE_ENTRY(last)->next = (entry); \
    COMPILE_DATA(data)->exc_tbl_last = (entry); \
} while (0)

#define NODE(p)             PTR_AS(YogNode, (p))
#define NODE_LINENO(node)   NODE((node))->lineno

static void
raise_error(YogEnv* env, YogVal filename, uint_t lineno, const char* fmt, ...)
{
    SAVE_ARG(env, filename);

#define BUFFER_SIZE     4096
    char head[BUFFER_SIZE];
#if defined(_MSC_VER)
#   define snprintf     _snprintf
#endif
    snprintf(head, array_sizeof(head), "file \"%s\", line %u: ", PTR_AS(YogCharArray, filename)->items, lineno);

    char s[BUFFER_SIZE];
    va_list ap;
    va_start(ap, fmt);
#if defined(_MSC_VER)
#   define vsnprintf(s, size, fmt, ap)  vsprintf(s, fmt, ap)
#endif
    vsnprintf(s, array_sizeof(s), fmt, ap);
    va_end(ap);

    char msg[BUFFER_SIZE];
    snprintf(msg, array_sizeof(msg), "%s%s", head, s);
#undef BUFFER_SIZE

    YogError_raise_SyntaxError(env, msg);

    /* NOTREACHED */
    RETURN_VOID(env);
}

static void
YogInst_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogVal inst = PTR2VAL(ptr);

    YogGC_keep(env, &INST(inst)->next, keeper, heap);

    if (INST(inst)->type != INST_OP) {
        return;
    }

    YogVal* p;
    switch (INST_OPCODE(inst)) {
    case OP(JUMP):
        p = &JUMP_DEST(inst);
        break;
    case OP(JUMP_IF_TRUE):
        p = &JUMP_IF_TRUE_DEST(inst);
        break;
    case OP(JUMP_IF_FALSE):
        p = &JUMP_IF_FALSE_DEST(inst);
        break;
    case OP(JUMP_IF_DEFINED):
        p = &JUMP_IF_DEFINED_DEST(inst);
        break;
    default:
        return;
        break;
    }
    YogGC_keep(env, p, keeper, heap);
}

static YogVal
YogInst_new(YogEnv* env, InstType type, uint_t lineno)
{
    YogVal inst = ALLOC_OBJ(env, YogInst_keep_children, NULL, YogInst);
    PTR_AS(YogInst, inst)->type = type;
    PTR_AS(YogInst, inst)->next = YUNDEF;
    PTR_AS(YogInst, inst)->lineno = lineno;
    PTR_AS(YogInst, inst)->pc = 0;

    return inst;
}

static YogVal
Inst_new(YogEnv* env, uint_t lineno)
{
    return YogInst_new(env, INST_OP, lineno);
}

static YogVal
Label_new(YogEnv* env)
{
    return YogInst_new(env, INST_LABEL, 0);
}

static YogVal
Anchor_new(YogEnv* env)
{
    return YogInst_new(env, INST_ANCHOR, 0);
}

static void
add_inst(YogEnv* env, YogVal data, YogVal inst)
{
    INST(COMPILE_DATA(data)->last_inst)->next = inst;
    COMPILE_DATA(data)->last_inst = inst;
}

#include "compile.inc"

static void
TryListEntry_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    TryListEntry* entry = PTR_AS(TryListEntry, ptr);
#define KEEP(member)    YogGC_keep(env, &entry->member, keeper, heap)
    KEEP(prev);
    KEEP(node);
    KEEP(exc_tbl);
#undef KEEP
}

static YogVal
TryListEntry_new(YogEnv* env, YogVal node)
{
    SAVE_ARG(env, node);
    YogVal ent = YUNDEF;
    PUSH_LOCAL(env, ent);

    ent = ALLOC_OBJ(env, TryListEntry_keep_children, NULL, TryListEntry);
    PTR_AS(TryListEntry, ent)->prev = YUNDEF;
    PTR_AS(TryListEntry, ent)->node = node;
    PTR_AS(TryListEntry, ent)->exc_tbl = YUNDEF;

    RETURN(env, ent);
}

static YogVal
CompileData_push_try(YogEnv* env, YogVal data, YogVal node)
{
    SAVE_ARGS2(env, data, node);
    YogVal ent = YUNDEF;
    PUSH_LOCAL(env, ent);

    ent = TryListEntry_new(env, node);
    TRY_LIST_ENTRY(ent)->prev = COMPILE_DATA(data)->try_list;
    COMPILE_DATA(data)->try_list = ent;

    RETURN(env, ent);
}

static void
CompileData_pop_try(YogEnv* env, YogVal data)
{
    SAVE_ARG(env, data);
    YogVal ent = YUNDEF;
    PUSH_LOCAL(env, ent);

    ent = COMPILE_DATA(data)->try_list;
    COMPILE_DATA(data)->try_list = TRY_LIST_ENTRY(ent)->prev;

    RETURN_VOID(env);
}

static void
visit_node(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal arg)
{
    if (!IS_PTR(node)) {
        return;
    }

#define VISIT(f)    do { \
    if (visitor->visit_##f != NULL) { \
        (*visitor->visit_##f)(env, visitor, node, arg); \
    } \
} while (0)
    switch (NODE(node)->type) {
    case NODE_ARRAY:
        VISIT(array);
        break;
    case NODE_ASSIGN:
        VISIT(assign);
        break;
    case NODE_ATTR:
        VISIT(attr);
        break;
    case NODE_VARIABLE:
        VISIT(variable);
        break;
    case NODE_LITERAL:
        VISIT(literal);
        break;
    case NODE_FUNC_DEF:
        VISIT(func_def);
        break;
    case NODE_FUNC_CALL:
        VISIT(func_call);
        break;
    case NODE_FINALLY:
        VISIT(finally);
        break;
    case NODE_EXCEPT:
        VISIT(except);
        break;
    case NODE_EXCEPT_BODY:
        VISIT(except_body);
        break;
    case NODE_WHILE:
        VISIT(while);
        break;
    case NODE_IF:
        VISIT(if);
        break;
    case NODE_IMPORT:
        VISIT(import);
        break;
    case NODE_BREAK:
        VISIT(break);
        break;
    case NODE_LOGICAL_AND:
        VISIT(logical_and);
        break;
    case NODE_LOGICAL_OR:
        VISIT(logical_or);
        break;
    case NODE_NEXT:
        VISIT(next);
        break;
    case NODE_NONLOCAL:
        VISIT(nonlocal);
        break;
    case NODE_RETURN:
        VISIT(return);
        break;
    case NODE_BLOCK_ARG:
        VISIT(block);
        break;
    case NODE_CLASS:
        VISIT(class);
        break;
    case NODE_SUBSCRIPT:
        VISIT(subscript);
        break;
    case NODE_NOT:
        VISIT(not);
        break;
    case NODE_DICT:
        VISIT(dict);
        break;
    case NODE_SET:
        VISIT(set);
        break;
    case NODE_MODULE:
        VISIT(module);
        break;
    case NODE_MULTI_ASSIGN:
        VISIT(multi_assign);
        break;
    case NODE_MULTI_ASSIGN_LHS:
        VISIT(multi_assign_lhs);
        break;
    case NODE_RAISE:
        VISIT(raise);
        break;
    case NODE_SUPER:
        VISIT(super);
        break;
    default:
        YOG_BUG(env, "Unknown node type (0x%08x)", NODE(node)->type);
        break;
    }
#undef VISIT
}

static void
visit_each_dict_elem(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal elems = YUNDEF;
    YogVal elem = YUNDEF;
    PUSH_LOCALS2(env, elems, elem);

    elems = NODE(node)->u.dict.elems;
    if (!IS_PTR(elems)) {
        RETURN_VOID(env);
    }
    uint_t size = YogArray_size(env, elems);
    uint_t i;
    for (i = 0; i < size; i++) {
        elem = YogArray_at(env, elems, i);
        YOG_ASSERT(env, NODE(elem)->type == NODE_DICT_ELEM, "invalid type (0x%08x)", NODE(elem)->type);
        visit_node(env, visitor, NODE(elem)->u.dict_elem.key, data);
        visit_node(env, visitor, NODE(elem)->u.dict_elem.value, data);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_dict(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    visit_each_dict_elem(env, visitor, node, data);
}

static void
compile_visit_dict(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visit_each_dict_elem(env, visitor, node, data);

    uint_t lineno = NODE(node)->lineno;
    uint_t elems = NODE(node)->u.dict.elems;
    uint_t size;
    if (IS_PTR(elems)) {
        size = YogArray_size(env, NODE(node)->u.dict.elems);
    }
    else {
        size = 0;
    }
    CompileData_add_make_dict(env, data, lineno, size);

    RETURN_VOID(env);
}

static void
compile_visit_stmt(YogEnv* env, AstVisitor* visitor, YogVal stmt, YogVal data)
{
    SAVE_ARGS2(env, stmt, data);
    COMPILE_DATA(data)->cur_stmt = stmt;
    visit_node(env, visitor, stmt, data);
    RETURN_VOID(env);
}

static void
process_stack_top_interactive(YogEnv* env, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    switch (NODE(node)->type) {
    case NODE_ASSIGN:
    case NODE_ATTR:
    case NODE_LITERAL:
    case NODE_SUBSCRIPT:
    case NODE_VARIABLE:
        CompileData_add_print_top(env, data, NODE_LINENO(node));
        break;
    case NODE_FUNC_CALL:
        CompileData_add_print_top_multi_value(env, data, NODE_LINENO(node));
        break;
    default:
        break;
    }

    RETURN_VOID(env);
}

static void
process_stack_top_uninteractive(YogEnv* env, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    switch (NODE(node)->type) {
    case NODE_ASSIGN:
    case NODE_ATTR:
    case NODE_LITERAL:
    case NODE_SUBSCRIPT:
    case NODE_VARIABLE:
        CompileData_add_pop(env, data, NODE_LINENO(node));
        break;
    default:
        break;
    }

    RETURN_VOID(env);
}

static void
process_stack_top(YogEnv* env, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    if (COMPILE_DATA(data)->interactive) {
        process_stack_top_interactive(env, node, data);
    }
    else {
        process_stack_top_uninteractive(env, node, data);
    }

    RETURN_VOID(env);
}

static void
compile_visit_stmts(YogEnv* env, AstVisitor* visitor, YogVal stmts, YogVal data)
{
    if (!IS_PTR(stmts)) {
        return;
    }

    SAVE_ARGS2(env, stmts, data);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    uint_t size = YogArray_size(env, stmts);
    uint_t i;
    for (i = 0; i < size; i++) {
        node = YogArray_at(env, stmts, i);
        visitor->visit_stmt(env, visitor, node, data);
        process_stack_top(env, node, data);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_stmts(YogEnv* env, AstVisitor* visitor, YogVal stmts, YogVal data)
{
    if (!IS_PTR(stmts)) {
        return;
    }

    SAVE_ARGS2(env, stmts, data);

    uint_t size = YogArray_size(env, stmts);
    uint_t i;
    for (i = 0; i < size; i++) {
        YogVal node = YogArray_at(env, stmts, i);
        visitor->visit_stmt(env, visitor, node, data);
    }

    RETURN_VOID(env);
}

static YogVal
ScanVarEntry_new(YogEnv* env, uint_t index, int_t flags)
{
    YogVal ent = ALLOC_OBJ(env, NULL, NULL, ScanVarEntry);
    PTR_AS(ScanVarEntry, ent)->index = index;
    PTR_AS(ScanVarEntry, ent)->flags = flags;

    return ent;
}

static void
scan_var_register(YogEnv* env, YogVal var_tbl, ID var, int_t flags)
{
    SAVE_ARG(env, var_tbl);

    YogVal key = ID2VAL(var);
    YogVal val = YUNDEF;
    if (!YogTable_lookup(env, var_tbl, key, &val)) {
        int_t index = YogTable_size(env, var_tbl);
        YogVal ent = ScanVarEntry_new(env, index, flags);
        YogTable_add_direct(env, var_tbl, key, ent);
    }
    else {
        SCAN_VAR_ENTRY(val)->flags |= flags;
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_lhs(YogEnv* env, AstVisitor* visitor, YogVal lhs, YogVal data)
{
    SAVE_ARGS2(env, lhs, data);
    if (!IS_PTR(lhs)) {
        RETURN_VOID(env);
    }

    if (NODE(lhs)->type == NODE_VARIABLE) {
        ID id = NODE(lhs)->u.variable.id;
        scan_var_register(env, SCAN_VAR_DATA(data)->var_tbl, id, VAR_ASSIGNED);
    }
    else {
        visit_node(env, visitor, lhs, data);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_multi_lhs(YogEnv* env, AstVisitor* visitor, YogVal lhs, YogVal data)
{
    SAVE_ARGS2(env, lhs, data);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);
    if (!IS_PTR(lhs)) {
        RETURN_VOID(env);
    }

    uint_t size = YogArray_size(env, lhs);
    uint_t i;
    for (i = 0; i < size; i++) {
        node = YogArray_at(env, lhs, i);
        scan_var_visit_lhs(env, visitor, node, data);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_multi_assign_lhs(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal left = YUNDEF;
    YogVal middle = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS3(env, left, middle, right);

    left = NODE(node)->u.multi_assign_lhs.left;
    scan_var_visit_multi_lhs(env, visitor, left, data);

    middle = NODE(node)->u.multi_assign_lhs.middle;
    scan_var_visit_lhs(env, visitor, middle, data);

    right = NODE(node)->u.multi_assign_lhs.right;
    scan_var_visit_multi_lhs(env, visitor, right, data);

    RETURN_VOID(env);
}

static void
visit_array_elements_opposite_order(YogEnv* env, AstVisitor* visitor, YogVal elems, YogVal data)
{
    SAVE_ARGS2(env, elems, data);
    YOG_ASSERT(env, IS_PTR(elems), "invalid elems (0x%08x)", elems);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(elems) == TYPE_ARRAY, "invalid elems type (0x%08x)", BASIC_OBJ_TYPE(elems));

    uint_t size = YogArray_size(env, elems);
    YOG_ASSERT(env, size < 256, "max array size is 255");
    uint_t i;
    for (i = size; 0 < i; i--) {
        YogVal elem = YogArray_at(env, elems, i - 1);
        visit_node(env, visitor, elem, data);
    }

    RETURN_VOID(env);
}

static void
visit_array_elements(YogEnv* env, AstVisitor* visitor, YogVal elems, YogVal data)
{
    SAVE_ARGS2(env, elems, data);
    if (!IS_PTR(elems)) {
        RETURN_VOID(env);
    }
    YOG_ASSERT(env, BASIC_OBJ_TYPE(elems) == TYPE_ARRAY, "invalid elems type (0x%08x)", BASIC_OBJ_TYPE(elems));

    uint_t size = YogArray_size(env, elems);
    YOG_ASSERT(env, size < 256, "max array size is 255");
    uint_t i;
    for (i = 0; i < size; i++) {
        YogVal elem = YogArray_at(env, elems, i);
        visit_node(env, visitor, elem, data);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_multi_assign(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal lhs = YUNDEF;
    YogVal rhs = YUNDEF;
    PUSH_LOCALS2(env, lhs, rhs);

    lhs = NODE(node)->u.multi_assign.lhs;
    YOG_ASSERT(env, NODE(lhs)->type == NODE_MULTI_ASSIGN_LHS, "invalid lhs (0x%08x)", NODE(lhs)->type);
    visit_node(env, visitor, lhs, data);

    rhs = NODE(node)->u.multi_assign.rhs;
    visit_array_elements(env, visitor, rhs, data);

    RETURN_VOID(env);
}

static void
scan_var_visit_assign(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal left = YUNDEF;
    PUSH_LOCAL(env, left);

    scan_var_visit_lhs(env, visitor, NODE(node)->u.assign.left, data);
    visit_node(env, visitor, NODE(node)->u.assign.right, data);

    RETURN_VOID(env);
}

static void
visit_decorators(YogEnv* env, AstVisitor* visitor, YogVal decorators, YogVal data)
{
    SAVE_ARGS2(env, decorators, data);
    YogVal decorator = YUNDEF;
    PUSH_LOCAL(env, decorator);

    if (!IS_PTR(decorators)) {
        RETURN_VOID(env);
    }

    uint_t size = YogArray_size(env, decorators);
    uint_t i;
    for (i = 0; i < size; i++) {
        decorator = YogArray_at(env, decorators, i);
        visit_node(env, visitor, decorator, data);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_func_def(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visit_decorators(env, visitor, NODE(node)->u.funcdef.decorators, data);

    ID id = NODE(node)->u.funcdef.name;
    scan_var_register(env, SCAN_VAR_DATA(data)->var_tbl, id, VAR_ASSIGNED);

    RETURN_VOID(env);
}

static void
scan_var_visit_func_call(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal posargs = YUNDEF;
    YogVal kwargs = YUNDEF;
    YogVal callee = YUNDEF;
    YogVal args = YUNDEF;
    YogVal blockarg = YUNDEF;
    YogVal child_node = YUNDEF;
    YogVal vararg = YUNDEF;
    YogVal varkwarg = YUNDEF;
    PUSH_LOCALS8(env, posargs, kwargs, callee, args, blockarg, child_node, vararg, varkwarg);
    YogVal and_block = YUNDEF;
    PUSH_LOCAL(env, and_block);

    callee = NODE(node)->u.func_call.callee;
    visit_node(env, visitor, callee, data);

    args = NODE(node)->u.func_call.args;
    if (IS_PTR(args)) {
        YOG_ASSERT(env, NODE(args)->type == NODE_ARGS, "invalid type (0x%02x)", NODE(args)->type);
        posargs = NODE(args)->u.args.posargs;
        if (IS_PTR(posargs)) {
            uint_t argc = YogArray_size(env, posargs);
            uint_t i;
            for (i = 0; i < argc; i++) {
                child_node = YogArray_at(env, posargs, i);
                visit_node(env, visitor, child_node, data);
            }
        }

        kwargs = PTR_AS(YogNode, args)->u.args.kwargs;
        if (IS_PTR(kwargs)) {
            uint_t argc = YogArray_size(env, kwargs);
            uint_t i;
            for (i = 0; i < argc; i++) {
                child_node = YogArray_at(env, kwargs, i);
                YOG_ASSERT(env, NODE(child_node)->type == NODE_KW_ARG, "invalid node (0x%02x)", NODE(node)->type);
                visit_node(env, visitor, NODE(child_node)->u.kwarg.value, data);
            }
        }

        vararg = PTR_AS(YogNode, args)->u.args.vararg;
        if (IS_PTR(vararg)) {
            visit_node(env, visitor, vararg, data);
        }

        varkwarg = PTR_AS(YogNode, args)->u.args.varkwarg;
        if (IS_PTR(varkwarg)) {
            visit_node(env, visitor, varkwarg, data);
        }

        and_block = PTR_AS(YogNode, args)->u.args.block;
        if (IS_PTR(and_block)) {
            visit_node(env, visitor, and_block, data);
        }
    }

    blockarg = NODE(node)->u.func_call.blockarg;
    if (IS_PTR(blockarg)) {
        visit_node(env, visitor, blockarg, data);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_finally(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visitor->visit_stmts(env, visitor, NODE(node)->u.finally.head, data);
    visitor->visit_stmts(env, visitor, NODE(node)->u.finally.body, data);

    RETURN_VOID(env);
}

static void
scan_var_visit_except_body(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal types = YUNDEF;
    PUSH_LOCAL(env, types);

    types = NODE(node)->u.except_body.types;
    if (IS_PTR(types)) {
        uint_t size = YogArray_size(env, types);
        uint_t i;
        for (i = 0; i < size; i++) {
            visit_node(env, visitor, YogArray_at(env, types, i), data);
        }
    }

    ID id = NODE(node)->u.except_body.var;
    if (id != NO_EXC_VAR) {
        scan_var_register(env, SCAN_VAR_DATA(data)->var_tbl, id, VAR_ASSIGNED);
    }
    visitor->visit_stmts(env, visitor, NODE(node)->u.except_body.stmts, data);

    RETURN_VOID(env);
}

static void
scan_var_visit_while(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visit_node(env, visitor, NODE(node)->u.while_.test, data);
    visitor->visit_stmts(env, visitor, NODE(node)->u.while_.stmts, data);

    RETURN_VOID(env);
}

static void
scan_var_visit_if(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visit_node(env, visitor, NODE(node)->u.if_.test, data);
    visitor->visit_stmts(env, visitor, NODE(node)->u.if_.stmts, data);
    visitor->visit_stmts(env, visitor, NODE(node)->u.if_.tail, data);

    RETURN_VOID(env);
}

static void
scan_var_visit_import(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal names = YUNDEF;
    PUSH_LOCAL(env, names);

    names = NODE(node)->u.import.names;
    uint_t size = YogArray_size(env, names);
    uint_t i;
    for (i = 0; i < size; i++) {
        YogVal pkg_name = YogArray_at(env, names, i);
        YogVal id = YogArray_at(env, pkg_name, 0);
        scan_var_register(env, SCAN_VAR_DATA(data)->var_tbl, VAL2ID(id), VAR_ASSIGNED);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_break(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    visit_array_elements(env, visitor, NODE(node)->u.break_.exprs, data);
    RETURN_VOID(env);
}

static void
scan_var_visit_return(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    visit_array_elements(env, visitor, NODE(node)->u.return_.exprs, data);
    RETURN_VOID(env);
}

static void
scan_var_visit_except(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visitor->visit_stmts(env, visitor, NODE(node)->u.except.head, data);

    YogVal excepts = NODE(node)->u.except.excepts;
    PUSH_LOCAL(env, excepts);
    uint_t size = YogArray_size(env, excepts);
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        YogVal node = YogArray_at(env, excepts, i);
        visitor->visit_except_body(env, visitor, node, data);
    }

    visitor->visit_stmts(env, visitor, NODE(node)->u.except.else_, data);

    RETURN_VOID(env);
}

static void
scan_var_visit_module(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    ID name = NODE(node)->u.module.name;
    scan_var_register(env, SCAN_VAR_DATA(data)->var_tbl, name, VAR_ASSIGNED);

    RETURN_VOID(env);
}

static void
scan_var_visit_class(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visit_decorators(env, visitor, NODE(node)->u.klass.decorators, data);

    ID name = NODE(node)->u.klass.name;
    scan_var_register(env, SCAN_VAR_DATA(data)->var_tbl, name, VAR_ASSIGNED);

    YogVal super = NODE(node)->u.klass.super;
    visit_node(env, visitor, super, data);

    RETURN_VOID(env);
}

static void
scan_var_visit_subscript(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visit_node(env, visitor, NODE(node)->u.subscript.prefix, data);
    visit_node(env, visitor, NODE(node)->u.subscript.index, data);

    RETURN_VOID(env);
}

static void
scan_var_visit_nonlocal(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal names = NODE(node)->u.nonlocal.names;
    PUSH_LOCAL(env, names);
    uint_t size = YogArray_size(env, names);

    uint_t i = 0;
    for (i = 0; i < size; i++) {
        YogVal val = YogArray_at(env, names, i);
        ID name = VAL2ID(val);
        scan_var_register(env,  SCAN_VAR_DATA(data)->var_tbl, name, VAR_NONLOCAL);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_variable(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    ID id = NODE(node)->u.variable.id;
    scan_var_register(env, SCAN_VAR_DATA(data)->var_tbl, id, VAR_USED);
}

static void
scan_var_visit_attr(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    visit_node(env, visitor, NODE(node)->u.attr.obj, data);
}

static void
scan_var_visit_array(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    visit_array_elements(env, visitor, NODE(node)->u.array.elems, data);
    RETURN_VOID(env);
}

static void
scan_var_visit_logical_and(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    visit_node(env, visitor, NODE(node)->u.logical_and.left, data);
    visit_node(env, visitor, NODE(node)->u.logical_and.right, data);
    RETURN_VOID(env);
}

static void
scan_var_visit_logical_or(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    visit_node(env, visitor, NODE(node)->u.logical_or.left, data);
    visit_node(env, visitor, NODE(node)->u.logical_or.right, data);
    RETURN_VOID(env);
}

static void
scan_var_visit_not(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    visit_node(env, visitor, NODE(node)->u.not.expr, data);
}

static void
visit_each_set_elem(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal elems = YUNDEF;
    YogVal elem = YUNDEF;
    PUSH_LOCALS2(env, elems, elem);

    elems = NODE(node)->u.set.elems;
    uint_t size = YogArray_size(env, elems);
    uint_t i;
    for (i = 0; i < size; i++) {
        elem = YogArray_at(env, elems, i);
        visit_node(env, visitor, elem, data);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_set(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    visit_each_set_elem(env, visitor, node, data);
    RETURN_VOID(env);
}

static void
scan_var_visit_raise(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    visit_node(env, visitor, NODE(node)->u.raise.expr, data);
}

static void
scan_var_init_visitor(AstVisitor* visitor)
{
    visitor->visit_array = scan_var_visit_array;
    visitor->visit_assign = scan_var_visit_assign;
    visitor->visit_attr = scan_var_visit_attr;
    visitor->visit_block = NULL;
    visitor->visit_break = scan_var_visit_break;
    visitor->visit_dict = scan_var_visit_dict;
    visitor->visit_except = scan_var_visit_except;
    visitor->visit_except_body = scan_var_visit_except_body;
    visitor->visit_finally = scan_var_visit_finally;
    visitor->visit_func_call = scan_var_visit_func_call;
    visitor->visit_func_def = scan_var_visit_func_def;
    visitor->visit_if = scan_var_visit_if;
    visitor->visit_import = scan_var_visit_import;
    visitor->visit_class = scan_var_visit_class;
    visitor->visit_literal = NULL;
    visitor->visit_logical_and = scan_var_visit_logical_and;
    visitor->visit_logical_or = scan_var_visit_logical_or;
    visitor->visit_module = scan_var_visit_module;
    visitor->visit_multi_assign = scan_var_visit_multi_assign;
    visitor->visit_multi_assign_lhs = scan_var_visit_multi_assign_lhs;
    visitor->visit_next = scan_var_visit_break;
    visitor->visit_nonlocal = scan_var_visit_nonlocal;
    visitor->visit_not = scan_var_visit_not;
    visitor->visit_raise = scan_var_visit_raise;
    visitor->visit_return = scan_var_visit_return;
    visitor->visit_set = scan_var_visit_set;
    visitor->visit_stmt = visit_node;
    visitor->visit_stmts = scan_var_visit_stmts;
    visitor->visit_subscript = scan_var_visit_subscript;
    visitor->visit_super = NULL;
    visitor->visit_variable = scan_var_visit_variable;
    visitor->visit_while = scan_var_visit_while;
}

static void
ScanVarData_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    ScanVarData* data = PTR_AS(ScanVarData, ptr);
    YogGC_keep(env, &data->var_tbl, keeper, heap);
}

static YogVal
ScanVarData_new(YogEnv* env)
{
    YogVal data = ALLOC_OBJ(env, ScanVarData_keep_children, NULL, ScanVarData);
    PTR_AS(ScanVarData, data)->var_tbl = YUNDEF;

    return data;
}

static void
scan_defaults(YogEnv* env, AstVisitor* visitor, YogVal params, YogVal data)
{
    if (!IS_PTR(params)) {
        return;
    }
    SAVE_ARGS2(env, params, data);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    uint_t size = YogArray_size(env, params);
    uint_t i;
    for (i = 0; i < size; i++) {
        node = YogArray_at(env, params, i);
        visit_node(env, visitor, NODE(node)->u.param.default_, data);
    }

    RETURN_VOID(env);
}

static YogVal
make_var_table(YogEnv* env, YogVal params, YogVal stmts, YogVal var_tbl)
{
    SAVE_ARGS3(env, params, stmts, var_tbl);
    YogVal data = YUNDEF;
    PUSH_LOCAL(env, data);

    AstVisitor visitor;
    scan_var_init_visitor(&visitor);

    if (!IS_PTR(var_tbl)) {
        var_tbl = YogTable_new_symbol_table(env);
    }
    data = ScanVarData_new(env);
    SCAN_VAR_DATA(data)->var_tbl = var_tbl;

    scan_defaults(env, &visitor, params, data);
    visitor.visit_stmts(env, &visitor, stmts, data);

    RETURN(env, var_tbl);
}

static YogVal
lookup_var(YogEnv* env, YogVal vars, ID name)
{
    YogVal key = ID2VAL(name);
    YogVal val = YUNDEF;
    if (YogTable_lookup(env, vars, key, &val)) {
        return val;
    }
    else {
        return YNIL;
    }
}

static void
append_store(YogEnv* env, YogVal data, uint_t lineno, ID name)
{
    switch (COMPILE_DATA(data)->ctx) {
    case CTX_BLOCK:
    case CTX_FUNC:
        {
            YogVal var = lookup_var(env, COMPILE_DATA(data)->vars, name);
            if (!IS_PTR(var)) {
                YogVal s = YogVM_id2name(env, env->vm, name);
                YOG_BUG(env, "variable not found (%s)", STRING_CSTR(s));
            }
            switch (VAR(var)->type) {
            case VT_GLOBAL:
                CompileData_add_store_global(env, data, lineno, name);
                break;
            case VT_LOCAL:
                {
                    uint_t index = VAR(var)->u.local.index;
                    CompileData_add_store_local(env, data, lineno, index);
                }
                break;
            case VT_NONLOCAL:
                {
                    uint_t level = VAR(var)->u.nonlocal.level;
                    uint_t index = VAR(var)->u.nonlocal.index;
                    CompileData_add_store_nonlocal(env, data, lineno, level, index);
                }
                break;
            default:
                YOG_BUG(env, "unknown VarType (%d)", VAR(var)->type);
                break;
            }
            break;
        }
    case CTX_CLASS:
    case CTX_MODULE:
    case CTX_PKG:
        CompileData_add_store_name(env, data, lineno, name);
        break;
    default:
        YOG_ASSERT(env, FALSE, "Unkown context.");
        break;
    }
}

static void
compile_assign(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    uint_t lineno = NODE(node)->lineno;
    switch (NODE(node)->type) {
    case NODE_VARIABLE:
        {
            ID name = NODE(node)->u.variable.id;
            append_store(env, data, lineno, name);
        }
        break;
    case NODE_SUBSCRIPT:
        {
            visit_node(env, visitor, NODE(node)->u.subscript.index, data);
            visit_node(env, visitor, NODE(node)->u.subscript.prefix, data);
            ID attr = YogVM_intern(env, env->vm, "[]=");
            CompileData_add_load_attr(env, data, lineno, attr);

            CompileData_add_call_function(env, data, lineno, 2, 0, 0, 0, 0, 1, 0, 0);
        }
        break;
    case NODE_ATTR:
        {
            visit_node(env, visitor, NODE(node)->u.attr.obj, data);
            ID name = NODE(node)->u.attr.name;
            CompileData_add_store_attr(env, data, lineno, name);
        }
        break;
    default:
        YOG_BUG(env, "invalid node type (0x%08x)", NODE(node)->type);
        break;
    }

    RETURN_VOID(env);
}

static void
compile_visit_assign(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal left = YUNDEF;
    PUSH_LOCAL(env, left);

    uint_t lineno = NODE(node)->lineno;
    visit_node(env, visitor, NODE(node)->u.assign.right, data);
    left = NODE(node)->u.assign.left;
    switch (NODE(left)->type) {
    case NODE_ATTR:
    case NODE_VARIABLE:
        CompileData_add_dup(env, data, lineno);
        break;
    default:
        break;
    }

    compile_assign(env, visitor, left, data);

    RETURN_VOID(env);
}

static void
compile_multi_assign(YogEnv* env, AstVisitor* visitor, YogVal left, YogVal middle, YogVal right, YogVal data)
{
    SAVE_ARGS4(env, left, middle, right, data);

    uint_t right_num = IS_PTR(right) ? YogArray_size(env, right) : 0;
    uint_t i;
    for (i = right_num; 0 < i; i--) {
        compile_assign(env, visitor, YogArray_at(env, right, i - 1), data);
    }
    if (IS_PTR(middle)) {
        compile_assign(env, visitor, middle, data);
    }
    uint_t left_num = IS_PTR(left) ? YogArray_size(env, left) : 0;
    for (i = left_num; 0 < i; i--) {
        compile_assign(env, visitor, YogArray_at(env, left, i - 1), data);
    }

    RETURN_VOID(env);
}

static int_t
register_const(YogEnv* env, YogVal data, YogVal const_)
{
    SAVE_ARGS2(env, data, const_);
    YogVal const2index = YUNDEF;
    PUSH_LOCAL(env, const2index);

    const2index = COMPILE_DATA(data)->const2index;

    if (!IS_PTR(const2index)) {
        const2index = YogTable_new_val_table(env);
        COMPILE_DATA(data)->const2index = const2index;
    }

    YogVal index = YUNDEF;
    int_t retval = 0;
    if (!YogTable_lookup(env, const2index, const_, &index)) {
        int_t size = YogTable_size(env, const2index);
        index = INT2VAL(size);
        YogTable_add_direct(env, const2index, const_, index);
        retval = size;
    }
    else {
        retval = VAL2INT(index);
    }

    RETURN(env, retval);
}

static void
add_push_const(YogEnv* env, YogVal data, YogVal val, uint_t lineno)
{
    SAVE_ARGS2(env, data, val);

    uint_t index = register_const(env, data, val);
    CompileData_add_push_const(env, data, lineno, index);

    RETURN_VOID(env);
}

static void
ExceptionTableEntry_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    ExceptionTableEntry* entry = PTR_AS(ExceptionTableEntry, ptr);
#define KEEP(member)    YogGC_keep(env, &entry->member, keeper, heap)
    KEEP(next);
    KEEP(from);
    KEEP(to);
    KEEP(target);
#undef KEEP
}

static YogVal
ExceptionTableEntry_new(YogEnv* env)
{
    YogVal entry = ALLOC_OBJ(env, ExceptionTableEntry_keep_children, NULL, ExceptionTableEntry);
    PTR_AS(ExceptionTableEntry, entry)->next = YUNDEF;
    PTR_AS(ExceptionTableEntry, entry)->from = YUNDEF;
    PTR_AS(ExceptionTableEntry, entry)->to = YUNDEF;
    PTR_AS(ExceptionTableEntry, entry)->target = YUNDEF;

    return entry;
}

static void
compile_call_func(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data, uint_t lhs_left_num, uint_t lhs_middle_num, uint_t lhs_right_num)
{
    SAVE_ARGS2(env, node, data);
    YogVal posargs = YUNDEF;
    YogVal kwargs = YUNDEF;
    YogVal args = YUNDEF;
    YogVal blockarg = YUNDEF;
    YogVal child_node = YUNDEF;
    YogVal vararg = YUNDEF;
    YogVal varkwarg = YUNDEF;
    YogVal label_break_start = YUNDEF;
    PUSH_LOCALS8(env, posargs, kwargs, args, blockarg, child_node, vararg, varkwarg, label_break_start);
    YogVal label_break_end = YUNDEF;
    YogVal exc_tbl_entry = YUNDEF;
    YogVal and_block = YUNDEF;
    PUSH_LOCALS3(env, label_break_end, exc_tbl_entry, and_block);

    blockarg = NODE(node)->u.func_call.blockarg;
    if (IS_PTR(blockarg)) {
        visit_node(env, visitor, blockarg, data);
    }

    uint_t posargc = 0;
    uint_t kwargc = 0;
    args = NODE(node)->u.func_call.args;
    if (IS_PTR(args)) {
        and_block = PTR_AS(YogNode, args)->u.args.block;
        if (IS_PTR(and_block)) {
            if (IS_PTR(blockarg)) {
                YogError_raise_SyntaxError(env, "block argument repeated");
            }
            visit_node(env, visitor, and_block, data);
        }

        varkwarg = PTR_AS(YogNode, args)->u.args.varkwarg;
        if (IS_PTR(varkwarg)) {
            visit_node(env, visitor, varkwarg, data);
        }

        vararg = PTR_AS(YogNode, args)->u.args.vararg;
        if (IS_PTR(vararg)) {
            visit_node(env, visitor, vararg, data);
        }

        kwargs = PTR_AS(YogNode, args)->u.args.kwargs;
        if (IS_PTR(kwargs)) {
            kwargc = YogArray_size(env, kwargs);
            uint_t i;
            for (i = 0; i < kwargc; i++) {
                child_node = YogArray_at(env, kwargs, kwargc - i - 1);
                YOG_ASSERT(env, NODE(child_node)->type == NODE_KW_ARG, "invalid node (0x%02x)", NODE(node)->type);
                visit_node(env, visitor, NODE(child_node)->u.kwarg.value, data);

                ID name = NODE(child_node)->u.kwarg.name;
                uint_t lineno = NODE(child_node)->lineno;
                add_push_const(env, data, ID2VAL(name), lineno);
            }
        }

        posargs = PTR_AS(YogNode, args)->u.args.posargs;
        if (IS_PTR(posargs)) {
            posargc = YogArray_size(env, posargs);
            uint_t i;
            for (i = 0; i < posargc; i++) {
                child_node = YogArray_at(env, posargs, posargc - i - 1);
                visit_node(env, visitor, child_node, data);
            }
        }
    }

    visit_node(env, visitor, NODE(node)->u.func_call.callee, data);

    uint8_t varargc;
    if (IS_PTR(vararg)) {
        varargc = 1;
    }
    else {
        varargc = 0;
    }

    uint8_t varkwargc;
    if (IS_PTR(varkwarg)) {
        varkwargc = 1;
    }
    else {
        varkwargc = 0;
    }

    uint8_t blockargc = 0;
    if (IS_PTR(blockarg) || IS_PTR(and_block)) {
        blockargc = 1;
    }

    label_break_start = Label_new(env);
    label_break_end = Label_new(env);
    add_inst(env, data, label_break_start);
    CompileData_add_call_function(env, data, NODE(node)->lineno, posargc, kwargc, varargc, varkwargc, blockargc, lhs_left_num, lhs_middle_num, lhs_right_num);
    add_inst(env, data, label_break_end);

    RETURN_VOID(env);
}

static void
compile_visit_multi_assign(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal lhs = YUNDEF;
    YogVal left = YUNDEF;
    YogVal right = YUNDEF;
    YogVal rhs = YUNDEF;
    YogVal elem = YUNDEF;
    YogVal middle = YUNDEF;
    YogVal head = YUNDEF;
    PUSH_LOCALS7(env, lhs, left, right, rhs, elem, middle, head);

    lhs = NODE(node)->u.multi_assign.lhs;
    left = NODE(lhs)->u.multi_assign_lhs.left;
    uint_t left_num = IS_PTR(left) ? YogArray_size(env, left) : 0;
    right = NODE(lhs)->u.multi_assign_lhs.right;
    uint_t right_num = IS_PTR(right) ? YogArray_size(env, right) : 0;
    rhs = NODE(node)->u.multi_assign.rhs;
    uint_t rhs_num = YogArray_size(env, rhs);
    YOG_ASSERT(env, 0 < rhs_num, "no elements in right hand size");
    head = YogArray_at(env, rhs, 0);
    middle = NODE(lhs)->u.multi_assign_lhs.middle;
    if ((rhs_num == 1) && (NODE(head)->type == NODE_FUNC_CALL)) {
        compile_call_func(env, visitor, head, data, left_num, IS_PTR(middle) ? 1 : 0, right_num);
        compile_multi_assign(env, visitor, left, middle, right, data);
        RETURN_VOID(env);
    }

    if (rhs_num < left_num + right_num) {
        YogError_raise_SyntaxError(env, "too many values to assign");
    }
    uint_t n = rhs_num - right_num;
    uint_t i;
    for (i = 0; i < n; i++) {
        visit_node(env, visitor, YogArray_at(env, rhs, i), data);
    }
    if (IS_PTR(middle)) {
        uint_t lineno = NODE_LINENO(middle);
        uint_t middle_num = n - left_num;
        CompileData_add_make_array(env, data, lineno, middle_num);
    }
    for (i = n; i < rhs_num; i++) {
        visit_node(env, visitor, YogArray_at(env, rhs, i), data);
    }

    compile_multi_assign(env, visitor, left, middle, right, data);

    RETURN_VOID(env);
}

static void
compile_visit_logical_or(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal label = YUNDEF;
    PUSH_LOCAL(env, label);

    visit_node(env, visitor, NODE(node)->u.logical_or.left, data);

    label = Label_new(env);

    uint_t lineno = NODE(node)->lineno;
    CompileData_add_dup(env, data, lineno);
    CompileData_add_jump_if_true(env, data, lineno, label);

    CompileData_add_pop(env, data, lineno);
    visit_node(env, visitor, NODE(node)->u.logical_or.right, data);

    add_inst(env, data, label);

    RETURN_VOID(env);
}

static void
compile_visit_logical_and(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal label = YUNDEF;
    PUSH_LOCAL(env, label);

    visit_node(env, visitor, NODE(node)->u.logical_and.left, data);

    label = Label_new(env);

    uint_t lineno = NODE(node)->lineno;
    CompileData_add_dup(env, data, lineno);
    CompileData_add_jump_if_false(env, data, lineno, label);

    CompileData_add_pop(env, data, lineno);
    visit_node(env, visitor, NODE(node)->u.logical_and.right, data);

    add_inst(env, data, label);

    RETURN_VOID(env);
}

static void
compile_visit_literal(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    val = NODE(node)->u.literal.val;
    uint_t lineno = NODE(node)->lineno;
    if (VAL2PTR(YogVal_get_class(env, val)) == VAL2PTR(env->vm->cString)) {
        uint_t index = register_const(env, data, val);
        CompileData_add_make_string(env, data, lineno, index);
    }
    else {
        add_push_const(env, data, val, lineno);
    }

    RETURN_VOID(env);
}

static int_t
table2array_count_index(YogEnv* env, YogVal key, YogVal value, YogVal* arg)
{
    if (VAL2INT(*arg) < VAL2INT(value)) {
        *arg = value;
    }

    return ST_CONTINUE;
}

static int_t
table2array_fill_array(YogEnv* env, YogVal key, YogVal value, YogVal* arg)
{
    int_t index = VAL2INT(value);
    PTR_AS(YogValArray, *arg)->items[index] = key;

    return ST_CONTINUE;
}

static YogVal
table2array(YogEnv* env, YogVal table)
{
    if (!IS_PTR(table)) {
        return YNIL;
    }

    SAVE_ARG(env, table);

    YogVal max_index = INT2VAL(INT_MIN);
    YogTable_foreach(env, table, table2array_count_index, &max_index);
    int_t index = VAL2INT(max_index);
    YogVal retval = YNIL;
    if (0 <= index) {
        uint_t size = index + 1;
        YogVal arg = YogValArray_new(env, size);
        YogTable_foreach(env, table, table2array_fill_array, &arg);
        retval = arg;
    }

    RETURN(env, retval);
}

static void
make_exception_table(YogEnv* env, YogVal code, YogVal data)
{
    SAVE_ARGS2(env, code, data);

    uint_t size = 0;
    YogVal entry = COMPILE_DATA(data)->exc_tbl;
    while (IS_PTR(entry)) {
        if (IS_PTR(EXCEPTION_TABLE_ENTRY(entry)->from)) {
            pc_t from = INST(EXCEPTION_TABLE_ENTRY(entry)->from)->pc;
            pc_t to = INST(EXCEPTION_TABLE_ENTRY(entry)->to)->pc;
            if (from != to) {
                size++;
            }
        }

        entry = EXCEPTION_TABLE_ENTRY(entry)->next;
    }

    if (0 < size) {
        YogVal exc_tbl = ALLOC_OBJ_ITEM(env, NULL, NULL, YogExceptionTable, size, YogExceptionTableEntry);

        uint_t i = 0;
        entry = COMPILE_DATA(data)->exc_tbl;
        while (IS_PTR(entry)) {
            if (IS_PTR(EXCEPTION_TABLE_ENTRY(entry)->from)) {
                pc_t from = INST(EXCEPTION_TABLE_ENTRY(entry)->from)->pc;
                pc_t to = INST(EXCEPTION_TABLE_ENTRY(entry)->to)->pc;
                if (from != to) {
                    YogExceptionTableEntry* ent = &PTR_AS(YogExceptionTable, exc_tbl)->items[i];
                    ent->from = from;
                    ent->to = to;
                    ent->target = INST(EXCEPTION_TABLE_ENTRY(entry)->target)->pc;
                    i++;
                }
            }

            entry = EXCEPTION_TABLE_ENTRY(entry)->next;
        }

        CODE(code)->exc_tbl = exc_tbl;
        CODE(code)->exc_tbl_size = size;
    }
    else {
        CODE(code)->exc_tbl = YNIL;
        CODE(code)->exc_tbl_size = 0;
    }

    RETURN_VOID(env);
}

static void
make_lineno_table(YogEnv* env, YogVal code, YogVal anchor)
{
    SAVE_ARGS2(env, code, anchor);

    YogVal inst = anchor;
    uint_t lineno = 0;
    uint_t size = 0;
    while (IS_PTR(inst)) {
        if (INST(inst)->type == INST_OP) {
            if (lineno != INST(inst)->lineno) {
                lineno = INST(inst)->lineno;
                size++;
            }
        }

        inst = INST(inst)->next;
    }

    YogVal tbl = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(YogLinenoTableEntry) * size);
    if (0 < size) {
        inst = anchor;
        int_t i = -1;
        lineno = 0;
        while (IS_PTR(inst)) {
            if (INST(inst)->type == INST_OP) {
                if (lineno != INST(inst)->lineno) {
                    i++;
                    YogLinenoTableEntry* entry = &PTR_AS(YogLinenoTableEntry, tbl)[i];
                    pc_t pc = INST(inst)->pc;
                    entry->pc_from = pc;
                    entry->pc_to = pc + INST(inst)->size;
                    lineno = INST(inst)->lineno;
                    entry->lineno = lineno;
                }
                else {
                    YOG_ASSERT(env, i != -1, "invalid line number");
                    YogLinenoTableEntry* entry = &PTR_AS(YogLinenoTableEntry, tbl)[i];
                    entry->pc_to = INST(inst)->pc + INST(inst)->size;
                }
            }

            inst = INST(inst)->next;
        }
    }

    CODE(code)->lineno_tbl = tbl;
    CODE(code)->lineno_tbl_size = size;

    RETURN_VOID(env);
}

static void
CompileData_add_inst(YogEnv* env, YogVal data, YogVal inst)
{
    INST(COMPILE_DATA(data)->last_inst)->next = inst;

    while (IS_PTR(INST(inst)->next)) {
        inst = INST(inst)->next;
    }
    COMPILE_DATA(data)->last_inst = inst;
}

static void
calc_pc(YogVal inst)
{
    pc_t pc = 0;
    while (IS_PTR(inst)) {
        switch (INST(inst)->type) {
        case INST_OP:
        case INST_LABEL:
            INST(inst)->pc = pc;
            break;
        default:
            break;
        }

        if (INST(inst)->type == INST_OP) {
            uint_t size = Yog_get_inst_size(INST_OPCODE(inst));
            INST(inst)->size = size;
            pc += size;
        }

        inst = INST(inst)->next;
    }
}

static void
CompileData_add_ret_nil(YogEnv* env, YogVal data, uint_t lineno)
{
    SAVE_ARG(env, data);
    CompileData_add_ret(env, data, lineno, 0);
    RETURN_VOID(env);
}

static int_t
count_locals_callback(YogEnv* env, YogVal key, YogVal val, YogVal* arg)
{
    if (VAR(val)->type == VT_LOCAL) {
        int_t nlocals = VAL2INT(*arg);
        *arg = INT2VAL(nlocals + 1);
    }

    return ST_CONTINUE;
}

static int_t
count_locals(YogEnv* env, YogVal vars)
{
    YogVal val = INT2VAL(0);
    YogTable_foreach(env, vars, count_locals_callback, &val);

    return VAL2INT(val);
}

static void
CompileData_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    CompileData* data = PTR_AS(CompileData, ptr);
#define KEEP(member)    YogGC_keep(env, &data->member, keeper, heap)
    KEEP(vars);
    KEEP(const2index);
    KEEP(last_inst);
    KEEP(exc_tbl);
    KEEP(exc_tbl_last);
    KEEP(label_while_start);
    KEEP(label_while_end);
    KEEP(finally_list);
    KEEP(try_list);
    KEEP(label_last_ret);
    KEEP(outer);
    KEEP(filename);
    KEEP(cur_stmt);
#undef KEEP
}

static YogVal
CompileData_new(YogEnv* env, Context ctx, YogVal vars, YogVal anchor, YogVal exc_tbl_ent, YogVal filename, ID class_name, YogVal upper_data, BOOL interactive)
{
    SAVE_ARGS5(env, vars, anchor, exc_tbl_ent, filename, upper_data);
    YogVal label_last_ret = YUNDEF;
    PUSH_LOCAL(env, label_last_ret);

    label_last_ret = Label_new(env);

    YogVal data = ALLOC_OBJ(env, CompileData_keep_children, NULL, CompileData);
    COMPILE_DATA(data)->ctx = ctx;
    COMPILE_DATA(data)->vars = vars;
    COMPILE_DATA(data)->const2index = YUNDEF;
    COMPILE_DATA(data)->label_while_start = YUNDEF;
    COMPILE_DATA(data)->label_while_end = YUNDEF;
    COMPILE_DATA(data)->finally_list = YUNDEF;
    COMPILE_DATA(data)->try_list = YUNDEF;
    COMPILE_DATA(data)->label_last_ret = label_last_ret;
    COMPILE_DATA(data)->class_name = INVALID_ID;
    COMPILE_DATA(data)->last_inst = anchor;
    COMPILE_DATA(data)->exc_tbl = exc_tbl_ent;
    COMPILE_DATA(data)->exc_tbl_last = exc_tbl_ent;
    COMPILE_DATA(data)->filename = filename;
    COMPILE_DATA(data)->class_name = class_name;
    COMPILE_DATA(data)->outer = upper_data;
    COMPILE_DATA(data)->max_outer_depth = 0;
    COMPILE_DATA(data)->cur_stmt = YUNDEF;
    COMPILE_DATA(data)->interactive = interactive;

    RETURN(env, data);
}

static int_t
get_max_outer_level_callback(YogEnv* env, YogVal key, YogVal val, YogVal* arg)
{
    if (VAR(val)->type == VT_NONLOCAL) {
        int_t max_level = VAL2INT(*arg);
        int_t level = VAR(val)->u.nonlocal.level;
        if (max_level < level) {
            *arg = INT2VAL(level);
        }
    }

    return ST_CONTINUE;
}

static uint_t
get_max_outer_level(YogEnv* env, YogVal vars)
{
    YogVal arg = INT2VAL(0);
    YogTable_foreach(env, vars, get_max_outer_level_callback, &arg);

    return VAL2INT(arg);
}

struct AllocLocalVarsTableArg {
    YogVal names;
    uint_t count;
};

typedef struct AllocLocalVarsTableArg AllocLocalVarsTableArg;

#define ALLOC_LOCAL_VARS_TABLE_ARG(v)   PTR_AS(AllocLocalVarsTableArg, (v))

static int_t
alloc_local_vars_table_callback(YogEnv* env, YogVal key, YogVal val, YogVal* arg)
{
    if (VAR(val)->type == VT_LOCAL) {
        uint_t index = VAR(val)->u.local.index;
        YOG_ASSERT(env, index < ALLOC_LOCAL_VARS_TABLE_ARG(*arg)->count, "local var index over count (%u, %u)", index, ALLOC_LOCAL_VARS_TABLE_ARG(*arg)->count);
        YogVal names = ALLOC_LOCAL_VARS_TABLE_ARG(*arg)->names;
        PTR_AS(ID, names)[index] = VAL2ID(key);
    }

    return ST_CONTINUE;
}

static void
AllocLocalVarsTableArg_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    AllocLocalVarsTableArg* arg = PTR_AS(AllocLocalVarsTableArg, ptr);
    YogGC_keep(env, &arg->names, keeper, heap);
}

static YogVal
AllocLocalVarsTableArg_new(YogEnv* env, YogVal names, uint_t count)
{
    SAVE_ARG(env, names);
    YogVal arg = YUNDEF;
    PUSH_LOCAL(env, arg);

    arg = ALLOC_OBJ(env, AllocLocalVarsTableArg_keep_children, NULL, AllocLocalVarsTableArg);
    PTR_AS(AllocLocalVarsTableArg, arg)->names = names;
    PTR_AS(AllocLocalVarsTableArg, arg)->count = count;

    RETURN(env, arg);
}

static ID*
alloc_local_vars_table(YogEnv* env, YogVal vars, uint_t count)
{
    SAVE_ARG(env, vars);
    YogVal names = YUNDEF;
    YogVal arg = YUNDEF;
    PUSH_LOCALS2(env, names, arg);

    names = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(ID) * count);
    arg = AllocLocalVarsTableArg_new(env, names, count);

    YogTable_foreach(env, vars, alloc_local_vars_table_callback, &arg);

    RETURN(env, PTR_AS(ID, names));
}

static void
update_max_outer_depth(YogVal data, uint_t depth)
{
    if (!IS_PTR(data)) {
        return;
    }
    if (COMPILE_DATA(data)->max_outer_depth + 1 < depth) {
        COMPILE_DATA(data)->max_outer_depth = depth - 1;
    }
}

static void
insert_inst_before_last_return(YogEnv* env, YogVal anchor, YogVal last, YogVal inst)
{
    SAVE_ARGS3(env, anchor, last, inst);
    YogVal i = YUNDEF;
    PUSH_LOCAL(env, i);

    i = anchor;
    while (PTR_AS(YogInst, i)->next != last) {
        i = PTR_AS(YogInst, i)->next;
    }

    PTR_AS(YogInst, i)->next = inst;
    PTR_AS(YogInst, inst)->next = last;

    RETURN_VOID(env);
}

static void
compile_default(YogEnv* env, AstVisitor* visitor, YogVal param, YogVal data)
{
    YOG_ASSERT(env, IS_PTR(param), "invalid param (0x%08x)", param);
    SAVE_ARGS2(env, param, data);
    YogVal expr = YUNDEF;
    YogVal label = YUNDEF;
    YogVal var = YUNDEF;
    PUSH_LOCALS3(env, expr, label, var);

    expr = NODE(param)->u.param.default_;
    if (!IS_PTR(expr)) {
        RETURN_VOID(env);
    }
    uint_t lineno = NODE_LINENO(param);
    ID name = NODE(param)->u.param.name;
    var = lookup_var(env, COMPILE_DATA(data)->vars, name);
    YOG_ASSERT(env, VAR(var)->type == VT_LOCAL, "invalid variable type (0x%08x)", VAR(var)->type);
    uint_t index = VAR(var)->u.local.index;
    label = Label_new(env);
    CompileData_add_jump_if_defined(env, data, lineno, index, label);
    visit_node(env, visitor, expr, data);
    append_store(env, data, lineno, name);
    add_inst(env, data, label);

    RETURN_VOID(env);
}

static void
compile_defaults(YogEnv* env, AstVisitor* visitor, YogVal params, YogVal data)
{
    if (!IS_PTR(params)) {
        return;
    }
    SAVE_ARGS2(env, params, data);
    YogVal param = YUNDEF;
    PUSH_LOCAL(env, param);

    uint_t size = YogArray_size(env, params);
    uint_t i;
    for (i = 0; i < size; i++) {
        param = YogArray_at(env, params, i);
        compile_default(env, visitor, param, data);
    }

    RETURN_VOID(env);
}

static YogVal
compile_stmts(YogEnv* env, AstVisitor* visitor, YogVal filename, ID class_name, ID func_name, YogVal params, YogVal stmts, YogVal vars, Context ctx, YogVal tail, YogVal upper_data, BOOL interactive)
{
    SAVE_ARGS6(env, filename, params, stmts, vars, tail, upper_data);
    YogVal anchor = YUNDEF;
    YogVal exc_tbl_ent = YUNDEF;
    YogVal data = YUNDEF;
    YogVal label_return_start = YUNDEF;
    YogVal label_return_end = YUNDEF;
    YogVal exc_tbl_entry = YUNDEF;
    PUSH_LOCALS6(env, anchor, exc_tbl_ent, data, label_return_start, label_return_end, exc_tbl_entry);

    anchor = Anchor_new(env);
    exc_tbl_ent = ExceptionTableEntry_new(env);
    data = CompileData_new(env, ctx, vars, anchor, exc_tbl_ent, filename, class_name, upper_data, interactive);

    label_return_start = Label_new(env);
    add_inst(env, data, label_return_start);

    compile_defaults(env, visitor, params, data);
    visitor->visit_stmts(env, visitor, stmts, data);
    if (IS_PTR(tail)) {
        CompileData_add_inst(env, data, tail);
    }
    switch (ctx) {
    case CTX_BLOCK:
    case CTX_FUNC:
    case CTX_MODULE:
    case CTX_PKG:
        {
            YogVal last_inst = COMPILE_DATA(data)->last_inst;
            if (INST(last_inst)->opcode != OP(RET)) {
                uint_t lineno = INST(last_inst)->lineno;
                CompileData_add_ret_nil(env, data, lineno);
            }
        }
        break;
    case CTX_CLASS:
    default:
        break;
    }

    label_return_end = COMPILE_DATA(data)->label_last_ret;
    insert_inst_before_last_return(env, anchor, COMPILE_DATA(data)->last_inst, label_return_end);

    YogVal bin = YUNDEF;
    YogVal code = YUNDEF;
    PUSH_LOCALS2(env, bin, code);

    calc_pc(anchor);
    bin = insts2bin(env, anchor);
    YogBinary_shrink(env, bin);

    code = YogCode_new(env);
    uint_t local_vars_count = count_locals(env, vars);
    CODE(code)->local_vars_count = local_vars_count;
    ID* local_vars_names = alloc_local_vars_table(env, vars, local_vars_count);
    CODE(code)->local_vars_names = local_vars_names;
    CODE(code)->stack_size = count_stack_size(env, anchor);
    YogVal consts = table2array(env, COMPILE_DATA(data)->const2index);
    CODE(code)->consts = consts;
    CODE(code)->insts = PTR_AS(YogBinary, bin)->body;
    uint_t outer_depth = get_max_outer_level(env, vars);
    if (outer_depth < COMPILE_DATA(data)->max_outer_depth) {
        outer_depth = COMPILE_DATA(data)->max_outer_depth;
    }
    CODE(code)->outer_size = outer_depth;
    update_max_outer_depth(upper_data, outer_depth);

    make_exception_table(env, code, data);
    make_lineno_table(env, code, anchor);

    CODE(code)->filename = filename;
    CODE(code)->class_name = class_name;
    CODE(code)->func_name = func_name;

#if 0 && !defined(MINIYOG)
    TRACE("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
    YogCode_dump(env, code);
#endif

    RETURN(env, code);
}

static void
register_params_var_table(YogEnv* env, YogVal params, YogVal var_tbl, YogVal filename)
{
    if (!IS_PTR(params)) {
        return;
    }

    SAVE_ARGS3(env, params, var_tbl, filename);

    uint_t size = YogArray_size(env, params);
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        YogVal node = YogArray_at(env, params, i);
        ID id = NODE(node)->u.param.name;
        YogVal name = ID2VAL(id);
        if (YogTable_lookup(env, var_tbl, name, NULL)) {
            raise_error(env, filename, NODE(node)->lineno, "duplicated argument name in function definition");
        }
        scan_var_register(env, var_tbl, id, VAR_PARAM);
    }

    RETURN_VOID(env);
}

static uint_t
count_required_argc(YogEnv* env, YogVal params)
{
    SAVE_ARG(env, params);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    uint_t n = 0;
    uint_t size = YogArray_size(env, params);
    uint_t i;
    for (i = 0; i < size; i++) {
        node = YogArray_at(env, params, i);
        if (NODE(node)->type != NODE_PARAM) {
            break;
        }
        if (IS_PTR(NODE(node)->u.param.default_)) {
            break;
        }
        n++;
    }

    RETURN(env, n);
}

static void
setup_params(YogEnv* env, YogVal vars, YogVal params, YogVal code)
{
    if (!IS_PTR(params)) {
        return;
    }
    SAVE_ARGS3(env, vars, params, code);
    YogVal arg_info = YUNDEF;
    PUSH_LOCAL(env, arg_info);

    arg_info = YogArgInfo_new(env);
    CODE(code)->arg_info = arg_info;

    ARG_INFO(arg_info)->required_argc = count_required_argc(env, params);

    uint_t size = YogArray_size(env, params);
    uint_t argc = 0;
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        YogVal node = YogArray_at(env, params, i);
        if (NODE(node)->type != NODE_PARAM) {
            break;
        }
        argc++;
    }

    YogVal argnames = YNIL;
    if (0 < argc) {
        argnames = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(ID) * argc);
        PUSH_LOCAL(env, argnames);
        for (i = 0; i < argc; i++) {
            YogVal node = YogArray_at(env, params, i);
            YOG_ASSERT(env, NODE(node)->type == NODE_PARAM, "Node must be NODE_PARAM.");
            PTR_AS(ID, argnames)[i] = NODE(node)->u.param.name;
        }
    }

    ARG_INFO(arg_info)->argc = argc;
    ARG_INFO(arg_info)->argnames = argnames;
    if (size == argc) {
        RETURN_VOID(env);
    }

    uint_t n = argc;
    YogVal node = YogArray_at(env, params, n);
    if (NODE(node)->type == NODE_VAR_PARAM) {
        ARG_INFO(arg_info)->varargc = 1;
        n++;
        if (size == n) {
            RETURN_VOID(env);
        }
        node = YogArray_at(env, params, n);
    }

    if (NODE(node)->type == NODE_KW_PARAM) {
        ARG_INFO(arg_info)->kwargc = 1;
        n++;
        if (size == n) {
            RETURN_VOID(env);
        }
        node = YogArray_at(env, params, n);
    }

    YOG_ASSERT(env, n == size - 1, "Parameters count is unmatched.");
    YOG_ASSERT(env, NODE(node)->type == NODE_BLOCK_PARAM, "Node must be NODE_BLOCK_PARAM.");
    ARG_INFO(arg_info)->blockargc = 1;

    RETURN_VOID(env);
}

static void
register_self(YogEnv* env, YogVal var_tbl)
{
    SAVE_ARG(env, var_tbl);

    ID name = YogVM_intern(env, env->vm, "self");
    scan_var_register(env, var_tbl, name, VAR_PARAM);

    RETURN_VOID(env);
}

static BOOL
find_outer_var(YogEnv* env, ID name, YogVal outer, uint_t* plevel, uint_t* pindex)
{
    SAVE_ARG(env, outer);

    YogVal key = ID2VAL(name);

    int_t level = 0;
    while (IS_PTR(outer) && IS_PTR(COMPILE_DATA(outer)->outer)) {
        YogVal val = YUNDEF;
        if (YogTable_lookup(env, COMPILE_DATA(outer)->vars, key, &val)) {
            switch (VAR(val)->type) {
            case VT_GLOBAL:
                RETURN(env, FALSE);
                break;
            case VT_LOCAL:
                *plevel = level + 1;
                *pindex = VAR(val)->u.local.index;
                RETURN(env, TRUE);
                break;
            case VT_NONLOCAL:
                *plevel = level + VAR(val)->u.nonlocal.level + 1;
                *pindex = VAR(val)->u.nonlocal.index;
                RETURN(env, TRUE);
                break;
            default:
                YOG_BUG(env, "unknown VarType (%d)", VAR(val)->type);
                break;
            }
        }

        level++;
        outer = COMPILE_DATA(outer)->outer;
    }

    RETURN(env, FALSE);
}

static YogVal
Var_new(YogEnv* env)
{
    return ALLOC_OBJ(env, NULL, NULL, Var);
}

static YogVal
var_table_new(YogEnv* env)
{
    SAVE_LOCALS(env);

    YogVal var_tbl = YUNDEF;
    PUSH_LOCAL(env, var_tbl);

    var_tbl = YogTable_new_symbol_table(env);
    register_self(env, var_tbl);

    RETURN(env, var_tbl);
}

static YogVal
vars_flags2type(YogEnv* env, YogVal var_tbl, YogVal outer)
{
    SAVE_ARGS2(env, var_tbl, outer);
    YogVal vars = YUNDEF;
    YogVal iter = YUNDEF;
    YogVal entry = YUNDEF;
    YogVal var = YUNDEF;
    PUSH_LOCALS4(env, vars, iter, entry, var);

    vars = YogTable_new_symbol_table(env);

    uint_t params_num = 0;
    iter = YogTable_get_iterator(env, var_tbl);
    while (YogTableIterator_next(env, iter)) {
        ID name = VAL2ID(YogTableIterator_current_key(env, iter));
        entry = YogTableIterator_current_value(env, iter);
        int_t flags = SCAN_VAR_ENTRY(entry)->flags;
        if (!IS_PARAM(flags)) {
            continue;
        }

        var = Var_new(env);
        VAR(var)->type = VT_LOCAL;
        VAR(var)->u.local.index = SCAN_VAR_ENTRY(entry)->index;
        YogTable_add_direct(env, vars, ID2VAL(name), var);

        params_num++;
    }

    uint_t locals_num = 0;
    iter = YogTable_get_iterator(env, var_tbl);
    while (YogTableIterator_next(env, iter)) {
        ID name = VAL2ID(YogTableIterator_current_key(env, iter));
        entry = YogTableIterator_current_value(env, iter);
        int_t flags = SCAN_VAR_ENTRY(entry)->flags;
        if (IS_PARAM(flags)) {
            continue;
        }

        var = Var_new(env);
        if (IS_NONLOCAL(flags) || !IS_ASSIGNED(flags)) {
            uint_t level = 0;
            uint_t index = 0;
            if (find_outer_var(env, name, outer, &level, &index)) {
                VAR(var)->type = VT_NONLOCAL;
                VAR(var)->u.nonlocal.level = level;
                VAR(var)->u.nonlocal.index = index;
            }
            else {
                VAR(var)->type = VT_GLOBAL;
            }
        }
        else {
            VAR(var)->type = VT_LOCAL;
            VAR(var)->u.local.index = params_num + locals_num;
            locals_num++;
        }

        YogTable_add_direct(env, vars, ID2VAL(name), var);
    }

    RETURN(env, vars);
}

static YogVal
compile_func(YogEnv* env, AstVisitor* visitor, YogVal filename, ID class_name, YogVal node, YogVal upper)
{
    SAVE_ARGS3(env, filename, node, upper);
    YogVal var_tbl = YUNDEF;
    YogVal params = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal vars = YUNDEF;
    PUSH_LOCALS4(env, var_tbl, params, stmts, vars);

    var_tbl = var_table_new(env);
    params = NODE(node)->u.funcdef.params;
    register_params_var_table(env, params, var_tbl, filename);
    stmts = NODE(node)->u.funcdef.stmts;
    var_tbl = make_var_table(env, params, stmts, var_tbl);
    vars = vars_flags2type(env, var_tbl, upper);

    ID func_name = NODE(node)->u.funcdef.name;

    YogVal code = compile_stmts(env, visitor, filename, class_name, func_name, params, stmts, vars, CTX_FUNC, YNIL, upper, FALSE);
    PUSH_LOCAL(env, code);
    setup_params(env, vars, params, code);

    RETURN(env, code);
}

static void
compile_decorators_call(YogEnv* env, AstVisitor* visitor, YogVal decorators, YogVal data)
{
    SAVE_ARGS2(env, decorators, data);
    YogVal decorator = YUNDEF;
    PUSH_LOCAL(env, decorator);

    if (!IS_PTR(decorators)) {
        RETURN_VOID(env);
    }

    uint_t size = YogArray_size(env, decorators);
    uint_t i;
    for (i = 0; i < size; i++) {
        decorator = YogArray_at(env, decorators, i);
        visit_node(env, visitor, decorator, data);

        uint_t lineno = NODE(decorator)->lineno;
        CompileData_add_call_function(env, data, lineno, 1, 0, 0, 0, 0, 1, 0, 0);
    }

    RETURN_VOID(env);
}

static void
compile_visit_func_def(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal var = YUNDEF;
    YogVal code = YUNDEF;
    YogVal decorators = YUNDEF;
    PUSH_LOCALS3(env, var, code, decorators);

    ID class_name = INVALID_ID;
    if (COMPILE_DATA(data)->ctx == CTX_CLASS) {
        class_name = COMPILE_DATA(data)->class_name;
    }

    code = compile_func(env, visitor, COMPILE_DATA(data)->filename, class_name, node, data);

    uint_t lineno = NODE(node)->lineno;
    add_push_const(env, data, code, lineno);

    ID func_name = NODE(node)->u.funcdef.name;
    CompileData_add_make_function(env, data, lineno, func_name);
    decorators = NODE(node)->u.funcdef.decorators;
    compile_decorators_call(env, visitor, decorators, data);
    append_store(env, data, lineno, func_name);

    RETURN_VOID(env);
}

static void
compile_visit_func_call(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    uint_t lhs_left_num;
    uint_t lhs_middle_num;
    if (COMPILE_DATA(data)->cur_stmt != node) {
        lhs_left_num = 1;
        lhs_middle_num = 0;
    }
    else if (COMPILE_DATA(data)->interactive) {
        lhs_left_num = 0;
        lhs_middle_num = 1;
    }
    else {
        lhs_left_num = 0;
        lhs_middle_num = 0;
    }
    compile_call_func(env, visitor, node, data, lhs_left_num, lhs_middle_num, 0);

    RETURN_VOID(env);
}

static void
compile_visit_variable(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal var = YUNDEF;
    PUSH_LOCAL(env, var);

    ID id = NODE(node)->u.variable.id;
    uint_t lineno = NODE(node)->lineno;
    switch (COMPILE_DATA(data)->ctx) {
    case CTX_BLOCK:
    case CTX_FUNC:
        {
            var = lookup_var(env, COMPILE_DATA(data)->vars, id);
            YogVal name = YogVM_id2name(env, env->vm, id);
            YOG_ASSERT(env, IS_PTR(var), "can't find variable (%s)", STRING_CSTR(name));
            switch (VAR(var)->type) {
            case VT_GLOBAL:
                CompileData_add_load_global(env, data, lineno, id);
                break;
            case VT_LOCAL:
                {
                    uint_t index = VAR(var)->u.local.index;
                    CompileData_add_load_local(env, data, lineno, index);
                }
                break;
            case VT_NONLOCAL:
                {
                    uint_t level = VAR(var)->u.nonlocal.level;
                    uint_t index = VAR(var)->u.nonlocal.index;
                    CompileData_add_load_nonlocal(env, data, lineno, level, index);
                }
                break;
            default:
                YOG_BUG(env, "unknown variable type (0x%x)", VAR(var)->type);
                break;
            }
            break;
        }
    case CTX_CLASS:
    case CTX_MODULE:
    case CTX_PKG:
        CompileData_add_load_name(env, data, lineno, id);
        break;
    default:
        YOG_ASSERT(env, FALSE, "Unknown context.");
        break;
    }

    RETURN_VOID(env);
}

static uint_t
get_last_lineno(YogEnv* env, YogVal array)
{
    SAVE_ARG(env, array);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    uint_t size = YogArray_size(env, array);
    YOG_ASSERT(env, 0 < size, "invalid size (0x%08x)", size);
    node = YogArray_at(env, array, size - 1);
    uint_t lineno = NODE(node)->lineno;

    RETURN(env, lineno);
}

static void
FinallyListEntry_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    FinallyListEntry* ent = PTR_AS(FinallyListEntry, ptr);
#define KEEP(member)    YogGC_keep(env, &ent->member, keeper, heap)
    KEEP(prev);
    KEEP(node);
#undef KEEP
}

static YogVal
FinallyListEntry_new(YogEnv* env)
{
    YogVal ent = ALLOC_OBJ(env, FinallyListEntry_keep_children, NULL, FinallyListEntry);
    PTR_AS(FinallyListEntry, ent)->prev = YUNDEF;
    PTR_AS(FinallyListEntry, ent)->node = YUNDEF;

    return ent;
}

static void
add_raise_last_exception(YogEnv* env, YogVal data, uint_t lineno)
{
    SAVE_ARG(env, data);
    CompileData_add_load_exception(env, data, lineno);
    ID id = YogVM_intern(env, env->vm, "raise_exception");
    CompileData_add_load_global(env, data, lineno, id);
    CompileData_add_call_function(env, data, lineno, 1, 0, 0, 0, 0, 0, 0, 0);
    RETURN_VOID(env);
}

static void
compile_visit_finally(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal label_head_start = YUNDEF;
    YogVal label_head_end = YUNDEF;
    YogVal label_finally_error_start = YUNDEF;
    YogVal label_finally_end = YUNDEF;
    YogVal try_list_entry = YUNDEF;
    PUSH_LOCALS5(env, label_head_start, label_head_end, label_finally_error_start, label_finally_end, try_list_entry);

    label_head_start = Label_new(env);
    label_head_end = Label_new(env);
    label_finally_error_start = Label_new(env);
    label_finally_end = Label_new(env);

    YogVal finally_list_entry = YUNDEF;
    YogVal exc_tbl_entry = YUNDEF;
    YogVal stmts = YUNDEF;
    PUSH_LOCALS3(env, finally_list_entry, exc_tbl_entry, stmts);

    finally_list_entry = FinallyListEntry_new(env);
    FINALLY_LIST_ENTRY(finally_list_entry)->prev = COMPILE_DATA(data)->finally_list;
    COMPILE_DATA(data)->finally_list = finally_list_entry;
    FINALLY_LIST_ENTRY(finally_list_entry)->node = node;

    try_list_entry = CompileData_push_try(env, data, node);

    exc_tbl_entry = ExceptionTableEntry_new(env);
    EXCEPTION_TABLE_ENTRY(exc_tbl_entry)->next = YNIL;
    EXCEPTION_TABLE_ENTRY(exc_tbl_entry)->from = label_head_start;
    EXCEPTION_TABLE_ENTRY(exc_tbl_entry)->to = label_head_end;
    EXCEPTION_TABLE_ENTRY(exc_tbl_entry)->target = label_finally_error_start;
    TRY_LIST_ENTRY(try_list_entry)->exc_tbl = exc_tbl_entry;

    add_inst(env, data, label_head_start);
    visitor->visit_stmts(env, visitor, NODE(node)->u.finally.head, data);
    add_inst(env, data, label_head_end);

    stmts = NODE(node)->u.finally.body;
    visitor->visit_stmts(env, visitor, stmts, data);
    uint_t lineno;
    if (IS_PTR(stmts)) {
        lineno = get_last_lineno(env, stmts);
    }
    else {
        lineno = NODE(node)->lineno;
    }
    CompileData_add_jump(env, data, lineno, label_finally_end);

    add_inst(env, data, label_finally_error_start);
    visitor->visit_stmts(env, visitor, stmts, data);
    add_raise_last_exception(env, data, NODE_LINENO(node));

    add_inst(env, data, label_finally_end);

    PUSH_EXCEPTION_TABLE_ENTRY(data, exc_tbl_entry);

    CompileData_pop_try(env, data);

    COMPILE_DATA(data)->finally_list = FINALLY_LIST_ENTRY(finally_list_entry)->prev;

    RETURN_VOID(env);
}

static void
compile_except(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data, YogVal label_else_end)
{
    SAVE_ARGS3(env, node, data, label_else_end);
    YogVal label_body_begin = YUNDEF;
    YogVal label_body_end = YUNDEF;
    YogVal types = YUNDEF;
    YogVal type = YUNDEF;
    YogVal stmts = YUNDEF;
    PUSH_LOCALS5(env, label_body_begin, label_body_end, types, type, stmts);

    label_body_begin = Label_new(env);
    label_body_end = Label_new(env);

    types = NODE(node)->u.except_body.types;
    if (IS_PTR(types)) {
        uint_t size = YogArray_size(env, types);
        uint_t j;
        for (j = 0; j < size; j++) {
            type = YogArray_at(env, types, j);
            uint_t lineno = NODE_LINENO(type);
            CompileData_add_load_exception(env, data, lineno);
            visit_node(env, visitor, type, data);
            CompileData_add_match_exception(env, data, lineno);
            CompileData_add_jump_if_true(env, data, lineno, label_body_begin);
        }
        uint_t lineno = get_last_lineno(env, types);
        CompileData_add_jump(env, data, lineno, label_body_end);
    }

    add_inst(env, data, label_body_begin);

    ID id = NODE(node)->u.except_body.var;
    if (id != NO_EXC_VAR) {
        uint_t lineno = NODE_LINENO(node);
        CompileData_add_load_exception(env, data, lineno);
        append_store(env, data, lineno, id);
    }

    stmts = NODE(node)->u.except_body.stmts;
    visitor->visit_stmts(env, visitor, stmts, data);
    uint_t lineno;
    if (IS_PTR(stmts)) {
        lineno = get_last_lineno(env, stmts);
    }
    else {
        lineno = NODE(node)->lineno;
    }
    CompileData_add_jump(env, data, lineno, label_else_end);

    add_inst(env, data, label_body_end);

    RETURN_VOID(env);
}

static void
compile_visit_except(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal label_head_start = YUNDEF;
    YogVal label_head_end = YUNDEF;
    YogVal label_excepts_start = YUNDEF;
    YogVal label_else_start = YUNDEF;
    YogVal label_else_end = YUNDEF;
    YogVal exc_tbl_entry = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal excepts = YUNDEF;
    PUSH_LOCALS8(env, label_head_start, label_head_end, label_excepts_start, label_else_start, label_else_end, exc_tbl_entry, stmts, excepts);
    YogVal except = YUNDEF;
    YogVal try_list_entry = YUNDEF;
    PUSH_LOCALS2(env, except, try_list_entry);

    label_head_start = Label_new(env);
    label_head_end = Label_new(env);
    label_excepts_start = Label_new(env);
    label_else_start = Label_new(env);
    label_else_end = Label_new(env);

    try_list_entry = CompileData_push_try(env, data, node);

    exc_tbl_entry = ExceptionTableEntry_new(env);
    EXCEPTION_TABLE_ENTRY(exc_tbl_entry)->next = YNIL;
    EXCEPTION_TABLE_ENTRY(exc_tbl_entry)->from = label_head_start;
    EXCEPTION_TABLE_ENTRY(exc_tbl_entry)->to = label_head_end;
    EXCEPTION_TABLE_ENTRY(exc_tbl_entry)->target = label_excepts_start;
    TRY_LIST_ENTRY(try_list_entry)->exc_tbl = exc_tbl_entry;

    add_inst(env, data, label_head_start);
    stmts = NODE(node)->u.except.head;
    visitor->visit_stmts(env, visitor, stmts, data);
    add_inst(env, data, label_head_end);
    uint_t lineno;
    if (IS_PTR(stmts)) {
        lineno = get_last_lineno(env, stmts);
    }
    else {
        lineno = NODE(node)->lineno;
    }
    CompileData_add_jump(env, data, lineno, label_else_start);

    add_inst(env, data, label_excepts_start);
    excepts = NODE(node)->u.except.excepts;
    uint_t size = YogArray_size(env, excepts);
    uint_t i;
    for (i = 0; i < size; i++) {
        except = YogArray_at(env, excepts, i);
        compile_except(env, visitor, except, data, label_else_end);
    }
    add_raise_last_exception(env, data, NODE_LINENO(node));

    add_inst(env, data, label_else_start);
    visitor->visit_stmts(env, visitor, NODE(node)->u.except.else_, data);
    add_inst(env, data, label_else_end);

    PUSH_EXCEPTION_TABLE_ENTRY(data, exc_tbl_entry);

    CompileData_pop_try(env, data);

    RETURN_VOID(env);
}

static void
compile_visit_while(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal while_start = YUNDEF;
    YogVal while_end = YUNDEF;
    YogVal label_while_start_prev = YUNDEF;
    YogVal label_while_end_prev = YUNDEF;
    YogVal test = YUNDEF;
    PUSH_LOCALS5(env, while_start, while_end, label_while_start_prev, label_while_end_prev, test);

    while_start = Label_new(env);
    while_end = Label_new(env);

    label_while_start_prev = COMPILE_DATA(data)->label_while_start;
    label_while_end_prev = COMPILE_DATA(data)->label_while_end;
    COMPILE_DATA(data)->label_while_start = while_start;
    COMPILE_DATA(data)->label_while_end = while_end;

    add_inst(env, data, while_start);
    test = NODE(node)->u.while_.test;
    visit_node(env, visitor, test, data);
    CompileData_add_jump_if_false(env, data, NODE(test)->lineno, while_end);

    YogVal stmts = NODE(node)->u.while_.stmts;
    PUSH_LOCAL(env, stmts);
    visitor->visit_stmts(env, visitor, stmts, data);
    uint_t lineno;
    if (IS_PTR(stmts)) {
        lineno = get_last_lineno(env, stmts);
    }
    else {
        lineno = NODE(node)->lineno;
    }
    CompileData_add_jump(env, data, lineno, while_start);
    add_inst(env, data, while_end);

    COMPILE_DATA(data)->label_while_end = label_while_end_prev;
    COMPILE_DATA(data)->label_while_start = label_while_start_prev;

    RETURN_VOID(env);
}

static void
compile_visit_if(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal label_tail_start = YUNDEF;
    YogVal label_stmt_end = YUNDEF;
    YogVal test = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal tail = YUNDEF;
    PUSH_LOCALS5(env, label_tail_start, label_stmt_end, test, stmts, tail);

    label_tail_start = Label_new(env);
    label_stmt_end = Label_new(env);

    test = NODE(node)->u.if_.test;
    visit_node(env, visitor, test, data);
    uint_t lineno = NODE(test)->lineno;
    CompileData_add_jump_if_false(env, data, lineno, label_tail_start);

    stmts = NODE(node)->u.if_.stmts;
    lineno = 0;
    if (IS_PTR(stmts)) {
        visitor->visit_stmts(env, visitor, stmts, data);
        lineno = get_last_lineno(env, stmts);
    }
    else {
        lineno = NODE(test)->lineno;
    }
    CompileData_add_jump(env, data, lineno, label_stmt_end);

    add_inst(env, data, label_tail_start);
    tail = NODE(node)->u.if_.tail;
    if (IS_PTR(tail)) {
        visitor->visit_stmts(env, visitor, tail, data);
    }
    add_inst(env, data, label_stmt_end);

    RETURN_VOID(env);
}

static void
split_exception_table(YogEnv* env, YogVal exc_tbl_entry, YogVal label_from, YogVal label_to)
{
    SAVE_ARGS3(env, exc_tbl_entry, label_from, label_to);

    YogVal entry = YUNDEF;
    PUSH_LOCAL(env, entry);

    entry = exc_tbl_entry;
    YOG_ASSERT(env, IS_PTR(entry), "Exception table is empty.");
    while (IS_PTR(EXCEPTION_TABLE_ENTRY(entry)->next)) {
        entry = EXCEPTION_TABLE_ENTRY(entry)->next;
    }

    YogVal new_entry = ExceptionTableEntry_new(env);
    EXCEPTION_TABLE_ENTRY(new_entry)->from = label_to;
    EXCEPTION_TABLE_ENTRY(new_entry)->to = EXCEPTION_TABLE_ENTRY(entry)->to;
    EXCEPTION_TABLE_ENTRY(new_entry)->target = EXCEPTION_TABLE_ENTRY(entry)->target;
    EXCEPTION_TABLE_ENTRY(entry)->to = label_from;
    EXCEPTION_TABLE_ENTRY(entry)->next = new_entry;

    RETURN_VOID(env);
}

static void
compile_while_jump(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data, YogVal jump_to)
{
    SAVE_ARGS3(env, node, data, jump_to);
    YogVal finally_list_entry = YUNDEF;
    YogVal label_start = YUNDEF;
    YogVal label_end = YUNDEF;
    YogVal try_list_entry = YUNDEF;
    PUSH_LOCALS4(env, finally_list_entry, label_start, label_end, try_list_entry);

    if (!IS_PTR(COMPILE_DATA(data)->label_while_start)) {
        RETURN_VOID(env);
    }

    finally_list_entry = COMPILE_DATA(data)->finally_list;
    while (IS_PTR(finally_list_entry)) {
        label_start = Label_new(env);
        label_end = Label_new(env);

        add_inst(env, data, label_start);
        visitor->visit_stmts(env, visitor, NODE(FINALLY_LIST_ENTRY(finally_list_entry)->node)->u.finally.body, data);
        add_inst(env, data, label_end);

        try_list_entry = COMPILE_DATA(data)->try_list;
        while (TRUE) {
            split_exception_table(env, TRY_LIST_ENTRY(try_list_entry)->exc_tbl, label_start, label_end);
            if (VAL2PTR(TRY_LIST_ENTRY(try_list_entry)->node) == VAL2PTR(FINALLY_LIST_ENTRY(finally_list_entry)->node)) {
                break;
            }

            try_list_entry = TRY_LIST_ENTRY(try_list_entry)->prev;
        }

        finally_list_entry = FINALLY_LIST_ENTRY(finally_list_entry)->prev;
    }
    CompileData_add_jump(env, data, NODE(node)->lineno, jump_to);

    RETURN_VOID(env);
}

static void
compile_visit_break(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal exprs = YUNDEF;
    PUSH_LOCAL(env, exprs);

    if (IS_PTR(COMPILE_DATA(data)->label_while_start)) {
        YogVal label_while_end = COMPILE_DATA(data)->label_while_end;
        compile_while_jump(env, visitor, node, data, label_while_end);
        RETURN_VOID(env);
    }

    uint8_t n;
    exprs = NODE(node)->u.break_.exprs;
    if (IS_PTR(exprs)) {
        YOG_ASSERT(env, BASIC_OBJ_TYPE(exprs) == TYPE_ARRAY, "invalid exprs (0x%08x)", BASIC_OBJ_TYPE(exprs));
        visit_array_elements_opposite_order(env, visitor, exprs, data);
        YOG_ASSERT(env, YogArray_size(env, exprs) <= UINT8_MAX, "too many returned values");
        n = YogArray_size(env, exprs);
    }
    else {
        n = 0;
    }

    CompileData_add_long_break(env, data, NODE_LINENO(node), n);

    RETURN_VOID(env);
}

static void
compile_visit_next(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal expr = YUNDEF;
    PUSH_LOCAL(env, expr);

    YogVal label_while_start = COMPILE_DATA(data)->label_while_start;
    if (IS_PTR(label_while_start)) {
        compile_while_jump(env, visitor, node, data, label_while_start);
        RETURN_VOID(env);
    }

    uint_t lineno = NODE(node)->lineno;
    expr = NODE(node)->u.next.expr;
    if (IS_PTR(expr)) {
        visit_node(env, visitor, expr, data);
    }
    else {
        add_push_const(env, data, YNIL, lineno);
    }
    CompileData_add_jump(env, data, lineno, COMPILE_DATA(data)->label_last_ret);

    RETURN_VOID(env);
}

static YogVal
compile_block(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal vars = YUNDEF;
    YogVal code = YUNDEF;
    YogVal filename = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal var_tbl = YUNDEF;
    YogVal params = YUNDEF;
    PUSH_LOCALS6(env, vars, code, filename, stmts, var_tbl, params);

    var_tbl = var_table_new(env);
    params = NODE(node)->u.blockarg.params;
    filename = COMPILE_DATA(data)->filename;
    register_params_var_table(env, params, var_tbl, filename);
    stmts = NODE(node)->u.blockarg.stmts;
    var_tbl = make_var_table(env, params, stmts, var_tbl);
    vars = vars_flags2type(env, var_tbl, data);

    ID class_name = INVALID_ID;
    ID func_name = YogVM_intern(env, env->vm, "<block>");
    code = compile_stmts(env, visitor, filename, class_name, func_name, params, stmts, vars, CTX_BLOCK, YNIL, data, FALSE);

    setup_params(env, vars, params, code);

    RETURN(env, code);
}

static void
compile_visit_block(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal code = compile_block(env, visitor, node, data);

    uint_t lineno = NODE(node)->lineno;
    add_push_const(env, data, code, lineno);
    CompileData_add_make_block(env, data, lineno);

    RETURN_VOID(env);
}

static YogVal
compile_class(YogEnv* env, AstVisitor* visitor, ID class_name, YogVal stmts, uint_t first_lineno, YogVal data)
{
    SAVE_ARGS2(env, stmts, data);

    YogVal var_tbl = YUNDEF;
    YogVal vars = YUNDEF;
    YogVal ret = YUNDEF;
    YogVal push_self_name = YUNDEF;
    PUSH_LOCALS4(env, var_tbl, vars, ret, push_self_name);

    var_tbl = make_var_table(env, YUNDEF, stmts, YUNDEF);
    vars = vars_flags2type(env, var_tbl, COMPILE_DATA(data)->outer);

    uint_t lineno;
    if (IS_PTR(stmts)) {
        lineno = get_last_lineno(env, stmts);
    }
    else {
        lineno = first_lineno;
    }

    ret = Inst_new(env, lineno);
    INST(ret)->next = YNIL;
    INST(ret)->opcode = OP(RET);
    INST(ret)->u.ret.n = 1;
    push_self_name = Inst_new(env, lineno);
    INST(push_self_name)->next = ret;
    INST(push_self_name)->opcode = OP(PUSH_SELF_NAME);

    YogVal filename = COMPILE_DATA(data)->filename;
    ID func_name = INVALID_ID;

    YogVal code = compile_stmts(env, visitor, filename, class_name, func_name, YUNDEF, stmts, vars, CTX_CLASS, push_self_name, COMPILE_DATA(data)->outer, FALSE);

    RETURN(env, code);
}

static YogVal
compile_module(YogEnv* env, AstVisitor* visitor, ID name, YogVal stmts, uint_t lineno, YogVal data)
{
    SAVE_ARGS2(env, stmts, data);
    YogVal var_tbl = YUNDEF;
    YogVal vars = YUNDEF;
    YogVal code = YUNDEF;
    YogVal filename = YUNDEF;
    PUSH_LOCALS4(env, var_tbl, vars, code, filename);

    var_tbl = make_var_table(env, YUNDEF, stmts, YUNDEF);
    vars = vars_flags2type(env, var_tbl, COMPILE_DATA(data)->outer);

    filename = COMPILE_DATA(data)->filename;
    ID class_name = name;
    ID func_name = YogVM_intern(env, env->vm, "<module>");

    code = compile_stmts(env, visitor, filename, class_name, func_name, YUNDEF, stmts, vars, CTX_MODULE, YUNDEF, COMPILE_DATA(data)->outer, FALSE);

    RETURN(env, code);
}

static void
compile_visit_module(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    PUSH_LOCALS2(env, stmts, code);

    ID name = NODE(node)->u.module.name;

    stmts = NODE(node)->u.module.stmts;
    uint_t lineno = NODE(node)->lineno;
    code = compile_module(env, visitor, name, stmts, lineno, data);
    add_push_const(env, data, code, lineno);

    CompileData_add_make_module(env, data, lineno, name);
    CompileData_add_pop(env, data, lineno);

    append_store(env, data, lineno, name);

    RETURN_VOID(env);
}

static void
compile_visit_class(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal super = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    YogVal decorators = YUNDEF;
    PUSH_LOCALS4(env, super, stmts, code, decorators);

    ID name = NODE(node)->u.klass.name;
    add_push_const(env, data, ID2VAL(name), NODE(node)->lineno);

    super = NODE(node)->u.klass.super;
    if (IS_PTR(super)) {
        visit_node(env, visitor, super, data);
    }
    else {
        YogVal cObject = env->vm->cObject;
        add_push_const(env, data, cObject, NODE(node)->lineno);
    }

    stmts = NODE(node)->u.klass.stmts;
    uint_t lineno = NODE(node)->lineno;
    code = compile_class(env, visitor, name, stmts, lineno, data);
    add_push_const(env, data, code, lineno);

    CompileData_add_make_class(env, data, lineno);
    decorators = NODE(node)->u.klass.decorators;
    compile_decorators_call(env, visitor, decorators, data);
    append_store(env, data, lineno, name);

    RETURN_VOID(env);
}

static void
compile_visit_return(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal exprs = YUNDEF;
    PUSH_LOCAL(env, exprs);

    uint8_t n;
    exprs = NODE(node)->u.return_.exprs;
    if (IS_PTR(exprs)) {
        YOG_ASSERT(env, BASIC_OBJ_TYPE(exprs) == TYPE_ARRAY, "invalid exprs (0x%08x)", BASIC_OBJ_TYPE(exprs));
        visit_array_elements_opposite_order(env, visitor, exprs, data);
        YOG_ASSERT(env, YogArray_size(env, exprs) <= UINT8_MAX, "too many returned values");
        n = YogArray_size(env, exprs);
    }
    else {
        n = 0;
    }

    uint_t lineno = NODE_LINENO(node);
    if (COMPILE_DATA(data)->ctx == CTX_BLOCK) {
        CompileData_add_long_return(env, data, lineno, n);
        RETURN_VOID(env);
    }

    CompileData_add_ret(env, data, lineno, n);

    RETURN_VOID(env);
}

static void
compile_visit_super(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    uint_t lineno = NODE(node)->lineno;
    CompileData_add_load_super(env, data, lineno);
    RETURN_VOID(env);
}

static void
compile_visit_subscript(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visit_node(env, visitor, NODE(node)->u.subscript.index, data);

    visit_node(env, visitor, NODE(node)->u.subscript.prefix, data);
    uint_t lineno = NODE(node)->lineno;
    ID attr = YogVM_intern(env, env->vm, "[]");
    CompileData_add_load_attr(env, data, lineno, attr);

    CompileData_add_call_function(env, data, lineno, 1, 0, 0, 0, 0, 1, 0, 0);

    RETURN_VOID(env);
}

static ID
join_package_names(YogEnv* env, YogVal pkg_names)
{
    SAVE_ARG(env, pkg_names);
    YogVal name = YUNDEF;
    YogVal s = YUNDEF;
    PUSH_LOCALS2(env, name, s);

    size_t len = 0;
    uint_t size = YogArray_size(env, pkg_names);
    uint_t i;
    for (i = 0; i < size; i++) {
        name = YogArray_at(env, pkg_names, i);
        s = YogVM_id2name(env, env->vm, VAL2ID(name));
        len += YogString_size(env, s);
    }
    len += size - 1;
#if defined(_alloca) && !defined(alloca)
#   define alloca   _alloca
#endif
    char* pkg = (char*)alloca(sizeof(char) * (len + 1));
    char* pc = pkg - 1;
    for (i = 0; i < size; i++) {
        name = YogArray_at(env, pkg_names, i);
        s = YogVM_id2name(env, env->vm, VAL2ID(name));
        pc++;
        uint_t size = YogString_size(env, s);
        memcpy(pc, STRING_CSTR(s), size);
        pc += size;
        *pc = '.';
    }
    *pc = '\0';

    RETURN(env, YogVM_intern(env, env->vm, pkg));
}

static void
compile_import(YogEnv* env, YogVal pkg_names, YogVal data, uint_t lineno)
{
    SAVE_ARGS2(env, pkg_names, data);

    ID pkg = join_package_names(env, pkg_names);
    uint_t c = register_const(env, data, ID2VAL(pkg));
    CompileData_add_push_const(env, data, lineno, c);

    ID import_package = YogVM_intern(env, env->vm, "import_package");
    CompileData_add_load_global(env, data, lineno, import_package);

    CompileData_add_call_function(env, data, lineno, 1, 0, 0, 0, 0, 1, 0, 0);

    YogVal var_name = YogArray_at(env, pkg_names, 0);
    append_store(env, data, lineno, VAL2ID(var_name));

    RETURN_VOID(env);
}

static void
compile_visit_import(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal names = YUNDEF;
    PUSH_LOCAL(env, names);

    uint_t lineno = NODE(node)->lineno;
    names = NODE(node)->u.import.names;
    uint_t size = YogArray_size(env, names);
    uint_t i;
    for (i = 0; i < size; i++) {
        YogVal pkg_names = YogArray_at(env, names, i);
        compile_import(env, pkg_names, data, lineno);
    }

    RETURN_VOID(env);
}

static void
compile_visit_attr(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visit_node(env, visitor, NODE(node)->u.attr.obj, data);

    uint_t lineno = NODE(node)->lineno;
    ID name = NODE(node)->u.attr.name;
    CompileData_add_load_attr(env, data, lineno, name);

    RETURN_VOID(env);
}

static void
compile_visit_array(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal elems = YUNDEF;
    PUSH_LOCAL(env, elems);

    elems = NODE(node)->u.array.elems;
    uint_t lineno = NODE(node)->lineno;
    if (!IS_PTR(elems)) {
        CompileData_add_make_array(env, data, lineno, 0);
        RETURN_VOID(env);
    }

    visit_array_elements(env, visitor, elems, data);
    uint_t size = YogArray_size(env, elems);
    CompileData_add_make_array(env, data, lineno, size);

    RETURN_VOID(env);
}

static void
compile_visit_not(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    visit_node(env, visitor, NODE(node)->u.not.expr, data);
    CompileData_add_not(env, data, NODE(node)->lineno);
    RETURN_VOID(env);
}

static void
compile_visit_raise(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visit_node(env, visitor, NODE(node)->u.raise.expr, data);
    uint_t lineno = NODE_LINENO(node);
    ID func = YogVM_intern(env, env->vm, "raise_exception");
    CompileData_add_load_global(env, data, lineno, func);
    CompileData_add_call_function(env, data, lineno, 1, 0, 0, 0, 0, 0, 0, 0);

    RETURN_VOID(env);
}

static void
compile_visit_set(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal elems = YUNDEF;
    PUSH_LOCAL(env, elems);

    visit_each_set_elem(env, visitor, node, data);

    uint_t lineno = NODE(node)->lineno;
    elems = NODE(node)->u.set.elems;
    uint_t size = YogArray_size(env, elems);
    CompileData_add_make_set(env, data, lineno, size);

    RETURN_VOID(env);
}

static void
compile_init_visitor(AstVisitor* visitor)
{
    visitor->visit_array = compile_visit_array;
    visitor->visit_assign = compile_visit_assign;
    visitor->visit_attr = compile_visit_attr;
    visitor->visit_block = compile_visit_block;
    visitor->visit_break = compile_visit_break;
    visitor->visit_dict = compile_visit_dict;
    visitor->visit_except = compile_visit_except;
    visitor->visit_except_body = NULL;
    visitor->visit_finally = compile_visit_finally;
    visitor->visit_func_call = compile_visit_func_call;
    visitor->visit_func_def = compile_visit_func_def;
    visitor->visit_if = compile_visit_if;
    visitor->visit_import = compile_visit_import;
    visitor->visit_class = compile_visit_class;
    visitor->visit_literal = compile_visit_literal;
    visitor->visit_logical_and = compile_visit_logical_and;
    visitor->visit_logical_or = compile_visit_logical_or;
    visitor->visit_module = compile_visit_module;
    visitor->visit_multi_assign = compile_visit_multi_assign;
    visitor->visit_multi_assign_lhs = NULL;
    visitor->visit_next = compile_visit_next;
    visitor->visit_nonlocal = NULL;
    visitor->visit_not = compile_visit_not;
    visitor->visit_raise = compile_visit_raise;
    visitor->visit_return = compile_visit_return;
    visitor->visit_set = compile_visit_set;
    visitor->visit_stmt = compile_visit_stmt;
    visitor->visit_stmts = compile_visit_stmts;
    visitor->visit_subscript = compile_visit_subscript;
    visitor->visit_super = compile_visit_super;
    visitor->visit_variable = compile_visit_variable;
    visitor->visit_while = compile_visit_while;
}

static YogVal
compile_package(YogEnv* env, const char* filename, YogVal stmts, BOOL interactive)
{
    SAVE_ARG(env, stmts);
    YogVal var_tbl = YUNDEF;
    YogVal vars = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS3(env, var_tbl, vars, name);

    var_tbl = make_var_table(env, YUNDEF, stmts, YUNDEF);
    vars = vars_flags2type(env, var_tbl, YUNDEF);

    AstVisitor visitor;
    compile_init_visitor(&visitor);

    if (filename == NULL) {
        filename = "<stdin>";
    }
    name = YogCharArray_new_str(env, filename);

    ID class_name = INVALID_ID;
    ID func_name = YogVM_intern(env, env->vm, "<package>");

    YogVal code = compile_stmts(env, &visitor, name, class_name, func_name, YUNDEF, stmts, vars, CTX_PKG, YNIL, YUNDEF, interactive);

    RETURN(env, code);
}

YogVal
YogCompiler_compile_package(YogEnv* env, const char* filename, YogVal stmts)
{
    return compile_package(env, filename, stmts, FALSE);
}

YogVal
YogCompiler_compile_interactive(YogEnv* env, YogVal stmts)
{
    return compile_package(env, MAIN_MODULE_NAME, stmts, TRUE);
}

YogVal
YogCompiler_compile_finish_code(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal inst = YUNDEF;
    YogVal bin = YUNDEF;
    YogVal code = YUNDEF;
    PUSH_LOCALS3(env, inst, bin, code);

    inst = Inst_new(env, 0);
    INST(inst)->type = INST_OP;
    INST_OPCODE(inst) = OP(FINISH);

    bin = insts2bin(env, inst);
    YogBinary_shrink(env, bin);

    code = YogCode_new(env);
    CODE(code)->stack_size = 1;
    CODE(code)->insts = PTR_AS(YogBinary, bin)->body;

    RETURN(env, code);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
