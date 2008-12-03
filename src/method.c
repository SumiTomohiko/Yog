#include "yog/method.h"
#include "yog/yog.h"

#define KEEP_SUPER  YogBasicObj_keep_children(env, ptr, keeper)

#define KEEP_SELF   method->self = YogVal_keep(env, method->self, keeper)
#define KEEP_MEMBER(member)     method->member = (*keeper)(env, method->member)

static void 
YogBuiltinBoundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    KEEP_SUPER;

    YogBuiltinBoundMethod* method = ptr;
    KEEP_SELF;
    KEEP_MEMBER(f);
}

static YogBasicObj* 
YogBuiltinBoundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, YogBuiltinBoundMethod_keep_children, YogBuiltinBoundMethod);
    YogBasicObj_init(env, obj, 0, klass);
    return obj;
}

static void 
YogBoundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    KEEP_SUPER;

    YogBoundMethod* method = ptr;
    KEEP_SELF;
    KEEP_MEMBER(code);
}

static YogBasicObj* 
YogBoundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, YogBoundMethod_keep_children, YogBoundMethod);
    YogBasicObj_init(env, obj, 0, klass);
    return obj;
}

static void 
YogBuiltinUnboundMethod_keep_chldren(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    KEEP_SUPER;

    YogBuiltinUnboundMethod* method = ptr;
    KEEP_MEMBER(f);
}

static YogBasicObj* 
YogBuiltinUnboundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, YogBuiltinUnboundMethod_keep_chldren, YogBuiltinUnboundMethod);
    YogBasicObj_init(env, obj, 0, klass);
    return obj;
}

static void 
YogUnboundMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    KEEP_SUPER;

    YogUnboundMethod* method = ptr;
    KEEP_MEMBER(code);
}

static YogBasicObj* 
YogUnboundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, YogUnboundMethod_keep_children, YogUnboundMethod);
    YogBasicObj_init(env, obj, 0, klass);
    return obj;
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
    method->self = YNIL;
    method->f = NULL;

    return method;
}

YogBoundMethod* 
YogBoundMethod_new(YogEnv* env) 
{
    YogBoundMethod* method = (YogBoundMethod*)YogBoundMethod_allocate(env, ENV_VM(env)->cBoundMethod);
    method->self = YNIL;
    method->code = NULL;

    return method;
}

YogBuiltinUnboundMethod* 
YogBuiltinUnboundMethod_new(YogEnv* env) 
{
    YogBuiltinUnboundMethod* method = (YogBuiltinUnboundMethod*)YogBuiltinUnboundMethod_allocate(env, ENV_VM(env)->cBuiltinUnboundMethod);
    method->f = NULL;

    return method;
}

YogUnboundMethod* 
YogUnboundMethod_new(YogEnv* env) 
{
    YogUnboundMethod* method = (YogUnboundMethod*)YogUnboundMethod_allocate(env, ENV_VM(env)->cUnboundMethod);
    method->code = NULL;

    return method;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
