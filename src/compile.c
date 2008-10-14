#include <limits.h>
#include <stdint.h>
#include "yog/opcodes.h"
#include "yog/st.h"
#include "yog/yog.h"

#define VISIT_EACH_ARGS()   do { \
    YogArray* args = NODE_ARGS(node); \
    if (args != NULL) { \
        unsigned int argc = YogArray_size(env, args); \
        unsigned int i = 0; \
        for (i = 0; i < argc; i++) { \
            YogVal val = YogArray_at(env, args, i); \
            YogNode* node = (YogNode*)YOGVAL_GCOBJ(val); \
            visit_node(env, visitor, node, arg); \
        } \
    } \
} while (0)

typedef struct AstVisitor AstVisitor;

typedef void (*VisitNode)(YogEnv*, AstVisitor*, YogNode*, void*);
typedef void (*VisitArray)(YogEnv*, AstVisitor*, YogArray*, void*);

struct AstVisitor {
    VisitArray visit_stmts;
    VisitNode visit_stmt;
    VisitNode visit_assign;
    VisitNode visit_method_call;
    VisitNode visit_literal;
    VisitNode visit_command_call;
    VisitNode visit_func_def;
    VisitNode visit_func_call;
    VisitNode visit_variable;
    VisitNode visit_try;
    VisitNode visit_except;
    VisitNode visit_while;
};

struct Var2IndexData {
    YogTable* var2index;
};

typedef struct Var2IndexData Var2IndexData;

enum Context {
    CTX_PKG, 
    CTX_FUNC, 
};

typedef enum Context Context;

struct CompileData {
    enum Context ctx;
    YogTable* var2index;
    YogTable* const2index;
    YogInst* last_inst;
};

typedef struct CompileData CompileData;

static YogInst* 
YogInst_new(YogEnv* env, InstType type) 
{
    YogInst* inst = ALLOC_OBJ(env, GCOBJ_INST, YogInst);
    inst->type = type;
    inst->next = NULL;

    return inst;
}

static YogInst* 
inst_new(YogEnv* env) 
{
    return YogInst_new(env, INST_OP);
}

static YogInst* 
label_new(YogEnv* env) 
{
    return YogInst_new(env, INST_LABEL);
}

static YogInst* 
anchor_new(YogEnv* env) 
{
    return YogInst_new(env, INST_ANCHOR);
}

static void 
append_inst(CompileData* data, YogInst* inst) 
{
    data->last_inst->next = inst;
    data->last_inst = inst;
}

#include "src/compile.inc"

static void 
visit_node(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
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
    case NODE_TRY:
        VISIT(visit_try);
        break;
    case NODE_EXCEPT:
        VISIT(visit_except);
        break;
    case NODE_WHILE:
        VISIT(visit_while);
        break;
    default:
        Yog_assert(env, FALSE, "Unknown node type.");
        break;
    }
#undef VISIT
}

static void 
var2index_visit_method_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_RECEIVER(node), arg);
    VISIT_EACH_ARGS();
}

static void 
visit_stmts(YogEnv* env, AstVisitor* visitor, YogArray* stmts, void* arg) 
{
    if (stmts == NULL) {
        return;
    }

    unsigned int size = YogArray_size(env, stmts);
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal val = YogArray_at(env, stmts, i);
        YogNode* node = (YogNode*)YOGVAL_GCOBJ(val);
        visitor->visit_stmt(env, visitor, node, arg);
    }
}

static void 
var2index_register(YogEnv* env, YogTable* var2index, ID var)
{
    YogVal symbol = YogVal_symbol(var);
    if (!YogTable_lookup(env, var2index, symbol, NULL)) {
        int size = YogTable_size(env, var2index);
        YogVal index = YogVal_int(size);
        YogTable_add_direct(env, var2index, symbol, index);
    }
}

static void 
var2index_visit_assign(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    ID id = NODE_LEFT(node);
    Var2IndexData* data = arg;
    var2index_register(env, data->var2index, id);
}

static void 
var2index_visit_command_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    VISIT_EACH_ARGS();
}

static void 
var2index_visit_func_def(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    ID id = NODE_NAME(node);
    Var2IndexData* data = arg;
    var2index_register(env, data->var2index, id);
}

static void 
var2index_visit_func_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    VISIT_EACH_ARGS();
}

static void 
generic_visit_try(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visitor->visit_stmts(env, visitor, NODE_TRY(node), arg);
    YogArray* excepts = NODE_EXCEPTS(node);
    if (excepts != NULL) {
        unsigned int size = YogArray_size(env, excepts);
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            YogVal val = YogArray_at(env, excepts, i);
            YogNode* node = (YogNode*)YOGVAL_GCOBJ(val);
            visit_node(env, visitor, node, arg);
        }
    }
    visitor->visit_stmts(env, visitor, NODE_ELSE(node), arg);
    visitor->visit_stmts(env, visitor, NODE_FINALLY(node), arg);
}

static void 
var2index_visit_except(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_EXC_TYPE(node), arg);
    ID id = NODE_EXC_VAR(node);
    if (id != NO_EXC_VAR) {
        Var2IndexData* data = arg;
        var2index_register(env, data->var2index, id);
    }
    visitor->visit_stmts(env, visitor, NODE_EXC_STMTS(node), arg);
}

static void 
var2index_visit_while(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_TEST(node), arg);
    visitor->visit_stmts(env, visitor, NODE_STMTS(node), arg);
}

static void 
var2index_init_visitor(AstVisitor* visitor) 
{
    visitor->visit_stmts = visit_stmts;
    visitor->visit_stmt = visit_node;
    visitor->visit_assign = var2index_visit_assign;
    visitor->visit_method_call = var2index_visit_method_call;
    visitor->visit_literal = NULL;
    visitor->visit_command_call = var2index_visit_command_call;
    visitor->visit_func_def = var2index_visit_func_def;
    visitor->visit_func_call = var2index_visit_func_call;
    visitor->visit_variable = NULL;
    visitor->visit_try = generic_visit_try;
    visitor->visit_except = var2index_visit_except;
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

static void 
compile_visit_assign(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    visit_node(env, visitor, NODE_RIGHT(node), arg);

    CompileData* data = arg;
    YogVal symbol = YogVal_symbol(NODE_LEFT(node));
    YogVal index = YogVal_nil();
    if (!YogTable_lookup(env, data->var2index, symbol, &index)) {
        Yog_assert(env, FALSE, "Can't find assigned symbol.");
    }
    CompileData_append_store_pkg(env, data, YOGVAL_INT(index));
}

static void 
compile_visit_method_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_RECEIVER(node), arg);
    VISIT_EACH_ARGS();

    unsigned int argc = YogArray_size(env, NODE_ARGS(node));
    Yog_assert(env, argc < UINT8_MAX + 1, "Too many arguments for method call.");
    CompileData* data = arg;
    CompileData_append_call_method(env, data, NODE_METHOD(node), argc);
}

static int
register_const(YogEnv* env, CompileData* data, YogVal val) 
{
    if (data->const2index == NULL) {
        data->const2index = YogTable_new_symbol_table(env);
    }

    YogVal index = YogVal_undef();
    if (!YogTable_lookup(env, data->const2index, val, &index)) {
        int size = YogTable_size(env, data->const2index);
        index = YogVal_int(size);
        YogTable_add_direct(env, data->const2index, val, index);
        return size;
    }
    else {
        return YOGVAL_INT(index);
    }
}

static void 
compile_visit_literal(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    CompileData* data = arg;
    YogVal val = NODE_VAL(node);
    int index = register_const(env, data, val);

    CompileData_append_push_const(env, data, index);
}

static void 
compile_visit_command_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    VISIT_EACH_ARGS();

    unsigned int argc = YogArray_size(env, NODE_ARGS(node));
    Yog_assert(env, argc < UINT8_MAX + 1, "Too many arguments for command call.");
    CompileData* data = arg;
    CompileData_append_call_command(env, data, NODE_COMMAND(node), argc);
}

#if 0
static void 
stack_size_visit_stmts(YogEnv* env, AstVisitor* visitor, YogArray* stmts, void* arg) 
{
    if (stmts == NULL) {
        return;
    }

    unsigned int i = 0;
    for (i = 0; i < YogArray_size(env, stmts); i++) {
        YogVal val = YogArray_at(env, stmts, i);
        YogNode* node = (YogNode*)YOGVAL_GCOBJ(val);

        unsigned int stack_size = 0;
        visitor->visit_stmt(env, visitor, node, &stack_size);
        unsigned int* max_stack_size = (unsigned int*)arg;
        if (*max_stack_size < stack_size) {
            *max_stack_size = stack_size;
        }
    }
}

static void 
stack_size_visit_assign(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_RIGHT(node), arg);
}

static void 
stack_size_visit_method_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_RECEIVER(node), arg);
    VISIT_EACH_ARGS();
}

static void 
stack_size_need_one(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    unsigned int* stack_size = (unsigned int*)arg;
    (*stack_size)++;
}

static void 
stack_size_visit_func_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_CALLEE(node), arg);
    /* TODO */
    VISIT_EACH_ARGS();
}

static void 
stack_size_visit_command_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    VISIT_EACH_ARGS();
}

static void 
stack_size_visit_try(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    unsigned int stack_size = 0;
    generic_visit_try(env, visitor, node, &stack_size);
    if (stack_size < 1) {
        stack_size = 1;
    }
    unsigned int* total_size = arg;
    *total_size += stack_size;
}

static void 
stack_size_visit_except(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    unsigned int stack_size = 0;
    visit_node(env, visitor, NODE_EXC_TYPE(node), &stack_size);
    if (stack_size < 1) {
        stack_size = 1;
    }

    unsigned int tmp = 0;
    visitor->visit_stmts(env, visitor, NODE_EXC_STMTS(node), &tmp);
    if (stack_size < tmp) {
        stack_size = tmp;
    }

    unsigned int* result = arg;
    *result = stack_size;
}

static void 
stack_size_init_visitor(AstVisitor* visitor) 
{
    visitor->visit_stmts = stack_size_visit_stmts;
    visitor->visit_stmt = visit_node;
    visitor->visit_assign = stack_size_visit_assign;
    visitor->visit_method_call = stack_size_visit_method_call;
    visitor->visit_literal = stack_size_need_one;
    visitor->visit_command_call = stack_size_visit_command_call;
    visitor->visit_func_def = stack_size_need_one;
    visitor->visit_func_call = stack_size_visit_func_call;
    visitor->visit_variable = stack_size_need_one;
    visitor->visit_try = stack_size_visit_try;
    visitor->visit_except = stack_size_visit_except;
}

static unsigned int 
count_stack_size(YogEnv* env, YogArray* stmts) 
{
    AstVisitor visitor;
    stack_size_init_visitor(&visitor);

    unsigned int stack_size = 0;
    visitor.visit_stmts(env, &visitor, stmts, &stack_size);

    return stack_size;
}
#endif

static int 
table2array_count_index(YogEnv* env, YogVal key, YogVal value, YogVal* arg) 
{
    if (YOGVAL_INT(*arg) < YOGVAL_INT(value)) {
        *arg = value;
    }

    return ST_CONTINUE;
}

static int 
table2array_fill_array(YogEnv* env, YogVal key, YogVal value, YogVal* arg) 
{
    YogValArray* array = (YogValArray*)YOGVAL_GCOBJ(*arg);
    int index = YOGVAL_INT(value);
    array->items[index] = key;

    return ST_CONTINUE;
}

static YogValArray* 
table2array(YogEnv* env, YogTable* table) 
{
    if (table == NULL) {
        return NULL;
    }

    YogVal max_index = YogVal_int(INT_MIN);
    YogTable_foreach(env, table, table2array_count_index, &max_index);
    int index = YOGVAL_INT(max_index);
    if (0 <= index) {
        unsigned int size = index + 1;
        YogValArray* array = YogValArray_new(env, size);
        YogVal arg = YogVal_gcobj(YOGGCOBJ(array));
        YogTable_foreach(env, table, table2array_fill_array, &arg);
        array->size = size;
        return array;
    }
    else {
        return NULL;
    }
}

void 
set_label_pos(YogEnv* env, YogInst* first_inst) 
{
    pc_t pc = 0;
    YogInst* inst = first_inst;
    while (inst != NULL) {
        switch (inst->type) {
            case INST_OP:
                pc += get_inst_size(inst);
                break;
            case INST_LABEL:
                LABEL_POS(inst) = pc;
                break;
            case INST_ANCHOR:
                Yog_assert(env, FALSE, "Instruction is anchor.");
                break;
            default:
                Yog_assert(env, FALSE, "Unknown inst type.");
                break;
        }

        inst = inst->next;
    }
}

static YogCode* 
compile_stmts(YogEnv* env, AstVisitor* visitor, YogArray* stmts, YogTable* var2index, Context ctx) 
{
    CompileData data;
    data.ctx = ctx;
    data.var2index = var2index;
    data.const2index = NULL;
    YogInst* anchor = anchor_new(env);
    data.last_inst = anchor;

    visitor->visit_stmts(env, visitor, stmts, &data);
    set_label_pos(env, anchor->next);
    YogBinary* bin = insts2bin(env, anchor->next);

    YogCode* code = YogCode_new(env);
    if (var2index != NULL) {
        code->local_vars_count = YogTable_size(env, var2index);
    }
#if 0
    code->stack_size = count_stack_size(env, stmts);
#endif
    code->stack_size = 32;
    code->consts = table2array(env, data.const2index);
    code->insts = bin->body;

    return code;
}

static void 
register_params_var2index(YogEnv* env, YogNode* node, YogTable* var2index) 
{
    YogArray* params = NODE_PARAMS(node);
    if (params == NULL) {
        return;
    }

    unsigned int size = YogArray_size(env, params);
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal param = YogArray_at(env, params, i);
        if (YogTable_lookup(env, var2index, param, NULL)) {
            Yog_assert(env, FALSE, "duplicated argument name in function definition");
        }
        YogVal index = YogVal_int(i);
        YogTable_add_direct(env, var2index, param, index);
    }
}

static YogCode* 
compile_func(YogEnv* env, AstVisitor* visitor, YogNode* node) 
{
    YogTable* var2index = YogTable_new_symbol_table(env);
    register_params_var2index(env, node, var2index);
    YogArray* stmts = NODE_STMTS(node);
    make_var2index(env, stmts, var2index);

    YogCode* code = compile_stmts(env, visitor, stmts, var2index, CTX_FUNC);

    YogArray* params = NODE_PARAMS(node);
    if (params != NULL) {
        code->argc = YogArray_size(env, params);
    }

    return code;
}

static int 
lookup_var_index(YogEnv* env, YogTable* var2index, ID id) 
{
    YogVal val = YogVal_symbol(id);
    YogVal index = YogVal_undef();
    if (!YogTable_lookup(env, var2index, val, &index)) {
        Yog_assert(env, FALSE, "Can't find var.");
    }
    return YOGVAL_INT(index);
}

static void 
compile_visit_func_def(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    YogCode* code = compile_func(env, visitor, node);

    CompileData* data = arg;
    YogVal val = YogVal_gcobj(YOGGCOBJ(code));
    int const_index = register_const(env, data, val);

    ID id = NODE_NAME(node);
    int var_index = lookup_var_index(env, data->var2index, id);

    CompileData_append_push_const(env, data, const_index);
    CompileData_append_make_func(env, data);
    CompileData_append_store_pkg(env, data, var_index);
}

static void 
compile_visit_func_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_CALLEE(node), arg);
    VISIT_EACH_ARGS();

    unsigned int argc = 0;
    YogArray* args = NODE_ARGS(node);
    if (args != NULL) {
        argc = YogArray_size(env, args);
    }

    CompileData* data = arg;
    CompileData_append_call_func(env, data, argc);
}

static void 
compile_visit_variable(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    CompileData* data = arg;
    ID id = NODE_ID(node);
    int index = lookup_var_index(env, data->var2index, id);
    switch (data->ctx) {
    case CTX_PKG:
        CompileData_append_load_pkg(env, data, index);
        break;
    case CTX_FUNC:
        CompileData_append_load_local(env, data, index);
        break;
    default:
        Yog_assert(env, FALSE, "Unknown context.");
        break;
    }
}

static void 
compile_visit_try(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    /* TODO */
}

static void 
compile_visit_except(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    /* TODO */
}

static void 
compile_visit_while(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    YogInst* while_start = label_new(env);
    YogInst* while_end = label_new(env);

    append_inst(data, while_start);
    visit_node(env, visitor, NODE_TEST(node), arg);
    CompileData_append_jump_if_false(env, data, while_end);
    visitor->visit_stmts(env, visitor, NODE_STMTS(node), arg);
    CompileData_append_jump(env, data, while_start);
    append_inst(data, while_end);
}

static void 
compile_init_visitor(AstVisitor* visitor) 
{
    visitor->visit_stmts = visit_stmts;
    visitor->visit_stmt = visit_node;
    visitor->visit_assign = compile_visit_assign;
    visitor->visit_method_call = compile_visit_method_call;
    visitor->visit_literal = compile_visit_literal;
    visitor->visit_command_call = compile_visit_command_call;
    visitor->visit_func_def = compile_visit_func_def;
    visitor->visit_func_call = compile_visit_func_call;
    visitor->visit_variable = compile_visit_variable;
    visitor->visit_try = compile_visit_try;
    visitor->visit_except = compile_visit_except;
    visitor->visit_while = compile_visit_while;
}

YogCode* 
Yog_compile_module(YogEnv* env, YogArray* stmts) 
{
    YogTable* var2index = make_var2index(env, stmts, NULL);

    AstVisitor visitor;
    compile_init_visitor(&visitor);

    YogCode* code = compile_stmts(env, &visitor, stmts, var2index, CTX_PKG);

    return code;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
