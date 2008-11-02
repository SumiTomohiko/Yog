#include <stdarg.h>
#include "yog/yog.h"

static void 
gc_builtin_function_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
#if 0
    YogBuiltinFunction* f = ptr;
    /* TODO */
#endif
}

YogBuiltinFunction* 
YogBuiltinFunction_new(YogEnv* env, const char* name, void* f, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, int required_argc, va_list ap)
{
#define ASSERT(name) do { \
    Yog_assert(env, (name == 0) || (name == 1), #name "must be zero or one."); \
} while (0)
    ASSERT(blockargc);
    ASSERT(varargc);
    ASSERT(kwargc);
#undef ASSERT

#define FOR_EACH_ARGNAME(ap, s) do { \
    const char* s = NULL; \
    while ((s = va_arg(ap, const char*)) != NULL)

#define FOR_EACH_ARGNAME_END \
} while (0)

    va_list aq;
    va_copy(aq, ap);

    unsigned int argc = 0;
    FOR_EACH_ARGNAME(ap, s) {
        argc++;
    }
    FOR_EACH_ARGNAME_END;

    ID* argnames = NULL;
    if (0 < argc) {
        argnames = YogVm_alloc(env, NULL, sizeof(ID) * argc);
        unsigned int i = 0;
        FOR_EACH_ARGNAME(aq, s) {
            argnames[i] = INTERN(s);
            i++;
        }
        FOR_EACH_ARGNAME_END;
    }

#undef FOR_EACH_ARGNAME_END
#undef FOR_EACH_ARGNAME

    YogBuiltinFunction* builtin_f = ALLOC_OBJ(env, gc_builtin_function_children, YogBuiltinFunction);
    YogArgInfo* arg_info = &builtin_f->arg_info;
    arg_info->argc = argc;
    arg_info->blockargc = blockargc;
    arg_info->varargc = varargc;
    arg_info->kwargc = kwargc;
    arg_info->argnames = argnames;
    builtin_f->required_argc = required_argc;

    builtin_f->f = f;

    return builtin_f;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
