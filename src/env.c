#include "yog/config.h"
#include <stdlib.h>
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Env {
    struct YogBasicObj base;
};

typedef struct Env Env;

#define TYPE_ENV TO_TYPE(YogEnv_new)

YogVal
YogEnv_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal e = YUNDEF;
    PUSH_LOCAL(env, e);

    e = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Env);
    YogBasicObj_init(env, e, TYPE_ENV, 0, env->vm->cEnv);

    RETURN(env, e);
}

static void
check_self(YogEnv* env, YogHandle* self)
{
    YOG_ASSERT(env, self != NULL, "self is undef");
    if (!IS_PTR(HDL2VAL(self)) || (BASIC_OBJ_TYPE(HDL2VAL(self)) != TYPE_ENV)) {
        YogError_raise_TypeError(env, "self must be Env, not %C", self);
    }
}

static const char*
getenv_(YogEnv* env, YogHandle* key)
{
    return getenv(BINARY_CSTR(YogString_to_bin_in_default_encoding(env, key)));
}

static YogVal
get(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* key, YogHandle* default_)
{
    check_self(env, self);
    if (!IS_PTR(HDL2VAL(key)) || (BASIC_OBJ_TYPE(HDL2VAL(key)) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "key must be String, not %C", key);
    }

    const char* val = getenv_(env, key);
    if (val == NULL) {
        return HDL2VAL(default_);
    }
    return YogString_from_string(env, val);
}

static YogVal
subscript_assign(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* key, YogHandle* value)
{
    check_self(env, self);
    YogMisc_check_string(env, key, "key");
    YogMisc_check_string(env, value, "value");

    YogHandle* k = VAL2HDL(env, YogString_to_bin_in_default_encoding(env, key));
    YogVal v = YogString_to_bin_in_default_encoding(env, value);
    setenv(BINARY_CSTR(HDL2VAL(k)), BINARY_CSTR(v), 1);

    return HDL2VAL(value);
}

static YogVal
subscript(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* key)
{
    check_self(env, self);
    if (!IS_PTR(HDL2VAL(key)) || (BASIC_OBJ_TYPE(HDL2VAL(key)) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "key must be String, not %C", key);
    }

    const char* val = getenv_(env, key);
    if (val == NULL) {
        YogError_raise_KeyError(env, "%S", key);
    }
    return YogString_from_string(env, val);
}

void
YogEnv_define_classes(YogEnv* env, YogHandle* pkg)
{
    YogVM* vm = env->vm;

    YogHandle* cEnv = VAL2HDL(env, YogClass_new(env, "Env", vm->cObject));
#define DEFINE_METHOD(name, f, ...) do { \
    YogClass_define_method2(env, HDL2VAL(cEnv), HDL2VAL(pkg), (name), (f), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD("[]", subscript, "key", NULL);
    DEFINE_METHOD("[]=", subscript_assign, "key", "value", NULL);
    DEFINE_METHOD("get", get, "key", "|", "default", NULL);
#undef DEFINE_METHOD
    vm->cEnv = HDL2VAL(cEnv);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
