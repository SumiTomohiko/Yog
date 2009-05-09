#include <sys/types.h>
#include "yog/env.h"
#if defined(GC_COPYING)
#   include "yog/gc/copying.h"
#elif defined(GC_MARK_SWEEP)
#   include "yog/gc/mark-sweep.h"
#elif defined(GC_MARK_SWEEP_COMPACT)
#   include "yog/gc/mark-sweep-compact.h"
#elif defined(GC_GENERATIONAL)
#   include "yog/gc/generational.h"
#elif defined(GC_BDW)
#   include "yog/gc/bdw.h"
#endif
#include "yog/thread.h"
#include "yog/yog.h"

YogVal 
YogGC_allocate(YogEnv* env, ChildrenKeeper keeper, Finalizer finalizer, size_t size) 
{
    YogVal thread = env->thread;
#if defined(GC_COPYING)
#   define GC       &PTR_AS(YogThread, thread)->copying
#   define ALLOC    YogCopying_alloc
#elif defined(GC_MARK_SWEEP)
#   define GC       &PTR_AS(YogThread, thread)->mark_sweep
#   define ALLOC    YogMarkSweep_alloc
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define GC       &PTR_AS(YogThread, thread)->mark_sweep_compact
#   define ALLOC    YogMarkSweepCompact_alloc
#elif defined(GC_GENERATIONAL)
#   define GC       &PTR_AS(YogThread, thread)->generational
#   define ALLOC    YogGenerational_alloc
#elif defined(GC_BDW)
#   define GC       &PTR_AS(YogThread, thread)->bdw
#   define ALLOC    YogBDW_alloc
#endif
    void* ptr = ALLOC(env, GC, keeper, finalizer, size);
#undef ALLOC
#undef GC

    if (ptr != NULL) {
        return PTR2VAL(ptr);
    }
    else {
        return YNIL;
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
