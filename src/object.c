#include "yog/st.h"
#include "yog/yog.h"

YogVal 
YogObj_get_attr(YogEnv* env, YogObj* obj, ID name) 
{
    if (obj->attrs == NULL) {
        return YUNDEF;
    }

    YogVal key = ID2VAL(name);
    YogVal attr = YNIL;
    if (YogTable_lookup(env, obj->attrs, key, &attr)) {
        return attr;
    }
    else {
        return YUNDEF;
    }
}

void 
YogObj_set_attr_id(YogEnv* env, YogObj* obj, ID name, YogVal val) 
{
    YogVal key = ID2VAL(name);

    if (obj->attrs == NULL) {
        obj->attrs = YogTable_new_symbol_table(env);
    }

    YogTable_insert(env, obj->attrs, key, val);
}

void 
YogObj_set_attr(YogEnv* env, YogObj* obj, const char* name, YogVal val) 
{
    ID id = YogVm_intern(env, ENV_VM(env), name);
    YogObj_set_attr_id(env, obj, id, val);
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
YogBasicObj_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogBasicObj* obj = ptr;
    obj->klass = (*keeper)(env, obj->klass);
}

void 
YogObj_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogBasicObj_keep_children(env, ptr, keeper);

    YogObj* obj = ptr;
    obj->attrs = (*keeper)(env, obj->attrs);
}

YogBasicObj* 
YogObj_allocate(YogEnv* env, YogKlass* klass)
{
    YogObj* obj = ALLOC_OBJ(env, YogObj_keep_children, NULL, YogObj);
    YogObj_init(env, obj, 0, klass);
    return (YogBasicObj*)obj;
}

YogObj*
YogObj_new(YogEnv* env, YogKlass* klass) 
{
    YogObj* obj = (YogObj*)YogObj_allocate(env, klass);
    return obj;
}

static YogVal 
initialize(YogEnv* env)
{
    return YNIL;
}

void 
YogObj_klass_init(YogEnv* env, YogKlass* klass) 
{
    YogKlass_define_method(env, klass, "initialize", initialize, 1, 1, 0, 0, "block", NULL);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
