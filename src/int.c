#include "yog/yog.h"

static YogVal 
add(YogEnv* env, YogVal receiver, int argc, YogVal* args) 
{
    Yog_assert(env, argc == 1, "");
    unsigned int i = 0;
    for (i = 0; i < argc; i++) {
        Yog_assert(env, args[i].type == VAL_INT, "");
    }

    int result = YOGVAL_INT(args[0]) + YOGVAL_INT(args[1]);

    return YogVal_int(result);
}

YogKlass* 
YogInt_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, ENV_VM(env)->obj_klass);
    YogObj* obj = YOGOBJ(klass);
    YogObj_define_method(env, obj, "+", add);

    return klass;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
