#include <stdarg.h>
#include "yog/error.h"
#include "yog/function.h"
#include "yog/method.h"
#include "yog/yog.h"

void 
YogKlass_define_method(YogEnv* env, YogVal klass, const char* name, void* f, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, int required_argc, ...)
{
    ID func_name = INTERN(name);

    va_list ap;
    va_start(ap, required_argc);
    YogBuiltinFunction* builtin_f = YogBuiltinFunction_new(env, f, OBJ_AS(YogKlass, klass)->name, func_name, blockargc, varargc, kwargc, required_argc, ap);
    va_end(ap);

    YogVal method = YogBuiltinUnboundMethod_new(env);
    OBJ_AS(YogBuiltinUnboundMethod, method)->f = builtin_f;

    YogObj_set_attr_id(env, klass, func_name, method);
}

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogObj_keep_children(env, ptr, keeper);

    YogKlass* klass = ptr;
    klass->super = PTR2VAL((*keeper)(env, VAL2PTR(klass->super)));
}

YogVal 
YogKlass_allocate(YogEnv* env, YogVal klass)
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    YogObj* obj = ALLOC_OBJ(env, keep_children, NULL, YogKlass);
    YogObj_init(env, obj, 0, klass);

    RETURN(env, OBJ2VAL(obj));
}

void 
YogKlass_define_allocator(YogEnv* env, YogVal klass, Allocator allocator) 
{
    OBJ_AS(YogKlass, klass)->allocator = allocator;
}

YogVal 
YogKlass_new(YogEnv* env, const char* name, YogVal super) 
{
    YogVal klass = YogKlass_allocate(env, ENV_VM(env)->cKlass);
    OBJ_AS(YogKlass, klass)->allocator = NULL;
    if (name != NULL) {
        OBJ_AS(YogKlass, klass)->name = INTERN(name);
    }
    OBJ_AS(YogKlass, klass)->super = super;

    return klass;
}

static YogVal 
new_(YogEnv* env)
{
    YogVal self = SELF(env);
    YogVal blockarg = ARG(env, 0);
    YogArray* vararg = OBJ_AS(YogArray, ARG(env, 1));

    Allocator allocator = OBJ_AS(YogKlass, self)->allocator;
    YogVal klass = self;
    while (allocator == NULL) {
        klass = OBJ_AS(YogKlass, klass)->super;
        if (VAL2PTR(klass) == NULL) {
            YOG_ASSERT(env, FALSE, "Can't allocate object.");
        }
        allocator = OBJ_AS(YogKlass, klass)->allocator;
    }

    YogVal obj = (*allocator)(env, self);
    YogThread_call_method(env, ENV_TH(env), obj, "initialize", vararg->body->size, vararg->body->items);

    return obj;
}

void 
YogKlass_klass_init(YogEnv* env, YogVal cKlass) 
{
    YogKlass_define_method(env, cKlass, "new", new_, 1, 1, 0, 0, "block", NULL);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
