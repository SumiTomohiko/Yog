#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "yog/yog.h"

struct CopyingHeader {
    unsigned int id;
    ChildrenKeeper keeper;
    void* forwarding_addr;
    size_t size;
};

typedef struct CopyingHeader CopyingHeader;

void 
YogVm_register_package(YogEnv* env, YogVm* vm, const char* name, YogPackage* pkg) 
{
    ID id = YogVm_intern(env, vm, name);
    YogVal key = ID2VAL(id);

    YogVal value = OBJ2VAL(pkg);

    YogTable_add_direct(env, vm->pkgs, key, value);
}

const char* 
YogVm_id2name(YogEnv* env, YogVm* vm, ID id) 
{
    YogVal sym = ID2VAL(id);
    YogVal val = YUNDEF;
    if (!YogTable_lookup(env, ENV_VM(env)->id2name, sym, &val)) {
        YOG_ASSERT(env, FALSE, "Can't find symbol.");
    }

    YogCharArray* ptr = VAL2PTR(val);
    return ptr->items;
#if 0
    return VAL2STR(val);
#endif
}

ID
YogVm_intern(YogEnv* env, YogVm* vm, const char* name)
{
    YogVal value = YUNDEF;
    if (YogTable_lookup_str(env, vm->name2id, name, &value)) {
        return VAL2ID(value);
    }

#if 0
    const char* s = YogString_dup(env, name);
    YogVal val = STR2VAL(s);
#endif
    YogCharArray* s = YogCharArray_new_str(env, name);
    YogVal val = PTR2VAL(s);

    ID id = vm->next_id;
    YogVal symbol = ID2VAL(id);

    YogTable_add_direct(env, vm->name2id, val, symbol);
    YogTable_add_direct(env, vm->id2name, symbol, val);

    vm->next_id++;

    return id;
}

static YogHeap* 
YogHeap_new(size_t size, YogHeap* next) 
{
    YogHeap* heap = malloc(sizeof(YogHeap));
    YOG_ASSERT(NULL, heap != NULL, "Can' allocate memory for heap.");

    void* ptr = malloc(size);
    YOG_ASSERT(NULL, heap != NULL, "Can' allocate memory for heap.");
    memset(ptr, 0xcb, size);

    heap->size = size;
    heap->base = heap->free = ptr;
    heap->next = next;

    return heap;
}

static unsigned int 
align(size_t size) 
{
    size_t unit = sizeof(void*);
    size_t align_size = ((size - 1) / unit + 1) * unit;

    return align_size;
}

void* 
YogVm_alloc(YogEnv* env, ChildrenKeeper keeper, size_t size) 
{
    size_t needed_size = size + sizeof(CopyingHeader);
    size_t aligned_size = align(needed_size);

    YogVm* vm = ENV_VM(env);
    YogHeap* heap = vm->heap;
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
        vm->heap = heap = YogHeap_new(allocate_size, heap);
        if (!vm->disable_gc) {
            vm->need_gc = TRUE;
        }
    }

    static unsigned int id = 0;

    CopyingHeader* head = (CopyingHeader*)heap->free;
    head->id = id++;
    head->keeper = keeper;
    head->forwarding_addr = NULL;
    head->size = aligned_size;

    heap->free += aligned_size;

    return head + 1;
}

static void 
setup_builtins(YogEnv* env, YogVm* vm) 
{
    YogPackage* builtins = YogBuiltins_new(env);
    YogVm_register_package(env, vm, BUILTINS, builtins);
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
    YogKlass* cObject = YogKlass_new(env, "Object", NULL);
    YogKlass_define_allocator(env, cObject, YogObj_allocate);

    YogKlass* cKlass = YogKlass_new(env, "Class", cObject);
    YogKlass_define_allocator(env, cKlass, YogKlass_allocate);

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
    vm->cNil = YogNil_klass_new(env);
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

static void 
setup_exceptions(YogEnv* env, YogVm* vm) 
{
    vm->eException = YogException_klass_new(env);
    vm->eBugException = YogKlass_new(env, "BugException", vm->eException);
}

void 
YogVm_boot(YogEnv* env, YogVm* vm) 
{
    setup_symbol_tables(env, vm);
    setup_basic_klass(env, vm);
    setup_klasses(env, vm);
    setup_exceptions(env, vm);

    vm->pkgs = YogTable_new_symbol_table(env);
    setup_builtins(env, vm);

    vm->encodings = YogTable_new_symbol_table(env);
    setup_encodings(env, vm);
}

YogVm* 
YogVm_new(size_t heap_size) 
{
    YogVm* vm = malloc(sizeof(YogVm));
    YOG_ASSERT(NULL, vm != NULL, "Can' allocate memory for YogVm.");

    vm->always_gc = FALSE;
    vm->disable_gc = FALSE;

    vm->need_gc = FALSE;
    vm->heap = YogHeap_new(heap_size, NULL);
    vm->scanned = NULL;
    vm->unscanned = NULL;

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
    vm->cNil = NULL;

    vm->eException = NULL;
    vm->eBugException = NULL;

    vm->pkgs = NULL;

    vm->encodings = NULL;

    vm->thread = NULL;

    return vm;
}

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogVm* vm = ptr;

#define KEEP_MEMBER(member)     vm->member = (*keeper)(env, vm->member)
    KEEP_MEMBER(id2name);
    KEEP_MEMBER(name2id);

    KEEP_MEMBER(cObject);
    KEEP_MEMBER(cKlass);
    KEEP_MEMBER(cInt);
    KEEP_MEMBER(cString);
    KEEP_MEMBER(cPackage);
    KEEP_MEMBER(cBool);
    KEEP_MEMBER(cBuiltinBoundMethod);
    KEEP_MEMBER(cBoundMethod);
    KEEP_MEMBER(cBuiltinUnboundMethod);
    KEEP_MEMBER(cUnboundMethod);
    KEEP_MEMBER(cPackageBlock);
    KEEP_MEMBER(cNil);

    KEEP_MEMBER(eException);
    KEEP_MEMBER(eBugException);

    KEEP_MEMBER(pkgs);

    KEEP_MEMBER(encodings);

    KEEP_MEMBER(thread);
#undef KEEP_MEMBER
}

static void* 
keep_object(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }

    CopyingHeader* header = (CopyingHeader*)ptr - 1;
    if (header->forwarding_addr != NULL) {
        return (CopyingHeader*)header->forwarding_addr + 1;
    }

    YogVm* vm = ENV_VM(env);
    unsigned char* dest = vm->unscanned;
    size_t size = header->size;
    memcpy(dest, header, size);

    header->forwarding_addr = dest;

    vm->unscanned += size;

    return (CopyingHeader*)dest + 1;
}

static void 
free_heap(YogVm* vm) 
{
    YogHeap* heap = vm->heap;
    while (heap != NULL) {
        YogHeap* next = heap->next;

        memset(heap->base, 0xfe, heap->size);
        free(heap->base);
        free(heap);

        heap = next;
    }
}

void 
YogVm_gc(YogEnv* env, YogVm* vm) 
{
    unsigned int used_size = 0;
    YogHeap* heap = vm->heap;
    while (heap != NULL) {
        unsigned int size = heap->free - heap->base;
        used_size += size;
        heap = heap->next;
    }

#define GROW_RATIO  (1.1)
    unsigned int new_size = align(GROW_RATIO * used_size);
#undef GROW_RATIO
    YogHeap* to_space = YogHeap_new(new_size, NULL);

    vm->scanned = vm->unscanned = to_space->free;

    keep_children(env, vm, keep_object);

    while (vm->scanned != vm->unscanned) {
        CopyingHeader* header = (CopyingHeader*)vm->scanned;
        ChildrenKeeper keeper = header->keeper;
        if (keeper != NULL) {
            (*keeper)(env, header + 1, keep_object);
        }

        vm->scanned += header->size;
    }

    free_heap(vm);

    to_space->free = vm->unscanned;
    vm->heap = to_space;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
