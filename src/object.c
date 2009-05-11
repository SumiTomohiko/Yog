#include "yog/env.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/st.h"
#include "yog/thread.h"
#include "yog/yog.h"

YogVal 
YogObj_get_attr(YogEnv* env, YogVal obj, ID name) 
{
    if (!IS_PTR(PTR_AS(YogObj, obj)->attrs)) {
        return YUNDEF;
    }

    YogVal key = ID2VAL(name);
    YogVal attr = YNIL;
    if (YogTable_lookup(env, PTR_AS(YogObj, obj)->attrs, key, &attr)) {
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

    if (!IS_PTR(PTR_AS(YogObj, obj)->attrs)) {
        YogVal attrs = YogTable_new_symbol_table(env);
        MODIFY(env, PTR_AS(YogObj, obj)->attrs, attrs);
    }

    YogTable_insert(env, PTR_AS(YogObj, obj)->attrs, key, val);

    RETURN_VOID(env);
}

void 
YogObj_set_attr(YogEnv* env, YogVal obj, const char* name, YogVal val) 
{
    SAVE_ARGS2(env, obj, val);

    ID id = YogVm_intern(env, env->vm, name);
    YogObj_set_attr_id(env, obj, id, val);

    RETURN_VOID(env);
}

void 
YogBasicObj_init(YogEnv* env, YogVal obj, unsigned int flags, YogVal klass) 
{
    PTR_AS(YogBasicObj, obj)->flags = flags;
    MODIFY(env, PTR_AS(YogBasicObj, obj)->klass, klass);
}

void 
YogObj_init(YogEnv* env, YogVal obj, unsigned int flags, YogVal klass) 
{
    PTR_AS(YogObj, obj)->attrs = YUNDEF;
    YogBasicObj_init(env, obj, flags | HAS_ATTRS, klass);
}

void 
YogBasicObj_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap) 
{
    YogBasicObj* obj = ptr;
    YogGC_keep(env, &obj->klass, keeper, heap);
}

void 
YogObj_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogObj* obj = ptr;
    YogGC_keep(env, &obj->attrs, keeper, heap);
}

YogVal 
YogObj_allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal obj = ALLOC_OBJ(env, YogObj_keep_children, NULL, YogObj);
    YogObj_init(env, obj, 0, klass);

    RETURN(env, obj);
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
