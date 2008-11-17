#include <limits.h>
#include <stdint.h>
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
    VisitNode visit_stmt;
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

struct LinenoList {
    struct LinenoList* prev;
    struct LinenoTableEntry entry;
};

typedef struct LinenoList LinenoList;

#define SET_LINENO()    set_lineno(env, data, node->lineno)

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

    struct LinenoList* lineno_list;
    pc_t pc;
};

typedef struct CompileData CompileData;

#define VISIT_EACH_ARGS()   do { \
    YogArray* args = NODE_ARGS(node); \
    if (args != NULL) { \
        unsigned int argc = YogArray_size(env, args); \
        unsigned int i = 0; \
        for (i = 0; i < argc; i++) { \
            YogVal val = YogArray_at(env, args, i); \
            YogNode* node = VAL2PTR(val); \
            visit_node(env, visitor, node, arg); \
        } \
    } \
    YogNode* blockarg = NODE_BLOCK(node); \
    if (blockarg != NULL) { \
        visit_node(env, visitor, blockarg, arg); \
    } \
} while (0)

#define RAISE   INTERN("raise")
#define RERAISE() \
    CompileData_add_call_command(env, data, RAISE, 0, 0, 0, 0, 0)

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

#define ADD_PUSH_CONST(val) do { \
    unsigned int index = register_const(env, data, val); \
    CompileData_add_push_const(env, data, index); \
} while (0)

static void 
gc_lineno_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    LinenoList* elem = ptr;
    elem->prev = do_gc(env, elem->prev);
}

static void 
set_lineno(YogEnv* env, CompileData* data, unsigned int lineno) 
{
    if (data->lineno_list != NULL) {
        if (data->lineno_list->entry.lineno == lineno) {
            data->lineno_list->entry.pc_to = data->pc;
            return;
        }
    }

    LinenoList* elem = ALLOC_OBJ(env, gc_lineno_children, LinenoList);
    elem->entry.pc_from = data->pc;
    elem->entry.pc_to = data->pc;
    elem->entry.lineno = lineno;
    elem->prev = data->lineno_list;
    data->lineno_list = elem;
}

static void 
gc_inst_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogInst* inst = ptr;
    inst->next = do_gc(env, inst->next);

    if (inst->type == INST_OP) {
        switch (INST_OPCODE(inst)) {
            case OP(JUMP):
                JUMP_DEST(inst) = do_gc(env, JUMP_DEST(inst));
                break;
            case OP(JUMP_IF_FALSE):
                JUMP_IF_FALSE_DEST(inst) = do_gc(env, JUMP_IF_FALSE_DEST(inst));
                break;
            default:
                break;
        }
    }
}

static YogInst* 
YogInst_new(YogEnv* env, InstType type) 
{
    YogInst* inst = ALLOC_OBJ(env, gc_inst_children, YogInst);
    inst->type = type;
    inst->next = NULL;

    return inst;
}

static YogInst* 
Inst_new(YogEnv* env) 
{
    return YogInst_new(env, INST_OP);
}

static YogInst* 
Label_new(YogEnv* env) 
{
    return YogInst_new(env, INST_LABEL);
}

static YogInst* 
Anchor_new(YogEnv* env) 
{
    return YogInst_new(env, INST_ANCHOR);
}

static void 
append_inst(CompileData* data, YogInst* inst) 
{
    if (inst->type == INST_LABEL) {
        LABEL_POS(inst) = data->pc;
    }

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
    case NODE_BLOCK_ARG:
        VISIT(visit_block);
        break;
    case NODE_KLASS:
        VISIT(visit_klass);
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
            case NODE_COMMAND_CALL:
            case NODE_FUNC_CALL:
            case NODE_LITERAL:
            case NODE_METHOD_CALL:
            case NODE_VARIABLE:
                CompileData_add_pop(env, data);
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
var2index_visit_finally(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visitor->visit_stmts(env, visitor, NODE_HEAD(node), arg);
    visitor->visit_stmts(env, visitor, NODE_BODY(node), arg);
}

static void 
var2index_visit_except_body(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_EXC_TYPE(node), arg);
    ID id = NODE_EXC_VAR(node);
    if (id != NO_EXC_VAR) {
        Var2IndexData* data = arg;
        var2index_register(env, data->var2index, id);
    }
    visitor->visit_stmts(env, visitor, NODE_BODY(node), arg);
}

static void 
var2index_visit_while(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_TEST(node), arg);
    visitor->visit_stmts(env, visitor, NODE_STMTS(node), arg);
}

static void 
var2index_visit_if(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_IF_TEST(node), arg);
    visitor->visit_stmts(env, visitor, NODE_IF_STMTS(node), arg);
    visitor->visit_stmts(env, visitor, NODE_IF_TAIL(node), arg);
}

static void 
var2index_visit_break(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_EXPR(node), arg);
}

static void 
var2index_visit_except(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visitor->visit_stmts(env, visitor, NODE_HEAD(node), arg);

    YogArray* excepts = NODE_EXCEPTS(node);
    unsigned int size = YogArray_size(env, excepts);
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal val = YogArray_at(env, excepts, i);
        YogNode* node = VAL2PTR(val);
        visitor->visit_except_body(env, visitor, node, arg);
    }

    visitor->visit_stmts(env, visitor, NODE_ELSE(node), arg);
}

static void 
var2index_visit_block(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    Var2IndexData* data = arg;

    YogArray* params = NODE_PARAMS(node);
    if (params != NULL) {
        unsigned int size = YogArray_size(env, params);
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            YogVal val = YogArray_at(env, params, i);
            YogNode* param = VAL2PTR(val);
            ID name = NODE_NAME(param);
            var2index_register(env, data->var2index, name);
            visit_node(env, visitor, NODE_DEFAULT(param), arg);
        }
    }

    visitor->visit_stmts(env, visitor, NODE_STMTS(node), arg);
}

static void 
var2index_visit_klass(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    Var2IndexData* data = arg;
    ID name = NODE_NAME(node);
    var2index_register(env, data->var2index, name);

    YogNode* super = NODE_SUPER(node);
    visit_node(env, visitor, super, arg);
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
    visitor->visit_stmt = visit_node;
    visitor->visit_stmts = var2index_visit_stmts;
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
    if (!YogTable_lookup(env, var2index, val, &index)) {
        Yog_assert(env, FALSE, "Can't find var.");
    }
    return VAL2INT(index);
}

static void 
append_store(YogEnv* env, CompileData* data, ID id) 
{
    switch (data->ctx) {
        case CTX_FUNC:
            {
                uint8_t index = lookup_var_index(env, data->var2index, id);
                CompileData_add_store_local(env, data, index);
                break;
            }
        case CTX_KLASS:
        case CTX_PKG:
            CompileData_add_store_pkg(env, data, id);
            break;
        default:
            Yog_assert(env, FALSE, "Unkown context.");
            break;
    }
}

static void 
compile_visit_assign(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    visit_node(env, visitor, NODE_RIGHT(node), arg);

    CompileData* data = arg;
    CompileData_add_dup(env, data);

    ID name = NODE_LEFT(node);
    append_store(env, data, name);
}

static void 
compile_visit_method_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    visit_node(env, visitor, NODE_RECEIVER(node), arg);
    VISIT_EACH_ARGS();

    unsigned int argc = 0;
    YogArray* args = NODE_ARGS(node);
    if (args != NULL) {
        argc = YogArray_size(env, args);
        Yog_assert(env, argc < UINT8_MAX + 1, "Too many arguments for method call.");
    }

    uint8_t blockargc = 0;
    if (NODE_BLOCK(node) != NULL) {
        blockargc = 1;
    }

    CompileData* data = arg;
    SET_LINENO();
    CompileData_add_call_method(env, data, NODE_METHOD(node), argc, 0, blockargc, 0, 0);
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

    YogVal val = NODE_VAL(node);
    ADD_PUSH_CONST(val);

    SET_LINENO();
}

static void 
compile_visit_command_call(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    VISIT_EACH_ARGS();

    unsigned int argc = 0;
    YogArray* args = NODE_ARGS(node);
    if (args != NULL) {
        argc = YogArray_size(env, args);
        Yog_assert(env, argc < UINT8_MAX + 1, "Too many arguments for command call.");
    }

    uint8_t blockargc = 0;
    if (NODE_BLOCK(node) != NULL) {
        blockargc = 1;
    }

    CompileData* data = arg;
    SET_LINENO();
    CompileData_add_call_command(env, data, NODE_COMMAND(node), argc, 0, blockargc, 0, 0);
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
        array->size = size;
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
            pc_t from = LABEL_POS(entry->from);
            pc_t to = LABEL_POS(entry->to);
            if (from != to) {
                size++;
            }
        }

        entry = entry->next;
    }

    if (0 < size) {
        YogExcTbl* exc_tbl = ALLOC_OBJ_ITEM(env, NULL, YogExcTbl, size, YogExcTblEntry);

        unsigned int i = 0;
        entry = data->exc_tbl;
        while (entry != NULL) {
            if (entry->from != NULL) {
                pc_t from = LABEL_POS(entry->from);
                pc_t to = LABEL_POS(entry->to);
                if (from != to) {
                    YogExcTblEntry* ent = &exc_tbl->items[i];
                    ent->from = from;
                    ent->to = to;
                    ent->target = LABEL_POS(entry->target);

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
make_lineno_table(YogEnv* env, YogCode* code, LinenoList* list)
{
    unsigned int size = 0;
    LinenoList* elem = list;
    while (elem != NULL) {
        size++;
        elem = elem->prev;
    }

    LinenoTableEntry* tbl = ALLOC_OBJ_SIZE(env, NULL, sizeof(LinenoTableEntry) * size);
    unsigned int i = size - 1;
    elem = list;
    while (elem != NULL) {
        LinenoTableEntry* entry = &tbl[i];
        entry->pc_from = elem->entry.pc_from;
        entry->pc_to = elem->entry.pc_to;
        entry->lineno = elem->entry.lineno;

        elem = elem->prev;
        i--;
    }

    code->lineno_tbl = tbl;
    code->lineno_tbl_size = size;
}

static void 
gc_exception_table_entry_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    ExceptionTableEntry* entry = ptr;
#define GC(m)   DO_GC(env, do_gc, entry->m)
    GC(next);
    GC(from);
    GC(to);
    GC(target);
#undef GC
}

static ExceptionTableEntry* 
ExceptionTableEntry_new(YogEnv* env) 
{
    ExceptionTableEntry* entry = ALLOC_OBJ(env, gc_exception_table_entry_children, ExceptionTableEntry);
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

static YogCode* 
compile_stmts(YogEnv* env, AstVisitor* visitor, YogArray* stmts, YogTable* var2index, Context ctx, YogInst* tail) 
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
    data.lineno_list = NULL;
    data.pc = 0;

    visitor->visit_stmts(env, visitor, stmts, &data);
    if (tail != NULL) {
        CompileData_add_inst(&data, tail);
    }
    YogBinary* bin = insts2bin(env, anchor);

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
    make_exception_table(env, code, &data);
    make_lineno_table(env, code, data.lineno_list);

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
        ID id = NODE_NAME(node);
        YogVal name = ID2VAL(id);
        if (YogTable_lookup(env, var2index, name, NULL)) {
            Yog_assert(env, FALSE, "duplicated argument name in function definition");
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
        ID id = NODE_NAME(node);
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
        argnames = YogVm_alloc(env, NULL, sizeof(ID) * argc);
        arg_index = YogVm_alloc(env, NULL, sizeof(uint8_t) * argc);
        for (i = 0; i < argc; i++) {
            YogVal val = YogArray_at(env, params, i);
            YogNode* node = VAL2PTR(val);
            Yog_assert(env, node->type == NODE_PARAM, "Node must be NODE_PARAM.");

            ID name = NODE_NAME(node);
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

        ID name = NODE_NAME(node);
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

        ID name = NODE_NAME(node);
        arg_info->vararg_index = lookup_var_index(env, var2index, name);

        n++;
        if (size == n) {
            return;
        }
        val = YogArray_at(env, params, n);
        node = VAL2PTR(val);
    }

    Yog_assert(env, node->type == NODE_KW_PARAM, "Node must be NODE_KW_PARAM.");
    arg_info->kwargc = 1;

    ID name = NODE_NAME(node);
    arg_info->kwarg_index = lookup_var_index(env, var2index, name);

    n++;
    Yog_assert(env, size == n, "Parameters count is unmatched.");
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
compile_func(YogEnv* env, AstVisitor* visitor, YogNode* node) 
{
    YogTable* var2index = Var2Index_new(env);

    YogArray* params = NODE_PARAMS(node);
    register_params_var2index(env, params, var2index);
    YogArray* stmts = NODE_STMTS(node);
    make_var2index(env, stmts, var2index);

    YogCode* code = compile_stmts(env, visitor, stmts, var2index, CTX_FUNC, NULL);
    setup_params(env, var2index, params, code);

    return code;
}

static void 
compile_visit_func_def(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    YogCode* code = compile_func(env, visitor, node);

    CompileData* data = arg;
    YogVal val = PTR2VAL(code);
    ADD_PUSH_CONST(val);

    ID id = NODE_NAME(node);
#if 0
    int var_index = lookup_var_index(env, data->var2index, id);
#endif
    switch (data->ctx) {
        case CTX_FUNC:
            Yog_assert(env, FALSE, "TODO: NOT IMPLEMENTED");
            break;
        case CTX_KLASS:
        case CTX_PKG:
            {
                switch (data->ctx) {
                    case CTX_KLASS:
                        CompileData_add_make_method(env, data);
                        break;
                    case CTX_PKG:
                        CompileData_add_make_package_method(env, data);
                        break;
                    default:
                        Yog_assert(env, FALSE, "Invalid context type.");
                        break;
                }
                CompileData_add_store_pkg(env, data, id);
                break;
            }
        default:
            Yog_assert(env, FALSE, "Unknown context.");
            break;
    }
    SET_LINENO();
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

    uint8_t blockargc = 0;
    if (NODE_BLOCK(node) != NULL) {
        blockargc = 1;
    }

    CompileData* data = arg;
    SET_LINENO();
    CompileData_add_call_function(env, data, argc, 0, blockargc, 0, 0);
}

static void 
compile_visit_variable(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg) 
{
    CompileData* data = arg;
    ID id = NODE_ID(node);
    SET_LINENO();
    switch (data->ctx) {
    case CTX_FUNC:
        {
            uint8_t index = lookup_var_index(env, data->var2index, id);
            CompileData_add_load_local(env, data, index);
            break;
        }
    case CTX_KLASS:
    case CTX_PKG:
        CompileData_add_load_pkg(env, data, id);
        break;
    default:
        Yog_assert(env, FALSE, "Unknown context.");
        break;
    }
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

    append_inst(data, label_head_start);
    visitor->visit_stmts(env, visitor, NODE_HEAD(node), arg);
    append_inst(data, label_head_end);

    visitor->visit_stmts(env, visitor, NODE_BODY(node), arg);
    CompileData_add_jump(env, data, label_finally_end);

    append_inst(data, label_finally_error_start);
    visitor->visit_stmts(env, visitor, NODE_BODY(node), arg);
    RERAISE();

    append_inst(data, label_finally_end);

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

    append_inst(data, label_head_start);
    visitor->visit_stmts(env, visitor, NODE_HEAD(node), arg);
    append_inst(data, label_head_end);
    CompileData_add_jump(env, data, label_else_start);

    append_inst(data, label_excepts_start);
    YogArray* excepts = NODE_EXCEPTS(node);
    unsigned int size = YogArray_size(env, excepts);
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogInst* label_body_end = Label_new(env);

        YogVal val = YogArray_at(env, excepts, i);
        YogNode* node = VAL2PTR(val);

        YogNode* node_type = NODE_EXC_TYPE(node);
        if (node_type != NULL) {
            visit_node(env, visitor, node_type, arg);
#define LOAD_EXC()  CompileData_add_load_special(env, data, INTERN("$!"))
            LOAD_EXC();
            CompileData_add_call_method(env, data, INTERN("==="), 1, 0, 0, 0, 0);
            CompileData_add_jump_if_false(env, data, label_body_end);

            ID id = NODE_EXC_VAR(node);
            if (id != NO_EXC_VAR) {
                LOAD_EXC();
                append_store(env, data, id);
            }
#undef LOAD_EXC
        }

        visitor->visit_stmts(env, visitor, NODE_BODY(node), arg);
        CompileData_add_jump(env, data, label_else_end);

        append_inst(data, label_body_end);
    }
    RERAISE();

    append_inst(data, label_else_start);
    visitor->visit_stmts(env, visitor, NODE_ELSE(node), arg);
    append_inst(data, label_else_end);

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

    append_inst(data, while_start);
    visit_node(env, visitor, NODE_TEST(node), arg);
    CompileData_add_jump_if_false(env, data, while_end);
    visitor->visit_stmts(env, visitor, NODE_STMTS(node), arg);
    CompileData_add_jump(env, data, while_start);
    append_inst(data, while_end);

    data->label_while_end = label_while_end_prev;
    data->label_while_start = label_while_start_prev;
}

static void 
compile_visit_if(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    YogInst* label_tail_start = Label_new(env);
    YogInst* label_stmt_end = Label_new(env);

    visit_node(env, visitor, NODE_IF_TEST(node), arg);
    CompileData_add_jump_if_false(env, data, label_tail_start);
    visitor->visit_stmts(env, visitor, NODE_IF_STMTS(node), arg);
    CompileData_add_jump(env, data, label_stmt_end);
    append_inst(data, label_tail_start);
    visitor->visit_stmts(env, visitor, NODE_IF_TAIL(node), arg);
    append_inst(data, label_stmt_end);
}

static void 
split_exception_table(YogEnv* env, ExceptionTableEntry* exc_tbl, YogInst* label_from, YogInst* label_to)
{
    ExceptionTableEntry* entry = exc_tbl;
    Yog_assert(env, entry != NULL, "Exception table is empty.");
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

    YogNode* expr = NODE_EXPR(node);
    if (data->label_while_start != NULL) {
        Yog_assert(env, expr == NULL, "Can't return value with break/next.");
        FinallyListEntry* finally_list_entry = data->finally_list;
        while (finally_list_entry != NULL) {
            YogInst* label_start = Label_new(env);
            YogInst* label_end = Label_new(env);

            append_inst(data, label_start);
            visitor->visit_stmts(env, visitor, NODE_BODY(finally_list_entry->node), arg);
            append_inst(data, label_end);

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
        SET_LINENO();
        CompileData_add_jump(env, data, jump_to);
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
    YogArray* params = NODE_PARAMS(node);
    YogTable* var2index = data->var2index;
    register_block_params_var2index(env, params, var2index);

    YogArray* stmts = NODE_STMTS(node);
    YogCode* code = compile_stmts(env, visitor, stmts, var2index, data->ctx, NULL);

    setup_params(env, var2index, params, code);

    return code;
}

static void 
compile_visit_block(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;
    YogCode* code = compile_block(env, visitor, node, data);

    YogVal val = PTR2VAL(code);
    ADD_PUSH_CONST(val);
    switch (data->ctx) {
        case CTX_FUNC:
        case CTX_KLASS:
            Yog_assert(env, FALSE, "NOT IMPLEMENTED");
            break;
        case CTX_PKG:
            CompileData_add_make_package_block(env, data);
            break;
        default:
            Yog_assert(env, FALSE, "Unknown context.");
            break;
    }
}

static YogCode* 
compile_klass(YogEnv* env, AstVisitor* visitor, YogArray* stmts, CompileData* daa)
{
    YogTable* var2index = Var2Index_new(env);
    make_var2index(env, stmts, var2index);

    YogInst* ret = Inst_new(env);
    ret->next = NULL;
    ret->opcode = OP(RET);
    YogInst* push_self_name = Inst_new(env);
    push_self_name->next = ret;
    push_self_name->opcode = OP(PUSH_SELF_NAME);

    YogCode* code = compile_stmts(env, visitor, stmts, var2index, CTX_KLASS, push_self_name);

    return code;
}

static void 
compile_visit_klass(YogEnv* env, AstVisitor* visitor, YogNode* node, void* arg)
{
    CompileData* data = arg;

    ID name = NODE_NAME(node);
    YogVal val = ID2VAL(name);
    ADD_PUSH_CONST(val);

    YogNode* super = NODE_SUPER(node);
    if (super != NULL) {
        visit_node(env, visitor, super, arg);
    }
    else {
        YogKlass* obj_klass = ENV_VM(env)->obj_klass;
        val = OBJ2VAL(obj_klass);
        ADD_PUSH_CONST(val);
    }

    YogArray* stmts = NODE_STMTS(node);
    YogCode* code = compile_klass(env, visitor, stmts, data);
    val = PTR2VAL(code);
    ADD_PUSH_CONST(val);

    CompileData_add_make_klass(env, data);

    append_store(env, data, name);
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
    visitor->visit_stmt = visit_node;
    visitor->visit_stmts = compile_visit_stmts;
    visitor->visit_variable = compile_visit_variable;
    visitor->visit_while = compile_visit_while;
}

YogCode* 
Yog_compile_module(YogEnv* env, YogArray* stmts) 
{
    YogTable* var2index = make_var2index(env, stmts, NULL);

    AstVisitor visitor;
    compile_init_visitor(&visitor);

    YogCode* code = compile_stmts(env, &visitor, stmts, var2index, CTX_PKG, NULL);

    return code;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
