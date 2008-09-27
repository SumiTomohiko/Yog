#ifndef __YOG_YOG_H__
#define __YOG_YOG_H__

#include <stdlib.h>

#define BOOL    int
#define FALSE   (0)
#define TRUE    (!FALSE)

struct Heap {
    size_t size;
    char* base;
    char* free;
    struct Heap* next;
};

typedef struct Heap Heap;

struct YogVm {
    BOOL do_gc;
    Heap* heap;
};

typedef struct YogVm YogVm;

struct YogEnv {
    YogVm* vm;
};

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

#define VAL_TYPE(v) ((v).type)
#define VAL_OBJ(v)  ((v).u.obj)
#define VAL_VM(v)   ((v).u.vm)
#define VAL_ENV(v)  ((v).u.env)

typedef struct YogVal YogVal;

struct YogHashType {
    int (*compare)(YogVal, YogVal, YogVal);
    int (*hash)(YogVal, YogVal);
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

struct YogTable {
    YOGOBJ_HEAD;
    YogHashType* type;
    int num_bins;
    int num_entries;
    YogTableEntry** bins;
};

typedef struct YogTable YogTable;

void Yog_assert(YogEnv*, BOOL, const char*); 
YogVm* YogVm_new(size_t);
YogVal YogVal_nil();
YogVal YogVal_obj(YogObj*);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
