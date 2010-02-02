#include <stdio.h>
#include <string.h>
#include "yog/array.h"
#include "yog/class.h"
#include "yog/classmethod.h"
#include "yog/compile.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/get_args.h"
#include "yog/misc.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/parser.h"
#include "yog/property.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
raise_exception(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal exc = YUNDEF;
    PUSH_LOCAL(env, exc);

    YogCArg params[] = { { "e", &exc }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "raise_exception", params, args, kw);

    if (!YogVal_is_subclass_of(env, exc, env->vm->eException)) {
        exc = YogEval_call_method1(env, env->vm->eException, "new", exc);
    }

    env->frame = PTR_AS(YogFrame, env->frame)->prev;

    YogError_raise(env, exc);

    /* NOTREACHED */
    RETURN(env, YNIL);
}

static YogVal
repr_as_str(YogEnv* env, YogVal obj)
{
    SAVE_ARG(env, obj);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);
    if (IS_PTR(obj) && (BASIC_OBJ_TYPE(obj) == TYPE_STRING)) {
        RETURN(env, obj);
    }

#define METHOD_NAME     "to_s"
    s = YogEval_call_method0(env, obj, METHOD_NAME);
    if (IS_PTR(s) && (BASIC_OBJ_TYPE(s) == TYPE_STRING)) {
        RETURN(env, s);
    }
    YogError_raise_TypeError(env, "%C#%s() returned non-string (%C)", obj, METHOD_NAME, s);
#undef METHOD_NAME

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static void
print_object(YogEnv* env, YogVal obj)
{
    SAVE_ARG(env, obj);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = repr_as_str(env, obj);
    printf("%s", STRING_CSTR(s));

    RETURN_VOID(env);
}

static YogVal
print(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    YogVal objs = YUNDEF;
    PUSH_LOCALS2(env, obj, objs);

    YogCArg params[] = { { "*", &objs }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "print", params, args, kw);

    uint_t size = YogArray_size(env, objs);
    uint_t i;
    for (i = 0; i < size; i++) {
        obj = YogArray_at(env, objs, i);
        print_object(env, obj);
    }

    RETURN(env, YNIL);
}

static YogVal
puts_(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    YogVal objs = YUNDEF;
    PUSH_LOCALS2(env, obj, objs);

    YogCArg params[] = { { "*", &objs }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "puts", params, args, kw);

    uint_t size = YogArray_size(env, objs);
    if (size == 0) {
        printf("\n");
        RETURN(env, YNIL);
    }

    uint_t i;
    for (i = 0; i < size; i++) {
        obj = YogArray_at(env, objs, i);
        print_object(env, obj);
        printf("\n");
    }

    RETURN(env, YNIL);
}

static YogVal
import_package(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal p = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS2(env, p, name);
    YogCArg params[] = { { "name", &name }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "import_package", params, args, kw);
    if (!IS_PTR(name) || (BASIC_OBJ_TYPE(name) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "package name must be String");
    }

    p = YogVM_import_package(env, env->vm, name);

    RETURN(env, p);
}

static YogVal
property(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal getter = YUNDEF;
    YogVal setter = YUNDEF;
    YogVal prop = YUNDEF;
    PUSH_LOCALS3(env, getter, setter, prop);

    YogCArg params[] = {
        { "getter", &getter },
        { "|", NULL },
        { "setter", &setter },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "property", params, args, kw);
    if (IS_UNDEF(setter)) {
        setter = YNIL;
    }

    prop = YogProperty_new(env);
    PTR_AS(YogProperty, prop)->getter = getter;
    PTR_AS(YogProperty, prop)->setter = setter;

    RETURN(env, prop);
}

static YogVal
classmethod(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal f = YUNDEF;
    YogVal method = YUNDEF;
    PUSH_LOCALS2(env, f, method);

    YogCArg params[] = { { "function", &f }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "classmethod", params, args, kw);

    method = YogClassMethod_new(env);
    PTR_AS(YogClassMethod, method)->f = f;

    RETURN(env, method);
}

static YogVal
argv2args(YogEnv* env, uint_t argc, char** argv)
{
    SAVE_LOCALS(env);
    YogVal args = YUNDEF;
    YogVal s = YUNDEF;
    PUSH_LOCALS2(env, args, s);

    args = YogArray_new(env);
    uint_t i;
    for (i = 0; i < argc; i++) {
        s = YogString_from_str(env, argv[i]);
        YogArray_push(env, args, s);
    }

    RETURN(env, args);
}

static YogVal
include_module(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal klass = YUNDEF;
    YogVal module = YUNDEF;
    PUSH_LOCALS2(env, klass, module);

    YogCArg params[] = {
        { "klass", &klass },
        { "mod", &module },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "include_module", params, args, kw);

    YogClass_include_module(env, klass, module);

    RETURN(env, klass);
}

static YogVal
get_current_thread(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal thread = env->thread;
    PUSH_LOCAL(env, thread);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_current_thread", params, args, kw);

    RETURN(env, thread);
}

void
YogBuiltins_boot(YogEnv* env, YogVal builtins, uint_t argc, char** argv)
{
    SAVE_ARG(env, builtins);
    YogVal args = YUNDEF;
    PUSH_LOCAL(env, args);

#define DEFINE_FUNCTION(name, f)    do { \
    YogPackage_define_function(env, builtins, name, f); \
} while (0)
    DEFINE_FUNCTION("classmethod", classmethod);
    DEFINE_FUNCTION("get_current_thread", get_current_thread);
    DEFINE_FUNCTION("import_package", import_package);
    DEFINE_FUNCTION("include_module", include_module);
    DEFINE_FUNCTION("print", print);
    DEFINE_FUNCTION("property", property);
    DEFINE_FUNCTION("puts", puts_);
    DEFINE_FUNCTION("raise_exception", raise_exception);
#undef DEFINE_FUNCTION

#define REGISTER_CLASS(c)   do { \
    YogVal klass = env->vm->c; \
    YogObj_set_attr_id(env, builtins, PTR_AS(YogClass, klass)->name, klass); \
} while (0)
    REGISTER_CLASS(cCoroutine);
    REGISTER_CLASS(cDict);
    REGISTER_CLASS(cFile);
    REGISTER_CLASS(cObject);
    REGISTER_CLASS(cRegexp);
    REGISTER_CLASS(cSet);
    REGISTER_CLASS(cString);
    REGISTER_CLASS(cThread);
    REGISTER_CLASS(eAttributeError);
    REGISTER_CLASS(eException);
    REGISTER_CLASS(eKeyError);
#undef REGISTER_CLASS

    args = argv2args(env, argc, argv);
    YogObj_set_attr(env, builtins, "ARGV",  args);

#if !defined(MINIYOG)
    const char* src = 
#   include "builtins.inc"
    ;
    YogMisc_eval_source(env, builtins, src);
#endif

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
