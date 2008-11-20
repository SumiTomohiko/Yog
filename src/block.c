#include "yog/yog.h"

static void 
YogBlock_init(YogEnv* env, YogBlock* block, YogKlass* klass) 
{
    YogBasicObj_init(env, YOGBASICOBJ(block), 0, klass);
}

static void 
YogPackageBlock_init(YogEnv* env, YogPackageBlock* block) 
{
    YogBlock_init(env, BLOCK(block), ENV_VM(env)->cPackageBlock);
    block->self = YUNDEF;
    block->vars = NULL;
}

YogPackageBlock* 
YogPackageBlock_new(YogEnv* env) 
{
    YogPackageBlock* block = ALLOC_OBJ(env, NULL, YogPackageBlock);
    YogPackageBlock_init(env, block);

    return block;
}

static YogBasicObj* 
allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, NULL, YogPackageBlock);
    YogBasicObj_init(env, obj, 0, klass);
    return obj;
}

YogKlass* 
YogPackageBlock_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "PackageBlock", ENV_VM(env)->cObject);
    YogKlass_define_allocator(env, klass, allocate);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
