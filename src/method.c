#include "yog/env.h"
#include "yog/klass.h"
#include "yog/method.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define KEEP_BASIC_OBJ  YogBasicObj_keep_children(env, ptr, keeper)

#define KEEP_VAL(member)    do { \
    method->member = YogVal_keep(env, method->member, keeper); \
} while (0)
#define KEEP_SELF           KEEP_VAL(self)

static void 
YogBuiltinBoundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    KEEP_BASIC_OBJ;

    YogBuiltinBoundMethod* method = ptr;
    KEEP_SELF;
    method->f = YogVal_keep(env, method->f, keeper);
}

static YogVal 
YogBuiltinBoundMethod_allocate(YogEnv* env, YogVal klass) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogBuiltinBoundMethod* method = ALLOC_OBJ(env, YogBuiltinBoundMethod_keep_children, NULL, YogBuiltinBoundMethod);
    method->self = YNIL;
    method->f = PTR2VAL(NULL);
    YogBasicObj_init(env, (YogBasicObj*)method, 0, klass);

    RETURN(env, OBJ2VAL(method));
}

static void 
YogScriptMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    KEEP_BASIC_OBJ;

    YogScriptMethod* method = ptr;

    method->code = YogVal_keep(env, method->code, keeper);
    KEEP_VAL(globals);
    KEEP_VAL(outer_vars);
}

static void 
YogScriptMethod_init(YogEnv* env, YogScriptMethod* method, YogVal klass) 
{
    YogBasicObj_init(env, (YogBasicObj*)method, 0, klass);

    method->code = YUNDEF;
    method->globals = YUNDEF;
    method->outer_vars = YUNDEF;
}

static void 
YogBoundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogScriptMethod_keep_children(env, ptr, keeper);

    YogBoundMethod* method = ptr;
    KEEP_SELF;
}

static YogVal 
YogBoundMethod_allocate(YogEnv* env, YogVal klass)
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogBoundMethod* method = ALLOC_OBJ(env, YogBoundMethod_keep_children, NULL, YogBoundMethod);
    YogScriptMethod_init(env, (YogScriptMethod*)method, klass);

    method->self = YNIL;

    RETURN(env, OBJ2VAL(method));
}

static void 
YogBuiltinUnboundMethod_keep_chldren(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    KEEP_BASIC_OBJ;

    YogBuiltinUnboundMethod* method = ptr;
    method->f = YogVal_keep(env, method->f, keeper);
}

static YogVal 
YogBuiltinUnboundMethod_allocate(YogEnv* env, YogVal klass) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogBuiltinUnboundMethod* method = ALLOC_OBJ(env, YogBuiltinUnboundMethod_keep_chldren, NULL, YogBuiltinUnboundMethod);
    method->f = PTR2VAL(NULL);
    YogBasicObj_init(env, (YogBasicObj*)method, 0, klass);

    RETURN(env, OBJ2VAL(method));
}

static void 
YogUnboundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogScriptMethod_keep_children(env, ptr, keeper);
}

static YogVal 
YogUnboundMethod_allocate(YogEnv* env, YogVal klass) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogScriptMethod* method = ALLOC_OBJ(env, YogUnboundMethod_keep_children, NULL, YogUnboundMethod);
    YogScriptMethod_init(env, method, klass);

    RETURN(env, OBJ2VAL(method));
}

#define RETURN_NEW_KLASS(allocator, name)  do { \
    YogVal klass = YogKlass_new(env, name, env->vm->cObject); \
    PUSH_LOCAL(env, klass); \
    \
    YogKlass_define_allocator(env, klass, allocator); \
    \
    POP_LOCALS(env); \
    return klass; \
} while (0)

YogVal 
YogBuiltinBoundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS(YogBuiltinBoundMethod_allocate, "BuiltinBoundMethod");
}

YogVal 
YogBoundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS(YogBoundMethod_allocate, "BoundMethod");
}

YogVal 
YogBuiltinUnboundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS(YogBuiltinUnboundMethod_allocate, "BuiltinUnboundMethod");
}

YogVal 
YogUnboundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS(YogUnboundMethod_allocate, "UnboundMethod");
}

#undef RETURN_NEW_KLASS

YogVal 
YogBuiltinBoundMethod_new(YogEnv* env) 
{
    return YogBuiltinBoundMethod_allocate(env, env->vm->cBuiltinBoundMethod);
}

YogVal 
YogBoundMethod_new(YogEnv* env) 
{
    return YogBoundMethod_allocate(env, env->vm->cBoundMethod);
}

YogVal 
YogBuiltinUnboundMethod_new(YogEnv* env) 
{
    return YogBuiltinUnboundMethod_allocate(env, env->vm->cBuiltinUnboundMethod);
}

YogVal 
YogUnboundMethod_new(YogEnv* env) 
{
    return YogUnboundMethod_allocate(env, env->vm->cUnboundMethod);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
