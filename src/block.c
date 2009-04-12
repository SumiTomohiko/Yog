#include "yog/block.h"
#include "yog/klass.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static void 
YogBasicBlock_init(YogEnv* env, YogBasicBlock* block, YogVal klass) 
{
    YogBasicObj_init(env, YOGBASICOBJ(block), 0, klass);
    block->code = YUNDEF;
}

static void 
YogPackageBlock_init(YogEnv* env, YogPackageBlock* block) 
{
    YogBasicBlock_init(env, BASIC_BLOCK(block), env->vm->cPackageBlock);

    block->self = YUNDEF;
    block->vars = YUNDEF;
}

#define KEEP_MEMBER(member)     block->member = (*keeper)(env, block->member)

static void 
YogBasicBlock_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogBasicObj_keep_children(env, ptr, keeper);

    YogBasicBlock* block = ptr;
    block->code = YogVal_keep(env, block->code, keeper);
}

static void 
YogPackageBlock_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogBasicBlock_keep_children(env, ptr, keeper);

    YogPackageBlock* block = ptr;
#define KEEP(member)    block->member = YogVal_keep(env, block->member, keeper)
    KEEP(self);
    KEEP(vars);
#undef KEEP
}

#undef KEEP_MEMBER

static YogVal 
YogPackageBlock_allocate(YogEnv* env, YogVal klass) 
{
    YogPackageBlock* block = ALLOC_OBJ(env, YogPackageBlock_keep_children, NULL, YogPackageBlock);
    YogPackageBlock_init(env, block);

    return OBJ2VAL(block);
}

YogVal 
YogPackageBlock_new(YogEnv* env) 
{
    return YogPackageBlock_allocate(env, env->vm->cPackageBlock);
}

static void 
YogBlock_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogBasicBlock_keep_children(env, ptr, keeper);

    YogBlock* block = ptr;
#define KEEP(member)    block->member = YogVal_keep(env, block->member, keeper)
    KEEP(locals);
    KEEP(outer_vars);
    KEEP(globals);
#undef KEEP
}

static void 
YogBlock_init(YogEnv* env, YogBlock* block) 
{
    YogBasicBlock_init(env, BASIC_BLOCK(block), YNIL);

    block->locals = YUNDEF;
    block->outer_vars = YUNDEF;
    block->globals = YUNDEF;
}

YogVal 
YogBlock_new(YogEnv* env) 
{
    YogBlock* block = ALLOC_OBJ(env, YogBlock_keep_children, NULL, YogBlock);
    YogBlock_init(env, block);

    return OBJ2VAL(block);
}

YogVal 
YogPackageBlock_klass_new(YogEnv* env) 
{
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "PackageBlock", env->vm->cObject);

    YogKlass_define_allocator(env, klass, YogPackageBlock_allocate);

    POP_LOCALS(env);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
