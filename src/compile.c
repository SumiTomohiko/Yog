#include "yog/config.h"
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
#include "yog/sysdeps.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"
#include "yog/gc/copying.h"

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
    VisitNode visit_class;
    VisitNode visit_dict;
    VisitNode visit_except;
    VisitNode visit_except_body;
    VisitNode visit_finally;
    VisitNode visit_from;
    VisitNode visit_func_call;
    VisitNode visit_func_def;
    VisitNode visit_if;
    VisitNode visit_import;
    VisitNode visit_imported_attr;
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

enum Context {
    CTX_BLOCK,
    CTX_CLASS, /* including module */
    CTX_FUNC,
    CTX_PKG,
};

typedef enum Context Context;

struct VarTable {
    enum Context ctx;
    YogVal inner_tbls;
    YogVal outer_tbl;
    YogVal vars;
};

typedef struct VarTable VarTable;

#define VAR_TABLE(v) PTR_AS(VarTable, (v))

struct Var {
    uint_t index;
    int_t flags;

    enum {
        VAR_GLOBAL,
        VAR_LOCAL,
        VAR_NONLOCAL
    } type;
    union {
        struct {
            uint_t index;
        } local;
        struct {
            uint_t level;
            union {
                uint_t index;
                ID name;
            } u;
        } nonlocal;
    } u;
};

typedef struct Var Var;

#define VAR(v) PTR_AS(Var, (v))

#define VAR_FLAG_ASSIGNED   (1 << 0)
#define VAR_FLAG_PARAM      (1 << 1)
#define VAR_FLAG_NONLOCAL   (1 << 2)
#define IS_ASSIGNED(flags)  ((flags) & VAR_FLAG_ASSIGNED)
#define IS_PARAM(flags)     ((flags) & VAR_FLAG_PARAM)
#define IS_NONLOCAL(flags)  ((flags) & VAR_FLAG_NONLOCAL)

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
    YogVal vars;
    YogVal const2index;
    YogVal last_inst;
    YogVal exc_tbl;
    YogVal exc_tbl_last;

    YogVal label_while_start;
    YogVal label_while_end;
    YogVal finally_list;
    YogVal try_list;

    YogVal filename;
    ID class_name;

    YogVal cur_stmt;
    BOOL interactive;
    int_t outer_depth;
    YogVal outer_data;
};

typedef struct CompileData CompileData;

#define COMPILE_DATA(v)     PTR_AS(CompileData, (v))

#define PUSH_EXCEPTION_TABLE_ENTRY(data, entry) do { \
    YogVal last = COMPILE_DATA((data))->exc_tbl_last; \
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(last), next, (entry)); \
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), exc_tbl_last, (entry)); \
} while (0)

#define NODE(p)             PTR_AS(YogNode, (p))
#define NODE_LINENO(node)   NODE((node))->lineno

static void
raise_error(YogEnv* env, YogVal filename, uint_t lineno, const char* fmt, ...)
{
    SAVE_ARG(env, filename);

#define BUFFER_SIZE     4096
    char head[BUFFER_SIZE];
    YogSysdeps_snprintf(head, array_sizeof(head), "file \"%s\", line %u: ", PTR_AS(YogCharArray, filename)->items, lineno);

    char s[BUFFER_SIZE];
    va_list ap;
    va_start(ap, fmt);
    YogSysdeps_vsnprintf(s, array_sizeof(s), fmt, ap);
    va_end(ap);

    char msg[BUFFER_SIZE];
    YogSysdeps_snprintf(msg, array_sizeof(msg), "%s%s", head, s);
#undef BUFFER_SIZE

    YogError_raise_SyntaxError(env, msg);

    /* NOTREACHED */
    RETURN_VOID(env);
}

static void
YogInst_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogVal inst = PTR2VAL(ptr);

    YogGC_KEEP(env, INST(inst), next, keeper, heap);

    if (INST(inst)->type != INST_OP) {
        return;
    }

    switch (INST(inst)->opcode) {
    case OP(JUMP):
        YogGC_KEEP(env, INST(inst), u.jump.dest, keeper, heap);
        break;
    case OP(JUMP_IF_TRUE):
        YogGC_KEEP(env, INST(inst), u.jump_if_true.dest, keeper, heap);
        break;
    case OP(JUMP_IF_FALSE):
        YogGC_KEEP(env, INST(inst), u.jump_if_false.dest, keeper, heap);
        break;
    case OP(JUMP_IF_DEFINED):
        YogGC_KEEP(env, INST(inst), u.jump_if_defined.dest, keeper, heap);
        break;
    default:
        break;
    }
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
    SAVE_ARGS2(env, data, inst);
    YogVal last_inst = YUNDEF;
    PUSH_LOCAL(env, last_inst);
    last_inst = COMPILE_DATA(data)->last_inst;

    YogGC_UPDATE_PTR(env, INST(last_inst), next, inst);
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), last_inst, inst);

    RETURN_VOID(env);
}

#include "compile.inc"

static void
TryListEntry_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    TryListEntry* entry = PTR_AS(TryListEntry, ptr);
#define KEEP(member)    YogGC_KEEP(env, entry, member, keeper, heap)
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
    YogGC_UPDATE_PTR(env, PTR_AS(TryListEntry, ent), node, node);
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
    YogGC_UPDATE_PTR(env, TRY_LIST_ENTRY(ent), prev, COMPILE_DATA(data)->try_list);
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), try_list, ent);

    RETURN(env, ent);
}

static void
CompileData_pop_try(YogEnv* env, YogVal data)
{
    SAVE_ARG(env, data);
    YogVal ent = YUNDEF;
    PUSH_LOCAL(env, ent);

    ent = COMPILE_DATA(data)->try_list;
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), try_list, TRY_LIST_ENTRY(ent)->prev);

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
    case NODE_BREAK:
        VISIT(break);
        break;
    case NODE_BLOCK_ARG:
        VISIT(block);
        break;
    case NODE_CLASS:
        VISIT(class);
        break;
    case NODE_DICT:
        VISIT(dict);
        break;
    case NODE_EXCEPT:
        VISIT(except);
        break;
    case NODE_EXCEPT_BODY:
        VISIT(except_body);
        break;
    case NODE_FINALLY:
        VISIT(finally);
        break;
    case NODE_FROM:
        VISIT(from);
        break;
    case NODE_FUNC_CALL:
        VISIT(func_call);
        break;
    case NODE_FUNC_DEF:
        VISIT(func_def);
        break;
    case NODE_IF:
        VISIT(if);
        break;
    case NODE_IMPORT:
        VISIT(import);
        break;
    case NODE_IMPORTED_ATTR:
        VISIT(imported_attr);
        break;
    case NODE_LITERAL:
        VISIT(literal);
        break;
    case NODE_LOGICAL_AND:
        VISIT(logical_and);
        break;
    case NODE_LOGICAL_OR:
        VISIT(logical_or);
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
    case NODE_NEXT:
        VISIT(next);
        break;
    case NODE_NONLOCAL:
        VISIT(nonlocal);
        break;
    case NODE_NOT:
        VISIT(not);
        break;
    case NODE_RAISE:
        VISIT(raise);
        break;
    case NODE_RETURN:
        VISIT(return);
        break;
    case NODE_SET:
        VISIT(set);
        break;
    case NODE_SUBSCRIPT:
        VISIT(subscript);
        break;
    case NODE_SUPER:
        VISIT(super);
        break;
    case NODE_VARIABLE:
        VISIT(variable);
        break;
    case NODE_WHILE:
        VISIT(while);
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
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), cur_stmt, stmt);
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
Var_new(YogEnv* env, int_t index, uint_t flags)
{
    SAVE_LOCALS(env);
    YogVal var = YUNDEF;
    PUSH_LOCAL(env, var);

    var = ALLOC_OBJ(env, NULL, NULL, Var);
    VAR(var)->index = index;
    VAR(var)->flags = flags;

    RETURN(env, var);
}

static void
register_var(YogEnv* env, YogVal tbl, ID name, int_t flags)
{
    SAVE_ARG(env, tbl);
    YogVal key = YUNDEF;
    YogVal val = YUNDEF;
    YogVal ent = YUNDEF;
    YogVal vars = YUNDEF;
    PUSH_LOCALS4(env, key, val, ent, vars);

    key = ID2VAL(name);
    vars = VAR_TABLE(tbl)->vars;
    if (YogTable_lookup(env, vars, key, &val)) {
        VAR(val)->flags |= flags;
        RETURN_VOID(env);
    }

    int_t index = YogTable_size(env, vars);
    ent = Var_new(env, index, flags);
    YogTable_add_direct(env, vars, key, ent);

    RETURN_VOID(env);
}

static void
register_var_as_param(YogEnv* env, YogVal tbl, ID name)
{
    SAVE_ARG(env, tbl);
    register_var(env, tbl, name, VAR_FLAG_PARAM);
    RETURN_VOID(env);
}

static void
register_var_as_assigned(YogEnv* env, YogVal tbl, ID name)
{
    SAVE_ARG(env, tbl);
    register_var(env, tbl, name, VAR_FLAG_ASSIGNED);
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
        register_var_as_assigned(env, data, id);
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
    register_var_as_assigned(env, data, id);

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
scan_var_visit_imported_attr(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal as = YUNDEF;
    PUSH_LOCAL(env, as);

    as = NODE(node)->u.imported_attr.as;
    if (IS_NIL(as)) {
        ID attr = NODE(node)->u.imported_attr.name;
        register_var_as_assigned(env, data, attr);
        RETURN_VOID(env);
    }

    YOG_ASSERT(env, IS_SYMBOL(as), "invalid \"as\" (0x%08x)", as);
    register_var_as_assigned(env, data, VAL2ID(as));
    RETURN_VOID(env);
}

static void
scan_var_visit_from(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal attrs = YUNDEF;
    YogVal attr = YUNDEF;
    PUSH_LOCALS2(env, attrs, attr);

    attrs = NODE(node)->u.from.attrs;
    uint_t size = YogArray_size(env, attrs);
    uint_t i;
    for (i = 0; i < size; i++) {
        attr = YogArray_at(env, attrs, i);
        visit_node(env, visitor, attr, data);
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
        register_var_as_assigned(env, data, id);
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
    YogVal name = YUNDEF;
    YogVal as = YUNDEF;
    PUSH_LOCALS2(env, name, as);

    as = NODE(node)->u.import.as;
    if (IS_SYMBOL(as)) {
        name = as;
    }
    else {
        YOG_ASSERT(env, IS_NIL(as), "invalid \"as\" (0x%08x)", as);
        name = YogArray_at(env, NODE(node)->u.import.name, 0);
    }
    register_var_as_assigned(env, data, name);

    RETURN_VOID(env);
}

static void
scan_var_visit_next(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    visit_array_elements(env, visitor, NODE(node)->u.next.exprs, data);
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
    register_var_as_assigned(env, data, name);

    RETURN_VOID(env);
}

static void
scan_var_visit_class(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    visit_decorators(env, visitor, NODE(node)->u.klass.decorators, data);

    ID name = NODE(node)->u.klass.name;
    register_var_as_assigned(env, data, name);

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
        register_var(env,  data, name, VAR_FLAG_NONLOCAL);
    }

    RETURN_VOID(env);
}

static void
scan_var_visit_variable(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    ID id = NODE(node)->u.variable.id;
    register_var(env, data, id, 0);
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
VarTable_add_child(YogEnv* env, YogVal self, YogVal child)
{
    SAVE_ARGS2(env, self, child);
    if (!IS_PTR(self)) {
        RETURN_VOID(env);
    }
    YogArray_push(env, VAR_TABLE(self)->inner_tbls, child);
    RETURN_VOID(env);
}

static void
VarTable_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    VarTable* tbl = (VarTable*)ptr;
#define KEEP(member) YogGC_KEEP(env, tbl, member, keeper, heap)
    KEEP(inner_tbls);
    KEEP(outer_tbl);
    KEEP(vars);
#undef KEEP
}

static YogVal
VarTable_new(YogEnv* env, Context ctx, YogVal outer)
{
    SAVE_ARG(env, outer);
    YogVal tbl = YUNDEF;
    YogVal inner_tbls = YUNDEF;
    YogVal vars = YUNDEF;
    PUSH_LOCALS3(env, tbl, inner_tbls, vars);

    tbl = ALLOC_OBJ(env, VarTable_keep_children, NULL, VarTable);
    VAR_TABLE(tbl)->ctx = ctx;
    VAR_TABLE(tbl)->inner_tbls = YUNDEF;
    YogGC_UPDATE_PTR(env, VAR_TABLE(tbl), outer_tbl, outer);
    VAR_TABLE(tbl)->vars = YUNDEF;

    inner_tbls = YogArray_new(env);
    YogGC_UPDATE_PTR(env, VAR_TABLE(tbl), inner_tbls, inner_tbls);
    vars = YogTable_new_symbol_table(env);
    YogGC_UPDATE_PTR(env, VAR_TABLE(tbl), vars, vars);

    RETURN(env, tbl);
}

static void
check_duplicate_argument(YogEnv* env, YogVal filename, uint_t lineno, ID name, YogVal tbl)
{
    SAVE_ARGS2(env, filename, tbl);
    if (!YogTable_lookup(env, VAR_TABLE(tbl)->vars, ID2VAL(name), NULL)) {
        RETURN_VOID(env);
    }
    raise_error(env, filename, lineno, "Duplicated argument name in function definition");
    RETURN_VOID(env);
}

static void
register_params(YogEnv* env, YogVal filename, YogVal params, YogVal var_tbl)
{
    if (!IS_PTR(params)) {
        return;
    }
    YOG_ASSERT(env, BASIC_OBJ_TYPE(params) == TYPE_ARRAY, "params must be Array");
    SAVE_ARGS3(env, filename, params, var_tbl);
    YogVal node = YUNDEF;
    PUSH_LOCAL(env, node);

    uint_t size = YogArray_size(env, params);
    uint_t i;
    for (i = 0; i < size; i++) {
        node = YogArray_at(env, params, i);
        uint_t lineno = NODE(node)->lineno;
        ID name = NODE(node)->u.param.name;
        check_duplicate_argument(env, filename, lineno, name, var_tbl);
        register_var_as_param(env, var_tbl, name);
    }

    RETURN_VOID(env);
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

static void
decide_param_type(YogEnv* env, YogVal var, uint_t* pnum)
{
    SAVE_ARG(env, var);
    if (!IS_PARAM(VAR(var)->flags)) {
        RETURN_VOID(env);
    }

    VAR(var)->type = VAR_LOCAL;
    VAR(var)->u.local.index = VAR(var)->index;
    (*pnum)++;
    RETURN_VOID(env);
}

static BOOL
is_visible(YogEnv* env, Context ctx, Context outer_ctx)
{
    if (((ctx == CTX_FUNC) || (ctx == CTX_BLOCK)) && (outer_ctx == CTX_CLASS)) {
        return FALSE;
    }
    return TRUE;
}

static BOOL
search_block_var_local_scope(YogEnv* env, ID name, YogVal outer_tbl, uint_t* plevel, uint_t* pindex, Context* pctx)
{
    SAVE_ARG(env, outer_tbl);
    YogVal tbl = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS2(env, tbl, val);

    int_t level = 1;
    tbl = outer_tbl;
    while ((VAR_TABLE(tbl)->ctx != CTX_PKG) && (VAR_TABLE(tbl)->ctx != CTX_CLASS)) {
#define CONTINUE \
    level++; \
    tbl = VAR_TABLE(tbl)->outer_tbl; \
    continue
        if (!YogTable_lookup(env, VAR_TABLE(tbl)->vars, ID2VAL(name), &val)) {
            CONTINUE;
        }
        Context outer_ctx = VAR_TABLE(tbl)->ctx;
        switch (VAR(val)->type) {
        case VAR_GLOBAL:
            RETURN(env, FALSE);
            break;
        case VAR_LOCAL:
            *plevel = level;
            *pindex = VAR(val)->u.local.index;
            *pctx = outer_ctx;
            RETURN(env, TRUE);
            break;
        case VAR_NONLOCAL:
            *plevel = level + VAR(val)->u.nonlocal.level;
            if ((outer_ctx == CTX_FUNC) || (outer_ctx == CTX_BLOCK)) {
                *pindex = VAR(val)->u.nonlocal.u.index;
            }
            *pctx = outer_ctx;
            RETURN(env, TRUE);
            break;
        default:
            YOG_BUG(env, "Unknown variable type (0x%x)", VAR(val)->type);
            break;
        }
        CONTINUE;
#undef CONTINUE
    }
    if (YogTable_lookup(env, VAR_TABLE(tbl)->vars, ID2VAL(name), &val) && (VAR(val)->type == VAR_LOCAL)) {
        *plevel = level;
        *pctx = VAR_TABLE(tbl)->ctx;
        RETURN(env, TRUE);
    }
    RETURN(env, FALSE);
}

static BOOL
find_outer_var(YogEnv* env, ID name, Context ctx, YogVal outer_tbl, uint_t* plevel, uint_t* pindex, Context* pctx)
{
    SAVE_ARG(env, outer_tbl);
    YogVal tbl = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS2(env, tbl, val);

    int_t level = 0;
    tbl = outer_tbl;
    Context outer_ctx;
    while (IS_PTR(tbl) && (VAR_TABLE(tbl)->ctx != CTX_PKG)) {
        outer_ctx = VAR_TABLE(tbl)->ctx;
        if (is_visible(env, ctx, outer_ctx)) {
            val = YUNDEF;
            if (YogTable_lookup(env, VAR_TABLE(tbl)->vars, ID2VAL(name), &val)) {
                *pctx = outer_ctx;
                switch (VAR(val)->type) {
                case VAR_GLOBAL:
                    RETURN(env, FALSE);
                    break;
                case VAR_LOCAL:
                    *plevel = level + 1;
                    *pindex = VAR(val)->u.local.index;
                    RETURN(env, TRUE);
                    break;
                case VAR_NONLOCAL:
                    *plevel = level + VAR(val)->u.nonlocal.level + 1;
                    if ((outer_ctx == CTX_FUNC) || (outer_ctx == CTX_BLOCK)) {
                        *pindex = VAR(val)->u.nonlocal.u.index;
                    }
                    RETURN(env, TRUE);
                    break;
                default:
                    YOG_BUG(env, "Unknown variable type (0x%x)", VAR(val)->type);
                    break;
                }
            }
        }

        level++;
        tbl = VAR_TABLE(tbl)->outer_tbl;
    }

    RETURN(env, FALSE);
}

static void
Var_set_global(YogEnv* env, YogVal var)
{
    VAR(var)->type = VAR_GLOBAL;
}

static void
Var_set_nonlocal(YogEnv* env, YogVal var, uint_t level, uint_t index, ID name, Context ctx)
{
    SAVE_ARG(env, var);
    VAR(var)->type = VAR_NONLOCAL;
    VAR(var)->u.nonlocal.level = level;
    if ((ctx == CTX_FUNC) || (ctx == CTX_BLOCK)) {
        VAR(var)->u.nonlocal.u.index = index;
        RETURN_VOID(env);
    }
    VAR(var)->u.nonlocal.u.name = name;
    RETURN_VOID(env);
}

static void
compute_nonlocal_depth(YogEnv* env, ID name, YogVal var, Context ctx, YogVal outer_tbl)
{
    SAVE_ARGS2(env, var, outer_tbl);

    uint_t level;
    uint_t index;
    Context outer_ctx;
    if (!find_outer_var(env, name, ctx, outer_tbl, &level, &index, &outer_ctx)) {
        Var_set_global(env, var);
        RETURN_VOID(env);
    }
    Var_set_nonlocal(env, var, level, index, name, outer_ctx);
    RETURN_VOID(env);
}

static BOOL
is_var_assigned_in_inner_scope(YogEnv* env, ID name, YogVal tbl)
{
    SAVE_ARG(env, tbl);
    YogVal vars = YUNDEF;
    YogVal var = YUNDEF;
    YogVal inner_tbls = YUNDEF;
    YogVal inner_tbl = YUNDEF;
    PUSH_LOCALS4(env, vars, var, inner_tbls, inner_tbl);

    inner_tbls = VAR_TABLE(tbl)->inner_tbls;
    uint_t size = YogArray_size(env, inner_tbls);
    uint_t i;
    for (i = 0; i < size; i++) {
        inner_tbl = YogArray_at(env, inner_tbls, i);
        vars = VAR_TABLE(inner_tbl)->vars;
        var = YUNDEF;
        if (YogTable_lookup(env, vars, ID2VAL(name), &var)) {
            if (IS_PARAM(VAR(var)->flags)) {
                continue;
            }
            if (IS_ASSIGNED(VAR(var)->flags)) {
                RETURN(env, TRUE);
            }
        }
        if (is_var_assigned_in_inner_scope(env, name, inner_tbl)) {
            RETURN(env, TRUE);
        }
    }

    RETURN(env, FALSE);
}

static void
decide_auto_var_type(YogEnv* env, ID name, YogVal var, YogVal tbl, uint_t* pindex)
{
    SAVE_ARGS2(env, var, tbl);
    YogVal outer = YUNDEF;
    PUSH_LOCAL(env, outer);
    int_t flags = VAR(var)->flags;
    if (IS_PARAM(flags)) {
        RETURN_VOID(env);
    }

    Context ctx = VAR_TABLE(tbl)->ctx;
    outer = VAR_TABLE(tbl)->outer_tbl;
    if (IS_NONLOCAL(flags)) {
        compute_nonlocal_depth(env, name, var, ctx, outer);
        RETURN_VOID(env);
    }

    if (ctx == CTX_BLOCK) {
        uint_t level;
        /**
         * gcc complains that "'index' may be used uninitialized in this
         * function".
         */
        uint_t index = 0;
        Context outer_ctx;
        if (!search_block_var_local_scope(env, name, outer, &level, &index, &outer_ctx)) {
            Var_set_global(env, var);
            RETURN_VOID(env);
        }
        Var_set_nonlocal(env, var, level, index, name, outer_ctx);
        RETURN_VOID(env);
    }

    if (!IS_ASSIGNED(flags)) {
        uint_t level;
        uint_t index;
        Context outer_ctx;
        if (find_outer_var(env, name, ctx, outer, &level, &index, &outer_ctx)) {
            Var_set_nonlocal(env, var, level, index, name, outer_ctx);
            RETURN_VOID(env);
        }
        Var_set_global(env, var);
        RETURN_VOID(env);
    }

    VAR(var)->type = VAR_LOCAL;
    VAR(var)->u.local.index = *pindex;
    (*pindex)++;
    RETURN_VOID(env);
}

static void
decide_var_type(YogEnv* env, YogVal tbl)
{
    SAVE_ARG(env, tbl);
    YogVal iter = YUNDEF;
    YogVal entry = YUNDEF;
    YogVal vars = YUNDEF;
    YogVal inner_tbls = YUNDEF;
    YogVal inner_tbl = YUNDEF;
    PUSH_LOCALS5(env, iter, entry, vars, inner_tbls, inner_tbl);

    vars = VAR_TABLE(tbl)->vars;
    uint_t params_num = 0;
    iter = YogTable_get_iterator(env, vars);
    while (YogTableIterator_next(env, iter)) {
        entry = YogTableIterator_current_value(env, iter);
        decide_param_type(env, entry, &params_num);
    }

    uint_t index = params_num;
    iter = YogTable_get_iterator(env, vars);
    while (YogTableIterator_next(env, iter)) {
        ID name = VAL2ID(YogTableIterator_current_key(env, iter));
        entry = YogTableIterator_current_value(env, iter);
        decide_auto_var_type(env, name, entry, tbl, &index);
    }

    inner_tbls = VAR_TABLE(tbl)->inner_tbls;
    uint_t size = YogArray_size(env, inner_tbls);
    uint_t i;
    for (i = 0; i < size; i++) {
        inner_tbl = YogArray_at(env, inner_tbls, i);
        decide_var_type(env, inner_tbl);
    }

    RETURN_VOID(env);
}

static void scan_var_init_visitor(AstVisitor*);

static void
register_self(YogEnv* env, YogVal var_tbl)
{
    SAVE_ARG(env, var_tbl);

    ID name = YogVM_intern(env, env->vm, "self");
    register_var_as_param(env, var_tbl, name);

    RETURN_VOID(env);
}

static YogVal
make_var_table(YogEnv* env, YogVal filename, Context ctx, YogVal params, YogVal stmts, YogVal outer)
{
    SAVE_ARGS4(env, filename, params, stmts, outer);
    YogVal tbl = YUNDEF;
    PUSH_LOCAL(env, tbl);
    AstVisitor visitor;
    scan_var_init_visitor(&visitor);

    tbl = VarTable_new(env, ctx, outer);
    VarTable_add_child(env, outer, tbl);
    if (ctx == CTX_FUNC) {
        register_self(env, tbl);
    }
    register_params(env, filename, params, tbl);
    scan_defaults(env, &visitor, params, tbl);
    visitor.visit_stmts(env, &visitor, stmts, tbl);

    RETURN(env, tbl);
}

static void
scan_var_visit_block(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal params = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal tbl = YUNDEF;
    PUSH_LOCALS3(env, params, stmts, tbl);

    params = NODE(node)->u.blockarg.params;
    stmts = NODE(node)->u.blockarg.stmts;
    tbl = make_var_table(env, YNIL, CTX_BLOCK, params, stmts, data);
    YogGC_UPDATE_PTR(env, NODE(node), u.blockarg.var_tbl, tbl);

    RETURN_VOID(env);
}

static void
scan_var_init_visitor(AstVisitor* visitor)
{
    visitor->visit_array = scan_var_visit_array;
    visitor->visit_assign = scan_var_visit_assign;
    visitor->visit_attr = scan_var_visit_attr;
    visitor->visit_block = scan_var_visit_block;
    visitor->visit_break = scan_var_visit_break;
    visitor->visit_class = scan_var_visit_class;
    visitor->visit_dict = scan_var_visit_dict;
    visitor->visit_except = scan_var_visit_except;
    visitor->visit_except_body = scan_var_visit_except_body;
    visitor->visit_finally = scan_var_visit_finally;
    visitor->visit_from = scan_var_visit_from;
    visitor->visit_func_call = scan_var_visit_func_call;
    visitor->visit_func_def = scan_var_visit_func_def;
    visitor->visit_if = scan_var_visit_if;
    visitor->visit_import = scan_var_visit_import;
    visitor->visit_imported_attr = scan_var_visit_imported_attr;
    visitor->visit_literal = NULL;
    visitor->visit_logical_and = scan_var_visit_logical_and;
    visitor->visit_logical_or = scan_var_visit_logical_or;
    visitor->visit_module = scan_var_visit_module;
    visitor->visit_multi_assign = scan_var_visit_multi_assign;
    visitor->visit_multi_assign_lhs = scan_var_visit_multi_assign_lhs;
    visitor->visit_next = scan_var_visit_next;
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

static Context
CompileData_get_context(YogEnv* env, YogVal self)
{
    return VAR_TABLE(COMPILE_DATA(self)->vars)->ctx;
}

static YogVal
CompileData_get_var_table(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal tbl = YUNDEF;
    PUSH_LOCAL(env, tbl);
    tbl = VAR_TABLE(COMPILE_DATA(self)->vars)->vars;
    RETURN(env, tbl);
}

static Context
CompileData_get_outer_context(YogEnv* env, YogVal data, uint_t level)
{
    SAVE_ARG(env, data);
    YogVal tbl = YUNDEF;
    PUSH_LOCAL(env, tbl);

    tbl = COMPILE_DATA(data)->vars;
    uint_t i;
    for (i = 0; i < level; i++) {
        tbl = VAR_TABLE(tbl)->outer_tbl;
    }
    RETURN(env, VAR_TABLE(tbl)->ctx);
}

static void
append_store(YogEnv* env, YogVal data, uint_t lineno, ID name)
{
    SAVE_ARG(env, data);
    YogVal var = YUNDEF;
    YogVal s = YUNDEF;
    PUSH_LOCALS2(env, var, s);

    uint_t level;
    uint_t index;
    Context ctx;
    switch (CompileData_get_context(env, data)) {
    case CTX_BLOCK:
    case CTX_FUNC:
        var = lookup_var(env, CompileData_get_var_table(env, data), name);
        if (!IS_PTR(var)) {
            s = YogVM_id2name(env, env->vm, name);
            YOG_BUG(env, "variable not found (%s)", STRING_CSTR(s));
        }
        switch (VAR(var)->type) {
        case VAR_GLOBAL:
            CompileData_add_store_global(env, data, lineno, name);
            break;
        case VAR_LOCAL:
            index = VAR(var)->u.local.index;
            CompileData_add_store_local_index(env, data, lineno, index);
            break;
        case VAR_NONLOCAL:
            level = VAR(var)->u.nonlocal.level;
            ctx = CompileData_get_outer_context(env, data, level);
            if ((ctx == CTX_FUNC) || (ctx == CTX_BLOCK)) {
                index = VAR(var)->u.nonlocal.u.index;
                CompileData_add_store_nonlocal_index(env, data, lineno, level, index);
            }
            else {
                ID name = VAR(var)->u.nonlocal.u.name;
                CompileData_add_store_nonlocal_name(env, data, lineno, level, name);
            }
            break;
        default:
            YOG_BUG(env, "unknown VarType (%d)", VAR(var)->type);
            break;
        }
        break;
    case CTX_CLASS:
    case CTX_PKG:
        CompileData_add_store_local_name(env, data, lineno, name);
        break;
    default:
        YOG_BUG(env, "Unknown context.");
        break;
    }

    RETURN_VOID(env);
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
        YogGC_UPDATE_PTR(env, COMPILE_DATA(data), const2index, const2index);
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
#define KEEP(member)    YogGC_KEEP(env, entry, member, keeper, heap)
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
    YogVal and_block = YUNDEF;
    PUSH_LOCALS2(env, label_break_end, and_block);

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
    YogVal middle = YUNDEF;
    YogVal head = YUNDEF;
    PUSH_LOCALS6(env, lhs, left, right, rhs, middle, head);

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

        YogGC_UPDATE_PTR(env, CODE(code), exc_tbl, exc_tbl);
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

    YogGC_UPDATE_PTR(env, CODE(code), lineno_tbl, tbl);
    CODE(code)->lineno_tbl_size = size;

    RETURN_VOID(env);
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
            uint_t size = Yog_get_inst_size(INST(inst)->opcode);
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
    if (VAR(val)->type == VAR_LOCAL) {
        int_t nlocals = VAL2INT(*arg);
        *arg = INT2VAL(nlocals + 1);
    }

    return ST_CONTINUE;
}

static int_t
count_locals(YogEnv* env, YogVal tbl)
{
    YogVal val = INT2VAL(0);
    YogTable_foreach(env, VAR_TABLE(tbl)->vars, count_locals_callback, &val);

    return VAL2INT(val);
}

static void
CompileData_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    CompileData* data = PTR_AS(CompileData, ptr);
#define KEEP(member)    YogGC_KEEP(env, data, member, keeper, heap)
    KEEP(vars);
    KEEP(const2index);
    KEEP(last_inst);
    KEEP(exc_tbl);
    KEEP(exc_tbl_last);
    KEEP(label_while_start);
    KEEP(label_while_end);
    KEEP(finally_list);
    KEEP(try_list);
    KEEP(filename);
    KEEP(cur_stmt);
    KEEP(outer_data);
#undef KEEP
}

static YogVal
CompileData_new(YogEnv* env, YogVal vars, YogVal anchor, YogVal exc_tbl_ent, YogVal filename, ID class_name, YogVal outer_data, BOOL interactive)
{
    SAVE_ARGS5(env, vars, anchor, exc_tbl_ent, filename, outer_data);

    YogVal data = ALLOC_OBJ(env, CompileData_keep_children, NULL, CompileData);
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), vars, vars);
    COMPILE_DATA(data)->const2index = YUNDEF;
    COMPILE_DATA(data)->label_while_start = YUNDEF;
    COMPILE_DATA(data)->label_while_end = YUNDEF;
    COMPILE_DATA(data)->finally_list = YUNDEF;
    COMPILE_DATA(data)->try_list = YUNDEF;
    COMPILE_DATA(data)->class_name = INVALID_ID;
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), last_inst, anchor);
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), exc_tbl, exc_tbl_ent);
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), exc_tbl_last, exc_tbl_ent);
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), filename, filename);
    COMPILE_DATA(data)->class_name = class_name;
    COMPILE_DATA(data)->cur_stmt = YUNDEF;
    COMPILE_DATA(data)->interactive = interactive;
    COMPILE_DATA(data)->outer_depth = 0;
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), outer_data, outer_data);

    RETURN(env, data);
}

static int_t
get_max_outer_level_callback(YogEnv* env, YogVal key, YogVal val, YogVal* arg)
{
    if (VAR(val)->type == VAR_NONLOCAL) {
        int_t max_level = VAL2INT(*arg);
        int_t level = VAR(val)->u.nonlocal.level;
        if (max_level < level) {
            *arg = INT2VAL(level);
        }
    }

    return ST_CONTINUE;
}

static int_t
get_max_outer_level(YogEnv* env, YogVal tbl)
{
    YogVal arg = INT2VAL(0);
    YogTable_foreach(env, VAR_TABLE(tbl)->vars, get_max_outer_level_callback, &arg);

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
    if (VAR(val)->type == VAR_LOCAL) {
        uint_t index = VAR(val)->u.local.index;
        YOG_ASSERT(env, index < ALLOC_LOCAL_VARS_TABLE_ARG(*arg)->count, "local var index over count (0x%08x, 0x%08x)", index, ALLOC_LOCAL_VARS_TABLE_ARG(*arg)->count);
        YogVal names = ALLOC_LOCAL_VARS_TABLE_ARG(*arg)->names;
        PTR_AS(ID, names)[index] = VAL2ID(key);
    }

    return ST_CONTINUE;
}

static void
AllocLocalVarsTableArg_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    AllocLocalVarsTableArg* arg = PTR_AS(AllocLocalVarsTableArg, ptr);
    YogGC_KEEP(env, arg, names, keeper, heap);
}

static YogVal
AllocLocalVarsTableArg_new(YogEnv* env, YogVal names, uint_t count)
{
    SAVE_ARG(env, names);
    YogVal arg = YUNDEF;
    PUSH_LOCAL(env, arg);

    arg = ALLOC_OBJ(env, AllocLocalVarsTableArg_keep_children, NULL, AllocLocalVarsTableArg);
    YogGC_UPDATE_PTR(env, PTR_AS(AllocLocalVarsTableArg, arg), names, names);
    PTR_AS(AllocLocalVarsTableArg, arg)->count = count;

    RETURN(env, arg);
}

static ID*
alloc_local_vars_table(YogEnv* env, YogVal tbl, uint_t count)
{
    SAVE_ARG(env, tbl);
    YogVal names = YUNDEF;
    YogVal arg = YUNDEF;
    PUSH_LOCALS2(env, names, arg);

    names = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(ID) * count);
    arg = AllocLocalVarsTableArg_new(env, names, count);

    YogTable_foreach(env, VAR_TABLE(tbl)->vars, alloc_local_vars_table_callback, &arg);

    RETURN(env, PTR_AS(ID, names));
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
    var = lookup_var(env, CompileData_get_var_table(env, data), name);
    YOG_ASSERT(env, VAR(var)->type == VAR_LOCAL, "invalid variable type (0x%08x)", VAR(var)->type);
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

static void
update_outer_depth(YogEnv* env, YogVal data, int_t depth)
{
    if (!IS_PTR(data)) {
        return;
    }
    SAVE_ARG(env, data);
    if (COMPILE_DATA(data)->outer_depth < depth) {
        COMPILE_DATA(data)->outer_depth = depth;
        update_outer_depth(env, COMPILE_DATA(data)->outer_data, depth - 1);
    }
    RETURN_VOID(env);
}

static YogVal
compile_stmts(YogEnv* env, AstVisitor* visitor, YogVal filename, ID class_name, ID func_name, YogVal params, YogVal stmts, YogVal vars, YogVal outer_data, BOOL interactive)
{
    SAVE_ARGS5(env, filename, params, stmts, vars, outer_data);
    YogVal anchor = YUNDEF;
    YogVal exc_tbl_ent = YUNDEF;
    YogVal data = YUNDEF;
    YogVal label_return_start = YUNDEF;
    YogVal last_inst = YUNDEF;
    PUSH_LOCALS5(env, anchor, exc_tbl_ent, data, label_return_start, last_inst);

    anchor = Anchor_new(env);
    exc_tbl_ent = ExceptionTableEntry_new(env);
    data = CompileData_new(env, vars, anchor, exc_tbl_ent, filename, class_name, outer_data, interactive);

    label_return_start = Label_new(env);
    add_inst(env, data, label_return_start);

    compile_defaults(env, visitor, params, data);
    visitor->visit_stmts(env, visitor, stmts, data);
    last_inst = COMPILE_DATA(data)->last_inst;
    if (INST(last_inst)->opcode != OP(RET)) {
        uint_t lineno = INST(last_inst)->lineno;
        CompileData_add_ret_nil(env, data, lineno);
    }

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
    YogGC_UPDATE_PTR(env, CODE(code), consts, consts);
    YogGC_UPDATE_PTR(env, CODE(code), insts, PTR_AS(YogBinary, bin)->body);
    int_t outer_depth = get_max_outer_level(env, vars);
    update_outer_depth(env, outer_data, outer_depth - 1);
    CODE(code)->outer_size = outer_depth < COMPILE_DATA(data)->outer_depth ? COMPILE_DATA(data)->outer_depth : outer_depth;

    make_exception_table(env, code, data);
    make_lineno_table(env, code, anchor);

    YogGC_UPDATE_PTR(env, CODE(code), filename, filename);
    CODE(code)->class_name = class_name;
    CODE(code)->func_name = func_name;

#if 0 && !defined(MINIYOG)
    TRACE("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
    YogCode_dump(env, code);
#endif

    RETURN(env, code);
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
    YogGC_UPDATE_PTR(env, CODE(code), arg_info, arg_info);

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
    YogGC_UPDATE_PTR(env, ARG_INFO(arg_info), argnames, argnames);
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
register_inner_var_as_assigned(YogEnv* env, YogVal tbl, ID name, YogVal var)
{
    SAVE_ARGS2(env, tbl, var);

    if (IS_PARAM(VAR(var)->flags)) {
        RETURN_VOID(env);
    }
    if (!IS_ASSIGNED(VAR(var)->flags)) {
        RETURN_VOID(env);
    }
    register_var_as_assigned(env, tbl, name);

    RETURN_VOID(env);
}

static void put_vars_assigned_in_block(YogEnv*, YogVal, YogVal);

static void
put_vars_assigned_in_inner_scope(YogEnv* env, YogVal tbl, YogVal inner_tbl)
{
    SAVE_ARGS2(env, tbl, inner_tbl);
    YogVal t = YUNDEF;
    PUSH_LOCAL(env, t);

    uint_t size = YogArray_size(env, VAR_TABLE(inner_tbl)->inner_tbls);
    uint_t i;
    for (i = 0; i < size; i++) {
        t = YogArray_at(env, VAR_TABLE(inner_tbl)->inner_tbls, i);
        put_vars_assigned_in_block(env, tbl, t);
    }

    RETURN_VOID(env);
}

static void
put_vars_assigned_in_block(YogEnv* env, YogVal tbl, YogVal inner_tbl)
{
    SAVE_ARGS2(env, tbl, inner_tbl);
    YogVal iter = YUNDEF;
    YogVal key = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS3(env, iter, key, val);

    iter = YogTable_get_iterator(env, VAR_TABLE(inner_tbl)->vars);
    while (YogTableIterator_next(env, iter)) {
        key = YogTableIterator_current_key(env, iter);
        val = YogTableIterator_current_value(env, iter);
        register_inner_var_as_assigned(env, tbl, VAL2ID(key), val);
    }
    put_vars_assigned_in_inner_scope(env, tbl, inner_tbl);

    RETURN_VOID(env);
}

static void
put_vars_assigned_in_block_to_top(YogEnv* env, YogVal tbl)
{
    SAVE_ARG(env, tbl);
    put_vars_assigned_in_inner_scope(env, tbl, tbl);
    RETURN_VOID(env);
}

static YogVal
make_var_table_of_func(YogEnv* env, YogVal filename, Context ctx, YogVal params, YogVal stmts, YogVal outer_vars)
{
    SAVE_ARGS4(env, filename, params, stmts, outer_vars);
    YogVal tbl = YUNDEF;
    PUSH_LOCAL(env, tbl);

    tbl = make_var_table(env, filename, ctx, params, stmts, outer_vars);
    put_vars_assigned_in_block_to_top(env, tbl);

    RETURN(env, tbl);
}

static YogVal
compile_func(YogEnv* env, AstVisitor* visitor, YogVal filename, ID class_name, YogVal node, YogVal outer_data)
{
    SAVE_ARGS3(env, filename, node, outer_data);
    YogVal var_tbl = YUNDEF;
    YogVal params = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal outer_vars = YUNDEF;
    YogVal code = YUNDEF;
    PUSH_LOCALS5(env, var_tbl, params, stmts, outer_vars, code);

    params = NODE(node)->u.funcdef.params;
    stmts = NODE(node)->u.funcdef.stmts;
    outer_vars = COMPILE_DATA(outer_data)->vars;
    var_tbl = make_var_table_of_func(env, filename, CTX_FUNC, params, stmts, outer_vars);
    decide_var_type(env, var_tbl);

    ID func_name = NODE(node)->u.funcdef.name;

    code = compile_stmts(env, visitor, filename, class_name, func_name, params, stmts, var_tbl, outer_data, FALSE);
    setup_params(env, var_tbl, params, code);

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
    YogVal code = YUNDEF;
    YogVal decorators = YUNDEF;
    PUSH_LOCALS2(env, code, decorators);

    ID class_name = CompileData_get_context(env, data) == CTX_CLASS ? COMPILE_DATA(data)->class_name : INVALID_ID;

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
    uint_t level;
    uint_t index;
    Context ctx;
    switch (CompileData_get_context(env, data)) {
    case CTX_BLOCK:
    case CTX_FUNC:
        var = lookup_var(env, CompileData_get_var_table(env, data), id);
        YOG_ASSERT(env, IS_PTR(var), "can't find variable");
        switch (VAR(var)->type) {
        case VAR_GLOBAL:
            CompileData_add_load_global(env, data, lineno, id);
            break;
        case VAR_LOCAL:
            index = VAR(var)->u.local.index;
            CompileData_add_load_local_index(env, data, lineno, index);
            break;
        case VAR_NONLOCAL:
            level = VAR(var)->u.nonlocal.level;
            ctx = CompileData_get_outer_context(env, data, level);
            if ((ctx == CTX_FUNC) || (ctx == CTX_BLOCK)) {
                index = VAR(var)->u.nonlocal.u.index;
                CompileData_add_load_nonlocal_index(env, data, lineno, level, index);
            }
            else {
                ID name = VAR(var)->u.nonlocal.u.name;
                CompileData_add_load_nonlocal_name(env, data, lineno, level, name);
            }
            break;
        default:
            YOG_BUG(env, "unknown variable type (0x%x)", VAR(var)->type);
            break;
        }
        break;
    case CTX_CLASS:
    case CTX_PKG:
        CompileData_add_load_local_name(env, data, lineno, id);
        break;
    default:
        YOG_BUG(env, "Unknown context.");
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
#define KEEP(member)    YogGC_KEEP(env, ent, member, keeper, heap)
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
    YogGC_UPDATE_PTR(env, FINALLY_LIST_ENTRY(finally_list_entry), prev, COMPILE_DATA(data)->finally_list);
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), finally_list, finally_list_entry);
    YogGC_UPDATE_PTR(env, FINALLY_LIST_ENTRY(finally_list_entry), node, node);

    try_list_entry = CompileData_push_try(env, data, node);

    exc_tbl_entry = ExceptionTableEntry_new(env);
    EXCEPTION_TABLE_ENTRY(exc_tbl_entry)->next = YNIL;
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(exc_tbl_entry), from, label_head_start);
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(exc_tbl_entry), to, label_head_end);
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(exc_tbl_entry), target, label_finally_error_start);
    YogGC_UPDATE_PTR(env, TRY_LIST_ENTRY(try_list_entry), exc_tbl, exc_tbl_entry);

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

    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), finally_list, FINALLY_LIST_ENTRY(finally_list_entry)->prev);

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
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(exc_tbl_entry), from, label_head_start);
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(exc_tbl_entry), to, label_head_end);
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(exc_tbl_entry), target, label_excepts_start);
    YogGC_UPDATE_PTR(env, TRY_LIST_ENTRY(try_list_entry), exc_tbl, exc_tbl_entry);

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
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), label_while_start, while_start);
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), label_while_end, while_end);

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

    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), label_while_end, label_while_end_prev);
    YogGC_UPDATE_PTR(env, COMPILE_DATA(data), label_while_start, label_while_start_prev);

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
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(new_entry), from, label_to);
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(new_entry), to, EXCEPTION_TABLE_ENTRY(entry)->to);
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(new_entry), target, EXCEPTION_TABLE_ENTRY(entry)->target);
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(entry), to, label_from);
    YogGC_UPDATE_PTR(env, EXCEPTION_TABLE_ENTRY(entry), next, new_entry);

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
    YogVal exprs = YUNDEF;
    PUSH_LOCAL(env, exprs);

    YogVal label_while_start = COMPILE_DATA(data)->label_while_start;
    if (IS_PTR(label_while_start)) {
        compile_while_jump(env, visitor, node, data, label_while_start);
        RETURN_VOID(env);
    }

    uint_t n;
    uint_t lineno = NODE(node)->lineno;
    exprs = NODE(node)->u.next.exprs;
    if (IS_PTR(exprs)) {
        visit_array_elements_opposite_order(env, visitor, exprs, data);
        n = YogArray_size(env, exprs);
    }
    else {
        add_push_const(env, data, YNIL, lineno);
        n = 1;
    }
    CompileData_add_ret(env, data, lineno, n);

    RETURN_VOID(env);
}

static YogVal
compile_block(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal params = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal vars = YUNDEF;
    YogVal code = YUNDEF;
    YogVal var_tbl = YUNDEF;
    YogVal filename = YUNDEF;
    PUSH_LOCALS6(env, params, stmts, vars, code, var_tbl, filename);

    filename = COMPILE_DATA(data)->filename;
    ID class_name = INVALID_ID;
    ID func_name = YogVM_intern(env, env->vm, "<block>");
    params = NODE(node)->u.blockarg.params;
    stmts = NODE(node)->u.blockarg.stmts;
    var_tbl = NODE(node)->u.blockarg.var_tbl;
    code = compile_stmts(env, visitor, filename, class_name, func_name, params, stmts, var_tbl, data, FALSE);
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
    YogVal filename = YUNDEF;
    YogVal outer_vars = YUNDEF;
    PUSH_LOCALS3(env, var_tbl, filename, outer_vars);

    filename = COMPILE_DATA(data)->filename;
    outer_vars = COMPILE_DATA(data)->vars;
    var_tbl = make_var_table_of_func(env, filename, CTX_CLASS, YNIL, stmts, outer_vars);
    decide_var_type(env, var_tbl);

    ID func_name = INVALID_ID;
    YogVal code = compile_stmts(env, visitor, filename, class_name, func_name, YUNDEF, stmts, var_tbl, data, FALSE);

    RETURN(env, code);
}

static YogVal
compile_module(YogEnv* env, AstVisitor* visitor, ID name, YogVal stmts, uint_t lineno, YogVal data)
{
    SAVE_ARGS2(env, stmts, data);
    YogVal var_tbl = YUNDEF;
    YogVal code = YUNDEF;
    YogVal filename = YUNDEF;
    YogVal outer_vars = YUNDEF;
    PUSH_LOCALS4(env, var_tbl, code, filename, outer_vars);

    filename = COMPILE_DATA(data)->filename;
    outer_vars = COMPILE_DATA(data)->vars;
    var_tbl = make_var_table_of_func(env, filename, CTX_CLASS, YNIL, stmts, outer_vars);
    decide_var_type(env, var_tbl);

    ID class_name = name;
    ID func_name = YogVM_intern(env, env->vm, "<module>");

    code = compile_stmts(env, visitor, filename, class_name, func_name, YUNDEF, stmts, var_tbl, data, FALSE);

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
    CompileData_add_pop(env, data, lineno);
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
    if (CompileData_get_context(env, data) == CTX_BLOCK) {
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
    char* pkg = (char*)YogSysdeps_alloca(sizeof(char) * (len + 1));
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

    RETURN(env, YogString_from_str(env, pkg));
}

static void
compile_import(YogEnv* env, YogVal data, uint_t lineno, YogVal name)
{
    SAVE_ARGS2(env, name, data);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = join_package_names(env, name);
    uint_t c = register_const(env, data, pkg);
    CompileData_add_push_const(env, data, lineno, c);

    ID import_package = YogVM_intern(env, env->vm, "import_package");
    CompileData_add_load_global(env, data, lineno, import_package);
    CompileData_add_call_function(env, data, lineno, 1, 0, 0, 0, 0, 1, 0, 0);

    RETURN_VOID(env);
}

static void
compile_load_pkg_attr(YogEnv* env, YogVal data, uint_t lineno, YogVal name)
{
    SAVE_ARGS2(env, name, data);
    YogVal attr = YUNDEF;
    PUSH_LOCAL(env, attr);

    uint_t size = YogArray_size(env, name);
    uint_t i;
    for (i = 1; i < size; i++) {
        attr = YogArray_at(env, name, i);
        CompileData_add_load_attr(env, data, lineno, VAL2ID(attr));
    }

    RETURN_VOID(env);
}

static void
compile_store_pkg_attr(YogEnv* env, YogVal data, uint_t lineno, YogVal attrs, uint_t index)
{
    SAVE_ARGS2(env, attrs, data);
    YogVal attr = YUNDEF;
    YogVal as = YUNDEF;
    PUSH_LOCALS2(env, attr, as);

    attr = YogArray_at(env, attrs, index);
    YOG_ASSERT(env, NODE(attr)->type == NODE_IMPORTED_ATTR, "invalid node type (0x%x)", NODE(attr)->type);
    ID name = NODE(attr)->u.imported_attr.name;
    CompileData_add_load_attr(env, data, lineno, name);
    as = NODE(attr)->u.imported_attr.as;
    if (IS_NIL(as)) {
        append_store(env, data, lineno, name);
        RETURN_VOID(env);
    }

    YOG_ASSERT(env, IS_SYMBOL(as), "invalid \"as\" (0x%08x)", as);
    append_store(env, data, lineno, VAL2ID(as));
    RETURN_VOID(env);
}

static void
compile_visit_from(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal pkg = YUNDEF;
    YogVal attrs = YUNDEF;
    PUSH_LOCALS2(env, pkg, attrs);

    uint_t lineno = NODE_LINENO(node);
    pkg = NODE(node)->u.from.pkg;
    compile_import(env, data, lineno, pkg);
    compile_load_pkg_attr(env, data, lineno, pkg);

    attrs = NODE(node)->u.from.attrs;
    uint_t size = YogArray_size(env, attrs);
    uint_t i;
    for (i = 0; i < size - 1; i++) {
        CompileData_add_dup(env, data, lineno);
        compile_store_pkg_attr(env, data, lineno, attrs, i);
    }
    compile_store_pkg_attr(env, data, lineno, attrs, size - 1);

    RETURN_VOID(env);
}

static void
compile_visit_import(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);
    YogVal name = YUNDEF;
    YogVal as = YUNDEF;
    YogVal var = YUNDEF;
    PUSH_LOCALS3(env, name, as, var);

    uint_t lineno = NODE_LINENO(node);
    name = NODE(node)->u.import.name;
    compile_import(env, data, lineno, name);

    as = NODE(node)->u.import.as;
    if (IS_NIL(as)) {
        var = YogArray_at(env, name, 0);
        append_store(env, data, lineno, VAL2ID(var));
        RETURN_VOID(env);
    }

    YOG_ASSERT(env, IS_SYMBOL(as), "invalid \"as\" (0x%08x)", as);
    compile_load_pkg_attr(env, data, lineno, name);
    append_store(env, data, lineno, VAL2ID(as));

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
    visitor->visit_class = compile_visit_class;
    visitor->visit_dict = compile_visit_dict;
    visitor->visit_except = compile_visit_except;
    visitor->visit_except_body = NULL;
    visitor->visit_finally = compile_visit_finally;
    visitor->visit_from = compile_visit_from;
    visitor->visit_func_call = compile_visit_func_call;
    visitor->visit_func_def = compile_visit_func_def;
    visitor->visit_if = compile_visit_if;
    visitor->visit_import = compile_visit_import;
    visitor->visit_imported_attr = NULL;
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
    YogVal name = YUNDEF;
    PUSH_LOCALS2(env, var_tbl, name);

    if (filename == NULL) {
        filename = "<stdin>";
    }
    name = YogCharArray_new_str(env, filename);
    var_tbl = make_var_table_of_func(env, name, CTX_PKG, YNIL, stmts, YNIL);
    decide_var_type(env, var_tbl);

    AstVisitor visitor;
    compile_init_visitor(&visitor);

    ID class_name = INVALID_ID;
    ID func_name = YogVM_intern(env, env->vm, "<package>");

    YogVal code = compile_stmts(env, &visitor, name, class_name, func_name, YUNDEF, stmts, var_tbl, YNIL, interactive);

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
    INST(inst)->opcode = OP(FINISH);

    bin = insts2bin(env, inst);
    YogBinary_shrink(env, bin);

    code = YogCode_new(env);
    CODE(code)->stack_size = 1;
    YogGC_UPDATE_PTR(env, CODE(code), insts, PTR_AS(YogBinary, bin)->body);

    RETURN(env, code);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
