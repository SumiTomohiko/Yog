#include "yog/yog.h"

static void 
YogBlock_init(YogEnv* env, YogBlock* block, YogKlass* klass) 
{
    YogBasicObj_init(env, YOGBASICOBJ(block), klass);
}

static void 
YogPackageBlock_init(YogEnv* env, YogPackageBlock* block) 
{
    YogBlock_init(env, BLOCK(block), ENV_VM(env)->pkg_block_klass);
    block->pkg = NULL;
    block->vars = NULL;
}

YogPackageBlock* 
YogPackageBlock_new(YogEnv* env) 
{
    YogPackageBlock* block = ALLOC_OBJ(env, NULL, YogPackageBlock);
    YogPackageBlock_init(env, block);

    return block;
}

YogKlass* 
YogPackageBlock_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, ENV_VM(env)->obj_klass);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
