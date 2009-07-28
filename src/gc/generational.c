#include <string.h>
#include "yog/env.h"
#include "yog/gc/generational.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"
#if defined(TEST_GENERATIONAL)
#   include <stdio.h>
#   include <stdlib.h>
#   include <CUnit/Basic.h>
#   include <CUnit/CUnit.h>
#endif

#if 0
#   define DEBUG(x)     x
#else
#   define DEBUG(x)
#endif

static void 
oldify(YogEnv* env, YogGenerational* gen, void* ptr) 
{
    YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
    header->survive_num = gen->tenure - 1;
}

static void 
oldify_all_callback(YogEnv* env, YogCopyingHeader* header) 
{
    YogGenerational* gen = PTR_AS(YogThread, env->thread)->heap;
    oldify(env, gen, header + 1);
}

void 
YogGenerational_oldify_all(YogEnv* env, YogGenerational* gen) 
{
    YogCopying_iterate_objects(env, &gen->copying, oldify_all_callback);
}

void* 
YogGenerational_copy_young_object(YogEnv* env, void* ptr, ObjectKeeper obj_keeper, void* heap)
{
    DEBUG(DPRINTF("YogGenerational_copy_young_object(env=%p, ptr=%p, obj_keeper=%p, heap=%p)", env, ptr, obj_keeper, heap));

    YogGenerational* gen = heap;
    YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
    DEBUG(DPRINTF("alive: %p (%p)", header, ptr));
    if (header->forwarding_addr != NULL) {
        DEBUG(DPRINTF("moved: %p->%p", ptr, header->forwarding_addr));
        return header->forwarding_addr;
    }

    header->survive_num++;
    if (header->survive_num < gen->tenure) {
        void* dest = YogCopying_copy(env, &gen->copying, ptr);
        DEBUG(DPRINTF("copied: %p->%p", ptr, dest));
        return dest;
    }
    else {
        YogMarkSweepCompact* msc = &gen->msc;
        ChildrenKeeper keeper = header->keeper;
        Finalizer finalizer = header->finalizer;
        size_t size = header->size - sizeof(YogCopyingHeader);
        void* p = YogMarkSweepCompact_alloc(env, msc, keeper, finalizer, size);
        DEBUG(DPRINTF("tenure: %p-%p (%p)->%p-%p (%p)", header, (unsigned char*)header + header->size, ptr, (YogMarkSweepCompactHeader*)p - 1, (unsigned char*)((YogMarkSweepCompactHeader*)p) + header->size, p));
        memcpy(p, ptr, size);
        header->forwarding_addr = p;
        YogMarkSweepCompact_mark_recursively(env, p, obj_keeper, heap);
        return p;
    }
}

static void*
major_gc_keep_object(YogEnv* env, void* ptr, void* heap)
{
    DEBUG(DPRINTF("major_gc_keep_object(env=%p, ptr=%p, heap=%p)", env, ptr, heap));
    if (ptr == NULL) {
        return NULL;
    }

    if (!IS_YOUNG(ptr)) {
        return YogMarkSweepCompact_mark_recursively(env, ptr, major_gc_keep_object, heap);
    }
    else {
        return YogGenerational_copy_young_object(env, ptr, major_gc_keep_object, heap);
    }
}

#if 0
static void* 
update_pointer(YogEnv* env, void* ptr) 
{
    DEBUG(DPRINTF("updating: %p", ptr));
    if (ptr == NULL) {
        return NULL;
    }

    if (IS_YOUNG(ptr)) {
        YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
        if (!header->updated) {
            header->updated = TRUE;
            ChildrenKeeper keeper = header->keeper;
            if (keeper != NULL) {
                DEBUG(DPRINTF("ptr=%p, keeper=%p", ptr, keeper));
                (*keeper)(env, ptr, update_pointer);
            }
        }
        return ptr;
    }
    else {
        return YogMarkSweepCompact_update_pointer(env, ptr, update_pointer);
    }
}

static void 
initialize_young_updated_callback(YogEnv* env, YogCopyingHeader* header) 
{
    header->updated = FALSE;
}

static void 
initialize_young_updated(YogEnv* env, YogGenerational* generational) 
{
    YogCopying_iterate_objects(env, &generational->copying, initialize_young_updated_callback);
}

void 
YogGenerational_major_gc(YogEnv* env, YogGenerational* generational) 
{
    DEBUG(DPRINTF("major GC..."));
    YogMarkSweepCompact* msc = &generational->msc;
    msc->in_gc = TRUE;
    YogMarkSweepCompact_unprotect_all_pages(env, msc);
    YogMarkSweepCompact_prepare(env, msc);

    YogCopying_prepare(env, &generational->copying);
    YogCopying_do_gc(env, &generational->copying, major_gc_keep_object);
    YogMarkSweepCompact_delete_garbage(env, msc);

    initialize_young_updated(env, generational);
    YogMarkSweepCompact_protect_white_pages(env, msc);
    msc->in_gc = FALSE;
    DEBUG(DPRINTF("major GC done"));
}
#endif

static void*
minor_gc_keep_object(YogEnv* env, void* ptr, void* heap)
{
    DEBUG(DPRINTF("minor_gc_keep_object(env=%p, ptr=%p, heap=%p)", env, ptr, heap));
    if (ptr == NULL) {
        return NULL;
    }
    if (!IS_YOUNG(ptr)) {
        DEBUG(DPRINTF("%p: %p is in old generation.", env, ptr));
        return ptr;
    }

    return YogGenerational_copy_young_object(env, ptr, minor_gc_keep_object, heap);
}

void 
YogGenerational_initialize(YogEnv* env, YogGenerational* generational, size_t young_heap_size, size_t old_chunk_size, size_t old_threshold, uint_t tenure)
{
    generational->err = ERR_GEN_NONE;

    YogCopying* copying = &generational->copying;
    YogCopying_initialize(env, copying, young_heap_size);

    YogMarkSweepCompact* msc = &generational->msc;
    YogMarkSweepCompact_initialize(env, msc, old_chunk_size, old_threshold);

    generational->tenure = tenure;
    generational->has_young_ref = FALSE;
}

void 
YogGenerational_finalize(YogEnv* env, YogGenerational* generational) 
{
    generational->err = ERR_GEN_NONE;

    YogMarkSweepCompact* msc = &generational->msc;
    YogMarkSweepCompact_finalize(env, msc);

    YogCopying* copying = &generational->copying;
    YogCopying_finalize(env, copying);
}

void* 
YogGenerational_alloc(YogEnv* env, YogGenerational* generational, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    YogCopying* copying = &generational->copying;
    void* ptr = YogCopying_alloc(env, copying, keeper, finalizer, size);
    if (ptr != NULL) {
        return ptr;
    }

    YogMarkSweepCompact* msc = &generational->msc;
    ptr = YogMarkSweepCompact_alloc(env, msc, keeper, finalizer, size);
    if (ptr != NULL) {
        return ptr;
    }

    uint_t err;
    switch (msc->err) {
    case ERR_MSC_MMAP:
        err = ERR_GEN_MMAP;
        break;
    case ERR_MSC_MUNMAP: 
        err = ERR_GEN_MUNMAP;
        break;
    case ERR_MSC_MALLOC: 
        err = ERR_GEN_MALLOC;
        break;
    default:
        err = ERR_GEN_UNKNOWN;
        break;
    }
    generational->err = err;

    return NULL;
}

void
YogGenerational_prepare(YogEnv* env, YogGenerational* generational)
{
    YogMarkSweepCompact* msc = &generational->msc;
    msc->in_gc = TRUE;
    YogMarkSweepCompact_unprotect_all_pages(env, msc);
    YogMarkSweepCompact_prepare(env, msc);

    YogCopying_prepare(env, &generational->copying);
}

void
YogGenerational_minor_keep_vm(YogEnv* env, YogGenerational* generational)
{
    YogVM_keep_children(env, env->vm, minor_gc_keep_object, generational);
}

void
YogGenerational_minor_cheney_scan(YogEnv* env, YogGenerational* generational)
{
    YogCopying* copying = &generational->copying;
    YogCopying_scan(env, copying, minor_gc_keep_object, generational);
}

void
YogGenerational_minor_delete_garbage(YogEnv* env, YogGenerational* generational)
{
    YogCopying_delete_garbage(env, &generational->copying);
}

void
YogGenerational_trace_grey(YogEnv* env, YogGenerational* generational)
{
    YogMarkSweepCompact* major_heap = &generational->msc;
    YogMarkSweepCompact_trace_grey_children(env, major_heap, generational);
}

static void
post_gc(YogEnv* env, YogGenerational* generational)
{
    YogMarkSweepCompact* msc = &generational->msc;
    YogMarkSweepCompact_protect_white_pages(env, msc);
    msc->in_gc = FALSE;
    YogCopying_post_gc(env, &generational->copying);
}

void
YogGenerational_minor_post_gc(YogEnv* env, YogGenerational* generational)
{
    post_gc(env, generational);
}

BOOL
YogGenerational_is_empty(YogEnv* env, YogGenerational* generational)
{
    if (!YogCopying_is_empty(env, &generational->copying)) {
        return FALSE;
    }
    if (!YogMarkSweepCompact_is_empty(env, &generational->msc)) {
        return FALSE;
    }

    return TRUE;
}

void
YogGenerational_allocate_heap(YogEnv* env, YogGenerational* generational)
{
    YogCopying_allocate_heap(env, &generational->copying);
}

void
YogGenerational_major_keep_vm(YogEnv* env, YogGenerational* generational)
{
    YogVM_keep_children(env, env->vm, major_gc_keep_object, generational);
}

void
YogGenerational_major_cheney_scan(YogEnv* env, YogGenerational* generational)
{
    YogCopying* copying = &generational->copying;
    YogCopying_scan(env, copying, major_gc_keep_object, generational);
}

void
YogGenerational_major_delete_garbage(YogEnv* env, YogGenerational* generational)
{
    YogCopying_delete_garbage(env, &generational->copying);
    YogMarkSweepCompact_delete_garbage(env, &generational->msc);
}

void
YogGenerational_major_post_gc(YogEnv* env, YogGenerational* generational)
{
    post_gc(env, generational);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
