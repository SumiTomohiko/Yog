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

#define PAGE_SIZE       4096
#define PTR2PAGE(p)     ((YogMarkSweepCompactPage*)((uintptr_t)p & ~(PAGE_SIZE - 1)))

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
    struct YogMarkSweepCompactChunk* all_chunks_next;
    struct YogMarkSweepCompactFreeList* pages;
    struct YogMarkSweepCompactPage* first_page;
};

typedef struct YogMarkSweepCompactChunk YogMarkSweepCompactChunk;

struct YogMarkSweepCompactPage {
    unsigned int flags;
    struct YogMarkSweepCompactPage* next;
    size_t obj_size;
    unsigned int num_obj;
    unsigned int num_obj_avail;
    struct YogMarkSweepCompactFreeList* freelist;
    struct YogMarkSweepCompactChunk* chunk;
};

#define PAGE_USED           0x01
#define IS_PAGE_USED(p)     ((p)->flags & PAGE_USED)

typedef struct YogMarkSweepCompactPage YogMarkSweepCompactPage;

struct YogMarkSweepCompactHeader {
    unsigned int flags;
    struct GcObjectStat stat;
    struct YogMarkSweepCompactHeader* forwarding_addr;
    struct YogMarkSweepCompactHeader* prev;
    struct YogMarkSweepCompactHeader* next;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    BOOL marked;
    BOOL updated;
    size_t size;
};

#define OBJ_USED        0x01
#define IS_OBJ_USED(o)  ((o)->flags & OBJ_USED)

typedef struct YogMarkSweepCompactHeader YogMarkSweepCompactHeader;

struct YogCompactor {
    void (*callback)(struct YogEnv*, struct YogCompactor*, struct YogMarkSweepCompactHeader*); 
    struct YogMarkSweepCompactChunk* cur_chunk;
    struct YogMarkSweepCompactPage* next_page;
    struct YogMarkSweepCompactPage* cur_page[MARK_SWEEP_COMPACT_NUM_SIZE];
    unsigned int cur_index[MARK_SWEEP_COMPACT_NUM_SIZE];
};

typedef struct YogCompactor YogCompactor;

#if 0
struct YogHeap {
    size_t size;
    unsigned char* free;
    unsigned char items[0];
};

typedef struct YogHeap YogHeap;
#endif

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

#if 0
struct CopyingHeader {
    struct GcObjectStat stat;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    void* forwarding_addr;
    size_t size;
    unsigned int id;
};

typedef struct CopyingHeader CopyingHeader;
#endif

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
YogVm_register_package(YogEnv* env, YogVm* vm, const char* name, YogVal pkg) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, pkg);

    ID id = YogVm_intern(env, vm, name);
    YogVal key = ID2VAL(id);

    YogTable_add_direct(env, vm->pkgs, key, pkg);

    RETURN_VOID(env);
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
    SAVE_LOCALS(env);

    YogVal value = YUNDEF;
    if (YogTable_lookup_str(env, vm->name2id, name, &value)) {
        return VAL2ID(value);
    }

#if 0
    const char* s = YogString_dup(env, name);
    YogVal val = STR2VAL(s);
#endif
    /* TODO: dirty hack */
    size_t size = strlen(name);
    char buffer[size + 1];
    strcpy(buffer, name);

    YogCharArray* s = YogCharArray_new_str(env, buffer);
    YogVal val = PTR2VAL(s);
    PUSH_LOCAL(env, val);

    ID id = vm->next_id;
    YogVal symbol = ID2VAL(id);

    YogTable_add_direct(env, vm->name2id, val, symbol);
    YogTable_add_direct(env, vm->id2name, symbol, val);

    vm->next_id++;

    RETURN(env, id);
}

static void 
initialize_memory(void* ptr, size_t size) 
{
    memset(ptr, 0xcb, size);
}

#if 0
static YogHeap* 
YogHeap_new(size_t size)
{
    YogHeap* heap = malloc(sizeof(YogHeap) + size);
    YOG_ASSERT(NULL, heap != NULL, "Can' allocate memory for heap.");

    heap->size = size;
    heap->free = heap->items;
    initialize_memory(heap->items, size);

    return heap;
}

static size_t
round_size(size_t size) 
{
    size_t unit = sizeof(void*);
    return (size + unit - 1) & ~(unit - 1);
}
#endif

static void 
GcObjectStat_initialize(GcObjectStat* stat) 
{
    stat->survive_num = 0;
}

static void* 
alloc_mem_mark_sweep(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    if (!vm->disable_gc) {
        unsigned int threshold = vm->gc.mark_sweep.threshold;
        unsigned int allocated_size = vm->gc.mark_sweep.allocated_size;
        if ((threshold < allocated_size) || vm->gc_stress) {
            YogVm_gc(env, vm);
        }
    }

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

    vm->gc.mark_sweep.allocated_size += total_size;
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

    void* ptr = (*vm->alloc_mem)(env, vm, keeper, finalizer, size);

    return ptr;
}

static void 
setup_builtins(YogEnv* env, YogVm* vm) 
{
    YogVal builtins = YogBuiltins_new(env);
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
    YogVal cObject = YUNDEF;
    YogVal cKlass = YUNDEF;
    PUSH_LOCALS2(env, cObject, cKlass);

    cObject = YogKlass_new(env, "Object", YNIL);
    YogKlass_define_allocator(env, cObject, YogObj_allocate);

    cKlass = YogKlass_new(env, "Class", cObject);
    YogKlass_define_allocator(env, cKlass, YogKlass_allocate);

    OBJ_AS(YogBasicObj, cObject)->klass = cKlass;
    OBJ_AS(YogBasicObj, cKlass)->klass = cKlass;

    vm->cObject = cObject;
    vm->cKlass = cKlass;

    POP_LOCALS(env);
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
    /* TODO: changed not to use macro */
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
YogVm_initialize_gc(YogEnv* env, YogVm* vm) 
{
    (*vm->init_gc)(env, vm);
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

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogVm* vm = ptr;

    YogThread_keep_children(env, vm->thread, keeper);

#define KEEP_MEMBER(member)     do { \
    vm->member = YogVal_keep(env, vm->member, keeper); \
} while (0)
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
#undef KEEP_MEMBER
}

#if 0
static void* 
keep_object_copying(YogEnv* env, void* ptr) 
{
#if 0
#   define PRINT(...)   DPRINTF(__VA_ARGS__)
#else
#   define PRINT(...)
#endif
    if (ptr == NULL) {
        PRINT("exec_num=0x%08x, NULL->NULL", ENV_VM(env)->gc_stat.exec_num);
        return NULL;
    }

    CopyingHeader* header = (CopyingHeader*)ptr - 1;
    if (header->forwarding_addr != NULL) {
        PRINT("exec_num=0x%08x, id=0x%08x, %p->(%p)", ENV_VM(env)->gc_stat.exec_num, header->id, ptr, (CopyingHeader*)header->forwarding_addr + 1);
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

    PRINT("exec_num=0x%08x, id=0x%08x, %p->%p", ENV_VM(env)->gc_stat.exec_num, header->id, ptr, (CopyingHeader*)dest + 1);
    return (CopyingHeader*)dest + 1;
#undef PRINT
}
#endif

static void 
destroy_memory(void* ptr, size_t size) 
{
    memset(ptr, 0xfe, size);
}

#if 0
static void 
free_heap_internal(YogHeap* heap) 
{
    destroy_memory(heap->items, heap->size);
    free(heap);
}

static void 
free_heap(YogVm* vm) 
{
#define FREE_HEAP(heap)     do { \
    free_heap_internal((heap)); \
    (heap) = NULL; \
} while (0)
    FREE_HEAP(vm->gc.copying.active_heap);
    FREE_HEAP(vm->gc.copying.inactive_heap);
#undef FREE_HEAP
}

static void 
finalize_copying(YogEnv* env, YogVm* vm) 
{
    YogHeap* heap = vm->gc.copying.active_heap;

    unsigned char* ptr = heap->items;
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
}
#endif

static void 
free_mem_copying(YogEnv* env, YogVm* vm) 
{
    YogCopying_finalize(env, &vm->gc.copying);
}

#if 0
static void 
swap_heap(YogHeap** a, YogHeap** b) 
{
    YogHeap* tmp = *a;
    *a = *b;
    *b = tmp;
}

static void 
copying_gc(YogEnv* env, YogVm* vm) 
{
    YogHeap* from_space = vm->gc.copying.active_heap;
    YogHeap* to_space = vm->gc.copying.inactive_heap;
#if 0
#   define PRINT_HEAP(text, heap)   do { \
    DPRINTF("%s: exec_num=0x%08x, %p-%p", (text), vm->gc_stat.exec_num, (heap)->items, (unsigned char*)(heap)->items + (heap)->size); \
} while (0)
#else 
#   define PRINT_HEAP(text, heap)
#endif
    PRINT_HEAP("from-space", from_space);
    PRINT_HEAP("to-space", to_space);

    vm->gc.copying.scanned = vm->gc.copying.unscanned = to_space->items;

    keep_children(env, vm, keep_object_copying);

    while (vm->gc.copying.scanned != vm->gc.copying.unscanned) {
        CopyingHeader* header = (CopyingHeader*)vm->gc.copying.scanned;
        ChildrenKeeper keeper = header->keeper;
        if (keeper != NULL) {
            (*keeper)(env, header + 1, keep_object_copying);
        }

        vm->gc.copying.scanned += header->size;
    }

    finalize_copying(env, vm);

    size_t size = from_space->free - from_space->items;
    destroy_memory(from_space->items, size);

    to_space->free = vm->gc.copying.unscanned;

    swap_heap(&vm->gc.copying.active_heap, &vm->gc.copying.inactive_heap);
}
#endif

static void* 
alloc_mem_copying(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    return YogCopying_alloc(env, &vm->gc.copying, keeper, finalizer, size);
#if 0
    size_t needed_size = size + sizeof(CopyingHeader);
    size_t rounded_size = round_size(needed_size);
    vm->gc_stat.total_allocated_size += rounded_size;

    YogHeap* heap = vm->gc.copying.active_heap;;
#define REST_SIZE(heap)     ((heap)->size - ((heap)->free - (heap)->items))
    size_t rest_size = REST_SIZE(heap);
    if ((rest_size < rounded_size) || vm->gc_stress) {
        if (!vm->disable_gc) {
            YogVm_gc(env, vm);
            heap = vm->gc.copying.active_heap;
            rest_size = REST_SIZE(heap);
        }
        YOG_ASSERT(env, rounded_size <= rest_size, "out of memory");
    }
#undef REST_SIZE

    CopyingHeader* header = (CopyingHeader*)heap->free;
    GcObjectStat_initialize(&header->stat);
    header->keeper = keeper;
    header->finalizer = finalizer;
    header->forwarding_addr = NULL;
    header->size = rounded_size;
    static unsigned int id = 0;
    header->id = id++;

    heap->free += rounded_size;

    increment_total_object_number(vm);

    return header + 1;
#endif
}

static void 
initialize_copying(YogEnv* env, YogVm* vm) 
{
#if 0
    size_t heap_size = vm->gc.copying.init_heap_size;
    vm->gc.copying.active_heap = YogHeap_new(heap_size);
    vm->gc.copying.inactive_heap = YogHeap_new(heap_size);
#endif
#define HEAP_SIZE   (16 * 1024 * 1024)
    YogCopying_initialize(env, &vm->gc.copying, vm->gc_stress, HEAP_SIZE, vm, keep_children);
#undef HEAP_SIZE
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
#if 0
    REALLOC(CopyingHeader);
#endif
    return NULL;
}

static void*
realloc_mem_mark_sweep(YogEnv* env, YogVm* vm, void* ptr, size_t size)
{
    REALLOC(YogMarkSweepHeader);
}

#undef REALLOC

static void* 
alloc_mem_mark_sweep_compact(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    return YogMarkSweepCompact_alloc(env, &vm->gc.mark_sweep_compact, keeper, finalizer, size);
}

static void 
initialize_mark_sweep_compact(YogEnv* env, YogVm* vm) 
{
#if 0
#define CHUNK_SIZE (16 * 1024 * 1024)
#endif
#define CHUNK_SIZE (1 * 1024 * 1024)
    YogMarkSweepCompact_initialize(env, &vm->gc.mark_sweep_compact, CHUNK_SIZE, 0, vm, keep_children);
#undef CHUNK_SIZE
}

static void 
free_mem_mark_sweep_compact(YogEnv* env, YogVm* vm) 
{
    /* TODO */
}

#if 0
static void 
dump_mem_not_supported(YogEnv* env, YogVm* vm) 
{
    YOG_ASSERT(env, FALSE, "dumping memory is not supported for this GC");
}

static void 
dump_mem_copying(YogEnv* env, YogVm* vm) 
{
    printf("-------------------- memory dump --------------------\n");

    YogHeap* heap = vm->gc.copying.heap;
    unsigned char* ptr = heap->base;
    while (ptr < heap->free) {
        CopyingHeader* header = (CopyingHeader*)ptr;
        printf("address: %p-%p\n", header, ptr + header->size);
        printf("  keeper: %p\n", header->keeper);
        printf("  finalizer: %p\n", header->finalizer);
        printf("  forwarding_addr: %p\n", header->forwarding_addr);
        printf("  size: %u\n", header->size);
        printf("\n");

        ptr += header->size;
    }
}

void 
YogVm_dump_memory(YogEnv* env, YogVm* vm) 
{
    (*vm->dump_mem)(env, vm);
}
#endif

void 
YogVm_init(YogVm* vm, YogGcType gc)
{
    vm->gc_stress = FALSE;
    vm->disable_gc = FALSE;

    switch (gc) {
    case GC_BDW:
        vm->init_gc = initialize_bdw;
        vm->exec_gc = bdw_gc;
        vm->alloc_mem = alloc_mem_bdw;
        vm->realloc_mem = realloc_mem_bdw;
        vm->free_mem = free_mem_bdw;
#if 0
        vm->dump_mem = dump_mem_not_supported;
#endif
        break;
    case GC_COPYING:
        vm->init_gc = initialize_copying;
#if 0
        vm->exec_gc = copying_gc;
#endif
        vm->exec_gc = NULL;
        vm->alloc_mem = alloc_mem_copying;
#if 0
        vm->realloc_mem = realloc_mem_copying;
#endif
        vm->realloc_mem = NULL;
        vm->free_mem = free_mem_copying;
#if 0
        vm->dump_mem = dump_mem_copying;
        vm->gc.copying.init_heap_size = 0;
        vm->gc.copying.active_heap = NULL;
        vm->gc.copying.inactive_heap = NULL;
        vm->gc.copying.scanned = NULL;
        vm->gc.copying.unscanned = NULL;
#endif
        break;
    case GC_MARK_SWEEP:
        vm->init_gc = initialize_mark_sweep;
        vm->exec_gc = mark_sweep_gc;
        vm->alloc_mem = alloc_mem_mark_sweep;
        vm->realloc_mem = realloc_mem_mark_sweep;
        vm->free_mem = free_mem_mark_sweep;
#if 0
        vm->dump_mem = dump_mem_not_supported;
#endif
        vm->gc.mark_sweep.header = NULL;
        vm->gc.mark_sweep.threshold = 0;
        vm->gc.mark_sweep.allocated_size = 0;
        break;
    case GC_MARK_SWEEP_COMPACT:
        vm->init_gc = initialize_mark_sweep_compact;
#if 0
        vm->exec_gc = mark_sweep_compact_gc;
#endif
        vm->exec_gc = NULL;
        vm->alloc_mem = alloc_mem_mark_sweep_compact;
        vm->realloc_mem = NULL;
        vm->free_mem = free_mem_mark_sweep_compact;
#if 0
        vm->dump_mem = dump_mem_not_supported;
        vm->gc.mark_sweep_compact.heap.chunks = NULL;
        vm->gc.mark_sweep_compact.heap.all_chunks = NULL;
        vm->gc.mark_sweep_compact.heap.all_chunks_last = NULL;
        vm->gc.mark_sweep_compact.heap.large_obj = NULL;
        vm->gc.mark_sweep_compact.heap.chunk_size = 0;
        vm->gc.mark_sweep_compact.threshold = 0;
        vm->gc.mark_sweep_compact.allocated_size = 0;
#endif
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
    vm->gc_stat.total_allocated_size = 0;
    vm->gc_stat.exec_num = 0;

    vm->next_id = 0;
    vm->id2name = PTR2VAL(NULL);
    vm->name2id = PTR2VAL(NULL);

    vm->cObject = YUNDEF;
    vm->cKlass = YUNDEF;
    vm->cInt = YUNDEF;
    vm->cString = YUNDEF;
    vm->cRegexp = YUNDEF;
    vm->cMatch = YUNDEF;
    vm->cPackage = YUNDEF;
    vm->cBool = YUNDEF;
    vm->cBuiltinBoundMethod = YUNDEF;
    vm->cBoundMethod = YUNDEF;
    vm->cBuiltinUnboundMethod = YUNDEF;
    vm->cUnboundMethod = YUNDEF;
    vm->cPackageBlock = YUNDEF;
    vm->cNil = YUNDEF;

    vm->eException = YUNDEF;
    vm->eBugException = YUNDEF;
    vm->eTypeError = YUNDEF;
    vm->eIndexError = YUNDEF;

    vm->pkgs = PTR2VAL(NULL);

    vm->encodings = PTR2VAL(NULL);

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
    vm->gc_stat.exec_num++;

    if (vm->gc_stat.print) {
        print_gc_statistics(vm, duration);
    }
}

void 
YogVm_config_copying(YogEnv* env, YogVm* vm, unsigned int init_heap_size) 
{
#if 0
    vm->gc.copying.init_heap_size = init_heap_size;
#endif
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
    /* TODO: removed */
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
