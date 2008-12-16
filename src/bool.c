#include "yog/error.h"
#include "yog/yog.h"

#define CHECK_TYPE(v)   do { \
    YOG_ASSERT(env, VAL_TYPE(v) == VAL_BOOL, "value isn't bool."); \
} while (0)

static YogVal 
to_s(YogEnv* env)
{
    YogVal self = SELF(env);
    CHECK_TYPE(self);

    const char* s = NULL;
    if (VAL2BOOL(self)) {
        s = "true";
    }
    else {
        s = "false";
    }

    YogString* obj = YogString_new_str(env, s);
    YogVal val = OBJ2VAL(obj);

    return val;
}

YogKlass* 
YogBool_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "Bool", ENV_VM(env)->cObject);
    FRAME_DECL_LOCAL(env, klass_idx, OBJ2VAL(klass));

#define UPDATE_PTR  FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx)
    UPDATE_PTR;
    YogKlass_define_method(env, klass, "to_s", to_s, 0, 0, 0, 0, NULL);

    UPDATE_PTR;
    return klass;
#undef UPDATE_PTR
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
