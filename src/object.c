#include "yog/yog.h"

void 
YogObj_set_attr(YogEnv* env, YogObj* obj, const char* name, YogVal val) 
{
    ID id = YogVm_intern(env, ENV_VM(env), name);
    YogVal key = YogVal_symbol(id);
    YogTable_insert(env, obj->attrs, key, val);
}

void 
YogObj_init(YogEnv* env, YogObj* obj, YogObj* klass) 
{
    obj->attrs = YogTable_new_symbol_table(env);
    YogObj_set_attr(env, obj, "class", YogVal_gcobj(YOGGCOBJ(klass)));
}

YogObj*
YogObj_new(YogEnv* env) 
{
    YogObj* obj = ALLOC_OBJ(env, GCOBJ_OBJ, YogObj);
    YogObj_init(env, obj, NULL);

    return obj;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
