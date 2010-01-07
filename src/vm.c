#include "yog/config.h"
#if defined(HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#if defined(HAVE_DLFCN_H)
#   include <dlfcn.h>
#endif
#include <errno.h>
#if defined(HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#if defined(HAVE_STRINGS_H)
#   include <strings.h>
#endif
#if defined(HAVE_SYS_MMAN_H)
#   include <sys/mman.h>
#endif
/* Linux and Windows both have <sys/stat.h>. */
#include <sys/stat.h>
#if defined(HAVE_SYS_TIME_H)
#   include <sys/time.h>
#endif
#include <sys/types.h>
#include <time.h>
#if defined(HAVE_UNISTD_H)
#   include <unistd.h>
#endif
#if defined(HAVE_WINDOWS_H)
#   include <windows.h>
#endif
#include "yog/array.h"
#include "yog/bignum.h"
#include "yog/block.h"
#include "yog/bool.h"
#include "yog/builtins.h"
#include "yog/class.h"
#include "yog/classmethod.h"
#include "yog/code.h"
#include "yog/comparable.h"
#include "yog/compile.h"
#include "yog/coroutine.h"
#include "yog/dict.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/file.h"
#include "yog/fixnum.h"
#include "yog/float.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/gc/bdw.h"
#include "yog/misc.h"
#include "yog/module.h"
#include "yog/nil.h"
#include "yog/package.h"
#include "yog/property.h"
#include "yog/regexp.h"
#include "yog/set.h"
#include "yog/symbol.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define PAGE_SIZE       4096
#define PTR2PAGE(p)     ((YogMarkSweepCompactPage*)((uintptr_t)p & ~(PAGE_SIZE - 1)))

#if defined(__MINGW32__) || defined(_MSC_VER)
#   define SEPARATOR    '\\'
#else
#   define SEPARATOR    '/'
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

YogVal
YogVM_id2name(YogEnv* env, YogVM* vm, ID id)
{
    SAVE_LOCALS(env);
    YogVal s = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS2(env, s, val);

    acquire_symbols_read_lock(env, vm);

    YogVal sym = ID2VAL(id);
    val = YUNDEF;
    if (!YogTable_lookup(env, env->vm->id2name, sym, &val)) {
        YOG_BUG(env, "can't find symbol (0x%x)", id);
    }

    uint_t size = PTR_AS(YogCharArray, val)->size;
    s = YogString_new_size(env, size);
    strlcpy(STRING_CSTR(s), PTR_AS(YogCharArray, val)->items, size);
    STRING_SIZE(s) = size;

    release_symbols_lock(env, vm);

    RETURN(env, s);
}

ID
YogVM_intern(YogEnv* env, YogVM* vm, const char* name)
{
    SAVE_LOCALS(env);
    YogVal value = YUNDEF;
    PUSH_LOCAL(env, value);

#define FIND_SYM    do { \
    if (YogTable_lookup_str(env, vm->name2id, name, &value)) { \
        release_symbols_lock(env, vm); \
        RETURN(env, VAL2ID(value)); \
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
    uint_t size = strlen(name) + 1;
#if !defined(alloca) && defined(_alloca)
#   define alloca   _alloca
#endif
    char* buffer = (char*)alloca(sizeof(char) * size);
    strlcpy(buffer, name, size);

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
setup_builtins(YogEnv* env, YogVM* vm, YogVal builtins, uint_t argc, char** argv)
{
    SAVE_ARG(env, builtins);
    YogBuiltins_boot(env, builtins, argc, argv);
    YogVM_register_package(env, vm, "builtins", builtins);
    RETURN_VOID(env);
}

static void
setup_symbol_tables(YogEnv* env, YogVM* vm)
{
    vm->id2name = YogTable_new_symbol_table(env);
    vm->name2id = YogTable_new_string_table(env);
}

static void
setup_basic_class(YogEnv* env, YogVM* vm)
{
    YogVal cObject = YUNDEF;
    YogVal cClass = YUNDEF;
    PUSH_LOCALS2(env, cObject, cClass);

    cObject = YogClass_new(env, "Object", YNIL);
    YogClass_define_allocator(env, cObject, YogObj_alloc);

    cClass = YogClass_new(env, "Class", cObject);
    YogClass_define_allocator(env, cClass, YogClass_alloc);

    PTR_AS(YogBasicObj, cObject)->klass = cClass;
    PTR_AS(YogBasicObj, cClass)->klass = cClass;

    vm->cObject = cObject;
    vm->cClass = cClass;

    POP_LOCALS(env);
}

static void
setup_classes(YogEnv* env, YogVM* vm, YogVal builtins)
{
    SAVE_ARG(env, builtins);

    YogFunction_define_classes(env, builtins);

    YogObj_class_init(env, vm->cObject, builtins);
    YogClass_class_init(env, vm->cClass, builtins);
    YogProperty_define_classes(env, builtins);

    YogComparable_define_classes(env, builtins);

    YogArray_define_classes(env, builtins);
    YogBignum_define_classes(env, builtins);
    YogBool_define_classes(env, builtins);
    YogClassMethod_define_classes(env, builtins);
    YogCode_define_classes(env, builtins);
    YogCoroutine_define_classes(env, builtins);
    YogDict_define_classes(env, builtins);
    YogFile_define_classes(env, builtins);
    YogFixnum_define_classes(env, builtins);
    YogFloat_define_classes(env, builtins);
    YogRegexp_define_classes(env, builtins);
    YogModule_define_classes(env, builtins);
    YogNil_define_classes(env, builtins);
    YogPackage_define_classes(env, builtins);
    YogPackageBlock_define_classes(env, builtins);
    YogSet_define_classes(env, builtins);
    YogString_define_classes(env, builtins);
    YogSymbol_define_classes(env, builtins);
    YogThread_define_classes(env, builtins);

    RETURN_VOID(env);
}

static void
setup_encodings(YogEnv* env, YogVM* vm)
{
    /* TODO: changed not to use macro */
#define REGISTER_ENCODING(name, onig)   do { \
    ID id = YogVM_intern(env, env->vm, name); \
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
set_main_thread_class(YogEnv* env, YogVM* vm)
{
    PTR_AS(YogBasicObj, vm->main_thread)->klass = vm->cThread;
}

static YogVal
alloc_skelton_pkg(YogEnv* env, YogVM* vm)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = ALLOC_OBJ(env, YogPackage_keep_children, NULL, YogPackage);
    YogObj_init(env, pkg, TYPE_PACKAGE, 0, YUNDEF);

    RETURN(env, pkg);
}

void
YogVM_boot(YogEnv* env, YogVM* vm, uint_t argc, char** argv)
{
    SAVE_LOCALS(env);
    YogVal builtins = YUNDEF;
    PUSH_LOCAL(env, builtins);

    setup_symbol_tables(env, vm);
    setup_basic_class(env, vm);
    builtins = alloc_skelton_pkg(env, vm);
    setup_classes(env, vm, builtins);
    YogPackage_init(env, builtins);
    set_main_thread_class(env, vm);
    YogException_define_classes(env, builtins);
    YogObject_boot(env, vm->cObject, builtins);
    YogClass_boot(env, vm->cClass, builtins);

    vm->pkgs = YogTable_new_symbol_table(env);

    vm->encodings = YogTable_new_symbol_table(env);
    setup_encodings(env, vm);

    vm->finish_code = YogCompiler_compile_finish_code(env);

    setup_builtins(env, vm, builtins, argc, argv);
    YogArray_eval_builtin_script(env, vm->cArray);
    YogDict_eval_builtin_script(env, vm->cDict);
    YogObject_eval_builtin_script(env, vm->cObject);
    YogString_eval_builtin_script(env, vm->cString);

    RETURN_VOID(env);
}

#if defined(GC_GENERATIONAL)
#   include "yog/gc/copying.h"
#endif

static void
keep_local_vals(YogEnv* env, YogVal* vals, uint_t size, ObjectKeeper keeper, void* heap)
{
    if (vals == NULL) {
        return;
    }
    DEBUG(TRACE("vals=%p, size=0x%08x", vals, size));

    uint_t i;
    for (i = 0; i < size; i++) {
        YogVal* val = &vals[i];
        DEBUG(TRACE("val=%p, *val=0x%08x", val, *val));
        DEBUG(YogVal old_val = *val);
        YogGC_keep(env, val, keeper, heap);
        DEBUG(TRACE("val=%p, 0x%08x->0x%08x", val, old_val, *val));
    }
}

static void
keep_locals(YogEnv* env, YogLocals* locals, ObjectKeeper keeper, void* heap)
{
    uint_t i;
    for (i = 0; i < locals->num_vals; i++) {
        keep_local_vals(env, locals->vals[i], locals->size, keeper, heap);
    }
}

static void
keep_locals_list(YogEnv* env, YogLocals* list, ObjectKeeper keeper, void* heap)
{
    while (list != NULL) {
        DEBUG(TRACE("list=%p, list->next=%p", list, list->next));
        keep_locals(env, list, keeper, heap);
        list = list->next;
    }
}

void
YogVM_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogVM* vm = PTR_AS(YogVM, ptr);

    YogLocalsAnchor* locals = vm->locals;
    while (locals != NULL) {
        keep_locals_list(env, locals->body, keeper, locals->heap);
        locals = locals->next;
    }

#define KEEP(member)    do { \
    YogGC_keep(env, &vm->member, keeper, heap); \
} while (0)
    KEEP(id2name);
    KEEP(name2id);

    KEEP(cArray);
    KEEP(cBignum);
    KEEP(cBool);
    KEEP(cClassMethod);
    KEEP(cCode);
    KEEP(cCoroutine);
    KEEP(cDict);
    KEEP(cFile);
    KEEP(cFixnum);
    KEEP(cFloat);
    KEEP(cFunction);
    KEEP(cInstanceMethod);
    KEEP(cClass);
    KEEP(cMatch);
    KEEP(cModule);
    KEEP(cNativeFunction);
    KEEP(cNativeInstanceMethod);
    KEEP(cNil);
    KEEP(cObject);
    KEEP(cPackage);
    KEEP(cPackageBlock);
    KEEP(cProperty);
    KEEP(cRegexp);
    KEEP(cSet);
    KEEP(cString);
    KEEP(cSymbol);
    KEEP(cThread);

    KEEP(eArgumentError);
    KEEP(eAttributeError);
    KEEP(eEOFError);
    KEEP(eException);
    KEEP(eImportError);
    KEEP(eIndexError);
    KEEP(eKeyError);
    KEEP(eLocalJumpError);
    KEEP(eNameError);
    KEEP(eSyntaxError);
    KEEP(eSystemCallError);
    KEEP(eTypeError);
    KEEP(eValueError);
    KEEP(eZeroDivisionError);
#if !defined(MINIYOG)
#   include "errno_keep.inc"
#endif

    KEEP(mComparable);

    KEEP(pkgs);
    KEEP(search_path);
    KEEP(encodings);
    KEEP(finish_code);
    KEEP(main_thread);
    KEEP(running_threads);

    YogIndirectPointer* indirect_ptr = vm->indirect_ptr;
    while (indirect_ptr != NULL) {
        KEEP(indirect_ptr->val);
        indirect_ptr = indirect_ptr->next;
    }
#undef KEEP
}

static void
init_read_write_lock(pthread_rwlock_t* lock)
{
    pthread_rwlockattr_t* pattr;
#if defined(HAVE_PTHREAD_RWLOCKATTR_INIT)
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
#   if defined(HAVE_PTHREAD_RWLOCKATTR_SETKIND_NP)
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NP);
#   endif
    pattr = &attr;
#else
    pattr = NULL;
#endif
    int err;
    if ((err = pthread_rwlock_init(lock, pattr)) != 0) {
        YOG_BUG(NULL, "pthread_rwlock_init failed: %s", strerror(err));
    }
#if defined(HAVE_PTHREAD_RWLOCKATTR_INIT) && defined(HAVE_PTHREAD_RWLOCKATTR_DESTROY)
    pthread_rwlockattr_destroy(&attr);
#endif
}

void
YogVM_init(YogVM* vm)
{
    vm->gc_stress = FALSE;

#define INIT(member)    vm->member = YUNDEF
    vm->next_id = 0;
    INIT(id2name);
    INIT(name2id);
    init_read_write_lock(&vm->sym_lock);

    INIT(cArray);
    INIT(cBignum);
    INIT(cBool);
    INIT(cClassMethod);
    INIT(cCode);
    INIT(cCoroutine);
    INIT(cDict);
    INIT(cFile);
    INIT(cFixnum);
    INIT(cFloat);
    INIT(cFunction);
    INIT(cInstanceMethod);
    INIT(cClass);
    INIT(cMatch);
    INIT(cModule);
    INIT(cNativeFunction);
    INIT(cNativeInstanceMethod);
    INIT(cNil);
    INIT(cObject);
    INIT(cPackage);
    INIT(cPackageBlock);
    INIT(cProperty);
    INIT(cRegexp);
    INIT(cSet);
    INIT(cString);
    INIT(cSymbol);
    INIT(cThread);

    INIT(eArgumentError);
    INIT(eAttributeError);
    INIT(eEOFError);
    INIT(eException);
    INIT(eImportError);
    INIT(eIndexError);
    INIT(eKeyError);
    INIT(eLocalJumpError);
    INIT(eNameError);
    INIT(eSyntaxError);
    INIT(eSystemCallError);
    INIT(eTypeError);
    INIT(eValueError);
    INIT(eZeroDivisionError);
#if !defined(MINIYOG)
#   include "errno_init.inc"
#endif

    INIT(mComparable);

    vm->pkgs = PTR2VAL(NULL);
    init_read_write_lock(&vm->pkgs_lock);
    INIT(search_path);

    vm->encodings = PTR2VAL(NULL);

    INIT(finish_code);

    INIT(running_threads);
    vm->next_thread_id = 0;
    pthread_mutex_init(&vm->next_thread_id_lock, NULL);
#undef INIT

    pthread_mutexattr_t global_interp_lock_attr;
    pthread_mutexattr_init(&global_interp_lock_attr);
    int err;
    if ((err = pthread_mutexattr_settype(&global_interp_lock_attr, PTHREAD_MUTEX_RECURSIVE)) != 0) {
        YOG_BUG(NULL, "pthread_mutexattr_settype failed: %s", strerror(err));
    }
    pthread_mutex_init(&vm->global_interp_lock, &global_interp_lock_attr);
    pthread_mutexattr_destroy(&global_interp_lock_attr);

    vm->running_gc = FALSE;
    vm->waiting_suspend = FALSE;
    vm->suspend_counter = 0;
    if ((err = pthread_cond_init(&vm->threads_suspend_cond, NULL)) != 0) {
        YOG_BUG(NULL, "pthread_cond_init failed: %s", strerror(err));
    }
    if ((err = pthread_cond_init(&vm->gc_finish_cond, NULL)) != 0) {
        YOG_BUG(NULL, "pthread_cond_init failed: %s", strerror(err));
    }
    if ((err = pthread_cond_init(&vm->vm_finish_cond, NULL)) != 0) {
        YOG_BUG(NULL, "pthread_cond_init failed: %s", strerror(err));
    }
    vm->heaps = vm->last_heap = NULL;
    vm->gc_id = 0;
    vm->locals = NULL;
#if defined(GC_GENERATIONAL)
    vm->has_young_ref = FALSE;
#endif

    vm->indirect_ptr = NULL;
    pthread_mutex_init(&vm->indirect_ptr_lock, NULL);
}

void
YogVM_delete(YogEnv* env, YogVM* vm)
{
#if !defined(GC_BDW)
    YogGC_delete(env);
#endif

    YogIndirectPointer* indirect_ptr = vm->indirect_ptr;
    while (indirect_ptr != NULL) {
        YogIndirectPointer* next = indirect_ptr->next;
        free(indirect_ptr);
        indirect_ptr = next;
    }

    int err;
    if ((err = pthread_mutex_destroy(&vm->indirect_ptr_lock)) != 0) {
        YOG_WARN(env, "pthread_mutex_destroy failed: %s", strerror(err));
    }
    if ((err = pthread_cond_destroy(&vm->vm_finish_cond)) != 0) {
        YOG_WARN(env, "pthread_cond_destroy failed: %s", strerror(err));
    }
    if ((err = pthread_cond_destroy(&vm->gc_finish_cond)) != 0) {
        YOG_WARN(env, "pthread_cond_destroy failed: %s", strerror(err));
    }
    if ((err = pthread_cond_destroy(&vm->threads_suspend_cond)) != 0) {
        YOG_WARN(env, "pthread_cond_destroy failed: %s", strerror(err));
    }
    if ((err = pthread_mutex_destroy(&vm->global_interp_lock)) != 0) {
        YOG_WARN(env, "pthread_mutex_destroy failed: %s", strerror(err));
    }
    if ((err = pthread_mutex_destroy(&vm->next_thread_id_lock)) != 0) {
        YOG_WARN(env, "pthread_mutex_destroy failed: %s", strerror(err));
    }
    pthread_rwlock_destroy(&vm->pkgs_lock);
}

static void
acquire_lock(YogEnv* env, pthread_mutex_t* lock)
{
    int err;
    if ((err = pthread_mutex_lock(lock)) != 0) {
        YOG_BUG(env, "pthread_mutex_lock failed: %s", strerror(err));
    }
}

static void
release_lock(YogEnv* env, pthread_mutex_t* lock)
{
    int err;
    if ((err = pthread_mutex_unlock(lock)) != 0) {
        YOG_BUG(env, "pthread_mutex_unlock failed: %s", strerror(err));
    }
}

void
YogVM_acquire_global_interp_lock(YogEnv* env, YogVM* vm)
{
    acquire_lock(env, &vm->global_interp_lock);
}

void
YogVM_release_global_interp_lock(YogEnv* env, YogVM* vm)
{
    release_lock(env, &vm->global_interp_lock);
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
        ((GC_TYPE*)vm->last_heap)->next = (GC_TYPE*)heap;
        ((GC_TYPE*)heap)->prev = (GC_TYPE*)vm->last_heap;
        vm->last_heap = (GC_TYPE*)heap;
    }
    else {
        vm->heaps = vm->last_heap = (GC_TYPE*)heap;
    }
    ((GC_TYPE*)heap)->next = NULL;
    YogVM_release_global_interp_lock(env, vm);
}
#endif

static uint_t
count_running_threads(YogEnv* env, YogVM* vm)
{
    uint_t n = 0;
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
    struct YogBasicObj base;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    YogVal pkg;
};

typedef struct ImportingPackage ImportingPackage;

#define TYPE_IMPORTING_PKG  ((type_t)ImportingPackage_new)

static void
ImportingPackage_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    ImportingPackage* pkg = PTR_AS(ImportingPackage, ptr);
    YogGC_keep(env, &pkg->pkg, keeper, heap);
}

static void
ImportingPackage_finalize(YogEnv* env, void* ptr)
{
    ImportingPackage* pkg = PTR_AS(ImportingPackage, ptr);
    int err;
    if ((err = pthread_mutex_destroy(&pkg->lock)) != 0) {
        YOG_WARN(env, "pthread_mutex_destroy failed: %s", strerror(err));
    }
    if ((err = pthread_cond_destroy(&pkg->cond)) != 0) {
        YOG_WARN(env, "pthread_cond_destroy failed: %s", strerror(err));
    }
}

static YogVal
ImportingPackage_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = ALLOC_OBJ(env, ImportingPackage_keep_children, ImportingPackage_finalize, ImportingPackage);
    YogBasicObj_init(env, pkg, TYPE_IMPORTING_PKG, FLAG_PKG, YUNDEF);

    pthread_mutex_init(&PTR_AS(ImportingPackage, pkg)->lock, NULL);
    pthread_cond_init(&PTR_AS(ImportingPackage, pkg)->cond, NULL);
    PTR_AS(ImportingPackage, pkg)->pkg = YUNDEF;

    RETURN(env, pkg);
}

static void
ImportingPackage_lock(YogEnv* env, YogVal pkg)
{
    acquire_lock(env, &PTR_AS(ImportingPackage, pkg)->lock);
}

static void
ImportingPackage_unlock(YogEnv* env, YogVal pkg)
{
    release_lock(env, &PTR_AS(ImportingPackage, pkg)->lock);
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
    YOG_ASSERT(env, PTR_AS(YogBasicObj, pkg)->flags & FLAG_PKG, "invalid package");
    if (BASIC_OBJ_TYPE(pkg) == TYPE_IMPORTING_PKG) {
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

#if defined(__MINGW32__) || defined(_MSC_VER)
#   define LIB_HANDLE   HINSTANCE
#else
#   define LIB_HANDLE   void*
#endif

static LIB_HANDLE
open_library(YogEnv* env, const char* path)
{
#if defined(__MINGW32__) || defined(_MSC_VER)
    return LoadLibrary(path);
#else
    return dlopen(path, RTLD_LAZY);
#endif
}

static void*
get_proc(YogEnv* env, LIB_HANDLE handle, const char* name)
{
#if defined(__MINGW32__) || defined(_MSC_VER)
    return GetProcAddress(handle, name);
#else
    return dlsym(handle, name);
#endif
}

static YogVal
import_so(YogEnv* env, YogVM* vm, const char* filename, const char* pkg_name)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    /* 3 is for "./" and '\0'. */
    size_t size = strlen(filename) + 3;
    char* path = (char*)alloca(sizeof(char) * size);
    if (strchr(filename, SEPARATOR) != NULL) {
        strlcpy(path, filename, size);
    }
    else {
#define CUR_DIR_SIZE    3
        char cur_dir[CUR_DIR_SIZE];
#if defined(_MSC_VER)
#   define snprintf   _snprintf
#endif
        snprintf(cur_dir, CUR_DIR_SIZE, ".%c", SEPARATOR);
        strlcpy(path, cur_dir, size);
        strlcat(path, filename, size);
#undef CUR_DIR_SIZE
    }

#if defined(__MINGW32__) || defined(_MSC_VER)
#   define CLEAR_ERROR
#else
#   define CLEAR_ERROR     dlerror()
#endif
    CLEAR_ERROR;
    LIB_HANDLE handle = open_library(env, path);
    if (handle == NULL) {
        RETURN(env, YUNDEF);
    }

#define INIT_NAME_HEAD  "YogInit_"
    size_t init_name_size = strlen(INIT_NAME_HEAD) + strlen(pkg_name) + 1;
    char* init_name = (char*)alloca(sizeof(char) * init_name_size);
    strlcpy(init_name, INIT_NAME_HEAD, init_name_size);
#undef INIT_NAME_HEAD
    const char* pc = strrchr(pkg_name, '.');
    if (pc != NULL) {
        pc++;
    }
    else {
        pc = pkg_name;
    }
    strlcat(init_name, pc, init_name_size);

    CLEAR_ERROR;
    typedef YogVal (*Initializer)(YogEnv*);
    Initializer init = (Initializer)get_proc(env, handle, init_name);
    if (init == NULL) {
        YogError_raise_ImportError(env, "dynamic package does not define init function (%s)", init_name);
    }
#undef CLEAR_ERROR

    pkg = (*init)(env);
    YOG_ASSERT(env, IS_PTR(pkg), "invalid package");
    YOG_ASSERT(env, BASIC_OBJ(pkg)->flags & FLAG_PKG, "invalid package");

    RETURN(env, pkg);
}

#undef LIB_HANDLE

static void
join_path(char* dest, const char* head, const char* tail, size_t size)
{
    strlcpy(dest, head, size);

    size_t len = strlen(dest);
    dest[len] = SEPARATOR;

    strlcpy(dest + len + 1, tail, size - (len + 1));
}

static YogVal
import_yg(YogEnv* env, const char* yg, const char* pkg_name)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    FILE* fp = fopen(yg, "r");
    if (fp == NULL) {
        RETURN(env, YNIL);
    }

    pkg = YogEval_eval_file(env, fp, yg, pkg_name);

    fclose(fp);

    RETURN(env, pkg);
}

static YogVal
import(YogEnv* env, YogVM* vm, const char* path_head, const char* pkg_name)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    YogVal dir = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS3(env, pkg, dir, body);

    uint_t size = YogArray_size(env, vm->search_path);
    uint_t i;
    for (i = 0; i < size; i++) {
        dir = YogArray_at(env, vm->search_path, i);
        body = PTR_AS(YogString, dir)->body;
        size_t dir_len = strlen(PTR_AS(YogCharArray, body)->items);

        size_t len = strlen(path_head);
#define HEAD2PATH(var, ext) \
    /* directory + separactor + head + extention + '\0' */ \
    size_t size_##var = dir_len + 1 + len + strlen(ext) + 1; \
    char* var = (char*)alloca(sizeof(char) * size_##var); \
    join_path(var, PTR_AS(YogCharArray, body)->items, path_head, size_##var); \
    strlcat(var, ext, size_##var)
        HEAD2PATH(yg, ".yg");
        pkg = import_yg(env, yg, pkg_name);
        if (IS_PTR(pkg)) {
            RETURN(env, pkg);
        }

#if defined(__MINGW32__) || defined(_MSC_VER)
#   define SOEXT   ".dll"
#else
#   define SOEXT   ".so"
#endif
        HEAD2PATH(so, SOEXT);
#undef SOEXT
#undef HEAD2PATH
        pkg = import_so(env, vm, so, pkg_name);
        if (IS_PTR(pkg)) {
            RETURN(env, pkg);
        }
    }

    YogError_raise_ImportError(env, "no package named \"%s\"", pkg_name);

    /* NOTREACHED */
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

    uint_t len = strlen(name) + 1;
    char* head = (char*)alloca(sizeof(char) * len);
    strlcpy(head, name, len);
    package_name2path_head(head);

    pkg = import(env, vm, head, name);
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
    YogVal s = YUNDEF;
    PUSH_LOCALS4(env, top, parent, pkg, s);

    s = YogVM_id2name(env, env->vm, name);
    uint_t n = 0;
    while (1) {
        const char* begin = STRING_CSTR(s);
        const char* end = strchr(begin + n, '.');
        if (end == NULL) {
            end = begin + STRING_SIZE(s) - 1;
        }
        uint_t endpos = end - begin;
        char* str = (char*)alloca(sizeof(char) * (endpos + 1));
        strncpy(str, begin, endpos);
        str[endpos] = '\0';

        pkg = import_package(env, vm, str);
        if (IS_PTR(parent)) {
            uint_t size = endpos - n;
            char* attr = (char*)alloca(sizeof(char) * (size + 1));
            strncpy(attr, STRING_CSTR(s) + n, size);
            attr[size] = '\0';
            ID id = YogVM_intern(env, vm, attr);
            YogObj_set_attr_id(env, parent, id, pkg);
        }
        if (!IS_PTR(top)) {
            top = pkg;
        }

        if (endpos == STRING_SIZE(s) - 1) {
            break;
        }

        parent = pkg;
        n = endpos + 1;
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
#if defined(_MSC_VER)
    uint_t attr = GetFileAttributes(filename);
    return attr & FILE_ATTRIBUTE_DIRECTORY;
#else
    struct stat buf;
    if (stat(filename, &buf) != 0) {
        return FALSE;
    }
    if (!S_ISDIR(buf.st_mode)) {
        return FALSE;
    }
    return TRUE;
#endif
}

static BOOL
is_executable_file(const char* filename)
{
#if defined(_MSC_VER)
    uint_t attrs = GetFileAttributes(filename);
    return attrs & FILE_ATTRIBUTE_NORMAL;
#else
    struct stat buf;
    if (stat(filename, &buf) != 0) {
        return FALSE;
    }
    if (!S_ISREG(buf.st_mode)) {
        return FALSE;
    }
    uint_t mode;
#if defined(__MINGW32__)
    mode = S_IEXEC;
#else
    mode = S_IXUSR | S_IXGRP | S_IXOTH;
#endif
    if ((buf.st_mode & mode) == 0) {
        return FALSE;
    }
    return TRUE;
#endif
}

static BOOL
search_program(char* dest, const char* path, const char* prog, size_t size)
{
    size_t len = strlen(path) + 1;
    char* s = (char*)alloca(sizeof(char) * len);
    strlcpy(s, path, len);

    char* pc = s;
    while (1) {
        char* delim = strchr(pc, ':');
        split_path(delim);
        join_path(dest, pc, prog, size);
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
        size_t size = strlen(path) + 1 + strlen(argv0) + 1;
        char* s = (char*)alloca(sizeof(char) * size);
        if (!search_program(s, path, argv0, size)) {
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
    uint_t len = YogString_size(env, prog);
#if defined(_MSC_VER)
#   define ROOT_DIR_DEV     "..\\..\\..\\"
#else
#   define ROOT_DIR_DEV     "../"
#endif
#define EXT_DIR     ROOT_DIR_DEV "ext"
    /* 1 of middle is for '/' */
    size_t size = len + 1 + strlen(EXT_DIR) + 1;
    char* extpath = (char*)alloca(sizeof(char) * size);
    body = PTR_AS(YogString, prog)->body;
    join_path(extpath, PTR_AS(YogCharArray, body)->items, EXT_DIR, size);
#undef EXT_DIR
    if (is_directory(extpath)) {
        s = YogString_new_str(env, extpath);
        YogArray_push(env, search_path, s);

#define LIB_DIR     ROOT_DIR_DEV "lib"
        size_t size = len + 1 + strlen(LIB_DIR) + 1;
        char* libpath = (char*)alloca(sizeof(char) * size);
        join_path(libpath, PTR_AS(YogCharArray, body)->items, LIB_DIR, size);
#undef LIB_DIR
        s = YogString_new_str(env, libpath);
        YogArray_push(env, search_path, s);
    }
    else {
#if defined(_MSC_VER)
#   define EXT_DIR  "..\\lib"
#else
#   define EXT_DIR  PREFIX "/lib/yog/" VERSION
#endif
        s = YogString_new_str(env, EXT_DIR);
#undef EXT_DIR
        YogArray_push(env, search_path, s);
    }
    vm->search_path = search_path;
#undef ROOT_DIR_DEV

    RETURN_VOID(env);
}

uint_t
YogVM_issue_thread_id(YogEnv* env, YogVM* vm)
{
    pthread_mutex_t* lock = &vm->next_thread_id_lock;
    acquire_lock(env, lock);
    uint_t id = vm->next_thread_id;
    vm->next_thread_id++;
    YOG_ASSERT(env, vm->next_thread_id != 0, "thread id overflow");
    release_lock(env, lock);
    return id;
}

void
YogVM_add_locals(YogEnv* env, YogVM* vm, YogLocalsAnchor* locals)
{
    YogVM_acquire_global_interp_lock(env, vm);
    ADD_TO_LIST(vm->locals, locals);
    YogVM_release_global_interp_lock(env, vm);
}

void
YogVM_remove_locals(YogEnv* env, YogVM* vm, YogLocalsAnchor* locals)
{
    YogVM_acquire_global_interp_lock(env, vm);
    DELETE_FROM_LIST(vm->locals, locals);
    YogVM_release_global_interp_lock(env, vm);
}

static void
acquire_indirect_ptr_lock(YogEnv* env, YogVM* vm)
{
    acquire_lock(env, &vm->indirect_ptr_lock);
}

static void
release_indirect_ptr_lock(YogEnv* env, YogVM* vm)
{
    release_lock(env, &vm->indirect_ptr_lock);
}

YogIndirectPointer*
YogVM_alloc_indirect_ptr(YogEnv* env, YogVM* vm, YogVal val)
{
    acquire_indirect_ptr_lock(env, vm);
    YogIndirectPointer* ptr = malloc(sizeof(YogIndirectPointer));
    if (ptr == NULL) {
        YogError_out_of_memory(env);
    }
    ADD_TO_LIST(vm->indirect_ptr, ptr);
    ptr->val = val;
    release_indirect_ptr_lock(env, vm);
    return ptr;
}

void
YogVM_free_indirect_ptr(YogEnv* env, YogVM* vm, YogIndirectPointer* ptr)
{
    acquire_indirect_ptr_lock(env, vm);
    DELETE_FROM_LIST(vm->indirect_ptr, ptr);
    YogGC_free_memory(env, ptr, sizeof(YogIndirectPointer));
    release_indirect_ptr_lock(env, vm);
}

YogEnv*
YogVM_get_env(YogVM* vm)
{
    YogVM_acquire_global_interp_lock(NULL, vm);
    pthread_t self = pthread_self();
    YogVal thread = vm->running_threads;
    while (IS_PTR(thread)) {
        if (pthread_equal(self, PTR_AS(YogThread, thread)->pthread)) {
            break;
        }
        thread = PTR_AS(YogThread, thread)->next;
    }
    YogVM_release_global_interp_lock(NULL, vm);

    if (IS_PTR(thread)) {
        return PTR_AS(YogThread, thread)->env;
    }
    else {
        return NULL;
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
