#if !defined(__YOG_YOG_H__)
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

#define CHECK_INT(v, msg)   do { \
    if (!IS_INT(v)) { \
        YogError_raise_type_error(env, msg); \
    } \
} while (0)

typedef struct YogVal YogVal;

typedef struct YogEnv YogEnv;

typedef void* (*ObjectKeeper)(YogEnv*, void*);
typedef void (*ChildrenKeeper)(YogEnv*, void*, ObjectKeeper);
typedef void (*Finalizer)(YogEnv*, void*);

typedef struct YogBasicObj YogBasicObj;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

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

/* PROTOTYPE_END */

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
