#ifndef __YOG_YOG_H__
#define __YOG_YOG_H__

#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include "oniguruma.h"

#define BOOL    int
#define FALSE   (0)
#define TRUE    (!FALSE)

typedef unsigned int pc_t;

typedef unsigned int ID;

#define INVALID_ID  (UINT_MAX)

struct YogEnv {
    struct YogVm* vm;
    struct YogThread* th;
};

#define ENV_VM(env)     ((env)->vm)
#define ENV_TH(env)     ((env)->th)

typedef struct YogEnv YogEnv;

enum YogGcType {
    GC_BDW, 
    GC_COPYING, 
    GC_MARK_SWEEP, 
};

typedef enum YogGcType YogGcType;

typedef void* (*ObjectKeeper)(YogEnv*, void*);
typedef void (*ChildrenKeeper)(YogEnv*, void*, ObjectKeeper);
typedef void (*Finalizer)(YogEnv*, void*);

struct YogVm {
    BOOL gc_stress;
    BOOL disable_gc;

    void (*init_gc)(struct YogEnv*, struct YogVm*);
    void* (*alloc_mem)(struct YogEnv*, struct YogVm*, ChildrenKeeper, Finalizer, size_t);
    void* (*realloc_mem)(struct YogEnv*, struct YogVm*, void*, size_t);
    void (*free_mem)(struct YogEnv*, struct YogVm*);
    union {
        struct {
            unsigned int init_heap_size;
            struct YogHeap* heap;
            unsigned char* scanned;
            unsigned char* unscanned;
        } copying; 
        struct {
            struct YogMarkSweepHeader* header;
            size_t threshold;
            size_t allocated_size;
        } mark_sweep;
    } gc;

    ID next_id;
    struct YogTable* id2name;
    struct YogTable* name2id;

    struct YogKlass* cObject;
    struct YogKlass* cKlass;
    struct YogKlass* cInt;
    struct YogKlass* cString;
    struct YogKlass* cRegexp;
    struct YogKlass* cMatch;
    struct YogKlass* cPackage;
    struct YogKlass* cBool;
    struct YogKlass* cBuiltinBoundMethod;
    struct YogKlass* cBoundMethod;
    struct YogKlass* cBuiltinUnboundMethod;
    struct YogKlass* cUnboundMethod;
    struct YogKlass* cPackageBlock;
    struct YogKlass* cNil;

    struct YogKlass* eException;
    struct YogKlass* eBugException;
    struct YogKlass* eTypeError;
    struct YogKlass* eIndexError;

    struct YogTable* pkgs;

    struct YogTable* encodings;

    struct YogThread* thread;
};

typedef struct YogVm YogVm;

enum YogValType {
    VAL_UNDEF, 
    VAL_INT, 
    VAL_FLOAT, 
    VAL_PTR, 
    VAL_OBJ, 
    VAL_BOOL, 
    VAL_NIL, 
    VAL_SYMBOL, 
    VAL_STR, 
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
        const char* str;
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
#define OBJ_AS(type, v)     ((type*)VAL2OBJ(v))

#define IS_UNDEF(v)     (VAL_TYPE(v) == VAL_UNDEF)
#define IS_PTR(v)       (VAL_TYPE(v) == VAL_PTR)
#define IS_OBJ(v)       (VAL_TYPE(v) == VAL_OBJ)
#define IS_INT(v)       (VAL_TYPE(v) == VAL_INT)
#define IS_FLOAT(v)     (VAL_TYPE(v) == VAL_FLOAT)
#define IS_BOOL(v)      (VAL_TYPE(v) == VAL_BOOL)
#define IS_NIL(v)       (VAL_TYPE(v) == VAL_NIL)
#define IS_SYMBOL(v)    (VAL_TYPE(v) == VAL_SYMBOL)
#define IS_STR(v)       (VAL_TYPE(v) == VAL_STR)

#define KLASS_OF(v)     (OBJ_AS(YogBasicObj, v)->klass)
#define IS_OBJ_OF(klass, v) \
                        (IS_OBJ(v) && (KLASS_OF(v) == ENV_VM(env)->klass))

#define CHECK_INT(v, msg)   do { \
    if (!IS_INT(v)) { \
        YogError_raise_type_error(env, msg); \
    } \
} while (0)

typedef struct YogVal YogVal;

struct YogBasicObj {
    unsigned int flags;
    struct YogKlass* klass;
};

#define HAS_ATTRS   (1)

#define YOGBASICOBJ_HEAD    struct YogBasicObj base
#define YOGBASICOBJ(obj)    ((struct YogBasicObj*)obj)

typedef struct YogBasicObj YogBasicObj;

struct YogObj {
    YOGBASICOBJ_HEAD;
    struct YogTable* attrs;
};

#define YOGOBJ_HEAD struct YogObj base
#define YOGOBJ(obj) ((struct YogObj*)obj)

typedef struct YogObj YogObj;

typedef YogBasicObj* (*Allocator)(struct YogEnv*, struct YogKlass*);

enum YogFrameType {
    FRAME_C, 
    FRAME_SCRIPT, 
};

typedef enum YogFrameType YogFrameType;

struct YogFrame {
    struct YogFrame* prev;
    enum YogFrameType type;
    struct YogValArray* locals;
    unsigned int locals_size;
};

#define FRAME(f)    ((YogFrame*)(f))

#include "yog/array.h"

#define MAX_LOCALS  (8)

#define CUR_FRAME(env)  (env)->th->cur_frame
#define FRAME_DECL_LOCAL(env, index, val) \
    unsigned int index = CUR_FRAME(env)->locals_size; \
    YogFrame_add_locals(env, CUR_FRAME(env), 1, val)
#define FRAME_DECL_LOCALS2(env, index0, val0, index1, val1) \
    unsigned int index0 = CUR_FRAME(env)->locals_size; \
    unsigned int index1 = CUR_FRAME(env)->locals_size + 1; \
    YogFrame_add_locals(env, CUR_FRAME(env), 2, val0, val1)
#define FRAME_DECL_LOCALS3(env, index0, val0, index1, val1, index2, val2) \
    unsigned int index0 = CUR_FRAME(env)->locals_size; \
    unsigned int index1 = CUR_FRAME(env)->locals_size + 1; \
    unsigned int index2 = CUR_FRAME(env)->locals_size + 2; \
    YogFrame_add_locals(env, CUR_FRAME(env), 3, val0, val1, val2)
#define FRAME_DECL_LOCALS4(env, index0, val0, index1, val1, index2, val2, index3, val3) \
    unsigned int index0 = CUR_FRAME(env)->locals_size; \
    unsigned int index1 = CUR_FRAME(env)->locals_size + 1; \
    unsigned int index2 = CUR_FRAME(env)->locals_size + 2; \
    unsigned int index3 = CUR_FRAME(env)->locals_size + 3; \
    YogFrame_add_locals(env, CUR_FRAME(env), 4, val0, val1, val2, val3)

#define __FRAME_LOCAL__(env, i) \
    YogValArray_at(env, CUR_FRAME(env)->locals, i)
#define FRAME_LOCAL(env, val, i)    val = __FRAME_LOCAL__(env, i)
#define FRAME_LOCAL_PTR(env, ptr, i) \
                                    ptr = VAL2PTR(__FRAME_LOCAL__(env, i))
#define FRAME_LOCAL_OBJ(env, obj, type, i) \
                                    obj = OBJ_AS(type, __FRAME_LOCAL__(env, i))
#define FRAME_LOCAL_ARRAY(env, obj, i) \
                                    FRAME_LOCAL_OBJ(env, obj, YogArray, i)

typedef struct YogFrame YogFrame;

struct YogCFrame {
    struct YogFrame base;
    struct YogVal self;
    struct YogValArray* args;
    struct YogBuiltinFunction* f;
};

typedef struct YogCFrame YogCFrame;

#define C_FRAME(frame)      ((YogCFrame*)(frame))
#define CUR_C_FRAME(env)    (C_FRAME((env)->th->cur_frame))
#define SELF(env)           (CUR_C_FRAME(env)->self)
#define ARG(env, i)         (CUR_C_FRAME(env)->args->items[i])

struct YogScriptFrame {
    struct YogFrame base;
    pc_t pc;
    struct YogCode* code;
    unsigned int stack_size;
    struct YogValArray* stack;
};

#define SCRIPT_FRAME(f)     ((YogScriptFrame*)f)

typedef struct YogScriptFrame YogScriptFrame;

struct YogNameFrame {
    struct YogScriptFrame base;
    YogVal self;
    struct YogTable* vars;
};

#define NAME_FRAME(f)   ((YogNameFrame*)f)
#define NAME_VARS(f)    (NAME_FRAME(f)->vars)

typedef struct YogNameFrame YogNameFrame;

#define YogKlassFrame       YogNameFrame
#define YogPackageFrame     YogNameFrame

struct YogMethodFrame {
    struct YogScriptFrame base;
    struct YogValArray* vars;
};

#define LOCAL_VARS(f)   (((YogMethodFrame*)f)->vars)

typedef struct YogMethodFrame YogMethodFrame;

struct YogJmpBuf {
    jmp_buf buf;
    struct YogJmpBuf* prev;
};

typedef struct YogJmpBuf YogJmpBuf;

struct YogThread {
    struct YogFrame* cur_frame;
    struct YogJmpBuf* jmp_buf_list;
    struct YogVal jmp_val;
    struct YogParser* parser;
};

typedef struct YogThread YogThread;

#include "yog/klass.h"
#include "yog/package.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/frame.c */
YogCFrame* YogCFrame_new(YogEnv*);
unsigned int YogFrame_add_local(YogEnv*, YogFrame*, YogVal);
void YogFrame_add_locals(YogEnv*, YogFrame*, unsigned int, ...);
YogMethodFrame* YogMethodFrame_new(YogEnv*);
YogNameFrame* YogNameFrame_new(YogEnv*);
YogVal YogScriptFrame_pop_stack(YogEnv*, YogScriptFrame*);
void YogScriptFrame_push_stack(YogEnv*, YogScriptFrame*, YogVal);

/* src/object.c */
void YogBasicObj_init(YogEnv*, YogBasicObj*, unsigned int, YogKlass*);
void YogBasicObj_keep_children(YogEnv*, void*, ObjectKeeper);
YogBasicObj* YogObj_allocate(YogEnv*, YogKlass*);
YogVal YogObj_get_attr(YogEnv*, YogObj*, ID);
void YogObj_init(YogEnv*, YogObj*, unsigned int, YogKlass*);
void YogObj_keep_children(YogEnv*, void*, ObjectKeeper);
void YogObj_klass_init(YogEnv*, YogKlass*);
YogObj* YogObj_new(YogEnv*, YogKlass*);
void YogObj_set_attr(YogEnv*, YogObj*, const char*, YogVal);
void YogObj_set_attr_id(YogEnv*, YogObj*, ID, YogVal);

/* src/thread.c */
YogVal YogThread_call_block(YogEnv*, YogThread*, YogVal, unsigned int, YogVal*);
YogVal YogThread_call_method(YogEnv*, YogThread*, YogVal, const char*, unsigned int, YogVal*);
YogVal YogThread_call_method_id(YogEnv*, YogThread*, YogVal, ID, unsigned int, YogVal*);
void YogThread_eval_package(YogEnv*, YogThread*, YogPackage*);
void YogThread_init(YogEnv*, YogThread*);
void YogThread_keep_children(YogEnv*, void*, ObjectKeeper);
YogThread* YogThread_new(YogEnv*);

/* src/value.c */
BOOL YogVal_equals_exact(YogEnv*, YogVal, YogVal);
YogVal YogVal_false();
YogVal YogVal_float(float);
YogVal YogVal_get_attr(YogEnv*, YogVal, ID);
YogKlass* YogVal_get_klass(YogEnv*, YogVal);
int YogVal_hash(YogEnv*, YogVal);
YogVal YogVal_int(int);
BOOL YogVal_is_subklass_of(YogEnv*, YogVal, YogKlass*);
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
void YogVm_config_mark_sweep(YogEnv*, YogVm*, size_t);
void YogVm_delete(YogEnv*, YogVm*);
const char* YogVm_id2name(YogEnv*, YogVm*, ID);
void YogVm_init(YogVm*, YogGcType);
void YogVm_initialize_memory(YogEnv*, YogVm*);
ID YogVm_intern(YogEnv*, YogVm*, const char*);
void* YogVm_realloc(YogEnv*, YogVm*, void*, size_t);
void YogVm_register_package(YogEnv*, YogVm*, const char*, YogPackage*);

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

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
