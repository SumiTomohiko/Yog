#include "yog/env.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/method.h"
#include "yog/thread.h"
#include "yog/yog.h"

#define KEEP_BASIC_OBJ      YogBasicObj_keep_children(env, ptr, keeper, heap)
#define KEEP_VAL(member)    YogGC_keep(env, &method->member, keeper, heap)
#define KEEP_SELF           KEEP_VAL(self)

static void 
YogBuiltinBoundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    KEEP_BASIC_OBJ;

    YogBuiltinBoundMethod* method = ptr;
    KEEP_SELF;
    KEEP_VAL(f);
}

static YogVal 
YogBuiltinBoundMethod_allocate(YogEnv* env, YogVal klass) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogVal method = YUNDEF;
    PUSH_LOCAL(env, method);

    ALLOC_OBJ(env, method, YogBuiltinBoundMethod_keep_children, NULL, YogBuiltinBoundMethod);
    PTR_AS(YogBuiltinBoundMethod, method)->self = YUNDEF;
    PTR_AS(YogBuiltinBoundMethod, method)->f = YUNDEF;
    YogBasicObj_init(env, method, 0, klass);

    RETURN(env, method);
}

static void 
YogScriptMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    KEEP_BASIC_OBJ;

    YogScriptMethod* method = ptr;

    KEEP_VAL(code);
    KEEP_VAL(globals);
    KEEP_VAL(outer_vars);
}

static void 
YogScriptMethod_init(YogEnv* env, YogVal method, YogVal klass) 
{
    YogBasicObj_init(env, method, 0, klass);

    PTR_AS(YogScriptMethod, method)->code = YUNDEF;
    PTR_AS(YogScriptMethod, method)->globals = YUNDEF;
    PTR_AS(YogScriptMethod, method)->outer_vars = YUNDEF;
}

static void 
YogBoundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogScriptMethod_keep_children(env, ptr, keeper, heap);

    YogBoundMethod* method = ptr;
    KEEP_SELF;
}

static YogVal 
YogBoundMethod_allocate(YogEnv* env, YogVal klass)
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogVal method = YUNDEF;
    PUSH_LOCAL(env, method);

    ALLOC_OBJ(env, method, YogBoundMethod_keep_children, NULL, YogBoundMethod);
    YogScriptMethod_init(env, method, klass);

    PTR_AS(YogBoundMethod, method)->self = YUNDEF;

    RETURN(env, method);
}

static void 
YogBuiltinUnboundMethod_keep_chldren(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    KEEP_BASIC_OBJ;

    YogBuiltinUnboundMethod* method = ptr;
    KEEP_VAL(f);
}

static YogVal 
YogBuiltinUnboundMethod_allocate(YogEnv* env, YogVal klass) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogVal method = YUNDEF;
    PUSH_LOCAL(env, method);

    ALLOC_OBJ(env, method, YogBuiltinUnboundMethod_keep_chldren, NULL, YogBuiltinUnboundMethod);
    PTR_AS(YogBuiltinUnboundMethod, method)->f = YUNDEF;
    YogBasicObj_init(env, method, 0, klass);

    RETURN(env, method);
}

static void 
YogUnboundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogScriptMethod_keep_children(env, ptr, keeper, heap);
}

static YogVal 
YogUnboundMethod_allocate(YogEnv* env, YogVal klass) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogVal method = YUNDEF;
    PUSH_LOCAL(env, method);

    ALLOC_OBJ(env, method, YogUnboundMethod_keep_children, NULL, YogUnboundMethod);
    YogScriptMethod_init(env, method, klass);

    RETURN(env, method);
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
