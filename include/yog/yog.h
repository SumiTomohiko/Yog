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
};

typedef struct YogVm YogVm;

struct YogEnv {
    YogVm* vm;
};

#define ENV_VM(env) ((env)->vm)

typedef struct YogEnv YogEnv;

enum YogValType {
    VAL_INT, 
    VAL_FLOAT, 
    VAL_GCOBJ, 
    VAL_TRUE, 
    VAL_FALSE, 
    VAL_NIL, 
    VAL_SYMBOL, 
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
    GCOBJ_MODULE, 
    GCOBJ_CODE, 
    GCOBJ_BYTE_ARRAY, 
    GCOBJ_BINARY, 
    GCOBJ_FRAME, 
    GCOBJ_THREAD, 
};

typedef enum YogGCObjType YogGCObjType;

struct YogGCObj {
    YogGCObjType type;
    void* forwarding_addr;
    size_t size;
};

#define YOGGCOBJ_HEAD                   YogGCObj gcobj_head
#define YOGGCOBJ(obj)                   ((YogGCObj*)(obj))
#define YOGGCOBJ_FORWARDING_ADDR(obj)   (YOGGCOBJ(obj)->forwarding_addr)
#define YOGGCOBJ_SIZE(obj)              (YOGGCOBJ(obj)->size)

typedef struct YogGCObj YogGCObj;

struct YogVal {
    YogValType type;
    union {
        int n;
        double f;
        ID symbol;
        YogGCObj* gcobj;
    } u;
};

#define YOGVAL_TYPE(v)      ((v).type)
#define YOGVAL_INT(v)       ((v).u.n)
#define YOGVAL_FLOAT(v)     ((v).u.f)
#define YOGVAL_SYMBOL(v)    ((v).u.symbol)
#define YOGVAL_GCOBJ(v)     ((v).u.gcobj)

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
    YogVal items[0];
};

typedef struct YogValArray YogValArray;

struct YogArray {
    YOGGCOBJ_HEAD;
    unsigned int size;
    YogValArray* body; 
};

typedef struct YogArray YogArray;

enum YogNodeType {
    NODE_ASSIGN, 
    NODE_VARIABLE, 
    NODE_LITERAL, 
    NODE_METHOD_CALL, 
    NODE_FUNCTION_CALL, 
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
#define NODE_ARGS(node)     (node)->u3.array

typedef struct YogNode YogNode;

struct YogObj {
    YOGGCOBJ_HEAD;
    YogTable* attrs;
};

#define YOGOBJ_HEAD YogObj  obj_head
#define YOGOBJ(obj) ((YogObj*)obj)

typedef struct YogObj YogObj;

struct YogCharArray {
    YOGGCOBJ_HEAD;
    char items[0];
};

typedef struct YogCharArray YogCharArray;

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

struct YogModule {
    YOGOBJ_HEAD;
    struct YogCode* code;
};

typedef struct YogModule YogModule;

struct YogFrame {
    YOGOBJ_HEAD;
    struct YogCode* code;
    unsigned int pc;
    struct YogTable* pkg_vars;
    struct YogValArray* stack;
};

#define PKG_VARS(f) (f)->pkg_vars

typedef struct YogFrame YogFrame;

struct YogThread {
    YOGOBJ_HEAD;
    struct YogFrame* cur_frame;
};

typedef struct YogThread YogThread;

/* $PROTOTYPE_START$ */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/array.c */
YogVal YogArray_at(YogEnv*, YogArray*, unsigned int);
unsigned int YogArray_size(YogEnv*, YogArray*);
YogValArray* YogValArray_new(YogEnv*, unsigned int);
void YogArray_push(YogEnv*, YogArray*, YogVal);
YogArray* YogArray_new(YogEnv*);

/* src/object.c */
void YogObj_set_attr(YogEnv*, YogObj*, const char*, YogVal);
void YogObj_init(YogEnv*, YogObj*, YogObj*);
YogObj* YogObj_new(YogEnv*);

/* src/binary.c */
YogByteArray* YogByteArray_new(YogEnv*, unsigned int);
void YogBinary_push_uint8(YogEnv*, YogBinary*, uint8_t);
void YogBinary_push_uint32(YogEnv*, YogBinary*, uint32_t);
YogBinary* YogBinary_new(YogEnv*, unsigned int);

/* src/thread.c */
void YogThread_eval_code(YogEnv*, YogThread*, YogCode*);
YogThread* YogThread_new(YogEnv*);

/* src/value.c */
int YogVal_hash(YogEnv*, YogVal);
BOOL YogVal_equals_exact(YogEnv*, YogVal, YogVal);
YogVal YogVal_nil();
YogVal YogVal_gcobj(YogGCObj*);
YogVal YogVal_int(int);
YogVal YogVal_symbol(ID);

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
BOOL YogTable_foreach(YogEnv*, YogTable*, int (*)(YogEnv*, YogVal, YogVal, YogVal), YogVal);
void YogTable_cleanup_safe(YogEnv*, YogTable*, YogVal);
YogTable* YogTable_new_symbol_table(YogEnv*);
YogTable* YogTable_new_string_table(YogEnv*);
BOOL YogTable_lookup_str(YogEnv*, YogTable*, const char*, YogVal*);
YogTable* YogTable_new_val_table(YogEnv*);

/* src/vm.c */
ID YogVm_intern(YogEnv*, YogVm*, const char*);
YogGCObj* YogVm_alloc_gcobj(YogEnv*, YogVm*, YogGCObjType, size_t);
YogVm* YogVm_new(size_t);

/* src/string.c */
YogCharArray* YogCharArray_new(YogEnv*, unsigned int);
YogCharArray* YogCharArray_new_str(YogEnv*, const char*);

/* $PROTOTYPE_END$ */

#define ALLOC_OBJ(env, obj_type, type)  (type*)YogVm_alloc_gcobj(env, ENV_VM(env), obj_type, sizeof(type))
#define ALLOC_OBJ_SIZE(env, obj_type, type, size)   (type*)YogVm_alloc_gcobj(env, ENV_VM(env), obj_type, size)
#define ALLOC_OBJ_ITEM(env, obj_type, type, size, item_type)   ALLOC_OBJ_SIZE(env, obj_type, type, sizeof(type) + size * sizeof(item_type))

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
