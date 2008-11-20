#include "yog/yog.h"

#define GC_SELF(method) do { \
    YogVal self = method->self; \
    if (IS_OBJ(self)) { \
        DO_GC(env, do_gc, VAL2OBJ(self)); \
    } \
} while (0)

static void 
gc_builtin_bound_method_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogBuiltinBoundMethod* method = ptr;
    GC_SELF(method);
    DO_GC(env, do_gc, method->f);
}

static YogBasicObj* 
YogBuiltinBoundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, gc_builtin_bound_method_children, YogBuiltinBoundMethod);
    YogBasicObj_init(env, obj, 0, klass);
    return obj;
}

static void 
gc_bound_method_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogBoundMethod* method = ptr;
    GC_SELF(method);
    DO_GC(env, do_gc, method->code);
}

static YogBasicObj* 
YogBoundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, gc_bound_method_children, YogBoundMethod);
    YogBasicObj_init(env, obj, 0, klass);
    return obj;
}

static void 
gc_builtin_unbound_method_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogBuiltinUnboundMethod* method = ptr;
    DO_GC(env, do_gc, method->f);
}

static YogBasicObj* 
YogBuiltinUnboundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, gc_builtin_unbound_method_children, YogBuiltinUnboundMethod);
    YogBasicObj_init(env, obj, 0, klass);
    return obj;
}

static void 
gc_unbound_method_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogUnboundMethod* method = ptr;
    DO_GC(env, do_gc, method->code);
}

static YogBasicObj* 
YogUnboundMethod_allocate(YogEnv* env, YogKlass* klass) 
{
    YogBasicObj* obj = ALLOC_OBJ(env, gc_unbound_method_children, YogUnboundMethod);
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
