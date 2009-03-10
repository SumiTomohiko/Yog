#include <stdarg.h>
#include "yog/arg.h"
#include "yog/error.h"
#include "yog/function.h"
#include "yog/yog.h"

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogBuiltinFunction* f = ptr;
    f->arg_info = YogVal_keep(env, f->arg_info, keeper);
}

YogVal 
YogBuiltinFunction_new(YogEnv* env, void* f, ID klass_name, ID func_name, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, int required_argc, va_list ap)
{
    SAVE_LOCALS(env);

    YogVal argnames = YUNDEF;
    YogVal builtin_f = YUNDEF;
    YogVal arg_info = YUNDEF;
    PUSH_LOCALS3(env, argnames, builtin_f, arg_info);

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

    argnames = PTR2VAL(NULL);
    ID blockargname = 0;
    if (0 < argc) {
        argnames = PTR2VAL(ALLOC_OBJ_SIZE(env, NULL, NULL, sizeof(ID) * argc));
        unsigned int i = 0;
        for (i = 0; i < argc; i++) {
            const char* s = NEXT_STR(aq);
            ID id = INTERN(s);
            PTR_AS(ID, argnames)[i] = id;
        }
        if (0 < blockargc) {
            blockargname = INTERN(s);
            i++;
        }
    }

#undef NEXT_STR

    builtin_f = PTR2VAL(ALLOC_OBJ(env, keep_children, NULL, YogBuiltinFunction));
    BUILTIN_FUNCTION(builtin_f)->arg_info = YUNDEF;
    BUILTIN_FUNCTION(builtin_f)->f = f;
    BUILTIN_FUNCTION(builtin_f)->klass_name = klass_name;
    BUILTIN_FUNCTION(builtin_f)->func_name = func_name;
    BUILTIN_FUNCTION(builtin_f)->required_argc = required_argc;

    arg_info = YogArgInfo_new(env);
    ARG_INFO(arg_info)->argc = argc;
    ARG_INFO(arg_info)->argnames = VAL2PTR(argnames);
    ARG_INFO(arg_info)->arg_index = NULL;
    ARG_INFO(arg_info)->blockargc = blockargc;
    ARG_INFO(arg_info)->blockargname = blockargname;
    ARG_INFO(arg_info)->varargc = varargc;
    ARG_INFO(arg_info)->kwargc = kwargc;
    BUILTIN_FUNCTION(builtin_f)->arg_info = arg_info;

    RETURN(env, builtin_f);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
