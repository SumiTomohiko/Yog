#ifndef __YOG_BLOCK_H__
#define __YOG_BLOCK_H__

#include "yog/st.h"
#include "yog/yog.h"

struct YogBasicBlock {
    YOGBASICOBJ_HEAD;
    struct YogVal code;
};

#define BASIC_BLOCK(obj)  ((YogBasicBlock*)obj)

typedef struct YogBasicBlock YogBasicBlock;

struct YogBlock {
    struct YogBasicBlock base;
    struct YogVal locals;
    struct YogVal outer_vars;
    struct YogVal globals;
};

#define BLOCK(obj)  ((YogBlock*)(obj))

typedef struct YogBlock YogBlock;

struct YogPackageBlock {
    struct YogBasicBlock base;
    struct YogVal self;
    struct YogVal vars;
};

#define PACKAGE_BLOCK(obj)  ((YogPackageBlock*)obj)

typedef struct YogPackageBlock YogPackageBlock;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/block.c */
YogVal YogBlock_new(YogEnv*);
YogVal YogPackageBlock_klass_new(YogEnv*);
YogVal YogPackageBlock_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
