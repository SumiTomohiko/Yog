#include "yog/config.h"
#include <stdlib.h>
#include "yog/class.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/get_args.h"
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
check_self(YogEnv* env, YogVal self)
{
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_ENV)) {
        YogError_raise_TypeError(env, "self must be Env, not %C", self);
    }
}

static YogVal
get(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    YogVal key = YUNDEF;
    YogVal default_ = YNIL;
    PUSH_LOCALS3(env, s, key, default_);
    YogCArg params[] = {
        { "key", &key },
        { "|", NULL },
        { "default", &default_ },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "get", params, args, kw);
    check_self(env, self);
    if (!IS_PTR(key) || (BASIC_OBJ_TYPE(key) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "key must be String, not %C", key);
    }

    const char* val = getenv(STRING_CSTR(key));
    if (val == NULL) {
        RETURN(env, default_);
    }
    s = YogString_from_str(env, val);

    RETURN(env, s);
}

static YogVal
subscript_assign(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal key = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS2(env, key, val);
    YogCArg params[] = { { "key", &key }, { "value", &val }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "[]=", params, args, kw);
    check_self(env, self);
    if (!IS_PTR(key) || (BASIC_OBJ_TYPE(key) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "key must be String, not %C", key);
    }
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "value must be String, not %C", val);
    }

    setenv(STRING_CSTR(key), STRING_CSTR(val), 1);

    RETURN(env, val);
}

static YogVal
subscript(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    YogVal key = YUNDEF;
    PUSH_LOCALS2(env, s, key);
    YogCArg params[] = { { "key", &key }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "[]", params, args, kw);
    check_self(env, self);
    if (!IS_PTR(key) || (BASIC_OBJ_TYPE(key) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "key must be String, not %C", key);
    }

    const char* val = getenv(STRING_CSTR(key));
    if (val == NULL) {
        YogError_raise_KeyError(env, "%S", key);
    }
    s = YogString_from_str(env, val);

    RETURN(env, s);
}

void
YogEnv_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cEnv = YUNDEF;
    PUSH_LOCAL(env, cEnv);
    YogVM* vm = env->vm;

    cEnv = YogClass_new(env, "Env", vm->cObject);
    YogClass_define_method(env, cEnv, pkg, "[]", subscript);
    YogClass_define_method(env, cEnv, pkg, "[]=", subscript_assign);
    YogClass_define_method(env, cEnv, pkg, "get", get);
    vm->cEnv = cEnv;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
