#if !defined(YOG_VM_H_INCLUDED)
#define YOG_VM_H_INCLUDED

#include <pthread.h>
#if HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif
#include "yog/gc.h"
#include "yog/yog.h"

struct YogVM {
    BOOL gc_stress;

    ID next_id;
    YogVal id2name;
    YogVal name2id;
    pthread_rwlock_t sym_lock;

    YogVal cArray;
    YogVal cArrayField;
    YogVal cBignum;
    YogVal cBinary;
    YogVal cBool;
    YogVal cBuffer;
    YogVal cBufferField;
    YogVal cClass;
    YogVal cClassMethod;
    YogVal cCode;
    YogVal cCoroutine;
    YogVal cDict;
    YogVal cEncoding;
    YogVal cEnv;
    YogVal cField;
    YogVal cFieldArray;
    YogVal cFile;
    YogVal cFixnum;
    YogVal cFloat;
    YogVal cFunction;
    YogVal cInstanceMethod;
    YogVal cInt;
    YogVal cLib;
    YogVal cLibFunc;
    YogVal cMatch;
    YogVal cModule;
    YogVal cNativeFunction2;
    YogVal cNativeFunction;
    YogVal cNativeInstanceMethod2;
    YogVal cNativeInstanceMethod;
    YogVal cNil;
    YogVal cObject;
    YogVal cPackage;
    YogVal cPath;
    YogVal cPointer;
    YogVal cPointerField;
    YogVal cProcess;
    YogVal cProperty;
    YogVal cRegexp;
    YogVal cSet;
    YogVal cString;
    YogVal cStringField;
    YogVal cStructBase;
    YogVal cStructClass;
    YogVal cStructClassClass;
    YogVal cStructField;
    YogVal cSymbol;
    YogVal cThread;
    YogVal cUnionClass;
    YogVal cUnionClassClass;

    YogVal eArgumentError;
    YogVal eAttributeError;
    YogVal eCoroutineError;
    YogVal eEOFError;
    YogVal eException;
    YogVal eFFIError;
    YogVal eIOError;
    YogVal eImportError;
    YogVal eIndexError;
    YogVal eKeyError;
    YogVal eLocalJumpError;
    YogVal eNameError;
    YogVal eSyntaxError;
    YogVal eSystemError;
    YogVal eTypeError;
    YogVal eUnboundLocalError;
    YogVal eValueError;
    YogVal eWindowsError;
    YogVal eZeroDivisionError;

    YogVal mComparable;

    YogVal pkgs;
    pthread_rwlock_t pkgs_lock;
    YogVal search_path;

    YogVal encodings;
    YogVal encAscii;
    YogVal encUtf8;
    YogVal default_encoding;

    ID id_star;
    ID id_star2;
    ID id_amp;

    YogVal finish_code;

    YogVal main_thread;
    YogVal running_threads;
    uint_t next_thread_id;
    pthread_mutex_t next_thread_id_lock;

    pthread_mutex_t global_interp_lock;
    BOOL running_gc;
    BOOL waiting_suspend;
    uint_t suspend_counter;
    pthread_cond_t threads_suspend_cond;
    pthread_cond_t gc_finish_cond;
    YogHeap* heaps;
    YogHeap* last_heap;
    pthread_cond_t vm_finish_cond;
    uint_t gc_id;
    struct YogLocalsAnchor* locals;
    struct YogHandles* handles;
#if defined(GC_GENERATIONAL)
    /**
     * Generational GC needs kind of global variables. The following two
     * variables role this.
     */
    BOOL major_gc_flag;
    BOOL compaction_flag;
#endif

    struct YogIndirectPointer* indirect_ptr;
    pthread_mutex_t indirect_ptr_lock;

    BOOL debug_import;
    YogVal path_separator;
};

typedef struct YogVM YogVM;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/vm.c */
YOG_EXPORT void YogVM_acquire_global_interp_lock(YogEnv*, YogVM*);
YOG_EXPORT void YogVM_add_handles(YogEnv*, YogVM*, YogHandles*);
YOG_EXPORT void YogVM_add_heap(YogEnv*, YogVM*, YogHeap*);
YOG_EXPORT void YogVM_add_locals(YogEnv*, YogVM*, YogLocalsAnchor*);
YOG_EXPORT void YogVM_add_thread(YogEnv*, YogVM*, YogVal);
YOG_EXPORT YogIndirectPointer* YogVM_alloc_indirect_ptr(YogEnv*, YogVM*, YogVal);
YOG_EXPORT void YogVM_boot(YogEnv*, YogVM*);
YOG_EXPORT void YogVM_configure_search_path(YogEnv*, YogVM*, YogHandle*);
YOG_EXPORT void YogVM_delete(YogEnv*, YogVM*);
YOG_EXPORT void YogVM_disable_gc_stress(YogEnv*, YogVM*);
YOG_EXPORT void YogVM_enable_gc_stress(YogEnv*, YogVM*);
YOG_EXPORT void YogVM_free_indirect_ptr(YogEnv*, YogVM*, YogIndirectPointer*);
YOG_EXPORT YogEnv* YogVM_get_env(YogVM*);
YOG_EXPORT YogVal YogVM_id2bin(YogEnv*, YogVM*, ID);
YOG_EXPORT YogVal YogVM_id2name(YogEnv*, YogVM*, ID);
YOG_EXPORT YogHandle* YogVM_import_package(YogEnv*, YogVM*, YogHandle*);
YOG_EXPORT void YogVM_init(YogVM*);
YOG_EXPORT ID YogVM_intern(YogEnv*, YogVM*, const char*);
YOG_EXPORT ID YogVM_intern2(YogEnv*, YogVM*, YogVal);
YOG_EXPORT uint_t YogVM_issue_thread_id(YogEnv*, YogVM*);
YOG_EXPORT void YogVM_keep_children(YogEnv*, void*, ObjectKeeper, void*);
YOG_EXPORT void YogVM_register_args(YogEnv*, YogVM*, YogHandle*);
YOG_EXPORT void YogVM_register_executable(YogEnv*, YogVM*, YogHandle*);
YOG_EXPORT void YogVM_register_package(YogEnv*, YogVM*, YogHandle*, YogHandle*);
YOG_EXPORT void YogVM_release_global_interp_lock(YogEnv*, YogVM*);
YOG_EXPORT void YogVM_remove_handles(YogEnv*, YogVM*, YogHandles*);
YOG_EXPORT void YogVM_remove_locals(YogEnv*, YogVM*, YogLocalsAnchor*);
YOG_EXPORT void YogVM_remove_thread(YogEnv*, YogVM*, YogVal);
YOG_EXPORT void YogVM_set_main_thread(YogEnv*, YogVM*, YogVal);
YOG_EXPORT void YogVM_wait_finish(YogEnv*, YogVM*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
