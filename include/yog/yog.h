#ifndef __YOG_YOG_H__
#define __YOG_YOG_H__

#include <stdlib.h>

enum ValType {
    VAL_INT, 
    VAL_FLOAT, 
    VAL_OBJ, 
    VAL_TRUE, 
    VAL_FALSE, 
    VAL_NIL, 
};

typedef enum ValType ValType;

enum GCObjType {
    OBJ_BUFFER, 
};

struct GCObj {
    GCObjType type;
    void* forwarding_addr;
    size_t size;
};

typedef struct GCObj GCObj;

struct YogVal {
    ValType type;
    union {
        int n;
        double f;
        GCObj* obj;
    } u;
};

#define u_n     u.n
#define u_f     u.f
#define u_obj   u.obj

typedef struct YogVal YogVal;

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
