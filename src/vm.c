#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "yog/yog.h"

ID
YogVm_intern(YogEnv* env, YogVm* vm, const char* name)
{
    YogVal value = YogVal_nil();
    if (YogTable_lookup(env, vm->name2id, YogVal_string(name), &value)) {
        return YOGVAL_SYMBOL(value);
    }

    YogVal s = YogVal_string(strdup(name));
    ID id = vm->next_id;
    YogVal symbol = YogVal_symbol(id);
    YogTable_add_direct(env, vm->name2id, s, symbol);
    YogTable_add_direct(env, vm->id2name, symbol, s);

    vm->next_id++;

    return id;
}

static Heap* 
new_heap(size_t size, Heap* next) 
{
    Heap* heap = malloc(sizeof(Heap));
    Yog_assert(NULL, heap != NULL, "Can' allocate memory for heap.");

    void* ptr = malloc(size);
    Yog_assert(NULL, heap != NULL, "Can' allocate memory for heap.");
    bzero(ptr, size);

    heap->base = heap->free = ptr;
    heap->next = next;

    return heap;
}

YogObj* 
YogVm_alloc_obj(YogEnv* env, YogVm* vm, YogObjType type, size_t size) 
{
    size_t unit = sizeof(void*);
    size_t alimented_size = ((size - unit) / unit + 1) * unit;

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
        vm->need_gc = TRUE;
    }

    YogObj* obj = (YogObj*)heap->free;
    obj->type = type;
    obj->forwarding_addr = NULL;
    obj->size = alimented_size;

    heap->free += alimented_size;

    return obj;
}

YogVm* 
YogVm_new(size_t heap_size) 
{
    YogVm* vm = malloc(sizeof(YogVm));
    Yog_assert(NULL, vm != NULL, "Can' allocate memory for YogVm.");

    vm->need_gc = FALSE;
    vm->heap = new_heap(heap_size, NULL);
    vm->next_id = 0;
    vm->id2name = NULL;
    vm->name2id = NULL;

    return vm;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
