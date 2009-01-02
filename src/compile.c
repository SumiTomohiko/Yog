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
    VisitNode visit_return;
    VisitNode visit_stmt;
    VisitNode visit_subscript;
    VisitNode visit_variable;
    VisitNode visit_while;
};

struct Var2IndexData {
    YogTable* var2index;
};

typedef struct Var2IndexData Var2IndexData;

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
    struct YogTable* var2index;
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
};

typedef struct CompileData CompileData;

#define VISIT_EACH_ARGS(args, blockarg)   do { \
    if (args != NULL) { \
        unsigned int argc = YogArray_size(env, args); \
        unsigned int i = 0; \
        for (i = 0; i < argc; i++) { \
            YogVal val = YogArray_at(env, args, i); \
            YogNode* node = VAL2PTR(val); \
            visit_node(env, visitor, node, arg); \
        } \
    } \
    if (blockarg != NULL) { \
        visit_node(env, visitor, blockarg, arg); \
    } \
} while (0)

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

#define ADD_PUSH_CONST(val, lineno) do { \
    unsigned int index = register_const(env, data, val); \
    CompileData_add_push_const(env, data, lineno, index); \
} while (0)

static void 
Inst_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
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
    YogInst* inst = ALLOC_OBJ(env, Inst_keep_children, NULL, YogInst);
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
var2index_visit_method_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.method_call.recv, arg);
    VISIT_EACH_ARGS(node->u.method_call.args, node->u.method_call.blockarg);
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
        visitor->visit_stmt(env, visitor, node, arg);

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
var2index_visit_stmts(YogEnv* env, AstVisitor* visitor, YogArray* stmts, void* arg) 
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

static void 
var2index_register(YogEnv* env, YogTable* var2index, ID var)
{
    YogVal symbol = ID2VAL(var);
    if (!YogTable_lookup(env, var2index, symbol, NULL)) {
        int size = YogTable_size(env, var2index);
        YogVal index = INT2VAL(size);
        YogTable_add_direct(env, var2index, symbol, index);
    }
}

static void 
var2index_visit_assign(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    YogNode* left = node->u.assign.left;
    if (left->type == NODE_VARIABLE) {
        ID id = left->u.variable.id;
        Var2IndexData* data = arg;
        var2index_register(env, data->var2index, id);
    }
    else {
        visit_node(env, visitor, left, arg);
    }
}

static void 
var2index_visit_command_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    VISIT_EACH_ARGS(node->u.command_call.args, node->u.command_call.blockarg);
}

static void 
var2index_visit_func_def(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    ID id = node->u.funcdef.name;
    Var2IndexData* data = arg;
    var2index_register(env, data->var2index, id);
}

static void 
var2index_visit_func_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    VISIT_EACH_ARGS(node->u.func_call.args, node->u.func_call.blockarg);
}

static void 
var2index_visit_finally(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visitor->visit_stmts(env, visitor, node->u.finally.head, arg);
    visitor->visit_stmts(env, visitor, node->u.finally.body, arg);
}

static void 
var2index_visit_except_body(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.except_body.type, arg);
    ID id = node->u.except_body.var;
    if (id != NO_EXC_VAR) {
        Var2IndexData* data = arg;
        var2index_register(env, data->var2index, id);
    }
    visitor->visit_stmts(env, visitor, node->u.except_body.stmts, arg);
}

static void 
var2index_visit_while(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.while_.test, arg);
    visitor->visit_stmts(env, visitor, node->u.while_.stmts, arg);
}

static void 
var2index_visit_if(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.if_.test, arg);
    visitor->visit_stmts(env, visitor, node->u.if_.stmts, arg);
    visitor->visit_stmts(env, visitor, node->u.if_.tail, arg);
}

static void 
var2index_visit_break(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.break_.expr, arg);
}

static void 
var2index_visit_except(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
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
var2index_visit_block(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    Var2IndexData* data = arg;

    YogArray* params = node->u.blockarg.params;
    if (params != NULL) {
        unsigned int size = YogArray_size(env, params);
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            YogVal val = YogArray_at(env, params, i);
            YogNode* param = VAL2PTR(val);
            ID name = param->u.param.name;
            var2index_register(env, data->var2index, name);
            visit_node(env, visitor, param->u.param.default_, arg);
        }
    }

    visitor->visit_stmts(env, visitor, node->u.blockarg.stmts, arg);
}

static void 
var2index_visit_klass(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    Var2IndexData* data = arg;
    ID name = node->u.klass.name;
    var2index_register(env, data->var2index, name);

    YogNode* super = node->u.klass.super;
    visit_node(env, visitor, super, arg);
}

static void 
var2index_visit_subscript(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.subscript.prefix, arg);
    visit_node(env, visitor, node->u.subscript.index, arg);
}

static void 
var2index_init_visitor(AstVisitor* visitor) 
{
    visitor->visit_assign = var2index_visit_assign;
    visitor->visit_block = var2index_visit_block;
    visitor->visit_break = var2index_visit_break;
    visitor->visit_command_call = var2index_visit_command_call;
    visitor->visit_except = var2index_visit_except;
    visitor->visit_except_body = var2index_visit_except_body;
    visitor->visit_finally = var2index_visit_finally;
    visitor->visit_func_call = var2index_visit_func_call;
    visitor->visit_func_def = var2index_visit_func_def;
    visitor->visit_if = var2index_visit_if;
    visitor->visit_klass = var2index_visit_klass;
    visitor->visit_literal = NULL;
    visitor->visit_method_call = var2index_visit_method_call;
    visitor->visit_next = var2index_visit_break;
    visitor->visit_return = var2index_visit_break;
    visitor->visit_stmt = visit_node;
    visitor->visit_stmts = var2index_visit_stmts;
    visitor->visit_subscript = var2index_visit_subscript;
    visitor->visit_variable = NULL;
    visitor->visit_while = var2index_visit_while;
}

static YogTable*
make_var2index(YogEnv* env, YogArray* stmts, YogTable* var2index)
{
    AstVisitor visitor;
    var2index_init_visitor(&visitor);

    if (var2index == NULL) {
        var2index = YogTable_new_symbol_table(env);
    }
    Var2IndexData data;
    data.var2index = var2index;

    visitor.visit_stmts(env, &visitor, stmts, &data);

    return var2index;
}

static int 
lookup_var_index(YogEnv* env, YogTable* var2index, ID id) 
{
    YogVal val = ID2VAL(id);
    YogVal index = YUNDEF;
    if (YogTable_lookup(env, var2index, val, &index)) {
        return VAL2INT(index);
    }
    else {
        return -1;
    }
}

static void 
append_store(YogEnv* env, CompileData* data, unsigned int lineno, ID id) 
{
    switch (data->ctx) {
    case CTX_FUNC:
        {
            int index = lookup_var_index(env, data->var2index, id);
            YOG_ASSERT(env, 0 <= index, "can't find variable index");
            CompileData_add_store_local(env, data, lineno, index);
            break;
        }
    case CTX_KLASS:
    case CTX_PKG:
        CompileData_add_store_name(env, data, lineno, id);
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
            visit_node(env, visitor, node->u.assign.right, arg);
            CompileData_add_dup(env, data, lineno);

            ID name = node->u.variable.id;
            append_store(env, data, lineno, name);
            break;
        }
    case NODE_SUBSCRIPT:
        visit_node(env, visitor, left->u.subscript.prefix, arg);
        visit_node(env, visitor, left->u.subscript.index, arg);
        visit_node(env, visitor, node->u.assign.right, arg);
        CompileData_add_call_method(env, data, lineno, INTERN("[]="), 2, 0, 0, 0, 0);
        break;
    default:
        YOG_ASSERT(env, FALSE, "invalid node type.");
        break;
    }
}

static void 
compile_visit_method_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, node->u.method_call.recv, arg);
    VISIT_EACH_ARGS(node->u.method_call.args, node->u.method_call.blockarg);

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

    YogVal index = YUNDEF;
    if (!YogTable_lookup(env, data->const2index, val, &index)) {
        int size = YogTable_size(env, data->const2index);
        index = INT2VAL(size);
        YogTable_add_direct(env, data->const2index, val, index);
        return size;
    }
    else {
        return VAL2INT(index);
    }
}

static void 
compile_visit_literal(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    CompileData* data = arg;

    YogVal val = node->u.literal.val;
    unsigned int lineno = node->lineno;
    if (IS_STR(val)) {
        unsigned int index = register_const(env, data, val);
        CompileData_add_make_string(env, data, lineno, index);
    }
    else {
        ADD_PUSH_CONST(val, lineno);
    }
}

static void 
compile_visit_command_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    VISIT_EACH_ARGS(node->u.command_call.args, node->u.command_call.blockarg);

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
#define KEEP_MEMBER(member)     entry->member = (*keeper)(env, entry->member)
    KEEP_MEMBER(next);
    KEEP_MEMBER(from);
    KEEP_MEMBER(to);
    KEEP_MEMBER(target);
#undef KEEP_MEMBER
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
    ADD_PUSH_CONST(YNIL, lineno);
    CompileData_add_ret(env, data, lineno);
}

static YogCode* 
compile_stmts(YogEnv* env, AstVisitor* visitor, const char* filename, ID klass_name, ID func_name, YogArray* stmts, YogTable* var2index, Context ctx, YogInst* tail) 
{
    CompileData data;
    data.ctx = ctx;
    data.var2index = var2index;
    data.const2index = NULL;
    YogInst* anchor = Anchor_new(env);
    data.last_inst = anchor;
    data.exc_tbl = ExceptionTableEntry_new(env);
    data.exc_tbl_last = data.exc_tbl;
    data.label_while_start = NULL;
    data.label_while_end = NULL;
    data.finally_list = NULL;
    data.try_list = NULL;
    data.filename = filename;
    data.klass_name = klass_name;

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
                YogNode* node = OBJ_AS(YogNode, val);
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
    YogByteArray* insts = bin->body;

    YogCode* code = YogCode_new(env);
    if (var2index != NULL) {
        code->local_vars_count = YogTable_size(env, var2index);
    }
    code->stack_size = count_stack_size(env, anchor);
    code->consts = table2array(env, data.const2index);
    code->insts = insts;

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
register_params_var2index(YogEnv* env, YogArray* params, YogTable* var2index) 
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
        if (YogTable_lookup(env, var2index, name, NULL)) {
            YOG_ASSERT(env, FALSE, "duplicated argument name in function definition");
        }
        YogVal index = INT2VAL(var2index->num_entries);
        YogTable_add_direct(env, var2index, name, index);
    }
}

static void 
register_block_params_var2index(YogEnv* env, YogArray* params, YogTable* var2index) 
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
        if (!YogTable_lookup(env, var2index, name, NULL)) {
            YogVal index = INT2VAL(var2index->num_entries);
            YogTable_add_direct(env, var2index, name, index);
        }
    }
}

static void 
setup_params(YogEnv* env, YogTable* var2index, YogArray* params, YogCode* code) 
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
            arg_index[i] = lookup_var_index(env, var2index, name);
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
        arg_info->blockarg_index = lookup_var_index(env, var2index, name);

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
        arg_info->vararg_index = lookup_var_index(env, var2index, name);

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
    arg_info->kwarg_index = lookup_var_index(env, var2index, name);

    n++;
    YOG_ASSERT(env, size == n, "Parameters count is unmatched.");
}

static void 
register_self(YogEnv* env, YogTable* var2index) 
{
    ID name = INTERN("self");
    YogVal key = ID2VAL(name);
    YogVal val = INT2VAL(0);
    YogTable_add_direct(env, var2index, key, val);
}

static YogTable* 
Var2Index_new(YogEnv* env) 
{
    YogTable* var2index = YogTable_new_symbol_table(env);
    register_self(env, var2index);

    return var2index;
}

static YogCode* 
compile_func(YogEnv* env, AstVisitor* visitor, const char* filename, ID klass_name, YogNode* node) 
{
    YogTable* var2index = Var2Index_new(env);

    YogArray* params = node->u.funcdef.params;
    register_params_var2index(env, params, var2index);
    YogArray* stmts = node->u.funcdef.stmts;
    make_var2index(env, stmts, var2index);

    ID func_name = node->u.funcdef.name;

    YogCode* code = compile_stmts(env, visitor, filename, klass_name, func_name, stmts, var2index, CTX_FUNC, NULL);
    setup_params(env, var2index, params, code);

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

    YogCode* code = compile_func(env, visitor, data->filename, klass_name, node);

    YogVal val = PTR2VAL(code);
    ADD_PUSH_CONST(val, node->lineno);

    ID id = node->u.funcdef.name;
#if 0
    int var_index = lookup_var_index(env, data->var2index, id);
#endif
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
    VISIT_EACH_ARGS(node->u.func_call.args, node->u.func_call.blockarg);

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
            int index = lookup_var_index(env, data->var2index, id);
            if (0 <= index) {
                CompileData_add_load_local(env, data, lineno, index);
            }
            else {
                CompileData_add_load_global(env, data, lineno, id);
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
    YogTable* var2index = data->var2index;
    register_block_params_var2index(env, params, var2index);

    const char* filename = data->filename;
    ID klass_name = INVALID_ID;
    ID func_name = INTERN("<block>");
    YogArray* stmts = node->u.blockarg.stmts;
    YogCode* code = compile_stmts(env, visitor, filename, klass_name, func_name, stmts, var2index, data->ctx, NULL);

    setup_params(env, var2index, params, code);

    return code;
}

static void 
compile_visit_block(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;
    YogCode* code = compile_block(env, visitor, node, data);

    YogVal val = PTR2VAL(code);
    unsigned int lineno = node->lineno;
    ADD_PUSH_CONST(val, lineno);
    switch (data->ctx) {
        case CTX_FUNC:
        case CTX_KLASS:
            YOG_ASSERT(env, FALSE, "NOT IMPLEMENTED");
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
    YogTable* var2index = Var2Index_new(env);
    make_var2index(env, stmts, var2index);

    unsigned int lineno = get_last_lineno(env, stmts);

    YogInst* ret = Inst_new(env, lineno);
    ret->next = NULL;
    ret->opcode = OP(RET);
    YogInst* push_self_name = Inst_new(env, lineno);
    push_self_name->next = ret;
    push_self_name->opcode = OP(PUSH_SELF_NAME);

    const char* filename = data->filename;
    ID func_name = INVALID_ID;

    YogCode* code = compile_stmts(env, visitor, filename, klass_name, func_name, stmts, var2index, CTX_KLASS, push_self_name);

    return code;
}

static void 
compile_visit_klass(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    ID name = node->u.klass.name;
    YogVal val = ID2VAL(name);
    ADD_PUSH_CONST(val, node->lineno);

    YogNode* super = node->u.klass.super;
    if (super != NULL) {
        visit_node(env, visitor, super, arg);
    }
    else {
        YogKlass* cObject = ENV_VM(env)->cObject;
        val = OBJ2VAL(cObject);
        ADD_PUSH_CONST(val, node->lineno);
    }

    YogArray* stmts = node->u.klass.stmts;
    YogCode* code = compile_klass(env, visitor, name, stmts, data);
    val = PTR2VAL(code);
    unsigned int lineno = node->lineno;
    ADD_PUSH_CONST(val, lineno);

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
        visit_node(env, visitor, expr, arg);
    }
    else {
        ADD_PUSH_CONST(YNIL, lineno);
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
    YogTable* var2index = make_var2index(env, stmts, NULL);

    AstVisitor visitor;
    compile_init_visitor(&visitor);

    if (filename == NULL) {
        filename = "<stdin>";
    }
    filename = YogString_dup(env, filename);

    ID klass_name = INVALID_ID;
    ID func_name = INTERN("<module>");

    YogCode* code = compile_stmts(env, &visitor, filename, klass_name, func_name, stmts, var2index, CTX_PKG, NULL);

    return code;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
