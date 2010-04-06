#include "yog/exception.h"
#include "yog/gc.h"
#include "yog/yog.h"

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogStackTraceEntry* entry = PTR_AS(YogStackTraceEntry, ptr);
#define KEEP(member)    YogGC_KEEP(env, entry, member, keeper, heap)
    KEEP(lower);
    KEEP(filename);
#undef KEEP_MEMBER
}

YogVal
YogStackTraceEntry_new(YogEnv* env)
{
    YogVal entry = ALLOC_OBJ(env, keep_children, NULL, YogStackTraceEntry);
    PTR_AS(YogStackTraceEntry, entry)->lower = YUNDEF;
    PTR_AS(YogStackTraceEntry, entry)->lineno = 0;
    PTR_AS(YogStackTraceEntry, entry)->filename = YUNDEF;
    PTR_AS(YogStackTraceEntry, entry)->class_name = INVALID_ID;
    PTR_AS(YogStackTraceEntry, entry)->func_name = INVALID_ID;

    return entry;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
