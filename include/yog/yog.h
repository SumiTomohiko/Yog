#ifndef __YOG_YOG_H__
#define __YOG_YOG_H__

#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
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

    struct YogKlass* obj_klass;
    struct YogKlass* klass_klass;
    struct YogKlass* func_klass;
    struct YogKlass* int_klass;
    struct YogKlass* pkg_klass;
    struct YogKlass* bool_klass;
    struct YogKlass* builtin_bound_method_klass;
    struct YogKlass* bound_method_klass;
    struct YogKlass* builtin_unbound_method_klass;
    struct YogKlass* unbound_method_klass;

    struct YogTable* pkgs;
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

#define YOGVAL_TYPE(v)      ((v).type)
#define YOGVAL_INT(v)       ((v).u.n)
#define YOGVAL_FLOAT(v)     ((v).u.f)
#define YOGVAL_SYMBOL(v)    ((v).u.symbol)
#define YOGVAL_PTR(v)       ((v).u.ptr)
#define YOGVAL_BOOL(v)      ((v).u.b)
#define YOGVAL_OBJ(v)       ((v).u.obj)

#define IS_UNDEF(v)     (YOGVAL_TYPE(v) == VAL_UNDEF)
#define IS_PTR(v)       (YOGVAL_TYPE(v) == VAL_PTR)
#define IS_OBJ(v)       (YOGVAL_TYPE(v) == VAL_OBJ)
#define IS_INT(v)       (YOGVAL_TYPE(v) == VAL_INT)
#define IS_FLOAT(v)     (YOGVAL_TYPE(v) == VAL_FLOAT)
#define IS_BOOL(v)      (YOGVAL_TYPE(v) == VAL_BOOL)
#define IS_NIL(v)       (YOGVAL_TYPE(v) == VAL_NIL)
#define IS_SYMBOL(v)    (YOGVAL_TYPE(v) == VAL_SYMBOL)

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
    struct YogKlass* klass;
};

#define YOGBASICOBJ_HEAD    struct YogBasicObj base
#define YOGBASICOBJ(obj)    ((YogBasicObj*)obj)

typedef struct YogBasicObj YogBasicObj;

struct YogValArray {
    unsigned int size;
    unsigned int capacity;
    YogVal items[0];
};

typedef struct YogValArray YogValArray;

struct YogArray {
    YOGBASICOBJ_HEAD;
    YogValArray* body; 
};

typedef struct YogArray YogArray;

enum YogNodeType {
    NODE_ASSIGN, 
    NODE_BLOCK_ARG, 
    NODE_BLOCK_PARAM, 
    NODE_BREAK, 
    NODE_COMMAND_CALL, 
    NODE_EXCEPT, 
    NODE_EXCEPT_BODY, 
    NODE_FINALLY, 
    NODE_FUNC_CALL, 
    NODE_FUNC_DEF, 
    NODE_IF, 
    NODE_KW_PARAM, 
    NODE_LITERAL, 
    NODE_METHOD_CALL, 
    NODE_NEXT, 
    NODE_PARAM, 
    NODE_VARIABLE, 
    NODE_VAR_PARAM, 
    NODE_WHILE, 
};

typedef enum YogNodeType YogNodeType;

struct YogNode {
    YogNodeType type;
    unsigned int lineno;
    union {
        ID id;
        YogVal val;
        struct YogNode* nd;
        struct YogArray* array;
    } u1;
    union {
        ID id;
        struct YogNode* nd;
        struct YogArray* array;
    } u2;
    union {
        struct YogArray* array;
    } u3;
    union {
        struct YogNode* nd;
        struct YogArray* array;
    } u4;
};

#define NODE_LEFT(node)     (node)->u1.id
#define NODE_RIGHT(node)    (node)->u2.nd

#define NODE_ID(node)       (node)->u1.id

#define NODE_VAL(node)      (node)->u1.val

#define NODE_RECEIVER(node) (node)->u1.nd
#define NODE_METHOD(node)   (node)->u2.id
#define NODE_COMMAND(node)  (node)->u2.id
#define NODE_CALLEE(node)   (node)->u1.nd
#define NODE_ARGS(node)     (node)->u3.array
#define NODE_BLOCK(node)    (node)->u4.nd

#define NODE_NAME(node)     (node)->u1.id
#define NODE_PARAMS(node)   (node)->u2.array
#define NODE_STMTS(node)    (node)->u3.array
#define NODE_DEFAULT(node)  (node)->u2.nd

#define NODE_HEAD(node)     (node)->u1.array
#define NODE_BODY(node)     (node)->u3.array
#define NODE_EXCEPTS(node)  (node)->u2.array
#define NODE_ELSE(node)     (node)->u3.array
#define NODE_EXC_TYPE(node) (node)->u1.nd
#define NODE_EXC_VAR(node)  (node)->u2.id
#define NO_EXC_VAR          (UINT_MAX)

#define NODE_TEST(node)     (node)->u1.nd

#define NODE_EXPR(node)     (node)->u1.nd

#define NODE_IF_TEST(node)  (node)->u1.nd
#define NODE_IF_STMTS(node) (node)->u2.array
#define NODE_IF_TAIL(node)  (node)->u3.array

typedef struct YogNode YogNode;

struct YogObj {
    YOGBASICOBJ_HEAD;
    YogTable* attrs;
};

#define YOGOBJ_HEAD struct YogObj base
#define YOGOBJ(obj) ((YogObj*)obj)

typedef struct YogObj YogObj;

#define YogPkg      YogObj
#define pkg_klass   obj_klass

struct YogKlass {
    YOGOBJ_HEAD;
    struct YogKlass* super;
};

typedef struct YogKlass YogKlass;

struct YogArgInfo {
    unsigned int argc;
    ID* argnames;
    unsigned int blockargc;
    ID blockargname;
    unsigned int varargc;
    unsigned int kwargc;
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
    unsigned int capacity;
    uint8_t items[0];
};

typedef struct YogByteArray YogByteArray;

struct YogBinary {
    YOGBASICOBJ_HEAD;
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

enum YogFrameType {
    FT_C, 
    FT_PKG, 
    FT_METHOD, 
};

typedef enum YogFrameType YogFrameType;

struct YogFrame {
    enum YogFrameType type;
};

#define FRAME(f)    ((YogFrame*)(f))

typedef struct YogFrame YogFrame;

#define YogCFrame   YogFrame

struct YogScriptFrame {
    struct YogFrame base;
    pc_t pc;
    struct YogCode* code;
    struct YogValArray* stack;
};

#define SCRIPT_FRAME(f)     ((YogScriptFrame*)f)

typedef struct YogScriptFrame YogScriptFrame;

struct YogPkgFrame {
    struct YogScriptFrame base;
    struct YogTable* vars;
    struct YogPkg* pkg;
};

#define PKG_FRAME(f)    ((YogPkgFrame*)f)
#define PKG_VARS(f)     (PKG_FRAME(f)->vars)

typedef struct YogPkgFrame YogPkgFrame;

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

/* $PROTOTYPE_START$ */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/array.c */
void YogValArray_push(YogEnv*, YogValArray*, YogVal);
YogVal YogValArray_pop(YogEnv*, YogValArray*);
YogVal YogValArray_at(YogEnv*, YogValArray*, unsigned int);
YogVal YogArray_at(YogEnv*, YogArray*, unsigned int);
unsigned int YogValArray_size(YogEnv*, YogValArray*);
unsigned int YogArray_size(YogEnv*, YogArray*);
YogValArray* YogValArray_new(YogEnv*, unsigned int);
void YogArray_push(YogEnv*, YogArray*, YogVal);
void YogArray_extend(YogEnv*, YogArray*, YogArray*);
YogArray* YogArray_new(YogEnv*);

/* src/binary.c */
unsigned int YogByteArray_size(YogEnv*, YogByteArray*);
uint8_t YogByteArray_at(YogEnv*, YogByteArray*, unsigned int);
void YogByteArray_print(YogEnv*, YogByteArray*);
YogByteArray* YogByteArray_new(YogEnv*, unsigned int);
void YogBinary_push_uint8(YogEnv*, YogBinary*, uint8_t);
void YogBinary_push_id(YogEnv*, YogBinary*, ID);
void YogBinary_push_unsigned_int(YogEnv*, YogBinary*, unsigned int);
void YogBinary_push_pc(YogEnv*, YogBinary*, pc_t);
YogBinary* YogBinary_new(YogEnv*, unsigned int);

/* src/bool.c */
YogKlass* YogBool_klass_new(YogEnv*);

/* src/builtins.c */
YogPkg* Yog_bltins_new(YogEnv*);

/* src/code.c */
void YogCode_dump(YogEnv*, YogCode*);
YogCode* YogCode_new(YogEnv*);

/* src/compile.c */
YogCode* Yog_compile_module(YogEnv*, YogArray*);

/* src/error.c */
void Yog_assert(YogEnv*, BOOL, const char*);

/* src/frame.c */
YogPkgFrame* YogPkgFrame_new(YogEnv*);
YogMethodFrame* YogMethodFrame_new(YogEnv*);
YogCFrame* YogCFrame_new(YogEnv*);

/* src/function.c */
YogBuiltinFunction* YogBuiltinFunction_new(YogEnv*, const char*, void*, unsigned int, unsigned int, unsigned int, int, va_list);

/* src/inst.c */
unsigned int Yog_get_inst_size(OpCode);

/* src/int.c */
YogKlass* YogInt_klass_new(YogEnv*);

/* src/klass.c */
void YogKlass_define_method(YogEnv*, YogKlass*, const char*, void*, unsigned int, unsigned int, unsigned int, int, ...);
YogKlass* YogKlass_new(YogEnv*, YogKlass*);

/* src/lexer.l */
void Yog_reset_lineno();
unsigned int Yog_get_lineno();

/* src/method.c */
YogKlass* YogBuiltinBoundMethod_klass_new(YogEnv*);
YogKlass* YogBoundMethod_klass_new(YogEnv*);
YogKlass* YogBuiltinUnboundMethod_klass_new(YogEnv*);
YogKlass* YogUnboundMethod_klass_new(YogEnv*);
YogBuiltinBoundMethod* YogBuiltinBoundMethod_new(YogEnv*);
YogBoundMethod* YogBoundMethod_new(YogEnv*);
YogBuiltinUnboundMethod* YogBuiltinUnboundMethod_new(YogEnv*);
YogUnboundMethod* YogUnboundMethod_new(YogEnv*);

/* src/object.c */
YogVal YogObj_get_attr(YogEnv*, YogObj*, ID);
void YogObj_set_attr(YogEnv*, YogObj*, const char*, YogVal);
void YogBasicObj_init(YogEnv*, YogBasicObj*, YogKlass*);
void YogObj_init(YogEnv*, YogObj*, YogKlass*);
void YogObj_gc_children(YogEnv*, void*, DoGc);
YogObj* YogObj_new(YogEnv*, YogKlass*);

/* src/package.c */
void YogPkg_define_method(YogEnv*, YogPkg*, const char*, void*, unsigned int, unsigned int, unsigned int, unsigned int, ...);
YogKlass* YogPkg_klass_new(YogEnv*);
YogPkg* YogPkg_new(YogEnv*);

/* src/parser.y */
void Yog_set_parsing_env(YogEnv*);
YogEnv* Yog_get_parsing_env();
YogArray* Yog_get_parsed_tree();

/* src/st.c */
BOOL YogTable_lookup(YogEnv*, YogTable*, YogVal, YogVal*);
BOOL YogTable_insert(YogEnv*, YogTable*, YogVal, YogVal);
void YogTable_add_direct(YogEnv*, YogTable*, YogVal, YogVal);
YogTable* YogTable_copy(YogEnv*, YogTable*);
BOOL YogTable_delete(YogEnv*, YogTable*, YogVal*, YogVal*);
BOOL YogTable_delete_safe(YogEnv*, YogTable*, YogVal*, YogVal*, YogVal);
BOOL YogTable_foreach(YogEnv*, YogTable*, int (*)(YogEnv*, YogVal, YogVal, YogVal*), YogVal*);
void YogTable_cleanup_safe(YogEnv*, YogTable*, YogVal*);
YogTable* YogTable_new_symbol_table(YogEnv*);
YogTable* YogTable_new_string_table(YogEnv*);
BOOL YogTable_lookup_str(YogEnv*, YogTable*, const char*, YogVal*);
YogTable* YogTable_new_val_table(YogEnv*);
int YogTable_size(YogEnv*, YogTable*);

/* src/string.c */
YogCharArray* YogCharArray_new(YogEnv*, unsigned int);
YogCharArray* YogCharArray_new_str(YogEnv*, const char*);
YogString* YogString_new_str(YogEnv*, const char*);
YogString* YogString_new_format(YogEnv*, const char*, ...);

/* src/thread.c */
YogVal YogThread_call_method(YogEnv*, YogThread*, YogVal, const char*, unsigned int, YogVal*);
YogVal YogThread_call_method_id(YogEnv*, YogThread*, YogVal, ID, unsigned int, YogVal*);
void YogThread_call_command(YogEnv*, ID, unsigned int, YogVal*);
void YogThread_eval_package(YogEnv*, YogThread*, YogPkg*, YogCode*);
YogThread* YogThread_new(YogEnv*);

/* src/value.c */
void YogVal_print(YogEnv*, YogVal);
int YogVal_hash(YogEnv*, YogVal);
BOOL YogVal_equals_exact(YogEnv*, YogVal, YogVal);
YogVal YogVal_true();
YogVal YogVal_false();
YogVal YogVal_nil();
YogVal YogVal_undef();
YogVal YogVal_obj(YogBasicObj*);
YogVal YogVal_ptr(void *);
YogVal YogVal_int(int);
YogVal YogVal_symbol(ID);
YogKlass* YogVal_get_klass(YogEnv*, YogVal);
YogVal YogVal_get_attr(YogEnv*, YogVal, ID);

/* src/vm.c */
const char* YogVm_id2name(YogEnv*, YogVm*, ID);
ID YogVm_intern(YogEnv*, YogVm*, const char*);
void* YogVm_alloc(YogEnv*, GcChildren, size_t);
void YogVm_boot(YogEnv*, YogVm*);
YogVm* YogVm_new(size_t);

/* $PROTOTYPE_END$ */

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

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
