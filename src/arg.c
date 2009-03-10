#include "yog/arg.h"
#include "yog/yog.h"

static void 
YogArgInfo_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogArgInfo* arg_info = ptr;
#define KEEP_MEMBER(member)     do { \
    arg_info->member = (*keeper)(env, arg_info->member); \
} while (0)
    KEEP_MEMBER(argnames);
    KEEP_MEMBER(arg_index);
#undef KEEP_MEMBER
}

YogVal 
YogArgInfo_new(YogEnv* env) 
{
    YogArgInfo* arg_info = ALLOC_OBJ(env, YogArgInfo_keep_children, NULL, YogArgInfo);
    arg_info->argc = 0;
    arg_info->argnames = NULL;
    arg_info->arg_index = NULL;
    arg_info->blockargc = 0;
    arg_info->blockargname = 0;
    arg_info->varargc = 0;
    arg_info->kwargc = 0;

    return PTR2VAL(arg_info);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
