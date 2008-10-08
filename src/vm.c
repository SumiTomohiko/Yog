#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "yog/yog.h"

ID
YogVm_intern(YogEnv* env, YogVm* vm, const char* name)
{
    YogVal value = YogVal_nil();
    if (YogTable_lookup_str(env, vm->name2id, name, &value)) {
        return YOGVAL_SYMBOL(value);
    }

    YogCharArray* s = YogCharArray_new_str(env, name);
    YogVal val = YogVal_gcobj(YOGGCOBJ(s));
    ID id = vm->next_id;
    YogVal symbol = YogVal_symbol(id);
    YogTable_add_direct(env, vm->name2id, val, symbol);
    YogTable_add_direct(env, vm->id2name, symbol, val);

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

    heap->size = size;
    heap->base = heap->free = ptr;
    heap->next = next;

    return heap;
}

YogGCObj* 
YogVm_alloc_gcobj(YogEnv* env, YogVm* vm, YogGCObjType type, size_t size) 
{
    size_t unit = sizeof(void*);
    size_t alimented_size = ((size - 1) / unit + 1) * unit;

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

    YogGCObj* gcobj = (YogGCObj*)heap->free;
    gcobj->type = type;
    YOGGCOBJ_FORWARDING_ADDR(gcobj) = NULL;
    YOGGCOBJ_SIZE(gcobj) = alimented_size;

    heap->free += alimented_size;

    return gcobj;
}

void 
YogVm_boot(YogEnv* env, YogVm* vm) 
{
    vm->id2name = YogTable_new_symbol_table(env);
    vm->name2id = YogTable_new_string_table(env);

    YogKlass* obj_klass = YogKlass_new(env, NULL);
    YogKlass* klass_klass = YogKlass_new(env, obj_klass);
    YOGBASICOBJ(obj_klass)->klass = klass_klass;
    YOGBASICOBJ(klass_klass)->klass = klass_klass;
    vm->obj_klass = obj_klass;
    vm->klass_klass = klass_klass;

    vm->int_klass = YogInt_klass_new(env);
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

    vm->obj_klass = NULL;
    vm->klass_klass = NULL;
    vm->func_klass = NULL;

    return vm;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
