#include "yog/yog.h"

typedef struct AstVisitor AstVisitor;

typedef void (*VisitNode)(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg);
typedef void (*VisitArray)(YogEnv* env, AstVisitor* visitor, YogArray* array, void* arg);

struct AstVisitor {
    VisitArray visit_stmts;
    VisitNode visit_stmt;
    VisitNode visit_assign;
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

static void 
visit_node(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    switch (node->type) {
    case NODE_ASSIGN: 
        visitor->visit_assign(env, visitor, node, arg);
        break;
    case NODE_VARIABLE:
        break;
    case NODE_LITERAL:
        break;
    case NODE_METHOD_CALL:
        {
            visit_node(env, visitor, NODE_RECEIVER(node), arg);

            unsigned int i = 0;
            YogArray* args = NODE_ARGS(node);
            unsigned int size = YogArray_size(env, args);
            for (i = 0; i < size; i++) {
                YogVal val = YogArray_at(env, args, i);
                YogNode* node = (YogNode*)YOGVAL_GCOBJ(val);
                visit_node(env, visitor, node, arg);
            }
        }
        break;
    case NODE_FUNCTION_CALL:
        break;
    default:
        break;
    }
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

static YogTable*
make_var2index(YogEnv* env, YogArray* stmts)
{
    AstVisitor visitor;
    visitor.visit_stmts = visit_stmts;
    visitor.visit_stmt = visit_node;
    visitor.visit_assign = var2index_visit_assign;

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

static YogTable* 
make_const2index(YogEnv* env, YogArray* stmts) 
{
    AstVisitor visitor;
    visitor.visit_stmts = visit_stmts;
    visitor.visit_stmt = visit_node;
    visitor.visit_assign = const2index_visit_assign;

    YogTable* const2index = YogTable_new_val_table(env);
    Const2IndexData data;
    data.next_id = 0;
    data.const2index = const2index;

    visitor.visit_stmts(env, &visitor, stmts, &data);

    return const2index;
}

YogCode* 
Yog_compile_module(YogEnv* env, YogArray* stmts) 
{
    YogTable* var2index = make_var2index(env, stmts);
    YogTable* const2index = make_const2index(env, stmts);

    return NULL;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
