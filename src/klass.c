#include <stdarg.h>
#include "yog/yog.h"

void 
YogKlass_define_method(YogEnv* env, YogKlass* klass, const char* name, void* f, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, int required_argc, ...)
{
    va_list ap;
    va_start(ap, required_argc);
    YogBuiltinFunction* builtin_f = YogBuiltinFunction_new(env, name, f, blockargc, varargc, kwargc, required_argc, ap);
    va_end(ap);

    YogBuiltinUnboundMethod* method = YogBuiltinUnboundMethod_new(env);
    method->f = builtin_f;

    YogVal val = OBJ2VAL(method);
    YogObj_set_attr(env, YOGOBJ(klass), name, val);
}

static void 
gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogObj_gc_children(env, ptr, do_gc);

    YogKlass* klass = ptr;
    klass->super = do_gc(env, klass->super);
}

YogBasicObj* 
YogKlass_allocate(YogEnv* env, YogKlass* klass) 
{
    YogObj* obj = ALLOC_OBJ(env, gc_children, YogKlass);
    YogObj_init(env, obj, 0, klass);

    return (YogBasicObj*)obj;
}

YogKlass* 
YogKlass_new(YogEnv* env, Allocator allocator, const char* name, YogKlass* super) 
{
    YogKlass* klass = (YogKlass*)YogKlass_allocate(env, ENV_VM(env)->klass_klass);
    klass->allocator = allocator;
    if (name != NULL) {
        klass->name = INTERN(name);
    }
    klass->super = super;

    return klass;
}

static YogVal 
klass_new(YogEnv* env, YogVal self, YogVal blockarg, YogArray* vararg)
{
    YogKlass* klass = (YogKlass*)VAL2OBJ(self);
    YogBasicObj* obj = (*klass->allocator)(env, klass);
    YogVal val = OBJ2VAL(obj);
    YogThread_call_method(env, ENV_TH(env), val, "initialize", vararg->body->size, vararg->body->items);

    return val;
}

void 
YogKlass_klass_init(YogEnv* env, YogKlass* klass_klass) 
{
    YogKlass_define_method(env, klass_klass, "new", klass_new, 1, 1, 0, 0, "block", NULL);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
