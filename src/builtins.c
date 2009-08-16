#include <stdio.h>
#include <string.h>
#include "yog/array.h"
#include "yog/classmethod.h"
#include "yog/compile.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/parser.h"
#include "yog/property.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal
raise(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal exc = YogArray_at(env, args, 0);

    if (!YogVal_is_subklass_of(env, exc, env->vm->eException)) {
        YogVal receiver = env->vm->eException;
        YogVal args[] = { exc };
        PUSH_LOCALSX(env, array_sizeof(args), args);
        exc = YogEval_call_method(env, receiver, "new", 1, args);
        POP_LOCALS(env);
    }

    YogVal cur_frame = PTR_AS(YogThread, env->thread)->cur_frame;
    YogVal prev_frame = PTR_AS(YogFrame, cur_frame)->prev;
    PTR_AS(YogThread, env->thread)->cur_frame = prev_frame;

    YogError_raise(env, exc);

    /* NOTREACHED */
    RETURN(env, YNIL);
}

static void
print_object(YogEnv* env, YogVal obj)
{
    SAVE_ARG(env, obj);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    if (IS_PTR(obj) && IS_OBJ_OF(env, obj, cString)) {
        s = obj;
    }
    else {
        s = YogEval_call_method(env, obj, "to_s", 0, NULL);
        YOG_ASSERT(env, IS_PTR(s), "invalid string");
        YOG_ASSERT(env, IS_OBJ_OF(env, s, cString), "invalid object");
    }
    printf("%s", STRING_CSTR(s));

    RETURN_VOID(env);
}

static YogVal
print(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    uint_t size = YogArray_size(env, args);
    uint_t i;
    for (i = 0; i < size; i++) {
        obj = YogArray_at(env, args, i);
        print_object(env, obj);
    }

    RETURN(env, YNIL);
}

static YogVal
puts_(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    uint_t size = YogArray_size(env, args);
    if (size == 0) {
        printf("\n");
        RETURN(env, YNIL);
    }

    uint_t i;
    for (i = 0; i < size; i++) {
        obj = YogArray_at(env, args, i);
        print_object(env, obj);
        printf("\n");
    }

    RETURN(env, YNIL);
}

static YogVal
import_package(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogVal name = YogArray_at(env, args, 0);
    return YogVM_import_package(env, env->vm, VAL2ID(name));
}

static YogVal
property(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal getter = YUNDEF;
    YogVal setter = YUNDEF;
    YogVal prop = YUNDEF;
    PUSH_LOCALS3(env, getter, setter, prop);

    getter = YogArray_at(env, args, 0);
    if (1 < YogArray_size(env, args)) {
        setter = YogArray_at(env, args, 1);
    }

    prop = YogProperty_new(env);
    PTR_AS(YogProperty, prop)->getter = getter;
    PTR_AS(YogProperty, prop)->setter = setter;

    RETURN(env, prop);
}

static YogVal
classmethod(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal f = YUNDEF;
    YogVal method = YUNDEF;
    PUSH_LOCALS2(env, f, method);

    f = YogArray_at(env, args, 0);

    method = YogClassMethod_new(env);
    PTR_AS(YogClassMethod, method)->f = f;

    RETURN(env, method);
}

#if !defined(MINIYOG)
static char* builtins_src = 
#   include "builtins.inc"
;
#endif

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
        s = YogString_new_str(env, argv[i]);
        YogArray_push(env, args, s);
    }

    RETURN(env, args);
}

static YogVal
include_module(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal klass = YUNDEF;
    YogVal module = YUNDEF;
    PUSH_LOCALS2(env, klass, module);

    klass = YogArray_at(env, args, 0);
    module = YogArray_at(env, args, 1);
    YogKlass_include_module(env, klass, module);

    RETURN(env, klass);
}

YogVal
YogBuiltins_new(YogEnv* env, uint_t argc, char** argv)
{
    SAVE_LOCALS(env);
    YogVal builtins = YUNDEF;
    YogVal src = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    YogVal args = YUNDEF;
    PUSH_LOCALS5(env, builtins, src, code, stmts, args);

    builtins = YogPackage_new(env);

    YogPackage_define_function(env, builtins, "classmethod", classmethod);
    YogPackage_define_function(env, builtins, "import_package", import_package);
    YogPackage_define_function(env, builtins, "include_module", include_module);
    YogPackage_define_function(env, builtins, "print", print);
    YogPackage_define_function(env, builtins, "property", property);
    YogPackage_define_function(env, builtins, "puts", puts_);
    YogPackage_define_function(env, builtins, "raise", raise);

#define REGISTER_KLASS(c)   do { \
    YogVal klass = env->vm->c; \
    YogObj_set_attr_id(env, builtins, PTR_AS(YogKlass, klass)->name, klass); \
} while (0)
    REGISTER_KLASS(cDict);
    REGISTER_KLASS(cFile);
    REGISTER_KLASS(cObject);
    REGISTER_KLASS(cThread);
    REGISTER_KLASS(eException);
#undef REGISTER_KLASS

    args = argv2args(env, argc, argv);
    YogObj_set_attr(env, builtins, "ARGV",  args);

#if !defined(MINIYOG)
    src = YogString_new_str(env, builtins_src);
    stmts = YogParser_parse(env, src);
    code = YogCompiler_compile_package(env, "builtin", stmts);
    YogEval_eval_package(env, builtins, code);
#endif

    RETURN(env, builtins);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
