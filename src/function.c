#include <stdarg.h>
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
    const char* s = NULL;
    while ((s = NEXT_STR(ap)) != NULL) {
        argc++;
    }
    if (0 < blockargc) {
        argc--;
    }

    ID* argnames = NULL;
    ID blockargname = 0;
    if (0 < argc) {
        argnames = ALLOC_OBJ_SIZE(env, NULL, sizeof(ID) * argc);
        unsigned int i = 0;
        for (i = 0; i < argc; i++) {
            const char* s = NEXT_STR(aq);
            argnames[i] = INTERN(s);
        }
        if (0 < blockargc) {
            blockargname = INTERN(s);
            i++;
        }
    }

#undef NEXT_STR

    YogBuiltinFunction* builtin_f = ALLOC_OBJ(env, keep_children, YogBuiltinFunction);
    YogArgInfo* arg_info = &builtin_f->arg_info;
    arg_info->argc = argc;
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
