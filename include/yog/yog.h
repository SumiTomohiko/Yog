#ifndef __YOG_YOG_H__
#define __YOG_YOG_H__

#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include "oniguruma.h"
#include "yog/opcodes.h"

#define BOOL    int
#define FALSE   (0)
#define TRUE    (!FALSE)

typedef unsigned int pc_t;

struct Heap {
    size_t size;
    unsigned char* base;
    unsigned char* free;
    struct Heap* next;
};

typedef struct Heap Heap;

typedef unsigned int ID;

struct YogVm {
    BOOL need_gc;
    Heap* heap;

    ID next_id;
    struct YogTable* id2name;
    struct YogTable* name2id;

    struct YogKlass* cObject;
    struct YogKlass* cKlass;
    struct YogKlass* cInt;
    struct YogKlass* cString;
    struct YogKlass* cPackage;
    struct YogKlass* cBool;
    struct YogKlass* cBuiltinBoundMethod;
    struct YogKlass* cBoundMethod;
    struct YogKlass* cBuiltinUnboundMethod;
    struct YogKlass* cUnboundMethod;
    struct YogKlass* cPackageBlock;

    struct YogTable* pkgs;

    struct YogTable* encodings;
};

typedef struct YogVm YogVm;

struct YogEnv {
    struct YogVm* vm;
    struct YogThread* th;
};

#define ENV_VM(env) ((env)->vm)
#define ENV_TH(env) ((env)->th)

typedef struct YogEnv YogEnv;

enum YogValType {
    VAL_UNDEF, 
    VAL_INT, 
    VAL_FLOAT, 
    VAL_PTR, 
    VAL_OBJ, 
    VAL_BOOL, 
    VAL_NIL, 
    VAL_SYMBOL, 
};

typedef enum YogValType YogValType;

typedef void* (*DoGc)(YogEnv* env, void* ptr);

typedef void (*GcChildren)(YogEnv* env, void* ptr, DoGc do_gc);

struct GcHead {
    GcChildren gc_children;
    void* forwarding_addr;
    size_t size;
};

typedef struct GcHead GcHead;

struct YogVal {
    enum YogValType type;
    union {
        int n;
        double f;
        ID symbol;
        void * ptr;
        struct YogBasicObj* obj;
        BOOL b;
    } u;
};

#define VAL_TYPE(v)     ((v).type)
#define VAL2INT(v)      ((v).u.n)
#define VAL2FLOAT(v)    ((v).u.f)
#define VAL2ID(v)       ((v).u.symbol)
#define VAL2PTR(v)      ((v).u.ptr)
#define VAL2BOOL(v)     ((v).u.b)
#define VAL2OBJ(v)      ((v).u.obj)

#define IS_UNDEF(v)     (VAL_TYPE(v) == VAL_UNDEF)
#define IS_PTR(v)       (VAL_TYPE(v) == VAL_PTR)
#define IS_OBJ(v)       (VAL_TYPE(v) == VAL_OBJ)
#define IS_INT(v)       (VAL_TYPE(v) == VAL_INT)
#define IS_FLOAT(v)     (VAL_TYPE(v) == VAL_FLOAT)
#define IS_BOOL(v)      (VAL_TYPE(v) == VAL_BOOL)
#define IS_NIL(v)       (VAL_TYPE(v) == VAL_NIL)
#define IS_SYMBOL(v)    (VAL_TYPE(v) == VAL_SYMBOL)

typedef struct YogVal YogVal;

struct YogHashType {
    int (*compare)(YogEnv*, YogVal, YogVal);
    int (*hash)(YogEnv*, YogVal);
};

typedef struct YogHashType YogHashType;

struct YogTableEntry {
    unsigned int hash;
    YogVal key;
    YogVal record;
    struct YogTableEntry* next;
};

typedef struct YogTableEntry YogTableEntry;

struct YogTableEntryArray {
    unsigned int size;
    YogTableEntry* items[0];
};

typedef struct YogTableEntryArray YogTableEntryArray;

struct YogTable {
    YogHashType* type;
    int num_bins;
    int num_entries;
    YogTableEntryArray* bins;
};

typedef struct YogTable YogTable;

struct YogBasicObj {
    unsigned int flags;
    struct YogKlass* klass;
};

#define HAS_ATTRS   (1)

#define YOGBASICOBJ_HEAD    struct YogBasicObj base
#define YOGBASICOBJ(obj)    ((YogBasicObj*)obj)

typedef struct YogBasicObj YogBasicObj;

struct YogValArray {
    unsigned int size;
    YogVal items[0];
};

typedef struct YogValArray YogValArray;

struct YogArray {
    YOGBASICOBJ_HEAD;
    unsigned int size;
    YogValArray* body; 
};

typedef struct YogArray YogArray;

struct YogObj {
    YOGBASICOBJ_HEAD;
    YogTable* attrs;
};

#define YOGOBJ_HEAD struct YogObj base
#define YOGOBJ(obj) ((YogObj*)obj)

typedef struct YogObj YogObj;

#define YogPackage          YogObj
#define YogPackage_init     YogObj_init

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

struct YogCharArray {
    unsigned int size;
    unsigned int capacity;
    char items[0];
};

typedef struct YogCharArray YogCharArray;

struct YogString {
    YOGBASICOBJ_HEAD;
    struct YogCharArray* body;
};

typedef struct YogString YogString;

struct YogByteArray {
    unsigned int size;
    uint8_t items[0];
};

typedef struct YogByteArray YogByteArray;

struct YogBinary {
    YOGBASICOBJ_HEAD;
    unsigned int size;
    struct YogByteArray* body;
};

typedef struct YogBinary YogBinary;

struct YogExcTblEntry {
    pc_t from;
    pc_t to;
    pc_t target;
};

typedef struct YogExcTblEntry YogExcTblEntry;

struct YogExcTbl {
    struct YogExcTblEntry items[0];
};

typedef struct YogExcTbl YogExcTbl;

struct LinenoTableEntry {
    pc_t pc_from;
    pc_t pc_to;
    unsigned int lineno;
};

typedef struct LinenoTableEntry LinenoTableEntry;

struct YogCode {
    struct YogArgInfo arg_info;

    unsigned int stack_size;
    unsigned int local_vars_count;
    struct YogValArray* consts;
    struct YogByteArray* insts;

    unsigned int exc_tbl_size;
    struct YogExcTbl* exc_tbl;

    unsigned int lineno_tbl_size;
    struct LinenoTableEntry* lineno_tbl;
};

typedef struct YogCode YogCode;

struct YogFrame {
    struct YogFrame* prev;
};

#define FRAME(f)    ((YogFrame*)(f))

typedef struct YogFrame YogFrame;

#define YogCFrame   YogFrame

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

enum InstType {
    INST_ANCHOR, 
    INST_OP, 
    INST_LABEL, 
};

typedef enum InstType InstType;

#include "yog/inst.h"

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

struct YogEncoding {
    OnigEncoding onig_enc;
};

typedef struct YogEncoding YogEncoding;

/* $PROTOTYPE_START$ */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/array.c */
YogVal YogArray_at(YogEnv*, YogArray*, unsigned int);
void YogArray_extend(YogEnv*, YogArray*, YogArray*);
YogArray* YogArray_new(YogEnv*);
void YogArray_push(YogEnv*, YogArray*, YogVal);
unsigned int YogArray_size(YogEnv*, YogArray*);
YogVal YogValArray_at(YogEnv*, YogValArray*, unsigned int);
YogValArray* YogValArray_new(YogEnv*, unsigned int);
unsigned int YogValArray_size(YogEnv*, YogValArray*);

/* src/binary.c */
YogBinary* YogBinary_new(YogEnv*, unsigned int);
void YogBinary_push_id(YogEnv*, YogBinary*, ID);
void YogBinary_push_pc(YogEnv*, YogBinary*, pc_t);
void YogBinary_push_uint8(YogEnv*, YogBinary*, uint8_t);
void YogBinary_push_unsigned_int(YogEnv*, YogBinary*, unsigned int);
void YogBinary_shrink(YogEnv*, YogBinary*);
unsigned int YogBinary_size(YogEnv*, YogBinary*);
uint8_t YogByteArray_at(YogEnv*, YogByteArray*, unsigned int);
YogByteArray* YogByteArray_new(YogEnv*, unsigned int);
void YogByteArray_print(YogEnv*, YogByteArray*);
unsigned int YogByteArray_size(YogEnv*, YogByteArray*);

/* src/block.c */
YogKlass* YogPackageBlock_klass_new(YogEnv*);
YogPackageBlock* YogPackageBlock_new(YogEnv*);

/* src/bool.c */
YogKlass* YogBool_klass_new(YogEnv*);

/* src/builtins.c */
YogPackage* Yog_bltins_new(YogEnv*);

/* src/code.c */
void YogCode_dump(YogEnv*, YogCode*);
YogCode* YogCode_new(YogEnv*);

/* src/compile.c */
YogCode* Yog_compile_module(YogEnv*, YogArray*);

/* src/encoding.c */
YogEncoding* YogEncoding_get_default(YogEnv*);
int YogEncoding_mbc_size(YogEnv*, YogEncoding*, const char*);
YogEncoding* YogEncoding_new(YogEnv*, OnigEncoding);
YogString* YogEncoding_normalize_name(YogEnv*, YogString*);

/* src/error.c */
void Yog_assert(YogEnv*, BOOL, const char*);

/* src/frame.c */
YogCFrame* YogCFrame_new(YogEnv*);
YogMethodFrame* YogMethodFrame_new(YogEnv*);
YogNameFrame* YogNameFrame_new(YogEnv*);
YogVal YogScriptFrame_pop_stack(YogEnv*, YogScriptFrame*);
void YogScriptFrame_push_stack(YogEnv*, YogScriptFrame*, YogVal);

/* src/function.c */
YogBuiltinFunction* YogBuiltinFunction_new(YogEnv*, const char*, void*, unsigned int, unsigned int, unsigned int, int, va_list);

/* src/inst.c */
unsigned int Yog_get_inst_size(OpCode);

/* src/int.c */
YogKlass* YogInt_klass_new(YogEnv*);

/* src/klass.c */
YogBasicObj* YogKlass_allocate(YogEnv*, YogKlass*);
void YogKlass_define_method(YogEnv*, YogKlass*, const char*, void*, unsigned int, unsigned int, unsigned int, int, ...);
void YogKlass_klass_init(YogEnv*, YogKlass*);
YogKlass* YogKlass_new(YogEnv*, Allocator, const char*, YogKlass*);

/* src/method.c */
YogKlass* YogBoundMethod_klass_new(YogEnv*);
YogBoundMethod* YogBoundMethod_new(YogEnv*);
YogKlass* YogBuiltinBoundMethod_klass_new(YogEnv*);
YogBuiltinBoundMethod* YogBuiltinBoundMethod_new(YogEnv*);
YogKlass* YogBuiltinUnboundMethod_klass_new(YogEnv*);
YogBuiltinUnboundMethod* YogBuiltinUnboundMethod_new(YogEnv*);
YogKlass* YogUnboundMethod_klass_new(YogEnv*);
YogUnboundMethod* YogUnboundMethod_new(YogEnv*);

/* src/object.c */
void YogBasicObj_init(YogEnv*, YogBasicObj*, unsigned int, YogKlass*);
YogBasicObj* YogObj_allocate(YogEnv*, YogKlass*);
void YogObj_gc_children(YogEnv*, void*, DoGc);
YogVal YogObj_get_attr(YogEnv*, YogObj*, ID);
void YogObj_init(YogEnv*, YogObj*, unsigned int, YogKlass*);
void YogObj_klass_init(YogEnv*, YogKlass*);
YogObj* YogObj_new(YogEnv*, YogKlass*);
void YogObj_set_attr(YogEnv*, YogObj*, const char*, YogVal);

/* src/package.c */
void YogPackage_define_method(YogEnv*, YogPackage*, const char*, void*, unsigned int, unsigned int, unsigned int, unsigned int, ...);
YogKlass* YogPackage_klass_new(YogEnv*);
YogPackage* YogPackage_new(YogEnv*);

/* src/string.c */
YogCharArray* YogCharArray_new(YogEnv*, unsigned int);
YogCharArray* YogCharArray_new_str(YogEnv*, const char*);
char YogString_at(YogEnv*, YogString*, unsigned int);
void YogString_clear(YogEnv*, YogString*);
YogString* YogString_clone(YogEnv*, YogString*);
ID YogString_intern(YogEnv*, YogString*);
YogKlass* YogString_klass_new(YogEnv*);
YogString* YogString_new(YogEnv*);
YogString* YogString_new_format(YogEnv*, const char*, ...);
YogString* YogString_new_str(YogEnv*, const char*);
void YogString_push(YogEnv*, YogString*, char);
unsigned int YogString_size(YogEnv*, YogString*);

/* src/table.c */
void YogTable_add_direct(YogEnv*, YogTable*, YogVal, YogVal);
void YogTable_cleanup_safe(YogEnv*, YogTable*, YogVal*);
BOOL YogTable_delete(YogEnv*, YogTable*, YogVal*, YogVal*);
BOOL YogTable_delete_safe(YogEnv*, YogTable*, YogVal*, YogVal*, YogVal);
BOOL YogTable_foreach(YogEnv*, YogTable*, int (*)(YogEnv*, YogVal, YogVal, YogVal*), YogVal*);
BOOL YogTable_insert(YogEnv*, YogTable*, YogVal, YogVal);
BOOL YogTable_lookup(YogEnv*, YogTable*, YogVal, YogVal*);
BOOL YogTable_lookup_str(YogEnv*, YogTable*, const char*, YogVal*);
YogTable* YogTable_new_string_table(YogEnv*);
YogTable* YogTable_new_symbol_table(YogEnv*);
YogTable* YogTable_new_val_table(YogEnv*);
int YogTable_size(YogEnv*, YogTable*);

/* src/thread.c */
YogVal YogThread_call_block(YogEnv*, YogThread*, YogVal, unsigned int, YogVal*);
YogVal YogThread_call_method(YogEnv*, YogThread*, YogVal, const char*, unsigned int, YogVal*);
YogVal YogThread_call_method_id(YogEnv*, YogThread*, YogVal, ID, unsigned int, YogVal*);
void YogThread_eval_package(YogEnv*, YogThread*, YogPackage*, YogCode*);
YogThread* YogThread_new(YogEnv*);

/* src/value.c */
BOOL YogVal_equals_exact(YogEnv*, YogVal, YogVal);
YogVal YogVal_false();
YogVal YogVal_float(float);
YogVal YogVal_get_attr(YogEnv*, YogVal, ID);
YogKlass* YogVal_get_klass(YogEnv*, YogVal);
int YogVal_hash(YogEnv*, YogVal);
YogVal YogVal_int(int);
YogVal YogVal_nil();
YogVal YogVal_obj(YogBasicObj*);
void YogVal_print(YogEnv*, YogVal);
YogVal YogVal_ptr(void *);
YogVal YogVal_symbol(ID);
YogVal YogVal_true();
YogVal YogVal_undef();

/* src/vm.c */
void* YogVm_alloc(YogEnv*, GcChildren, size_t);
void YogVm_boot(YogEnv*, YogVm*);
const char* YogVm_id2name(YogEnv*, YogVm*, ID);
ID YogVm_intern(YogEnv*, YogVm*, const char*);
YogVm* YogVm_new(size_t);

/* $PROTOTYPE_END$ */

#define YogKlassFrame_new       YogNameFrame_new
#define YogPackageFrame_new     YogNameFrame_new

#define ALLOC_OBJ_SIZE(env, gc_children, size) \
    YogVm_alloc(env, gc_children, size)
#define ALLOC_OBJ(env, gc_children, type) \
    ALLOC_OBJ_SIZE(env, gc_children, sizeof(type))
#define ALLOC_OBJ_ITEM(env, gc_children, type, size, item_type) \
    ALLOC_OBJ_SIZE(env, gc_children, sizeof(type) + size * sizeof(item_type))

#define JMP_RAISE   (1)

#define DO_GC(env, do_gc, obj)  obj = (do_gc)((env), (obj))

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

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
