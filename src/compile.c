#include <limits.h>
#include <stdint.h>
#include "yog/opcodes.h"
#include "yog/st.h"
#include "yog/yog.h"
#include <stdio.h>

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
};

struct Var2IndexData {
    YogTable* var2index;
};

typedef struct Var2IndexData Var2IndexData;

struct CompileData {
    YogTable* var2index;
    YogTable* const2index;
    YogInst* last_inst;
};

typedef struct CompileData CompileData;

static YogInst* 
YogInst_new(YogEnv* env) 
{
    YogInst* inst = ALLOC_OBJ(env, GCOBJ_INST, YogInst);
    inst->next = NULL;

    return inst;
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

    unsigned int i = 0;
    for (i = 0; i < YogArray_size(env, stmts); i++) {
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
}

static YogTable*
make_var2index(YogEnv* env, YogArray* stmts, YogTable* var2index)
{
    AstVisitor visitor;
    var2index_init_visitor(&visitor);

    ID next_id = 0;
    if (var2index == NULL) {
        var2index = YogTable_new_symbol_table(env);
    }
    else {
        next_id = YogTable_size(env, var2index);
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

static YogCode* 
compile_stmts(YogEnv* env, AstVisitor* visitor, YogArray* stmts, YogTable* var2index) 
{
    make_var2index(env, stmts, var2index);

    CompileData data;
    data.var2index = var2index;
    data.const2index = NULL;
    YogInst* dummy_inst = YogInst_new(env);
    dummy_inst->type = INST_DUMMY;
    data.last_inst = dummy_inst;

    visitor->visit_stmts(env, visitor, stmts, &data);
    YogBinary* bin = insts2bin(env, dummy_inst->next);

    YogCode* code = YogCode_new(env);
    code->stack_size = count_stack_size(env, stmts);
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
    YogCode* code = compile_stmts(env, visitor, stmts, var2index);

    return code;
}

static void 
compile_visit_func_def(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    YogCode* code = compile_func(env, visitor, node);

    CompileData* data = arg;
    YogVal val = YogVal_gcobj(YOGGCOBJ(code));
    int index = register_const(env, data, val);

    ID id = NODE_NAME(node);

    CompileData_append_push_const(env, data, index);
    CompileData_append_make_func(env, data);
    CompileData_append_store_pkg(env, data, id);
}

#if 0
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
#endif

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
    ID id = NODE_ID(node);
    CompileData* data = arg;
#if 0
    int index = lookup_var_index(env, data->var2index, id);
#endif
    CompileData_append_load_pkg(env, data, id);
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
}

YogCode* 
Yog_compile_module(YogEnv* env, YogArray* stmts) 
{
    AstVisitor visitor;
    compile_init_visitor(&visitor);

    return compile_stmts(env, &visitor, stmts, NULL);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
