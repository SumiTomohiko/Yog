#ifndef __YOG_YOG_H__
#define __YOG_YOG_H__

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

struct YogVm {
    BOOL need_gc;
    Heap* heap;
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
    VAL_OBJ, 
    VAL_TRUE, 
    VAL_FALSE, 
    VAL_NIL, 
    VAL_SYMBOL, 
};

typedef enum YogValType YogValType;

enum YogObjType {
    OBJ_BUFFER, 
    OBJ_TABLE, 
    OBJ_TABLE_ENTRY, 
    OBJ_TABLE_ENTRY_ARRAY, 
};

typedef enum YogObjType YogObjType;

struct YogObj {
    YogObjType type;
    void* forwarding_addr;
    size_t size;
};

#define YOGOBJ_HEAD YogObj obj_head
#define YOGOBJ(x)   ((YogObj*)(x))

typedef struct YogObj YogObj;

struct YogVal {
    YogValType type;
    union {
        int n;
        double f;
        unsigned int symbol;
        YogObj* obj;
    } u;
};

#define YOGVAL_TYPE(v)      ((v).type)
#define YOGVAL_NUM(v)       ((v).u.n)
#define YOGVAL_FLOAT(v)     ((v).u.f)
#define YOGVAL_SYMBOL(v)    ((v).u.symbol)
#define YOGVAL_OBJ(v)       ((v).u.obj)

typedef struct YogVal YogVal;

struct YogHashType {
    int (*compare)(YogEnv*, YogVal, YogVal);
    int (*hash)(YogEnv*, YogVal);
};

typedef struct YogHashType YogHashType;

struct YogTableEntry {
    YOGOBJ_HEAD;
    unsigned int hash;
    YogVal key;
    YogVal record;
    struct YogTableEntry* next;
};

typedef struct YogTableEntry YogTableEntry;

struct YogTableEntryArray {
    YOGOBJ_HEAD;
    YogTableEntry* items[1];
};

typedef struct YogTableEntryArray YogTableEntryArray;

struct YogTable {
    YOGOBJ_HEAD;
    YogHashType* type;
    int num_bins;
    int num_entries;
    YogTableEntryArray* bins;
};

typedef struct YogTable YogTable;

void Yog_assert(YogEnv*, BOOL, const char*); 
YogVm* YogVm_new(size_t);
YogVal YogVal_nil();
YogVal YogVal_obj(YogObj*);
YogObj* YogVm_alloc_obj(YogEnv*, YogVm* vm, YogObjType, size_t);
BOOL YogVal_equals_exact(YogEnv*, YogVal, YogVal);
YogTable* YogTable_new_symbol_table(YogEnv*);

#define ALLOC_OBJ(env, obj_type, type)  (type*)YogVm_alloc_obj(env, ENV_VM(env), obj_type, sizeof(type))
#define ALLOC_OBJ_SIZE(env, obj_type, type, size)   (type*)YogVm_alloc_obj(env, ENV_VM(env), obj_type, size)

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
