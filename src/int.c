#include "yog/array.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/klass.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_TYPE(v) do { \
    YOG_ASSERT(env, IS_INT(v), "Value isn't int."); \
} while (0)

#define CHECK_ARGS(self, v) do { \
    CHECK_TYPE(self); \
    CHECK_TYPE(v); \
} while (0)

static YogVal 
to_s(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    CHECK_TYPE(self);
    YogVal retval = YogString_new_format(env, "%d", VAL2INT(self));
    RETURN(env, retval);
}

static YogVal 
add(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal n = YogArray_at(env, args, 0);

    CHECK_ARGS(self, n);

    int result = VAL2INT(self) + VAL2INT(n);

    RETURN(env, INT2VAL(result));
}

static YogVal 
less(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal n = YogArray_at(env, args, 0);

    CHECK_ARGS(self, n);

    YogVal retval;
    if (VAL2INT(self) < VAL2INT(n)) {
        retval = YTRUE;
    }
    else {
        retval = YFALSE;
    }
    RETURN(env, retval);
}

static YogVal 
times(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    int n = VAL2INT(self);

    unsigned int i = 0;
    unsigned int argc = 1;
    for (i = 0; i < n; i++) {
        YogVal args[argc];
        args[0] = INT2VAL(i);
        YogEval_call_block(env, block, argc, args);
    }

    RETURN(env, YNIL);
}

YogVal 
YogInt_klass_new(YogEnv* env) 
{
    YogVal klass = YogKlass_new(env, "Int", env->vm->cObject);
    PUSH_LOCAL(env, klass);
#define DEFINE_METHOD(name, f) do { \
    YogKlass_define_method(env, klass, name, f, 0, 0, 0, -1, "n", NULL); \
} while (0)
    DEFINE_METHOD("+", add);
    DEFINE_METHOD("<", less);
#undef DEFINE_METHOD
    YogKlass_define_method(env, klass, "to_s", to_s, 0, 0, 0, 0, NULL);
    YogKlass_define_method(env, klass, "times", times, 1, 0, 0, 0, "block", NULL);

    POP_LOCALS(env);
    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
