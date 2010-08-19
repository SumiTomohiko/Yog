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

#if WINDOWS
#   define SEP '\\'
#else
#   define SEP '/'
#endif

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
    SAVE_ARG(env, obj);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);
    if (IS_PTR(obj) && (BASIC_OBJ_TYPE(obj) == TYPE_STRING)) {
        RETURN(env, obj);
    }

#define METHOD_NAME     "to_s"
    s = YogEval_call_method0(env, obj, METHOD_NAME);
    YOG_ASSERT(env, !IS_UNDEF(s), "to_s returned undef");
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
mkdir_(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal path = YUNDEF;
    PUSH_LOCAL(env, path);
    YogCArg params[] = { { "path", &path }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "mkdir", params, args, kw);
    if (!IS_PTR(path) || (BASIC_OBJ_TYPE(path) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "path must be String");
    }

    if (YogSysdeps_mkdir(STRING_CSTR(path)) != 0) {
        YogError_raise_sys_err(env, errno, path);
    }

    RETURN(env, YNIL);
}

static YogVal
load_lib(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal path = YUNDEF;
    YogVal lib = YUNDEF;
    PUSH_LOCALS2(env, path, lib);
    YogCArg params[] = { { "path", &path }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "load_lib", params, args, kw);
    if (!IS_PTR(path) || (BASIC_OBJ_TYPE(path) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "path must be String, not %C", path);
    }

    lib = YogFFI_load_lib(env, path);

    RETURN(env, lib);
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

    path = YogString_of_encoding(env, STRING_ENCODING(head));
    YogString_append(env, path, head);
    YogString_append(env, path, env->vm->path_separator);
    YogString_append(env, path, tail);

    RETURN(env, path);
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
argv2args(YogEnv* env, uint_t argc, char** argv)
{
    SAVE_LOCALS(env);
    YogVal args = YUNDEF;
    YogVal s = YUNDEF;
    YogVal enc = YUNDEF;
    PUSH_LOCALS3(env, args, s, enc);

    enc = YogEncoding_get_default(env);
    args = YogArray_new(env);
    uint_t i;
    for (i = 0; i < argc; i++) {
        s = YogString_from_str(env, argv[i]);
        STRING_ENCODING(s) = enc;
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

static const char*
get_path()
{
    const char* path = getenv("PATH");
    if (path == NULL) {
        return NULL;
    }
    return strchr(path, '=');
}

static const char*
find_path_end(const char* path)
{
    const char* pc = strchr(path, ':');
    if (pc != NULL) {
        return pc;
    }
    return path + strlen(path);
}

static BOOL
is_executable(const char* path)
{
    struct stat buf;
    if (stat(path, &buf) != 0) {
        return FALSE;
    }
    if (!S_ISREG(buf.st_mode)) {
        return FALSE;
    }
    if ((buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) {
        return FALSE;
    }
    return TRUE;
}

static YogVal
find_exe(YogEnv* env, const char* exe)
{
    if (strchr(exe, SEP) != NULL) {
        return YogString_from_str(env, exe);
    }
    const char* path = get_path();
    if (path == NULL) {
        return YogString_from_str(env, exe);
    }
    const char* begin = path;
    const char* end = path + strlen(path);
    while (begin < end) {
        const char* pc = find_path_end(begin);
        uint_t size = pc - begin;
        char dir[size + 1];
        memcpy(dir, begin, size);
        dir[size] = '\0';

        uint_t len = strlen(exe) + size;
        char path[len + 1];
        snprintf(path, len + 1, "%s%c%s", dir, SEP, exe);
        if (is_executable(path)) {
            return YogString_from_str(env, path);
        }

        begin = pc + 1;
    }

    return YNIL;
}

static YogVal
absolutize(YogEnv* env, YogVal path)
{
    if (!IS_PTR(path) || (STRING_CSTR(path)[0] == SEP)) {
        return path;
    }
    char cwd[1024]; /* TODO: enouph? */
    getcwd(cwd, array_sizeof(cwd));
    char abs_path[2048];
    snprintf(abs_path, array_sizeof(abs_path), "%s%c%s", cwd, SEP, STRING_CSTR(path));
    return YogString_from_str(env, abs_path);
}

static void
set_executable(YogEnv* env, YogHandle* builtins, const char* exe)
{
    YogVal path = find_exe(env, exe);
    YogVal s = absolutize(env, path);
    YogObj_set_attr(env, HDL2VAL(builtins), "EXECUTABLE", s);
}

static void
set_path_separator(YogEnv* env, YogVal builtins)
{
    SAVE_ARG(env, builtins);
    YogVal s = YUNDEF;
    YogVal enc = YUNDEF;
    PUSH_LOCALS2(env, s, enc);

    s = YogSprintf_sprintf(env, "%c", SEP);
    enc = YogEncoding_get_default(env);
    STRING_ENCODING(s) = enc;

    YogObj_set_attr(env, builtins, "PATH_SEPARATOR", s);
    env->vm->path_separator = s;

    RETURN_VOID(env);
}

void
YogBuiltins_boot(YogEnv* env, YogVal builtins, const char* exe, uint_t argc, char** argv)
{
    SAVE_ARG(env, builtins);
    YogVal args = YUNDEF;
    YogVal errno_ = YUNDEF;
    YogVal e = YUNDEF;
    PUSH_LOCALS3(env, args, errno_, e);

#define DEFINE_FUNCTION(name, f)    do { \
    YogPackage_define_function(env, builtins, name, f); \
} while (0)
    DEFINE_FUNCTION("classmethod", classmethod);
    DEFINE_FUNCTION("disable_gc_stress", disable_gc_stress);
    DEFINE_FUNCTION("enable_gc_stress", enable_gc_stress);
    DEFINE_FUNCTION("get_current_thread", get_current_thread);
    DEFINE_FUNCTION("import_package", import_package);
    DEFINE_FUNCTION("include_module", include_module);
    DEFINE_FUNCTION("join_path", join_path);
    DEFINE_FUNCTION("load_lib", load_lib);
    DEFINE_FUNCTION("mkdir", mkdir_);
    DEFINE_FUNCTION("print", print);
    DEFINE_FUNCTION("property", property);
    DEFINE_FUNCTION("puts", puts_);
    DEFINE_FUNCTION("raise_exception", raise_exception);
#undef DEFINE_FUNCTION

#define REGISTER_CLASS(c)   do { \
    YogVal klass = env->vm->c; \
    YogObj_set_attr_id(env, builtins, PTR_AS(YogClass, klass)->name, klass); \
} while (0)
    REGISTER_CLASS(cArray);
    REGISTER_CLASS(cBinary);
    REGISTER_CLASS(cBuffer);
    REGISTER_CLASS(cCoroutine);
    REGISTER_CLASS(cDict);
    REGISTER_CLASS(cFile);
    REGISTER_CLASS(cInt);
    REGISTER_CLASS(cObject);
    REGISTER_CLASS(cPointer);
    REGISTER_CLASS(cProcess);
    REGISTER_CLASS(cRegexp);
    REGISTER_CLASS(cSet);
    REGISTER_CLASS(cString);
    REGISTER_CLASS(cStructClass);
    REGISTER_CLASS(cThread);
    REGISTER_CLASS(eAttributeError);
    REGISTER_CLASS(eException);
    REGISTER_CLASS(eImportError);
    REGISTER_CLASS(eIndexError);
    REGISTER_CLASS(eKeyError);
    REGISTER_CLASS(eSyntaxError);
    REGISTER_CLASS(eSystemError);
    REGISTER_CLASS(eValueError);
    REGISTER_CLASS(eWindowsError);
#undef REGISTER_CLASS

    args = argv2args(env, argc, argv);
    YogObj_set_attr(env, builtins, "ARGV",  args);
    e = YogEnv_new(env);
    YogObj_set_attr(env, builtins, "ENV", e);
    YogObj_set_attr(env, builtins, "ENCODINGS", env->vm->encodings);

    set_path_separator(env, builtins);
    YogHandle* h = YogHandle_REGISTER(env, builtins);
    set_executable(env, h, exe);

#if !defined(MINIYOG)
#   define REGISTER_ERRNO(e)    do { \
    errno_ = YogVal_from_int(env, (e)); \
    YogObj_set_attr(env, builtins, #e, errno_); \
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
