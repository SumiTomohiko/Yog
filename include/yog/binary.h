#if !defined(__YOG_BINARY_H__)
#define __YOG_BINARY_H__

#include <stdint.h>
#include "yog/object.h"
#include "yog/yog.h"

struct YogByteArray {
    uint_t size;
    uint8_t items[0];
};

typedef struct YogByteArray YogByteArray;

struct YogBinary {
    YOGBASICOBJ_HEAD;
    uint_t size;
    YogVal body;
};

typedef struct YogBinary YogBinary;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/binary.c */
YogVal YogBinary_new(YogEnv*, uint_t);
void YogBinary_push_id(YogEnv*, YogVal, ID);
void YogBinary_push_pc(YogEnv*, YogVal, pc_t);
void YogBinary_push_uint8(YogEnv*, YogVal, uint8_t);
void YogBinary_push_unsigned_int(YogEnv*, YogVal, uint_t);
void YogBinary_shrink(YogEnv*, YogVal);
uint_t YogBinary_size(YogEnv*, YogVal);
YogVal YogByteArray_new(YogEnv*, uint_t);
uint_t YogByteArray_size(YogEnv*, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
