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

    YogVal val = YogVal_obj(YOGBASICOBJ(method));
    YogObj_set_attr(env, YOGOBJ(klass), name, val);
}

static void 
gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogObj_gc_children(env, ptr, do_gc);

    YogKlass* klass = ptr;
    klass->super = do_gc(env, klass->super);
}

YogKlass* 
YogKlass_new(YogEnv* env, const char* name, YogKlass* super) 
{
    YogKlass* klass = ALLOC_OBJ(env, gc_children, YogKlass);
    YogObj_init(env, YOGOBJ(klass), ENV_VM(env)->klass_klass);
    if (name != NULL) {
        klass->name = INTERN(name);
    }
    klass->super = super;

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
