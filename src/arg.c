#include "yog/arg.h"
#include "yog/gc.h"
#include "yog/yog.h"

static void
YogArgInfo_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogArgInfo* arg_info = ptr;
#define KEEP(member)    YogGC_keep(env, &arg_info->member, keeper, heap)
    KEEP(argnames);
    KEEP(arg_index);
#undef KEEP
}

YogVal
YogArgInfo_new(YogEnv* env)
{
    YogVal arg_info = ALLOC_OBJ(env, YogArgInfo_keep_children, NULL, YogArgInfo);
    PTR_AS(YogArgInfo, arg_info)->argc = 0;
    PTR_AS(YogArgInfo, arg_info)->argnames = YUNDEF;
    PTR_AS(YogArgInfo, arg_info)->arg_index = YUNDEF;
    PTR_AS(YogArgInfo, arg_info)->blockargc = 0;
    PTR_AS(YogArgInfo, arg_info)->blockargname = 0;
    PTR_AS(YogArgInfo, arg_info)->varargc = 0;
    PTR_AS(YogArgInfo, arg_info)->kwargc = 0;

    return arg_info;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
