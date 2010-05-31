#include "yog/config.h"
#if defined(HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yog/array.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/classmethod.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/property.h"
#include "yog/sysdeps.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_PTR((self)) || ((BASIC_OBJ_FLAGS((self)) & FLAG_CLASS) == 0)) { \
        YogError_raise_TypeError((env), "self must be Class"); \
    } \
} while (0)

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

    YogError_raise_AttributeError(env, "%C object has no attribute \"%I\"", self, name);

    /* NOTREACHED */
    RETURN_VOID(env);
}

void
YogClass_define_class_method(YogEnv* env, YogVal self, YogVal pkg, const char* name, YogAPI f)
{
    SAVE_ARGS2(env, self, pkg);
    YogVal func = YUNDEF;
    YogVal method = YUNDEF;
    PUSH_LOCALS2(env, func, method);

    YogVal class_name = PTR_AS(YogClass, self)->name;
    func = YogNativeFunction_new(env, class_name, pkg, name, f);
    method = YogClassMethod_new(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogClassMethod, method), f, func);

    YogObj_set_attr(env, self, name, method);

    RETURN_VOID(env);
}

void
YogClass_define_method(YogEnv* env, YogVal klass, YogVal pkg, const char* name, YogAPI f)
{
    SAVE_ARGS2(env, klass, pkg);
    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    YogVal class_name = PTR_AS(YogClass, klass)->name;
    func = YogNativeFunction_new(env, class_name, pkg, name, f);
    YogObj_set_attr(env, klass, name, func);

    RETURN_VOID(env);
}

void
YogClass_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogObj_keep_children(env, ptr, keeper, heap);

    YogClass* klass = PTR_AS(YogClass, ptr);
    YogGC_KEEP(env, klass, super, keeper, heap);
}

void
YogClass_define_allocator(YogEnv* env, YogVal klass, Allocator allocator)
{
    PTR_AS(YogClass, klass)->allocator = allocator;
}

void
YogClass_init(YogEnv* env, YogVal self, type_t type, YogVal klass)
{
    SAVE_ARGS2(env, self, klass);

    YogObj_init(env, self, type, FLAG_CLASS, klass);
    PTR_AS(YogClass, self)->allocator = NULL;
    PTR_AS(YogClass, self)->name = INVALID_ID;
    PTR_AS(YogClass, self)->super = YUNDEF;
    PTR_AS(YogClass, self)->allocator = NULL;
    PTR_AS(YogClass, self)->exec_get_attr = NULL;
    PTR_AS(YogClass, self)->call_get_attr = NULL;
    PTR_AS(YogClass, self)->exec_get_descr = NULL;
    PTR_AS(YogClass, self)->call_get_descr = NULL;
    PTR_AS(YogClass, self)->exec_set_descr = NULL;
    PTR_AS(YogClass, self)->call = NULL;
    PTR_AS(YogClass, self)->exec = NULL;

    RETURN_VOID(env);
}

YogVal
YogClass_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = ALLOC_OBJ(env, YogClass_keep_children, NULL, YogClass);
    YogClass_init(env, obj, TYPE_CLASS, klass);

    RETURN(env, obj);
}

YogVal
YogClass_new(YogEnv* env, const char* name, YogVal super)
{
    SAVE_ARG(env, super);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogClass_alloc(env, env->vm->cClass);
    YogGC_UPDATE_PTR(env, PTR_AS(YogClass, obj), super, super);
    ID id = name == NULL ? INVALID_ID : YogVM_intern(env, env->vm, name);
    PTR_AS(YogClass, obj)->name = id;

    RETURN(env, obj);
}

static YogVal
new_(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
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
    YogVal* arg = (YogVal*)YogSysdeps_alloca(sizeof(YogVal) * argc);
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
YogClass_class_init(YogEnv* env, YogVal cClass, YogVal pkg)
{
    SAVE_ARGS2(env, cClass, pkg);
    YogClass_define_method(env, cClass, pkg, "new", new_);
    RETURN_VOID(env);
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
YogClass_define_property(YogEnv* env, YogVal self, YogVal pkg, const char* name, YogAPI get, YogAPI set)
{
    SAVE_ARGS2(env, self, pkg);
    YogVal getter = YUNDEF;
    YogVal setter = YUNDEF;
    YogVal prop = YUNDEF;
    PUSH_LOCALS3(env, getter, setter, prop);

    ID class_name = PTR_AS(YogClass, self)->name;

    if (get != NULL) {
#define GETTER_HEAD     "get_"
        uint_t size = strlen(GETTER_HEAD) + strlen(name) + 1;
        char* getter_name = (char*)YogSysdeps_alloca(sizeof(char) * size);
        YogSysdeps_snprintf(getter_name, size, GETTER_HEAD "%s", name);
        getter = YogNativeFunction_new(env, class_name, pkg, getter_name, get);
#undef GETTER_HEAD
    }

    if (set != NULL) {
#define SETTER_HEAD     "set_"
        uint_t size = strlen(SETTER_HEAD) + strlen(name) + 1;
        char* setter_name = (char*)YogSysdeps_alloca(sizeof(char) * size);
        YogSysdeps_snprintf(setter_name, size, SETTER_HEAD "%s", name);
        setter = YogNativeFunction_new(env, class_name, pkg, setter_name, set);
#undef setter_HEAD
    }

    prop = YogProperty_new(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogProperty, prop), getter, getter);
    YogGC_UPDATE_PTR(env, PTR_AS(YogProperty, prop), setter, setter);

    YogObj_set_attr(env, self, name, prop);

    RETURN_VOID(env);
}

struct ModuleClass {
    struct YogObj base;
    YogVal super;
    Allocator allocator;
};

typedef struct ModuleClass ModuleClass;

DECL_AS_TYPE(ModuleClass_new);
#define TYPE_MODULE_CLASS TO_TYPE(ModuleClass_new)

static void
ModuleClass_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogObj_keep_children(env, ptr, keeper, heap);

    ModuleClass* klass = PTR_AS(ModuleClass, ptr);
#define KEEP(member)   YogGC_KEEP(env, klass, member, keeper, heap)
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
    YogObj_init(env, klass, TYPE_MODULE_CLASS, 0, YUNDEF);
    PTR_AS(ModuleClass, klass)->super = YUNDEF;
    PTR_AS(ModuleClass, klass)->allocator = NULL;

    RETURN(env, klass);
}

void
YogClass_include_module(YogEnv* env, YogVal self, YogVal module)
{
    SAVE_ARGS2(env, self, module);
    YogVal module_class = YUNDEF;
    PUSH_LOCAL(env, module_class);

    CHECK_SELF_TYPE(env, self);

    module_class = ModuleClass_new(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogObj, module_class), attrs, PTR_AS(YogObj, module)->attrs);
    YogGC_UPDATE_PTR(env, PTR_AS(ModuleClass, module_class), super, PTR_AS(YogClass, self)->super);
    YogGC_UPDATE_PTR(env, PTR_AS(YogClass, self), super, module_class);

    RETURN_VOID(env);
}

void
YogClass_boot(YogEnv* env, YogVal cClass, YogVal pkg)
{
    SAVE_ARGS2(env, cClass, pkg);

    YogClass_define_get_attr_caller(env, cClass, call_get_attr);
    YogClass_define_get_attr_executor(env, cClass, exec_get_attr);

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
