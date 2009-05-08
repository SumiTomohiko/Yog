#include <stdarg.h>
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/method.h"
#include "yog/yog.h"

void 
YogKlass_define_method(YogEnv* env, YogVal klass, const char* name, void* f, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, int required_argc, ...)
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, klass);

    ID func_name = INTERN(name);

    va_list ap;
    va_start(ap, required_argc);
    YogVal builtin_f = YogBuiltinFunction_new(env, f, PTR_AS(YogKlass, klass)->name, func_name, blockargc, varargc, kwargc, required_argc, ap);
    va_end(ap);
    PUSH_LOCAL(env, builtin_f);

    YogVal method = YogBuiltinUnboundMethod_new(env);
    MODIFY(env, PTR_AS(YogBuiltinUnboundMethod, method)->f, builtin_f);
    PUSH_LOCAL(env, method);

    YogObj_set_attr_id(env, klass, func_name, method);

    RETURN_VOID(env);
}

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogObj_keep_children(env, ptr, keeper);

    YogKlass* klass = ptr;
    klass->super = YogVal_keep(env, klass->super, keeper);
}

YogVal 
YogKlass_allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal obj = ALLOC_OBJ(env, keep_children, NULL, YogKlass);
    YogObj_init(env, obj, 0, klass);

    RETURN(env, obj);
}

void 
YogKlass_define_allocator(YogEnv* env, YogVal klass, Allocator allocator) 
{
    PTR_AS(YogKlass, klass)->allocator = allocator;
}

YogVal 
YogKlass_new(YogEnv* env, const char* name, YogVal super) 
{
    SAVE_ARG(env, super);

    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_allocate(env, env->vm->cKlass);
    PTR_AS(YogKlass, klass)->allocator = NULL;
    PTR_AS(YogKlass, klass)->name = INVALID_ID;
    MODIFY(env, PTR_AS(YogKlass, klass)->super, PTR2VAL(NULL));

    PTR_AS(YogKlass, klass)->allocator = NULL;
    if (name != NULL) {
        ID id = INTERN(name);
        PTR_AS(YogKlass, klass)->name = id;
    }
    MODIFY(env, PTR_AS(YogKlass, klass)->super, super);
    RETURN(env, klass);
}

static YogVal 
new_(YogEnv* env)
{
    YogVal self = SELF(env);
    YogVal blockarg = ARG(env, 0);
    YogVal vararg = ARG(env, 1);
    YogVal obj = YUNDEF;
    PUSH_LOCALS4(env, self, blockarg, vararg, obj);

    Allocator allocator = PTR_AS(YogKlass, self)->allocator;
    YogVal klass = self;
    while (allocator == NULL) {
        klass = PTR_AS(YogKlass, klass)->super;
        if (VAL2PTR(klass) == NULL) {
            YOG_ASSERT(env, FALSE, "Can't allocate object.");
        }
        allocator = PTR_AS(YogKlass, klass)->allocator;
    }

    obj = (*allocator)(env, self);
    YogVal body = PTR_AS(YogArray, vararg)->body;
    unsigned int size = PTR_AS(YogValArray, body)->size;
    YogVal* items = PTR_AS(YogValArray, body)->items;
    /* TODO: dirty hack */
    YogVal args[size];
    unsigned int i;
    for (i = 0; i < size; i++) {
        args[i] = items[i];
    }
    PUSH_LOCALSX(env, size, args);
    YogEval_call_method(env, obj, "initialize", size, args);

    POP_LOCALS(env);
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
