#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "yog/arg.h"
#include "yog/array.h"
#include "yog/binary.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/inst.h"
#include "yog/opcodes.h"
#include "yog/parser.h"
#include "yog/string.h"
#include "yog/table.h"
#include "yog/thread.h"
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
    VisitNode visit_except;
    VisitNode visit_except_body;
    VisitNode visit_finally;
    VisitNode visit_func_call;
    VisitNode visit_func_def;
    VisitNode visit_if;
    VisitNode visit_import;
    VisitNode visit_klass;
    VisitNode visit_literal;
    VisitNode visit_next;
    VisitNode visit_nonlocal;
    VisitNode visit_return;
    VisitNode visit_stmt;
    VisitNode visit_subscript;
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
    CTX_FUNC, 
    CTX_KLASS, 
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

    YogVal filename;
    ID klass_name;

    YogVal outer;
    uint_t max_outer_depth;

    BOOL interactive;
};

typedef struct CompileData CompileData;

#define COMPILE_DATA(v)     PTR_AS(CompileData, (v))

#define RERAISE(env)    do { \
    ID id = YogVM_intern(env, env->vm, "raise"); \
    CompileData_add_load_global(env, data, lineno, id); \
    CompileData_add_call_function(env, data, lineno, 1, 0, 0, 0, 0); \
} while (0)

#define PUSH_TRY()  do { \
    YogVal try_list_entry = TryListEntry_new(env); \
    PUSH_LOCAL(env, try_list_entry); \
    \
    TRY_LIST_ENTRY(try_list_entry)->prev = COMPILE_DATA(data)->try_list; \
    COMPILE_DATA(data)->try_list = try_list_entry; \
    TRY_LIST_ENTRY(try_list_entry)->node = node; \
    TRY_LIST_ENTRY(try_list_entry)->exc_tbl = YNIL;

#define POP_TRY() \
    COMPILE_DATA(data)->try_list = TRY_LIST_ENTRY(try_list_entry)->prev; \
    POP_LOCALS(env); \
} while (0)

#define PUSH_EXCEPTION_TABLE_ENTRY() do { \
    YogVal last = COMPILE_DATA(data)->exc_tbl_last; \
    EXCEPTION_TABLE_ENTRY(last)->next = exc_tbl_entry; \
    COMPILE_DATA(data)->exc_tbl_last = exc_tbl_entry; \
} while (0)

#define NODE(p)     PTR_AS(YogNode, (p))

static void
raise_error(YogEnv* env, YogVal filename, uint_t lineno, const char* fmt, ...)
{
    SAVE_ARG(env, filename);

#define BUFFER_SIZE     4096
    char head[BUFFER_SIZE];
    snprintf(head, array_sizeof(head), "file \"%s\", line %u: ", PTR_AS(YogCharArray, filename)->items, lineno);

    char s[BUFFER_SIZE];
    va_list ap;
    va_start(ap, fmt);
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

    if (INST(inst)->type == INST_OP) {
        switch (INST_OPCODE(inst)) {
        case OP(JUMP):
            YogGC_keep(env, &JUMP_DEST(inst), keeper, heap);
            break;
        case OP(JUMP_IF_FALSE):
            YogGC_keep(env, &JUMP_IF_FALSE_DEST(inst), keeper, heap);
            break;
        default:
            break;
        }
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
    INST(COMPILE_DATA(data)->last_inst)->next = inst;
    COMPILE_DATA(data)->last_inst = inst;
}

#include "src/compile.inc"

static void 
TryListEntry_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    TryListEntry* entry = ptr;
#define KEEP(member)    YogGC_keep(env, &entry->member, keeper, heap)
    KEEP(prev);
    KEEP(node);
    KEEP(exc_tbl);
#undef KEEP
}

static YogVal 
TryListEntry_new(YogEnv* env) 
{
    YogVal entry = ALLOC_OBJ(env, TryListEntry_keep_children, NULL, TryListEntry);
    PTR_AS(TryListEntry, entry)->prev = YUNDEF;
    PTR_AS(TryListEntry, entry)->node = YUNDEF;
    PTR_AS(TryListEntry, entry)->exc_tbl = YUNDEF;

    return entry;
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
    case NODE_KLASS:
        VISIT(klass);
        break;
    case NODE_SUBSCRIPT:
        VISIT(subscript);
        break;
    default:
        YOG_BUG(env, "Unknown node type (0x%08x)", NODE(node)->type);
        break;
    }
#undef VISIT
}

static void 
visit_each_args(YogEnv* env, AstVisitor* visitor, YogVal args, YogVal blockarg, YogVal arg) 
{
    SAVE_ARGS3(env, args, blockarg, arg);

    if (IS_PTR(args)) {
        uint_t argc = YogArray_size(env, args);
        uint_t i = 0;
        for (i = 0; i < argc; i++) {
            YogVal node = YogArray_at(env, args, i);
            visit_node(env, visitor, node, arg);
        }
    }
    if (IS_PTR(blockarg)) {
        visit_node(env, visitor, blockarg, arg);
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
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        node = YogArray_at(env, stmts, i);

        visitor->visit_stmt(env, visitor, node, data);

        switch (NODE(node)->type) {
        case NODE_ASSIGN:
        case NODE_COMMAND_CALL:
        case NODE_FUNC_CALL:
        case NODE_LITERAL:
        case NODE_METHOD_CALL:
        case NODE_VARIABLE:
            {
                uint_t lineno = NODE(node)->lineno;
                if (COMPILE_DATA(data)->interactive) {
                    CompileData_add_print_top(env, data, lineno);
                }
                else {
                    CompileData_add_pop(env, data, lineno);
                }
            }
            break;
        default:
            break;
        }
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
    uint_t i = 0;
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
scan_var_visit_assign(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal left = NODE(node)->u.assign.left;
    if (NODE(left)->type == NODE_VARIABLE) {
        ID id = NODE(left)->u.variable.id;
        scan_var_register(env, SCAN_VAR_DATA(data)->var_tbl, id, VAR_ASSIGNED);
    }
    else {
        visit_node(env, visitor, left, data);
    }

    visit_node(env, visitor, NODE(node)->u.assign.right, data);

    RETURN_VOID(env);
}

static void 
scan_var_visit_func_def(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data) 
{
    ID id = NODE(node)->u.funcdef.name;
    scan_var_register(env, SCAN_VAR_DATA(data)->var_tbl, id, VAR_ASSIGNED);
}

static void 
scan_var_visit_func_call(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data) 
{
    SAVE_ARGS2(env, node, data);

    YogVal callee = NODE(node)->u.func_call.callee;
    visit_node(env, visitor, callee, data);

    YogVal args = NODE(node)->u.func_call.args;
    YogVal blockarg = NODE(node)->u.func_call.blockarg;
    visit_each_args(env, visitor, args, blockarg, data);

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

    visit_node(env, visitor, NODE(node)->u.except_body.type, data);

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
    visit_node(env, visitor, NODE(node)->u.break_.expr, data);
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
scan_var_visit_klass(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

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
visit_array_elements(YogEnv* env, AstVisitor* visitor, YogVal elems, YogVal data)
{
    SAVE_ARGS2(env, elems, data);

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
scan_var_visit_array(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    YogVal elems = NODE(node)->u.array.elems;
    if (!IS_PTR(elems)) {
        return;
    }

    visit_array_elements(env, visitor, elems, data);
}

static void 
scan_var_init_visitor(AstVisitor* visitor) 
{
    visitor->visit_array = scan_var_visit_array;
    visitor->visit_assign = scan_var_visit_assign;
    visitor->visit_attr = scan_var_visit_attr;
    visitor->visit_block = NULL;
    visitor->visit_break = scan_var_visit_break;
    visitor->visit_except = scan_var_visit_except;
    visitor->visit_except_body = scan_var_visit_except_body;
    visitor->visit_finally = scan_var_visit_finally;
    visitor->visit_func_call = scan_var_visit_func_call;
    visitor->visit_func_def = scan_var_visit_func_def;
    visitor->visit_if = scan_var_visit_if;
    visitor->visit_import = scan_var_visit_import;
    visitor->visit_klass = scan_var_visit_klass;
    visitor->visit_literal = NULL;
    visitor->visit_next = scan_var_visit_break;
    visitor->visit_nonlocal = scan_var_visit_nonlocal;
    visitor->visit_return = scan_var_visit_break;
    visitor->visit_stmt = visit_node;
    visitor->visit_stmts = scan_var_visit_stmts;
    visitor->visit_subscript = scan_var_visit_subscript;
    visitor->visit_variable = scan_var_visit_variable;
    visitor->visit_while = scan_var_visit_while;
}

static void 
ScanVarData_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap) 
{
    ScanVarData* data = ptr;
    YogGC_keep(env, &data->var_tbl, keeper, heap);
}

static YogVal 
ScanVarData_new(YogEnv* env) 
{
    YogVal data = ALLOC_OBJ(env, ScanVarData_keep_children, NULL, ScanVarData);
    PTR_AS(ScanVarData, data)->var_tbl = YUNDEF;

    return data;
}

static YogVal
make_var_table(YogEnv* env, YogVal stmts, YogVal var_tbl)
{
    SAVE_ARGS2(env, stmts, var_tbl);

    AstVisitor visitor;
    scan_var_init_visitor(&visitor);

    if (!IS_PTR(var_tbl)) {
        var_tbl = YogTable_new_symbol_table(env);
    }
    YogVal data = ScanVarData_new(env);
    SCAN_VAR_DATA(data)->var_tbl = var_tbl;

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
    case CTX_FUNC:
        {
            YogVal var = lookup_var(env, COMPILE_DATA(data)->vars, name);
            if (!IS_PTR(var)) {
                const char* s = YogVM_id2name(env, env->vm, name);
                YOG_BUG(env, "variable not found (%s)", s);
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
    case CTX_KLASS:
    case CTX_PKG:
        CompileData_add_store_name(env, data, lineno, name);
        break;
    default:
        YOG_ASSERT(env, FALSE, "Unkown context.");
        break;
    }
}

static void 
compile_visit_assign(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal left = YUNDEF;
    PUSH_LOCAL(env, left);

    uint_t lineno = NODE(node)->lineno;

    left = NODE(node)->u.assign.left;
    switch (NODE(left)->type) {
    case NODE_VARIABLE:
        {
            visit_node(env, visitor, NODE(node)->u.assign.right, data);
            CompileData_add_dup(env, data, lineno);
            ID name = NODE(left)->u.variable.id;
            append_store(env, data, lineno, name);
            break;
        }
    case NODE_SUBSCRIPT:
        {
            visit_node(env, visitor, NODE(left)->u.subscript.prefix, data);

            uint_t lineno = NODE(left)->lineno;
            ID attr = YogVM_intern(env, env->vm, "[]=");
            CompileData_add_load_attr(env, data, lineno, attr);

            visit_node(env, visitor, NODE(left)->u.subscript.index, data);
            visit_node(env, visitor, NODE(node)->u.assign.right, data);

            CompileData_add_call_function(env, data, lineno, 2, 0, 0, 0, 0);
            break;
        }
    case NODE_ATTR:
        {
            visit_node(env, visitor, NODE(node)->u.assign.right, data);
            CompileData_add_dup(env, data, lineno);

            visit_node(env, visitor, NODE(left)->u.attr.obj, data);
            ID name = NODE(left)->u.attr.name;
            CompileData_add_store_attr(env, data, lineno, name);
        }
        break;
    default:
        YOG_ASSERT(env, FALSE, "invalid node type (0x%08x)", NODE(left)->type);
        break;
    }

    RETURN_VOID(env);
}

static int_t
register_const(YogEnv* env, YogVal data, YogVal const_) 
{
    SAVE_ARGS2(env, data, const_);

    YogVal const2index = COMPILE_DATA(data)->const2index;

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
compile_visit_literal(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data) 
{
    SAVE_ARGS2(env, node, data);

    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    val = NODE(node)->u.literal.val;
    uint_t lineno = NODE(node)->lineno;
    if (VAL2PTR(YogVal_get_klass(env, val)) == VAL2PTR(env->vm->cString)) {
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

    YogVal tbl = PTR2VAL(ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(YogLinenoTableEntry) * size));
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
ExceptionTableEntry_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    ExceptionTableEntry* entry = ptr;
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

    add_push_const(env, data, YNIL, lineno);
    CompileData_add_ret(env, data, lineno);

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
    CompileData* data = ptr;
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
    KEEP(outer);
    KEEP(filename);
#undef KEEP
}

static YogVal 
CompileData_new(YogEnv* env, Context ctx, YogVal vars, YogVal anchor, YogVal exc_tbl_ent, YogVal filename, ID klass_name, YogVal upper_data, BOOL interactive)
{
    SAVE_ARGS5(env, vars, anchor, exc_tbl_ent, filename, upper_data);

    YogVal data = ALLOC_OBJ(env, CompileData_keep_children, NULL, CompileData);
    COMPILE_DATA(data)->ctx = ctx;
    COMPILE_DATA(data)->vars = vars;
    COMPILE_DATA(data)->const2index = YUNDEF;
    COMPILE_DATA(data)->label_while_start = YUNDEF;
    COMPILE_DATA(data)->label_while_end = YUNDEF;
    COMPILE_DATA(data)->finally_list = YUNDEF;
    COMPILE_DATA(data)->try_list = YUNDEF;
    COMPILE_DATA(data)->klass_name = INVALID_ID;
    COMPILE_DATA(data)->last_inst = anchor;
    COMPILE_DATA(data)->exc_tbl = exc_tbl_ent;
    COMPILE_DATA(data)->exc_tbl_last = exc_tbl_ent;
    COMPILE_DATA(data)->filename = filename;
    COMPILE_DATA(data)->klass_name = klass_name;
    COMPILE_DATA(data)->outer = upper_data;
    COMPILE_DATA(data)->max_outer_depth = 0;
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
        YOG_ASSERT(env, index < ALLOC_LOCAL_VARS_TABLE_ARG(*arg)->count, "local var index over count", index, ALLOC_LOCAL_VARS_TABLE_ARG(*arg)->count);
        YogVal names = ALLOC_LOCAL_VARS_TABLE_ARG(*arg)->names;
        PTR_AS(ID, names)[index] = VAL2ID(key);
    }

    return ST_CONTINUE;
}

static void 
AllocLocalVarsTableArg_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    AllocLocalVarsTableArg* arg = ptr;
    YogGC_keep(env, &arg->names, keeper, heap);
}

static YogVal 
AllocLocalVarsTableArg_new(YogEnv* env, YogVal names, uint_t count) 
{
    SAVE_ARG(env, names);

    YogVal arg = ALLOC_OBJ(env, AllocLocalVarsTableArg_keep_children, NULL, AllocLocalVarsTableArg);
    PTR_AS(AllocLocalVarsTableArg, arg)->names = names;
    PTR_AS(AllocLocalVarsTableArg, arg)->count = count;

    RETURN(env, PTR2VAL(arg));
}

static ID* 
alloc_local_vars_table(YogEnv* env, YogVal vars, uint_t count) 
{
    SAVE_ARG(env, vars);

    YogVal names = YUNDEF;
    YogVal arg = YUNDEF;
    PUSH_LOCALS2(env, names, arg);

    names = PTR2VAL(ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(ID) * count));
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

static YogVal 
compile_stmts(YogEnv* env, AstVisitor* visitor, YogVal filename, ID klass_name, ID func_name, YogVal stmts, YogVal vars, Context ctx, YogVal tail, YogVal upper_data, BOOL interactive)
{
    SAVE_ARGS5(env, filename, stmts, vars, tail, upper_data);

    YogVal anchor = YUNDEF;
    YogVal exc_tbl_ent = YUNDEF;
    YogVal data = YUNDEF;
    PUSH_LOCALS3(env, anchor, exc_tbl_ent, data);

    anchor = Anchor_new(env);
    exc_tbl_ent = ExceptionTableEntry_new(env);
    data = CompileData_new(env, ctx, vars, anchor, exc_tbl_ent, filename, klass_name, upper_data, interactive);

    visitor->visit_stmts(env, visitor, stmts, data);
    if (IS_PTR(tail)) {
        CompileData_add_inst(env, data, tail);
    }
    if ((ctx == CTX_FUNC) || (ctx == CTX_PKG)) {
        if (IS_PTR(stmts)) {
            uint_t size = YogArray_size(env, stmts);
            if (size < 1) {
                CompileData_add_ret_nil(env, data, 0);
            }
            else {
                YogVal node = YogArray_at(env, stmts, size - 1);
                if (NODE(node)->type != NODE_RETURN) {
                    CompileData_add_ret_nil(env, data, NODE(node)->lineno);
                }
            }
        }
        else {
            CompileData_add_ret_nil(env, data, 0);
        }
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
    CODE(code)->klass_name = klass_name;
    CODE(code)->func_name = func_name;

#if 0
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
lookup_local_var_index(YogEnv* env, YogVal vars, ID name) 
{
    YogVal var = lookup_var(env, vars, name);
    if (VAR(var)->type == VT_LOCAL) {
        return VAR(var)->u.local.index;
    }
    else {
        return 0;
    }
}

static void 
setup_params(YogEnv* env, YogVal vars, YogVal params, YogVal code)
{
    if (!IS_PTR(params)) {
        return;
    }

    SAVE_ARGS3(env, vars, params, code);

    YogVal arg_info = YogArgInfo_new(env);
    CODE(code)->arg_info = arg_info;
    PUSH_LOCAL(env, arg_info);

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
    YogVal arg_index = YNIL;
    if (0 < argc) {
        argnames = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(ID) * argc);
        PUSH_LOCAL(env, argnames);
        arg_index = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(uint8_t) * argc);
        PUSH_LOCAL(env, arg_index);
        for (i = 0; i < argc; i++) {
            YogVal node = YogArray_at(env, params, i);
            YOG_ASSERT(env, NODE(node)->type == NODE_PARAM, "Node must be NODE_PARAM.");

            ID name = NODE(node)->u.param.name;
            PTR_AS(ID, argnames)[i] = name;
            PTR_AS(uint8_t, arg_index)[i] = lookup_local_var_index(env, vars, name);
        }
    }

    ARG_INFO(arg_info)->argc = argc;
    ARG_INFO(arg_info)->argnames = argnames;
    ARG_INFO(arg_info)->arg_index = arg_index;
    if (size == argc) {
        RETURN_VOID(env);
    }

    uint_t n = argc;
    YogVal node = YogArray_at(env, params, n);
    if (NODE(node)->type == NODE_BLOCK_PARAM) {
        ARG_INFO(arg_info)->blockargc = 1;

        ID name = NODE(node)->u.param.name;
        ARG_INFO(arg_info)->blockargname = name;
        uint_t blockarg_index = lookup_local_var_index(env, vars, name);
        ARG_INFO(arg_info)->blockarg_index = blockarg_index;

        n++;
        if (size == n) {
            RETURN_VOID(env);
        }
        node = YogArray_at(env, params, n);
    }

    if (NODE(node)->type == NODE_VAR_PARAM) {
        ARG_INFO(arg_info)->varargc = 1;

        ID name = NODE(node)->u.param.name;
        uint_t vararg_index = lookup_local_var_index(env, vars, name);
        ARG_INFO(arg_info)->vararg_index = vararg_index;

        n++;
        if (size == n) {
            RETURN_VOID(env);
        }
        node = YogArray_at(env, params, n);
    }

    YOG_ASSERT(env, NODE(node)->type == NODE_KW_PARAM, "Node must be NODE_KW_PARAM.");
    ARG_INFO(arg_info)->kwargc = 1;

    ID name = NODE(node)->u.param.name;
    uint_t kwarg_index = lookup_local_var_index(env, vars, name);
    ARG_INFO(arg_info)->kwarg_index = kwarg_index;

    n++;
    YOG_ASSERT(env, size == n, "Parameters count is unmatched.");

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

struct Flags2TypeArg {
    YogVal vars;
    YogVal outer;
    uint_t next_local_index;
};

typedef struct Flags2TypeArg Flags2TypeArg;

#define FLAGS2TYPE_ARG(v)   PTR_AS(Flags2TypeArg, (v))

static int_t 
vars_flags2type_callback(YogEnv* env, YogVal key, YogVal val, YogVal* arg) 
{
    SAVE_ARGS2(env, key, val);

    YogVal var = YUNDEF;
    PUSH_LOCAL(env, var);

    ID name = VAL2ID(key);
    int_t flags = SCAN_VAR_ENTRY(val)->flags;
    var = Var_new(env);
    if (IS_NONLOCAL(flags) || (!IS_ASSIGNED(flags) && !IS_PARAM(flags))) {
        uint_t level = 0;
        uint_t index = 0;
        if (find_outer_var(env, name, FLAGS2TYPE_ARG(*arg)->outer, &level, &index)) {
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
        uint_t index;
        if (name != YogVM_intern(env, env->vm, "self")) {
            index = FLAGS2TYPE_ARG(*arg)->next_local_index;
            FLAGS2TYPE_ARG(*arg)->next_local_index++;
        }
        else {
            index = 0;
        }
        VAR(var)->u.local.index = index;
    }

    YogTable_add_direct(env, FLAGS2TYPE_ARG(*arg)->vars, key, var);

    RETURN(env, ST_CONTINUE);
}

static void 
Flags2TypeArg_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    Flags2TypeArg* arg = ptr;
#define KEEP(member)    YogGC_keep(env, &arg->member, keeper, heap)
    KEEP(vars);
    KEEP(outer);
#undef KEEP
}

static YogVal 
Flags2TypeArg_new(YogEnv* env) 
{
    YogVal arg = ALLOC_OBJ(env, Flags2TypeArg_keep_children, NULL, Flags2TypeArg);
    PTR_AS(Flags2TypeArg, arg)->vars = YUNDEF;
    PTR_AS(Flags2TypeArg, arg)->outer = YUNDEF;
    PTR_AS(Flags2TypeArg, arg)->next_local_index = 0;

    return PTR2VAL(arg);
}

static YogVal 
vars_flags2type(YogEnv* env, YogVal var_tbl, YogVal outer) 
{
    SAVE_ARGS2(env, var_tbl, outer);

    YogVal vars = YUNDEF;
    YogVal arg = YUNDEF;
    PUSH_LOCALS2(env, vars, arg);

    vars = YogTable_new_symbol_table(env);

    arg = Flags2TypeArg_new(env);
    ID self = YogVM_intern(env, env->vm, "self");
    if (YogTable_lookup(env, var_tbl, ID2VAL(self), NULL)) {
        PTR_AS(Flags2TypeArg, arg)->next_local_index = 1;
    }
    FLAGS2TYPE_ARG(arg)->vars = vars;
    FLAGS2TYPE_ARG(arg)->outer = outer;

    YogTable_foreach(env, var_tbl, vars_flags2type_callback, &arg);

    RETURN(env, vars);
}

static YogVal 
compile_func(YogEnv* env, AstVisitor* visitor, YogVal filename, ID klass_name, YogVal node, YogVal upper) 
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
    var_tbl = make_var_table(env, stmts, var_tbl);

    vars = vars_flags2type(env, var_tbl, upper);

    ID func_name = NODE(node)->u.funcdef.name;

    YogVal code = compile_stmts(env, visitor, filename, klass_name, func_name, stmts, vars, CTX_FUNC, YNIL, upper, FALSE);
    PUSH_LOCAL(env, code);
    setup_params(env, vars, params, code);

    RETURN(env, code);
}

static void 
compile_visit_func_def(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data) 
{
    SAVE_ARGS2(env, node, data);

    ID klass_name = INVALID_ID;
    if (COMPILE_DATA(data)->ctx == CTX_KLASS) {
        klass_name = COMPILE_DATA(data)->klass_name;
    }

    YogVal code = compile_func(env, visitor, COMPILE_DATA(data)->filename, klass_name, node, data);

    add_push_const(env, data, code, NODE(node)->lineno);

    ID id = NODE(node)->u.funcdef.name;
    uint_t lineno = NODE(node)->lineno;
    switch (COMPILE_DATA(data)->ctx) {
    case CTX_FUNC:
        YOG_ASSERT(env, FALSE, "TODO: NOT IMPLEMENTED");
        break;
    case CTX_KLASS:
    case CTX_PKG:
        CompileData_add_make_function(env, data, lineno);
        CompileData_add_store_name(env, data, lineno, id);
        break;
    default:
        YOG_ASSERT(env, FALSE, "Unknown context.");
        break;
    }

    RETURN_VOID(env);
}

static void 
compile_visit_func_call(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data) 
{
    SAVE_ARGS2(env, node, data);

    YogVal args = YUNDEF;
    YogVal blockarg = YUNDEF;
    PUSH_LOCALS2(env, args, blockarg);

    visit_node(env, visitor, NODE(node)->u.func_call.callee, data);

    args = NODE(node)->u.func_call.args;
    blockarg = NODE(node)->u.func_call.blockarg;
    visit_each_args(env, visitor, args, blockarg, data);

    uint_t argc = 0;
    if (IS_PTR(args)) {
        argc = YogArray_size(env, args);
    }

    uint8_t blockargc = 0;
    if (IS_PTR(blockarg)) {
        blockargc = 1;
    }

    uint_t lineno = NODE(node)->lineno;
    CompileData_add_call_function(env, data, lineno, argc, 0, blockargc, 0, 0);

    RETURN_VOID(env);
}

static void 
compile_visit_variable(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data) 
{
    SAVE_ARGS2(env, node, data);

    ID id = NODE(node)->u.variable.id;
    uint_t lineno = NODE(node)->lineno;
    switch (COMPILE_DATA(data)->ctx) {
    case CTX_FUNC:
        {
            YogVal var = lookup_var(env, COMPILE_DATA(data)->vars, id);
            YOG_ASSERT(env, IS_PTR(var), "can't find variable");
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
                YOG_BUG(env, "unknown variable type (%d)", VAR(var)->type);
                break;
            }
            break;
        }
    case CTX_KLASS:
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
get_last_lineno(YogEnv* env, YogVal stmts) 
{
    uint_t size = YogArray_size(env, stmts);
    YogVal node = YogArray_at(env, stmts, size - 1);
    return NODE(node)->lineno;
}

static void 
FinallyListEntry_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    FinallyListEntry* ent = ptr;
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
compile_visit_finally(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data) 
{
    SAVE_ARGS2(env, node, data);

    YogVal label_head_start = YUNDEF;
    YogVal label_head_end = YUNDEF;
    YogVal label_finally_error_start = YUNDEF;
    YogVal label_finally_end = YUNDEF;
    PUSH_LOCALS4(env, label_head_start, label_head_end, label_finally_error_start, label_finally_end);

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

    PUSH_TRY();

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
    RERAISE(env);

    add_inst(env, data, label_finally_end);

    PUSH_EXCEPTION_TABLE_ENTRY();

    POP_TRY();

    COMPILE_DATA(data)->finally_list = FINALLY_LIST_ENTRY(finally_list_entry)->prev;

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
    PUSH_LOCALS5(env, label_head_start, label_head_end, label_excepts_start, label_else_start, label_else_end);

    label_head_start = Label_new(env);
    label_head_end = Label_new(env);
    label_excepts_start = Label_new(env);
    label_else_start = Label_new(env);
    label_else_end = Label_new(env);

    PUSH_TRY();

    YogVal exc_tbl_entry = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal excepts = YUNDEF;
    PUSH_LOCALS3(env, exc_tbl_entry, stmts, excepts);

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
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        YogVal label_body_end = YUNDEF;
        YogVal node = YUNDEF;
        YogVal node_type = YUNDEF;
        YogVal stmts = YUNDEF;
        PUSH_LOCALS4(env, label_body_end, node, node_type, stmts);

        label_body_end = Label_new(env);

        node = YogArray_at(env, excepts, i);
        node_type = NODE(node)->u.except_body.type;
        if (IS_PTR(node_type)) {
            visit_node(env, visitor, node_type, data);
            ID attr = YogVM_intern(env, env->vm, "===");
            CompileData_add_load_attr(env, data, lineno, attr);
#define LOAD_EXC()  do { \
    ID name = YogVM_intern(env, env->vm, "$!"); \
    CompileData_add_load_special(env, data, lineno, name); \
} while (0)
            lineno = NODE(node_type)->lineno;
            LOAD_EXC();
            CompileData_add_call_function(env, data, lineno, 1, 0, 0, 0, 0);
            CompileData_add_jump_if_false(env, data, lineno, label_body_end);

            ID id = NODE(node)->u.except_body.var;
            if (id != NO_EXC_VAR) {
                LOAD_EXC();
                append_store(env, data, NODE(node)->lineno, id);
            }
#undef LOAD_EXC
        }

        stmts = NODE(node)->u.except_body.stmts;
        visitor->visit_stmts(env, visitor, stmts, data);
        if (IS_PTR(stmts)) {
            lineno = get_last_lineno(env, stmts);
        }
        else {
            lineno = NODE(node)->lineno;
        }
        CompileData_add_jump(env, data, lineno, label_else_end);

        add_inst(env, data, label_body_end);

        POP_LOCALS(env);
    }
    RERAISE(env);

    add_inst(env, data, label_else_start);
    visitor->visit_stmts(env, visitor, NODE(node)->u.except.else_, data);
    add_inst(env, data, label_else_end);

    PUSH_EXCEPTION_TABLE_ENTRY();

    POP_TRY();

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

    YogVal expr = YUNDEF;
    YogVal finally_list_entry = YUNDEF;
    YogVal label_start = YUNDEF;
    YogVal label_end = YUNDEF;
    YogVal try_list_entry = YUNDEF;
    PUSH_LOCALS5(env, expr, finally_list_entry, label_start, label_end, try_list_entry);

    if (NODE(node)->type == NODE_BREAK) {
        expr = NODE(node)->u.break_.expr;
    }
    else {
        expr = NODE(node)->u.next.expr;
    }

    if (IS_PTR(COMPILE_DATA(data)->label_while_start)) {
        YOG_ASSERT(env, !IS_PTR(expr), "Can't return value with break/next.");
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
    }
    else {
        /* TODO */
    }

    RETURN_VOID(env);
}

static void 
compile_visit_break(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    YogVal label_while_end = COMPILE_DATA(data)->label_while_end;
    compile_while_jump(env, visitor, node, data, label_while_end);
}

static void 
compile_visit_next(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data) 
{
    YogVal label_while_start = COMPILE_DATA(data)->label_while_start;
    compile_while_jump(env, visitor, node, data, label_while_start);
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
    PUSH_LOCALS5(env, vars, code, filename, stmts, var_tbl);

    YogVal params = YUNDEF;
    PUSH_LOCAL(env, params);

    var_tbl = var_table_new(env);

    params = NODE(node)->u.blockarg.params;
    filename = COMPILE_DATA(data)->filename;
    register_params_var_table(env, params, var_tbl, filename);

    stmts = NODE(node)->u.blockarg.stmts;
    var_tbl = make_var_table(env, stmts, var_tbl);

    vars = vars_flags2type(env, var_tbl, data);

    ID klass_name = INVALID_ID;
    ID func_name = YogVM_intern(env, env->vm, "<block>");
    code = compile_stmts(env, visitor, filename, klass_name, func_name, stmts, vars, CTX_FUNC, YNIL, data, FALSE);

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
    CompileData_add_make_function(env, data, lineno);

    RETURN_VOID(env);
}

static YogVal 
compile_klass(YogEnv* env, AstVisitor* visitor, ID klass_name, YogVal stmts, uint_t first_lineno, YogVal data) 
{
    SAVE_ARGS2(env, stmts, data);

    YogVal var_tbl = YUNDEF;
    YogVal vars = YUNDEF;
    YogVal ret = YUNDEF;
    YogVal push_self_name = YUNDEF;
    PUSH_LOCALS4(env, var_tbl, vars, ret, push_self_name);

    var_tbl = make_var_table(env, stmts, YUNDEF);
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
    push_self_name = Inst_new(env, lineno);
    INST(push_self_name)->next = ret;
    INST(push_self_name)->opcode = OP(PUSH_SELF_NAME);

    YogVal filename = COMPILE_DATA(data)->filename;
    ID func_name = INVALID_ID;

    YogVal code = compile_stmts(env, visitor, filename, klass_name, func_name, stmts, vars, CTX_KLASS, push_self_name, COMPILE_DATA(data)->outer, FALSE);

    RETURN(env, code);
}

static void 
compile_visit_klass(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data) 
{
    SAVE_ARGS2(env, node, data);

    YogVal super = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    PUSH_LOCALS3(env, super, stmts, code);

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
    code = compile_klass(env, visitor, name, stmts, lineno, data);
    add_push_const(env, data, code, lineno);

    CompileData_add_make_klass(env, data, lineno);

    append_store(env, data, lineno, name);

    RETURN_VOID(env);
}

static void 
compile_visit_return(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data)
{
    SAVE_ARGS2(env, node, data);

    YogVal expr = NODE(node)->u.return_.expr;
    uint_t lineno = NODE(node)->lineno;
    if (IS_PTR(expr)) {
        visit_node(env, visitor, expr, data);
    }
    else {
        add_push_const(env, data, YNIL, lineno);
    }

    CompileData_add_ret(env, data, lineno);

    RETURN_VOID(env);
}

static void 
compile_visit_subscript(YogEnv* env, AstVisitor* visitor, YogVal node, YogVal data) 
{
    SAVE_ARGS2(env, node, data);

    visit_node(env, visitor, NODE(node)->u.subscript.prefix, data);

    uint_t lineno = NODE(node)->lineno;
    ID attr = YogVM_intern(env, env->vm, "[]");
    CompileData_add_load_attr(env, data, lineno, attr);

    visit_node(env, visitor, NODE(node)->u.subscript.index, data);

    CompileData_add_call_function(env, data, lineno, 1, 0, 0, 0, 0);

    RETURN_VOID(env);
}

static ID
join_package_names(YogEnv* env, YogVal pkg_name)
{
    SAVE_ARG(env, pkg_name);

    size_t len = 0;
    uint_t size = YogArray_size(env, pkg_name);
    uint_t i;
    for (i = 0; i < size; i++) {
        YogVal name = YogArray_at(env, pkg_name, i);
        const char* s = YogVM_id2name(env, env->vm, VAL2ID(name));
        len += strlen(s);
    }
    len += size - 1;
    char pkg[len + 1];
    char* pc = pkg - 1;
    for (i = 0; i < size; i++) {
        YogVal name = YogArray_at(env, pkg_name, i);
        const char* s = YogVM_id2name(env, env->vm, VAL2ID(name));
        pc++;
        strcpy(pc, s);
        size_t l = strlen(s);
        pc += l;
        *pc = '.';
    }
    *pc = '\0';

    RETURN(env, YogVM_intern(env, env->vm, pkg));
}

static void
compile_import(YogEnv* env, YogVal pkg_name, YogVal data, uint_t lineno)
{
    SAVE_ARGS2(env, pkg_name, data);

    ID import_package = YogVM_intern(env, env->vm, "import_package");
    CompileData_add_load_global(env, data, lineno, import_package);

    ID pkg = join_package_names(env, pkg_name);
    uint_t c = register_const(env, data, ID2VAL(pkg));
    CompileData_add_push_const(env, data, lineno, c);

    CompileData_add_call_function(env, data, lineno, 1, 0, 0, 0, 0);

    YogVal var_name = YogArray_at(env, pkg_name, 0);
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
        YogVal pkg_name = YogArray_at(env, names, i);
        compile_import(env, pkg_name, data, lineno);
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
compile_init_visitor(AstVisitor* visitor) 
{
    visitor->visit_array = compile_visit_array;
    visitor->visit_assign = compile_visit_assign;
    visitor->visit_attr = compile_visit_attr;
    visitor->visit_block = compile_visit_block;
    visitor->visit_break = compile_visit_break;
    visitor->visit_except = compile_visit_except;
    visitor->visit_except_body = NULL;
    visitor->visit_finally = compile_visit_finally;
    visitor->visit_func_call = compile_visit_func_call;
    visitor->visit_func_def = compile_visit_func_def;
    visitor->visit_if = compile_visit_if;
    visitor->visit_import = compile_visit_import;
    visitor->visit_klass = compile_visit_klass;
    visitor->visit_literal = compile_visit_literal;
    visitor->visit_next = compile_visit_next;
    visitor->visit_nonlocal = NULL;
    visitor->visit_return = compile_visit_return;
    visitor->visit_stmt = visit_node;
    visitor->visit_stmts = compile_visit_stmts;
    visitor->visit_subscript = compile_visit_subscript;
    visitor->visit_variable = compile_visit_variable;
    visitor->visit_while = compile_visit_while;
}

static YogVal
compile_module(YogEnv* env, const char* filename, YogVal stmts, BOOL interactive)
{
    SAVE_ARG(env, stmts);

    YogVal var_tbl = YUNDEF;
    YogVal vars = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS3(env, var_tbl, vars, name);

    var_tbl = make_var_table(env, stmts, YUNDEF);
    vars = vars_flags2type(env, var_tbl, YUNDEF);

    AstVisitor visitor;
    compile_init_visitor(&visitor);

    if (filename == NULL) {
        filename = "<stdin>";
    }
    name = YogCharArray_new_str(env, filename);

    ID klass_name = INVALID_ID;
    ID func_name = YogVM_intern(env, env->vm, "<module>");

    YogVal code = compile_stmts(env, &visitor, name, klass_name, func_name, stmts, vars, CTX_PKG, YNIL, YUNDEF, interactive);

    RETURN(env, code);
}

YogVal 
YogCompiler_compile_module(YogEnv* env, const char* filename, YogVal stmts) 
{
    return compile_module(env, filename, stmts, FALSE);
}

YogVal
YogCompiler_compile_interactive(YogEnv* env, YogVal stmts)
{
    return compile_module(env, MAIN_MODULE_NAME, stmts, TRUE);
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
