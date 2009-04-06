#ifndef __YOG_YOG_H__
#define __YOG_YOG_H__

#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "oniguruma.h"

#define BOOL    int
#define FALSE   (0)
#define TRUE    (!FALSE)

typedef unsigned int pc_t;

typedef unsigned int ID;

#define INVALID_ID  (UINT_MAX)

enum YogValType {
    VAL_UNDEF, 
    VAL_INT, 
    VAL_FLOAT, 
    VAL_PTR, 
    VAL_OBJ, 
    VAL_BOOL, 
    VAL_NIL, 
    VAL_SYMBOL, 
#if 0
    VAL_STR, 
#endif
};

typedef enum YogValType YogValType;

struct YogVal {
    enum YogValType type;
    union {
        int n;
        double f;
        ID symbol;
        void * ptr;
        struct YogBasicObj* obj;
        BOOL b;
#if 0
        const char* str;
#endif
    } u;
};

#define VAL_TYPE(v)         ((v).type)
#define VAL2INT(v)          ((v).u.n)
#define VAL2FLOAT(v)        ((v).u.f)
#define VAL2ID(v)           ((v).u.symbol)
#define VAL2PTR(v)          ((v).u.ptr)
#define VAL2BOOL(v)         ((v).u.b)
#define VAL2OBJ(v)          ((v).u.obj)
#define VAL2STR(v)          ((v).u.str)
#define PTR_AS(type, v)     ((type*)VAL2PTR(v))
#define OBJ_AS(type, v)     ((type*)VAL2OBJ(v))

#define IS_UNDEF(v)     (VAL_TYPE(v) == VAL_UNDEF)
#define IS_PTR(v)       ((VAL_TYPE(v) == VAL_PTR) || IS_OBJ(v))
#define IS_OBJ(v)       (VAL_TYPE(v) == VAL_OBJ)
#define IS_INT(v)       (VAL_TYPE(v) == VAL_INT)
#define IS_FLOAT(v)     (VAL_TYPE(v) == VAL_FLOAT)
#define IS_BOOL(v)      (VAL_TYPE(v) == VAL_BOOL)
#define IS_NIL(v)       (VAL_TYPE(v) == VAL_NIL)
#define IS_SYMBOL(v)    (VAL_TYPE(v) == VAL_SYMBOL)
#define IS_STR(v)       (VAL_TYPE(v) == VAL_STR)

#define KLASS_OF(v)     (OBJ_AS(YogBasicObj, v)->klass)
#define IS_OBJ_OF(k, v) (IS_OBJ(v) && (VAL2OBJ(KLASS_OF(v)) == VAL2OBJ(ENV_VM(env)->k)))

#define CHECK_INT(v, msg)   do { \
    if (!IS_INT(v)) { \
        YogError_raise_type_error(env, msg); \
    } \
} while (0)

typedef struct YogVal YogVal;

struct YogEnv {
    struct YogVm* vm;
    struct YogThread* th;
};

#define ENV_VM(env)     ((env)->vm)
#define ENV_TH(env)     ((env)->th)

typedef struct YogEnv YogEnv;

typedef void* (*ObjectKeeper)(YogEnv*, void*);
typedef void (*ChildrenKeeper)(YogEnv*, void*, ObjectKeeper);
typedef void (*Finalizer)(YogEnv*, void*);

#define SURVIVE_INDEX_MAX    8

#if GC_COPYING
#   include "yog/gc/copying.h"
#elif GC_MARK_SWEEP
#   include "yog/gc/mark-sweep.h"
#elif GC_MARK_SWEEP_COMPACT
#   include "yog/gc/mark-sweep-compact.h"
#elif GC_GENERATIONAL
#   include "yog/gc/generational.h"
#endif

struct YogVm {
    BOOL gc_stress;

    void* (*alloc_mem)(struct YogEnv*, struct YogVm*, ChildrenKeeper, Finalizer, size_t);
    void (*free_mem)(struct YogEnv*, struct YogVm*);
    union {
#if GC_COPYING
        YogCopying copying;
#elif GC_MARK_SWEEP
        YogMarkSweep mark_sweep;
#elif GC_MARK_SWEEP_COMPACT
        YogMarkSweepCompact mark_sweep_compact;
#elif GC_GENERATIONAL
        YogGenerational generational;
#endif
    } gc;
    struct {
        BOOL print;
        unsigned int duration_total;
        unsigned int living_obj_num[SURVIVE_INDEX_MAX];
        unsigned int total_obj_num;
        unsigned int num_alloc;
        size_t total_allocated_size;
        unsigned int exec_num;
    } gc_stat;

    ID next_id;
    struct YogVal id2name;
    struct YogVal name2id;

    struct YogVal cObject;
    struct YogVal cKlass;
    struct YogVal cInt;
    struct YogVal cString;
    struct YogVal cRegexp;
    struct YogVal cMatch;
    struct YogVal cPackage;
    struct YogVal cBool;
    struct YogVal cBuiltinBoundMethod;
    struct YogVal cBoundMethod;
    struct YogVal cBuiltinUnboundMethod;
    struct YogVal cUnboundMethod;
    struct YogVal cPackageBlock;
    struct YogVal cNil;

    struct YogVal eException;
    struct YogVal eBugException;
    struct YogVal eTypeError;
    struct YogVal eIndexError;

    struct YogVal pkgs;

    struct YogVal encodings;

    struct YogThread* thread;
};

typedef struct YogVm YogVm;

struct YogBasicObj {
    unsigned int flags;
    struct YogVal klass;
};

#define HAS_ATTRS   (1)

#define YOGBASICOBJ_HEAD    struct YogBasicObj base
#define YOGBASICOBJ(obj)    ((struct YogBasicObj*)obj)

typedef struct YogBasicObj YogBasicObj;

struct YogObj {
    YOGBASICOBJ_HEAD;
    struct YogVal attrs;
};

#define YOGOBJ_HEAD struct YogObj base
#define YOGOBJ(obj) ((struct YogObj*)obj)

typedef struct YogObj YogObj;

typedef YogVal (*Allocator)(struct YogEnv*, struct YogVal);

enum YogFrameType {
    FRAME_C, 
    FRAME_METHOD, 
    FRAME_NAME, 
};

typedef enum YogFrameType YogFrameType;

struct YogFrame {
    struct YogVal prev;
    enum YogFrameType type;
};

typedef struct YogFrame YogFrame;

struct YogCFrame {
    struct YogFrame base;
    struct YogVal self;
    struct YogVal args;
    struct YogVal f;
#if 0
    struct YogVal locals;
    unsigned int locals_size;
#endif
};

typedef struct YogCFrame YogCFrame;

#include "yog/array.h"

#define C_FRAME(frame)      ((YogCFrame*)(frame))
#define CUR_C_FRAME(env)    PTR_AS(YogCFrame, (env)->th->cur_frame)
#define SELF(env)           (CUR_C_FRAME(env)->self)
#define ARG(env, i)         (PTR_AS(YogValArray, CUR_C_FRAME(env)->args)->items[i])

struct YogOuterVars {
    unsigned int size;
    struct YogVal items[0];
};

typedef struct YogOuterVars YogOuterVars;

struct YogScriptFrame {
    struct YogFrame base;
    pc_t pc;
    struct YogVal code;
    unsigned int stack_size;
    struct YogVal stack;
    struct YogVal globals;
    struct YogVal outer_vars;
};

typedef struct YogScriptFrame YogScriptFrame;

#define SCRIPT_FRAME(v)     PTR_AS(YogScriptFrame, (v))

struct YogNameFrame {
    struct YogScriptFrame base;
    struct YogVal self;
    struct YogVal vars;
};

typedef struct YogNameFrame YogNameFrame;

#define NAME_FRAME(v)   PTR_AS(YogNameFrame, (v))
#define NAME_VARS(v)    (NAME_FRAME(v)->vars)

#define YogKlassFrame       YogNameFrame
#define YogPackageFrame     YogNameFrame

struct YogMethodFrame {
    struct YogScriptFrame base;
    struct YogVal vars;
};

typedef struct YogMethodFrame YogMethodFrame;

#define METHOD_FRAME(v)     PTR_AS(YogMethodFrame, (v))
#define LOCAL_VARS(f)       (METHOD_FRAME(f)->vars)

struct YogJmpBuf {
    jmp_buf buf;
    struct YogJmpBuf* prev;
};

typedef struct YogJmpBuf YogJmpBuf;

#define NUM_VALS    5

struct YogLocals {
    struct YogLocals* next;
    unsigned int num_vals;
    unsigned int size;
    struct YogVal* vals[NUM_VALS];
};

typedef struct YogLocals YogLocals;

#define SAVE_LOCALS(env)        YogLocals* __cur_locals__ = ENV_TH(env)->locals
#define RESTORE_LOCALS(env)     ENV_TH(env)->locals = __cur_locals__
#if 0
#   define PUSH_LOCAL_TABLE(env, tbl) \
do { \
    unsigned int i; \
    for (i = 0; i < tbl.num_vals; i++) { \
        DPRINTF("tbl.vals[%d]=%p", i, tbl.vals[i]); \
    } \
    tbl.next = ENV_TH(env)->locals; \
    ENV_TH(env)->locals = &tbl; \
} while (0)
#else
#   define PUSH_LOCAL_TABLE(env, tbl) \
do { \
    tbl.next = ENV_TH(env)->locals; \
    ENV_TH(env)->locals = &tbl; \
} while (0)
#endif
#define PUSH_LOCAL(env, x) \
    YogLocals __locals_##x##__; \
    __locals_##x##__.num_vals = 1; \
    __locals_##x##__.size = 1; \
    __locals_##x##__.vals[0] = &(x); \
    __locals_##x##__.vals[1] = NULL; \
    __locals_##x##__.vals[2] = NULL; \
    __locals_##x##__.vals[3] = NULL; \
    __locals_##x##__.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##__);
#define PUSH_LOCALS2(env, x, y) \
    YogLocals __locals_##x##_##y##__; \
    __locals_##x##_##y##__.num_vals = 2; \
    __locals_##x##_##y##__.size = 1; \
    __locals_##x##_##y##__.vals[0] = &(x); \
    __locals_##x##_##y##__.vals[1] = &(y); \
    __locals_##x##_##y##__.vals[2] = NULL; \
    __locals_##x##_##y##__.vals[3] = NULL; \
    __locals_##x##_##y##__.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##__);
#define PUSH_LOCALS3(env, x, y, z) \
    YogLocals __locals_##x##_##y##_##z##__; \
    __locals_##x##_##y##_##z##__.num_vals = 3; \
    __locals_##x##_##y##_##z##__.size = 1; \
    __locals_##x##_##y##_##z##__.vals[0] = &(x); \
    __locals_##x##_##y##_##z##__.vals[1] = &(y); \
    __locals_##x##_##y##_##z##__.vals[2] = &(z); \
    __locals_##x##_##y##_##z##__.vals[3] = NULL; \
    __locals_##x##_##y##_##z##__.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##_##z##__);
#define PUSH_LOCALS4(env, x, y, z, t) \
    YogLocals __locals_##x##_##y##_##z##_##t##__; \
    __locals_##x##_##y##_##z##_##t##__.num_vals = 4; \
    __locals_##x##_##y##_##z##_##t##__.size = 1; \
    __locals_##x##_##y##_##z##_##t##__.vals[0] = &(x); \
    __locals_##x##_##y##_##z##_##t##__.vals[1] = &(y); \
    __locals_##x##_##y##_##z##_##t##__.vals[2] = &(z); \
    __locals_##x##_##y##_##z##_##t##__.vals[3] = &(t); \
    __locals_##x##_##y##_##z##_##t##__.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##_##z##_##t##__);
#define PUSH_LOCALS5(env, x, y, z, t, u) \
    YogLocals __locals_##x##_##y##_##z##_##t##_##u##__; \
    __locals_##x##_##y##_##z##_##t##_##u##__.num_vals = 5; \
    __locals_##x##_##y##_##z##_##t##_##u##__.size = 1; \
    __locals_##x##_##y##_##z##_##t##_##u##__.vals[0] = &(x); \
    __locals_##x##_##y##_##z##_##t##_##u##__.vals[1] = &(y); \
    __locals_##x##_##y##_##z##_##t##_##u##__.vals[2] = &(z); \
    __locals_##x##_##y##_##z##_##t##_##u##__.vals[3] = &(t); \
    __locals_##x##_##y##_##z##_##t##_##u##__.vals[4] = &(u); \
    PUSH_LOCAL_TABLE(env, __locals_##x##_##y##_##z##_##t##_##u##__);
#define PUSH_LOCALSX(env, num, x) \
    YogLocals __locals_##x##__; \
    __locals_##x##__.num_vals = 1; \
    __locals_##x##__.size = (num); \
    __locals_##x##__.vals[0] = (x); \
    __locals_##x##__.vals[1] = NULL; \
    __locals_##x##__.vals[2] = NULL; \
    __locals_##x##__.vals[3] = NULL; \
    __locals_##x##__.vals[4] = NULL; \
    PUSH_LOCAL_TABLE(env, __locals_##x##__);
#define SAVE_ARG(env, x)        SAVE_LOCALS((env)); \
                                PUSH_LOCAL((env), x)
#define SAVE_ARGS2(env, x, y)   SAVE_LOCALS((env)); \
                                PUSH_LOCALS2((env), x, y)
#define SAVE_ARGS3(env, x, y, z)  \
                                SAVE_LOCALS((env)); \
                                PUSH_LOCALS3((env), x, y, z)
#define SAVE_ARGS4(env, x, y, z, t)  \
                                SAVE_LOCALS((env)); \
                                PUSH_LOCALS4((env), x, y, z, t)
#define SAVE_ARGS5(env, x, y, z, t, u)  \
                                SAVE_LOCALS((env)); \
                                PUSH_LOCALS5((env), x, y, z, t, u)
#define POP_LOCALS(env)         ENV_TH(env)->locals = ENV_TH(env)->locals->next
#define RETURN(env, val)        do { \
    RESTORE_LOCALS(env); \
    return val; \
} while (0)
#define RETURN_VOID(env)        do { \
    RESTORE_LOCALS(env); \
    return; \
} while (0)

struct YogThread {
    struct YogVal cur_frame;
    struct YogJmpBuf* jmp_buf_list;
    struct YogVal jmp_val;
    struct YogLocals* locals;
#if defined(GC_GENERATIONAL)
    void*** ref_tbl;
    void*** ref_tbl_ptr;
    void*** ref_tbl_limit;
#endif
};

typedef struct YogThread YogThread;

#include "yog/error.h"

#if defined(GC_GENERATIONAL)
#   define IS_YOUNG_PTR(env, ptr)  (((env)->vm->gc.generational.copying.active_heap->items <= (unsigned char*)(ptr)) && ((unsigned char*)(ptr) <= (env)->vm->gc.generational.copying.active_heap->items + (env)->vm->gc.generational.copying.active_heap->size))
#   define IS_YOUNG(env, val)   (IS_PTR((val)) && IS_YOUNG_PTR((env), VAL2PTR(val)))
#   define REF_TBL_GROW_SIZE    256
#   define ADD_REF(env, val)    do { \
    YogThread* thread = (env)->th; \
    if (thread->ref_tbl_ptr == thread->ref_tbl_limit) { \
        unsigned int size = thread->ref_tbl_limit - thread->ref_tbl; \
        unsigned int new_size = size + REF_TBL_GROW_SIZE * sizeof(void**); \
        thread->ref_tbl = realloc(thread->ref_tbl, new_size); \
        YOG_ASSERT((env), thread->ref_tbl != NULL, "Can't grow ref_tbl"); \
        thread->ref_tbl_ptr = thread->ref_tbl + size / sizeof(void**); \
        thread->ref_tbl_limit = thread->ref_tbl + new_size / sizeof(void**); \
    } \
    *thread->ref_tbl_ptr = &(val); \
    thread->ref_tbl_ptr++; \
} while (0)
#   define MODIFY(env, fp, val)    do { \
    if (!IS_YOUNG_PTR((env), &(fp))) { \
        YogVal old = (fp); \
        if (!IS_YOUNG((env), old) && IS_YOUNG((env), (val))) { \
            ADD_REF((env), VAL2PTR((fp))); \
        } \
    } \
    (fp) = (val); \
} while (0)
#else
#   define ADD_REF(env, ptr)
#   define MODIFY(env, fp, val)     (fp) = (val)
#endif

#include "yog/klass.h"
#include "yog/package.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/frame.c */
YogVal YogCFrame_new(YogEnv*);
void YogFrame_add_locals(YogEnv*, YogCFrame*, unsigned int, ...);
YogMethodFrame* YogMethodFrame_new(YogEnv*);
YogVal YogNameFrame_new(YogEnv*);
YogVal YogOuterVars_new(YogEnv*, unsigned int);
YogVal YogScriptFrame_pop_stack(YogEnv*, YogScriptFrame*);
void YogScriptFrame_push_stack(YogEnv*, YogScriptFrame*, YogVal);

/* src/object.c */
void YogBasicObj_init(YogEnv*, YogBasicObj*, unsigned int, YogVal);
void YogBasicObj_keep_children(YogEnv*, void*, ObjectKeeper);
YogVal YogObj_allocate(YogEnv*, YogVal);
YogVal YogObj_get_attr(YogEnv*, YogObj*, ID);
void YogObj_init(YogEnv*, YogObj*, unsigned int, YogVal);
void YogObj_keep_children(YogEnv*, void*, ObjectKeeper);
void YogObj_klass_init(YogEnv*, YogVal);
YogVal YogObj_new(YogEnv*, YogVal);
void YogObj_set_attr(YogEnv*, YogVal, const char*, YogVal);
void YogObj_set_attr_id(YogEnv*, YogVal, ID, YogVal);

/* src/thread.c */
YogVal YogThread_call_block(YogEnv*, YogThread*, YogVal, unsigned int, YogVal*);
YogVal YogThread_call_method(YogEnv*, YogThread*, YogVal, const char*, unsigned int, YogVal*);
YogVal YogThread_call_method_id(YogEnv*, YogThread*, YogVal, ID, unsigned int, YogVal*);
void YogThread_eval_package(YogEnv*, YogThread*, YogVal);
void YogThread_finalize(YogEnv*, YogThread*);
void YogThread_initialize(YogEnv*, YogThread*);
void YogThread_keep_children(YogEnv*, void*, ObjectKeeper);
YogThread* YogThread_new(YogEnv*);
void YogThread_shrink_ref_tbl(YogEnv*, YogThread*);

/* src/value.c */
BOOL YogVal_equals_exact(YogEnv*, YogVal, YogVal);
YogVal YogVal_false();
YogVal YogVal_float(float);
YogVal YogVal_get_attr(YogEnv*, YogVal, ID);
YogVal YogVal_get_klass(YogEnv*, YogVal);
int YogVal_hash(YogEnv*, YogVal);
YogVal YogVal_int(int);
BOOL YogVal_is_subklass_of(YogEnv*, YogVal, YogVal);
YogVal YogVal_keep(YogEnv*, YogVal, ObjectKeeper);
YogVal YogVal_nil();
YogVal YogVal_obj(YogBasicObj*);
void YogVal_print(YogEnv*, YogVal);
YogVal YogVal_ptr(void *);
YogVal YogVal_str(const char*);
YogVal YogVal_symbol(ID);
YogVal YogVal_true();
YogVal YogVal_undef();

/* src/vm.c */
void* YogVm_alloc(YogEnv*, YogVm*, ChildrenKeeper, Finalizer, size_t);
void YogVm_boot(YogEnv*, YogVm*);
void YogVm_config_copying(YogEnv*, YogVm*, unsigned int);
void YogVm_config_generational(YogEnv*, YogVm*, size_t, size_t, size_t, unsigned int);
void YogVm_config_mark_sweep(YogEnv*, YogVm*, size_t);
void YogVm_config_mark_sweep_compact(YogEnv*, YogVm*, size_t, size_t);
void YogVm_delete(YogEnv*, YogVm*);
void YogVm_gc(YogEnv*, YogVm*);
const char* YogVm_id2name(YogEnv*, YogVm*, ID);
void YogVm_init(YogVm*);
ID YogVm_intern(YogEnv*, YogVm*, const char*);
void YogVm_register_package(YogEnv*, YogVm*, const char*, YogVal);

/* PROTOTYPE_END */

#define YogKlassFrame_new       YogNameFrame_new
#define YogPackageFrame_new     YogNameFrame_new

#define ALLOC_OBJ_SIZE(env, keep_children, finalizer, size) \
    YogVm_alloc(env, ENV_VM(env), keep_children, finalizer, size)
#define ALLOC_OBJ(env, keep_children, finalizer, type) \
    ALLOC_OBJ_SIZE(env, keep_children, finalizer, sizeof(type))
#define ALLOC_OBJ_ITEM(env, keep_children, finalizer, type, size, item_type) \
    ALLOC_OBJ_SIZE(env, keep_children, finalizer, sizeof(type) + size * sizeof(item_type))
#define REALLOC_OBJ(env, ptr, size) \
    YogVm_realloc(env, ENV_VM(env), ptr, size)

#define JMP_RAISE   (1)

#define INTERN(s)   YogVm_intern(env, ENV_VM(env), s)
#define BUILTINS    "builtins"

#define YTRUE           YogVal_true()
#define YFALSE          YogVal_false()
#define YNIL            YogVal_nil()
#define YUNDEF          YogVal_undef()
#define INT2VAL(n)      YogVal_int(n)
#define FLOAT2VAL(f)    YogVal_float(f)
#define OBJ2VAL(obj)    YogVal_obj((YogBasicObj*)obj)
#define PTR2VAL(ptr)    YogVal_ptr(ptr)
#define ID2VAL(id)      YogVal_symbol(id)
#define STR2VAL(str)    YogVal_str(str)

#define DPRINTF(...)    do { \
    printf("%s:%d ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
    fflush(stdout); \
} while (0)

#define array_sizeof(a)     (sizeof(a) / sizeof(a[0]))

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
