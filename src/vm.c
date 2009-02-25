#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include "gc.h"
#include "yog/block.h"
#include "yog/bool.h"
#include "yog/builtins.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/int.h"
#include "yog/method.h"
#include "yog/nil.h"
#include "yog/regexp.h"
#include "yog/st.h"
#include "yog/yog.h"

#define PAGE_SIZE   4096

struct GcObjectStat {
    unsigned int survive_num;
};

typedef struct GcObjectStat GcObjectStat;

struct YogMarkSweepCompactFreeList {
    struct YogMarkSweepCompactFreeList* next;
};

typedef struct YogMarkSweepCompactFreeList YogMarkSweepCompactFreeList;

struct YogMarkSweepCompactChunk {
    struct YogMarkSweepCompactChunk* next;
    struct YogMarkSweepCompactFreeList* pages;
};

typedef struct YogMarkSweepCompactChunk YogMarkSweepCompactChunk;

struct YogMarkSweepCompactPage {
    struct YogMarkSweepCompactPage* next;
    size_t obj_size;
    unsigned int num_obj;
    unsigned int num_obj_avail;
    struct YogMarkSweepCompactFreeList* freelist;
    struct YogMarkSweepCompactChunk* chunk;
};

typedef struct YogMarkSweepCompactPage YogMarkSweepCompactPage;

struct YogMarkSweepCompactHeader {
    struct GcObjectStat stat;
    struct YogMarkSweepCompactHeader* prev;
    struct YogMarkSweepCompactHeader* next;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    BOOL marked;
    size_t size;
};

typedef struct YogMarkSweepCompactHeader YogMarkSweepCompactHeader;

struct YogHeap {
    size_t size;
    unsigned char* base;
    unsigned char* free;
    struct YogHeap* next;
};

typedef struct YogHeap YogHeap;

struct YogMarkSweepHeader {
    struct GcObjectStat stat;
    struct YogMarkSweepHeader* prev;
    struct YogMarkSweepHeader* next;
    unsigned int size;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    BOOL marked;
};

typedef struct YogMarkSweepHeader YogMarkSweepHeader;

struct CopyingHeader {
    struct GcObjectStat stat;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    void* forwarding_addr;
    size_t size;
};

typedef struct CopyingHeader CopyingHeader;

struct BdwHeader {
    Finalizer finalizer;
};

typedef struct BdwHeader BdwHeader;

#define SURVIVE_NUM_UNIT    8

static void 
GcObjectStat_increment_survive_num(GcObjectStat* stat) 
{
    stat->survive_num++;
}

static void 
reset_total_object_count(YogVm* vm) 
{
    vm->gc_stat.total_obj_num = 0;
}

static void 
reset_living_object_count(YogVm* vm)
{
    int i;
    for (i = 0; i < SURVIVE_INDEX_MAX; i++) {
        vm->gc_stat.living_obj_num[i] = 0;
    }
}

static void 
increment_total_object_number(YogVm* vm) 
{
    vm->gc_stat.total_obj_num++;
}

static void 
increment_living_object_number(YogVm* vm, unsigned int survive_num) 
{
    int index = survive_num / SURVIVE_NUM_UNIT;
    if (SURVIVE_INDEX_MAX  - 1 < index) {
        index = SURVIVE_INDEX_MAX - 1;
    }
    vm->gc_stat.living_obj_num[index]++;
}

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
        YOG_BUG(env, "can't find symbol (0x%x)", id);
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

static void 
GcObjectStat_initialize(GcObjectStat* stat) 
{
    stat->survive_num = 0;
}

static void* 
alloc_mem_copying(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
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

    CopyingHeader* header = (CopyingHeader*)heap->free;
    GcObjectStat_initialize(&header->stat);
    header->keeper = keeper;
    header->finalizer = finalizer;
    header->forwarding_addr = NULL;
    header->size = aligned_size;

    heap->free += aligned_size;

    increment_total_object_number(vm);

    return header + 1;
}

static void* 
alloc_mem_mark_sweep(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    size_t total_size = size + sizeof(YogMarkSweepHeader);
    YogMarkSweepHeader* header = malloc(total_size);
    initialize_memory(header, total_size);
    GcObjectStat_initialize(&header->stat);

    header->prev = NULL;
    header->next = vm->gc.mark_sweep.header;
    if (vm->gc.mark_sweep.header != NULL) {
        vm->gc.mark_sweep.header->prev = header;
    }
    vm->gc.mark_sweep.header = header;

    header->size = total_size;
    header->keeper = keeper;
    header->finalizer = finalizer;
    header->marked = FALSE;

    if (!vm->disable_gc) {
        vm->gc.mark_sweep.allocated_size += total_size;
        if (vm->gc.mark_sweep.threshold < vm->gc.mark_sweep.allocated_size) {
            vm->need_gc = TRUE;
        }
    }

    increment_total_object_number(ENV_VM(env));

    return header + 1;
}

void* 
YogVm_realloc(YogEnv* env, YogVm* vm, void* ptr, size_t size) 
{
    return (*vm->realloc_mem)(env, vm, ptr, size);
}

void* 
YogVm_alloc(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    vm->gc_stat.num_alloc++;

    return (*vm->alloc_mem)(env, vm, keeper, finalizer, size);
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

    GcObjectStat_increment_survive_num(&header->stat);
    increment_living_object_number(ENV_VM(env), header->stat.survive_num);
    increment_total_object_number(ENV_VM(env));

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
finalize_copying(YogEnv* env, YogVm* vm) 
{
    YogHeap* heap = vm->gc.copying.heap;
    while (heap != NULL) {
        unsigned char* ptr = heap->base;
        unsigned char* to = heap->free;
        while (ptr < to) {
            CopyingHeader* header = (CopyingHeader*)ptr;
            if (header->forwarding_addr == NULL) {
                if (header->finalizer != NULL) {
                    (*header->finalizer)(env, header + 1);
                }
            }

            ptr += header->size;
        }

        heap = heap->next;
    }
}

static void 
free_mem_copying(YogEnv* env, YogVm* vm) 
{
    finalize_copying(env, vm);
    free_heap(vm);
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

    free_mem_copying(env, vm);

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
        GcObjectStat_increment_survive_num(&header->stat);
        increment_living_object_number(ENV_VM(env), header->stat.survive_num);
        increment_total_object_number(ENV_VM(env));
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
finalize_mark_sweep(YogEnv* env, YogMarkSweepHeader* header) 
{
    if (header->finalizer != NULL) {
        (*header->finalizer)(env, header + 1);
    }
}

static void* 
keep_object_mark_sweep_compact(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }

    YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)ptr - 1;
    if (!header->marked) {
        GcObjectStat_increment_survive_num(&header->stat);
        increment_living_object_number(ENV_VM(env), header->stat.survive_num);
        increment_total_object_number(ENV_VM(env));
        header->marked = TRUE;

        ChildrenKeeper keeper = header->keeper;
        if (keeper != NULL) {
            (*keeper)(env, ptr, keep_object_mark_sweep_compact);
        }
    }

    return ptr;
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
            finalize_mark_sweep(env, header);

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
free_mem_mark_sweep(YogEnv* env, YogVm* vm) 
{
    YogMarkSweepHeader* header = vm->gc.mark_sweep.header;
    while (header != NULL) {
        YogMarkSweepHeader* next = header->next;

        finalize_mark_sweep(env, header);
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

static void 
bdw_finalizer(void* obj, void* client_data)
{
    BdwHeader* header = obj;
    YogEnv* env = client_data;
    (*header->finalizer)(env, header + 1);
}

static void* 
alloc_mem_bdw(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    unsigned int total_size = size + sizeof(BdwHeader);
    BdwHeader* header = GC_MALLOC(total_size);
    initialize_memory(header, total_size);

    header->finalizer = finalizer;

    if (finalizer != NULL) {
        GC_REGISTER_FINALIZER(header, bdw_finalizer, env, 0, 0);
    }

    return header + 1;
}

static void 
free_mem_bdw(YogEnv* env, YogVm* vm) 
{
    /* empty */
}

static void* 
realloc_mem_bdw(YogEnv* env, YogVm* vm, void* ptr, size_t size) 
{
    return GC_REALLOC(ptr, size);
}

#define REALLOC(type)   do { \
    type* header = (type*)ptr - 1; \
    size_t total_size = size + sizeof(type); \
    if (total_size < header->size) { \
        return ptr; \
    } \
\
    void* dest = YogVm_alloc(env, vm, header->keeper, header->finalizer, size); \
    memcpy(dest, ptr, header->size - sizeof(type)); \
\
    return dest; \
} while (0)

static void* 
realloc_mem_copying(YogEnv* env, YogVm* vm, void* ptr, size_t size)
{
    REALLOC(CopyingHeader);
}

static void*
realloc_mem_mark_sweep(YogEnv* env, YogVm* vm, void* ptr, size_t size)
{
    REALLOC(YogMarkSweepHeader);
}

#undef REALLOC

static void 
initialize_mark_sweep_compact(YogEnv* env, YogVm* vm) 
{
    vm->gc.mark_sweep_compact.heap.chunks = NULL;

    unsigned int i;
    for (i = 0; i < MARK_SWEEP_COMPACT_NUM_SIZE; i++) {
        vm->gc.mark_sweep_compact.heap.pages[i] = NULL;
    }

    unsigned int sizes[] = { 32, 64, 128, 256, 512, 1024, 2048, };
    unsigned int index = 0;
    unsigned int size;
    for (size = 0; size < MARK_SWEEP_COMPACT_SIZE2INDEX_SIZE; size++) {
        if (sizes[index] < size) {
            index++;
        }
        vm->gc.mark_sweep_compact.heap.size2index[size] = index;
    }

    for (i = 0; i < MARK_SWEEP_COMPACT_NUM_SIZE; i++) {
        vm->gc.mark_sweep_compact.heap.freelist_size[i] = sizes[i];
    }

    vm->gc.mark_sweep_compact.header = NULL;
}

static void 
finalize_mark_sweep_compact(YogEnv* env, YogMarkSweepCompactHeader* header) 
{
    if (header->finalizer != NULL) {
        (*header->finalizer)(env, header + 1);
    }
}

static void 
YogMarkSweepCompactHeader_delete(YogVm* vm, YogMarkSweepCompactHeader* header) 
{
    size_t size = header->size;
    destroy_memory(header, size);

    YogMarkSweepCompactPage* page = (YogMarkSweepCompactPage*)((uintptr_t)header & ~(PAGE_SIZE - 1));
    page->num_obj_avail++;
    YogMarkSweepCompactHeap* heap = &vm->gc.mark_sweep_compact.heap;
    unsigned int index = heap->size2index[size];
    if (page->num_obj_avail == page->num_obj) {
        YogMarkSweepCompactPage* pages = heap->pages[index];
        if (pages == page) {
            heap->pages[index] = pages->next;
        }
        else {
            while (pages != NULL) {
                if (pages->next == page) {
                    pages->next = page->next;
                    break;
                }
                pages = pages->next;
            }
        }

        YogMarkSweepCompactChunk* chunk = page->chunk;
        ((YogMarkSweepCompactFreeList*)page)->next = chunk->pages;
        chunk->pages = (YogMarkSweepCompactFreeList*)page;
    }
    else {
        if (page->num_obj_avail == 1) {
            page->next = heap->pages[index];
            heap->pages[index] = page;
        }
        ((YogMarkSweepCompactFreeList*)header)->next = page->freelist;
        page->freelist = (YogMarkSweepCompactFreeList*)header;
    }
}

static void 
mark_sweep_compact_gc(YogEnv* env, YogVm* vm) 
{
    YogMarkSweepCompactHeader* header = vm->gc.mark_sweep_compact.header;
    while (header != NULL) {
        header->marked = FALSE;
        header = header->next;
    }

    keep_children(env, vm, keep_object_mark_sweep_compact);

    header = vm->gc.mark_sweep_compact.header;
    while (header != NULL) {
        YogMarkSweepCompactHeader* next = header->next;

        if (!header->marked) {
            finalize_mark_sweep_compact(env, header);

            if (header->prev != NULL) {
                header->prev->next = next;
            }
            else {
                vm->gc.mark_sweep_compact.header = next;
            }
            if (next != NULL) {
                next->prev = header->prev;
            }

            YogMarkSweepCompactHeader_delete(vm, header);
        }

        header = next;
    }

    /**
     * TODO: free large objects.
     */

    vm->gc.mark_sweep_compact.allocated_size = 0;
}

static void* 
alloc_mem_mark_sweep_compact(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    size_t total_size = size + sizeof(YogMarkSweepCompactHeader);
    vm->gc.mark_sweep_compact.allocated_size += total_size;
    if (!vm->disable_gc) {
        if (vm->gc.mark_sweep_compact.threshold < vm->gc.mark_sweep_compact.allocated_size) {
            vm->need_gc = TRUE;
        }
    }

#define IS_SMALL_OBJECT(size) (size < (MARK_SWEEP_COMPACT_SIZE2INDEX_SIZE - 1))
    if (IS_SMALL_OBJECT(total_size)) {
        unsigned int index = vm->gc.mark_sweep_compact.heap.size2index[total_size];
        YogMarkSweepCompactPage* page = vm->gc.mark_sweep_compact.heap.pages[index];
        if (page == NULL) {
            YogMarkSweepCompactChunk* chunk = vm->gc.mark_sweep_compact.heap.chunks;
            if (chunk == NULL) {
                chunk = malloc(sizeof(YogMarkSweepCompactChunk));

                size_t chunk_size = vm->gc.mark_sweep_compact.heap.chunk_size;
                size_t mmap_size = chunk_size + PAGE_SIZE;
                unsigned char* mmap_begin = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                YOG_ASSERT(env, mmap_begin != MAP_FAILED, "mmap failure");
                unsigned char* chunk_begin = (unsigned char*)(((uintptr_t)mmap_begin + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
                if (mmap_begin != chunk_begin) {
                    munmap(mmap_begin, chunk_begin - mmap_begin);
                }
                unsigned char* mmap_end = mmap_begin + mmap_size;
                unsigned char* chunk_end = chunk_begin + chunk_size;
                if (mmap_end != chunk_end) {
                    munmap(chunk_end, mmap_end - chunk_end);
                }

                YogMarkSweepCompactFreeList* pages = (YogMarkSweepCompactFreeList*)mmap_begin;
                unsigned int num_pages = chunk_size / PAGE_SIZE;
                unsigned int i;
                for (i = 0; i < num_pages - 1; i++) {
                    pages->next = (YogMarkSweepCompactFreeList*)((unsigned char*)pages + PAGE_SIZE);
                    pages = pages->next;
                }
                pages->next = NULL;

                chunk->pages = (YogMarkSweepCompactFreeList*)mmap_begin;

                chunk->next = NULL;
                vm->gc.mark_sweep_compact.heap.chunks = chunk;
            }

            page = (YogMarkSweepCompactPage*)chunk->pages;
            chunk->pages = chunk->pages->next;
            if (chunk->pages == NULL) {
                vm->gc.mark_sweep_compact.heap.chunks = chunk->next;
            }
            vm->gc.mark_sweep_compact.heap.pages[index] = page;

            unsigned int size = vm->gc.mark_sweep_compact.heap.freelist_size[index];
            unsigned int num_obj = (PAGE_SIZE - sizeof(YogMarkSweepCompactPage)) / size;
            YogMarkSweepCompactFreeList* obj = (YogMarkSweepCompactFreeList*)(page + 1);
            page->freelist = obj;
            unsigned int i;
            for (i = 0; i < num_obj - 1; i++) {
                obj->next = (YogMarkSweepCompactFreeList*)((unsigned char*)obj + size);
                obj = obj->next;
            }
            obj->next = NULL;

            page->next = NULL;
            page->obj_size = size;
            page->num_obj = num_obj;
            page->num_obj_avail = num_obj;
            page->chunk = chunk;
        }

        YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)page->freelist;
        page->freelist = page->freelist->next;
        page->num_obj_avail--;
        if (page->num_obj_avail == 0) {
            vm->gc.mark_sweep_compact.heap.pages[index] = page->next;
        }

        initialize_memory(header, total_size);
        GcObjectStat_initialize(&header->stat);
        header->prev = NULL;
        header->next = vm->gc.mark_sweep_compact.header;
        if (vm->gc.mark_sweep_compact.header != NULL) {
            vm->gc.mark_sweep_compact.header->prev = header;
        }
        vm->gc.mark_sweep_compact.header = header;

        header->size = total_size;
        header->keeper = keeper;
        header->finalizer = finalizer;
        header->marked = FALSE;

        return header + 1;
    }
    else {
        /* TODO */
        abort();
    }
#undef IS_SMALL_OBJECT

    return NULL;
}

static void 
free_mem_mark_sweep_compact(YogEnv* env, YogVm* vm) 
{
    /* TODO */
}

void 
YogVm_init(YogVm* vm, YogGcType gc)
{
    vm->gc_stress = FALSE;
    vm->disable_gc = FALSE;

    vm->need_gc = FALSE;
    switch (gc) {
    case GC_BDW:
        vm->init_gc = initialize_bdw;
        vm->exec_gc = bdw_gc;
        vm->alloc_mem = alloc_mem_bdw;
        vm->realloc_mem = realloc_mem_bdw;
        vm->free_mem = free_mem_bdw;
        break;
    case GC_COPYING:
        vm->init_gc = initialize_copying;
        vm->exec_gc = copying_gc;
        vm->alloc_mem = alloc_mem_copying;
        vm->realloc_mem = realloc_mem_copying;
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
        vm->realloc_mem = realloc_mem_mark_sweep;
        vm->free_mem = free_mem_mark_sweep;
        vm->gc.mark_sweep.header = NULL;
        vm->gc.mark_sweep.threshold = 0;
        vm->gc.mark_sweep.allocated_size = 0;
        break;
    case GC_MARK_SWEEP_COMPACT:
        vm->init_gc = initialize_mark_sweep_compact;
        vm->exec_gc = mark_sweep_compact_gc;
        vm->alloc_mem = alloc_mem_mark_sweep_compact;
        vm->realloc_mem = NULL;
        vm->free_mem = free_mem_mark_sweep_compact;
        vm->gc.mark_sweep_compact.heap.chunks = NULL;
        vm->gc.mark_sweep_compact.heap.large_obj = NULL;
        vm->gc.mark_sweep_compact.heap.chunk_size = 0;
        vm->gc.mark_sweep_compact.threshold = 0;
        vm->gc.mark_sweep_compact.allocated_size = 0;
        break;
    default:
        fprintf(stderr, "Unknown GC type.");
        abort();
        break;
    }
    vm->gc_stat.print = FALSE;
    vm->gc_stat.duration_total = 0;
    reset_living_object_count(vm);
    reset_total_object_count(vm);
    vm->gc_stat.num_alloc = 0;

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

static void 
print_gc_statistics(YogVm* vm, unsigned int duration) 
{
    printf("--- GC infomation ---\n");
    printf("Duration[usec] %d\n", duration);

    printf("Servive #");
    int i;
    for (i = 0; i < SURVIVE_INDEX_MAX - 1; i++) {
        printf(" %2d-%2d", SURVIVE_NUM_UNIT * i + 1, SURVIVE_NUM_UNIT * (i + 1));
    }
    printf(" %d+\n", SURVIVE_NUM_UNIT * (SURVIVE_INDEX_MAX - 1) + 1);
    printf("Objects #");
    for (i = 0; i < SURVIVE_INDEX_MAX; i++) {
        printf(" %5d", vm->gc_stat.living_obj_num[i]);
    }
    printf("\n");
}

void 
YogVm_gc(YogEnv* env, YogVm* vm) 
{
    reset_living_object_count(vm);
    reset_total_object_count(vm);

    struct timeval begin;
    gettimeofday(&begin, NULL);

    (*vm->exec_gc)(env, vm);

    struct timeval end;
    gettimeofday(&end, NULL);
    unsigned int duration = 1000000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec);

    vm->gc_stat.duration_total += duration;

    if (vm->gc_stat.print) {
        print_gc_statistics(vm, duration);
    }
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

void 
YogVm_config_mark_sweep_compact(YogEnv* env, YogVm* vm, size_t chunk_size, size_t threshold) 
{
    vm->gc.mark_sweep_compact.heap.chunk_size = chunk_size;
    vm->gc.mark_sweep_compact.threshold = threshold;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
