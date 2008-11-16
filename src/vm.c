#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "yog/yog.h"

const char* 
YogVm_id2name(YogEnv* env, YogVm* vm, ID id) 
{
    YogVal sym = YogVal_symbol(id);
    YogVal val = YogVal_undef();
    if (!YogTable_lookup(env, ENV_VM(env)->id2name, sym, &val)) {
        Yog_assert(env, FALSE, "Can't find symbol.");
    }

    YogCharArray* ptr = YOGVAL_PTR(val);
    return ptr->items;
}

ID
YogVm_intern(YogEnv* env, YogVm* vm, const char* name)
{
    YogVal value = YogVal_nil();
    if (YogTable_lookup_str(env, vm->name2id, name, &value)) {
        return YOGVAL_SYMBOL(value);
    }

    YogCharArray* s = YogCharArray_new_str(env, name);
    YogVal val = YogVal_ptr(s);
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

void* 
YogVm_alloc(YogEnv* env, GcChildren gc_children, size_t size) 
{
    size_t unit = sizeof(void*);
    size_t needed_size = size + sizeof(GcHead);
    size_t aligned_size = ((needed_size - 1) / unit + 1) * unit;

    YogVm* vm = ENV_VM(env);
    Heap* heap = vm->heap;
    size_t used_size = heap->free - heap->base;
    size_t rest_size = heap->size - used_size;
    if (rest_size < aligned_size) {
        size_t allocate_size = 0;
        if (heap->size < aligned_size) {
            allocate_size = aligned_size;
        }
        else {
            allocate_size = heap->size;
        }
        vm->heap = heap = new_heap(allocate_size, heap);
        vm->need_gc = TRUE;
    }

    GcHead* head = (GcHead*)heap->free;
    head->gc_children = gc_children;
    head->forwarding_addr = NULL;
    head->size = aligned_size;

    heap->free += aligned_size;

    return head + 1;
}

static void 
setup_builtins(YogEnv* env, YogVm* vm) 
{
    YogObj* builtins = Yog_bltins_new(env);
    ID name = YogVm_intern(env, vm, BUILTINS);
    YogTable_add_direct(env, vm->pkgs, YogVal_symbol(name), YogVal_ptr(builtins));
}

static void 
setup_symbol_tables(YogEnv* env, YogVm* vm) 
{
    vm->id2name = YogTable_new_symbol_table(env);
    vm->name2id = YogTable_new_string_table(env);
}

static void 
setup_basic_klass(YogEnv* env, YogVm* vm) 
{
    YogKlass* obj_klass = YogKlass_new(env, YogObj_allocate, "Object", NULL);
    YogKlass* klass_klass = YogKlass_new(env, YogKlass_allocate, "Class", obj_klass);
    YOGBASICOBJ(obj_klass)->klass = klass_klass;
    YOGBASICOBJ(klass_klass)->klass = klass_klass;
    vm->obj_klass = obj_klass;
    vm->klass_klass = klass_klass;
}

static void 
setup_klasses(YogEnv* env, YogVm* vm) 
{
    vm->builtin_bound_method_klass = YogBuiltinBoundMethod_klass_new(env);
    vm->bound_method_klass = YogBoundMethod_klass_new(env);
    vm->builtin_unbound_method_klass = YogBuiltinUnboundMethod_klass_new(env);
    vm->unbound_method_klass = YogUnboundMethod_klass_new(env);

    YogObj_klass_init(env, vm->obj_klass);
    YogKlass_klass_init(env, vm->klass_klass);

    vm->int_klass = YogInt_klass_new(env);
    vm->string_klass = YogString_klass_new(env);
    vm->pkg_klass = YogPkg_klass_new(env);
    vm->bool_klass = YogBool_klass_new(env);
    vm->pkg_block_klass = YogPackageBlock_klass_new(env);
}

static void 
setup_encodings(YogEnv* env, YogVm* vm) 
{
#define REGISTER_ENCODING(name, onig)   do { \
    ID id = INTERN(name); \
    YogVal key = YogVal_symbol(id); \
    YogEncoding* enc = YogEncoding_new(env, onig); \
    YogVal val = YogVal_ptr(enc); \
    YogTable_add_direct(env, vm->encodings, key, val); \
} while (0)
    REGISTER_ENCODING("ascii", ONIG_ENCODING_ASCII);
    REGISTER_ENCODING("utf-8", ONIG_ENCODING_UTF8);
    REGISTER_ENCODING("euc-jp", ONIG_ENCODING_EUC_JP);
    REGISTER_ENCODING("shift-jis", ONIG_ENCODING_SJIS);
#undef REGISTER_ENCODING
}

void 
YogVm_boot(YogEnv* env, YogVm* vm) 
{
    setup_symbol_tables(env, vm);
    setup_basic_klass(env, vm);
    setup_klasses(env, vm);

    vm->pkgs = YogTable_new_symbol_table(env);
    setup_builtins(env, vm);

    vm->encodings = YogTable_new_symbol_table(env);
    setup_encodings(env, vm);
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
    vm->int_klass = NULL;
    vm->string_klass = NULL;
    vm->pkg_klass = NULL;
    vm->bool_klass = NULL;
    vm->builtin_bound_method_klass = NULL;
    vm->bound_method_klass = NULL;
    vm->builtin_unbound_method_klass = NULL;
    vm->unbound_method_klass = NULL;
    vm->pkg_block_klass = NULL;

    vm->pkgs = NULL;

    vm->encodings = NULL;

    return vm;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
