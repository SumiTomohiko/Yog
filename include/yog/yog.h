#ifndef __YOG_YOG_H__
#define __YOG_YOG_H__

#include <stdint.h>
#include <stdlib.h>

#define BOOL    int
#define FALSE   (0)
#define TRUE    (!FALSE)

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

    struct YogTable* pkgs;
};

typedef struct YogVm YogVm;

struct YogEnv {
    YogVm* vm;
};

#define ENV_VM(env) ((env)->vm)

typedef struct YogEnv YogEnv;

enum YogValType {
    VAL_UNDEF, 
    VAL_INT, 
    VAL_FLOAT, 
    VAL_GCOBJ, 
    VAL_TRUE, 
    VAL_FALSE, 
    VAL_NIL, 
    VAL_SYMBOL, 
    VAL_FUNC, 
};

typedef enum YogValType YogValType;

enum YogGCObjType {
    GCOBJ_TABLE, 
    GCOBJ_TABLE_ENTRY, 
    GCOBJ_TABLE_ENTRY_ARRAY, 
    GCOBJ_ARRAY, 
    GCOBJ_VAL_ARRAY, 
    GCOBJ_NODE, 
    GCOBJ_OBJ, 
    GCOBJ_CHAR_ARRAY, 
    GCOBJ_STRING, 
    GCOBJ_CODE, 
    GCOBJ_BYTE_ARRAY, 
    GCOBJ_BINARY, 
    GCOBJ_FRAME, 
    GCOBJ_THREAD, 
#if 0
    GCOBJ_FUNC, 
#endif
    GCOBJ_KLASS, 
    GCOBJ_INST, 
};

typedef enum YogGCObjType YogGCObjType;

struct YogGCObj {
    YogGCObjType type;
    void* forwarding_addr;
    size_t size;
};

#define YOGGCOBJ_HEAD                   struct YogGCObj base
#define YOGGCOBJ(obj)                   ((YogGCObj*)(obj))
#define YOGGCOBJ_FORWARDING_ADDR(obj)   (YOGGCOBJ(obj)->forwarding_addr)
#define YOGGCOBJ_SIZE(obj)              (YOGGCOBJ(obj)->size)

typedef struct YogGCObj YogGCObj;

typedef struct YogVal (*YogFuncBody)(struct YogEnv*, struct YogVal, int, struct YogVal*);

struct YogVal {
    YogValType type;
    union {
        int n;
        double f;
        ID symbol;
        YogGCObj* gcobj;
        YogFuncBody func;
    } u;
};

#define YOGVAL_TYPE(v)      ((v).type)
#define YOGVAL_INT(v)       ((v).u.n)
#define YOGVAL_FLOAT(v)     ((v).u.f)
#define YOGVAL_SYMBOL(v)    ((v).u.symbol)
#define YOGVAL_GCOBJ(v)     ((v).u.gcobj)
#define YOGVAL_FUNC(v)      ((v).u.func)

typedef struct YogVal YogVal;

struct YogHashType {
    int (*compare)(YogEnv*, YogVal, YogVal);
    int (*hash)(YogEnv*, YogVal);
};

typedef struct YogHashType YogHashType;

struct YogTableEntry {
    YOGGCOBJ_HEAD;
    unsigned int hash;
    YogVal key;
    YogVal record;
    struct YogTableEntry* next;
};

typedef struct YogTableEntry YogTableEntry;

struct YogTableEntryArray {
    YOGGCOBJ_HEAD;
    unsigned int size;
    YogTableEntry* items[0];
};

typedef struct YogTableEntryArray YogTableEntryArray;

struct YogTable {
    YOGGCOBJ_HEAD;
    YogHashType* type;
    int num_bins;
    int num_entries;
    YogTableEntryArray* bins;
};

typedef struct YogTable YogTable;

struct YogValArray {
    YOGGCOBJ_HEAD;
    unsigned int size;
    unsigned int capacity;
    YogVal items[0];
};

typedef struct YogValArray YogValArray;

struct YogArray {
    YOGGCOBJ_HEAD;
    YogValArray* body; 
};

typedef struct YogArray YogArray;

enum YogNodeType {
    NODE_ASSIGN, 
    NODE_VARIABLE, 
    NODE_LITERAL, 
    NODE_METHOD_CALL, 
    NODE_COMMAND_CALL, 
};

typedef enum YogNodeType YogNodeType;

struct YogNode {
    YOGGCOBJ_HEAD;
    YogNodeType type;
    union {
        ID id;
        YogVal val;
        struct YogNode* node;
    } u1;
    union {
        ID id;
        struct YogNode* node;
    } u2;
    union {
        struct YogArray* array;
    } u3;
};

#define NODE_LEFT(node)     (node)->u1.id
#define NODE_RIGHT(node)    (node)->u2.node

#define NODE_ID(node)       (node)->u1.id

#define NODE_VAL(node)      (node)->u1.val

#define NODE_RECEIVER(node) (node)->u1.node
#define NODE_METHOD(node)   (node)->u2.id
#define NODE_COMMAND(node)  (node)->u2.id
#define NODE_ARGS(node)     (node)->u3.array

typedef struct YogNode YogNode;

struct YogBasicObj {
    YOGGCOBJ_HEAD;
    struct YogKlass* klass;
};

#define YOGBASICOBJ_HEAD    struct YogBasicObj base
#define YOGBASICOBJ(obj)    ((YogBasicObj*)obj)

typedef struct YogBasicObj YogBasicObj;

struct YogObj {
    YOGBASICOBJ_HEAD;
    YogTable* attrs;
};

#define YOGOBJ_HEAD struct YogObj base
#define YOGOBJ(obj) ((YogObj*)obj)

typedef struct YogObj YogObj;

struct YogKlass {
    YOGOBJ_HEAD;
    struct YogKlass* super;
};

typedef struct YogKlass YogKlass;

#if 0
struct YogFunc {
    YOGBASICOBJ_HEAD;
    YogFuncBody* body;
};

typedef struct YogFunc YogFunc;
#endif

struct YogCharArray {
    YOGGCOBJ_HEAD;
    unsigned int size;
    unsigned int capacity;
    char items[0];
};

typedef struct YogCharArray YogCharArray;

struct YogString {
    YOGGCOBJ_HEAD;
    struct YogCharArray* body;
};

typedef struct YogString YogString;

struct YogByteArray {
    YOGGCOBJ_HEAD;
    unsigned int size;
    unsigned int capacity;
    uint8_t items[0];
};

typedef struct YogByteArray YogByteArray;

struct YogBinary {
    YOGGCOBJ_HEAD;
    struct YogByteArray* body;
};

typedef struct YogBinary YogBinary;

struct YogCode {
    YOGGCOBJ_HEAD;
    unsigned int stack_size;
    struct YogValArray* consts;
    struct YogByteArray* insts;
};

typedef struct YogCode YogCode;

struct YogFrame {
    YOGGCOBJ_HEAD;
    struct YogTable* pkg_vars;
    struct YogValArray* stack;
};

#define PKG_VARS(f) (f)->pkg_vars

typedef struct YogFrame YogFrame;

struct YogThread {
    YOGGCOBJ_HEAD;
    struct YogFrame* cur_frame;
};

typedef struct YogThread YogThread;

enum InstType {
    INST_DUMMY, 
    INST_OP, 
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
YogArray* YogArray_new(YogEnv*);

/* src/package.c */
YogKlass* YogPkg_klass_new(YogEnv*);

/* src/int.c */
YogKlass* YogInt_klass_new(YogEnv*);

/* src/object.c */
YogVal YogObj_get_attr(YogEnv*, YogObj*, ID);
void YogObj_set_attr(YogEnv*, YogObj*, const char*, YogVal);
void YogObj_define_method(YogEnv*, YogObj*, const char*, YogFuncBody);
void YogBasicObj_init(YogEnv*, YogBasicObj*, YogKlass*);
void YogObj_init(YogEnv*, YogObj*, YogKlass*);
YogObj* YogObj_new(YogEnv*, YogKlass*);

/* src/binary.c */
unsigned int YogByteArray_size(YogEnv*, YogByteArray*);
uint8_t YogByteArray_at(YogEnv*, YogByteArray*, unsigned int);
void YogByteArray_print(YogEnv*, YogByteArray*);
YogByteArray* YogByteArray_new(YogEnv*, unsigned int);
void YogBinary_push_uint8(YogEnv*, YogBinary*, uint8_t);
void YogBinary_push_uint32(YogEnv*, YogBinary*, uint32_t);
YogBinary* YogBinary_new(YogEnv*, unsigned int);

/* src/thread.c */
YogVal YogThread_call_method(YogEnv*, YogVal, const char*, unsigned int, YogVal*);
YogVal YogThread_call_method_id(YogEnv*, YogVal, ID, unsigned int, YogVal*);
void YogThread_call_command(YogEnv*, ID, unsigned int, YogVal*);
void YogThread_eval_code(YogEnv*, YogThread*, YogCode*);
YogThread* YogThread_new(YogEnv*);

/* src/klass.c */
YogKlass* YogKlass_new(YogEnv*, YogKlass*);

/* src/value.c */
void YogVal_print(YogEnv*, YogVal);
int YogVal_hash(YogEnv*, YogVal);
BOOL YogVal_equals_exact(YogEnv*, YogVal, YogVal);
YogVal YogVal_nil();
YogVal YogVal_undef();
YogVal YogVal_gcobj(YogGCObj*);
YogVal YogVal_int(int);
YogVal YogVal_symbol(ID);
YogVal YogVal_func(YogFuncBody);
YogKlass* YogVal_get_klass(YogEnv*, YogVal);
YogVal YogVal_get_attr(YogEnv*, YogVal, ID);

/* src/parser.y */
void Yog_set_parsing_env(YogEnv*);
YogEnv* Yog_get_parsing_env();
YogArray* Yog_get_parsed_tree();

/* src/compile.c */
YogCode* Yog_compile_module(YogEnv*, YogArray*);

/* src/code.c */
YogCode* YogCode_new(YogEnv*);

/* src/error.c */
void Yog_assert(YogEnv*, BOOL, const char*);

/* src/frame.c */
YogFrame* YogFrame_new(YogEnv*);

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

/* src/builtins.c */
YogObj* Yog_bltins_new(YogEnv*);

/* src/vm.c */
ID YogVm_intern(YogEnv*, YogVm*, const char*);
YogGCObj* YogVm_alloc_gcobj(YogEnv*, YogVm*, YogGCObjType, size_t);
void YogVm_boot(YogEnv*, YogVm*);
YogVm* YogVm_new(size_t);

/* src/string.c */
YogCharArray* YogCharArray_new(YogEnv*, unsigned int);
YogCharArray* YogCharArray_new_str(YogEnv*, const char*);
YogString* YogString_new_format(YogEnv*, const char*, ...);
YogString* YogString_new(YogEnv*);

/* $PROTOTYPE_END$ */

#define ALLOC_OBJ(env, gcobj_type, type)  (type*)YogVm_alloc_gcobj(env, ENV_VM(env), gcobj_type, sizeof(type))
#define ALLOC_OBJ_SIZE(env, gcobj_type, type, size)   (type*)YogVm_alloc_gcobj(env, ENV_VM(env), gcobj_type, size)
#define ALLOC_OBJ_ITEM(env, gcobj_type, type, size, item_type)   ALLOC_OBJ_SIZE(env, gcobj_type, type, sizeof(type) + size * sizeof(item_type))

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
