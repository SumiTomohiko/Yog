#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include "yog/block.h"
#include "yog/bool.h"
#include "yog/builtins.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/exception.h"
#include "yog/float.h"
#include "yog/gc.h"
#include "yog/gc/bdw.h"
#include "yog/int.h"
#include "yog/klass.h"
#include "yog/method.h"
#include "yog/misc.h"
#include "yog/nil.h"
#include "yog/package.h"
#include "yog/regexp.h"
#include "yog/st.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#if 0
#   define DEBUG(x)     x
#else
#   define DEBUG(x)
#endif

#define PAGE_SIZE       4096
#define PTR2PAGE(p)     ((YogMarkSweepCompactPage*)((uintptr_t)p & ~(PAGE_SIZE - 1)))

#if 0
struct GcObjectStat {
    unsigned int survive_num;
};

typedef struct GcObjectStat GcObjectStat;
#endif

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
    if (!YogTable_lookup(env, env->vm->id2name, sym, &val)) {
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

    YogVal s = YogCharArray_new_str(env, buffer);
    PUSH_LOCAL(env, s);

    ID id = vm->next_id;
    YogVal symbol = ID2VAL(id);

    YogTable_add_direct(env, vm->name2id, s, symbol);
    YogTable_add_direct(env, vm->id2name, symbol, s);

    vm->next_id++;

    RETURN(env, id);
}

#if 0
static void 
GcObjectStat_initialize(GcObjectStat* stat) 
{
    stat->survive_num = 0;
}
#endif

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

    MODIFY(env, PTR_AS(YogBasicObj, cObject)->klass, cKlass);
    MODIFY(env, PTR_AS(YogBasicObj, cKlass)->klass, cKlass);

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
    vm->cFloat = YogFloat_klass_new(env);
}

static void 
setup_encodings(YogEnv* env, YogVm* vm) 
{
    /* TODO: changed not to use macro */
#define REGISTER_ENCODING(name, onig)   do { \
    ID id = INTERN(name); \
    YogVal key = ID2VAL(id); \
    YogVal enc = YogEncoding_new(env, onig); \
    YogTable_add_direct(env, vm->encodings, key, enc); \
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

void 
YogVm_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogVm* vm = ptr;

#define KEEP(member)    YogGC_keep(env, &vm->member, keeper, heap)
    KEEP(id2name);
    KEEP(name2id);

    KEEP(cObject);
    KEEP(cKlass);
    KEEP(cInt);
    KEEP(cString);
    KEEP(cRegexp);
    KEEP(cMatch);
    KEEP(cPackage);
    KEEP(cBool);
    KEEP(cBuiltinBoundMethod);
    KEEP(cBoundMethod);
    KEEP(cBuiltinUnboundMethod);
    KEEP(cUnboundMethod);
    KEEP(cPackageBlock);
    KEEP(cNil);

    KEEP(eException);
    KEEP(eBugException);
    KEEP(eTypeError);
    KEEP(eIndexError);

    KEEP(pkgs);
    KEEP(encodings);
    KEEP(threads);
#undef KEEP
}

void 
YogVm_init(YogVm* vm) 
{
    vm->gc_stat.print = FALSE;
    vm->gc_stat.duration_total = 0;
    reset_living_object_count(vm);
    reset_total_object_count(vm);
    vm->gc_stat.num_alloc = 0;
    vm->gc_stat.total_allocated_size = 0;
    vm->gc_stat.exec_num = 0;

    vm->next_id = 0;
    vm->id2name = YUNDEF;
    vm->name2id = YUNDEF;

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

    vm->threads = YUNDEF;

    pthread_mutex_init(&vm->global_interp_lock, NULL);
    vm->running_gc = FALSE;
    vm->waiting_suspend = FALSE;
    vm->suspend_counter = 0;
    pthread_cond_init(&vm->threads_suspend_cond, NULL);
    pthread_cond_init(&vm->gc_finish_cond, NULL);
    vm->heaps = NULL;
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
YogVm_delete(YogEnv* env, YogVm* vm) 
{
#if 0
    if (vm->free_mem != NULL) {
        (*vm->free_mem)(env, vm);
    }
#endif
}

void 
YogVm_aquire_global_interp_lock(YogEnv* env, YogVm* vm)
{
    pthread_mutex_lock(&vm->global_interp_lock);
}

void 
YogVm_release_global_interp_lock(YogEnv* env, YogVm* vm) 
{
    pthread_mutex_unlock(&vm->global_interp_lock);
}

#if 0
void 
YogVm_add_thread(YogEnv* env, YogVm* vm, YogVal thread) 
{
    YogVm_aquire_global_interp_lock(env, vm);
    if (vm->waiting_suspend) {
        YogGC_suspend(env);
    }

    PTR_AS(YogThread, vm->threads)->prev = thread;
    PTR_AS(YogThread, thread)->next = vm->threads;
    vm->threads = thread;

    YogVm_release_global_interp_lock(env, vm);
}
#endif

void 
YogVm_set_main_thread(YogEnv* env, YogVm* vm, YogVal thread) 
{
    vm->threads = thread;
}

#if !defined(GC_BDW)
void
YogVm_add_heap(YogEnv* env, YogVm* vm, GC_TYPE* heap)
{
    YogVm_aquire_global_interp_lock(env, vm);
    ADD_TO_LIST(vm->heaps, heap);
    YogVm_release_global_interp_lock(env, vm);
}
#endif

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
