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
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogObj_keep_children(env, ptr, keeper);

    YogKlass* klass = ptr;
    klass->super = (*keeper)(env, klass->super);
}

YogBasicObj* 
YogKlass_allocate(YogEnv* env, YogKlass* klass) 
{
    YogObj* obj = ALLOC_OBJ(env, keep_children, YogKlass);
    YogObj_init(env, obj, 0, klass);

    return (YogBasicObj*)obj;
}

void 
YogKlass_define_allocator(YogEnv* env, YogKlass* klass, Allocator allocator) 
{
    klass->allocator = allocator;
}

YogKlass* 
YogKlass_new(YogEnv* env, const char* name, YogKlass* super) 
{
    YogKlass* klass = (YogKlass*)YogKlass_allocate(env, ENV_VM(env)->cKlass);
    klass->allocator = NULL;
    if (name != NULL) {
        klass->name = INTERN(name);
    }
    klass->super = super;

    return klass;
}

static YogVal 
new_(YogEnv* env)
{
    YogVal self = SELF(env);
    YogVal blockarg = ARG(env, 0);
    YogArray* vararg = OBJ_AS(YogArray, ARG(env, 1));

    YogKlass* klass = OBJ_AS(YogKlass, self);
    Allocator allocator = klass->allocator;
    while (allocator == NULL) {
        klass = klass->super;
        if (klass == NULL) {
            YOG_ASSERT(env, FALSE, "Can't allocate object.");
        }
        allocator = klass->allocator;
    }

    klass = OBJ_AS(YogKlass, self);
    YogBasicObj* obj = (*allocator)(env, klass);
    YogVal val = OBJ2VAL(obj);
    YogThread_call_method(env, ENV_TH(env), val, "initialize", vararg->body->size, vararg->body->items);

    return val;
}

void 
YogKlass_klass_init(YogEnv* env, YogKlass* cKlass) 
{
    YogKlass_define_method(env, cKlass, "new", new_, 1, 1, 0, 0, "block", NULL);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
