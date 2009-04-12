#include "yog/exception.h"
#include "yog/vm.h"
#include "yog/yog.h"

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogStackTraceEntry* entry = ptr;
#define KEEP(member)    entry->member = YogVal_keep(env, entry->member, keeper)
    KEEP(lower);
    KEEP(filename);
#undef KEEP_MEMBER
}

YogVal 
YogStackTraceEntry_new(YogEnv* env) 
{
    YogStackTraceEntry* entry = ALLOC_OBJ(env, keep_children, NULL, YogStackTraceEntry);
    entry->lower = YUNDEF;
    entry->lineno = 0;
    entry->filename = YUNDEF;
    entry->klass_name = INVALID_ID;
    entry->func_name = INVALID_ID;

    return PTR2VAL(entry);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
