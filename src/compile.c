#include <limits.h>
#include <stdint.h>
#include "yog/opcodes.h"
#include "yog/st.h"
#include "yog/yog.h"

#define VISIT_EACH_ARGS()   do { \
    unsigned int i = 0; \
    YogArray* args = NODE_ARGS(node); \
    unsigned int argc = YogArray_size(env, args); \
    for (i = 0; i < argc; i++) { \
        YogVal val = YogArray_at(env, args, i); \
        YogNode* node = (YogNode*)YOGVAL_GCOBJ(val); \
        visit_node(env, visitor, node, arg); \
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
};

struct Var2IndexData {
    ID next_id;
    YogTable* var2index;
};

typedef struct Var2IndexData Var2IndexData;

struct Const2IndexData {
    ID next_id;
    YogTable* const2index;
};

typedef struct Const2IndexData Const2IndexData;

struct CompileData {
    YogTable* var2index;
    YogTable* const2index;
    YogBinary* insts;
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
    default:
        Yog_assert(env, FALSE, "Unknown node type.");
        break;
    }
#undef VISIT
}

static void 
xxx2index_visit_method_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_RECEIVER(node), arg);
    VISIT_EACH_ARGS();
}

static void 
visit_stmts(YogEnv* env, AstVisitor* visitor, YogArray* stmts, void* arg) 
{
    unsigned int i = 0;
    for (i = 0; i < YogArray_size(env, stmts); i++) {
        YogVal val = YogArray_at(env, stmts, i);
        YogNode* node = (YogNode*)YOGVAL_GCOBJ(val);
        visitor->visit_stmt(env, visitor, node, arg);
    }
}

static void 
var2index_visit_assign(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    ID id = NODE_LEFT(node);
    YogVal symbol = YogVal_symbol(id);
    Var2IndexData* data = (Var2IndexData*)arg;
    if (!YogTable_lookup(env, data->var2index, symbol, NULL)) {
        YogVal next_id = YogVal_int(data->next_id);
        YogTable_add_direct(env, data->var2index, symbol, next_id);
        data->next_id++;
    }
}

static void 
xxx2index_visit_command_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    VISIT_EACH_ARGS();
}

static YogTable*
make_var2index(YogEnv* env, YogArray* stmts)
{
    AstVisitor visitor;
    visitor.visit_stmts = visit_stmts;
    visitor.visit_stmt = visit_node;
    visitor.visit_assign = var2index_visit_assign;
    visitor.visit_method_call = xxx2index_visit_method_call;
    visitor.visit_literal = NULL;
    visitor.visit_command_call = xxx2index_visit_command_call;

    YogTable* var2index = YogTable_new_symbol_table(env);
    Var2IndexData data;
    data.next_id = 0;
    data.var2index = var2index;

    visitor.visit_stmts(env, &visitor, stmts, &data);

    return var2index;
}

static void 
const2index_visit_assign(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_RIGHT(node), arg);
}

static void 
const2index_visit_literal(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    Const2IndexData* data = arg;
    YogVal val = NODE_VAL(node);
    if (!YogTable_lookup(env, data->const2index, val, NULL)) {
        YogVal next_id = YogVal_int(data->next_id);
        YogTable_add_direct(env, data->const2index, val, next_id);
        data->next_id++;
    }
}

static YogTable* 
make_const2index(YogEnv* env, YogArray* stmts) 
{
    AstVisitor visitor;
    visitor.visit_stmts = visit_stmts;
    visitor.visit_stmt = visit_node;
    visitor.visit_assign = const2index_visit_assign;
    visitor.visit_method_call = xxx2index_visit_method_call;
    visitor.visit_literal = const2index_visit_literal;
    visitor.visit_command_call = xxx2index_visit_command_call;

    YogTable* const2index = YogTable_new_val_table(env);
    Const2IndexData data;
    data.next_id = 0;
    data.const2index = const2index;

    visitor.visit_stmts(env, &visitor, stmts, &data);

    return const2index;
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
    YogBinary* insts = data->insts;
    YogBinary_push_uint8(env, insts, OP(STORE_PKG));
    YogBinary_push_uint32(env, insts, YOGVAL_INT(index));
}

static void 
compile_visit_method_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_RECEIVER(node), arg);
    VISIT_EACH_ARGS();

    unsigned int argc = YogArray_size(env, NODE_ARGS(node));
    Yog_assert(env, argc < UINT8_MAX + 1, "Too many arguments for method call.");

    CompileData* data = arg;
    YogBinary* insts = data->insts;
    YogBinary_push_uint8(env, insts, OP(CALL_METHOD));
    YogBinary_push_uint32(env, insts, YogVm_intern(env, ENV_VM(env), "+"));
    YogBinary_push_uint8(env, insts, argc);
}

static void 
compile_visit_literal(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    CompileData* data = arg;
    YogVal index = YogVal_nil();
    if (!YogTable_lookup(env, data->const2index, NODE_VAL(node), &index)) {
        Yog_assert(env, FALSE, "Can't find constant.");
    }
    YogBinary* insts = data->insts;
    YogBinary_push_uint8(env, insts, OP(PUSH_CONST));
    YogBinary_push_uint8(env, insts, YOGVAL_INT(index));
}

static void 
compile_visit_command_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    VISIT_EACH_ARGS();

    unsigned int argc = YogArray_size(env, NODE_ARGS(node));
    Yog_assert(env, argc < UINT8_MAX + 1, "Too many arguments for command call.");

    CompileData* data = arg;
    YogBinary* insts = data->insts;
    YogBinary_push_uint8(env, insts, OP(CALL_COMMAND));
    YogBinary_push_uint32(env, insts, NODE_COMMAND(node));
    YogBinary_push_uint8(env, insts, argc);
}

static YogBinary* 
compile_module(YogEnv* env, YogArray* stmts, YogTable* var2index, YogTable* const2index) 
{
    AstVisitor visitor;
    visitor.visit_stmts = visit_stmts;
    visitor.visit_stmt = visit_node;
    visitor.visit_assign = compile_visit_assign;
    visitor.visit_method_call = compile_visit_method_call;
    visitor.visit_literal = compile_visit_literal;
    visitor.visit_command_call = compile_visit_command_call;

    CompileData data;
    data.var2index = var2index;
    data.const2index = const2index;
#define INIT_INSTS_SIZE (0)
    data.insts = YogBinary_new(env, INIT_INSTS_SIZE);
#undef INIT_INSTS_SIZE
    YogInst* first_inst = YogInst_new(env);
    first_inst->type = INST_DUMMY;
    data.last_inst = first_inst;

    visitor.visit_stmts(env, &visitor, stmts, &data);

    return data.insts;
}

static void 
stack_size_visit_stmts(YogEnv* env, AstVisitor* visitor, YogArray* stmts, void* arg) 
{
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

    unsigned int i = 0;
    YogArray* args = NODE_ARGS(node);
    unsigned int argc = YogArray_size(env, args);
    for (i = 0; i < argc; i++) {
        YogVal val = YogArray_at(env, args, i);
        YogNode* node = (YogNode*)YOGVAL_GCOBJ(val);
        visit_node(env, visitor, node, arg);
    }
}

static void 
stack_size_visit_literal(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    unsigned int* stack_size = (unsigned int*)arg;
    (*stack_size)++;
}

static unsigned int 
count_stack_size(YogEnv* env, YogArray* stmts) 
{
    AstVisitor visitor;
    visitor.visit_stmts = stack_size_visit_stmts;
    visitor.visit_stmt = visit_node;
    visitor.visit_assign = stack_size_visit_assign;
    visitor.visit_method_call = stack_size_visit_method_call;
    visitor.visit_literal = stack_size_visit_literal;

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

YogCode* 
Yog_compile_module(YogEnv* env, YogArray* stmts) 
{
    YogTable* var2index = make_var2index(env, stmts);
    YogTable* const2index = make_const2index(env, stmts);
    unsigned int stack_size = count_stack_size(env, stmts);
    YogBinary* insts = compile_module(env, stmts, var2index, const2index);

    YogCode* code = YogCode_new(env);
    code->stack_size = stack_size;
    code->consts = table2array(env, const2index);
    code->insts = insts->body;

    return code;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
