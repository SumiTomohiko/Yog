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
#include "yog/eval.h"
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
reset_total_object_count(YogVM* vm) 
{
    vm->gc_stat.total_obj_num = 0;
}

static void 
reset_living_object_count(YogVM* vm)
{
    int i;
    for (i = 0; i < SURVIVE_INDEX_MAX; i++) {
        vm->gc_stat.living_obj_num[i] = 0;
    }
}

#if 0
static void 
increment_total_object_number(YogVM* vm) 
{
    vm->gc_stat.total_obj_num++;
}

static void 
increment_living_object_number(YogVM* vm, unsigned int survive_num) 
{
    int index = survive_num / SURVIVE_NUM_UNIT;
    if (SURVIVE_INDEX_MAX  - 1 < index) {
        index = SURVIVE_INDEX_MAX - 1;
    }
    vm->gc_stat.living_obj_num[index]++;
}
#endif

void 
YogVM_register_package(YogEnv* env, YogVM* vm, const char* name, YogVal pkg) 
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, pkg);

    ID id = YogVM_intern(env, vm, name);
    YogVal key = ID2VAL(id);

    YogTable_add_direct(env, vm->pkgs, key, pkg);

    RETURN_VOID(env);
}

const char* 
YogVM_id2name(YogEnv* env, YogVM* vm, ID id) 
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
YogVM_intern(YogEnv* env, YogVM* vm, const char* name)
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
setup_builtins(YogEnv* env, YogVM* vm) 
{
    YogVal builtins = YogBuiltins_new(env);
    YogVM_register_package(env, vm, BUILTINS, builtins);
}

static void 
setup_symbol_tables(YogEnv* env, YogVM* vm) 
{
    vm->id2name = YogTable_new_symbol_table(env);
    vm->name2id = YogTable_new_string_table(env);
}

static void 
setup_basic_klass(YogEnv* env, YogVM* vm) 
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
setup_klasses(YogEnv* env, YogVM* vm) 
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
    vm->cThread = YogThread_klass_new(env);
}

static void 
setup_encodings(YogEnv* env, YogVM* vm) 
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
setup_exceptions(YogEnv* env, YogVM* vm) 
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

static void
set_main_thread_klass(YogEnv* env, YogVM* vm)
{
    PTR_AS(YogBasicObj, vm->main_thread)->klass = vm->cThread;
}

void 
YogVM_boot(YogEnv* env, YogVM* vm) 
{
    setup_symbol_tables(env, vm);
    setup_basic_klass(env, vm);
    setup_klasses(env, vm);
    set_main_thread_klass(env, vm);
    setup_exceptions(env, vm);

    vm->pkgs = YogTable_new_symbol_table(env);
    setup_builtins(env, vm);

    vm->encodings = YogTable_new_symbol_table(env);
    setup_encodings(env, vm);
}

#if defined(GC_GENERATIONAL)
#   include "yog/gc/copying.h"
#endif

static void
keep_local_vals(YogEnv* env, YogVal* vals, unsigned int size, ObjectKeeper keeper, void* heap)
{
    if (vals == NULL) {
        return;
    }

    unsigned int i;
    for (i = 0; i < size; i++) {
        YogVal* val = &vals[i];
        DEBUG(DPRINTF("val=%p", val));
        DEBUG(YogVal old_val = *val);
        YogGC_keep(env, val, keeper, heap);
        DEBUG(DPRINTF("val=%p, 0x%08x->0x%08x", val, old_val, *val));
    }
}

static void
keep_locals(YogEnv* env, YogLocals* locals, ObjectKeeper keeper, void* heap)
{
    unsigned int i;
    for (i = 0; i < locals->num_vals; i++) {
        keep_local_vals(env, locals->vals[i], locals->size, keeper, heap);
    }
}

static void
keep_thread_locals(YogEnv* env, YogVal thread, ObjectKeeper keeper, void* heap)
{
    YogLocals* locals = PTR_AS(YogThread, thread)->locals;
    while (locals != NULL) {
        keep_locals(env, locals, keeper, heap);
        locals = locals->next;
    }
}

void 
YogVM_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogVM* vm = ptr;

    YogVal thread = vm->running_threads;
    while (IS_PTR(thread)) {
        void* thread_heap = PTR_AS(YogThread, thread)->THREAD_GC;
        keep_thread_locals(env, thread, keeper, thread_heap);
        thread = PTR_AS(YogThread, thread)->next;
    }

#define KEEP(member)    do { \
    YogGC_keep(env, &vm->member, keeper, heap); \
} while (0)
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
    KEEP(cFloat);
    KEEP(cThread);

    KEEP(eException);
    KEEP(eBugException);
    KEEP(eTypeError);
    KEEP(eIndexError);

    KEEP(pkgs);
    KEEP(encodings);
    KEEP(main_thread);
    KEEP(running_threads);
#undef KEEP
}

static void
initialize_packages_lock(pthread_rwlock_t* lock)
{
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NP);
    pthread_rwlock_init(lock, &attr);
    pthread_rwlockattr_destroy(&attr);
}

void 
YogVM_init(YogVM* vm) 
{
    vm->gc_stress = FALSE;

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
    vm->cFloat = YUNDEF;
    vm->cThread = YUNDEF;

    vm->eException = YUNDEF;
    vm->eBugException = YUNDEF;
    vm->eTypeError = YUNDEF;
    vm->eIndexError = YUNDEF;

    vm->pkgs = PTR2VAL(NULL);
    initialize_packages_lock(&vm->pkgs_lock);

    vm->encodings = PTR2VAL(NULL);

    vm->running_threads = YUNDEF;

    pthread_mutex_init(&vm->global_interp_lock, NULL);
    vm->running_gc = FALSE;
    vm->waiting_suspend = FALSE;
    vm->suspend_counter = 0;
    pthread_cond_init(&vm->threads_suspend_cond, NULL);
    pthread_cond_init(&vm->gc_finish_cond, NULL);
    pthread_cond_init(&vm->vm_finish_cond, NULL);
    vm->heaps = vm->last_heap = NULL;
    vm->gc_id = 0;
}

#if 0
static void 
print_gc_statistics(YogVM* vm, unsigned int duration) 
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
YogVM_gc(YogEnv* env, YogVM* vm) 
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
YogVM_delete(YogEnv* env, YogVM* vm) 
{
#if 0
    if (vm->free_mem != NULL) {
        (*vm->free_mem)(env, vm);
    }
#endif
    pthread_rwlock_destroy(&vm->pkgs_lock);
}

void 
YogVM_aquire_global_interp_lock(YogEnv* env, YogVM* vm)
{
    pthread_mutex_lock(&vm->global_interp_lock);
}

void 
YogVM_release_global_interp_lock(YogEnv* env, YogVM* vm) 
{
    pthread_mutex_unlock(&vm->global_interp_lock);
}

static void
gc(YogEnv* env, YogVM* vm)
{
    while (vm->waiting_suspend) {
        YogGC_suspend(env);
    }
}

void 
YogVM_add_thread(YogEnv* env, YogVM* vm, YogVal thread) 
{
    YogVM_aquire_global_interp_lock(env, vm);
    gc(env, vm);

    PTR_AS(YogThread, vm->running_threads)->prev = thread;
    PTR_AS(YogThread, thread)->next = vm->running_threads;
    vm->running_threads = thread;

    YogVM_release_global_interp_lock(env, vm);
}

void 
YogVM_set_main_thread(YogEnv* env, YogVM* vm, YogVal thread) 
{
    vm->main_thread = vm->running_threads = thread;
}

void
YogVM_remove_thread(YogEnv* env, YogVM* vm, YogVal thread)
{
    SAVE_ARG(env, thread);

    YogVM_aquire_global_interp_lock(env, vm);
    gc(env, vm);

    YogVal prev = PTR_AS(YogThread, thread)->prev;
    YogVal next = PTR_AS(YogThread, thread)->next;
    if (IS_PTR(prev)) {
        PTR_AS(YogThread, prev)->next = next;
    }
    else {
        vm->running_threads = next;
    }
    if (IS_PTR(next)) {
        PTR_AS(YogThread, next)->prev = prev;
    }

    if (!IS_PTR(vm->running_threads)) {
        pthread_cond_signal(&vm->vm_finish_cond);
    }

    YogVM_release_global_interp_lock(env, vm);

    RETURN_VOID(env);
}

#if !defined(GC_BDW)
void
YogVM_add_heap(YogEnv* env, YogVM* vm, GC_TYPE* heap)
{
    YogVM_aquire_global_interp_lock(env, vm);
    if (vm->last_heap != NULL) {
        vm->last_heap->next = heap;
        heap->prev = vm->last_heap;
        vm->last_heap = heap;
    }
    else {
        vm->heaps = vm->last_heap = heap;
    }
    heap->next = NULL;
    YogVM_release_global_interp_lock(env, vm);
}
#endif

static unsigned int
count_running_threads(YogEnv* env, YogVM* vm)
{
    unsigned int n = 0;
    YogVal thread = vm->running_threads;
    while (IS_PTR(thread)) {
        n++;
        thread = PTR_AS(YogThread, thread)->next;
    }

    return n;
}

void
YogVM_wait_finish(YogEnv* env, YogVM* vm)
{
    YogVM_aquire_global_interp_lock(env, vm);
    gc(env, vm);

    while (0 < count_running_threads(env, vm)) {
        pthread_cond_wait(&vm->vm_finish_cond, &vm->global_interp_lock);
    }

    YogVM_release_global_interp_lock(env, vm);
}

static void
acquire_packages_read_lock(YogEnv* env, YogVM* vm)
{
    FREE_FROM_GC(env);
    pthread_rwlock_rdlock(&vm->pkgs_lock);
    BIND_TO_GC(env);
}

static void
acquire_packages_write_lock(YogEnv* env, YogVM* vm)
{
    FREE_FROM_GC(env);
    pthread_rwlock_wrlock(&vm->pkgs_lock);
    BIND_TO_GC(env);
}

static void
release_packages_lock(YogEnv* env, YogVM* vm)
{
    pthread_rwlock_unlock(&vm->pkgs_lock);
}

struct ImportingPackage {
    flags_t flags;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    YogVal pkg;
};

typedef struct ImportingPackage ImportingPackage;

static void
ImportingPackage_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    ImportingPackage* pkg = ptr;
    YogGC_keep(env, &pkg->pkg, keeper, heap);
}

static void
ImportingPackage_finalize(YogEnv* env, void* ptr)
{
    ImportingPackage* pkg = ptr;
    if (pthread_mutex_destroy(&pkg->lock) != 0) {
        YOG_WARN(env, "pthread_mutex_destroy failed");
    }
    if (pthread_cond_destroy(&pkg->cond) != 0) {
        YOG_WARN(env, "pthread_cond_destroy failed");
    }
}

static YogVal
ImportingPackage_new(YogEnv* env)
{
    YogVal pkg = ALLOC_OBJ(env, ImportingPackage_keep_children, ImportingPackage_finalize, ImportingPackage);
    PTR_AS(ImportingPackage, pkg)->flags = IMPORTING_PKG;
    pthread_mutex_init(&PTR_AS(ImportingPackage, pkg)->lock, NULL);
    pthread_cond_init(&PTR_AS(ImportingPackage, pkg)->cond, NULL);
    PTR_AS(ImportingPackage, pkg)->pkg = YUNDEF;
    return pkg;
}

static void
ImportingPackage_lock(YogEnv* env, YogVal pkg)
{
    pthread_mutex_t* lock = &PTR_AS(ImportingPackage, pkg)->lock;
    if (pthread_mutex_lock(lock) != 0) {
        YOG_BUG(env, "pthread_mutex_lock failed");
    }
}

static void
ImportingPackage_unlock(YogEnv* env, YogVal pkg)
{
    pthread_mutex_t* lock = &PTR_AS(ImportingPackage, pkg)->lock;
    if (pthread_mutex_unlock(lock) != 0) {
        YOG_BUG(env, "pthread_mutex_unlock failed");
    }
}

static void
wait_package(YogEnv* env, YogVal pkg)
{
    pthread_cond_t* cond = &PTR_AS(ImportingPackage, pkg)->cond;
    pthread_mutex_t* lock = &PTR_AS(ImportingPackage, pkg)->lock;
    while (!IS_PTR(PTR_AS(ImportingPackage, pkg)->pkg)) {
        FREE_FROM_GC(env);
        pthread_cond_wait(cond, lock);
        BIND_TO_GC(env);
    }
}

static YogVal
get_package(YogEnv* env, YogVM* vm, YogVal pkg)
{
    if (PTR_AS(YogBasicObj, pkg)->flags & IMPORTING_PKG) {
        ImportingPackage_lock(env, pkg);
        release_packages_lock(env, vm);
        wait_package(env, pkg);
        ImportingPackage_unlock(env, pkg);
        return PTR_AS(ImportingPackage, pkg)->pkg;
    }
    else {
        release_packages_lock(env, vm);
        return pkg;
    }
}

static void
package2path(char* name)
{
    char* pc = name;
    while (*pc != '\0') {
        if (*pc == '.') {
#define SEPARATOR   '/'
            *pc = SEPARATOR;
#undef SEPARATOR
        }
        pc++;
    }
    strcpy(pc, ".yg");
}

static YogVal
import_package(YogEnv* env, YogVM* vm, const char* name)
{
    SAVE_LOCALS(env);

    YogVal pkg = YUNDEF;
    YogVal tmp_pkg = YUNDEF;
    PUSH_LOCALS2(env, pkg, tmp_pkg);

    ID id = YogVM_intern(env, vm, name);

    acquire_packages_read_lock(env, vm);
#define FIND_PKG    do { \
    if (YogTable_lookup(env, vm->pkgs, ID2VAL(id), &pkg)) { \
        return get_package(env, vm, pkg); \
    } \
} while (0)
    FIND_PKG;
    release_packages_lock(env, vm);

    acquire_packages_write_lock(env, vm);
    FIND_PKG;
#undef FIND_PKG
    tmp_pkg = ImportingPackage_new(env);
    YogTable_add_direct(env, vm->pkgs, ID2VAL(id), tmp_pkg);
    release_packages_lock(env, vm);

    size_t len = strlen(name);
    char s[len + 3 + 1];    /* name + ".yg" + '\0' */
    strcpy(s, name);
    package2path(s);

    pkg = YogEval_eval_file(env, s, name);
    if (!IS_PTR(pkg)) {
        pkg = YogPackage_new(env);
    }
    PTR_AS(ImportingPackage, tmp_pkg)->pkg = pkg;

    acquire_packages_write_lock(env, vm);
    YogVal key = ID2VAL(id);
    if (!YogTable_delete(env, vm->pkgs, &key, NULL)) {
        YOG_BUG(env, "Can't delete importing package");
    }
    YogVal imported_pkg = PTR_AS(ImportingPackage, tmp_pkg)->pkg;
    YogTable_add_direct(env, vm->pkgs, ID2VAL(id), imported_pkg);
    YogVM_register_package(env, vm, name, imported_pkg);
    pthread_cond_broadcast(&PTR_AS(ImportingPackage, tmp_pkg)->cond);
    release_packages_lock(env, vm);

    RETURN(env, imported_pkg);
}

YogVal
YogVM_import_package(YogEnv* env, YogVM* vm, ID name)
{
    SAVE_LOCALS(env);

    YogVal top = YUNDEF;
    YogVal parent = YUNDEF;
    YogVal pkg = YUNDEF;
    PUSH_LOCALS3(env, top, parent, pkg);

    const char* s = YogVM_id2name(env, env->vm, name);
    size_t len = strlen(s);
    char t[len + 1];
    strcpy(t, s);
    const char* begin = t;
    const char* pc = begin;
    while (1) {
        const char* end = strchr(pc, '.');
        if (end == NULL) {
            end = begin + len;
        }
        unsigned int size = end - begin;
        char s[size + 1];
        strncpy(s, begin, size);
        s[size] = '\0';

        pkg = import_package(env, vm, s);
        if (IS_PTR(parent)) {
            unsigned int size = end - pc;
            char attr[size + 1];
            strncpy(attr, pc, size);
            attr[size] = '\0';
            ID id = YogVM_intern(env, vm, attr);
            YogObj_set_attr_id(env, parent, id, pkg);
        }
        if (!IS_PTR(top)) {
            top = pkg;
        }

        if (end == begin + len) {
            break;
        }

        parent = pkg;
        pc = end + 1;
    }

    RETURN(env, top);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
