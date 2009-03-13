#include <stdarg.h>
#include "yog/error.h"
#include "yog/function.h"
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
    YogVal builtin_f = YogBuiltinFunction_new(env, f, OBJ_AS(YogKlass, klass)->name, func_name, blockargc, varargc, kwargc, required_argc, ap);
    va_end(ap);
    PUSH_LOCAL(env, builtin_f);

    YogVal method = YogBuiltinUnboundMethod_new(env);
    OBJ_AS(YogBuiltinUnboundMethod, method)->f = builtin_f;
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
    SAVE_ARG(env, super);

    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_allocate(env, ENV_VM(env)->cKlass);
    OBJ_AS(YogKlass, klass)->allocator = NULL;
    OBJ_AS(YogKlass, klass)->name = INVALID_ID;
    OBJ_AS(YogKlass, klass)->super = PTR2VAL(NULL);

    OBJ_AS(YogKlass, klass)->allocator = NULL;
    if (name != NULL) {
        ID id = INTERN(name);
        OBJ_AS(YogKlass, klass)->name = id;
    }
    OBJ_AS(YogKlass, klass)->super = super;
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

    Allocator allocator = OBJ_AS(YogKlass, self)->allocator;
    YogVal klass = self;
    while (allocator == NULL) {
        klass = OBJ_AS(YogKlass, klass)->super;
        if (VAL2PTR(klass) == NULL) {
            YOG_ASSERT(env, FALSE, "Can't allocate object.");
        }
        allocator = OBJ_AS(YogKlass, klass)->allocator;
    }

    obj = (*allocator)(env, self);
    unsigned int size = OBJ_AS(YogArray, vararg)->body->size;
    YogVal* items = OBJ_AS(YogArray, vararg)->body->items;
    /* TODO: dirty hack */
    YogVal args[size];
    unsigned int i;
    for (i = 0; i < size; i++) {
        args[i] = items[i];
    }
    YogThread_call_method(env, ENV_TH(env), obj, "initialize", size, args);

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
