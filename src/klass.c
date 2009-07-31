#include <stdarg.h>
#include "yog/array.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/thread.h"
#include "yog/yog.h"

void 
YogKlass_define_method(YogEnv* env, YogVal klass, const char* name, void* f)
{
    SAVE_ARG(env, klass);

    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    YogVal klass_name = PTR_AS(YogKlass, klass)->name;
    func = YogNativeFunction_new(env, klass_name, name, f);
    YogObj_set_attr(env, klass, name, func);

    RETURN_VOID(env);
}

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogObj_keep_children(env, ptr, keeper, heap);

    YogKlass* klass = ptr;
    YogGC_keep(env, &klass->super, keeper, heap);
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
    PTR_AS(YogKlass, klass)->super = PTR2VAL(NULL);

    PTR_AS(YogKlass, klass)->allocator = NULL;
    if (name != NULL) {
        ID id = YogVM_intern(env, env->vm, name);
        PTR_AS(YogKlass, klass)->name = id;
    }
    PTR_AS(YogKlass, klass)->super = super;
    PTR_AS(YogKlass, klass)->get_attr = NULL;
    PTR_AS(YogKlass, klass)->get_descr = NULL;
    PTR_AS(YogKlass, klass)->call = NULL;
    PTR_AS(YogKlass, klass)->exec = NULL;
    RETURN(env, klass);
}

static YogVal 
new_(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

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
    uint_t argc = YogArray_size(env, args);
    YogVal body = PTR_AS(YogArray, args)->body;
    YogVal* items = PTR_AS(YogValArray, body)->items;
    /* TODO: dirty hack */
    YogVal arg[argc];
    uint_t i;
    for (i = 0; i < argc; i++) {
        arg[i] = items[i];
    }
    PUSH_LOCALSX(env, argc, arg);
    if (IS_PTR(block)) {
        YogEval_call_method2(env, obj, "initialize", argc, arg, block);
    }
    else {
        YogEval_call_method(env, obj, "initialize", argc, arg);
    }

    RETURN(env, obj);
}

void 
YogKlass_klass_init(YogEnv* env, YogVal cKlass) 
{
    YogKlass_define_method(env, cKlass, "new", new_);
}

YogVal
YogKlass_get_attr(YogEnv* env, YogVal self, ID name)
{
    SAVE_ARG(env, self);
    YogVal attr = YUNDEF;
    YogVal klass = YUNDEF;
    PUSH_LOCALS2(env, attr, klass);

    klass = self;
    do {
        attr = YogObj_get_attr(env, klass, name);
        if (!IS_UNDEF(attr)) {
            RETURN(env, attr);
        }
        klass = PTR_AS(YogKlass, klass)->super;
    } while (IS_PTR(klass));

    RETURN(env, YUNDEF);
}

void
YogKlass_define_descr_getter(YogEnv* env, YogVal self, void* getter)
{
    PTR_AS(YogKlass, self)->get_descr = getter;
}

void
YogKlass_define_attr_getter(YogEnv* env, YogVal self, AttrGetter getter)
{
    PTR_AS(YogKlass, self)->get_attr = getter;
}

void
YogKlass_define_caller(YogEnv* env, YogVal self, Caller call)
{
    PTR_AS(YogKlass, self)->call = call;
}

void
YogKlass_define_executor(YogEnv* env, YogVal self, Executor exec)
{
    PTR_AS(YogKlass, self)->exec = exec;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
