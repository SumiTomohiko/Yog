#include "yog/yog.h"

void 
YogObj_init(YogEnv* env, YogObj* obj) 
{
    obj->attrs = YogTable_new_symbol_table(env);
}

YogObj*
YogObj_new(YogEnv* env) 
{
    YogObj* obj = ALLOC_OBJ(env, GCOBJ_OBJ, YogObj);
    YogObj_init(env, obj);

    return obj;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
