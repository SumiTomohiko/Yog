#include "yog/yog.h"

YogVal 
YogObj_get_attr(YogEnv* env, YogObj* obj, ID name) 
{
    if (obj->attrs == NULL) {
        Yog_assert(env, FALSE, "Object doesn't have attributes.");
    }

    YogVal key = YogVal_symbol(name);
    YogVal attr = YogVal_nil();
    if (!YogTable_lookup(env, obj->attrs, key, &attr)) {
        Yog_assert(env, FALSE, "Can't find attribute.");
    }

    return attr;
}

void 
YogObj_set_attr(YogEnv* env, YogObj* obj, const char* name, YogVal val) 
{
    ID id = YogVm_intern(env, ENV_VM(env), name);
    YogVal key = YogVal_symbol(id);

    if (obj->attrs == NULL) {
        obj->attrs = YogTable_new_symbol_table(env);
    }

    YogTable_insert(env, obj->attrs, key, val);
}

void 
YogObj_define_method(YogEnv* env, YogObj* obj, const char* name, YogFuncBody f) 
{
    YogVal val = YogVal_func(f);
    YogObj_set_attr(env, obj, name, val);
}

void 
YogBasicObj_init(YogEnv* env, YogBasicObj* obj, YogKlass* klass) 
{
    obj->klass = klass;
}

void 
YogObj_init(YogEnv* env, YogObj* obj, YogKlass* klass) 
{
    obj->attrs = NULL;
    YogBasicObj_init(env, YOGBASICOBJ(obj), klass);
}

#if 0
YogObj*
YogObj_new(YogEnv* env) 
{
    YogObj* obj = ALLOC_OBJ(env, GCOBJ_OBJ, YogObj);
    YogObj_init(env, obj, NULL);

    return obj;
}
#endif

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
