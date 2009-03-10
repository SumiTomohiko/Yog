#include "yog/string.h"
#include "yog/yog.h"

static YogVal 
to_s(YogEnv* env)
{
    return YogString_new_str(env, "nil");
}

YogVal 
YogNil_klass_new(YogEnv* env) 
{
    YogVal klass = YogKlass_new(env, "Nil", ENV_VM(env)->cObject);
    PUSH_LOCAL(env, klass);

    YogKlass_define_method(env, klass, "to_s", to_s, 0, 0, 0, 0, NULL);

    POP_LOCALS(env);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
