#include "yog/yog.h"

#define CHECK_TYPE(v)   do { \
    Yog_assert(env, YOGVAL_TYPE(v) == VAL_BOOL, "value isn't bool."); \
} while (0)

static YogVal 
bool_to_s(YogEnv* env, YogVal recv, int argc, YogVal* args) 
{
    CHECK_TYPE(recv);

    const char* s = NULL;
    if (YOGVAL_BOOL(recv)) {
        s = "true";
    }
    else {
        s = "false";
    }

    YogString* obj = YogString_new_str(env, s);
    YogVal val = YogVal_gcobj(YOGGCOBJ(obj));

    return val;
}

YogKlass* 
YogBool_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, ENV_VM(env)->obj_klass);
    YogObj* obj = YOGOBJ(klass);
    YogObj_define_method(env, obj, "to_s", bool_to_s);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
