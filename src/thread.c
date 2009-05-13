#include <stdlib.h>
#if HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif
#include "yog/gc.h"
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

#if 0
#   define DEBUG(x)     x
#else
#   define DEBUG(x)
#endif

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogThread* thread = ptr;

#define KEEP(member)    YogGC_keep(env, &thread->member, keeper, heap)
    KEEP(prev);
    KEEP(next);
#undef KEEP

    void* thread_heap = thread->THREAD_GC;
#define KEEP(member)    YogGC_keep(env, &thread->member, keeper, thread_heap)
    KEEP(cur_frame);
    KEEP(jmp_val);
#undef KEEP

    YogLocals* locals = thread->locals;
    while (locals != NULL) {
        unsigned int i;
        for (i = 0; i < locals->num_vals; i++) {
            YogVal* vals = locals->vals[i];
            if (vals == NULL) {
                continue;
            }

            unsigned int j;
            for (j = 0; j < locals->size; j++) {
                YogVal* val = &vals[j];
                DEBUG(YogVal old_val = *val);
                YogGC_keep(env, val, keeper, thread_heap);
                DEBUG(DPRINTF("val=%p, 0x%08x->0x%08x", val, old_val, *val));
            }
        }

        locals = locals->next;
    }
}

void 
YogThread_initialize(YogEnv* env, YogVal thread)
{
    PTR_AS(YogThread, thread)->prev = YUNDEF;
    PTR_AS(YogThread, thread)->next = YUNDEF;

#if defined(GC_COPYING)
#   define GC   copying
#elif defined(GC_MARK_SWEEP)
#   define GC   mark_sweep
#elif defined(GC_MARK_SWEEP_COMPACT)
#   define GC   mark_sweep_compact
#elif defined(GC_GENERATIONAL)
#   define GC   generational
#elif defined(GC_BDW)
#   define GC   bdw
#endif
    PTR_AS(YogThread, thread)->GC = NULL;
#undef GC

    PTR_AS(YogThread, thread)->cur_frame = YNIL;
    PTR_AS(YogThread, thread)->jmp_buf_list = NULL;
    PTR_AS(YogThread, thread)->jmp_val = YUNDEF;
    PTR_AS(YogThread, thread)->locals = NULL;
}

#if defined(GC_COPYING)
void 
YogThread_config_copying(YogEnv* env, YogVal thread, BOOL gc_stress, size_t init_heap_size, void* root, ChildrenKeeper root_keeper) 
{
    YogCopying* copying = malloc(sizeof(YogCopying));
    YogCopying_initialize(env, copying, gc_stress, init_heap_size, root, root_keeper);
    copying->refered = TRUE;
    YogVm_add_heap(env, env->vm, copying);

    PTR_AS(YogThread, thread)->copying = copying;
}
#endif

#if defined(GC_MARK_SWEEP)
void 
YogThread_config_mark_sweep(YogEnv* env, YogVal thread, size_t threshold, void* root, ChildrenKeeper root_keeper) 
{
    YogMarkSweep* mark_sweep = malloc(sizeof(YogMarkSweep));
    YogMarkSweep_initialize(env, mark_sweep, threshold, root, root_keeper);
    mark_sweep->refered = TRUE;
    YogVm_add_heap(env, env->vm, mark_sweep);

    PTR_AS(YogThread, thread)->mark_sweep = mark_sweep;
}
#endif

#if defined(GC_MARK_SWEEP_COMPACT)
void 
YogThread_config_mark_sweep_compact(YogEnv* env, YogVal thread, size_t chunk_size, size_t threshold, void* root, ChildrenKeeper root_keeper) 
{
    size_t size = sizeof(YogMarkSweepCompact);
    YogMarkSweepCompact* mark_sweep_compact = malloc(size);
    YogMarkSweepCompact_initialize(env, mark_sweep_compact, chunk_size, threshold, root, root_keeper);
    mark_sweep_compact->refered = TRUE;
    YogVm_add_heap(env, env->vm, mark_sweep_compact);

    PTR_AS(YogThread, thread)->mark_sweep_compact = mark_sweep_compact;
}
#endif

#if defined(GC_GENERATIONAL)
void 
YogThread_config_generational(YogEnv* env, YogVal thread, BOOL gc_stress, size_t young_heap_size, size_t old_chunk_size, size_t old_threshold, unsigned int tenure, void* root, ChildrenKeeper root_keeper) 
{
    YogGenerational* generational = malloc(sizeof(YogGenerational));
    YogGenerational_initialize(env, generational, gc_stress, young_heap_size, old_chunk_size, old_threshold, tenure, root, root_keeper);
    generational->refered = TRUE;
    YogVm_add_heap(env, env->vm, generational);

    PTR_AS(YogThread, thread)->generational = generational;
}
#endif

#if defined(GC_BDW)
void 
YogThread_config_bdw(YogEnv* env, YogVal thread, BOOL gc_stress) 
{
    YogBDW* bdw = malloc(sizeof(YogBDW));
    YogBDW_initialize(env, bdw, gc_stress);
    bdw->refered = TRUE;
    YogVm_add_heap(env, env->vm, bdw);

    PTR_AS(YogThread, thread)->bdw = bdw;
}
#endif

static void
finalize(YogEnv* env, void* ptr)
{
    YogThread* thread = ptr;
    GC_TYPE* heap = thread->THREAD_GC;
    if (heap != NULL) {
        heap->refered = FALSE;
    }
    thread->THREAD_GC = NULL;
}

YogVal 
YogThread_new(YogEnv* env) 
{
    YogVal thread = ALLOC_OBJ(env, keep_children, finalize, YogThread);
    YogThread_initialize(env, thread);

    return thread;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
