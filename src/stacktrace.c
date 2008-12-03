#include "yog/exception.h"
#include "yog/yog.h"

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogStackTraceEntry* entry = ptr;
#define KEEP_MEMBER(member)     entry->member = (*keeper)(env, (void*)entry->member)
    KEEP_MEMBER(lower);
    KEEP_MEMBER(filename);
#undef KEEP_MEMBER
}

YogStackTraceEntry* 
YogStackTraceEntry_new(YogEnv* env) 
{
    YogStackTraceEntry* entry = ALLOC_OBJ(env, keep_children, YogStackTraceEntry);
    entry->lower = NULL;
    entry->lineno = 0;
    entry->filename = NULL;
    entry->klass_name = INVALID_ID;
    entry->func_name = INVALID_ID;

    return entry;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
