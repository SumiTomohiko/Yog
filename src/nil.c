#include "yog/yog.h"

static YogVal 
to_s(YogEnv* env)
{
    YogString* s = YogString_new_str(env, "nil");
    YogVal val = OBJ2VAL(s);

    return val;
}

YogKlass* 
YogNil_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "Nil", ENV_VM(env)->cObject);
    YogKlass_define_method(env, klass, "to_s", to_s, 0, 0, 0, 0, NULL);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
