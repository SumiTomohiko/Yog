#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/gc/copying.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define IS_IN_HEAP(ptr, heap)   do { \
    void* from = heap->items; \
    void* to = heap->items + heap->size; \
    return (from <= ptr) && (ptr < to); \
} while (0)

BOOL
YogCopying_is_in_active_heap(YogEnv* env, YogCopying* copying, void* ptr)
{
    YogCopyingHeap* heap = copying->active_heap;
    IS_IN_HEAP(ptr, heap);
}

BOOL
YogCopying_is_in_inactive_heap(YogEnv* env, YogCopying* copying, void* ptr)
{
    YogCopyingHeap* heap = copying->inactive_heap;
    IS_IN_HEAP(ptr, heap);
}

#undef IS_IN_HEAP

/* TODO: commonize with the other GC */
static void
init_memory(void* ptr, size_t size)
{
    memset(ptr, 0xcb, size);
}

static YogCopyingHeap*
YogCopyingHeap_new(YogCopying* copying, size_t size)
{
    YogCopyingHeap* heap = (YogCopyingHeap*)malloc(sizeof(YogCopyingHeap) + size);
    if (heap == NULL) {
        copying->err = ERR_COPYING_MALLOC;
        return NULL;
    }

    heap->size = size;
    heap->free = heap->items;
    init_memory(heap->items, size);

    return heap;
}

static size_t
round_size(size_t size)
{
    size_t unit = sizeof(void*);
    return (size + unit - 1) & ~(unit - 1);
}

void*
YogCopying_copy(YogEnv* env, YogCopying* copying, void* ptr)
{
    if (ptr == NULL) {
        DEBUG(TRACE("%p: copy: NULL->NULL", env));
        return NULL;
    }

    YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
    if (header->forwarding_addr != NULL) {
        DEBUG(TRACE("%p: forward: %p->(%p)", env, ptr, header->forwarding_addr));
        return header->forwarding_addr;
    }

    unsigned char* dest = copying->unscanned;
    size_t size = header->size;
    YOG_ASSERT(env, 0 < size, "invalid size: header=%p, obj=%p, size=%x", header, header + 1, size);
    memcpy(dest, header, size);

    header->forwarding_addr = (YogCopyingHeader*)dest + 1;

    copying->unscanned += size;
    DEBUG(TRACE("%p: unscanned: %p->%p (0x%02x)", env, dest, copying->unscanned, size));
    DEBUG(TRACE("%p: copy: %p->%p, unscanned=%p, size=%u", env, ptr, (YogCopyingHeader*)dest + 1, copying->unscanned - size, size));

    return (YogCopyingHeader*)dest + 1;
}

static void
destroy_memory(void* ptr, size_t size)
{
    memset(ptr, 0xfd, size);
}

static void
free_heap_internal(YogCopyingHeap* heap)
{
    destroy_memory(heap->items, heap->size);
    free(heap);
}

static void
free_heap(YogCopying* copying)
{
#define FREE_HEAP(heap)     do { \
    free_heap_internal((heap)); \
    (heap) = NULL; \
} while (0)
    FREE_HEAP(copying->active_heap);
    FREE_HEAP(copying->inactive_heap);
#undef FREE_HEAP
}

static void
iterate_objects(YogEnv* env, YogCopyingHeap* heap, void (*callback)(YogEnv*, YogCopyingHeader*))
{
    unsigned char* ptr = heap->items;
    unsigned char* to = heap->free;
    while (ptr < to) {
        YogCopyingHeader* header = (YogCopyingHeader*)ptr;
        YOG_ASSERT(env, 0 < header->size, "invalid size (%x)", header->size);
        (*callback)(env, header);

        ptr += header->size;
    }
}

void
YogCopying_iterate_objects(YogEnv* env, YogCopying* copying, void (*callback)(YogEnv*, YogCopyingHeader*))
{
    iterate_objects(env, copying->active_heap, callback);
}

static void
delete_garbage_each(YogEnv* env, YogCopyingHeader* header)
{
    if (header->forwarding_addr == NULL) {
        DEBUG(TRACE("%p: finalize: %p", env, header));
        DEBUG(TRACE("%p: header->finalizer=%p", env, header->finalizer));
        if (header->finalizer != NULL) {
            (*header->finalizer)(env, header + 1);
        }
    }
}

void
YogCopying_delete_garbage(YogEnv* env, YogCopying* copying)
{
    YogCopying_iterate_objects(env, copying, delete_garbage_each);
}

void
YogCopying_finalize(YogEnv* env, YogCopying* copying)
{
    YogCopying_delete_garbage(env, copying);
    free_heap(copying);
}

static void
swap_heap(YogCopyingHeap** a, YogCopyingHeap** b)
{
    YogCopyingHeap* tmp = *a;
    *a = *b;
    *b = tmp;
}

void
YogCopying_prepare(YogEnv* env, YogCopying* copying)
{
    YogCopyingHeap* to_space = copying->inactive_heap;
    copying->scanned = copying->unscanned = to_space->items;
#define PRINT_HEAP(text, heap)   do { \
    DEBUG(TRACE("%p: %s: %p-%p", env, (text), (heap)->items, (char*)(heap)->items + (heap)->size)); \
} while (0)
    PRINT_HEAP("active heap", copying->active_heap);
    PRINT_HEAP("inactive heap", copying->inactive_heap);
#undef PRINT_HEAP
}

void
YogCopying_post_gc(YogEnv* env, YogCopying* copying)
{
    YogCopyingHeap* from_space = copying->active_heap;
    YogCopyingHeap* to_space = copying->inactive_heap;

    size_t size = from_space->free - from_space->items;
    destroy_memory(from_space->items, size);

    to_space->free = copying->unscanned;

    swap_heap(&copying->active_heap, &copying->inactive_heap);
}

void
YogCopying_scan(YogEnv* env, YogCopying* copying, ObjectKeeper keeper, void* heap)
{
    while (copying->scanned != copying->unscanned) {
        YogCopyingHeader* header = (YogCopyingHeader*)copying->scanned;
        ChildrenKeeper children_keeper = header->keeper;
        if (children_keeper != NULL) {
            DEBUG(TRACE("children_keeper=%p", children_keeper));
            (*children_keeper)(env, header + 1, keeper, heap);
        }

        copying->scanned += header->size;
    }
}

#if defined(GC_COPYING)
static void*
keep_object(YogEnv* env, void* ptr, void* heap)
{
    return YogCopying_copy(env, (YogCopying*)heap, ptr);
}

void
YogCopying_keep_vm(YogEnv* env, YogCopying* copying)
{
    YogVM_keep_children(env, env->vm, keep_object, copying);
}

void
YogCopying_cheney_scan(YogEnv* env, YogCopying* copying)
{
    YogCopying_scan(env, copying, keep_object, copying);
}
#endif

void*
YogCopying_alloc(YogEnv* env, YogCopying* copying, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    size_t needed_size = size + sizeof(YogCopyingHeader);
    size_t rounded_size = round_size(needed_size);
    YOG_ASSERT(env, 0 < needed_size, "invalid size (0x%08x)", needed_size);
    YOG_ASSERT(env, 0 < rounded_size, "invalid size (0x%08x)", rounded_size);
#define PRINT_HEAP(text, heap)   do { \
    DEBUG(TRACE("%p: %s: %p-%p", env, (text), (heap)->items, (char*)(heap)->items + (heap)->size)); \
} while (0)
    PRINT_HEAP("active heap", copying->active_heap);
    PRINT_HEAP("inactive heap", copying->inactive_heap);
#undef PRINT_HEAP

    YogCopyingHeap* heap = copying->active_heap;
#define REST_SIZE(heap)     ((heap)->size - ((heap)->free - (heap)->items))
    size_t rest_size = REST_SIZE(heap);
    BOOL gc_stress = env->vm->gc_stress;
    if ((rest_size < rounded_size) || gc_stress) {
        if (!gc_stress && (heap->size < rounded_size)) {
            return NULL;
        }

#if defined(GC_COPYING)
        YogGC_perform(env);
#elif defined(GC_GENERATIONAL)
        YogGC_perform_minor(env);
        if (gc_stress) {
#if 0
            YogGenerational_oldify_all(env, gen);
#endif
            YogGC_perform_major(env);
        }
#endif
        heap = copying->active_heap;
        rest_size = REST_SIZE(heap);
        if (rest_size < rounded_size) {
            copying->err = ERR_COPYING_OUT_OF_MEMORY;
            return NULL;
        }
    }
#undef REST_SIZE

    YogCopyingHeader* header = (YogCopyingHeader*)heap->free;
    header->keeper = keeper;
    header->finalizer = finalizer;
    header->forwarding_addr = NULL;
    header->size = rounded_size;
#if defined(GC_GENERATIONAL)
    header->survive_num = 0;
    header->updated = FALSE;
    header->gen = GEN_YOUNG;
#endif

    heap->free += rounded_size;

    return header + 1;
}

void
YogCopying_alloc_heap(YogEnv* env, YogCopying* copying)
{
    if (copying->active_heap) {
        return;
    }

    size_t heap_size = copying->heap_size;
    copying->active_heap = YogCopyingHeap_new(copying, heap_size);
    copying->inactive_heap = YogCopyingHeap_new(copying, heap_size);
}

void
YogCopying_init(YogEnv* env, YogCopying* copying, size_t heap_size)
{
    copying->err = ERR_COPYING_NONE;
    copying->heap_size = heap_size;
    copying->active_heap = NULL;
    copying->inactive_heap = NULL;
    copying->scanned = NULL;
    copying->unscanned = NULL;
}

BOOL
YogCopying_is_empty(YogEnv* env, YogCopying* copying)
{
    YogCopyingHeap* active_heap = copying->active_heap;
    if (active_heap->items == active_heap->free) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

#if 0
#include <stdarg.h>

static void
report_bug(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    abort();
}

static void
check_object(YogEnv* env, YogCopyingHeader* header)
{
#define ASSERT(expr, fmt, ...)  do { \
    if (!(expr)) { \
        report_bug(fmt, __VA_ARGS__); \
    } \
} while (0)
    ASSERT(0 < header->size, "invalid size (header=%p, obj=%p, size=0x%x)", header, (YogCopyingHeader*)header + 1, header->size);
    ASSERT(header->forwarding_addr == NULL, "invalid forwarding address (header=%p, obj=%p, addr=%p)", header, (YogCopyingHeader*)header + 1, header->forwarding_addr);
    ASSERT(((uint_t)header->finalizer & (sizeof(void*) - 1)) == 0, "invalid finalier (%p)", header->finalizer);
#undef ASSERT
}

#   define ESCAPE_PROTO
ESCAPE_PROTO void
YogCopying_check(YogEnv* env, YogCopying* copying)
{
    iterate_objects(env, copying->active_heap, check_object);
}

#   define ESCAPE_PROTO
ESCAPE_PROTO void
YogCopying_check_inactive_heap(YogEnv* env, YogCopying* copying)
{
    unsigned char* ptr = copying->inactive_heap->items;
    unsigned char* to = copying->unscanned;
    while (ptr < to) {
        YogCopyingHeader* header = (YogCopyingHeader*)ptr;
        YOG_ASSERT(env, 0 < header->size, "invalid size (%x)", header->size);
        check_object(env, header);

        ptr += header->size;
    }
}
#endif

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
