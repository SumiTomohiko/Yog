#include <stdlib.h>
#include <string.h>
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/gc/copying.h"
#include "yog/gc/generational.h"
#include "yog/gc/internal.h"
#include "yog/gc/mark-sweep-compact.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define PAYLOAD2YOUNG_HEADER(ptr)   ((YoungHeader*)(ptr) - 1)
#define PAYLOAD2OLD_HEADER(ptr)     ((OldHeader*)(ptr) - 1)

struct Generational {
    struct YogHeap base;
    struct YogHeap* young_heap;
    struct YogHeap* old_heap;
    uint_t max_age;
    struct RememberedSet* remembered_set;
};

typedef struct Generational Generational;

#define GENERATIONAL(heap)          ((Generational*)(heap))
#define GENERATIONAL_YOUNG_HEAP(heap) \
                                    GENERATIONAL((heap))->young_heap
#define GENERATIONAL_OLD_HEAP(heap) \
                                    GENERATIONAL((heap))->old_heap
#define GENERATIONAL_MAX_AGE(heap)  GENERATIONAL((heap))->max_age
#define GENERATIONAL_REMEMBERED_SET(heap) \
                                    GENERATIONAL((heap))->remembered_set

#define INTERNAL /* for escaping from tools/update_prototype.py */

INTERNAL RememberedSet*
YogGenerational_get_remembered_set(YogEnv* env, YogHeap* heap)
{
    return GENERATIONAL_REMEMBERED_SET(heap);
}

static void
grow_remembered_set(YogEnv* env, YogHeap* heap)
{
    uint_t size = GENERATIONAL_REMEMBERED_SET(heap)->size + 1024;
    if (size < GENERATIONAL_REMEMBERED_SET(heap)->size) {
        YOG_BUG(env, "remembered set is too large");
    }
    RememberedSet* remembered_set = (RememberedSet*)realloc(GENERATIONAL_REMEMBERED_SET(heap), SIZEOF_REMEMBERED_SET(size));
    if (remembered_set == NULL) {
        YogError_out_of_memory(env);
    }
    remembered_set->size = size;
    GENERATIONAL_REMEMBERED_SET(heap) = remembered_set;
}

void
YogGenerational_add_to_remembered_set(YogEnv* env, YogHeap* heap, void* ptr)
{
    uint_t pos = GENERATIONAL_REMEMBERED_SET(heap)->pos;
    if (GENERATIONAL_REMEMBERED_SET(heap)->size - 1 < pos) {
        grow_remembered_set(env, heap);
    }
    GENERATIONAL_REMEMBERED_SET(heap)->items[pos] = ptr;
    GENERATIONAL_REMEMBERED_SET(heap)->pos = pos + 1;
    PAYLOAD2OLD_HEADER(ptr)->remembered = TRUE;
}

static void
YoungHeader_init(YogEnv* env, YoungHeader* header)
{
    header->age = 0;
    header->generation = GENERATION_YOUNG;
}

static void
OldHeader_init(YogEnv* env, OldHeader* header)
{
    header->remembered = FALSE;
    header->generation = GENERATION_OLD;
}

static void*
tenure(YogEnv* env, YogHeap* heap, void* ptr)
{
    YogHeap* young_heap = GENERATIONAL_YOUNG_HEAP(heap);
    YogHeap* old_heap = GENERATIONAL_OLD_HEAP(heap);
    ChildrenKeeper keeper = YogCopying_get_keeper(env, young_heap, ptr);
    Finalizer finalizer = YogCopying_get_finalizer(env, young_heap, ptr);
    size_t size = YogCopying_get_payload_size(env, young_heap, ptr);
    void* p = YogMarkSweepCompact_alloc(env, old_heap, keeper, finalizer, size);
    if (p == NULL) {
        return NULL;
    }
    OldHeader_init(env, PAYLOAD2OLD_HEADER(p));
    memcpy(p, ptr, size);
    YogCopying_set_forwarding_addr(env, young_heap, ptr, p);

    return p;
}

typedef void (*ProcForTenured)(YogEnv*, void*, ObjectKeeper, void*);

static void*
copy_young_obj(YogEnv* env, void* ptr, ObjectKeeper obj_keeper, void* heap, ProcForTenured proc_for_tenured)
{
    YogHeap* young_heap = GENERATIONAL_YOUNG_HEAP(heap);
    void* forwarding_addr = YogCopying_get_forwarding_addr(env, young_heap, ptr);
    if (forwarding_addr != NULL) {
        return forwarding_addr;
    }

    PAYLOAD2YOUNG_HEADER(ptr)->age++;
    if (PAYLOAD2YOUNG_HEADER(ptr)->age < GENERATIONAL_MAX_AGE(heap)) {
        return YogCopying_copy(env, young_heap, ptr);
    }

    void* p = tenure(env, heap, ptr);
    if (p == NULL) {
        return YogCopying_copy(env, young_heap, ptr);
    }
    (*proc_for_tenured)(env, p, obj_keeper, heap);

    return p;
}

static void
proc_for_tenured_major(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogMarkSweepCompact_mark(env, ptr, keeper, heap);
}

static void*
major_gc_keep_object(YogEnv* env, void* ptr, void* heap)
{
    DEBUG(TRACE("major_gc_keep_object(env=%p, ptr=%p, heap=%p)", env, ptr, heap));
    if (ptr == NULL) {
        return NULL;
    }
    if (YogGC_IS_OLD(ptr)) {
        return YogMarkSweepCompact_mark(env, ptr, major_gc_keep_object, heap);
    }
    YOG_ASSERT(env, YogGC_IS_YOUNG(ptr), "Invalid generation (0x%08x at 0x%08x)", PAYLOAD2GENERATION(ptr), &PAYLOAD2GENERATION(ptr));

    return copy_young_obj(env, ptr, major_gc_keep_object, heap, proc_for_tenured_major);
}

static void
proc_for_tenured_minor(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    ChildrenKeeper children_keeper = YogMarkSweepCompact_get_children_keeper(env, GENERATIONAL_OLD_HEAP(heap), ptr);
    if (children_keeper == NULL) {
        return;
    }
    YogHeap_add_to_marked_objects(env, heap, PTR2VAL(ptr));
}

static void*
minor_gc_keep_object(YogEnv* env, void* ptr, void* heap)
{
    DEBUG(TRACE("minor_gc_keep_object(env=%p, ptr=%p, heap=%p)", env, ptr, heap));
    if (ptr == NULL) {
        return NULL;
    }
    if (YogGC_IS_OLD(ptr)) {
        DEBUG(TRACE("%p: %p is in old generation.", env, ptr));
        return ptr;
    }
    YOG_ASSERT(env, YogGC_IS_YOUNG(ptr), "Invalid generation (0x%08x at 0x%08x)", PAYLOAD2GENERATION(ptr), &PAYLOAD2GENERATION(ptr));

    return copy_young_obj(env, ptr, minor_gc_keep_object, heap, proc_for_tenured_minor);
}

static void
init_remembered_set(YogEnv* env, Generational* heap, uint_t size)
{
    RememberedSet* remembered_set = (RememberedSet*)YogGC_malloc(env, SIZEOF_REMEMBERED_SET(size));
    remembered_set->pos = 0;
    remembered_set->size = size;
    GENERATIONAL_REMEMBERED_SET(heap) = remembered_set;
}

YogHeap*
YogGenerational_new(YogEnv* env, uint_t young_heap_size, uint_t old_heap_size, uint_t max_age)
{
    Generational* heap = (Generational*)YogGC_malloc(env, sizeof(Generational));
    YogHeap_init(env, (YogHeap*)heap);

    GENERATIONAL_YOUNG_HEAP(heap) = YogCopying_new(env, young_heap_size);
    GENERATIONAL_OLD_HEAP(heap) = YogMarkSweepCompact_new(env, old_heap_size);
    GENERATIONAL_MAX_AGE(heap) = max_age;
    init_remembered_set(env, heap, 1024);

    return (YogHeap*)heap;
}

void
YogGenerational_delete(YogEnv* env, YogHeap* heap)
{
    YogMarkSweepCompact_delete(env, GENERATIONAL_OLD_HEAP(heap));
    YogCopying_delete(env, GENERATIONAL_YOUNG_HEAP(heap));
    YogGC_free(env, GENERATIONAL_REMEMBERED_SET(heap), SIZEOF_REMEMBERED_SET(GENERATIONAL_REMEMBERED_SET(heap)->size));
    YogHeap_finalize(env, heap);
    YogGC_free(env, heap, sizeof(Generational));
}

void*
YogGenerational_alloc(YogEnv* env, YogHeap* heap, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    YogHeap* young_heap = GENERATIONAL_YOUNG_HEAP(heap);
    void* ptr = YogCopying_alloc(env, young_heap, keeper, finalizer, size);
    if (ptr != NULL) {
        YoungHeader_init(env, PAYLOAD2YOUNG_HEADER(ptr));
        return ptr;
    }

    YogHeap* old_heap = GENERATIONAL_OLD_HEAP(heap);
    ptr = YogMarkSweepCompact_alloc(env, old_heap, keeper, finalizer, size);
    if (ptr != NULL) {
        OldHeader_init(env, PAYLOAD2OLD_HEADER(ptr));
        return ptr;
    }

    return NULL;
}

void
YogGenerational_prepare_minor(YogEnv* env, YogHeap* heap)
{
    YogCopying_prepare(env, GENERATIONAL_YOUNG_HEAP(heap));
    init_remembered_set(env, GENERATIONAL(heap), GENERATIONAL_REMEMBERED_SET(heap)->size);
}

static void
reset_remembered_set(YogEnv* env, YogHeap* heap)
{
    uint_t size = GENERATIONAL_REMEMBERED_SET(heap)->pos;
    uint_t i;
    for (i = 0; i < size; i++) {
        void* ptr = GENERATIONAL_REMEMBERED_SET(heap)->items[i];
        PAYLOAD2OLD_HEADER(ptr)->remembered = FALSE;
    }
    GENERATIONAL_REMEMBERED_SET(heap)->pos = 0;
}

void
YogGenerational_prepare_major(YogEnv* env, YogHeap* heap)
{
    YogCopying_prepare(env, GENERATIONAL_YOUNG_HEAP(heap));
    reset_remembered_set(env, heap);
}

void
YogGenerational_minor_keep_vm(YogEnv* env, YogHeap* heap)
{
    YogVM_keep_children(env, env->vm, minor_gc_keep_object, heap);
}

static void
mark(YogEnv* env, YogHeap* heap, ObjectKeeper keeper)
{
    while (!YogHeap_is_marked_objects_empty(env, heap)) {
        YogHeap_init_marked_objects(env, heap);
        YogMarkSweepCompact_mark_children(env, heap, keeper);
        YogHeap_finish_marked_objects(env, heap);
    }
}

static void
cheney_scan(YogEnv* env, YogHeap* heap, ObjectKeeper keeper)
{
    YogHeap* young_heap = GENERATIONAL_YOUNG_HEAP(heap);
    YogCopying_scan(env, young_heap, keeper, heap);
}

void
YogGenerational_minor_delete_garbage(YogEnv* env, YogHeap* heap)
{
    YogCopying_delete_garbage(env, GENERATIONAL_YOUNG_HEAP(heap));
}

INTERNAL void
YogGenerational_trace_remembered_set(YogEnv* env, YogHeap* heap, RememberedSet* remembered_set)
{
    uint_t pos = remembered_set->pos;
    uint_t i;
    for (i = 0; i < pos; i++) {
        void* ptr = remembered_set->items[i];
        PAYLOAD2OLD_HEADER(ptr)->remembered = FALSE;
        proc_for_tenured_minor(env, ptr, minor_gc_keep_object, heap);
    }
}

static void
post_gc(YogEnv* env, YogHeap* heap)
{
    YogCopying_post_gc(env, GENERATIONAL_YOUNG_HEAP(heap));
}

void
YogGenerational_minor_post_gc(YogEnv* env, YogHeap* heap)
{
    post_gc(env, heap);
}

BOOL
YogGenerational_is_empty(YogEnv* env, YogHeap* heap)
{
    if (!YogCopying_is_empty(env, GENERATIONAL_YOUNG_HEAP(heap))) {
        return FALSE;
    }
    if (!YogMarkSweepCompact_is_empty(env, GENERATIONAL_OLD_HEAP(heap))) {
        return FALSE;
    }

    return TRUE;
}

void
YogGenerational_major_keep_vm(YogEnv* env, YogHeap* heap)
{
    YogVM_keep_children(env, env->vm, major_gc_keep_object, heap);
}

void
YogGenerational_major_delete_garbage(YogEnv* env, YogHeap* heap)
{
    YogCopying_delete_garbage(env, GENERATIONAL_YOUNG_HEAP(heap));
    YogMarkSweepCompact_delete_garbage(env, GENERATIONAL_OLD_HEAP(heap));
}

void
YogGenerational_major_post_gc(YogEnv* env, YogHeap* heap)
{
    post_gc(env, heap);
}

static void
traverse(YogEnv* env, YogHeap* heap, ObjectKeeper keeper)
{
    /**
     * YogHeap::marked_objects of heap has some objects which were added in
     * processing root pointers. They must be consumed first.
     */
    mark(env, heap, keeper);

    YogHeap* yound_heap = GENERATIONAL_YOUNG_HEAP(heap);
    while (!YogCopying_is_finished(env, yound_heap)) {
        YogHeap_init_marked_objects(env, heap);
        cheney_scan(env, heap, keeper);
        YogHeap_finish_marked_objects(env, heap);

        mark(env, heap, keeper);
    }
}

void
YogGenerational_minor_traverse(YogEnv* env, YogHeap* heap)
{
    traverse(env, heap, minor_gc_keep_object);
}

void
YogGenerational_major_traverse(YogEnv* env, YogHeap* heap)
{
    traverse(env, heap, major_gc_keep_object);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
