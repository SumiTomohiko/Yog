#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "yog/yog.h"

const char* 
YogVm_id2name(YogEnv* env, YogVm* vm, ID id) 
{
    YogVal sym = ID2VAL(id);
    YogVal val = YUNDEF;
    if (!YogTable_lookup(env, ENV_VM(env)->id2name, sym, &val)) {
        Yog_assert(env, FALSE, "Can't find symbol.");
    }

    YogCharArray* ptr = VAL2PTR(val);
    return ptr->items;
}

ID
YogVm_intern(YogEnv* env, YogVm* vm, const char* name)
{
    YogVal value = YNIL;
    if (YogTable_lookup_str(env, vm->name2id, name, &value)) {
        return VAL2ID(value);
    }

    YogCharArray* s = YogCharArray_new_str(env, name);
    YogVal val = PTR2VAL(s);
    ID id = vm->next_id;
    YogVal symbol = ID2VAL(id);
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
    YogTable_add_direct(env, vm->pkgs, ID2VAL(name), PTR2VAL(builtins));
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
    YogKlass* cObject = YogKlass_new(env, YogObj_allocate, "Object", NULL);
    YogKlass* cKlass = YogKlass_new(env, YogKlass_allocate, "Class", cObject);
    YOGBASICOBJ(cObject)->klass = cKlass;
    YOGBASICOBJ(cKlass)->klass = cKlass;
    vm->cObject = cObject;
    vm->cKlass = cKlass;
}

static void 
setup_klasses(YogEnv* env, YogVm* vm) 
{
    vm->cBuiltinBoundMethod = YogBuiltinBoundMethod_klass_new(env);
    vm->cBoundMethod = YogBoundMethod_klass_new(env);
    vm->cBuiltinUnboundMethod = YogBuiltinUnboundMethod_klass_new(env);
    vm->cUnboundMethod = YogUnboundMethod_klass_new(env);

    YogObj_klass_init(env, vm->cObject);
    YogKlass_klass_init(env, vm->cKlass);

    vm->cInt = YogInt_klass_new(env);
    vm->cString = YogString_klass_new(env);
    vm->cPackage = YogPackage_klass_new(env);
    vm->cBool = YogBool_klass_new(env);
    vm->cPackageBlock = YogPackageBlock_klass_new(env);
}

static void 
setup_encodings(YogEnv* env, YogVm* vm) 
{
#define REGISTER_ENCODING(name, onig)   do { \
    ID id = INTERN(name); \
    YogVal key = ID2VAL(id); \
    YogEncoding* enc = YogEncoding_new(env, onig); \
    YogVal val = PTR2VAL(enc); \
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

    vm->cObject = NULL;
    vm->cKlass = NULL;
    vm->cInt = NULL;
    vm->cString = NULL;
    vm->cPackage = NULL;
    vm->cBool = NULL;
    vm->cBuiltinBoundMethod = NULL;
    vm->cBoundMethod = NULL;
    vm->cBuiltinUnboundMethod = NULL;
    vm->cUnboundMethod = NULL;
    vm->cPackageBlock = NULL;

    vm->pkgs = NULL;

    vm->encodings = NULL;

    return vm;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
