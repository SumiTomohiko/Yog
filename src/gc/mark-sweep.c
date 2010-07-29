#include <stdlib.h>
#include <string.h>
#include "yog/gc.h"
#include "yog/gc/mark-sweep.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct MarkSweep {
    struct YogHeap base;

    struct Header* header;
    size_t threshold;
    size_t allocated_size;
};

typedef struct MarkSweep MarkSweep;

struct Header {
    struct Header* prev;
    struct Header* next;
    size_t size;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    BOOL marked;
};

typedef struct Header Header;

static void*
keep_object(YogEnv* env, void* ptr, void* heap)
{
    if (ptr == NULL) {
        return NULL;
    }
    Header* header = (Header*)ptr - 1;
    if (header->marked) {
        return ptr;
    }

    header->marked = TRUE;

    ChildrenKeeper keeper = header->keeper;
    if (keeper == NULL) {
        return ptr;
    }
    (*keeper)(env, ptr, keep_object, heap);

    return ptr;
}

static void
finalize(YogEnv* env, Header* header)
{
    if (header->finalizer != NULL) {
        (*header->finalizer)(env, header + 1);
    }
}

static void
delete(YogEnv* env, MarkSweep* ms, Header* header)
{
    size_t size = header->size;
    YogGC_free(env, header, size);
    ms->allocated_size -= size;
}

void
YogMarkSweep_prepare(YogEnv* env, YogHeap* heap)
{
    MarkSweep* ms = (MarkSweep*)heap;
    Header* header = ms->header;
    while (header != NULL) {
        header->marked = FALSE;
        header = header->next;
    }
}

void
YogMarkSweep_delete_garbage(YogEnv* env, YogHeap* heap)
{
    MarkSweep* ms = (MarkSweep*)heap;
    Header* header = ms->header;
    while (header != NULL) {
        Header* next = header->next;

        if (!header->marked) {
            finalize(env, header);

            if (header->prev != NULL) {
                header->prev->next = next;
            }
            else {
                ms->header = next;
            }
            if (next != NULL) {
                next->prev = header->prev;
            }

            delete(env, ms, header);
        }

        header = next;
    }
}

YogHeap*
YogMarkSweep_new(YogEnv* env, size_t threshold)
{
    MarkSweep* heap = (MarkSweep*)YogGC_malloc(env, sizeof(MarkSweep));
    YogHeap_init(env, (YogHeap*)heap);

    heap->header = NULL;
    heap->threshold = threshold;
    heap->allocated_size = 0;

    return (YogHeap*)heap;
}

void
YogMarkSweep_delete(YogEnv* env, YogHeap* heap)
{
    MarkSweep* ms = (MarkSweep*)heap;
    Header* header = ms->header;
    while (header != NULL) {
        Header* next = header->next;

        finalize(env, header);
        delete(env, ms, header);

        header = next;
    }
}

void*
YogMarkSweep_alloc(YogEnv* env, YogHeap* heap, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    MarkSweep* ms = (MarkSweep*)heap;
    if (env->vm->gc_stress || (ms->threshold <= ms->allocated_size)) {
        YogGC_perform(env);
    }

    size_t total_size = size + sizeof(Header);
    Header* header = (Header*)YogGC_malloc(env, total_size);
    header->prev = NULL;
    header->next = ms->header;
    if (ms->header != NULL) {
        ms->header->prev = header;
    }
    ms->header = header;

    header->size = total_size;
    header->keeper = keeper;
    header->finalizer = finalizer;
    header->marked = FALSE;

    ms->allocated_size += total_size;

    return header + 1;
}

void
YogMarkSweep_keep_root(YogEnv* env, void* ptr, ChildrenKeeper keeper, YogHeap* heap)
{
    (*keeper)(env, ptr, keep_object, heap);
}

BOOL
YogMarkSweep_is_empty(YogEnv* env, YogHeap* heap)
{
    MarkSweep* ms = (MarkSweep*)heap;
    if (ms->header == NULL) {
        return TRUE;
    }
    return FALSE;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
