#if defined(HAVE_DLFCN_H)
#   include <dlfcn.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "yog/array.h"
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/classmethod.h"
#include "yog/compile.h"
#include "yog/encoding.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/eval.h"
#include "yog/ffi.h"
#include "yog/frame.h"
#include "yog/get_args.h"
#include "yog/misc.h"
#include "yog/module.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/parser.h"
#include "yog/property.h"
#include "yog/sprintf.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
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

    YogEval_pop_frame(env);
    YogError_raise(env, exc);
    /* NOTREACHED */
    RETURN(env, YNIL);
}

static YogVal
repr_as_str(YogEnv* env, YogVal obj)
{
    if (IS_PTR(obj) && (BASIC_OBJ_TYPE(obj) == TYPE_STRING)) {
        return obj;
    }
    YogHandle* h = VAL2HDL(env, obj);

#define METHOD_NAME "to_s"
    YogVal s = YogEval_call_method0(env, obj, METHOD_NAME);
    YOG_ASSERT(env, !IS_UNDEF(s), "%s returned undef", METHOD_NAME);
    if (IS_PTR(s) && (BASIC_OBJ_TYPE(s) == TYPE_STRING)) {
        return s;
    }
    const char* fmt = "%C#%s() returned non-string (%C)";
    YogError_raise_TypeError(env, fmt, HDL2VAL(h), METHOD_NAME, s);
#undef METHOD_NAME
    /* NOTREACHED */

    return YUNDEF;
}

static void
print_object(YogEnv* env, YogVal obj)
{
    YogHandle* s = YogHandle_REGISTER(env, repr_as_str(env, obj));
    printf("%s", BINARY_CSTR(YogString_to_bin_in_default_encoding(env, s)));
}

static YogVal
mkdir_(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* path)
{
    YogVal obj = HDL2VAL(path);
    if (!IS_PTR(obj) || (BASIC_OBJ_TYPE(obj) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "path must be String");
    }

    YogVal s = YogString_to_bin_in_default_encoding(env, path);
    if (YogSysdeps_mkdir(BINARY_CSTR(s)) != 0) {
        YogError_raise_sys_err(env, errno, HDL2VAL(path));
    }

    return YNIL;
}

static YogVal
load_lib(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* path)
{
    return YogFFI_load_lib(env, path);
}

static YogVal
join_path(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal head = YUNDEF;
    YogVal tail = YUNDEF;
    YogVal path = YUNDEF;
    PUSH_LOCALS3(env, head, tail, path);
    YogCArg params[] = { { "head", &head }, { "tail", &tail }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "join_path", params, args, kw);
#define CHECK_TYPE(name, s)   do { \
    if (!IS_PTR(s) || (BASIC_OBJ_TYPE(s) != TYPE_STRING)) { \
        YogError_raise_TypeError(env, "%s must be String", name); \
    } \
} while (0)
    CHECK_TYPE("head", head);
    CHECK_TYPE("tail", tail);
#undef CHECK_TYPE

    path = YogString_new(env);
    YogString_append(env, path, head);
    YogString_append(env, path, env->vm->path_separator);
    YogString_append(env, path, tail);

    RETURN(env, path);
}

static YogVal
minor_gc(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
#if defined(GC_GENERATIONAL)
#   define GC_PROC(env) YogGC_perform_minor((env))
#elif !defined(GC_BDW)
#   define GC_PROC(env) YogGC_perform((env))
#else
#   define GC_PROC(env)
#endif
    GC_PROC(env);
#undef GC_PROC
    return YNIL;
}

static YogVal
major_gc(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
#if defined(GC_GENERATIONAL)
    YogGC_perform_minor(env);
    return YNIL;
#else
    return minor_gc(env, self, pkg);
#endif
}

static YogVal
print(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* args)
{
    uint_t size = YogArray_size(env, HDL2VAL(args));
    uint_t i;
    for (i = 0; i < size; i++) {
        print_object(env, YogArray_at(env, HDL2VAL(args), i));
    }
    return YNIL;
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
import_package(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* name)
{
    YogMisc_check_String(env, name, "Package name");
    return HDL2VAL(YogVM_import_package(env, env->vm, name));
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
    YogGC_UPDATE_PTR(env, PTR_AS(YogProperty, prop), getter, getter);
    YogGC_UPDATE_PTR(env, PTR_AS(YogProperty, prop), setter, setter);

    RETURN(env, prop);
}

static YogVal
disable_gc_stress(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "disable_gc_stress", params, args, kw);
    YogVM_disable_gc_stress(env, env->vm);
    RETURN(env, YNIL);
}

static YogVal
enable_gc_stress(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "enable_gc_stress", params, args, kw);
    YogVM_enable_gc_stress(env, env->vm);
    RETURN(env, YNIL);
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
    YogGC_UPDATE_PTR(env, PTR_AS(YogClassMethod, method), f, f);

    RETURN(env, method);
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

static void
set_path_separator(YogEnv* env, YogHandle* builtins)
{
    YogVal sep = YogSprintf_sprintf(env, "%c", PATH_SEPARATOR);
    YogVal path = YogString_to_path(env, sep);
    env->vm->path_separator = path;
    YogObj_set_attr(env, HDL2VAL(builtins), "PATH_SEPARATOR", path);
}

void
YogBuiltins_boot(YogEnv* env, YogHandle* builtins)
{
    SAVE_LOCALS(env);
    YogVal errno_ = YUNDEF;
    YogVal e = YUNDEF;
    PUSH_LOCALS2(env, errno_, e);

#define DEFINE_FUNCTION(name, f)    do { \
    YogPackage_define_function(env, HDL2VAL(builtins), name, f); \
} while (0)
    DEFINE_FUNCTION("classmethod", classmethod);
    DEFINE_FUNCTION("disable_gc_stress", disable_gc_stress);
    DEFINE_FUNCTION("enable_gc_stress", enable_gc_stress);
    DEFINE_FUNCTION("get_current_thread", get_current_thread);
    DEFINE_FUNCTION("include_module", include_module);
    DEFINE_FUNCTION("join_path", join_path);
    DEFINE_FUNCTION("property", property);
    DEFINE_FUNCTION("puts", puts_);
    DEFINE_FUNCTION("raise_exception", raise_exception);
#undef DEFINE_FUNCTION
#define DEFINE_FUNCTION2(name, f, ...) do { \
    YogPackage_define_function2(env, builtins, (name), (f), __VA_ARGS__); \
} while (0)
    DEFINE_FUNCTION2("import_package", import_package, "name", NULL);
    DEFINE_FUNCTION2("load_lib", load_lib, "path", NULL);
    DEFINE_FUNCTION2("major_gc", major_gc, NULL);
    DEFINE_FUNCTION2("minor_gc", minor_gc, NULL);
    DEFINE_FUNCTION2("mkdir", mkdir_, "path", NULL);
    DEFINE_FUNCTION2("print", print, "*", NULL);
#undef DEFINE_FUNCTION2

#define REGISTER_CLASS(c)   do { \
    YogVal klass = env->vm->c; \
    ID name = PTR_AS(YogClass, klass)->name; \
    YogObj_set_attr_id(env, HDL2VAL(builtins), name, klass); \
} while (0)
    REGISTER_CLASS(cArray);
    REGISTER_CLASS(cBinary);
    REGISTER_CLASS(cBuffer);
    REGISTER_CLASS(cCoroutine);
    REGISTER_CLASS(cDatetime);
    REGISTER_CLASS(cDict);
    REGISTER_CLASS(cFile);
    REGISTER_CLASS(cInt);
    REGISTER_CLASS(cObject);
    REGISTER_CLASS(cPath);
    REGISTER_CLASS(cPointer);
    REGISTER_CLASS(cProcess);
    REGISTER_CLASS(cRegexp);
    REGISTER_CLASS(cSet);
    REGISTER_CLASS(cString);
    REGISTER_CLASS(cStructClass);
    REGISTER_CLASS(cSymbol);
    REGISTER_CLASS(cThread);
    REGISTER_CLASS(cUnionClass);
    REGISTER_CLASS(eAttributeError);
    REGISTER_CLASS(eException);
    REGISTER_CLASS(eImportError);
    REGISTER_CLASS(eIndexError);
    REGISTER_CLASS(eKeyError);
    REGISTER_CLASS(eNotImplementedError);
    REGISTER_CLASS(eSyntaxError);
    REGISTER_CLASS(eSystemError);
    REGISTER_CLASS(eValueError);
    REGISTER_CLASS(eWindowsError);
#undef REGISTER_CLASS
#define REGISTER_MODULE(m) do { \
    YogVal mod = env->vm->m; \
    ID name = PTR_AS(YogModule, mod)->name; \
    YogObj_set_attr_id(env, HDL2VAL(builtins), name, mod); \
} while (0)
    REGISTER_MODULE(mComparable);
#undef REGISTER_MODULE

    e = YogEnv_new(env);
    YogObj_set_attr(env, HDL2VAL(builtins), "ENV", e);
    YogObj_set_attr(env, HDL2VAL(builtins), "ENCODINGS", env->vm->encodings);
    YogVal enc = env->vm->default_encoding;
    YogObj_set_attr(env, HDL2VAL(builtins), "DEFAULT_ENCODING", enc);

    set_path_separator(env, builtins);

#if !defined(MINIYOG)
#   define REGISTER_ERRNO(e)    do { \
    errno_ = YogVal_from_int(env, (e)); \
    YogObj_set_attr(env, HDL2VAL(builtins), #e, errno_); \
} while (0)
#   include "errno.inc"
#   undef REGISTER_ERRNO

    const char* src = 
#   include "builtins.inc"
    ;
    YogMisc_eval_source(env, builtins, src);
    const char* sysdeps_src =
#   if WINDOWS
#       include "builtins_win.inc"
#   else
#       include "builtins_unix.inc"
#   endif
    ;
    YogMisc_eval_source(env, builtins, sysdeps_src);
#endif

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
