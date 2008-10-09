#include "yog/yog.h"

YogKlass* 
YogPkg_klass_new(YogEnv* env) 
{
    return YogKlass_new(env, ENV_VM(env)->obj_klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
