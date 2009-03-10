#ifndef __YOG_BINARY_H__
#define __YOG_BINARY_H__

#include "yog/yog.h"

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

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/binary.c */
YogVal YogBinary_new(YogEnv*, unsigned int);
void YogBinary_push_id(YogEnv*, YogVal, ID);
void YogBinary_push_pc(YogEnv*, YogVal, pc_t);
void YogBinary_push_uint8(YogEnv*, YogVal, uint8_t);
void YogBinary_push_unsigned_int(YogEnv*, YogVal, unsigned int);
void YogBinary_shrink(YogEnv*, YogVal);
unsigned int YogBinary_size(YogEnv*, YogVal);
uint8_t YogByteArray_at(YogEnv*, YogByteArray*, unsigned int);
YogByteArray* YogByteArray_new(YogEnv*, unsigned int);
void YogByteArray_print(YogEnv*, YogByteArray*);
unsigned int YogByteArray_size(YogEnv*, YogByteArray*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
