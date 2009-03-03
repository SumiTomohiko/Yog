#include <limits.h>
#include <stdint.h>
#include "yog/arg.h"
#include "yog/binary.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/inst.h"
#include "yog/opcodes.h"
#include "yog/parser.h"
#include "yog/st.h"
#include "yog/yog.h"

typedef struct AstVisitor AstVisitor;

typedef void (*VisitNode)(YogEnv*, AstVisitor*, YogNode*, void*);
typedef void (*VisitArray)(YogEnv*, AstVisitor*, YogArray*, void*);

struct AstVisitor {
    VisitArray visit_stmts;
    VisitNode visit_assign;
    VisitNode visit_block;
    VisitNode visit_break;
    VisitNode visit_command_call;
    VisitNode visit_except;
    VisitNode visit_except_body;
    VisitNode visit_finally;
    VisitNode visit_func_call;
    VisitNode visit_func_def;
    VisitNode visit_if;
    VisitNode visit_klass;
    VisitNode visit_literal;
    VisitNode visit_method_call;
    VisitNode visit_next;
    VisitNode visit_nonlocal;
    VisitNode visit_return;
    VisitNode visit_stmt;
    VisitNode visit_subscript;
    VisitNode visit_variable;
    VisitNode visit_while;
};

struct ScanVarData {
    struct YogTable* var_tbl;
};

typedef struct ScanVarData ScanVarData;

struct ScanVarEntry {
    unsigned int index;
    int flags;
};

#define VAR_ASSIGNED        (0x01)
#define VAR_PARAM           (0x02)
#define VAR_NONLOCAL        (0x04)
#define VAR_USED            (0x08)
#define IS_ASSIGNED(flags)  (flags & VAR_ASSIGNED)
#define IS_PARAM(flags)     (flags & VAR_PARAM)
#define IS_NONLOCAL(flags)  (flags & VAR_NONLOCAL)
#define IS_USED(flags)      (flags & VAR_USED)

typedef struct ScanVarEntry ScanVarEntry;

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
            unsigned int index;
        } local;
        struct {
            unsigned int level;
            unsigned int index;
        } nonlocal;
    } u;
};

typedef struct Var Var;

enum Context {
    CTX_FUNC, 
    CTX_KLASS, 
    CTX_PKG, 
};

typedef enum Context Context;

struct FinallyListEntry {
    struct FinallyListEntry* prev;

    struct YogNode* node;
};

typedef struct FinallyListEntry FinallyListEntry;

struct ExceptionTableEntry {
    struct ExceptionTableEntry* next;

    struct YogInst* from;
    struct YogInst* to;
    struct YogInst* target;
};

typedef struct ExceptionTableEntry ExceptionTableEntry;

struct TryListEntry {
    struct TryListEntry* prev;

    struct YogNode* node;
    struct ExceptionTableEntry* exc_tbl;
};

typedef struct TryListEntry TryListEntry;

struct CompileData {
    enum Context ctx;
    struct YogTable* vars;
    struct YogTable* const2index;
    struct YogInst* last_inst;
    struct ExceptionTableEntry* exc_tbl;
    struct ExceptionTableEntry* exc_tbl_last;

    struct YogInst* label_while_start;
    struct YogInst* label_while_end;
    struct FinallyListEntry* finally_list;
    struct TryListEntry* try_list;

    const char* filename;
    ID klass_name;

    struct CompileData* outer;
};

typedef struct CompileData CompileData;

#define RAISE   INTERN("raise")
#define RERAISE() \
    CompileData_add_call_command(env, data, lineno, RAISE, 0, 0, 0, 0, 0)

#define PUSH_TRY()  do { \
    TryListEntry try_list_entry; \
    try_list_entry.prev = data->try_list; \
    data->try_list = &try_list_entry; \
    try_list_entry.node = node; \
    try_list_entry.exc_tbl = NULL;

#define POP_TRY() \
    data->try_list = try_list_entry.prev; \
} while (0)

#define PUSH_EXCEPTION_TABLE_ENTRY() do { \
    data->exc_tbl_last->next = entry; \
    data->exc_tbl_last = entry; \
} while (0)

static void 
YogInst_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogInst* inst = ptr;
    inst->next = (*keeper)(env, inst->next);

    if (inst->type == INST_OP) {
        switch (INST_OPCODE(inst)) {
        case OP(JUMP):
            JUMP_DEST(inst) = (*keeper)(env, JUMP_DEST(inst));
            break;
        case OP(JUMP_IF_FALSE):
            JUMP_IF_FALSE_DEST(inst) = (*keeper)(env, JUMP_IF_FALSE_DEST(inst));
            break;
        default:
            break;
        }
    }
}

static YogInst* 
YogInst_new(YogEnv* env, InstType type, unsigned int lineno) 
{
    YogInst* inst = ALLOC_OBJ(env, YogInst_keep_children, NULL, YogInst);
    inst->type = type;
    inst->next = NULL;
    inst->lineno = lineno;
    inst->pc = 0;

    return inst;
}

static YogInst* 
Inst_new(YogEnv* env, unsigned int lineno) 
{
    return YogInst_new(env, INST_OP, lineno);
}

static YogInst* 
Label_new(YogEnv* env)
{
    return YogInst_new(env, INST_LABEL, 0);
}

static YogInst* 
Anchor_new(YogEnv* env) 
{
    return YogInst_new(env, INST_ANCHOR, 0);
}

static void 
add_inst(CompileData* data, YogInst* inst) 
{
    data->last_inst->next = inst;
    data->last_inst = inst;
}

#include "src/compile.inc"

static void 
visit_node(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    if (node == NULL) {
        return;
    }

#define VISIT(f)    do { \
    if (visitor->f != NULL) { \
        visitor->f(env, visitor, node, arg); \
    } \
} while (0)
    switch (node->type) {
    case NODE_ASSIGN: 
        VISIT(visit_assign);
        break;
    case NODE_VARIABLE:
        VISIT(visit_variable);
        break;
    case NODE_LITERAL:
        VISIT(visit_literal);
        break;
    case NODE_METHOD_CALL:
        VISIT(visit_method_call);
        break;
    case NODE_COMMAND_CALL:
        VISIT(visit_command_call);
        break;
    case NODE_FUNC_DEF:
        VISIT(visit_func_def);
        break;
    case NODE_FUNC_CALL:
        VISIT(visit_func_call);
        break;
    case NODE_FINALLY:
        VISIT(visit_finally);
        break;
    case NODE_EXCEPT:
        VISIT(visit_except);
        break;
    case NODE_EXCEPT_BODY:
        VISIT(visit_except_body);
        break;
    case NODE_WHILE:
        VISIT(visit_while);
        break;
    case NODE_IF:
        VISIT(visit_if);
        break;
    case NODE_BREAK:
        VISIT(visit_break);
        break;
    case NODE_NEXT:
        VISIT(visit_next);
        break;
    case NODE_NONLOCAL:
        VISIT(visit_nonlocal);
        break;
    case NODE_RETURN:
        VISIT(visit_return);
        break;
    case NODE_BLOCK_ARG:
        VISIT(visit_block);
        break;
    case NODE_KLASS:
        VISIT(visit_klass);
        break;
    case NODE_SUBSCRIPT:
        VISIT(visit_subscript);
        break;
    default:
        YOG_ASSERT(env, FALSE, "Unknown node type.");
        break;
    }
#undef VISIT
}

static void 
visit_each_args(YogEnv* env, AstVisitor* visitor, YogArray* args, YogNode* blockarg, void* arg) 
{
    if (args != NULL) {
        unsigned int argc = YogArray_size(env, args);
        unsigned int i = 0;
        for (i = 0; i < argc; i++) {
            YogVal val = YogArray_at(env, args, i);
            YogNode* node = VAL2PTR(val);
            visit_node(env, visitor, node, arg);
        }
    }
    if (blockarg != NULL) {
        visit_node(env, visitor, blockarg, arg);
    }
}

static void 
scan_var_visit_method_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.method_call.recv, arg);
    visit_each_args(env, visitor, node->u.method_call.args, node->u.method_call.blockarg, arg);
}

static void 
compile_visit_stmts(YogEnv* env, AstVisitor* visitor, YogArray* stmts, void* arg) 
{
    if (stmts == NULL) {
        return;
    }

    CompileData* data = arg;

    unsigned int size = YogArray_size(env, stmts);
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal val = YogArray_at(env, stmts, i);
        YogNode* node = VAL2PTR(val);
        visitor->visit_stmt(env, visitor, node, data);

        switch (node->type) {
        case NODE_ASSIGN:
        case NODE_COMMAND_CALL:
        case NODE_FUNC_CALL:
        case NODE_LITERAL:
        case NODE_METHOD_CALL:
        case NODE_VARIABLE:
            CompileData_add_pop(env, data, node->lineno);
            break;
        default:
            break;
        }
    }
}

static void 
scan_var_visit_stmts(YogEnv* env, AstVisitor* visitor, YogArray* stmts, void* arg) 
{
    if (stmts == NULL) {
        return;
    }

    unsigned int size = YogArray_size(env, stmts);
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal val = YogArray_at(env, stmts, i);
        YogNode* node = VAL2PTR(val);
        visitor->visit_stmt(env, visitor, node, arg);
    }
}

static ScanVarEntry* 
ScanVarEntry_new(YogEnv* env, unsigned int index, int flags) 
{
    ScanVarEntry* ent = ALLOC_OBJ(env, NULL, NULL, ScanVarEntry);
    ent->index = index;
    ent->flags = flags;

    return ent;
}

static void 
scan_var_register(YogEnv* env, YogTable* var_tbl, ID var, int flags)
{
    YogVal key = ID2VAL(var);
    YogVal val = YUNDEF;
    if (!YogTable_lookup(env, var_tbl, key, &val)) {
        int index = YogTable_size(env, var_tbl);
        ScanVarEntry* ent = ScanVarEntry_new(env, index, flags);
        YogTable_add_direct(env, var_tbl, key, PTR2VAL(ent));
    }
    else {
        ScanVarEntry* ent = VAL2PTR(val);
        ent->flags |= flags;
    }
}

static void 
scan_var_visit_assign(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    YogNode* left = node->u.assign.left;
    if (left->type == NODE_VARIABLE) {
        ID id = left->u.variable.id;
        ScanVarData* data = arg;
        scan_var_register(env, data->var_tbl, id, VAR_ASSIGNED);
    }
    else {
        visit_node(env, visitor, left, arg);
    }
}

static void 
scan_var_visit_command_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_each_args(env, visitor, node->u.command_call.args, node->u.command_call.blockarg, arg);
}

static void 
scan_var_visit_func_def(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    ScanVarData* data = arg;
    ID id = node->u.funcdef.name;
    scan_var_register(env, data->var_tbl, id, VAR_ASSIGNED);
}

static void 
scan_var_visit_func_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.func_call.callee, arg);
    visit_each_args(env, visitor, node->u.func_call.args, node->u.func_call.blockarg, arg);
}

static void 
scan_var_visit_finally(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visitor->visit_stmts(env, visitor, node->u.finally.head, arg);
    visitor->visit_stmts(env, visitor, node->u.finally.body, arg);
}

static void 
scan_var_visit_except_body(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.except_body.type, arg);

    ID id = node->u.except_body.var;
    if (id != NO_EXC_VAR) {
        ScanVarData* data = arg;
        scan_var_register(env, data->var_tbl, id, VAR_ASSIGNED);
    }
    visitor->visit_stmts(env, visitor, node->u.except_body.stmts, arg);
}

static void 
scan_var_visit_while(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.while_.test, arg);
    visitor->visit_stmts(env, visitor, node->u.while_.stmts, arg);
}

static void 
scan_var_visit_if(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.if_.test, arg);
    visitor->visit_stmts(env, visitor, node->u.if_.stmts, arg);
    visitor->visit_stmts(env, visitor, node->u.if_.tail, arg);
}

static void 
scan_var_visit_break(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.break_.expr, arg);
}

static void 
scan_var_visit_except(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visitor->visit_stmts(env, visitor, node->u.except.head, arg);

    YogArray* excepts = node->u.except.excepts;
    unsigned int size = YogArray_size(env, excepts);
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal val = YogArray_at(env, excepts, i);
        YogNode* node = VAL2PTR(val);
        visitor->visit_except_body(env, visitor, node, arg);
    }

    visitor->visit_stmts(env, visitor, node->u.except.else_, arg);
}

static void 
scan_var_visit_block(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    ScanVarData* data = arg;

    YogArray* params = node->u.blockarg.params;
    if (params != NULL) {
        unsigned int size = YogArray_size(env, params);
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            YogVal val = YogArray_at(env, params, i);
            YogNode* param = VAL2PTR(val);
            ID name = param->u.param.name;
            scan_var_register(env, data->var_tbl, name, VAR_PARAM);

            visit_node(env, visitor, param->u.param.default_, data);
        }
    }

    visitor->visit_stmts(env, visitor, node->u.blockarg.stmts, data);
}

static void 
scan_var_visit_klass(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    ScanVarData* data = arg;

    ID name = node->u.klass.name;
    scan_var_register(env, data->var_tbl, name, VAR_ASSIGNED);

    YogNode* super = node->u.klass.super;
    visit_node(env, visitor, super, data);
}

static void 
scan_var_visit_subscript(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.subscript.prefix, arg);
    visit_node(env, visitor, node->u.subscript.index, arg);
}

static void 
scan_var_visit_nonlocal(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    YogArray* names = node->u.nonlocal.names;
    unsigned int size = YogArray_size(env, names);

    ScanVarData* data = arg;

    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal val = YogArray_at(env, names, i);
        ID name = VAL2ID(val);
        scan_var_register(env,  data->var_tbl, name, VAR_NONLOCAL);
    }
}

static void 
scan_var_visit_variable(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    ScanVarData* data = arg;
    scan_var_register(env, data->var_tbl, node->u.variable.id, VAR_USED);
}

static void 
scan_var_init_visitor(AstVisitor* visitor) 
{
    visitor->visit_assign = scan_var_visit_assign;
    visitor->visit_block = scan_var_visit_block;
    visitor->visit_break = scan_var_visit_break;
    visitor->visit_command_call = scan_var_visit_command_call;
    visitor->visit_except = scan_var_visit_except;
    visitor->visit_except_body = scan_var_visit_except_body;
    visitor->visit_finally = scan_var_visit_finally;
    visitor->visit_func_call = scan_var_visit_func_call;
    visitor->visit_func_def = scan_var_visit_func_def;
    visitor->visit_if = scan_var_visit_if;
    visitor->visit_klass = scan_var_visit_klass;
    visitor->visit_literal = NULL;
    visitor->visit_method_call = scan_var_visit_method_call;
    visitor->visit_next = scan_var_visit_break;
    visitor->visit_nonlocal = scan_var_visit_nonlocal;
    visitor->visit_return = scan_var_visit_break;
    visitor->visit_stmt = visit_node;
    visitor->visit_stmts = scan_var_visit_stmts;
    visitor->visit_subscript = scan_var_visit_subscript;
    visitor->visit_variable = scan_var_visit_variable;
    visitor->visit_while = scan_var_visit_while;
}

static YogTable*
make_var_table(YogEnv* env, YogArray* stmts, YogTable* var_tbl)
{
    AstVisitor visitor;
    scan_var_init_visitor(&visitor);

    if (var_tbl == NULL) {
        var_tbl = YogTable_new_symbol_table(env);
    }
    ScanVarData data;
    data.var_tbl = var_tbl;

    visitor.visit_stmts(env, &visitor, stmts, &data);

    return var_tbl;
}

static Var*
lookup_var(YogEnv* env, YogTable* vars, ID name) 
{
    YogVal key = ID2VAL(name);
    YogVal val = YUNDEF;
    if (YogTable_lookup(env, vars, key, &val)) {
        return VAL2PTR(val);
    }
    else {
        return NULL;
    }
}

static void 
append_store(YogEnv* env, CompileData* data, unsigned int lineno, ID name) 
{
    switch (data->ctx) {
    case CTX_FUNC:
        {
            Var* var = lookup_var(env, data->vars, name);
            if (var == NULL) {
                const char* s = YogVm_id2name(env, env->vm, name);
                YOG_BUG(env, "variable not found (%s)", s);
            }
            switch (var->type) {
            case VT_GLOBAL:
                CompileData_add_store_global(env, data, lineno, name);
                break;
            case VT_LOCAL:
                {
                    unsigned int index = var->u.local.index;
                    CompileData_add_store_local(env, data, lineno, index);
                }
                break;
            case VT_NONLOCAL:
                {
                    unsigned int level = var->u.nonlocal.level;
                    unsigned int index = var->u.nonlocal.index;
                    CompileData_add_store_nonlocal(env, data, lineno, level, index);
                }
                break;
            default:
                YOG_BUG(env, "unknown VarType (%d)", var->type);
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
compile_visit_assign(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    unsigned int lineno = node->lineno;

    YogNode* left = node->u.assign.left;
    switch (left->type) {
    case NODE_VARIABLE:
        {
            visit_node(env, visitor, node->u.assign.right, data);
            CompileData_add_dup(env, data, lineno);
            ID name = left->u.variable.id;
            append_store(env, data, lineno, name);
            break;
        }
    case NODE_SUBSCRIPT:
        {
            visit_node(env, visitor, left->u.subscript.prefix, data);
            visit_node(env, visitor, left->u.subscript.index, data);
            visit_node(env, visitor, node->u.assign.right, data);
            CompileData_add_call_method(env, data, lineno, INTERN("[]="), 2, 0, 0, 0, 0);
            break;
#undef UPDATE_LEFT
        }
    default:
        YOG_ASSERT(env, FALSE, "invalid node type.");
        break;
    }
}

static void 
compile_visit_method_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.method_call.recv, arg);

    visit_each_args(env, visitor, node->u.method_call.args, node->u.method_call.blockarg, arg);

    unsigned int argc = 0;
    YogArray* args = node->u.method_call.args;
    if (args != NULL) {
        argc = YogArray_size(env, args);
        YOG_ASSERT(env, argc < UINT8_MAX + 1, "Too many arguments for method call.");
    }

    uint8_t blockargc = 0;
    if (node->u.method_call.blockarg != NULL) {
        blockargc = 1;
    }

    CompileData* data = arg;
    CompileData_add_call_method(env, data, node->lineno, node->u.method_call.name, argc, 0, blockargc, 0, 0);
}

static int
register_const(YogEnv* env, CompileData* data, YogVal val) 
{
    if (data->const2index == NULL) {
        data->const2index = YogTable_new_symbol_table(env);
    }

    YogTable* const2index = data->const2index;

    YogVal index = YUNDEF;
    if (!YogTable_lookup(env, const2index, val, &index)) {
        int size = YogTable_size(env, const2index);
        index = INT2VAL(size);
        YogTable_add_direct(env, const2index, val, index);
        return size;
    }
    else {
        return VAL2INT(index);
    }
}

static void 
add_push_const(YogEnv* env, CompileData* data, YogVal val, unsigned int lineno) 
{
    unsigned int index = register_const(env, data, val);
    CompileData_add_push_const(env, data, lineno, index);
}

static void 
compile_visit_literal(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    CompileData* data = arg;

    YogVal val = node->u.literal.val;
    unsigned int lineno = node->lineno;
    if (VAL2PTR(YogVal_get_klass(env, val)) == VAL2PTR(ENV_VM(env)->cString)) {
        unsigned int index = register_const(env, data, val);
        CompileData_add_make_string(env, data, lineno, index);
    }
    else {
        add_push_const(env, data, val, lineno);
    }
}

static void 
compile_visit_command_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    visit_each_args(env, visitor, node->u.command_call.args, node->u.command_call.blockarg, arg);

    unsigned int argc = 0;
    YogArray* args = node->u.command_call.args;
    if (args != NULL) {
        argc = YogArray_size(env, args);
        YOG_ASSERT(env, argc < UINT8_MAX + 1, "Too many arguments for command call.");
    }

    uint8_t blockargc = 0;
    if (node->u.command_call.blockarg != NULL) {
        blockargc = 1;
    }

    CompileData* data = arg;
    CompileData_add_call_command(env, data, node->lineno, node->u.command_call.name, argc, 0, blockargc, 0, 0);
}

static int 
table2array_count_index(YogEnv* env, YogVal key, YogVal value, YogVal* arg) 
{
    if (VAL2INT(*arg) < VAL2INT(value)) {
        *arg = value;
    }

    return ST_CONTINUE;
}

static int 
table2array_fill_array(YogEnv* env, YogVal key, YogVal value, YogVal* arg) 
{
    YogValArray* array = VAL2PTR(*arg);
    int index = VAL2INT(value);
    array->items[index] = key;

    return ST_CONTINUE;
}

static YogValArray* 
table2array(YogEnv* env, YogTable* table) 
{
    if (table == NULL) {
        return NULL;
    }

    YogVal max_index = INT2VAL(INT_MIN);
    YogTable_foreach(env, table, table2array_count_index, &max_index);
    int index = VAL2INT(max_index);
    if (0 <= index) {
        unsigned int size = index + 1;
        YogValArray* array = YogValArray_new(env, size);
        YogVal arg = PTR2VAL(array);
        YogTable_foreach(env, table, table2array_fill_array, &arg);
        return array;
    }
    else {
        return NULL;
    }
}

static void 
make_exception_table(YogEnv* env, YogCode* code, CompileData* data)
{
    unsigned int size = 0;
    ExceptionTableEntry* entry = data->exc_tbl;
    while (entry != NULL) {
        if (entry->from != NULL) {
            pc_t from = entry->from->pc;
            pc_t to = entry->to->pc;
            if (from != to) {
                size++;
            }
        }

        entry = entry->next;
    }

    if (0 < size) {
        YogExceptionTable* exc_tbl = ALLOC_OBJ_ITEM(env, NULL, NULL, YogExceptionTable, size, YogExceptionTableEntry);

        unsigned int i = 0;
        entry = data->exc_tbl;
        while (entry != NULL) {
            if (entry->from != NULL) {
                pc_t from = entry->from->pc;
                pc_t to = entry->to->pc;
                if (from != to) {
                    YogExceptionTableEntry* ent = &exc_tbl->items[i];
                    ent->from = from;
                    ent->to = to;
                    ent->target = entry->target->pc;

                    i++;
                }
            }

            entry = entry->next;
        }

        code->exc_tbl = exc_tbl;
        code->exc_tbl_size = size;
    }
    else {
        code->exc_tbl = NULL;
        code->exc_tbl_size = 0;
    }
}

static void 
make_lineno_table(YogEnv* env, YogCode* code, YogInst* anchor)
{
    YogInst* inst = anchor;
    unsigned int lineno = 0;
    unsigned int size = 0;
    while (inst != NULL) {
        if (inst->type == INST_OP) {
            if (lineno != inst->lineno) {
                lineno = inst->lineno;
                size++;
            }
        }

        inst = inst->next;
    }

    YogLinenoTableEntry* tbl = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(YogLinenoTableEntry) * size);
    if (0 < size) {
        inst = anchor;
        int i = -1;
        lineno = 0;
        while (inst != NULL) {
            if (inst->type == INST_OP) {
                if (lineno != inst->lineno) {
                    i++;
                    YogLinenoTableEntry* entry = &tbl[i];
                    pc_t pc = inst->pc;
                    entry->pc_from = pc;
                    entry->pc_to = pc + inst->size;
                    lineno = inst->lineno;
                    entry->lineno = lineno;
                }
                else {
                    YogLinenoTableEntry* entry = &tbl[i];
                    entry->pc_to = inst->pc + inst->size;
                }
            }

            inst = inst->next;
        }
    }

    code->lineno_tbl = tbl;
    code->lineno_tbl_size = size;
}

static void 
ExceptionTableEntry_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    ExceptionTableEntry* entry = ptr;
#define KEEP(member)    entry->member = (*keeper)(env, entry->member)
    KEEP(next);
    KEEP(from);
    KEEP(to);
    KEEP(target);
#undef KEEP
}

static ExceptionTableEntry* 
ExceptionTableEntry_new(YogEnv* env) 
{
    ExceptionTableEntry* entry = ALLOC_OBJ(env, ExceptionTableEntry_keep_children, NULL, ExceptionTableEntry);
    entry->next = NULL;
    entry->from = NULL;
    entry->to = NULL;
    entry->target = NULL;

    return entry;
}

static void 
CompileData_add_inst(CompileData* data, YogInst* inst) 
{
    data->last_inst->next = inst;

    while (inst->next != NULL) {
        inst = inst->next;
    }
    data->last_inst = inst;
}

static void 
calc_pc(YogInst* inst) 
{
    pc_t pc = 0;
    while (inst != NULL) {
        switch (inst->type) {
        case INST_OP:
        case INST_LABEL:
            inst->pc = pc;
            break;
        default:
            break;
        }

        if (inst->type == INST_OP) {
            unsigned int size = Yog_get_inst_size(INST_OPCODE(inst));
            inst->size = size;
            pc += size;
        }

        inst = inst->next;
    }
}

static void 
CompileData_add_ret_nil(YogEnv* env, CompileData* data, unsigned int lineno) 
{
    add_push_const(env, data, YNIL, lineno);
    CompileData_add_ret(env, data, lineno);
}

static int 
count_locals_callback(YogEnv* env, YogVal key, YogVal val, YogVal* arg) 
{
    Var* var = VAL2PTR(val);
    if (var->type == VT_LOCAL) {
        int nlocals = VAL2INT(*arg);
        *arg = INT2VAL(nlocals + 1);
    }

    return ST_CONTINUE;
}

static int 
count_locals(YogEnv* env, YogTable* vars)
{
    YogVal val = INT2VAL(0);
    YogTable_foreach(env, vars, count_locals_callback, &val);

    return VAL2INT(val);
}

static void 
CompileData_initialize(CompileData* data, Context ctx, YogTable* vars, YogInst* anchor, ExceptionTableEntry* exc_tbl_ent, const char* filename, ID klass_name) 
{
    data->ctx = ctx;
    data->vars = vars;
    data->const2index = NULL;
    data->label_while_start = NULL;
    data->label_while_end = NULL;
    data->finally_list = NULL;
    data->try_list = NULL;
    data->filename = NULL;
    data->klass_name = INVALID_ID;
    data->last_inst = anchor;
    data->exc_tbl = exc_tbl_ent;
    data->exc_tbl_last = exc_tbl_ent;
    data->filename = filename;
    data->klass_name = klass_name;
    data->outer = NULL;
}

static int 
get_max_outer_level_callback(YogEnv* env, YogVal key, YogVal val, YogVal* arg) 
{
    Var* var = VAL2PTR(val);
    if (var->type == VT_NONLOCAL) {
        int max_level = VAL2INT(*arg);
        int level = var->u.nonlocal.level;
        if (max_level < level) {
            *arg = INT2VAL(level);
        }
    }

    return ST_CONTINUE;
}

static unsigned int 
get_max_outer_level(YogEnv* env, YogTable* vars) 
{
    YogVal arg = INT2VAL(0);
    YogTable_foreach(env, vars, get_max_outer_level_callback, &arg);

    return VAL2INT(arg);
}

struct AllocLocalVarsTableArg {
    ID* names;
    unsigned int count;
};

typedef struct AllocLocalVarsTableArg AllocLocalVarsTableArg;

static int 
alloc_local_vars_table_callback(YogEnv* env, YogVal key, YogVal val, YogVal* arg) 
{
    Var* var = VAL2PTR(val);
    if (var->type == VT_LOCAL) {
        AllocLocalVarsTableArg* p = VAL2PTR(*arg);
        Var* var = VAL2PTR(val);
        unsigned int index = var->u.local.index;
        YOG_ASSERT(env, index < p->count, "local var index over count");
        p->names[index] = VAL2ID(key);
    }

    return ST_CONTINUE;
}

static ID* 
alloc_local_vars_table(YogEnv* env, YogTable* vars, unsigned int count) 
{
    ID* names = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(ID) * count);

    AllocLocalVarsTableArg arg;
    arg.names = names;
    arg.count = count;
    YogVal val = PTR2VAL(&arg);

    YogTable_foreach(env, vars, alloc_local_vars_table_callback, &val);

    return names;
}

static YogCode* 
compile_stmts(YogEnv* env, AstVisitor* visitor, const char* filename, ID klass_name, ID func_name, YogArray* stmts, YogTable* vars, Context ctx, YogInst* tail) 
{
    YogInst* anchor = Anchor_new(env);
    ExceptionTableEntry* exc_tbl_ent = ExceptionTableEntry_new(env);
    CompileData data;
    CompileData_initialize(&data, ctx, vars, anchor, exc_tbl_ent, filename, klass_name);

    visitor->visit_stmts(env, visitor, stmts, &data);
    if (tail != NULL) {
        CompileData_add_inst(&data, tail);
    }
    if (ctx == CTX_FUNC) {
        if (stmts != NULL) {
            unsigned int size = YogArray_size(env, stmts);
            if (size < 1) {
                CompileData_add_ret_nil(env, &data, 0);
            }
            else {
                YogVal val = YogArray_at(env, stmts, size - 1);
                YogNode* node = VAL2PTR(val);
                if (node->type != NODE_RETURN) {
                    CompileData_add_ret_nil(env, &data, node->lineno);
                }
            }
        }
        else {
            CompileData_add_ret_nil(env, &data, 0);
        }
    }

    calc_pc(anchor);
    YogBinary* bin = insts2bin(env, anchor);
    YogBinary_shrink(env, bin);

    YogCode* code = YogCode_new(env);
    code->local_vars_count = count_locals(env, vars);
    code->local_vars_names = alloc_local_vars_table(env, vars, code->local_vars_count);
    code->stack_size = count_stack_size(env, anchor);
    code->consts = table2array(env, data.const2index);
    code->insts = bin->body;
    code->outer_size = get_max_outer_level(env, vars);

    make_exception_table(env, code, &data);
    make_lineno_table(env, code, anchor);

    code->filename = filename;
    code->klass_name = klass_name;
    code->func_name = func_name;

#if 0
    YogCode_dump(env, code);
#endif

    return code;
}

static void 
register_params_var_table(YogEnv* env, YogArray* params, YogTable* var_tbl) 
{
    if (params == NULL) {
        return;
    }

    unsigned int size = YogArray_size(env, params);
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal param = YogArray_at(env, params, i);
        YogNode* node = VAL2PTR(param);
        ID id = node->u.param.name;
        YogVal name = ID2VAL(id);
        if (YogTable_lookup(env, var_tbl, name, NULL)) {
            YOG_ASSERT(env, FALSE, "duplicated argument name in function definition");
        }
        scan_var_register(env, var_tbl, id, VAR_PARAM);
    }
}

#if 0
static void 
register_block_params_var_table(YogEnv* env, YogArray* params, YogTable* var_tbl) 
{
    if (params == NULL) {
        return;
    }

    unsigned int size = YogArray_size(env, params);
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal param = YogArray_at(env, params, i);
        YogNode* node = VAL2PTR(param);
        ID id = node->u.param.name;
        YogVal name = ID2VAL(id);
        if (!YogTable_lookup(env, var_tbl, name, NULL)) {
            scan_var_register(env, var_tbl, id, VAR_ASSIGNED);
        }
    }
}
#endif

static unsigned int 
lookup_local_var_index(YogEnv* env, YogTable* vars, ID name) 
{
    Var* var = lookup_var(env, vars, name);
    if (var->type == VT_LOCAL) {
        return var->u.local.index;
    }
    else {
        return 0;
    }
}

static void 
setup_params(YogEnv* env, YogTable* vars, YogArray* params, YogCode* code) 
{
    YogArgInfo* arg_info = &code->arg_info;
    arg_info->argc = 0;
    arg_info->argnames = NULL;
    arg_info->arg_index = 0;
    arg_info->blockargc = 0;
    arg_info->blockargname = 0;
    arg_info->blockarg_index = 0;
    arg_info->varargc = 0;
    arg_info->vararg_index = 0;
    arg_info->kwargc = 0;
    arg_info->kwarg_index = 0;

    if (params == NULL) {
        return;
    }

    unsigned int size = YogArray_size(env, params);
    unsigned int argc = 0;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal val = YogArray_at(env, params, i);
        YogNode* node = VAL2PTR(val);
        if (node->type != NODE_PARAM) {
            break;
        }
        argc++;
    }

    ID* argnames = NULL;
    uint8_t* arg_index = NULL;
    if (0 < argc) {
        argnames = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(ID) * argc);
        arg_index = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(uint8_t) * argc);
        for (i = 0; i < argc; i++) {
            YogVal val = YogArray_at(env, params, i);
            YogNode* node = VAL2PTR(val);
            YOG_ASSERT(env, node->type == NODE_PARAM, "Node must be NODE_PARAM.");

            ID name = node->u.param.name;
            argnames[i] = name;
            arg_index[i] = lookup_local_var_index(env, vars, name);
        }
    }

    arg_info->argc = argc;
    arg_info->argnames = argnames;
    arg_info->arg_index = arg_index;
    if (size == argc) {
        return;
    }

    unsigned int n = argc;
    YogVal val = YogArray_at(env, params, n);
    YogNode* node = VAL2PTR(val);
    if (node->type == NODE_BLOCK_PARAM) {
        arg_info->blockargc = 1;

        ID name = node->u.param.name;
        arg_info->blockargname = name;
        arg_info->blockarg_index = lookup_local_var_index(env, vars, name);

        n++;
        if (size == n) {
            return;
        }
        val = YogArray_at(env, params, n);
        node = VAL2PTR(val);
    }

    if (node->type == NODE_VAR_PARAM) {
        arg_info->varargc = 1;

        ID name = node->u.param.name;
        arg_info->vararg_index = lookup_local_var_index(env, vars, name);

        n++;
        if (size == n) {
            return;
        }
        val = YogArray_at(env, params, n);
        node = VAL2PTR(val);
    }

    YOG_ASSERT(env, node->type == NODE_KW_PARAM, "Node must be NODE_KW_PARAM.");
    arg_info->kwargc = 1;

    ID name = node->u.param.name;
    arg_info->kwarg_index = lookup_local_var_index(env, vars, name);

    n++;
    YOG_ASSERT(env, size == n, "Parameters count is unmatched.");
}

static void 
register_self(YogEnv* env, YogTable* var_tbl) 
{
    ID name = INTERN("self");
    scan_var_register(env, var_tbl, name, VAR_PARAM);
}

static BOOL
find_outer_var(YogEnv* env, ID name, CompileData* outer, unsigned int* plevel, unsigned int* pindex) 
{
    YogVal key = ID2VAL(name);

    int level = 0;
    while (outer != NULL) {
        if (outer->outer == NULL) {
            return FALSE;
        }

        YogVal val = YUNDEF;
        if (YogTable_lookup(env, outer->vars, key, &val)) {
            Var* var = VAL2PTR(val);
            switch (var->type) {
            case VT_GLOBAL:
                return FALSE;
                break;
            case VT_LOCAL:
                *plevel = level + 1;
                *pindex = var->u.local.index;
                return TRUE;
                break;
            case VT_NONLOCAL:
                *plevel = level + var->u.nonlocal.level + 1;
                *pindex = var->u.nonlocal.index;
                return TRUE;
                break;
            default:
                YOG_BUG(env, "unknown VarType (%d)", var->type);
                break;
            }
        }

        level++;
        outer = outer->outer;
    }

    return FALSE;
}

static Var* 
Var_new(YogEnv* env) 
{
    return ALLOC_OBJ(env, NULL, NULL, Var);
}

static YogTable* 
var_table_new(YogEnv* env) 
{
    YogTable* var_tbl = YogTable_new_symbol_table(env);
    register_self(env, var_tbl);
    return var_tbl;
}

struct Flags2TypeArg {
    struct YogTable* vars;
    struct CompileData* data;
};

typedef struct Flags2TypeArg Flags2TypeArg;

static int 
vars_flags2type_callback(YogEnv* env, YogVal key, YogVal val, YogVal* arg) 
{
    ID name = VAL2ID(key);
    ScanVarEntry* ent = VAL2PTR(val);
    int flags = ent->flags;
    Flags2TypeArg* parg = VAL2PTR(*arg);
    Var* var = Var_new(env);
    if (IS_NONLOCAL(flags) || (!IS_ASSIGNED(flags) && !IS_PARAM(flags))) {
        unsigned int level = 0;
        unsigned int index = 0;
        if (find_outer_var(env, name, parg->data, &level, &index)) {
            var->type = VT_NONLOCAL;
            var->u.nonlocal.level = level;
            var->u.nonlocal.index = index;
        }
        else {
            var->type = VT_GLOBAL;
        }
    }
    else {
        var->type = VT_LOCAL;
        var->u.local.index = ent->index;
    }

    YogTable_add_direct(env, parg->vars, key, PTR2VAL(var));

    return ST_CONTINUE;
}

static YogTable* 
vars_flags2type(YogEnv* env, YogTable* var_tbl, CompileData* outer) 
{
    YogTable* vars = YogTable_new_symbol_table(env);

    Flags2TypeArg arg;
    arg.vars = vars;
    arg.data = outer;
    YogVal val = PTR2VAL(&arg);
    YogTable_foreach(env, var_tbl, vars_flags2type_callback, &val);

    return vars;
}

static YogCode* 
compile_func(YogEnv* env, AstVisitor* visitor, const char* filename, ID klass_name, YogNode* node, CompileData* upper) 
{
    YogTable* var_tbl = var_table_new(env);

    YogArray* params = node->u.funcdef.params;
    register_params_var_table(env, params, var_tbl);

    YogArray* stmts = node->u.funcdef.stmts;
    make_var_table(env, stmts, var_tbl);
    YogTable* vars = vars_flags2type(env, var_tbl, upper);

    ID func_name = node->u.funcdef.name;

    YogCode* code = compile_stmts(env, visitor, filename, klass_name, func_name, stmts, vars, CTX_FUNC, NULL);
    setup_params(env, vars, params, code);

    return code;
}

static void 
compile_visit_func_def(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    CompileData* data = arg;

    ID klass_name = INVALID_ID;
    if (data->ctx == CTX_KLASS) {
        klass_name = data->klass_name;
    }

    YogCode* code = compile_func(env, visitor, data->filename, klass_name, node, data);

    YogVal val = PTR2VAL(code);
    add_push_const(env, data, val, node->lineno);

    ID id = node->u.funcdef.name;
    unsigned int lineno = node->lineno;
    switch (data->ctx) {
    case CTX_FUNC:
        YOG_ASSERT(env, FALSE, "TODO: NOT IMPLEMENTED");
        break;
    case CTX_KLASS:
    case CTX_PKG:
        {
            switch (data->ctx) {
            case CTX_KLASS:
                CompileData_add_make_method(env, data, lineno);
                break;
            case CTX_PKG:
                CompileData_add_make_package_method(env, data, lineno);
                break;
            default:
                YOG_ASSERT(env, FALSE, "Invalid context type.");
                break;
            }
            CompileData_add_store_name(env, data, lineno, id);
            break;
        }
    default:
        YOG_ASSERT(env, FALSE, "Unknown context.");
        break;
    }
}

static void 
compile_visit_func_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.func_call.callee, arg);

    visit_each_args(env, visitor, node->u.func_call.args, node->u.func_call.blockarg, arg);

    unsigned int argc = 0;
    YogArray* args = node->u.func_call.args;
    if (args != NULL) {
        argc = YogArray_size(env, args);
    }

    uint8_t blockargc = 0;
    if (node->u.func_call.blockarg != NULL) {
        blockargc = 1;
    }

    CompileData* data = arg;
    CompileData_add_call_function(env, data, node->lineno, argc, 0, blockargc, 0, 0);
}

static void 
compile_visit_variable(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    CompileData* data = arg;
    ID id = node->u.variable.id;
    unsigned int lineno = node->lineno;
    switch (data->ctx) {
    case CTX_FUNC:
        {
            Var* var = lookup_var(env, data->vars, id);
            YOG_ASSERT(env, var != NULL, "can't find variable");
            switch (var->type) {
            case VT_GLOBAL:
                CompileData_add_load_global(env, data, lineno, id);
                break;
            case VT_LOCAL:
                {
                    unsigned int index = var->u.local.index;
                    CompileData_add_load_local(env, data, lineno, index);
                }
                break;
            case VT_NONLOCAL:
                {
                    unsigned int level = var->u.nonlocal.level;
                    unsigned int index = var->u.nonlocal.index;
                    CompileData_add_load_nonlocal(env, data, lineno, level, index);
                }
                break;
            default:
                YOG_BUG(env, "unknown variable type (%d)", var->type);
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
}

static unsigned int 
get_last_lineno(YogEnv* env, YogArray* stmts) 
{
    unsigned int size = YogArray_size(env, stmts);
    YogVal val = YogArray_at(env, stmts, size - 1);
    YogNode* node = VAL2PTR(val);

    return node->lineno;
}

static void 
compile_visit_finally(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    CompileData* data = arg;

    YogInst* label_head_start = Label_new(env);
    YogInst* label_head_end = Label_new(env);
    YogInst* label_finally_error_start = Label_new(env);
    YogInst* label_finally_end = Label_new(env);

    FinallyListEntry finally_list_entry;
    finally_list_entry.prev = data->finally_list;
    data->finally_list = &finally_list_entry;
    finally_list_entry.node = node;

    PUSH_TRY();

    ExceptionTableEntry* entry = ExceptionTableEntry_new(env);
    entry->next = NULL;
    entry->from = label_head_start;
    entry->to = label_head_end;
    entry->target = label_finally_error_start;
    try_list_entry.exc_tbl = entry;

    add_inst(data, label_head_start);
    visitor->visit_stmts(env, visitor, node->u.finally.head, arg);
    add_inst(data, label_head_end);

    YogArray* stmts = node->u.finally.body;
    visitor->visit_stmts(env, visitor, stmts, arg);
    unsigned int lineno = get_last_lineno(env, stmts);
    CompileData_add_jump(env, data, lineno, label_finally_end);

    add_inst(data, label_finally_error_start);
    visitor->visit_stmts(env, visitor, stmts, arg);
    lineno = get_last_lineno(env, stmts);
    RERAISE();

    add_inst(data, label_finally_end);

    PUSH_EXCEPTION_TABLE_ENTRY();

    POP_TRY();

    data->finally_list = finally_list_entry.prev;
}

static void 
compile_visit_except(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    YogInst* label_head_start = Label_new(env);
    YogInst* label_head_end = Label_new(env);
    YogInst* label_excepts_start = Label_new(env);
    YogInst* label_else_start = Label_new(env);
    YogInst* label_else_end = Label_new(env);

    PUSH_TRY();

    ExceptionTableEntry* entry = ExceptionTableEntry_new(env);
    entry->next = NULL;
    entry->from = label_head_start;
    entry->to = label_head_end;
    entry->target = label_excepts_start;
    try_list_entry.exc_tbl = entry;

    add_inst(data, label_head_start);
    YogArray* stmts = node->u.except.head;
    visitor->visit_stmts(env, visitor, stmts, arg);
    add_inst(data, label_head_end);
    unsigned int lineno = get_last_lineno(env, stmts);
    CompileData_add_jump(env, data, lineno, label_else_start);

    add_inst(data, label_excepts_start);
    YogArray* excepts = node->u.except.excepts;
    unsigned int size = YogArray_size(env, excepts);
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogInst* label_body_end = Label_new(env);

        YogVal val = YogArray_at(env, excepts, i);
        YogNode* node = VAL2PTR(val);

        YogNode* node_type = node->u.except_body.type;
        if (node_type != NULL) {
            visit_node(env, visitor, node_type, arg);
#define LOAD_EXC()  CompileData_add_load_special(env, data, lineno, INTERN("$!"))
            lineno = node_type->lineno;
            LOAD_EXC();
            CompileData_add_call_method(env, data, lineno, INTERN("==="), 1, 0, 0, 0, 0);
            CompileData_add_jump_if_false(env, data, lineno, label_body_end);

            ID id = node->u.except_body.var;
            if (id != NO_EXC_VAR) {
                LOAD_EXC();
                append_store(env, data, node->lineno, id);
            }
#undef LOAD_EXC
        }

        YogArray* stmts = node->u.except_body.stmts;
        visitor->visit_stmts(env, visitor, stmts, arg);
        lineno = get_last_lineno(env, stmts);
        CompileData_add_jump(env, data, lineno, label_else_end);

        add_inst(data, label_body_end);
    }
    RERAISE();

    add_inst(data, label_else_start);
    visitor->visit_stmts(env, visitor, node->u.except.else_, arg);
    add_inst(data, label_else_end);

    PUSH_EXCEPTION_TABLE_ENTRY();

    POP_TRY();
}

static void 
compile_visit_while(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    YogInst* while_start = Label_new(env);
    YogInst* while_end = Label_new(env);

    YogInst* label_while_start_prev = data->label_while_start;
    YogInst* label_while_end_prev = data->label_while_end;
    data->label_while_start = while_start;
    data->label_while_end = while_end;

    add_inst(data, while_start);
    YogNode* test = node->u.while_.test;
    visit_node(env, visitor, test, arg);
    CompileData_add_jump_if_false(env, data, test->lineno, while_end);

    YogArray* stmts = node->u.while_.stmts;
    visitor->visit_stmts(env, visitor, stmts, arg);
    unsigned int lineno = get_last_lineno(env, stmts);
    CompileData_add_jump(env, data, lineno, while_start);
    add_inst(data, while_end);

    data->label_while_end = label_while_end_prev;
    data->label_while_start = label_while_start_prev;
}

static void 
compile_visit_if(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    YogInst* label_tail_start = Label_new(env);
    YogInst* label_stmt_end = Label_new(env);

    YogNode* test = node->u.if_.test;
    visit_node(env, visitor, test, arg);
    CompileData_add_jump_if_false(env, data, test->lineno, label_tail_start);

    YogArray* stmts = node->u.if_.stmts;
    unsigned int lineno = 0;
    if (stmts != NULL) {
        visitor->visit_stmts(env, visitor, stmts, arg);
        lineno = get_last_lineno(env, stmts);
    }
    else {
        lineno = test->lineno;
    }
    CompileData_add_jump(env, data, lineno, label_stmt_end);

    add_inst(data, label_tail_start);
    YogArray* tail = node->u.if_.tail;
    if (tail != NULL) {
        visitor->visit_stmts(env, visitor, tail, arg);
    }
    add_inst(data, label_stmt_end);
}

static void 
split_exception_table(YogEnv* env, ExceptionTableEntry* exc_tbl, YogInst* label_from, YogInst* label_to)
{
    ExceptionTableEntry* entry = exc_tbl;
    YOG_ASSERT(env, entry != NULL, "Exception table is empty.");
    while (entry->next != NULL) {
        entry = entry->next;
    }

    ExceptionTableEntry* new_entry = ExceptionTableEntry_new(env);
    new_entry->from = label_to;
    new_entry->to = entry->to;
    new_entry->target = entry->target;
    entry->to = label_from;
    entry->next = new_entry;
}

static void 
compile_while_jump(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg, YogInst* jump_to) 
{
    CompileData* data = arg;

    YogNode* expr = NULL;
    if (node->type == NODE_BREAK) {
        expr = node->u.break_.expr;
    }
    else {
        expr = node->u.next.expr;
    }

    if (data->label_while_start != NULL) {
        YOG_ASSERT(env, expr == NULL, "Can't return value with break/next.");
        FinallyListEntry* finally_list_entry = data->finally_list;
        while (finally_list_entry != NULL) {
            YogInst* label_start = Label_new(env);
            YogInst* label_end = Label_new(env);

            add_inst(data, label_start);
            visitor->visit_stmts(env, visitor, finally_list_entry->node->u.finally.body, arg);
            add_inst(data, label_end);

            TryListEntry* try_list_entry = data->try_list;
            while (TRUE) {
                split_exception_table(env, try_list_entry->exc_tbl, label_start, label_end);
                if (try_list_entry->node == finally_list_entry->node) {
                    break;
                }

                try_list_entry = try_list_entry->prev;
            }

            finally_list_entry = finally_list_entry->prev;
        }
        CompileData_add_jump(env, data, node->lineno, jump_to);
    }
    else {
        /* TODO */
    }
}

static void 
compile_visit_break(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;
    compile_while_jump(env, visitor, node, arg, data->label_while_end);
}

static void 
compile_visit_next(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;
    compile_while_jump(env, visitor, node, arg, data->label_while_start);
}

static YogCode* 
compile_block(YogEnv* env, AstVisitor* visitor, YogNode* node, CompileData* data) 
{
    YogArray* params = node->u.blockarg.params;
    YogTable* vars = data->vars;
#if 0
    register_block_params_var_table(env, params, var_tbl);
    YogTable* vars = vars_flags2type(env, var_tbl, data->upper);
#endif

    const char* filename = data->filename;
    ID klass_name = INVALID_ID;
    ID func_name = INTERN("<block>");
    YogArray* stmts = node->u.blockarg.stmts;
    YogCode* code = compile_stmts(env, visitor, filename, klass_name, func_name, stmts, vars, data->ctx, NULL);

    setup_params(env, vars, params, code);

    return code;
}

static void 
compile_visit_block(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    YogCode* code = compile_block(env, visitor, node, data);

    YogVal val = PTR2VAL(code);
    unsigned int lineno = node->lineno;
    add_push_const(env, data, val, lineno);
    switch (data->ctx) {
        case CTX_FUNC:
            CompileData_add_make_block(env, data, lineno);
            break;
        case CTX_KLASS:
            YOG_BUG(env, "not implemented");
            break;
        case CTX_PKG:
            CompileData_add_make_package_block(env, data, lineno);
            break;
        default:
            YOG_ASSERT(env, FALSE, "Unknown context.");
            break;
    }
}

static YogCode* 
compile_klass(YogEnv* env, AstVisitor* visitor, ID klass_name, YogArray* stmts, CompileData* data)
{
    YogTable* var_tbl = make_var_table(env, stmts, NULL);
    YogTable* vars = vars_flags2type(env, var_tbl, data->outer);

    unsigned int lineno = get_last_lineno(env, stmts);

    YogInst* ret = Inst_new(env, lineno);
    ret->next = NULL;
    ret->opcode = OP(RET);
    YogInst* push_self_name = Inst_new(env, lineno);
    push_self_name->next = ret;
    push_self_name->opcode = OP(PUSH_SELF_NAME);

    const char* filename = data->filename;
    ID func_name = INVALID_ID;

    YogCode* code = compile_stmts(env, visitor, filename, klass_name, func_name, stmts, vars, CTX_KLASS, push_self_name);

    return code;
}

static void 
compile_visit_klass(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    ID name = node->u.klass.name;
    YogVal val = ID2VAL(name);
    add_push_const(env, data, val, node->lineno);

    YogNode* super = node->u.klass.super;
    if (super != NULL) {
        visit_node(env, visitor, super, data);
    }
    else {
        YogVal cObject = ENV_VM(env)->cObject;
        add_push_const(env, data, cObject, node->lineno);
    }

    YogArray* stmts = node->u.klass.stmts;
    YogCode* code = compile_klass(env, visitor, name, stmts, data);
    val = PTR2VAL(code);
    unsigned int lineno = node->lineno;
    add_push_const(env, data, val, lineno);

    CompileData_add_make_klass(env, data, lineno);

    append_store(env, data, lineno, name);
}

static void 
compile_visit_return(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    YogNode* expr = node->u.return_.expr;
    unsigned int lineno = node->lineno;
    if (expr != NULL) {
        visit_node(env, visitor, expr, data);
    }
    else {
        add_push_const(env, data, YNIL, lineno);
    }

    CompileData_add_ret(env, data, lineno);
}

static void 
compile_visit_subscript(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    visit_node(env, visitor, node->u.subscript.prefix, arg);
    visit_node(env, visitor, node->u.subscript.index, arg);

    CompileData* data = arg;
    ID method = INTERN("[]");
    CompileData_add_call_method(env, data, node->lineno, method, 1, 0, 0, 0, 0);
}

static void 
compile_init_visitor(AstVisitor* visitor) 
{
    visitor->visit_assign = compile_visit_assign;
    visitor->visit_block = compile_visit_block;
    visitor->visit_break = compile_visit_break;
    visitor->visit_command_call = compile_visit_command_call;
    visitor->visit_except = compile_visit_except;
    visitor->visit_except_body = NULL;
    visitor->visit_finally = compile_visit_finally;
    visitor->visit_func_call = compile_visit_func_call;
    visitor->visit_func_def = compile_visit_func_def;
    visitor->visit_if = compile_visit_if;
    visitor->visit_klass = compile_visit_klass;
    visitor->visit_literal = compile_visit_literal;
    visitor->visit_method_call = compile_visit_method_call;
    visitor->visit_next = compile_visit_next;
    visitor->visit_nonlocal = NULL;
    visitor->visit_return = compile_visit_return;
    visitor->visit_stmt = visit_node;
    visitor->visit_stmts = compile_visit_stmts;
    visitor->visit_subscript = compile_visit_subscript;
    visitor->visit_variable = compile_visit_variable;
    visitor->visit_while = compile_visit_while;
}

YogCode* 
YogCompiler_compile_module(YogEnv* env, const char* filename, YogArray* stmts) 
{
    YogTable* var_tbl = make_var_table(env, stmts, NULL);
    YogTable* vars = vars_flags2type(env, var_tbl, NULL);

    AstVisitor visitor;
    compile_init_visitor(&visitor);

    if (filename == NULL) {
        filename = "<stdin>";
    }
    filename = YogString_dup(env, filename);

    ID klass_name = INVALID_ID;
    ID func_name = INTERN("<module>");

    YogCode* code = compile_stmts(env, &visitor, filename, klass_name, func_name, stmts, vars, CTX_PKG, NULL);

    return code;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
