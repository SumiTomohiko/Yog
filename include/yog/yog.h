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

struct YogVm {
    BOOL always_gc;
    BOOL disable_gc;

    void (*init_gc)(struct YogEnv*, struct YogVm*);
    void (*exec_gc)(struct YogEnv*, struct YogVm*);
    void* (*alloc_mem)(struct YogEnv*, struct YogVm*, ChildrenKeeper, size_t);
    void (*free_mem)(struct YogEnv*, struct YogVm*);
    BOOL need_gc;
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
#define YOGBASICOBJ(obj)    ((YogBasicObj*)obj)

typedef struct YogBasicObj YogBasicObj;

struct YogObj {
    YOGBASICOBJ_HEAD;
    struct YogTable* attrs;
};

#define YOGOBJ_HEAD struct YogObj base
#define YOGOBJ(obj) ((YogObj*)obj)

typedef struct YogObj YogObj;

typedef YogBasicObj* (*Allocator)(struct YogEnv*, struct YogKlass*);

struct YogKlass {
    YOGOBJ_HEAD;
    Allocator allocator;
    ID name;
    struct YogKlass* super;
};

typedef struct YogKlass YogKlass;

struct YogArgInfo {
    unsigned int argc;
    ID* argnames;
    uint8_t* arg_index;

    unsigned int blockargc;
    ID blockargname;
    uint8_t blockarg_index;

    unsigned int varargc;
    uint8_t vararg_index;

    unsigned int kwargc;
    uint8_t kwarg_index;
};

typedef struct YogArgInfo YogArgInfo;

struct YogBuiltinFunction {
    struct YogArgInfo arg_info;
    int required_argc;

    YogVal (*f)();

    ID klass_name;
    ID func_name;
};

typedef struct YogBuiltinFunction YogBuiltinFunction;

struct YogBuiltinBoundMethod {
    YOGBASICOBJ_HEAD;
    YogVal self;
    struct YogBuiltinFunction* f;
};

typedef struct YogBuiltinBoundMethod YogBuiltinBoundMethod;

struct YogBoundMethod {
    YOGBASICOBJ_HEAD;
    YogVal self;
    struct YogCode* code;
};

typedef struct YogBoundMethod YogBoundMethod;

struct YogBuiltinUnboundMethod {
    YOGBASICOBJ_HEAD;
    struct YogBuiltinFunction* f;
};

typedef struct YogBuiltinUnboundMethod YogBuiltinUnboundMethod;

struct YogUnboundMethod {
    YOGBASICOBJ_HEAD;
    struct YogCode* code;
};

typedef struct YogUnboundMethod YogUnboundMethod;

enum YogFrameType {
    FRAME_C, 
    FRAME_SCRIPT, 
};

typedef enum YogFrameType YogFrameType;

struct YogFrame {
    struct YogFrame* prev;
    enum YogFrameType type;
};

#define FRAME(f)    ((YogFrame*)(f))

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
#define ARG(env, i)         (CUR_C_FRAME(env)->args->items[(i)])

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
};

typedef struct YogThread YogThread;

struct YogBlock {
    YOGBASICOBJ_HEAD;
    struct YogCode* code;
};

#define BLOCK(obj)  ((YogBlock*)obj)

typedef struct YogBlock YogBlock;

struct YogPackageBlock {
    struct YogBlock base;
    YogVal self;
    struct YogTable* vars;
};

#define PACKAGE_BLOCK(obj)  ((YogPackageBlock*)obj)

typedef struct YogPackageBlock YogPackageBlock;

#include "yog/package.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/arg.c */
void YogArgInfo_keep_children(YogEnv*, void*, ObjectKeeper);

/* src/block.c */
YogKlass* YogPackageBlock_klass_new(YogEnv*);
YogPackageBlock* YogPackageBlock_new(YogEnv*);

/* src/bool.c */
YogKlass* YogBool_klass_new(YogEnv*);

/* src/builtins.c */
YogPackage* YogBuiltins_new(YogEnv*);

/* src/error.c */
void YogError_raise(YogEnv*, YogVal);
void YogError_raise_index_error(YogEnv*, const char*);
void YogError_raise_type_error(YogEnv*, const char*);

/* src/frame.c */
YogCFrame* YogCFrame_new(YogEnv*);
YogMethodFrame* YogMethodFrame_new(YogEnv*);
YogNameFrame* YogNameFrame_new(YogEnv*);
YogVal YogScriptFrame_pop_stack(YogEnv*, YogScriptFrame*);
void YogScriptFrame_push_stack(YogEnv*, YogScriptFrame*, YogVal);

/* src/function.c */
YogBuiltinFunction* YogBuiltinFunction_new(YogEnv*, void*, ID, ID, unsigned int, unsigned int, unsigned int, int, va_list);

/* src/int.c */
YogKlass* YogInt_klass_new(YogEnv*);

/* src/klass.c */
YogBasicObj* YogKlass_allocate(YogEnv*, YogKlass*);
void YogKlass_define_allocator(YogEnv*, YogKlass*, Allocator);
void YogKlass_define_method(YogEnv*, YogKlass*, const char*, void*, unsigned int, unsigned int, unsigned int, int, ...);
void YogKlass_klass_init(YogEnv*, YogKlass*);
YogKlass* YogKlass_new(YogEnv*, const char*, YogKlass*);

/* src/method.c */
YogKlass* YogBoundMethod_klass_new(YogEnv*);
YogBoundMethod* YogBoundMethod_new(YogEnv*);
YogKlass* YogBuiltinBoundMethod_klass_new(YogEnv*);
YogBuiltinBoundMethod* YogBuiltinBoundMethod_new(YogEnv*);
YogKlass* YogBuiltinUnboundMethod_klass_new(YogEnv*);
YogBuiltinUnboundMethod* YogBuiltinUnboundMethod_new(YogEnv*);
YogKlass* YogUnboundMethod_klass_new(YogEnv*);
YogUnboundMethod* YogUnboundMethod_new(YogEnv*);

/* src/nil.c */
YogKlass* YogNil_klass_new(YogEnv*);

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
void* YogVm_alloc(YogEnv*, YogVm*, ChildrenKeeper, size_t);
void YogVm_boot(YogEnv*, YogVm*);
void YogVm_config_copying(YogEnv*, YogVm*, unsigned int);
void YogVm_config_mark_sweep(YogEnv*, YogVm*, size_t);
void YogVm_delete(YogEnv*, YogVm*);
void YogVm_gc(YogEnv*, YogVm*);
const char* YogVm_id2name(YogEnv*, YogVm*, ID);
void YogVm_init(YogVm*, YogGcType);
ID YogVm_intern(YogEnv*, YogVm*, const char*);
void YogVm_register_package(YogEnv*, YogVm*, const char*, YogPackage*);
void* alloc_mem_bdw(YogEnv*, YogVm*, ChildrenKeeper, size_t);

/* PROTOTYPE_END */

#define YogKlassFrame_new       YogNameFrame_new
#define YogPackageFrame_new     YogNameFrame_new

#define ALLOC_OBJ_SIZE(env, keep_children, size) \
    YogVm_alloc(env, ENV_VM(env), keep_children, size)
#define ALLOC_OBJ(env, keep_children, type) \
    ALLOC_OBJ_SIZE(env, keep_children, sizeof(type))
#define ALLOC_OBJ_ITEM(env, keep_children, type, size, item_type) \
    ALLOC_OBJ_SIZE(env, keep_children, sizeof(type) + size * sizeof(item_type))

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

#include "yog/exception.h"
#include "yog/string.h"

#define YOG_ASSERT(env, test, ...)  do { \
    if (!(test)) { \
        YogString* msg = YogString_new_format(env, __VA_ARGS__); \
        YogException* exc = YogBugException_new(env); \
        exc->message = OBJ2VAL(msg); \
        YogVal val = OBJ2VAL(exc); \
        YogError_raise(env, val); \
    } \
} while (0)

#include "yog/array.h"

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
