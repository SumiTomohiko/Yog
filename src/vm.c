#include <stdlib.h>
#include "yog/yog.h"

static Heap* 
new_heap(size_t size, Heap* next) 
{
    Heap* heap = malloc(sizeof(Heap));
    Yog_assert(NULL, heap != NULL, "Can' allocate memory for heap.");

    void* ptr = malloc(size);
    Yog_assert(NULL, heap != NULL, "Can' allocate memory for heap.");

    heap->base = heap->free = ptr;
    heap->next = next;

    return heap;
}

void*
YogVm_malloc(YogEnv* env, size_t size) 
{
    size_t unit = sizeof(void*);
    size_t alimented_size = ((size - unit) / unit + 1) * unit;

    YogVm* vm = env->vm;
    Heap* heap = vm->heap;
    size_t rest_size = heap->size - (heap->free - heap->base);
    if (rest_size < alimented_size) {
        size_t allocate_size = 0;
        if (heap->size < alimented_size) {
            allocate_size = alimented_size;
        }
        else {
            allocate_size = heap->size;
        }
        vm->heap = new_heap(allocate_size, heap);
        vm->do_gc = TRUE;
    }

    void* ptr = heap->free;

    heap->free += alimented_size;

    return ptr;
}

YogVm* 
YogVm_new(size_t heap_size) 
{
    YogVm* vm = malloc(sizeof(YogVm));
    Yog_assert(NULL, vm != NULL, "Can' allocate memory for YogVm.");

    vm->do_gc = TRUE;
    vm->heap = new_heap(heap_size, NULL);

    return vm;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
