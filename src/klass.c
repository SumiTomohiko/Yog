#include "yog/yog.h"

YogKlass* 
YogKlass_new(YogEnv* env, YogKlass* super) 
{
    YogKlass* klass = ALLOC_OBJ(env, GCOBJ_KLASS, YogKlass);
    YogObj_init(env, YOGOBJ(klass), ENV_VM(env)->klass_klass);
    klass->super = super;

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
