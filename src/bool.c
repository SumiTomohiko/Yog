#include "yog/error.h"
#include "yog/frame.h"
#include "yog/klass.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_TYPE(v)   do { \
    YOG_ASSERT(env, IS_BOOL(v), "value isn't bool."); \
} while (0)

static YogVal 
to_s(YogEnv* env)
{
    YogVal self = SELF(env);
    CHECK_TYPE(self);

    const char* s = NULL;
    if (VAL2BOOL(self)) {
        s = "true";
    }
    else {
        s = "false";
    }

    return YogString_new_str(env, s);
}

YogVal 
YogBool_klass_new(YogEnv* env) 
{
    YogVal klass = YogKlass_new(env, "Bool", env->vm->cObject);
    PUSH_LOCAL(env, klass);

    YogKlass_define_method(env, klass, "to_s", to_s, 0, 0, 0, 0, NULL);

    POP_LOCALS(env);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
