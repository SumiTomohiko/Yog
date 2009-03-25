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

#if 0
struct GcObjectStat {
    unsigned int survive_num;
};

typedef struct GcObjectStat GcObjectStat;
#endif

struct BdwHeader {
    Finalizer finalizer;
};

typedef struct BdwHeader BdwHeader;

#define SURVIVE_NUM_UNIT    8

#if 0
static void 
GcObjectStat_increment_survive_num(GcObjectStat* stat) 
{
    stat->survive_num++;
}
#endif

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

#if 0
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
#endif

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
static void 
GcObjectStat_initialize(GcObjectStat* stat) 
{
    stat->survive_num = 0;
}
#endif

static void* 
alloc_mem_mark_sweep(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    return YogMarkSweep_alloc(env, &vm->gc.mark_sweep, keeper, finalizer, size);
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

static void 
free_mem_copying(YogEnv* env, YogVm* vm) 
{
    YogCopying_finalize(env, &vm->gc.copying);
}

static void* 
alloc_mem_copying(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    return YogCopying_alloc(env, &vm->gc.copying, keeper, finalizer, size);
}

static void 
free_mem_mark_sweep(YogEnv* env, YogVm* vm) 
{
    return YogMarkSweep_finalize(env, &vm->gc.mark_sweep);
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
alloc_mem_mark_sweep_compact(YogEnv* env, YogVm* vm, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    return YogMarkSweepCompact_alloc(env, &vm->gc.mark_sweep_compact, keeper, finalizer, size);
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

    switch (gc) {
    case GC_BDW:
        vm->alloc_mem = alloc_mem_bdw;
        vm->free_mem = free_mem_bdw;
        break;
    case GC_COPYING:
        vm->alloc_mem = alloc_mem_copying;
        vm->free_mem = free_mem_copying;
        break;
    case GC_MARK_SWEEP:
        vm->alloc_mem = alloc_mem_mark_sweep;
        vm->free_mem = free_mem_mark_sweep;
        break;
    case GC_MARK_SWEEP_COMPACT:
        vm->alloc_mem = alloc_mem_mark_sweep_compact;
        vm->free_mem = free_mem_mark_sweep_compact;
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

#if 0
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
#endif

#if 0
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
#endif

void 
YogVm_config_copying(YogEnv* env, YogVm* vm, unsigned int init_heap_size) 
{
    YogCopying_initialize(env, &vm->gc.copying, vm->gc_stress, init_heap_size, vm, keep_children);
}

void 
YogVm_config_mark_sweep(YogEnv* env, YogVm* vm, size_t threshold) 
{
    YogMarkSweep_initialize(env, &vm->gc.mark_sweep, threshold, vm, keep_children);
}

void 
YogVm_delete(YogEnv* env, YogVm* vm) 
{
    (*vm->free_mem)(env, vm);
}

void 
YogVm_config_mark_sweep_compact(YogEnv* env, YogVm* vm, size_t chunk_size, size_t threshold) 
{
    YogMarkSweepCompact_initialize(env, &vm->gc.mark_sweep_compact, chunk_size, threshold, vm, keep_children);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
