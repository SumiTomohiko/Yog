#include "yog/yog.h"

YogVal 
YogObj_get_attr(YogEnv* env, YogObj* obj, ID name) 
{
    if (obj->attrs == NULL) {
        Yog_assert(env, FALSE, "Object doesn't have attributes.");
    }

    YogVal key = YogVal_symbol(name);
    YogVal attr = YogVal_nil();
    if (YogTable_lookup(env, obj->attrs, key, &attr)) {
        return attr;
    }
    else {
        return YogVal_undef();
    }
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
YogBasicObj_init(YogEnv* env, YogBasicObj* obj, unsigned int flags, YogKlass* klass) 
{
    obj->flags = flags;
    obj->klass = klass;
}

void 
YogObj_init(YogEnv* env, YogObj* obj, unsigned int flags, YogKlass* klass) 
{
    obj->attrs = NULL;
    YogBasicObj_init(env, YOGBASICOBJ(obj), flags | HAS_ATTRS, klass);
}

void 
YogObj_gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogObj* obj = ptr;
    obj->attrs = do_gc(env, obj->attrs);
}

YogBasicObj* 
YogObj_allocate(YogEnv* env, YogKlass* klass)
{
    YogObj* obj = ALLOC_OBJ(env, YogObj_gc_children, YogObj);
    YogObj_init(env, obj, 0, klass);
    return (YogBasicObj*)obj;
}

YogObj*
YogObj_new(YogEnv* env, YogKlass* klass) 
{
    YogObj* obj = (YogObj*)YogObj_allocate(env, klass);
    return obj;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
