#include <stdlib.h>
#include <string.h>
#include "yog/gc.h"
#include "yog/gc/mark-sweep.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct YogMarkSweepHeader {
    struct YogMarkSweepHeader* prev;
    struct YogMarkSweepHeader* next;
    size_t size;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    BOOL marked;
};

typedef struct YogMarkSweepHeader YogMarkSweepHeader;

static void
init_memory(void* ptr, size_t size)
{
    memset(ptr, 0xcb, size);
}

static void*
keep_object(YogEnv* env, void* ptr, void* heap)
{
    if (ptr == NULL) {
        return NULL;
    }

    YogMarkSweepHeader* header = (YogMarkSweepHeader*)ptr - 1;
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
finalize(YogEnv* env, YogMarkSweepHeader* header)
{
    if (header->finalizer != NULL) {
        (*header->finalizer)(env, header + 1);
    }
}

static void
destroy_memory(void* ptr, size_t size)
{
    memset(ptr, 0xfd, size);
}

static void
delete(YogMarkSweepHeader* header)
{
    destroy_memory(header, header->size);
    free(header);
}

void
YogMarkSweep_prepare(YogEnv* env, YogMarkSweep* ms)
{
    YogMarkSweepHeader* header = ms->header;
    while (header != NULL) {
        header->marked = FALSE;
        header = header->next;
    }
}

void
YogMarkSweep_delete_garbage(YogEnv* env, YogMarkSweep* ms)
{
    YogMarkSweepHeader* header = ms->header;
    while (header != NULL) {
        YogMarkSweepHeader* next = header->next;

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

            delete(header);
        }

        header = next;
    }
}

void
YogMarkSweep_post_gc(YogEnv* env, YogMarkSweep* ms)
{
    ms->allocated_size = 0;
}

void
YogMarkSweep_init(YogEnv* env, YogMarkSweep* ms, size_t threshold)
{
    ms->prev = ms->next = NULL;
    ms->refered = TRUE;

    ms->header = NULL;
    ms->threshold = threshold;
    ms->allocated_size = 0;
}

void
YogMarkSweep_finalize(YogEnv* env, YogMarkSweep* ms)
{
    YogMarkSweepHeader* header = ms->header;
    while (header != NULL) {
        YogMarkSweepHeader* next = header->next;

        finalize(env, header);
        delete(header);

        header = next;
    }
}

void*
YogMarkSweep_alloc(YogEnv* env, YogMarkSweep* ms, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    if (env->vm->gc_stress || (ms->threshold <= ms->allocated_size)) {
        YogGC_perform(env);
    }

    size_t total_size = size + sizeof(YogMarkSweepHeader);
    YogMarkSweepHeader* header = malloc(total_size);
    init_memory(header, total_size);

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
YogMarkSweep_keep_vm(YogEnv* env, YogMarkSweep* ms)
{
    YogVM_keep_children(env, env->vm, keep_object, ms);
}

BOOL
YogMarkSweep_is_empty(YogEnv* env, YogMarkSweep* ms)
{
    if (ms->header == NULL) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
