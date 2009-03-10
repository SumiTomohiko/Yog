#include "yog/st.h"
#include "yog/yog.h"

YogVal 
YogObj_get_attr(YogEnv* env, YogObj* obj, ID name) 
{
    if (VAL2PTR(obj->attrs) == NULL) {
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
YogObj_set_attr_id(YogEnv* env, YogVal obj, ID name, YogVal val) 
{
    SAVE_LOCALS(env);
    PUSH_LOCALS2(env, obj, val);

    YogVal key = ID2VAL(name);

    if (VAL2PTR(OBJ_AS(YogObj, obj)->attrs) == NULL) {
        YogVal attrs = YogTable_new_symbol_table(env);
        OBJ_AS(YogObj, obj)->attrs = attrs;
    }

    YogTable_insert(env, OBJ_AS(YogObj, obj)->attrs, key, val);

    RETURN_VOID(env);
}

void 
YogObj_set_attr(YogEnv* env, YogVal obj, const char* name, YogVal val) 
{
    SAVE_ARGS2(env, obj, val);

    ID id = YogVm_intern(env, ENV_VM(env), name);
    YogObj_set_attr_id(env, obj, id, val);

    RETURN_VOID(env);
}

void 
YogBasicObj_init(YogEnv* env, YogBasicObj* obj, unsigned int flags, YogVal klass) 
{
    obj->flags = flags;
    obj->klass = klass;
}

void 
YogObj_init(YogEnv* env, YogObj* obj, unsigned int flags, YogVal klass) 
{
    obj->attrs = PTR2VAL(NULL);
    YogBasicObj_init(env, YOGBASICOBJ(obj), flags | HAS_ATTRS, klass);
}

void 
YogBasicObj_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogBasicObj* obj = ptr;
    obj->klass = OBJ2VAL((*keeper)(env, VAL2OBJ(obj->klass)));
}

void 
YogObj_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogBasicObj_keep_children(env, ptr, keeper);

    YogObj* obj = ptr;
    obj->attrs = PTR2VAL((*keeper)(env, VAL2PTR(obj->attrs)));
}

YogVal 
YogObj_allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogObj* obj = ALLOC_OBJ(env, YogObj_keep_children, NULL, YogObj);
    YogObj_init(env, obj, 0, klass);

    RETURN(env, OBJ2VAL(obj));
}

YogVal 
YogObj_new(YogEnv* env, YogVal klass)
{
    return YogObj_allocate(env, klass);
}

static YogVal 
initialize(YogEnv* env)
{
    return YNIL;
}

void 
YogObj_klass_init(YogEnv* env, YogVal klass) 
{
    YogKlass_define_method(env, klass, "initialize", initialize, 1, 1, 0, 0, "block", NULL);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
