#include "yog/yog.h"

#define CHECK_TYPE(v)   do { \
    Yog_assert(env, VAL_TYPE(v) == VAL_BOOL, "value isn't bool."); \
} while (0)

static YogVal 
bool_to_s(YogEnv* env, YogVal self)
{
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
    YogKlass* klass = YogKlass_new(env, NULL, "Bool", ENV_VM(env)->obj_klass);
    YogKlass_define_method(env, klass, "to_s", bool_to_s, 0, 0, 0, 0, NULL);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
