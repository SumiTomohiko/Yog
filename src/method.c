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

static YogBasicObj* 
YogBuiltinBoundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBuiltinBoundMethod* method = ALLOC_OBJ(env, YogBuiltinBoundMethod_keep_children, NULL, YogBuiltinBoundMethod);
    YogBasicObj_init(env, (YogBasicObj*)method, 0, klass);

    method->self = YNIL;
    method->f = NULL;

    return (YogBasicObj*)method;
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
YogScriptMethod_init(YogEnv* env, YogScriptMethod* method, YogKlass* klass) 
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

static YogBasicObj* 
YogBoundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBoundMethod* method = ALLOC_OBJ(env, YogBoundMethod_keep_children, NULL, YogBoundMethod);
    YogScriptMethod_init(env, (YogScriptMethod*)method, klass);

    method->self = YNIL;

    return (YogBasicObj*)method;
}

static void 
YogBuiltinUnboundMethod_keep_chldren(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    KEEP_BASIC_OBJ;

    YogBuiltinUnboundMethod* method = ptr;
    KEEP(f);
}

static YogBasicObj* 
YogBuiltinUnboundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBuiltinUnboundMethod* method = ALLOC_OBJ(env, YogBuiltinUnboundMethod_keep_chldren, NULL, YogBuiltinUnboundMethod);
    YogBasicObj_init(env, (YogBasicObj*)method, 0, klass);

    method->f = NULL;

    return (YogBasicObj*)method;
}

static void 
YogUnboundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogScriptMethod_keep_children(env, ptr, keeper);
}

static YogBasicObj* 
YogUnboundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogScriptMethod* method = ALLOC_OBJ(env, YogUnboundMethod_keep_children, NULL, YogUnboundMethod);
    YogScriptMethod_init(env, method, klass);

    return (YogBasicObj*)method;
}

#define RETURN_NEW_KLASS(allocator, name)  do { \
    YogKlass* klass = YogKlass_new(env, name, ENV_VM(env)->cObject); \
    YogKlass_define_allocator(env, klass, allocator); \
    return klass; \
} while (0)

YogKlass* 
YogBuiltinBoundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS(YogBuiltinBoundMethod_allocate, "BuiltinBoundMethod");
}

YogKlass* 
YogBoundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS(YogBoundMethod_allocate, "BoundMethod");
}

YogKlass* 
YogBuiltinUnboundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS(YogBuiltinUnboundMethod_allocate, "BuiltinUnboundMethod");
}

YogKlass* 
YogUnboundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS(YogUnboundMethod_allocate, "UnboundMethod");
}

#undef RETURN_NEW_KLASS

YogBuiltinBoundMethod* 
YogBuiltinBoundMethod_new(YogEnv* env) 
{
    YogBuiltinBoundMethod* method = (YogBuiltinBoundMethod*)YogBuiltinBoundMethod_allocate(env, ENV_VM(env)->cBuiltinBoundMethod);

    return method;
}

YogBoundMethod* 
YogBoundMethod_new(YogEnv* env) 
{
    YogBoundMethod* method = (YogBoundMethod*)YogBoundMethod_allocate(env, ENV_VM(env)->cBoundMethod);

    return method;
}

YogBuiltinUnboundMethod* 
YogBuiltinUnboundMethod_new(YogEnv* env) 
{
    YogBuiltinUnboundMethod* method = (YogBuiltinUnboundMethod*)YogBuiltinUnboundMethod_allocate(env, ENV_VM(env)->cBuiltinUnboundMethod);

    return method;
}

YogUnboundMethod* 
YogUnboundMethod_new(YogEnv* env) 
{
    YogUnboundMethod* method = (YogUnboundMethod*)YogUnboundMethod_allocate(env, ENV_VM(env)->cUnboundMethod);

    return method;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
