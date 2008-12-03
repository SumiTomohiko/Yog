#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "gc.h"
#include "yog/encoding.h"
#include "yog/regexp.h"
#include "yog/st.h"
#include "yog/yog.h"

struct YogHeap {
    size_t size;
    unsigned char* base;
    unsigned char* free;
    struct YogHeap* next;
};

typedef struct YogHeap YogHeap;

struct YogMarkSweepHeader {
    struct YogMarkSweepHeader* prev;
    struct YogMarkSweepHeader* next;
    unsigned int size;
    ChildrenKeeper keeper;
    BOOL marked;
};

typedef struct YogMarkSweepHeader YogMarkSweepHeader;

struct CopyingHeader {
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

static void 
initialize_memory(void* ptr, size_t size) 
{
    memset(ptr, 0xcb, size);
}

static YogHeap* 
YogHeap_new(size_t size, YogHeap* next) 
{
    YogHeap* heap = malloc(sizeof(YogHeap));
    YOG_ASSERT(NULL, heap != NULL, "Can' allocate memory for heap.");

    void* ptr = malloc(size);
    YOG_ASSERT(NULL, heap != NULL, "Can' allocate memory for heap.");
    initialize_memory(ptr, size);

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

static void* 
alloc_mem_copying(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, size_t size)
{
    size_t needed_size = size + sizeof(CopyingHeader);
    size_t aligned_size = align(needed_size);

    YogHeap* heap = vm->gc.copying.heap;
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
        vm->gc.copying.heap = heap = YogHeap_new(allocate_size, heap);
        if (!vm->disable_gc) {
            vm->need_gc = TRUE;
        }
    }

    CopyingHeader* head = (CopyingHeader*)heap->free;
    head->keeper = keeper;
    head->forwarding_addr = NULL;
    head->size = aligned_size;

    heap->free += aligned_size;

    return head + 1;
}

static void* 
alloc_mem_mark_sweep(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, size_t size)
{
    size_t total_size = size + sizeof(YogMarkSweepHeader);
    YogMarkSweepHeader* header = malloc(total_size);
    initialize_memory(header, total_size);

    header->prev = NULL;
    header->next = vm->gc.mark_sweep.header;
    if (vm->gc.mark_sweep.header != NULL) {
        vm->gc.mark_sweep.header->prev = header;
    }
    vm->gc.mark_sweep.header = header;

    header->size = total_size;
    header->keeper = keeper;
    header->marked = FALSE;

    if (!vm->disable_gc) {
        vm->gc.mark_sweep.allocated_size += total_size;
        if (vm->gc.mark_sweep.threshold < vm->gc.mark_sweep.allocated_size) {
            vm->need_gc = TRUE;
        }
    }

    return header + 1;
}

void* 
YogVm_alloc(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, size_t size)
{
    return (*vm->alloc_mem)(env, vm, keeper, size);
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
    vm->cRegexp = YogRegexp_klass_new(env);
    vm->cMatch = YogMatch_klass_new(env);
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
#define EXCEPTION_NEW(member, name)  do { \
    vm->member = YogKlass_new(env, name, vm->eException); \
} while (0)
    EXCEPTION_NEW(eBugException, "BugException");
    EXCEPTION_NEW(eTypeError, "TypeError");
    EXCEPTION_NEW(eIndexError, "IndexError");
#undef EXCEPTION_NEW
}

void 
YogVm_boot(YogEnv* env, YogVm* vm) 
{
    (*vm->init_gc)(env, vm);

    setup_symbol_tables(env, vm);
    setup_basic_klass(env, vm);
    setup_klasses(env, vm);
    setup_exceptions(env, vm);

    vm->pkgs = YogTable_new_symbol_table(env);
    setup_builtins(env, vm);

    vm->encodings = YogTable_new_symbol_table(env);
    setup_encodings(env, vm);
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
    KEEP_MEMBER(cRegexp);
    KEEP_MEMBER(cMatch);
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
    KEEP_MEMBER(eTypeError);
    KEEP_MEMBER(eIndexError);

    KEEP_MEMBER(pkgs);

    KEEP_MEMBER(encodings);

    KEEP_MEMBER(thread);
#undef KEEP_MEMBER
}

static void* 
keep_object_copying(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }

    CopyingHeader* header = (CopyingHeader*)ptr - 1;
    if (header->forwarding_addr != NULL) {
        return (CopyingHeader*)header->forwarding_addr + 1;
    }

    YogVm* vm = ENV_VM(env);
    unsigned char* dest = vm->gc.copying.unscanned;
    size_t size = header->size;
    memcpy(dest, header, size);

    header->forwarding_addr = dest;

    vm->gc.copying.unscanned += size;

    return (CopyingHeader*)dest + 1;
}

static void 
destroy_memory(void* ptr, size_t size) 
{
    memset(ptr, 0xfe, size);
}

static void 
free_heap(YogVm* vm) 
{
    YogHeap* heap = vm->gc.copying.heap;
    while (heap != NULL) {
        YogHeap* next = heap->next;

        destroy_memory(heap->base, heap->size);
        free(heap->base);
        free(heap);

        heap = next;
    }
}

static void 
copying_gc(YogEnv* env, YogVm* vm) 
{
    unsigned int used_size = 0;
    YogHeap* heap = vm->gc.copying.heap;
    while (heap != NULL) {
        unsigned int size = heap->free - heap->base;
        used_size += size;
        heap = heap->next;
    }

#define GROW_RATIO  (1.1)
    unsigned int new_size = align(GROW_RATIO * used_size);
#undef GROW_RATIO
    YogHeap* to_space = YogHeap_new(new_size, NULL);

    vm->gc.copying.scanned = vm->gc.copying.unscanned = to_space->free;

    keep_children(env, vm, keep_object_copying);

    while (vm->gc.copying.scanned != vm->gc.copying.unscanned) {
        CopyingHeader* header = (CopyingHeader*)vm->gc.copying.scanned;
        ChildrenKeeper keeper = header->keeper;
        if (keeper != NULL) {
            (*keeper)(env, header + 1, keep_object_copying);
        }

        vm->gc.copying.scanned += header->size;
    }

    free_heap(vm);

    to_space->free = vm->gc.copying.unscanned;
    vm->gc.copying.heap = to_space;
}

static void 
initialize_copying(YogEnv* env, YogVm* vm) 
{
    vm->gc.copying.heap = YogHeap_new(vm->gc.copying.init_heap_size, NULL);
}

static void 
initialize_mark_sweep(YogEnv* env, YogVm* vm) 
{
    vm->gc.mark_sweep.header = NULL;
    vm->gc.mark_sweep.allocated_size = 0;
}

static void* 
keep_object_mark_sweep(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }

    YogMarkSweepHeader* header = (YogMarkSweepHeader*)ptr - 1;
    if (!header->marked) {
        header->marked = TRUE;

        ChildrenKeeper keeper = header->keeper;
        if (keeper != NULL) {
            (*keeper)(env, ptr, keep_object_mark_sweep);
        }
    }

    return ptr;
}

static void 
YogMarkSweepHeader_delete(YogMarkSweepHeader* header) 
{
    destroy_memory(header, header->size);
    free(header);
}

static void 
mark_sweep_gc(YogEnv* env, YogVm* vm) 
{
    YogMarkSweepHeader* header = vm->gc.mark_sweep.header;
    while (header != NULL) {
        header->marked = FALSE;
        header = header->next;
    }

    keep_children(env, vm, keep_object_mark_sweep);

    header = vm->gc.mark_sweep.header;
    while (header != NULL) {
        YogMarkSweepHeader* next = header->next;

        if (!header->marked) {
            if (header->prev != NULL) {
                header->prev->next = next;
            }
            else {
                vm->gc.mark_sweep.header = next;
            }
            if (next != NULL) {
                next->prev = header->prev;
            }

            YogMarkSweepHeader_delete(header);
        }

        header = next;
    }

    vm->gc.mark_sweep.allocated_size = 0;
}

static void 
free_mem_copying(YogEnv* env, YogVm* vm) 
{
    free_heap(vm);
}

static void 
free_mem_mark_sweep(YogEnv* env, YogVm* vm) 
{
    YogMarkSweepHeader* header = vm->gc.mark_sweep.header;
    while (header != NULL) {
        YogMarkSweepHeader* next = header->next;

        YogMarkSweepHeader_delete(header);

        header = next;
    }
}

static void 
initialize_bdw(YogEnv* env, YogVm* vm) 
{
    /* empty */
}

static void 
bdw_gc(YogEnv* env, YogVm* vm) 
{
    /* empty */
}

void* 
alloc_mem_bdw(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, size_t size)
{
    void* ptr = GC_MALLOC(size);
    initialize_memory(ptr, size);

    return ptr;
}

static void 
free_mem_bdw(YogEnv* env, YogVm* vm) 
{
    /* empty */
}

void 
YogVm_init(YogVm* vm, YogGcType gc)
{
    vm->always_gc = FALSE;
    vm->disable_gc = FALSE;

    vm->need_gc = FALSE;
    switch (gc) {
    case GC_BDW:
        vm->init_gc = initialize_bdw;
        vm->exec_gc = bdw_gc;
        vm->alloc_mem = alloc_mem_bdw;
        vm->free_mem = free_mem_bdw;
        break;
    case GC_COPYING:
        vm->init_gc = initialize_copying;
        vm->exec_gc = copying_gc;
        vm->alloc_mem = alloc_mem_copying;
        vm->free_mem = free_mem_copying;
        vm->gc.copying.init_heap_size = 0;
        vm->gc.copying.heap = NULL;
        vm->gc.copying.scanned = NULL;
        vm->gc.copying.unscanned = NULL;
        break;
    case GC_MARK_SWEEP:
        vm->init_gc = initialize_mark_sweep;
        vm->exec_gc = mark_sweep_gc;
        vm->alloc_mem = alloc_mem_mark_sweep;
        vm->free_mem = free_mem_mark_sweep;
        vm->gc.mark_sweep.header = NULL;
        vm->gc.mark_sweep.threshold = 0;
        vm->gc.mark_sweep.allocated_size = 0;
        break;
    default:
        fprintf(stderr, "Unknown GC type.\n");
        return;
        break;
    }

    vm->next_id = 0;
    vm->id2name = NULL;
    vm->name2id = NULL;

    vm->cObject = NULL;
    vm->cKlass = NULL;
    vm->cInt = NULL;
    vm->cString = NULL;
    vm->cRegexp = NULL;
    vm->cMatch = NULL;
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
    vm->eTypeError = NULL;
    vm->eIndexError = NULL;

    vm->pkgs = NULL;

    vm->encodings = NULL;

    vm->thread = NULL;
}

void 
YogVm_gc(YogEnv* env, YogVm* vm) 
{
    (*vm->exec_gc)(env, vm);
}

void 
YogVm_config_copying(YogEnv* env, YogVm* vm, unsigned int init_heap_size) 
{
    vm->gc.copying.init_heap_size = init_heap_size;
}

void 
YogVm_config_mark_sweep(YogEnv* env, YogVm* vm, size_t threshold) 
{
    vm->gc.mark_sweep.threshold = threshold;
}

void 
YogVm_delete(YogEnv* env, YogVm* vm) 
{
    (*vm->free_mem)(env, vm);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
