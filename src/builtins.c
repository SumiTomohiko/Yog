#include <stdio.h>
#include <string.h>
#include "yog/array.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

static YogVal 
raise(YogEnv* env)
{
    YogVal exc = ARG(env, 0);

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
    return YNIL;
}

static YogVal 
puts_(YogEnv* env)
{
    YogVal vararg = ARG(env, 0);
    PUSH_LOCAL(env, vararg);

    unsigned int size = YogArray_size(env, vararg);
    if (0 < size) {
        unsigned int i = 0;
        for (i = 0; i < size; i++) {
            YogString* s = NULL;
            YogVal arg = YogArray_at(env, vararg, i);
            if (IS_OBJ_OF(cString, arg)) {
                s = PTR_AS(YogString, arg);
            }
            else {
                YogVal val = YogEval_call_method(env, arg, "to_s", 0, NULL);
                s = VAL2PTR(val);
            }
            printf("%s", PTR_AS(YogCharArray, s->body)->items);
            printf("\n");
        }
    }
    else {
        printf("\n");
    }

    POP_LOCALS(env);
    return YNIL;
}

#include "yog/st.h"

static YogVal
import_package(YogEnv* env)
{
    SAVE_LOCALS(env);

    YogVal top = YUNDEF;
    YogVal parent = YUNDEF;
    PUSH_LOCALS2(env, top, parent);

    YogVM* vm = env->vm;

    YogVal name = ARG(env, 0);
    const char* s = YogVM_id2name(env, env->vm, VAL2ID(name));
    size_t len = strlen(s);
    char t[len + 1];
    strcpy(t, s);
    const char* begin = t;
    const char* pc = begin;
    while (1) {
        const char* end = strchr(pc, '.');
        if (end == NULL) {
            end = begin + len;
        }
        unsigned int size = end - begin;
        char s[size + 1];
        strncpy(s, begin, size);
        s[size] = '\0';

        ID id = YogVM_intern(env, vm, s);
        YogVal pkg = YogVM_import_package(env, vm, id);
        if (IS_PTR(parent)) {
            unsigned int size = end - pc;
            char attr[size + 1];
            strncpy(attr, pc, size);
            attr[size] = '\0';
            ID id = YogVM_intern(env, vm, attr);
            YogObj_set_attr_id(env, parent, id, pkg);
        }
        if (!IS_PTR(top)) {
            top = pkg;
        }

        if (end == begin + len) {
            break;
        }

        parent = pkg;
        pc = end + 1;
    }

    RETURN(env, top);
}

YogVal 
YogBuiltins_new(YogEnv* env) 
{
    YogVal bltins = YogPackage_new(env);
    PUSH_LOCAL(env, bltins);

    YogPackage_define_method(env, bltins, "puts", puts_, 0, 1, 0, 0, NULL);
    YogPackage_define_method(env, bltins, "raise", raise, 0, 0, 0, 0, "exc", NULL);
    YogPackage_define_method(env, bltins, "import_package", import_package, 0, 0, 0, 0, "package", NULL);

#define REGISTER_KLASS(c)   do { \
    YogVal klass = env->vm->c; \
    YogObj_set_attr_id(env, bltins, PTR_AS(YogKlass, klass)->name, klass); \
} while (0)
    REGISTER_KLASS(cObject);
    REGISTER_KLASS(cThread);
    REGISTER_KLASS(eException);
#undef REGISTER_KLASS

    POP_LOCALS(env);
    return bltins;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
