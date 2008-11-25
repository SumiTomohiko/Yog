#include "yog/yog.h"

void 
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

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
