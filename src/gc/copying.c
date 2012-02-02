#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/gc/copying.h"
#if defined(GC_GENERATIONAL)
#   include "yog/gc/internal.h"
#endif
#include "yog/vm.h"
#include "yog/yog.h"

struct Header {
    ChildrenKeeper keeper;
    Finalizer finalizer;
    void* forwarding_addr;
    size_t size;
#if defined(GC_GENERATIONAL)
    struct YoungHeader generational_part;
#endif
};

typedef struct Header Header;

#define PAYLOAD2HEADER(ptr) ((Header*)(ptr) - 1)

struct Space {
    unsigned char* free;
    unsigned char items[0];
};

typedef struct Space Space;

struct Copying {
    struct YogHeap base;

    uint_t space_size;
    struct Space* from_space;
    struct Space* to_space;
    unsigned char* scanned;
    unsigned char* unscanned;
};

typedef struct Copying Copying;

static Space*
Space_new(YogEnv* env, uint_t size)
{
    Space* space = (Space*)YogGC_malloc(env, sizeof(Space) + size);
    space->free = space->items;
    YogGC_init_memory(env, space->items, size);
    return space;
}

static size_t
round_size(size_t size)
{
    size_t unit = sizeof(void*);
    return (size + unit - 1) & ~(unit - 1);
}

void*
YogCopying_copy(YogEnv* env, YogHeap* heap, void* ptr)
{
    if (ptr == NULL) {
        DEBUG(TRACE("%p: copy: NULL->NULL", env));
        return NULL;
    }

    Header* header = PAYLOAD2HEADER(ptr);
    if (header->forwarding_addr != NULL) {
        DEBUG(TRACE("%p: forward: %p->(%p)", env, ptr, header->forwarding_addr));
        return header->forwarding_addr;
    }

    Copying* copying = (Copying*)heap;
    void* dest = copying->unscanned;
    size_t size = header->size;
    YOG_ASSERT(env, 0 < size, "invalid size: header=%p, obj=%p, size=%x", header, header + 1, size);
    memcpy(dest, header, size);

    header->forwarding_addr = (Header*)dest + 1;

    copying->unscanned += size;
    DEBUG(TRACE("%p: unscanned: %p->%p (0x%02x)", env, dest, copying->unscanned, size));
    DEBUG(TRACE("%p: copy: %p->%p, unscanned=%p, size=%u", env, ptr, (Header*)dest + 1, copying->unscanned - size, size));

    return (Header*)dest + 1;
}

static void
destroy_memory(void* ptr, size_t size)
{
    memset(ptr, 0xfd, size);
}

static void
free_space(YogEnv* env, Copying* copying, Space* space)
{
    YogGC_free(env, space, sizeof(Space) + copying->space_size);
}

static void
free_spaces(YogEnv* env, Copying* copying)
{
#define FREE_SPACE(heap)     do { \
    free_space(env, copying, (heap)); \
    (heap) = NULL; \
} while (0)
    FREE_SPACE(copying->from_space);
    FREE_SPACE(copying->to_space);
#undef FREE_SPACE
}

static void
iterate_objects(YogEnv* env, Copying* copying, void (*callback)(YogEnv*, Header*))
{
    Space* space = copying->from_space;
    unsigned char* ptr = space->items;
    unsigned char* to = space->free;
    while (ptr < to) {
        Header* header = (Header*)ptr;
        YOG_ASSERT(env, 0 < header->size, "invalid size (%x)", header->size);
        (*callback)(env, header);
        ptr += header->size;
    }
}

static void
delete_garbage_each(YogEnv* env, Header* header)
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
YogCopying_delete_garbage(YogEnv* env, YogHeap* heap)
{
    Copying* copying = (Copying*)heap;
    iterate_objects(env, copying, delete_garbage_each);
}

static void
swap_spaces(Space** a, Space** b)
{
    Space* tmp = *a;
    *a = *b;
    *b = tmp;
}

void
YogCopying_prepare(YogEnv* env, YogHeap* heap)
{
    Copying* copying = (Copying*)heap;
    Space* to_space = copying->to_space;
    copying->scanned = copying->unscanned = to_space->items;
#define PRINT_HEAP(text, heap)   do { \
    DEBUG(TRACE("%p: %s: %p-%p", env, (text), (heap)->items, (char*)(heap)->items + (heap)->size)); \
} while (0)
    PRINT_HEAP("active heap", copying->from_space);
    PRINT_HEAP("inactive heap", copying->to_space);
#undef PRINT_HEAP
}

void
YogCopying_post_gc(YogEnv* env, YogHeap* heap)
{
    Copying* copying = (Copying*)heap;
    Space* from_space = copying->from_space;
    Space* to_space = copying->to_space;

    size_t size = from_space->free - from_space->items;
    destroy_memory(from_space->items, size);

    to_space->free = copying->unscanned;

    swap_spaces(&copying->from_space, &copying->to_space);
}

BOOL
YogCopying_is_finished(YogEnv* env, YogHeap* heap)
{
    Copying* copying = (Copying*)heap;
    return copying->scanned == copying->unscanned;
}

void
YogCopying_scan(YogEnv* env, YogHeap* heap, ObjectKeeper keeper, void* param)
{
    while (!YogCopying_is_finished(env, heap)) {
        Copying* copying = (Copying*)heap;
        DEBUG(TRACE("scanned=%p, unscanned=%p", copying->scanned, copying->unscanned));
        Header* header = (Header*)copying->scanned;
        ChildrenKeeper children_keeper = header->keeper;
        if (children_keeper != NULL) {
            DEBUG(TRACE("ptr=%p, children_keeper=%p", header + 1, children_keeper));
            (*children_keeper)(env, header + 1, keeper, param);
        }

        copying->scanned += header->size;
    }
}

#if defined(GC_COPYING)
static void*
keep_object(YogEnv* env, void* ptr, void* heap)
{
    return YogCopying_copy(env, (YogHeap*)heap, ptr);
}

void
YogCopying_keep_root(YogEnv* env, void* ptr, ChildrenKeeper keeper, YogHeap* heap)
{
    (*keeper)(env, ptr, keep_object, heap);
}

void
YogCopying_cheney_scan(YogEnv* env, YogHeap* heap)
{
    YogCopying_scan(env, heap, keep_object, heap);
}
#endif

void*
YogCopying_alloc(YogEnv* env, YogHeap* heap, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    Copying* copying = (Copying*)heap;
    size_t needed_size = size + sizeof(Header);
    size_t rounded_size = round_size(needed_size);
    YOG_ASSERT(env, 0 < needed_size, "invalid size (0x%08x)", needed_size);
    YOG_ASSERT(env, 0 < rounded_size, "invalid size (0x%08x)", rounded_size);
#define PRINT_HEAP(text, heap)   do { \
    DEBUG(TRACE("%p: %s: %p-%p", env, (text), (heap)->items, (char*)(heap)->items + (heap)->size)); \
} while (0)
    PRINT_HEAP("from space", copying->from_space);
    PRINT_HEAP("to space", copying->to_space);
#undef PRINT_HEAP

    Space* space = copying->from_space;
#define REST_SIZE(heap, space) \
    ((heap)->space_size - ((space)->free - (space)->items))
    size_t rest_size = REST_SIZE(copying, space);
    BOOL gc_stress = env->vm->gc_stress;
    if ((rest_size < rounded_size) || gc_stress) {
        if (!gc_stress && (copying->space_size < rounded_size)) {
            return NULL;
        }

#if defined(GC_COPYING)
        YogGC_perform(env);
#elif defined(GC_GENERATIONAL)
        YogGC_perform_minor(env);
        if (gc_stress) {
            YogGC_perform_major(env);
        }
#endif
        space = copying->from_space;
        rest_size = REST_SIZE(copying, space);
        if (rest_size < rounded_size) {
            return NULL;
        }
    }
#undef REST_SIZE

    Header* header = (Header*)space->free;
    header->keeper = keeper;
    header->finalizer = finalizer;
    header->forwarding_addr = NULL;
    header->size = rounded_size;

    space->free += rounded_size;

    return header + 1;
}

BOOL
YogCopying_is_empty(YogEnv* env, YogHeap* heap)
{
    Copying* copying = (Copying*)heap;
    Space* from_space = copying->from_space;
    if (from_space->items != from_space->free) {
        return FALSE;
    }
    return TRUE;
}

YogHeap*
YogCopying_new(YogEnv* env, uint_t heap_size)
{
    Copying* heap = (Copying*)YogGC_malloc(env, sizeof(Copying));
    YogHeap_init(env, (YogHeap*)heap);

    uint_t space_size = heap_size / 2;
    heap->space_size = space_size;
    heap->from_space = Space_new(env, space_size);
    heap->to_space = Space_new(env, space_size);
    heap->scanned = NULL;
    heap->unscanned = NULL;

    return (YogHeap*)heap;
}

void
YogCopying_delete(YogEnv* env, YogHeap* heap)
{
    YogCopying_delete_garbage(env, heap);
    free_spaces(env, (Copying*)heap);
    YogGC_free(env, heap, sizeof(Copying));
}

#if defined(GC_GENERATIONAL)
void
YogCopying_set_forwarding_addr(YogEnv* env, YogHeap* heap, void* ptr, void* forwarding_addr)
{
    PAYLOAD2HEADER(ptr)->forwarding_addr = forwarding_addr;
}

void*
YogCopying_get_forwarding_addr(YogEnv* env, YogHeap* heap, void* ptr)
{
    return PAYLOAD2HEADER(ptr)->forwarding_addr;
}

ChildrenKeeper
YogCopying_get_keeper(YogEnv* env, YogHeap* heap, void* ptr)
{
    return PAYLOAD2HEADER(ptr)->keeper;
}

Finalizer
YogCopying_get_finalizer(YogEnv* env, YogHeap* heap, void* ptr)
{
    return PAYLOAD2HEADER(ptr)->finalizer;
}

size_t
YogCopying_get_payload_size(YogEnv* env, YogHeap* heap, void* ptr)
{
    return PAYLOAD2HEADER(ptr)->size - sizeof(Header);
}
#endif

#if 0
#   include <stdarg.h>
#   include "yog/thread.h"

static void
report_bug(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    abort();
}

static void*
check_ptr(YogEnv* env, void* ptr, void* heap)
{
    if (ptr == NULL) {
        return ptr;
    }
    Header* header = (Header*)ptr - 1;
#if 0
    if (header->forwarding_addr != NULL) {
        return ptr;
    }
    header->forwarding_addr = (void*)0xdeadbeef;
#endif

    if (header->size == 0xfdfdfdfd) {
        report_bug("invalid size");
    }
    Space* to_space = ((Copying*)heap)->to_space;
    void* from = to_space->items;
    void* to = to_space->items + to_space->size;
    if ((from <= ptr) && (ptr < to)) {
        report_bug("an object in the inactive heap");
    }

#if 0
    if (header->keeper != NULL) {
        header->keeper(env, ptr, check_ptr, heap);
    }
#endif
    return ptr;
}

static void
reset_forwarding_addr(YogEnv* env, Header* header)
{
    header->forwarding_addr = NULL;
}

static void
check_object(YogEnv* env, Header* header)
{
#if 0
#define ASSERT(expr, fmt, ...)  do { \
    if (!(expr)) { \
        report_bug(fmt, __VA_ARGS__); \
    } \
} while (0)
    ASSERT(0 < header->size, "invalid size (header=%p, obj=%p, size=0x%x)", header, (Header*)header + 1, header->size);
    ASSERT(header->size != 0xfdfdfdfd, "invalid size (header=%p, obj=%p, size=0x%x)", header, (Header*)header + 1, header->size);
    ASSERT(header->forwarding_addr == NULL, "invalid forwarding address (header=%p, obj=%p, addr=%p)", header, (Header*)header + 1, header->forwarding_addr);
    ASSERT(((uint_t)header->finalizer & (sizeof(void*) - 1)) == 0, "invalid finalier (%p)", header->finalizer);
#undef ASSERT
#endif
    if (header->keeper == NULL) {
        return;
    }
    header->keeper(env, header + 1, check_ptr, NULL);
}

void YogCopying_check(YogEnv* env, Copying* copying)
{
    iterate_objects(env, copying->from_space, reset_forwarding_addr);
    YogVM_keep_children(env, env->vm, check_ptr, PTR_AS(YogThread, env->thread)->heap);
    iterate_objects(env, copying->from_space, reset_forwarding_addr);
}

void YogCopying_check_to_space(YogEnv* env, Copying* copying)
{
#if 0
    unsigned char* ptr = copying->to_space->items;
#endif
    unsigned char* ptr = copying->unscanned;
    unsigned char* to = copying->scanned;
    while (ptr < to) {
        Header* header = (Header*)ptr;
        YOG_ASSERT(env, 0 < header->size, "invalid size (%x)", header->size);
        if (header->keeper != NULL) {
            header->keeper(env, header + 1, check_ptr, NULL);
        }

        ptr += header->size;
    }
}
#endif

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
