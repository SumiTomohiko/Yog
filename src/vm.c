#include "yog/config.h"
#if defined(HAVE_DLFCN_H)
#   include <dlfcn.h>
#endif
#include <errno.h>
#if defined(HAVE_MALLOC_H) && !defined(__OpenBSD__)
#   include <malloc.h>
#endif
#include <pthread.h>
#include <stdio.h>
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
#include "yog/binary.h"
#include "yog/bool.h"
#include "yog/builtins.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/classmethod.h"
#include "yog/code.h"
#include "yog/comparable.h"
#include "yog/compile.h"
#include "yog/coroutine.h"
#include "yog/dict.h"
#include "yog/encoding.h"
#include "yog/env.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/ffi.h"
#include "yog/file.h"
#include "yog/fixnum.h"
#include "yog/float.h"
#include "yog/gc.h"
#include "yog/gc/bdw.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/module.h"
#include "yog/nil.h"
#include "yog/package.h"
#include "yog/private.h"
#include "yog/property.h"
#include "yog/regexp.h"
#include "yog/set.h"
#include "yog/string.h"
#include "yog/symbol.h"
#include "yog/sysdeps.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"

#if WINDOWS
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
    YogGC_free_from_gc(env);
    pthread_rwlock_rdlock(lock);
    YogGC_bind_to_gc(env);
}

static void
acquire_write_lock(YogEnv* env, pthread_rwlock_t* lock)
{
    YogGC_free_from_gc(env);
    pthread_rwlock_wrlock(lock);
    YogGC_bind_to_gc(env);
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
    s = YogString_of_size(env, size);
    memcpy(STRING_CSTR(s), PTR_AS(YogCharArray, val)->items, size);
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
    char* buffer = (char*)YogSysdeps_alloca(sizeof(char) * size);
    memcpy(buffer, name, size);

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
setup_basic_classes(YogEnv* env, YogVM* vm)
{
    SAVE_LOCALS(env);
    YogVal cObject = YUNDEF;
    YogVal cClass = YUNDEF;
    PUSH_LOCALS2(env, cObject, cClass);

    cObject = YogClass_new(env, "Object", YNIL);
    YogClass_define_allocator(env, cObject, YogObj_alloc);

    cClass = YogClass_new(env, "Class", cObject);
    YogClass_define_allocator(env, cClass, YogClass_alloc);

    YogGC_UPDATE_PTR(env, PTR_AS(YogBasicObj, cObject), klass, cClass);
    YogGC_UPDATE_PTR(env, PTR_AS(YogBasicObj, cClass), klass, cClass);

    vm->cObject = cObject;
    vm->cClass = cClass;

    RETURN_VOID(env);
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
    YogBinary_define_classes(env, builtins);
    YogBool_define_classes(env, builtins);
    YogClassMethod_define_classes(env, builtins);
    YogCode_define_classes(env, builtins);
    YogCoroutine_define_classes(env, builtins);
    YogDict_define_classes(env, builtins);
    YogEncoding_define_classes(env, builtins);
    YogEnv_define_classes(env, builtins);
    YogFFI_define_classes(env, builtins);
    YogFile_define_classes(env, builtins);
    YogFixnum_define_classes(env, builtins);
    YogFloat_define_classes(env, builtins);
    YogModule_define_classes(env, builtins);
    YogNil_define_classes(env, builtins);
    YogPackage_define_classes(env, builtins);
    YogRegexp_define_classes(env, builtins);
    YogSet_define_classes(env, builtins);
    YogString_define_classes(env, builtins);
    YogSymbol_define_classes(env, builtins);
    YogThread_define_classes(env, builtins);

    RETURN_VOID(env);
}

static void
register_encoding(YogEnv* env, YogVM* vm, const char* name, YogVal enc)
{
    SAVE_ARG(env, enc);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = YogString_from_str(env, name);
    YogDict_set(env, vm->encodings, s, enc);

    RETURN_VOID(env);
}

static void
setup_encodings(YogEnv* env, YogVM* vm)
{
    SAVE_LOCALS(env);
    YogVal enc = YUNDEF;
    YogVal ascii = YUNDEF;
    YogVal utf8 = YUNDEF;
    PUSH_LOCALS3(env, enc, ascii, utf8);

    ascii = YogEncoding_new(env, ONIG_ENCODING_ASCII);
    vm->encAscii = ascii;
    utf8 = YogEncoding_new(env, ONIG_ENCODING_UTF8);
    vm->encUtf8 = utf8;
    register_encoding(env, vm, "ascii", ascii);
    register_encoding(env, vm, "utf-8", utf8);

#define REGISTER_ENCODING(name, onig)   do { \
    enc = YogEncoding_new(env, (onig)); \
    register_encoding(env, vm, (name), enc); \
} while (0)
    REGISTER_ENCODING("euc-jp", ONIG_ENCODING_EUC_JP);
    REGISTER_ENCODING("shift-jis", ONIG_ENCODING_SJIS);
#undef REGISTER_ENCODING

    RETURN_VOID(env);
}

static void
set_main_thread_class(YogEnv* env, YogVM* vm)
{
    YogGC_UPDATE_PTR(env, PTR_AS(YogBasicObj, vm->main_thread), klass, vm->cThread);
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
    YogHandleScope scope;
    YogHandleScope_OPEN(env, &scope);

    setup_symbol_tables(env, vm);
    setup_basic_classes(env, vm);
    YogHandle* builtins = YogHandle_REGISTER(env, alloc_skelton_pkg(env, vm));
    setup_classes(env, vm, HDL2VAL(builtins));
    YogPackage_init(env, HDL2VAL(builtins), TYPE_PACKAGE);
    set_main_thread_class(env, vm);
    YogException_define_classes(env, HDL2VAL(builtins));
    YogObject_boot(env, vm->cObject, HDL2VAL(builtins));
    YogClass_boot(env, vm->cClass, HDL2VAL(builtins));

    vm->pkgs = YogTable_new_symbol_table(env);

    vm->encodings = YogDict_new(env);
    setup_encodings(env, vm);

    vm->finish_code = YogCompiler_compile_finish_code(env);

    setup_builtins(env, vm, HDL2VAL(builtins), argc, argv);
    YogArray_eval_builtin_script(env, vm->cArray);
    YogDict_eval_builtin_script(env, vm->cDict);
    YogObject_eval_builtin_script(env, vm->cObject);
    YogString_eval_builtin_script(env, vm->cString);

    vm->id_star = YogVM_intern(env, vm, "*");
    vm->id_star2 = YogVM_intern(env, vm, "**");
    vm->id_amp = YogVM_intern(env, vm, "&");

    YogHandleScope_close(env);
}

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
        *val = YogGC_keep(env, *val, keeper, heap);
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
        DEBUG(TRACE("list=%p, list->filename=\"%s\", list->lineno=%u, list->num_vals=%u, list->next=%p", list, list->filename, list->lineno, list->num_vals, list->next));
        keep_locals(env, list, keeper, heap);
        list = list->next;
    }
}

static void
keep_handle(YogEnv* env, YogHandle* begin, YogHandle* end, ObjectKeeper keeper, void* heap)
{
    YogHandle* p;
    for (p = begin; p < end; p++) {
        p->val = YogGC_keep(env, p->val, keeper, heap);
    }
}

static void
keep_scope1(YogEnv* env, YogHandles* handles, YogHandle* pos, uint_t end, ObjectKeeper keeper, void* heap)
{
    if (pos == NULL) {
        return;
    }
    keep_handle(env, handles->ptr[end - 1], pos, keeper, heap);
}

static void
keep_scope2(YogEnv* env, YogHandles* handles, uint_t begin , uint_t end, ObjectKeeper keeper, void* heap)
{
    if (begin == end) {
        return;
    }
    uint_t i;
    for (i = end - 1; begin < i; i--) {
        YogHandle* begin = handles->ptr[i - 1];
        YogHandle* end = begin + HANDLES_SIZE;
        keep_handle(env, begin, end, keeper, heap);
    }
}

static void
keep_handles(YogEnv* env, YogHandles* handles, ObjectKeeper keeper, void* heap)
{
    uint_t end = handles->used_num;
    if (end == 0) {
        return;
    }
    YogHandleScope* scope = handles->scope;
    while (scope != NULL) {
        keep_scope1(env, handles, scope->pos, end, keeper, heap);

        uint_t begin = end - scope->used_num;
        keep_scope2(env, handles, begin, end, keeper, heap);

        end = begin;
        scope = scope->next;
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
    YogHandles* handles = vm->handles;
    while (handles != NULL) {
        keep_handles(env, handles, keeper, handles->heap);
        handles = handles->next;
    }

#define KEEP(member)    do { \
    vm->member = YogGC_keep(env, vm->member, keeper, heap); \
} while (0)
    KEEP(id2name);
    KEEP(name2id);

    KEEP(cArray);
    KEEP(cArrayField);
    KEEP(cBignum);
    KEEP(cBinary);
    KEEP(cBool);
    KEEP(cBuffer);
    KEEP(cBufferField);
    KEEP(cClass);
    KEEP(cClassMethod);
    KEEP(cCode);
    KEEP(cCoroutine);
    KEEP(cDict);
    KEEP(cEncoding);
    KEEP(cEnv);
    KEEP(cField);
    KEEP(cFieldArray);
    KEEP(cFile);
    KEEP(cFixnum);
    KEEP(cFloat);
    KEEP(cFunction);
    KEEP(cInstanceMethod);
    KEEP(cInt);
    KEEP(cLib);
    KEEP(cLibFunc);
    KEEP(cMatch);
    KEEP(cModule);
    KEEP(cNativeFunction);
    KEEP(cNativeFunction2);
    KEEP(cNativeInstanceMethod);
    KEEP(cNativeInstanceMethod2);
    KEEP(cNil);
    KEEP(cObject);
    KEEP(cPackage);
    KEEP(cPointer);
    KEEP(cProperty);
    KEEP(cRegexp);
    KEEP(cSet);
    KEEP(cString);
    KEEP(cStringField);
    KEEP(cStructClass);
    KEEP(cStructClassClass);
    KEEP(cSymbol);
    KEEP(cThread);

    KEEP(eArgumentError);
    KEEP(eAttributeError);
    KEEP(eCoroutineError);
    KEEP(eEOFError);
    KEEP(eException);
    KEEP(eFFIError);
    KEEP(eIOError);
    KEEP(eImportError);
    KEEP(eIndexError);
    KEEP(eKeyError);
    KEEP(eLocalJumpError);
    KEEP(eNameError);
    KEEP(eSyntaxError);
    KEEP(eSystemError);
    KEEP(eTypeError);
    KEEP(eUnboundLocalError);
    KEEP(eValueError);
    KEEP(eWindowsError);
    KEEP(eZeroDivisionError);

    KEEP(mComparable);

    KEEP(pkgs);
    KEEP(search_path);
    KEEP(encodings);
    KEEP(encAscii);
    KEEP(encUtf8);
    KEEP(finish_code);
    KEEP(main_thread);
    KEEP(running_threads);

    KEEP(path_separator);
#undef KEEP

    YogIndirectPointer* indirect_ptr = vm->indirect_ptr;
    while (indirect_ptr != NULL) {
        indirect_ptr->val = YogGC_keep(env, indirect_ptr->val, keeper, heap);
        indirect_ptr = indirect_ptr->next;
    }
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
YogVM_disable_gc_stress(YogEnv* env, YogVM* vm)
{
    vm->gc_stress = FALSE;
}

void
YogVM_enable_gc_stress(YogEnv* env, YogVM* vm)
{
    vm->gc_stress = TRUE;
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
    INIT(cArrayField);
    INIT(cBignum);
    INIT(cBinary);
    INIT(cBool);
    INIT(cBuffer);
    INIT(cBufferField);
    INIT(cClass);
    INIT(cClassMethod);
    INIT(cCode);
    INIT(cCoroutine);
    INIT(cDict);
    INIT(cEncoding);
    INIT(cEnv);
    INIT(cField);
    INIT(cFieldArray);
    INIT(cFile);
    INIT(cFixnum);
    INIT(cFloat);
    INIT(cFunction);
    INIT(cInstanceMethod);
    INIT(cInt);
    INIT(cLib);
    INIT(cLibFunc);
    INIT(cMatch);
    INIT(cModule);
    INIT(cNativeFunction);
    INIT(cNativeFunction2);
    INIT(cNativeInstanceMethod);
    INIT(cNativeInstanceMethod2);
    INIT(cNil);
    INIT(cObject);
    INIT(cPackage);
    INIT(cPointer);
    INIT(cProperty);
    INIT(cRegexp);
    INIT(cSet);
    INIT(cString);
    INIT(cStringField);
    INIT(cStructClass);
    INIT(cStructClassClass);
    INIT(cSymbol);
    INIT(cThread);

    INIT(eArgumentError);
    INIT(eAttributeError);
    INIT(eCoroutineError);
    INIT(eEOFError);
    INIT(eException);
    INIT(eFFIError);
    INIT(eIOError);
    INIT(eImportError);
    INIT(eIndexError);
    INIT(eKeyError);
    INIT(eLocalJumpError);
    INIT(eNameError);
    INIT(eSyntaxError);
    INIT(eSystemError);
    INIT(eTypeError);
    INIT(eUnboundLocalError);
    INIT(eValueError);
    INIT(eWindowsError);
    INIT(eZeroDivisionError);

    INIT(mComparable);

    vm->pkgs = YNIL;
    init_read_write_lock(&vm->pkgs_lock);
    INIT(search_path);

    INIT(encodings);
    INIT(encAscii);
    INIT(encUtf8);

    INIT(finish_code);

    INIT(running_threads);
    vm->next_thread_id = 0;
    pthread_mutex_init(&vm->next_thread_id_lock, NULL);

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
    vm->handles = NULL;
#if defined(GC_GENERATIONAL)
    vm->major_gc_flag = FALSE;
    vm->compaction_flag = FALSE;
#endif

    vm->indirect_ptr = NULL;
    pthread_mutex_init(&vm->indirect_ptr_lock, NULL);

    vm->debug_import = FALSE;
    INIT(path_separator);
#undef INIT
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

    YogGC_UPDATE_PTR(env, PTR_AS(YogThread, vm->running_threads), prev, thread);
    YogGC_UPDATE_PTR(env, PTR_AS(YogThread, thread), next, vm->running_threads);
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
        YogGC_UPDATE_PTR(env, PTR_AS(YogThread, prev), next, next);
    }
    else {
        vm->running_threads = next;
    }
    if (IS_PTR(next)) {
        YogGC_UPDATE_PTR(env, PTR_AS(YogThread, next), prev, prev);
    }

    if (!IS_PTR(vm->running_threads)) {
        pthread_cond_signal(&vm->vm_finish_cond);
    }

    RESTORE_LOCALS(env);

    YogVM_release_global_interp_lock(env, vm);
}

#if !defined(GC_BDW)
void
YogVM_add_heap(YogEnv* env, YogVM* vm, YogHeap* heap)
{
    YogVM_acquire_global_interp_lock(env, vm);
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

DECL_AS_TYPE(ImportingPackage_new);
#define TYPE_IMPORTING_PKG TO_TYPE(ImportingPackage_new)

static void
ImportingPackage_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    ImportingPackage* pkg = PTR_AS(ImportingPackage, ptr);
    YogGC_KEEP(env, pkg, pkg, keeper, heap);
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
        YogGC_free_from_gc(env);
        pthread_cond_wait(cond, lock);
        YogGC_bind_to_gc(env);
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

static void
print_dlopen_error(YogEnv* env)
{
    SAVE_LOCALS(env);
    if (!env->vm->debug_import) {
        RETURN_VOID(env);
    }
    const char* msg;
#if defined(__MINGW32__) || defined(_MSC_VER)
    TCHAR buf[1024];
    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, buf, array_sizeof(buf), NULL) != 0) {
        RETURN_VOID(env);
    }
    msg = (const char*)buf;
#else
    msg = dlerror();
    if (msg == NULL) {
        RETURN_VOID(env);
    }
#endif
    fprintf(stderr, "%s\n", msg);
    RETURN_VOID(env);
}

static YogVal
import_so(YogEnv* env, YogVM* vm, const char* filename, const char* pkg_name)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    size_t size = strlen(filename);
    char* path;
    if (strchr(filename, SEPARATOR) != NULL) {
        path = (char*)YogSysdeps_alloca(sizeof(char) * (size + 1));
        memcpy(path, filename, size + 1);
    }
    else {
        /* 2 is for the string of "./" excluding null terminator */
#define CUR_DIR_SIZE    2
        size_t len = size + CUR_DIR_SIZE + 1;
        path = (char*)YogSysdeps_alloca(sizeof(char) * len);
        YogSysdeps_snprintf(path, len, ".%c", SEPARATOR);
        memcpy(path + CUR_DIR_SIZE, filename, size + 1);
#undef CUR_DIR_SIZE
    }

#if defined(__MINGW32__) || defined(_MSC_VER)
#   define CLEAR_ERROR
#else
#   define CLEAR_ERROR     dlerror()
#endif
    CLEAR_ERROR;
    LIB_HANDLE handle = YogSysdeps_open_lib(path);
    if (handle == NULL) {
        print_dlopen_error(env);
        RETURN(env, YUNDEF);
    }

#define INIT_NAME_HEAD  "YogInit_"
    size_t head_size = strlen(INIT_NAME_HEAD);
    size_t init_name_size = head_size + strlen(pkg_name) + 1;
    char* init_name = (char*)YogSysdeps_alloca(sizeof(char) * init_name_size);
    memcpy(init_name, INIT_NAME_HEAD, head_size);
#undef INIT_NAME_HEAD
    const char* pc = strrchr(pkg_name, '.');
    if (pc != NULL) {
        pc++;
    }
    else {
        pc = pkg_name;
    }
    memcpy(init_name + head_size, pc, init_name_size - head_size - 1);
    init_name[init_name_size - 1] = '\0';

    CLEAR_ERROR;
    typedef YogVal (*Initializer)(YogEnv*);
    Initializer init = (Initializer)YogSysdeps_get_proc(handle, init_name);
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

static char*
join_path(char* dest, const char* head, const char* tail)
{
    size_t head_size = strlen(head);
    memcpy(dest, head, head_size);
    dest[head_size] = SEPARATOR;
    size_t tail_size = strlen(tail);
    memcpy(dest + head_size + 1, tail, tail_size);
    char* p = &dest[head_size + tail_size + 1];
    *p = '\0';

    return p;
}

static void
print_error(YogEnv* env, const char* filename)
{
    if (!env->vm->debug_import) {
        return;
    }
    perror(filename);
}

static YogVal
import_yg(YogEnv* env, const char* yg, const char* pkg_name)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    FILE* fp = fopen(yg, "r");
    if (fp == NULL) {
        print_error(env, yg);
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
    /* directory + separator + head + extention + '\0' */ \
    size_t size_##var = dir_len + 1 + len + strlen(ext) + 1; \
    char* var = (char*)YogSysdeps_alloca(sizeof(char) * size_##var); \
    char* p_##var = join_path(var, PTR_AS(YogCharArray, body)->items, path_head); \
    memcpy(p_##var, ext, strlen(ext) + 1)
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
    char* head = (char*)YogSysdeps_alloca(sizeof(char) * len);
    memcpy(head, name, len);
    package_name2path_head(head);

    pkg = import(env, vm, head, name);
    YogGC_UPDATE_PTR(env, PTR_AS(ImportingPackage, tmp_pkg), pkg, pkg);

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
YogVM_import_package(YogEnv* env, YogVM* vm, YogVal name)
{
    SAVE_ARG(env, name);
    YogVal top = YUNDEF;
    YogVal parent = YUNDEF;
    YogVal pkg = YUNDEF;
    PUSH_LOCALS3(env, top, parent, pkg);

    uint_t n = 0;
    while (1) {
        const char* begin = STRING_CSTR(name);
        const char* end = strchr(begin + n, '.');
        if (end == NULL) {
            end = begin + STRING_SIZE(name) - 1;
        }
        uint_t endpos = end - begin;
        char* str = (char*)YogSysdeps_alloca(sizeof(char) * (endpos + 1));
        strncpy(str, begin, endpos);
        str[endpos] = '\0';

        pkg = import_package(env, vm, str);
        if (IS_PTR(parent)) {
            uint_t size = endpos - n;
            char* attr = (char*)YogSysdeps_alloca(sizeof(char) * (size + 1));
            strncpy(attr, STRING_CSTR(name) + n, size);
            attr[size] = '\0';
            ID id = YogVM_intern(env, vm, attr);
            YogObj_set_attr_id(env, parent, id, pkg);
        }
        if (!IS_PTR(top)) {
            top = pkg;
        }

        if (endpos == STRING_SIZE(name) - 1) {
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
search_program(char* dest, const char* path, const char* prog)
{
    size_t len = strlen(path) + 1;
    char* s = (char*)YogSysdeps_alloca(sizeof(char) * len);
    memcpy(s, path, len);

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

static char*
find_separactor(YogEnv* env, const char* s)
{
#if WINDOWS
    const char seps[] = { '\\', '/' };
#else
    const char seps[] = { '/' };
#endif
    uint_t i;
    for (i = 0; i < array_sizeof(seps); i++) {
        char* pc = strrchr(s, seps[i]);
        if (pc != NULL) {
            return pc;
        }
    }
    return NULL;
}

static void
dirname(YogEnv* env, YogVal filename)
{
    SAVE_ARG(env, filename);

    char* pc = find_separactor(env, STRING_CSTR(filename));
    YOG_ASSERT(env, pc != NULL, "%s doesn't include directory name", filename);
    *pc = '\0';
    STRING_SIZE(filename) = pc - STRING_CSTR(filename) + 1;

    RETURN_VOID(env);
}

static void
add_current_directory(YogEnv* env, YogVal search_path)
{
    SAVE_ARG(env, search_path);

    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    s = YogString_from_str(env, ".");
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

    if (find_separactor(env, argv0) == NULL) {
        char* path = getenv("PATH");
        YOG_ASSERT(env, path != NULL, "PATH is empty");
        /* 1 of middle is for '/' */
        size_t size = strlen(path) + 1 + strlen(argv0) + 1;
        char* s = (char*)YogSysdeps_alloca(sizeof(char) * size);
        if (!search_program(s, path, argv0)) {
            YOG_BUG(env, "Can't find %s in %s", argv0, path);
        }
        prog = YogString_from_str(env, s);
    }
    else {
        prog = YogString_from_str(env, argv0);
    }

    search_path = YogArray_new(env);
    add_current_directory(env, search_path);

    dirname(env, prog);
    uint_t len = YogString_size(env, prog);
#if WINDOWS
#   define ROOT_DIR_DEV     "..\\"
#else
#   define ROOT_DIR_DEV     "../"
#endif
#define EXT_DIR     ROOT_DIR_DEV "ext"
    /* 1 of middle is for '/' */
    size_t size = len + 1 + strlen(EXT_DIR) + 1;
    char* extpath = (char*)YogSysdeps_alloca(sizeof(char) * size);
    join_path(extpath, STRING_CSTR(prog), EXT_DIR);
#undef EXT_DIR
    if (is_directory(extpath)) {
        s = YogString_from_str(env, extpath);
        YogArray_push(env, search_path, s);

#define LIB_DIR     ROOT_DIR_DEV "lib"
        size_t size = len + 1 + strlen(LIB_DIR) + 1;
        char* libpath = (char*)YogSysdeps_alloca(sizeof(char) * size);
        join_path(libpath, STRING_CSTR(prog), LIB_DIR);
#undef LIB_DIR
        s = YogString_from_str(env, libpath);
        YogArray_push(env, search_path, s);
    }
    else {
#if WINDOWS
        YogString_add_cstr(env, prog, "\\..\\lib");
        YogArray_push(env, search_path, prog);
#else
        s = YogString_from_str(env, PREFIX "/lib/yog/" PACKAGE_VERSION);
        YogArray_push(env, search_path, s);
#endif
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
YogVM_add_handles(YogEnv* env, YogVM* vm, YogHandles* handles)
{
    YogVM_acquire_global_interp_lock(env, vm);
    ADD_TO_LIST(vm->handles, handles);
    YogVM_release_global_interp_lock(env, vm);
}

void
YogVM_remove_handles(YogEnv* env, YogVM* vm, YogHandles* handles)
{
    YogVM_acquire_global_interp_lock(env, vm);
    DELETE_FROM_LIST(vm->handles, handles);
    YogVM_release_global_interp_lock(env, vm);
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
    size_t size = sizeof(YogIndirectPointer);
    YogIndirectPointer* ptr = (YogIndirectPointer*)malloc(size);
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
    YogGC_free(env, ptr, sizeof(YogIndirectPointer));
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
