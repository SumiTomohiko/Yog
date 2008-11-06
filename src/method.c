#include "yog/yog.h"

#define RETURN_NEW_KLASS(name)  do { \
    return YogKlass_new(env, name, ENV_VM(env)->obj_klass); \
} while (0)

YogKlass* 
YogBuiltinBoundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS("BuiltinBoundMethod");
}

YogKlass* 
YogBoundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS("BoundMethod");
}

YogKlass* 
YogBuiltinUnboundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS("BuiltinUnboundMethod");
}

YogKlass* 
YogUnboundMethod_klass_new(YogEnv* env) 
{
    RETURN_NEW_KLASS("UnboundMethod");
}

#undef RETURN_NEW_KLASS

#define GC_SELF(method) do { \
    YogVal self = method->self; \
    if (IS_OBJ(self)) { \
        DO_GC(env, do_gc, YOGVAL_OBJ(self)); \
    } \
} while (0)

static void 
gc_builtin_bound_method_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogBuiltinBoundMethod* method = ptr;
    GC_SELF(method);
    DO_GC(env, do_gc, method->f);
}

YogBuiltinBoundMethod* 
YogBuiltinBoundMethod_new(YogEnv* env) 
{
    YogBuiltinBoundMethod* method = ALLOC_OBJ(env, gc_builtin_bound_method_children, YogBuiltinBoundMethod);
    YogBasicObj_init(env, YOGBASICOBJ(method), ENV_VM(env)->builtin_bound_method_klass);
    method->self = YogVal_nil();
    method->f = NULL;

    return method;
}

static void 
gc_bound_method_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogBoundMethod* method = ptr;
    GC_SELF(method);
    DO_GC(env, do_gc, method->code);
}

#undef GC_SELF

YogBoundMethod* 
YogBoundMethod_new(YogEnv* env) 
{
    YogBoundMethod* method = ALLOC_OBJ(env, gc_bound_method_children, YogBoundMethod);
    YogBasicObj_init(env, YOGBASICOBJ(method), ENV_VM(env)->bound_method_klass);
    method->self = YogVal_nil();
    method->code = NULL;

    return method;
}

static void 
gc_builtin_unbound_method_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogBuiltinUnboundMethod* method = ptr;
    DO_GC(env, do_gc, method->f);
}

YogBuiltinUnboundMethod* 
YogBuiltinUnboundMethod_new(YogEnv* env) 
{
    YogBuiltinUnboundMethod* method = ALLOC_OBJ(env, gc_builtin_unbound_method_children, YogBuiltinUnboundMethod);
    YogBasicObj_init(env, YOGBASICOBJ(method), ENV_VM(env)->builtin_unbound_method_klass);
    method->f = NULL;

    return method;
}

static void 
gc_unbound_method_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogUnboundMethod* method = ptr;
    DO_GC(env, do_gc, method->code);
}

YogUnboundMethod* 
YogUnboundMethod_new(YogEnv* env) 
{
    YogUnboundMethod* method = ALLOC_OBJ(env, gc_unbound_method_children, YogUnboundMethod);
    method->code = NULL;

    return method;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
