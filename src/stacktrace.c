#include "yog/exception.h"
#include "yog/gc.h"
#include "yog/thread.h"
#include "yog/yog.h"

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogStackTraceEntry* entry = ptr;
#define KEEP(member)    YogGC_keep(env, &entry->member, keeper, heap)
    KEEP(lower);
    KEEP(filename);
#undef KEEP_MEMBER
}

YogVal 
YogStackTraceEntry_new(YogEnv* env) 
{
    SAVE_LOCALS(env);

    YogVal entry = YUNDEF;
    PUSH_LOCAL(env, entry);

    ALLOC_OBJ(env, entry, keep_children, NULL, YogStackTraceEntry);
    PTR_AS(YogStackTraceEntry, entry)->lower = YUNDEF;
    PTR_AS(YogStackTraceEntry, entry)->lineno = 0;
    PTR_AS(YogStackTraceEntry, entry)->filename = YUNDEF;
    PTR_AS(YogStackTraceEntry, entry)->klass_name = INVALID_ID;
    PTR_AS(YogStackTraceEntry, entry)->func_name = INVALID_ID;

    RETURN(env, entry);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
