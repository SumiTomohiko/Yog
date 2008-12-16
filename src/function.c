#include <stdarg.h>
#include "yog/arg.h"
#include "yog/error.h"
#include "yog/function.h"
#include "yog/yog.h"

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogBuiltinFunction* f = ptr;
    YogArgInfo_keep_children(env, &f->arg_info, keeper);
}

YogBuiltinFunction* 
YogBuiltinFunction_new(YogEnv* env, void* f, ID klass_name, ID func_name, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, int required_argc, va_list ap)
{
#define ASSERT(name) do { \
    YOG_ASSERT(env, (name == 0) || (name == 1), #name "must be zero or one."); \
} while (0)
    ASSERT(blockargc);
    ASSERT(varargc);
    ASSERT(kwargc);
#undef ASSERT

#define NEXT_STR(ap)    va_arg(ap, const char*)
    va_list aq;
    va_copy(aq, ap);

    unsigned int argc = 0;
    while (NEXT_STR(ap) != NULL) {
        argc++;
    }
    if (0 < blockargc) {
        argc--;
    }

    ID* argnames = NULL;
    ID blockargname = 0;
    if (0 < argc) {
        argnames = ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(ID) * argc);
        FRAME_DECL_LOCAL(env, argnames_idx, PTR2VAL(argnames));
        unsigned int i = 0;
        const char* s = NULL;
        for (i = 0; i < argc; i++) {
            s = NEXT_STR(aq);
            FRAME_LOCAL_PTR(env, argnames, argnames_idx);
            argnames[i] = INTERN(s);
        }
        if (0 < blockargc) {
            s = NEXT_STR(aq);
            blockargname = INTERN(s);
            i++;
        }
        FRAME_LOCAL_PTR(env, argnames, argnames_idx);
    }
    FRAME_DECL_LOCAL(env, argnames_idx, PTR2VAL(argnames));
#undef NEXT_STR

    YogBuiltinFunction* builtin_f = ALLOC_OBJ(env, keep_children, NULL, YogBuiltinFunction);
    YogArgInfo* arg_info = &builtin_f->arg_info;
    arg_info->argc = argc;
    FRAME_LOCAL_PTR(env, argnames, argnames_idx);
    arg_info->argnames = argnames;
    arg_info->arg_index = NULL;
    arg_info->blockargc = blockargc;
    arg_info->blockargname = blockargname;
    arg_info->varargc = varargc;
    arg_info->kwargc = kwargc;
    builtin_f->required_argc = required_argc;

    builtin_f->f = f;

    builtin_f->klass_name = klass_name;
    builtin_f->func_name = func_name;

    return builtin_f;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
