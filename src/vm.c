#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "yog/array.h"
#include "yog/block.h"
#include "yog/bool.h"
#include "yog/builtins.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/float.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/gc/bdw.h"
#include "yog/int.h"
#include "yog/klass.h"
#include "yog/method.h"
#include "yog/misc.h"
#include "yog/nil.h"
#include "yog/package.h"
#include "yog/regexp.h"
#include "yog/table.h"
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

#define SEPARATOR       '/'

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

static void
acquire_read_lock(YogEnv* env, pthread_rwlock_t* lock)
{
    FREE_FROM_GC(env);
    pthread_rwlock_rdlock(lock);
    BIND_TO_GC(env);
}

static void
acquire_write_lock(YogEnv* env, pthread_rwlock_t* lock)
{
    FREE_FROM_GC(env);
    pthread_rwlock_wrlock(lock);
    BIND_TO_GC(env);
}

static void
acquire_symbols_read_lock(YogEnv* env, YogVM* vm)
{
    acquire_read_lock(env, &vm->sym_lock);
}

static void
acquire_symbols_write_lock(YogEnv* env, YogVM* vm)
{
    acquire_write_lock(env, &vm->sym_lock);
}

static void
release_symbols_lock(YogEnv* env, YogVM* vm)
{
    pthread_rwlock_unlock(&vm->sym_lock);
}

const char* 
YogVM_id2name(YogEnv* env, YogVM* vm, ID id) 
{
    acquire_symbols_read_lock(env, vm);

    YogVal sym = ID2VAL(id);
    YogVal val = YUNDEF;
    if (!YogTable_lookup(env, env->vm->id2name, sym, &val)) {
        YOG_BUG(env, "can't find symbol (0x%x)", id);
    }

    YogCharArray* ptr = VAL2PTR(val);

    release_symbols_lock(env, vm);
    return ptr->items;
}

ID
YogVM_intern(YogEnv* env, YogVM* vm, const char* name)
{
    SAVE_LOCALS(env);

    YogVal value = YUNDEF;

#define FIND_SYM    do { \
    if (YogTable_lookup_str(env, vm->name2id, name, &value)) { \
        release_symbols_lock(env, vm); \
        return VAL2ID(value); \
    } \
} while (0)
    acquire_symbols_read_lock(env, vm);
    FIND_SYM;
    release_symbols_lock(env, vm);

    acquire_symbols_write_lock(env, vm);
    FIND_SYM;
#undef FIND_SYM

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

    release_symbols_lock(env, vm);
    RETURN(env, id);
}

static void 
setup_builtins(YogEnv* env, YogVM* vm) 
{
    YogVal builtins = YogBuiltins_new(env);
    YogVM_register_package(env, vm, "builtins", builtins);
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
    vm->cFunction = YogFunction_klass_new(env);
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
    vm->cArray = YogArray_klass_new(env);
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
        keep_thread_locals(env, thread, keeper, THREAD_HEAP(thread));
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
    KEEP(cFunction);
    KEEP(cBuiltinBoundMethod);
    KEEP(cBoundMethod);
    KEEP(cBuiltinUnboundMethod);
    KEEP(cUnboundMethod);
    KEEP(cPackageBlock);
    KEEP(cNil);
    KEEP(cFloat);
    KEEP(cThread);
    KEEP(cArray);

    KEEP(eException);
    KEEP(eBugException);
    KEEP(eTypeError);
    KEEP(eIndexError);

    KEEP(pkgs);
    KEEP(search_path);
    KEEP(encodings);
    KEEP(main_thread);
    KEEP(running_threads);
#undef KEEP
}

static void
initialize_read_write_lock(pthread_rwlock_t* lock)
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

    vm->next_id = 0;
    vm->id2name = YUNDEF;
    vm->name2id = YUNDEF;
    initialize_read_write_lock(&vm->sym_lock);

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
    vm->cArray = YUNDEF;

    vm->eException = YUNDEF;
    vm->eBugException = YUNDEF;
    vm->eTypeError = YUNDEF;
    vm->eIndexError = YUNDEF;

    vm->pkgs = PTR2VAL(NULL);
    initialize_read_write_lock(&vm->pkgs_lock);
    vm->search_path = YUNDEF;

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
#if defined(GC_GENERATIONAL)
    vm->has_young_ref = FALSE;
#endif
}

void 
YogVM_delete(YogEnv* env, YogVM* vm) 
{
    if (pthread_cond_destroy(&vm->vm_finish_cond) != 0) {
        YOG_WARN(env, "pthread_cond_destroy failed");
    }
    if (pthread_cond_destroy(&vm->gc_finish_cond) != 0) {
        YOG_WARN(env, "pthread_cond_destroy failed");
    }
    if (pthread_cond_destroy(&vm->threads_suspend_cond) != 0) {
        YOG_WARN(env, "pthread_cond_destroy failed");
    }
    if (pthread_mutex_destroy(&vm->global_interp_lock) != 0) {
        YOG_WARN(env, "pthread_mutex_destroy failed");
    }
    pthread_rwlock_destroy(&vm->pkgs_lock);

#if !defined(GC_BDW)
    YogGC_delete(env);
#endif
}

void 
YogVM_acquire_global_interp_lock(YogEnv* env, YogVM* vm)
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
    SAVE_ARG(env, thread);

    YogVM_acquire_global_interp_lock(env, vm);
    gc(env, vm);

    PTR_AS(YogThread, vm->running_threads)->prev = thread;
    PTR_AS(YogThread, thread)->next = vm->running_threads;
    vm->running_threads = thread;

    YogVM_release_global_interp_lock(env, vm);

    RETURN_VOID(env);
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

    YogVM_acquire_global_interp_lock(env, vm);
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

    RESTORE_LOCALS(env);

    YogVM_release_global_interp_lock(env, vm);
}

#if !defined(GC_BDW)
void
YogVM_add_heap(YogEnv* env, YogVM* vm, void* heap)
{
    YogVM_acquire_global_interp_lock(env, vm);
    if (vm->last_heap != NULL) {
        ((GC_TYPE*)vm->last_heap)->next = heap;
        ((GC_TYPE*)heap)->prev = vm->last_heap;
        vm->last_heap = heap;
    }
    else {
        vm->heaps = vm->last_heap = heap;
    }
    ((GC_TYPE*)heap)->next = NULL;
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
    YogVM_acquire_global_interp_lock(env, vm);
    gc(env, vm);

    while (0 < count_running_threads(env, vm)) {
        pthread_cond_wait(&vm->vm_finish_cond, &vm->global_interp_lock);
    }

    YogVM_release_global_interp_lock(env, vm);
}

static void
acquire_packages_read_lock(YogEnv* env, YogVM* vm)
{
    acquire_read_lock(env, &vm->pkgs_lock);
}

static void
acquire_packages_write_lock(YogEnv* env, YogVM* vm)
{
    acquire_write_lock(env, &vm->pkgs_lock);
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
    SAVE_ARG(env, pkg);

    pthread_cond_t* cond = &PTR_AS(ImportingPackage, pkg)->cond;
    pthread_mutex_t* lock = &PTR_AS(ImportingPackage, pkg)->lock;
    while (!IS_PTR(PTR_AS(ImportingPackage, pkg)->pkg)) {
        FREE_FROM_GC(env);
        pthread_cond_wait(cond, lock);
        BIND_TO_GC(env);
    }

    RETURN_VOID(env);
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
package_name2path_head(char* name)
{
    char* pc = name;
    while (*pc != '\0') {
        if (*pc == '.') {
            *pc = SEPARATOR;
        }
        pc++;
    }
}

static YogVal
import_so(YogEnv* env, YogVM* vm, const char* filename, const char* pkg_name)
{
    SAVE_LOCALS(env);

    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    char path[strlen(filename) + 2];     /* 2 is for "./" */
    if (strchr(filename, '/') != NULL) {
        strcpy(path, filename);
    }
    else {
        strcpy(path, "./");
        strcat(path, filename);
    }

#define CLEAR_ERROR     dlerror()
    CLEAR_ERROR;
    void* handle = dlopen(path, RTLD_LAZY);
    if (handle == NULL) {
        RETURN(env, YUNDEF);
    }

#define INIT_NAME_HEAD  "YogInit_"
    char init_name[strlen(INIT_NAME_HEAD) + strlen(pkg_name) + 1];
    strcpy(init_name, INIT_NAME_HEAD);
#undef INIT_NAME_HEAD
    const char* pc = strrchr(pkg_name, '.');
    if (pc != NULL) {
        pc++;
    }
    else {
        pc = pkg_name;
    }
    strcat(init_name, pc);

    CLEAR_ERROR;
    void (*init)(YogEnv*, YogVal) = dlsym(handle, init_name);
    if (init == NULL) {
        RETURN(env, YUNDEF);
    }
#undef CLEAR_ERROR

    pkg = YogPackage_new(env);
    (*init)(env, pkg);

    RETURN(env, pkg);
}

static void
join_path(char* dest, const char* head, const char* tail)
{
    strcpy(dest, head);

    size_t len = strlen(dest);
    dest[len] = SEPARATOR;

    strcpy(dest + len + 1, tail);
}

static YogVal
import(YogEnv* env, YogVM* vm, const char* path_head, const char* pkg_name)
{
    SAVE_LOCALS(env);

    YogVal pkg = YUNDEF;
    YogVal dir = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS3(env, pkg, dir, body);

    unsigned int size = YogArray_size(env, vm->search_path);
    unsigned int i;
    for (i = 0; i < size; i++) {
        dir = YogArray_at(env, vm->search_path, i);
        body = PTR_AS(YogString, dir)->body;
        size_t dir_len = strlen(PTR_AS(YogCharArray, body)->items);

        size_t len = strlen(path_head);
#define HEAD2PATH(var, ext) \
        char var[dir_len + 1 + len + strlen(ext) + 1]; \
        join_path(var, PTR_AS(YogCharArray, body)->items, path_head); \
        strcat(var, ext)
        HEAD2PATH(yg, ".yg");
        pkg = YogEval_eval_file(env, yg, pkg_name);
        if (IS_PTR(pkg)) {
            RETURN(env, pkg);
        }

        HEAD2PATH(so, ".so");
#undef HEAD2PATH
        pkg = import_so(env, vm, so, pkg_name);
        if (IS_PTR(pkg)) {
            RETURN(env, pkg);
        }
    }

    RETURN(env, YUNDEF);
}

static YogVal
import_package(YogEnv* env, YogVM* vm, const char* name)
{
    SAVE_LOCALS(env);

    YogVal pkg = YUNDEF;
    YogVal tmp_pkg = YUNDEF;
    YogVal imported_pkg = YUNDEF;
    PUSH_LOCALS3(env, pkg, tmp_pkg, imported_pkg);

    ID id = YogVM_intern(env, vm, name);

    acquire_packages_read_lock(env, vm);
#define FIND_PKG    do { \
    if (YogTable_lookup(env, vm->pkgs, ID2VAL(id), &pkg)) { \
        RETURN(env, get_package(env, vm, pkg)); \
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
    char head[len + 1];
    strcpy(head, name);
    package_name2path_head(head);

    pkg = import(env, vm, head, name);
    if (!IS_PTR(pkg)) {
        pkg = YogPackage_new(env);
    }
    PTR_AS(ImportingPackage, tmp_pkg)->pkg = pkg;

    acquire_packages_write_lock(env, vm);
    YogVal key = ID2VAL(id);
    if (!YogTable_delete(env, vm->pkgs, &key, NULL)) {
        YOG_BUG(env, "Can't delete importing package");
    }
    imported_pkg = PTR_AS(ImportingPackage, tmp_pkg)->pkg;
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

static void
split_path(char* delim)
{
    if (delim == NULL) {
        return;
    }
    *delim = '\0';
}

static BOOL
is_directory(const char* filename)
{
    struct stat buf;
    if (stat(filename, &buf) != 0) {
        return FALSE;
    }
    if (!S_ISDIR(buf.st_mode)) {
        return FALSE;
    }
    return TRUE;
}

static BOOL
is_executable_file(const char* filename)
{
    struct stat buf;
    if (stat(filename, &buf) != 0) {
        return FALSE;
    }
    if (!S_ISREG(buf.st_mode)) {
        return FALSE;
    }
    if ((buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) {
        return FALSE;
    }
    return TRUE;
}

static BOOL
search_program(char* dest, const char* path, const char* prog)
{
    char s[strlen(path) + 1];
    strcpy(s, path);

    char* pc = s;
    while (1) {
        char* delim = strchr(pc, ':');
        split_path(delim);
        join_path(dest, pc, prog);
        if (is_executable_file(dest)) {
            return TRUE;
        }

        if (delim != NULL) {
            pc = delim + 1;
        }
        else {
            return FALSE;
        }
    }
}

static void
dirname(YogEnv* env, YogVal filename)
{
    YogVal body = PTR_AS(YogString, filename)->body;
    char* s = PTR_AS(YogCharArray, body)->items;
    char* pc = strrchr(s, SEPARATOR);
    YOG_ASSERT(env, pc != NULL, "%s doesn't include directory name", filename);
    *pc = '\0';
}

static void
add_current_directory(YogEnv* env, YogVal search_path)
{
    SAVE_ARG(env, search_path);

    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = YogString_new_str(env, ".");
    YogArray_push(env, search_path, s);

    RETURN_VOID(env);
}

void
YogVM_configure_search_path(YogEnv* env, YogVM* vm, const char* argv0)
{
    SAVE_LOCALS(env);

    YogVal prog = YUNDEF;
    YogVal prog_dir = YUNDEF;
    YogVal search_path = YUNDEF;
    YogVal body = YUNDEF;
    YogVal s = YUNDEF;
    PUSH_LOCALS5(env, prog, prog_dir, search_path, body, s);

    if (strchr(argv0, SEPARATOR) == NULL) {
        char* path = getenv("PATH");
        YOG_ASSERT(env, path != NULL, "PATH is empty");
        /* 1 of middle is for '/' */
        char s[strlen(path) + 1 + strlen(argv0) + 1];
        if (!search_program(s, path, argv0)) {
            YOG_BUG(env, "Can't find %s in %s", argv0, path);
        }
        prog = YogString_new_str(env, s);
    }
    else {
        prog = YogString_new_str(env, argv0);
    }

    search_path = YogArray_new(env);
    add_current_directory(env, search_path);

    dirname(env, prog);
    unsigned int len = YogString_size(env, prog);
#define EXT_DIR     "../ext"
    /* 1 of middle is for '/' */
    char extpath[len +  1 + strlen(EXT_DIR) + 1];
    body = PTR_AS(YogString, prog)->body;
    join_path(extpath, PTR_AS(YogCharArray, body)->items, EXT_DIR);
#undef EXT_DIR
    if (is_directory(extpath)) {
        s = YogString_new_str(env, extpath);
        YogArray_push(env, search_path, s);

#define LIB_DIR     "../lib"
        char libpath[len + 1 + strlen(LIB_DIR) + 1];
        join_path(libpath, PTR_AS(YogCharArray, body)->items, LIB_DIR);
#undef LIB_DIR
        s = YogString_new_str(env, libpath);
        YogArray_push(env, search_path, s);
    }
    else {
        s = YogString_new_str(env, "/usr/local/lib/yog/0.9.0");
        YogArray_push(env, search_path, s);
    }
    vm->search_path = search_path;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
