#include "yog/yog.h"

YogKlass* 
YogInt_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, ENV_VM(env)->obj_klass);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
