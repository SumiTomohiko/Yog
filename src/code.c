#include "yog/yog.h"

YogCode* 
YogCode_new(YogEnv* env) 
{
    YogCode* code = ALLOC_OBJ(env, GCOBJ_CODE, YogCode);
    code->consts = NULL;
    code->insts = NULL;

    return code;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
