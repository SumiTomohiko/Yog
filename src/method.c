#include "yog/method.h"
#include "yog/yog.h"

#define KEEP_BASIC_OBJ  YogBasicObj_keep_children(env, ptr, keeper)

#define KEEP_SELF       method->self = YogVal_keep(env, method->self, keeper)
#define KEEP(member)    method->member = (*keeper)(env, method->member)

static void 
YogBuiltinBoundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    KEEP_BASIC_OBJ;

    YogBuiltinBoundMethod* method = ptr;
    KEEP_SELF;
    KEEP(f);
}

static YogVal 
YogBuiltinBoundMethod_allocate(YogEnv* env, YogVal klass) 
{
    YogBuiltinBoundMethod* method = ALLOC_OBJ(env, YogBuiltinBoundMethod_keep_children, NULL, YogBuiltinBoundMethod);
    YogBasicObj_init(env, (YogBasicObj*)method, 0, klass);

    method->self = YNIL;
    method->f = NULL;

    return OBJ2VAL(method);
}

static void 
YogScriptMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    KEEP_BASIC_OBJ;

    YogScriptMethod* method = ptr;

    KEEP(code);
    KEEP(globals);
    KEEP(outer_vars);
}

static void 
YogScriptMethod_init(YogEnv* env, YogScriptMethod* method, YogVal klass) 
{
    YogBasicObj_init(env, (YogBasicObj*)method, 0, klass);

    method->code = NULL;
    method->globals = NULL;
    method->outer_vars = NULL;
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
    YogBoundMethod* method = ALLOC_OBJ(env, YogBoundMethod_keep_children, NULL, YogBoundMethod);
    YogScriptMethod_init(env, (YogScriptMethod*)method, klass);

    method->self = YNIL;

    return OBJ2VAL(method);
}

static void 
YogBuiltinUnboundMethod_keep_chldren(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    KEEP_BASIC_OBJ;

    YogBuiltinUnboundMethod* method = ptr;
    KEEP(f);
}

static YogVal 
YogBuiltinUnboundMethod_allocate(YogEnv* env, YogVal klass) 
{
    YogBuiltinUnboundMethod* method = ALLOC_OBJ(env, YogBuiltinUnboundMethod_keep_chldren, NULL, YogBuiltinUnboundMethod);
    YogBasicObj_init(env, (YogBasicObj*)method, 0, klass);

    method->f = NULL;

    return OBJ2VAL(method);
}

static void 
YogUnboundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogScriptMethod_keep_children(env, ptr, keeper);
}

static YogVal 
YogUnboundMethod_allocate(YogEnv* env, YogVal klass) 
{
    YogScriptMethod* method = ALLOC_OBJ(env, YogUnboundMethod_keep_children, NULL, YogUnboundMethod);
    YogScriptMethod_init(env, method, klass);

    return OBJ2VAL(method);
}

#define RETURN_NEW_KLASS(allocator, name)  do { \
    YogVal klass = YogKlass_new(env, name, ENV_VM(env)->cObject); \
    YogKlass_define_allocator(env, klass, allocator); \
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
    return YogBuiltinBoundMethod_allocate(env, ENV_VM(env)->cBuiltinBoundMethod);
}

YogVal 
YogBoundMethod_new(YogEnv* env) 
{
    return YogBoundMethod_allocate(env, ENV_VM(env)->cBoundMethod);
}

YogVal 
YogBuiltinUnboundMethod_new(YogEnv* env) 
{
    return YogBuiltinUnboundMethod_allocate(env, ENV_VM(env)->cBuiltinUnboundMethod);
}

YogVal 
YogUnboundMethod_new(YogEnv* env) 
{
    return YogUnboundMethod_allocate(env, ENV_VM(env)->cUnboundMethod);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
