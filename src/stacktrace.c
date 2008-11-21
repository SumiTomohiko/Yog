#include "yog/yog.h"

YogStackTraceEntry* 
YogStackTraceEntry_new(YogEnv* env) 
{
    YogStackTraceEntry* entry = ALLOC_OBJ(env, NULL, YogStackTraceEntry);
    entry->lineno = 0;
    entry->filename = NULL;
    entry->fname = 0;

    return entry;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
