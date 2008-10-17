#include "yog/yog.h"

static void 
gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogObj_gc_children(env, ptr, do_gc);

    YogKlass* klass = ptr;
    klass->super = do_gc(env, klass->super);
}

YogKlass* 
YogKlass_new(YogEnv* env, YogKlass* super) 
{
    YogKlass* klass = ALLOC_OBJ(env, gc_children, YogKlass);
    YogObj_init(env, YOGOBJ(klass), ENV_VM(env)->klass_klass);
    klass->super = super;

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
