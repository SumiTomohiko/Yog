#include "yog/config.h"
#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "yog/class.h"
#include "yog/error.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogHandle*
append_separator(YogEnv* env, YogHandle* self)
{
    YogHandle* path = VAL2HDL(env, YogString_to_path(env, HDL2VAL(self)));
    YogString_push(env, HDL2VAL(path), PATH_SEPARATOR);
    return path;
}

YogHandle*
YogPath_join2(YogEnv* env, YogHandle* self, YogHandle* name)
{
    YogHandle* path = append_separator(env, self);
    YogString_append(env, HDL2VAL(path), HDL2VAL(name));
    return path;
}

YogHandle*
YogPath_join(YogEnv* env, YogHandle* self, const char* name)
{
    YogHandle* path = append_separator(env, self);
    YogString_append_string(env, HDL2VAL(path), name);
    return path;
}

YogVal
YogPath_from_string(YogEnv* env, const char* path)
{
    YogVal s = YogString_from_string(env, path);
    YogString_change_class_to_path(env, s);
    return s;
}

static YogVal
YogPath_of_current_dir(YogEnv* env)
{
    return YogPath_from_string(env, ".");
}


static YogVal
YogPath_slice(YogEnv* env, YogHandle* self, uint_t pos, uint_t size)
{
    YogVal s = YogString_slice(env, self, pos, size);
    YogString_change_class_to_path(env, s);
    return s;
}

static YogVal
trim_trailing_separators(YogEnv* env, YogHandle* self)
{
    YogVal s = HDL2VAL(self);
    uint_t size = STRING_SIZE(s);
    uint_t i;
    for (i = size; (0 < i) && (STRING_CHARS(s)[i - 1] == PATH_SEPARATOR); i--) {
    }
    if (i == size) {
        return s;
    }
    return YogPath_slice(env, self, 0, i);
}

YogVal
YogPath_dirname(YogEnv* env, YogHandle* self)
{
    YogVal s = trim_trailing_separators(env, self);
    if (STRING_SIZE(s) == 0) {
        return env->vm->path_separator;
    }
    int_t pos = YogString_strrchr(env, s, PATH_SEPARATOR);
    if (pos < 0) {
        return YogPath_of_current_dir(env);
    }
    if (pos == 0) {
        return env->vm->path_separator;
    }
    return YogPath_slice(env, VAL2HDL(env, s), 0, pos);
}

YogVal
YogPath_getcwd(YogEnv* env)
{
    size_t size = 1;
    char* buf;
    char* dir;
    do {
        size *= 1024;
        buf = alloca(sizeof(char) * size);
    } while (((dir = getcwd(buf, size)) == NULL) && (errno == ERANGE));
    if (dir == NULL) {
        YogError_raise_sys_err(env, errno, YNIL);
        /* NOTREACHED */
    }
    return YogString_from_string(env, dir);
}

static YogVal
getcwd_(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    char* cwd = getcwd(NULL, 0);
    YogVal s = YogPath_from_string(env, cwd);
    free(cwd);
    return s;
}

static YogVal
get_root(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    return env->vm->path_separator;
}

#define CHECK_SELF_TYPE(env, self) do { \
    YogMisc_check_String((env), (self), "self"); \
} while (0)

static YogVal
get_dirname(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
    return YogPath_dirname(env, self);
}

static YogVal
YogPath_clone(YogEnv* env, YogVal self)
{
    YogVal s = YogString_clone(env, self);
    YogString_change_class_to_path(env, s);
    return s;
}

static YogVal
get_basename(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
    YogVal path = trim_trailing_separators(env, self);
    if (STRING_SIZE(path) == 0) {
        return env->vm->path_separator;
    }
    const char parent[] = "..";
    if ((STRING_SIZE(path) == strlen(parent)) && (STRING_CHARS(path)[0] == parent[0]) && (STRING_CHARS(path)[1] == parent[1])) {
        return YogPath_of_current_dir(env);
    }
    int_t pos = YogString_strrchr(env, path, PATH_SEPARATOR);
    if (pos < 0) {
        return YogPath_clone(env, path);
    }
    return YogPath_slice(env, self, pos + 1, STRING_SIZE(path) - pos - 1);
}

void
YogPath_eval_builtin_script(YogEnv* env, YogVal klass)
{
    YogMisc_eval_source(env, VAL2HDL(env, klass),
#include "path.inc"
    );
}

void
YogPath_define_classes(YogEnv* env, YogHandle* pkg)
{
    YogVM* vm = env->vm;
    YogHandle* cPath = VAL2HDL(env, YogClass_new(env, "Path", vm->cString));
    vm->cPath = HDL2VAL(cPath);

#define DEFINE_PROP(name, getter, setter) do { \
    YogClass_define_property2(env, cPath, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("dirname", get_dirname, NULL);
    DEFINE_PROP("basename", get_basename, NULL);
#undef DEFINE_PROP
#define DEFINE_CLASS_METHOD(name, ...) do { \
    YogClass_define_class_method2(env, cPath, pkg, (name), __VA_ARGS__); \
} while (0)
    DEFINE_CLASS_METHOD("get_root", get_root, NULL);
    DEFINE_CLASS_METHOD("getcwd", getcwd_, NULL);
#undef DEFINE_CLASS_METHOD
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
