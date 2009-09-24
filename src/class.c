#include "config.h"
#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#if defined(HAVE_MALLOC_H)
#   include <malloc.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "yog/array.h"
#include "yog/classmethod.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/class.h"
#include "yog/property.h"
#include "yog/thread.h"
#include "yog/yog.h"

static YogVal
call_get_attr(YogEnv* env, YogVal self, ID name)
{
    SAVE_ARG(env, self);
    YogVal klass = YUNDEF;
    YogVal attr = YUNDEF;
    PUSH_LOCALS2(env, klass, attr);

    klass = YogVal_get_class(env, self);
    attr = YogClass_get_attr(env, klass, name);
    if (!IS_UNDEF(attr)) {
        attr = YogVal_get_descr(env, attr, self, klass);
        RETURN(env, attr);
    }

    attr = YogObj_get_attr(env, self, name);
    if (!IS_UNDEF(attr)) {
        attr = YogVal_get_descr(env, attr, YNIL, self);
        RETURN(env, attr);
    }

    RETURN(env, YUNDEF);
}

#include "yog/vm.h"

static void
exec_get_attr(YogEnv* env, YogVal self, ID name)
{
    SAVE_ARG(env, self);
    YogVal klass = YUNDEF;
    YogVal attr = YUNDEF;
    PUSH_LOCALS2(env, klass, attr);

    klass = YogVal_get_class(env, self);
    attr = YogClass_get_attr(env, klass, name);
    if (!IS_UNDEF(attr)) {
        attr = YogVal_get_descr(env, attr, self, klass);
        FRAME_PUSH(env, attr);
        RETURN_VOID(env);
    }

    attr = YogObj_get_attr(env, self, name);
    if (!IS_UNDEF(attr)) {
        attr = YogVal_get_descr(env, attr, YNIL, self);
        FRAME_PUSH(env, attr);
        RETURN_VOID(env);
    }

    YOG_BUG(env, "attribute not found");

    /* NOTREACHED */
    RETURN_VOID(env);
}

void
YogClass_define_class_method(YogEnv* env, YogVal self, const char* name, void* f)
{
    SAVE_ARG(env, self);
    YogVal func = YUNDEF;
    YogVal method = YUNDEF;
    PUSH_LOCALS2(env, func, method);

    YogVal class_name = PTR_AS(YogClass, self)->name;
    func = YogNativeFunction_new(env, class_name, name, f);
    method = YogClassMethod_new(env);
    PTR_AS(YogClassMethod, method)->f = func;

    YogObj_set_attr(env, self, name, method);

    RETURN_VOID(env);
}

void
YogClass_define_method(YogEnv* env, YogVal klass, const char* name, void* f)
{
    SAVE_ARG(env, klass);

    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    YogVal class_name = PTR_AS(YogClass, klass)->name;
    func = YogNativeFunction_new(env, class_name, name, f);
    YogObj_set_attr(env, klass, name, func);

    RETURN_VOID(env);
}

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogObj_keep_children(env, ptr, keeper, heap);

    YogClass* klass = PTR_AS(YogClass, ptr);
    YogGC_keep(env, &klass->super, keeper, heap);
}

YogVal
YogClass_allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal obj = ALLOC_OBJ(env, keep_children, NULL, YogClass);
    YogObj_init(env, obj, 0, klass);

    RETURN(env, obj);
}

void
YogClass_define_allocator(YogEnv* env, YogVal klass, Allocator allocator)
{
    PTR_AS(YogClass, klass)->allocator = allocator;
}

YogVal
YogClass_new(YogEnv* env, const char* name, YogVal super)
{
    SAVE_ARG(env, super);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_allocate(env, env->vm->cClass);
    PTR_AS(YogClass, klass)->allocator = NULL;
    PTR_AS(YogClass, klass)->name = INVALID_ID;
    PTR_AS(YogClass, klass)->super = PTR2VAL(NULL);

    PTR_AS(YogClass, klass)->allocator = NULL;
    if (name != NULL) {
        ID id = YogVM_intern(env, env->vm, name);
        PTR_AS(YogClass, klass)->name = id;
    }
    PTR_AS(YogClass, klass)->super = super;
    PTR_AS(YogClass, klass)->exec_get_attr = NULL;
    PTR_AS(YogClass, klass)->call_get_attr = NULL;
    PTR_AS(YogClass, klass)->exec_get_descr = NULL;
    PTR_AS(YogClass, klass)->call_get_descr = NULL;
    PTR_AS(YogClass, klass)->exec_set_descr = NULL;
    PTR_AS(YogClass, klass)->call = NULL;
    PTR_AS(YogClass, klass)->exec = NULL;

    RETURN(env, klass);
}

static YogVal
new_(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    Allocator allocator = PTR_AS(YogClass, self)->allocator;
    YogVal klass = self;
    while (allocator == NULL) {
        klass = PTR_AS(YogClass, klass)->super;
        if (VAL2PTR(klass) == NULL) {
            YOG_ASSERT(env, FALSE, "Can't allocate object.");
        }
        allocator = PTR_AS(YogClass, klass)->allocator;
    }

    obj = (*allocator)(env, self);
    uint_t argc = YogArray_size(env, args);
    YogVal body = PTR_AS(YogArray, args)->body;
    YogVal* items = PTR_AS(YogValArray, body)->items;
    /* TODO: dirty hack */
#if defined(_alloca) && !defined(alloca)
#   define alloca   _alloca
#endif
    YogVal* arg = (YogVal*)alloca(sizeof(YogVal) * argc);
    uint_t i;
    for (i = 0; i < argc; i++) {
        arg[i] = items[i];
    }
    PUSH_LOCALSX(env, argc, arg);
    if (IS_PTR(block)) {
        YogEval_call_method2(env, obj, "init", argc, arg, block);
    }
    else {
        YogEval_call_method(env, obj, "init", argc, arg);
    }

    RETURN(env, obj);
}

void
YogClass_class_init(YogEnv* env, YogVal cClass)
{
    YogClass_define_method(env, cClass, "new", new_);
}

YogVal
YogClass_get_attr(YogEnv* env, YogVal self, ID name)
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
        klass = PTR_AS(YogClass, klass)->super;
    } while (IS_PTR(klass));

    RETURN(env, YUNDEF);
}

void
YogClass_define_descr_get_executor(YogEnv* env, YogVal self, void (*getter)(YogEnv*, YogVal, YogVal, YogVal))
{
    PTR_AS(YogClass, self)->exec_get_descr = getter;
}

void
YogClass_define_descr_get_caller(YogEnv* env, YogVal self, YogVal (*getter)(YogEnv*, YogVal, YogVal, YogVal))
{
    PTR_AS(YogClass, self)->call_get_descr = getter;
}

void
YogClass_define_descr_set_executor(YogEnv* env, YogVal self, void (*setter)(YogEnv*, YogVal, YogVal, YogVal))
{
    PTR_AS(YogClass, self)->exec_set_descr = setter;
}

void
YogClass_define_get_attr_caller(YogEnv* env, YogVal self, GetAttrCaller getter)
{
    PTR_AS(YogClass, self)->call_get_attr = getter;
}

void
YogClass_define_get_attr_executor(YogEnv* env, YogVal self, GetAttrExecutor getter)
{
    PTR_AS(YogClass, self)->exec_get_attr = getter;
}

void
YogClass_define_caller(YogEnv* env, YogVal self, Caller call)
{
    PTR_AS(YogClass, self)->call = call;
}

void
YogClass_define_executor(YogEnv* env, YogVal self, Executor exec)
{
    PTR_AS(YogClass, self)->exec = exec;
}

void
YogClass_define_property(YogEnv* env, YogVal self, const char* name, void* get, void* set)
{
    SAVE_ARG(env, self);
    YogVal getter = YUNDEF;
    YogVal setter = YUNDEF;
    YogVal prop = YUNDEF;
    PUSH_LOCALS3(env, getter, setter, prop);

    ID class_name = PTR_AS(YogClass, self)->name;

    if (get != NULL) {
#define GETTER_HEAD     "get_"
        uint_t size = strlen(GETTER_HEAD) + strlen(name) + 1;
        char* getter_name = (char*)alloca(sizeof(char) * size);
#if defined(_MSC_VER)
#   define snprintf     _snprintf
#endif
        snprintf(getter_name, size, GETTER_HEAD "%s", name);
        getter = YogNativeFunction_new(env, class_name, getter_name, get);
#undef GETTER_HEAD
    }

    if (set != NULL) {
#define SETTER_HEAD     "set_"
        uint_t size = strlen(SETTER_HEAD) + strlen(name) + 1;
        char* setter_name = (char*)alloca(sizeof(char) * size);
        snprintf(setter_name, size, SETTER_HEAD "%s", name);
        setter = YogNativeFunction_new(env, class_name, setter_name, set);
#undef setter_HEAD
    }

    prop = YogProperty_new(env);
    PTR_AS(YogProperty, prop)->getter = getter;
    PTR_AS(YogProperty, prop)->setter = setter;

    YogObj_set_attr(env, self, name, prop);

    RETURN_VOID(env);
}

struct ModuleClass {
    struct YogObj base;
    YogVal super;
    Allocator allocator;
};

typedef struct ModuleClass ModuleClass;

static void
ModuleClass_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogObj_keep_children(env, ptr, keeper, heap);

    ModuleClass* klass = PTR_AS(ModuleClass, ptr);
#define KEEP(member)   YogGC_keep(env, &klass->member, keeper, heap)
    KEEP(super);
#undef KEEP
}

static YogVal
ModuleClass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = ALLOC_OBJ(env, ModuleClass_keep_children, NULL, ModuleClass);
    YogObj_init(env, klass, 0, YUNDEF);
    PTR_AS(ModuleClass, klass)->super = YUNDEF;
    PTR_AS(ModuleClass, klass)->allocator = NULL;

    RETURN(env, klass);
}

void
YogClass_include_module(YogEnv* env, YogVal self, YogVal module)
{
    YOG_ASSERT(env, IS_PTR(self), "self is not pointer");
    YOG_ASSERT(env, IS_OBJ_OF(env, self, cClass), "self is not Class");

    SAVE_ARGS2(env, self, module);
    YogVal module_class = YUNDEF;
    PUSH_LOCAL(env, module_class);

    module_class = ModuleClass_new(env);
    PTR_AS(YogObj, module_class)->attrs = PTR_AS(YogObj, module)->attrs;
    PTR_AS(ModuleClass, module_class)->super = PTR_AS(YogClass, self)->super;
    PTR_AS(YogClass, self)->super = module_class;

    RETURN_VOID(env);
}

void
YogClass_boot(YogEnv* env, YogVal cClass)
{
    SAVE_ARG(env, cClass);

    YogClass_define_get_attr_caller(env, cClass, call_get_attr);
    YogClass_define_get_attr_executor(env, cClass, exec_get_attr);

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
