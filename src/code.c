#include <stdio.h>
#include "yog/yog.h"

#include "src/code.inc"

void 
YogCode_dump(YogEnv* env, YogCode* code) 
{
    pc_t pc = 0;
    while (pc < code->insts->size) {
        printf("%d", pc);

        OpCode op = code->insts->items[pc];
        printf(" %s", get_op_name(op));

        printf("\n");
        pc += Yog_get_inst_size(op);
    }
}

static void 
gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogCode* code = ptr;
    code->consts = do_gc(env, code->consts);
    code->insts = do_gc(env, code->insts);
    code->exc_tbl = do_gc(env, code->exc_tbl);
}

YogCode* 
YogCode_new(YogEnv* env) 
{
    YogCode* code = ALLOC_OBJ(env, gc_children, YogCode);
    code->argc = 0;
    code->stack_size = 0;
    code->local_vars_count = 0;
    code->consts = NULL;
    code->insts = NULL;
    code->exc_tbl_size = 0;
    code->exc_tbl = NULL;
    code->lineno_tbl_size = 0;
    code->lineno_tbl = NULL;

    return code;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
