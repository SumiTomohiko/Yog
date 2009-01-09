#include "yog/block.h"
#include "yog/yog.h"

static void 
YogBasicBlock_init(YogEnv* env, YogBasicBlock* block, YogKlass* klass) 
{
    YogBasicObj_init(env, YOGBASICOBJ(block), 0, klass);
}

static void 
YogPackageBlock_init(YogEnv* env, YogPackageBlock* block) 
{
    YogBasicBlock_init(env, BASIC_BLOCK(block), ENV_VM(env)->cPackageBlock);

    block->self = YUNDEF;
    block->vars = NULL;
}

#define KEEP_MEMBER(member)     block->member = (*keeper)(env, block->member)

static void 
YogBasicBlock_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogBasicObj_keep_children(env, ptr, keeper);

    YogBasicBlock* block = ptr;
    KEEP_MEMBER(code);
}

static void 
YogPackageBlock_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogBasicBlock_keep_children(env, ptr, keeper);

    YogPackageBlock* block = ptr;
    block->self = YogVal_keep(env, block->self, keeper);
    KEEP_MEMBER(vars);
}

#undef KEEP_MEMBER

static YogBasicObj* 
YogPackageBlock_allocate(YogEnv* env, YogKlass* klass) 
{
    YogPackageBlock* block = ALLOC_OBJ(env, YogPackageBlock_keep_children, NULL, YogPackageBlock);
    YogPackageBlock_init(env, block);

    return (YogBasicObj*)block;
}

YogPackageBlock* 
YogPackageBlock_new(YogEnv* env) 
{
    return (YogPackageBlock*)YogPackageBlock_allocate(env, ENV_VM(env)->cPackageBlock);
}

static void 
YogBlock_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogBasicBlock_keep_children(env, ptr, keeper);

    YogBlock* block = ptr;
#define KEEP(member)    block->member = (*keeper)(env, block->member)
    KEEP(locals);
    KEEP(outer_vars);
    KEEP(globals);
#undef KEEP
}

static void 
YogBlock_init(YogEnv* env, YogBlock* block) 
{
    YogBasicBlock_init(env, BASIC_BLOCK(block), NULL);

    block->locals = NULL;
    block->outer_vars = NULL;
    block->globals = NULL;
}

YogBlock* 
YogBlock_new(YogEnv* env) 
{
    YogBlock* block = ALLOC_OBJ(env, YogBlock_keep_children, NULL, YogBlock);
    YogBlock_init(env, block);

    return block;
}

YogKlass* 
YogPackageBlock_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "PackageBlock", ENV_VM(env)->cObject);
    YogKlass_define_allocator(env, klass, YogPackageBlock_allocate);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
